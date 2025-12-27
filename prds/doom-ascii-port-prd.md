# PRD: Doom ASCII port into WibWobDOS TUI

## TL;DR
Embed an ASCII DOOM renderer inside WibWobDOS using timer-based animation (33ms ticks, 30 FPS). Vendor doom-ascii to `third_party/doom-ascii/`, implement TView-based window with Unicode block rendering (▀▄█▌▐░▒▓), refactor D_DoomLoop → D_DoomTick for frame-by-frame execution. Single-threaded architecture matches WibWobDOS pattern (35+ existing animated views). Menu item under Tools, API control, WAD config via env var or dialog.

## Problem statement
Users want a "DOOM in the terminal" experience inside WibWobDOS, not a separate terminal process. The existing `doom-ascii` project already renders DOOM to ASCII, but it writes directly to stdout and manages its own terminal input. We need a WibWobDOS-native window that renders DOOM and accepts key input without breaking the Turbo Vision event loop.

## Source of truth (libs / repos)
- Primary repo: https://github.com/wojciech-graj/doom-ascii (GPL-2.0).
- Use its doomgeneric interface (`doomgeneric.h/.c`) and ASCII rendering tables in `src/doomgeneric_ascii.c`.
- WibWobDOS code vendors this repo into `third_party/doom-ascii/` (git clone, not submodule).

## Repo-specific notes (doom-ascii)
- Source base: doomgeneric; terminal glue lives in `src/doomgeneric_ascii.c`.
- Rendering: converts `DG_ScreenBuffer` (RGB pixels) to glyphs and ANSI color, writes a full-frame string to stdout every frame. It uses double-width output (two glyphs per pixel) to preserve aspect ratio.
- Character sets: ASCII gradient, Unicode block elements, and braille patterns (see `doomgeneric_ascii.c` tables).
- Input: reads raw terminal input (termios/Win console) and exposes `DG_GetKey()` events with optional smoothing.
- Defaults: `-scaling` (default 4), `-chars ascii|block|braille`, `-nocolor`, `-nograd`, `-nobold`, `-fixgamma`, `-erase`, `-kpsmooth`.
- Requires a DOOM WAD file (shareware or user-supplied).

## Goals
- Run DOOM ASCII in a WibWobDOS window with interactive controls.
- Maintain stable 30 FPS using timer-based animation (33ms ticks).
- Initial character set: Unicode block elements (`▀▄█▌▐░▒▓`) for best quality/compatibility trade-off.
- Fixed viewport that scales game resolution (320x200) to window size dynamically.
- Provide clear error messaging if WAD is missing or invalid.
- Single-threaded architecture following WibWobDOS patterns (matches 35+ existing animated views).

## Non-goals
- Audio support.
- Multiplayer or net play.
- Full compatibility with every DOOM engine feature.
- Packaging commercial WADs.

