# Mechs Menu System PRD

**tl;dr**: Grid-based mechanoid generator with configurable layout, random generation, border styles, and form-based controls using TUI components.

## Overview

A new "Mechs" menu system that displays a configurable grid of ASCII art mechanoids generated according to mechs.md ruleset. Default 3x3 grid with TDialog-based configuration options for grid dimensions and display settings.

## Features

### Core Functionality
- **Grid Display**: Default 3x3 mechanoid grid (configurable 1x1 to 6x6)
- **Random Generation**: Each mech randomly generated from ruleset components
- **Border Styles**: Selectable styles (single, double, round, fat, etc.)
- **Live Refresh**: Regenerate all mechs with keyboard shortcut

### Configuration Dialog
- **Grid Size**: TInputLine for rows/cols (1-6 range validation)
- **Border Style**: TRadioButtons for style selection
- **Pattern Mode**: TCheckBoxes for display options
- **Preview**: Live preview pane showing single mech with current settings

### Mech Generation Rules
- **Canvas**: 19w x 9h per mech as per mechs.md
- **Components**: Head (required), body (required), legs/arms (optional)
- **Emoji**: Whitelisted emoji only from mechs.md ALLOWED_EMOJI
- **Anchoring**: Feet must touch ground row if present
- **Composition**: Box drawing chars with runtime style mapping

## Technical Architecture

### Core Classes
```cpp
class TMechWindow : public TWindow {
    // Main window containing mech grid
    TMechGrid* grid;
    void refresh();
    void showConfig();
};

class TMechGrid : public TView {
    // Grid layout and rendering
    int rows, cols;
    vector<TMech> mechs;
    void generateMechs();
    void draw() override;
};

class TMech {
    // Individual mechanoid data
    string pattern[9];  // 9 lines x 19 chars
    BorderStyle style;
    void generate();
    void applyStyle(BorderStyle);
};

class TMechConfigDialog : public TDialog {
    // Configuration form
    TInputLine* rowsInput;
    TInputLine* colsInput; 
    TRadioButtons* styleButtons;
    TMechPreview* preview;
};
```

### Generation System
- **Component Library**: Head patterns, body patterns, limb patterns from mechs.md
- **Random Assembly**: Pick random components, validate constraints
- **Style Mapping**: Convert spec chars (┌─┐) to runtime chars (╭─╮, ╔═╗, etc.)
- **Validation**: Ensure feet anchor, components connect, canvas bounds

### Integration Points
- **Menu Integration**: Add to main application menu system
- **Keyboard Shortcuts**: 
  - `F5` - Regenerate all mechs
  - `F6` - Configuration dialog
  - `S` - Cycle border styles
- **API Compatibility**: Works with existing programmatic control API

## Implementation Plan

### Phase 1: Core Generation
1. TMech class with pattern storage and generation
2. Component library from mechs.md examples
3. Border style mapping system
4. Basic random assembly logic

### Phase 2: Grid Display  
1. TMechGrid view class
2. Multi-mech layout and rendering
3. TMechWindow container
4. Basic keyboard controls

### Phase 3: Configuration
1. TMechConfigDialog with TInputLine validation
2. TRadioButtons for border style selection
3. Live preview functionality
4. Settings persistence

### Phase 4: Integration
1. Menu system integration
2. Keyboard shortcut mapping  
3. API endpoint exposure
4. Documentation and testing

## Technical Considerations

### Memory Management
- Static component libraries (no dynamic allocation for patterns)
- Efficient string operations for 19x9 grids
- Reuse TMech objects between generations

### Performance
- Pre-generate component combinations
- Lazy rendering for off-screen mechs
- Efficient Unicode handling for emoji

### Validation
- Input range validation (1-6 for grid size)
- Component connectivity validation
- Canvas bounds checking
- Emoji whitelist enforcement

## Files to Create/Modify

### New Files
- `test-tui/mech_window.h/cpp` - TMechWindow implementation
- `test-tui/mech_grid.h/cpp` - TMechGrid view
- `test-tui/mech.h/cpp` - TMech generation logic
- `test-tui/mech_config.h/cpp` - Configuration dialog
- `test-tui/mech_components.h` - Component library constants

### Modified Files
- `test-tui/test_pattern_app.cpp` - Add menu integration
- `test-tui/CMakeLists.txt` - Add new source files
- `tools/api_server/controller.py` - Add mech generation endpoints

## Success Criteria

✅ **Functional**: 3x3 grid displays unique mechanoids following ruleset  
✅ **Configurable**: Grid size and border styles changeable via dialog  
✅ **Random**: Each refresh generates completely new mechs  
✅ **Valid**: All mechs follow mechs.md constraints (anchored feet, connected components)  
✅ **Performant**: Sub-100ms generation time for 6x6 grid  
✅ **Integrated**: Accessible via menu and API endpoints