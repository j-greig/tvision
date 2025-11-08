# Monodraw ↔ Turbo Vision Integration — Comprehensive PRD

**tl;dr**: Transform Monodraw (macOS ASCII art editor) into a visual TUI layout designer for Turbo Vision. Parse Monodraw's layered JSON exports to spawn positioned text windows, enabling rapid prototyping of ASCII art compositions and TUI interfaces. Already implemented in commit `9c63d9b` — this PRD documents the complete feature, API surface, best practices, and future enhancement pathways.

---

## Status: ✅ IMPLEMENTED (v1.0)

**Implementation Commit**: `9c63d9b` — 🎨📋✨ feat: Monodraw JSON Window Loader - Visual ASCII Art to TUI Layouts
**Files Modified**:
- `tools/api_server/main.py` — API endpoints `/monodraw/load` and `/monodraw/parse`
- `tools/api_server/controller.py` — Core loading logic, window spawning, type inference
- `tools/api_server/monodraw_parser.py` — JSON parsing, layer extraction, coordinate scaling
- `tools/api_server/schemas.py` — Pydantic models for API requests/responses

**Current Capabilities**:
- ✅ Parse Monodraw v5 JSON format (`.monojson` files)
- ✅ Extract named text frame layers (type_id=20)
- ✅ Spawn text_view windows with precise positioning
- ✅ Auto-size windows based on text content dimensions
- ✅ Coordinate scaling to fit terminal size
- ✅ Offset adjustments for layout positioning
- ✅ Preview/parse mode (dry-run without window creation)
- ✅ Explicit window type overrides via API parameter
- ✅ Error handling for missing files, corrupted JSON, invalid layers

---

## Context: Bridging Visual Design & Terminal UI

### What is Monodraw?

**Monodraw** (by Helftone) is a professional macOS ASCII art editor featuring:
- **Layer-based composition** — Like Photoshop for ASCII art
- **Precision drawing tools** — Lines, rectangles, text frames, pencil, bucket fill
- **FIGlet integration** — 148+ fonts for ASCII banners
- **Attachment points** — Dynamic connections between shapes
- **Export formats** — Plain text, PNG, SVG, and **JSON**
- **CLI support** — Automation via command-line interface (Direct version)

### Why This Integration Matters

**For ASCII Artists**:
- Design complex layouts visually in Monodraw's WYSIWYG editor
- Instantly materialise designs as live TUI windows
- Rapid prototyping without manual coordinate calculation
- Bridge static art ↔ dynamic, interactive terminal applications

**For TUI Developers**:
- Visual layout designer for Turbo Vision applications
- No need to hardcode window positions in C++
- Iterate on UI designs without recompiling
- Use Monodraw as a "TUI wireframing tool"

**For the Ecosystem**:
- Monodraw becomes a first-class tool for terminal UI design
- Lowers barrier to entry for TUI development
- Enables new workflows: design → export → deploy

---

## Technical Architecture

### Monodraw JSON Structure (v5)

Monodraw exports `.monojson` files with this structure:

```json
{
  "header": {
    "v": 5
  },
  "object_list": [
    {
      "object_id": "UUID",
      "type_id": 20,           // Text frame
      "name": "LayerName",
      "name_edited": true,     // User-renamed layer
      "origin": "x,y",         // Position string
      "frame_size": "w,h",     // Dimensions string
      "model_text": "UUID"     // Reference to text content object
    },
    {
      "object_id": "UUID",
      "type_id": 27,           // Text model
      "text": "Actual ASCII content\nMultiline text here..."
    }
    // ... other objects (lines, shapes, canvases)
  ]
}
```

**Key Object Types**:
- `type_id=1` — Canvas (base artboard)
- `type_id=11` — Line drawing
- `type_id=18` — Rectangle/shape
- `type_id=20` — **Text frame** (what we parse)
- `type_id=27` — **Text model** (content storage)

