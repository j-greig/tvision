# Test Pattern Window Spawner PRD

**tl;dr:** TUI app spawning unlimited resizable windows showing test patterns. Pattern: 16 ANSI colors→16 grayscale shades in tiled blocks. Modular TTestPatternView class for reuse. File menu→New Window spawns instances.

## Turbo Vision Terminology Guide

### Core Concepts
- **Application (TApplication)**: The main program class that manages the event loop, desktop, menu bar, and status line
- **View (TView)**: Base class for all visual components - anything that can be drawn on screen
- **Window (TWindow)**: A framed, moveable, resizable view that contains other views
- **Dialog (TDialog)**: A specialized window for user interaction (modal or modeless)
- **Desktop (TDeskTop)**: The background area where windows are placed
- **Group (TGroup)**: A view that contains and manages child views
- **Interior**: The client area inside a window frame where content is drawn

### Component Hierarchy
```
TObject
  └── TView (base for all visual elements)
       ├── TFrame (window borders)
       ├── TScrollBar
       ├── TScroller (scrollable interior)
       ├── TListViewer
       ├── TGroup (container for other views)
       │    ├── TWindow
       │    │    └── TDialog
       │    └── TDeskTop
       └── TStaticText, TButton, TInputLine, etc.
```

### Key Terms
- **Owner**: The view that contains another view
- **Modal**: Blocks interaction with other views until closed
- **Palette**: Color scheme mapping for a view
- **Event**: User input (keyboard, mouse) or system message
- **Command**: High-level action triggered by menus/buttons (e.g., cmQuit)
- **Interior**: The drawable client area of a window

## Test Pattern Specification

### Pattern Design
The test pattern consists of two rows:
1. **Row 1**: 16 ANSI colors (black, blue, green, cyan, red, magenta, brown, light gray, dark gray, light blue, light green, light cyan, light red, light magenta, yellow, white)
2. **Row 2**: 16 grayscale shades from black (0x00) to white (0xFF)

### Visual Layout
```
[COLOR BAR - 16 ANSI COLORS]
[GRAYSCALE - 16 SHADES]
```
This 2-row pattern tiles to fill the window.

### Implementation Architecture

#### 1. TTestPattern Class (Reusable Component)
```cpp
class TTestPattern {
public:
    static void drawPattern(TDrawBuffer& b, int y, int width);
    static const TColorAttr ansiColors[16];
    static const TColorAttr grayShades[16];
};
```

#### 2. TTestPatternView Class (Interior View)
```cpp
class TTestPatternView : public TScroller {
public:
    TTestPatternView(const TRect& bounds);
    virtual void draw();
private:
    static const int patternHeight = 2;
};
```

#### 3. TTestPatternWindow Class
```cpp
class TTestPatternWindow : public TWindow {
public:
    TTestPatternWindow(const TRect& bounds, const char* title);
    static int windowCounter;  // For unique titles
};
```

#### 4. TTestPatternApp Class
```cpp
class TTestPatternApp : public TApplication {
public:
    TTestPatternApp();
    virtual void handleEvent(TEvent& event);
    void newTestWindow();
private:
    int windowNumber;
};
```

## Features

### Core Functionality
1. **Unlimited Windows**: Each "New Window" creates a new test pattern window
2. **Resizable Windows**: Standard Turbo Vision window resizing
3. **Moveable Windows**: Drag by title bar
4. **Tiling Pattern**: Pattern repeats to fill window dimensions
5. **Window Management**: Close, zoom, resize, cascade, tile

### Menu Structure
```
File
  ├── New Test Window  (Ctrl+N)
  ├── Cascade
  ├── Tile
  ├── Close All
  └── Exit            (Alt+X)

Windows
  └── [List of open windows]
```

### Color Definitions

