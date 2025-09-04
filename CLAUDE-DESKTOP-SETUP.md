# Claude Desktop MCP Configuration for TUI Control

## Overview
To use the TUI control API with Claude Desktop app (not Claude Code CLI), you need to configure an MCP server in Claude Desktop's settings.

## Configuration Steps

### 1. Locate Claude Desktop Config
Claude Desktop stores MCP configuration in:
- **macOS**: `~/Library/Application Support/Claude/claude_desktop_config.json`
- **Windows**: `%APPDATA%\Claude\claude_desktop_config.json`

### 2. Add TUI Control MCP Server
Add this configuration to your `claude_desktop_config.json`:

```json
{
  "mcpServers": {
    "tui-control": {
      "command": "node",
      "args": [
        "-e",
        "const http = require('http'); const url = require('url'); const server = http.createServer((req, res) => { if (req.method === 'POST') { let body = ''; req.on('data', chunk => { body += chunk; }); req.on('end', () => { const mcpReq = JSON.parse(body); const options = { hostname: '127.0.0.1', port: 8089, path: '/mcp', method: 'POST', headers: { 'Content-Type': 'application/json', 'Accept': 'application/json, text/event-stream' } }; const proxyReq = http.request(options, (proxyRes) => { let data = ''; proxyRes.on('data', chunk => { data += chunk; }); proxyRes.on('end', () => { res.writeHead(proxyRes.statusCode, proxyRes.headers); res.end(data); }); }); proxyReq.on('error', (e) => { res.writeHead(500); res.end(JSON.stringify({error: e.message})); }); proxyReq.write(body); proxyReq.end(); }); } }); server.listen(0, () => { console.log('MCP proxy listening on port', server.address().port); });"
      ],
      "env": {}
    }
  }
}
```

**⚠️ IMPORTANT**: This approach creates a proxy because Claude Desktop expects stdio/command-based MCP servers, but our server uses HTTP.

### 3. Simpler Alternative: Use MCP Bridge
Instead of the complex proxy, use this simpler Python-based bridge:

```json
{
  "mcpServers": {
    "tui-control": {
      "command": "python3",
      "args": [
        "-c",
        "import sys, json, requests, io; [print(json.dumps({'jsonrpc': '2.0', 'id': msg['id'], 'result': requests.post('http://127.0.0.1:8089/mcp', json=msg, headers={'Accept': 'application/json, text/event-stream'}).json().get('result', {})})) for line in sys.stdin for msg in [json.loads(line)]]"
      ],
      "env": {}
    }
  }
}
```

### 4. Best Approach: MCP Stdio Wrapper
Create a dedicated MCP stdio wrapper script:

**File: `~/tui-mcp-bridge.py`**
```python
#!/usr/bin/env python3
import sys
import json
import requests

def main():
    while True:
        try:
            line = sys.stdin.readline()
            if not line:
                break
            
            msg = json.loads(line.strip())
            
            # Forward to HTTP MCP server
            response = requests.post(
                'http://127.0.0.1:8089/mcp',
                json=msg,
                headers={
                    'Content-Type': 'application/json',
                    'Accept': 'application/json, text/event-stream'
                }
            )
            
            # Return response via stdout
            print(json.dumps(response.json()))
            sys.stdout.flush()
            
        except Exception as e:
            error_response = {
                "jsonrpc": "2.0",
                "id": msg.get("id") if 'msg' in locals() else None,
                "error": {"code": -32603, "message": str(e)}
            }
            print(json.dumps(error_response))
            sys.stdout.flush()

if __name__ == "__main__":
    main()
```

**Then configure Claude Desktop:**
```json
{
  "mcpServers": {
    "tui-control": {
      "command": "python3",
      "args": ["/Users/james/tui-mcp-bridge.py"],
      "env": {}
    }
  }
}
```

## Usage in Claude Desktop

Once configured, you can use natural language in Claude Desktop:

- "Show me the current TUI window state"
- "Create a test pattern window"
- "Move the focused window to position 50, 20"
- "Arrange all windows in a cascade layout"
- "Take a screenshot of the TUI application"

## Prerequisites

1. **TUI Application Running**: `cd test-tui && ./build/test_pattern`
2. **API Server Running**: `/path/to/venv/bin/python -m tools.api_server.main --port=8089`
3. **Claude Desktop Restarted**: Restart Claude Desktop after config changes

## Limitations

- Claude Desktop MCP integration requires restart after config changes
- HTTP-to-stdio bridging adds slight latency
- Error handling may be less robust than native MCP implementations

## Alternative: Use Claude Code CLI Instead

For better MCP integration, consider using Claude Code CLI in headless mode:
```bash
claude -p --mcp-config=.mcp.json "Create a gradient window and move it to 30,10"
```