## Scope constraints
- GPL-2.0 licensing applies to doom-ascii. The embedded window implies WibWobDOS must be GPL-compatible when distributed.
- WAD file is required (shareware or user-provided).
- Rendering must avoid direct stdout writes and raw terminal control (doom-ascii's current output path cannot be embedded as-is).

## Proposed approach (timer-based in-process port)

### Architecture Overview

```
┌─────────────────────────────────────────────────────┐
│  TDoomAsciiWindow (TWindow wrapper)                 │
│  ├─ ofTileable, wnNoNumber                          │
│  └─ Contains TDoomAsciiView in client rect          │
├─────────────────────────────────────────────────────┤
│  TDoomAsciiView (TView + timer animation)           │
│  ├─ eventMask |= evBroadcast (for cmTimerExpired)  │
│  ├─ timerId = setTimer(33ms, 33ms)  // 30 FPS      │
│  ├─ handleEvent: cmTimerExpired → advance()        │
│  ├─ advance(): D_DoomTick() + render buffer        │
│  ├─ draw(): blit frame buffer to TDrawBuffer       │
│  └─ Input queue: std::vector<DoomKey>              │
├─────────────────────────────────────────────────────┤
│  app/doom/doomgeneric_tvision.c (Platform Layer)    │
│  ├─ DG_Init() - Load WAD, initialize game          │
│  ├─ DG_DrawFrame(buffer) - Copy to view buffer     │
│  ├─ DG_GetKey() - Dequeue input events             │
│  ├─ DG_GetTicksMs() - Monotonic time               │
│  ├─ DG_SleepMs() - No-op (timer controls frame)    │
│  └─ DG_SetWindowTitle() - Update window title      │
├─────────────────────────────────────────────────────┤
│  app/doom/doom_tick_wrapper.c (Game Loop Adapter)   │
│  └─ D_DoomTick() - Single frame execution          │
│     ├─ Wraps D_DoomLoop iteration                  │
│     └─ Returns after one frame render              │
├─────────────────────────────────────────────────────┤
│  third_party/doom-ascii/ (Vendored Sources)         │
│  ├─ src/doomgeneric.c (core engine)                │
│  ├─ src/doomgeneric_ascii.c (character tables)     │
│  └─ ~60 DOOM source files (i_*.c, m_*.c, etc.)     │
└─────────────────────────────────────────────────────┘
```

### Component Details

#### TDoomAsciiView (app/doom_ascii_view.{h,cpp})
- **Base class**: `TView` (not TScroller - fixed viewport, no scrollbar)
- **Timer animation**: `setTimer(33, 33)` for 30 FPS (matches WibWobDOS pattern)
- **Frame buffer**: `std::vector<TScreenCell> frameBuffer` for rendered ASCII
- **Input queue**: `std::vector<uint8_t> inputQueue` for DOOM key codes
- **Key methods**:
  - `draw()`: Blit frame buffer to screen via `writeLine()`
  - `handleEvent()`: Process `cmTimerExpired` and keyboard input
  - `setState()`: Start/stop timer on `sfExposed`
  - `changeBounds()`: Recalculate scaling and redraw on resize
  - `advance()`: Call `D_DoomTick()` + `convertFrameBuffer()`

#### Platform Layer (app/doom/doomgeneric_tvision.c)
Implements 7 required doomgeneric functions:
- **`DG_Init()`**: Load WAD file, validate format, call `D_DoomMain()`
- **`DG_DrawFrame(uint32_t* buffer)`**: Copy RGB buffer to view's frame buffer
- **`DG_GetKey(int* pressed, unsigned char* key)`**: Dequeue input from view's queue
- **`DG_GetTicksMs()`**: Monotonic time via `clock_gettime(CLOCK_MONOTONIC)`
- **`DG_SleepMs(uint32_t ms)`**: No-op (timer controls frame rate, not busy-wait)
- **`DG_SetWindowTitle(const char* title)`**: Update TWindow title text

#### Game Loop Adapter (app/doom/doom_tick_wrapper.c)
- **Problem**: `D_DoomLoop()` is structured as infinite `while(1)` loop
- **Solution**: Refactor to expose `D_DoomTick()` for single-frame execution
- **Pattern**: Extract loop body (TryRunTics + DrawFrame) into callable function
- **Fallback**: If refactoring is too complex, run D_DoomLoop in worker thread (adds mutex complexity, breaks codebase pattern)

### Rendering Implementation

#### Character Mapping (Unicode Blocks)
```cpp
// Convert RGB pixel to Unicode block character
const char* UNICODE_BLOCKS = " ░▒▓█";  // 5 luminance levels

char mapRGBToBlock(uint8_t r, uint8_t g, uint8_t b) {
    int lum = (r * 299 + g * 587 + b * 114) / 1000;  // Luminance
    int level = (lum * 4) / 255;
    return UNICODE_BLOCKS[level];
}
```

**Alternative** (full color):
```cpp
// Use █ (full block) for all pixels, vary RGB color attribute
setCell(cell, 0x2588, TColorAttr(TColorRGB(r,g,b), TColorRGB(0,0,0)));
```

#### Aspect Ratio Preservation
- **DOOM resolution**: 320x200 (16:10 aspect ratio)
- **Terminal cells**: ~2:1 width-to-height ratio (fonts are taller than wide)
- **Solution**: Double-width rendering (2 cells per pixel horizontally)
  - Game pixel → 2 terminal cells (both same character/color)
  - 320 pixels → 640 cells wide
  - 80-column terminal → 40-pixel game viewport width

#### Buffer Management
- **Storage**: `std::vector<TScreenCell> frameBuffer` (sized `width * height`)
- **Render**: Use `TView::writeLine()` with `TScreenCell` array (not `TDrawBuffer` - avoids 132-col cap)
- **Scaling**: Calculate on resize in `changeBounds()`
  ```cpp
  int scaledW = viewWidth / 2;   // Double-width cells
  int scaledH = viewHeight;
  // Sample DG_ScreenBuffer with scaling
  int srcX = (pixelX * 320) / scaledW;
  int srcY = (pixelY * 200) / scaledH;
  ```

### Input Handling

#### Key Mapping (TView → DOOM)
```cpp
// Map Turbo Vision keys to DOOM key codes
kbUp       → KEY_UPARROW
kbDown     → KEY_DOWNARROW
kbLeft     → KEY_LEFTARROW
kbRight    → KEY_RIGHTARROW
kbCtrlA    → KEY_FIRE      // Shoot
kbSpace    → KEY_USE       // Open doors
kbEsc      → KEY_ESCAPE    // Menu
kbEnter    → KEY_ENTER
// Add F1-F12, weapon keys (1-7), etc.
```

#### Queue Management
```cpp
// In TDoomAsciiView::handleEvent()
if (ev.what == evKeyDown) {
    uint8_t doomKey = mapTVKeyToDoom(ev.keyDown.keyCode);
    if (doomKey != 0) {
        inputQueue.push_back(doomKey);
        clearEvent(ev);
    }
}

// In doomgeneric_tvision.c DG_GetKey()
if (!inputQueue.empty()) {
    *pressed = 1;
    *doomKey = inputQueue.front();
    inputQueue.erase(inputQueue.begin());
} else {
    *pressed = 0;
}
```

### Game Loop (Timer-Based)

**Pattern**: Timer-driven animation (matches 35+ existing WibWobDOS views)

#### Timer Setup
```cpp
void TDoomAsciiView::startTimer() {
    if (timerId == 0)
        timerId = setTimer(33, 33);  // 33ms = 30 FPS
}

void TDoomAsciiView::handleEvent(TEvent &ev) {
    if (ev.what == evBroadcast && ev.message.command == cmTimerExpired) {
        if (timerId != 0 && ev.message.infoPtr == timerId) {
            advance();   // Call D_DoomTick() + render
            drawView();  // Trigger draw()
            clearEvent(ev);
        }
    }
}
```

#### Frame Execution
- **No worker thread** - all execution on main TUI thread
- **No mutex** - single-threaded, no race conditions
- **Clean shutdown** - `killTimer()` in destructor, no thread join
- **Terminal I/O safety** - ncurses/Win32 console not thread-safe, this avoids corruption

#### Frame Rate
- **Target**: 30 FPS (33ms timer period)
- **DOOM default**: 35ms (28 FPS) - our 30 FPS is slightly faster but acceptable
- **Timing**: `DG_GetTicksMs()` provides accurate game timing, not fixed tics

### Configuration

#### WAD Path Resolution
1. **Environment variable**: `DOOMWADDIR=/path/to/wad`
2. **File dialog** (future enhancement): TDialog with file picker
3. **Hardcoded default**: `/usr/share/games/doom/doom1.wad` (shareware fallback)

#### Error Handling
- Check WAD file exists in `DG_Init()`
- Validate WAD header (magic bytes: `IWAD` or `PWAD`)
- Show error dialog if missing/invalid: `messageBox("WAD file not found!", mfError | mfOKButton)`
- Provide shareware WAD download URL in error message

## UX
- **Menu location**: `Tools → DOOM ASCII` (command constant: `cmDoomAscii = 162`)
- **Window title**: Shows current level, e.g. `DOOM ASCII - E1M1: Hangar`
- **Optional FPS overlay**: `DOOM ASCII (30 FPS)` in title or status line
- **Factory function**: `createDoomAsciiWindow(const TRect &bounds, const char* wadPath)`
- **API registration**: Window registered for IPC control (`{"type": "doom_ascii", "wad": "doom1.wad", "fps": 30}`)

## Acceptance criteria
- Can launch DOOM ASCII window via menu (`Tools → DOOM ASCII`) and play with keyboard
- Timer-based animation runs at stable 30 FPS (no stutter, smooth gameplay)
- Window adapts to resize (recalculates scaling in `changeBounds()`, redraws immediately)
- Error dialog displayed if WAD missing or invalid (with shareware download URL)
- Rendering is stable, no terminal corruption or flicker
- Character set uses Unicode blocks (`▀▄█▌▐░▒▓`) with proper RGB color mapping
- Input mapping works (arrow keys, Ctrl=fire, Space=use, Esc=menu, etc.)
- Circular view traversal uses `nextView()` (not `->next`) to avoid infinite loops
- Single-threaded architecture (no worker thread, no mutex)
- Build option `BUILD_DOOM_ASCII` allows disabling for GPL-free builds (future enhancement)
- Can play E1M1 for 5+ minutes without crashes or memory leaks

## Risks and mitigations

### Risk 1: D_DoomLoop Refactoring Complexity
**Impact**: High - core game loop must be restructured for timer-based execution
**Mitigation**:
- **Option A**: Extract `D_DoomTick()` from loop body (preferred, clean)
- **Option B**: Patch `D_DoomLoop` directly in vendored code (easier to maintain)
- **Option C**: Fall back to worker thread (adds mutex, breaks codebase pattern)
- **Timeline**: Phase 5 (2-3 hours, highest uncertainty)

### Risk 2: Frame Timing Drift
**Impact**: Medium - timer fires at 33ms (30 FPS), DOOM expects 35ms (28 FPS)
**Mitigation**:
- DOOM tic rate is configurable via `DG_GetTicksMs()` (not hardcoded)
- Use monotonic time for accuracy, not fixed tics
- Accept 30 FPS as design constraint (acceptable for TUI gameplay)
- Test with E1M1 playthrough for timing issues

### Risk 3: GPL-2.0 License Contamination
**Impact**: High - doom-ascii is GPL-2.0, affects distribution
**Mitigation**:
- Document GPL status in README and LICENSE files
- Keep doom-ascii isolated in `third_party/` (no mixing with MIT code)
- Add `BUILD_DOOM_ASCII` CMake option (default ON, can disable)
- For commercial use: consider external process model (out of scope)

### Risk 4: Missing WAD File
**Impact**: Medium - game cannot run without valid WAD
**Mitigation**:
- Check file exists in `DG_Init()`, show error dialog if missing
- Validate WAD header (magic bytes: `IWAD` or `PWAD`)
- Provide shareware WAD download URL in error message
- Add config dialog for WAD selection (Phase 7)

### Risk 5: Terminal Corruption on Resize
**Impact**: Medium - buffer size mismatch can cause flicker/corruption
**Mitigation**:
- Implement `changeBounds()` to recalculate scaling and redraw
- Stop timer during resize, restart after
- Follow `TAnimatedBlocksWindow::changeBounds()` pattern (line 141-148)
- Reallocate frame buffer to match new view size

### Risk 6: Performance Bottlenecks
**Impact**: Low-Medium - RGB→ASCII conversion may be slow
**Mitigation**:
- Use reduced resolution (40x25 viewport vs full 320x200)
- Optimize character mapping (lookup table vs calculation)
- Profile with 30 FPS target, adjust if needed

## Milestones

### Phase 1: Vendoring & Build Setup (2-3 hours)
- Clone doom-ascii into `third_party/doom-ascii/`
- Create `third_party/doom-ascii/CMakeLists.txt` (static library)
- Update `app/CMakeLists.txt` to link doom_ascii_lib
- Build succeeds with undefined reference errors for DG_* functions

### Phase 2: Platform Layer Stubs (1-2 hours)
- Create `app/doom/doomgeneric_tvision.c` with stub implementations
- Create `app/doom/doom_tick_wrapper.c` skeleton
- Build succeeds and links (DOOM won't run yet)

### Phase 3: View Wrapper & Rendering (3-4 hours)
- Create `app/doom_ascii_view.{h,cpp}` following `animated_blocks_view.cpp`
- Implement timer setup, frame buffer conversion (RGB → Unicode blocks)
- Window opens and renders static DOOM frame (no input)

### Phase 4: Input Handling (2 hours)
- Implement key mapping (TView keys → DOOM keys)
- Queue management in `handleEvent()` and `DG_GetKey()`
- Can play DOOM with keyboard (move, shoot, open doors)

### Phase 5: D_DoomLoop Refactoring (2-3 hours) **[CRITICAL PATH]**
- Extract `D_DoomTick()` from continuous loop
- Game runs smoothly at 30 FPS, no stutter
- **Fallback**: Worker thread if refactoring fails

### Phase 6: Menu Integration (30 mins)
- Add `cmDoomAscii = 162` command constant
- Menu item under Tools, case handler in `handleEvent()`
- WAD path resolution (env var or default)

### Phase 7: Polish & Error Handling (1-2 hours)
- Error dialog for missing WAD
- API registration for IPC control
- Window title updates (level name, FPS)
- Integration test: E1M1 playthrough, resize test

**Total estimated effort**: 12-17 hours

## Dependencies

### Build-time
- C++14 compiler (matches tvision requirement)
- CMake 3.10+ (matches project baseline)
- Math library (`-lm`) for DOOM floating point calculations
- pthread (implicit, for `std::chrono` on Linux/macOS)

### Runtime
- **DOOM WAD file** (required):
  - Shareware: `doom1.wad` (~2.3 MB, freely available)
  - Commercial: `doom.wad`, `doom2.wad`, etc.
  - Location: `$DOOMWADDIR`, `/usr/share/games/doom/`, or user-specified path
- **Terminal**: UTF-8 support for Unicode block characters
- **Color support**: `COLORTERM=truecolor` for RGB rendering (fallback to 256-color)

### Optional
- Config dialog for WAD path selection (future enhancement)
- MCP server for API control (already exists in WibWobDOS)

## Reference Architecture

Following existing WibWobDOS patterns:

### Code References
- **Timer animation**: `app/animated_blocks_view.cpp:56-66` (`setTimer`, `cmTimerExpired`)
- **View lifecycle**: `app/animated_blocks_view.cpp:106-117` (`setState`, `sfExposed`)
- **Resize handling**: `app/animated_blocks_view.cpp:119-124` (`changeBounds`)
- **Window wrapper**: `app/animated_blocks_view.cpp:127-149` (`TWindow`, `initFrame`)
- **Build integration**: `app/CMakeLists.txt:42-68` (source lists, linking)
- **Menu integration**: `app/test_pattern_app.cpp:104,866,1820` (command constant, handler, menu item)

### Key Patterns to Follow
1. **Extend TView** (not TScroller) for fixed-viewport animation
2. **Register for broadcast** events: `eventMask |= evBroadcast`
3. **Timer management**: `setTimer(periodMs, periodMs)` in `startTimer()`
4. **Handle timer events**: Check `ev.message.infoPtr == timerId` in `handleEvent()`
5. **Lifecycle**: Start/stop timer in `setState()` on `sfExposed`/hidden
6. **Resize**: Redraw immediately in `changeBounds()` via `drawView()`
7. **Factory pattern**: `TWindow* createDoomAsciiWindow(bounds, wadPath)`
8. **Circular list safety**: Use `v->nextView()` not `v->next` when iterating desktop children

### Architecture Diagram
```
Main TUI Thread (Single-Threaded)
    │
    ├─ TApplication::getEvent() ────┐
    │                                │
    ├─ Timer expires (33ms) ────────┤
    │                                ↓
    ├─ Broadcasts cmTimerExpired ──→ TDoomAsciiView::handleEvent()
    │                                    │
    │                                    ├─ advance()
    │                                    │   ├─ D_DoomTick()  // Single frame
    │                                    │   │   ├─ TryRunTics()  // Game logic
    │                                    │   │   └─ D_Display()  // Calls DG_DrawFrame
    │                                    │   │       └─ DG_DrawFrame(buffer)  // Copy RGB→view
    │                                    │   └─ convertFrameBuffer()  // RGB→ASCII
    │                                    │
    │                                    └─ drawView() ──→ draw()
    │                                                       └─ writeLine()  // Blit to screen
    │
    └─ Keyboard event ─────────────────→ handleEvent()
                                             └─ mapTVKeyToDoom() ──→ inputQueue.push()
                                                                      └─ DG_GetKey() dequeues

No worker threads | No mutexes | Terminal I/O on main thread only
```
