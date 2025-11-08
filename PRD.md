# Product Requirements Document: wibwob-dos
## Text-Native Operating System for Text-Native Intelligence

**Version:** 1.0
**Date:** November 8, 2025
**Product Name:** wibwob-dos (Wib and Wob Dual Operating System)
**Vision:** A text-native operating system designed for seamless collaboration between human intelligence and language model intelligence.

---

## Executive Summary

**wibwob-dos** is a revolutionary text-based user interface (TUI) operating system built on the Turbo Vision framework. It provides a dual-interface environment where both humans and AI language models have equal control over the operating system and user interface. The system features a chat module and comprehensive API for dynamically spawning and populating windows with content, enabling real-time collaboration between text-native intelligences.

The name reflects its dual nature:
- **Wib**: The artist (creative, human-driven interaction)
- **Wob**: The scientist (analytical, AI-driven processing)

---

## Phase 1: Core Application Restructuring & Documentation

### Objectives
1. Consolidate the main application architecture
2. Establish clear documentation and codebase structure
3. Identify and document all functional vs. placeholder features
4. Create foundation for future API development

### Deliverables

#### 1.1 Directory Restructuring

**Current Structure:**
```
/tvision
  /examples         # Multiple demo applications
    /tvdemo         # Main demonstration app (to be promoted)
    /tvedit         # Text editor example
    /tvdir          # Directory browser
    /mmenu          # Menu example
    /palette        # Palette example
    /others...
  /source           # Turbo Vision library source
  /include          # Library headers
  /test             # Unit tests
  hello.cpp         # Basic example
```

**New Structure:**
```
/wibwob-dos
  /app              # Main wibwob-dos application (formerly tvdemo)
    /src            # Application source files
    /include        # Application headers
    /resources      # Help files, assets
    CMakeLists.txt
  /lib              # Turbo Vision library (formerly /source)
    /source
    /include
  /workings         # Experimental code and alternative examples
    /text-editor    # tvedit - functional text editor
    /file-browser   # tvdir - directory browser
    /examples       # Other examples (mmenu, palette, etc.)
    /prototypes     # Experimental features
  /tests            # Unit and integration tests
  /docs             # Comprehensive documentation
    /api            # API documentation
    /user-guide     # User documentation
    /development    # Development guides
  CMakeLists.txt    # Root build configuration
  README.md         # Primary documentation
  CLAUDE.md         # AI collaboration guide
  PRD.md            # This document
```

#### 1.2 Core Application Features Documentation

**Fully Functional Features:**

1. **Window Management System**
   - Resize/Move windows (`Ctrl+F5`)
   - Zoom windows (`F5`)
   - Navigate between windows (`F6`)
   - Close windows (`Alt+F3`)
   - Tile windows automatically
   - Cascade windows
   - Desktop persistence (save/restore)

2. **Interactive Tools**
   - **Puzzle Game**: Fully functional sliding tile puzzle
   - **Calendar**: Interactive calendar viewer with date navigation
   - **ASCII Table**: Complete ASCII character reference
   - **Calculator**: Functional calculator with basic operations
   - **Event Viewer**: Real-time event debugging tool (`Alt+0`)
   - **File Viewer**: Text file browser with Unicode support

3. **System Features**
   - **File Operations**: Open files with dialog (`F3`)
   - **Directory Navigation**: Change directory dialog
   - **Mouse Control**: Configure mouse behavior
   - **Color Customization**: Full palette editor for UI theming
   - **Background Patterns**: Customizable desktop background
   - **Help System**: Context-sensitive help (`F1`)

4. **Core Infrastructure**
   - Clock display (top-right)
   - Heap memory monitor (bottom-right)
   - Status bar with keyboard shortcuts
   - Menu system with keyboard navigation
   - UTF-8 Unicode support
   - 24-bit color support
   - Cross-platform (Linux, Windows, macOS)

**Placeholder/Partially Implemented Features:**

1. **DOS Shell** (`File > DOS Shell`)
   - Menu item exists
   - Handled by TApplication default handler
   - Functional on original DOS/Windows
   - Needs modernization for Unix systems

**Feature Status Matrix:**

