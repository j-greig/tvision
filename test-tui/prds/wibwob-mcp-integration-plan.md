# WibWob MCP Integration Plan

**tl;dr**: Add MCP tool support to wibwob chat window using createSdkMcpServer approach from Claude Code SDK. Enables AI to control TUI windows programmatically through type-safe custom tools. Architecture: C++ wibwob → Node.js bridge → SDK with MCP tools → Claude.

## Overview

Integrate Model Context Protocol (MCP) tools into the existing wibwob chat window to enable AI-driven TUI application control. This will expose the tools/api_server/ REST endpoints as type-safe custom tools accessible to Claude during wibwob conversations.

## Current Architecture

```
C++ TUI Application (test_pattern)
├── WibWob Chat Window (TWibWobView)  
└── Claude Code SDK Bridge (Node.js)
    ├── claude_sdk_bridge.js (streaming interface)
    ├── Unix socket IPC to C++
    └── Claude Code SDK (@anthropic-ai/claude-code)
```

## Target Architecture

```  
C++ TUI Application (test_pattern)
├── WibWob Chat Window (TWibWobView)
├── API Server (FastAPI) - tools/api_server/
│   ├── REST endpoints for window management
│   └── Unix socket IPC to C++ app
└── Enhanced Claude Code SDK Bridge (Node.js)
    ├── claude_sdk_bridge.js (streaming interface)
    ├── MCP Server (createSdkMcpServer)
    │   ├── Type-safe custom tools (Zod validation)
    │   ├── HTTP client to API server
    │   └── Tool registration for window management
    └── Claude Code SDK with MCP tools enabled
```

## Implementation Plan

### Phase 1: MCP Server Setup

#### 1.1 Enhance Node.js Bridge Dependencies
- Add required dependencies to `test-tui/llm/sdk_bridge/package.json`:
  - `zod` for type-safe schema validation  
  - `axios` for HTTP client to API server
  - Update `@anthropic-ai/claude-code` to latest version with MCP support

#### 1.2 Create MCP Tools Module
Create `test-tui/llm/sdk_bridge/mcp_tools.js`:

