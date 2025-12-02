# PRD: One‑Click SVG Export of Current Screen (test-tui)

## Summary

Add a first-class, one-click “Export to SVG” capability to the test-tui apps, producing a faithful, single-file SVG snapshot of the current Turbo Vision screen. The exporter reads TVision’s `TScreen::screenBuffer` to preserve characters, colors, and styles (bold/italic/underline), including Unicode, double-width, and combining characters. Users access it via a new menu item and hotkey; output lands in `build/snapshots/` with timestamped filenames.

## Goals

- Accurate, deterministic SVG snapshot of the current screen (no recording).
- Preserve text, colors, and common styles from `TColorAttr`.
- Handle UTF‑8, combining marks, and double‑width characters.
- Simple UX: menu item + hotkey; sensible default file path/name.
- No external runtime dependencies.

## Non‑Goals

- Animated SVG (multi-frame) — out of scope.
- Terminal recorder integration (asciinema/termtosvg) — not in this PR.
- Pixel-perfect per-terminal font rendering — we rely on a monospace font and cell metrics.

## User Stories

- As a developer, I press a hotkey and get `…/build/snapshots/screen-YYYYMMDD-HHMMSS.svg` capturing exactly what I see.
- As a designer, I export a UI state to embed in docs without rasterization.
- As a tester, I attach an SVG artifact to bug reports to show precise TUI output.

## UX & IA

- Menu: `File -> Export -> Save SVG…` (new submenu if `Export` doesn’t exist).
- Hotkey: `F12` (default); also `Ctrl+E S` on platforms where function keys are constrained.
- Status line hint: “F12 Save SVG”.
- After save, show a brief modal “Saved: screen-2025-09-15-120455.svg”.

## Affected Apps (initial scope)

- `test-tui/test_pattern_app.cpp` (primary playground)
- `test-tui/simple_tui.cpp` (reference app)
- Optional follow-ups: `ansi_viewer_main.cpp`, `frame_file_player_main.cpp`, `paint/paint_app.cpp`

## Architecture

Leverage existing capture path in `test-tui/frame_capture.{h,cpp}` which already snapshots `TScreen::screenBuffer` into `CapturedFrame`. Add an SVG exporter alongside existing PlainText/ANSI/HTML/JSON exporters.

### API Changes

- Enum: `enum class CaptureFormat { PlainText, AnsiEscapes, Html, Json, Clipboard, Svg };`
- New method: `std::string exportAsSvg(const CapturedFrame& frame, const CaptureOptions& options);`
- Helper: `static inline std::string colorToCssHex(TColorRGB);`

### Menu/Command Wiring

- Command ID: `cmExportSvg = 0xF201` (reserve in app scope).
- Menu item: `File -> Export -> Save SVG…\tF12` mapped to `cmExportSvg`.
- Status line item: `F12 Save SVG`.
- Handler in app class: on `cmExportSvg`, call `quickSaveScreen(defaultSvgPath(), CaptureFormat::Svg)` and notify.

## Technical Design

### Data Source

- Use `CapturedFrame` (already built from `TScreen::screenBuffer`).
- For each `TScreenCell`:
  - Text: `cell._ch.getText()` (UTF‑8). Skip wide-char trails via `cell._ch.isWideCharTrail()`.
  - Width: `cell.isWide()` counts as 2 cells for x-advance.
  - Colors: `getFore(attr).asRGB()`, `getBack(attr).asRGB()` from `include/tvision/colors.h`.
  - Style bits: `getStyle(attr)` → bold/italic/underline flags.

### Layout Strategy

- Coordinate system: fixed cell grid.
  - Defaults: `cellW = 9px`, `cellH = 16px`, baseline offset ~ `12px` (tuned by eyeball once).
  - SVG root: `viewBox="0 0 (cols*cellW) (rows*cellH)"`.
- Backgrounds: per row, merge runs of equal background into one `<rect>` to reduce nodes.
- Text: per row, group runs by same fg+style into a single `<text>` with `<tspan>` segments.
  - `font-family='monospace'`, `font-variant-ligatures: none`, `xml:space='preserve'`.
  - Set `x` advances by whole cells; skip drawing for space + default bg when safe (size optimization toggle).

### Styles Mapping

- Bold: `font-weight: bold` if `(style & slBold)`.
- Italic: `font-style: italic` if `(style & slItalic)`.
- Underline: `text-decoration: underline` if `(style & slUnderline)`.
- Reverse: already baked into fg/bg during draw; we export as-is (no terminal reverse at SVG time).

### File Output

- Directory: ensure `build/snapshots/` exists.
- Filename: `screen-YYYYMMDD-HHMMSS.svg` (optionally include app slug, e.g., `testpattern-screen-…`).
- Return value: boolean success; on failure, show error dialog with path.

### Error Handling

- If `TScreen::screenBuffer == nullptr`: abort with message “No screen buffer available”.
- On filesystem error: include `strerror(errno)` in the status dialog.

## Performance & Size

- Complexity ~ O(rows*cols). For 160x50, background rects are O(rows * runs), text nodes O(rows * runs).
- Basic run-length coalescing reduces nodes by ~5–20x vs per-cell.
- Keep SVG under a few MB for typical screens.

