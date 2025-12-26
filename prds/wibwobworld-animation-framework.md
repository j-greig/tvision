# WIBWOBWORLD Animation Framework PRD

**tl;dr:** Transform existing WIBWOBWORLD TUI app into modular animation framework with external APIs, timeline-based window manipulation, MCP server integration, and programmatic control for creating animated sequences and agent-driven interactions.

## Context

WIBWOBWORLD is currently a functional TUI application built on Turbo Vision with unlimited resizable windows, gradient effects, ASCII art wallpaper, and window management. The existing codebase demonstrates solid architecture with pattern viewers, screenshot functionality, and custom theming. The goal is to evolve this into a programmable animation framework that can be controlled externally for creating sequences and agent interactions.

## Objective

Create a modular animation framework that transforms WIBWOBWORLD from an interactive TUI application into a programmable system for:
- Creating and manipulating window arrangements programmatically
- Timeline-based animation sequences
- External API control via JSON-RPC or similar
- MCP server integration for agentic AI manipulation
- Reusable animation primitives and presets

## Current Architecture Analysis

### Existing Components
- **TTestPatternApp**: Main application with event handling, window creation
- **TTestPatternWindow/TGradientWindow**: Resizable windows with pattern/gradient content
- **TWallpaperView**: WIBWOBWORLD ASCII art background
- **TCustomMenuBar**: Kaomoji-enhanced menu system
- **Window Management**: Cascade, tile, close all functionality
- **Screenshot System**: Terminal capture with timestamp
- **Pattern System**: Continuous/tiled modes with configurable rendering
- **Gradient System**: 4 types (horizontal, vertical, radial, diagonal) with 24-bit color

### Architecture Strengths
- Modular window/view separation
- Event-driven architecture via Turbo Vision
- Extensible gradient/pattern system
- Cross-platform terminal compatibility
- UTF-8/Unicode support with modern color

## Phase 1: Core Animation Infrastructure (MVP)

### 1.1 Animation State Management
**Component**: `AnimationController`
- **WindowState**: Position, size, visibility, content type, properties
- **AnimationFrame**: Single state snapshot with timestamp
- **AnimationSequence**: Collection of frames with interpolation rules
- **StateManager**: Current system state tracking and diff generation

**Implementation**:
```cpp
class WindowState {
    TRect bounds;
    std::string title;
    WindowType type;
    bool visible;
    std::map<std::string, std::any> properties;
};

class AnimationController {
    std::vector<WindowState> currentState;
    std::queue<AnimationSequence> sequences;
    void applyFrame(const AnimationFrame& frame);
    void interpolateFrames(const AnimationFrame& from, const AnimationFrame& to, float progress);
};
```

### 1.2 Timeline System
**Component**: `TimelineManager`
- **Timeline**: Ordered sequence of keyframes with timing
- **Keyframe**: State definition at specific time point
- **Interpolation**: Smooth transitions between keyframes (linear, ease-in/out, custom)
- **Playback Control**: Play, pause, seek, speed control

**Key Features**:
- Frame-accurate playback at configurable FPS
- Multiple concurrent timelines
- Loop modes (once, repeat, ping-pong)
- Real-time timeline editing capabilities

### 1.3 External API Layer
**Component**: `APIServer`
- **JSON-RPC Interface**: Standard protocol for external control
- **WebSocket Support**: Real-time bidirectional communication
- **Command Queue**: Asynchronous command processing
- **State Broadcasting**: Live state updates to connected clients

**API Endpoints**:
```json
{
  "createWindow": {"type": "pattern|gradient", "bounds": [x,y,w,h], "title": "string"},
  "moveWindow": {"id": "string", "bounds": [x,y,w,h], "duration": 1000},
  "setContent": {"id": "string", "content": {...}},
  "cascade": {"duration": 500},
  "tile": {"duration": 500},
  "getState": {},
  "playSequence": {"timeline": {...}},
  "takeScreenshot": {"path": "string"}
}
```

### 1.4 Animation Primitives Library
**Component**: `AnimationPrimitives`

**Core Primitives**:
- **Move**: Window position changes with easing
- **Resize**: Smooth size transitions
- **Fade**: Visibility/opacity changes
- **Morph**: Content type transitions
- **Cascade/Tile**: Animated layout arrangements
- **Spawn**: Window creation with entrance effects
- **Destroy**: Window removal with exit effects

**Composite Animations**:
- **WindowDance**: Multiple windows moving in patterns
- **ColorWave**: Gradient color cycling across windows
- **PatternFlow**: Pattern mode transitions
- **ScreenWipe**: Coordinated window movements for transitions

## Phase 2: Advanced Animation Features

### 2.1 MCP Server Integration
**Component**: `MCPServer`
- **Claude Integration**: Direct AI agent control via MCP protocol
- **Tool Definitions**: Exposed animation functions as MCP tools
- **Context Awareness**: AI understanding of current window state
- **Natural Language Commands**: Text-to-animation conversion