| Menu Item | Keyboard | Status | Notes |
|-----------|----------|--------|-------|
| About | - | ✅ Functional | Static dialog |
| Video Mode | - | ⚠️ DOS Only | Screen mode switching |
| Puzzle | - | ✅ Functional | Complete game |
| Calendar | - | ✅ Functional | Date navigation |
| ASCII Table | - | ✅ Functional | Full character set |
| Calculator | - | ✅ Functional | Basic operations |
| Event Viewer | Alt+0 | ✅ Functional | Debug tool |
| Open File | F3 | ✅ Functional | File browser |
| Change Dir | - | ✅ Functional | Directory dialog |
| DOS Shell | - | ⚠️ Partial | Needs modernization |
| Exit | Alt+X | ✅ Functional | Clean shutdown |
| Resize/Move | Ctrl+F5 | ✅ Functional | Window operations |
| Zoom | F5 | ✅ Functional | Maximize/restore |
| Next Window | F6 | ✅ Functional | Window cycling |
| Close | Alt+F3 | ✅ Functional | Close active |
| Tile | - | ✅ Functional | Auto-arrange |
| Cascade | - | ✅ Functional | Overlap windows |
| Mouse Config | - | ✅ Functional | Mouse settings |
| Colors | - | ✅ Functional | Palette editor |
| Background | - | ✅ Functional | Pattern chooser |
| Save Desktop | - | ✅ Functional | Persist layout |
| Restore Desktop | - | ✅ Functional | Load layout |

#### 1.3 Documentation Deliverables

1. **CLAUDE.md** - AI Collaboration Guide
   - Project architecture
   - API design philosophy
   - Coding conventions for AI assistance
   - How LMs can interact with the codebase

2. **README.md** - Updated for wibwob-dos
   - New project identity
   - Quick start guide
   - Build instructions
   - Feature overview

3. **FUNCTIONALITY_REPORT.md** - Comprehensive Feature Analysis
   - Detailed feature descriptions
   - Code architecture documentation
   - Module dependencies
   - Extension points for future development

4. **API_DESIGN.md** - Future API Specification (Phase 2 preview)
   - Window spawning API
   - Text population mechanisms
   - Event handling for LM integration
   - Chat module architecture

#### 1.4 Success Criteria

- [x] All source files reorganized into new structure
- [ ] Build system (CMake) updated and functional
- [ ] All applications compile and run correctly
- [ ] Documentation complete and accurate
- [ ] Git history preserved
- [ ] No functionality regression

---

## Phase 2: CLI/Terminal Distribution & Shareability

### Objectives
1. Make wibwob-dos easily distributable via package managers
2. Enable simple installation and execution via terminal
3. Create portable binaries for multiple platforms
4. Establish CI/CD pipeline for automated builds

### Deliverables

#### 2.1 Package Distribution

**Platform-Specific Packages:**

1. **Linux**
   - **Debian/Ubuntu**: `.deb` package
   - **Red Hat/Fedora**: `.rpm` package
   - **Arch Linux**: AUR package
   - **Universal**: AppImage or Snap package
   - **Package Managers**:
     - apt: `sudo apt install wibwob-dos`
     - dnf: `sudo dnf install wibwob-dos`
     - pacman: `pacman -S wibwob-dos`

2. **macOS**
   - **Homebrew Formula**: `brew install wibwob-dos`
   - **MacPorts**: `port install wibwob-dos`
   - Standalone `.pkg` installer

3. **Windows**
   - **Chocolatey**: `choco install wibwob-dos`
   - **Scoop**: `scoop install wibwob-dos`
   - **WinGet**: `winget install wibwob-dos`
   - Standalone `.exe` installer
   - Portable `.zip` distribution

4. **Cross-Platform**
   - **vcpkg**: Already supported - enhance visibility
   - **Conda**: `conda install wibwob-dos`
   - **Docker**: `docker run -it wibwob-dos`

#### 2.2 Terminal Shareability Features

**Installation Methods:**

```bash
# Quick install script (curl-to-bash)
curl -sSL https://wibwob.sh | bash

# Or with wget
wget -qO- https://wibwob.sh | bash

# Direct binary download
wget https://releases.wibwob.dev/latest/wibwob-dos-linux-amd64
chmod +x wibwob-dos-linux-amd64
./wibwob-dos-linux-amd64
```

**Remote Session Sharing:**

