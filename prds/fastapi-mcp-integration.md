# FastAPI-MCP Integration for TUI Programmatic Control

**tl;dr**: Extend existing FastAPI TUI control system with MCP protocol support using tadata-org/fastapi_mcp, enabling AI agents like Claude Code to directly control TUI windows while preserving REST API compatibility.

## Executive Summary

The existing TUI control system (`tools/api_server/`) provides REST endpoints for programmatic window management. This PRD outlines integration of the Model Context Protocol (MCP) using the `fastapi_mcp` library, enabling AI agents to interact with TUI applications through standardized MCP tools while maintaining backward compatibility.

**Current Architecture:**
```
AI Agents/Clients → REST API → Unix Socket IPC → C++ TUI Apps
```

**Target Architecture:**
```
AI Agents (Claude Code) → MCP Protocol ↘
External REST Clients   → REST API    → Unix Socket IPC → C++ TUI Apps
```

## Technical Research Summary

### Claude Code MCP Usage Patterns
- **External Integration**: MCP enables Claude to "pull from external datasources like Google Drive, Figma, and Slack"
- **Three Server Types**: stdio, SSE (Server-Sent Events), and HTTP servers
- **Configuration Scopes**: local (project-private), project (shared via .mcp.json), user (global)
- **Resource Discovery**: Dynamic server discovery with @ mentions
- **Authentication**: OAuth 2.0 support with environment variables

### FastAPI-MCP Library Analysis
- **Native Integration**: Direct FastAPI extension, not OpenAPI converter
- **Zero Config**: Minimal setup with `FastApiMCP(app).mount()`
- **Authentication**: Preserves existing FastAPI `Depends()` patterns
- **ASGI Transport**: Efficient communication via ASGI interface
- **Deployment Flexibility**: Same app or separate deployment options

## Current System Architecture

### Existing Components
```ascii
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   REST Client   │───▶│   FastAPI Server │───▶│  Unix Socket    │
│   (curl/Python) │    │  (tools/api_*)   │    │  (/tmp/*.sock)  │
└─────────────────┘    └──────────────────┘    └─────────────────┘
                                                         │
                       ┌──────────────────┐              │
                       │   WebSocket      │              │
                       │   Events         │              │
                       └──────────────────┘              │
                                                         ▼
                                                ┌─────────────────┐
                                                │  C++ TUI App    │
                                                │  (test_pattern) │
                                                └─────────────────┘
```

### Current REST Endpoints
- `GET /state` - Application and window state
- `POST /windows` - Create windows with type/props
- `POST /windows/{id}/move` - Move/resize operations
- `POST /windows/{id}/focus` - Focus management  
- `POST /windows/{id}/close` - Window closure
- `POST /windows/cascade|tile|close_all` - Layout operations
- `GET /ws` - WebSocket events

## MCP Integration Design

### Target MCP Architecture
```ascii
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   Claude Code   │───▶│   MCP Server     │    │  Unix Socket    │
│   (MCP Client)  │    │  (FastAPI-MCP)   │───▶│  (/tmp/*.sock)  │
└─────────────────┘    └──────────────────┘    └─────────────────┘
                              │                          │
┌─────────────────┐           │                          │
│   REST Client   │───────────┘                          │
│   (curl/Python) │                                      │
└─────────────────┘                                      ▼
                                              ┌─────────────────┐
                                              │  C++ TUI App    │
                                              │  (test_pattern) │
                                              └─────────────────┘
```

### MCP Tools Mapping

#### Core Window Management Tools
1. **`tui_get_state`** 
   - Maps to: `GET /state`
   - Returns: Current window list, app status
   - Schema: `{windows: [{id, title, rect, type}], pattern_mode}`

2. **`tui_create_window`**
   - Maps to: `POST /windows`  
   - Parameters: `type`, `title?`, `rect?`, `props?`
   - Returns: Created window object with ID

3. **`tui_move_window`**
   - Maps to: `POST /windows/{id}/move`
   - Parameters: `window_id`, `x?`, `y?`, `width?`, `height?`
   - Returns: Updated window state

4. **`tui_focus_window`**
   - Maps to: `POST /windows/{id}/focus`
   - Parameters: `window_id`
   - Returns: Success confirmation

5. **`tui_close_window`**
   - Maps to: `POST /windows/{id}/close`
   - Parameters: `window_id`
   - Returns: Success confirmation

#### Layout Management Tools
6. **`tui_cascade_windows`**
   - Maps to: `POST /windows/cascade`
   - Returns: Layout confirmation

7. **`tui_tile_windows`**
   - Maps to: `POST /windows/tile`
   - Parameters: `columns?`
   - Returns: Layout confirmation

8. **`tui_close_all_windows`**
   - Maps to: `POST /windows/close_all`
   - Returns: Success confirmation

#### Application Control Tools  
9. **`tui_set_pattern_mode`**
   - Maps to: `POST /pattern_mode`
   - Parameters: `mode` ("continuous"|"tiled")
   - Returns: Success confirmation

