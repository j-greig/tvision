# Monodraw Integration Status & Roadmap

**tl;dr**: Monodraw JSON window loader implemented (Sept 2024, commit `9c63d9b`). Spawns positioned text_view windows from Monodraw layers via REST API. Next phase: direct text editor import with layer selection UI and shared C++/Python parser. This doc summarises current state, API surface, and evolution pathway toward editor-centric workflows.

---

## Current Implementation (v1.0 — Window Loader)

**Status**: ✅ **SHIPPED** (commit `9c63d9b`)

**Canonical Specification**: See [`prds/monodraw-json-window-loader-prd.md`](./monodraw-json-window-loader-prd.md) for complete technical details, usage examples, and original requirements.

### What Works Today

The existing Monodraw integration enables:
- **Parse Monodraw JSON** (`.monojson` v5 format) to extract named text frame layers
- **Spawn positioned windows** — Each layer becomes a `text_view` window at precise coordinates
- **Coordinate scaling** — Auto-fit large designs to terminal size, apply offsets
- **REST API endpoints** — `/monodraw/load` (create windows), `/monodraw/parse` (preview layers)
- **Batch window creation** — Process multiple layers in single API call

**Key Files**:
- `tools/api_server/main.py:369-385` — FastAPI endpoints
- `tools/api_server/controller.py:571-714` — Load/parse implementation
- `tools/api_server/monodraw_parser.py:1-156` — Python JSON parser
- `tools/api_server/schemas.py:53-63` — Request/response models

**Demo Asset**: `test-tui/monodraw-demo-simple-primers.monojson` (4 layers: symbient-city, chaos-vs-order, cat-cat-simple, time-shamans)

### API Surface (Current)

#### POST /monodraw/load
Spawn positioned text_view windows from Monodraw layers.

**Request**:
```json
{
  "file_path": "/path/to/design.monojson",
  "scale": 1.0,
  "offset_x": 0,
  "offset_y": 0,
  "window_types": {"LayerName": "text_view"}
}
```

**Response**:
```json
{
  "ok": true,
  "windows_created": [{...}],
  "errors": [],
  "total_layers": 4,
  "windows_spawned": 4
}
```

#### POST /monodraw/parse
Preview layer structure without creating windows (dry-run).

**Request**: `{"file_path": "/path/to/design.monojson"}`
**Response**: `{"ok": true, "layers": [...], "canvas_bounds": {...}}`

### Architecture (Current)

```ascii
REST API → Python Parser → Temp Files → IPC (Unix socket) → C++ TUI → Spawn text_view Windows
           (monodraw_parser.py)                              (api_ipc.cpp)
```

**Limitations**:
- Only spawns `text_view` windows (read-only display)
- No layer selection UI (processes all named layers)
- Parser exists only in Python (no C++ access)
- Temp files required for text content (disk I/O overhead)
- No direct text editor integration

---

## Forward Evolution: Text Editor Import

**Next Phase Specification**: See [`prds/monodraw-text-editor-import-prd.md`](./monodraw-text-editor-import-prd.md) for complete requirements, workflows, and technical approach.

### New Goals (Beyond Window Loader)

**Primary Objective**: Allow users to import Monodraw layers **directly into text editor windows** for live editing, not just read-only viewing.

**Key Capabilities**:
- **Menu-driven workflow** — `File → Import Monodraw…` with layer preview dialog
- **Layer selection UI** — Choose specific layers, reorder, flatten multiple layers
- **Text editor integration** — Populate `TTextEditorWindow` with layer content (replace/append/insert modes)
- **API automation** — `POST /monodraw/import` with `target: "text_editor"` parameter
- **Shared parser** — Unified C++/Python parser module to eliminate divergence

### How This Extends Window Loader

| Feature                    | Window Loader (v1.0)     | Editor Import (v2.0)        |
|----------------------------|--------------------------|-----------------------------|
| **Target window type**     | `text_view` (read-only)  | `text_editor` (editable)    |
| **Layer selection**        | All named layers         | User-selectable subset      |
| **UI workflow**            | API-only                 | Menu + dialog + API         |
| **Multi-layer handling**   | Separate windows         | Flatten to single document  |
| **Parser location**        | Python-only              | Shared C++/Python module    |
| **Content delivery**       | Temp files               | In-memory + IPC             |
| **Use case**               | Static layout preview    | Live editing, iteration     |

