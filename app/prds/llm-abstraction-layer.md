# LLM Abstraction Layer PRD

**tl;dr:** Replace hardcoded Claude Code integration with pluggable LLM provider system supporting multiple inference backends (Claude Code, Anthropic API, OpenRouter) through unified interface with JSON configuration and drop-in compatibility for existing chat system.

## Context

The current WibWobEngine in the TVision test-tui chat system is tightly coupled to Claude Code CLI via subprocess calls. This creates vendor lock-in and limits provider options. We need a neutral abstraction layer that enables swapping LLM providers while maintaining existing chat interface functionality.

### Current Architecture Pain Points
- **Hardcoded Provider**: `wibwob_engine.{h,cpp}` directly calls `claude -p` subprocess
- **No Flexibility**: Cannot test different models or switch providers
- **Vendor Lock-in**: Entirely dependent on Claude Code CLI availability
- **Configuration Rigidity**: No runtime provider switching capability

## Objective

Create a pluggable LLM abstraction layer that:
1. **Decouples** chat interface from specific LLM provider implementation
2. **Enables** seamless provider switching via configuration
3. **Maintains** existing chat functionality and TVision integration
4. **Supports** multiple providers with unified API surface
5. **Facilitates** testing with different models (specifically Anthropic Haiku)

## Requirements

### Core Abstraction Layer
• **Unified Interface**: Abstract base class defining standard LLM operations
• **Provider Registry**: Dynamic provider discovery and instantiation system
• **Session Management**: Multi-turn conversation state across providers
• **Async Support**: Non-blocking operations compatible with TVision event loop
• **Error Handling**: Standardized error reporting across all providers

### Provider Implementations
• **Claude Code Provider**: Current subprocess-based implementation (preserved)
• **Anthropic API Provider**: Direct REST API integration with Haiku model testing
• **OpenRouter Provider**: Support for OpenRouter endpoint with multiple models
• **Extensible Architecture**: Plugin system for future provider additions

### Configuration System
• **JSON Configuration**: Provider selection and settings in config file
• **Environment Overrides**: API keys and sensitive data via environment variables
• **Runtime Reloading**: Hot-swap providers without application restart
• **Provider-Specific Settings**: Model names, endpoints, parameters per provider
• **Validation**: Configuration schema validation and error reporting

### Integration Constraints
• **Drop-in Compatibility**: Zero changes required to TWibWobView chat interface
• **Async Preservation**: Maintain spinner animation and non-blocking operations
• **Session Continuity**: Preserve multi-turn conversation functionality
• **Custom Prompts**: Keep system prompt loading capability
• **TVision Patterns**: Follow existing C++14 and TVision architectural conventions

## Technical Architecture

### Interface Design
```cpp
// Base abstraction interface
class ILLMProvider {
public:
    virtual ~ILLMProvider() = default;
    virtual void sendMessage(const std::string& message, 
                           const std::string& systemPrompt = "") = 0;
    virtual bool isProcessing() const = 0;
    virtual std::string getResponse() = 0;
    virtual void reset() = 0;
    virtual std::string getProviderName() const = 0;
};

// Provider factory system
class LLMProviderFactory {
public:
    static std::unique_ptr<ILLMProvider> createProvider(const std::string& type);
    static void registerProvider(const std::string& type, 
                               std::function<std::unique_ptr<ILLMProvider>()> factory);
};
```

### Provider Plugin Architecture
```
llm/
├── base/
│   ├── illm_provider.h          # Abstract interface
│   ├── llm_config.h             # Configuration management
│   └── llm_provider_factory.h   # Provider registry
├── providers/
│   ├── claude_code_provider.{h,cpp}    # Subprocess implementation
│   ├── anthropic_api_provider.{h,cpp}  # REST API implementation  
│   └── openrouter_provider.{h,cpp}     # OpenRouter implementation
└── config/
    └── llm_config.json          # Provider configuration
```

### Configuration Schema
```json
{
  "activeProvider": "anthropic_api",
  "providers": {
    "claude_code": {
      "enabled": true,
      "command": "claude",
      "args": ["-p"]
    },
    "anthropic_api": {
      "enabled": true,
      "model": "claude-3-haiku-20240307",
      "endpoint": "https://api.anthropic.com/v1/messages",
      "apiKeyEnv": "ANTHROPIC_API_KEY",
      "maxTokens": 4096,
      "temperature": 0.7
    },
    "openrouter": {
      "enabled": true,
      "model": "anthropic/claude-3-haiku",
      "endpoint": "https://openrouter.ai/api/v1/chat/completions",
      "apiKeyEnv": "OPENROUTER_API_KEY"
    }
  }
}
```

### WibWobEngine Integration
```cpp
// Modified WibWobEngine using abstraction
class WibWobEngine {
private:
    std::unique_ptr<ILLMProvider> m_provider;
    LLMConfig m_config;
    
public:
    WibWobEngine();
    void switchProvider(const std::string& providerName);
    void reloadConfig();
    // Existing interface preserved unchanged
    void sendMessage(const std::string& message);
    bool isProcessing() const;
    std::string getResponse();
};
```

## Implementation Phases

### Phase 1: Core Abstraction (Week 1)
**Deliverables:**
- `ILLMProvider` abstract interface
- `LLMProviderFactory` registry system  
- `LLMConfig` configuration management
- Unit test framework for providers

**Success Criteria:**
□ Abstract interface compiles and links
□ Factory pattern correctly instantiates providers
□ Configuration loads and validates JSON schema
□ Test harness ready for provider implementations

