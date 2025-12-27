# DOOM ASCII Keyboard Input Fix

**tl;dr:** Current implementation only sends key PRESS events to DOOM, missing key RELEASE events. DOOM thinks keys are held forever → turning continues after releasing arrow, gets stuck after ~5s. Fix: Handle TurboVision's `evKeyUp` events and pack press/release flag into input queue.

## Problem Statement

**Observed Symptoms:**
1. ✅ Menus work (Space/Enter to select)
2. ❌ Movement doesn't work (arrows don't move player in-game)
3. ❌ Keys "stick" - turning continues after releasing arrow (in first attempt at fix)
4. ❌ Game becomes unresponsive after ~5 seconds

**Root Cause:**
DOOM requires **paired press/release events** for every key. Current tvision port only sends **press events**, causing DOOM to think keys are held infinitely.

## Current Architecture

### Event Flow

```
User Keyboard
    ↓
TurboVision Event System
    ├─ evKeyDown  ✅ (currently handled)
    └─ evKeyUp    ❌ (MISSING - root cause!)
    ↓
TDoomAsciiView::handleEvent()
    ↓
inputQueue: vector<uint8_t>  ❌ (only stores key codes, no press/release flag)
    ↓
doomInputQueuePop() → DG_GetKey()
    ↓
*pressed = 1;  ❌ (always 1 - hardcoded!)
    ↓
DOOM i_input.c:I_GetEvent()
    ├─ Receives: ev_keydown
    └─ NEVER receives: ev_keyup  ❌
    ↓
RESULT: DOOM thinks key held forever
```

### Code Analysis

**File: `app/doom_ascii_view.cpp:264-278`** (CURRENT - BROKEN)
```cpp
void TDoomAsciiView::handleEvent(TEvent &ev)
{
    TView::handleEvent(ev);

    // ❌ ONLY handles evKeyDown, NOT evKeyUp!
    if (ev.what == evKeyDown) {
        unsigned char doomKey = mapTVKeyToDoom(ev.keyDown.keyCode);
        if (doomKey != 0) {
            inputQueue.push_back(doomKey);  // ❌ No press/release flag!
            clearEvent(ev);
        }
    }
    // Timer handling...
}
```

**File: `app/doom/doomgeneric_tvision.c:125-137`** (CURRENT - BROKEN)
```cpp
int DG_GetKey(int* pressed, unsigned char* doomKey)
{
    if (doomInputQueueEmpty()) {
        *pressed = 0;
        return 0;
    }

    *pressed = 1;  // ❌ HARDCODED - always press, never release!
    *doomKey = doomInputQueuePop();
    return 1;
}
```

## Research: How DOOM Input Works

### DOOM's Expected Interface (from i_input.c:280-326)

```c
void I_GetEvent(void)
{
    event_t event;
    int pressed;
    unsigned char key;

    DG_ReadInput();  // Platform-specific input poll

    // ❗ DOOM polls in a loop until no more events
    while (DG_GetKey(&pressed, &key))
    {
        if (pressed) {
            event.type = ev_keydown;
            event.data1 = TranslateKey(key);
            D_PostEvent(&event);
        }
        else {
            event.type = ev_keyup;  // ❗ REQUIRED FOR KEY RELEASE
            event.data1 = TranslateKey(key);
            D_PostEvent(&event);
            break;  // Process one release per tick
        }
    }
}
```

**Key Insight:** DOOM expects `DG_GetKey()` to return:
- `pressed=1, key=X` when key X is pressed
- `pressed=0, key=X` when key X is released

### Reference Implementation: doom-ascii (doomgeneric_ascii.c:624-715)

```c
static struct timespec input_buffer[256];  // Timestamp per key
static unsigned keypress_smoothing_ms = 42;  // Temporal smoothing

void DG_ReadInput(void)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    for (int i = 1; i < 256; i++) {
        if (!input_buffer[i].tv_sec)
            continue;

        // RELEASE: If key held > 42ms, generate release event
        if (sub_timespec_ms(&now, &input_buffer[i]) > keypress_smoothing_ms) {
            input_buffer[i] = (struct timespec){ 0 };
            event_buffer[idx++] = { .key = i, .pressed = false };  // ❗ RELEASE
        }

        // PRESS: If key newly pressed this frame
        if (!prev_input_buffer[i].tv_sec) {
            event_buffer[idx++] = { .key = i, .pressed = true };   // ❗ PRESS
        }
    }
}
```

**Pattern:** doom-ascii uses temporal smoothing (42ms window) because raw terminal input doesn't provide key-up events.

### Reference Implementation: SDL/Win32 Ports

```c
#define KEYQUEUE_SIZE 128
static unsigned short s_KeyQueue[KEYQUEUE_SIZE];  // Packed: (pressed<<8)|keycode

int DG_GetKey(int *pressed, unsigned char *doomKey)
{
    if (s_KeyQueueReadIndex == s_KeyQueueWriteIndex)
        return 0;  // Empty

    unsigned short keyData = s_KeyQueue[s_KeyQueueReadIndex++];
    s_KeyQueueReadIndex %= KEYQUEUE_SIZE;

    *pressed = keyData >> 8;      // Upper byte: 0=release, 1=press
    *doomKey = keyData & 0xFF;    // Lower byte: key code
    return 1;
}
```

