# Could we add a menu option "Save workspace" (like VS Code) to persist and restore the test_pattern app windows (type, contents, position, etc.) so the session can be restored later?

## TL;DR

Yes. Add File menu items to save/load a JSON “workspace” capturing the desktop layout. For MVP we keep it simple: store each window’s type, title, outer bounds, zoomed state, Z-order, and a small set of props; and store app-wide globals (notably the pattern mode) in one place. On load, we close current windows and recreate the saved ones in order, clamping bounds to the current terminal size. We implement a tiny embedded JSON writer/reader specific to this schema (no external deps), on the main thread.

Approaches ranked:
1) App-level serializer to JSON (recommended): walk `TProgram::deskTop` and write a compact, versioned JSON. Clear, human-readable, easy to evolve.
2) Turbo Vision streaming (op/ipstream): binary format, tightly coupled to class internals, harder to diff/edit; better as a later phase if needed.
3) Third-party JSON lib: feature-rich but overkill and adds dependency for a small schema.

MVP bias: minimize moving parts now (globals at top-level, simple window props), defer richer per-window props and extra types to later phases.

---

## Background

The test_pattern app is a Turbo Vision (`TApplication`) desktop with multiple windows (e.g., Test Pattern, various Gradients). Turbo Vision supports overlapping, tiling, zoom, and standard commands via the menu/status lines. We want VS Code–style “workspace” persistence so users can close the app and later restore their layout and windows.

Key Turbo Vision concepts relevant here:
- `TProgram::deskTop`: container of `TWindow` instances, provides iteration and z-order.
- `TWindow`: has a frame, client area, title, zoom state; inserted into the desktop.
- Views (e.g., `TTestPatternView`, gradient views) provide type-specific rendering and configuration.

---

## Goals

### Must
- Add File → “Save Workspace…” to write a JSON file representing the current session.
- Add File → “Open Workspace…” to restore a previously saved session.
- Save per-window: type, title, bounds (x, y, w, h), zoomed state, Z-order (by array order), and minimal type-specific props:
  - Gradient: `gradientType` = `"horizontal"|"vertical"|"radial"|"diagonal"`.
- Save app-wide globals: `globals.patternMode` = `"continuous" | "tiled"` (the current app uses a global toggle).
- Restore: create windows in saved order; clamp and adjust bounds to current screen; apply type-specific props and globals.
- JSON includes `version`, `app` id, `timestamp`, and `screen` size for context.

### Should
- Optionally store gradient start/end colors if/when user-facing controls exist.
- Persist which window had focus; restore focus where possible (`focusedIndex`).
- Default save location proposal: `./workspaces/last_workspace.json` (relative to app working dir), plus allow choosing a path.
- Provide a quick “Restore Last Workspace” action that loads the default path if present.
- Future per-window pattern mode: if/when we refactor `TTestPatternView` to own its mode, move `patternMode` back into each Test Pattern window’s `props`.

### Won’t (MVP)
- Persist menu selections, transient dialogs, or clipboard.
- Persist animation state of any live animations beyond type-specific props (e.g., donut frame index).
- Cross-app migration tooling (JSON is human-editable for ad-hoc changes).

---

## JSON Schema (MVP)

Top-level object fields:
- `version: 1` — increment if breaking changes occur.
- `app: "test_pattern"` — app identifier.
- `timestamp: ISO-8601` — time of save.
- `screen: { width: number, height: number }` — terminal size at save time (informational).
- `globals: { patternMode: "continuous" | "tiled" }` — app-wide settings used by the current build.
- `windows: Array<Window>` — z-ordered list; first array entry is bottom-most, last is top-most.

Window entry (common fields):
- `id: string` — stable id within file (for debugging; not required for restore).
- `type: "test_pattern" | "gradient" | "donut_simple" | "custom"` — window kind (note: `donut_simple` may not appear in current builds; future use).
- `title: string` — window title.
- `bounds: { x: number, y: number, w: number, h: number }` — outer window rect in desktop coordinates (character cells).
- `zoomed: boolean` — whether the window is zoomed.
- `props: object` — type-specific properties:
  - For `gradient`: `{ gradientType: "horizontal" | "vertical" | "radial" | "diagonal", startColor?: "#RRGGBB", endColor?: "#RRGGBB" }`.
  - For others: free-form until defined.

Example:
```json
{
  "version": 1,
  "app": "test_pattern",
  "timestamp": "2025-09-03T14:55:00Z",
  "screen": { "width": 160, "height": 48 },
  "globals": { "patternMode": "continuous" },
  "windows": [
    {
      "id": "w1",
      "type": "test_pattern",
      "title": "Test Pattern 1",
      "bounds": { "x": 2, "y": 1, "w": 50, "h": 15 },
      "zoomed": false,
      "props": {}
    },
    {
      "id": "w2",
      "type": "gradient",
      "title": "Horizontal Gradient 2",
      "bounds": { "x": 10, "y": 6, "w": 60, "h": 18 },
      "zoomed": false,
      "props": { "gradientType": "horizontal" }
    }
  ]
}
```

## Sample Workspace File (MVP)

Copy-pasteable example you can save as `workspaces/last_workspace.json` to verify the loader. Values are illustrative and safe to adjust.

