# Running Wib&Wob with MCP (3-terminal workflow)

Use this quickstart to build the app, start the MCP/API server, and run the TUI with logs. The setup assumes three terminal windows side-by-side.

## 1) Build the app
```bash
cd /Users/james/Repos/tvision
cmake -S . -B build
cmake --build build -j
```

## 2) Start the API/MCP server
From the repo root:
```bash
# Using uv (preferred)
cd tools/api_server
uv venv
uv pip install -r requirements.txt
cd ../..
uv run python -m tools.api_server.main --port=8089
```
This mounts MCP at `http://127.0.0.1:8089/mcp` and the REST API at `http://127.0.0.1:8089`.

## 3) Run the TUI with logging
In a new terminal:
```bash
cd /Users/james/Repos/tvision
DEBUG_CLAUDE_AGENT_SDK=1 ./build/app/test_pattern 2> /tmp/sdk_debug.log
```
MCP tools are enabled by default (bridge uses `mcp__tui-control__*` tools). Default model is `claude-haiku-4-5`.

## 4) Watch logs
In a third terminal:
```bash
tail -f /tmp/sdk_debug.log
```
Look for tool_use events and IPC errors. API errors (e.g., broken pipe) now return HTTP 502 from the server.

## Notes
- Ensure the API server is running and reachable on 127.0.0.1:8089 before chatting.
- The bridge streams prompts (async) and passes MCP server via `mcpServers: { "tui-control": … }` with allowed tools `mcp__tui-control__...`.
- If the TUI socket dies, API will surface IPC failures as HTTP 502 to the SDK.***
