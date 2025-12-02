/*---------------------------------------------------------*/
/*                                                         */
/*   claude_code_sdk_provider.cpp - Claude Code SDK Provider*/
/*   Streaming mode with customSystemPrompt support       */
/*                                                         */
/*---------------------------------------------------------*/

#include "claude_code_sdk_provider.h"
#include "claude_code_provider.h" // Fallback
#include "../base/llm_provider_factory.h"

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <iostream>

// Register this provider with the factory
REGISTER_LLM_PROVIDER("claude_code_sdk", ClaudeCodeSDKProvider);

// Node.js bridge process management
struct ClaudeCodeSDKProvider::NodeBridge {
    FILE* input = nullptr;
    FILE* output = nullptr;
    pid_t pid = -1;
    std::string scriptPath;
    bool active = false;
    
    ~NodeBridge() {
        shutdown();
    }
    
    bool start(const std::string& script) {
        if (active) return true;
        
        scriptPath = script;
        
        // Create pipes for communication
        int inputPipe[2], outputPipe[2];
        if (pipe(inputPipe) == -1 || pipe(outputPipe) == -1) {
            return false;
        }
        
        pid = fork();
        if (pid == -1) {
            close(inputPipe[0]); close(inputPipe[1]);
            close(outputPipe[0]); close(outputPipe[1]);
            return false;
        }
        
        if (pid == 0) {
            // Child process
            close(inputPipe[1]);   // Close write end of input
            close(outputPipe[0]);  // Close read end of output
            
            dup2(inputPipe[0], STDIN_FILENO);
            dup2(outputPipe[1], STDOUT_FILENO);
            
            close(inputPipe[0]);
            close(outputPipe[1]);
            
            // Execute Node.js bridge
            execl("/usr/bin/env", "env", "node", scriptPath.c_str(), nullptr);
            _exit(1); // execl failed
        } else {
            // Parent process
            close(inputPipe[0]);   // Close read end of input
            close(outputPipe[1]);  // Close write end of output
            
            input = fdopen(inputPipe[1], "w");
            output = fdopen(outputPipe[0], "r");
            
            if (!input || !output) {
                shutdown();
                return false;
            }
            
            // Set non-blocking mode on output
            int flags = fcntl(fileno(output), F_GETFL, 0);
            fcntl(fileno(output), F_SETFL, flags | O_NONBLOCK);
            
            active = true;
            return true;
        }
    }
    
    void shutdown() {
        if (!active) return;
        
        if (input) {
            fclose(input);
            input = nullptr;
        }
        
        if (output) {
            fclose(output);
            output = nullptr;
        }
        
        if (pid > 0) {
            kill(pid, SIGTERM);
            int status;
            waitpid(pid, &status, WNOHANG);
            pid = -1;
        }
        
        active = false;
    }
    
    bool sendCommand(const std::string& command) {
        if (!active || !input) return false;
        
        fprintf(input, "%s\n", command.c_str());
        fflush(input);
        return true;
    }
    
    std::string readResponse() {
        if (!active || !output) return "";
        
        char buffer[4096];
        if (fgets(buffer, sizeof(buffer), output)) {
            std::string result(buffer);
            if (!result.empty() && result.back() == '\n') {
                result.pop_back();
            }
            return result;
        }
        return "";
    }
};

ClaudeCodeSDKProvider::ClaudeCodeSDKProvider() 
    : nodeBridge(std::make_unique<NodeBridge>()) {
    
    // Default script path - will be overridden by configuration
    nodeScriptPath = "llm/sdk_bridge/claude_sdk_bridge.js";
    
    // Initialize fallback provider
    initializeFallback();
}

ClaudeCodeSDKProvider::~ClaudeCodeSDKProvider() {
    cancel();
    shutdownSDK();
}

bool ClaudeCodeSDKProvider::isAvailable() const {
    // Check if Node.js is available
    int result = system("which node > /dev/null 2>&1");
    if (result != 0) {
        fprintf(stderr, "DEBUG: Node.js not available\n");
        return false;
    }
    
    // Check if bridge script exists
    bool scriptExists = access(nodeScriptPath.c_str(), R_OK) == 0;
    if (!scriptExists) {
        fprintf(stderr, "DEBUG: Bridge script not found at: %s\n", nodeScriptPath.c_str());
        return false;
    }
    
    fprintf(stderr, "DEBUG: claude_code_sdk provider is available\n");
    return true;
}

