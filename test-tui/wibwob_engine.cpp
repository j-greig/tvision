/*---------------------------------------------------------*/
/*                                                         */
/*   wibwob_engine.cpp - Claude Code Integration Engine   */
/*                                                         */
/*---------------------------------------------------------*/

#include "wibwob_engine.h"

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>

WibWobEngine::WibWobEngine() {
    // Default system prompt for wib&wob
    systemPrompt = "You are wib&wob, a helpful AI assistant integrated into a Turbo Vision TUI application.";
}

WibWobEngine::~WibWobEngine() {
    // Cancel any pending request
    cancel();
}

bool WibWobEngine::sendQuery(const std::string& query, ResponseCallback callback) {
    if (busy || query.empty()) {
        return false;
    }
    
    clearError();
    
    // Start async command execution
    return startAsyncCommand(query, callback);
}

void WibWobEngine::poll() {
    if (!busy || !activePipe) {
        return;
    }
    
    // Read available data from pipe (non-blocking)
    char buffer[4096];
    size_t bytesRead = fread(buffer, 1, sizeof(buffer) - 1, activePipe);
    
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        outputBuffer += buffer;
    }
    
    // Check if process is done
    if (feof(activePipe) || ferror(activePipe)) {
        int exitCode = pclose(activePipe);
        activePipe = nullptr;
        busy = false;
        
        // Parse response and call callback
        ClaudeResponse response;
        if (exitCode == 0) {
            response = parseClaudeResponse(outputBuffer);
        } else {
            response.is_error = true;
            response.error_message = "Claude command failed with exit code " + std::to_string(exitCode);
            if (!outputBuffer.empty()) {
                response.error_message += ": " + outputBuffer;
            }
        }
        
        // Update session ID
        if (!response.session_id.empty()) {
            currentSessionId = response.session_id;
        }
        
        // Call the callback
        if (pendingCallback) {
            pendingCallback(response);
            pendingCallback = nullptr;
        }
        
        outputBuffer.clear();
    }
}

void WibWobEngine::cancel() {
    if (busy && activePipe) {
        pclose(activePipe);
        activePipe = nullptr;
        busy = false;
        
        if (pendingCallback) {
            ClaudeResponse response;
            response.is_error = true;
            response.error_message = "Request cancelled by user";
            pendingCallback(response);
            pendingCallback = nullptr;
        }
        
        outputBuffer.clear();
    }
}

bool WibWobEngine::isClaudeAvailable() const {
    // Try to run claude --version to check availability
    std::string command = claudePath + " --version 2>/dev/null";
    
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return false;
    }
    
    char buffer[128];
    bool hasOutput = fgets(buffer, sizeof(buffer), pipe) != nullptr;
    int exitCode = pclose(pipe);
    
    return exitCode == 0 && hasOutput;
}

void WibWobEngine::setSystemPrompt(const std::string& prompt) {
    systemPrompt = prompt;
}

void WibWobEngine::setClaudePath(const std::string& path) {
    claudePath = path;
}

bool WibWobEngine::startAsyncCommand(const std::string& query, ResponseCallback callback) {
    if (!isClaudeAvailable()) {
        ClaudeResponse response;
        response.is_error = true;
        response.error_message = "Claude Code binary not found at: " + claudePath;
        setError(response.error_message);
        if (callback) callback(response);
        return false;
    }
    
    // Build the command
    std::string command = buildClaudeCommand(query);
    
    // Start async execution
    activePipe = popen(command.c_str(), "r");
    if (!activePipe) {
        ClaudeResponse response;
        response.is_error = true;
        response.error_message = "Failed to execute Claude command";
        setError(response.error_message);
        if (callback) callback(response);
        return false;
    }
    
    // Set non-blocking mode on pipe
    int fd = fileno(activePipe);
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    
    busy = true;
    pendingCallback = callback;
    outputBuffer.clear();
    
    return true;
}

ClaudeResponse WibWobEngine::executeClaudeCommand(const std::string& query) {
    ClaudeResponse response;
    
    if (!isClaudeAvailable()) {
        response.is_error = true;
        response.error_message = "Claude Code binary not found at: " + claudePath;
        setError(response.error_message);
        return response;
    }
    
    // Build the command
    std::string command = buildClaudeCommand(query);
    
    // Debug: Print the command being executed (remove in production)
    // fprintf(stderr, "Executing: %s\n", command.c_str());
    
    // Execute the command
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        response.is_error = true;
        response.error_message = "Failed to execute Claude command";
        setError(response.error_message);
        return response;
    }
    
    // Read the output
    std::string output;
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }
    
    int exitCode = pclose(pipe);
    
    if (exitCode != 0) {
        response.is_error = true;
        response.error_message = "Claude command failed with exit code " + std::to_string(exitCode);
        if (!output.empty()) {
            response.error_message += ": " + output;
        }
        setError(response.error_message);
        return response;
    }
    
    // Parse the JSON response
    response = parseClaudeResponse(output);
    
    // Update session ID if we got one
    if (!response.session_id.empty()) {
        currentSessionId = response.session_id;
    }
    
    return response;
}

