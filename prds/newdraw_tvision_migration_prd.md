# NewDraw → TVision Migration PRD (Clean Rebuild with Extracted Algorithms)

## TL;DR
- Yes — reimplementing NewDraw’s app in TVision by extracting its core algorithms (line/box/fill/text, color selection, file flow) is the right approach for a maintainable, native experience.
- We will not “drop-in” ncurses code; instead we’ll rebuild the UI atop TVision (views/menus/dialogs), reuse the drawing algorithms (ported or pseudocoded), and adopt a TV-friendly data model and rendering path.
- MVP target: a TV-native paint app with NewDraw-equivalent core tools, 16-color editing, and minimal file IO, followed by iterative expansion.

## Why Rebuild vs. Shim
- TV-native UI: Menus, dialogs, windows, event loop — cleaner, consistent UX.
- Simpler rendering model: TV draws via TView + TDrawBuffer row flushes; curses’ immediate drawing is an impedance mismatch.
- Fewer dependency hacks: No ncurses emulation; no hybrid event plumbing.
- Long-term maintainability: Algorithms are portable; UI, input, and rendering should be idiomatic TV.

Note: A “shim” is feasible but introduces glue code and keeps curses semantics alive. We prefer a clean TV rebuild with extracted logic.

## Scope (Feature Parity with NewDraw, Phased)

### MVP (Phase 1)
- Canvas: character + 16-color (BIOS) foreground/background per logical pixel.
- Tools: Pencil (block or brush char), Line, Box/Rectangle, Fill, Text insert.
- Colors: 16-color palette selection (FG/BG), swap FG/BG.
- Input: Keyboard-first (arrows, Shift+Arrows draw), basic mouse (click/drag to draw).
- File: New/Open/Save — minimal format (text grid or color-RLE per line).
- UI: Menubar (File/Edit/Colors/Tools/View), Status line with cursor/tool/colors, one window per canvas.
- Pixel modes: Full pixels and Half-vertical pixels (using ▀/▄/█ with FG/BG mix) for denser look.

### Phase 2
- Palette bar + Tool bar views (TV subviews) with hotkeys.
- Undo/Redo (ring buffer or log of ops).
- Import/Export: ANSI-like with color runs; SAUCE metadata (read-only at first).
- Scrolling for large canvases (TScroller).

### Phase 3
- Enhanced tools: circle/ellipse, filled shapes, spray, brushes.
- Selection/copy/paste; region fills; block moves.
- 256-color / 24-bit enhancements (where terminals support) — optional.

## Architecture (TVision)

- `PaintApp : TApplication` — Menubar/StatusLine/DeskTop.
- `PaintWindow : TWindow` — Hosts canvas and optional subviews; tileable/zoomable.
- `PaintCanvasView : TView` (or `TScroller` later) — Owns the 2D buffer of cells and handles draw and input.
- `ColorDialog`, `ToolDialog` — TV dialogs to pick options.

Rendering:
- Internal buffer: vector< Cell >, size = cols × rows, where `Cell { ch, fg, bg }` (full mode) or `Cell { upperOn, upperFg, lowerOn, lowerFg }` (half-vertical mode).
- `draw()` builds a `TDrawBuffer` row and writes via `writeLine`.

Input:
- TV keyboard and mouse events in `handleEvent()`; map arrows, Shift modifiers, mouse buttons.

## Data Model

- Full-pixel mode: per cell → `ch` (char), `fg` (0..15), `bg` (0..15).
- Half-pixel mode: per cell encodes upper/lower subpixels with colors:
  - Map to glyph + FG/BG at draw time:
    - upper only → ‘▀’ with fg = upper, bg = canvasBg
    - lower only → ‘▄’ with fg = lower, bg = canvasBg
    - both same → ‘█’ fg
    - both diff → ‘▀’ fg = upper, bg = lower (two colors in one cell)
- Global canvas background color (for empty halves), default black.

## Algorithms (Extracted/Pseudocode)

