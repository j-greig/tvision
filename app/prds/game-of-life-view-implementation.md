# Game of Life View Implementation

**TL;DR**: Implement Conway's Game of Life as an animated TUI view with configurable rules, patterns, and visual styles for the test-tui menu system.

## Context

Adding a Game of Life simulation to the existing test-tui application alongside the animated blocks view. The simulation should integrate with the TVision framework's timer system and window management, providing an engaging cellular automaton demonstration.

## Objective

Create a fully-featured Game of Life implementation that:
- Runs smoothly in the TVision TUI environment
- Provides multiple starting patterns and configurations
- Offers real-time parameter adjustment
- Integrates seamlessly with the existing view menu system

## Requirements

### Core Game Logic
• Standard Conway's Game of Life rules implementation
• Configurable grid size (adapts to window dimensions)
• Multiple initial pattern presets (gliders, oscillators, still lifes, etc.)
• Wraparound/infinite grid support option
• Generation counter and statistics tracking

### Visual Presentation  
• Character-based cell rendering (░▒▓█ or custom symbols)
• Colour-coded cell states (alive/dead/dying/newborn)
• Smooth animation using TVision timer system
• Configurable animation speed (1-30 FPS)
• Optional trail effects showing recent cell history

### User Interface Controls
• Pattern selection menu (Glider, Pulsar, Gosper Gun, Random, etc.)
• Speed adjustment (pause/play, step mode, speed slider)
• Grid size configuration
• Reset/clear grid functionality
• Save/load pattern files (simple text format)

### Integration Points
• Add to existing view menu alongside animated blocks
• Use TTimerId-based animation (no threading)
• Window-responsive grid sizing
• Keyboard shortcuts for common actions

## Technical Specifications

### File Structure
```
test-tui/
├── game_of_life_view.h      # GameOfLifeView class definition
├── game_of_life_view.cpp    # Implementation
├── patterns/                # Preset pattern files
│   ├── glider.gol
│   ├── pulsar.gol
│   └── gosper_gun.gol
└── test_pattern_app.cpp     # Menu integration
```

### Class Design
```cpp
class GameOfLifeView : public TView {
    struct Cell { bool alive; int age; };
    struct Grid { 
        std::vector<std::vector<Cell>> cells;
        int width, height, generation;
    };
    
    // Core methods
    void computeNextGeneration();
    void loadPattern(const std::string& name);
    void handleEvent(TEvent& event);
    void draw();
};
```

### Animation System
• Timer-driven updates using `setTimer(timeout, period)`
• Handle `cmTimerExpired` events for generation advancement
• Configurable update intervals (33ms for 30fps down to 1000ms for 1fps)
• Pause/resume capability

### Pattern File Format
Simple text format for easy editing:
```
# Glider pattern
NAME: Glider
SIZE: 5x5
GENERATION: 0
.....
.#...
..##.
.##..
.....
```

## Expected Output

**Format**: Working TUI view class with full Game of Life simulation
**Structure**: 
- Header file with class declaration
- Implementation file with game logic and rendering
- Pattern files for interesting starting configurations
- Integration with existing menu system

**Files Created**:
- `game_of_life_view.h` - Class definition
- `game_of_life_view.cpp` - Implementation
- `patterns/` directory with sample patterns
- Updated `test_pattern_app.cpp` for menu integration

## Success Criteria

□ Game of Life simulation runs correctly with standard rules
□ Smooth animation at configurable speeds (1-30 FPS)
□ Multiple preset patterns load and display properly
□ User can pause/resume, step through generations, adjust speed
□ Grid adapts to window size changes
□ No memory leaks or performance issues during long runs
□ Integrates cleanly with existing view menu system
□ Pattern files load correctly and follow documented format

## Implementation Phases

1. **Core Engine** - Basic Game of Life logic and grid management
2. **Visualization** - TUI rendering and character-based display  
3. **Animation** - Timer integration and smooth updates
4. **Patterns** - Pattern loading system and preset library
5. **Controls** - User interaction and parameter adjustment
6. **Integration** - Menu system integration and testing

## Edge Cases & Considerations

• Handle very small window sizes gracefully
• Optimize for large grids (100x100+) without blocking UI
• Prevent infinite loops in oscillating patterns
• Graceful degradation if pattern files are missing/corrupt
• Memory management for grid resizing operations
• Proper cleanup when view is closed or destroyed