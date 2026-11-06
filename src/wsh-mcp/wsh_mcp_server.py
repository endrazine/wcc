#!/usr/bin/env python3
"""
WSH MCP Server — Exposes the Witchcraft Compiler Collection Shell (wsh) to LLMs.

Architecture: wraps a long-lived wsh subprocess, piping Lua commands via stdin
and parsing stdout. WSH's non-interactive mode (run_shell with !isatty(stdin))
accumulates multiline Lua, executes complete statements, and prints results —
which is exactly the interface we need.

Author: Jonathan Brossard / endrazine
License: MIT
"""

import asyncio
import json
import logging
import os
import re
import shutil
import signal
import sys
import uuid
from contextlib import asynccontextmanager
from typing import Any

from mcp.server.mcpserver import MCPServer

logger = logging.getLogger("wsh-mcp")

# ---------------------------------------------------------------------------
# Sentinel protocol
# ---------------------------------------------------------------------------
# WSH's non-interactive mode executes each complete Lua statement and prints
# results to stdout.  We wrap every command in sentinel markers so we can
# reliably delimit where one command's output ends and the next begins.
#
# The sentinel is a UUID-tagged line that cannot collide with normal output.

SENTINEL_PREFIX = "__WSH_MCP_"


def _make_sentinel() -> str:
    return f"{SENTINEL_PREFIX}{uuid.uuid4().hex[:12]}"


# ---------------------------------------------------------------------------
# WSH Session — manages a long-lived wsh subprocess
# ---------------------------------------------------------------------------

class WshSession:
    """Manages a persistent wsh subprocess for one MCP session."""

    def __init__(self, wsh_path: str = "wsh", extra_args: list[str] | None = None):
        self.wsh_path = wsh_path
        self.extra_args = extra_args or []
        self.proc: asyncio.subprocess.Process | None = None
        self._lock = asyncio.Lock()
        self._started = False

    async def start(self) -> None:
        """Spawn the wsh subprocess."""
        if self._started and self.proc and self.proc.returncode is None:
            return

        cmd = [self.wsh_path] + self.extra_args
        logger.info("Starting wsh: %s", " ".join(cmd))

        self.proc = await asyncio.create_subprocess_exec(
            *cmd,
            stdin=asyncio.subprocess.PIPE,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE,
            # Don't let wsh's signal handling mess with us
            preexec_fn=lambda: signal.signal(signal.SIGALRM, signal.SIG_DFL),
        )
        self._started = True

        # Drain any startup banner by sending a no-op and reading until sentinel
        await self._exec_raw('print("__WSH_READY__")\n')
        logger.info("wsh subprocess ready (pid=%d)", self.proc.pid)

    async def stop(self) -> None:
        """Terminate the wsh subprocess."""
        if self.proc and self.proc.returncode is None:
            self.proc.stdin.close()
            try:
                await asyncio.wait_for(self.proc.wait(), timeout=3.0)
            except asyncio.TimeoutError:
                self.proc.kill()
                await self.proc.wait()
        self._started = False

    async def _ensure_alive(self) -> None:
        """Restart wsh if it died."""
        if not self._started or self.proc is None or self.proc.returncode is not None:
            logger.warning("wsh process died (rc=%s), restarting...",
                           self.proc.returncode if self.proc else "N/A")
            await self.start()

    async def _exec_raw(self, lua_code: str) -> str:
        """Send raw Lua to wsh stdin and read all stdout until a sentinel.

        Returns the captured output between sentinels.
        """
        sentinel = _make_sentinel()
        # Wrap the user code so output is delimited
        wrapped = (
            f'io.write("{sentinel}_BEGIN\\n")\n'
            f'io.flush()\n'
            f'{lua_code}\n'
            f'io.write("\\n{sentinel}_END\\n")\n'
            f'io.flush()\n'
        )

        self.proc.stdin.write(wrapped.encode())
        await self.proc.stdin.drain()

        # Read lines until we see the end sentinel
        output_lines: list[str] = []
        in_capture = False

        begin_marker = f"{sentinel}_BEGIN"
        end_marker = f"{sentinel}_END"

        while True:
            try:
                line_bytes = await asyncio.wait_for(
                    self.proc.stdout.readline(), timeout=30.0
                )
            except asyncio.TimeoutError:
                logger.error("Timeout reading from wsh (30s)")
                return "[timeout: wsh did not respond within 30 seconds]"

            if not line_bytes:
                # EOF — wsh died
                return "[error: wsh process terminated unexpectedly]"

            line = line_bytes.decode("utf-8", errors="replace").rstrip("\n")

            if line == begin_marker:
                in_capture = True
                continue
            if line == end_marker:
                break
            if in_capture:
                output_lines.append(line)

        return "\n".join(output_lines)

    async def execute(self, lua_code: str) -> str:
        """Execute Lua code in the wsh session. Thread-safe."""
        async with self._lock:
            await self._ensure_alive()
            try:
                result = await self._exec_raw(lua_code)
            except Exception as e:
                logger.exception("Error executing Lua")
                result = f"[error: {e}]"
                # Try to restart for next call
                await self.stop()
            return result