**MCP Tools**:
```typescript
{
  "create_window_sequence": {
    "description": "Create animated sequence of window spawning",
    "parameters": {"count": "number", "pattern": "string", "timing": "object"}
  },
  "arrange_windows": {
    "description": "Arrange windows in specific layout with animation", 
    "parameters": {"layout": "grid|circle|spiral", "duration": "number"}
  },
  "create_story": {
    "description": "Generate narrative animation sequence",
    "parameters": {"theme": "string", "duration": "number", "complexity": "low|medium|high"}
  }
}
```

### 2.2 Advanced Animation Engine
**Component**: `AdvancedAnimator`

**Features**:
- **Physics Simulation**: Realistic motion with gravity, momentum, collisions
- **Particle Systems**: Window spawning/destruction effects
- **Path Following**: Windows following complex curves and shapes
- **Synchronized Choreography**: Multiple windows moving in coordination
- **Dynamic Content**: Pattern/gradient parameters changing over time
- **Sound Integration**: Audio-visual synchronization (ASCII waveforms)

### 2.3 Preset Animation Library
**Component**: `AnimationPresets`

**Preset Categories**:
- **Layouts**: Professional arrangements (dashboard, presentation, workspace)
- **Transitions**: Scene changes (fade, wipe, spiral, explosion)
- **Demonstrations**: Feature showcases (gradient showcase, pattern demo)
- **Stories**: Narrative sequences (window lifecycle, data visualization)
- **Interactive**: User-guided animation experiences

### 2.4 Animation Recording & Playback
**Component**: `RecordingSystem`
- **Live Recording**: Capture user interactions as animation sequences
- **Timeline Export**: Save animations in portable format (JSON/YAML)
- **Animation Sharing**: Import/export preset libraries
- **Performance Analysis**: Timing statistics and optimization suggestions

## Technical Implementation Details

### 2.5 Event System Extension
**Current**: Turbo Vision event loop with command handling
**Enhanced**: 
- Animation event injection into TV event stream
- Custom event types for animation state changes
- Event filtering and routing for API commands
- Non-blocking animation updates during user interaction

### 2.6 State Serialization
**Format**: JSON-based state representation
**Components**:
- Full system state snapshots
- Delta/diff compression for large timelines
- Binary format for high-performance scenarios
- Human-readable timeline definitions

### 2.7 Performance Optimization
**Challenges**: 
- Terminal refresh rate limitations
- Memory usage for complex sequences
- CPU usage during intensive animations

**Solutions**:
- Frame rate limiting with TVISION_MAX_FPS
- Lazy evaluation of off-screen windows
- Animation LOD (Level of Detail) system
- Background pre-computation of sequences

## API Design

### 2.8 REST API Structure
```
GET    /api/state                    - Current system state
POST   /api/windows                  - Create window
PUT    /api/windows/{id}             - Update window
DELETE /api/windows/{id}             - Remove window
POST   /api/animations/play          - Execute animation sequence
GET    /api/animations/presets       - List available presets
POST   /api/animations/record/start  - Begin recording
POST   /api/animations/record/stop   - End recording
POST   /api/screenshots              - Capture screenshot
```

### 2.9 WebSocket Events
```json
{
  "window_created": {"id": "string", "state": {...}},
  "window_moved": {"id": "string", "from": {...}, "to": {...}},
  "animation_started": {"sequence_id": "string", "duration": 1000},
  "animation_completed": {"sequence_id": "string"},
  "state_changed": {"diff": {...}}
}
```

## Success Criteria

### Phase 1 (MVP)
- [ ] External API can create, move, and destroy windows programmatically
- [ ] Timeline system can execute multi-step animation sequences
- [ ] Basic animation primitives work smoothly (move, resize, fade)
- [ ] JSON-RPC interface functional with client library
- [ ] Screenshot API captures animation frames
- [ ] Performance maintains >30 FPS for reasonable complexity

### Phase 2 (Advanced)
- [ ] MCP server integration allows AI agent control
- [ ] Physics-based animations look natural
- [ ] Preset library contains 20+ professional animations
- [ ] Recording system captures and replays user sessions
- [ ] Complex choreographed sequences (10+ windows) execute smoothly
- [ ] Natural language commands generate appropriate animations