**Parser Strategy**:
1. Build `objects_by_id` lookup map from `object_list`
2. Filter for `type_id=20` with `name_edited=true` (user-named text frames)
3. Parse `origin` string → `(x, y)` coordinates
4. Dereference `model_text` UUID → retrieve actual text content
5. Calculate window size from text dimensions (lines × max_line_width)
6. Apply scaling/offset transformations
7. Return structured `MonodrawLayer` objects

---

## API Surface

### POST /monodraw/load

**Purpose**: Load Monodraw JSON file and spawn positioned TUI windows.

**Request Schema** (`MonodrawLoadRequest`):
```json
{
  "file_path": "/absolute/path/to/design.monojson",
  "scale": 1.0,                    // Coordinate scaling factor (default: 1.0)
  "offset_x": 0,                   // Horizontal offset in chars (default: 0)
  "offset_y": 0,                   // Vertical offset in chars (default: 0)
  "window_types": {                // Optional type overrides
    "LayerName": "text_view",
    "AnotherLayer": "test_pattern"
  }
}
```

**Response Schema**:
```json
{
  "ok": true,
  "windows_created": [
    {
      "id": "w42",
      "type": "text_view",
      "title": "symbient-city",
      "rect": {"x": 146, "y": 61, "w": 45, "h": 20},
      "monodraw_layer": "symbient-city"
    }
  ],
  "errors": [],                    // Error messages for failed layers
  "total_layers": 4,
  "windows_spawned": 4
}
```

**Behaviour**:
1. Parse Monodraw JSON to extract named text frame layers
2. Apply coordinate scaling if terminal size differs or `scale ≠ 1.0`
3. Apply offset to all layer positions
4. Create temporary files containing layer text content
5. Spawn `text_view` windows with `path` prop pointing to temp files
6. Return window metadata for all successfully created windows

**Error Handling**:
- File not found → `{"ok": false, "error": "File not found: ..."}`
- JSON parse error → `{"ok": false, "error": "Invalid JSON: ..."}`
- No named layers → `{"ok": false, "error": "No named layers found"}`
- Per-layer errors → Included in `errors` array, doesn't halt other windows

---

### POST /monodraw/parse

**Purpose**: Preview Monodraw file structure without creating windows (dry-run mode).

**Request Schema** (`MonodrawParseRequest`):
```json
{
  "file_path": "/absolute/path/to/design.monojson"
}
```

**Response Schema**:
```json
{
  "ok": true,
  "layers": [
    {
      "name": "symbient-city",
      "position": {"x": 146, "y": 61},
      "size": {"w": 45, "h": 20},
      "content_preview": "First 100 chars of text content...",
      "suggested_type": "text_view",
      "confidence": 1.0
    }
  ],
  "canvas_bounds": {"w": 250, "h": 120}
}
```

**Use Cases**:
- Inspect Monodraw file before spawning windows
- Validate layer names and positions
- Calculate required terminal size for design
- Debug coordinate scaling issues

---

## Implementation Details

### Coordinate System Translation

**Monodraw Coordinates**: Character-based (x, y) in Monodraw canvas space
**TUI Coordinates**: Terminal columns/rows (0-indexed from top-left)

**Scaling Algorithm**:
```python
def scale_coordinates(layers, scale_factor=1.0, terminal_size=(80, 24)):
    canvas_w, canvas_h = get_canvas_bounds(layers)
    term_w, term_h = terminal_size

    # Auto-scale if design exceeds terminal size
    if scale_factor == 1.0 and (canvas_w > term_w or canvas_h > term_h):
        scale_factor = min(term_w / canvas_w, term_h / canvas_h) * 0.9

    # Scale positions only (not window sizes)
    for layer in layers:
        layer.origin = (
            int(layer.origin[0] * scale_factor),
            int(layer.origin[1] * scale_factor)
        )

    return layers
```

**Design Decision**: Window sizes are **NOT** scaled — they auto-size based on text content dimensions. Only *positions* are scaled to fit terminal. This preserves readability of ASCII art content.

