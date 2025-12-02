# Wibwob Chat System: Claude Code SDK Modernisation & Streaming Integration

**tl;dr:** Modernise wibwob chat system from basic CLI subprocess calls to world-class Claude Code SDK integration with streaming mode, customSystemPrompt support, and real-time response display for proper conversational AI experience in TUI.

---

## Current State Analysis

### Existing Architecture Limitations

**Current Implementation:**
- **Subprocess Execution**: Uses `popen()` with `claude -p` CLI commands
- **File-based Prompts**: Loads system prompt from `wibandwob.prompt.md` file
- **Single-shot Queries**: No persistent session context or streaming
- **Polling-based Async**: Spinner animation with manual polling, not real-time updates
- **Basic Error Handling**: Limited Claude Code CLI error parsing

**Performance Issues:**
- Process startup overhead for each query (50-200ms overhead)
- No session context preservation between queries
- Blocking UI during response generation
- No real-time response streaming capability

**SDK Non-compliance:**
- **Missing `customSystemPrompt`**: Uses file loading vs SDK parameter
- **No Streaming Input Mode**: Uses single message mode instead of recommended streaming
- **Limited Tool Integration**: Basic provider abstraction vs proper SDK patterns
- **Manual JSON Parsing**: Custom parsing vs SDK response handling

### Technical Debt

```cpp
// Current: Subprocess with manual JSON parsing
LLMResponse executeClaudeCommand(const LLMRequest& request);
std::string buildClaudeCommand(const LLMRequest& request) const;
LLMResponse parseClaudeResponse(const std::string& json) const;

// Current: File-based system prompt loading
std::ifstream promptFile("wibandwob.prompt.md");
engine->setSystemPrompt(customPrompt);
```

---

## Technical Architecture

### Proposed SDK Integration

**Replace subprocess execution with proper SDK usage:**

```typescript
// New: Streaming Input Mode with customSystemPrompt
async function* generateWibwobMessages(userQuery: string, systemPrompt: string) {
    yield {
        type: "user",
        message: {
            role: "user", 
            content: userQuery
        }
    };
}

for await (const message of query({
    prompt: generateWibwobMessages(userInput, customSystemPrompt),
    options: {
        customSystemPrompt: systemPrompt,  // Direct SDK parameter usage
        maxTurns: 50,                      // Multi-turn conversation support
        allowedTools: ["Read", "Grep", "Write", "Bash"],
        streamResponse: true               // Real-time response display
    }
})) {
    // Real-time UI updates
    updateChatDisplay(message);
}
```

### Component Redesign

#### 1. **WibWobEngine Modernisation**

**Current Interface:**
```cpp
class WibWobEngine {
    bool sendQuery(const std::string& query, ResponseCallback callback);
    void setSystemPrompt(const std::string& prompt);  // File-based
};
```

**New SDK-based Interface:**
```cpp
class WibWobEngine {
    // Streaming session management
    bool startStreamingSession(const std::string& customSystemPrompt);
    bool sendStreamingQuery(const std::string& query, StreamingCallback callback);
    void pauseStreaming();
    void resumeStreaming();
    void endSession();
    
    // Real-time prompt modification
    bool updateSystemPrompt(const std::string& customSystemPrompt);
    
    // Session state
    std::string getSessionId() const;
    bool hasActiveSession() const;
    SessionState getSessionState() const;
};
```

#### 2. **TWibWobView Real-time Updates**

**Current Pattern:**
```cpp
// Polling-based with spinner
void TWibWobView::updateSpinner() {
    spinnerFrame++;
    drawView(); // Full redraw
}
```

**New Streaming Pattern:**
```cpp
// Real-time streaming updates
void TWibWobView::onStreamingResponse(const StreamChunk& chunk) {
    if (chunk.isPartial) {
        appendToCurrentMessage(chunk.content);
        drawPartialUpdate(); // Incremental update
    } else {
        finalizeCurrentMessage();
        drawView();
    }
}
```

#### 3. **System Prompt Management**

**Current: File-based Loading**
```cpp
std::ifstream promptFile("wibandwob.prompt.md");
engine->setSystemPrompt(customPrompt);
```

