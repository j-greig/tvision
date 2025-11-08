# PRD: Animated Blocks Window and Animation System MVP (TVision)

This document specifies a complete, standalone plan to add a new animated “zigzag blocks” window to the `test-tui` app, and to establish an extensible animation approach using Turbo Vision (TVision) timers and broadcast events. It contains sufficient detail for another contributor to implement, test, and iterate without additional context.

## Objectives
- Provide a new View → Animated Blocks menu item that opens a tileable window showing rows of colored block characters that appear to “move”: even-numbered rows shift right each frame; odd-numbered rows shift left.
- Establish a minimal, idiomatic animation mechanism for TVision views: UI-thread timers, broadcast handling, and view lifecycle awareness.
- Keep the MVP small and robust, but pave a path to more advanced animation controllers and timelines.

## Non-Goals (MVP)
- No multithreading or worker threads.
- No external configuration files for the new window.
- No global animation manager yet; each animated view owns its timer.
- No persistence of animation parameters across runs.

## User Story
As a user of the `test-tui` playground, I can choose View → Animated Blocks to create an animated window that shows rows of colored blocks shifting left-right in alternating rows, producing a classic TUI “marching” effect. I can open multiple such windows, tile and cascade them, and the animations remain smooth and independent.

## UI/UX
- Menu entry: View → Animated Blocks
- Window title: “Animated Blocks”
- Window behavior:
  - Resizable; content adapts to the current client size.
  - Tileable/cascadeable; independent animation per window.
  - Pauses when not exposed (hidden/minimized or behind another modal), resumes when exposed.

## Architecture Overview
TVision supplies a timer mechanism delivered via the event loop:
- `TView::setTimer(periodMs, firstTimeoutMs) → TTimerId` schedules periodic timeouts.
- Timeouts are emitted as broadcast events in `TProgram::idle()` with `ev.message.command == cmTimerExpired` and `ev.message.infoPtr == TTimerId`.
- Views opt-in to receive broadcasts: `eventMask |= evBroadcast`.
- A view compares the incoming `TTimerId` to its own; if it matches, it advances animation state and redraws.
- Visibility lifecycle: start timer on `setState(sfExposed, True)`, stop on `setState(sfExposed, False)` to avoid work when hidden.

This pattern avoids threads, is portable, and matches terminal redraw constraints (only animates when the event loop is idle).

## Detailed Design

### New Types and Files
- `TAnimatedBlocksView` in `test-tui/animated_blocks_view.h` and `test-tui/animated_blocks_view.cpp`
  - Base: `TView`
  - Data:
    - `unsigned periodMs` (default 100 ms → ~10 FPS)
    - `TTimerId timerId`
    - `int phase` (increments each tick)
  - Methods:
    - `draw()`: Renders `size.y` rows and `size.x` columns of block glyphs.
    - `handleEvent(TEvent&)`: On `cmTimerExpired` for own `timerId`, increments `phase` and triggers `drawView()`.
    - `setState(ushort, Boolean)`: Starts timer on expose, stops on hide; resets `phase` when shown.
    - `setSpeed(unsigned)`: Optional tuning hook (restarts timer if running).
  - Rendering algorithm per cell `(x, y)`:
    - Determine row parity: even rows “move right”, odd rows “move left”.
    - Compute `shifted = (moveRight ? x + phase : x - phase)`.
    - `colorIndex = ((shifted % 16) + 16) % 16` (wrap, handle negatives) to select from a 16-color palette.
    - Use full block char `0xDB` (CP437 ‘█’) with chosen foreground color on black background.
  - Resilience:
    - If width/height ≤ 0, `draw()` does nothing.
    - No allocation per frame; uses `TDrawBuffer` line-by-line.

- Factory: `createAnimatedBlocksWindow(const TRect &bounds)` in `animated_blocks_view.cpp`
  - Creates a tileable `TWindow` titled “Animated Blocks”, inserts `TAnimatedBlocksView` sized to the client area.