bool ClaudeCodeSDKProvider::sendQuery(const LLMRequest& request, ResponseCallback callback) {
    if (busy.load()) {
        return false;
    }
    
    clearError();
    
    // Try SDK approach first
    if (isAvailable() && !useFallback) {
        if (!streamingActive) {
            // Start session with current system prompt
            if (!startStreamingSession(request.system_prompt)) {
                // Fall back to legacy provider
                return tryFallback(request, callback);
            }
        }
        
        // Store callback for completion
        activeFallbackCallback = callback;
        
        // Create streaming callback that buffers for final response
        std::string* responseBuffer = new std::string();
        auto streamCallback = [this, callback, responseBuffer](const StreamChunk& chunk) {
            if (chunk.type == StreamChunk::CONTENT_DELTA) {
                *responseBuffer += chunk.content;
            } else if (chunk.type == StreamChunk::MESSAGE_COMPLETE) {
                // Convert to LLMResponse format
                LLMResponse response;
                response.provider_name = getProviderName();
                response.result = *responseBuffer;
                response.session_id = chunk.session_id;
                response.is_error = false;
                
                if (callback) callback(response);
                delete responseBuffer;
                
                activeFallbackCallback = nullptr;
            } else if (chunk.type == StreamChunk::ERROR_OCCURRED) {
                LLMResponse response;
                response.provider_name = getProviderName();
                response.is_error = true;
                response.error_message = chunk.error_message;
                
                if (callback) callback(response);
                delete responseBuffer;
                
                activeFallbackCallback = nullptr;
            }
        };
        
        return sendStreamingQuery(request.message, streamCallback);
    }
    
    // Fallback to legacy provider
    return tryFallback(request, callback);
}

bool ClaudeCodeSDKProvider::tryFallback(const LLMRequest& request, ResponseCallback callback) {
    if (!fallbackProvider) {
        LLMResponse response;
        response.provider_name = getProviderName();
        response.is_error = true;
        response.error_message = "SDK unavailable and no fallback provider";
        if (callback) callback(response);
        return false;
    }
    
    useFallback = true;
    return fallbackProvider->sendQuery(request, callback);
}

bool ClaudeCodeSDKProvider::startStreamingSession(const std::string& customSystemPrompt) {
    if (streamingActive) {
        return true; // Already active
    }
    
    if (!initializeSDK()) {
        setError("Failed to initialize Claude Code SDK");
        return false;
    }
    
    // Send session start command
    std::ostringstream command;
    command << R"({"type":"START_SESSION","data":{"customSystemPrompt":")" 
            << customSystemPrompt << R"(","maxTurns":)" << maxTurns 
            << R"(,"allowedTools":["Read","Write","Grep","Bash","LS","WebSearch","WebFetch","mcp__tui-control__tui_create_window","mcp__tui-control__tui_move_window","mcp__tui-control__tui_get_state","mcp__tui-control__tui_close_window","mcp__tui-control__tui_cascade_windows","mcp__tui-control__tui_tile_windows","mcp__tui-control__tui_send_text","mcp__tui-control__tui_send_figlet"],"model":"sonnet"}})";
    
    if (!nodeBridge->sendCommand(command.str())) {
        setError("Failed to send session start command");
        return false;
    }
    
    // Wait for session confirmation
    for (int i = 0; i < 50; ++i) { // 5 second timeout
        std::string response = nodeBridge->readResponse();
        if (!response.empty()) {
            // Parse JSON response (simplified)
            if (response.find("SESSION_STARTED") != std::string::npos) {
                streamingActive = true;
                sessionStarted = true;
                currentSystemPrompt = customSystemPrompt;
                
                // Extract session ID (simplified JSON parsing)
                size_t idPos = response.find("\"sessionId\":\"");
                if (idPos != std::string::npos) {
                    idPos += 13; // Length of "sessionId":"
                    size_t endPos = response.find("\"", idPos);
                    if (endPos != std::string::npos) {
                        currentSessionId = response.substr(idPos, endPos - idPos);
                    }
                }
                
                return true;
            } else if (response.find("ERROR") != std::string::npos) {
                setError("Session start failed: " + response);
                return false;
            }
        }
        usleep(100000); // 100ms delay
    }
    
    setError("Session start timeout");
    return false;
}

