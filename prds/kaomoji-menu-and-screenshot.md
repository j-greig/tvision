# Kaomoji Menu Enhancement & Terminal Screenshot Implementation PRD

**tl;dr:** Add `つ◕‿◕‿◕ ༽つ` kaomoji to menu bar (right-aligned/first item), explore Unifont support. Implement terminal screenshot via: in-app ANSI capture→image, VSCode extension API, or macOS screencapture CLI. Research terminal buffer access methods.

## Executive Summary

This PRD outlines the implementation of two features for the Turbo Vision test pattern application:
1. Adding a decorative kaomoji to the menu bar with potential custom font support
2. Implementing terminal screenshot capabilities through multiple approaches

## Current State

- **Application**: Turbo Vision test pattern app with monochrome theme
- **Platform**: macOS terminal environment
- **Framework**: Turbo Vision with UTF-8 support enabled
- **Font**: System terminal font (likely SF Mono or similar)

## Feature 1: Kaomoji Menu Enhancement

### Requirements

1. Add `つ◕‿◕‿◕ ༽つ` to the menu bar
2. Position: Either right-aligned or as first menu item
3. Explore using Unifont for this element specifically

### Technical Challenges

#### Unicode Rendering
- **Issue**: Complex Unicode characters (◕) may not render correctly
- **Kaomoji bytes**: `つ` (U+3064), `◕` (U+25D5), `‿` (U+203F), `༽` (U+0F3D)
- **Width calculation**: Multi-byte characters affect positioning

#### Font Support
- **Terminal limitation**: Cannot mix fonts within same terminal session
- **Unifont**: Would need to be set at terminal level, not app level
- **Alternative**: Use ASCII art approximation if Unicode fails

### Implementation Approaches

#### Approach A: Right-Aligned in Menu Bar
```cpp
TMenuBar* initMenuBar(TRect r) {
    // Calculate position for right-aligned text
    int kaomojiWidth = 12; // Width of kaomoji
    // Override draw() method to add custom text
}
```

#### Approach B: Custom Menu Item
```cpp
*new TMenuItem("つ◕‿◕‿◕ ༽つ", cmNoCommand, kbNoKey) +
*new TSubMenu("~F~ile", kbAltF) + ...
```

#### Approach C: Status Line Addition
- Add to status line instead if menu bar proves difficult
- More flexibility for positioning

## Feature 2: Terminal Screenshot Implementation

### Requirements

1. Capture current terminal display as image
2. Support multiple methods (in-app, VSCode, system)
3. Save to file or clipboard

### Research Areas

#### Method 1: In-App ANSI Capture
- **Concept**: Read terminal buffer, convert ANSI to image
- **Tools**: 
  - `ansi2html` + `wkhtmltoimage`
  - Custom ANSI parser to Canvas/SVG
  - Libraries: `terminal-to-html`, `ansi-to-svg`

#### Method 2: VSCode Extension Integration
- **Extensions to research**:
  - "Terminal Capture"
  - "CodeSnap"
  - "Polacode"
- **API**: `vscode.window.activeTerminal.sendText()`
- **Limitation**: Requires VSCode as terminal host

#### Method 3: macOS System Tools
```bash
# Using screencapture with window ID
screencapture -l$(osascript -e 'tell app "Terminal" to id of window 1') screenshot.png

# Using terminal.app screenshot service
osascript -e 'tell application "Terminal" to do script "screenshot"'
```

#### Method 4: Terminal Emulator Features
- **iTerm2**: Has built-in capture via `Cmd+Shift+S`
- **Alacritty**: No native support, needs external tool
- **Terminal.app**: Limited programmatic access

### Implementation Plan

#### Phase 1: Research & Prototype
1. Test Unicode rendering of kaomoji
2. Investigate `TScreen::screenBuffer` access
3. Test screencapture command integration

#### Phase 2: Kaomoji Implementation
1. Add kaomoji to menu bar or status line
2. Handle width calculation for alignment
3. Fallback to ASCII if Unicode fails

#### Phase 3: Screenshot - Basic
1. Implement macOS screencapture wrapper
2. Add menu command (e.g., "~S~creenshot")
3. Save to `./screenshots/` directory

#### Phase 4: Screenshot - Advanced
1. ANSI buffer capture to text file
2. Convert ANSI to HTML/SVG
3. Optional: clipboard integration

## Code Structure

```
test-tui/
├── screenshot.h        # Screenshot utilities
├── screenshot.cpp      # Implementation
├── unicode_utils.h     # Unicode width helpers
└── test_pattern_app.cpp # Modified with new features
```

## Technical Specifications

### Unicode Width Calculation
```cpp
int getUnicodeWidth(const char* str) {
    // Use wcwidth() for proper width calculation
    // Handle combining characters
}
```

### Screenshot Command Integration
```cpp
class TScreenshot {
public:
    static bool captureScreen(const char* filename);
    static bool captureANSI(TScreen* screen, const char* filename);
    static bool captureToClipboard();
};
```

### Menu Bar Customization
```cpp
class TCustomMenuBar : public TMenuBar {
    virtual void draw();  // Override to add kaomoji
};
```

## Testing Plan

1. **Unicode Support**: Test on different terminals
2. **Screenshot Quality**: Verify color accuracy
3. **Performance**: Ensure no UI lag
4. **Compatibility**: Test on Terminal.app, iTerm2, VSCode

## Success Criteria

- ✅ Kaomoji displays correctly or falls back gracefully
- ✅ At least one screenshot method works reliably
- ✅ No performance degradation
- ✅ Cross-terminal compatibility

## Risks & Mitigations

1. **Unicode Rendering Issues**
   - Risk: Kaomoji displays as boxes
   - Mitigation: ASCII fallback `(o^_^o)`

2. **Screenshot Permissions**
   - Risk: macOS security blocks screencapture
   - Mitigation: Request screen recording permission

3. **Terminal Buffer Access**
   - Risk: Cannot read buffer programmatically
   - Mitigation: Use system-level capture

## References

- [Turbo Vision Unicode Support](https://github.com/magiblot/tvision#unicode-support)
- [ANSI Escape Codes](https://en.wikipedia.org/wiki/ANSI_escape_code)
- [macOS screencapture man page](https://ss64.com/osx/screencapture.html)
- [iTerm2 Image Protocol](https://iterm2.com/documentation-images.html)
- [terminal-kit screenshot](https://github.com/cronvel/terminal-kit)

---

*Document Version: 1.0*  
*Date: 2025-09-03*  
*Purpose: Enhanced menu UI and screenshot capability for TUI applications*