---

### Window Type Inference

**Current Implementation** (`controller.py:679-690`):
```python
def _infer_window_type(self, layer: MonodrawLayer, explicit_types: Dict[str, str]):
    # Check explicit overrides first
    if layer.name in explicit_types:
        return WindowType(explicit_types[layer.name])

    # Default: text_view for all content
    # TODO: Pattern detection for test_pattern, gradient, etc.
    return WindowType.text_view
```

**Future Enhancement Pathways** (see "Phase 2" below):
- Detect gradient characters (`░▒▓█`) → `gradient` type
- Detect box-drawing patterns → `test_pattern` type
- Empty layers → `wallpaper` or frame-only windows
- Layer name conventions: `gradient:*`, `pattern:*`, `editor:*`

---

### Content Processing Pipeline

```ascii
┌─────────────────┐
│ Monodraw JSON   │
│  (.monojson)    │
└────────┬────────┘
         │
         ▼
┌─────────────────────────────────────┐
│ MonodrawParser.parse_file()         │
│  • Load JSON                        │
│  • Build object_by_id lookup        │
│  • Filter type_id=20, name_edited   │
└────────┬────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────┐
│ Extract Layer Data                  │
│  • Parse origin "x,y" → (x, y)     │
│  • Dereference model_text → text    │
│  • Calculate size from content      │
│  • Return MonodrawLayer objects     │
└────────┬────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────┐
│ scale_coordinates()                 │
│  • Apply scaling factor             │
│  • Auto-scale if exceeds terminal   │
│  • Preserve text-based window sizes │
└────────┬────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────┐
│ Apply Offset (if specified)         │
│  • Shift all origins by (Δx, Δy)    │
└────────┬────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────┐
│ Spawn Windows                       │
│  • Create temp file per layer       │
│  • Infer/override window type       │
│  • Call create_window() for each    │
│  • Collect results + errors         │
└────────┬────────────────────────────┘
         │
         ▼
┌─────────────────┐
│ TUI Windows     │
│ (text_view)     │
└─────────────────┘
```

---

## Demo File Analysis

**Reference File**: `test-tui/monodraw-demo-simple-primers.monojson`

**Layers** (4 named text frames):

| Layer Name         | Origin      | Content Size | Description                          |
|--------------------|-------------|--------------|--------------------------------------|
| `symbient-city`    | (146, 61)   | 1145 chars   | Geometric architectural blocks       |
| `chaos-vs-order`   | (168, 45)   | 565 chars    | Chaotic vs ordered systems diagram   |
| `cat-cat-simple`   | (141, 45)   | 130 chars    | Simple ASCII cat art                 |
| `time-shamans`     | (208, 24)   | 2477 chars   | Pyramid structures with TIME STREAMS |

**Load Command**:
```bash
curl -X POST "http://127.0.0.1:8089/monodraw/load" \
  -H "Content-Type: application/json" \
  -d '{
    "file_path": "/Users/james/Repos/tvision/test-tui/monodraw-demo-simple-primers.monojson",
    "scale": 0.5,
    "offset_x": 10,
    "offset_y": 5
  }'
```

**Expected Behaviour**:
- Parse 4 named text frames from 22 total objects
- Scale coordinates by 0.5× to fit smaller terminals
- Offset all windows by (10, 5) characters
- Create 4 text_view windows with primer content
- Return window IDs and final positions

---

## Best Practices for Monodraw → TUI Workflow

### 1. Layer Naming Conventions

**Always rename layers** in Monodraw before exporting:
- Unnamed layers are **ignored** by the parser
- Use descriptive names: `header`, `sidebar`, `content-panel`
- Name prefixes for type hints: `gradient:sunset`, `pattern:checkerboard`

### 2. Design for Terminal Constraints

**Typical terminal sizes**:
- Small: 80×24 (classic)
- Medium: 120×30
- Large: 160×40
- Ultra-wide: 200×50