# ---------------------------------------------------------------------------
# Lua code generators — build Lua snippets for each tool
# ---------------------------------------------------------------------------

def _lua_load_binary(path: str) -> str:
    return f'loadbin("{_lua_escape(path)}")'


def _lua_list_symbols(kind: str = "functions", pattern: str = "") -> str:
    """Return Lua that captures symbol listing as text."""
    fn = {"functions": "functions", "symbols": "symbols",
          "objects": "objects", "libs": "libs"}.get(kind, "functions")
    if pattern:
        # Use wsh's built-in grep on the function table
        return f"""
local t = {fn}()
if t then
    for k, v in pairs(t) do
        local name = tostring(k)
        if string.find(name, "{_lua_escape(pattern)}") then
            print(name .. "\\t" .. tostring(v))
        end
    end
end
"""
    return f"{fn}()"


def _lua_call_function(func: str, args: list[str | int]) -> str:
    """Build a crash-safe function invocation.

    func can be a symbol name (string) or a numeric/hex address.
    args are up to 8 integer/pointer arguments.

    Calling convention (verified against wsh.c / elfloader64.c):
      * Every callable symbol loaded by wsh is exposed as a Lua *global
        wrapper* named after the symbol; the wrapper internally does
        `j,k = libcall(reflect_<name>, ...)` and returns (ret, ctx).
        So a named symbol is invoked by calling that wrapper directly —
        NOT by `libcall(<name>, ...)`, which would pass the Lua closure as
        arg0; libcall's lua_tocfunction() returns NULL on a closure and the
        call lands on address 0.
      * A numeric/hex address is passed straight to libcall(), which takes
        the address as arg0.
    Both paths return exactly two values: the return value and a ctx table.
    """
    arg_strs = [str(a) for a in args[:8]]
    args_joined = ", ".join(arg_strs)

    if isinstance(func, int) or (isinstance(func, str) and func.startswith("0x")):
        # Numeric / hex address -> libcall takes the address as arg0.
        call_expr = f"libcall({func}{', ' + args_joined if args_joined else ''})"
        resolve = ""
    else:
        # Symbol name -> call its auto-generated wrapper. Index _G so that
        # mangled / non-identifier names (C++, foo.lto_priv.0) also resolve.
        name = _lua_escape(func)
        resolve = (
            f'local fn = _G["{name}"]\n'
            f'if type(fn) ~= "function" then\n'
            f'    print("[error: no callable wrapper for \'{name}\'. '
            f'Load the binary with wsh_load_binary and confirm the exact '
            f'symbol name with wsh_list_symbols.]")\n'
            f'    return\n'
            f'end\n'
        )
        call_expr = f"fn({args_joined})"

    return f"""{resolve}local ret, ctx = {call_expr}
print("return_value: " .. tostring(ret))
if type(ctx) == "table" then
    if ctx.retval ~= nil then print("retval: " .. tostring(ctx.retval)) end
    if ctx.errno and ctx.errno ~= 0 then
        print("errno: " .. tostring(ctx.errno) .. " (" .. tostring(ctx.errnostr) .. ")")
    end
    if ctx.signal and ctx.signal ~= "" then print("signal: " .. tostring(ctx.signal)) end
    if ctx.faultaddr and ctx.faultaddr ~= 0 then
        print(string.format("faultaddr: 0x%x", ctx.faultaddr))
        print("mode: " .. tostring(ctx.mode))
    end
    if ctx.sicode and ctx.sicode ~= "" then print("sicode: " .. tostring(ctx.sicode)) end
    if ctx.caller then print("caller: " .. tostring(ctx.caller)) end
end
"""


