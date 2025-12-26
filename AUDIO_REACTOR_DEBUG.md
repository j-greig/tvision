# Audio Reactor POC - Debug Analysis

## CRITICAL MISCONCEPTION TO ADDRESS

**THERE IS NO SOUND YET** - This is a PROOF-OF-CONCEPT with:
- ❌ NO actual audio file loading
- ❌ NO SDL2 audio playback
- ❌ NO real FFT analysis
- ✅ ONLY mock sine wave data visualization

The spectrum bars should still be **animating** with fake data, but there's no actual audio.

---

## Expected Code Flow (Pseudocode)

```
WINDOW CREATION:
1. User selects "View → Audio Reactor"
2. newAudioReactorWindow() called
   → Creates TAudioReactorWindow with bounds
   → Window calculates interior rect (shrunk by frame)
   → Creates TAudioReactorView(interior)
   → Inserts view into window
   → Calls audioView->play()

VIEW INITIALIZATION (TAudioReactorView constructor):
3. TView(bounds) - parent constructor
4. Set growMode, eventMask |= evBroadcast
5. Initialize freqBands[] to zero
6. Start timer: timerId = setTimer(50ms, 50ms)
7. Timer should fire every 50ms

ANIMATION LOOP:
8. Every 50ms: Turbo Vision broadcasts cmTimerExpired
9. handleEvent() receives evBroadcast with cmTimerExpired
10. Calls updateFrequencyAnalysis()
    → mockFFTAnalysis()
    → Updates freqBands[] with sine wave data (0.2 - 1.0)
11. Calls drawView()
    → Triggers draw()

RENDERING (draw() method):
12. Draw magenta test line at y=0
13. drawSpectrum() - draws 8 colored bars
14. drawInfo() - draws status line at bottom
15. Each line written via writeLine(x, y, width, height, buffer)
```

---

## Why Is Window Black? - Ranked Hypotheses

### TIER 1 - MOST LIKELY (View Setup Issues)

**#1 - View size is invalid (size.x or size.y == 0)**
- **Evidence**: If interior.grow(-1,-1) on tiny window → size becomes 0
- **Test**: Add debug output to draw() to print size.x, size.y
- **Fix**: Ensure minimum window size, add size validation

**#2 - View not being drawn initially**
- **Evidence**: Timer fires after 50ms, but initial state is blank
- **Test**: Call drawView() explicitly in constructor
- **Fix**: Add `drawView()` at end of TAudioReactorView constructor

**#3 - View not exposed/visible**
- **Evidence**: Turbo Vision requires views to be "exposed" to render
- **Test**: Check if setState(sfExposed, true) needed
- **Fix**: Mark view as exposed after insertion

### TIER 2 - POSSIBLE (Rendering Issues)

**#4 - TDrawBuffer not being populated correctly**
- **Evidence**: moveChar() might not be working as expected
- **Test**: Try moveStr() with text instead of colored blocks
- **Fix**: Use simpler rendering approach with text characters

**#5 - RGB colors not rendering (terminal compatibility)**
- **Evidence**: Even test magenta line not showing
- **Test**: Try BIOS palette colors instead of TColorRGB
- **Fix**: Use palette indices (0x0E = yellow, etc.)

**#6 - writeLine() coordinates wrong**
- **Evidence**: Writing outside view bounds
- **Test**: Verify 0 <= x,y < size.x, size.y
- **Fix**: Add bounds checking

### TIER 3 - UNLIKELY (Timer/Event Issues)

**#7 - Timer not firing**
- **Evidence**: setTimer() returns 0 on failure
- **Test**: Check if timerId != 0 after setTimer()
- **Fix**: Verify timer system working

**#8 - Events not being handled**
- **Evidence**: handleEvent not receiving cmTimerExpired
- **Test**: Add debug output in handleEvent()
- **Fix**: Check eventMask setup

---

## Immediate Action Plan

### Quick Test #1 - Verify View Exists
Add to draw():
```cpp
void TAudioReactorView::draw()
{
    // EMERGENCY DEBUG - Draw pure text
    TDrawBuffer b;
    const char* msg = "VIEW IS RENDERING!";

    for (int i = 0; msg[i]; i++) {
        b.moveChar(i, msg[i], 0x0E, 1);  // Yellow text
    }
    writeLine(0, 0, strlen(msg), 1, b);
}
```

### Quick Test #2 - Check Size
Add at start of draw():
```cpp
// Debug: Print size to stderr (won't interfere with TUI)
fprintf(stderr, "AudioReactor draw(): size.x=%d, size.y=%d\n", size.x, size.y);
fflush(stderr);
```

### Quick Test #3 - Force Initial Draw
Add to constructor:
```cpp
TAudioReactorView::TAudioReactorView(const TRect& bounds) : /*...*/ {
    // ... existing code ...

    // FORCE initial render
    drawView();
}
```

---

## Root Cause Suspects - Ranked

1. **View size is 0 or invalid** (80% likely)
2. **Initial draw not triggered** (70% likely)
3. **RGB color rendering broken** (40% likely)
4. **Timer not firing** (20% likely)
5. **Wrong coordinate system** (10% likely)

---

## Next Steps

1. Run Quick Test #2 first (add size debug output)
2. Look at stderr when running: `./build/app/test_pattern 2>audio_debug.log`
3. Open window, check log file for size values
4. Based on size output, apply appropriate fix
