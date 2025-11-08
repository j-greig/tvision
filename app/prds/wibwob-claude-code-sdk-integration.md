# WibWob Claude Code SDK Integration PRD

**TL;DR:** Enable natural‑language TUI control with a Node SDK server (Express `/chat` on 8090) that streams via Claude Code SDK + MCP tools to FastAPI (8089) controlling the running TUI app; includes Zod validation, diagnostics, and clear error handling.

## Quick Start

Get running in 5 minutes (all three must run):

```bash
# 1. Setup Python venv (ONE TIME ONLY)
cd /Users/james/Repos/tvision
python3 -m venv .venv
source .venv/bin/activate
pip install -r tools/api_server/requirements.txt

# 2. Start FastAPI server (REQUIRED - port 8089)
# (In repo root with venv activated)
source .venv/bin/activate
python -m tools.api_server.main --port=8089

# 3. Start TUI app (separate terminal)
cd test-tui && ./build/test_pattern

# 4. Configure API key + run SDK server (separate terminal)
# Put your key in test-tui/.env: ANTHROPIC_API_KEY=sk-ant-...
cd tools/sdk-server && npm install && npm start

# 5. Test tool execution
curl -X POST "http://localhost:8090/chat" \
  -H "Content-Type: application/json" \
  -d '{"message": "Create a gradient window at position 30,10"}'
```

## 🚨 **Critical Startup Order**
All three components must be running simultaneously:

1. **FastAPI Server** (port 8089) - Handles TUI window creation
2. **TUI App** - Displays the windows via Unix socket
3. **SDK Server** (port 8090) - Express `/chat` endpoint for Claude + MCP

**App Flow:**
```
curl/Chat UI → SDK Server (Express /chat :8090)
  → Claude Code SDK → MCP Tools → FastAPI (:8089) → TUI App
```

## Problem Statement

The current wibwob chat interface operates as a passive conversation UI with limited interactive capabilities. Users cannot leverage the full power of the TUI application's features through natural language commands.

**Core Issues:**
- Chat interface cannot manipulate TUI windows or application state
- No integration between conversational AI and programmatic control
- Manual API calls required for advanced operations
- Missed opportunity for agentic workflow automation

## Solution Architecture

### Overview
Transform the wibwob chat interface into an agentic control center by:
1. TypeScript ↔ Python bridge: Express SDK server (Node) → FastAPI (Python)
2. Streaming input via `AsyncIterable` and chunked response handling
3. Zod validation for tool parameters + robust error handling
4. (Next) Conversation memory and state synchronization

### Technical Architecture
```
WibWob Chat Window (C++)
  ↓ (HTTP today; WS optional later)
Express SDK Server (:8090) — tools/sdk-server/main.js
  ↓ (streaming AsyncIterable)
Claude Code SDK + in‑proc MCP server
  ↓ (HTTP)
FastAPI Server (:8089) — tools/api_server
  ↓ (Unix socket IPC)
TUI App (test_pattern, etc.)
```

## Implementation Plan

### Phase 1: SDK Bridge Foundation (2 days)
- [x] TypeScript SDK server with streaming input (`/chat`)
- [ ] WebSocket bridge between C++ chat and SDK server (optional)
- [x] MCP server creation with basic tools
- [x] Tool execution pipeline to FastAPI
- [x] Response streaming and chunk handling

### Phase 2: Core Tool Suite (2 days)
- [x] Map FastAPI endpoints to MCP tools with Zod schemas
- [x] Error handling for tool failures
- [~] Window management: create (gradient/pattern), move, close; [ ] focus
- [~] State tool (`/state`); [!] Screenshot deferred (not implemented in TUI)

### Phase 3: Chat Integration (1 day)
- [ ] Conversation memory across tool calls
- [ ] State synchronization between chat and TUI
- [ ] Basic workspace save/load through chat

### Phase 4: Polish & Testing (1 day)
- [ ] End-to-end testing of tool workflows
- [ ] Error recovery for common failure modes
- [ ] Simple usage examples and documentation

