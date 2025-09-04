FastAPI MVP for Programmatic Control API (v1)

Overview
- Local-only FastAPI server that implements the v1 API described in `prds/programmatic-control-api-prd.md`.
- In-memory controller models windows, layout, menu commands, properties, workspace, screenshots, and pattern mode.
- WebSocket event stream broadcasts state changes for reactive clients (humans or LLM tools).
- Designed for a clean swap to an IPC or in-process adapter that talks to the Turbo Vision app.

Structure
- `tools/api_server/main.py`: FastAPI app, routes, and WebSocket endpoint.
- `tools/api_server/controller.py`: In-memory controller; replace with C++ IPC/in-process bridge later.
- `tools/api_server/models.py`: Domain entities (Window, Rect, AppState, WindowType).
- `tools/api_server/schemas.py`: Pydantic request/response models (create, move, props, state, etc.).
- `tools/api_server/events.py`: Broadcast hub for `/ws` clients.
- `tools/api_server/requirements.txt`: Python dependencies.
- `tools/api_server/README.md`: This guide.

Install and Run
- Python: 3.9+ (3.11 recommended). The code is 3.9-compatible.
- Create venv (from repo root): `python3 -m venv .venv`
- Activate venv: `source .venv/bin/activate`
- (Optional) Upgrade pip: `python -m pip install --upgrade pip`
- Install deps: `pip install -r tools/api_server/requirements.txt`
- Start server: `python -m tools.api_server` (binds `127.0.0.1:8089`)
- OpenAPI docs: `http://127.0.0.1:8089/docs`
- WebSocket: `ws://127.0.0.1:8089/ws`
- Deactivate venv when finished: `deactivate`

Run the C++ App (for live control)
- Build once (from repo root): `cmake -S . -B build && cmake --build build -j`
- Build test app (from `test-tui`): `mkdir -p build && cd build && cmake .. && cmake --build . -j`
- Run the app: `./test_pattern`
- Verify socket exists (new terminal): `ls -l /tmp/test_pattern_app.sock`

IPC Bridge to the C++ App
- The C++ `test_pattern` app now starts a Unix-domain socket listener at `/tmp/test_pattern_app.sock`.
- This FastAPI server auto-forwards core commands to the live app when the socket is available.
- To change the socket path, set `TV_IPC_SOCK=/custom/path.sock` (in both app and server if changed).

Key Endpoints
- Health: `GET /health` — basic liveness.
- Capabilities: `GET /capabilities` — window types, commands, properties schema.
- State: `GET /state` — `pattern_mode`, `windows`, `last_workspace`, `last_screenshot`, `uptime_sec`.
- Window lifecycle:
  - `POST /windows` — create: `{type, title?, rect?, props?}`
  - `POST /windows/{id}/move` — move/resize: `{x?, y?, w?, h?}`
  - `POST /windows/{id}/focus` — bring to front and focus
  - `POST /windows/{id}/clone` — duplicate window and props
  - `POST /windows/{id}/close` — close a window
  - `POST /windows/cascade` — cascade layout
  - `POST /windows/tile` — tile layout: `{cols?}`
  - `POST /windows/close_all` — close all windows
- Properties: `POST /props/{id}` — update window props: `{props:{...}}`
- Menu commands: `POST /menu/command` — `{command, args?}` (supports `cascade`, `tile`, `close_all`, `save_workspace`, `open_workspace`, `screenshot`)
- Workspace: `POST /workspace/save|open` — `{path}`
- Screenshot: `POST /screenshot` — `{path?}` returns `{path}`
- Pattern mode: `POST /pattern_mode` — `{mode:"continuous"|"tiled"}`
- WebSocket events: `GET /ws`

WebSocket Events
- `window.created` — `{id, type, title, rect, z, focused, props}`
- `window.updated` — same payload as created
- `window.closed` — `{id}`
- `layout.cascade` — `{}`
- `layout.tile` — `{cols}`
- `command.executed` — `{command, ok, ...}`
- `workspace.saved` — `{path}`
- `workspace.opened` — `{path}`
- `screenshot.saved` — `{path}`
- `pattern.mode` — `{mode}`

Examples (curl)
- Create a test pattern window:
  - `curl -X POST localhost:8089/windows -H 'Content-Type: application/json' -d '{"type":"test_pattern","title":"TP #1","rect":{"x":2,"y":1,"w":40,"h":12}}'`
- Tile all windows (2 columns):
  - `curl -X POST localhost:8089/windows/tile -H 'Content-Type: application/json' -d '{"cols":2}'`
- Update frame player FPS:
  - `curl -X POST localhost:8089/props/<id> -H 'Content-Type: application/json' -d '{"props":{"fps":24}}'`
- Execute a menu command (save workspace):
  - `curl -X POST localhost:8089/menu/command -H 'Content-Type: application/json' -d '{"command":"save_workspace","args":{"path":"workspace.json"}}'`

Design Notes and Next Steps
- Localhost-only, no authentication (v1 scope). API keys and remote binding come in v2.
- The `Controller` is the swap point for a C++ bridge:
  - Option A: In-process HTTP server embedded into the TV app and route directly to UI-thread handlers.
  - Option B: IPC (Unix domain socket / named pipe) where Python speaks JSON-RPC to the app; `Controller` forwards calls and mirrors state.
- Once bridged, endpoints should call into actual menu commands and window factories from `test_pattern_app.cpp` and related modules.
