# Animated Kaomoji Menu Bar

**tl;dr:** Autonomous blinking + 8 emotion states for menu bar kaomoji つ◕‿◕‿◕༽つ with event-driven mood changes (LLM thinking, tool use, errors). Blinking works now, event hooks ready to wire up.

---

## Status: Blinking Complete, Event Hooks Ready

### Completed ✅

1. **Autonomous Blinking Animation**
   - Random blink intervals (3-6 seconds)
   - 150ms blink duration つ-‿-‿-༽つ
   - Blinking overrides mood states

2. **8 Kaomoji Mood States**
   - つ◕‿◕‿◕༽つ NEUTRAL (default)
   - つ◉‿◉‿◉༽つ EXCITED (tool use, window spawning)
   - つ●‿●‿●༽つ THINKING (LLM processing)
   - つ◡‿◡‿◡༽つ SLEEPY (idle)
   - つ○‿○‿○༽つ CURIOUS (user input)
   - つ■‿■‿■༽つ MEMORY (symbient-brain tool)
   - つ□‿□‿□༽つ GEOMETRIC (pattern mode changes)
   - つ◎‿◎‿◎༽つ SURPRISED (errors)

3. **Automatic Mood Reversion**
   - Moods expire after configurable duration (default 2000ms)
   - Auto-reverts to NEUTRAL

4. **Update Loop Integration**
   - `TTestPatternApp::idle()` calls `menuBar->update()` every frame
   - Smooth animation without blocking

5. **Helper Method**
   - `app.setKaomojiMood(mood, durationMs)` for easy triggering

---

## Implementation Details

### Files Modified

**test-tui/test_pattern_app.cpp**
- Lines 176-290: Enhanced `TCustomMenuBar` class with animation state
- Lines 544-552: Added `setKaomojiMood()` helper method
- Lines 1711-1717: Added `update()` call in `idle()` loop

### Architecture

```cpp
TCustomMenuBar (menu bar)
  ├─ KaomojiMood enum (8 states)
  ├─ Blink timer (3-6s random)
  ├─ Mood timer (auto-revert)
  └─ draw() override (renders current state)

TTestPatternApp
  └─ setKaomojiMood(mood, duration) → triggers state change
```

### Usage Examples

```cpp
// When LLM starts processing
setKaomojiMood(TCustomMenuBar::THINKING);

// When symbient-brain tool executes
setKaomojiMood(TCustomMenuBar::MEMORY, 3000);

// When windows spawn
setKaomojiMood(TCustomMenuBar::EXCITED, 1500);

// On errors
setKaomojiMood(TCustomMenuBar::SURPRISED, 2500);
```

---

## Pending: Event Hooks (Ready to Wire Up)

### Where to Add Mood Triggers

1. **LLM Response Callback** (wibwob_view.cpp)
   ```cpp
   // Line ~318: Before sendQuery
   TProgram::application->setKaomojiMood(TCustomMenuBar::THINKING);

   // Line ~326: In response callback
   if (response.is_error) {
       TProgram::application->setKaomojiMood(TCustomMenuBar::SURPRISED);
   } else {
       TProgram::application->setKaomojiMood(TCustomMenuBar::NEUTRAL);
   }
   ```

2. **Tool Execution Detection** (wibwob_engine.cpp)
   ```cpp
   // Line ~64: Tool execution wrapper
   if (response.needs_tool_execution) {
       // Check tool names in response.tool_calls
       for (const auto& toolCall : response.tool_calls) {
           if (toolCall.name.find("save_memory") != std::string::npos) {
               TProgram::application->setKaomojiMood(TCustomMenuBar::MEMORY, 3000);
           }
       }
   }
   ```

3. **Window Spawning** (test_pattern_app.cpp)
   ```cpp
   // In newTestWindow(), newGradientWindow(), etc.
   setKaomojiMood(TCustomMenuBar::EXCITED, 1500);
   ```

4. **Pattern Mode Changes** (test_pattern_app.cpp)
   ```cpp
   // In setPatternMode()
   setKaomojiMood(TCustomMenuBar::GEOMETRIC, 2000);
   ```

5. **User Input Detection** (wibwob_view.cpp)
   ```cpp
   // In processInput() when user types
   setKaomojiMood(TCustomMenuBar::CURIOUS, 1000);
   ```

---

## Technical Notes

### Timing Strategy

- **Blink**: 3-6s random intervals (human-like)
- **Mood duration**: Configurable per-event (default 2s)
- **Update rate**: Every idle() call (~60fps)

### Thread Safety

- All state changes happen on UI thread via `idle()`
- No mutex needed - single-threaded event loop

### Performance

- Negligible overhead: simple state machine + string lookup
- No additional timers beyond existing `idle()` loop

---

## Future Enhancements

### Sentiment Analysis (Optional)

Parse LLM response text for emotional keywords:
- "error", "failed" → SURPRISED
- "excited", "amazing" → EXCITED
- "thinking", "processing" → THINKING

### Multi-Character States (Optional)

Wib (left kaomoji) vs Wob (right kaomoji) with independent moods:
```
つ◕‿●‿◕༽つ  // Wib neutral, Wob thinking
```

### Animation Sequences (Optional)

Chain multiple moods for complex expressions:
```cpp
// "Aha!" moment
setKaomojiMood(SURPRISED, 500);  // つ◎‿◎‿◎༽つ
// then auto-transitions to
setKaomojiMood(EXCITED, 1500);   // つ◉‿◉‿◉༽つ
```

---

## Testing

### Manual Test

1. Build: `cmake --build test-tui/build`
2. Run: `cd test-tui && ./build/test_pattern`
3. Observe: Kaomoji blinks every 3-6 seconds autonomously
4. Test hook: Add `setKaomojiMood(EXCITED)` to any menu action

### Expected Behavior

- つ◕‿◕‿◕༽つ displays in top-right menu bar
- Blinks to つ-‿-‿-༽つ briefly every few seconds
- Moods change when `setKaomojiMood()` is called
- Auto-reverts to neutral after duration expires

---

## Related Work

- **MCP Integration**: Chat window already has LLM callbacks (wibwob_view.cpp:326)
- **Tool Registry**: Tool execution wrapper exists (wibwob_engine.cpp:64)
- **Window Management**: Window creation hooks available (test_pattern_app.cpp:1079+)

All the infrastructure is ready - just need to sprinkle `setKaomojiMood()` calls at the right moments.

---

## Git Commit Summary

**Added:**
- Animated kaomoji with autonomous blinking (3-6s intervals)
- 8 distinct mood states with Unicode eye variations
- Event hook infrastructure ready for LLM/tool/UI events
- Helper method for triggering mood changes

**Next Steps:**
- Wire up LLM response callbacks → THINKING/SURPRISED moods
- Detect tool names in MCP responses → MEMORY/GEOMETRIC moods
- Hook window spawning → EXCITED mood
- Add user input detection → CURIOUS mood