**New: Runtime Configuration**
```cpp
// UI controls for prompt modification
class WibWobPromptManager {
    bool loadPromptFromFile(const std::string& path);
    bool setCustomPrompt(const std::string& prompt);
    std::string getCurrentPrompt() const;
    std::vector<std::string> getPromptPresets() const;
    
    // Real-time prompt switching
    bool switchPromptPreset(const std::string& presetName);
    bool updatePromptParameter(const std::string& key, const std::string& value);
};
```

---

## Implementation Plan

### Phase 1: SDK Foundation (Week 1)

#### Tasks
1. **Replace ClaudeCodeProvider with SDK Integration**
   - [ ] Remove subprocess-based `claude -p` execution
   - [ ] Implement direct Claude Code SDK TypeScript/Node.js bridge
   - [ ] Add streaming input mode support
   - [ ] Implement `customSystemPrompt` parameter usage

2. **Streaming Architecture Setup**
   - [ ] Design streaming response callback interface
   - [ ] Implement incremental UI update system
   - [ ] Add session state management
   - [ ] Create real-time response buffering

3. **Configuration Migration**
   - [ ] Migrate from file-based prompts to SDK parameters
   - [ ] Add runtime prompt modification capability
   - [ ] Implement prompt preset system
   - [ ] Create configuration UI controls

#### Acceptance Criteria
- [ ] Single streaming conversation works end-to-end
- [ ] `customSystemPrompt` parameter functional
- [ ] Real-time response display operational
- [ ] No subprocess execution for chat queries

### Phase 2: Advanced Features (Week 2)

#### Tasks
4. **Multi-Session Management**
   - [ ] Implement persistent session storage
   - [ ] Add session switching UI
   - [ ] Create session history navigation
   - [ ] Implement session export/import

5. **Real-time UI Enhancements**
   - [ ] Streaming response type indicators (thinking, typing, complete)
   - [ ] Partial message cancellation support
   - [ ] Response speed metrics display
   - [ ] Advanced scroll management for streaming

6. **Tool Integration**
   - [ ] Configure allowed tools via SDK parameters
   - [ ] Add tool usage display in chat
   - [ ] Implement tool permission management
   - [ ] Create tool usage analytics

#### Acceptance Criteria
- [ ] Multiple concurrent sessions supported
- [ ] Tool integration working properly
- [ ] Advanced UI features operational
- [ ] Session persistence reliable

### Phase 3: Polish & Optimisation (Week 3)

#### Tasks
7. **Performance Optimisation**
   - [ ] Optimise incremental rendering for large conversations
   - [ ] Implement conversation chunking for memory management
   - [ ] Add response caching for repeated queries
   - [ ] Optimise SDK connection pooling

8. **User Experience Enhancements**
   - [ ] Add conversation search functionality
   - [ ] Implement conversation templates
   - [ ] Create quick prompt switching hotkeys
   - [ ] Add conversation export formats

9. **Testing & Documentation**
   - [ ] Comprehensive SDK integration testing
   - [ ] Performance benchmarks vs old implementation
   - [ ] User documentation for new features
   - [ ] Developer documentation for SDK patterns

#### Acceptance Criteria
- [ ] Performance meets or exceeds current implementation
- [ ] All user-facing features documented
- [ ] Comprehensive test coverage
- [ ] Production-ready stability

---

## Technical Specifications

### Streaming vs Single Mode Decision

**Recommendation: Streaming Input Mode**

**Rationale:**
- **Real-time responsiveness**: Essential for conversational AI UX
- **Session persistence**: Required for multi-turn conversations  
- **Tool integration**: Needed for advanced wibwob capabilities
- **Context preservation**: Critical for coherent conversations
- **Performance**: Eliminates process startup overhead

**Implementation Approach:**
```typescript
// Streaming session with real-time updates
const streamingSession = await createStreamingSession({
    customSystemPrompt: wibwobPrompt,
    maxTurns: 100,
    allowedTools: ["Read", "Write", "Grep", "Bash"],
    sessionTimeout: 3600000  // 1 hour
});

// Real-time response handling
for await (const chunk of streamingSession.sendQuery(userInput)) {
    if (chunk.type === "content_delta") {
        appendToResponse(chunk.content);
        updateDisplay();
    } else if (chunk.type === "message_complete") {
        finaliseMessage();
    }
}
```

### CustomSystemPrompt Integration

**Current Limitation:**
```cpp
// File-based static loading
std::ifstream promptFile("wibandwob.prompt.md");
engine->setSystemPrompt(customPrompt);
```