def _lua_info(symbol: str) -> str:
    return f'info("{_lua_escape(symbol)}")'


def _lua_grep(pattern: str, hex_mode: bool = False) -> str:
    if hex_mode:
        return f'grepptr("{_lua_escape(pattern)}")'
    return f'grep("{_lua_escape(pattern)}")'


def _lua_disasm(target: str, count: int = 30) -> str:
    """Disassemble a symbol or address."""
    if target.startswith("0x") or target.isdigit():
        return f"disasm({target}, {count})"
    return f'disasm_sym("{_lua_escape(target)}", {count})'


def _lua_hexdump(address: str, length: int = 256) -> str:
    return f"hexdump({address}, {length})"


def _lua_memory_read(address: str, length: int = 64) -> str:
    # rawmemread(addr, len) returns the bytes as a Lua string (lua_pushlstring).
    return f"""
local data = rawmemread({address}, {length})
if data then
    local hex = ""
    for i = 1, #data do
        hex = hex .. string.format("%02x", string.byte(data, i))
        if i % 16 == 0 then hex = hex .. "\\n"
        elseif i % 4 == 0 then hex = hex .. " "
        end
    end
    print(hex)
else
    print("[error: could not read memory at {address}]")
end
"""


def _lua_memory_write(address: str, hex_data: str) -> str:
    """Write hex bytes to memory via rawmemwrite(addr, data, len).

    rawmemwrite memmoves `len` bytes from the Lua string `data`, so we emit a
    Lua string literal of the exact bytes. Decimal escapes (\\ddd, always 3
    digits) are used: valid across all Lua 5.x, unambiguous (Lua reads at most
    3 digits), and 8-bit clean.
    """
    byte_vals = [int(hex_data[i:i + 2], 16) for i in range(0, len(hex_data), 2)]
    lua_str = "".join(f"\\{b:03d}" for b in byte_vals)
    n = len(byte_vals)
    return (
        f'local n = rawmemwrite({address}, "{lua_str}", {n})\n'
        f'print("wrote " .. tostring(n) .. " bytes to {address}")'
    )


def _lua_xalloc(size: int) -> str:
    return f"""
local ptr = xalloc({size})
print("allocated: " .. string.format("0x%x", ptr))
"""


def _lua_xfree(address: str) -> str:
    return f"xfree({address})"


def _lua_procmap() -> str:
    return "procmap()"


def _lua_help(topic: str = "") -> str:
    if topic:
        return f'man("{_lua_escape(topic)}")'
    return "help()"


def _lua_breakpoint(address: str) -> str:
    return f"breakpoint({address})"


def _lua_headers() -> str:
    return "headers()"


def _lua_entrypoints() -> str:
    return "entrypoints()"


def _lua_phdrs() -> str:
    return "phdrs()"


def _lua_shdrs() -> str:
    return "shdrs()"


def _lua_escape(s: str) -> str:
    """Escape a string for use inside a Lua double-quoted string."""
    return (s.replace("\\", "\\\\")
             .replace('"', '\\"')
             .replace("\n", "\\n")
             .replace("\r", "\\r")
             .replace("\0", "\\0"))


# ---------------------------------------------------------------------------
# MCP Server definition
# ---------------------------------------------------------------------------

# Global session — created during lifespan
_session: WshSession | None = None


@asynccontextmanager
async def lifespan(server: MCPServer):
    """Start and stop the wsh subprocess with the MCP server."""
    global _session

    wsh_path = os.environ.get("WSH_PATH", "wsh")
    extra_args_str = os.environ.get("WSH_ARGS", "")
    extra_args = extra_args_str.split() if extra_args_str else []

    _session = WshSession(wsh_path=wsh_path, extra_args=extra_args)
    await _session.start()

    try:
        yield {}
    finally:
        await _session.stop()


