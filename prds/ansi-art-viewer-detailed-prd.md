# ANSI Art Viewer for Turbo Vision - Detailed PRD

**tl;dr**: Build world-class ANSI art viewer leveraging TV's existing color/Unicode systems. MVP: Parse ANSI escape sequences into TColorAttr buffer, render via TScroller for navigation, integrate with existing file browser. Modular design enables future editing capabilities.

## Executive Summary

Create a professional ANSI art viewer that preserves the heritage of BBS-era digital art while leveraging Turbo Vision's modern terminal capabilities. The solution builds incrementally on TV's proven architecture patterns, ensuring seamless integration with existing test-tui applications.

## Problem Analysis

### Current State
- test-tui has robust text/animation viewing via TTextFileView and FrameFilePlayerView
- TV framework provides complete color system (16-color → 24-bit RGB)
- Sample ANSI files exist but display as raw escape sequences
- No native ANSI parsing capability in current codebase

### User Pain Points
- Classic ANSI art appears as cryptic escape codes
- No way to appreciate historical digital art collections
- Missing bridge between modern terminal capabilities and legacy formats
- Disconnect between TV's color power and actual usage

## Technical Analysis of Sample Files

### rainbow_title.ans Deep Dive
```
Structure: Standard ANSI with systematic color usage
- Reset sequences: \x1b[0m (clear formatting)
- Background: \x1b[40m (black background throughout)  
- Foreground colors: \x1b[31m (red) → \x1b[37m (white)
- Block characters: ████ (Unicode U+2588 FULL BLOCK)
- Layout: 80-column fixed width, bordered design
```

**Technical Challenges:**
- State management: Colors persist until reset/override
- Mixed content: ANSI codes + Unicode + ASCII
- Cursor positioning: Implicit newlines vs explicit positioning

### boxes.ans Deep Dive
```
Structure: Unicode box-drawing with selective coloring
- Box drawing chars: ┌┐└┘│─┼├┤┬┴ (Unicode Box Drawing block)
- Sparse color usage: Only labels colored, structure monochrome
- Complex nesting: Multiple box levels with alignment
- Character density: High Unicode character usage
```

**Technical Challenges:**
- Unicode rendering: Proper width calculation for box chars
- Color isolation: Colors apply only to specific text spans
- Alignment preservation: Box structures must maintain geometry

### mario_mushroom.ans, space_invader.ans, wave.ans Analysis
Based on naming patterns, these likely represent:
- **mario_mushroom**: Pixel art using block characters + colors
- **space_invader**: Classic video game sprite reproduction
- **wave**: Gradient effects using background colors

## Architectural Approaches Analysis

### Modern C++ Libraries Survey (2024)

**TerminalImageViewer (TIV) - High TV Compatibility**
- **Architecture**: Clean C++ library (`tiv_lib.h/cc`), Apache 2.0/GPL3 dual license
- **Algorithm**: 4x8 pixel cell mapping to Unicode blocks with color channel analysis
- **TV Integration**: Perfect fit - character-based rendering aligns with `TDrawBuffer` model
- **Color System**: 24-bit RGB + 256-color fallback matches TV's `TColorAttr` capabilities

**ANSI🔥Art (mafik/ansi-art) - Modern Approach**
- **Architecture**: Modern C++ with FreeType dependency, virtual interface pattern
- **Features**: 24-bit RGB, Unicode, animation support, multi-threaded rendering
- **TV Integration**: Good compatibility with TV's modular design philosophy
- **Unique**: Progress tracking, cancellation support, bash-compatible output

**libcaca - Mature Ecosystem**
- **Architecture**: C library with extensive driver support (ncurses, X11, Win32)
- **Features**: Image → ASCII/ANSI conversion, multiple output formats, proven stability
- **TV Integration**: Excellent - existing ncurses driver, used in VLC/FFmpeg/MPlayer
- **Applications**: Comprehensive tooling (cacaview, img2txt)

### Implementation Approaches