1. **SSH Integration**
   - Optimized for SSH sessions
   - Automatic terminal detection
   - Bandwidth optimization for remote use
   - Session recording/playback

2. **tmux/screen Integration**
   - Attach/detach support
   - Multi-user collaboration in same session
   - Persistent sessions across disconnects

3. **Cloud Terminal Support**
   - Google Cloud Shell
   - AWS CloudShell
   - Azure Cloud Shell
   - GitHub Codespaces
   - Replit, Glitch, etc.

#### 2.3 Binary Distribution Strategy

**Release Artifacts (per release):**
- `wibwob-dos-linux-x86_64` (static binary)
- `wibwob-dos-linux-aarch64` (ARM64)
- `wibwob-dos-darwin-x86_64` (macOS Intel)
- `wibwob-dos-darwin-aarch64` (macOS Apple Silicon)
- `wibwob-dos-windows-x86_64.exe`
- `wibwob-dos-windows-arm64.exe`

**Hosting Options:**
1. **GitHub Releases**: Primary distribution
2. **CDN**: Fast global delivery
3. **Registry Mirrors**: Regional availability

#### 2.4 CI/CD Pipeline

**GitHub Actions Workflow:**
```yaml
name: Build and Release
on: [push, tag]

jobs:
  build:
    strategy:
      matrix:
        os: [ubuntu-latest, macos-latest, windows-latest]
        arch: [x64, arm64]

    steps:
      - Build static binaries
      - Run test suite
      - Package for distribution
      - Upload to releases
      - Publish to package registries
```

**Automated Testing:**
- Unit tests
- Integration tests
- Cross-platform compatibility tests
- Performance benchmarks
- Memory leak detection

#### 2.5 Success Criteria

- [ ] One-command installation on all major platforms
- [ ] Static binaries under 5MB
- [ ] Launch time under 100ms
- [ ] Works in constrained environments (256MB RAM)
- [ ] Perfect SSH terminal compatibility
- [ ] Automated builds on every release tag
- [ ] Package manager submissions accepted

---

## Phase 3: Web Wrapper & Browser-Based Access

### Objectives
1. Enable browser-based access to wibwob-dos
2. Maintain full functionality in web environment
3. Support collaborative multi-user sessions
4. Provide embeddable widget for documentation/demos

### Deliverables

#### 3.1 Web Terminal Framework Evaluation

**Option 1: xterm.js + WebSocket Backend (Recommended)**

**Pros:**
- ✅ Industry standard (used by VS Code, Jupyter)
- ✅ Excellent terminal emulation (xterm.js)
- ✅ Real WebSocket communication
- ✅ Active maintenance and community
- ✅ Full ANSI/VT100 support
- ✅ Copy/paste, mouse support
- ✅ Supports terminal protocols (Sixel, images)

**Cons:**
- ⚠️ Requires WebSocket server component
- ⚠️ More complex deployment

**Architecture:**
```
[Browser] ←→ xterm.js ←→ WebSocket ←→ wibwob-dos process
```

**Implementation:**
- Frontend: xterm.js + xterm-addon-fit, xterm-addon-web-links
- Backend: Go/Rust/Node.js WebSocket server
- Spawns wibwob-dos process per session
- PTY (pseudo-terminal) interface

