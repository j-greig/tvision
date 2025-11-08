# Product Requirements Document: Webcam ASCII Art in Turbo Vision

## Executive Summary

Integrate real-time webcam ASCII art conversion into the Turbo Vision `tvdemo` application, accessible via a menu command. This will bridge the web-based ASCII webcam app with the classic TUI framework.

## Target Platform

- Primary: Linux/macOS terminal environments
- Secondary: Windows console
- Test Platform: MacBook Air 2020
- Framework: Turbo Vision (C++ TUI framework)

---

## Technical Architecture Analysis

### Current TVision Structure

Based on codebase analysis:

```
TVDemo (TApplication)
├── initMenuBar() - Creates menu structure
├── handleEvent() - Dispatches commands
└── Methods for each feature:
    ├── puzzle() - Creates TPuzzleWindow
    ├── calculator() - Creates TCalculator
    ├── asciiTable() - Creates TAsciiChart
    └── [NEW] webcamAscii() - Will create TWebcamAsciiWindow
```

**Menu System**:
- Commands defined in `tvcmds.h` (e.g., `cmAsciiCmd = 103`)
- Menu items added in `tvdemo3.cpp::initMenuBar()`
- Event handling in `tvdemo2.cpp::handleEvent()`

**Window Pattern**:
```cpp
void TVDemo::featureName() {
    TFeatureWindow *win = (TFeatureWindow *) validView(new TFeatureWindow);
    if(win != 0) {
        win->helpCtx = hcFeature;
        deskTop->insert(win);
    }
}
```

---

## Tech Stack Evaluation

### Option 1: OpenCV + Custom Rendering (RECOMMENDED)

**Libraries**:
- OpenCV 4.x for webcam capture
- Native Turbo Vision for display
- C++17/20 standard library

**Pros**:
- OpenCV is industry standard for computer vision
- Cross-platform (Linux, macOS, Windows)
- Excellent documentation and community support
- Simple API: `VideoCapture cap(0);`
- Already handles camera permissions/initialization
- Can process frames efficiently

**Cons**:
- Large dependency (~100MB OpenCV)
- Requires CMake build integration
- May be overkill for simple frame capture

**Implementation Approach**:
```cpp
cv::VideoCapture cap(0);
cv::Mat frame;
cap >> frame;
// Convert frame to ASCII
// Render to TView
```

**Ranking**: 5/5

---

### Option 2: V4L2 (Linux) + Native Platform APIs

**Libraries**:
- V4L2 (Video4Linux2) for Linux
- AVFoundation for macOS
- DirectShow for Windows

**Pros**:
- No large dependencies
- Direct hardware access
- Minimal overhead
- Better performance

