# PRD: Claude Code SDK Bridge v2 for Turbo Vision TUI

## Overview

- Replace the multi-provider LLM abstraction with a single, robust Claude Code SDK integration using a long‑lived Node.js bridge and a minimal C++ interface.
- Eliminate brittle config parsing and failure paths while delivering world‑class reliability, streaming, and tool interop for the Wib&Wob chat window in the `test-tui` app.

## Goals

- Simplify: Remove provider factory, API-based backends, and ad-hoc JSON parsing.
- Reliability: One long-lived Node bridge process, resilient restarts, graceful degradation.
- Streaming: Low-latency content deltas and clean completion semantics.
- Tools: Map TUI-native tools to the SDK’s allowed tools and support custom tool results roundtrips.
- UX: Never abort the TUI; show clear in-app status when SDK is unavailable.
- DX: One clear TypeScript package (`llm/sdk_bridge`), clear C++ interface (`ClaudeBridge`), minimal configuration.

## Non-Goals

- No alternative LLM providers in this path.
- No interactive Claude UI; only headless SDK use.
- No separate HTTP API clients (Anthropic/OpenRouter) in this bridge.

## Users

- TUI users interacting with Wib&Wob chat inside `test-tui`.
- Developers running and extending the TUI and its LLM features.

## Functional Requirements

- Start/stop a Claude session with an optional system prompt.
- Send a query and receive streaming content deltas and a completion event.
- Cancel an in-flight request.
- Update system prompt mid-session.
- Optional tool execution: forward tool_use events to C++ for native execution and send results back.
- Expose SDK version and health to the TUI.
- Persist nothing sensitive; no API keys required for SDK path.

## Architecture

- C++ `ClaudeBridge` (in-process) spawns a Node child process running `llm/sdk_bridge/claude_sdk_bridge.ts`.
- IPC: newline-delimited JSON (NDJSON) over stdio with strict framing, schema validation, and backpressure controls.
- Single active streaming request at a time; queue or reject with clear error.
- Heartbeat and auto-restart of Node process with exponential backoff and jitter.
- `WibWobEngine` calls `ClaudeBridge` directly; provider factory and other providers are removed.

## IPC Protocol

- Transport: NDJSON. Each line is a JSON object with `type` and `data`.
- Commands (C++ → Node):
  - `START_SESSION`: `{ customSystemPrompt?, maxTurns?, allowedTools?, disallowedTools?, sessionTimeout? }`
  - `SEND_QUERY`: `{ query, attachments? }`
  - `UPDATE_PROMPT`: `{ customSystemPrompt }`
  - `END_SESSION`: `{}`
  - `PING`: `{ id }`
  - `SEND_TOOL_RESULTS`: `{ results: [{ tool_use_id, content, is_error? }] }`
  - `GET_VERSION`: `{}`
- Events (Node → C++):
  - `SESSION_STARTED`: `{ sessionId, sdkVersion }`
  - `CONTENT_DELTA`: `{ text }`
  - `MESSAGE_COMPLETE`: `{ finishReason, usage: { inputTokens, outputTokens } }`
  - `ERROR`: `{ code, message, details? }`
  - `TOOL_USE`: `{ id, name, input }` (optional, if surfaced by SDK)
  - `PONG`: `{ id }`
  - `LOG`: `{ level, message }`
- Constraints:
  - Max frame size: 256 KB; oversize frames emit `ERROR{ code: "FRAME_TOO_LARGE" }`.
  - JSON is UTF-8; messages must be one line; any newline in text is escaped.

### IPC Examples

Command (C++ → Node):

```json
{"type":"START_SESSION","data":{"customSystemPrompt":"You are Wib&Wob","maxTurns":50,"allowedTools":["Read","Write","Grep","Bash","LS"],"sessionTimeout":3600}}
```

Event (Node → C++):

```json
{"type":"SESSION_STARTED","data":{"sessionId":"abc123","sdkVersion":"1.0.113"}}
{"type":"CONTENT_DELTA","data":{"text":"Hello"}}
{"type":"CONTENT_DELTA","data":{"text":", world"}}
{"type":"MESSAGE_COMPLETE","data":{"finishReason":"stop","usage":{"inputTokens":57,"outputTokens":42}}}
```

## TypeScript SDK Bridge

- Location: `test-tui/llm/sdk_bridge/`
- Structure:
  - `src/claude_sdk_bridge.ts`: main; reads stdin, writes stdout, runs SDK.
  - `src/ipc.ts`: schema validation (prefer Zod) and framing utilities.
  - `src/session.ts`: session lifecycle, query streaming, heartbeat monitor.
  - `package.json`: `@anthropic-ai/claude-code` pinned; scripts: `build`, `start`, `lint`.
- Behavior:
  - On `START_SESSION`, create/reuse a session, return `SESSION_STARTED`.
  - On `SEND_QUERY`, stream partials (`CONTENT_DELTA`) and end with `MESSAGE_COMPLETE`.
  - On `UPDATE_PROMPT`, update prompt live if supported; else restart transparently.
  - On SDK error, emit `ERROR` and exit; C++ will restart with backoff.
  - Heartbeat: respond to `PING` with `PONG` within ~100ms; log delays > 500ms.
- Logging:
  - Controlled by `BRIDGE_DEBUG=1` (env). Emitted as `LOG` events to C++ and to stderr in dev.

## C++ Integration Layer

