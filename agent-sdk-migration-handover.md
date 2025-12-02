# Agent SDK Migration Handover (Wib&Wob Chat)

This note captures the current state of the Agent SDK migration and what remains to finish streaming/MCP for the Wib&Wob chat in `/app`. It is intended for a fresh LLM handoff with no prior context.

## Repo Context
- Root: `/Users/james/Repos/tvision-develop`
- TUI app lives in `app/`
- Wib&Wob chat uses a Node bridge (`app/llm/sdk_bridge/`) and C++ provider (`app/llm/providers/claude_code_sdk_provider.*`).
- Build/run: `cmake` in root, app executable at `./build/app/test_pattern`.
- Testing command used: `./build/app/test_pattern 2> /tmp/sdk_debug.log`

## Current State (After Partial Migration)
- Legacy SDK removed: `@anthropic-ai/claude-code` is no longer in `package.json`; loader now uses `@anthropic-ai/claude-agent-sdk`.
- Dynamic loader: `app/llm/sdk_bridge/sdk_loader.js` loads agent SDK and exposes `query` + source.
- Bridge: `app/llm/sdk_bridge/claude_sdk_bridge.js`
  - Uses agent SDK `query` but still sends `prompt: data.query` (string) instead of `messages` array.
  - Handles `partial_assistant`, `assistant`, and a fallback `stream_event` (content_block_delta/message_delta). Ignores other stream events.
  - Tracks `sdkSessionId` from `result` messages and passes `resume` to query options.
  - Accepts both `systemPrompt` and `customSystemPrompt` inputs.
  - MCP server passed as an in-proc instance (from `mcp_tools.js`) in `mcpServers`.
  - Uses short model aliases (sonnet/haiku) instead of full IDs.
- MCP tools: `app/llm/sdk_bridge/mcp_tools.js` defines tools via `createSdkMcpServer` and `tool` (agent SDK) calling FastAPI at `http://127.0.0.1:8089`.
- C++ provider: `app/llm/providers/claude_code_sdk_provider.cpp`
  - Manages Node bridge process; has streaming thread.
  - Recent fixes: join/reset processing thread before starting a new one to avoid `std::terminate`; lightweight JSON parsing now unescapes and uses `fullResponse` from `MESSAGE_COMPLETE`.
  - Still uses string scanning, not a real JSON parser.
- Config: `app/llm/config/llm_config.json` sets `activeProvider: "claude_code_sdk"`, model `claude-haiku-4-5`, script path `app/llm/sdk_bridge/claude_sdk_bridge.js`.
- Logs: chat logs in `logs/chat_*.log`; debug in `/tmp/sdk_debug.log`.
- Lockfile: `app/llm/sdk_bridge/package-lock.json` removed `claude-code` but needs regeneration after `npm install` to include agent SDK.

## Outstanding Issues (per Agent SDK docs + MCP docs)
1. **Message shape**: Agent SDK expects `messages` array with content blocks (system/user). Bridge still uses `prompt: <string>`.
2. **Stream events**: Only handles `partial_assistant`, `assistant`, and limited `stream_event` deltas. Other Agent SDK events (e.g., `content_block_start`, `message_stop`) are ignored, which may drop text or completions.
3. **MCP config**: Passing an in-proc SDK server instance; the guide suggests using MCP server configs (stdio/http/sse/sdk). If in-proc is unstable, switch to HTTP/SSE pointing at FastAPI MCP endpoint, or validate SDK server config is acceptable.
4. **Options/models**: Uses short aliases (`sonnet`, `haiku`). Agent SDK may require full model ids; also need to audit option names (permission modes, max turns, etc.).
5. **Lockfile**: Needs `npm install` in `app/llm/sdk_bridge` to record agent SDK in `package-lock.json`.
6. **C++ parsing**: Still uses string scans for JSON; could switch to a proper parser to avoid truncated escapes.

## Key Files
- Bridge JS: `app/llm/sdk_bridge/claude_sdk_bridge.js`
- MCP tools: `app/llm/sdk_bridge/mcp_tools.js`
- SDK loader: `app/llm/sdk_bridge/sdk_loader.js`
- Package manifest: `app/llm/sdk_bridge/package.json` (agent SDK only)
- Package lock: `app/llm/sdk_bridge/package-lock.json` (regen)
- C++ provider: `app/llm/providers/claude_code_sdk_provider.cpp` / `.h`
- LLM config: `app/llm/config/llm_config.json`
- TUI chat window: `app/wibwob_view.cpp` (uses provider)
- Agent SDK docs: MCP https://platform.claude.com/docs/en/agent-sdk/mcp.md; streaming/messages https://platform.claude.com/docs/en/build-with-claude/working-with-messages.md; migration guide https://platform.claude.com/docs/en/agent-sdk/migration-guide.md

## Known Crash Causes (addressed)
- Syntax error in bridge previously dumped control sequences; fixed.
- `std::terminate` when destroying a joinable thread; fixed by joining before start in C++ provider.

## Proposed Completion Steps
1. Switch bridge to the Agent SDK “messages” API:
   - Build `messages` array with content blocks; put system prompt per docs.
   - Stop using `prompt: <string>`.
2. Expand stream event handling:
   - Handle all text-carrying events (`partial_assistant`, `assistant`, `content_block_delta`, `message_delta`, and any other stream events listed in the guide).
   - Ensure `fullResponse` aggregates correctly and MESSAGE_COMPLETE returns content even if only deltas or result arrive.
3. MCP wiring:
   - Decide on MCP transport per guide. If required, pass HTTP/SSE MCP server config instead of the in-proc `createSdkMcpServer` instance; otherwise validate stability.
4. Options/models:
   - Map model names to full ids expected by agent SDK.
   - Align option names (max turns, permission modes, etc.) with the agent SDK.
5. Regenerate lockfile:
   - `cd app/llm/sdk_bridge && npm install` to capture `@anthropic-ai/claude-agent-sdk`.
6. (Optional) Improve C++ JSON parsing to reduce fragility.
7. Test:
   - `node -c app/llm/sdk_bridge/claude_sdk_bridge.js`
   - `./build/app/test_pattern 2> /tmp/sdk_debug.log`
   - Open Wib&Wob Chat, send a prompt, verify streaming chunks and completion with MCP on/off.

## Snippets (Current Bridge Behavior)
- Query options (approx lines ~200):
  ```js
  const queryOptions = {
    systemPrompt: this.systemPrompt,
    maxTurns: this.sessionConfig.maxTurns,
    allowedTools: this.mcpServer ? allAllowedTools : this.sessionConfig.allowedTools,
    model: this.sessionConfig.model || 'haiku',
    includePartialMessages: true,
    resume: this.sdkSessionId, // if set
    mcpServers: { "tui-control": this.mcpServer } // if created
  };
  ```
- Stream handling:
  - `partial_assistant`: delta text via `message.delta?.text`
  - `assistant`: content array to text; only sent if `!fullResponse`
  - `stream_event`: fallback for `content_block_delta`/`message_delta`
  - `result`: captures `session_id` for resume; uses `fullResponse` if empty