bool ClaudeCodeSDKProvider::sendStreamingQuery(const std::string& query, StreamingCallback streamCallback) {
    if (!streamingActive || !nodeBridge->active) {
        return false;
    }
    
    busy.store(true);
    activeStreamCallback = streamCallback;
    
    // Send query command
    std::ostringstream command;
    command << R"({"type":"SEND_QUERY","data":{"query":")" << query << R"("}})";
    
    if (!nodeBridge->sendCommand(command.str())) {
        setError("Failed to send query command");
        busy.store(false);
        return false;
    }
    
    // Start processing thread
    if (!processingActive.load()) {
        processingActive.store(true);
        processingThread = std::make_unique<std::thread>(&ClaudeCodeSDKProvider::processStreamingThread, this);
    }
    
    return true;
}

void ClaudeCodeSDKProvider::processStreamingThread() {
    while (processingActive.load() && !shouldCancel.load()) {
        std::string response = nodeBridge->readResponse();
        
        if (!response.empty()) {
            StreamChunk chunk;
            
            // Parse response (simplified JSON parsing)
            if (response.find("CONTENT_DELTA") != std::string::npos) {
                chunk.type = StreamChunk::CONTENT_DELTA;
                
                // Extract content
                size_t contentPos = response.find("\"content\":\"");
                if (contentPos != std::string::npos) {
                    contentPos += 11; // Length of "content":"
                    size_t endPos = response.find("\"", contentPos);
                    if (endPos != std::string::npos) {
                        chunk.content = response.substr(contentPos, endPos - contentPos);
                    }
                }
                
                if (activeStreamCallback) {
                    activeStreamCallback(chunk);
                }
                
            } else if (response.find("MESSAGE_COMPLETE") != std::string::npos) {
                chunk.type = StreamChunk::MESSAGE_COMPLETE;
                chunk.session_id = currentSessionId;
                
                if (activeStreamCallback) {
                    activeStreamCallback(chunk);
                }
                
                busy.store(false);
                activeStreamCallback = nullptr;
                break;
                
            } else if (response.find("ERROR") != std::string::npos) {
                chunk.type = StreamChunk::ERROR_OCCURRED;
                chunk.error_message = response;
                
                if (activeStreamCallback) {
                    activeStreamCallback(chunk);
                }
                
                busy.store(false);
                activeStreamCallback = nullptr;
                break;
            }
        }
        
        usleep(50000); // 50ms polling
    }
    
    processingActive.store(false);
}

bool ClaudeCodeSDKProvider::isBusy() const {
    if (useFallback && fallbackProvider) {
        return fallbackProvider->isBusy();
    }
    return busy.load();
}

void ClaudeCodeSDKProvider::cancel() {
    if (useFallback && fallbackProvider) {
        fallbackProvider->cancel();
        return;
    }
    
    shouldCancel.store(true);
    busy.store(false);
    
    if (processingThread && processingThread->joinable()) {
        processingThread->join();
        processingThread.reset();
    }
    
    if (activeStreamCallback) {
        StreamChunk chunk;
        chunk.type = StreamChunk::ERROR_OCCURRED;
        chunk.error_message = "Request cancelled by user";
        activeStreamCallback(chunk);
        activeStreamCallback = nullptr;
    }
    
    shouldCancel.store(false);
}

void ClaudeCodeSDKProvider::poll() {
    if (useFallback && fallbackProvider) {
        fallbackProvider->poll();
    }
    // SDK version doesn't need polling - uses threads
}

bool ClaudeCodeSDKProvider::initializeSDK() {
    if (nodeBridge->active) {
        return true;
    }
    
    return nodeBridge->start(nodeScriptPath);
}

void ClaudeCodeSDKProvider::shutdownSDK() {
    if (processingThread && processingThread->joinable()) {
        processingActive.store(false);
        processingThread->join();
        processingThread.reset();
    }
    
    nodeBridge->shutdown();
    streamingActive = false;
    sessionStarted = false;
}

std::string ClaudeCodeSDKProvider::getVersion() const {
    return "SDK v1.0.0";
}

std::vector<std::string> ClaudeCodeSDKProvider::getSupportedModels() const {
    return {"claude-3-opus", "claude-3-sonnet", "claude-3-haiku"};
}