### 1) Pencil
- Full mode:
```
function drawPoint(x, y, fg, bg):
  cell(x,y).ch = BLOCK             // 0xDB
  cell(x,y).fg = fg
  cell(x,y).bg = bg
```
- Half mode:
```
function drawHalf(x, y, ySub, fg):
  if ySub == UPPER:
    cell(x,y).upperOn = true
    cell(x,y).upperFg = fg
  else:
    cell(x,y).lowerOn = true
    cell(x,y).lowerFg = fg
```

### 2) Line (Bresenham)
```
function drawLine(x0,y0, x1,y1):
  dx = abs(x1-x0); sx = x0 < x1 ? 1 : -1
  dy = -abs(y1-y0); sy = y0 < y1 ? 1 : -1
  err = dx + dy
  while true:
    drawPoint(x0, y0, fg, bg)
    if x0==x1 and y0==y1: break
    e2 = 2*err
    if e2 >= dy: err += dy; x0 += sx
    if e2 <= dx: err += dx; y0 += sy
```
- Half mode: draw using drawHalf() along the path (choose UPPER/LOWER based on cursor mode or auto-map subrows).

### 3) Box/Rectangle
```
function drawBox(x0,y0,x1,y1):
  // horizontal lines
  for x in [min(x0,x1) .. max(x0,x1)]:
    drawPoint(x, y0)
    drawPoint(x, y1)
  // vertical lines
  for y in [min(y0,y1) .. max(y0,y1)]:
    drawPoint(x0, y)
    drawPoint(x1, y)
```
- Filled box: fill interior with drawPoint/drawHalf.

### 4) Flood Fill (4-neighbor)
```
function floodFill(x,y, targetColor, newColor):
  if cell(x,y).color != targetColor or targetColor == newColor: return
  stack = [(x,y)]
  while stack not empty:
    (cx,cy) = stack.pop()
    if cell(cx,cy).color == targetColor:
      cell(cx,cy).color = newColor
      push neighbors (cx±1,cy) (cx,cy±1) if in bounds
```
- For half mode: define “color” per half; fill based on active subpixel or both halves.

### 5) Text Insert
```
function insertText(x,y,string, fg,bg):
  for i=0..len-1:
    cell(x+i,y).ch = string[i]
    cell(x+i,y).fg = fg
    cell(x+i,y).bg = bg
```

### 6) Color Picker (16 colors)
- Present 16 FG and 16 BG swatches; update current `fg/bg`.
- Optional: quick keys 0..9, A..F.

## File Formats (MVP)
- Option A: Plain text (lossy: ignore color): simplest but discards colors.
- Option B (recommended): Minimal RLE per line with runs: (ch, fg, bg, count). Serialize lines with concise ASCII tags (e.g., JSONL-like or custom plain text lines). Example:
```
LINE 0: (█,14,0,10) ( ,7,0,5) ...
```
- Later: ANSI export (SGR runs), SAUCE metadata.

## UI Mapping
- Menubar:
  - File: New, Open, Save, Exit
  - Edit: Clear, (Undo/Redo later)
  - Colors: Foreground…, Background…, Swap
  - Tools: Pencil, Line, Box, Fill, Text
  - View: Pixel mode (Full/Half), Toggle grid (later), Zoom (later)
- Status line: x,y, tool, FG/BG, pixel mode (U/L when half)

## Implementation Plan (Detailed)

1) Canvas & Rendering
- [ ] Define `PaintCanvasView` with buffer and row-wise draw via `TDrawBuffer`.
- [ ] Implement full-pixel mode.
- [ ] Implement half-pixel vertical mode (mapping upper/lower → ▀/▄/█ with FG/BG mix).
- [ ] Focus cue by reversing attribute at cursor.

2) Input & Tools
- [ ] Keyboard arrows; Shift+arrows draw while moving; Space toggles draw.
- [ ] Mouse move/left-click; Shift+click draws.
- [ ] Tools: Pencil → Line → Box → Fill → Text in order.

