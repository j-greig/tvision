# Canvas Size API Enhancement

**tl;dr**: Add terminal/canvas dimensions to MCP state API so AI agents know available screen real estate for precise window positioning and layout planning.

## Problem

Currently the MCP state API only returns:
- `pattern_mode` 
- `windows[]` array
- `uptime_sec`

AI agents need to know terminal dimensions to:
- Calculate optimal window positioning
- Plan text layouts that fit the screen
- Avoid placing windows outside visible area
- Create proportional designs

## Solution

### API Changes

Add canvas size info to `/state` endpoint response:

```json
{
  "pattern_mode": "continuous",
  "windows": [...],
  "canvas": {
    "width": 120,
    "height": 40,
    "cols": 120,
    "rows": 40
  },
  "uptime_sec": 5957.7
}
```

### Implementation Stack

1. **C++ TUI App** (`test-tui/api_ipc.cpp`)
   - Add `get_canvas_size` IPC command
   - Return `TProgram::desktop->size` dimensions

2. **Python Controller** (`tools/api_server/controller.py`)  
   - Add canvas size to state response
   - Cache dimensions, refresh on window resize events

3. **MCP Schema** (`tools/api_server/schemas.py`)
   - Add `CanvasInfo` model with width/height fields
   - Update `StateResponse` to include canvas

### Technical Details

```cpp
// In api_ipc.cpp
if (command == "get_canvas_size") {
    TRect desktop = TProgram::desktop->getBounds();
    response = "canvas_size width=" + std::to_string(desktop.b.x) + 
               " height=" + std::to_string(desktop.b.y);
}
```

```python
# In controller.py  
def get_canvas_size(self) -> dict:
    response = self.send_command("get_canvas_size")
    # Parse "canvas_size width=120 height=40"
    return {"width": w, "height": h, "cols": w, "rows": h}
```

## Benefits

- AI agents can plan layouts within screen bounds
- Enables responsive window arrangements
- Better typography fitting for text compositions
- Prevents off-screen window placement

## Testing

- Resize terminal and verify dimensions update
- Test with various terminal sizes (80x24, 120x40, etc.)
- Ensure MCP tools receive updated canvas info

## Implementation Priority

**High** - Required for proper AI-driven layout planning