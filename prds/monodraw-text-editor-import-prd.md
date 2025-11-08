# Monodraw Text Editor Import PRD

**TL;DR**: Extend T-Vision tooling so artists can open Monodraw ASCII layouts directly inside the in-app text editor. Parse `.monojson` (and later native `.monodraw`) files, expose a friendly import workflow that maps layers to editable text, and reuse the existing IPC/APIs so both the desktop app and automation scripts can bring Monodraw scenes into `TTextEditorView` windows with accurate geometry and metadata.

## Background
- Turbo Vision test-tui already supports text viewers (`TTextFileView`) and editing (`TTextEditorView`) plus IPC endpoints for spawning windows (`api_open_text_view_path`, `api_spawn_text_editor`).
- Recent backend commit `9c63d9b` introduced a Python-side Monodraw JSON loader that spawns multiple text windows by writing layer content to temp files and calling the IPC API.
- Designers increasingly prototype ASCII layouts in Monodraw for macOS. Today they must export to plain text, manually clean up JSON artifacts, or rely on the experimental API loader to view content; editing within T-Vision still requires manual copy/paste.
- A unified import path into the interactive text editor closes the loop for iteration, lets artists tweak content live, and reduces double handling between tooling.

## Goals
- Allow a user to choose a Monodraw file and populate a `TTextEditorWindow` with the rendered ASCII for one or more layers.
- Support both manual (menu/dialog) and programmatic (API/CLI) flows so automation keeps pace with UX.
- Preserve spatial fidelity (line wrapping, whitespace, Unicode glyphs) between the Monodraw canvas and the Turbo Vision editor.
- Provide layer-aware tooling (preview, selection, metadata badges) that reflects how artists structure their Monodraw projects.
- Ship documentation, sample assets, and regression tests that exercise the importer end-to-end.

## Non-Goals
- Replacing the existing window-spawning loader (it remains for multi-window layouts).
- Live round-trip editing back into Monodraw’s proprietary format.
- Rendering non-text Monodraw primitives (connectors, shapes) beyond extracting the text-layer payload Monodraw already encodes.
- Implementing a full Monodraw UI inside Turbo Vision.

## Personas & Key Scenarios
- **ASCII Artist / Monodraw Power User**: Crafts layered ASCII scenes in Monodraw, wants to preview and edit copy inside Turbo Vision without leaving terminal context.
- **CLI Automation Engineer**: Pipes Monodraw exports into scripted workflows (e.g., CI pipelines, batch templating) and needs deterministic APIs that surface parsed text to editor windows for further macro operations.
- **QA / Release Engineer**: Validates that importer output respects sizing, clipping, and Unicode handling across Linux/macOS terminals.

## Use Cases
1. Artist triggers `File → Import Monodraw…`, previews available layers, selects one, and the text editor opens populated with that layer’s ASCII, cursor placed at top.
2. Automation script POSTs `/monodraw/import` with `target: "text_editor"` and `layer: "symbient-city"`; the running app launches/flashes an editor window, injects content via `send_text` in `replace` mode, and returns metadata (line count, bounding box).
3. User chooses “Merge layers” option to flatten multiple selected layers ordered by Z, generating a single editor document with divider comments.
4. User imports a `.monodraw` bundle (zipped archive), importer extracts `document.json`, converts to internal schema, falls back gracefully if features are unsupported.

## Functional Requirements
### File & Format Support
- Accept `.monojson` exports as the primary MVP format.
- Detect `.monodraw` bundles (ZIP containers) and extract JSON payload using a lightweight reader (e.g., miniz) in Phase 2.
- Validate Monodraw version (`header.v`) and surface a warning for unsupported revisions.

### Parsing & Normalisation
- Promote `tools/api_server/monodraw_parser.py` logic into a shared library (e.g., `shared/monodraw/`) with a clean data model (`MonodrawDocument`, `MonodrawLayer`). Generate both Python and C++ bindings or emit a CLI the C++ side can call.
- Normalise newline handling, trim trailing blank rows, respect full Unicode grapheme width when computing editor column widths (use `TText::width`).
- Capture layer metadata: name, origin, frame size, z-index (infer from array order), text content, optional notes.

### Import Workflow (TUI)
- Add `File → Import Monodraw…` menu entry (keyboard shortcut TBD) that opens a file dialog filtered to `.monojson;.monodraw`.
- After file selection, show a lightweight preview dialog listing layers with dimensions (`w×h`), origin, and a content snippet. Allow multi-select, reorder via hotkeys, and choose “Flatten” vs “Open separately”.
- When importing into a single editor window:
  - Spawn (or reuse) `TTextEditorWindow` via `createTextEditorWindow`.
  - Issue `replace` mode `sendText` with the prepared document payload.
  - Optionally insert a header comment summarising file name, layer count, import timestamp.
- When “Open separately” is chosen, create one editor per layer with cascaded positioning and include layer metadata in window title.

### Import Workflow (API/Automation)
- Extend API controller with `POST /monodraw/import` accepting:
  - `file_path`, optional `layer` (string or array), `mode` (`replace|append|insert`), `target_window` (`auto|window_id`).
  - `flatten`: bool, `order`: `source|custom`, `insert_position`: `start|end|cursor`.
