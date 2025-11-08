# Terminal Screenshot MCP Integration

**tl;dr:** Replace placeholder screenshot endpoint with real terminal capture via IPC bridge, extend cross-platform support beyond macOS `screencapture`, add programmatic screenshot metadata/formatting options for MCP clients, implement proper async/sync flow between FastAPI and C++ TUI app.

## Context & Current State

Working with TVision TUI framework, FastAPI MCP server (tools/api_server/), existing Unix socket IPC bridge (api_ipc.cpp), and C++ screenshot implementation that currently uses macOS-specific `screencapture` command targeting terminal windows by application name.

**Current Implementation:**
- C++ TUI: `takeScreenshot()` in test_pattern_app.cpp uses `screencapture -x -l$(osascript...)` 
- MCP API: `POST /screenshot` endpoint exists but only records metadata, doesn't trigger actual capture
- IPC Bridge: `send_cmd("screenshot")` exists but FastAPI doesn't wait for completion
- Output: PNG files saved to `test-tui/screenshots/` with timestamp naming

**Limitations:**
- macOS-only capture method
- No synchronization between API request and actual screenshot completion  
- No programmatic control over capture format/quality/region
- Missing cross-platform terminal detection and capture methods

## Objective

Implement robust, cross-platform terminal screenshot functionality accessible via MCP HTTP endpoints that provides real-time capture of TUI application state with proper async coordination between Python API server and C++ TUI application.

## Requirements

### Core Functionality
• **Synchronous Screenshot API** - `POST /screenshot` blocks until PNG file is written to disk
• **Cross-Platform Support** - Works on macOS, Linux, Windows terminal environments  
• **IPC Coordination** - Proper request/response flow between FastAPI and C++ via Unix socket
• **File Path Control** - Accept optional custom output path, return absolute file path in response
• **Error Handling** - Distinguish between capture failures, permission issues, file I/O errors
• **Screenshot Metadata** - Return file size, dimensions, capture timestamp in API response

### Platform-Specific Capture Methods
• **macOS**: Enhance existing `screencapture` with better terminal detection (Terminal.app, iTerm2, VS Code integrated terminal)
• **Linux**: Implement `scrot`, `gnome-screenshot`, `import` (ImageMagick) fallback chain
• **Windows**: PowerShell `Add-Type` screen capture or third-party tool integration
• **Terminal Detection**: Environment variable analysis (TERM_PROGRAM, SSH_TTY, etc.)

### MCP Integration Enhancements
• **Enhanced State Response** - Include last screenshot path/metadata in `GET /state`
• **WebSocket Events** - Emit `screenshot.started`, `screenshot.completed`, `screenshot.failed` events
• **Batch Operations** - Support screenshot before/after window operations for documentation
• **Format Options** - PNG (default), optional JPEG with quality settings

### Performance & Quality
• **Capture Timing** - Configurable delay before capture (settle animations, redraws)
• **Resolution Control** - Respect terminal DPI/scaling, optional resolution overrides  
• **File Management** - Automatic cleanup of old screenshots, configurable retention policy
• **Background Capture** - Non-blocking screenshot queue for rapid sequential captures

## Technical Architecture

### IPC Protocol Enhancement

**Current Flow:**
```
FastAPI POST /screenshot → send_cmd("screenshot") → return metadata immediately
```

**Enhanced Flow:**
```
FastAPI POST /screenshot → send_cmd("screenshot path=X") → wait for response → return file info
```

**IPC Commands:**
- `cmd:screenshot path=/path/to/file.png delay=100` - Capture with optional path and delay
- Response: `ok:screenshot path=/abs/path size=12345 width=120 height=40 timestamp=1234567890`
- Error: `error:screenshot msg="Permission denied"`

### Cross-Platform Capture Implementation

**Capture Strategy Chain:**
1. **Terminal Detection** - Identify terminal application and capabilities
2. **Method Selection** - Choose best available capture method for platform/terminal
3. **Execution** - Run capture command with proper error handling and output parsing
4. **Verification** - Validate captured file exists and contains valid image data

**Platform Methods:**

