# JavaScript Animation Porting Framework PRD

**tl;dr:** Create modular TAnimatedView system to port JavaScript animations (starting with donut.js 3D ASCII) into TUI windows, establishing foundation for comprehensive animation framework with external control APIs and timeline-based orchestration.

## Context

WIBWOBWORLD currently has a solid TUI foundation with unlimited resizable windows, multiple content types (patterns/gradients), and window management. The existing [wibwobworld-animation-framework.md](./wibwobworld-animation-framework.md) outlines a comprehensive animation framework for external control and AI integration. The immediate goal is to port JavaScript animations into TUI windows while laying groundwork for the larger framework.

## Objective

Create a JavaScript-to-TUI animation porting system that:
- Runs JS-style animations (donut.js) natively in C++ TUI windows
- Establishes modular animation architecture compatible with existing PRD
- Provides foundation for timeline-based control and external APIs
- Maintains performance suitable for terminal rendering
- Demonstrates extensibility for additional JS animation ports

## Current Architecture Analysis

### Existing TUI Components
- **TTestPatternApp**: Main application with event handling, window management
- **TTestPatternWindow/TGradientWindow**: Resizable windows with custom content types
- **TView System**: Draw-based rendering with frame buffers and color attributes
- **Menu Integration**: TCustomMenuBar with kaomoji and extensible commands
- **Window Management**: Cascade, tile, close functionality

### JavaScript Animation Pattern (donut.js)
- **Frame Buffer**: Character array with z-buffer depth testing
- **Time-Based Updates**: Continuous animation using context.time
- **3D Mathematics**: Torus rendering with rotation (sin/cos calculations)
- **Character Mapping**: Brightness-based ASCII palette (`.,-~:;=!*#$@`)
- **Performance**: Direct buffer manipulation for real-time rendering

## Phase 1: Core Animation Infrastructure

### 1.1 TAnimatedView Base Class
**Component**: `TAnimatedView` (test-tui/animated_view.h/cpp)
- **Frame Buffer System**: 2D character/color buffer matching JS animation expectations
- **Time-Based Updates**: Continuous animation loop with configurable FPS
- **Virtual Interface**: Pure virtual methods for animation logic
- **Turbo Vision Integration**: Seamless integration with existing TView system

**Implementation Architecture**:
```cpp
class TAnimatedView : public TView {
public:
    struct AnimationContext {
        double time;           // Elapsed time since start
        int cols, rows;        // Buffer dimensions
        double deltaTime;      // Time since last frame
    };
    
    struct FrameBuffer {
        std::vector<char> chars;
        std::vector<TColorAttr> colors;
        std::vector<double> zBuffer;  // Depth testing
    };

protected:
    virtual void updateAnimation(const AnimationContext& ctx) = 0;
    virtual void initAnimation() {}
    virtual void cleanupAnimation() {}
    
private:
    FrameBuffer frameBuffer;
    std::chrono::high_resolution_clock::time_point startTime;
    int targetFPS = 30;
    bool animationActive = false;
};
```

### 1.2 Animation Mathematics Utilities
**Component**: `AnimationUtils` (test-tui/animation_utils.h/cpp)
- **3D Transformations**: Matrix operations, rotation calculations
- **Interpolation**: Linear, smoothstep, easing functions
- **Performance Optimization**: Lookup tables for trigonometric functions
- **Character Mapping**: Brightness-to-ASCII conversion utilities

**Key Functions**:
```cpp
namespace AnimationUtils {
    // 3D mathematics
    struct Vec3 { double x, y, z; };
    struct Matrix3x3 { double m[3][3]; };
    
    Vec3 rotateX(const Vec3& v, double angle);
    Vec3 rotateY(const Vec3& v, double angle);
    double dot(const Vec3& a, const Vec3& b);
    
    // Character mapping
    char brightnessToChar(double brightness);
    TColorAttr calculateShading(double normal, TColorAttr baseColor);
    
    // Performance utilities
    void initTrigLookup();
    double fastSin(double angle);
    double fastCos(double angle);
}
```

### 1.3 TDonutView Implementation
**Component**: `TDonutView` (test-tui/donut_view.h/cpp)
- **3D Torus Rendering**: Direct port of donut.js mathematics
- **Lighting System**: Surface normal calculations for character brightness
- **Z-Buffer**: Proper depth testing for 3D rendering
- **Parameter Control**: Configurable torus radii, rotation speed, scale