**Auto-scaling triggers** when canvas bounds exceed terminal size:
- Scale factor = `min(term_w/canvas_w, term_h/canvas_h) × 0.9`
- 0.9 multiplier provides 10% margin for frame borders

### 3. Text Frame Best Practices

**In Monodraw**:
- Use text frames (⌘T) for content that will become windows
- Position frames precisely — coordinates map 1:1 to TUI (pre-scaling)
- Preview in monospace font (Courier, Menlo, SF Mono)
- Test at target terminal size before exporting

**Avoid**:
- Overlapping text frames (will cascade in TUI)
- Frames outside canvas bounds (may be clipped)
- Empty unnamed frames (ignored by parser)

### 4. Export Settings

**File format**: JSON export (not text/PNG/SVG)
- **File → Export → JSON** (or use CLI: `monodraw export --json`)
- Save as `.monojson` extension for clarity
- Include in version control for reproducible layouts

### 5. Iteration Workflow

```ascii
Design in Monodraw → Export JSON → Load via API → Review in TUI → Iterate
        ▲                                                             │
        └─────────────────────────────────────────────────────────────┘
```

**Fast iteration**:
1. Keep Monodraw file open during development
2. Edit layout, save as JSON
3. Re-call `/monodraw/load` API (auto-closes old windows)
4. Instant visual feedback in running TUI app
5. Refine positions/content in Monodraw
6. Repeat

---

## ASCII Artist Perspective: Advanced Techniques

### Compositional Strategies

**Grid-based layouts**:
- Design primers/panels in regular grid
- Use Monodraw's alignment guides (⌘;)
- Consistent spacing enables clean tile layouts

**Layered depth**:
- Foreground: Sharp ASCII art, high contrast
- Midground: Medium detail, text content
- Background: Gradients, subtle patterns (future)

**Typography integration**:
- Use FIGlet fonts in Monodraw → Export as text frames
- Map to TUI `text_view` windows with figlet content
- API supports `/text_editor/send_figlet` for dynamic updates

### Artistic Enhancements (Future)

**Gradient windows** (when type inference implemented):
```
░░▒▒▓▓██  →  Detected as gradient, spawned as gradient window
```

**Pattern windows** (geometric ASCII):
```
┌─┬─┬─┐
├─┼─┼─┤  →  Detected as pattern, spawned as test_pattern window
└─┴─┴─┘
```

**Wallpaper layers** (background fills):
```
Empty layer named "wallpaper:*" → Spawned as full-screen wallpaper window
```

---

## Implementation Phases (Historical & Future)

### ✅ Phase 1: Core Parser (Completed — Sept 2024)
- [x] Basic JSON parsing for `type_id=20` text frames
- [x] Layer extraction with `name`, `origin`, `model_text` dereferencing
- [x] Simple coordinate mapping with 1:1 scaling
- [x] Auto-sizing windows based on text content dimensions
- [x] Single API endpoint `/monodraw/load`
- [x] Error handling for missing files, invalid JSON
- [x] Demo file: `monodraw-demo-simple-primers.monojson`

### ✅ Phase 1.5: Preview & Scaling (Completed — Sept 2024)
- [x] Preview endpoint `/monodraw/parse` (dry-run mode)
- [x] Coordinate scaling to fit terminal size
- [x] Offset support for layout positioning
- [x] Explicit window type overrides via API parameter
- [x] Canvas bounds calculation

### 🔮 Phase 2: Advanced Inference (Future)
- [ ] Pattern recognition for `test_pattern` windows (box-drawing chars)
- [ ] Gradient detection for `gradient` windows (`░▒▓█` sequences)
- [ ] FIGlet text detection (banner fonts)
- [ ] Layer name convention parsing (`type:name` format)
- [ ] Content preprocessing (strip trailing whitespace, normalize line endings)
- [ ] Confidence scoring for type inference