**macOS:**
```cpp
// Enhanced terminal detection
std::string detectTerminal() {
    if (const char* term = getenv("TERM_PROGRAM")) {
        if (strcmp(term, "iTerm.app") == 0) return "iTerm2";
        if (strcmp(term, "Apple_Terminal") == 0) return "Terminal";
        if (strcmp(term, "vscode") == 0) return "VSCode";
    }
    return "Terminal"; // fallback
}

// Improved screencapture command  
std::string buildMacScreencapture(const std::string& outputPath) {
    std::string app = detectTerminal();
    return "screencapture -x -l$(osascript -e 'tell app \"" + app + 
           "\" to id of window 1' 2>/dev/null) \"" + outputPath + "\" 2>/dev/null";
}
```

**Linux:**
```cpp
std::vector<std::string> getLinuxMethods() {
    return {
        "scrot -s", // Interactive selection
        "gnome-screenshot -w", // Active window  
        "import -window root", // ImageMagick fallback
        "xwd -root | convert xwd:- png:-" // X11 fallback
    };
}
```

**Windows:**
```cpp
std::string getWindowsMethod() {
    return "powershell -Command \"Add-Type -AssemblyName System.Windows.Forms;"
           "[System.Windows.Forms.SendKeys]::SendWait('%{PRTSC}');\"";
}
```

### FastAPI Controller Updates

**Enhanced Screenshot Method:**
```python
async def screenshot(self, path: Optional[str] = None, delay: int = 100) -> Dict[str, Any]:
    """Take screenshot with proper IPC coordination"""
    timestamp = int(time.time())
    target_path = path or f"screenshots/tui_{timestamp}.png"
    
    # Ensure absolute path for return
    abs_path = os.path.abspath(target_path)
    
    try:
        # Send IPC command with parameters
        cmd = f"screenshot path={target_path}"
        if delay > 0:
            cmd += f" delay={delay}"
        
        response = await send_cmd_with_response(cmd, timeout=10.0)
        
        if response.startswith("ok:screenshot"):
            # Parse response metadata
            metadata = parse_screenshot_response(response)
            
            async with self._lock:
                self._state.last_screenshot = abs_path
            
            await self._events.emit("screenshot.completed", {
                "path": abs_path,
                "size": metadata.get("size"),
                "dimensions": {"width": metadata.get("width"), "height": metadata.get("height")},
                "timestamp": timestamp
            })
            
            return {
                "success": True,
                "path": abs_path,
                "size": metadata.get("size"),
                "dimensions": {"width": metadata.get("width"), "height": metadata.get("height")},
                "timestamp": timestamp
            }
        else:
            error_msg = parse_error_response(response)
            await self._events.emit("screenshot.failed", {"error": error_msg})
            raise Exception(f"Screenshot failed: {error_msg}")
            
    except Exception as e:
        await self._events.emit("screenshot.failed", {"error": str(e)})
        raise
```

## Implementation Phases

### Phase 1: IPC Synchronization (Week 1)
**Deliverables:**
- Enhanced IPC protocol with request/response pattern
- Updated C++ screenshot function to return metadata via IPC
- FastAPI controller waits for IPC response before returning
- Basic error handling for capture failures

**Success Criteria:**
□ POST /screenshot blocks until PNG file exists on disk
□ Response includes absolute file path and basic metadata  
□ Proper error responses for capture failures
□ WebSocket events emitted for screenshot lifecycle

### Phase 2: Cross-Platform Capture (Week 2)  
**Deliverables:**
- Platform detection logic in C++ TUI application
- Linux capture methods (scrot, gnome-screenshot, import)
- Windows PowerShell capture integration
- Fallback method chain with graceful degradation

**Success Criteria:**
□ Screenshot works on Ubuntu/Debian with GNOME
□ Screenshot works on CentOS/RHEL with scrot installed
□ Screenshot works on Windows 10/11 with PowerShell
□ Graceful fallback when preferred methods unavailable

### Phase 3: Enhanced Features (Week 3)
**Deliverables:**
- Configurable capture delay for animation settling
- File format options (PNG default, JPEG with quality)
- Screenshot metadata in GET /state responses
- Automatic cleanup of old screenshots

**Success Criteria:**
□ Delay parameter prevents blurry/partial captures
□ JPEG format reduces file size for large terminals
□ State API includes last screenshot info
□ Old screenshots cleaned up automatically

### Phase 4: MCP Integration Polish (Week 4)
**Deliverables:**
- MCP tool descriptions updated with new parameters
- Batch screenshot operations for window management workflows
- Performance optimization for rapid sequential captures
- Documentation and testing for AI agent workflows

**Success Criteria:**
□ Claude Code can take screenshots via MCP reliably
□ Screenshot before/after window operations work smoothly
□ Performance acceptable for automated testing workflows
□ Comprehensive error messages for debugging