mcp = MCPServer(
    name="wsh-mcp",
    title="WSH — Witchcraft Shell MCP Server",
    version="0.1.0",
    description=(
        "Exposes the Witchcraft Compiler Collection Shell (wsh) to LLMs. "
        "WSH can libify arbitrary ELF binaries (turning executables into callable "
        "shared libraries), load them, call individual functions, execute syscalls, "
        "inspect memory, disassemble code, and more — all via text-based Lua commands. "
        "This MCP server wraps a persistent wsh session, exposing its primitives as tools."
    ),
    instructions=(
        "This server provides binary analysis and code reuse capabilities through WSH. "
        "Start by loading a binary with wsh_load_binary, then explore it with "
        "wsh_list_symbols and wsh_info. Use wsh_call_function to invoke any function "
        "in the loaded binary. Use wsh_eval for any operation not covered by a "
        "specific tool — it accepts raw Lua code that wsh will execute."
    ),
    lifespan=lifespan,
)


# ---- Tools ----

@mcp.tool(
    name="wsh_eval",
    description=(
        "Execute arbitrary Lua code in the WSH session. This is the escape hatch: "
        "any WSH operation not covered by a specific tool can be done here. "
        "WSH exposes all its functions as Lua globals: libcall(), grep(), loadbin(), "
        "symbols(), info(), hexdump(), disasm(), xalloc(), rawmemread(), rawmemwrite(), etc. "
        "Use print() to produce output. Multi-line Lua is supported."
    ),
)
async def wsh_eval(code: str) -> str:
    """Execute raw Lua code in wsh.

    Args:
        code: Lua code to execute. Use print() for output.
    """
    return await _session.execute(code)


@mcp.tool(
    name="wsh_load_binary",
    description=(
        "Load an ELF binary into the WSH address space. WSH will libify it "
        "(convert the executable into a callable shared library) and map it. "
        "After loading, all exported and internal symbols become callable via "
        "wsh_call_function. Shared libraries (.so) are loaded directly. "
        "Executables are libified first. The binary's symbols will appear in "
        "subsequent wsh_list_symbols calls."
    ),
)
async def wsh_load_binary(path: str) -> str:
    """Load a binary into the wsh session.

    Args:
        path: Absolute path to the ELF binary or shared library.
    """
    return await _session.execute(_lua_load_binary(path))


@mcp.tool(
    name="wsh_call_function",
    description=(
        "Call a function by name or address using libcall(). Supports up to 8 "
        "integer/pointer arguments. Returns the function's return value plus an "
        "execution context table: errno/errnostr, any signal (by name), fault "
        "address and access mode (READ/WRITE/EXEC) if it crashed, and the "
        "resolved return value. WSH uses sigsetjmp to survive crashes — if the "
        "called function segfaults, you get the fault info instead of losing the "
        "session. For string arguments, first allocate memory with wsh_xalloc, or "
        "use wsh_eval with balloc/bset/bget (char* arrays) or rawmemwrite."
    ),
)
async def wsh_call_function(
    function: str,
    args: list[int | str] | None = None,
) -> str:
    """Call a function in the loaded binary.

    Args:
        function: Function name (e.g. "main", "MD5_Init") or hex address (e.g. "0x401000").
        args: Up to 8 integer or hex-string arguments. Strings starting with 0x are
              treated as addresses.
    """
    parsed_args: list[str | int] = []
    for a in (args or []):
        if isinstance(a, str) and a.startswith("0x"):
            parsed_args.append(a)
        else:
            parsed_args.append(int(a) if not isinstance(a, int) else a)

    lua = _lua_call_function(function, parsed_args)
    return await _session.execute(lua)


@mcp.tool(
    name="wsh_list_symbols",
    description=(
        "List symbols from all loaded binaries. Returns symbol names and addresses. "
        "Kind can be: 'functions' (default — executable code), 'symbols' (all symbols "
        "including data), 'objects' (data objects), or 'libs' (loaded libraries). "
        "Optionally filter by a substring pattern."
    ),
)
async def wsh_list_symbols(
    kind: str = "functions",
    pattern: str = "",
) -> str:
    """List symbols from loaded binaries.

    Args:
        kind: Type of symbols: 'functions', 'symbols', 'objects', or 'libs'.
        pattern: Optional substring filter.
    """
    return await _session.execute(_lua_list_symbols(kind, pattern))