**Cons**:
- Platform-specific code required (#ifdef hell)
- Complex low-level APIs
- More code to maintain
- Camera format negotiation is manual
- Different API for each platform

**Ranking**: 3/5

---

### Option 3: FFmpeg libavdevice

**Libraries**:
- FFmpeg libavdevice for camera input

**Pros**:
- Cross-platform
- Powerful media handling
- Can record/save if needed

**Cons**:
- Heavy dependency
- Complex API for simple use case
- Overkill for live capture
- More difficult to integrate than OpenCV

**Ranking**: 3/5

---

### Option 4: Lightweight Cross-Platform Libraries

**Libraries**:
- openpnp-capture
- escapi (Windows only)
- Simple-Capture

**Pros**:
- Smaller than OpenCV
- Simpler API
- Purpose-built for webcam

**Cons**:
- Less mature/tested
- Smaller community
- May have platform-specific issues
- Documentation varies

**Ranking**: 4/5

---

## Recommended Approach: OpenCV

**Rationale**:
1. Proven reliability on macOS/Linux/Windows
2. Simple API for frame capture
3. Built-in format conversion (BGR to grayscale)
4. Well-documented
5. Easy CMake integration
6. Active development and support

**Trade-off**: Larger binary size is acceptable for demo application

---

## Feature Requirements

### Core Features (MVP)

1. **Menu Integration**
   - Add "Webcam ASCII" menu item to tvdemo
   - Menu location: Under system menu (with Puzzle, Calendar, etc.)
   - Keyboard shortcut: Alt-W

2. **Webcam Window (TWebcamAsciiWindow)**
   - Inherits from TWindow
   - Resizable window with standard frame
   - Title: "Webcam ASCII Art"
   - Default size: 80x25 characters

3. **ASCII Conversion**
   - Real-time frame capture from webcam
   - Convert pixel brightness to ASCII characters
   - Character set: Standard (from web version)
   - Update rate: 10-15 FPS (TUI limitation)

4. **Display**
   - Render ASCII in window using TView
   - Monochrome display (Turbo Vision style)
   - Auto-scale to window size

5. **Controls**
   - Start/Stop button or automatic start
   - Close window to stop camera
   - Status indicator (FPS, camera status)

### Enhanced Features (Optional)

- Resolution adjustment dialog
- Character set selection
- Color/grayscale toggle (if TV supports color)
- Frame capture/save
- Multiple camera selection

---

## Implementation Plan

### Phase 1: Infrastructure Setup

**Files to Create**:
```
examples/tvdemo/
├── webcam.h         # TWebcamAsciiWindow class declaration
├── webcam.cpp       # Implementation
└── CMakeLists.txt   # Update to link OpenCV
```

**Files to Modify**:
```
examples/tvdemo/
├── tvcmds.h         # Add cmWebcamAsciiCmd = 116
├── tvdemo.h         # Add webcamAscii() method declaration
├── tvdemo2.cpp      # Add handleEvent case
└── tvdemo3.cpp      # Add menu item
```

**CMake Integration**:
```cmake
find_package(OpenCV REQUIRED)
target_link_libraries(tvdemo ${OpenCV_LIBS})
```

---

### Phase 2: Core Implementation

**Class Structure**:

```cpp
class TWebcamAsciiWindow : public TWindow {
public:
    TWebcamAsciiWindow();
    ~TWebcamAsciiWindow();
    virtual void handleEvent(TEvent& event);
    virtual void draw();

private:
    cv::VideoCapture capture;
    TWebcamView *asciiView;
    bool isCapturing;
    std::string charSet;

    void startCapture();
    void stopCapture();
    void updateFrame();
};

class TWebcamView : public TView {
public:
    TWebcamView(TRect bounds);
    virtual void draw();
    void setAsciiData(const std::string& ascii);

private:
    std::vector<std::string> asciiLines;
    int frameWidth, frameHeight;
};
```

**ASCII Conversion Logic** (ported from JavaScript):

```cpp
class AsciiConverter {
public:
    AsciiConverter();
    void setCharSet(const std::string& charset);
    std::string frameToAscii(const cv::Mat& frame, int width, int height);

private:
    std::string charSet;
    char pixelToChar(uint8_t r, uint8_t g, uint8_t b);
    int getBrightness(uint8_t r, uint8_t g, uint8_t b);
};
```

**Update Mechanism**:
- Use Turbo Vision's idle() callback for frame updates
- Capture frame in background (non-blocking)
- Render to view on each draw cycle

---

### Phase 3: Integration & Testing

**Testing Strategy**:

1. **Unit Tests**:
   - ASCII conversion accuracy
   - Brightness calculation
   - Character mapping

2. **Integration Tests**:
   - Window creation/destruction
   - Menu command handling
   - Event dispatching

3. **Manual Tests**:
   - Camera initialization on macOS
   - Frame rate performance
   - Window resizing
   - Memory leak check (extended use)
   - Terminal compatibility (xterm, Konsole, Terminal.app)

4. **Performance Targets**:
   - Frame processing: <50ms per frame
   - FPS: 10-20 FPS (limited by terminal refresh)
   - Memory: <100MB including OpenCV
   - CPU: <25% on MacBook Air 2020

---

## Technical Challenges & Solutions

### Challenge 1: Terminal Refresh Rate

**Problem**: Terminals have limited refresh rates (60Hz max, often less)

**Solution**:
- Cap frame rate at 15 FPS
- Use double buffering in TView
- Only update on actual frame changes

### Challenge 2: OpenCV Dependency Size

**Problem**: OpenCV adds ~100MB to binary

**Solution**:
- Link only required modules (core, videoio, imgproc)
- Use shared libraries, not static
- Document installation requirements

### Challenge 3: Camera Permissions (macOS)

**Problem**: macOS requires camera permissions for terminal apps

**Solution**:
- Add entitlements if needed
- Provide clear error messages
- Document permission setup in README

### Challenge 4: Thread Safety

**Problem**: Turbo Vision is single-threaded, OpenCV may use threads

**Solution**:
- Capture frames in main thread using `cap.read()`
- Avoid async OpenCV operations
- Use simple synchronous API

### Challenge 5: Character Aspect Ratio

**Problem**: Terminal characters are typically 2:1 (height:width)

**Solution**:
- Adjust sampling to account for aspect ratio
- Scale frame height by 0.5 before ASCII conversion
- Make aspect ratio configurable

---

## Build Requirements

### Dependencies

**Required**:
- C++17 compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.13+
- OpenCV 4.0+
- Turbo Vision (included)

**Platform-Specific**:

**macOS**:
```bash
brew install opencv
```

**Linux (Ubuntu/Debian)**:
```bash
sudo apt-get install libopencv-dev
```

**Linux (Fedora)**:
```bash
sudo dnf install opencv-devel
```

---

## File Structure

```
examples/tvdemo/
├── CMakeLists.txt           # Modified: Add OpenCV
├── tvcmds.h                 # Modified: Add cmWebcamAsciiCmd
├── tvdemo.h                 # Modified: Add webcamAscii() declaration
├── tvdemo2.cpp              # Modified: Add event handler
├── tvdemo3.cpp              # Modified: Add menu item
├── webcam.h                 # New: Window & view classes
├── webcam.cpp               # New: Implementation
└── README-WEBCAM.md         # New: Documentation
```

---

## Testing Approach

### Automated Testing

**Framework**: None required (simple unit tests)

**Test Files**:
```
examples/tvdemo/test/
├── test_ascii_converter.cpp  # ASCII conversion logic
└── CMakeLists.txt            # Test executable
```

**Coverage**:
- Brightness calculation (RGB to grayscale)
- Character mapping (brightness to ASCII char)
- Frame scaling/resizing

### Manual Testing Checklist

**Functionality**:
- [ ] Menu item appears correctly
- [ ] Alt-W keyboard shortcut works
- [ ] Window opens when selected
- [ ] Camera initializes successfully
- [ ] ASCII feed displays in real-time
- [ ] Window can be resized
- [ ] Window can be closed
- [ ] Camera stops when window closes

**Performance**:
- [ ] FPS >= 10 on MacBook Air 2020
- [ ] No memory leaks during extended use
- [ ] CPU usage reasonable (<25%)
- [ ] No frame drops or stuttering

**Compatibility**:
- [ ] Works in macOS Terminal.app
- [ ] Works in iTerm2
- [ ] Works in Linux terminals (xterm, gnome-terminal)
- [ ] Handles camera permission dialogs gracefully

**Error Handling**:
- [ ] Graceful failure if no camera
- [ ] Clear error message if OpenCV not found
- [ ] Handles camera busy/in-use scenario
- [ ] Handles permission denied

---

## Documentation Requirements

### README-WEBCAM.md

**Contents**:
1. Feature overview
2. Installation instructions
3. Building with OpenCV
4. Camera permissions setup (macOS)
5. Troubleshooting guide
6. Performance tuning

### Code Documentation

**Required**:
- Class documentation (Doxygen style)
- Method documentation for public APIs
- Complex algorithm comments
- CMake option documentation

---

## Risk Analysis

| Risk | Impact | Probability | Mitigation |
|------|--------|-------------|------------|
| OpenCV build issues | High | Medium | Clear docs, use pkg-config |
| Camera permission denied | High | Medium | Error handling, user docs |
| Poor performance in terminal | Medium | Low | FPS cap, optimization |
| Platform-specific bugs | Medium | Medium | Test on all platforms |
| OpenCV version incompatibility | Medium | Low | Specify minimum version |

---

## Success Criteria

**Functional**:
- Menu command successfully launches webcam window
- ASCII conversion displays in real-time
- Camera permissions work on macOS
- Window integrates seamlessly with tvdemo

**Performance**:
- 10+ FPS on MacBook Air 2020
- <100ms frame processing latency
- Smooth visual experience

**Quality**:
- No memory leaks
- No crashes during normal operation
- Graceful error handling
- Clean code following tvdemo patterns

---

## Alternative: Fallback Without OpenCV

If OpenCV integration proves too complex, implement a simpler version:

**Approach**: Test pattern generator
- Display animated ASCII art pattern (no real webcam)
- Demonstrates TUI integration
- Can be upgraded to real webcam later

**Purpose**: Validates architecture without webcam dependency

---

## Timeline Estimate

**Phase 1: Setup** (30 min)
- CMake configuration
- OpenCV integration
- File structure

**Phase 2: Implementation** (2-3 hours)
- TWebcamAsciiWindow class
- ASCII converter
- Menu integration
- Event handling

**Phase 3: Testing** (1 hour)
- Manual testing
- Bug fixes
- Documentation

**Total**: ~4 hours

---

## Open Questions

1. Should we support color ASCII (if terminal supports it)?
2. Should frame capture run in separate thread or main loop?
3. Do we need configuration dialog or hardcode settings?
4. Should we add recording capability?

---

## Conclusion

**Recommended Approach**:
- Use OpenCV for webcam capture (proven, cross-platform)
- Integrate as TWindow following existing tvdemo patterns
- Add menu item under system menu
- Target 10-15 FPS for smooth TUI experience

**Key Advantages**:
- Reuses proven OpenCV technology
- Follows tvdemo architecture patterns
- Minimal changes to existing code
- Cross-platform from day one

**Next Steps**: Proceed with implementation using OpenCV approach
