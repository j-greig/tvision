# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is **Turbo Vision** - a modern port of the classic 1990s text-based UI framework originally by Borland. It's a C++ library for building TUI (Terminal User Interface) applications that work across platforms (Linux, Windows, macOS) with Unicode support.

## Build Commands

### macOS/Linux Build
```bash
# Configure and build the library and examples
cmake . -B ./build -DCMAKE_BUILD_TYPE=Release
cmake --build ./build

# Run example applications (built to ./build/)
./build/tvdemo    # Main demo application
./build/tvedit    # Text editor demo
./build/tvhc      # Help compiler
./build/hello     # Simple hello world
```

### Build Options
- `TV_BUILD_EXAMPLES` (default ON) - Build example applications
- `TV_BUILD_USING_GPM` (default ON on Linux) - Enable GPM mouse support
- `TV_BUILD_TESTS` (default OFF) - Build and run tests
- `TV_OPTIMIZE_BUILD` (default ON) - Use precompiled headers (CMake 3.16+)

### Development Commands
```bash
# Clean build
rm -rf ./build

# Debug build
cmake . -B ./build -DCMAKE_BUILD_TYPE=Debug
cmake --build ./build

# Run specific example
./build/examples/tvdemo/tvdemo
```

## Architecture & Key Components

### Core Library Structure
- **source/tvision/** - Main library implementation (~190 source files)
  - View classes (t*.cpp) - Core UI components like TView, TWindow, TDialog
  - Stream classes (s*.cpp) - Serialization support  
  - Name classes (nm*.cpp) - Class name registration
  - Assembly files (.asm) - Low-level optimizations (legacy)

### Key Abstractions
- **TView** - Base class for all visual components
- **TApplication** - Main application class that manages the event loop
- **TEvent** - Event system for keyboard/mouse input
- **TDrawBuffer** - Buffer for screen drawing operations
- **TScreen** - Hardware abstraction for terminal I/O
- **TPalette** - Color management system

### Platform Support
- **Unix/Linux**: Uses ncurses for terminal handling
- **Windows**: Win32 Console API
- **macOS**: Terminal emulator support via ncurses

### Unicode & Extended Features
- UTF-8 support throughout (not in Borland C++ builds)
- 24-bit color support (extends original 16 colors)
- Mouse wheel and middle button support
- Clipboard integration (system clipboard where available)
- Resizable windows and responsive layouts

### Include Structure
- **include/tvision/** - Main headers
  - tv.h - Main include file
  - internal/ - Platform-specific implementations
  - compat/borland/ - Borland C++ compatibility headers

## Key Implementation Details

### Event System
- Events are processed through `TProgram::getEvent()`
- Supports keyboard (with Unicode), mouse, and timer events
- Extended key combinations via `TKey` class

### Color System
- `TColorAttr` - Modern color attribute type (replaces uchar)
- Supports BIOS colors, RGB, xterm-256, and terminal defaults
- Backward compatible with legacy code

#### Pure Black Backgrounds
**Issue**: Terminal color schemes often map ANSI color 0 (black) to dark grey (#15191E) instead of pure black (#000000)

**Solution**: Use RGB colors to bypass terminal palette interpretation:
```cpp
// Wrong: Uses terminal palette (may be dark grey)
TAttrPair black = 0x00;

// Correct: Uses true RGB black
TColorRGB trueBlack(0, 0, 0);
TColorAttr black(trueBlack, trueBlack);
```

**Requirements**: Requires `COLORTERM=truecolor` for full RGB support

### Drawing System
- Views draw to `TDrawBuffer` then write to screen
- Unicode text handling with proper width calculations
- Support for combining characters and double-width characters

## Environment Variables

- `TVISION_MAX_FPS` - Refresh rate limit (default 60)
- `TVISION_CODEPAGE` - Character set for extended ASCII (437 or 850)
- `TVISION_USE_STDIO` - Use stdin/stdout instead of /dev/tty
- `ESCDELAY` - Milliseconds to wait after ESC key (default 10)

## Dependencies

### Required
- C++14 compiler
- CMake 3.5+
- ncursesw (note the 'w' for wide character support)

### Optional
- libgpm (Linux console mouse support)
- xsel/xclip (X11 clipboard)
- wl-clipboard (Wayland clipboard)

## Test Applications (test-tui/)

The **test-tui/** directory contains experimental TUI applications for testing and development:

### Available Applications
- **test_pattern** - Multi-window test pattern generator with gradients, wallpaper, screenshot capability
- **simple_tui** - Basic TUI demonstrating fundamental TV usage patterns
- **frame_file_player** - Timer-based ASCII animation player (loads frame files, no threads)

### Build Test Apps
```bash
cd test-tui
cmake . -B ./build -DCMAKE_BUILD_TYPE=Release
cmake --build ./build

# Run applications (examples)
./build/test_pattern                              # Pattern generator
./build/frame_file_player --file frames_demo.txt # Animation player
```

## Common Development Tasks

### Adding a New View Class
1. Create header in include/tvision/
2. Implement in source/tvision/t*.cpp
3. Add stream registration in source/tvision/nm*.cpp
4. Update CMakeLists.txt if needed

### Testing Changes
```bash
# Build and run the demo app to test UI changes
cmake --build ./build && ./build/tvdemo

# Test the editor for text handling
./build/tvedit test.txt

# Test with experimental apps
cd test-tui && cmake --build ./build && ./build/test_pattern
```

### Animation Development
- **Frame files**: Use `----` delimiter lines, optional `FPS=NN` header
- **Timer-based**: Use `TTimerId setTimer(timeout, period)` and `cmTimerExpired` events
- **No threads**: Keep animations on main UI thread via TV timer system

### Debugging
- Use `TVISION_MAX_FPS=-1` for immediate screen updates (useful for debugging)
- Event viewer in tvdemo helps debug input events
- Claude should never attempt to run tvision apps using bash as it borks the REPL - only humans should run the apps. Claude should ask the human to run if required and tell the human how. eg "cd test-tui && ./build/simple_tui"

## Programmatic Control API

The **tools/api_server** directory contains a FastAPI-based REST API server that provides programmatic control over TUI applications via HTTP endpoints. This enables remote window management, automated testing, and integration with external tools.

### Architecture

- **FastAPI Server** (`tools/api_server/`) - REST API with WebSocket events
- **Unix Socket IPC** (`test-tui/api_ipc.*`) - Bridge between Python API and C++ TUI apps
- **Window Registry** - Stable window ID management in C++ applications
- **State Sync** - Real-time bidirectional state synchronization

### Running the API Server

#### 1. Setup Virtual Environment
```bash
cd tools/api_server
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
```

#### 2. Start the API Server
```bash
# From project root
cd /path/to/tvision
/path/to/tvision/tools/api_server/venv/bin/python -m tools.api_server.main --port=8089
```

The API server will be available at: **http://127.0.0.1:8089**

#### 3. Run a Compatible TUI Application
```bash
# In a separate terminal
cd test-tui
./build/test_pattern
```

The TUI app automatically creates a Unix socket at `/tmp/test_pattern_app.sock` for IPC communication.

### API Endpoints

#### Interactive Documentation
- **Swagger UI**: http://127.0.0.1:8089/docs
- **ReDoc**: http://127.0.0.1:8089/redoc

#### Core Endpoints

##### Application State
- `GET /state` — Get current application state, window list, and properties

##### Window Management  
- `POST /windows` — Create window: `{type, title?, rect?, props?}`
- `POST /windows/{id}/move` — Move/resize window: `{x?, y?, w?, h?}`  
- `POST /windows/{id}/focus` — Focus window (bring to front)
- `POST /windows/{id}/close` — Close specific window
- `POST /windows/cascade` — Cascade all windows  
- `POST /windows/tile` — Tile windows: `{cols?}`
- `POST /windows/close_all` — Close all windows

##### Application Control
- `POST /pattern_mode` — Set pattern mode: `{mode:"continuous"|"tiled"}`
- `POST /screenshot` — Take screenshot  
- `POST /workspace/save` — Save workspace layout
- `POST /workspace/load` — Load workspace: `{path}`

##### Real-time Events
- `GET /ws` — WebSocket connection for live events

### Usage Examples

#### Basic Window Control
```bash
# Create a test pattern window
curl -X POST "http://127.0.0.1:8089/windows" \
  -H "Content-Type: application/json" \
  -d '{"type": "test_pattern", "title": "Test Window"}'

# Get current state to see window IDs  
curl "http://127.0.0.1:8089/state"

# Move window to position (30, 10)
curl -X POST "http://127.0.0.1:8089/windows/w3/move" \
  -H "Content-Type: application/json" \
  -d '{"x": 30, "y": 10}'

# Create gradient window
curl -X POST "http://127.0.0.1:8089/windows" \
  -H "Content-Type: application/json" \
  -d '{"type": "gradient", "props": {"gradient": "radial"}}'

# Arrange windows in cascade
curl -X POST "http://127.0.0.1:8089/windows/cascade"
```

#### Python Integration
```python
import requests

api_base = "http://127.0.0.1:8089"

# Create window
response = requests.post(f"{api_base}/windows", json={
    "type": "test_pattern", 
    "title": "Remote Window"
})
print(response.json())

# Get all windows  
state = requests.get(f"{api_base}/state").json()
windows = state["windows"]

# Move first window
if windows:
    win_id = windows[0]["id"]
    requests.post(f"{api_base}/windows/{win_id}/move", json={
        "x": 50, "y": 15, "w": 60, "h": 20
    })
```

#### WebSocket Events
```javascript
const ws = new WebSocket('ws://127.0.0.1:8089/ws');
ws.onmessage = (event) => {
    const data = JSON.parse(event.data);
    console.log('Event:', data.type, data.payload);
};
```

### Window Types

- **test_pattern** - Test pattern generator window
- **gradient** - Gradient display window (horizontal, vertical, radial, diagonal)
- **frame_player** - Animation player window: `props: {"path": "file.txt"}`
- **text_view** - Text file viewer window: `props: {"path": "file.txt"}`

### IPC Protocol (Advanced)

For direct integration, the Unix socket at `/tmp/test_pattern_app.sock` accepts commands:

```bash
# Command format: "cmd:<name> key=value key=value"
echo "cmd:get_state" | nc -U /tmp/test_pattern_app.sock
echo "cmd:move_window id=w1 x=20 y=10" | nc -U /tmp/test_pattern_app.sock
```

### Events & WebSocket

The API emits real-time events via WebSocket:
- `window.created` — `{id, type, title, rect, focused, props}`
- `window.updated` — Same payload as created  
- `window.closed` — `{id}`
- `layout.cascade` — `{}`
- `layout.tile` — `{cols}`

### Troubleshooting

#### Connection Issues
```bash
# Check if socket exists
ls -la /tmp/test_pattern_app.sock

# Check if app is running
lsof /tmp/test_pattern_app.sock

# Test direct IPC
echo "cmd:get_state" | nc -U /tmp/test_pattern_app.sock
```

#### Socket Permission Issues  
```bash
# Remove stale socket and restart TUI app
rm -f /tmp/test_pattern_app.sock
cd test-tui && ./build/test_pattern
```

The API provides full programmatic control over TUI applications, enabling powerful automation and integration capabilities while maintaining real-time responsiveness.