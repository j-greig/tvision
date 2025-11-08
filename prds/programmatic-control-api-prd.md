# Programmatic Control API PRD — Turbo Vision Test TUI Suite

## TL;DR
- Provide a local API (HTTP + WebSocket) to programmatically control the Test Pattern app (`test_pattern_app.cpp`) and its built-in windows: create/manage windows, invoke menu commands, open files (text/frames), and change properties (e.g., FPS, pattern mode).
- Extensible registry for window types and commands so future modules can participate without changing the API layer.
- Safe UI-thread dispatch: all UI actions are queued to the main Turbo Vision event loop.

---

## Background
This repo contains Turbo Vision–based TUI apps under `test-tui/`, with the API targeting `test_pattern_app.cpp`:
- Test Pattern app (`test_pattern_app.cpp`): spawns unlimited windows showing a 2-row color + grayscale test pattern with global pattern mode (continuous vs tiled). Rich menu: File/Edit/View/Window/Tools/Help. Wallpaper background and screenshot support.
- Gradients (`gradient.{h,cpp}`): horizontal, vertical, radial, diagonal gradient views used in dedicated windows.
- File windows (`TFrameAnimationWindow`): loads a text file; if it has frame delimiters (`----`) it plays frames with a UI timer (optional `FPS=NN` header), otherwise shows a scrollable text view.
- Workspace save/load captures windows, bounds, titles, zoom state, and global pattern mode.

Goal: Expose a world-class, modular API so a human or an LLM can create and manipulate windows, issue menu commands, move/resize them, and load content programmatically.

---

## Objectives
- Full programmatic control of windows and views (create, move, resize, focus, zoom, clone, close, cascade, tile).
- Trigger any menu/command by stable name or id.
- Manipulate content (open text/ANSI/frames; set FPS; change gradient type; toggle pattern mode; paint settings, etc.).
- Observe state and events via a subscription channel.
- (V2) Orchestrate multi-step animations via a declarative Timeline API.
- First-class extensibility: new window types and commands register schemas and factories; API introspects and advertises them.

### Non-Goals (MVP)
- Multi-user concurrency or remote network exposure beyond localhost.
- Pixel-perfect cross-terminal screenshots beyond current macOS helper.
- Advanced ANSI parsing/rendering.

---

## High-Level Architecture
- In-process control server (HTTP + WebSocket) bound to `127.0.0.1`.
- Request handler enqueues UI actions (lambdas) to a thread-safe queue.
- UI thread drains queue via periodic timer tick or custom broadcast command, executing operations safely.
- Window/command registries map API payloads to concrete TVision constructs.

Implementation note: Server framework is flexible (e.g., C++ embedded HTTP server). The PRD presents a framework-agnostic API contract. If a Python FastAPI façade is preferred, it must IPC to the app and still dispatch on the UI thread.

---

## Data Model
### Window
- `id` (string): Stable identifier (e.g., `w1`).
- `type` (string): `test_pattern` | `gradient` | `frame_player` | `text_view` | future.
- `title` (string)
- `bounds` (object): `{ x, y, w, h }` in character cells.
- `zoomed` (bool)
- `focused` (bool)
- `props` (object): Type-specific properties.

### Type-specific props
- `test_pattern`: `{ mode: "continuous" | "tiled" }` (MVP: global mode; per-window later).
- `gradient`: `{ gradientType: "horizontal"|"vertical"|"radial"|"diagonal", startColor?: RGB, endColor?: RGB }`.
- `frame_player`: `{ file: string, fps?: number, playing?: boolean }`.
- `text_view`: `{ file: string }`.

### RGB
- `{ r: 0..255, g: 0..255, b: 0..255 }`.

### Command
- `name` (string): stable dotted path (e.g., `file.save_workspace`).
- `id` (int): Turbo Vision command id (e.g., `cmSaveWorkspace`).
- `group` (string): menu section (File/Edit/View/Window/Tools/Help).
- `title` (string), `shortcut` (string?), `params` (JSON schema, optional).

---

## REST API
Base URL: `http://127.0.0.1:<port>`

Security (V1): Localhost-only, no authentication (dev loop and scripting). Add simple rate limiting on mutation endpoints.

### GET /state
- Returns current state: globals, windows, z-order, focused id, wallpaper, capabilities.
- Response (abridged):
```json
{
  "globals": { "patternMode": "continuous" },
  "windows": [ {"id":"w1","type":"test_pattern","title":"Test Pattern 1","bounds":{"x":2,"y":1,"w":50,"h":15},"zoomed":false,"focused":true,"props":{} } ],
  "focusedId": "w1",
  "capabilities": { "windowTypes": [...], "commands": [...] }
}
```

