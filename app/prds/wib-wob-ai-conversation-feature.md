# Wib&Wob: AI Conversation Feature for Turbo Vision TUI

**tl;dr:** Integrate Claude Code headless mode into TVision test-tui as "wib&wob" - a conversational AI interface with 3-stage rollout (POC→v1→v2), leveraging existing MCP infrastructure for enhanced AI capabilities, session persistence, and seamless TUI window integration.

---

## Executive Summary

### Vision
Create "wib&wob" - an AI conversation partner integrated directly into the Turbo Vision TUI framework, providing users with an intelligent assistant accessible within the familiar TVision interface. The feature leverages Claude Code's headless mode and existing MCP tool access to deliver enhanced AI capabilities while maintaining TVision's performance characteristics.

### Strategic Goals
- **Differentiate** TVision as an AI-enhanced TUI framework
- **Leverage** existing MCP/API infrastructure investment
- **Demonstrate** advanced TUI capabilities beyond traditional applications
- **Enable** AI-assisted development workflows within terminal environments

### Success Metrics
- Successful multi-turn conversations with session persistence
- Zero memory leaks during extended conversation sessions
- Seamless integration with existing window management system

---

## Implementation Roadmap

### Stage 1: Proof of Concept (POC)
**Timeline:** 1-2 weeks  
**Scope:** Basic Claude Code integration with minimal UI

#### Features
- Basic chat window view (`TWibWobView`) integrated into test_pattern app
- Simple text input/output interface
- Single-turn conversations (no session persistence)
- Direct `claude -p --output-format json` subprocess execution
- Basic JSON response parsing and display
- Error handling for Claude Code execution failures

#### Technical Requirements
- New view class inheriting from `TView`
- Subprocess management using `popen()` or `system()`
- JSON parsing (lightweight library or manual parsing)
- Basic input handling via TVision event system
- Integration with existing test_pattern window creation

#### Success Criteria
- User can type query and receive Claude response
- Proper error messages for API failures
- No crashes during normal operation
- Basic text display formatting

### Stage 2: Production v1
**Timeline:** 2-3 weeks  
**Scope:** Full-featured chat interface with session management

#### Features
- **Advanced Chat UI**
  - Scrollable conversation history
  - Input field with line editing capabilities
  - Status indicators (thinking, error, ready)
  - Message timestamps and session info
  
- **Session Management**
  - Multi-turn conversations with `--continue` flag
  - Session persistence across app restarts
  - Session list/management interface
  - Configurable session timeout

- **Enhanced Integration**
  - MCP tool access configuration
  - Custom system prompts for "wib&wob" persona
  - Integration with existing window management
  - Keyboard shortcuts and navigation

- **Robust Error Handling**
  - Network connectivity issues
  - Claude Code binary not found
  - API quota/rate limiting
  - Graceful degradation modes

#### Technical Requirements
- Advanced text rendering with word wrapping
- Session state serialization/deserialization
- Asynchronous subprocess execution (non-blocking UI)
- Memory management for conversation history
- Configuration file support for prompts/settings

#### Success Criteria
- Multi-turn conversations work seamlessly
- Sessions persist across app restarts
- UI remains responsive during API calls
- Memory usage stable during extended use

### Stage 3: Production v2
**Timeline:** 2-3 weeks  
**Scope:** Advanced features and ecosystem integration

#### Features
- **Streaming Responses**
  - Real-time response display using `--output-format stream-json`
  - Progressive text rendering
  - Interrupt capability for long responses
  
- **Advanced AI Capabilities**
  - Full MCP tool access for enhanced responses
  - Context-aware system prompts
  - Conversation branching/forking
  - Export conversations to files

- **TUI Integration**
  - Window manipulation commands via AI
  - Screenshot analysis and discussion
  - Workspace automation through conversation
  - Integration with other test-tui applications

- **Developer Features**
  - Debug mode with raw JSON display
  - Performance metrics and timing
  - Conversation analytics
  - Plugin architecture for extensions

#### Technical Requirements
- Streaming JSON parser implementation
- Advanced IPC integration with existing API server
- Plugin/extension system design
- Performance monitoring and optimization
- Advanced memory management

