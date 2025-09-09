# PRD: New Text Document (Editable) + Optional Paste From Clipboard

This PRD adds an editable text document flow to the `test-tui` app so users can create a new, empty document, type into it, and optionally initialize it with the current system clipboard contents. It reuses Turbo Vision’s editor components and minimal platform glue for clipboard reads.

## Objectives
- Add File → New → Text Document… that opens an editable text window.
- On creation: optional prompt “Initialize from clipboard?” that, if accepted, pastes current system clipboard contents into the empty buffer.
- Provide in-window editing (insert, delete, cursor move, select, copy/cut/paste, scroll) with standard TVision editor behavior.
- Keep implementation small, portable, and consistent with existing app patterns.

## Non-Goals (MVP)
- No syntax highlighting or soft-wrap.
- No auto-save or recovery.
- No cross-platform write-back to system clipboard (copy) for MVP; focus on paste-in. (Copy/ Cut remain intra-app unless OS support is trivially available.)

## User Story
As a user, I can choose File → New → Text Document… to open a blank editable window. I can type or paste text. If I choose, I can initialize the new document from the system clipboard in one step. I can open multiple documents and save them later via File → Save/Save As.

## UX / Menu
- File menu:
  - New → Text Document… (`cmNewTextDoc`)
  - Save (`cmSave`), Save As… (`cmSaveAs`) apply to the active editor window.
- On create: modal prompt: “Initialize from clipboard?” [Yes/No].
- Status line: show standard editor hints (Ins, Del, Ctrl+S = Save, F2 = Save As, etc.).

## Architecture Overview
- Base editor: TVision `TEditor`/`TEditWindow` stack from the core library. Create one window per document.
- Document lifecycle:
  - Unsaved “Untitled N” buffers until saved.
  - Close prompts for save if modified.
- Clipboard-integration (paste only): tiny helper that detects a platform tool and captures UTF-8 text into a std::string; inserted into the editor at caret.
  - macOS: `pbpaste`
  - Wayland: `wl-paste`
  - X11: `xclip -selection clipboard -out` or `xsel --clipboard --output`
  - Windows (optional): `powershell -command Get-Clipboard`
  - If unavailable or empty, proceed with blank document.

## Detailed Design

### Types and Files (app)
- `test-tui/text_doc_window.h/.cpp` (new)
  - `TTextDocWindow : public TEditWindow`
    - Title: “Untitled N” until saved.
    - Wires Save/Save As via standard `TEditor` commands.
    - Adds a helper `initializeFromString(const std::string&)` to insert initial content.
- `test-tui/clipboard_read.h/.cpp` (new)
  - `bool readClipboard(std::string &out, std::string &err)`:
    - Tries tools in order (configurable): macOS pbpaste → wl-paste → xclip → xsel → PowerShell.
    - Returns UTF-8 text; trims trailing CR for CRLF.
    - Fails fast with clear error string; non-fatal to caller.

### Menu Integration
- In `test_pattern_app.cpp`:
  - Add command id: `const ushort cmNewTextDoc = 140;` (pick the next free id).
  - File → New → Text Document… menu item.
  - Handler:
    - Compute bounds similar to other windows.
    - Create `TTextDocWindow` (untitled); prompt “Initialize from clipboard?” — if Yes and `readClipboard(...)` succeeds, call `initializeFromString(...)` before showing.
    - Insert to desktop and register with IPC if applicable.

### Editor Window Details
- Construction
  - Use `TEditWindow` (TVision built-in) with an empty buffer and default editor behaviors.
  - `growMode = gfGrowHiX | gfGrowHiY` to fill/rescale.
- Save/Save As
  - On Save without path, show file dialog (reuse existing file dialog or a minimal path prompt); write UTF-8.
  - On Save As: always show file dialog; update title.
  - On close: if modified, show modal “Save changes?” [Yes/No/Cancel].
- Paste (runtime)
  - Map `cmPaste` to insert from TVision’s internal clipboard.
  - Add a secondary command `cmPasteFromSystem` to run `readClipboard()` and insert returned text at caret; fallback to message on error.

### Clipboard Strategy (MVP)
- Exec-based read with small timeouts; strip trailing CR; preserve LF.
- Don’t block the UI: run synchronously but short; present message if command missing.
- Security: no shell interpolation of clipboard content; capture via argv form; don’t log secrets.

### Commands and IDs
- `cmNewTextDoc = 140` (new)
- `cmPasteFromSystem = 141` (new)
- Reuse editor-standard: `cmSave`, `cmSaveAs`, `cmClose`, `cmCut`, `cmCopy`, `cmPaste`.

## Error Handling
- Clipboard missing/unavailable: continue with empty document; show non-blocking message (info/OK) if user chose to initialize.
- Save errors (permissions, path): show error dialog; leave editor open and modified.

## Performance Considerations
- Editor is lightweight; content in-memory per document.
- Clipboard read bounded by small buffer and timeout; typical payloads are small (<1 MB). Very large pastes: truncate to a capped size (configurable, e.g., 1 MB) with warning.

## Edge Cases
- Empty clipboard: proceed blank without error.
- CRLF inputs: normalize to LF internally; preserve on write if desired (future option). MVP writes LF.
- Multiple windows: independent editors with their own modified states.

## Testing Plan (Manual)
- Create new text doc; type; save; reopen file to verify contents.
- Choose “Initialize from clipboard?” Yes: verify contents match system clipboard.
- Choose No: verify blank document.
- Paste from system while editing via a menu item: verify insertion at caret.
- Close with unsaved changes: prompt appears and behaves correctly.

## Implementation Checklist
- Files
  - [ ] `text_doc_window.h/.cpp` — wrapper around `TEditWindow`
  - [ ] `clipboard_read.h/.cpp` — platform helper for paste-in
  - [ ] Menu wiring in `test_pattern_app.cpp` (`cmNewTextDoc`, create/insert, prompt)
- Behavior
  - [ ] Editor opens empty; typing works; save/save-as wired
  - [ ] Optional initialize-from-clipboard prompt
  - [ ] `cmPasteFromSystem` command inserts clipboard text at caret

## Future Work
- System clipboard write-back for Copy/Cut (`pbcopy`, `wl-copy`, `xclip -in`, etc.).
- Soft-wrap, syntax highlighting, and larger-file handling.
- Configurable line endings and encoding.
- Drag-and-drop (path handoff), recent files, and auto-save.

## Acceptance Criteria
- New “Text Document…” creates an editable window.
- Optional clipboard initialization works across platforms with installed tools; gracefully degrades otherwise.
- Save/Save As function as expected; close prompts on unsaved edits.
- Multiple documents operate independently; feature integrates seamlessly with existing app windows and tiling/cascading.