---

## API Evolution Pathway

### Phase 2a: Add `target` Parameter to `/monodraw/load`

**Goal**: Allow existing endpoint to route to either window spawner or text editor importer.

**Changes to `tools/api_server/schemas.py`**:
```python
class MonodrawLoadRequest(BaseModel):
    file_path: str
    scale: float = 1.0
    offset_x: int = 0
    offset_y: int = 0
    window_types: Optional[Dict[str, str]] = None

    # NEW: Target destination for imported content
    target: Literal["windows", "text_editor"] = "windows"

    # NEW: Layer filtering (for text_editor target)
    layers: Optional[List[str]] = None  # Layer names to import, None = all

    # NEW: Text editor behaviour
    mode: Literal["replace", "append", "insert"] = "replace"
    flatten: bool = True  # Merge multiple layers into single document
    insert_position: Literal["start", "end", "cursor"] = "end"
```

**TODO (`tools/api_server/controller.py:571-714`)**:
```python
async def load_monodraw_file(self, ...):
    # Parse layers (existing logic)
    layers = MonodrawParser.parse_file(file_path)

    # NEW: Branch on target parameter
    if request.target == "text_editor":
        # TODO: Filter layers if request.layers specified
        selected_layers = self._filter_layers(layers, request.layers)

        # TODO: Flatten layers if request.flatten=True
        document_text = self._flatten_layers(selected_layers) if request.flatten else selected_layers[0].text_content

        # TODO: Call new IPC command to spawn text editor with content
        # cmd:spawn_text_editor_with_content text={document_text} mode={request.mode}
        window_id = await self._spawn_text_editor_with_content(document_text, request.mode)

        return {
            "ok": True,
            "target": "text_editor",
            "window_id": window_id,
            "layers_imported": [l.name for l in selected_layers],
            "lines": document_text.count('\n') + 1
        }
    else:
        # Existing window spawning logic (unchanged)
        return await self._spawn_windows(layers, ...)
```

**TODO (`test-tui/api_ipc.cpp`)**:
```cpp
// NEW IPC command: spawn_text_editor_with_content
// Payload: text={content} mode={replace|append|insert} position={start|end|cursor}
if (cmd == "spawn_text_editor_with_content") {
    std::string content = params["text"];
    std::string mode = params.get("mode", "replace");
    std::string position = params.get("position", "end");

    // TODO: Implement text editor spawning with initial content
    TTextEditorWindow* editor = app->createTextEditorWindow();
    editor->sendText(content, mode, position);

    reply = json{{"ok", true}, {"window_id", editor->id}};
}
```

---

### Phase 2b: New Dedicated Endpoint `/monodraw/import`

**Goal**: Separate endpoint optimised for text editor workflows, distinct from window spawning.

**TODO (`tools/api_server/schemas.py`)**:
```python
class MonodrawImportRequest(BaseModel):
    """Import Monodraw layers into text editor (not as separate windows)."""
    file_path: str
    layers: Optional[List[str]] = None  # Layer names, None = preview all
    mode: Literal["replace", "append", "insert"] = "replace"
    flatten: bool = True
    target_window: Literal["auto", "new"] | str = "auto"  # "auto", "new", or window_id
    insert_header: bool = True  # Add "// Imported from {filename}" comment
    order: Literal["source", "reverse", "custom"] = "source"

class MonodrawImportResponse(BaseModel):
    ok: bool
    window_id: str
    layers_imported: List[str]
    lines: int
    width: int  # Max line width in characters
    warnings: List[str] = []
```

**TODO (`tools/api_server/main.py`)**:
```python
@app.post("/monodraw/import", response_model=MonodrawImportResponse)
async def monodraw_import(payload: MonodrawImportRequest) -> MonodrawImportResponse:
    """Import Monodraw layers into text editor window."""
    return await ctl.import_monodraw_to_editor(payload)
```