```javascript
import { createSdkMcpServer, tool } from '@anthropic-ai/claude-code';
import { z } from 'zod';
import axios from 'axios';

// HTTP client for API server
const apiClient = axios.create({
  baseURL: 'http://127.0.0.1:8089',
  timeout: 5000
});

// Tool schemas using Zod
const WindowTypeSchema = z.enum(['test_pattern', 'gradient', 'frame_player', 'text_view', 'text_editor']);
const RectSchema = z.object({
  x: z.number().int().min(0),
  y: z.number().int().min(0),
  w: z.number().int().min(1),
  h: z.number().int().min(1)
}).optional();

// Create MCP server with TUI control tools
export const createTuiMcpServer = () => {
  return createSdkMcpServer({
    name: "tui-control",
    version: "1.0.0",
    tools: [
      // Window management tools
      tool(
        "get_tui_state",
        "Get current TUI application state, window list, and canvas dimensions",
        {},
        async (args) => {
          const response = await apiClient.get('/state');
          return { content: [{ type: "text", text: JSON.stringify(response.data, null, 2) }] };
        }
      ),
      
      tool(
        "create_window",
        "Create a new TUI window with precise positioning and properties",
        {
          type: WindowTypeSchema.describe("Type of window to create"),
          title: z.string().optional().describe("Optional window title"),
          rect: RectSchema.describe("Optional precise position and size {x,y,w,h}"),
          props: z.record(z.any()).optional().describe("Additional window properties")
        },
        async (args) => {
          const response = await apiClient.post('/windows', args);
          return { content: [{ type: "text", text: JSON.stringify(response.data, null, 2) }] };
        }
      ),
      
      tool(
        "move_window",
        "Move or resize an existing TUI window",
        {
          window_id: z.string().describe("ID of window to move/resize"),
          x: z.number().int().optional().describe("New X position"),
          y: z.number().int().optional().describe("New Y position"),
          w: z.number().int().optional().describe("New width"),
          h: z.number().int().optional().describe("New height")
        },
        async (args) => {
          const { window_id, ...moveParams } = args;
          const response = await apiClient.post(`/windows/${window_id}/move`, moveParams);
          return { content: [{ type: "text", text: JSON.stringify(response.data, null, 2) }] };
        }
      ),
      
      tool(
        "close_window",
        "Close a specific TUI window",
        {
          window_id: z.string().describe("ID of window to close")
        },
        async (args) => {
          const response = await apiClient.post(`/windows/${args.window_id}/close`);
          return { content: [{ type: "text", text: JSON.stringify(response.data, null, 2) }] };
        }
      ),
      
      tool(
        "cascade_windows",
        "Arrange all windows in cascade layout",
        {},
        async (args) => {
          const response = await apiClient.post('/windows/cascade');
          return { content: [{ type: "text", text: JSON.stringify(response.data, null, 2) }] };
        }
      ),
      
      tool(
        "tile_windows",
        "Arrange all windows in tiled grid layout",
        {
          cols: z.number().int().min(1).optional().describe("Number of columns for tiling")
        },
        async (args) => {
          const response = await apiClient.post('/windows/tile', args);
          return { content: [{ type: "text", text: JSON.stringify(response.data, null, 2) }] };
        }
      ),
      
      tool(
        "send_text",
        "Send text content to a text editor window (great for ASCII art)",
        {
          window_id: z.string().describe("ID of text editor window"),
          content: z.string().describe("Text content to send"),
          mode: z.enum(['append', 'replace', 'insert']).default('append').describe("How to add the content"),
          position: z.enum(['cursor', 'start', 'end']).default('end').describe("Where to position content")
        },
        async (args) => {
          const response = await apiClient.post(`/windows/${args.window_id}/send_text`, args);
          return { content: [{ type: "text", text: JSON.stringify(response.data, null, 2) }] };
        }
      ),
      
      tool(
        "send_figlet",
        "Generate and send FIGlet ASCII art to a text editor window",
        {
          window_id: z.string().describe("ID of text editor window or 'auto' to create one"),
          text: z.string().describe("Text to convert to ASCII art"),
          font: z.string().default('standard').describe("FIGlet font (standard, slant, big, etc.)"),
          mode: z.enum(['append', 'replace']).default('append').describe("How to add the content")
        },
        async (args) => {
          const endpoint = args.window_id === 'auto' ? 
            '/text_editor/send_figlet' : 
            `/windows/${args.window_id}/send_figlet`;
          const response = await apiClient.post(endpoint, args);
          return { content: [{ type: "text", text: JSON.stringify(response.data, null, 2) }] };
        }
      ),
      
      tool(
        "batch_layout",
        "Create complex window layouts with grid macros and precise positioning",
        {
          request_id: z.string().describe("Unique request ID"),
          dry_run: z.boolean().default(false).describe("Simulate without applying changes"),
          ops: z.array(z.object({
            op: z.enum(['create', 'move_resize', 'close', 'macro.create_grid', 'macro.create_ring']),
            view_type: z.string().optional(),
            title: z.string().optional(),
            bounds: z.object({x: z.number(), y: z.number(), w: z.number(), h: z.number()}).optional(),
            grid: z.object({
              cols: z.number().int(),
              rows: z.number().int(),
              cell_w: z.number().int(),
              cell_h: z.number().int(),
              gap_x: z.number().int().default(0),
              gap_y: z.number().int().default(0),
              origin: z.object({x: z.number(), y: z.number(), w: z.number(), h: z.number()}),
              order: z.enum(['row_major', 'col_major']).default('row_major')
            }).optional()
          })).describe("List of operations to perform")
        },
        async (args) => {
          const response = await apiClient.post('/windows/batch_layout', args);
          return { content: [{ type: "text", text: JSON.stringify(response.data, null, 2) }] };
        }
      ),
      
      tool(
        "take_screenshot",
        "Capture a screenshot of the TUI application",
        {},
        async (args) => {
          const response = await apiClient.post('/screenshot');
          return { content: [{ type: "text", text: JSON.stringify(response.data, null, 2) }] };
        }
      )
    ]
  });
};
```

### Phase 2: Bridge Integration

#### 2.1 Enhance claude_sdk_bridge.js
Modify the existing bridge to integrate MCP server:

```javascript
// Add imports
import { createTuiMcpServer } from './mcp_tools.js';

class ClaudeSDKBridge {
    constructor() {
        // ... existing code ...
        this.mcpServer = null;
        this.setupMcpServer();
    }
    
    setupMcpServer() {
        try {
            this.mcpServer = createTuiMcpServer();
            console.error('DEBUG: MCP server created successfully');
        } catch (error) {
            console.error('ERROR: Failed to create MCP server:', error.message);
        }
    }
    
    async sendQuery(data) {
        // ... existing code ...
        
        // Add MCP tools to allowed tools
        const mcpToolNames = this.mcpServer ? 
            this.mcpServer.tools.map(t => `mcp__tui-control__${t.name}`) : [];
        
        const allAllowedTools = [
            ...this.sessionConfig.allowedTools,
            ...mcpToolNames
        ];
        
        for await (const message of query({
            prompt: messageGenerator,
            options: {
                customSystemPrompt: this.customSystemPrompt,
                maxTurns: this.sessionConfig.maxTurns,
                allowedTools: allAllowedTools,
                model: this.sessionConfig.model || 'sonnet',
                mcpServers: this.mcpServer ? [this.mcpServer] : []  // Key addition
            }
        })) {
            // ... existing streaming logic ...
        }
    }
}
```

#### 2.2 Package.json Updates
Update dependencies in `test-tui/llm/sdk_bridge/package.json`:

```json
{
  "dependencies": {
    "@anthropic-ai/claude-code": "^1.0.115",
    "axios": "^1.6.0",
    "zod": "^3.22.0"
  },
  "type": "module"
}
```

### Phase 3: API Server Integration

#### 3.1 Ensure API Server Running
The existing API server at `tools/api_server/` needs to be running on localhost:8089. Add startup logic to the bridge or C++ app to ensure API server is available.

#### 3.2 Error Handling & Fallbacks
Add robust error handling in MCP tools:
- API server connection failures
- Invalid tool parameters  
- Window not found errors
- Type validation failures

### Phase 4: C++ Integration Points

#### 4.1 WibWob View Enhancements
Enhance `wibwob_view.cpp` to display MCP tool usage status:
- Show when AI is using tools
- Display tool execution results
- Handle tool-generated content (screenshots, text, etc.)

#### 4.2 Status Indicators
Add visual indicators in chat for:
- Tool execution in progress
- Tool success/failure status
- Window management operations

## Tool Categories & Use Cases

### Core Window Management
- `get_tui_state` - Understand current layout
- `create_window` - Spawn new windows with precise positioning  
- `move_window` - Rearrange existing windows
- `close_window` - Clean up windows

### Layout Automation  
- `cascade_windows` - Quick cascade arrangement
- `tile_windows` - Grid layout arrangement
- `batch_layout` - Complex multi-window operations with macros

### Content Creation
- `send_text` - Dynamic text/ASCII art delivery
- `send_figlet` - Beautiful ASCII art banners
- `take_screenshot` - Capture current state

### Advanced Use Cases
1. **Smiley Face Demo**: Create positioned gradient windows as eyes/nose
2. **ASCII Art Gallery**: Generate figlet headers in text windows
3. **Grid Layouts**: Use batch_layout for perfect window arrangements
4. **Interactive Tutorials**: AI can create, position, and populate windows dynamically

## Type Safety & Validation

### Zod Schema Benefits
- Runtime type validation for all tool parameters
- Auto-generated TypeScript types
- Clear error messages for invalid inputs
- IDE autocomplete support

### Error Handling Strategy
```javascript
// Robust error handling pattern
tool("example_tool", "Description", schema, async (args) => {
  try {
    // Validate against API server availability  
    if (!await isApiServerHealthy()) {
      return { content: [{ type: "text", text: "API server not available" }] };
    }
    
    const response = await apiClient.post('/endpoint', args);
    return { content: [{ type: "text", text: JSON.stringify(response.data, null, 2) }] };
  } catch (error) {
    return { content: [{ type: "text", text: `Tool error: ${error.message}` }] };
  }
});
```

## Testing Strategy

### 1. Unit Testing
- Test individual MCP tools in isolation
- Mock API server responses
- Validate Zod schema enforcement

### 2. Integration Testing  
- Test full wibwob → bridge → MCP → API server flow
- Verify tool execution results in TUI
- Test error handling and recovery

### 3. User Acceptance Testing
- Natural language prompts that trigger tools
- Complex multi-tool operations
- Visual verification of window management

## Security Considerations

### 1. Tool Permissions
- Only expose safe window management operations
- No file system access beyond screenshots
- Restricted to localhost API server

### 2. Input Validation
- Zod schemas prevent malicious parameters
- Bounded numeric inputs (positions, sizes)
- Enum validation for mode parameters

### 3. API Server Security
- API server runs on localhost only
- Unix socket IPC for C++ communication
- No external network access

## Implementation Timeline

### Week 1: Foundation
- [ ] Add MCP dependencies to bridge
- [ ] Create basic MCP tools module  
- [ ] Test simple tool execution

### Week 2: Core Tools
- [ ] Implement window management tools
- [ ] Add type-safe schemas
- [ ] Test with wibwob chat

### Week 3: Advanced Features
- [ ] Add batch layout tools
- [ ] Implement text/figlet tools
- [ ] Add error handling & status display

### Week 4: Polish & Testing
- [ ] Comprehensive testing
- [ ] Documentation updates
- [ ] Performance optimisation

## Success Metrics

1. **Tool Integration**: All 10 core MCP tools working with wibwob
2. **Type Safety**: 100% Zod schema coverage with validation
3. **Error Handling**: Graceful handling of API server failures
4. **User Experience**: Natural language prompts successfully trigger tools
5. **Performance**: Tool execution under 500ms for simple operations

## Architecture Benefits

### For AI (Claude)
- Rich set of TUI control capabilities
- Type-safe tool interfaces
- Immediate visual feedback in TUI
- Complex multi-step operations possible

### For Users
- More powerful wibwob chat experience
- AI can create visual demonstrations
- Dynamic window management
- Interactive learning and exploration

### For Developers
- Clean separation of concerns
- Reusable MCP tools architecture
- Type-safe tool development
- Easy to extend with new capabilities

This plan provides a comprehensive path to integrating MCP tools into wibwob while maintaining the existing architecture and ensuring robust, type-safe operation.