# Claude Code SDK Integration Debugging Solution PRD

**TL;DR:** Comprehensive fix for non-responding Claude Code SDK integration. Addresses streaming input patterns, tool registration, API key handling, and response processing. Expected outcome: working AI conversation with MCP tool execution.

## Context
FastAPI server creates TUI windows perfectly via direct HTTP calls. Node.js SDK server exists but Claude Code SDK isn't responding - logs show "Processing message" but empty responses array. Need working AI conversation where Claude can create TUI windows through natural language.

## Objective
Transform broken SDK integration into working AI-driven TUI control system where users can say "create a smiley face" and Claude executes multiple MCP tool calls to create positioned gradient windows.

## Root Cause Analysis

### Current Broken Pattern
```typescript
// ❌ BROKEN: Basic generator without proper message structure
async function* generateMessages() {
  yield {
    type: "user",
    message: {
      role: "user",
      content: message
    }
  };
}

// ❌ BROKEN: Basic query call that returns empty responses
for await (const response of query({
  prompt: generateMessages(),
  options: { mcpServers: {...}, allowedTools: [...] }
})) {
  responses.push(response); // Always empty!
}
```

### Claude Code SDK Requirements (From Documentation)
1. **Proper streaming input format** - Must use correct AsyncIterable pattern
2. **Message structure validation** - Exact format required for Claude consumption
3. **Tool registration** - MCP server must be properly initialized before query
4. **API key configuration** - Must be available in process environment
5. **Response stream handling** - Different response types need distinct processing

## Solution Architecture

### Fixed Implementation Pattern
```typescript
// ✅ CORRECT: Proper streaming input with validated message structure
async function* createStreamingInput(userMessage: string) {
  yield {
    type: "user" as const,
    content: [{
      type: "text" as const,
      text: userMessage
    }]
  };
}

// ✅ CORRECT: Proper query configuration with error handling
const responses = [];
try {
  const stream = query({
    messages: createStreamingInput(message),
    model: "claude-3-5-sonnet-20241022",
    maxTokens: 2000,
    mcpServers: { "tui-control": mcpServer },
    toolChoice: "auto"
  });

  for await (const chunk of stream) {
    if (chunk.type === "text") {
      responses.push({ type: "text", content: chunk.text });
    } else if (chunk.type === "tool_use") {
      responses.push({ type: "tool_use", name: chunk.name, result: chunk.result });
    }
  }
} catch (error) {
  console.error("Stream processing failed:", error);
}
```

## Requirements

### Critical Fixes Required
• **Streaming Input Format** - Use proper AsyncIterable with correct message structure
• **Response Processing** - Handle different chunk types (text, tool_use, error)
• **Tool Registration** - Verify MCP server initialization before query calls
• **Error Handling** - Comprehensive error catching and meaningful error messages
• **API Key Validation** - Ensure ANTHROPIC_API_KEY is accessible and valid

### Technical Specifications
• **Input Format**: Messages must follow Claude API schema exactly
• **Stream Processing**: Handle multiple response chunk types appropriately
• **Tool Execution**: MCP tools must execute and return results to conversation
• **State Management**: Maintain conversation context across multiple tool calls
• **Error Recovery**: Graceful handling of network failures and invalid responses

## Expected Output

### Working Conversation Flow
```bash
# Input
curl -X POST "http://localhost:8090/chat" \
  -d '{"message": "Create a smiley face using gradient windows"}'

# Expected Response
{
  "success": true,
  "responses": [
    {
      "type": "text",
      "content": "I'll create a smiley face using gradient windows positioned as eyes, nose, and mouth."
    },
    {
      "type": "tool_use",
      "name": "mcp__tui_control__create_gradient",
      "result": {"id": "w1", "title": "Left Eye"}
    },
    {
      "type": "tool_use",
      "name": "mcp__tui_control__create_gradient",
      "result": {"id": "w2", "title": "Right Eye"}
    },
    {
      "type": "text",
      "content": "Created a smiley face with gradient windows! The eyes are radial gradients positioned at (25,8) and (55,8)."
    }
  ]
}
```

### Debugging Capabilities
• **Verbose Logging** - Show each step of stream processing
• **Tool Execution Tracking** - Log MCP tool calls and results
• **Error Classification** - Distinguish between API, network, and configuration errors
• **State Inspection** - Ability to query current TUI state for verification

## Success Criteria

□ **Claude Responds**: Non-empty responses array with actual AI conversation
□ **Tools Execute**: MCP tools successfully create TUI windows via FastAPI
□ **Natural Language**: Commands like "create a smiley face" work end-to-end
□ **Error Handling**: Meaningful error messages when things go wrong
□ **State Sync**: Can query TUI state and get accurate window information

## Implementation Priority

