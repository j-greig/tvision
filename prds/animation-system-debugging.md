# Animation System Debugging PRD

**tl;dr:** Turbo Vision TUI app crashes/hangs on startup when animation system is integrated. Need diagnosis of thread synchronization, object lifecycle, or API usage issues in triple-buffered ASCII canvas architecture.

## Problem Statement

A working Turbo Vision C++ TUI application (patterns/gradients in resizable windows) crashes or hangs immediately on startup when a new animation system is added. The crash occurs before any animation windows are created - just having the animation code present causes the main app to become unresponsive.

## Current Architecture Attempt

### Base Canvas System
```cpp
// ascii_canvas.h
class TAsciiCanvasView : public TView {
public:
    TAsciiCanvasView(const TRect& bounds);
    virtual ~TAsciiCanvasView();
    
    void startAnimation();
    void stopAnimation(); 
    void swapIfFrameReady(); // Called from main thread idle()
    
protected:
    virtual void renderFrame(float time, int width, int height) {
        // Default: do nothing (safe for base class)
    }
    
private:
    // Triple buffering
    std::vector<Cell> frontBuffer, backBuffer, stagingBuffer;
    std::vector<float> zBuffer;
    
    // Synchronization  
    std::atomic<bool> frameReady{false};
    std::atomic<bool> animating{false};
    std::atomic<bool> shouldStop{false};
    
    // Worker thread
    std::unique_ptr<std::thread> workerThread;
    
    void workerLoop(); // Fixed 30 FPS timestep
    void resizeBuffers(int width, int height);
    void swapBackToFront();
};

struct Cell { char ch = ' '; TColorAttr attr = TColorAttr{0x07}; };
```

### Worker Thread Implementation
```cpp
void TAsciiCanvasView::workerLoop() {
    const float targetFPS = 30.0f;
    const auto frameInterval = duration<float>(1.0f / targetFPS);
    
    auto animationStart = steady_clock::now();
    auto lastFrame = animationStart;
    
    while (!shouldStop) {
        const auto now = steady_clock::now();
        const auto elapsed = now - lastFrame;
        
        if (elapsed >= frameInterval) {
            const float time = duration<float>(now - animationStart).count();
            
            clearBuffers();
            renderFrame(time, size.x, size.y); // Virtual call to derived class
            
            backBuffer.swap(stagingBuffer); // Publish frame
            frameReady = true;
            
            lastFrame = now;
        }
        
        std::this_thread::sleep_until(lastFrame + frameInterval);
    }
}
```

### Donut Animation Implementation  
```cpp
// donut_view.h
class TDonutView : public TAsciiCanvasView {
protected:
    virtual void renderFrame(float time, int width, int height) override;
    // Direct port of JavaScript donut.js 3D torus math
};

// donut_window.h  
class TDonutWindow : public TWindow {
    std::unique_ptr<TDonutView> donutView;
public:
    TDonutWindow(const TRect& bounds, const char* title);
    TDonutView* getDonutView() const { return donutView.get(); }
};
```

### Main Application Integration
```cpp
// In TTestPatternApp::idle()
void TTestPatternApp::idle() {
    TApplication::idle();
    
    TView* current = deskTop->first();
    while (current) {
        if (current->getState(sfVisible)) {
            TDonutWindow* donutWindow = dynamic_cast<TDonutWindow*>(current);
            if (donutWindow) {
                TDonutView* donutView = donutWindow->getDonutView();
                if (donutView) {
                    donutView->swapIfFrameReady(); // Swap front/back buffers
                }
            }
        }
        current = current->next;
    }
}

// Frame swapping implementation
void TAsciiCanvasView::swapIfFrameReady() {
    if (frameReady.exchange(false)) {
        frontBuffer.swap(backBuffer);
        drawView(); // Turbo Vision invalidate
    }
}
```

### Turbo Vision Drawing Integration
```cpp
void TAsciiCanvasView::draw() {
    TDrawBuffer buf;
    const int width = size.x;
    
    for (int y = 0; y < size.y; y++) {
        const int rowStart = y * width;
        for (int x = 0; x < width; x++) {
            const Cell& cell = frontBuffer[rowStart + x];
            buf.moveChar(x, cell.ch, cell.attr, 1);
        }
        writeLine(0, y, width, 1, buf);
    }
}
```

## Observed Behavior

1. **Without animation system:** App starts normally, patterns/gradients work
2. **With animation system added:** App crashes/hangs immediately on startup  
3. **Before any animation windows created:** Crash happens just from having the code present
4. **Visual:** Can see main window with test pattern, but can't click anything

## Suspected Issues

### Thread Lifecycle Problems
- Worker thread may be starting during object construction
- Destructor may be called while worker thread is running
- Race condition between main thread and worker thread initialization

### Turbo Vision API Violations  
- `drawView()` being called from wrong thread context
- Improper use of TView lifecycle methods
- Invalid object state during construction/destruction

### Memory Management
- Vectors being resized while worker thread is accessing them
- Atomic variables not properly initialized
- Smart pointer lifecycle issues with TView insertion

### Object Iteration Issues
- `dynamic_cast` on objects in inconsistent states
- Iterating through TView hierarchy unsafely  
- Desktop traversal during object construction

## Technical Context

### Turbo Vision Requirements
- All drawing must happen on main UI thread via `TView::draw()`
- `drawView()`/`invalidate()` requests repaints from main thread
- TView objects inserted into windows via `insert(view.get())`

### Threading Model Attempted
- **Main thread:** Turbo Vision event loop, frame swapping in `idle()`  
- **Worker thread:** Animation computation, buffer rendering
- **Synchronization:** Atomic flags (`frameReady`, `shouldStop`, `animating`)

### Build System
- CMake with C++14, threading support via `Threads::Threads`
- Links against libtvision.a and ncurses

## Questions for Analysis

1. **Thread Safety:** Are there Turbo Vision objects that cannot be accessed from worker threads?

2. **Construction Order:** Is the worker thread starting before the TView is fully constructed and inserted?

3. **Virtual Function Calls:** Is calling virtual `renderFrame()` from worker thread during construction unsafe?

4. **Object Lifecycle:** Are atomic variables being accessed after object destruction?

5. **API Usage:** Is there a proper way to integrate background rendering with Turbo Vision's drawing model?

## Success Criteria for Solution

- App starts without hanging/crashing  
- Can create animation windows that display spinning 3D ASCII donut
- Main thread remains responsive to user input
- Clean shutdown when animation windows are closed
- Multiple animation windows can coexist

## Files Involved

- `ascii_canvas.h/cpp` - Base triple-buffered canvas
- `donut_view.h/cpp` - 3D donut animation implementation  
- `donut_window.h/cpp` - Window wrapper for donut view
- `test_pattern_app.cpp` - Main application with `idle()` integration
- `CMakeLists.txt` - Build configuration with threading

The core question: **Why does adding worker thread animation support cause immediate crashes in a previously stable Turbo Vision application?**