**TODO (`tools/api_server/controller.py`)**:
```python
async def import_monodraw_to_editor(self, request: MonodrawImportRequest) -> MonodrawImportResponse:
    # Parse file using shared parser
    layers = MonodrawParser.parse_file(request.file_path)

    # Filter layers if specified
    if request.layers:
        layers = [l for l in layers if l.name in request.layers]

    # Flatten if requested
    if request.flatten:
        document = self._flatten_layers(layers, insert_header=request.insert_header, filename=request.file_path)
    else:
        # Import first layer only if not flattening
        document = layers[0].text_content if layers else ""

    # Determine target window
    if request.target_window == "new":
        window_id = await self._create_text_editor_window()
    elif request.target_window == "auto":
        # Find existing text editor or create new
        window_id = await self._get_or_create_text_editor()
    else:
        window_id = request.target_window

    # Send text to editor via IPC
    await self.send_text(window_id, document, request.mode, "end")

    # Calculate response metadata
    lines = document.count('\n') + 1
    width = max(len(line) for line in document.split('\n')) if document else 0

    return MonodrawImportResponse(
        ok=True,
        window_id=window_id,
        layers_imported=[l.name for l in layers],
        lines=lines,
        width=width
    )
```

---

## Parser Sharing: Python ↔ C++ Unification

### Current State (Divergence Risk)

**Problem**: Parser exists only in Python (`tools/api_server/monodraw_parser.py`). Future C++ menu workflows will need identical parsing logic, risking divergence.

**Current Implementation**:
```python
# tools/api_server/monodraw_parser.py:1-156
class MonodrawParser:
    @staticmethod
    def parse_file(file_path: str) -> List[MonodrawLayer]:
        # JSON parsing, object_id lookup, layer extraction
        ...
```

### Proposed Refactoring (Phase 2)

**Goal**: Single source of truth for Monodraw parsing, callable from both Python (API server) and C++ (TUI app).

#### Option 1: Shared C++ Module (Preferred)

**Structure**:
```
shared/monodraw/
  ├── monodraw_parser.h       # C++ API
  ├── monodraw_parser.cpp     # Implementation (uses rapidjson/nlohmann)
  └── monodraw_types.h        # POD structs: MonodrawDocument, MonodrawLayer
```

**C++ API** (`shared/monodraw/monodraw_parser.h`):
```cpp
namespace monodraw {

struct MonodrawLayer {
    std::string name;
    int origin_x, origin_y;
    int width, height;
    std::string text_content;
    std::string object_id;
};

struct MonodrawDocument {
    int version;  // header.v
    std::vector<MonodrawLayer> layers;
    std::pair<int, int> canvas_bounds;
};

// Parse .monojson file, return document
// Throws MonodrawParseError on invalid JSON/structure
MonodrawDocument parse_file(const std::string& file_path);

// Parse in-memory JSON string
MonodrawDocument parse_json(const std::string& json_content);

}  // namespace monodraw
```

**Python Wrapper** (replace `tools/api_server/monodraw_parser.py`):

**Option 1a: pybind11**
```cpp
// shared/monodraw/python_bindings.cpp
#include <pybind11/pybind11.h>
#include "monodraw_parser.h"

PYBIND11_MODULE(monodraw_parser, m) {
    py::class_<monodraw::MonodrawLayer>(m, "MonodrawLayer")
        .def_readonly("name", &monodraw::MonodrawLayer::name)
        .def_readonly("origin_x", &monodraw::MonodrawLayer::origin_x)
        // ...

    m.def("parse_file", &monodraw::parse_file);
}
```

**Option 1b: CLI Wrapper** (simpler, no pybind11 dependency)
```python
# tools/api_server/monodraw_parser.py (thin wrapper)
import subprocess, json

def parse_file(file_path: str) -> List[MonodrawLayer]:
    # Call shared C++ binary that outputs JSON
    result = subprocess.run(
        ["./shared/monodraw/monodraw_parser_cli", file_path],
        capture_output=True, text=True, check=True
    )
    doc = json.loads(result.stdout)
    return [MonodrawLayer(**layer) for layer in doc["layers"]]
```

**TODO**: Decide on approach (pybind11 vs CLI), implement shared module, migrate Python API to use it.

---

#### Option 2: JSON Schema + Dual Parsers (Fallback)

If shared C++ module is too complex, maintain separate parsers but enforce consistency via:
- **JSON schema** — Document Monodraw v5 format in `shared/monodraw/schema.json`
- **Shared test fixtures** — `test-tui/monodraw_samples/*.monojson` with expected parse results
- **CI validation** — Both Python and C++ parsers must produce identical output for test files

