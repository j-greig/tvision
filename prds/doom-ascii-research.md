# Research: Doom ASCII repo feasibility for WibWobDOS

## Quick take
- Repo: https://github.com/wojciech-graj/doom-ascii
- Language: C; Doom engine source port based on doomgeneric.
- License: GPL-2.0 (strong copyleft).
- Runtime: requires a DOOM WAD file (shareware or full), no sound.
- Output: writes ANSI to stdout, with color and gradient modes.
- Input: reads raw terminal input (termios/Win console).

## What the repo does (from README and code)
- "Text-based DOOM in your terminal", no sound, WAD required. README documents build with `make` and runtime flags like `-scaling` and `-chars` (ASCII, block, braille).
- Rendering pipeline is in `src/doomgeneric_ascii.c`:
  - Uses `DG_ScreenBuffer` (per-pixel RGB data) and converts each pixel to two characters wide output.
  - Supports ASCII gradient, Unicode block elements, and braille patterns.
  - Emits 24-bit ANSI color sequences and a full-frame string to stdout every frame.
  - Optional modes: `-nocolor`, `-nograd`, `-nobold`, `-erase`, `-fixgamma`.
- Input pipeline is in `src/doomgeneric_ascii.c`:
  - Reads raw terminal input (termios on Unix, console events on Windows).
  - Maintains a key buffer and provides events via `DG_GetKey()`.
  - Uses smoothing (`-kpsmooth`) to handle repeat delay jitter.

## Key technical observations for a TUI port
- Doomgeneric interface: `doomgeneric.h` defines hooks `DG_Init`, `DG_DrawFrame`, `DG_GetKey`, `DG_GetTicksMs`, etc. The engine calls these; the ASCII port implements them.
- The current ASCII implementation directly controls the terminal (cursor movement, bold, color reset) and writes to stdout. This conflicts with Turbo Vision's screen and event loop and cannot be used as-is inside a TUI window.
- Output is double-width (two glyphs per pixel) and assumes a full-screen terminal. For an embedded window, resolution must be derived from view size.
- Color precision is full 24-bit and can be mapped to `TColorRGB` in tvision.

## Licensing constraints (high impact)
- GPL-2.0 means any linked or derived work must be GPL when distributed.
- If WibWobDOS is not GPL, the only safe path is to keep doom-ascii as a separate, optional program that is launched externally, not linked.
- If you are willing to distribute WibWobDOS under GPL-2.0 (or dual license), an in-process port is feasible.

## Feasibility summary
Feasible, but only with one of these decisions:
1) Accept GPL-2.0 for the combined binary (full in-process integration), OR
2) Keep doom-ascii as an external program and treat it as a separate tool (less integrated).

## Open questions
- Are we willing to ship GPL-2.0 for WibWobDOS, or do we need an external-process approach?
- What is the preferred user experience: fully embedded TUI window, or a "launch doom-ascii in a separate terminal" shortcut?
- Do we want to support only shareware WAD by default, or let the user provide any WAD path?