### Phase 2: Provider Implementations (Week 2)
**Deliverables:**
- `ClaudeCodeProvider` (migrate existing logic)
- `AnthropicAPIProvider` with Haiku model support
- `OpenRouterProvider` basic implementation
- HTTP client integration for API providers

**Success Criteria:**
□ Claude Code provider maintains exact current functionality
□ Anthropic API successfully connects and responds with Haiku
□ OpenRouter provider handles authentication and basic requests
□ All providers pass interface compliance tests

### Phase 3: Integration & Testing (Week 3)
**Deliverables:**
- Modified `WibWobEngine` using abstraction layer
- Configuration file and environment variable support
- Runtime provider switching capability
- Comprehensive testing with all providers

**Success Criteria:**
□ Existing chat interface works unchanged with new system
□ Provider switching works seamlessly during runtime
□ Configuration hot-reload functions correctly
□ Haiku model testing validates Anthropic API integration

### Phase 4: Documentation & Polish (Week 4)
**Deliverables:**
- Provider development documentation
- Configuration reference guide
- Error handling improvements
- Performance optimization

**Success Criteria:**
□ Clear documentation for adding new providers
□ All error cases handled gracefully
□ Performance matches or exceeds current implementation
□ Code ready for production deployment

## Testing Strategy

### Unit Testing
- **Provider Interface Compliance**: Each provider implements full interface correctly
- **Configuration Validation**: JSON schema validation and error handling
- **Factory Pattern**: Provider instantiation and registration
- **Session Management**: Multi-turn conversation state preservation

### Integration Testing
- **Drop-in Compatibility**: Existing TWibWobView works unchanged
- **Provider Switching**: Runtime switching preserves session state
- **Configuration Hot-reload**: Config changes applied without restart
- **Error Propagation**: Provider errors handled gracefully by chat interface

### Provider-Specific Testing
- **Claude Code Provider**: Subprocess communication, JSON parsing, error handling
- **Anthropic API Provider**: REST API calls, authentication, Haiku model responses
- **OpenRouter Provider**: API compatibility, model selection, rate limiting
- **Cross-Provider**: Same conversation across different providers

### Performance Testing
- **Latency Comparison**: Response times across providers
- **Memory Usage**: Resource consumption of different implementations  
- **Concurrency**: Multiple concurrent requests handling
- **Error Recovery**: Graceful degradation and recovery mechanisms

## Risk Assessment & Mitigation

### High Risk: Breaking Existing Functionality
**Risk**: Chat interface stops working during refactoring
**Mitigation**: 
- Implement abstraction layer alongside existing code
- Use feature flags to switch between old and new implementations
- Extensive regression testing before removing old code

### Medium Risk: Provider API Changes
**Risk**: External API providers change interfaces or authentication
**Mitigation**:
- Version external API calls explicitly
- Implement retry logic with exponential backoff
- Provider health checks and automatic failover
- Comprehensive error handling and user feedback

### Medium Risk: Configuration Complexity
**Risk**: Complex configuration causes user errors or system instability
**Mitigation**:
- JSON schema validation with clear error messages
- Sensible defaults for all optional settings
- Configuration wizard or setup assistant
- Runtime validation and error reporting

### Low Risk: Performance Degradation
**Risk**: Abstraction layer introduces performance overhead
**Mitigation**:
- Benchmark current performance as baseline
- Profile abstraction layer for bottlenecks
- Implement caching where appropriate
- Direct provider access for performance-critical paths

## Success Metrics

### Functional Requirements
□ **Drop-in Compatibility**: Existing chat interface works without modification
□ **Provider Switching**: Can switch between any configured provider at runtime
□ **Haiku Testing**: Successfully test conversations using Anthropic API with Haiku model
□ **Configuration Management**: JSON config loading, validation, and hot-reload
□ **Error Handling**: Graceful error handling across all providers

### Quality Requirements
□ **Test Coverage**: >90% code coverage for abstraction layer and providers
□ **Documentation**: Complete API documentation and provider development guide
□ **Performance**: Response latency within 10% of current implementation
□ **Reliability**: Zero crashes during provider switching or configuration changes
□ **Extensibility**: New provider can be added in <4 hours by following documentation

### Integration Requirements
□ **TVision Compatibility**: Maintains spinner animation and async behavior
□ **Session Preservation**: Multi-turn conversations work across all providers
□ **System Prompt Loading**: Custom prompt functionality preserved
□ **C++14 Compliance**: Code follows existing project standards and patterns

## Future Extensibility

### Provider Ecosystem
- **Local Models**: Ollama, llama.cpp integration
- **Cloud Providers**: AWS Bedrock, Azure OpenAI, Google Vertex AI
- **Specialized APIs**: Perplexity, Cohere, Together AI
- **Custom Endpoints**: Enterprise or self-hosted model servers

### Advanced Features
- **Load Balancing**: Distribute requests across multiple provider instances
- **Fallback Chains**: Automatic failover between providers
- **Cost Optimization**: Route requests to cheapest available provider
- **Model Selection**: Dynamic model selection based on request characteristics
- **Streaming Responses**: Real-time token streaming for improved UX

### Monitoring & Analytics
- **Provider Performance**: Response times, success rates, cost tracking
- **Usage Metrics**: Request patterns, popular providers, error rates
- **Health Monitoring**: Provider availability, API quota tracking
- **A/B Testing**: Compare provider performance for specific use cases

This abstraction layer provides a solid foundation for LLM provider flexibility while maintaining the existing chat experience and enabling future enhancements to the TVision TUI ecosystem.