3) Colors
- [ ] 16-color picker dialog for FG/BG.
- [ ] Toggle FG/BG (Tab or explicit menu) and show in status line.

4) File IO
- [ ] Save/Open minimal format (start with plain text; then RLE with color runs).
- [ ] New (clear with confirmation when dirty), Exit.

5) UI Polish
- [ ] Menubar and Status line per above.
- [ ] Optional palette/tool views as subviews.

6) Testing
- [ ] Draw lines/boxes/fill/text and verify output visually.
- [ ] Half-pixel mapping correctness for single-color and dual-color cells.
- [ ] Save/Load round-trip correctness (RLE mode).
- [ ] Performance on large canvases (e.g., 160×50).

### Milestones
- Day 1–2: Canvas+draw paths; full-pixel pencil.
- Day 3–4: Line/box; color dialog; minimal save/open.
- Day 5–6: Fill and Text; half-pixel mode; status.
- Day 7: Testing + polish.

## Risks & Mitigations
- Performance: Avoid per-cell syscalls; use row buffers; add dirty-rect later if needed.
- Unicode widths: Block chars should be single-width on modern terminals; test early.
- Color drift: Stick to BIOS 16-colors for consistent mapping.
- Fill complexity: Use 4-neighbor fill with bounds checks; switch to queue for big areas.

## Alternatives Considered
- Curses shim to compile NewDraw directly → more glue, less TV-native; rejected for long-term maintainability.
- Full rewrite with new feature set → drifting scope; we keep parity first.

## Success Criteria
- Visual parity with NewDraw core tools on simple tasks.
- Smooth drawing with keyboard/mouse in both full and half pixel modes.
- Reliable save/load round-trips for small canvases.
- TV-native look-and-feel (menus/dialogs) with no curses deps.

## Notes on Licensing & Attribution
- Verify NewDraw’s license; if permissive, attribute algorithms as “inspired by” with no direct code copy unless allowed.
- Maintain this PRD as the canonical mapping spec; embed pseudo-code references in code comments where algorithms were ported.

---

## Implementation Journal (Live TL;DRs)

This section is updated as we code. Each entry includes a TL;DR and key decisions.

### Entry 1 — Scaffold paint_tui and Canvas (Full Pixels)
- TL;DR: Added `paint_tui` app under `test-tui` with `PaintApp`, `PaintWindow`, and `TPaintCanvasView`. Keyboard arrows move the cursor; Shift+Arrows draw; Space toggles a block. Focus indicator: current cell is reversed when focused.
- Files: `test-tui/paint/paint_app.cpp`, `paint_canvas.h/.cpp`, `CMakeLists.txt` target `paint_tui`.
- Rendering: Row-wise `TDrawBuffer` → `writeLine` flush.
- Input: TV `evKeyDown` with `kbLeft/Right/Up/Down`, `evMouseDown` for basic click draw.
- Decisions: Start with 16-color BIOS mapping in `TColorAttr`; background is global canvas bg.

### Entry 2 — Half-Pixel (Vertical) Mode
- TL;DR: Implemented vertical half pixels using ‘▀/▄/█’ with FG/BG mapping so we can show two colors per cell (upper via FG, lower via BG). Added View → Toggle Half Pixels and Tab to switch subpixel (U/L).
- Mapping:
  - upper only → ‘▀’ fg=upper, bg=canvasBg
  - lower only → ‘▄’ fg=lower, bg=canvasBg
  - both same → ‘█’ fg=color
  - both different → ‘▀’ fg=upper, bg=lower (two-color cell)
- Cursor: Reversed at (x,y) to indicate focus; status update pending.
- Notes: Unicode blocks assumed single-width; tested in modern terminals.

### Next — Color Picker + Status Line
- TL;DR: Add a 16-color picker dialog for FG/BG and show (x,y, mode U/L, FG/BG) in the status line. Then minimal Save/Open (RLE per line) and line/box/fill/text tools.
- Decisions: Keep file format minimal and human-readable for MVP; add ANSI/SAUCE later.
