# SDK Agent Integration - Architecture Notes

**tl;dr**: Claude Code SDK sessions cannot directly invoke `.claude/agents/` but can achieve similar delegation via tool chaining within single session or subprocess spawning. Wibwob can do "generate ASCII art → spawn MCP window" entirely within one streaming SDK session using existing tool infrastructure.

## Question

Can the streaming Claude Code SDK chat invoke agents defined in `.claude/agents/` for subtasks? Example use case: Main wibwob prompt delegates to "ASCII art agent" → agent creates art → MCP tool spawns text window with artwork.

## Refined Question

Does the Claude Code SDK support calling agents from `.claude/agents/` during a programmatic session, similar to `claude --agent <name>` in CLI? If not, what's the recommended pattern for task delegation in SDK-based applications?

## Answer Summary

### SDK Agent Limitations

**Agents are CLI-only primitives** - cannot be invoked from SDK sessions:
- `.claude/agents/` definitions only work with `claude --agent <name>` CLI invocation
- SDK sessions are standalone, atomic interactions without child agent spawning
- Agents run as separate subprocess invocations, not part of parent session

**Architecture difference**:
```
CLI:     claude --agent ascii-art "prompt" → spawns isolated agent process
SDK:     session.sendMessage("prompt")     → single session, no agent calls
```

### Recommended Patterns for Wibwob

#### Pattern 1: Single Session Tool Chaining (Recommended)

**Use case**: "Create ASCII art, display in new window"

**Architecture**:
```javascript
// SDK session has both generation + tool capabilities
const tools = [
  { name: "create_window", ... },     // MCP tool
  { name: "set_window_content", ... } // MCP tool
];

// Single streaming session does:
// 1. Generate ASCII art (via LLM)
// 2. Call create_window tool (via tool use)
// 3. Call set_window_content with art (via tool use)
```

**Implementation** (already supported):
- `claude_sdk_bridge.js` includes tools array with MCP tools
- Streaming callbacks handle both content deltas AND tool calls
- No separate agent processes needed

**Code reference**: `/Users/james/Repos/tvision-develop/app/llm/sdk_bridge/claude_sdk_bridge.js`

#### Pattern 2: Subprocess Agent Spawning (If Isolation Needed)

**Use case**: Truly isolated agent processing (rare)

**Implementation** (Node.js bridge extension):
```javascript
// Add to claude_sdk_bridge.js
const { exec } = require('child_process');

async function invokeAgent(agentName, prompt) {
  return new Promise((resolve, reject) => {
    const cmd = `claude --agent ${agentName} "${prompt.replace(/"/g, '\\"')}"`;
    exec(cmd, { maxBuffer: 10 * 1024 * 1024 }, (err, stdout, stderr) => {
      if (err) {
        reject(new Error(`Agent ${agentName} failed: ${stderr}`));
      } else {
        resolve(stdout);
      }
    });
  });
}

// Usage within SDK session tool execution:
// 1. SDK calls custom "invoke_agent" tool
// 2. Bridge spawns CLI agent subprocess
// 3. Agent output returned as tool result
// 4. SDK continues with agent's output
```

**Trade-offs**:
- ➕ True isolation (separate context, memory)
- ➕ Can use existing `.claude/agents/` definitions
- ➖ Slower (subprocess spawn overhead)
- ➖ No streaming from agent
- ➖ More complex error handling

### Wibwob-Specific Implementation

**Current architecture** (`app/wibwob_view.cpp` + `claude_code_sdk_provider`):
```cpp
// Streaming SDK session
sdkProvider->sendStreamingQuery(userMessage,
    [this](const StreamChunk& chunk) {
        if (chunk.type == StreamChunk::CONTENT_DELTA) {
            appendToStreamingMessage(chunk.content);  // ASCII art appears incrementally
        } else if (chunk.type == StreamChunk::TOOL_USE) {
            // MCP tool call (create_window, etc)
            executeTool(chunk.tool_name, chunk.tool_args);
        }
    }
);
```

**Example flow** (no agents needed):
```
User: "Draw ASCII cat, show in window"
  ↓
SDK Session:
  1. [CONTENT_DELTA] "Here's an ASCII cat:\n  /\\_/\\"
  2. [CONTENT_DELTA] " ( o.o )\n  > ^ <"
  3. [TOOL_USE] create_window({title: "ASCII Cat", x: 20, y: 10})
  4. [TOOL_USE] set_window_content({content: "[cat art]"})
  5. [MESSAGE_COMPLETE]
  ↓