---

## Technical Architecture

### Core Components

#### 1. View Layer (`TWibWobView`)
```cpp
class TWibWobView : public TView {
    // Conversation display area
    // Input field management  
    // Scrolling and navigation
    // Message rendering with formatting
};
```

#### 2. Conversation Engine (`WibWobEngine`)
```cpp
class WibWobEngine {
    // Claude Code subprocess management
    // Session state tracking
    // JSON response parsing
    // Error handling and retry logic
};
```

#### 3. Session Manager (`SessionManager`)
```cpp
class SessionManager {
    // Session persistence (file-based)
    // Multi-session support
    // Session metadata tracking
    // Cleanup and garbage collection
};
```

#### 4. Message Processing (`MessageProcessor`)
```cpp
class MessageProcessor {
    // Text formatting and rendering
    // Command parsing (for TUI integration)
    // Streaming response handling
    // Content filtering and validation
};
```

### Integration Points

#### With Existing TVision Framework
- **Window System**: Standard TWindow container for chat interface
- **Event System**: Keyboard/mouse input through standard TEvent handling
- **Drawing System**: TDrawBuffer for efficient text rendering
- **Color System**: Consistent theming with existing TVision applications

#### With MCP Infrastructure
- **Tool Access**: Leverage existing MCP server configuration
- **Enhanced Responses**: AI can manipulate TUI windows during conversation
- **State Synchronization**: Real-time integration with window state

#### With Test-TUI Ecosystem
- **Window Creation**: AI can create/manipulate test_pattern windows
- **Screenshot Integration**: Discuss and analyze TUI screenshots
- **Animation Control**: Interact with frame_file_player and animations

### Data Flow Architecture

```
User Input → TWibWobView → WibWobEngine → claude -p subprocess
                ↓                              ↓
         UI Rendering ←  MessageProcessor  ← JSON Response
                ↓                              ↓  
        Session Update ←  SessionManager  ← Session Data
```

### File Structure
```
test-tui/
├── src/
│   ├── wibwob/
│   │   ├── wibwob_view.cpp/h       # Main chat interface
│   │   ├── wibwob_engine.cpp/h     # Claude Code integration
│   │   ├── session_manager.cpp/h   # Session persistence
│   │   ├── message_processor.cpp/h # Text processing
│   │   └── wibwob_config.cpp/h     # Configuration management
│   └── test_pattern.cpp            # Integration point
├── sessions/                        # Session storage directory
├── config/
│   └── wibwob_config.json          # Default configuration
└── CMakeLists.txt                  # Build integration
```

---

## User Experience Design

### Chat Interface Layout
```
┌─ Wib&Wob Chat ──────────────────────────────────┐
│ [Session: conversation_123] [Status: Ready]      │
├─────────────────────────────────────────────────┤
│ User: How do I create a gradient window?         │
│ Wib: To create a gradient window, you can use... │
│                                                 │
│ User: Can you create one for me?               │  
│ Wib: *Creating radial gradient window*          │
│      [Window w5 created at position 30,10]     │
│                                                 │
│ ▼ [Conversation continues...]                   │
├─────────────────────────────────────────────────┤
│ > _                                            │
└─────────────────────────────────────────────────┘
```

### Key User Interactions
- **F1**: Open/focus wib&wob chat window
- **Ctrl+N**: New conversation session
- **Ctrl+S**: Save current conversation
- **Ctrl+L**: Load previous session
- **ESC**: Close/minimize chat window
- **Up/Down**: Navigate conversation history
- **Ctrl+C**: Interrupt long responses (v2)

### Persona Definition: "wib&wob"
- **Tone**: Knowledgeable but approachable TUI expert
- **Specialties**: TVision framework, terminal applications, C++ development
- **Capabilities**: Window manipulation, code generation, debugging assistance
- **Personality**: Helpful assistant with subtle British wit (matching Claude persona)

---

## Risk Analysis & Mitigation

### High Priority Risks

