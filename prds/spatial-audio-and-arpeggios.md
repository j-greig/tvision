# Spatial Audio Panning + Melody Arpeggios

**Created**: 2025-12-26
**Status**: Planning
**Priority**: P1 - User Request

---

## TL;DR

**Feature 0**: Melody window uses inverse monochrome scheme (light shades at bottom → dark at top, opposite of bass)
**Feature 1**: Window X position controls stereo panning (left screen = left pan, right screen = right pan, middle = center)
**Feature 2**: Melody window occasionally enters arpeggio mode (<20% probability) playing rising 1/4 notes

**Implementation order**: Feature 0 + 1 first (test), then Feature 2 if approved

---

## Feature 0: Inverse Color Scheme for Melody Window

### Rationale
Visual differentiation between bass (dark bottom → light top) and melody (light bottom → dark top) layers

### Current State
Both windows use same monochrome gradient:
```
Bottom 25%: █ full block (intensity 1.0)
25-50%:     ▓ dark shade (intensity 0.9)
50-75%:     ▒ medium shade (intensity 0.8)
Top 25%:    ░ light shade (intensity 0.7)
```

### Target State
**Bass window** (unchanged):
```
Bottom: █ brightest (1.0)
Top:    ░ dimmest (0.7)
```

**Melody window** (inverted):
```
Bottom: ░ dimmest (0.7)
Top:    █ brightest (1.0)
```

### Implementation

- [ ] **Step 1**: Add `bool invertGradient` parameter to `drawSpectrum()`
- [ ] **Step 2**: In `TAudioReactorView`, set `invertGradient = (engineLayer == "melody")`
- [ ] **Step 3**: In `drawSpectrum()`, reverse `positionInBar` calculation for inverted mode
  ```cpp
  if (invertGradient) {
      positionInBar = 1.0f - positionInBar;  // Flip gradient
  }
  ```
- [ ] **Step 4**: Test bass window unchanged, melody window inverted

**Files to modify:**
- `app/audio_reactor.cpp` (drawSpectrum method)
- `app/audio_reactor.h` (add invertGradient member)

---

## Feature 1: Spatial Audio Panning Based on Window Position

### Rationale
Windows positioned on left side of screen should sound left-panned, right side right-panned, creating spatial audio illusion matching visual layout

### Current State
- Each voice has static `pan` value (-1 to +1) assigned at initialization
- Pan values are random and don't change
- No relationship between window position and audio panning

### Target State
- Window X position dynamically controls **all voices' pan** for that engine
- Left edge of screen (x=0) → hard left pan (-1.0)
- Right edge of screen (x=screenWidth) → hard right pan (+1.0)
- Center of screen → center pan (0.0)
- Updates when window moves

### Pan Calculation
```cpp
// Normalize window center X to [-1, +1] range
float windowCenterX = windowBounds.a.x + (windowBounds.b.x - windowBounds.a.x) / 2;
float screenCenterX = screenWidth / 2;
float panPosition = (windowCenterX - screenCenterX) / screenCenterX;
panPosition = std::max(-1.0f, std::min(1.0f, panPosition));
```

### Implementation

#### Phase 1: Add Pan Control to GenerativeMusicEngine

- [ ] **Step 1.1**: Add `void setGlobalPan(float pan)` method to `GenerativeMusicEngine`
  - Updates all voices' `pan` member
  - Clamps to [-1.0, +1.0] range

- [ ] **Step 1.2**: Add `float globalPan` member to `GenerativeMusicEngine`
  - Default = 0.0 (center)
  - Applied to all voice outputs

**Files to modify:**
- `app/generative_music.h` - Add setGlobalPan() declaration
- `app/generative_music.cpp` - Implement setGlobalPan()

#### Phase 2: Calculate Window Position Pan

- [ ] **Step 2.1**: Add `void updateSpatialPan()` method to `TAudioReactorView`
  - Gets window bounds from parent `TWindow`
  - Gets screen width from desktop
  - Calculates normalized pan position
  - Calls `genMusic->setGlobalPan(panPosition)`

- [ ] **Step 2.2**: Override `void changeBounds(const TRect& bounds)` in `TAudioReactorView`
  - Call parent `TView::changeBounds(bounds)`
  - Call `updateSpatialPan()` to recalculate pan on move/resize