@mcp.tool(
    name="wsh_info",
    description=(
        "Display detailed information about a symbol or memory address. "
        "Shows the symbol name, type, size, containing library, and address. "
        "For functions, shows the prototype if known."
    ),
)
async def wsh_info(symbol: str) -> str:
    """Get info about a symbol or address.

    Args:
        symbol: Symbol name or hex address (e.g. "printf" or "0x7f1234").
    """
    return await _session.execute(_lua_info(symbol))


@mcp.tool(
    name="wsh_grep",
    description=(
        "Search all mapped memory for a byte pattern. Scans every loaded ELF "
        "section. In default mode, searches for a string. In hex mode (grepptr), "
        "searches for a pointer-sized value. Returns addresses where the pattern "
        "was found."
    ),
)
async def wsh_grep(pattern: str, hex_mode: bool = False) -> str:
    """Search memory for a pattern.

    Args:
        pattern: String to search for, or hex address for pointer search.
        hex_mode: If true, use grepptr() to search for pointer values.
    """
    return await _session.execute(_lua_grep(pattern, hex_mode))


@mcp.tool(
    name="wsh_disasm",
    description=(
        "Disassemble machine code at a symbol or address. Uses Capstone for "
        "multi-architecture support (x86, x86_64, ARM, MIPS, etc.). "
        "For a symbol name, uses disasm_sym() which resolves and disassembles. "
        "For a numeric address, uses disasm() directly."
    ),
)
async def wsh_disasm(target: str, count: int = 30) -> str:
    """Disassemble code.

    Args:
        target: Symbol name (e.g. "main") or hex address (e.g. "0x401000").
        count: Number of instructions to disassemble (default 30).
    """
    return await _session.execute(_lua_disasm(target, count))


@mcp.tool(
    name="wsh_hexdump",
    description=(
        "Hex dump memory at the given address. Shows both hex bytes and ASCII "
        "representation, similar to xxd."
    ),
)
async def wsh_hexdump(address: str, length: int = 256) -> str:
    """Hex dump memory.

    Args:
        address: Hex address (e.g. "0x401000") or numeric address.
        length: Number of bytes to dump (default 256).
    """
    return await _session.execute(_lua_hexdump(address, length))


@mcp.tool(
    name="wsh_memory_read",
    description=(
        "Read raw bytes from a memory address. Returns hex-encoded bytes. "
        "Uses WSH's rawmemread(addr, len), which returns the bytes as a string."
    ),
)
async def wsh_memory_read(address: str, length: int = 64) -> str:
    """Read bytes from memory.

    Args:
        address: Hex address or numeric address to read from.
        length: Number of bytes to read (default 64).
    """
    return await _session.execute(_lua_memory_read(address, length))


@mcp.tool(
    name="wsh_memory_write",
    description=(
        "Write raw bytes to a memory address. Takes hex-encoded data. "
        "Be careful: writing to code sections or unmapped memory will fault. "
        "Use wsh_xalloc to get safe writable memory."
    ),
)
async def wsh_memory_write(address: str, hex_data: str) -> str:
    """Write bytes to memory.

    Args:
        address: Hex address or numeric address to write to.
        hex_data: Hex-encoded bytes (e.g. "deadbeef0041").
    """
    if not re.match(r'^[0-9a-fA-F]*$', hex_data) or len(hex_data) % 2 != 0:
        return "[error: hex_data must be an even-length hex string]"
    return await _session.execute(_lua_memory_write(address, hex_data))


@mcp.tool(
    name="wsh_xalloc",
    description=(
        "Allocate page-aligned memory with guard pages. The allocated region has: "
        "page 1 = usable (read/write), page 2 = usable (read/write), "
        "page 3 = PROT_NONE (guard page). This is designed for controlled overflow "
        "testing — writes past the allocation hit the guard page and fault cleanly. "
        "Memory is initialized with auto-referencing pointers for heap analysis."
    ),
)
async def wsh_xalloc(size: int) -> str:
    """Allocate safe memory with guard pages.

    Args:
        size: Number of bytes to allocate.
    """
    return await _session.execute(_lua_xalloc(size))


@mcp.tool(
    name="wsh_xfree",
    description="Free memory previously allocated with wsh_xalloc.",
)
async def wsh_xfree(address: str) -> str:
    """Free xalloc'd memory.

    Args:
        address: Address returned by wsh_xalloc (hex or numeric).
    """
    return await _session.execute(_lua_xfree(address))


