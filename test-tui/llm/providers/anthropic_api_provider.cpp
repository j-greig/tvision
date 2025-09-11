/*---------------------------------------------------------*/
/*                                                         */
/*   anthropic_api_provider.cpp - Anthropic API Provider  */
/*                                                         */
/*---------------------------------------------------------*/

#include "anthropic_api_provider.h"
#include "../base/llm_provider_factory.h"

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <chrono>
#include <fcntl.h>
#include <unistd.h>

// Register this provider with the factory
REGISTER_LLM_PROVIDER("anthropic_api", AnthropicAPIProvider);

AnthropicAPIProvider::AnthropicAPIProvider() {
    // Default configuration will be overridden by configure()
}

AnthropicAPIProvider::~AnthropicAPIProvider() {
    cancel(); // Cleanup any active request
}

bool AnthropicAPIProvider::sendQuery(const LLMRequest& request, ResponseCallback callback) {
    if (busy || request.message.empty()) {
        return false;
    }
    
    clearError();
    
    // Start async API request
    return startAsyncAPIRequest(request, callback);
}

bool AnthropicAPIProvider::isAvailable() const {
    // Check if we have an API key
    return !apiKey.empty();
}

bool AnthropicAPIProvider::isBusy() const {
    return busy;
}

void AnthropicAPIProvider::cancel() {
    if (busy && activePipe) {
        pclose(activePipe);
        activePipe = nullptr;
        busy = false;
        
        if (pendingCallback) {
            LLMResponse response;
            response.provider_name = getProviderName();
            response.is_error = true;
            response.error_message = "Request cancelled by user";
            pendingCallback(response);
            pendingCallback = nullptr;
        }
        
        outputBuffer.clear();
    }
}

void AnthropicAPIProvider::poll() {
    pollAsyncRequest();
}

std::vector<std::string> AnthropicAPIProvider::getSupportedModels() const {
    return {
        "claude-3-5-haiku-latest",
        "claude-sonnet-4-20250514",
        "claude-opus-4-20250514", 
        "claude-opus-4-1-20250805",
        "claude-3-5-sonnet-latest",
        "claude-3-haiku-20240307",
        "claude-3-sonnet-20240229",
        "claude-3-opus-20240229"
    };
}

bool AnthropicAPIProvider::configure(const std::string& config) {
    // Parse configuration JSON
    // Look for endpoint
    size_t endpointPos = config.find("\"endpoint\"");
    if (endpointPos != std::string::npos) {
        size_t start = config.find("\"", endpointPos + 10);
        if (start != std::string::npos) {
            start++; // Skip opening quote
            size_t end = config.find("\"", start);
            if (end != std::string::npos) {
                endpoint = config.substr(start, end - start);
            }
        }
    }
    
    // Look for model
    size_t modelPos = config.find("\"model\"");
    if (modelPos != std::string::npos) {
        size_t start = config.find("\"", modelPos + 7);
        if (start != std::string::npos) {
            start++; // Skip opening quote
            size_t end = config.find("\"", start);
            if (end != std::string::npos) {
                model = config.substr(start, end - start);
            }
        }
    }
    
    // Look for API key environment variable
    size_t apiKeyEnvPos = config.find("\"apiKeyEnv\"");
    if (apiKeyEnvPos != std::string::npos) {
        size_t start = config.find("\"", apiKeyEnvPos + 11);
        if (start != std::string::npos) {
            start++; // Skip opening quote
            size_t end = config.find("\"", start);
            if (end != std::string::npos) {
                std::string envVar = config.substr(start, end - start);
                const char* envValue = std::getenv(envVar.c_str());
                if (envValue) {
                    apiKey = std::string(envValue);
                    // Trim whitespace and newlines from API key
                    apiKey.erase(apiKey.find_last_not_of(" \t\r\n") + 1);
                    apiKey.erase(0, apiKey.find_first_not_of(" \t\r\n"));
                }
            }
        }
    }
    
    // Look for maxTokens
    size_t maxTokensPos = config.find("\"maxTokens\"");
    if (maxTokensPos != std::string::npos) {
        size_t start = config.find("\"", maxTokensPos + 11);
        if (start != std::string::npos) {
            start++; // Skip opening quote
            size_t end = config.find("\"", start);
            if (end != std::string::npos) {
                std::string value = config.substr(start, end - start);
                try {
                    maxTokens = std::stoi(value);
                } catch (...) {
                    maxTokens = 4096; // Default
                }
            }
        }
    }
    
    // Look for temperature
    size_t tempPos = config.find("\"temperature\"");
    if (tempPos != std::string::npos) {
        size_t start = config.find("\"", tempPos + 13);
        if (start != std::string::npos) {
            start++; // Skip opening quote
            size_t end = config.find("\"", start);
            if (end != std::string::npos) {
                std::string value = config.substr(start, end - start);
                try {
                    temperature = std::stod(value);
                } catch (...) {
                    temperature = 0.7; // Default
                }
            }
        }
    }
    
    return !apiKey.empty();
}