10. **`tui_screenshot`**
    - Maps to: `POST /screenshot`
    - Returns: Screenshot path

### MCP Resources
1. **`tui://windows`** - List of current windows
2. **`tui://windows/{id}`** - Individual window details
3. **`tui://state`** - Full application state
4. **`tui://events`** - Event stream (via WebSocket bridge)

## Implementation Plan

### Phase 1: Core MCP Integration (Week 1)
**Scope**: Basic MCP server setup with essential window management

**Tasks:**
- [ ] Install and configure `fastapi_mcp` dependency
- [ ] Create MCP server mount in existing FastAPI app
- [ ] Implement core tools: `tui_get_state`, `tui_create_window`, `tui_move_window`
- [ ] Add MCP server configuration for Claude Code integration
- [ ] Create basic integration tests

**Acceptance Criteria:**
- ✅ Claude Code can discover and connect to MCP server
- ✅ Basic window creation and movement work via MCP
- ✅ Existing REST endpoints remain fully functional
- ✅ MCP tools return consistent schemas with REST API

**Deliverables:**
- Modified `tools/api_server/main.py` with MCP mount
- MCP tool implementations in `tools/api_server/mcp_tools.py`
- Updated `requirements.txt` with `fastapi-mcp`
- Integration test suite

### Phase 2: Advanced TUI Controls (Week 2)
**Scope**: Complete MCP tool coverage and resource system

**Tasks:**
- [ ] Implement remaining MCP tools (focus, close, layout operations)
- [ ] Add MCP resources for state querying
- [ ] Create MCP-specific error handling and response formatting
- [ ] Add authentication integration (if required)
- [ ] Implement WebSocket bridge for MCP event streaming

**Acceptance Criteria:**
- ✅ All REST endpoints have MCP tool equivalents
- ✅ MCP resources provide read-only access to TUI state
- ✅ Real-time events work with MCP clients
- ✅ Error responses are consistent between REST and MCP

**Deliverables:**
- Complete MCP tool suite
- MCP resource definitions
- Event streaming integration
- Comprehensive test coverage

### Phase 3: Production Readiness (Week 3)
**Scope**: Documentation, optimization, and deployment preparation

**Tasks:**
- [ ] Create MCP server configuration templates
- [ ] Add comprehensive documentation with Claude Code examples
- [ ] Performance optimization for MCP vs REST paths
- [ ] Add monitoring and logging for MCP operations
- [ ] Create deployment guide for MCP-enabled server

**Acceptance Criteria:**
- ✅ Complete documentation with usage examples
- ✅ Performance benchmarks show acceptable overhead
- ✅ Production deployment guide validated
- ✅ Claude Code integration fully documented

**Deliverables:**
- MCP configuration templates
- Complete documentation update in CLAUDE.md
- Performance analysis report
- Deployment automation scripts

## Technical Implementation Details

### FastAPI-MCP Integration Code

```python
# tools/api_server/main.py
from fastapi import FastAPI
from fastapi_mcp import FastApiMCP
from .mcp_tools import register_tui_tools

app = FastAPI(title="TUI Control API", version="1.0.0")

# Existing REST endpoints
from .endpoints import router
app.include_router(router)

# MCP Integration
mcp = FastApiMCP(app)
register_tui_tools(mcp)
mcp.mount()  # Mounts MCP server at /mcp

# WebSocket for events (existing)
from .websocket import websocket_endpoint
app.websocket("/ws")(websocket_endpoint)
```

### MCP Tool Implementation Pattern

```python
# tools/api_server/mcp_tools.py
from fastapi_mcp import FastApiMCP
from .controller import Controller

def register_tui_tools(mcp: FastApiMCP):
    
    @mcp.tool("tui_get_state")
    async def get_tui_state() -> dict:
        """Get current TUI application state and window list"""
        controller = get_controller()
        state = await controller.get_state()
        return {
            "windows": [serialize_window(w) for w in state.windows],
            "pattern_mode": state.pattern_mode,
            "uptime_sec": state.uptime_sec
        }
    
    @mcp.tool("tui_move_window") 
    async def move_tui_window(
        window_id: str,
        x: int | None = None,
        y: int | None = None,
        width: int | None = None,
        height: int | None = None
    ) -> dict:
        """Move or resize a TUI window"""
        controller = get_controller()
        try:
            window = await controller.move_resize(
                window_id, x=x, y=y, w=width, h=height
            )
            return {"success": True, "window": serialize_window(window)}
        except KeyError:
            return {"success": False, "error": "Window not found"}
```

### Claude Code Configuration

```json
// .mcp.json (project scope)
{
  "mcpServers": {
    "tui-control": {
      "command": "stdio",
      "type": "http",
      "url": "http://127.0.0.1:8089/mcp",
      "description": "TUI application control via MCP"
    }
  }
}
```

### Claude Code Headless Mode Requirements

**⚠️ IMPORTANT**: This integration is designed specifically for **Claude Code headless mode** operation. Interactive Claude Code UI cannot be used with TUI applications as both compete for terminal control.

