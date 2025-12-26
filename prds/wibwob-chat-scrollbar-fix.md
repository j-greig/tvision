# PRD: Wib&Wob Chat Window Scrollbar Fix

**tl;dr:** Chat scrollbar only visible at certain widths and doesn't fill window height because TWibWobView uses TView+manual scrollbar instead of TScroller pattern used by all working TV scrollable views. Fix: either use standardScrollBar() with correct growMode (quick fix), or refactor to inherit from TScroller (proper fix). Adding test command with static ASCII content for resize testing.

---

## Problem Statement

The Wib&Wob chat window (opened via Tools menu) has a broken scrollbar:
1. **Scrollbar only visible at specific window widths** - disappears when resizing
2. **Scrollbar doesn't fill window height** - only ~2/3 visible, stuck to lower half
3. **Previous fix attempt (commit 4e07579) failed** - removed manual `locate()` but didn't address root cause

## Root Cause Analysis

### Current Architecture (WRONG)

```
app/wibwob_view.cpp lines 677-692:

TWibWobWindow constructor:
├── Gets interior with client.grow(-1, -1)
├── Creates TScrollBar manually in client area (WRONG LOCATION)
├── Sets vScrollBar->growMode = gfGrowLoY | gfGrowHiY (WRONG FLAGS)
└── Creates TWibWobView (extends TView, not TScroller)
```

**Specific Issues:**

1. **Wrong scrollbar location**: Created inside client area instead of in frame area via `standardScrollBar()`

2. **Wrong growMode flags**: `gfGrowLoY | gfGrowHiY` means:
   - `gfGrowLoY`: TOP edge tracks parent's bottom (WRONG - should stay fixed)
   - `gfGrowHiY`: bottom edge tracks parent's bottom (correct)
   - Missing `gfGrowHiX`: right edge should track parent's right edge

3. **Wrong base class**: `TWibWobView` extends `TView` instead of `TScroller`
   - No automatic scroll event handling
   - No automatic limit/bounds synchronisation
   - The `cmScrollBarChanged` handler was REMOVED (line 235) because it caused segfaults - symptom of broken architecture

4. **Manual scroll syncing**: Uses custom `scrollOffset`, `notifyScrollBarUpdate()` - fragile and doesn't work with resize

### Correct Architecture (from ansi_view.cpp, fileview.cpp)

```
TAnsiMiniWindow (lines 208-220):
├── Creates scrollbars via standardScrollBar(sbVertical | sbHandleKeyboard)
├── Creates TAnsiMiniView which extends TScroller
├── Passes scrollbar references to TScroller constructor
└── Uses setLimit() for content bounds, delta.y for scroll position
```

**Key difference**: `standardScrollBar()` creates scrollbars in the **frame area** with correct automatic positioning and resize behaviour:

```cpp
// From twindow.cpp lines 197-210
TScrollBar *TWindow::standardScrollBar( ushort aOptions ) noexcept
{
    TRect r = getExtent();
    if( (aOptions & sbVertical) != 0 )
        r = TRect( r.b.x-1, r.a.y+1, r.b.x, r.b.y-1 );  // Frame area, skips corners
    // ...
}
```

## Solution Options

### Option A: Minimal Fix (Quick but Incomplete)

**Scope**: Fix scrollbar positioning without architectural changes

**Changes to `app/wibwob_view.cpp`:**

```cpp
// Replace lines 680-692 in TWibWobWindow constructor:

// OLD (broken):
TRect scrollBarRect = client;
scrollBarRect.a.x = scrollBarRect.b.x - 1;
vScrollBar = new TScrollBar(scrollBarRect);
vScrollBar->growMode = gfGrowLoY | gfGrowHiY;
insert(vScrollBar);

// NEW (fixed positioning):
vScrollBar = standardScrollBar(sbVertical | sbHandleKeyboard);
// standardScrollBar() handles positioning and growMode automatically
```

**Pros**: Minimal code change, fixes visible symptoms
**Cons**: Still uses TView instead of TScroller, scroll syncing remains fragile

---

### Option B: Proper Refactor (Recommended)

**Scope**: Convert TWibWobView to use TScroller pattern

**Changes:**

#### 1. wibwob_view.h - Change inheritance

```cpp
// OLD:
class TWibWobView : public TView {

// NEW:
class TWibWobView : public TScroller {
public:
    TWibWobView(const TRect& bounds, TScrollBar* hScroll, TScrollBar* vScroll);
    // Remove: scrollOffset, notifyScrollBarUpdate()
    // Use inherited: delta.y, setLimit(), scrollTo()
```

#### 2. wibwob_view.cpp - Use TScroller mechanics

```cpp
// Constructor - pass scrollbars to TScroller
TWibWobView::TWibWobView(const TRect& bounds, TScrollBar* hScroll, TScrollBar* vScroll)
    : TScroller(bounds, hScroll, vScroll) {
    // ...
}

// draw() - use delta.y instead of scrollOffset
void TWibWobView::drawMessages() {
    int startMsg = std::max(0, (int)messages.size() - maxY + delta.y);  // was scrollOffset
    // ...
}

// When content changes - use setLimit()
void TWibWobView::addMessage(...) {
    messages.push_back(msg);
    setLimit(size.x, calculateTotalWrappedLines());  // Automatic scrollbar update
}

// Remove: scrollOffset, notifyScrollBarUpdate(), manual scroll methods
// Override scrollDraw() for scroll-triggered redraws
```