void AnthropicAPIProvider::resetSession() {
    conversationHistory.clear();
}

bool AnthropicAPIProvider::startAsyncAPIRequest(const LLMRequest& request, ResponseCallback callback) {
    if (!isAvailable()) {
        LLMResponse response;
        response.provider_name = getProviderName();
        response.is_error = true;
        response.error_message = "Anthropic API key not configured";
        setError(response.error_message);
        if (callback) callback(response);
        return false;
    }
    
    // Debug: check API key
    // fprintf(stderr, "API key configured: %s\n", apiKey.empty() ? "NO" : "YES");
    
    // Build the request JSON
    std::string requestJson = buildRequestJson(request);
    
    // Build headers (without \\r\\n - curl will add proper line endings)
    std::string headers = 
        "Content-Type: application/json\n"
        "Authorization: Bearer " + apiKey + "\n"
        "anthropic-version: 2023-06-01\n";
    
    // Build the curl command for async execution with timeout and verbose error output
    std::string command = "curl --max-time 30 --connect-timeout 10 -X POST '" + endpoint + "' ";
    
    // Add headers
    std::istringstream headerStream(headers);
    std::string line;
    while (std::getline(headerStream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back(); // Remove \\r
        }
        if (!line.empty()) {
            command += "-H \"" + line + "\" ";
        }
    }
    
    // Add payload
    command += "--data '";
    // Escape single quotes in payload
    std::string escapedPayload;
    for (char c : requestJson) {
        if (c == '\'') {
            escapedPayload += "'\\\"'\\\"'"; // End quote, escaped quote, start quote
        } else {
            escapedPayload += c;
        }
    }
    command += escapedPayload + "'";
    
    // Debug: write command to stderr for troubleshooting
    // fprintf(stderr, "Executing: %s\n", command.c_str());
    
    // Start async execution
    activePipe = popen(command.c_str(), "r");
    if (!activePipe) {
        LLMResponse response;
        response.provider_name = getProviderName();
        response.is_error = true;
        response.error_message = "Failed to execute curl command";
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
    pendingRequest = request;
    outputBuffer.clear();
    
    return true;
}

void AnthropicAPIProvider::pollAsyncRequest() {
    if (!busy || !activePipe) {
        return;
    }
    
    // Read available data from pipe (non-blocking)
    char buffer[4096];
    clearerr(activePipe); // Clear any previous error flags
    size_t bytesRead = fread(buffer, 1, sizeof(buffer) - 1, activePipe);
    
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        outputBuffer += buffer;
    }
    
    // Check if process is done (only check EOF, not errors for non-blocking)
    if (feof(activePipe)) {
        int exitCode = pclose(activePipe);
        activePipe = nullptr;
        busy = false;
        
        // Debug output
        // fprintf(stderr, "curl process finished with exit code: %d\n", exitCode);
        
        // Parse response and call callback
        LLMResponse response;
        response.provider_name = getProviderName();
        response.model_used = model;
        
        if (exitCode == 0) {
            response = parseAPIResponse(outputBuffer);
            response.provider_name = getProviderName();
            response.model_used = model;
            
            // Add to conversation history if successful
            if (!response.is_error) {
                conversationHistory.push_back(std::make_pair("user", pendingRequest.message));
                conversationHistory.push_back(std::make_pair("assistant", response.result));
            }
        } else {
            response.is_error = true;
            response.error_message = "Curl command failed with exit code " + std::to_string(exitCode);
            if (!outputBuffer.empty()) {
                response.error_message += ": " + outputBuffer;
            }
            setError(response.error_message);
        }
        
        // Call the callback
        if (pendingCallback) {
            pendingCallback(response);
            pendingCallback = nullptr;
        }
        
        outputBuffer.clear();
    }
}