#### ANSI Colors (16 standard colors)
```cpp
// Using TColorAttr for extended color support
TColorAttr ansiColors[16] = {
    TColorAttr(0x00), // Black
    TColorAttr(0x01), // Blue  
    TColorAttr(0x02), // Green
    TColorAttr(0x03), // Cyan
    TColorAttr(0x04), // Red
    TColorAttr(0x05), // Magenta
    TColorAttr(0x06), // Brown
    TColorAttr(0x07), // Light Gray
    TColorAttr(0x08), // Dark Gray
    TColorAttr(0x09), // Light Blue
    TColorAttr(0x0A), // Light Green
    TColorAttr(0x0B), // Light Cyan
    TColorAttr(0x0C), // Light Red
    TColorAttr(0x0D), // Light Magenta
    TColorAttr(0x0E), // Yellow
    TColorAttr(0x0F)  // White
};
```

#### Grayscale Shades (16 levels)
```cpp
// Using RGB values for true grayscale
TColorAttr grayShades[16] = {
    TColorRGB(0x00, 0x00, 0x00), // Black
    TColorRGB(0x11, 0x11, 0x11),
    TColorRGB(0x22, 0x22, 0x22),
    TColorRGB(0x33, 0x33, 0x33),
    TColorRGB(0x44, 0x44, 0x44),
    TColorRGB(0x55, 0x55, 0x55),
    TColorRGB(0x66, 0x66, 0x66),
    TColorRGB(0x77, 0x77, 0x77),
    TColorRGB(0x88, 0x88, 0x88),
    TColorRGB(0x99, 0x99, 0x99),
    TColorRGB(0xAA, 0xAA, 0xAA),
    TColorRGB(0xBB, 0xBB, 0xBB),
    TColorRGB(0xCC, 0xCC, 0xCC),
    TColorRGB(0xDD, 0xDD, 0xDD),
    TColorRGB(0xEE, 0xEE, 0xEE),
    TColorRGB(0xFF, 0xFF, 0xFF)  // White
};
```

### Block Character
Use `█` (U+2588 FULL BLOCK) or ASCII 219 `█` for solid blocks.

## Implementation Plan

### Phase 1: Create Reusable Test Pattern Module
1. Create `test_pattern.h` with TTestPattern class
2. Implement static drawing method
3. Define color arrays

### Phase 2: Create Pattern View
1. Implement TTestPatternView extending TScroller
2. Override draw() method
3. Handle tiling logic

### Phase 3: Create Window Class
1. Implement TTestPatternWindow
2. Add window counter for unique titles
3. Insert TTestPatternView as interior

### Phase 4: Create Application
1. Implement TTestPatternApp
2. Add File menu with New Window command
3. Implement window spawning logic
4. Add cascade/tile functionality

### Phase 5: Testing
1. Verify pattern rendering
2. Test window resizing
3. Test multiple windows
4. Verify color accuracy

## Code Structure

```
test-tui/
├── test_pattern_app.cpp    # Main application
├── test_pattern.h          # Reusable pattern module
├── test_pattern.cpp        # Pattern implementation
└── CMakeLists.txt          # Build configuration
```

## Reusability

The `TTestPattern` class can be used in other applications:
```cpp
// Example usage in another app
#include "test_pattern.h"

void MyView::draw() {
    TDrawBuffer b;
    TTestPattern::drawPattern(b, 0, size.x);
    writeLine(0, 0, size.x, 1, b);
}
```

## Success Criteria
- ✅ Spawns unlimited windows from File menu
- ✅ Windows are resizable and moveable
- ✅ Pattern shows 16 ANSI colors + 16 grayscale shades
- ✅ Pattern tiles to fill window
- ✅ Test pattern module is reusable
- ✅ Clean separation of concerns

## Turbo Vision Best Practices
1. **Inherit appropriately**: Use TWindow for windows, TScroller for scrollable content
2. **Event handling**: Override handleEvent() and call parent
3. **Drawing**: Use TDrawBuffer for efficient screen updates
4. **Commands**: Define unique command constants (>=100)
5. **Memory**: Use destroy() to clean up dynamically allocated views

---

*Document Version: 1.0*  
*Date: 2025-09-03*  
*Purpose: Multi-window test pattern viewer for Turbo Vision testing*