#### 1. Subprocess Management Complexity
**Risk**: Deadlocks, zombie processes, or UI blocking during Claude Code execution  
**Mitigation**: 
- Implement timeout mechanisms
- Use non-blocking subprocess execution
- Proper cleanup on application exit
- Fallback to synchronous mode if async fails

#### 2. Memory Management in Long Sessions
**Risk**: Memory leaks from accumulated conversation history  
**Mitigation**:
- Implement conversation history limits
- Periodic cleanup of old sessions
- Memory monitoring and alerting
- Smart garbage collection for inactive sessions

#### 3. Claude Code Binary Dependency
**Risk**: Application fails if Claude Code not installed or not in PATH  
**Mitigation**:
- Graceful degradation with clear error messages
- Configuration option for Claude Code binary path
- Optional integration (feature flag)
- Installation validation on startup

### Medium Priority Risks

#### 4. JSON Parsing Robustness
**Risk**: Malformed JSON responses breaking the interface  
**Mitigation**:
- Robust JSON parsing with error recovery
- Fallback to plain text display
- Logging of parse failures for debugging

#### 5. Network Connectivity Issues  
**Risk**: API failures disrupting user experience  
**Mitigation**:
- Retry mechanisms with exponential backoff
- Offline mode with cached responses
- Clear status indicators for connectivity

### Low Priority Risks

#### 6. Performance Impact on TUI
**Risk**: Chat feature slowing down overall TUI performance  
**Mitigation**:
- Lazy loading of chat components
- Optional feature that can be disabled
- Performance monitoring and optimization

---

## Implementation Considerations

### Build System Integration
- Add `TV_BUILD_WIBWOB` CMake option (default ON)
- Conditional compilation for systems without required dependencies
- Separate library target for potential reuse

### Configuration Management
- JSON-based configuration file
- Environment variable overrides
- Runtime configuration through UI settings

### Testing Strategy
- **Unit Tests**: Core engine and message processing logic
- **Integration Tests**: Subprocess execution and JSON parsing
- **UI Tests**: Manual verification of chat interface
- **Performance Tests**: Memory usage and response latency

### Documentation Requirements
- User guide for chat interface
- Developer guide for extending functionality
- Configuration reference
- Troubleshooting guide

### Platform Considerations
- **Primary Target**: Unix/Linux (existing IPC infrastructure)
- **Future Extension**: Windows support via similar subprocess model
- **macOS**: Full compatibility expected

---

## Success Criteria & Acceptance Testing

### POC Success Criteria
- [ ] User can ask question and receive Claude response
- [ ] Basic error handling prevents crashes
- [ ] Integration with test_pattern app works
- [ ] JSON response parsing functional

### v1 Success Criteria  
- [ ] Multi-turn conversations work seamlessly
- [ ] Sessions persist across app restarts
- [ ] UI remains responsive during API calls
- [ ] Memory usage stable during 1-hour session
- [ ] All keyboard shortcuts functional

### v2 Success Criteria
- [ ] Streaming responses display progressively
- [ ] AI can create/manipulate TUI windows
- [ ] Conversation export/import works
- [ ] Performance metrics under acceptable thresholds
- [ ] Plugin architecture allows extensions

---

## Appendix: Technical References

### Claude Code Integration Commands
```bash
# Basic query with JSON output
claude -p --output-format json "Your question here"

# Continue existing session
claude -p --continue --output-format json "Follow-up question"

# Stream JSON responses
claude -p --output-format stream-json "Your question"

# Resume specific session
claude -p --resume session_123 "Your question"
```

### Example JSON Response Structure
```json
{
  "result": "Response content here...",
  "session_id": "session_abc123", 
  "cost": 0.025,
  "duration": 1.245,
  "is_error": false
}
```

### MCP Tool Integration Example
```json
{
  "mcpServers": {
    "tui-control": {
      "type": "http",
      "url": "http://127.0.0.1:8089/mcp",
      "description": "TUI application control"
    }
  }
}
```

This PRD provides the comprehensive foundation for implementing the wib&wob AI conversation feature while maintaining TVision's architectural principles and leveraging existing infrastructure investments.