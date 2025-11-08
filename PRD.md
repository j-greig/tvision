# Product Requirements Document: wibwob-dos
## A Text-Native Operating System for Text-Native Intelligence

**Version:** 2.0
**Date:** November 8, 2025
**Product Name:** wibwob-dos (Wib and Wob - Dual Operating System)
**Tagline:** "A text-native operating system for text-native intelligence"
**Current Codename:** test-tui / test_pattern_app

---

## Executive Summary

**wibwob-dos** is a revolutionary text-based user interface (TUI) operating system that provides equal control to both human and AI intelligences. Built on the Turbo Vision framework, it features a sophisticated window management system, generative art engines, API-driven automation, and an embedded AI chat interface (Wib&Wob).

**Core Vision:**
- **Wib** (つ◕‿◕‿⚆༽つ): The artist - chaotic creativity, ASCII art, surreal experiences
- **Wob** (つ⚆‿◕‿◕༽つ): The scientist - methodical analysis, precise control, structured systems

**Current State:** Highly functional prototype with 40+ features, REST API integration, MCP support for AI agents, and embedded LLM chat.

---

## Current Application Analysis

### Application Architecture

**Primary Application:** `test_pattern_app.cpp` (2,800+ lines)
**Supporting Modules:** 38 C++ files, 50+ custom view classes
**Total Codebase:** ~15,000 lines of application code

**Key Components:**
- Window manager with unlimited window support
- API/IPC server (Unix domain socket at `/tmp/test_pattern_app.sock`)
- Window registry system for programmatic control
- Generative art engines (8+ algorithms)
- Frame-based animation system
- Glitch effects engine
- Workspace persistence (JSON)
- Screenshot capture system
- Embedded AI chat (Wib&Wob)

### Feature Inventory: Functional vs Placeholder

#### ✅ **FULLY FUNCTIONAL** (95% of features)

**File Menu:**
- ✅ New Test Pattern (Ctrl+N) - Unlimited windows with configurable patterns
- ✅ New H-Gradient - Horizontal color gradients
- ✅ New V-Gradient - Vertical color gradients
- ✅ New Radial Gradient - Radial/circular gradients
- ✅ New Diagonal Gradient - Diagonal color gradients
- ✅ New Mechs Grid (Ctrl+M) - Mech grid visualization
- ✅ New Animation (Ctrl+D) - Animated donut/effects
- ✅ Open Text/Animation (Ctrl+O) - Load text or frame-delimited animations
- ✅ Open Image - ASCII art image viewer (STB image library)
- ✅ Open Monodraw - Monodraw JSON file viewer
- ✅ Save Workspace (Ctrl+S) - Serialize all windows to JSON
- ✅ Open Workspace - Restore saved workspace
- ✅ Exit (Alt+X) - Clean shutdown

**Edit Menu:**
- ✅ Screenshot (Ctrl+P) - Capture entire screen to file
- ✅ Pattern Mode → Continuous (Diagonal) - Flowing diagonal patterns
- ✅ Pattern Mode → Tiled (Cropped) - Cropped/tiled patterns

**View Menu:**
- ✅ ASCII Grid Demo - ASCII character grid demonstration
- ✅ Animated Blocks - Color-changing block animation
- ✅ Animated Gradient - Flowing gradient animation
- ✅ Animated Score - Musical score ASCII animation
- ✅ Score BG Color - Background color picker for score
- ✅ Verse Field (Generative) - Verse field generative art
- ✅ Orbit Field (Generative) - Orbital motion visualization
- ✅ Mycelium Field (Generative) - Mycelium growth simulation
- ✅ Torus Field (Generative) - 3D torus rendering
- ✅ Cube Spinner (Generative) - Rotating 3D cube
- ✅ Monster Portal (Generative) - Emoji tile patterns
- ✅ Monster Verse (Generative) - Monster emoji verse field
- ✅ Monster Cam (Emoji) - Emoji camera effect
- ⚠️ Zoom In/Out/Actual Size - Placeholder (no implementation)
- ⚠️ Full Screen (F11) - Placeholder (conflicts with Quantum Printer)