### POST /windows
- Create a window.
- Body:
```json
{ "type":"gradient", "title":"Gradient", "bounds":{"x":4,"y":2,"w":40,"h":12}, "props": {"gradientType":"vertical"} }
```
- Response: `{ "id": "w3" }`

### POST /windows/{id}/move
- Move/resize (atomic). Body may include `animate`.
```json
{ "x": 20, "y": 5, "w": 60, "h": 18, "animate": {"durationMs": 800, "easing": "easeInOut"} }
```

### POST /windows/{id}/focus | /zoom | /close | /clone
- `clone` body: `{ "offset": {"dx":6, "dy":2}, "title":"Copy of TP" }`

### POST /windows/cascade | /windows/tile | /windows/close_all

### POST /props/{id}
- Update type-specific props. Examples:
  - Frame player: `{ "fps": 24, "playing": true }`
  - Gradient: `{ "gradientType": "radial" }`
  - Test pattern: `{ "mode": "tiled" }`

### POST /menu/command
- Invoke a menu/command by name or id.
```json
{ "name": "file.save_workspace" }
```

### POST /workspace/save
- Optional body: `{ "path": "test-tui/workspaces/session.json" }`.
- Response: `{ "path": ".../session.json" }`

### POST /workspace/open
- Body: `{ "path": "test-tui/workspaces/last_workspace.json" }`.
- Response: `{ "state": { ... as in /state ... } }`

### POST /screenshot
- Body: `{ "fullScreen": false }` (MVP captures the terminal window on macOS using `screencapture`).
- Response: `{ "path": "test-tui/screenshots/tui_YYYYMMDD_HHMMSS.png" }` or PNG bytes if `Accept: image/png`.

### POST /pattern_mode
- Body: `{ "mode": "continuous" }` (MVP global toggle).

### POST /wallpaper
- Body: `{ "mode": "wibwob" | "custom" | "disabled", "path"?: "..." }`.

### POST /frame_player/{id}
- Body: `{ "action": "play"|"pause"|"next"|"prev", "fps"?: 12 }`.

---

## WebSocket Events (GET /ws)
- `window.created` `{ id, type, title, bounds }`
- `window.updated` `{ id, bounds|props|title|zoomed|focused }`
- `window.closed` `{ id }`
- `layout.cascade` / `layout.tile`
- `command.executed` `{ name|id, ok, error? }`
- `workspace.saved` `{ path }` / `workspace.opened` `{ path }`
- `screenshot.saved` `{ path }`
- `animation.tick` `{ id, frameIndex }` (rate-limited)
- `error` `{ message, context }`

---

## Version 2 Enhancements (Roadmap)
- Timeline Orchestration API (ASCII “video” sequences): high-level, declarative sequences to `spawn/clone/move/resize/set_props/command` with easing and scheduling.
- Additional WS events (e.g., `animation.tick` for frame players, rate-limited).
- Authentication: optional `X-API-Key` required for all endpoints; configurable network binding beyond localhost.
- More window types and richer props as they land (e.g., editors, paint). ANSI viewer and advanced ANSI features may be integrated here if desired.

---

## Capabilities and Introspection
### GET /capabilities
- Returns discoverable spec for LLMs and tools:
```json
{
  "windowTypes": [
    { "type": "test_pattern", "schema": {"mode": {"enum": ["continuous","tiled"]}}, "defaults": {"mode":"continuous"} },
    { "type": "gradient", "schema": {"gradientType": {"enum":["horizontal","vertical","radial","diagonal"]}} }
  ],
  "commands": [
    { "name": "file.new_test_pattern", "id": 100, "group": "File", "title": "New Test Pattern", "shortcut": "Ctrl-N" },
    { "name": "file.open_animation", "id": 109, "group": "File", "title": "Open Animation..." },
    { "name": "file.save_workspace", "id": 110, "group": "File", "title": "Save Workspace", "shortcut": "Ctrl-S" },
    { "name": "edit.screenshot", "id": 101, "group": "Edit", "title": "Screenshot", "shortcut": "Ctrl-P" },
    { "name": "view.cascade", "id": 0, "group": "Window", "title": "Cascade" }
  ]
}
```

### GET /help/{name}
- Returns human-readable description and examples for a window type or command.

---

## Error Handling
- Standard HTTP error codes with JSON body `{ "error": "message", "details"?: any }`.
- Event `error` broadcast for asynchronous failures.
- Validation errors include field paths and expected values.

---