- Files: `test-tui/llm/bridge/claude_bridge.h/.cpp`
- API:
  - `bool startSession(const std::string& systemPrompt, const Config& cfg)`
  - `bool sendQuery(const std::string& text, StreamCb onDelta, DoneCb onDone, ErrCb onError)`
  - `bool updateSystemPrompt(const std::string& systemPrompt)`
  - `void endSession()`
  - `bool isAvailable() const`
  - `bool isBusy() const`
  - `void cancel()`
  - `std::string getVersion() const`
  - `std::string lastError() const`
- Process management:
  - Spawn `node` with absolute path to built JS: `node dist/claude_sdk_bridge.js`.
  - Non-blocking pipes, dedicated read thread, bounded queue to the main thread.
  - Backoff on restart: 100ms → 3s with jitter, cap at 5 retries in 60s.
- Error model:
  - No throws across public API; failures return false and set `lastError`.
  - Node-side `ERROR` events always surface via callbacks.

## Configuration

- File: `test-tui/llm/config/claude_bridge.json`
- Keys:
  - `nodeScriptPath` (default: `llm/sdk_bridge/dist/claude_sdk_bridge.js`)
  - `allowedTools` (array of strings)
  - `disallowedTools` (array)
  - `maxTurns` (int, default 50)
  - `sessionTimeout` (seconds, default 3600)
  - `customSystemPrompt` (optional)
- Parsing:
  - Use minimal robust JSON parsing; default on failure.
  - No `.env` requirement for SDK-only path.

## Tool Support

- Map TUI tools to SDK allowed tools via `allowedTools`.
- If SDK surfaces `TOOL_USE`, forward to C++; execute via ToolRegistry and return via `SEND_TOOL_RESULTS`.
- If SDK does not surface tool_use, keep C++ tool execution disabled by default; optional compile-time flag to enable experimental pass-through.

## Security & Privacy

- No API keys stored or required for SDK path.
- Sanitize and cap payload sizes; reject binary data.
- Node child runs with inherited user privileges; no elevated perms.
- Avoid writing logs with user content by default; explicit opt-in for dev.

## Performance

- Latency target: first token < 300ms after `SEND_QUERY`.
- Throughput: stable streaming at > 10 events/sec.
- Memory: bridge < 100MB RSS; Node process reused across queries.
- Backpressure: if C++ callbacks are slow, buffer up to 1MB, then pause reads; emit `ERROR{ code: "BACKPRESSURE" }` if persistent.

## Observability

- Structured logs (timestamps, categories) in Node and C++.
- Health via `GET_VERSION` and periodic `PING/PONG`.
- In TUI, show status: “Claude: Ready / Busy / Unavailable”.

## Repo & Build Changes

- Remove: `test-tui/llm/base/*`, `test-tui/llm/providers/*`, and their includes in `CMakeLists.txt`.
- Add: `test-tui/llm/bridge/*` sources to build.
- Node package:
  - `cd test-tui/llm/sdk_bridge && npm ci && npm run build`
  - Pre-run check: verify `dist/claude_sdk_bridge.js` exists; otherwise show guidance in-app.
- CMake: optional custom target `sdk_bridge_build` to run `npm ci && npm run build` (opt-in to avoid network during normal builds).

## Developer Experience

- Build steps:
  - `cd test-tui && cmake -S . -B build && cmake --build build`
  - `cd test-tui/llm/sdk_bridge && npm ci && npm run build`
  - `./build/test_pattern`
- Debug flags:
  - `BRIDGE_DEBUG=1` (Node logs)
  - `TV_LLMBRIDGE_DEBUG=1` (C++ logs)
- Documentation updates in `CLAUDE.md` to reflect new headless bridge usage.

## Testing Strategy

- Unit (TS): IPC validator, session lifecycle, streaming emitter, error codes.
- Unit (C++): frame parser, backoff logic, cancel/restart using a fake echo child.
- Integration: real Node bridge with a short prompt; assert session start, at least one delta, completion.
- Chaos: kill Node mid-stream; assert restart and clear error surfaced.
- Performance: measure time to first delta and sustained delta cadence.

## Migration Plan

1. Implement `ClaudeBridge` alongside existing code; behind flag `TV_USE_CLAUDE_BRIDGE=1`.
2. Switch `WibWobEngine` to call `ClaudeBridge`; leave providers unused.
3. Remove provider code and configuration files; update docs and examples.
4. Clean up CMake and remove dead includes.

## Rollout Plan

- Ship behind flag for internal testing for 1–2 days.
- Collect logs and user feedback; fix stability issues.
- Make default; keep fallback to “LLM disabled” with clear guidance if SDK unavailable.

## Risks & Mitigations

- Node not installed:
  - Detect at startup; show actionable message (`brew install node` or download link).
- SDK version changes:
  - Pin version in `package.json`; add compatibility check and log on startup.
- IPC framing bugs:
  - Strict schema validation and robust parser; fuzz test inputs.
- Tooling mismatch:
  - If SDK tool model diverges, keep tools on C++ side and treat SDK as pure chat for v1.

## Success Metrics

- Zero process aborts from `stoi` or config parsing errors.
- 99% of queries complete without bridge restart.
- Time to first token < 300ms median.
- No “interactive UI conflict” incidents reported.

## Open Questions

- Do we forward `tool_use` from SDK or keep tool execution solely in C++ for v1?
- Should we auto-build the Node bridge from CMake, or keep it a separate step to avoid network builds?
- Do we need Windows support imminently? If yes, verify stdio/pipe handling and Node path resolution on Windows.

