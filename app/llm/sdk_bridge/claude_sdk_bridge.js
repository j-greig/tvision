#!/usr/bin/env node
/**
 * Claude Code SDK Bridge for C++ TUI Applications
 * 
 * Provides streaming interface between C++ and Claude Code SDK
 * Supports customSystemPrompt and real-time response streaming
 */

const { query } = require('@anthropic-ai/claude-code');
const process = require('process');
const readline = require('readline');
const { createTuiMcpServer } = require('./mcp_tools');

class ClaudeSDKBridge {
    constructor() {
        this.activeSession = null;
        this.sessionId = null;
        this.customSystemPrompt = null;
        this.allowedTools = ['Read', 'Write', 'Grep', 'Bash', 'LS', 'WebSearch', 'WebFetch'];
        this.maxTurns = 50;
        
        // Initialize MCP server for TUI control tools
        try {
            console.error('🔧 Creating MCP server...');
            this.mcpServer = createTuiMcpServer();
            console.error('🔧 MCP server created successfully');
            console.error('🔧 MCP server name:', this.mcpServer.name);
            console.error('🔧 MCP server object keys:', Object.keys(this.mcpServer));
            console.error('🔧 MCP server tools property:', typeof this.mcpServer.tools);
            console.error('🔧 MCP server tools count:', this.mcpServer.tools ? Object.keys(this.mcpServer.tools).length : 'NO TOOLS');
            if (this.mcpServer.tools) {
                console.error('🔧 Tool names:', Object.keys(this.mcpServer.tools));
            }
        } catch (error) {
            console.error('💥 MCP server creation FAILED:', error.message);
            console.error('💥 Stack:', error.stack);
            this.mcpServer = null;
        }
        
        this.setupInputHandler();
        this.sendResponse('BRIDGE_READY', { version: '1.0.0', mcpTools: 'enabled' });
        
        // MASSIVE DEBUG: Log bridge startup
        console.error('🚀 BRIDGE STARTED - PID:', process.pid);
        console.error('🚀 MCP Server Created:', !!this.mcpServer);
    }
    
    setupInputHandler() {
        const rl = readline.createInterface({
            input: process.stdin,
            output: process.stdout,
            terminal: false
        });
        
        rl.on('line', (line) => {
            console.error('🔥 BRIDGE INPUT RECEIVED:', line);
            
            // Only process lines that look like JSON commands
            const trimmed = line.trim();
            if (!trimmed.startsWith('{') || !trimmed.endsWith('}')) {
                console.error('🔥 SKIPPING NON-JSON LINE:', trimmed.substring(0, 50) + '...');
                return;
            }
            
            try {
                const command = JSON.parse(trimmed);
                if (command.type) {
                    console.error('🔥 PARSED COMMAND:', command.type);
                    this.handleCommand(command);
                } else {
                    console.error('🔥 SKIPPING JSON WITHOUT TYPE:', command);
                }
            } catch (error) {
                console.error('🔥 PARSE ERROR:', error.message);
                // Don't send error for non-JSON lines, just ignore them
            }
        });
        
        rl.on('close', () => {
            this.cleanup();
            process.exit(0);
        });
    }
    
    async handleCommand(command) {
        try {
            switch (command.type) {
                case 'START_SESSION':
                    await this.startSession(command.data);
                    break;
                    
                case 'SEND_QUERY':
                    await this.sendQuery(command.data);
                    break;
                    
                case 'UPDATE_PROMPT':
                    await this.updateSystemPrompt(command.data);
                    break;
                    
                case 'END_SESSION':
                    await this.endSession();
                    break;
                    
                case 'CONFIGURE':
                    this.configure(command.data);
                    break;
                    
                default:
                    this.sendError('UNKNOWN_COMMAND', `Unknown command type: ${command.type}`);
            }
        } catch (error) {
            this.sendError('COMMAND_ERROR', error.message);
        }
    }
    
    async startSession(data) {
        try {
            this.customSystemPrompt = data.customSystemPrompt;
            this.sessionId = this.generateSessionId();
            
            console.error('DEBUG: Starting session with customSystemPrompt length:', this.customSystemPrompt?.length || 0);
            
            // Store session configuration
            this.sessionConfig = {
                customSystemPrompt: this.customSystemPrompt,
                maxTurns: data.maxTurns || this.maxTurns,
                allowedTools: data.allowedTools || this.allowedTools,
                model: data.model || 'sonnet'  // Default to sonnet if not specified
            };
            
            this.sendResponse('SESSION_STARTED', {
                sessionId: this.sessionId,
                customSystemPrompt: this.customSystemPrompt,
                configuration: this.sessionConfig
            });
            
            console.error('DEBUG: Session started with model:', this.sessionConfig.model);
            
        } catch (error) {
            this.sendError('SESSION_START_ERROR', error.message);
        }
    }
    