```json
{
  "version": 1,
  "app": "test_pattern",
  "timestamp": "2025-09-03T15:15:00Z",
  "screen": { "width": 120, "height": 30 },
  "globals": { "patternMode": "tiled" },
  "windows": [
    {
      "id": "w1",
      "type": "gradient",
      "title": "Diagonal Gradient",
      "bounds": { "x": 3, "y": 2, "w": 50, "h": 12 },
      "zoomed": false,
      "props": { "gradientType": "diagonal" }
    },
    {
      "id": "w2",
      "type": "test_pattern",
      "title": "Test Pattern",
      "bounds": { "x": 12, "y": 8, "w": 60, "h": 18 },
      "zoomed": false,
      "props": {}
    }
  ]
}
```

Notes:
- `globals.patternMode` is app-wide for MVP. If the app is currently in Continuous mode, use `"continuous"`.
- `windows` order defines Z-order: last item is top-most.
- Bounds are outer window rects and will be clamped to the current terminal.

---

## UI/UX

- File → Save Workspace… (Ctrl/Cmd-S alternative is already used; we’ll keep Ctrl-S for screenshots in this app; use plain menu selection here.)
  - Opens a simple file dialog or input prompt (MVP: fixed default path and overwrite confirmation; optional manual path input if easy).
  - On success: show brief confirmation.
  - On failure: show error with path and reason.

- File → Open Workspace…
  - Opens a simple file dialog or input prompt; reads JSON and restores windows.
  - Before restoring: close all existing windows (ask confirmation if non-empty desktop).
  - On failure: show error; keep current desktop intact.

- Optional: File → Restore Last Workspace (disabled if no default file).

---

## Restore Semantics

- Apply globals: set `patternMode` before creating windows, so views draw with expected defaults.
- Bounds clamping: ensure restored windows fit within current desktop extent; if out-of-bounds, shift inward; if too large, shrink to a minimum (e.g., 20×5) while preserving proportions.
- Z-order: insert in the order of the `windows` array; last inserted becomes top-most. If a window is `zoomed`, call `zoom()` after insertion.
- Focus: optional; if a `focusedIndex` is present (later phase), focus that window; otherwise, the last inserted (top-most) is focused.
- Unknown types: skip gracefully and continue; report in a message dialog.
- Version mismatch: if `version` unsupported, show an error and abort restore.

---

## Persistence Location

- Default directory: `./workspaces/` relative to the app working directory (MVP keeps things local and inspectable).
- Default file: `./workspaces/last_workspace.json` for the quick “Restore Last” action.
- Users can specify arbitrary paths in Save/Open dialogs (MVP may defer to fixed path to simplify).

---

## Error Handling

- Write failures: show a modal error with path and reason (permissions, disk full, etc.).
- Read/parse failures: show a modal error with the first parse error location and a hint.
- Partial restores: unknown window types or malformed entries are skipped; count and show a summary to the user.

---

## Security / Safety

- Only read JSON files from local disk when user explicitly selects a path.
- Do not execute or import code from the workspace.
- Clamp values (bounds, sizes) to sane limits to avoid pathological layouts.

---

## Implementation Plan (change `-[ ]` to `-[x]` as you complete steps)

-[ ] Define JSON schema constants and helpers (version, app id).
-[ ] Add File menu items: “Save Workspace…”, “Open Workspace…”, and optionally “Restore Last Workspace”.
-[ ] Implement serializer: enumerate `deskTop` windows, detect type via `dynamic_cast`, collect fields; capture `globals.patternMode`.
-[ ] Implement minimal JSON writer (string escapes, numbers, booleans) tailored to our schema (no third-party deps).
-[ ] Implement saver: create `./workspaces` if missing, write `last_workspace.json` (or chosen path), confirm success.
-[ ] Implement loader: parse JSON (minimal parser for our schema), validate `version` and `app`.
-[ ] Implement restore: set globals from `globals`, close existing windows, recreate per entry (switch on `type`), set title, bounds, zoom, props.
-[ ] Bounds clamping utility: fit rect into current desktop, ensure minimum size.
-[ ] Error reporting: centralized helper for save/load failures.
-[ ] Manual test matrix: small/large terminal, multiple window mixes, zoomed windows, bad JSON, unknown types.

Note: Mark each completed step by changing its prefix from `-[ ]` to `-[x]` in this document.

---

## Testing

- Save/Load round trip with:
  - Only Test Pattern windows with mixed pattern modes.
  - Mix of Gradient windows of all types.
  - Zoomed windows and overlapping z-order.
  - Terminal smaller than saved layout (verify clamping/shrinking).
- Corrupted JSON (missing commas/braces): loader fails gracefully with a clear message.
- Unknown window type: skipped; success message reports skipped count.

---

## Future Enhancements

- Per-window pattern mode if `TTestPatternView` becomes per-instance configurable; remove global.
- Persist gradient colors and any future window-specific settings.
- Persist workspace-wide settings (e.g., palette, wallpaper options) in `globals`.
- Explicit `focusedIndex` and restoring focus.
- Recently used workspaces list; choose from menu.
- Auto-save on exit and auto-restore on start (optional toggle).
- Cross-app schema alignment if more apps adopt workspaces.