@mcp.tool(
    name="wsh_procmap",
    description=(
        "Show the process memory map. Lists all mapped regions with their "
        "addresses, permissions (r/w/x), and backing file paths. Useful for "
        "understanding what's loaded and where."
    ),
)
async def wsh_procmap() -> str:
    """Show process memory map."""
    return await _session.execute(_lua_procmap())


@mcp.tool(
    name="wsh_help",
    description=(
        "Get help on a WSH function or list all available functions. "
        "With no argument, lists all functions. With a topic, shows the "
        "function's prototype, description, and return values."
    ),
)
async def wsh_help(topic: str = "") -> str:
    """Get WSH help.

    Args:
        topic: Function name to get help for (e.g. "libcall"). Empty for overview.
    """
    return await _session.execute(_lua_help(topic))


@mcp.tool(
    name="wsh_breakpoint",
    description=(
        "Set a breakpoint at an address. When the breakpoint is hit, wsh will "
        "trap into the debugger. Use with tracing functions (sstrace, btrace, "
        "systrace) for dynamic analysis."
    ),
)
async def wsh_breakpoint(address: str) -> str:
    """Set a breakpoint.

    Args:
        address: Hex address or symbol name to break on.
    """
    return await _session.execute(_lua_breakpoint(address))


@mcp.tool(
    name="wsh_headers",
    description="Display ELF headers of all loaded binaries.",
)
async def wsh_headers() -> str:
    """Show ELF headers."""
    return await _session.execute(_lua_headers())


@mcp.tool(
    name="wsh_sections",
    description=(
        "Display ELF section headers (shdrs) or program headers (phdrs) "
        "of loaded binaries."
    ),
)
async def wsh_sections(kind: str = "sections") -> str:
    """Show ELF section or program headers.

    Args:
        kind: 'sections' for section headers, 'segments' for program headers.
    """
    if kind == "segments":
        return await _session.execute(_lua_phdrs())
    return await _session.execute(_lua_shdrs())


@mcp.tool(
    name="wsh_entrypoints",
    description="List entry points of all loaded binaries.",
)
async def wsh_entrypoints() -> str:
    """Show entry points."""
    return await _session.execute(_lua_entrypoints())


# ---- Prompts ----

@mcp.prompt(
    name="analyze_binary",
    description=(
        "Step-by-step workflow for analyzing an unknown ELF binary. "
        "Loads the binary, enumerates symbols, identifies interesting functions, "
        "and provides a structured analysis approach."
    ),
)
async def analyze_binary_prompt(binary_path: str) -> str:
    return f"""You are analyzing the ELF binary at: {binary_path}

Follow this workflow:

1. **Load the binary**
   Call wsh_load_binary with the path.

2. **Survey the binary**
   - Call wsh_headers to see ELF metadata (arch, type, entry point)
   - Call wsh_list_symbols with kind="libs" to see loaded libraries
   - Call wsh_list_symbols with kind="functions" to enumerate all callable functions
   - Call wsh_entrypoints to see entry points

3. **Identify interesting functions**
   Look for: main, init/fini, authentication-related, crypto, network, file I/O,
   parsing functions, and anything with "unsafe" patterns (strcpy, sprintf, gets).

4. **Deep-dive on selected functions**
   For each interesting function:
   - Call wsh_info to get type/size info
   - Call wsh_disasm to read the assembly
   - Call wsh_call_function to test it (start with safe inputs)

5. **Memory analysis** (if needed)
   - Use wsh_xalloc to create controlled buffers
   - Use wsh_hexdump / wsh_memory_read to inspect memory
   - Use wsh_grep to search for strings or patterns
   - Use wsh_procmap to understand the memory layout

6. **Report findings**
   Summarize: binary metadata, symbol count, interesting functions found,
   any vulnerabilities or notable behaviors observed.
"""