- API uses shared parser, prepares payload string(s), calls existing IPC commands (`spawn_text_editor`, `send_text`), and returns `{window_id, layers, lines, width, warnings}`.
- Maintain backwards compatibility with `/monodraw/load` by allowing `target: "windows"` or `target: "text_editor"` so automation can reuse one endpoint.

### Data Representation
- In C++, represent parsed data with immutable structs; expose helper to render flattened documents with configurable separators (`// --- layer ---`).
- Cache last-import metadata in app state (e.g., `MonodrawImportSession`) so UI can offer “Re-import last file”.
- Keep generated temp files (if used) under `build/tmp/monodraw/` and clean up on exit; prefer in-memory buffers to avoid disk churn where possible.

### Error Handling & UX Feedback
- Show modal errors for malformed files (missing `object_list`, empty `model_text`), include layer names.
- For layers without text, list them as disabled with tooltip “No text content; skipped”.
- If a selected layer exceeds editor size limits, prompt to auto-resize window or clamp.
- Provide status-line feedback (“Imported 3 layers from symbient-city.monojson in 42 ms”).

## Technical Approach
- **Shared Parser Module**: Move parsing logic into `shared/monodraw/monodraw_parser.{h,cpp}`; expose a slim API returning POD structs. Use the same module from Python via a tiny wrapper (pybind11 or subprocess CLI) to avoid divergent implementations.
- **C++ Import Service**: Add `MonodrawImporter` under `test-tui/monodraw/` responsible for orchestrating file selection, preview, flattening, and editor injection. Integrate with `TTestPatternApp` (new command id, menu entry, dialog classes).
- **Text Editor Integration**: Extend `createTextEditorWindow` / `TTextEditorWindow` to accept initial content and metadata (e.g., `setDocumentInfo`). Reuse `TTextEditorView::sendText` for actual injection to leverage cursor/scroll updates.
- **API Server Changes**: Refactor `/monodraw/load` to call shared parser; add `/monodraw/import` route plus schema definitions. Update IPC handler to accept a `cmd:import_monodraw` message if direct control is desired.
- **Metadata Surfacing**: Introduce optional gutter overlay displaying layer origin/size, powered by editor view background hooks (future enhancement).

## Testing Strategy
- Unit tests for parser covering:
  - Valid multi-layer `.monojson` (existing `monodraw-demo-simple-primers.monojson`).
  - Edge cases: missing `name`, empty `text`, unusual `origin` strings, Unicode emoji.
- C++ tests under `test/monodraw/` verifying flattening logic, editor payload generation, width calculations, error reporting.
- API integration tests hitting `/monodraw/import` with mocked IPC (use existing controller harness).
- Manual QA checklist: import via menu, ensure window sizing matches `calculateWindowBounds`, verify undo/redo still works, confirm behaviour after subsequent imports (append, replace, merge).
- Add sample fixture files under `test-tui/monodraw_samples/` plus README with provenance.

## Documentation & Samples
- Update `README.md` (test-tui section) with new command instructions.
- Add `test-tui/FRAME_PLAYER.md` appendix referencing Monodraw importer.
- Provide a tutorial (`docs/monodraw-import.md`) showing Monodraw → Turbo Vision workflow with screenshots/GIFs.
- Document API contract in `tools/api_server/README.md` and generate OpenAPI snippet.

## Rollout Plan
1. **Phase 0** – Parser consolidation (shared module, Python wrapper migration).
2. **Phase 1** – Core C++ importer: menu command, single-layer import, replace mode.
3. **Phase 2** – Multi-layer/flattening, API endpoint, automation hooks.
4. **Phase 3** – `.monodraw` ZIP support, metadata overlays, polish (status messages, re-import shortcut).
5. **Phase 4** – Optional enhancements (layer search/filter, favourites, workspace persistence).

## Risks & Mitigations
- **Divergent parsers (Python vs C++)** → Single shared module with tests, enforce via CI.
- **Large Monodraw canvases overflowing terminal** → Use existing `calculateWindowBounds` logic, add warning when width/height exceed desk bounds and offer scale-down.
- **Unicode width discrepancies** → Rely on `TText::width` for column calculations, add tests with double-width glyphs.
- **Performance on big files** → Stream parse JSON with rapidjson or nlohmann, avoid quadratic string concatenation by using `std::ostringstream` with reserved capacity.
- **Dependency bloat for ZIP support** → Prefer header-only `miniz` already used elsewhere; make advanced format optional behind flag until stabilised.

## Dependencies
- Existing IPC infrastructure (`api_ipc.cpp`) must expose a new command if direct import is needed.
- Potential third-party miniz/zip library (verify licensing).
- Coordination with documentation maintainers for updated guides.

## Open Questions
- Should flattened imports preserve Z-order strictly or allow custom stacking per import?
- Do we need syntax highlighting or annotations inside the editor for Monodraw metadata (e.g., layer markers)?
- What is the desired behaviour when importing into an editor with unsaved changes (prompt, append, cancel)?
- Is bidirectional sync (export back to Monodraw) a near-term requirement worth prototyping hooks for now?

## Appendix
- Prior art: `prds/monodraw-json-window-loader-prd.md` and commit `9c63d9b` (Monodraw JSON Window Loader) describe the window-spawning pathway that this PRD complements.
- Sample asset: `test-tui/monodraw-demo-simple-primers.monojson`.