**Technical Mapping**:
| JavaScript | C++ Equivalent | Purpose |
|------------|----------------|---------|
| `context.time * 0.0015` | `ctx.time * 0.0015` | Rotation angle A |
| `buffer[k].char = 'x'` | `frameBuffer.chars[k] = 'x'` | Character assignment |
| `Math.cos(A)` | `AnimationUtils::fastCos(A)` | Optimized trigonometry |
| `z[o] = D` | `frameBuffer.zBuffer[o] = D` | Depth buffer |

**Core Algorithm**:
```cpp
void TDonutView::updateAnimation(const AnimationContext& ctx) {
    const double A = ctx.time * 0.0015;  // X rotation
    const double B = ctx.time * 0.0017;  // Z rotation
    
    // Pre-calculate rotation matrices
    const double cA = cos(A), sA = sin(A);
    const double cB = cos(B), sB = sin(B);
    
    // Clear buffers
    clearBuffers();
    
    // Render torus with dual nested loops
    for (double theta = 0; theta < TAU; theta += 0.05) {
        for (double phi = 0; phi < TAU; phi += 0.01) {
            // Calculate 3D point on torus surface
            Vec3 point = calculateTorusPoint(theta, phi, A, B);
            
            // Project to screen coordinates
            int x, y;
            double z;
            if (projectToScreen(point, x, y, z)) {
                // Calculate lighting and character
                double normal = calculateNormal(theta, phi, A, B);
                char ch = brightnessToChar(normal);
                
                // Depth test and render
                int index = y * ctx.cols + x;
                if (z > frameBuffer.zBuffer[index]) {
                    frameBuffer.chars[index] = ch;
                    frameBuffer.zBuffer[index] = z;
                }
            }
        }
    }
}
```

## Phase 2: Window Integration & Management

### 2.1 TAnimationWindow Wrapper
**Component**: `TAnimationWindow` (test-tui/animation_window.h/cpp)
- **Standard TWindow**: Maintains all existing window functionality
- **Animation Lifecycle**: Start/stop/pause control with proper cleanup
- **Performance Monitoring**: FPS display, CPU usage tracking
- **Context Menu**: Animation-specific controls (pause, speed, parameters)

**Features**:
```cpp
class TAnimationWindow : public TWindow {
public:
    enum AnimationType {
        DONUT_3D,
        // Future: MATRIX_RAIN, STARFIELD, etc.
    };
    
    TAnimationWindow(const TRect& bounds, const char* aTitle, AnimationType type);
    
    // Animation control
    void startAnimation();
    void stopAnimation();
    void pauseAnimation();
    void setFPS(int fps);
    
    // Performance monitoring
    double getCurrentFPS() const;
    double getCPUUsage() const;
    
protected:
    void handleEvent(TEvent& event) override;
    void close() override;  // Proper animation cleanup
    
private:
    std::unique_ptr<TAnimatedView> animationView;
    AnimationType type;
    bool isPaused = false;
};
```

### 2.2 Application Integration
**Component**: Updates to `TTestPatternApp` (test-tui/test_pattern_app.cpp)
- **Menu Extensions**: Add "Animation" submenu with available types
- **Command Handling**: New command constants for animation operations
- **Window Factory**: Centralized animation window creation
- **Resource Management**: Track active animations for cleanup

**Menu Structure**:
```
File
├── New Pattern Window
├── New Gradient >
│   ├── Horizontal
│   ├── Vertical  
│   ├── Radial
│   └── Diagonal
├── New Animation >        ← NEW
│   ├── 3D Donut          ← NEW
│   ├── [Future animations]
│   └── Animation Settings ← NEW
├── Screenshot
└── Exit
```

**Command Constants**:
```cpp
const ushort cmNewAnimationDonut = 110;
const ushort cmAnimationPause = 111;
const ushort cmAnimationResume = 112;
const ushort cmAnimationSettings = 113;
```

## Phase 3: Performance & Optimization

### 3.1 FPS Control & Timing
**Component**: Integrated timing system
- **Target FPS**: Configurable 15-60 FPS with automatic adjustment
- **Frame Limiting**: Sleep/yield to maintain consistent timing
- **Performance Scaling**: Automatic quality reduction under load
- **Multiple Animations**: Resource sharing and priority management

**Implementation Strategy**:
```cpp
class AnimationTimer {
    std::chrono::high_resolution_clock::time_point lastFrame;
    double targetFrameTime;
    int currentFPS = 30;
    
public:
    bool shouldUpdate() {
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = now - lastFrame;
        return elapsed >= std::chrono::duration<double>(targetFrameTime);
    }
    
    void frameComplete() {
        // Update timing statistics
        // Implement frame rate limiting
    }
};
```

