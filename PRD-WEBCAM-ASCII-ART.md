# Product Requirements Document: Webcam ASCII Art Web App

## Executive Summary

A standalone web application that captures real-time webcam feed and converts it to ASCII art, with customizable resolution controls and a minimal dark theme interface.

## Target Platform

- **Primary**: MacBook Air 2020 (Safari/Chrome)
- **Deployment**: Standalone HTML page (no server required)
- **Browser Requirements**: Modern browsers with getUserMedia API support

---

## Research: Existing Solutions Analysis

### Evaluated Approaches

#### 1. **Vanilla JavaScript (RECOMMENDED) ⭐⭐⭐⭐⭐**

**Examples**:
- idevelop/ascii-camera
- joeskeen's vanilla implementation gist

**Pros**:
- Zero dependencies, fully portable
- No build process required
- Smallest file size (~10-20KB total)
- Works on any modern browser
- Easy to debug and customize
- Single HTML file deployment possible

**Cons**:
- More boilerplate code to write
- Manual canvas manipulation

**Tech Stack**:
- HTML5 getUserMedia API
- Canvas API for pixel processing
- Pure JavaScript for conversion logic
- CSS for styling

**Ranking**: ⭐⭐⭐⭐⭐ (5/5)

---

#### 2. **p5.js Framework ⭐⭐⭐⭐**

**Examples**:
- hphng/AsciiCam
- deonvz/ImageToAsciiart

**Pros**:
- Simpler video/webcam handling
- Creative coding friendly
- Built-in video processing utilities
- Good for prototyping

**Cons**:
- 1MB+ library overhead
- Overkill for simple ASCII conversion
- Slower performance than vanilla
- Extra dependency to maintain

**Ranking**: ⭐⭐⭐⭐ (4/5)

---

#### 3. **React + video-stream-ascii npm package ⭐⭐⭐**

**Examples**:
- Im-Rises/video-stream-ascii-webcam
- video-stream-ascii npm package

**Pros**:
- Component-based architecture
- npm package available
- Good for larger applications

**Cons**:
- Requires build process (webpack/vite)
- Larger bundle size
- Overkill for standalone page
- Build complexity for simple app

**Ranking**: ⭐⭐⭐ (3/5)

---

## Recommended Approach: Vanilla JavaScript

### Implementation Strategy

**Core Algorithm**:
```
1. Capture video frame from webcam
2. Draw frame to hidden canvas
3. Get pixel data from canvas
4. Sample pixels at defined resolution
5. Calculate brightness for each sample
6. Map brightness to ASCII character
7. Render characters to display element
8. Repeat at target FPS
```

**ASCII Character Set** (dark to light):
```
' ·:!+*oe&#%@'  // Simple 12-character set
'$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\|()1{}[]?-_+~<>i!lI;:,"^`\'. '  // Extended 70-character set
```

---

## Feature Requirements

### Core Features (MVP)

1. **Webcam Capture**
   - Request camera permissions
   - Display error messages for denied/unavailable camera
   - Support Safari and Chrome on macOS

2. **ASCII Conversion**
   - Real-time conversion at 15-30 FPS
   - Character-based rendering (actual text, not canvas)
   - Monochrome white-on-black display

3. **Resolution Controls**
   - Adjustable width (characters across): 50-200 range
   - Auto-calculated height to maintain aspect ratio
   - Real-time updates when changed

4. **Visual Design**
   - Black background (#000000)
   - White monospaced text (#FFFFFF)
   - Minimal UI controls
   - Centered layout

### Enhanced Features (Optional)

- Font size adjustment
- Character set selection
- Screenshot/download capability
- Performance metrics display
- Mobile responsive design

---

## Technical Architecture

### File Structure
```
test-tui-apps/
└── webcam-ascii/
    ├── index.html          # Single-file app (or split if needed)
    ├── style.css           # Styling (can be inline)
    ├── script.js           # Core logic (can be inline)
    ├── README.md           # Documentation & usage
    └── test/
        └── test.html       # Manual testing page