LLMResponse AnthropicAPIProvider::makeAPIRequest(const LLMRequest& request) {
    LLMResponse response;
    response.provider_name = getProviderName();
    response.model_used = model;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    if (!isAvailable()) {
        response.is_error = true;
        response.error_message = "Anthropic API key not configured";
        setError(response.error_message);
        return response;
    }
    
    // Build the request JSON
    std::string requestJson = buildRequestJson(request);
    
    // Build headers
    std::string headers = 
        "Content-Type: application/json\r\n"
        "Authorization: Bearer " + apiKey + "\r\n"
        "anthropic-version: 2023-06-01\r\n";
    
    // Make the HTTP request
    std::string apiResponse = performHttpRequest(endpoint, headers, requestJson);
    
    auto endTime = std::chrono::high_resolution_clock::now();
    response.duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    
    if (apiResponse.empty()) {
        response.is_error = true;
        response.error_message = "Empty response from Anthropic API";
        setError(response.error_message);
        return response;
    }
    
    // Parse the response
    response = parseAPIResponse(apiResponse);
    response.provider_name = getProviderName();
    response.model_used = model;
    response.duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    
    // Add to conversation history if successful
    if (!response.is_error) {
        conversationHistory.push_back(std::make_pair("user", request.message));
        conversationHistory.push_back(std::make_pair("assistant", response.result));
    }
    
    return response;
}

std::string AnthropicAPIProvider::buildRequestJson(const LLMRequest& request) const {
    std::ostringstream json;
    
    json << "{\n";
    json << "  \"model\": \"" << model << "\",\n";
    json << "  \"max_tokens\": " << maxTokens << ",\n";
    json << "  \"temperature\": " << temperature << ",\n";
    
    // System prompt
    if (!request.system_prompt.empty()) {
        json << "  \"system\": \"";
        // Escape JSON string
        for (char c : request.system_prompt) {
            if (c == '"') json << "\\\"";
            else if (c == '\\') json << "\\\\";
            else if (c == '\n') json << "\\n";
            else if (c == '\r') json << "\\r";
            else if (c == '\t') json << "\\t";
            else json << c;
        }
        json << "\",\n";
    }
    
    json << "  \"messages\": [\n";
    
    // Add conversation history
    bool first = true;
    for (const auto& msg : conversationHistory) {
        if (!first) json << ",\n";
        first = false;
        
        json << "    {\n";
        json << "      \"role\": \"" << msg.first << "\",\n";
        json << "      \"content\": \"";
        // Escape JSON string
        for (char c : msg.second) {
            if (c == '"') json << "\\\"";
            else if (c == '\\') json << "\\\\";
            else if (c == '\n') json << "\\n";
            else if (c == '\r') json << "\\r";
            else if (c == '\t') json << "\\t";
            else json << c;
        }
        json << "\"\n";
        json << "    }";
    }
    
    // Add current message
    if (!first) json << ",\n";
    json << "    {\n";
    json << "      \"role\": \"user\",\n";
    json << "      \"content\": \"";
    // Escape JSON string
    for (char c : request.message) {
        if (c == '"') json << "\\\"";
        else if (c == '\\') json << "\\\\";
        else if (c == '\n') json << "\\n";
        else if (c == '\r') json << "\\r";
        else if (c == '\t') json << "\\t";
        else json << c;
    }
    json << "\"\n";
    json << "    }\n";
    
    json << "  ]\n";
    json << "}\n";
    
    return json.str();
}