### 🔮 Phase 3: Production Features (Future)
- [ ] Workspace integration: Save Monodraw layouts as TUI workspaces
- [ ] Live reload: Watch `.monojson` file for changes, auto-reload
- [ ] Performance optimisation for large files (>50 layers)
- [ ] Streaming JSON parser for very large exports
- [ ] Validation: Warn about overlapping frames, out-of-bounds positions
- [ ] CLI tool: `tui-load-monodraw design.monojson`

### 🔮 Phase 4: Bidirectional Sync (Vision)
- [ ] **TUI → Monodraw export**: Reverse operation (save TUI window layout as Monodraw JSON)
- [ ] Live sync: Drag window in TUI → Update Monodraw layer position
- [ ] Round-trip workflow: Design → Load → Modify → Export → Refine
- [ ] Monodraw plugin/extension for direct TUI preview

---

## Testing Strategy

### Unit Tests (Python)

**`test_monodraw_parser.py`**:
- [x] Parse valid Monodraw v5 JSON
- [x] Extract named text frames correctly
- [x] Handle missing `name_edited` attribute
- [x] Parse `origin` string → (x, y) tuple
- [x] Dereference `model_text` UUID to content
- [ ] Handle malformed `origin` strings gracefully
- [ ] Calculate canvas bounds from layer positions
- [ ] Scale coordinates with various factors (0.5, 1.0, 2.0)

**`test_coordinate_scaling.py`**:
- [ ] Auto-scale when design exceeds terminal size
- [ ] Preserve window sizes (only scale positions)
- [ ] Apply offset correctly
- [ ] Handle negative coordinates (clamp to 0?)

**`test_window_type_inference.py`**:
- [ ] Detect gradient patterns in text content
- [ ] Detect box-drawing patterns
- [ ] Respect explicit type overrides
- [ ] Default to `text_view` when uncertain

### Integration Tests (API)

**`test_monodraw_endpoints.py`**:
- [x] POST `/monodraw/load` with valid file → 200 OK, windows created
- [x] POST `/monodraw/parse` → 200 OK, layer previews returned
- [ ] POST `/monodraw/load` with missing file → 400 error
- [ ] POST `/monodraw/load` with corrupted JSON → 400 error
- [ ] POST `/monodraw/load` with no named layers → 400 error
- [ ] Verify temporary files created for text content
- [ ] Verify window positions match scaled coordinates

### Manual Testing (User Acceptance)

**Workflow validation**:
1. Open `test-tui/monodraw-demo-simple-primers.monojson` in Monodraw
2. Verify 4 named text frames visible in layer panel
3. Export as JSON (should match existing file)
4. Run `test_pattern` TUI app
5. Call `/monodraw/load` API with demo file
6. **Expected**: 4 text windows spawn at correct positions
7. **Expected**: Window content matches Monodraw text frames
8. **Expected**: Layouts align with Monodraw visual preview

**Edge case testing**:
- Very large designs (>200×100 canvas) → Auto-scaling works
- Designs with Unicode art → Characters render correctly
- Designs with ANSI color codes (if Monodraw supports) → Colors preserved?
- Rapid re-loading same file → Old windows closed, new ones spawned

---

## Performance Considerations

### Current Bottlenecks

**File I/O**:
- JSON parsing: `json.load()` — O(n) where n = file size
- Temp file creation: One file per layer (4 files for demo)
- Recommendation: Stream large JSON files, pool temp file creation

**Window Creation**:
- Sequential window spawning via IPC (Unix socket)
- Each `create_window()` call waits for C++ app response
- Recommendation: Batch window creation API (future enhancement)

**Coordinate Scaling**:
- O(n) iteration over all layers
- Negligible for typical designs (<100 layers)
- Pre-compute scale factor once, apply to all layers

### Scalability Targets