```

### Key Components

**1. VideoCapture Module**
- Initialize getUserMedia
- Handle permissions
- Manage video stream lifecycle

**2. AsciiConverter Module**
- Pixel sampling logic
- Brightness calculation
- Character mapping

**3. RenderEngine Module**
- DOM manipulation for ASCII display
- Performance optimization (requestAnimationFrame)
- Font/spacing management

**4. UIController Module**
- Resolution slider controls
- Start/stop functionality
- Error display

---

## Testing Strategy

### Manual Testing Checklist

- [ ] Camera permission request works
- [ ] ASCII rendering starts correctly
- [ ] Resolution controls update display
- [ ] Performance is smooth (>15 FPS)
- [ ] Works in Safari on macOS
- [ ] Works in Chrome on macOS
- [ ] Error handling for no camera
- [ ] Error handling for denied permissions

### Automated Testing (Lightweight)

Given the visual and hardware-dependent nature, use:

**Approach 1: Unit Tests for Core Logic**
- Test brightness calculation function
- Test character mapping function
- Test resolution calculation
- No framework needed - simple HTML test runner

**Approach 2: Visual Regression (Manual)**
- Screenshot comparisons
- Performance benchmarking
- Cross-browser testing

**Recommended**: Create a simple `test.html` page with:
- Unit tests for pure functions
- Manual test scenarios documented
- Performance measurement tools

### Testing Framework Options

**None Required** - Use vanilla assertions:
```javascript
function assert(condition, message) {
  if (!condition) throw new Error(message);
}
```

**If Framework Desired** (ranked):
1. **No Framework** ⭐⭐⭐⭐⭐ - Simple assertions in HTML
2. **Jest (lightweight config)** ⭐⭐⭐⭐ - If npm is acceptable
3. **QUnit** ⭐⭐⭐ - Minimal jQuery-like test framework

---

## Performance Targets

- **Frame Rate**: 15-30 FPS (adjust based on resolution)
- **Load Time**: <1 second
- **Memory**: <50MB
- **Latency**: <100ms between camera and display

---

## Browser Compatibility

**Primary Targets**:
- Safari 14+ (macOS Big Sur on MacBook Air 2020)
- Chrome 90+

**getUserMedia Support**: ✅ Both supported

---

## Implementation Plan

### Phase 1: Core Functionality (Day 1)
1. ✅ Research and PRD
2. Create basic HTML structure
3. Implement webcam capture
4. Implement basic ASCII conversion
5. Test on MacBook Air 2020

### Phase 2: Controls & Polish (Day 1-2)
1. Add resolution controls
2. Implement UI styling (black/white theme)
3. Add error handling
4. Performance optimization

### Phase 3: Testing (Day 2)
1. Create test suite
2. Cross-browser testing
3. Documentation

---

## Success Metrics

- ✅ App loads in single HTML file
- ✅ Works on MacBook Air 2020 Safari/Chrome
- ✅ Smooth real-time conversion (>15 FPS)
- ✅ Resolution adjustable via UI
- ✅ Black background, white text
- ✅ No build process required
- ✅ Basic test coverage

---

## Dependencies

**None** - Pure vanilla JavaScript

Optional CDN for advanced features:
- None required for MVP

---

## Code Quality Standards

- ESLint rules (if desired): Standard JS style
- Code comments for complex algorithms
- Clear variable naming
- Modular function design
- Error handling for all async operations

---

## Deployment

**Method**: Static file hosting
- Local file:// protocol
- GitHub Pages
- Any static web server
- Simple Python server: `python3 -m http.server`

---

## Risk Analysis

| Risk | Impact | Mitigation |
|------|--------|------------|
| Camera permission denied | High | Clear UI instructions, fallback message |
| Low performance on older devices | Medium | Adjustable resolution, FPS limiting |
| Browser compatibility | Low | Focus on Safari/Chrome, test early |
| getUserMedia HTTPS requirement | Medium | Use localhost or file:// for testing |

---

## Conclusion

**Recommended Approach**: Vanilla JavaScript implementation with single-file deployment.

**Rationale**:
- Meets all requirements
- Zero dependencies
- Optimal performance
- Simple maintenance
- Fast development
- Easy testing

**Next Steps**: Proceed with implementation using vanilla JS approach.