**TODO**: If choosing this option, document schema and add cross-language parser tests.

---

## Menu-Driven Workflow (C++ TUI)

### Phase 2: Direct Editor Import from Menu

**Goal**: Users can import Monodraw files without using API, via native TUI menu/dialog.

**Implementation Sketch** (`test-tui/test_pattern_app.cpp`):

```cpp
// NEW menu entry
TMenuItem& monodrawMenu = *new TSubMenu("~M~onodraw", kbAltM) +
    *new TMenuItem("~I~mport to Editor...", cmMonodrawImport, kbCtrlShiftI);

// Command handler
void TTestPatternApp::handleEvent(TEvent& event) {
    if (event.what == evCommand) {
        switch (event.message.command) {
            case cmMonodrawImport:
                handleMonodrawImport();
                clearEvent(event);
                break;
        }
    }
    TApplication::handleEvent(event);
}

void TTestPatternApp::handleMonodrawImport() {
    // 1. Open file dialog filtered to .monojson
    std::string file_path = selectMonodrawFile();
    if (file_path.empty()) return;

    // 2. Parse file using shared parser
    monodraw::MonodrawDocument doc = monodraw::parse_file(file_path);

    // 3. Show layer preview dialog (multi-select list)
    std::vector<int> selected_indices = showLayerPreviewDialog(doc.layers);
    if (selected_indices.empty()) return;

    // 4. Flatten selected layers
    std::string flattened = flattenLayers(doc.layers, selected_indices);

    // 5. Spawn text editor window with content
    TTextEditorWindow* editor = createTextEditorWindow();
    editor->sendText(flattened, "replace", "start");
    editor->setTitle(std::string("Monodraw: ") + basename(file_path));
}
```

**TODO**: Implement `selectMonodrawFile()`, `showLayerPreviewDialog()`, `flattenLayers()` helper functions.

**TODO**: Design `TMonodrawLayerPreviewDialog` — TListBox showing layers with checkboxes, metadata badges (origin, size).

---

## Updated Future Phases

### ✅ Phase 1: Window Loader (Completed — Sept 2024)
- [x] Python-only Monodraw JSON parser
- [x] REST API endpoints `/monodraw/load`, `/monodraw/parse`
- [x] Spawn positioned `text_view` windows
- [x] Coordinate scaling, offset support
- [x] Demo file with 4 primer layers

### 🚧 Phase 2: Text Editor Import (In Progress)
**Milestones from [`monodraw-text-editor-import-prd.md`](./monodraw-text-editor-import-prd.md)**:

#### Phase 2a: Shared Parser Module
- [ ] Refactor `monodraw_parser.py` into shared C++ module (`shared/monodraw/`)
- [ ] Python wrapper (pybind11 or CLI) to maintain API compatibility
- [ ] Cross-language parser tests (identical output for Python/C++)
- [ ] Update API server to use shared parser

#### Phase 2b: API Evolution
- [ ] Add `target` parameter to `/monodraw/load` (route to editor vs windows)
- [ ] Implement `/monodraw/import` endpoint for editor-specific workflows
- [ ] Add `cmd:spawn_text_editor_with_content` IPC command
- [ ] Layer filtering logic (`layers` parameter)
- [ ] Layer flattening with customizable separators
- [ ] Insert header comments (filename, layer names, timestamp)

#### Phase 2c: Menu Workflow
- [ ] Add `File → Import Monodraw…` menu entry
- [ ] File dialog filtered to `.monojson` files
- [ ] `TMonodrawLayerPreviewDialog` — Multi-select layer list with metadata
- [ ] Implement `flattenLayers()` helper in C++
- [ ] Integrate with `createTextEditorWindow()` + `sendText()`

#### Phase 2d: UX Polish
- [ ] Status line feedback ("Imported 3 layers in 42ms")
- [ ] Error modals for malformed files, missing layers
- [ ] "Re-import last file" shortcut
- [ ] Window title metadata (layer names, filename)

### 🔮 Phase 3: Advanced Features (Future)
**From editor import PRD**:
- [ ] `.monodraw` ZIP bundle support (extract `document.json`)
- [ ] Metadata gutter overlays in editor (layer origin, size badges)
- [ ] Custom layer ordering (drag-and-drop reorder in preview dialog)
- [ ] Syntax highlighting for layer separator comments
- [ ] Workspace persistence (save import session metadata)
- [ ] "Open separately" mode (one editor window per layer, cascaded)

