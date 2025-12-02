# SDK Agent Migration Changelog (Node Bridge)

Tracking the move from `@anthropic-ai/claude-code` to `@anthropic-ai/claude-agent-sdk` for the Wib&Wob chat bridge. Items are marked ✅ when done, ⬜️ when pending.

## Checklist
- ✅ Add agent SDK dependency (legacy SDK removed) and dynamic loader
- ✅ Update bridge to handle `stream_event` deltas + string prompt path, log source SDK
- ✅ Improve C++ provider parsing to use fullResponse and unescape content
- ⬜️ Install new dependency (`npm install` in `app/llm/sdk_bridge`) and smoke test with MCP on/off
- ⬜️ Align bridge input format with official messages array if the agent SDK requires it (currently using string prompt + customSystemPrompt)
- ⬜️ Add handling for any additional agent SDK stream event types encountered in testing
- ⬜️ Drop legacy SDK once agent SDK is verified stable

## Notes
- Loader: uses `@anthropic-ai/claude-agent-sdk` and errors clearly if not present.
- Bridge now opts into `includePartialMessages` and listens for `content_block_delta` / `message_delta` text.
- C++ side now propagates `fullResponse` from the bridge’s `MESSAGE_COMPLETE`, so “result-only” completions render.*** End Patch