### Approach 1: Custom TIV-Inspired (Recommended)
**Concept:** Build TV-native parser using TIV's proven color mapping algorithms
```
Pros:
+ Perfect TV integration with TColorAttr/TDrawBuffer
+ Leverage TIV's 4x8 cell mapping for sub-character rendering
+ Clean separation: parsing vs rendering
+ Proven color channel analysis algorithm
+ No external dependencies beyond TV framework

Cons:
- Custom implementation effort
- Need to build CP437 encoding support
- Limited to ANSI parsing (no image import initially)
```

### Approach 2: libcaca Integration
**Concept:** Wrapper around mature libcaca for advanced features
```
Pros:
+ Mature, battle-tested codebase
+ Extensive format support (images → ASCII/ANSI)
+ Multiple rendering backends available
+ Rich feature set for advanced capabilities

Cons:
- External dependency management
- Wrapper complexity for TV integration
- Potential performance overhead
- Less control over rendering pipeline
```

### Approach 3: Hybrid Architecture (Future)
**Concept:** Custom ANSI parser + selective library integration
```
Pros:
+ Best of both worlds: native TV integration + advanced features
+ Modular design allows feature-by-feature integration
+ Performance optimization where needed

Cons:
+ Complex architecture
+ Multiple dependency management
+ Longer development timeline
```

## Selected Architecture: TIV-Inspired Pre-parsed Document Model

### Core Design Philosophy
1. **TV-Native Integration**: Leverage existing TScroller, TColorAttr, TDrawBuffer systems
2. **Proven Algorithms**: Adopt TIV's 4x8 pixel cell mapping and color channel analysis
3. **Separation of Concerns**: Clean parser → document → renderer pipeline  
4. **Performance**: Parse once, render many times with optimized cell access
5. **Standards Compliance**: Proper ANSI specification adherence with graceful degradation
6. **Extensibility**: Architecture enables future libcaca integration for advanced features

### Document Model Specification

```cpp
class TAnsiDocument {
    struct Cell {
        char32_t character;      // Unicode codepoint
        TColorAttr attributes;   // TV's color system
    };
    
    TPoint dimensions;           // Actual content size
    vector<vector<Cell>> grid;   // 2D cell array
    
    // Metadata
    string originalPath;
    string encoding;             // "CP437", "UTF-8", etc.
    vector<string> comments;     // Embedded sauce/metadata
};
```

**Design Rationale:**
- **Cell-based**: Matches terminal character grid model
- **Unicode support**: char32_t handles full Unicode range
- **TV integration**: TColorAttr provides color abstraction
- **Metadata preservation**: Maintains original file context
- **Grid structure**: Enables random access for navigation

### Parser Architecture with TIV Integration

```cpp
class TAnsiParser {
    enum ParseState { 
        TEXT, ESCAPE, CSI, OSC 
    };
    
    struct ParserContext {
        ParseState state;
        TColorAttr currentAttrs;
        TPoint cursorPos;
        TPoint savedCursor;
        // TIV-inspired color mapping state
        ColorChannelAnalysis channelState;
    };
    
    TAnsiDocument* parse(const string& content, const string& encoding);
    
private:
    void handleTextChar(char32_t ch, ParserContext& ctx);
    void handleEscapeSequence(const string& seq, ParserContext& ctx);
    void handleCSI(const vector<int>& params, char final, ParserContext& ctx);
    
    // TIV-inspired color mapping for future image import
    TColorAttr mapPixelCell(const PixelData& cell, const ColorChannelAnalysis& analysis);
    char32_t selectOptimalUnicodeBlock(const BitmapCell& cell);
};
```

**Design Rationale:**
- **State machine**: Proper ANSI sequence parsing with TV integration
- **TIV algorithm adoption**: Color channel analysis for future sub-character rendering
- **Context preservation**: Maintains cursor/color state compatible with TV's system
- **Extensible**: Architecture supports future libcaca wrapper integration
- **Error resilient**: Graceful handling of malformed sequences

