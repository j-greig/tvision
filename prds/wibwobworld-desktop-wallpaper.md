# WIBWOBWORLD Desktop Wallpaper PRD

**tl;dr:** Add centered ASCII art "WIBWOBWORLD" wallpaper to desktop background as reusable TDeskTopView component, inheriting from TBackground with proper draw() override and palette support.

## Executive Summary

This PRD outlines the implementation of a custom desktop wallpaper featuring ASCII art text "WIBWOBWORLD" centered on the Turbo Vision desktop background, following framework best practices for reusability and modularity.

## ASCII Art Design

```
   █     █░ ██▓ ▄▄▄▄    █     █░ ▒█████   ▄▄▄▄   
 ▓█░ █ ░█░▓██▒▓█████▄ ▓█░ █ ░█░▒██▒  ██▒▓█████▄ 
 ▒█░ █ ░█ ▒██▒▒██▒ ▄██▒█░ █ ░█ ▒██░  ██▒▒██▒ ▄██
 ░█░ █ ░█ ░██░▒██░█▀  ░█░ █ ░█ ▒██   ██░▒██░█▀  
 ░░██▒██▓ ░██░░▓█  ▀█▓░░██▒██▓ ░ ████▓▒░░▓█  ▀█▓
 ░ ▓░▒ ▒  ░▓  ░▒▓███▀▒░ ▓░▒ ▒  ░ ▒░▒░▒░ ░▒▓███▀▒
   ▒ ░ ░   ▒ ░▒░▒   ░   ▒ ░ ░    ░ ▒ ▒░ ▒░▒   ░ 
   ░   ░   ▒ ░ ░    ░   ░   ░  ░ ░ ░ ▒   ░    ░ 
  █     █░ ▒█████   ██▀███   ██▓    ▓█████▄     
 ▓█░ █ ░█░▒██▒  ██▒▓██ ▒ ██▒▓██▒    ▒██▀ ██▌    
 ▒█░ █ ░█ ▒██░  ██▒▓██ ░▄█ ▒▒██░    ░██   █▌    
 ░█░ █ ░█ ▒██   ██░▒██▀▀█▄  ▒██░    ░▓█▄   ▌    
 ░░██▒██▓ ░ ████▓▒░░██▓ ▒██▒░██████▒░▒████▓     
 ░ ▓░▒ ▒  ░ ▒░▒░▒░ ░ ▒▓ ░▒▓░░ ▒░▓  ░ ▒▒▓  ▒     
   ▒ ░ ░    ░ ▒ ▒░   ░▒ ░ ▒░░ ░ ░ ▒  ░ ░ ▒  ▒     
   ░   ░  ░ ░ ░ ▒    ░░   ░   ░ ░    ░ ░  ░     
     ░        ░ ░     ░         ░       ░       
```

## Technical Requirements

### Component Architecture

1. **Create `wallpaper.h/cpp`** - Reusable wallpaper module
   - Define `TWallpaperView` class extending `TBackground`
   - Store ASCII art as static const char* array
   - Implement centered positioning logic

2. **Integration Points**
   - Override `TTestPatternApp::initDeskTop()`
   - Replace default background with `TWallpaperView`
   - Maintain existing desktop functionality

### Implementation Design

```cpp
// wallpaper.h
class TWallpaperView : public TBackground
{
public:
    TWallpaperView(const TRect& bounds);
    virtual void draw();
    
private:
    static const char* asciiArt[];
    static const int artWidth;
    static const int artHeight;
    
    void drawCenteredArt(TDrawBuffer& b, int y);
};
```

### Key Features

1. **Centering Logic**
   - Calculate horizontal offset: `(size.x - artWidth) / 2`
   - Calculate vertical offset: `(size.y - artHeight) / 2`
   - Handle cases where desktop is smaller than art

2. **Drawing Strategy**
   - Fill background with pattern character (░ or ▒)
   - Overlay ASCII art at calculated position
   - Use contrasting colors for visibility

3. **Palette Support**
   - Use desktop background colors (indices 0-7)
   - Art in bright color (0x0F or 0x70)
   - Background pattern in dim color (0x07)

## Implementation Steps

### Phase 1: Create Wallpaper Component
1. Create `wallpaper.h` with class declaration
2. Create `wallpaper.cpp` with ASCII art storage
3. Implement `draw()` method with centering

### Phase 2: Integrate into Application
1. Add `#include "wallpaper.h"` to main app
2. Override `initDeskTop()` method
3. Replace `new TBackground` with `new TWallpaperView`

### Phase 3: Enhancement Options
1. Add configuration for different ASCII arts
2. Support color themes for the art
3. Add subtle animation (shimmer/pulse)

## Code Structure

```cpp
// wallpaper.cpp
const char* TWallpaperView::asciiArt[] = {
    "   █     █░ ██▓ ▄▄▄▄    █     █░ ▒█████   ▄▄▄▄   ",
    " ▓█░ █ ░█░▓██▒▓█████▄ ▓█░ █ ░█░▒██▒  ██▒▓█████▄ ",
    // ... rest of lines
};

void TWallpaperView::draw()
{
    TDrawBuffer b;
    TAttrPair artColor = getColor(0x01);    // Bright text
    TAttrPair bgColor = getColor(0x02);     // Dim background
    
    for (int y = 0; y < size.y; y++)
    {
        // Fill with background pattern
        b.moveChar(0, '░', bgColor, size.x);
        
        // Draw ASCII art if in range
        int artY = y - ((size.y - artHeight) / 2);
        if (artY >= 0 && artY < artHeight)
        {
            drawCenteredArt(b, artY);
        }
        
        writeLine(0, y, size.x, 1, b);
    }
}
```

## Testing Plan

1. **Visual Verification**
   - Art appears centered on desktop
   - Readable against background pattern
   - Windows properly overlay the wallpaper

2. **Edge Cases**
   - Terminal resize maintains centering
   - Small terminals crop art gracefully
   - Large terminals show full art

3. **Performance**
   - No flickering during redraws
   - Smooth window movement over wallpaper

## Success Criteria

- ✅ ASCII art displays centered on desktop
- ✅ Component is reusable in other TV apps
- ✅ Follows Turbo Vision best practices
- ✅ Clean separation of concerns
- ✅ No performance degradation

## Future Enhancements

1. **Multiple Wallpapers**
   - Wallpaper selector in menu
   - Save preference to config

2. **Dynamic Content**
   - Clock display
   - System info overlay
   - Random quotes

3. **Effects**
   - Matrix rain background
   - Starfield simulation
   - Plasma effect

---

*Document Version: 1.0*  
*Date: 2025-01-03*  
*Purpose: Desktop wallpaper enhancement for Turbo Vision TUI*