#### 3. TWibWobWindow constructor

```cpp
TWibWobWindow::TWibWobWindow(const TRect& bounds, const std::string& title)
    : TWindow(bounds, title.c_str(), wnNoNumber)
    , TWindowInit(&TWibWobWindow::initFrame)
{
    options |= ofTileable;

    TRect client = getExtent();
    client.grow(-1, -1);

    // Create scrollbars using standard method
    vScrollBar = standardScrollBar(sbVertical | sbHandleKeyboard);
    // No horizontal scrollbar needed for chat

    // Pass scrollbar to view
    chatView = new TWibWobView(client, nullptr, vScrollBar);
    chatView->growMode = gfGrowHiX | gfGrowHiY;
    insert(chatView);
}
```

**Pros**:
- Proper TV architecture
- Automatic resize handling via TScroller::changeBounds()
- Automatic scroll event handling
- No manual syncing code needed
- Matches working examples (ansi_view, fileview)

**Cons**:
- More code changes
- Need to retest scroll behaviour

---

## Test Infrastructure

### New Command: Tools > Wib&Wob Chat (Test Mode)

Add a test version that opens with static content + ASCII art for resize testing:

```cpp
// In test_pattern_app.cpp, add menu item:
// Tools > "Wib&Wob Chat (Test)" - cmd cmWibWobTest

// Creates chat window pre-populated with:
void createTestChatWindow() {
    auto* win = createWibWobWindow(...);
    auto* view = /* get chatView */;

    // Static test content - 2 large ASCII arts with text between
    view->addMessage("System", "=== SCROLLBAR TEST MODE ===");
    view->addMessage("Wib", R"(
    /\_/\
   ( o.o )
    > ^ <
   /|   |\
  ( |   | )
    "---"
    CAT ONE
)");
    view->addMessage("Wob", "This is text between the ASCII arts to test scroll behavior with mixed content types.");
    view->addMessage("Wib", R"(
      ___
     /   \
    | o_o |
    |  >  |
    /_____\
   |       |
   |_______|
   ROBOT HEAD
)");
    // Add 20+ more messages to ensure scrollable content
    for (int i = 0; i < 25; i++) {
        view->addMessage("Test", "Line " + std::to_string(i) + ": Lorem ipsum dolor sit amet");
    }
}
```

### Manual Test Protocol

1. Open chat window (test mode)
2. Resize window by dragging corners/edges
3. Verify at each size:
   - [ ] Scrollbar visible and fills full window height
   - [ ] Scrollbar thumb position reflects content position
   - [ ] Keyboard scroll (Up/Down/PgUp/PgDn) works
   - [ ] Mouse click on scrollbar works
   - [ ] Content redraws correctly after resize

---

## Implementation Steps

### Phase 1: Quick Fix (Option A) - Est. 30 mins

- [ ] Replace manual scrollbar creation with `standardScrollBar()`
- [ ] Remove custom growMode assignment
- [ ] Build and test resize behaviour
- [ ] If fixed, commit with tag "quick-fix"

### Phase 2: Proper Refactor (Option B) - Est. 2-3 hours

If Phase 1 doesn't fully resolve issues:

1. [ ] Modify `TWibWobView` to extend `TScroller`
2. [ ] Update constructor to take scrollbar refs
3. [ ] Replace `scrollOffset` with `delta.y`
4. [ ] Replace `notifyScrollBarUpdate()` with `setLimit()`
5. [ ] Remove manual scroll syncing code
6. [ ] Update `draw()` to use TScroller delta
7. [ ] Override `scrollDraw()` for scroll redraws
8. [ ] Update `TWibWobWindow` to use new pattern
9. [ ] Build and test thoroughly

### Phase 3: Test Mode - Est. 1 hour

- [ ] Add `cmWibWobTest` command
- [ ] Create test window with static ASCII content
- [ ] Add to Tools menu
- [ ] Document test protocol

---

## Files to Modify

| File | Changes |
|------|---------|
| `app/wibwob_view.h` | Change inheritance TView → TScroller, update constructor |
| `app/wibwob_view.cpp` | Use TScroller mechanics, remove manual scroll code |
| `app/test_pattern_app.cpp` | Add test mode menu item and handler |

---

## Success Criteria

- [ ] Scrollbar visible at ALL window widths (not just specific widths)
- [ ] Scrollbar fills full height between frame top and bottom
- [ ] Scrollbar thumb position reflects content scroll state
- [ ] Resize works smoothly without scrollbar disappearing
- [ ] Keyboard and mouse scroll continue to work
- [ ] No segfaults or crashes

---

## Reference Files

- `app/ansi_view.cpp` - Correct TScroller pattern (lines 32-37, 208-220)
- `workings/examples/tvdemo/fileview.cpp` - Standard file viewer with scrollbars
- `source/tvision/tscrolle.cpp` - TScroller implementation
- `source/tvision/twindow.cpp:197-210` - standardScrollBar() implementation
- `include/tvision/views.h:91-99` - growMode flags documentation

---

**Author**: Claude (Wob)
**Date**: 2025-12-01
**Status**: Ready for Implementation