## API Design

### Enhanced Endpoint Schema

**Request:**
```json
POST /screenshot
{
  "path": "/custom/path/screenshot.png",  // Optional custom path
  "delay": 200,                          // Optional settle delay (ms)
  "format": "png",                       // Optional: "png" (default) | "jpeg" 
  "quality": 90                          // Optional: JPEG quality 1-100
}
```

**Response:**
```json
{
  "success": true,
  "path": "/abs/path/to/screenshot.png",
  "size": 145678,
  "dimensions": {
    "width": 120,
    "height": 40  
  },
  "format": "png",
  "timestamp": 1234567890,
  "capture_duration_ms": 150
}
```

**Error Response:**
```json
{
  "success": false,
  "error": "capture_failed",
  "message": "screencapture command failed with exit code 1",
  "details": {
    "platform": "darwin",
    "method": "screencapture",
    "terminal": "iTerm2"
  }
}
```

### WebSocket Events

**screenshot.started:**
```json
{"type": "screenshot.started", "payload": {"path": "/path/to/file.png", "delay": 100}}
```

**screenshot.completed:**  
```json
{"type": "screenshot.completed", "payload": {"path": "/path/file.png", "size": 12345, "duration_ms": 150}}
```

**screenshot.failed:**
```json
{"type": "screenshot.failed", "payload": {"error": "permission_denied", "message": "Cannot access display"}}
```

## Testing Strategy

### Unit Tests
- IPC request/response parsing and formatting
- Platform detection logic for different environments
- File path validation and absolute path resolution  
- Error handling for various capture failure modes

### Integration Tests
- End-to-end screenshot via POST /screenshot on each platform
- WebSocket event emission during screenshot lifecycle
- Screenshot metadata accuracy (file size, dimensions)
- Concurrent screenshot requests handling

### Platform Testing Matrix
```
| Platform      | Terminal        | Method            | Status |
|---------------|-----------------|-------------------|--------|
| macOS 13+     | Terminal.app    | screencapture     | ✓      |
| macOS 13+     | iTerm2          | screencapture     | ✓      | 
| macOS 13+     | VS Code         | screencapture     | ✓      |
| Ubuntu 22.04  | gnome-terminal  | gnome-screenshot  | ✓      |
| Ubuntu 22.04  | xterm           | scrot             | ✓      |
| CentOS 8      | konsole         | import            | ✓      |
| Windows 11    | Windows Terminal| PowerShell        | ✓      |
| Windows 11    | Command Prompt  | PowerShell        | ✓      |
```

### Error Condition Testing
- Terminal application not found/accessible
- Screenshot directory not writable  
- Concurrent screenshot requests
- Very large terminal dimensions
- Terminal minimized or hidden
- SSH/remote terminal sessions

## Success Criteria

### Functional Requirements
□ Screenshot API responds with actual PNG file saved to disk
□ Works reliably across macOS, Linux, Windows platforms  
□ Proper error messages for debugging platform-specific issues
□ File metadata (size, dimensions) accurately reported
□ WebSocket events provide real-time capture status

### Performance Requirements  
□ Screenshot completes within 5 seconds on typical terminal sizes
□ No memory leaks during repeated screenshot operations
□ Handles concurrent requests gracefully (queue or reject)
□ File I/O operations don't block TUI application responsiveness

### Integration Requirements
□ MCP clients can reliably trigger screenshots programmatically  
□ Screenshot works during active window manipulation (cascade, tile, etc.)
□ State synchronization between FastAPI and C++ TUI remains consistent
□ Existing window management APIs continue working during screenshot operations

## Risk Mitigation

### Technical Risks
- **Platform Capture Failures**: Implement robust fallback chains and clear error messages
- **Permission Issues**: Document required permissions, provide setup instructions
- **Large File Handling**: Implement size limits and compression options
- **IPC Timeouts**: Configurable timeout values, retry logic for transient failures

### Operational Risks  
- **Disk Space**: Automatic cleanup of old files, configurable retention policies
- **Security**: Validate all file paths, prevent directory traversal attacks
- **Performance**: Queue management for high-frequency screenshot requests
- **Compatibility**: Extensive testing across terminal applications and OS versions

This PRD provides a comprehensive roadmap for implementing robust, cross-platform terminal screenshot functionality that integrates seamlessly with your existing MCP server architecture while extending beyond the current macOS-only limitation.