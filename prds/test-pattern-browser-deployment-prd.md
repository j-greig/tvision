# Test Pattern App Browser Deployment PRD

## TL;DR
Three viable approaches: 1) Emscripten+xterm.js (90% functionality, complex), 2) JavaScript rewrite with blessed.js (80% functionality, medium effort), 3) Canvas-based renderer (95% functionality, high effort). Recommended: blessed.js rewrite for optimal dev experience.

## Research Findings

### Approach 1: Emscripten + xterm.js Terminal Emulation
**Description**: Compile existing C++ Turbo Vision code to WebAssembly, run in browser terminal emulator.

**Pros:**
- Preserves 90%+ of existing codebase
- Maintains exact TUI behavior and rendering
- Leverages proven Emscripten toolchain
- xterm.js handles terminal emulation robustly

**Cons:** 
- Complex build pipeline (emcc + wasm setup)
- File I/O requires OPFS/IndexedDB bridge layer
- Large bundle size (~2-5MB WASM + JS)
- Limited browser file system access
- Debugging C++ in browser environment

**Implementation Complexity:** High
**Functionality Retention:** 90%

### Approach 2: JavaScript Rewrite with blessed.js
**Description**: Reimplement app architecture in Node.js/browser using blessed TUI library.

**Pros:**
- Native web development workflow
- blessed.js provides TUI widgets similar to Turbo Vision
- Smaller bundle size (~500KB)
- Direct browser API access (File System API, localStorage)
- Easier debugging and iteration
- Progressive web app potential

**Cons:**
- Complete rewrite required (significant dev effort)
- Different rendering behavior than original
- Some Turbo Vision features may need custom implementation
- blessed.js learning curve for team

**Implementation Complexity:** Medium
**Functionality Retention:** 80%

### Approach 3: Custom Canvas-Based Renderer
**Description**: Build custom TUI renderer using HTML5 Canvas with JavaScript recreation of TV concepts.

**Pros:**
- Pixel-perfect control over rendering
- Optimal performance for character-based drawing
- Can match original visual behavior exactly
- Direct browser integration capabilities
- Maximum flexibility for future enhancements

**Cons:**
- Significant development effort (6-12 months)
- No existing TUI framework to build upon
- Complex text rendering, font management, and input handling
- Accessibility challenges (canvas is bitmap-based)
- Need to implement window management from scratch

**Implementation Complexity:** High
**Functionality Retention:** 95%

## Recommended Implementation Plan

**Phase 1: Proof of Concept (2-3 weeks)**
- Set up blessed.js environment
- Implement basic window with ASCII art rendering
- Test file picker integration for primers/ folder
- Create simple menu system prototype

**Phase 2: Core Functionality Port (6-8 weeks)**
- Implement auto-sizing windows with content detection
- Port pattern generators (test patterns, gradients)
- Add tile/cascade window management
- Integrate File System API for local file access

**Phase 3: Feature Parity (4-6 weeks)** 
- Complete menu system with all options
- Add screenshot functionality (download as image)
- Implement workspace save/load (localStorage/IndexedDB)
- Polish UX for web environment (responsive design)

## Technical Requirements

**Build System:**
- Node.js with webpack/vite bundler
- blessed.js + blessed-contrib for TUI widgets
- Modern browser APIs (File System, Canvas for screenshots)

**Dependencies:**
- blessed.js (terminal interface library)
- File System API polyfill for older browsers  
- Canvas/WebGL for image generation
- IndexedDB for persistent storage

**Performance Targets:**
- Initial load: <3 seconds
- Window operations: <100ms response time
- ASCII art rendering: 60fps smooth scrolling
- Bundle size: <1MB gzipped

## Risk Assessment

**Show-stopper Risks:**
- blessed.js limitations vs Turbo Vision capabilities
- File System API browser support (Chrome 86+, limited Safari/Firefox)
- Performance bottlenecks in JavaScript ASCII rendering

**Mitigation Strategies:**
- Create blessed.js prototype early to validate approach
- Implement File System API with fallbacks (drag-drop, manual selection)
- Use web workers for heavy ASCII processing
- Progressive enhancement for older browsers

**Alternative Fallback:** If blessed.js proves insufficient, pivot to Approach 1 (Emscripten) with extended timeline.

## Browser Compatibility

**Target Support:**
- Chrome/Edge 90+ (File System API)
- Firefox 88+ (limited file access)
- Safari 14+ (basic functionality)

**Fallback Strategy:** 
- Drag-and-drop file upload for older browsers
- localStorage-only workspace management
- Graceful degradation of advanced features

## Success Metrics

**Functional Parity:**
- [ ] All window types render correctly
- [ ] File browsing and loading works
- [ ] Menu system fully functional
- [ ] Auto-sizing windows match desktop behavior
- [ ] Tile/cascade operations work smoothly

**Performance Benchmarks:**
- [ ] <3s initial load time
- [ ] <100ms window creation
- [ ] Smooth 60fps scrolling in large files
- [ ] <1MB total bundle size

**User Experience:**
- [ ] Feels like desktop app in browser
- [ ] Responsive design works on tablets
- [ ] Keyboard shortcuts match original
- [ ] File operations are intuitive