### Menu Integration
- In `test-tui/test_pattern_app.cpp`:
  - Add command id: `const ushort cmAnimatedBlocks = 134;`
  - Add menu item under View: `*new TMenuItem("~A~nimated Blocks", cmAnimatedBlocks, kbNoKey)`.
  - Handle command in `handleEvent`:
    - Compute a reasonable client rect (e.g., `deskTop->getExtent(); r.grow(-10, -5);`).
    - `deskTop->insert(createAnimatedBlocksWindow(r));`

### Color Palette
- 16 “ANSI-like” foreground colors on black background (array of `TColorAttr`), repeated across columns.
- Same palette used in `test_pattern.cpp` ensures cohesive look & easy reuse.

### Performance Considerations
- FPS: Default 10 FPS (100 ms period) for low CPU and good terminal compatibility.
- Work per frame is O(width × height); typical windows remain snappy at 10–30 FPS.
- The timer ticks only when the UI loop is idle; inputs preempt animation frames.
- Timers are paused when a view is not exposed, saving CPU when hidden.

### Edge Cases
- Extremely narrow or short windows: algorithm still works; `draw()` respects `size`.
- Resizes while animating: next frame uses new `size` automatically.
- Multiple windows: each has its own timer; animations remain independent.

## Testing Plan
Manual checks (MVP):
- Launch app, open View → Animated Blocks; verify animation is visible.
- Open 3+ animated windows; cascade and tile; confirm they all animate independently.
- Resize an animated window continuously; verify rows still alternate movement and draw stays aligned.
- Cover an animated window (or open dialogs) so it’s not exposed; ensure it pauses and resumes when visible.
- Sanity check CPU usage at default FPS.

Automated opportunities (future):
- Add a tiny unit test that validates the palette index mapping function for even/odd rows given `phase`.
- Add a headless draw test (if a mock TDrawBuffer is available) to assert a few cells match expected attributes.

## Build and Run
- From `test-tui/`:
  - `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build`
  - Run: `./build/test_pattern`
- In-app: View → Animated Blocks

## Implementation Checklist
- Files
  - [x] `animated_blocks_view.h/.cpp`
  - [x] Factory `createAnimatedBlocksWindow`
  - [x] Integrations in `test_pattern_app.cpp` (include header, command id, menu item, handler)
- Behavior
  - [x] Timer lifecycle tied to `sfExposed`
  - [x] Even/odd row movement directions
  - [x] 16-color repeating palette
  - [x] Resizes handled in `draw()` via `size`

## Future Work (beyond MVP)
- Parameterization UI: dialog or hotkeys to adjust speed, direction, palette.
- Global Animation Controller (optional layer):
  - Centralized clock and FPS cap; per-view registration.
  - Group start/stop and time dilation (slow/fast-mo).
- Keyframes and easing:
  - Small keyframe struct; cubic/linear easing helpers to change view parameters over time.
- Orchestration and persistence:
  - A simple timeline format (JSON/YAML) to coordinate multiple windows; import/export sequences.
- IPC hooks:
  - Extend existing API (see `api_ipc.*`) to create/close animated windows and adjust speed at runtime.

## Acceptance Criteria
- A new “Animated Blocks” menu entry exists under View and spawns an animated window.
- Rows alternate horizontal movement; first row moves right, second left, and so on.
- Animation progresses while the window is exposed; it pauses when not exposed.
- Multiple animated windows can run concurrently without interfering.
- Code follows existing patterns (naming, placement, style), and builds cleanly.

## References (Codebase)
- `test-tui/frame_file_player_view.{h,cpp}` – existing pattern for setTimer + cmTimerExpired + evBroadcast lifecycle.
- `test-tui/test_pattern_app.cpp` – main app, menu wiring, window factories.
- `test-tui/test_pattern.{h,cpp}` – color palettes and block drawing patterns.

## See Also
- Explainer: `tvision-animation-explainer.md` — a focused guide to timer-driven animations in TVision with a minimal skeleton and best practices. Pairs with this PRD and references the Animated Blocks implementation.
