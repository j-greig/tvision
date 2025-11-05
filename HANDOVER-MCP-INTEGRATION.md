# MCP Integration Handover: Wib&Wob Chat with Tool Support

**Date**: 2025-11-05
**Status**: ⚠️ In Progress - Config loading issue
**Goal**: Enable MCP tool support in TUI chat window via Claude Code CLI

---

## Problem Statement

The `test_pattern` TUI application has an embedded "Wib&Wob Chat" window that uses LLM providers for conversational AI. Originally, it only supported direct HTTP API calls (`anthropic_api` provider), which has **no tool/MCP support**.

**Objective**: Switch the chat to use Claude Code CLI (`claude_code` provider) which supports:
- MCP servers (Model Context Protocol)
- Tool execution (window spawning, time queries, memory storage)
- Session continuity across turns

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│ TUI Chat Window (test-tui/wibwob_view.cpp)                  │
│   └─> WibWobEngine (wibwob_engine.cpp)                      │
│        └─> Provider (claude_code_provider.cpp)              │
│             └─> claude CLI                                  │
│                  ├─> MCP: tui-control (port 8089)           │
│                  └─> MCP: symbient-brain                    │
└─────────────────────────────────────────────────────────────┘
```

### Key Components

1. **Config File**: `test-tui/llm/config/llm_config.json`
   - Specifies active provider and provider configurations
   - Loaded at runtime by `WibWobEngine::loadConfiguration()`

2. **Provider Implementation**: `test-tui/llm/providers/claude_code_provider.cpp`
   - Calls `claude -p` CLI with arguments from config
   - Parses JSON responses
   - Handles session management

3. **MCP Config**: `test-tui/.claude/settings.local.json`
   - Defines enabled MCP servers and permissions
   - Passed to Claude CLI via `--mcp-config` flag

4. **API Server**: `tools/api_server/main.py`
   - FastAPI server on port 8089
   - Exposes REST API + MCP endpoint at `/mcp`
   - Bridges HTTP to Unix socket IPC

---

## Changes Made

### 1. Updated Provider Config

**File**: `test-tui/llm/config/llm_config.json`

```json
{
  "activeProvider": "claude_code",  // ← Changed from "anthropic_api"
  "providers": {
    "claude_code": {
      "enabled": true,
      "command": "claude",
      "args": ["-p", "--mcp-config", ".claude/settings.local.json", "--output-format", "json"]
    }
  }
}
```

### 2. Enhanced claude_code_provider

**File**: `test-tui/llm/providers/claude_code_provider.cpp`

**Changes**:
- **Line 126-173**: Updated `configure()` to parse `args` array from config
- **Line 330-382**: Fixed `buildClaudeCommand()` to:
  - Use args from config (includes `--mcp-config`)
  - Use `--resume <session_id>` instead of `--continue`
  - Avoid duplicating `--output-format json`

**Key Fix**: Session management now uses explicit session IDs:
```cpp
if (!currentSessionId.empty()) {
    cmd << " --resume " << currentSessionId;
}
```

### 3. Documentation Updates

- **CLAUDE.md**: Added "Wib&Wob AI Chat Window (MCP-Enabled)" section (lines 555-644)
- **tools/api_server/README.md**: Fixed setup instructions with uv support
- **test-tui/README.md**: Added API server quick reference
- **README.md**: Added pointer to API docs
- **start_api_server.sh**: Helper script to start API server

---

## RESOLVED: Config Not Loading

**Fixed**: Changed default provider in `llm/base/llm_config.cpp:335` from `"anthropic_api"` to `"claude_code"`.

---

## Current Issue: UI Freeze During LLM Request

### Symptom

After restart, chat log still shows:
```
[2025-11-05 16:02:43.446] System: Using provider: anthropic_api, model: claude-3-5-haiku-latest
```

Despite config file containing:
```json
{"activeProvider": "claude_code"}
```

### Debug Steps Taken

1. ✅ Verified config file exists and has correct content
2. ✅ Rebuilt `test_pattern` app with updated provider code
3. ✅ Restarted TUI application
4. ❌ Provider still shows as `anthropic_api`

### Possible Causes

1. **Config caching**: Old config loaded before file was written
2. **Working directory mismatch**: App not running from expected directory
3. **Load failure fallback**: Config load fails silently, falls back to default
4. **Multiple config files**: Another config file taking precedence

### Debug Output to Check

The app should print to stderr:
```
DEBUG: Config file load result: SUCCESS
DEBUG: Config loaded successfully, active provider: claude_code
DEBUG: Attempting to initialize provider: claude_code
DEBUG: ClaudeCodeProvider configured: command=claude, args=4
```

**RESOLVED**: Fixed by changing default provider in compiled code.

### UI Freeze Issue

**Symptom**: Chat window freezes on message submit, spinner doesn't animate smoothly.

**Root Cause**: `claude -p` CLI takes 2-5 seconds to:
- Initialize Claude Code runtime
- Load MCP servers (tui-control, symbient-brain)
- Connect to Anthropic API
- Start processing request

Even though the pipe is set to non-blocking mode, the **first `fread()` call** can block if Claude hasn't written output yet.

**Async Infrastructure (Already Present)**:
- ✅ Spinner starts on line 317: `startSpinner()`
- ✅ Timer fires every 200ms (line 509)
- ✅ Timer calls `engine->poll()` (line 230)
- ✅ Poll calls `pollAsyncExecution()` which does non-blocking reads

**Why It Still Freezes**:
The initial `popen()` + first `fread()` can take 2-3 seconds before returning, during which the UI thread is blocked.

**Potential Fixes**:

1. **Immediate feedback** (Easy):
   ```cpp
   // In processInput(), before sendQuery:
   addMessage("System", "⏳ Connecting to Claude Code...");
   drawView();
   ```

2. **True async** (Better):
   - Spawn `claude -p` in a background thread
   - Use message queue to pass results back to UI thread
   - Requires thread safety for TView

3. **Keep-alive session** (Best):
   - Keep a persistent `claude` process running
   - Use `--input-format stream-json` for streaming input
   - Eliminates startup delay on subsequent requests

**Action Required**: Choose fix approach based on UX priorities

---

## Expected Behavior (When Fixed)

### 1. Startup Logs
```
DEBUG: Config file load result: SUCCESS
DEBUG: Config loaded successfully, active provider: claude_code
DEBUG: Successfully initialized provider: claude_code
```

### 2. Chat Session Log
```
[System] Using provider: claude_code, model: Claude Code
[User] create test pattern window
[System] Using provider: claude_code, model: Claude Code
[Wib&Wob] つ◕‿◕‿⚆༽つ *manifesting geometric chaos...*
         [Tool: create_test_pattern_window]
         つ⚆‿◕‿◕༽つ Window spawned: ID=w42