- [ ] **Step 2.3**: Call `updateSpatialPan()` in constructor after engine creation

**Files to modify:**
- `app/audio_reactor.h` - Add updateSpatialPan(), override changeBounds()
- `app/audio_reactor.cpp` - Implement spatial pan logic

#### Phase 3: Get Window Bounds from Parent

- [ ] **Step 3.1**: In `TAudioReactorView::updateSpatialPan()`:
  ```cpp
  TWindow* parentWindow = dynamic_cast<TWindow*>(owner);
  if (parentWindow) {
      TRect windowBounds = parentWindow->getBounds();
      // Calculate pan from windowBounds.a.x, windowBounds.b.x
  }
  ```

- [ ] **Step 3.2**: Get screen width from `TProgram::deskTop->size.x`

**Files to modify:**
- `app/audio_reactor.cpp` - Implement window bounds detection

#### Phase 4: Testing Checklist

- [ ] **Test 4.1**: Create bass window on left edge → verify hard left pan
- [ ] **Test 4.2**: Move bass window to center → verify center pan
- [ ] **Test 4.3**: Move bass window to right edge → verify hard right pan
- [ ] **Test 4.4**: Create melody window on right → verify right pan
- [ ] **Test 4.5**: Move melody window to left → verify pan follows
- [ ] **Test 4.6**: Create both windows at different positions → verify independent panning

**Expected behavior:**
- Window at x=0 should sound entirely in left speaker
- Window at center should sound equally in both speakers
- Window at right edge should sound entirely in right speaker
- Moving window updates pan in real-time

---

## Feature 2: Melody Arpeggio Mode (PENDING USER APPROVAL)

### Rationale
Occasional melodic variation adds interest without becoming annoying. Rare enough to be a pleasant surprise rather than constant distraction.

### Current State
- Melody voices evolve randomly within scale
- All voices change at independent 10-25 second intervals
- No rhythmic patterns

### Target State
- Melody window occasionally enters "arpeggio mode" (<20% probability)
- When active, plays rising 1/4 note pattern through current scale
- Duration: 2-4 bars (2-4 seconds at 120 BPM)
- Returns to normal random evolution after arpeggio completes

### Arpeggio Pattern
```
120 BPM = 500ms per quarter note
Pattern: Scale[0] → Scale[1] → Scale[2] → Scale[3] (ascending)
Duration: 4 × 500ms = 2 seconds
```

### Implementation

#### Phase 1: Add Arpeggio State to Voice

- [ ] **Step 1.1**: Add to `Voice` struct:
  ```cpp
  bool arpeggioMode;        // Currently in arpeggio
  int arpeggioStep;         // Current note index (0-3)
  int arpeggioFrames;       // Frames until next note change
  float arpeggioDuration;   // Frames per note (500ms at 44.1kHz = 22050)
  ```

- [ ] **Step 1.2**: Initialize in Voice constructor:
  ```cpp
  arpeggioMode = false;
  arpeggioStep = 0;
  arpeggioFrames = 0;
  arpeggioDuration = 22050.0f; // 500ms
  ```

**Files to modify:**
- `app/generative_music.h` - Add arpeggio members to Voice struct

#### Phase 2: Arpeggio Trigger Logic

- [ ] **Step 2.1**: In `GenerativeMusicEngine::generate()`, when voice `changeTimer` expires:
  ```cpp
  if (engineLayer == "melody") {  // Only melody layer
      std::uniform_real_distribution<float> arpDist(0.0f, 1.0f);
      if (arpDist(rng) < 0.20f) {  // 20% chance
          voice.arpeggioMode = true;
          voice.arpeggioStep = 0;
          voice.arpeggioFrames = 0;
      }
  }
  ```

- [ ] **Step 2.2**: Track engine layer in `GenerativeMusicEngine`:
  ```cpp
  std::string engineLayer;  // "bass" or "melody"
  void setLayer(const std::string& layer) { engineLayer = layer; }
  ```

- [ ] **Step 2.3**: Call `genMusic->setLayer("melody")` in `TAudioReactorView` constructor

**Files to modify:**
- `app/generative_music.h` - Add engineLayer member, setLayer()
- `app/generative_music.cpp` - Implement arpeggio trigger logic
- `app/audio_reactor.cpp` - Call setLayer() in constructor

#### Phase 3: Arpeggio Execution