### Phase 1: Core SDK Fix (Immediate)
1. **Fix streaming input format** - Use proper AsyncIterable pattern
2. **Fix response processing** - Handle different chunk types correctly
3. **Add comprehensive error handling** - Catch and classify all error types
4. **Validate API key setup** - Ensure Claude can actually respond

### Phase 2: Tool Execution (Next)
1. **Verify MCP server initialization** - Tools must be registered before use
2. **Test individual tool calls** - Each MCP tool should work in isolation
3. **Fix tool parameter validation** - Ensure Zod schemas match expectations
4. **Add tool execution logging** - Track when/why tools succeed or fail

### Phase 3: Integration Testing (Final)
1. **End-to-end conversation testing** - Full user message → tool execution → response
2. **Complex command handling** - Multi-tool commands like "create smiley face"
3. **Error recovery testing** - Graceful handling when FastAPI is down
4. **Performance optimization** - Response times under 3 seconds

## Troubleshooting Guide

### Symptom: "Processing message" but empty responses array
**Logs**: `💬 Processing message: ...` but no `📨 Claude response chunk: ...`
**Root Cause**: Streaming input format incorrect - old SDK used different message structure
**Solution**: Use `{ type: "user", content: [{ type: "text", text: message }] }` format

### Symptom: Claude responds but tools never execute
**Logs**: Text responses work but no `tool_use` chunks appear
**Root Cause**: MCP server not registered or tool names don't match allowedTools
**Solution**: Verify `mcpServers: { "tui-control": mcpServer }` and tool name consistency

### Symptom: "API key not found" or authentication errors
**Logs**: `❌ CRITICAL: ANTHROPIC_API_KEY not found in environment!`
**Root Cause**: Environment variable not loaded from test-tui/.env
**Solution**: Verify `.env` file exists with `ANTHROPIC_API_KEY=sk-ant-...` and path is correct

### Symptom: Tools execute but FastAPI calls fail
**Logs**: `📡 Calling POST http://localhost:8089/windows` followed by `❌ API error:`
**Root Cause**: FastAPI server down or TUI app not connected
**Solution**: Start FastAPI server and verify TUI app is running with Unix socket

### Symptom: Stream processing fails immediately
**Logs**: `❌ Stream processing error: ...`
**Root Cause**: Model name incorrect or rate limiting
**Solution**: Verify model name `claude-3-5-sonnet-20241022` and API quota

## Updated File Structure

**Primary Implementation**: `/Users/james/Repos/tvision/tools/sdk-server/main.js`
**This PRD Location**: `/Users/james/Repos/tvision/test-tui/prds/claude-code-sdk-debugging-solution.md`
**Original PRD**: `/Users/james/Repos/tvision/test-tui/prds/wibwob-claude-code-sdk-integration.md`

## Testing Commands

### Step-by-Step Debugging Process

#### 1. Verify Component Health
```bash
# Check all services are running
curl http://localhost:8089/state  # FastAPI server
curl http://localhost:8090/health # SDK server with diagnostics
ps aux | grep test_pattern        # TUI app process
```

#### 2. Test Claude API Connection (Without MCP)
```bash
# Test basic Claude response first
curl -X POST "http://localhost:8090/debug-claude" \
  -H "Content-Type: application/json" \
  -d '{"message": "Hello Claude, please respond with just \"Working\""}'

# Expected: { "success": true, "responses": [...], "count": 1 }
# If failed: Check API key in test-tui/.env
```

#### 3. Test MCP Tool Integration
```bash
# Test simple tool call
curl -X POST "http://localhost:8090/chat" \
  -H "Content-Type: application/json" \
  -d '{"message": "Get the current TUI state"}'

# Expected: Claude calls get_state tool and reports window information
```

#### 4. Test Window Creation
```bash
# Test window creation through Claude
curl -X POST "http://localhost:8090/chat" \
  -H "Content-Type: application/json" \
  -d '{"message": "Create a radial gradient window at position 30,10"}'

# Expected: Claude creates window and confirms success
```

#### 5. Test Complex Commands
```bash
# Test multi-window arrangement
curl -X POST "http://localhost:8090/chat" \
  -H "Content-Type: application/json" \
  -d '{"message": "Create a smiley face using gradient windows positioned as eyes and nose"}'

# Expected: Claude creates multiple positioned windows
```

## Expected Log Output (After Fix)
```
💬 Processing message: Create a gradient window at position 30,10
🔄 Creating streaming input for Claude...
📨 Claude response chunk: text - "I'll create a radial gradient window at the specified position."
📨 Claude response chunk: tool_use - mcp__tui_control__create_gradient
📡 Calling POST http://localhost:8089/windows {"type":"gradient",...}
✅ API success: {"id":"w3","focused":true}
📨 Claude response chunk: text - "Successfully created a radial gradient window at position (30,10)!"
✅ Total responses: 3 (2 text, 1 tool_use)
```

This comprehensive solution addresses the core streaming API issues, provides proper error handling, and establishes a working foundation for AI-driven TUI control.