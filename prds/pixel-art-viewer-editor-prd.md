# Pixel Art Viewer/Editor for Turbo Vision PRD

**tl;dr**: Terminal-based pixel art viewer/editor leveraging TV's color system for ANSI art, classic BBS graphics, and modern 24-bit terminal pixel art. Phase 1: load/view ANSI art from classic archives. Phase 2+: editing, creation, modern formats.

## Context

Building on the existing Turbo Vision TUI framework's advanced color capabilities (BIOS 16-color, XTerm-256, 24-bit RGB) to create a terminal-native pixel art application. Combines the nostalgic ANSI art tradition with modern terminal graphics capabilities.

## Objective

Create a comprehensive pixel art ecosystem within Turbo Vision that preserves the heritage of BBS ANSI art while embracing modern terminal capabilities for pixel-perfect graphics creation and editing.

## Technical Foundation Research

### TV Color System Analysis
- **TColorAttr**: Supports BIOS 16-color, XTerm-256, and 24-bit RGB
- **TDrawBuffer**: Character-by-character rendering with full color attributes
- **Terminal Detection**: Built-in capability detection for color support levels
- **Unicode Support**: Full UTF-8 with combining characters and double-width handling

### Reference Projects Analysis
- **rich-pixels**: Python library for terminal image rendering via Rich/Textual
  - Pillow image loading, ASCII art with custom styling
  - Character-based pixel approximation techniques
- **pxltrm**: Bash-based pixel editor
  - Uses any character as brush, 256-color + truecolor support
  - Save/load functionality, vim-style navigation
- **Terminal Color Standards**: 
  - COLORTERM=truecolor detection
  - Fallback strategies for limited color terminals

### Classic ANSI Art Sources
- **16colo.rs**: Primary archive of BBS artpacks (1990s-present)
- **textfiles.com/artscene**: Historical ANSI/ASCII collection
- **Common Formats**: .ANS files, CP437 encoding, 80x25 dimensions

## Phase 1: Classic ANSI Art Viewer (MVP)

### Core Requirements
- **File Format Support**:
  - .ANS files (ANSI escape sequences)
  - .ASC files (ASCII art)
  - .TXT files (plain text art)
  - CP437 character encoding support
- **Display Engine**:
  - Accurate ANSI sequence parsing
  - 16-color BIOS palette rendering
  - Proper character positioning for 80-column layouts
  - Preserve original aspect ratios

### MVP Features
- File browser integrated with TV file dialogs
- Zoom/pan navigation for large artworks
- Metadata display (dimensions, colors used, file info)
- Support for animated ANSI sequences
- Export to modern formats (PNG screenshot via terminal capture)

### Sample Content Integration
- Bundle classic artpack samples from 16colo.rs
- Include representative works from major groups (ACiD, iCE)
- Test suite of various ANSI complexity levels

### Technical Implementation
```cpp
class TAnsiArtView : public TScroller {
    TAnsiDocument *doc;          // Parsed ANSI content
    TPoint virtualSize;          // Full artwork dimensions
    TColorAttr *buffer;          // Rendered character/color pairs
    
    void parseAnsiFile(const char* filename);
    void renderToBuffer();       // Convert ANSI to TColorAttr array
    void draw() override;        // Display visible portion
};
```

## Phase 2: Basic Editing Capabilities