### 3.2 Memory Management
**Component**: Efficient buffer allocation
- **Buffer Pooling**: Reuse frame buffers to avoid allocation overhead
- **Memory Monitoring**: Track usage for performance tuning
- **Cleanup Guarantees**: RAII patterns for animation resources
- **Size Optimization**: Minimize memory footprint per animation

### 3.3 Terminal Compatibility
**Component**: Cross-platform optimization
- **Refresh Rate Detection**: Adapt to terminal capabilities
- **Character Set Support**: Fallback for limited character sets
- **Color Depth**: Graceful degradation for monochrome terminals
- **Performance Profiles**: Platform-specific optimizations

## Integration with Existing Animation Framework PRD

### 4.1 Timeline System Preparation
**Foundation**: Design animation state interface for future timeline control
```cpp
class AnimationState {
public:
    virtual nlohmann::json serialize() const = 0;
    virtual void deserialize(const nlohmann::json& state) = 0;
    virtual void interpolate(const AnimationState& from, 
                           const AnimationState& to, 
                           double t) = 0;
};

class TDonutView : public TAnimatedView, public AnimationState {
    // Enable timeline control of rotation speed, colors, etc.
    double rotationSpeedA = 0.0015;
    double rotationSpeedB = 0.0017;
    TColorAttr donutColor = TColorAttr::White;
};
```

### 4.2 External API Hooks
**Foundation**: Prepare for JSON-RPC control interface
```cpp
// Future API endpoints
{
    "createAnimationWindow": {
        "type": "donut|matrix|starfield",
        "bounds": [x, y, w, h],
        "parameters": {"speed": 1.0, "color": "white"}
    },
    "controlAnimation": {
        "id": "animation_window_id",
        "action": "pause|resume|stop",
        "parameters": {...}
    },
    "getAnimationState": {
        "id": "animation_window_id"
    }
}
```

### 4.3 MCP Server Integration Points
**Foundation**: Design for AI agent control
```cpp
// MCP tools for animation control
{
    "create_animation_sequence": {
        "description": "Create animated sequence with JS-ported animations",
        "parameters": {
            "type": "donut|matrix", 
            "count": "number",
            "arrangement": "grid|spiral|random"
        }
    },
    "animate_donut_parameters": {
        "description": "Control donut animation properties",
        "parameters": {
            "speed": "number",
            "color": "string", 
            "size": "number"
        }
    }
}
```

## Technical Implementation Details

### 5.1 JavaScript-to-C++ Porting Pattern
**Process**: Systematic conversion approach for future animations

1. **Context Mapping**:
   - `context.time` → `AnimationContext::time`
   - `context.cols/rows` → `AnimationContext::cols/rows`
   - `buffer[i].char` → `frameBuffer.chars[i]`

2. **Mathematics Conversion**:
   - JavaScript Math functions → C++ `<cmath>` equivalents
   - Array operations → `std::vector` operations
   - Performance-critical loops → optimized C++ implementations

3. **Rendering Pipeline**:
   - Frame buffer clearing → `clearBuffers()`
   - Character assignment → Direct buffer manipulation
   - Color/attribute handling → TColorAttr integration

### 5.2 Build System Integration
**Component**: CMakeLists.txt updates
```cmake
# Add animation source files
set(ANIMATION_SOURCES
    animated_view.cpp
    animation_utils.cpp
    donut_view.cpp
    animation_window.cpp
)

# Link threading libraries for animation timing
find_package(Threads REQUIRED)
target_link_libraries(test_tui Threads::Threads)

# Enable optimizations for mathematics
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(test_tui PRIVATE -O3 -ffast-math)
endif()
```

## Success Criteria & Testing

### Phase 1 Completion (Week 1)
- [ ] TDonutView renders smooth 3D donut animation in TUI window
- [ ] Animation runs at stable 30+ FPS without terminal flickering
- [ ] Multiple donut windows can run simultaneously without performance degradation
- [ ] Standard window operations work correctly (move, resize, close, cascade, tile)
- [ ] Menu integration allows easy animation window creation
- [ ] Memory usage remains stable during extended animation periods

### Framework Integration (Week 2)
- [ ] Animation state can be serialized/deserialized for timeline compatibility
- [ ] Performance monitoring provides accurate FPS and resource metrics
- [ ] API hooks prepared for external control integration
- [ ] Animation system supports multiple animation types through factory pattern
- [ ] Documentation covers complete JavaScript-to-C++ porting process

