# Turbo Vision macOS Setup & MVP Test Application PRD

**tl;dr:** Build Turbo Vision TUI framework on macOS, create minimal test app with menu/dialog/events. Steps: install deps→cmake build→write simple_tui.cpp→compile→run. Success: working TUI with clickable UI elements.

## Executive Summary

This PRD outlines the implementation plan for setting up Turbo Vision (a modern port of Borland's 1990s text-based UI framework) on macOS and creating a minimal viable TUI application that demonstrates core framework capabilities.

## Current State Analysis

### Environment
- **Platform**: macOS (Darwin)
- **Repository**: Fresh clone of github.com/magiblot/tvision
- **Dependencies Status**: Unknown (need to verify CMake, Xcode tools, ncurses)
- **Build Status**: Not yet attempted

### Framework Capabilities
- Cross-platform TUI framework with Unicode support
- Event-driven architecture (keyboard, mouse, timers)
- Rich widget set (windows, dialogs, menus, buttons)
- 24-bit color support
- System clipboard integration via pbcopy/pbpaste on macOS

## Goals & Success Criteria

### Primary Goals
1. Successfully build Turbo Vision library on macOS
2. Create a minimal but functional TUI application
3. Demonstrate core framework features
4. Establish development workflow for future TUI projects

### Success Criteria
- ✅ Library compiles without errors
- ✅ Example applications (tvdemo, tvedit) run correctly
- ✅ Custom test application launches and responds to user input
- ✅ Mouse and keyboard events work properly
- ✅ Menus, dialogs, and buttons render correctly
- ✅ Application exits cleanly

## Implementation Plan

### Phase 1: Environment Setup (15 min)

#### 1.1 Verify Dependencies
```bash
# Check Xcode Command Line Tools
xcode-select --version

# Check CMake
cmake --version

# Check ncurses
ls -la /usr/lib/libncurses*
```

#### 1.2 Install Missing Dependencies
```bash
# Install Xcode tools if missing
xcode-select --install

# Install CMake via Homebrew if missing
brew install cmake
```

### Phase 2: Build Turbo Vision (20 min)

#### 2.1 Configure Build
```bash
cd /Users/james/Repos/tvision
cmake . -B ./build -DCMAKE_BUILD_TYPE=Release
```

#### 2.2 Compile Library & Examples
```bash
cmake --build ./build -j$(sysctl -n hw.logicalcpu)
```

#### 2.3 Verify Build
```bash
# Check build artifacts
ls -la ./build/
./build/hello  # Run hello world example
```

### Phase 3: Create MVP Test Application (30 min)

#### 3.1 Application Specification

**Name**: SimpleTUI  
**Purpose**: Demonstrate Turbo Vision capabilities in minimal code

**Features**:
- Application window with custom title
- Menu bar with File and Help menus
- Status line showing keyboard shortcuts
- Main dialog with:
  - Static text label
  - Input field
  - Radio buttons
  - Action buttons (OK, Cancel)
- Event handling for:
  - Menu selections
  - Button clicks
  - Keyboard shortcuts (Alt-X to exit)
  - Mouse interactions

#### 3.2 File Structure
```
test-tui/
├── CMakeLists.txt
└── simple_tui.cpp
```

#### 3.3 Implementation Details

**simple_tui.cpp** structure:
```cpp
// Core components
class TSimpleApp : public TApplication {
    // Menu initialization
    // Status line setup
    // Event handling
    // Main dialog creation
};

// Features to implement:
1. Custom menu with items
2. Dialog with form controls
3. Event loop with command handling
4. Proper cleanup on exit
```

### Phase 4: Build & Test MVP (15 min)

#### 4.1 Create Build Configuration
```cmake
# CMakeLists.txt for test application
cmake_minimum_required(VERSION 3.5)
project(SimpleTUI)
set(CMAKE_CXX_STANDARD 14)
find_package(tvision REQUIRED)
add_executable(simple_tui simple_tui.cpp)
target_link_libraries(simple_tui tvision::tvision)
```

#### 4.2 Compile Test Application
```bash
cd test-tui
cmake . -B build
cmake --build build
```

#### 4.3 Run & Validate
```bash
./build/simple_tui
```

## Risk Mitigation

### Potential Issues & Solutions

1. **ncurses Library Issues**
   - Risk: macOS uses ncurses instead of ncursesw
   - Mitigation: Turbo Vision auto-detects and falls back to ncurses

2. **CMake Version Compatibility**
   - Risk: Old CMake doesn't support -B flag
   - Mitigation: Use traditional mkdir build && cd build approach

3. **Compiler Issues**
   - Risk: Missing Xcode Command Line Tools
   - Mitigation: Install via xcode-select --install

4. **Terminal Compatibility**
   - Risk: Terminal emulator doesn't support required features
   - Mitigation: Use Terminal.app or iTerm2 which are fully supported

5. **Apple Silicon Compatibility**
   - Risk: M1/M2 architecture issues
   - Mitigation: Framework confirmed working on Apple Silicon via CI

## Testing Strategy

### Functional Tests
1. Launch application - verify window appears
2. Navigate menus with keyboard - verify selection works
3. Click menu items with mouse - verify mouse support
4. Open dialog - verify rendering
5. Tab between controls - verify focus management
6. Enter text in input field - verify text input
7. Click buttons - verify event handling
8. Exit with Alt-X - verify shortcuts work

### Performance Tests
1. Responsive to rapid keyboard input
2. Smooth mouse tracking
3. No screen flicker during updates
4. Clean exit without memory leaks

## Code Quality Standards

### TUI Best Practices
1. Inherit from TApplication for main app class
2. Override initMenuBar() and initStatusLine()
3. Use proper event handling with handleEvent()
4. Clean up resources with destroy()
5. Follow Turbo Vision naming conventions (T prefix for classes)

### C++ Standards
1. Use C++14 features appropriately
2. RAII for resource management
3. Const correctness
4. Clear separation of concerns

## Deliverables

1. ✅ Working Turbo Vision build on macOS
2. ✅ Compiled example applications
3. ✅ Custom SimpleTUI application source
4. ✅ Build configuration files
5. ✅ This PRD document with results

## Implementation Results

### Build Status: ✅ SUCCESS

#### Environment Verification
- **CMake Version**: 4.1.0
- **Xcode Tools**: Version 2409
- **Compiler**: AppleClang 16.0.0.16000026
- **ncurses**: Found at /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/lib/libncurses.tbd

#### Turbo Vision Build Results
- **Library**: Successfully built libtvision.a (static library)
- **Examples Built**: All 10 example applications compiled successfully
  - hello (413 KB)
  - tvdemo (663 KB)
  - tvedit (562 KB)
  - tvdir (480 KB)
  - tvforms (611 KB)
  - tvhc (203 KB)
  - palette (431 KB)
  - mmenu (353 KB)
  - genparts (543 KB)
  - genphone (513 KB)

#### Test Application Build Results
- **simple_tui**: Successfully compiled (485 KB)
- **Location**: `/Users/james/Repos/tvision/build/simple_tui`

### How to Run

```bash
# Run the test application
./build/simple_tui

# Run example applications
./build/tvdemo     # Full demo suite
./build/tvedit     # Text editor
./build/hello      # Hello world example
```

### Key Features Demonstrated in SimpleTUI

1. **Menu System**: File, Test, and Help menus with keyboard shortcuts
2. **Status Line**: Shows available commands and shortcuts
3. **Dialog Boxes**: Main dialog with form controls, About dialog
4. **Form Controls**:
   - Text input fields with labels
   - Radio buttons (3 options)
   - Checkboxes (3 features)
   - Action buttons (OK, Test, Cancel)
5. **Event Handling**: Keyboard shortcuts, mouse support, command processing
6. **Data Collection**: Form submission with value extraction

### Keyboard Shortcuts Implemented
- `Alt-X`: Exit application
- `Alt-M`: Show main dialog
- `Alt-F`: File menu
- `Alt-T`: Test menu
- `Alt-H`: Help menu
- `Alt-A`: About dialog
- `F10`: Activate menu bar

### Technical Notes

1. **macOS Compatibility**: The framework automatically uses `ncurses` instead of `ncursesw` on macOS
2. **Clipboard Support**: Built-in support via pbcopy/pbpaste
3. **Unicode**: Full UTF-8 support enabled
4. **Colors**: 24-bit color support available
5. **Terminal Compatibility**: Works with Terminal.app, iTerm2, and other modern terminal emulators

## Timeline

- **Total Estimated Time**: 80 minutes
- Phase 1: 15 minutes - Environment setup
- Phase 2: 20 minutes - Build framework
- Phase 3: 30 minutes - Create MVP
- Phase 4: 15 minutes - Build and test

## Success Metrics

- Zero build errors
- All examples run successfully
- Custom app launches and accepts input
- Clean shutdown without crashes
- Responsive UI with <100ms latency

## Next Steps

After successful MVP:
1. Explore advanced widgets (file dialogs, lists)
2. Implement Unicode text handling
3. Add persistent configuration
4. Create multi-window application
5. Integrate with system clipboard

## References

- [Turbo Vision Repository](https://github.com/magiblot/tvision)
- [Original Borland Documentation](https://archive.org/details/BorlandTurboVisionForCUserSGuide)
- [CMake Documentation](https://cmake.org/documentation/)
- [ncurses on macOS](https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man3/ncurses.3x.html)

---

*Document Version: 1.0*  
*Date: 2025-09-03*  
*Author: World-class Turbo Vision Developer*