**Pattern:** SDL/Win32 pack press/release flag and key code into single uint16_t.

## TurboVision Event System

### Available Events (CORRECTED)

**❌ CRITICAL DISCOVERY:** This TurboVision fork does NOT have `evKeyUp`!

**From include/tvision/system.h:30-42:**
```cpp
const int evMouseDown   = 0x0001;
const int evMouseUp     = 0x0002;
const int evMouseMove   = 0x0004;
const int evMouseAuto   = 0x0008;
const int evMouseWheel  = 0x0020;  // ❗ NOT evKeyUp!
const int evKeyDown     = 0x0010;  // ✅ Key press ONLY
const int evCommand     = 0x0100;
const int evBroadcast   = 0x0200;

const int evKeyboard    = 0x0010;  // ❗ ONLY evKeyDown, not evKeyDown|evKeyUp
```

**Implication:** We ONLY get press events, NEVER release events from TurboVision!

### Event Structure

```cpp
struct TEvent
{
    ushort what;  // evKeyDown or evKeyUp
    union {
        struct KeyDownEvent {
            union KeyDownStruct {
                ushort keyCode;       // Key code (e.g., kbUp = 0x0500)
                struct CharScan {
                    unsigned char charCode;
                    unsigned char scanCode;
                };
            } keyDown;
            ushort controlKeyState;
        };
        // ... other event types
    };
};
```

### Current Subscription (doom_ascii_view.cpp:87)

```cpp
// In TDoomAsciiView constructor
options |= ofSelectable;
eventMask |= evBroadcast | evKeyboard;  // ✅ Subscribes to keyboard events
```

**Note:** `evKeyboard = evKeyDown | evKeyUp` - we get BOTH, but only handle evKeyDown!

## Solution Design

### ~~Option 1: Direct Event Mapping~~ (NOT POSSIBLE)

**❌ REJECTED:** This TurboVision fork has no `evKeyUp` event, so we cannot directly map press/release events.

### Option 1: Key State Tracking + Auto-Release (RECOMMENDED)

**Approach:** Track which keys are currently "down", auto-generate release events after a short delay.

**Advantages:**
- ✅ Works with evKeyDown-only event system
- ✅ Simpler than temporal smoothing (no timestamps)
- ✅ Responsive - releases generate after 3-5 frames (~100-150ms)
- ✅ Follows doom-ascii pattern (temporal smoothing)

**Implementation:**

**Step 1: Add key state tracking**

`app/doom_ascii_view.h`:
```cpp
private:
    // Track key state: key -> frames since pressed (0 = not pressed)
    std::map<unsigned char, int> keyPressedFrames;
    static const int KEY_RELEASE_DELAY = 4;  // Release after 4 frames (~120ms at 30 FPS)

public:
    // Input queue - stores (pressed << 8) | keyCode
    std::vector<uint16_t> inputQueue;  // Changed from vector<uint8_t>
```

**Step 2: Handle evKeyDown - queue press + track state**

`app/doom_ascii_view.cpp`:
```cpp
void TDoomAsciiView::handleEvent(TEvent &ev)
{
    TView::handleEvent(ev);

    // ✅ CRITICAL: Set ofSelectable so view receives keyboard focus!
    if (!options & ofSelectable) {
        options |= ofSelectable;
    }

    // Handle key press
    if (ev.what == evKeyDown) {
        unsigned char doomKey = mapTVKeyToDoom(ev.keyDown.keyCode);
        if (doomKey != 0) {
            // Only queue press if key wasn't already down (no repeat)
            if (keyPressedFrames[doomKey] == 0) {
                uint16_t data = (1 << 8) | doomKey;  // pressed=1
                inputQueue.push_back(data);
                keyPressedFrames[doomKey] = 1;  // Mark as pressed
            }
            clearEvent(ev);
        }
    }

    // Timer handling...
}
```

**Step 3: Auto-generate releases in advance()**

`app/doom_ascii_view.cpp`:
```cpp
void TDoomAsciiView::advance()
{
    // Call DOOM tick
    D_DoomTick();

    // Auto-release keys after delay
    for (auto it = keyPressedFrames.begin(); it != keyPressedFrames.end(); ) {
        if (it->second > 0) {
            it->second++;  // Increment frame counter

            if (it->second >= KEY_RELEASE_DELAY) {
                // Generate release event
                uint16_t data = (0 << 8) | it->first;  // pressed=0
                inputQueue.push_back(data);
                it = keyPressedFrames.erase(it);  // Remove from map
                continue;
            }
        }
        ++it;
    }

    // Convert RGB buffer to ASCII
    convertFrameBuffer();
}
```

