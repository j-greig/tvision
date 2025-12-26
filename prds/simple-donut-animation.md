# Simple Donut Animation PRD

**tl;dr:** Port the JavaScript donut.js 3D ASCII animation to run smoothly in a TUI window using proper Turbo Vision architecture with triple buffering and worker threads.

## Context

- Working TUI app with unlimited resizable windows (patterns/gradients)
- JavaScript donut demo exists with proper 3D torus math and ASCII shading
- Previous threading attempts crashed due to improper TV integration
- Need foundation for future animation framework but start minimal

## Objective

Create a single working 3D donut animation window that:
- Spins smoothly at 30 FPS without blocking main thread
- Follows Turbo Vision drawing rules (main thread only)
- Uses triple buffering (staging → back → front)
- Ports the exact JS math to C++ with optimizations

## Architecture

### Core Components

1. **TAsciiCanvasView : public TView**
   - Triple buffers: `vector<Cell> front, back, staging`
   - Z-buffer: `vector<float> zbuf`
   - `draw()` only blits front buffer to screen (fast)
   - `frameReady` atomic flag for synchronization

2. **DonutRenderer**
   - Worker thread renders to staging buffer
   - Exact port of JS math: theta/phi loops, 3D transforms
   - ASCII shading: `.,-~:;=!*#$@`
   - Fixed timestep at 30 FPS

3. **TAnimationWindow**
   - Contains one canvas view
   - Menu integration for "3D Donut" option
   - Auto-start animation on creation

### Data Flow

```
Worker Thread: renderDonut() → staging buffer → frameReady = true
Main Thread: idle() → swap(back,front) → invalidate() → draw()
```

## Implementation Details

### Cell Structure
```cpp
struct Cell {
    char ch = ' ';
    TColorAttr attr = TColorAttr{0x07};
};
```

### Donut Math (Direct JS Port)
- Rotation angles: `A = time * 0.0015`, `B = time * 0.0017`
- Torus params: R1=2, R2=1 (standard donut proportions)
- Theta step: 0.05, Phi step: 0.01
- Perspective: `D = 1/(sinPhi*h*sinA + sinTheta*cosA + 5)`
- Brightness: Surface normal calculation → character index

### Performance Optimizations
- Precompute sin/cos of A,B per frame
- Contiguous buffer access: `offset = y*width + x`
- Z-buffer depth testing: only draw closer pixels
- Aspect ratio correction for terminal chars

### Integration Points
- Add menu item: "New Animation → 3D Donut"
- Command constant: `cmNewAnimationDonut = 108`
- Factory method in AnimationFactory namespace
- `idle()` checks all canvas views for `frameReady`

## Success Criteria

- [ ] Donut window opens from menu
- [ ] Animation spins smoothly without stuttering
- [ ] No crashes or hangs
- [ ] Main thread stays responsive (can resize, move windows)
- [ ] Proper cleanup on window close

## Files to Create/Modify

### New Files
- `ascii_canvas.h/cpp` - Triple-buffered canvas view
- `donut_renderer.h/cpp` - JS math port with worker thread

### Modified Files  
- `test_pattern_app.cpp` - Add menu item, idle() frame swapping
- `CMakeLists.txt` - Add new source files
- Reuse existing `animation_window.h/cpp` structure

## Testing

1. Build and run `./build/test_pattern`
2. File → New Animation → 3D Donut
3. Verify smooth rotation at ~30 FPS
4. Test window resize, move, close
5. Create multiple donut windows simultaneously

## Future Extensions (Not in Scope)

- Additional JS animation ports (matrix, starfield)  
- External control APIs
- Timeline-based orchestration
- Performance monitoring/adaptive quality

## Risk Mitigation

- Start with exact JS math port (proven working)
- Use simple atomic flags for synchronization
- Minimize TV API surface area (only draw() and idle())
- Test early and often with single window

---

This PRD focuses on getting one animation working perfectly before expanding the framework. The triple buffering approach follows your TV mental model exactly while staying minimal enough to debug easily.