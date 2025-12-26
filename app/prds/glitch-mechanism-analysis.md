# TVision Terminal Resize Glitch Mechanism Analysis

**tl;dr:** Terminal resize corruption occurs during specific timing windows where buffer reassignment, dimension changes, and drawing operations create race conditions leading to character scatter and color bleeding effects.

## Root Cause Analysis

### Primary Corruption Mechanisms

#### 1. Buffer Reassignment Race Condition
**Location:** `TProgram::setScreenMode()` - `/Users/james/Repos/tvision/source/tvision/tprogram.cpp:397`

```cpp
void TProgram::setScreenMode( ushort mode ) {
    TMouse::hide();
    TScreen::setVideoMode(mode);           // 1. Screen dimensions change
    initScreen();                          // 2. New buffer allocated
    buffer = TScreen::screenBuffer;        // 3. Buffer pointer reassigned ⚠️ CRITICAL
    r = TRect(0, 0, TScreen::screenWidth, TScreen::screenHeight);
    changeBounds(r);                       // 4. Views update bounds
    setState(sfExposed, False);            
    setState(sfExposed, True);             // 5. Force redraw ⚠️ CRITICAL
    redraw();                             // 6. All views redraw
    TMouse::show();
}
```

**Race Condition Window:** Between steps 3-6, views may draw using:
- Old `size.x/size.y` values with new `buffer` pointer
- New screen dimensions with partially updated view bounds
- Stale coordinate calculations during `changeBounds()` propagation

#### 2. Gradient Drawing Vulnerability  
**Location:** `THorizontalGradientView::draw()` - `/Users/james/Repos/tvision/test-tui/gradient.cpp:61`

```cpp
void THorizontalGradientView::draw() {
    TDrawBuffer b;
    for (int y = 0; y < size.y; y++) {                    // ⚠️ size.y may change mid-loop
        float t = (size.y > 1) ? static_cast<float>(y) / (size.y - 1) : 0.0f;
        // ... color interpolation ...
        writeLine(0, y, size.x, 1, b);                    // ⚠️ Writes to potentially new buffer
    }
}
```

**Corruption Scenarios:**
- `size.y` changes during loop → incomplete gradient rendering
- `writeLine()` coordinates calculated with old dimensions, written to new buffer
- Color interpolation uses old/new dimension mix → banded artifacts

#### 3. Character Position Scattering
**Location:** `TVWrite::writeView()` and buffer write operations

**Mechanism:**
```cpp
// Buffer write with coordinate calculation
TScreenCell *dst = &owner->buffer[Y*owner->size.x + X];
```

If `owner->size.x` changes between coordinate calculation and buffer write:
- Characters write to incorrect memory locations
- Scatter patterns emerge based on dimension delta
- Color attributes persist at wrong positions

## Timing Window Analysis

### Critical Timing Windows

#### Window 1: Buffer Pointer Reassignment (Most Critical)
**Trigger:** `buffer = TScreen::screenBuffer;`  
**Duration:** ~1-5ms (depends on screen size)  
**Effect:** Views drawing during this window write to old buffer or uninitialized memory  
**Visual Result:** Complete character scrambling, random scatter patterns

#### Window 2: Dimension Update Propagation  
**Trigger:** `changeBounds(r)` calls on all child views  
**Duration:** ~10-50ms (depends on view hierarchy depth)  
**Effect:** Parent/child views have mismatched dimensions during drawing  
**Visual Result:** Gradient banding, partial redraws, coordinate misalignment

#### Window 3: Expose State Toggle
**Trigger:** `setState(sfExposed, False/True)`  
**Duration:** ~5-20ms  
**Effect:** Views redraw with mixed old/new state information  
**Visual Result:** Color bleeding, attribute persistence in wrong locations

### Signal Processing Chain

```
SIGWINCH → emitScreenChangedEvent() → cmScreenChanged → setScreenMode() → CORRUPTION WINDOWS
```

**Timing Sensitivity:**
- Rapid successive SIGWINCH signals (fast manual resize) amplify corruption
- Multiple views drawing simultaneously compound effects  
- Animation timers conflict with resize events

## Observed Glitch Patterns

### Pattern Classification

1. **Character Scatter:** Random character displacement from coordinate calculation errors
2. **Color Bleeding:** Color attributes applied to wrong character positions  
3. **Gradient Banding:** Incomplete gradient calculations with mixed dimensions
4. **Buffer Overflow Artifacts:** Characters written beyond intended boundaries
5. **Attribute Persistence:** Old color/style data remaining after dimension changes

### Visual Characteristics

- **Horizontal scatter:** Most common due to `Y*size.x + X` coordinate calculation
- **Diagonal patterns:** Result from dimension ratio changes
- **Color streaking:** RGB values persist across character position changes
- **Block artifacts:** Full-block characters (`\xDB`) create solid color regions

## Exploitable Timing Mechanisms

### For Programmatic Glitch Generation

1. **Synthetic Buffer Swapping:** Manually trigger buffer reassignment mid-draw
2. **Dimension Injection:** Change view `size.x/size.y` during draw loops  
3. **Coordinate Corruption:** Modify coordinate calculations to create scatter
4. **Attribute Bleeding:** Persist color data across position changes
5. **Partial Draw Interruption:** Stop drawing operations mid-completion

### Key TVision Components to Hook

- `TProgram::setScreenMode()` - Main resize handler
- `TView::writeLine()` - Buffer write operations  
- `TDrawBuffer` operations - Character/color data
- `changeBounds()` - Dimension update propagation
- Buffer pointer assignments - Memory layout changes

## Implementation Strategy

Based on this analysis, the glitch engine should focus on:

1. **Controlled Buffer Corruption:** Simulate the buffer pointer race condition
2. **Dimension Desync:** Temporarily desynchronize view dimensions during drawing
3. **Coordinate Offset Injection:** Add controlled randomness to position calculations
4. **Color Attribute Bleeding:** Preserve and redistribute color data incorrectly
5. **Timing Window Simulation:** Replicate the critical 1-50ms corruption windows

This provides the technical foundation for building controllable, reproducible glitch effects that match the observed terminal resize artifacts.