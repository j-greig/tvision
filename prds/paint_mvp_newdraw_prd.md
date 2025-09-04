# TVision Paint MVP — PRD (based on NewDraw)

## TL;DR
- Goal: Build a minimal, fast “paint-like” ANSI/ASCII art editor as a standalone TVision app in `test-tui` (new target, not part of `test_pattern`).
- Strategy: Port the core UX from NewDraw (C/ncurses) to TVision (C++), mapping curses I/O to TVision views, events, and menus. Keep scope tight: basic drawing tools, colors, file ops.
- Outcome: A working MVP in days (not weeks), proving TVision’s suitability for editor-style apps and establishing a foundation for future features (Durdraw-level capabilities).

## Background & Ranking Summary

1) NewDraw (Best for MVP)
- Direct C/ncurses implementation; straightforward conceptual mapping to TVision.
- TheDraw-inspired interface; familiar UX; avoids design bikeshedding.
- Minimal deps (just ncurses); clean, small codebase.
- Feature set matches MVP needs (draw, text, lines, box, fill, color pick).

2) Durdraw (Future 2.0 candidate)
- Modern, feature-rich (256 colors, Unicode, animation), clean architecture.
- Python/ncurses; would require full rewrite in C++ → larger scope than MVP.

3) ACiDDraw/TheDraw (Historical reference)
- Assembly/Pascal; DOS-era assumptions; rewrite required. Use for UX patterns only.

4) IcyDraw (Overkill for MVP)
- Rust + egui; desktop oriented; complex formats and layers; far from TVision model.

Repo for inspiration (do not copy blindly):
- NewDraw: https://github.com/richinseattle/newdraw

## Goals (MVP)
- New TVision app in `test-tui`: `paint_tui` (standalone binary).
- Editor canvas with
  - Draw pixels (block/space) and line/box tools
  - Color pick (16 ANSI colors), foreground/background
  - Text tool (insert ASCII/UTF-8 text)
  - Fill tool (flood fill on character/color)
- Basic file ops
  - New, Open, Save (plaintext/ANSI-like; minimal format)
  - Quit confirmation on dirty buffer
- UI
  - Menubar (File/Edit/View/Colors/Tools) with accelerators
  - Status line (coordinates, tool, color)
  - Optional palette bar / tool bar view
- Input
  - Keyboard-driven cursor/brush control
  - Optional mouse drawing (if terminal supports)

## Non-Goals (MVP)
- Animation/SAUCE/complex formats (export/import beyond plain text)
- 256-color/24-bit color; stick to 16 ANSI colors
- Layers, transparency, effects, undo history beyond a simple single-level (optional)
- Cross-app integration; this is a standalone app under `test-tui`

## Technical Approach

### High-level Mapping (ncurses → TVision)
- Screen and render:
  - NewDraw’s curses window → TVision custom `TPaintCanvasView : TView`
  - Use `TDrawBuffer` + `writeLine` for fast row writes; maintain an internal cell buffer `{ch, fg, bg}` per canvas cell
- Input:
  - Curses key/mouse events → TVision `TEvent` (keyboard, mouse)
- Menus & dialogs:
  - TVision `TMenuBar`, `TMenuBox`, `TDialog` for file/color/tool selection
- Colors:
  - `TColorAttr` with BIOS colors (0–15) for FG/BG; bold not required; map to ANSI-like palette

### Data Model
- Canvas buffer: 2D array of `Cell { char32_t ch; uint8_t fg; uint8_t bg; }`
- Tools:
  - Pencil (set ch=█ or configurable brush char)
  - Line (Bresenham-like on discrete grid)
  - Box (h/v lines, corners), filled box (optional)
  - Text insert (place characters, advance cursor)
  - Fill (flood by ch+color or ch-only; bounded by canvas extents)
- Selection (optional for MVP): not required initially

### Views & Components
- `TPaintCanvasView : TView`
  - Owns canvas buffer; handles draw, mouse/keyboard events, scroll offset
  - Supports simple viewport scrolling via arrow/PageUp/Down (or add scrollbars with `TScroller` later)
- `TPaintWindow : TWindow`
  - Hosts canvas view plus optional palette/tool views
- `TPaletteView` (optional): simple 2-line view with 16 FG colors and 16 BG colors
- `TToolView` (optional): shows active tool; keys to toggle

### File Format (MVP)
- Plain text grid or minimal tagged format:
  - Option A: `ch` only (ignores colors on save, simplest)
  - Option B: sidecar palette/attributes per line (simple RLE of color runs)