LLMResponse AnthropicAPIProvider::parseAPIResponse(const std::string& response) const {
    LLMResponse result;
    
    // Simple JSON parsing - look for error first
    if (response.find("\"error\"") != std::string::npos) {
        result.is_error = true;
        
        // Extract error message
        size_t msgStart = response.find("\"message\"");
        if (msgStart != std::string::npos) {
            size_t start = response.find("\"", msgStart + 9);
            if (start != std::string::npos) {
                start++; // Skip opening quote
                size_t end = response.find("\"", start);
                if (end != std::string::npos) {
                    result.error_message = response.substr(start, end - start);
                }
            }
        }
        
        if (result.error_message.empty()) {
            result.error_message = "API request failed";
        }
        
        return result;
    }
    
    // Look for content in the response
    size_t contentPos = response.find("\"content\"");
    if (contentPos != std::string::npos) {
        // Find the text content
        size_t textPos = response.find("\"text\"", contentPos);
        if (textPos != std::string::npos) {
            size_t start = response.find("\"", textPos + 6);
            if (start != std::string::npos) {
                start++; // Skip opening quote
                size_t end = start;
                
                // Find the end of the string value, handling escaped quotes
                while (end < response.length()) {
                    if (response[end] == '"' && (end == start || response[end-1] != '\\')) {
                        break;
                    }
                    end++;
                }
                
                if (end < response.length()) {
                    result.result = response.substr(start, end - start);
                    
                    // Unescape basic JSON escape sequences
                    size_t pos = 0;
                    while ((pos = result.result.find("\\n", pos)) != std::string::npos) {
                        result.result.replace(pos, 2, "\n");
                        pos += 1;
                    }
                    pos = 0;
                    while ((pos = result.result.find("\\r", pos)) != std::string::npos) {
                        result.result.replace(pos, 2, "\r");
                        pos += 1;
                    }
                    pos = 0;
                    while ((pos = result.result.find("\\t", pos)) != std::string::npos) {
                        result.result.replace(pos, 2, "\t");
                        pos += 1;
                    }
                    pos = 0;
                    while ((pos = result.result.find("\\\"", pos)) != std::string::npos) {
                        result.result.replace(pos, 2, "\"");
                        pos += 1;
                    }
                    pos = 0;
                    while ((pos = result.result.find("\\\\", pos)) != std::string::npos) {
                        result.result.replace(pos, 2, "\\");
                        pos += 1;
                    }
                }
            }
        }
    }
    
    if (result.result.empty()) {
        result.is_error = true;
        result.error_message = "Could not extract content from API response";
    }
    
    return result;
}

std::string AnthropicAPIProvider::performHttpRequest(const std::string& url, const std::string& headers, const std::string& payload) const {
    // Use curl to make the HTTP request
    std::string command = "curl -s -X POST '" + url + "' ";
    
    // Add headers
    std::istringstream headerStream(headers);
    std::string line;
    while (std::getline(headerStream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back(); // Remove \r
        }
        if (!line.empty()) {
            command += "-H '" + line + "' ";
        }
    }
    
    // Add payload
    command += "--data '";
    // Escape single quotes in payload
    std::string escapedPayload;
    for (char c : payload) {
        if (c == '\'') {
            escapedPayload += "'\"'\"'"; // End quote, escaped quote, start quote
        } else {
            escapedPayload += c;
        }
    }
    command += escapedPayload + "'";
    
    // Execute the command
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return "";
    }
    
    std::string result;
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    
    pclose(pipe);
    
    return result;
}

void AnthropicAPIProvider::setError(const std::string& error) {
    lastError = error;
}

void AnthropicAPIProvider::clearError() {
    lastError.clear();
}