Result: Art generated + window spawned in one session
```

## Key Files

**SDK Bridge** (tool definitions):
- `/Users/james/Repos/tvision-develop/app/llm/sdk_bridge/claude_sdk_bridge.js`
- `/Users/james/Repos/tvision-develop/app/llm/sdk_bridge/package.json`

**Streaming Provider** (handles tool use):
- `/Users/james/Repos/tvision-develop/app/llm/providers/claude_code_sdk_provider.{cpp,h}`

**UI Layer** (renders incremental updates):
- `/Users/james/Repos/tvision-develop/app/wibwob_view.{cpp,h}` (needs streaming methods ported)

**Config** (SDK as default):
- `/Users/james/Repos/tvision-develop/app/llm/config/llm_config.json`

## MCP Integration

**MCP tools already available** via API server:
- `POST /windows` - Create window (test_pattern, gradient, text_view, etc)
- `POST /windows/{id}/move` - Position/resize
- `POST /primers/batch` - Spawn primer text windows

**Making MCP tools available to SDK session**:

1. **Expose via bridge** (`claude_sdk_bridge.js`):
```javascript
// Add MCP server tools to SDK tools array
const tools = [
  {
    name: "create_tui_window",
    description: "Create a new TUI window with content",
    input_schema: {
      type: "object",
      properties: {
        window_type: { type: "string", enum: ["text_view", "gradient", "test_pattern"] },
        title: { type: "string" },
        content: { type: "string" },
        x: { type: "integer" },
        y: { type: "integer" }
      },
      required: ["window_type", "content"]
    }
  }
];

// Tool execution (call TUI API)
async function executeTool(toolName, args) {
  if (toolName === "create_tui_window") {
    const response = await fetch('http://127.0.0.1:8089/windows', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        type: args.window_type,
        title: args.title,
        rect: { x: args.x, y: args.y, w: 40, h: 15 },
        props: { content: args.content }
      })
    });
    return await response.json();
  }
}
```

2. **Wire to C++ provider** (`claude_code_sdk_provider.cpp`):
```cpp
// Already handles tool execution in processStreamingThread()
if (chunk.type == StreamChunk::TOOL_USE) {
    // Bridge executes tool, returns result
    // Result sent back to SDK session
}
```

## Feature Request

**Current gap**: SDK cannot invoke `.claude/agents/` directly.

**Feedback channel**: `claude code` CLI → `/feedback` command

**Proposed feature**: Agent invocation API for SDK sessions:
```typescript
// Hypothetical API
session.invokeAgent('ascii-artist', {
  prompt: 'draw a cat',
  tools: [...] // Agent can use tools
});
```

Would enable true agent composition in programmatic contexts.

## Decision Matrix

| Approach | Use When | Pros | Cons |
|----------|----------|------|------|
| **Single Session Tool Chaining** | Default for most cases | Fast, streaming, simple | No isolation between subtasks |
| **Subprocess Agent Spawning** | Need isolated context/state | True isolation, can use `.claude/agents/` | Slower, no streaming, complex |
| **Future SDK Agent API** | When available | Best of both | Doesn't exist yet |

## Implementation Status

- ✅ SDK streaming session works
- ✅ Tool infrastructure exists
- ✅ MCP server has window management tools
- ⏳ Streaming methods need porting to `app/wibwob_view.cpp`
- ⏳ MCP tools not yet wired to SDK session (need bridge updates)
- ❌ Agent invocation not supported (architectural limitation)

## Next Steps

1. **Complete SDK migration** (see `app/prds/sdk-streaming-migration-handover.md`)
2. **Port streaming methods** to `app/wibwob_view.{cpp,h}`
3. **Add MCP tools to SDK bridge** (modify `claude_sdk_bridge.js` tools array)
4. **Test tool chaining**: "create art → spawn window" flow
5. **Consider subprocess agent pattern** only if isolation truly needed

## Related Documentation

- **Claude Code Agent SDK**: Standard SDK documentation (via claude-code-guide agent)
- **MCP Protocol**: Model Context Protocol for tool definitions
- **Blocking vs Streaming**: `/Users/james/Repos/tvision/test-tui/prds/blocking-vs-streaming-chat-comparison.md`
- **Migration Guide**: `/Users/james/Repos/tvision-develop/app/prds/sdk-streaming-migration-handover.md`
- **API Server**: `/Users/james/Repos/tvision/CLAUDE.md` (Programmatic Control API section)

---

**Author**: Claude Code (Sonnet 4.5)
**Date**: 2024-12-02
**Context**: SDK streaming migration, agent delegation investigation
**Status**: Architecture notes for dev handover