std::string WibWobEngine::buildClaudeCommand(const std::string& query) const {
    std::ostringstream cmd;
    
    cmd << claudePath << " -p";
    cmd << " --output-format json";
    
    // Add continue flag if we have a session
    if (!currentSessionId.empty()) {
        cmd << " --continue";
    }
    
    // Check if wibandwob.prompt.md exists and use it as system prompt
    FILE* promptCheck = fopen("wibandwob.prompt.md", "r");
    if (promptCheck) {
        fclose(promptCheck);
        cmd << " --system-prompt-file wibandwob.prompt.md";
    } else {
        // Fallback to simple system prompt if file doesn't exist
        if (!systemPrompt.empty()) {
            cmd << " --append-system-prompt \"";
            for (char c : systemPrompt) {
                if (c == '"' || c == '\\' || c == '$' || c == '`') {
                    cmd << '\\';
                }
                cmd << c;
            }
            cmd << "\"";
        }
    }
    
    // Escape the query for shell
    cmd << " \"";
    for (char c : query) {
        if (c == '"' || c == '\\' || c == '$' || c == '`') {
            cmd << '\\';
        }
        cmd << c;
    }
    cmd << "\"";
    
    cmd << " 2>&1";  // Capture stderr too
    
    return cmd.str();
}

ClaudeResponse WibWobEngine::parseClaudeResponse(const std::string& json) const {
    ClaudeResponse response;
    
    if (json.empty()) {
        response.is_error = true;
        response.error_message = "Empty response from Claude";
        return response;
    }
    
    // Simple JSON parsing (for POC - production would use a proper JSON library)
    response.result = extractJsonField(json, "result");
    response.session_id = extractJsonField(json, "session_id");
    response.cost = extractJsonNumber(json, "total_cost_usd");
    response.duration_ms = (int)extractJsonNumber(json, "duration_ms");
    response.is_error = extractJsonBool(json, "is_error");
    
    // If marked as error in JSON, extract error details
    if (response.is_error) {
        std::string errorField = extractJsonField(json, "error");
        if (!errorField.empty()) {
            response.error_message = errorField;
        } else {
            response.error_message = "Claude returned an error";
        }
    }
    
    // If we couldn't parse result, treat as error
    if (response.result.empty() && !response.is_error) {
        response.is_error = true;
        response.error_message = "Could not parse Claude response: " + json.substr(0, 200);
    }
    
    return response;
}

std::string WibWobEngine::extractJsonField(const std::string& json, const std::string& field) const {
    std::string pattern = "\"" + field + "\":\"";
    size_t start = json.find(pattern);
    if (start == std::string::npos) {
        return "";
    }
    
    start += pattern.length();
    size_t end = start;
    
    // Find the end of the string value, handling escaped quotes
    while (end < json.length()) {
        if (json[end] == '"' && (end == start || json[end-1] != '\\')) {
            break;
        }
        end++;
    }
    
    if (end >= json.length()) {
        return "";
    }
    
    std::string result = json.substr(start, end - start);
    
    // Unescape basic escaped characters
    size_t pos = 0;
    // Handle \\n first (escaped backslash followed by n)
    while ((pos = result.find("\\\\n", pos)) != std::string::npos) {
        result.replace(pos, 3, "\\n");
        pos += 2;
    }
    // Handle \n (actual newlines)
    pos = 0;
    while ((pos = result.find("\\n", pos)) != std::string::npos) {
        result.replace(pos, 2, "\n");
        pos += 1;
    }
    // Handle \r (carriage returns)
    pos = 0;
    while ((pos = result.find("\\r", pos)) != std::string::npos) {
        result.replace(pos, 2, "\r");
        pos += 1;
    }
    // Handle \t (tabs)
    pos = 0;
    while ((pos = result.find("\\t", pos)) != std::string::npos) {
        result.replace(pos, 2, "\t");
        pos += 1;
    }
    // Handle \" (quotes)
    pos = 0;
    while ((pos = result.find("\\\"", pos)) != std::string::npos) {
        result.replace(pos, 2, "\"");
        pos += 1;
    }
    // Handle \\ (backslashes) - do this last
    pos = 0;
    while ((pos = result.find("\\\\", pos)) != std::string::npos) {
        result.replace(pos, 2, "\\");
        pos += 1;
    }
    
    return result;
}

bool WibWobEngine::extractJsonBool(const std::string& json, const std::string& field) const {
    std::string pattern = "\"" + field + "\":";
    size_t start = json.find(pattern);
    if (start == std::string::npos) {
        return false;
    }
    
    start += pattern.length();
    
    // Skip whitespace
    while (start < json.length() && (json[start] == ' ' || json[start] == '\t')) {
        start++;
    }
    
    if (start + 4 <= json.length() && json.substr(start, 4) == "true") {
        return true;
    }
    
    return false;
}

double WibWobEngine::extractJsonNumber(const std::string& json, const std::string& field) const {
    std::string pattern = "\"" + field + "\":";
    size_t start = json.find(pattern);
    if (start == std::string::npos) {
        return 0.0;
    }
    
    start += pattern.length();
    
    // Skip whitespace
    while (start < json.length() && (json[start] == ' ' || json[start] == '\t')) {
        start++;
    }
    
    size_t end = start;
    while (end < json.length() && 
           (std::isdigit(json[end]) || json[end] == '.' || json[end] == '-' || json[end] == '+' || json[end] == 'e' || json[end] == 'E')) {
        end++;
    }
    
    if (end > start) {
        std::string numStr = json.substr(start, end - start);
        try {
            return std::stod(numStr);
        } catch (...) {
            return 0.0;
        }
    }
    
    return 0.0;
}

void WibWobEngine::setError(const std::string& error) {
    lastError = error;
}

void WibWobEngine::clearError() {
    lastError.clear();
}