- [ ] **Step 3.1**: In `Voice::evolve()`, check for arpeggio mode:
  ```cpp
  if (arpeggioMode) {
      arpeggioFrames--;
      if (arpeggioFrames <= 0) {
          // Advance to next note in scale
          arpeggioStep++;
          if (arpeggioStep >= 4) {
              arpeggioMode = false;  // End arpeggio
              arpeggioStep = 0;
          } else {
              // Jump to next scale degree
              frequency = currentScale[arpeggioStep % scaleSize];
              arpeggioFrames = arpeggioDuration;
          }
      }
      return;  // Skip normal evolution
  }
  // ... normal evolution code
  ```

- [ ] **Step 3.2**: Ensure scale access in Voice (pass from engine)

**Files to modify:**
- `app/generative_music.cpp` - Implement arpeggio execution in Voice::evolve()

#### Phase 4: Tuning Parameters

**Configurable values:**
- `ARPEGGIO_PROBABILITY = 0.20f` (20% chance)
- `ARPEGGIO_NOTE_DURATION = 500ms` (1/4 note at 120 BPM)
- `ARPEGGIO_STEPS = 4` (4 notes ascending)

**Adjustment options:**
- Decrease probability to 10% if too frequent
- Increase note duration to 750ms (1/4 note at 80 BPM) if too frantic
- Add descending arpeggios (50/50 chance up/down)

#### Phase 5: Testing Checklist

- [ ] **Test 5.1**: Create melody window, wait 2 minutes → verify arpeggios occasionally occur
- [ ] **Test 5.2**: Listen for rising 1/4 note pattern (should be clear and rhythmic)
- [ ] **Test 5.3**: Verify frequency <20% (time arpeggios over 5 minutes)
- [ ] **Test 5.4**: Verify bass window never arpeggios (only melody layer)
- [ ] **Test 5.5**: Verify voices return to normal evolution after arpeggio
- [ ] **Test 5.6**: User approval: is it pleasant or annoying?

**Success criteria:**
- Arpeggios add interest without dominating
- Pattern is clear and musical
- Doesn't trigger more than 1-2 times per minute
- User finds it enhancing rather than distracting

---

## Implementation Order

### Sprint 1: Feature 0 + Feature 1 (Implement & Test)

**Priority: Immediate**

1. Feature 0: Inverse melody gradient (5 min)
2. Feature 1: Spatial panning (30 min)
3. Test both features together
4. Get user approval before proceeding

### Sprint 2: Feature 2 (Pending Approval)

**Priority: After Feature 1 testing**

1. Only proceed if user approves after testing Feature 1
2. Implement arpeggio mode (45 min)
3. Test and tune probability/duration
4. Get final user approval

---

## File Summary

**Feature 0 + 1 (Sprint 1):**
- `app/generative_music.h` - Add setGlobalPan(), setLayer()
- `app/generative_music.cpp` - Implement pan control
- `app/audio_reactor.h` - Add updateSpatialPan(), changeBounds(), invertGradient
- `app/audio_reactor.cpp` - Implement spatial pan + inverse gradient

**Feature 2 (Sprint 2, pending):**
- `app/generative_music.h` - Add arpeggio state to Voice
- `app/generative_music.cpp` - Implement arpeggio logic in Voice::evolve()

---

## Risk Assessment

### Feature 0: Inverse Gradient
**Risk**: Low
**Mitigation**: Simple visual change, easy to revert

### Feature 1: Spatial Panning
**Risk**: Medium
**Concern**: Window movement might cause jarring pan jumps
**Mitigation**: Add pan smoothing (lerp over 100ms) if needed

### Feature 2: Arpeggios
**Risk**: High
**Concern**: Could be annoying if too frequent or too fast
**Mitigation**: Conservative 20% probability, user testing before commit

---

## Success Metrics

- [ ] Feature 0: Melody and bass windows visually distinct
- [ ] Feature 1: Window position clearly correlates to stereo position
- [ ] Feature 1: Panning feels natural when moving windows
- [ ] Feature 2: Arpeggios occur <20% of time (measured)
- [ ] Feature 2: User finds arpeggios pleasant, not annoying

---

**Last Updated**: 2025-12-26
**Approved For**: Sprint 1 (Feature 0 + 1)
**Pending Approval**: Sprint 2 (Feature 2)