    async sendQuery(data) {
        if (!this.sessionId) {
            this.sendError('NO_SESSION', 'No active session. Call START_SESSION first.');
            return;
        }
        
        try {
            // MASSIVE DEBUG: Log everything about this request
            console.error('=== BRIDGE DEBUG: sendQuery called ===');
            console.error('Query:', data.query);
            console.error('Query contains "window":', data.query.toLowerCase().includes('window'));
            console.error('Query contains "create":', data.query.toLowerCase().includes('create'));
            console.error('Session ID:', this.sessionId);
            console.error('Allowed Tools:', this.sessionConfig.allowedTools);
            console.error('MCP Server initialized:', !!this.mcpServer);
            console.error('=== END BRIDGE DEBUG ===');
            
            // Start streaming query
            this.sendResponse('QUERY_STARTED', { 
                sessionId: this.sessionId,
                query: data.query 
            });
            
            // Create message generator for streaming input
            const messageGenerator = this.createMessageGenerator(data.query);
            
            let fullResponse = '';
            
            // Use Claude Code SDK with proper customSystemPrompt and MCP tools
            console.error('DEBUG: About to query with model:', this.sessionConfig.model);
            console.error('DEBUG: MCP server initialized:', !!this.mcpServer);
            
            // Build allowed tools list including MCP tools
            const mcpTools = [
                "mcp__tui-control__tui_create_window",
                "mcp__tui-control__tui_move_window", 
                "mcp__tui-control__tui_get_state",
                "mcp__tui-control__tui_close_window",
                "mcp__tui-control__tui_cascade_windows",
                "mcp__tui-control__tui_tile_windows",
                "mcp__tui-control__tui_send_text",
                "mcp__tui-control__tui_send_figlet"
            ];
            
            const allAllowedTools = [...this.sessionConfig.allowedTools, ...mcpTools];
            console.error('DEBUG: Allowed tools:', allAllowedTools);
            
            for await (const message of query({
                prompt: messageGenerator,
                options: {
                    customSystemPrompt: this.customSystemPrompt,  // Proper SDK parameter!
                    maxTurns: this.sessionConfig.maxTurns,
                    allowedTools: allAllowedTools,  // Include MCP tools in allowed list
                    model: this.sessionConfig.model || 'sonnet',  // Model selection
                    mcpServers: {
                        "tui-control": this.mcpServer  // Pass as object/dictionary
                    }
                }
            })) {
                console.error('=== SDK MESSAGE DEBUG ===');
                console.error('Message type:', message.type);
                console.error('Message data:', JSON.stringify(message, null, 2));
                console.error('=== END SDK MESSAGE DEBUG ===');

                if (message.type === 'assistant') {
                    const content = message.message.content;

                    // Send streaming chunk
                    this.sendResponse('CONTENT_DELTA', {
                        sessionId: this.sessionId,
                        content: content,
                        isPartial: true
                    });

                    fullResponse += content;

                } else if (message.type === 'result') {
                    // Handle SDK result messages (errors and completion)
                    if (message.result === 'error_max_turns') {
                        this.sendResponse('ERROR_OCCURRED', {
                            sessionId: this.sessionId,
                            error: 'MAX_TURNS_EXCEEDED',
                            message: 'Conversation turn limit reached'
                        });
                        return;
                    } else if (message.result === 'error_during_execution') {
                        this.sendResponse('ERROR_OCCURRED', {
                            sessionId: this.sessionId,
                            error: 'EXECUTION_ERROR',
                            message: message.error?.message || 'Unknown execution error'
                        });
                        return;
                    }
                    // Normal completion handled after loop
                }
            }

            // Send completion
            this.sendResponse('MESSAGE_COMPLETE', {
                sessionId: this.sessionId,
                fullResponse: fullResponse,
                isPartial: false
            });
            
        } catch (error) {
            this.sendError('QUERY_ERROR', error.message);
        }
    }
    
    async* createMessageGenerator(userQuery) {
        yield {
            type: "user",
            message: {
                role: "user",
                content: userQuery
            }
        };
    }
    
    async updateSystemPrompt(data) {
        this.customSystemPrompt = data.customSystemPrompt;
        if (this.sessionConfig) {
            this.sessionConfig.customSystemPrompt = this.customSystemPrompt;
        }
        
        this.sendResponse('PROMPT_UPDATED', {
            sessionId: this.sessionId,
            customSystemPrompt: this.customSystemPrompt
        });
    }
    
    async endSession() {
        const oldSessionId = this.sessionId;
        
        this.sessionId = null;
        this.customSystemPrompt = null;
        this.sessionConfig = null;
        
        this.sendResponse('SESSION_ENDED', {
            sessionId: oldSessionId
        });
    }
    
    configure(data) {
        if (data.allowedTools) {
            this.allowedTools = data.allowedTools;
        }
        if (data.maxTurns) {
            this.maxTurns = data.maxTurns;
        }
        
        this.sendResponse('CONFIGURED', {
            allowedTools: this.allowedTools,
            maxTurns: this.maxTurns
        });
    }
    
    
    generateSessionId() {
        return 'wib_' + Date.now().toString(36) + '_' + Math.random().toString(36).substr(2, 9);
    }
    
    sendResponse(type, data) {
        const response = {
            type: type,
            data: data,
            timestamp: Date.now()
        };
        
        console.log(JSON.stringify(response));
    }
    
    sendError(errorType, message) {
        this.sendResponse('ERROR', {
            errorType: errorType,
            message: message
        });
    }
    
    cleanup() {
        if (this.sessionId) {
            this.endSession();
        }
    }
}

// Handle process termination
process.on('SIGINT', () => {
    process.exit(0);
});

process.on('SIGTERM', () => {
    process.exit(0);
});

// Start the bridge
new ClaudeSDKBridge();