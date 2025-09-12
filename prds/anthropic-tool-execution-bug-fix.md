# Anthropic API Tool Execution Bug Fix - PRD

**TL;DR**: Turbo Vision TUI chat can send tools to Anthropic API and receives tool_use responses, but our JSON parser fails to extract tool calls due to incorrect search patterns. Tool execution never happens - conversations stop after "I'll use the tool" without actual tool results.

## Problem Statement

### Current Behavior
1. User asks: "What time is it?"
2. Anthropic API returns: `{"content":[{"type":"tool_use","name":"get_current_time"}], "stop_reason":"tool_use"}`
3. Parser searches for: `"type": "tool_use"` (with spaces)
4. JSON contains: `"type":"tool_use"` (no spaces)
5. Parser fails: "No tool_use pattern found"  
6. Conversation stops, no tool execution

### Expected Behavior
1. Parser extracts tool_use blocks correctly
2. Tool executor runs get_current_time → "15:30:45"
3. Follow-up request sent to Anthropic with tool results
4. Final response: "The current time is 15:30:45"

## Technical Architecture

### File Structure
```
/Users/james/Repos/tvision/test-tui/
├── llm/
│   ├── base/
│   │   ├── illm_provider.h           # Tool support interface
│   │   ├── itool.h                   # Tool definitions  
│   │   └── tool_executor.cpp         # Tool registry
│   ├── providers/
│   │   └── anthropic_api_provider.cpp # MAIN BUG: JSON parser
│   └── tools/
│       ├── time_tools.cpp            # get_current_time, etc.
│       └── tui_tools.cpp             # list_windows, create_window
├── wibwob_engine.cpp                 # Tool execution workflow
├── wibwob_view.cpp                   # Chat UI - logs debug info
└── logs/chat_*.log                   # All debug output here
```

### Current Debug Evidence
From `/Users/james/Repos/tvision/test-tui/logs/chat_20250911_155306_413091.log`:

**Line 13**: `{"content":[{"type":"tool_use","id":"toolu_01PSggvEeeAQW4i1zHAz8yYg","name":"get_current_time","input":{}}]}`

**Line 15**: `PARSE_DEBUG: No tool_use pattern found`

## Root Cause Analysis

### Primary Issue: JSON Pattern Matching
**File**: `/Users/james/Repos/tvision/test-tui/llm/providers/anthropic_api_provider.cpp:503`

**Broken code**:
```cpp
size_t toolUsePos = response.find("\"type\": \"tool_use\"");  // WRONG: has spaces
```

**Actual JSON**:
```json
{"type":"tool_use"}  // NO SPACES
```

### Secondary Issues
1. **Content array search** may also have spacing issues
2. **JSON block boundary detection** using brace counting may be flawed  
3. **Tool result formatting** for follow-up requests not tested

## Technical Requirements

### 1. Fix Search Patterns
Replace all JSON search patterns to handle both spaced and non-spaced variants:
- `"type": "tool_use"` → `"type":"tool_use"` or use regex
- `"content": [` → `"content":[` or flexible matching
- All field extractions: `"id": "` → `"id":"`

### 2. Improve JSON Parsing  
Current brace-counting logic is fragile. Consider:
- Find complete content array bounds first
- Parse each array element separately  
- Use more robust JSON structure detection

### 3. Tool Execution Flow
Verify the complete workflow in `/Users/james/Repos/tvision/test-tui/wibwob_engine.cpp:59-82`:
- Tool extraction sets `needs_tool_execution = true`
- Tool executor runs and gets results
- Follow-up request sent with proper tool_result format
- Final response displayed in chat

## Implementation Plan

### Phase 1: Fix Pattern Matching (Critical)
```cpp
// Replace in anthropic_api_provider.cpp around line 503:
size_t toolUsePos = response.find("\"type\":\"tool_use\"");  // Remove spaces
```

### Phase 2: Verify Tool Result Format
Follow Anthropic API spec for tool result messages:
```json
{
  "role": "user", 
  "content": [
    {
      "type": "tool_result",
      "tool_use_id": "toolu_123",
      "content": "15:30:45"
    }
  ]
}
```

### Phase 3: End-to-End Testing
Test cases:
1. **Single tool**: "What time is it?" → actual time displayed
2. **Multiple tools**: "Time and list windows" → both executed  
3. **TUI tools**: "Create window" → actual window appears on screen
4. **Error handling**: Invalid tool calls handled gracefully

## Reference Documentation

### Anthropic API Tool Use Docs
- **Overview**: https://docs.anthropic.com/en/docs/agents-and-tools/tool-use/overview
- **Implementation**: https://docs.anthropic.com/en/docs/agents-and-tools/tool-use/implement-tool-use  
- **Message format**: https://docs.anthropic.com/en/api/messages

### Key API Patterns
1. **Tool request**: `{"tools": [...], "messages": [...]}`
2. **Tool response**: `{"stop_reason": "tool_use", "content": [{"type": "tool_use", ...}]}`
3. **Tool results**: `{"role": "user", "content": [{"type": "tool_result", ...}]}`
4. **Continue conversation**: Send tool results back to get final response

## Success Criteria

### Functional Requirements
- [ ] Parser extracts tool calls from `stop_reason: "tool_use"` responses
- [ ] Tool executor runs and returns results (time, window list, etc.)
- [ ] Follow-up requests continue conversation automatically
- [ ] Chat shows final results, not just "I'll use the tool"

### Debug Requirements  
- [ ] All debug info logged to chat files (no separate stderr logs)
- [ ] Clear indication when tools are found vs. not found
- [ ] Tool execution results logged with success/failure status

### Performance Requirements
- [ ] Tool execution happens within 3 seconds of API response
- [ ] No hanging conversations or infinite loops
- [ ] Clean error messages for failed tool calls

## Test Validation

**Simple Test**: Ask "What time is it?" in chat
- ✅ Should show actual time (e.g., "15:30:45") 
- ❌ Currently shows: "Temporal flux! Chronological cascade incoming..."

**Complex Test**: Ask "Create a window and list all windows"
- ✅ Should create visible window + show window list
- ❌ Currently shows: "I'll help you..." then stops

The fix is straightforward but critical - JSON pattern matching needs to handle the actual Anthropic API response format without spaces.