### Viewer Implementation

```cpp
class TAnsiArtView : public TScroller {
    TAnsiDocument* document;
    
public:
    TAnsiArtView(const TRect& bounds, TScrollBar* hScrollBar, TScrollBar* vScrollBar);
    
    bool loadFile(const string& path);
    void draw() override;
    void handleEvent(TEvent& event) override;
    
    // Metadata access
    TPoint getArtworkSize() const;
    string getFileInfo() const;
    
private:
    void updateScrollBars();
    void renderRegion(const TRect& region, TDrawBuffer& buffer);
    // View options (MVP)
    bool fitWidth = false;   // Fit artwork width into window
    bool actualSize = true;  // 1:1 cell mapping (default)
};
```

**Design Rationale:**
- **TScroller inheritance**: Automatic scroll bar management
- **Clean interface**: Simple load/display workflow
- **Performance optimized**: Only renders visible regions
- **Event handling**: Keyboard navigation, zoom controls, Fit width/Actual size toggles

## Integration Strategy

### File Browser Integration
Extend existing `TFileDialog` usage in test_pattern_app.cpp:
- Add ".ans", ".ansi" to file filter patterns
- Detect ANSI content via quick inspection (presence of ESC[) or by extension
- Route to `TAnsiArtView` instead of `TTextFileView`

### Window Management Integration
Follow established test_pattern patterns:
- New menu item: "View → ANSI Art File..."
- Window creation using existing numbering system
- Integrate with workspace save/restore:
  - Save: `type: "ansi_art"`, `path`, `encoding: "cp437"|"utf8"`, scroll offsets `hScroll`, `vScroll`, and toggle `fitWidth`
  - Load: reopen file, apply encoding, restore scroll offsets and toggles

### Color System Integration
Leverage TV's existing color capabilities:
- Map ANSI 16-color to TV's palette system
- Support 256-color and 24-bit where available
- Graceful degradation for limited terminals

## Feature Specification

### Core Viewer Features
- **File Loading**: .ans, .ansi, .asc file support; ANSI auto-detection by ESC[ sniff
- **Navigation**: Arrow keys, Page Up/Down, Home/End; preserve/restore scroll offsets
- **Zoom**: Fit width toggle and Actual size; custom zoom levels later
- **Information**: Dimensions, color count, file metadata (SAUCE if present)
- **Export**: Screenshot via existing mechanism; image export later

### Advanced Features (Future)
- **Search**: Text content searching within artwork
- **Thumbnails**: Preview mode for file browsers
- **Batch Processing**: Convert multiple files
- **Animation**: Support for ANSI animation sequences

## Technical Specifications

### ANSI Sequence Support
**Priority 1 (MVP):**
- ESC[0m (reset)
- ESC[1m (bold/bright)
- ESC[30-37m, ESC[40-47m (16 colors)
- ESC[39m, ESC[49m (default colors)
- CR/LF handling (CR: col 0; LF: down); CRLF handling
- Cursor moves: CSI nA/B/C/D (Up/Down/Right/Left)
- Tab expansion (width 8)

**Priority 2:**
- ESC[2J (clear screen)
- ESC[H, ESC[row;colH (cursor positioning)
- ESC[90-97m, ESC[100-107m (bright colors)

**Priority 3:**
- ESC[38;5;n, ESC[48;5;n (256-color)
- ESC[38;2;r;g;b (24-bit color)

### Character Encoding Support
- **CP437 (default)**: IBM PC character set (essential). Map bytes → Unicode; ensure box/block chars display.
- **UTF-8**: Modern Unicode content.
- **Auto-detection**: Sniff UTF‑8; provide runtime toggle CP437/UTF‑8.

### Performance Requirements
- **Load time**: <500ms for typical ANSI files (<50KB)
- **Navigation**: <16ms response time for smooth scrolling
- **Memory**: <10MB RAM for large artworks (200x1000 chars)

## Development Phases

### Phase 1: TV-Native ANSI Parser (Week 1-2)
**Deliverables:**
- TAnsiParser class with TColorAttr integration
- TIV-inspired color mapping foundation (for future use)
- Unit tests for parser edge cases with TV framework validation
- Support for rainbow_title.ans and boxes.ans samples

**Success Criteria:**
- Correctly parse all sample files with TV color system compatibility
- Handle malformed sequences gracefully with TV error patterns
- Preserve original character positioning (CR/LF, CSI cursor moves, tabs)
- Validate color rendering accuracy against reference implementations

### Phase 2: Document Model (Week 3)
**Deliverables:**
- TAnsiDocument class implementation
- Memory-efficient grid storage
- Metadata extraction and storage

**Success Criteria:**
- Load sample files into document model
- Access arbitrary cells efficiently
- Track original file information

### Phase 3: Viewer Integration (Week 4-5)
**Deliverables:**
- TAnsiArtView class with TScroller integration
- File browser integration
- Basic navigation controls

**Success Criteria:**
- Display sample files correctly in test_pattern app
- Smooth scrolling for large artworks
- Proper color rendering (SGR 0/1/30–37/40–47, 39/49)

### Phase 4: Polish & Features (Week 6)
**Deliverables:**
- Export functionality
- Information display
- Performance optimizations
- Documentation

**Success Criteria:**
- Professional user experience
- Export capabilities working
- Ready for community feedback

## Risk Analysis

### Technical Risks
**ANSI Parsing Complexity**
- *Risk*: Incomplete sequence support causes rendering errors
- *Mitigation*: Comprehensive test suite with real-world files
- *Fallback*: Graceful degradation to plain text display

**Performance Issues**
- *Risk*: Large files cause UI lag
- *Mitigation*: Profiling during development, lazy rendering
- *Fallback*: File size limits with user warnings

**Character Encoding Problems**
- *Risk*: Incorrect encoding causes garbled display
- *Mitigation*: Multiple encoding detection strategies
- *Fallback*: User-selectable encoding override

### Integration Risks
**TV Framework Compatibility**
- *Risk*: Color system changes break integration
- *Mitigation*: Use stable TV APIs, avoid internal dependencies
- *Fallback*: Maintain compatibility layer

## Success Metrics

### MVP Success Criteria
- [ ] Load all provided sample files correctly (CR/LF, CSI A/B/C/D, tabs, CP437 mapping)
- [ ] Display colors accurately on 16-color terminals
- [ ] Navigate large artworks smoothly (TScroller)
- [ ] Integrate seamlessly with existing test_pattern app (ANSI detection + menu)
- [ ] Export screenshots for sharing (existing capture path)

### Quality Metrics
- **Parsing accuracy**: 100% for well-formed ANSI sequences
- **Performance**: <16ms frame times during navigation
- **Memory efficiency**: <5MB for typical artwork files
- **User satisfaction**: Intuitive controls, professional feel

## Future Roadmap

### Short-term (3 months) - TIV Integration
- Extended ANSI sequence support (256-color, 24-bit)
- TIV's 4x8 pixel cell mapping for image import capability
- Animation sequence handling
- Search functionality within artwork

### Medium-term (6 months) - libcaca Integration  
- Wrapper layer for libcaca's mature image conversion
- Multiple format support (PNG, JPEG → ANSI)
- Color palette analysis and optimization
- Batch conversion tools using libcaca backend

### Long-term (12 months) - Hybrid Ecosystem
- ANSI🔥Art integration for modern 24-bit artwork creation
- Full pixel art editor with sub-character precision
- Community sharing integration
- Educational content showcasing BBS art history

## Conclusion

This architecture provides a solid foundation for world-class ANSI art viewing within the Turbo Vision ecosystem. The modular design ensures clean integration with existing components while maintaining extensibility for future enhancements. By leveraging TV's proven patterns and color capabilities, we can deliver a professional tool that honors both the technical heritage of ANSI art and the modern capabilities of today's terminals.
