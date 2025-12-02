# SDK Streaming Fixes - 2025-12-02

## Key changes this session

| File | Change | Why Critical |
|------|--------|--------------|
| `app/llm/providers/claude_code_sdk_provider.cpp:29-48` | Added `escapeJsonString()` | Multi-line system prompt was breaking JSON across lines, bridge rejected as "non-JSON" |
| `app/llm/providers/claude_code_sdk_provider.cpp:284-292` | Uses escaped prompt + configuredModel | Without escaping, `\n` in prompt = JSON split = session never starts |
| `app/llm/providers/claude_code_sdk_provider.cpp:345-357` | Auto-start session in `sendStreamingQuery()` | Was failing silently because session not active on first query |
| `app/llm/providers/claude_code_sdk_provider.cpp:577-586` | Parse model from config | Was hardcoded "sonnet", ignored `claude-haiku-4-5` in config |
| `app/llm/sdk_bridge/claude_sdk_bridge.js:208-231` | Extract text from content array | SDK returns `[{type:'text', text:'...'}]` not string - was concatenating `[object Object]` |
| `app/llm/sdk_bridge/claude_sdk_bridge.js:252-258` | Handle `result.subtype === 'success'` | Final response text is in `message.result`, not assistant message - was getting 0 content |
| `app/llm/sdk_bridge/claude_sdk_bridge.js:191-212` | Conditional MCP server passing | Null mcpServer was potentially crashing SDK before any response |
| `app/wibwob_view.cpp:723` | Pass system prompt to streaming query | SDK needs prompt to start session, wasn't being passed |
| `app/wibwob_engine.h:42` | Added `getSystemPrompt()` getter | View needed access to prompt for SDK |

## The cascade failure was

1. Unescaped `\n` in prompt → JSON split across lines
2. Bridge rejects split JSON → Session never starts
3. `sendStreamingQuery` returns false (no session)
4. Falls back to regular query → also fails
5. Empty response shown → crash on second message

## Debug logs to verify fix

Should see these in stderr:
```
[SDK] Configured model: haiku (from claude-haiku-4-5)
[SDK] Escaped prompt: 4399 chars -> 4483 chars
[SDK] Session auto-started OK
[BRIDGE] Query options: {...}
[BRIDGE] Result message: success result: ...
```

## Test command

```bash
./build/app/test_pattern 2> /tmp/sdk_debug.log
# Open Wib&Wob Chat, send message
# Check: tail -f /tmp/sdk_debug.log
```

---

## Session 2 - Agent SDK Migration (2025-12-02)

### Changes for `@anthropic-ai/claude-agent-sdk` v0.1.56

| File | Change | Why |
|------|--------|-----|
| `app/llm/sdk_bridge/claude_sdk_bridge.js:19` | `systemPrompt` replaces `customSystemPrompt` | Agent SDK uses `systemPrompt` option name |
| `app/llm/sdk_bridge/claude_sdk_bridge.js:18` | Added `sdkSessionId` tracking | Store SDK session_id for multi-turn resume |
| `app/llm/sdk_bridge/claude_sdk_bridge.js:210-213` | Added `resume` option with `sdkSessionId` | Enable multi-message conversation continuity |
| `app/llm/sdk_bridge/claude_sdk_bridge.js:244-256` | Handle `partial_assistant` message type | `SDKPartialAssistantMessage` provides streaming deltas |
| `app/llm/sdk_bridge/claude_sdk_bridge.js:303-306` | Capture `session_id` from result | Store for resume on subsequent queries |

### Message types handled

Per Agent SDK docs:
- `partial_assistant` - Real-time streaming deltas (when `includePartialMessages: true`)
- `assistant` - Complete assistant message (may duplicate partials)
- `stream_event` - Legacy/fallback streaming events
- `result` - Final result with `session_id` for resume

### Multi-turn conversation flow

1. First query: No `resume` option, SDK creates new session
2. SDK returns `SDKResultMessage` with `session_id`
3. Bridge stores `session_id` in `this.sdkSessionId`
4. Second query: Bridge passes `resume: sdkSessionId`
5. SDK continues from previous context

### Test multi-message

```bash
./build/app/test_pattern 2> /tmp/sdk_debug.log
# Open Wib&Wob Chat
# Send: "hello"
# Wait for response
# Send: "what did I just say?"
# Should reference previous message (resume working)

# Check logs for:
grep "Captured SDK session_id" /tmp/sdk_debug.log
grep "Resuming session" /tmp/sdk_debug.log
```
