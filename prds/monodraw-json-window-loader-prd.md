# Monodraw JSON Window Loader PRD

**tl;dr**: Parse Monodraw JSON files to auto-spawn TUI windows based on layer positions, creating instant ASCII art compositions from Monodraw designs. Each named layer becomes a positioned window with content derived from its tixels data, enabling rapid TUI layout prototyping from visual designs.

## Context

Monodraw is a macOS ASCII art editor that exports layered designs as JSON files. These files contain positioned text elements with precise coordinate data. The Turbo Vision TUI framework has a programmatic window control API that can create and position windows with pixel-perfect placement. This creates an opportunity to bridge visual ASCII art design with functional TUI layouts.

## Objective

Create a feature that parses Monodraw JSON files and spawns corresponding TUI windows, where each Monodraw layer becomes a positioned window in the TUI application. This enables designers to visually layout TUI interfaces in Monodraw and instantly materialise them as working TUI applications.

## Requirements

### Core Functionality
• Parse Monodraw JSON format to extract layer metadata and content
• Map each named layer to a TUI window with appropriate positioning
• Support window type inference based on layer content and naming conventions
• Maintain visual fidelity between Monodraw design and TUI output
• Handle coordinate system translation (Monodraw to terminal coordinates)

### API Integration
• Extend existing REST API with Monodraw-specific endpoints
• Leverage existing window creation API (POST /windows)
• Support batch window creation for performance
• Integrate with existing workspace save/load system

### Content Mapping
• Extract text content from tixels arrays for text_view windows
• Detect patterns for gradient window creation
• Support ASCII art content for test_pattern windows
• Handle empty layers as placeholder frames

### Error Handling
• Validate JSON structure and required fields
• Handle missing or corrupted layer data gracefully
• Provide clear error messages for unsupported content
• Fallback to basic window types when inference fails

## Technical Specification

### Monodraw JSON Structure Analysis

Based on examination of the sample file, Monodraw JSON contains:

```json
{
  "header": { "v": 5 },
  "object_list": [
    {
      "name": "Layer Name",
      "object_id": "UUID",
      "point": "x,y",
      "tixels": {
        "text_content": ["array", "of", "characters"]
      }
    }
  ]
}
```

### Window Type Inference Rules

**Text Content Windows**:
- Layers with substantial tixels.text_content → `text_view` type
- Content assembled from text_content array into readable text

**Pattern Windows**:
- Layers with geometric ASCII patterns → `test_pattern` type
- Box drawing characters, repeated patterns, borders

**Gradient Windows**:
- Layers with gradient-like character progression → `gradient` type
- Shading characters: `░▒▓█` or similar patterns

**Frame Windows**:
- Empty layers or simple borders → basic frames for layout structure

### API Endpoints

#### POST /monodraw/load
Load and spawn windows from Monodraw JSON file:
```json
{
  "file_path": "/path/to/design.monojson",
  "scale": 1.0,
  "offset": {"x": 0, "y": 0},
  "window_types": {
    "Layer Name": "text_view",
    "Pattern Layer": "test_pattern"
  }
}
```

Response:
```json
{
  "windows_created": [
    {"id": "w1", "type": "text_view", "title": "Layer Name", "rect": {...}},
    {"id": "w2", "type": "test_pattern", "title": "Pattern Layer", "rect": {...}}
  ],
  "errors": [],
  "total_layers": 4,
  "windows_spawned": 2
}
```

#### GET /monodraw/parse
Parse Monodraw file without creating windows (preview mode):
```json
{
  "file_path": "/path/to/design.monojson"
}
```

Response:
```json
{
  "layers": [
    {
      "name": "Text Field 1",
      "position": {"x": 10, "y": 5},
      "size": {"w": 30, "h": 8},
      "content_preview": "Sample text content...",
      "suggested_type": "text_view",
      "confidence": 0.9
    }
  ],
  "canvas_bounds": {"w": 160, "h": 40}
}
```

### Implementation Architecture

#### Python API Server Changes

**New modules**:
- `monodraw_parser.py` - JSON parsing and layer extraction
- `window_type_inference.py` - Content analysis and type detection
- `coordinate_mapper.py` - Coordinate system translation

**Enhanced endpoints in `controller.py`**:
```python
@app.post("/monodraw/load")
async def load_monodraw_file(request: MonodrawLoadRequest):
    # Parse JSON file
    layers = MonodrawParser.parse_file(request.file_path)
    
    # Apply scaling and offset
    transformed_layers = CoordinateMapper.transform(layers, request.scale, request.offset)
    
    # Infer window types
    typed_layers = WindowTypeInference.analyse_layers(transformed_layers, request.window_types)
    
    # Batch create windows via existing API
    windows = await batch_create_windows(typed_layers)
    
    return MonodrawLoadResponse(windows_created=windows)
```

#### C++ IPC Integration

