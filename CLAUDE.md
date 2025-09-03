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
```

### Debugging
- Use `TVISION_MAX_FPS=-1` for immediate screen updates (useful for debugging)
- Event viewer in tvdemo helps debug input events