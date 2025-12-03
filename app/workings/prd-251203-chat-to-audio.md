# TESTING (manual, macOS)
- One-liner: `cmake --build build -j && DEBUG_CLAUDE_AGENT_SDK=1 ./build/app/test_pattern 2> /tmp/sdk_debug.log`
- Ensure API/MCP server is running at `http://127.0.0.1:8089` if MCP tools are needed.
- Send a chat prompt; expect TTS via `say` (Wib=Sandy, Wob=Grandpa). Check `/tmp/sdk_debug.log` for errors.
- MVP v1 status: TTS live with filtered ASCII, sequential Wib/Wob voices; no overlap.

# PRD: Wib&Wob Chat → Audio (macOS TTS)

## Objective
Speak Wib & Wob chat responses on macOS using lightweight TTS (`say`), non-blocking, and TUI/C++ friendly. Do not speak ASCII art. Fixed voices for MVP: Wib → Sandy, Wob → Grandpa.

## Proposed Approach (ranked)
1) **C++ subprocess calling `say`** (recommended): filter text, spawn `say -v <voice> "<text>"` in the background after each assistant message. Minimal deps, fits TUI, non-blocking via a worker thread/subprocess.  
2) AppleScript via `osascript -e 'say ...'`: works but slower/more overhead than `say`.  
3) Node bridge wrapper: extra hop; only if we need JS-side filtering.  

Recommendation: implement a C++ helper that filters text (strip art) and invokes `say` asynchronously.

## MVP v1 Scope
- Trigger: after chat message completes (assistant result) in TUI; call TTS helper with filtered text.
- Voices: Wib=Sandy, Wob=Grandpa (configurable via small config/env).
- Filtering: remove code-fenced/marker blocks (``` or ---), or lines mostly symbols/too long; keep dialogue text.
- Runtime: non-blocking `say`; no batching or audio file output.
- Config: enable/disable, voice map, optional rate.
- Not included: dividers/pauses, SSML, multi-voice beyond Wib/Wob.

## MVP v2 (future)
- Additional voices (Scramble), emotion → voice mapping.
- Smarter markup/SSML, optional audio caching/queueing.
- UI/MCP toggle for TTS enable/disable; output device selection.

## Parking Lot
- Cross-platform TTS, volume/mixing controls, speak summaries when art present.

## Filtering ASCII Art (MVP v1)
- Strip fenced blocks (```…```, or ---…--- if present).
- Drop lines that are mostly non-alphanumeric or very long (e.g., >120 chars).
- Resulting text is what `say` speaks.

## Config Sketch (YAML/JSON)
```
tts:
  enabled: true
  voices:
    wib: "Sandy"
    wob: "Grandpa"
  rate: 200   # optional
  filter_ascii_art: true
```

## Reference: Existing Script Pattern (offline, for future batching)
The old script config/output (not used for MVP v1 live TTS) is kept here for reference (dividers, pauses, voice mapping, offline MP3 generation). Use only if we later add batching/offline rendering.

Example config syntax:
```
@config
wib = Sandy (English (UK))
wob = Grandpa (English (UK))
rate = 200
pause = 0.8
divider_pause = 0.3

@content
wib: ...
wob: ...
```
