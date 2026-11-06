# wsh-mcp — Witchcraft Shell MCP Server

An MCP (Model Context Protocol) server that exposes the
[Witchcraft Compiler Collection](https://github.com/endrazine/wcc) Shell
(wsh) to LLMs. Lets an AI load arbitrary ELF binaries, call their functions,
inspect memory, disassemble code, and perform controlled crash testing — all
through a persistent wsh session.

## What this does

WSH can **libify** any ELF executable — turning it into a callable shared
library at runtime. This MCP server wraps a long-lived wsh process and
exposes its primitives as MCP tools:

- **Load binaries**: `wsh_load_binary` — libify and map any ELF
- **Call functions**: `wsh_call_function` — invoke any function by name or
  address, crash-safe (signal handling via sigsetjmp)
- **Inspect symbols**: `wsh_list_symbols`, `wsh_info` — enumerate everything
  in loaded binaries
- **Disassemble**: `wsh_disasm` — multi-arch disassembly via Capstone
- **Memory ops**: `wsh_hexdump`, `wsh_memory_read`, `wsh_memory_write`,
  `wsh_grep`
- **Safe allocation**: `wsh_xalloc` — buffers with guard pages for overflow
  testing
- **Raw Lua**: `wsh_eval` — execute any WSH/Lua command directly

## Prerequisites

- **wsh** installed and in `$PATH` (or specify path via `--wsh-path`)
- **Python 3.11+**
- **mcp** Python package (`pip install mcp`)

### Installing WCC/WSH

```bash
# From source (see https://github.com/endrazine/wcc)
git clone https://github.com/endrazine/wcc.git
cd wcc
make
sudo make install
```

## Installation

```bash
# Clone or copy this directory
cd wsh-mcp
pip install mcp

# Verify wsh is available
wsh --version
```

## Usage

### Claude Code (CLI)

```bash
# Add as an MCP server
claude mcp add wsh-mcp -- python3 /path/to/wsh-mcp/wsh_mcp_server.py

# With a custom wsh path
claude mcp add wsh-mcp -- python3 /path/to/wsh-mcp/wsh_mcp_server.py --wsh-path /opt/wcc/bin/wsh

# With a binary pre-loaded on startup
claude mcp add wsh-mcp -- env WSH_ARGS="/usr/lib/x86_64-linux-gnu/libssl.so.3" python3 /path/to/wsh-mcp/wsh_mcp_server.py
```

### Claude Desktop

Add to `~/.config/claude/claude_desktop_config.json` (Linux) or
`~/Library/Application Support/Claude/claude_desktop_config.json` (macOS):

```json
{
  "mcpServers": {
    "wsh-mcp": {
      "command": "python3",
      "args": ["/path/to/wsh-mcp/wsh_mcp_server.py"],
      "env": {
        "WSH_PATH": "/usr/local/bin/wsh"
      }
    }
  }
}
```

### Streamable HTTP (remote / multi-client)

```bash
python3 wsh_mcp_server.py --transport streamable-http --port 8080
```

Then point your MCP client at `http://localhost:8080/mcp`.

### SSE transport (legacy)

```bash
python3 wsh_mcp_server.py --transport sse --port 8080
```

## Configuration

| Environment variable | Default | Description |
|---------------------|---------|-------------|
| `WSH_PATH` | `wsh` | Path to the wsh binary |
| `WSH_ARGS` | (empty) | Space-separated args to pass to wsh on startup |

You can also use command-line flags:

```
--wsh-path PATH    Path to wsh (default: wsh)
--wsh-args ARGS    Extra args for wsh (space-separated)
--transport TYPE   stdio|sse|streamable-http (default: stdio)
--port PORT        Port for HTTP transports (default: 8080)
```

## Architecture

```
┌─────────────┐     stdio/JSON-RPC      ┌──────────────┐
│   LLM       │◄────────────────────────►│  wsh-mcp     │
│  (Claude)   │     MCP protocol         │  server.py   │
└─────────────┘                          └──────┬───────┘
                                                │ stdin/stdout
                                                │ (Lua commands)
                                         ┌──────▼───────┐
                                         │     wsh      │
                                         │  subprocess  │
                                         │              │
                                         │ ┌──────────┐ │
                                         │ │ libified │ │
                                         │ │ binaries │ │
                                         │ └──────────┘ │
                                         └──────────────┘
```

The server spawns a single long-lived wsh process. Each MCP tool call
translates to Lua code piped to wsh's stdin, with sentinel markers to
delimit output. WSH's non-interactive mode (`!isatty(stdin)`) handles
this cleanly — it accumulates multiline Lua, executes complete statements,
and prints results.

If wsh crashes (unlikely — libcall handles signals), the server restarts
it automatically on the next tool call.

## Example session

```
User: "Analyze /usr/bin/id for me"

LLM uses:
  1. wsh_load_binary("/usr/bin/id")
  2. wsh_list_symbols(kind="functions")
     → sees: main, usage, print_user, print_group, ...
  3. wsh_disasm("main", 50)
     → reads the disassembly
  4. wsh_call_function("main", [1, argv_ptr])
     → gets output from /usr/bin/id
  5. Reports findings
```

## Security considerations

WSH executes real code in the server's process. The MCP server should run
in a sandboxed environment (container, VM) when analyzing untrusted binaries.
The LLM can call arbitrary functions and read/write arbitrary memory within
the wsh process — this is by design for binary analysis, but means the
server should not run with elevated privileges on a production system.

## License

MIT — same as WCC.
