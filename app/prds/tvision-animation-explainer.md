# Explainer: Timer-Driven Animations in TVision (MVP Pattern)

This document is a concise, practical guide to implementing smooth, UI-thread animations in Turbo Vision (TVision) using timers and broadcast events. It complements (but does not replace) the PRD:
- PRD: `animated-blocks-and-animation-mvp.md`
- Reference implementation: `test-tui/animated_blocks_view.{h,cpp}` (new), `test-tui/frame_file_player_view.{h,cpp}` (existing)

The goal is to enable any C++ developer to add animated views to a TVision app quickly and correctly, without threads or external schedulers.

## Why This Pattern
- UI-thread timers keep animations portable and safe for terminal UIs.
- Broadcast events integrate seamlessly with the event loop: frames render only when the UI is idle.
- Lifecycle-aware: animation pauses when hidden, resumes when shown.

## Quick Start Checklist
- Derive from `TView` and set `eventMask |= evBroadcast`.
- Start a periodic timer with `setTimer(periodMs, firstTimeoutMs)` and store the returned `TTimerId`.
- In `handleEvent`, on `evBroadcast` + `cmTimerExpired` and matching timer id, advance state and `drawView()`.
- In `setState(sfExposed, True/False)`, start/stop the timer to pause when hidden.
- Implement `draw()` to render current state using `TDrawBuffer`, `writeLine`, etc.
- Include `Uses_TEvent` in sources that inspect `TEvent` fields.

## Minimal Implementation (Skeleton)
```cpp
// my_animated_view.h
#define Uses_TView
#define Uses_TDrawBuffer
#define Uses_TRect
#include <tvision/tv.h>

class TMyAnimatedView : public TView {
public:
    explicit TMyAnimatedView(const TRect &bounds, unsigned periodMs = 100);
    ~TMyAnimatedView() override;

    void draw() override;
    void handleEvent(TEvent &ev) override;
    void setState(ushort aState, Boolean enable) override;

    void setSpeed(unsigned periodMs_);

private:
    void startTimer();
    void stopTimer();
    void advance();

    unsigned periodMs {100};
    TTimerId timerId {0};
    int phase {0};
};
```

```cpp
// my_animated_view.cpp
#include "my_animated_view.h"
#define Uses_TEvent
#include <tvision/tv.h>

TMyAnimatedView::TMyAnimatedView(const TRect &bounds, unsigned period)
    : TView(bounds), periodMs(period)
{
    growMode = gfGrowHiX | gfGrowHiY;    // or gfGrowAll
    eventMask |= evBroadcast;             // receive cmTimerExpired
}
TMyAnimatedView::~TMyAnimatedView() { stopTimer(); }

void TMyAnimatedView::setSpeed(unsigned p) {
    periodMs = p ? p : 1; if (timerId) { stopTimer(); startTimer(); }
}

void TMyAnimatedView::startTimer() {
    if (!timerId) timerId = setTimer(periodMs, (int)periodMs);
}
void TMyAnimatedView::stopTimer() {
    if (timerId) { killTimer(timerId); timerId = 0; }
}

void TMyAnimatedView::advance() { ++phase; }

void TMyAnimatedView::draw() {
    TDrawBuffer buf; const int W = size.x, H = size.y; if (W <= 0 || H <= 0) return;
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            // Example content: moving block with simple attr
            buf.moveChar(x, '\xDB', TColorAttr{0x07}, 1);
        }
        writeLine(0, y, W, 1, buf);
    }
}

void TMyAnimatedView::handleEvent(TEvent &ev) {
    TView::handleEvent(ev);
    if (ev.what == evBroadcast && ev.message.command == cmTimerExpired) {
        if (timerId && ev.message.infoPtr == timerId) {
            advance();
            drawView();
            clearEvent(ev);
        }
    }
}

void TMyAnimatedView::setState(ushort aState, Boolean enable) {
    TView::setState(aState, enable);
    if (aState & sfExposed) {
        if (enable) { phase = 0; startTimer(); drawView(); }
        else        { stopTimer(); }
    }
}
```

```cpp
// Factory (optional): my_animated_window_factory.cpp
#define Uses_TWindow
#define Uses_TRect
#include <tvision/tv.h>
#include "my_animated_view.h"

TWindow* createMyAnimatedWindow(const TRect &bounds) {
    auto *w = new TWindow(bounds, "My Animation", wnNoNumber);
    w->options |= ofTileable;
    TRect c = w->getExtent(); c.grow(-1, -1);
    w->insert(new TMyAnimatedView(c, /*periodMs=*/100));
    return w;
}
```

## Core Concepts
- Timer: `TTimerId id = setTimer(periodMs, firstTimeoutMs);` — schedules periodic callbacks on the UI thread. Stop with `killTimer(id)`.
- Broadcast: In `handleEvent`, check `ev.what == evBroadcast && ev.message.command == cmTimerExpired`. Match the specific timer via `ev.message.infoPtr == id`.
- Lifecycle: Tie work to visibility by starting/stopping timers in `setState(sfExposed, True/False)`.
- Redraw: In timer tick, call `drawView()` (not `draw()` directly) to request painting.
- Rendering: Use `TDrawBuffer` and `writeLine` per row; avoid per-frame heap allocations.

## Best Practices
- Include `Uses_TEvent` in any `.cpp` that inspects `TEvent` fields.
- Keep frame work O(width × height) and minimize allocations. Reuse `TDrawBuffer`.
- Default to 10–30 FPS (e.g., 100 ms period) for low CPU and compatible terminals.
- Use `growMode` so content adapts to resizes; read `size.{x,y}` in `draw()`.
- Clear handled timer events with `clearEvent(ev)` to avoid double-processing.
- Multiple windows: each view owns its own timer; no global manager needed for MVP.

## References (Code)
- Animated Blocks (new):
  - `test-tui/animated_blocks_view.h`
  - `test-tui/animated_blocks_view.cpp`
- Frame File Player (existing):
  - `test-tui/frame_file_player_view.h`
  - `test-tui/frame_file_player_view.cpp`
- App wiring (menu and window factories):
  - `test-tui/test_pattern_app.cpp` (command `cmAnimatedBlocks` → `createAnimatedBlocksWindow`).
- PRD:
  - `test-tui/prds/animated-blocks-and-animation-mvp.md`

## Common Pitfalls
- Missing `Uses_TEvent` causes incomplete-type errors when accessing `ev.message`.
- Forgetting `eventMask |= evBroadcast` prevents timer events from reaching your view.
- Not matching `infoPtr` to your `TTimerId` can cause cross-talk between timers.
- Doing heavy work or I/O in `handleEvent` will stutter the UI; keep it light.

## Extending the Pattern
- Speed control: expose `setSpeed(periodMs)` that restarts the timer if running.
- Centralized timing: optional global clock to coordinate multiple views (future).
- Parameterization: add hotkeys or dialogs to adjust palette, directions, or FPS at runtime.

---

This explainer is designed for creative coding with TVision: fast to copy, safe to run, and easy to extend. See the Animated Blocks code for a compact, idiomatic example.