**Window Menu:**
- ✅ Edit Text Editor - API-controllable text editor window
- ✅ Open Text File (Transparent BG) - Text viewer with transparent background
- ✅ Cascade - Arrange windows in cascade
- ✅ Tile - Tile windows across desktop
- ✅ Send to Back - Z-order control
- ✅ Next (F6) - Cycle to next window
- ✅ Previous (Shift+F6) - Cycle to previous window
- ✅ Close (Alt+F3) - Close active window
- ✅ Close All - Close all windows
- ✅ Background Color - Desktop background color picker

**Tools Menu:**
- ✅ Wib&Wob Chat (F12) - Embedded AI chat with Claude Code CLI + MCP
- ✅ Glitch Effects → Enable Glitch Mode (Ctrl+G) - Toggle glitch engine
- ✅ Glitch Effects → Scatter Pattern - Scatter glitch effect
- ✅ Glitch Effects → Color Bleed - Color bleeding effect
- ✅ Glitch Effects → Radial Distort - Radial distortion
- ✅ Glitch Effects → Diagonal Scatter - Diagonal scatter effect
- ✅ Glitch Effects → Capture Frame (F9) - Save glitched frame
- ✅ Glitch Effects → Reset Parameters - Reset glitch settings
- ✅ Glitch Effects → Glitch Settings - Configure glitch parameters
- ⚠️ ANSI Editor - Placeholder (no implementation)
- ⚠️ Paint Tools - Placeholder (no implementation)
- ⚠️ Animation Studio - Placeholder (no implementation)
- ⚠️ Quantum Printer (F11) - Placeholder (no implementation)

**Help Menu:**
- ✅ About WIBWOBWORLD - About dialog

**Programmatic API (REST + MCP):**
- ✅ GET /state - Get application state and window list
- ✅ POST /windows - Create window (test_pattern, gradient, frame_player, text_view)
- ✅ POST /windows/{id}/move - Move/resize window
- ✅ POST /windows/{id}/focus - Focus window
- ✅ POST /windows/{id}/close - Close window
- ✅ POST /windows/cascade - Cascade all windows
- ✅ POST /windows/tile - Tile windows
- ✅ POST /windows/close_all - Close all windows
- ✅ POST /pattern_mode - Set pattern mode (continuous/tiled)
- ✅ POST /screenshot - Take screenshot
- ✅ POST /workspace/save - Save workspace to file
- ✅ POST /workspace/load - Load workspace from file
- ✅ POST /send_text - Send text to text editor window
- ✅ POST /send_figlet - Send FIGlet text to window
- ✅ GET /primers/list - List all primer files (128 available)
- ✅ POST /primers/batch - Spawn multiple primer text windows
- ✅ WebSocket at /ws - Real-time event stream

#### ⚠️ **PLACEHOLDER/PARTIAL** (5% of features)

1. **Zoom Controls** - Menu items present, no implementation
2. **ANSI Editor** - Menu item only
3. **Paint Tools** - Menu item only
4. **Animation Studio** - Menu item only
5. **Quantum Printer** - Menu item only (keycode conflict with Full Screen)

### Technical Capabilities

**Window System:**
- Unlimited concurrent windows
- Per-window stable IDs (w1, w2, ... w999+)
- Dynamic registration/de-registration
- Automatic bounds calculation for content
- Auto-sizing for text files and animations
- Z-order management

**Animation System:**
- Timer-based animations (no threads)
- Multiple concurrent animations
- Configurable FPS per animation
- Frame file format (---- delimiters)
- Real-time generative art

**API Integration:**
- FastAPI REST server (tools/api_server/)
- Unix socket IPC bridge
- MCP (Model Context Protocol) endpoint at /mcp
- WebSocket event streaming
- JSON workspace format
- Batch operations (primers)

**AI Integration:**
- Wib&Wob chat window
- Claude Code CLI backend
- MCP tool access (window management, state queries)
- Configurable LLM provider (Haiku/Sonnet/Opus)
- Personality prompts (wibandwob.prompt.md)
- Session logging

---

## Phase 1: Core Refactoring & Documentation

### Objectives
1. Establish wibwob-dos as the primary project identity
2. Reorganize codebase for clarity and maintainability
3. Create comprehensive documentation
4. Consolidate and clean up experimental code

### Deliverables

#### 1.1 Directory Restructuring

