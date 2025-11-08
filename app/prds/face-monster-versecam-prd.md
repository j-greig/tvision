# PRD: Face Monster (Webcam → Emoji/ASCII) for test-tui

## TL;DR
- Add a new TUI window that renders a live “emoji monster” view of the webcam, evolving like Verse patterns but driven by camera frames. Prioritize smooth, cross‑platform operation and correct emoji width handling.
- Start with an out‑of‑process webcam worker (Python + OpenCV) that streams compact frame data to the TUI via a Unix socket/STDIN pipe. Render frames in Turbo Vision with a width‑aware emitter (emoji count as 2 cells) and Verse‑style glyph mapping.
- Later, optionally ship an in‑process C++ OpenCV backend for fewer dependencies.

---

## Problem / Goal
We want a new menu item that opens a “Face Monster” window: a live, evolving, smooth TUI visualization of the webcam that “monster‑fies” faces using emoji and ASCII, similar to Verse’s flow/feel but camera‑driven. It must:
- Run smoothly (8–15 FPS target) without flicker.
- Handle emoji (2‑cell width) correctly in centering and placement.
- Degrade gracefully when no camera exists.
- Be portable (macOS/Linux initially; Windows later).

Non‑goals (MVP)
- Pro‑grade face tracking/landmarks. Basic face/brightness mapping is enough.
- Fancy ML blending. Keep the worker simple (OpenCV functions only).

## Constraints
- Terminal Unicode widths vary; rely on TVision’s `moveCStr` and `strwidth` to emit glyphs at cell granularity.
- Network may be blocked; prefer local IPC/pipe.
- Python/OpenCV availability varies; keep a fallback mode (synthetic feed).
- test-tui already uses an IPC Unix socket for TUI tools (`/tmp/test_pattern_app.sock`).

## Approaches (Ranked)
1) Out‑of‑process webcam worker over IPC (RECOMMENDED)
   - Python + OpenCV script captures frames, downsamples, converts to a compact format (e.g., luminance grid + optional face bbox), and streams to TUI over a local Unix socket or STDIN pipe.
   - Pros: quick to build, isolates camera deps, easy iteration, no CMake/OpenCV pain.
   - Cons: Python runtime dependency.

2) In‑process C++ OpenCV backend
   - Add OpenCV to CMake, capture in a background thread, push frames to the view.
   - Pros: single binary, best control.
   - Cons: heavier integration, platform quirks.

3) ffmpeg pipe
   - Spawn ffmpeg to capture and output raw/PGM/ASCII; parse in TUI.
   - Pros: ubiquitous.
   - Cons: parsing quirks, platform flags vary.

4) Platform APIs (AVFoundation on macOS, v4l2 on Linux)
   - Pros: no Python.
   - Cons: multiple codepaths, higher effort.

Recommendation: Start with #1 for speed and stability, keep #2 as a follow‑up option.

## Architecture
- `face_worker.py` (Python):
  - OpenCV `VideoCapture(0)` at low res (e.g., 80×45) and 8–12 FPS.
  - Process frame (gray), optional simple face detect (Haar cascade) to annotate a bbox.
  - Serialize a compact message: header `{w,h,timestamp,has_face,bbox}` + payload `w*h` bytes of luminance.
  - Send via a Unix domain socket `/tmp/face_monster_cam.sock` or STDIN pipe.

- TUI side (C++):
  - MonsterCam window (new view): connects to socket (non‑blocking), reads latest full frame, drops older frames.
  - Glyph mapping per Verse style: whitespace → punctuation → geometric → emoji (heavier weight), tinted by camera luminance and smoothed with a small fbm overlay.
  - Width‑aware emitter: per row, emit glyphs at a running column using `moveCStr`, advancing by written cells (emoji=2).
  - Optional overlays: eyes band at detected face center.

## Data Format
- Header (ASCII JSON on one line) + newline + raw payload bytes.
  - Example: `{ "w":80, "h":45, "ts":1694450000, "has_face":true, "bbox":[x,y,w,h] }\n` then 3600 bytes.
- Simpler alternative: all-ASCII grid using ` .,:;ox%#@` chars (no payload), but raw bytes are faster and robust.

## Rendering Details
- Per cell (target TUI grid smaller than camera, e.g., W×H view → sample/average camera block):
  - Compute luminance L 0..1 and a smoothed Verse field V 0..1.
  - Choose glyph class by `(α*L + β*V)`; default α=0.7, β=0.3.
  - Emit emoji more often (bias) and preserve negative space.
- Eyes overlay: if `has_face`, compute a tile center (cx, cy) from bbox; render “👁️  ═══  👁️” centered using measured width (7 cells) at cy.
- Use a calm background and write full width per row to avoid artifacts.

## UX
- Menu: View → Monster Cam (Emoji)
- Controls:
  - Space: pause/resume streaming
  - +/-: adjust frame cadence
  - j/J: emoji bias up/down
  - w/W: whitespace bias up/down
  - x: toggle emoji “flood” (for fun)
  - f: toggle face overlays
  - r: reset smoothing buffers

## Risks
- No webcam / access denied: show a moving synthetic pattern (Perlin + emoji) and surface a status in the window.
- Socket hiccups: use non‑blocking reads, drop partial frames, always display the latest complete frame.
- Terminal emoji width: rely on TVision’s width helpers; avoid string concat; always emit with `moveCStr`.

## Milestones
1) Frame plumbing MVP (no camera): render synthetic frames with width‑aware emitter.
2) Python worker + socket: ingest real frames; simple luminance mapping.
3) Emoji/Verse mix + smoothing; face overlay.
4) Controls, menu wiring, docs.
5) Optional C++ OpenCV provider.

## Step‑by‑Step Plan
1) Add new view/window: `generative_monster_cam_view.{h,cpp}`
   - Column‑accurate emitter (reuse Monster Verse pattern).
   - Synthetic frame generator (fbm) when no data.
   - Menu item `cmMonsterCam` under View.

2) IPC client in view
   - Connect non‑blocking to `/tmp/face_monster_cam.sock`.
   - Read header+payload into a ring buffer, validate sizes.
   - Drop partials, use last complete frame.

3) Glyph mapper
   - Map luminance to class with Verse smoothing; implement emoji/geom/punct thresholds and biases.
   - Implement face overlay path (eyes band by bbox if enabled).

4) Controls
   - Space, +/-/jJ/wW/x/f/r as above; expose defaults in code.

5) Python worker (tools/face_worker.py)
   - Capture webcam (OpenCV), grayscale, resize, simple face detect (Haar), serialize and send.
   - Ship a README on how to run it.

6) Docs
   - Update README/menu reference; known issues (permissions, macOS camera prompt).

7) Optional: In‑process C++ OpenCV backend
   - If we want a single‑binary solution.

## Validation
- With worker running, toggle overlays and biases; verify emoji density, smooth motion, and correct eyes centering.
- Kill worker: window falls back to synthetic feed, no crash; status shows “no camera”.
- Resize window: content fills and re-centers correctly.