| Design Size       | Layers | Expected Load Time | Status        |
|-------------------|--------|---------------------|---------------|
| Small (demo)      | 4-10   | <0.5s               | ✅ Achieves   |
| Medium (UI mock)  | 10-30  | <1.5s               | ✅ Expected   |
| Large (artwork)   | 30-100 | <5s                 | 🔮 Untested   |
| Huge (atlas)      | 100+   | <15s                | 🔮 Future     |

### Optimisation Strategies (Future)

**Parallel window creation**:
- Spawn windows concurrently using `asyncio.gather()`
- Requires batch IPC command support in C++ app

**Lazy content loading**:
- Don't create temp files until window is visible/focused
- Stream text content on-demand

**Caching**:
- Cache parsed Monodraw JSON by file path + mtime
- Reuse cached layers if file unchanged

---

## Error Handling & Edge Cases

### File System Errors

| Error                     | Handling                                      |
|---------------------------|-----------------------------------------------|
| File not found            | Return `{"ok": false, "error": "File not found: ..."}` |
| Permission denied         | Return `{"ok": false, "error": "Permission denied: ..."}` |
| Corrupted JSON            | Return `{"ok": false, "error": "Invalid JSON: ..."}` |
| Empty file                | Return `{"ok": false, "error": "Empty Monodraw file"}` |

### Content Issues

| Issue                     | Handling                                      |
|---------------------------|-----------------------------------------------|
| No named layers           | Return `{"ok": false, "error": "No named layers found"}` |
| Missing `model_text` ref  | Skip layer, add to `errors` array             |
| Empty text content        | Create window with empty content (blank window) |
| Invalid `origin` string   | Default to `(0, 0)`, log warning              |
| Overlapping windows       | No special handling — spawn all, user can rearrange |

### Coordinate Edge Cases

| Case                      | Handling                                      |
|---------------------------|-----------------------------------------------|
| Negative coordinates      | Allow (may be off-screen, user can move)      |
| Coordinates beyond terminal | Allow (window may be partially visible)    |
| Very large window size    | Clamp to terminal bounds (future)             |
| Zero-size window          | Minimum 1×1 dimension                         |

### Recovery Strategies

**Partial failures**:
- If 3 of 4 layers fail, still create the 1 successful window
- Return both `windows_created` and `errors` arrays
- User can inspect errors, fix Monodraw file, retry

**Graceful degradation**:
- If type inference fails → Default to `text_view`
- If scaling fails → Use 1:1 coordinates (no scaling)
- If temp file creation fails → Pass text content inline (future)

---

## Security Considerations

### File Path Validation

**Risk**: Path traversal attacks via `file_path` parameter
**Mitigation**:
- Resolve absolute paths using `os.path.abspath()`
- Reject paths outside allowed directories (future: whitelist)
- Sanitise filenames before creating temp files

### Temp File Management

**Risk**: Temp file leaks, disk space exhaustion
**Mitigation**:
- Use `tempfile.NamedTemporaryFile()` with `delete=False` (manual cleanup)
- Track created temp files, clean up on shutdown
- Limit total temp file count per session (future)

### JSON Parsing

**Risk**: Malicious JSON causing DoS (deeply nested, huge files)
**Mitigation**:
- Limit file size to 10MB (future: configurable)
- Use `json.load()` with depth limits (future)
- Timeout for parsing operations (future)

### Content Injection

**Risk**: Malicious text content breaking TUI rendering
**Mitigation**:
- Sanitise control characters before passing to C++ app
- Validate UTF-8 encoding (reject invalid sequences)
- Escape ANSI codes unless explicitly allowed

---

## API Usage Examples

### Example 1: Simple Load

```bash
curl -X POST "http://127.0.0.1:8089/monodraw/load" \
  -H "Content-Type: application/json" \
  -d '{
    "file_path": "/Users/james/Repos/tvision/test-tui/monodraw-demo-simple-primers.monojson"
  }'
```