**Current Structure:**
```
/tvision
  /test-tui          # Main application (to be promoted)
  /examples          # Legacy Turbo Vision examples
    /tvdemo
    /tvedit
    /tvdir
    /others...
  /source            # Turbo Vision library
  /include           # Library headers
  /tools             # API server, utilities
```

**New Structure (wibwob-dos):**
```
/wibwob-dos
  /app               # Main wibwob-dos application (formerly test-tui)
    /src             # Application source files
    /views           # Custom view classes
    /llm             # LLM integration (chat, config)
    /primers         # Primer text files
    /workspaces      # Saved workspaces
    /ansi            # ANSI art files
    /images          # Image files
    CMakeLists.txt
    README.md
  /lib               # Turbo Vision library (formerly /source + /include)
    /source          # Library implementation
    /include         # Public headers
  /tools             # API server and utilities
    /api_server      # FastAPI REST + MCP server
  /workings          # Experimental/reference code
    /examples        # Original TV examples (tvdemo, tvedit, etc.)
    /prototypes      # Experimental features
  /docs              # Comprehensive documentation
    /api             # API documentation
    /user-guide      # User manual
    /development     # Developer guide
  CMakeLists.txt     # Root build
  README.md          # Project overview
  CLAUDE.md          # AI collaboration guide
  PRD.md             # This document
  FUNCTIONALITY_REPORT.md  # Feature analysis
```

#### 1.2 File Reorganization