**Required Usage Pattern:**
```bash
# Correct: Headless mode with MCP configuration (separate arguments)
claude -p --mcp-config=.mcp.json "Create a test pattern window and move it to position 50,20"

# Alternative: Explicitly separate the config and prompt
claude --print --mcp-config .mcp.json "Get current TUI window state"

# Correct: JSON output for programmatic parsing  
claude -p --output-format=json --mcp-config=.mcp.json "Get current TUI window state"

# Correct: Multi-turn conversation
claude -p --resume=conversation_id --mcp-config=.mcp.json "Now close all windows"

# ❌ INCORRECT: Interactive mode conflicts with TUI
claude  # Cannot use interactive mode with TUI applications

# ❌ INCORRECT: Malformed command (config path gets mixed with prompt)
claude -p --mcp-config .mcp.json "prompt"  # Parses ".mcp.json prompt" as config path
```

**Headless Mode Capabilities:**
- **Non-interactive execution**: `claude -p` runs without UI competition
- **MCP integration**: `--mcp-config` loads TUI control tools
- **JSON responses**: `--output-format json` for programmatic parsing
- **Multi-turn sessions**: `--resume` maintains conversation context
- **Tool control**: `--allowedTools` can restrict TUI operations if needed

**Integration Workflow:**
1. Start TUI application: `cd test-tui && ./build/test_pattern`
2. Start MCP server: `python -m tools.api_server.main --port=8089`
3. Use headless Claude: `claude -p --mcp-config .mcp.json "<command>"`

This ensures no terminal conflicts between Claude Code interactive mode and TUI applications.

## Security Considerations

### Authentication Strategy
- **REST API**: Existing FastAPI authentication (if any)
- **MCP Protocol**: Leverages FastAPI `Depends()` patterns
- **Local Development**: No auth required for localhost
- **Production**: API keys or OAuth 2.0 integration

### Access Control
- **Tool-level**: Granular permissions per MCP tool
- **Resource-level**: Read-only vs read-write access
- **Rate Limiting**: Prevent MCP abuse with existing FastAPI middleware

## Testing Strategy

### Unit Tests
- Each MCP tool independently tested
- Mock Unix socket IPC for isolated testing
- Schema validation for all tool responses

### Integration Tests  
- Full MCP client → server → C++ app flow
- REST and MCP parallel operation testing
- WebSocket event delivery to both interfaces

### Performance Tests
- MCP vs REST endpoint latency comparison
- Concurrent client handling (REST + MCP)
- Memory usage analysis with both protocols active

### Claude Code Integration Tests
- Manual testing with Claude Code as MCP client
- Automated workflow testing (create → move → close windows)
- Error handling and recovery scenarios

## Success Metrics

### Functional Requirements
- ✅ 100% REST endpoint coverage in MCP tools
- ✅ < 50ms additional latency for MCP vs REST paths  
- ✅ Zero breaking changes to existing REST API
- ✅ Claude Code can successfully control TUI windows

### Quality Requirements
- ✅ 95% test coverage for MCP implementation
- ✅ Complete documentation with examples
- ✅ Production-ready deployment process
- ✅ Backward compatibility maintained

## Risk Assessment

### Technical Risks
- **Dependency Risk**: `fastapi-mcp` library maturity
  - *Mitigation*: Comprehensive testing, fallback plans
- **Performance Impact**: Additional protocol overhead
  - *Mitigation*: Benchmarking, optimization, monitoring
- **Complexity**: Dual-protocol maintenance burden
  - *Mitigation*: Shared business logic, unified testing

### Integration Risks  
- **Claude Code Compatibility**: MCP client variations
  - *Mitigation*: Follow MCP specification strictly
- **Breaking Changes**: REST API modifications affect MCP
  - *Mitigation*: Comprehensive integration test suite

## Future Enhancements

### Advanced MCP Features
- **Streaming Resources**: Real-time window state updates
- **Batch Operations**: Multi-window operations in single MCP call
- **TUI Scripting**: Complex automation workflows via MCP

### Extended Integration
- **Multiple TUI Apps**: Support for different C++ applications
- **Remote TUI**: Network-based TUI control via MCP
- **Visual Feedback**: Screenshots and UI state visualization

## Appendix

### MCP Specification Compliance
- Implements MCP 1.0 protocol specification
- Supports all required MCP message types
- Handles MCP client capabilities negotiation

### Development Environment Setup
```bash
# Install dependencies
cd tools/api_server
pip install fastapi-mcp

# Configure Claude Code MCP
# Add .mcp.json to project root
# Test with: claude-code --mcp-test tui-control
```

### Related Documentation
- [MCP Specification](https://spec.modelcontextprotocol.io/)
- [FastAPI-MCP Documentation](https://fastapi-mcp.tadata.com/)
- [Claude Code MCP Integration](https://docs.anthropic.com/en/docs/claude-code/mcp)

---

**Implementation Timeline**: 3 weeks  
**Priority**: High - Enables AI agent integration  
**Complexity**: Medium - Well-defined integration path  
**Impact**: High - Unlocks programmatic AI control of TUI applications