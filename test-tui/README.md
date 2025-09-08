# Test TUI Applications

This directory contains test TUI applications built with Turbo Vision.

## Applications

### test_pattern
Multi-window test pattern generator with gradients and wallpaper:
- **Features**: Creates unlimited resizable windows with test patterns, gradient windows (horizontal, vertical, radial, diagonal), cascading/tiling window management, screenshot capability
- **Pattern modes**: Continuous (diagonal flowing patterns) or Tiled (cropped at window edges)
- **Background**: ASCII art wallpaper with custom desktop
- **Build**: `cmake . -B ./build && cmake --build ./build`
- **Run**: `./build/test_pattern`

### simple_tui
Basic TUI application demonstrating fundamental Turbo Vision usage:
- **Features**: Simple window with basic menu and status line
- **Purpose**: Learning/reference implementation
- **Build**: `cmake . -B ./build && cmake --build ./build`
- **Run**: `./build/simple_tui`

### frame_file_player
Timer-based ASCII animation player that loads frame files:
- **Features**: Plays frame-delimited text files using UI timers (no threads), configurable FPS, working menubar
- **File format**: Frames separated by `----` lines, optional `FPS=NN` header
- **Build**: `cmake . -B ./build && cmake --build ./build`
- **Run**: `./build/frame_file_player --file frames_demo.txt [--fps NN]`
- **Documentation**: See [FRAME_PLAYER.md](FRAME_PLAYER.md)

## Window Auto-sizing

When using File → Open Text/Animation… in the test pattern app, the window automatically sizes to fit the content:
- Text files: width = longest line; height = total lines.
- Animation files (`----`-delimited): sized to the largest frame (max width/height across frames).
The window never exceeds the desktop area and uses sensible minimum dimensions.

## Build System

All applications use CMake and link against the main Turbo Vision library:
```bash
# From test-tui directory
cmake . -B ./build -DCMAKE_BUILD_TYPE=Release
cmake --build ./build
```

## Files Structure

**Core Applications:**
- `test_pattern_app.cpp` - Main test pattern application
- `simple_tui.cpp` - Basic TUI example
- `frame_file_player_main.cpp` - Frame animation player

**Support Files:**
- `test_pattern.{h,cpp}` - Test pattern generation
- `gradient.{h,cpp}` - Gradient rendering views  
- `wallpaper.{h,cpp}` - ASCII art wallpaper
- `frame_file_player_view.{h,cpp}` - Animation player view
- `CMakeLists.txt` - Build configuration
- `frames_demo.txt` - Sample animation file

**Documentation:**
- `README.md` - This file
- `FRAME_PLAYER.md` - Animation player documentation
