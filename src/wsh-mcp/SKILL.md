---
name: wsh-binary-analysis
description: >
  Use when the user wants to analyze, test, or interact with ELF binaries
  using the Witchcraft Compiler Collection Shell (wsh). Covers loading
  binaries, calling functions, inspecting memory, disassembly, and
  vulnerability research workflows.
---

# WSH Binary Analysis Skill

You have access to a WSH (Witchcraft Compiler Collection Shell) MCP server
that lets you load arbitrary ELF binaries, call their functions, inspect
memory, disassemble code, and perform controlled crash testing — all
through text-based Lua commands.

## Core concept

WSH solves the **file format problem**: it turns opaque ELF binaries into
callable functions via libify. Your job is the **semantic problem**: deciding
which function to call, with what arguments, and interpreting the results.

WSH is 100% text-based. Every operation produces text output you can reason
about directly.

## Available tools

| Tool | What it does |
|------|-------------|
| `wsh_load_binary` | Load an ELF binary or .so into the address space (libifies executables) |
| `wsh_list_symbols` | Enumerate functions/symbols/objects/libs from loaded binaries |
| `wsh_call_function` | Call any function by name or address (up to 8 args, crash-safe) |
| `wsh_info` | Get detailed info about a symbol (address, type, library) |
| `wsh_disasm` | Disassemble a function or address (Capstone, multi-arch) |
| `wsh_grep` | Search all mapped memory for strings or pointer values |
| `wsh_hexdump` | Hex dump memory at an address |
| `wsh_memory_read` | Read raw bytes (hex-encoded) |
| `wsh_memory_write` | Write raw bytes to an address |
| `wsh_xalloc` | Allocate memory with guard pages (safe overflow testing) |
| `wsh_xfree` | Free xalloc'd memory |
| `wsh_procmap` | Show process memory map |
| `wsh_headers` | Show ELF headers |
| `wsh_sections` | Show section/program headers |
| `wsh_entrypoints` | List binary entry points |
| `wsh_breakpoint` | Set a breakpoint |
| `wsh_help` | Get help on any WSH function |
| `wsh_eval` | Execute arbitrary Lua code (escape hatch for anything not covered) |

## Workflow: analyzing an unknown binary

```
1. Load:        wsh_load_binary("/path/to/binary")
2. Survey:      wsh_headers → wsh_list_symbols(kind="functions")
3. Identify:    wsh_info("interesting_func") → wsh_disasm("interesting_func")
4. Test:        wsh_call_function("func", [arg1, arg2])
5. Inspect:     wsh_hexdump/wsh_memory_read on return values
6. Report:      summarize findings
```

## Workflow: calling a library function

Example — computing an MD5 hash using OpenSSL functions:

```
1. wsh_load_binary("/usr/lib/x86_64-linux-gnu/libcrypto.so")
2. wsh_info("MD5_Init")    -- confirm it's loaded
3. wsh_eval:
     ctx = xalloc(256)
     input_buf = xalloc(256)
     out_buf = xalloc(64)
     memcpy(input_buf, "hello world", 11)
4. wsh_call_function("MD5_Init", [ctx_addr])
5. wsh_call_function("MD5_Update", [ctx_addr, input_addr, 11])
6. wsh_call_function("MD5_Final", [out_addr, ctx_addr])
7. wsh_hexdump(out_addr, 16)    -- read the MD5 digest
```

## Workflow: testing for memory safety issues

WSH's `xalloc` creates buffers with a guard page (PROT_NONE) after the
usable region. Overflows hit the guard page and produce a clean SIGSEGV
that `libcall` catches and reports — without killing the session.

```
1. wsh_load_binary("/path/to/target")
2. wsh_list_symbols(pattern="str")   -- find string-handling functions
3. buf = wsh_xalloc(64)              -- 64 bytes + guard page
4. wsh_eval: memcpy(buf, string.rep("A", 64), 64)  -- fill buffer
5. wsh_call_function("vulnerable_func", [buf])
   → if it overflows, libcall catches SIGSEGV and reports:
     - signal: SIGSEGV
     - faultaddr: address in the guard page
     - the session survives for further testing
```

## Key patterns for wsh_eval

The `wsh_eval` tool is your escape hatch. Common patterns:

### Allocate and populate a char** array (e.g., for main(argc, argv))
```lua
a = balloc(10)            -- allocate char*[10]
bset(a, 0, "/usr/bin/ls") -- argv[0]
bset(a, 1, "--version")   -- argv[1]
libcall(main_addr, 2, a, 0)
```

### Read a string from a return pointer
```lua
-- Call a loaded symbol by name: wsh exposes each symbol as a Lua wrapper
-- global that returns (ret, ctx). Do NOT write libcall(some_func, ...) with
-- a symbol name — libcall expects a C function pointer or address as arg0,
-- and a wrapper closure resolves to NULL (call lands on address 0).
local ret, ctx = some_func(arg1)
local s = rdstr(ret)
print(s)

-- By raw address instead, libcall takes the address directly:
local ret, ctx = libcall(0x401000, arg1)
```

### Define and use a C struct
```lua
local def = load_struct_def({
    {"int", "type"},
    {"int", "size"},
    {"char*", "name"},
})
local s = lua2c(def, {type=1, size=42, name="test"})
-- s is now a userdata with .type, .size, .name fields
-- pass its address to C functions
```

### Search for a string across all loaded binaries
```lua
grep("password")     -- string search
grepptr("0x41414141") -- pointer value search
```

### Enable tracing
```lua
systrace()   -- trace syscalls
btrace()     -- trace branches
sstrace()    -- single-step
```

## Important notes

- **libcall is crash-safe**: it uses sigsetjmp/siglongjmp. If a called
  function crashes, you get the signal info back and can continue working.
  There's a 3-second alarm timeout per call.

- **Arguments are integers/pointers**: libcall passes raw uint64 values.
  For strings, allocate memory first (xalloc or balloc), write the string,
  and pass the address.

- **Return values are integers**: if a function returns a pointer, use
  rdstr() to read a string from it, or hexdump/rawmemread for raw bytes.

- **Calling convention**: `wsh_call_function("name", ...)` invokes the Lua
  wrapper global that wsh generates for each loaded symbol (it wraps
  `libcall(reflect_name, ...)` and returns `ret, ctx`). Pass a hex address
  instead to call `libcall(addr, ...)` directly. The `ctx` table carries
  `errno`/`errnostr`, `signal` (name, "" if none), `faultaddr`, `mode`
  (READ/WRITE/EXEC), `retval`, and `caller`.

- **State persists**: loaded binaries, allocated memory, and breakpoints
  persist across tool calls within a session. You're building up state
  incrementally.

- **Use wsh_help liberally**: `wsh_help("function_name")` gives you the
  prototype, description, and return value documentation for any WSH
  function.

- **Multi-architecture**: WSH supports x86, x86_64, ARM, MIPS for
  disassembly (via Capstone). Use `wsh_eval("arch_list()")` to see
  available architectures, and `wsh_eval('arch_set("arm")')` to switch.