**Move to /app:**
- test-tui/* → app/src/
- test-tui/llm/ → app/llm/
- test-tui/primers/ → app/primers/
- test-tui/workspaces/ → app/workspaces/
- test-tui/.claude/ → app/.claude/

**Move to /workings:**
- examples/* → workings/examples/
- hello.cpp → workings/examples/hello/

**Move to /lib:**
- source/tvision/ → lib/source/
- include/tvision/ → lib/include/

#### 1.3 Documentation Deliverables

**✅ COMPLETE:**
- **PRD.md** - This document (Product Requirements)
- **FUNCTIONALITY_REPORT.md** - Comprehensive feature analysis

**🚧 TODO:**
- **README.md** - Update for wibwob-dos identity
- **CLAUDE.md** - Update with new structure and vision
- **docs/USER_GUIDE.md** - End-user documentation
- **docs/API_REFERENCE.md** - API endpoint documentation
- **docs/DEVELOPER_GUIDE.md** - Development setup and architecture

#### 1.4 Success Criteria

- [ ] All source files reorganized
- [ ] Build system updated and functional
- [ ] All applications compile successfully
- [ ] No functionality regression
- [ ] Documentation coverage >90%
- [ ] Git history preserved
- [ ] API server still functional

---

## Phase 2: CLI/Terminal Distribution & Shareability

### Objectives
1. Make wibwob-dos easily installable via standard package managers
2. Enable one-command installation on all major platforms
3. Create portable standalone binaries
4. Establish CI/CD for automated releases

### Deliverables

#### 2.1 Package Distribution Strategy

**Linux Packages:**
- **Debian/Ubuntu**: `.deb` package via PPA or GitHub releases
- **Arch Linux**: AUR package (`wibwob-dos`, `wibwob-dos-git`)
- **Fedora/RHEL**: `.rpm` package via COPR
- **Universal**: AppImage, Snap, or Flatpak
- **Install commands:**
  ```bash
  # Ubuntu/Debian
  sudo add-apt-repository ppa:wibwob/dos
  sudo apt install wibwob-dos

  # Arch
  yay -S wibwob-dos

  # Fedora
  sudo dnf copr enable wibwob/dos
  sudo dnf install wibwob-dos
  ```

**macOS Packages:**
- **Homebrew**: `brew install wibwob-dos`
- **MacPorts**: `port install wibwob-dos`
- **Standalone**: `.pkg` installer or `.dmg` bundle

**Windows Packages:**
- **Chocolatey**: `choco install wibwob-dos`
- **Scoop**: `scoop install wibwob-dos`
- **WinGet**: `winget install wibwob.dos`
- **Portable**: `.zip` with static binary

**Universal Install Script:**
```bash
# One-liner install (detects OS and installs)
curl -sSL https://get.wibwob.dev | bash

# Or with wget
wget -qO- https://get.wibwob.dev | bash

# Direct binary download
curl -LO https://releases.wibwob.dev/latest/wibwob-dos-$(uname -s)-$(uname -m)
chmod +x wibwob-dos-*
./wibwob-dos-*
```

#### 2.2 Binary Distribution

**Release Artifacts (per version):**
- `wibwob-dos-linux-x86_64` (static, glibc 2.17+)
- `wibwob-dos-linux-aarch64` (ARM64)
- `wibwob-dos-darwin-x86_64` (macOS Intel)
- `wibwob-dos-darwin-aarch64` (Apple Silicon)
- `wibwob-dos-windows-x64.exe` (MSVC, Vista+)
- `wibwob-dos-windows-arm64.exe` (ARM64 Windows)

**Static Linking Strategy:**
- ncurses statically linked (Linux)
- No runtime dependencies except libc
- Target size: <5MB per binary (with compression)

#### 2.3 Cloud Terminal Support

**Optimizations for Remote Sessions:**
- Bandwidth optimization (efficient screen updates)
- SSH-friendly operation (respects TERM environment)
- Tmux/screen integration (persistent sessions)
- Cloud shell support:
  - Google Cloud Shell
  - AWS CloudShell
  - Azure Cloud Shell
  - GitHub Codespaces
  - Gitpod

**Sharing Features:**
```bash
# Start session with sharing
wibwob-dos --share

# Join existing session
wibwob-dos --join <session-id>

# Record session (asciinema compatible)
wibwob-dos --record session.cast
```

#### 2.4 CI/CD Pipeline

**GitHub Actions Workflow:**
```yaml
name: Build & Release

on:
  push:
    tags: ['v*']
  pull_request:

jobs:
  build:
    strategy:
      matrix:
        os: [ubuntu-20.04, macos-latest, windows-latest]
        arch: [x64, arm64]

    steps:
      - Build static binaries
      - Run integration tests
      - Package for distribution
      - Upload artifacts
      - Publish to registries (on tag)
```

**Automated Testing:**
- Unit tests (lib/tests/)
- Integration tests (app/tests/)
- API tests (tools/api_server/tests/)
- Cross-platform smoke tests
- Performance benchmarks

**Release Automation:**
- Tag-triggered builds (`v1.0.0`)
- Automatic changelog generation
- GitHub Releases with binaries
- Package registry publishing
- Docker image builds

#### 2.5 Docker Distribution

**Docker Images:**
```bash
# Run wibwob-dos in container
docker run -it wibwob/dos:latest

# With API server
docker run -it -p 8089:8089 wibwob/dos:api

# Persistent workspace
docker run -it -v $(pwd)/workspace:/workspace wibwob/dos:latest
```

**Docker Compose (Full Stack):**
```yaml
version: '3.8'
services:
  wibwob-dos:
    image: wibwob/dos:latest
    ports:
      - "8089:8089"
    volumes:
      - ./workspaces:/app/workspaces
```

#### 2.6 Success Criteria

- [ ] One-command install on 3+ platforms
- [ ] Binary size <5MB (compressed)
- [ ] Launch time <200ms
- [ ] Works in 256MB RAM environments
- [ ] GitHub Actions builds all platforms
- [ ] Homebrew formula accepted
- [ ] AUR package published
- [ ] 100+ installations in first month

---

## Phase 3: Web Wrapper & Browser Access

### Objectives
1. Enable browser-based access to wibwob-dos
2. Maintain full feature parity in web environment
3. Support collaborative multi-user sessions
4. Provide embeddable demos for documentation

### Framework Evaluation & Recommendation

#### **Option 1: xterm.js + WebSocket (RECOMMENDED ⭐⭐⭐⭐⭐)**

**Architecture:**
```
[Browser] ←→ xterm.js ←→ WebSocket ←→ PTY ←→ wibwob-dos process
```

**Pros:**
- ✅ Industry standard (VS Code, Jupyter, AWS CloudShell)
- ✅ Perfect terminal emulation (xterm.js)
- ✅ Full ANSI/Unicode support
- ✅ Mouse and keyboard support
- ✅ Clipboard integration
- ✅ Copy/paste, selection
- ✅ Active maintenance
- ✅ Multi-user capable

**Cons:**
- ⚠️ Requires server-side process per session
- ⚠️ More complex deployment than pure client-side

**Implementation:**
- **Frontend**: xterm.js + xterm-addon-fit + xterm-addon-webgl
- **Backend**: Go/Rust/Node.js WebSocket server
- **PTY**: Spawns wibwob-dos in pseudo-terminal
- **Session management**: Redis or in-memory

**Reference Projects:**
- [ttyd](https://github.com/tsl0922/ttyd) - C/libwebsockets
- [gotty](https://github.com/yudai/gotty) - Go implementation
- [wetty](https://github.com/butlerx/wetty) - Node.js implementation

**Estimated Effort:** 3-4 weeks

---

#### **Option 2: WebAssembly (WASM) Compilation**

**Architecture:**
```
[Browser] ←→ WASM Module (wibwob-dos) ←→ Canvas/DOM Renderer
```

**Pros:**
- ✅ Runs entirely in browser (serverless)
- ✅ Instant startup, no latency
- ✅ Offline capable
- ✅ Can be hosted on static CDN
- ✅ Perfect for demos/documentation

**Cons:**
- ⚠️ Significant porting effort (ncurses → canvas)
- ⚠️ File system limitations (IndexedDB/OPFS)
- ⚠️ API server integration complex
- ⚠️ Binary size concerns (~10-20MB)
- ⚠️ Not all Turbo Vision features may work

**Implementation:**
- Compile with Emscripten
- Port ncurses backend to canvas rendering
- Implement virtual filesystem (IDBFS)
- Service Worker for offline support

**Reference Projects:**
- [Turbo WASM experiments](https://github.com/magiblot/turbo) (partial)
- Various ncurses WASM ports

**Estimated Effort:** 8-12 weeks (heavy porting)

---

#### **Option 3: VNC/Remote Desktop (NOT RECOMMENDED)**

**Architecture:**
```
[Browser] ←→ noVNC ←→ VNC Server ←→ Xvfb ←→ wibwob-dos
```

**Pros:**
- ✅ Zero code changes to wibwob-dos
- ✅ Standard VNC protocol

**Cons:**
- ⚠️ Requires X11/framebuffer for TUI (massive overkill)
- ⚠️ High latency and bandwidth
- ⚠️ Resource intensive
- ⚠️ Poor user experience for text

**Estimated Effort:** 2 weeks (not worth it)

---

### Recommendation Matrix

| Criteria | xterm.js + WS | WASM | VNC |
|----------|---------------|------|-----|
| **Performance** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ |
| **Development Effort** | ⭐⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐⭐ |
| **Feature Parity** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| **Deployment Ease** | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐ |
| **Multi-user** | ⭐⭐⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐ |
| **Offline Support** | ⭐ | ⭐⭐⭐⭐⭐ | ⭐ |
| **Maintenance** | ⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐ |

**Final Recommendation:** **xterm.js + WebSocket** for primary deployment, with **WASM** as Phase 4 for embedded demos.

### Deliverables

#### 3.1 Web Application Features

**Core Functionality:**
- Full wibwob-dos instance in browser
- Real-time terminal emulation
- Keyboard shortcuts (all shortcuts work)
- Mouse support (clicks, drags, scroll)
- Clipboard integration (copy/paste)
- File upload/download
- Workspace persistence (browser storage + backend)

**Collaboration Features:**
- Multi-user shared sessions
- Session sharing via URL (`app.wibwob.dev/session/abc123`)
- Real-time cursor tracking
- Integrated text chat alongside TUI
- Session recording/replay (asciinema format)
- Read-only spectator mode

**Deployment Modes:**
1. **Full App**: `https://app.wibwob.dev/` - Interactive instance
2. **Demo**: `https://demo.wibwob.dev/` - Read-only showcase
3. **Embed**: `<iframe src="https://embed.wibwob.dev/">` - Embeddable widget
4. **API**: `https://api.wibwob.dev/` - WebSocket API endpoint

#### 3.2 Implementation Roadmap

**Week 1-2: Core Infrastructure**
- Set up xterm.js frontend
- WebSocket server (Go/Node.js)
- PTY process spawning
- Basic authentication
- Session management

**Week 3-4: Feature Parity**
- File operations (upload/download)
- Clipboard bidirectional sync
- Configuration persistence
- Error handling and reconnection
- Mobile touch support

**Week 5-6: Collaboration**
- Multi-user session support
- Shared cursor tracking
- Chat integration
- Recording/replay
- Session URLs

**Week 7-8: Polish & Deploy**
- Performance optimization (WebGL renderer)
- Security hardening (rate limiting, input validation)
- CDN deployment (Cloudflare/AWS)
- Documentation and examples
- Load testing (100+ concurrent users)

#### 3.3 Embedding & Documentation

**Embeddable Widget:**
```html
<!-- Embed wibwob-dos in documentation -->
<iframe src="https://embed.wibwob.dev/?demo=generative-art"
        width="800" height="600"
        style="border: 2px solid #000; border-radius: 8px;">
</iframe>

<!-- With specific workspace -->
<iframe src="https://embed.wibwob.dev/?workspace=demos/monster-portal.json">
</iframe>
```

**Customization Options:**
```javascript
// Embed with custom config
<script src="https://cdn.wibwob.dev/embed.js"></script>
<div id="wibwob-container"></div>
<script>
  WibWob.embed('#wibwob-container', {
    readonly: true,
    workspace: 'demos/verse-field.json',
    theme: 'dark',
    width: '100%',
    height: '600px'
  });
</script>
```

#### 3.4 Success Criteria

- [ ] Web app loads in <2 seconds
- [ ] Input latency <50ms (P95)
- [ ] Full keyboard shortcut support
- [ ] Works on mobile browsers
- [ ] 100+ concurrent users per server
- [ ] Session sharing functional
- [ ] Embeddable in external sites
- [ ] 99.9% uptime
- [ ] SEO-friendly landing page

---

## Non-Functional Requirements

### Performance Targets

**Native Application:**
- Startup time: <300ms cold start
- Input latency: <16ms (60 FPS)
- Memory usage: <50MB baseline, <100MB with 20 windows
- CPU: <2% idle, <20% active with animations

**Web Application:**
- Page load: <2s (TTI - Time to Interactive)
- WebSocket latency: <50ms (P95), <100ms (P99)
- Frame rate: 60 FPS in browser
- Memory: <150MB browser tab

**API Server:**
- Request latency: <20ms (local), <100ms (remote)
- Throughput: 1000+ req/sec per core
- WebSocket connections: 1000+ concurrent

### Security

**Application Security:**
- No arbitrary code execution
- Sandboxed file access (configurable allowlist)
- Input validation on all API endpoints
- Rate limiting (10 req/sec per IP)
- Authentication for web interface

**Web Security:**
- HTTPS/WSS only (no HTTP)
- CSP (Content Security Policy) headers
- XSS protection
- CSRF protection
- Session timeout (30 min idle)

### Accessibility

**Terminal Accessibility:**
- Full keyboard navigation (no mouse required)
- Screen reader support (via terminal)
- Configurable color schemes (high contrast, colorblind-friendly)
- Font size adjustments (terminal emulator dependent)

**Web Accessibility:**
- WCAG 2.1 Level AA compliance
- Keyboard navigation in web UI
- Screen reader compatibility
- Adjustable font sizes
- Color contrast ratios >4.5:1

### Compatibility

**Native Support:**
- **Linux**: Kernel 3.2+, glibc 2.17+ (CentOS 7+)
- **macOS**: 10.13+ (High Sierra and later)
- **Windows**: Windows 7+ (Vista with updates)

**Terminal Emulators (Tested):**
- gnome-terminal, konsole, xterm, alacritty, kitty
- iTerm2, Terminal.app (macOS)
- Windows Terminal, ConEmu (Windows)
- tmux, screen (multiplexers)

**Web Browsers:**
- Chrome 90+
- Firefox 88+
- Safari 14+
- Edge 90+

---

## Risk Assessment

| Risk | Impact | Probability | Mitigation |
|------|--------|-------------|------------|
| Directory refactor breaks builds | High | Medium | Incremental migration, extensive testing, rollback plan |
| Package manager rejections | Medium | Low | Follow packaging guidelines, engage maintainers early |
| Web performance issues | High | Medium | Extensive profiling, WebGL optimization, CDN usage |
| WASM porting complexity | High | High | Start with xterm.js, defer WASM to Phase 4 |
| API backwards compatibility | Medium | Low | Semantic versioning, deprecation warnings |
| Security vulnerabilities | High | Medium | Security audit, penetration testing, bug bounty |
| Scaling challenges (web) | High | Medium | Load testing, horizontal scaling, rate limiting |

---

## Timeline & Milestones

### Phase 1: Refactoring (2 weeks)
**Week 1:**
- Directory restructuring
- CMake updates
- Build verification

**Week 2:**
- Documentation creation
- Feature audit
- Git migration

**Milestone**: Clean, well-documented codebase with wibwob-dos identity

---

### Phase 2: Distribution (4 weeks)
**Week 1-2:**
- Static binary builds
- CI/CD setup (GitHub Actions)
- Package templates (deb, rpm, brew)

**Week 3:**
- Package submissions (Homebrew, AUR, etc.)
- Install script development
- Docker images

**Week 4:**
- Testing across platforms
- Documentation for installers
- Release v1.0.0

**Milestone**: wibwob-dos installable on all major platforms

---

### Phase 3: Web (8 weeks)
**Week 1-2:**
- xterm.js frontend prototype
- WebSocket server basic implementation
- PTY integration

**Week 3-4:**
- File operations, clipboard
- Authentication system
- Session management

**Week 5-6:**
- Multi-user collaboration
- Session sharing
- Recording/replay

**Week 7:**
- Performance optimization
- Security hardening
- Load testing

**Week 8:**
- Production deployment
- Documentation
- Launch

**Milestone**: web.wibwob.dev live and accessible

---

**Total Duration**: 14 weeks (3.5 months)

---

## Success Metrics

### Phase 1
- [x] Codebase reorganized
- [ ] All applications build successfully
- [ ] Zero functionality regression
- [ ] Documentation coverage >90%

### Phase 2
- [ ] Available on 3+ package managers per platform
- [ ] 1000+ downloads in first month
- [ ] <10 installation issues reported
- [ ] Average 4.5+ star rating

### Phase 3
- [ ] Web app handling 100+ concurrent users
- [ ] P95 latency <50ms
- [ ] 99.9% uptime over 30 days
- [ ] Embedded in 5+ external sites
- [ ] 10,000+ sessions in first month

---

## Stakeholders

**Primary:**
- Open source contributors
- AI/LM researchers and developers
- Terminal enthusiasts
- Generative art community

**Secondary:**
- Turbo Vision community
- Text-based UI developers
- Education sector (CS curricula)
- DevOps/sysadmin professionals

---

## Appendix A: Technology Stack

### Core Technologies
- **Language**: C++14
- **UI Framework**: Turbo Vision (modern port)
- **Build System**: CMake 3.5+
- **Terminal**: ncurses (Unix), Win32 Console (Windows)

### API Stack
- **API Server**: FastAPI (Python 3.8+)
- **IPC**: Unix domain sockets
- **Protocol**: REST + MCP + WebSocket
- **Serialization**: JSON

### Web Stack (Phase 3)
- **Frontend**: xterm.js + TypeScript
- **Backend**: Go/Node.js
- **Transport**: WebSocket (Socket.IO)
- **Auth**: JWT or session-based

### AI Integration
- **LLM Backend**: Claude Code CLI
- **Protocol**: MCP (Model Context Protocol)
- **Models**: Claude Haiku (default), Sonnet, Opus (configurable)
- **Personality**: Wib&Wob dual persona

---

## Appendix B: File Count & Complexity

**Application Files:**
- C++ source files: 38
- Header files: 38
- Total lines: ~15,000 (application)
- View classes: 50+
- Menu commands: 60+

**Supporting Files:**
- Primer files: 128
- Animation frames: 10+
- Monodraw files: 2
- Workspace templates: 5+

**Library Files:**
- Turbo Vision: ~190 source files
- Total lines: ~100,000 (library + app)

---

## Appendix C: Related Projects

- [Turbo Vision](https://github.com/magiblot/tvision) - Base framework
- [Turbo](https://github.com/magiblot/turbo) - Text editor
- [far2l](https://github.com/elfmz/far2l) - File manager with terminal extensions
- [tmbasic](https://github.com/electroly/tmbasic) - BASIC interpreter
- [asciinema](https://asciinema.org/) - Terminal session recording
- [xterm.js](https://xtermjs.org/) - Terminal in browser

---

**Document Control:**
- **Version**: 2.0
- **Last Updated**: 2025-11-08
- **Next Review**: Phase 1 completion
- **Owner**: wibwob-dos core team
- **Status**: Active Development