## Architecture Diagram

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   External      │    │  WIBWOBWORLD     │    │  Turbo Vision   │
│   Clients       │◄──►│  Animation       │◄──►│  Framework      │
│                 │    │  Framework       │    │                 │
│ • Web UI        │    │                  │    │ • TApplication  │
│ • AI Agents     │    │ ┌──────────────┐ │    │ • TWindow       │
│ • CLI Tools     │    │ │ API Server   │ │    │ • TView         │
│ • MCP Clients   │    │ │ JSON-RPC     │ │    │ • Event System  │
└─────────────────┘    │ │ WebSocket    │ │    │ • Drawing       │
                       │ └──────────────┘ │    └─────────────────┘
                       │                  │              │
                       │ ┌──────────────┐ │              │
                       │ │ Animation    │ │              │
                       │ │ Controller   │ │              │
                       │ │              │ │              │
                       │ │ • Timeline   │ │              │
                       │ │ • State Mgmt │ │              │
                       │ │ • Primitives │ │              │
                       │ │ • Presets    │ │              │
                       │ └──────────────┘ │              │
                       │                  │              │
                       │ ┌──────────────┐ │              │
                       │ │ Window       │◄┼──────────────┘
                       │ │ Management   │ │
                       │ │              │ │
                       │ │ • Patterns   │ │
                       │ │ • Gradients  │ │
                       │ │ • Wallpaper  │ │
                       │ │ • Layout     │ │
                       │ └──────────────┘ │
                       └──────────────────┘
```

## Implementation Timeline

### Week 1-2: Foundation
- Extract current window management into `WindowManager` class
- Implement basic `AnimationController` with state tracking
- Create simple JSON-RPC server with window creation/manipulation
- Basic movement animations with linear interpolation

### Week 3-4: Timeline System
- Implement `TimelineManager` with keyframe support
- Add easing functions and interpolation methods
- Create animation primitive library (move, resize, fade)
- WebSocket integration for real-time control

### Week 5-6: API Completion
- Complete REST API implementation
- Add preset animation library (5-10 basic presets)
- Screenshot integration with animation frame capture
- Performance optimization and testing

### Week 7-8: MCP Integration (Phase 2 Start)
- MCP server implementation with tool definitions
- Natural language command parsing
- AI-friendly state representation and feedback
- Example AI agents and use cases

### Week 9-10: Advanced Features
- Physics-based animations
- Complex choreographed sequences
- Recording and playback system
- Advanced preset library expansion

## Testing Strategy

### Unit Tests
- Animation primitive accuracy
- State serialization/deserialization
- Timeline calculation correctness
- API endpoint functionality

### Integration Tests  
- End-to-end animation sequences
- API client workflows
- MCP server integration
- Performance benchmarking

### Manual Testing
- Animation smoothness and visual quality
- User experience with external control
- Complex scenario stress testing
- Cross-platform compatibility

## Future Considerations (Parking Lot)

### Advanced Features
- **3D Perspective Effects**: Pseudo-3D window arrangements using ASCII art techniques
- **Network Collaboration**: Multiple clients controlling shared animation space
- **Video Export**: Sequence rendering to video files (GIF/MP4)
- **Plugin Architecture**: Third-party animation extensions
- **VR/AR Integration**: Spatial window arrangements for mixed reality
- **Machine Learning**: AI-generated animation sequences based on user preferences

### Platform Extensions
- **Web Browser Port**: WASM compilation for browser-based animations
- **Mobile Support**: Terminal emulators on iOS/Android
- **Cloud Deployment**: Headless animation rendering service
- **IoT Integration**: Animation sequences triggered by sensors/devices

### Content Expansions
- **Data Visualization**: Window-based charts, graphs, dashboards
- **Gaming Elements**: Interactive window-based puzzles and games  
- **Art Generation**: Procedural ASCII art and pattern generation
- **Music Visualization**: Audio-reactive window choreography

## Dependencies & Requirements

### Current Dependencies
- Turbo Vision framework (modern C++ port)
- ncurses/ncursesw for terminal handling
- CMake build system
- C++14 standard library

### New Dependencies (Phase 1)
- **JSON Library**: nlohmann/json or similar for API serialization
- **HTTP Server**: cpp-httplib or microhttpd for REST API
- **WebSocket**: websocketpp for real-time communication
- **Threading**: std::thread for non-blocking animations

### New Dependencies (Phase 2)  
- **MCP Protocol**: Official MCP libraries when available
- **Physics**: Simple 2D physics library or custom implementation
- **Audio**: PortAudio or similar for sound integration (optional)

## Risk Assessment

### Technical Risks
- **Performance**: Complex animations may exceed terminal refresh capabilities
- **Compatibility**: Different terminal emulators have varying capabilities
- **Memory Usage**: Large animation sequences could exhaust memory
- **Timing Accuracy**: System scheduling may affect animation smoothness

### Mitigation Strategies
- Progressive enhancement based on terminal capabilities
- Animation complexity scaling and performance monitoring
- Memory-efficient state management and garbage collection
- Adaptive timing with frame dropping capabilities

### Integration Risks
- **MCP Protocol Changes**: Early-stage protocol may evolve
- **API Versioning**: External client compatibility during development
- **Turbo Vision Updates**: Framework changes affecting custom extensions

### Mitigation Strategies
- Abstraction layers for external protocols
- Semantic versioning and backward compatibility
- Regular framework updates and testing

This PRD provides a comprehensive roadmap for transforming WIBWOBWORLD from an interactive TUI application into a powerful, programmable animation framework suitable for external control, AI integration, and complex animation sequences while maintaining the unique aesthetic and functionality of the original system.