## Telemetry / Logging (lightweight)

- Debug log line (optional): “SVG export: 120x40, bg rects=220, text runs=480, 56 ms”.

## Testing Plan

Unit Tests (GoogleTest):

- `test/frame_capture/svg_export.test.cpp`
  - 1) 3x2 grid with ASCII + differing fg/bg → assert SVG contains expected `rect` fills and text content at expected x,y.
  - 2) Double-width char (e.g., '表') followed by trail → ensure only one glyph emitted and x-advance +2.
  - 3) Combining marks (e.g., `म` + diacritics) are emitted as a single UTF‑8 string.
  - 4) Style mapping (bold/italic/underline) translates to attributes.

Manual QA:

- Visual check in browsers (Safari/Chrome/Firefox) for several test screens.
- Dark/bright BG cases; mixed styles; box-drawing; emoji fallback.

## Rollout

- Phase 1: Implement in `test_pattern_app` and `simple_tui`.
- Phase 2: Wire into `ansi_viewer_main` and `frame_file_player_main`.
- Docs: Add a brief note in `test-tui/README` once stabilized.

## Implementation Notes (Code Pointers)

- Extend `test-tui/frame_capture.h`:
  - Add `Svg` to `CaptureFormat`.
  - Declare `exportAsSvg(...)`.
- Extend `test-tui/frame_capture.cpp`:
  - Implement `exportAsSvg`: root `<svg>`, per-row bg rect merging, text grouping, helpers.
  - `colorToCssHex(TColorRGB rgb)`: `#RRGGBB` string.
  - Ensure `captureScreen()` continues to use `TScreen::screenBuffer`.
- Wire menu in apps (example: `TTestPatternApp`):
  - Define `const ushort cmExportSvg = 0xF201;`.
  - In `initMenuBar(TRect r)`: add `~E~xport` submenu with `~S~ave SVG…\tF12`.
  - In `initStatusLine`: add `TStatusItem("F12 Save SVG", kbF12, cmExportSvg)`.
  - In event handler: on `cmExportSvg` → `quickSaveScreen(defaultSvgPath(), CaptureFormat::Svg)` and show result dialog.

## Risks & Mitigations

- Font metrics variance → choose conservative `cellW/cellH`; allow future configuration if needed.
- Large SVG sizes on very wide screens → run-length merging and skip trivial spaces.
- Rare emoji/ZWJ clusters → TVision omits ZWJ; exporter mirrors that for consistency.

## Acceptance Criteria

- New menu item + F12 produces a valid SVG file under `build/snapshots/` without external tools.
- Colors and common styles appear as expected for typical screens.
- Unicode cases (double-width, combining) do not crash and render legibly.

---

## Step Plan & Status

Use this section as the authoritative task list for any contributor picking this up cold.

- [x] Author PRD with goals, UX, APIs, tests (this document)
- [x] Add standalone exporter module (no app wiring)
  - Files: `test-tui/svg_exporter.h`, `test-tui/svg_exporter.cpp`
  - APIs: `screenToSvgString(...)`, `saveCurrentScreenAsSvg(...)`
  - Behavior: Reads `TScreen::screenBuffer`; renders backgrounds + text runs; handles Unicode, styles
- [ ] Decide integration surface: keep standalone module vs. fold into `frame_capture` (recommend fold-in plus keep thin adapter)
- [ ] Extend `test-tui/frame_capture.h/.cpp` with SVG support
  - [ ] Add `Svg` to `CaptureFormat`
  - [ ] Implement `exportAsSvg(const CapturedFrame&, const CaptureOptions&)`
  - [ ] Reuse logic from `svg_exporter.*` or call it internally
- [ ] Unit tests (GoogleTest)
  - [ ] Add `test/frame_capture/svg_export.test.cpp` with cases listed in “Testing Plan”
- [ ] Wire into apps (phase 1)
  - [ ] Reserve command id: `cmExportSvg = 0xF201` (per app scope)
  - [ ] `test-tui/test_pattern_app.cpp`: add menu `File -> Export -> Save SVG…` and status item `F12 Save SVG`
  - [ ] `test-tui/simple_tui.cpp`: same menu and status item
  - [ ] Handler: on `cmExportSvg`, call `quickSaveScreen(defaultSvgPath(), CaptureFormat::Svg)`; show result dialog
- [ ] Defaults & paths
  - [ ] Ensure `build/snapshots/` exists (create if missing)
  - [ ] Filename format: `screen-YYYYMMDD-HHMMSS.svg`
- [ ] Manual QA
  - [ ] Visual check in Safari/Chrome/Firefox; verify colors, styles, Unicode
- [ ] Docs
  - [ ] Add brief usage note to `test-tui/README` after wiring

### Handoff Notes

- Current code compiled units added only: `test-tui/svg_exporter.h/.cpp`. No existing files were modified.
- When integrating, prefer folding the exporter into `frame_capture` to stay consistent with other formats, but keep `svg_exporter.*` as a thin adapter if helpful for tests/tools.
- Avoid UI wiring in parallel branches to prevent conflicts; gate via a single PR once tests pass.