```

### 3. Window Actually Spawns
A new test pattern window should appear in the TUI when requested.

---

## How to Test (Once Fixed)
(Ask user if API server is running, it usually is already)

### Prerequisites
```bash
# Terminal 1: Start API server
./start_api_server.sh

# Terminal 2: Run TUI app
cd test-tui && ./build/test_pattern
```

### Test Commands in Chat

1. **"what time is it?"**
   - Should use `get_current_time` tool
   - Returns actual time from system

2. **"create test pattern window"**
   - Should use `create_test_pattern_window` tool via MCP
   - Window spawns in TUI

3. **"list all windows"**
   - Should use `list_windows` tool
   - Returns JSON of current windows

4. **"save memory about this conversation"**
   - Should use `symbient-brain` MCP server
   - Stores memory to GitHub

---

## Files Modified

### Core Implementation
- `test-tui/llm/config/llm_config.json` - Provider config
- `test-tui/llm/providers/claude_code_provider.cpp` - CLI integration
- `test-tui/llm/providers/claude_code_provider.h` - Header (unchanged)

### Documentation
- `CLAUDE.md` - MCP chat section added
- `tools/api_server/README.md` - Setup fixes
- `test-tui/README.md` - API reference
- `README.md` - Quick links

### Helper Scripts
- `start_api_server.sh` - API server launcher (new)

---

## Next Steps

1. **Investigate config loading**:
   - Add more debug output to `wibwob_engine.cpp:loadConfiguration()`
   - Check if `loadFromFile()` actually succeeds
   - Verify `getActiveProvider()` returns correct value

2. **Fallback handling**:
   - Check why fallback to `anthropic_api` might be happening
   - Verify `claude_code` provider is registered in factory

3. **Alternative approach** (if config issue persists):
   - Hardcode `claude_code` as default
   - Remove fallback to `anthropic_api`
   - Force config reload on every chat open

---

## Success Criteria

- [ ] Chat log shows "Using provider: claude_code"
- [ ] Tool calls appear in debug output
- [ ] Windows spawn when requested via chat
- [ ] Session continuity works (multi-turn conversations)
- [ ] MCP tools (tui-control + symbient-brain) accessible

---

## Related PRDs

- `test-tui/prds/provider-selection-debug.md`
- `test-tui/prds/llm-abstraction-layer.md`
- `test-tui/prds/wibwob-claude-code-sdk-integration.md`

---

## Contact / Continuation

For next developer:
1. Start by checking stderr output during chat initialization
2. Add debug prints in `LLMConfig::loadFromFile()` to see what's loaded
3. Verify `LLMProviderFactory` has `claude_code` registered
4. Check if `claude` CLI is available in PATH when app runs

**Git Branch**: `test-tui-apps`
**Last Build**: `cmake --build test-tui/build` (2025-11-05)