## Technical Specifications

### Current SDK Server (tools/sdk-server/main.js)

- Express on port `8090` with endpoints:
  - `GET /health` — diagnostics: API key status, tool count, FastAPI connectivity
  - `POST /debug-claude` — test Claude response without tools
  - `POST /chat` — streams Claude chunks and enables MCP tools
- In‑process MCP server: `createSdkMcpServer({ name: "tui-control", tools })`
- Tools implemented (positional overload `tool(name, desc, schema, handler)`):
  - `mcp__tui_control__create_gradient(x,y,w,h,title?,gradient)` → `POST /windows`
  - `mcp__tui_control__create_pattern(x,y,w,h,title?)` → `POST /windows`
  - `mcp__tui_control__move_window(id,x?,y?,w?,h?)` → `POST /windows/{id}/move`
  - `mcp__tui_control__close_window(id)` → `POST /windows/{id}/close`
  - `mcp__tui_control__get_state()` → `GET /state`
- Streaming input: async generator yields `{ type: "user", content:[{ type:"text", text }] }`
- Chunk handling: aggregates `text` and `tool_use` (others passed through)
- Config: loads `ANTHROPIC_API_KEY` from `test-tui/.env` (should start with `sk-ant-`)

### Doc‑Aligned Alternative (future refactor)
- SDK docs prefer object‑form tools:
  `tool({ name, description, parameters: z.object(...), handler })`
- Consider switching to object‑form to match docs and ease evolution.

### C++ Client Hook (HTTP now; WS optional later)

```cpp
// test-tui/wibwob_sdk_client.h
class WibWobSDKClient {
private:
    std::string server_url;
    std::function<void(const std::string&)> response_callback;

public:
    WibWobSDKClient(const std::string& url) : server_url(url) {}

    void setResponseCallback(std::function<void(const std::string&)> callback) {
        response_callback = callback;
    }

    void sendMessage(const std::string& message) {
        nlohmann::json request = { {"message", message} };
        // HTTP POST to SDK server /chat is the MVP path
        sendHTTPRequest(request.dump());
    }

private:
    void sendHTTPRequest(const std::string& json_data) {
        // Simple HTTP POST to SDK server at http://localhost:8090/chat
        // Implementation details...
    }
};
```

## API/Tool Mappings

### Core Tools
| FastAPI Endpoint | MCP Tool Name | Description |
|------------------|---------------|-------------|
| `POST /windows` | `mcp__tui_control__create_gradient` | Create gradient window (x,y,w,h,title?,gradient) |
| `POST /windows` | `mcp__tui_control__create_pattern` | Create test pattern window (x,y,w,h,title?) |
| `POST /windows/{id}/move` | `mcp__tui_control__move_window` | Move/resize existing windows |
| `POST /windows/{id}/focus` | `mcp__tui_control__focus_window` | Bring window to front (planned) |
| `POST /windows/{id}/close` | `mcp__tui_control__close_window` | Close specific window |
| `GET /state` | `mcp__tui_control__get_state` | Get current application state |
| `POST /screenshot` | `mcp__tui_control__take_screenshot` | Capture current screen (deferred) |

### Usage Examples
```
User: "Create a gradient window at position 30,10"
→ Tool: mcp__tui_control__create_gradient(x:30,y:10,w:20,h:10,gradient:"radial")

User: "Show me the current state"
→ Tool: mcp__tui_control__get_state()
→ Response: JSON state summary (window count + canvas size)
```

## Success Criteria

### Functional Requirements
- [x] Core FastAPI endpoints mapped (create/move/close/state)
- [x] Natural language commands execute tool operations via `/chat`
- [ ] Conversation context maintained across tool calls
- [x] Error handling with user‑friendly messages and diagnostics

### Performance Requirements
- [ ] Tool execution latency < 1s for simple operations
- [x] Streaming responses work reliably
- [ ] No memory leaks during extended conversations

## Testing Strategy