**Response**:
```json
{
  "ok": true,
  "windows_created": [
    {"id": "w1", "type": "text_view", "title": "symbient-city", "rect": {...}},
    {"id": "w2", "type": "text_view", "title": "chaos-vs-order", "rect": {...}},
    {"id": "w3", "type": "text_view", "title": "cat-cat-simple", "rect": {...}},
    {"id": "w4", "type": "text_view", "title": "time-shamans", "rect": {...}}
  ],
  "errors": [],
  "total_layers": 4,
  "windows_spawned": 4
}
```

---

### Example 2: Scaled Load for Small Terminal

```python
import requests

api_base = "http://127.0.0.1:8089"

# Load design scaled to 50% for 80×24 terminal
response = requests.post(f"{api_base}/monodraw/load", json={
    "file_path": "/path/to/large-design.monojson",
    "scale": 0.5,
    "offset_x": 5,
    "offset_y": 2
})

print(f"Created {response.json()['windows_spawned']} windows")
```

---

### Example 3: Preview Before Loading

```bash
# Step 1: Preview layer structure
curl -X POST "http://127.0.0.1:8089/monodraw/parse" \
  -H "Content-Type: application/json" \
  -d '{"file_path": "/path/to/design.monojson"}'

# Output shows:
# {
#   "layers": [
#     {"name": "header", "position": {"x": 10, "y": 5}, "size": {"w": 60, "h": 3}},
#     {"name": "sidebar", "position": {"x": 5, "y": 10}, "size": {"w": 20, "h": 30}},
#     ...
#   ],
#   "canvas_bounds": {"w": 150, "h": 80}
# }

# Step 2: Decide on scaling based on terminal size (120×30)
# Canvas is 150×80, so scale = min(120/150, 30/80) * 0.9 ≈ 0.34

# Step 3: Load with calculated scale
curl -X POST "http://127.0.0.1:8089/monodraw/load" \
  -H "Content-Type: application/json" \
  -d '{
    "file_path": "/path/to/design.monojson",
    "scale": 0.34
  }'
```

---

### Example 4: Explicit Window Type Overrides

```python
# Load design but override certain layers to use gradient windows
response = requests.post("http://127.0.0.1:8089/monodraw/load", json={
    "file_path": "/path/to/mixed-design.monojson",
    "window_types": {
        "background-gradient": "gradient",
        "pattern-test": "test_pattern",
        "editor-panel": "text_editor"
    }
})

# Layers named "background-gradient", "pattern-test", "editor-panel"
# will spawn as their specified types instead of default text_view
```

---

## Integration with Existing Systems

### Workspace System

**Future enhancement**: Save Monodraw-loaded layouts as TUI workspaces

```python
# Load Monodraw design
response = requests.post("/monodraw/load", json={"file_path": "design.monojson"})

# Save resulting window layout as workspace
requests.post("/workspace/save", json={"path": "layouts/my-design.json"})

# Later: Reload workspace without re-parsing Monodraw file
requests.post("/workspace/open", json={"path": "layouts/my-design.json"})
```

**Benefits**:
- Faster load times (no JSON parsing)
- Persist manual window adjustments made after loading
- Version control for layout iterations

### Primer System

**Current behaviour**: Text content from Monodraw layers passed as temp files

**Alternative approach** (future):
- Save layer content as permanent primer files: `primers/monodraw-layer-{name}.txt`
- Update `text_view` windows to reference primer paths
- Enables sharing Monodraw-extracted content as reusable primers

### Screenshot System

**Use case**: Capture loaded Monodraw layout as PNG for documentation

```bash
# Load Monodraw design
curl -X POST "/monodraw/load" -d '{"file_path": "design.monojson"}'

# Take screenshot
curl -X POST "/screenshot" -d '{"path": "screenshots/monodraw-layout.png"}'
```

**Use case**: Compare Monodraw preview vs TUI rendering
→ Automated visual regression testing

---

## Troubleshooting Guide

### Problem: "No named layers found"

**Cause**: Layers in Monodraw file are not user-renamed
**Solution**: In Monodraw, double-click each text frame, rename in inspector panel