@mcp.prompt(
    name="call_library_function",
    description=(
        "Guide for calling a specific function from a shared library, "
        "including argument setup, allocation, and result interpretation."
    ),
)
async def call_library_function_prompt(library: str, function: str) -> str:
    return f"""You need to call {function}() from {library}.

Workflow:

1. **Load the library**
   Call wsh_load_binary with "{library}".

2. **Find the function**
   Call wsh_info("{function}") to confirm it's loaded and get its address.
   If the function name is mangled (C++), use wsh_list_symbols with a partial pattern.

3. **Understand the prototype**
   Call wsh_help("{function}") for WSH's knowledge, or wsh_disasm("{function}")
   to infer the calling convention from the assembly.

4. **Prepare arguments**
   - Integer/pointer args: pass directly to wsh_call_function
   - String args: use wsh_eval to allocate:
     ```
     buf = xalloc(256)
     memcpy(buf, "your string here", 17)
     ```
   - Struct args: use wsh_eval with lua2c() or load_struct_def()
   - char** args: use wsh_eval with balloc/bset:
     ```
     a = balloc(10)
     bset(a, 0, "/path/to/thing")
     bset(a, 1, "--flag")
     ```

5. **Make the call**
   Call wsh_call_function with the function name and prepared args.
   Check the return value AND the execution context (errno, signals).

6. **Interpret results**
   - Read return pointers with wsh_hexdump or wsh_memory_read
   - Convert return structs with ptr2struct via wsh_eval
   - Check errno for error conditions
"""


@mcp.prompt(
    name="vulnerability_research",
    description=(
        "Structured approach for finding and testing memory safety issues "
        "in a binary using WSH's controlled execution environment."
    ),
)
async def vulnerability_research_prompt(binary_path: str) -> str:
    return f"""Analyzing {binary_path} for memory safety issues.

WSH provides a controlled environment: libcall() catches crashes via signal handlers,
xalloc() creates buffers with guard pages, and the full memory map is inspectable.

Workflow:

1. **Load and enumerate**
   - wsh_load_binary("{binary_path}")
   - wsh_list_symbols to find all functions
   - Look for: input parsing, string handling, buffer operations, format strings

2. **Identify attack surface**
   - grep for dangerous patterns: "strcpy", "sprintf", "gets", "memcpy"
   - Look for functions that take user-controlled sizes or strings
   - Check for functions that allocate based on input

3. **Set up controlled inputs**
   Use wsh_xalloc for buffers with guard pages:
   ```
   -- xalloc gives you: [usable page][usable page][PROT_NONE guard]
   -- Overflows hit the guard page and fault cleanly
   buf = xalloc(256)
   ```

4. **Test functions**
   Call suspect functions with:
   - Normal inputs (baseline behavior)
   - Boundary-length inputs
   - Oversized inputs (xalloc guard pages catch overflows)
   - Format string inputs ("%x%x%x%n")

5. **Analyze crashes**
   When libcall() catches a signal:
   - Check ctx.signal — the signal name, e.g. SIGSEGV/SIGBUS/SIGFPE ("" if none)
   - Check ctx.faultaddr — where did it try to access?
   - Use wsh_disasm on the faulting instruction
   - Use wsh_hexdump to inspect the corrupted buffer

6. **Document findings**
   For each issue: function name, input that triggers it, signal received,
   fault address, root cause assessment, exploitability notes.
"""


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    import argparse

    parser = argparse.ArgumentParser(description="WSH MCP Server")
    parser.add_argument("--wsh-path", default="wsh",
                        help="Path to the wsh binary (default: wsh)")
    parser.add_argument("--wsh-args", default="",
                        help="Extra arguments to pass to wsh (space-separated)")
    parser.add_argument("--transport", choices=["stdio", "sse", "streamable-http"],
                        default="stdio", help="MCP transport (default: stdio)")
    parser.add_argument("--port", type=int, default=8080,
                        help="Port for HTTP transports (default: 8080)")
    args = parser.parse_args()

    # Set env vars for the lifespan to pick up
    os.environ["WSH_PATH"] = args.wsh_path
    if args.wsh_args:
        os.environ["WSH_ARGS"] = args.wsh_args

    # MCPServer.run(transport, **kwargs) forwards kwargs to run_sse_async /
    # run_streamable_http_async, which take host/port directly (there is no
    # sse_params / streamable_http_params argument).
    if args.transport == "stdio":
        mcp.run(transport="stdio")
    else:
        mcp.run(transport=args.transport, host="127.0.0.1", port=args.port)


if __name__ == "__main__":
    main()