bool ClaudeCodeSDKProvider::configure(const std::string& config) {
    // Parse configuration (tolerant JSON-ish parsing without throwing)
    fprintf(stderr, "DEBUG: SDK Provider configure() called with: %s\n", config.c_str());

    auto parseIntField = [&config](const std::string& key, int defaultValue) -> int {
        size_t keyPos = config.find(key);
        if (keyPos == std::string::npos) return defaultValue;
        size_t colonPos = config.find(":", keyPos);
        if (colonPos == std::string::npos) return defaultValue;

        // Skip spaces
        size_t valPos = colonPos + 1;
        while (valPos < config.size() && (config[valPos] == ' ' || config[valPos] == '\t')) {
            ++valPos;
        }

        // Extract numeric text, supporting optional quotes
        std::string numStr;
        if (valPos < config.size() && config[valPos] == '"') {
            // Quoted number: "123"
            ++valPos;
            size_t endQuote = config.find('"', valPos);
            if (endQuote == std::string::npos) return defaultValue;
            numStr = config.substr(valPos, endQuote - valPos);
        } else {
            // Unquoted: read digits/sign until delimiter
            size_t endPos = valPos;
            while (endPos < config.size()) {
                char c = config[endPos];
                if ((c >= '0' && c <= '9') || c == '-' || c == '+') {
                    ++endPos;
                } else {
                    break;
                }
            }
            if (endPos == valPos) return defaultValue;
            numStr = config.substr(valPos, endPos - valPos);
        }

        try {
            return std::stoi(numStr);
        } catch (...) {
            return defaultValue;
        }
    };

    auto parseStringField = [&config](const std::string& key, std::string defaultValue) -> std::string {
        size_t keyPos = config.find(key);
        if (keyPos == std::string::npos) return defaultValue;
        size_t colonPos = config.find(":", keyPos);
        if (colonPos == std::string::npos) return defaultValue;
        size_t startQuote = config.find('"', colonPos);
        if (startQuote == std::string::npos) return defaultValue;
        ++startQuote;
        size_t endQuote = config.find('"', startQuote);
        if (endQuote == std::string::npos) return defaultValue;
        return config.substr(startQuote, endQuote - startQuote);
    };

    // maxTurns (quoted or numeric)
    maxTurns = parseIntField("maxTurns", maxTurns);

    // nodeScriptPath
    nodeScriptPath = parseStringField("nodeScriptPath", nodeScriptPath);

    // sessionTimeout (quoted or numeric)
    sessionTimeout = parseIntField("sessionTimeout", sessionTimeout);

    // allowedTools: if present, keep defaults for now
    allowedTools.clear();
    if (config.find("allowedTools") != std::string::npos) {
        allowedTools = {"Read", "Write", "Grep", "Bash", "LS"};
    }

    return true;
}

void ClaudeCodeSDKProvider::resetSession() {
    endStreamingSession();
    currentSessionId.clear();
}

void ClaudeCodeSDKProvider::endStreamingSession() {
    if (!streamingActive) return;
    
    if (nodeBridge->active) {
        nodeBridge->sendCommand(R"({"type":"END_SESSION","data":{}})");
    }
    
    streamingActive = false;
    sessionStarted = false;
    currentSessionId.clear();
}

bool ClaudeCodeSDKProvider::updateSystemPrompt(const std::string& customSystemPrompt) {
    if (!streamingActive) {
        currentSystemPrompt = customSystemPrompt;
        return true;
    }
    
    std::ostringstream command;
    command << R"({"type":"UPDATE_PROMPT","data":{"customSystemPrompt":")" 
            << customSystemPrompt << R"("}})";
    
    bool success = nodeBridge->sendCommand(command.str());
    if (success) {
        currentSystemPrompt = customSystemPrompt;
    }
    
    return success;
}

void ClaudeCodeSDKProvider::registerTool(const Tool& tool) {
    registeredTools.push_back(tool);
}

void ClaudeCodeSDKProvider::clearTools() {
    registeredTools.clear();
}

void ClaudeCodeSDKProvider::setError(const std::string& error) {
    lastError = error;
}

void ClaudeCodeSDKProvider::clearError() {
    lastError.clear();
}

bool ClaudeCodeSDKProvider::initializeFallback() {
    try {
        fallbackProvider = std::make_unique<ClaudeCodeProvider>();
        return fallbackProvider->isAvailable();
    } catch (...) {
        fallbackProvider.reset();
        return false;
    }
}