**New SDK Approach:**
```typescript
// Dynamic runtime configuration
const dynamicPrompt = buildWibwobPrompt({
    personality: "dual_artistic_scientific",
    context: "tvision_tui_framework", 
    tools: ["filesystem", "development", "creative"],
    style: "british_glaswegian",
    expertise: ["cpp", "tui", "ascii_art"]
});

const options = {
    customSystemPrompt: dynamicPrompt,  // Direct SDK parameter
    // Completely replaces default system prompt
    // Manual safety/environment context inclusion as needed
};
```

### Error Handling & Resilience

**Connection Management:**
```cpp
class StreamingConnectionManager {
    bool reconnectOnFailure = true;
    int maxRetries = 3;
    std::chrono::seconds retryDelay{5};
    
    void handleConnectionLoss();
    void attemptReconnection();
    void fallbackToSingleMode();
};
```

**Graceful Degradation:**
- Stream interruption → fallback to polling
- SDK unavailable → fallback to CLI subprocess
- Network issues → queue messages for retry
- Session corruption → create new session

---

## Testing Strategy

### SDK Integration Testing
```bash
# End-to-end streaming conversation
curl -X POST /test/streaming-conversation \
  -d '{"prompt": "test customSystemPrompt", "streaming": true}'

# System prompt modification
curl -X PUT /test/system-prompt \
  -d '{"customSystemPrompt": "new prompt"}'

# Multi-session management  
curl -X GET /test/sessions
curl -X POST /test/sessions/switch -d '{"sessionId": "abc123"}'
```

### Performance Benchmarks
- **Response latency**: <100ms first chunk, <50ms subsequent chunks
- **Memory usage**: <50MB for 1000-message conversation
- **Session startup**: <200ms vs current 500ms+ subprocess overhead
- **UI responsiveness**: 60fps maintained during streaming

### UX Validation
- [ ] Real-time typing indicators work smoothly
- [ ] Conversation context preserved across sessions
- [ ] Tool usage clearly displayed and functional
- [ ] Error states handled gracefully with clear messaging

---

## Migration Strategy

### Backward Compatibility
- Maintain existing `WibWobEngine` interface during transition
- Support both file-based and SDK-based prompt loading
- Gradual rollout with feature flags
- Preserve existing conversation logs and session data

### Rollout Plan
1. **Development Phase**: Feature-flagged implementation alongside existing system
2. **Beta Testing**: Internal testing with both systems available
3. **Gradual Migration**: Default to new system with fallback option
4. **Full Deployment**: Remove legacy subprocess-based implementation

---

## Success Metrics

### Technical KPIs
- **Streaming latency**: <100ms first response chunk
- **Session reliability**: >99.5% session persistence success rate
- **Memory efficiency**: <50MB per active session
- **Tool integration**: 100% tool compatibility with SDK

### User Experience KPIs  
- **Conversation fluidity**: No perceptible response delays
- **Feature adoption**: >80% usage of streaming vs legacy mode
- **Error recovery**: <5% user-initiated session restarts
- **Performance satisfaction**: Measurably faster than current implementation

---

## Risk Mitigation

### Technical Risks
- **SDK dependency issues**: Maintain CLI fallback until SDK proven stable
- **Streaming performance**: Implement chunking and throttling for large responses
- **Session corruption**: Regular session state validation and recovery
- **Memory leaks**: Strict conversation history limits and cleanup

### UX Risks
- **Learning curve**: Preserve familiar interface during transition
- **Feature regression**: Comprehensive testing of all existing functionality
- **Performance degradation**: Continuous monitoring with automatic fallback

---

## Conclusion

This modernisation transforms wibwob from a basic CLI wrapper into a world-class conversational AI interface leveraging the full power of Claude Code SDK. The streaming architecture, `customSystemPrompt` support, and real-time updates will deliver a significantly enhanced user experience while establishing a robust foundation for future AI-enhanced TUI development.

**Key Benefits:**
- **Real-time streaming responses** vs static request/response
- **Proper session management** vs stateless queries  
- **SDK-native integration** vs subprocess overhead
- **Runtime prompt configuration** vs file-based static loading
- **Professional UX patterns** vs basic CLI interaction

This implementation positions tvision/test-tui as a reference implementation for AI-enhanced TUI applications using modern Claude Code SDK patterns.