**Extend existing api_ipc.cpp**:
- No major C++ changes required - leverage existing window creation commands
- Window content passed as props to existing window types
- Use existing batch window creation for performance

#### Content Processing Pipeline

1. **Parse JSON** - Extract object_list with name, point, tixels
2. **Filter Named Layers** - Skip unnamed objects, focus on designed elements  
3. **Extract Content** - Concatenate tixels.text_content arrays into strings
4. **Infer Types** - Analyse content patterns for window type suggestions
5. **Map Coordinates** - Convert "x,y" strings to TUI coordinates with scaling
6. **Calculate Bounds** - Determine window width/height from content dimensions
7. **Batch Spawn** - Create all windows via single API call

## Edge Cases & Error Handling

### File System Issues
- Missing Monodraw file → Clear error message with file path
- Permission denied → Suggest file permission fix
- Corrupted JSON → Parse error with line number if possible

### Content Issues  
- Empty layers → Create frame windows for layout structure
- Unsupported characters → Strip/replace with safe ASCII equivalents
- Overlapping coordinates → Cascade overlapping windows automatically

### Coordinate System Issues
- Negative coordinates → Clamp to (0,0) with warning
- Coordinates outside terminal bounds → Scale down or reposition to fit
- Very large designs → Offer auto-scaling options

### Performance Considerations
- Large files (>50 layers) → Batch creation with progress indication
- Complex content → Limit tixels array processing to prevent hangs
- Memory usage → Stream processing for very large JSON files

## Testing Strategy

### Unit Tests
- JSON parsing with valid/invalid Monodraw files
- Coordinate transformation with various scales and offsets
- Window type inference with known content patterns
- Error handling for edge cases

### Integration Tests
- End-to-end file loading with real Monodraw exports
- API endpoint testing with sample files
- Window creation verification via state API
- Performance testing with large designs

### Manual Testing
- Load known good Monodraw files
- Verify visual correspondence between design and TUI output
- Test with various terminal sizes and scaling factors
- Validate error messages are helpful and actionable

## MVP Test Case

**Reference File**: `/Users/james/Repos/tvision/test-tui/monodraw-demo-simple-primers.monojson`

This demo file contains 4 primer layers with distinct ASCII art content:
- `time-shamans` (pyramid structures with TIME STREAMS labels)
- `symbient-city` (geometric architectural blocks)  
- `chaos-vs-order` (chaotic vs ordered systems diagram)
- `cat-cat-simple` (simple cat ASCII art)

**MVP Success Criteria**: The system must successfully load this demo file and recreate the exact layout shown in the Monodraw screenshot, with each layer becoming a positioned text window displaying its primer content.

## Success Criteria

□ Successfully parse the MVP demo Monodraw file without errors
□ Create 4 positioned text windows matching the blue-outlined layer positions from screenshot
□ Display correct primer content in each window (time-shamans, symbient-city, chaos-vs-order, cat-cat-simple)
□ Maintain coordinate accuracy within terminal character precision  
□ Handle file errors gracefully with clear user feedback
□ Process the 4-layer demo file in <1 second
□ Maintain existing API performance for non-Monodraw operations

## Implementation Phases

### Phase 1: Core Parser (Week 1)
- Basic JSON parsing for name, point, tixels extraction
- Simple coordinate mapping with 1:1 scaling
- Basic window type inference (text vs empty)
- Single API endpoint for file loading

### Phase 2: Advanced Inference (Week 2)  
- Pattern recognition for test_pattern windows
- Gradient detection for gradient windows
- Content preprocessing and cleanup
- Preview/parse-only endpoint

### Phase 3: Production Features (Week 3)
- Scaling and offset support for responsive layouts
- Error handling and validation
- Performance optimisation for large files
- Integration with workspace system

### Phase 4: Polish & Testing (Week 4)
- Comprehensive test suite
- Documentation and examples
- Edge case handling refinement
- Performance benchmarking

## Example Usage

```bash
# Preview Monodraw file structure
curl "http://127.0.0.1:8089/monodraw/parse" \
  -d '{"file_path": "/Users/james/Dropbox/wibwob-brain/perception-assembles-itself-parts.monojson"}'

# Load design with auto-scaling to fit terminal
curl "http://127.0.0.1:8089/monodraw/load" \
  -d '{"file_path": "/Users/james/Dropbox/wibwob-brain/perception-assembles-itself-parts.monojson", "scale": 0.8}'

# Load with explicit window types for specific layers
curl "http://127.0.0.1:8089/monodraw/load" \
  -d '{
    "file_path": "/Users/james/Dropbox/wibwob-brain/perception-assembles-itself-parts.monojson",
    "window_types": {
      "Text Field 1": "text_view",
      "Surface 1": "gradient"
    }
  }'
```

This feature transforms Monodraw from a simple ASCII art editor into a visual TUI layout designer, bridging the gap between design and functional implementation.