- Recommendation: Option B minimal RLE for FG/BG runs so colors are preserved

### Color Handling
- Use BIOS 16 colors for FG/BG via `TColorAttr(TColorBIOS fg, TColorBIOS bg)` or via RGB fallback
- Keep palette consistent across views; optionally expose a “Colors” dialog to select FG/BG

## UI/UX (MVP)
- Menubar
  - File: New (Ctrl-N), Open (Ctrl-O), Save (Ctrl-S), Quit (Alt-X)
  - Edit: Clear, (optional Undo)
  - View: Toggle grid (optional), canvas size
  - Colors: FG/BG selection (dialog with 16-color samples)
  - Tools: Pencil, Line, Box, Fill, Text
- Status line
  - Show cursor (x,y), current tool, FG/BG
- Keys (proposal)
  - Arrows: move cursor; Shift+Arrows: draw with pencil
  - 1–5: tool shortcuts (pencil, line, box, fill, text)
  - C: color dialog; Tab: swap FG/BG
  - G: toggle grid (view only)

## Directory & CMake
- `test-tui/paint/` (new)
  - `paint_app.cpp` (main; menus; app scaffolding)
  - `paint_canvas.h/.cpp` (TPaintCanvasView; buffer; tools)
  - `paint_window.h/.cpp` (TPaintWindow; composes views)
  - `paint_file.h/.cpp` (save/load minimal format)
  - `README.md` (usage)
- CMake
  - Add `add_executable(paint_tui …)` and link TVision + ncurses

## Implementation Plan (Checklist)
- [ ] Bootstrap app skeleton (TApplication, menubar/statusline/desktop)
- [ ] Implement `TPaintCanvasView` (buffer draw; writeLine; FG/BG color mapping)
- [ ] Add pencil tool (keyboard + optional mouse drawing)
- [ ] Add line and box tools
- [ ] Add text tool (enter text at cursor; advance)
- [ ] Add fill (4-neighbor flood; match ch+color)
- [ ] Add color dialog (pick FG/BG from 16 colors)
- [ ] Add file ops: New, Open, Save (minimal RLE color runs)
- [ ] Add cursor display + status line info (x,y,tool,FG/BG)
- [ ] Optional: viewport scrolling if canvas > window
- [ ] Smoke test with simple drawings; verify colors; large canvases

## Milestones
1) Day 1–2: App scaffold; canvas view; pencil
2) Day 3–4: Line/box tools; color dialog; simple save/load (text-only acceptable initially)
3) Day 5–6: Fill; text tool; minimal polishing (status line, shortcuts)
4) Day 7: QA pass; docs; ready for demo

## Risks & Mitigations
- Performance: Large canvas redraws
  - Mitigate: Row-wise `TDrawBuffer` writes; dirty-region redraw later if needed
- Input complexity (mouse):
  - MVP keyboard-first; mouse as “nice to have”
- Color fidelity:
  - Stick to 16 colors; map consistently via `TColorAttr`
- File format churn:
  - Start text-only; add simple RLE later; document format

## Future Work (Post-MVP)
- Import/Export ANSI (SAUCE), PNG render (offline), 256/24-bit color mapping
- Undo/redo; selection; copy/paste; brushes; dithering
- Animation/timeline (Durdraw-inspired)
- Workspace persistence (window layout, last opened file)

## NewDraw Mapping Notes (for implementers)
- Replace ncurses init/teardown with TVision `TApplication` lifecycle
- Replace curses getch/mouse with `TEvent` handling in `TPaintCanvasView::handleEvent`
- Replace direct curses drawing with `TDrawBuffer` + `writeLine` inside canvas view’s `draw()`
- Replace curses menus with `TMenuBar`/`TMenuBox`; dialogs with `TDialog`
- Map NewDraw tools to canvas view methods (pencil/line/box/fill/text)

## Integration Plan
- New standalone app: `paint_tui` under `test-tui` (not in `test_pattern`)
- Optional subrepo: include NewDraw as a reference in `/third_party` (no runtime dependency) for code-reading only
- Licenses: Confirm compatibility; do not copy code verbatim; re-implement behavior

## Testing
- Manual tool tests on a 80×25 canvas; verify colors and output
- Save/load roundtrip; draw → save → clear → open; visually identical
- Resize the window; ensure viewport scroll/clip works
- Quick perf check on 160×50 canvas (large draw) — acceptable speed