### Problem: Windows spawn outside terminal bounds

**Cause**: Monodraw canvas larger than terminal size
**Solution**: Use `scale` parameter or enable auto-scaling:
```json
{"file_path": "...", "scale": 0.5}
```

### Problem: Text content missing/blank windows

**Cause**: `model_text` reference broken or missing
**Solution**: Re-export Monodraw file, ensure text frames have content

### Problem: Windows overlap in TUI

**Cause**: Monodraw layers designed close together
**Solution**: Use `offset_x`/`offset_y` to shift layout, or manually cascade in TUI

### Problem: Unicode characters render incorrectly

**Cause**: Terminal doesn't support UTF-8 or specific glyphs
**Solution**: Test terminal with `echo "▓▒░█"`, ensure locale set to UTF-8

---

## Conclusion & Vision

### What We've Built

The Monodraw integration transforms a **static ASCII art editor** into a **dynamic TUI layout designer**. Artists and developers can now:
- Design complex terminal interfaces visually
- Export designs as JSON with one click
- Load into running TUI applications instantly
- Iterate on layouts without touching code

This bridges two worlds:
- **Visual thinkers** who prototype in WYSIWYG editors
- **Terminal purists** who build in C++ and Turbo Vision

### Future Vision

**Short-term** (6 months):
- Advanced window type inference (gradients, patterns)
- Live file watching and auto-reload
- Performance optimisation for large designs
- CLI tool for loading Monodraw files

**Long-term** (1-2 years):
- **Bidirectional sync**: Edit TUI layout → Update Monodraw file
- **Monodraw plugin**: Preview TUI rendering inside Monodraw
- **Template library**: Pre-built TUI layouts for common patterns
- **Animation support**: Keyframed layer positions → Timeline-based window movement

### Call to Action

**For ASCII Artists**:
- Experiment with Monodraw → TUI workflows
- Share designs that push the boundaries of terminal art
- Contribute pattern detection algorithms for type inference

**For TUI Developers**:
- Use Monodraw for rapid UI prototyping
- Build tools that export to Monodraw format
- Integrate Monodraw-based layouts into production TUI apps

**For the Ecosystem**:
- Document best practices as patterns emerge
- Build a gallery of Monodraw → TUI showcase projects
- Advocate for Monodraw as a first-class TUI design tool

---

## References

### Documentation
- Monodraw Official Site: https://monodraw.helftone.com/
- Turbo Vision Docs: `CLAUDE.md` (this repository)
- API Server Docs: `tools/api_server/README.md`

### Related PRDs
- `monodraw-json-window-loader-prd.md` — Original implementation spec
- `primer-discovery-api-prd.md` — Primer file system integration
- `universal-window-backgrounds-prd.md` — Gradient/wallpaper windows

### Implementation Files
- `tools/api_server/monodraw_parser.py` — Core parsing logic
- `tools/api_server/controller.py:570-714` — Load/parse endpoints
- `tools/api_server/schemas.py:53-63` — API request/response models
- `test-tui/monodraw-demo-simple-primers.monojson` — Reference demo file

### Version History
- **v1.0** (Sept 2024, commit `9c63d9b`) — Initial implementation
  - Basic JSON parsing, text frame extraction
  - Coordinate scaling, offset support
  - Load and parse endpoints
  - Demo file with 4 primer layers

---

**Document Status**: ✅ Complete
**Last Updated**: 2025-10-20
**Author**: Wib & Wob (つ◕‿◕‿⚆༽つ つ⚆‿◕‿◕༽つ)
**Maintained By**: Turbo Vision Community

---

*This PRD serves as both historical documentation of the implemented feature and a living roadmap for future enhancements. The Monodraw integration represents a unique bridge between visual design and terminal-based development — a testament to the creative possibilities when ASCII art meets functional TUI applications.*

*Long live the character grid. Long live the terminal. Long may the pixels dance in monospace.*

🎨📋✨