## Security & Operations (V1)
- Bind to `127.0.0.1` by default; configurable port.
- No authentication (local dev/scripting). V2 will introduce API keys.
- Rate-limit high-frequency mutations.
- Structured logs for requests and UI actions.

---

## Extensibility
- Window Type Registry: modules register `{ typeName, factory, schema, defaults }` so `/windows` can construct instances; `/capabilities` lists them.
- Command Registry: modules register `{ name, id, title, group, params? }` with a callable; `/menu/command` dispatches by name or id.
- Content Loaders: register file handlers (e.g., `frame_player`, `text_view`) to validate and open resources.

---

## Implementation Plan (MVP)
1) Server process and UI queue
- Embed lightweight HTTP + WS server; implement a thread-safe action queue.
- Add UI-side dispatcher draining the queue via a timer or broadcast.

2) Window registry and ids
- Assign stable ids on create; map id <-> `TWindow*`.
- Enrich existing workspace JSON with ids (runtime) and leverage `buildWorkspaceJson` for `/state`.

3) Endpoint surface
- Implement `/state`, `/windows*`, `/props`, `/menu/command`, `/workspace/*`, `/screenshot`, `/pattern_mode`, `/frame_player/*`.

4) Events
- Push window lifecycle, commands, workspace and screenshot events over WS.

5) (V2) Timeline engine
- Scheduler that sequences steps by `t` and interpolates `move/resize` with easing; actions dispatch to UI.

6) Introspection
- Populate `/capabilities` from registries.

7) Docs & examples
- Ship curl examples for common flows (create windows, open files, cascade/tile, workspace save/load).

---

## Acceptance Criteria
- Can create, move, resize, focus, clone, and close windows via API; operations reflect in `/state` and WS events.
- Can trigger at least these commands: new test pattern, open animation (by path), save/open workspace, cascade, tile, screenshot.
- Can load content into frame player (`-- demo file provided`) and change FPS via API.
- `/capabilities` lists known window types and commands with minimal schemas.

---

## Examples
### Create two test pattern windows
```bash
# Create windows
curl -sX POST localhost:7777/windows -H 'Content-Type: application/json' \
  -d '{"type":"test_pattern","title":"TP1","bounds":{"x":2,"y":1,"w":40,"h":12}}'
curl -sX POST localhost:7777/windows -H 'Content-Type: application/json' \
  -d '{"type":"test_pattern","title":"TP2","bounds":{"x":10,"y":4,"w":40,"h":12}}'

# Toggle global pattern mode
curl -sX POST localhost:7777/pattern_mode -H 'Content-Type: application/json' -d '{"mode":"continuous"}'

# Cascade and tile
curl -sX POST localhost:7777/windows/cascade
curl -sX POST localhost:7777/windows/tile
```

### Play a frame-file animation and adjust FPS
```bash
curl -sX POST localhost:7777/windows -H 'Content-Type: application/json' \
  -d '{"type":"frame_player","title":"Donut","bounds":{"x":4,"y":2,"w":60,"h":20},"props":{"file":"test-tui/donut.txt"}}'
# Increase fps
curl -sX POST localhost:7777/props/w3 -H 'Content-Type: application/json' -d '{"fps":30}'
```

---

## Glossary (Turbo Vision)
- `TApplication`: App and event loop owner; hosts menu/status/desktop.
- `TDeskTop`: Desktop container where `TWindow`s are placed.
- `TWindow`: Moveable, resizable framed container with a client area.
- `TView`: Base for all visible components.
- Command: High-level action, usually triggered by menu/status shortcuts (`cm*` ids).

---

- Window types present: `TTestPatternWindow`, `TGradientWindow`, `TFrameAnimationWindow` (hosts `FrameFilePlayerView` or `TTextFileView`).
- Commands: File (new windows, open animation, save/open workspace, exit), Edit (screenshot, pattern mode), View (wallpaper, zoom/fullscreen), Window (cascade, tile, next/prev, close, close all, move, zoom), Tools (ansi editor, paint tools, animation studio, quantum printer placeholders), Help (about, shortcuts, debug info).
- Workspace persistence: `buildWorkspaceJson` and `loadWorkspaceFromFile` are good foundations for `/state` and `/workspace/*`.
- Animation: `FrameFilePlayerView` uses UI timers (`cmTimerExpired` broadcasts), suitable for server-driven FPS control.

---

## Future Work
- Per-window pattern mode override and serialization.
- Cross-platform screenshot abstraction.
- Richer ANSI parser (cursor moves, 256/24-bit colors) with graceful degradation.
- Authentication + scoped permissions for multi-user scenarios.
- Record/replay of user interactions as API scripts.