### Unit Tests
- [ ] Individual tool execution with mock responses
- [ ] Schema validation for tool parameters
- [ ] Error handling for network failures

### Integration Tests
- [ ] End-to-end tool execution with live FastAPI server
- [ ] Multi-tool conversations
- [ ] WebSocket connection stability (if WS added)

## Timeline & Milestones

### Week 1: Foundation (Days 1-2)
- **Milestone 1**: SDK server with streaming `/chat` (DONE)
- **Milestone 2**: WebSocket bridge (OPTIONAL)
- **Deliverables**: Core infrastructure, tool execution via FastAPI

### Week 2: Tools & Integration (Days 3-6)
- **Milestone 3**: Window tools functional (add `focus_window`)
- **Milestone 4**: Chat integration with conversation memory
- **Deliverables**: Complete tool suite, working chat interface

## Troubleshooting

### Common Issues

**Problem**: MCP tools not working
```bash
# Ensure streaming input mode:
async function* messages() {
  yield { type: "user", content: [{ type: "text", text: message }] };
}
```

**Problem**: SDK server shows message processing but no Claude responses
```bash
# FIXED: incorrect streaming format — content array with text
# Expected logs:
# ✅ "💬 Processing message: Create a gradient window..."
# ✅ "🔄 Creating Claude query with MCP tools..."
# ✅ "📨 Claude response chunk: text ..."
# ✅ "📨 Claude response chunk: tool_use"
# ✅ "📡 Calling POST http://localhost:8089/windows"
```

**Problem**: FastAPI server not running
```bash
# Check if FastAPI server is running on port 8089
curl http://localhost:8089/state
# If "Connection refused", start it:
python -m tools.api_server.main --port=8089
```

**Problem**: Direct window creation test
```bash
# Test FastAPI directly (bypass SDK)
curl -X POST "http://localhost:8089/windows" \
  -H "Content-Type: application/json" \
  -d '{"type": "gradient", "title": "Test", "rect": {"x": 25, "y": 5, "w": 15, "h": 10}, "props": {"gradient": "radial"}}'
```

**Problem**: Screenshot tool fails
```bash
# Known: Screenshot not implemented in test-tui — defer this tool
```

**Problem**: SDK server health check
```bash
curl http://localhost:8090/health
# Should show tools count, API key status, and FastAPI connectivity
```

## MVP Phase Order

### Step 1: Basic Window Spawning (Test First)
- [x] Gradient windows at various positions
- [x] Pattern windows at different locations
- [x] Verify positioning works correctly

### Step 2: Text Window Integration (Once Step 1 Confirmed)
- [ ] Spawn text windows from chat commands
- [ ] Pipe generated content into new text windows
- [ ] Chat-driven content creation workflow

### Step 3: Focus & Memory
- [ ] Add `mcp__tui_control__focus_window` tool (`POST /windows/{id}/focus`)
- [ ] Add rolling chat memory to `/chat` request generator

## Progress Tracking Instructions

### How to Update This PRD

**Daily Progress Updates:**
1. **Mark completed tasks**: Change `- [ ]` to `- [x]` for finished items
2. **Add blockers**: Note any issues in comments next to tasks
3. **Update timelines**: Adjust phase dates if behind/ahead of schedule

**Checkbox Status Guide:**
- `- [ ]` = Not started
- `- [x]` = Completed successfully
- `- [!]` = Blocked (add reason in comment)
- `- [~]` = In progress

**Example:**
```markdown
- [x] TypeScript SDK server with streaming input generators
- [~] WebSocket bridge between C++ chat and SDK server
- [!] MCP server creation <!-- Blocked: SDK version incompatibility -->
- [ ] Basic tool execution pipeline with error handling
```

**File Location:** Always keep this PRD at `/Users/james/Repos/tvision/test-tui/prds/wibwob-claude-code-sdk-integration.md`

---

This PRD establishes a simple, focused foundation for transforming the wibwob chat interface into an agentic control center with natural language TUI manipulation capabilities.