**Example Projects:**
- [gotty](https://github.com/yudai/gotty) - Go-based
- [ttyd](https://github.com/tsl0922/ttyd) - C-based
- [wetty](https://github.com/butlerx/wetty) - Node.js-based

**Estimated Effort:** 2-3 weeks

---

**Option 2: WebAssembly (WASM) Compilation**

**Pros:**
- ✅ Runs entirely in browser (no server-side process)
- ✅ Instant startup
- ✅ Offline capable
- ✅ Maximum portability
- ✅ Can embed in static sites

**Cons:**
- ⚠️ Significant porting effort (ncurses → canvas/DOM)
- ⚠️ File system limitations (virtual FS)
- ⚠️ May not support all Turbo Vision features
- ⚠️ Binary size concerns

**Architecture:**
```
[Browser] ←→ WASM Module (wibwob-dos) ←→ Canvas/Terminal Emulator
```

**Implementation:**
- Compile Turbo Vision to WASM with Emscripten
- Port ncurses backend to canvas rendering
- Use IndexedDB for file storage
- ServiceWorker for offline support

**Example Projects:**
- [Turbo editor WASM port](https://github.com/magiblot/turbo) (partial)

**Estimated Effort:** 6-8 weeks (significant porting)

---

**Option 3: VNC/Remote Desktop Protocol**

**Pros:**
- ✅ Zero code changes to wibwob-dos
- ✅ Standard protocol (VNC/RDP)
- ✅ Existing client libraries (noVNC)
- ✅ Full graphical fidelity

**Cons:**
- ⚠️ Requires full X11/framebuffer for TUI (overkill)
- ⚠️ Higher latency than native terminal
- ⚠️ More bandwidth intensive
- ⚠️ Server resource intensive

**Architecture:**
```
[Browser] ←→ noVNC ←→ VNC Server ←→ Virtual X11 ←→ wibwob-dos
```

**Implementation:**
- Run wibwob-dos in Xvfb (virtual framebuffer)
- x11vnc or TigerVNC server
- noVNC client in browser

**Estimated Effort:** 1-2 weeks (configuration heavy)

---

#### 3.2 Recommendation Matrix

| Criteria | xterm.js + WS | WebAssembly | VNC |
|----------|---------------|-------------|-----|
| **Performance** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ |
| **Development Effort** | ⭐⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐⭐⭐ |
| **Maintenance** | ⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐ |
| **Feature Completeness** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| **Deployment Complexity** | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐ |
| **Multi-user Support** | ⭐⭐⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐⭐ |
| **Offline Capability** | ⭐ | ⭐⭐⭐⭐⭐ | ⭐ |
| **Resource Usage** | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐ |

**Final Recommendation: xterm.js + WebSocket** ⭐⭐⭐⭐⭐

**Rationale:**
- Best balance of features, performance, and development effort
- Proven at scale (VS Code, AWS CloudShell)
- Enables multi-user collaboration features
- Maintains full terminal compatibility
- Can add WASM support later for offline mode

**Hybrid Approach (Future):**
- Primary: xterm.js + WebSocket for full features
- Secondary: WASM build for embedded demos/documentation
- Fallback: VNC for legacy browser support

#### 3.3 Web Application Features

**Core Features:**

1. **Instant Demo Mode**
   - Embeddable `<iframe>` or widget
   - Read-only demo with sample content
   - No server-side state required
   - Perfect for documentation

2. **Full Interactive Mode**
   - WebSocket-based real-time connection
   - Full keyboard and mouse support
   - File upload/download
   - Clipboard integration
   - Session persistence

3. **Collaborative Sessions**
   - Multi-user shared sessions
   - Real-time cursor tracking
   - Chat integration
   - Session recording/replay
   - Invite links for collaboration

4. **Cloud Storage Integration**
   - Save/load files to browser storage
   - GitHub integration (open repos)
   - Google Drive, Dropbox connectors
   - WebDAV support

**Example Deployment:**
```
https://app.wibwob.dev/          # Full application
https://demo.wibwob.dev/         # Read-only demo
https://embed.wibwob.dev/        # Embeddable widget
```

#### 3.4 Implementation Roadmap

**Week 1-2: Core Infrastructure**
- Set up xterm.js frontend
- Create WebSocket server (Go/Node.js)
- PTY process management
- Basic authentication

**Week 3-4: Feature Parity**
- File operations (upload/download)
- Clipboard support
- Session management
- Configuration persistence

**Week 5-6: Collaboration Features**
- Multi-user sessions
- Session sharing
- Chat integration
- Recording/replay

**Week 7-8: Polish & Deployment**
- Performance optimization
- Security hardening
- CDN deployment
- Documentation

#### 3.5 Success Criteria

- [ ] Web application loads in under 2 seconds
- [ ] No perceptible input lag (<50ms)
- [ ] Full keyboard shortcut support
- [ ] Works on mobile browsers (responsive)
- [ ] Supports concurrent sessions (100+ users per server)
- [ ] Embeddable in external websites
- [ ] Session sharing with unique URLs
- [ ] 99.9% uptime SLA

---

## Technical Architecture

### API Design Philosophy (Phase 2+)

**Core Principles:**

1. **LM-First Design**
   - API optimized for programmatic control
   - Natural language command parsing
   - Stateless operations where possible
   - Comprehensive event system

2. **Window Management API**
   ```cpp
   // Proposed API (Phase 2)
   class WindowAPI {
   public:
       WindowHandle spawn(WindowConfig config);
       void setText(WindowHandle handle, const std::string& content);
       void append(WindowHandle handle, const std::string& content);
       void setTitle(WindowHandle handle, const std::string& title);
       void close(WindowHandle handle);

       // Event subscription
       void onEvent(WindowHandle handle, EventType type, CallbackFn callback);
   };
   ```

3. **Chat Module Integration**
   ```cpp
   // Proposed API (Phase 2)
   class ChatModule {
   public:
       void sendMessage(const std::string& message, ChatRole role);
       void registerHandler(MessageHandler handler);
       void executeCommand(const std::string& command);
   };
   ```

### Data Flow Architecture

```
Human Input → Event System → Application Logic → UI Update → Screen
     ↓                              ↑
LM Input  → API Interface → Command Parser ──→ ─┘
     ↓
Chat Module → Context Manager → Response Generator
```

---

## Non-Functional Requirements

### Performance
- Startup time: <500ms (native), <2s (web)
- Input latency: <16ms (60 FPS)
- Memory usage: <50MB baseline
- CPU: <5% idle, <30% active

### Security
- No arbitrary code execution
- Sandboxed file access
- Input sanitization
- Rate limiting for web API
- HTTPS/WSS only for web

### Accessibility
- Full keyboard navigation
- Screen reader compatibility (terminal-based)
- Configurable color schemes (high contrast)
- Font size adjustments

### Compatibility
- Linux: kernel 3.2+ (glibc 2.17+)
- macOS: 10.13+
- Windows: Windows 7+
- Browsers: Chrome 90+, Firefox 88+, Safari 14+

---

## Success Metrics

### Phase 1 (Foundation)
- [x] Code reorganization complete
- [ ] Build system functional
- [ ] Documentation coverage >80%
- [ ] Zero regression bugs

### Phase 2 (Distribution)
- [ ] Available on 3+ package managers per platform
- [ ] 1000+ downloads in first month
- [ ] <10 installation issues reported
- [ ] 5-star average rating

### Phase 3 (Web)
- [ ] Web app live and accessible
- [ ] 100+ concurrent users supported
- [ ] <100ms P95 latency
- [ ] Embedded in 10+ external sites

---

## Risks and Mitigation

| Risk | Impact | Probability | Mitigation |
|------|--------|-------------|------------|
| Build system complexity | High | Medium | Incremental migration, extensive testing |
| Package manager rejections | Medium | Low | Follow guidelines, seek maintainer feedback |
| Web performance issues | High | Medium | Extensive profiling, CDN usage |
| WASM porting difficulty | High | High | Start with xterm.js, defer WASM to Phase 4 |
| Breaking API changes | High | Low | Semantic versioning, deprecation warnings |

---

## Timeline

**Phase 1:** 2 weeks (November 8-22, 2025)
**Phase 2:** 4 weeks (November 22 - December 20, 2025)
**Phase 3:** 8 weeks (December 20, 2025 - February 14, 2026)

**Total Duration:** 14 weeks (3.5 months)

---

## Stakeholders

**Primary:**
- Open source contributors
- Turbo Vision community
- Terminal application developers
- AI/LM researchers

**Secondary:**
- System administrators
- DevOps engineers
- Education/training sector
- Retro computing enthusiasts

---

## Appendix

### Related Projects
- [Turbo Vision](https://github.com/magiblot/tvision) - Base framework
- [Turbo](https://github.com/magiblot/turbo) - Text editor using TV
- [TMBASIC](https://github.com/electroly/tmbasic) - BASIC interpreter with TV
- [far2l](https://github.com/elfmz/far2l) - File manager with terminal extensions

### References
- [Borland Turbo Vision Programming Guide](https://archive.org/details/bitsavers_borlandTurrogrammingGuide1992_25707423)
- [Modern ncurses Programming](https://tldp.org/HOWTO/NCURSES-Programming-HOWTO/)
- [xterm.js Documentation](https://xtermjs.org/)
- [WebAssembly Documentation](https://webassembly.org/)

---

**Document Control:**
- Version: 1.0
- Last Updated: 2025-11-08
- Next Review: 2025-11-22
- Owner: wibwob-dos core team