### Quality Standards
- [ ] No memory leaks during animation lifecycle (verified with valgrind/sanitizers)
- [ ] Smooth performance across different terminal emulators (tested: iTerm2, Terminal.app, xterm)
- [ ] Proper cleanup when windows closed during active animation
- [ ] Code maintains existing TUI architecture patterns and conventions
- [ ] Mathematical accuracy produces visually correct 3D rendering results

### Performance Benchmarks
- **Single Animation**: 30+ FPS at 80x24 terminal size
- **Multiple Animations**: 4+ concurrent windows at 20+ FPS each
- **Memory Usage**: <10MB per animation window
- **CPU Usage**: <25% single core for 4 concurrent animations
- **Startup Time**: <100ms for animation window creation

## Future Animation Extensions

### Immediate Candidates for Porting
1. **Matrix Rain**: Character-falling effect from JS demos
2. **Starfield**: 3D starfield with depth and motion
3. **Plasma**: 2D plasma effects with color cycling
4. **Fire**: Particle-based fire simulation
5. **Wave**: Sine wave interference patterns

### Porting Complexity Assessment
| Animation | Complexity | Implementation Time | Key Challenges |
|-----------|------------|-------------------|----------------|
| Donut | Medium | 2-3 days | 3D math, z-buffer |
| Matrix Rain | Low | 1-2 days | Simple particle system |
| Starfield | Low-Medium | 1-2 days | 3D projection |
| Plasma | Medium | 2-3 days | Color interpolation |
| Fire | High | 4-5 days | Particle physics |

### Architecture Extensibility
```cpp
// Animation factory for easy extension
class AnimationFactory {
public:
    static std::unique_ptr<TAnimatedView> create(AnimationType type) {
        switch(type) {
            case DONUT_3D: return std::make_unique<TDonutView>();
            case MATRIX_RAIN: return std::make_unique<TMatrixView>();
            case STARFIELD: return std::make_unique<TStarfieldView>();
            // Easy to extend...
        }
    }
    
    static std::vector<AnimationType> getAvailableTypes();
    static const char* getTypeName(AnimationType type);
};
```

## Risk Assessment & Mitigation

### Performance Risks
**Risk**: Complex 3D mathematics may exceed terminal refresh capabilities
**Mitigation**: 
- Progressive FPS reduction under load
- Animation complexity scaling based on window size
- Optional quality settings (high/medium/low detail)

**Risk**: Multiple concurrent animations overwhelming system resources
**Mitigation**:
- Animation priority system with background throttling
- Automatic pause when windows not visible
- Resource pooling for frame buffers

### Compatibility Risks
**Risk**: Different terminal emulators have varying Unicode/color support
**Mitigation**:
- Character set detection and fallback ASCII mode
- Color depth detection with graceful degradation
- Comprehensive testing across popular terminal emulators

**Risk**: Platform-specific timing issues affecting animation smoothness
**Mitigation**:
- Adaptive timing system with platform detection
- Multiple timing backends (high_resolution_clock, steady_clock)
- Performance profiling on target platforms

### Integration Risks
**Risk**: Animation system conflicts with existing Turbo Vision event handling
**Mitigation**:
- Non-blocking animation updates using separate timing thread
- Proper event prioritization and filtering
- Comprehensive integration testing with existing features

**Risk**: Memory leaks or resource issues during animation lifecycle
**Mitigation**:
- RAII patterns for all animation resources
- Automated testing with memory sanitizers
- Proper exception handling and cleanup guarantees

## Implementation Timeline

### Week 1: Core Foundation
**Days 1-2**: TAnimatedView base class, timing system, basic frame buffer
**Days 3-4**: Mathematics utilities, TDonutView implementation, 3D rendering
**Days 5-7**: TAnimationWindow wrapper, menu integration, multi-window support

### Week 2: Polish & Integration
**Days 8-10**: Performance optimization, FPS control, memory management
**Days 11-12**: Timeline foundation, API hooks, state serialization
**Days 13-14**: Testing, documentation, integration with existing PRD framework

### Future Phases (Post-Week 2)
**Phase 2**: Additional JS animation ports (Matrix, Starfield, Plasma)
**Phase 3**: Timeline system integration with keyframe animation
**Phase 4**: External API development (JSON-RPC, WebSocket)
**Phase 5**: MCP server integration for AI control

This PRD establishes a solid foundation for JavaScript animation porting while maintaining compatibility with the comprehensive animation framework outlined in the existing wibwobworld-animation-framework.md. The modular design ensures easy extension to additional animations while preparing for external control systems and AI integration.