### Edit Mode Features
- **Drawing Tools**:
  - Character brush (any Unicode character)
  - Color picker (TV's full palette support)
  - Fill tool for regions
  - Line/rectangle drawing primitives
- **Navigation**:
  - Vim-style hjkl movement
  - Mouse support where available
  - Cursor position indicators

### Save/Load System
- Preserve original ANSI format fidelity
- Export to multiple formats:
  - .ANS (ANSI sequences)
  - .TXT (plain text with color codes)
  - Terminal screenshot formats

## Phase 3: Modern Terminal Graphics

### Advanced Rendering
- **Pixel Art Techniques**:
  - Block character utilization (▀▄█ etc.)
  - Unicode box drawing characters
  - Sub-character pixel approximation
- **Image Import**:
  - PNG/JPEG loading via external tools
  - Automatic dithering to terminal palette
  - Aspect ratio preservation

### Enhanced Features
- Layer system for complex compositions
- Animation timeline for ANSI sequences
- Color palette optimization
- Import from rich-pixels compatible formats

## Phase 4: Creation Studio

### Advanced Tools
- **Pixel Perfect Editing**:
  - Sub-pixel positioning using Unicode blocks
  - 24-bit color support for modern terminals
  - Gradient tools
- **Asset Management**:
  - Character library/favorites
  - Color palette presets
  - Template system

### Export Pipeline
- Modern web formats (animated WebP, GIF)
- Video export of animations
- Social media optimized formats
- Print-ready outputs

## Technical Architecture

### File Format Handler
```cpp
class TPixelArtDocument {
    enum Format { ANSI, ASCII, Modern };
    TPoint dimensions;
    vector<TAnsiSequence> sequences;    // For animated content
    TColorAttr* canvas;                 // Current frame
    
    bool loadFromFile(const char* path);
    bool saveToFile(const char* path, Format fmt);
    void convertToModern();             // Upgrade ANSI to RGB
};
```

### Rendering Engine
```cpp
class TPixelRenderer {
    TColorCapabilities termCaps;        // Detected terminal features
    
    void renderToDrawBuffer(TDrawBuffer& buf, const TPixelArtDocument& doc);
    void optimizeForTerminal(TPixelArtDocument& doc);
    void applyDithering(TPixelArtDocument& doc);
};
```

### Editor Interface
```cpp
class TPixelArtEditor : public TWindow {
    TPixelArtDocument* document;
    TPixelRenderer* renderer;
    TToolPalette* tools;               // Drawing tools palette
    TColorSelector* colors;            // Color picker
    
    void handleDraw(TEvent& event);
    void handleToolSelection(TEvent& event);
    void handleSave();
};
```

## Success Criteria

### Phase 1 (MVP) Success
□ Load and accurately display classic ANSI art from major archives
□ Smooth navigation of large artworks (>80x25)
□ Perfect color reproduction on 16-color terminals
□ Export capabilities for modern sharing

### Phase 2 Success  
□ Create and edit simple ANSI artwork
□ Save files compatible with original BBS tools
□ Undo/redo functionality
□ Basic drawing tool completeness

### Phase 3 Success
□ Import modern image formats with quality dithering
□ Support all terminal color capabilities (16-color through 24-bit)
□ Animation editing for ANSI sequences
□ Layer-based composition

### Phase 4 Success
□ Professional pixel art creation capabilities
□ Export pipeline suitable for modern digital art workflows
□ Community sharing integration
□ Educational resources for ANSI art history

## Development Dependencies

### External Tools
- Character encoding libraries (CP437 support)
- Image processing capabilities (for import/export)
- ANSI sequence parsing library
- Terminal capability detection

### TV Framework Integration
- Extend TFileDialog for art file browsing
- Utilize TV's color system throughout
- Integrate with existing animation framework
- Leverage TV's Unicode text handling

## File Structure
```
pixel-art-viewer/
├── src/
│   ├── ansi_parser.cpp          # ANSI sequence parsing
│   ├── pixel_document.cpp       # Document model
│   ├── pixel_renderer.cpp       # Terminal rendering
│   ├── pixel_editor.cpp         # Main editor interface
│   └── tools/                   # Drawing tools implementation
├── assets/
│   ├── classic-ansi/            # Sample artworks
│   ├── templates/               # Art templates
│   └── palettes/                # Color schemes
└── tests/
    ├── ansi-samples/            # Test ANSI files
    └── format-tests/            # Format compatibility tests
```

## Development Timeline

### Phase 1 (4-6 weeks)
- Week 1-2: ANSI parsing engine and color system integration
- Week 3-4: Basic viewer interface and navigation
- Week 5-6: File format support and sample content integration

### Phase 2 (6-8 weeks) 
- Editing interface and drawing tools
- Save/load system implementation
- User experience refinement

### Phase 3 (8-10 weeks)
- Modern format support and import pipeline
- Advanced rendering techniques
- Animation system integration

### Phase 4 (10-12 weeks)
- Professional creation tools
- Export pipeline completion
- Documentation and community features

## Risk Mitigation

### Terminal Compatibility
- Comprehensive terminal testing across platforms
- Graceful degradation for limited color terminals
- Fallback ASCII rendering for unsupported features

### Performance Concerns
- Lazy loading for large artworks
- Efficient diff-based rendering
- Memory management for large canvas sizes

### Format Complexity
- Incremental format support implementation
- Extensive test suite for edge cases
- Community feedback integration for authentic ANSI handling