### 🔮 Phase 4: Ecosystem Integration (Vision)
- [ ] **Bidirectional sync**: Export TUI layout back to Monodraw JSON
- [ ] **Live reload**: Watch `.monojson` file, auto-reimport on change
- [ ] **CLI tool**: `tvision-import-monodraw design.monojson --editor`
- [ ] **Monodraw plugin**: Preview TUI rendering inside Monodraw app
- [ ] **Animation support**: Keyframed layer positions → Timeline API

---

## Testing Strategy Updates

### New Test Requirements (Phase 2)

**Shared Parser Module**:
- [ ] C++ unit tests (`test/monodraw/test_parser.cpp`) — Parse demo file, validate layer extraction
- [ ] Python wrapper tests — Identical output to C++ parser for all fixtures
- [ ] Cross-language regression suite — JSON schema validation

**API Endpoints**:
- [ ] `POST /monodraw/import` with `target: "text_editor"` → 200 OK, editor window spawned
- [ ] `POST /monodraw/load?target=text_editor` → Same behaviour as `/monodraw/import`
- [ ] Layer filtering: `layers: ["symbient-city"]` → Only selected layer imported
- [ ] Flatten mode: `flatten: true` → Multiple layers merged with separators
- [ ] Invalid target window ID → 404 error

**Menu Workflow**:
- [ ] Manual QA: Import via `File → Import Monodraw…`, verify content matches Monodraw preview
- [ ] Layer preview dialog: Multi-select, reorder, verify final document order
- [ ] Flatten vs separate modes: Confirm behaviour matches user choice
- [ ] Unicode handling: Import layers with emoji, box-drawing chars → No corruption

**Integration**:
- [ ] End-to-end: Monodraw → Export JSON → API import → TUI editor → Edit → Save
- [ ] Performance: Large file (50+ layers) imports in <3 seconds
- [ ] Memory: No temp file leaks, verify cleanup on exit

---

## Documentation Updates Required

### Existing Docs to Update
- [ ] `CLAUDE.md` — Add text editor import workflow, update API examples
- [ ] `tools/api_server/README.md` — Document `/monodraw/import` endpoint
- [ ] `test-tui/README.md` — Add "Import Monodraw" menu usage instructions

### New Docs to Create
- [ ] `docs/monodraw-workflows.md` — Complete guide: Window loader vs Editor import use cases
- [ ] `shared/monodraw/README.md` — Parser module API documentation (C++ and Python)
- [ ] `test-tui/MONODRAW_IMPORT.md` — Tutorial with screenshots, sample files

### Sample Assets
- [ ] Add diverse fixtures to `test-tui/monodraw_samples/`:
  - `single-layer.monojson` — Minimal test case
  - `unicode-art.monojson` — Emoji, box-drawing, wide chars
  - `large-canvas.monojson` — 200×100 design for scaling tests
  - `empty-layers.monojson` — Edge case with unnamed/empty layers

---

## Summary

**Current State**: Monodraw window loader works, spawns read-only `text_view` windows via REST API.

**Next Steps**:
1. Refactor Python parser into shared C++ module (eliminate divergence)
2. Extend API with `target: "text_editor"` parameter + `/monodraw/import` endpoint
3. Implement C++ menu workflow (`File → Import Monodraw…` with layer preview)
4. Add IPC command `spawn_text_editor_with_content` for direct content injection
5. Update Future Phases to reflect editor-import milestones (no more redundant window-type inference items)

**Key References**:
- **Window Loader Spec**: [`prds/monodraw-json-window-loader-prd.md`](./monodraw-json-window-loader-prd.md)
- **Editor Import Spec**: [`prds/monodraw-text-editor-import-prd.md`](./monodraw-text-editor-import-prd.md)
- **Implementation Files**: `tools/api_server/{main.py, controller.py, monodraw_parser.py}`, `test-tui/api_ipc.cpp`

---

**Document Status**: ✅ Refactored Summary (replaces comprehensive PRD)
**Last Updated**: 2025-10-20
**Maintained By**: Turbo Vision Community

つ◕‿◕‿⚆༽つ つ⚆‿◕‿◕༽つ