**Step 3: Update DG_GetKey to unpack press/release**

`app/doom/doomgeneric_tvision.c`:
```cpp
int DG_GetKey(int* pressed, unsigned char* doomKey)
{
    if (doomInputQueueEmpty()) {
        *pressed = 0;
        return 0;
    }

    uint16_t data = doomInputQueuePop();  // Now returns uint16_t
    *pressed = data >> 8;                 // Upper byte: 0 or 1
    *doomKey = data & 0xFF;               // Lower byte: key code
    return 1;
}
```

**Step 4: Update accessor signature**

`app/doom_ascii_view.cpp`:
```cpp
extern "C" {

uint16_t doomInputQueuePop(void)  // Changed from unsigned char
{
    if (!g_activeDoomView || g_activeDoomView->inputQueue.empty())
        return 0;

    uint16_t data = g_activeDoomView->inputQueue.front();
    g_activeDoomView->inputQueue.erase(g_activeDoomView->inputQueue.begin());
    return data;
}

}
```

### Option 2: Temporal Smoothing (NOT RECOMMENDED)

Simulate doom-ascii's temporal approach - queue press immediately, auto-generate release after delay.

**Disadvantages:**
- ❌ Complex - requires timer tracking per key
- ❌ Higher latency - artificial delay before release
- ❌ Redundant - TurboVision already provides releases
- ❌ More code to maintain

**Only use if TurboVision doesn't reliably send evKeyUp** (testing required).

## Testing Plan

### Test Cases

1. **Arrow Keys - Movement**
   - Press UP → should move forward
   - Release UP → should STOP moving
   - Expected log: `pressed=1, key=0xad` then `pressed=0, key=0xad`

2. **Arrow Keys - Turning**
   - Press LEFT → should turn left
   - Release LEFT → should STOP turning
   - No "sticky" turning after release

3. **Fire Key (F or Ctrl)**
   - Press F → should fire once
   - Hold F → should fire repeatedly (auto-fire)
   - Release F → should stop firing

4. **Menu Navigation**
   - Press ENTER → should activate menu item
   - Press UP/DOWN → should change selection
   - Should NOT stick or repeat unintentionally

5. **Long Play Session**
   - Play for 60+ seconds
   - Should NOT freeze or become unresponsive
   - Input queue should stay manageable size

### Debug Logging

Add temporary logging to verify events:

```cpp
fprintf(stderr, "[DOOM] evKeyDown: tvKey=0x%04x, doomKey=0x%02x, queued: (1<<8)|0x%02x\n",
        ev.keyDown.keyCode, doomKey, doomKey);

fprintf(stderr, "[DOOM] evKeyUp: tvKey=0x%04x, doomKey=0x%02x, queued: (0<<8)|0x%02x\n",
        ev.keyDown.keyCode, doomKey, doomKey);

fprintf(stderr, "[DOOM] DG_GetKey: pressed=%d, key=0x%02x\n", *pressed, *doomKey);
```

### Success Criteria

- ✅ Can navigate menus (UP/DOWN/ENTER work)
- ✅ Can move in-game (arrows start AND stop movement)
- ✅ No sticky keys (releasing stops action immediately)
- ✅ Can play for 60+ seconds without freezing
- ✅ Input feels responsive (< 50ms latency)

## Implementation Checklist

- [ ] Change `inputQueue` from `vector<uint8_t>` to `vector<uint16_t>`
- [ ] Add `evKeyUp` handling in `handleEvent()`
- [ ] Pack press/release flag: `(pressed << 8) | keyCode`
- [ ] Update `doomInputQueuePop()` to return `uint16_t`
- [ ] Update `DG_GetKey()` to unpack `pressed` and `key`
- [ ] Add debug logging for press/release events
- [ ] Test all 5 test cases above
- [ ] Remove debug logging after verification
- [ ] Commit fix with clear description

## Risk Analysis

**Low Risk:**
- TurboVision's `evKeyUp` is well-tested (used throughout framework)
- Pattern matches reference implementations (SDL/Win32)
- Simple change - 4 files, ~20 lines modified

**Potential Issues:**
1. **If evKeyUp doesn't fire reliably** → fallback to temporal smoothing
2. **If queue grows unbounded** → add circular buffer (like SDL port)
3. **If multiple keys pressed simultaneously** → should work (queue is FIFO)

## References

- **DOOM Source:** `third_party/doom-ascii/src/i_input.c:280-326` (I_GetEvent)
- **doom-ascii Reference:** `third_party/doom-ascii/src/doomgeneric_ascii.c:624-726`
- **TurboVision Events:** `include/tvision/system.h` (TEvent structure)
- **Current Broken Code:** `app/doom_ascii_view.cpp:264`, `app/doom/doomgeneric_tvision.c:125`

---

**Status:** Research complete, solution designed, ready for implementation.

**Estimated Time:** 30 minutes (straightforward code changes + testing)

**Complexity:** Low (direct event mapping, no state machines needed)
