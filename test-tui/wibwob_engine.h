/*---------------------------------------------------------*/
/*                                                         */
/*   wibwob_engine.h - Claude Code Integration Engine     */
/*                                                         */
/*---------------------------------------------------------*/

#ifndef WIBWOB_ENGINE_H
#define WIBWOB_ENGINE_H

#include <string>
#include <functional>

struct ClaudeResponse {
    std::string result;
    std::string session_id;
    double cost = 0.0;
    int duration_ms = 0;
    bool is_error = false;
    std::string error_message;
};

class WibWobEngine {
public:
    WibWobEngine();
    ~WibWobEngine();
    
    // Callback for response handling
    using ResponseCallback = std::function<void(const ClaudeResponse&)>;
    
    // Send a query to Claude Code (non-blocking)
    bool sendQuery(const std::string& query, ResponseCallback callback);
    
    // Poll for completion of async request
    void poll();
    
    // Cancel current request
    void cancel();
    
    // Check if Claude Code is available
    bool isClaudeAvailable() const;
    
    // Configuration
    void setSystemPrompt(const std::string& prompt);
    void setClaudePath(const std::string& path);
    
    // Status
    bool isBusy() const { return busy; }
    std::string getLastError() const { return lastError; }

private:
    bool busy = false;
    std::string claudePath = "claude";
    std::string systemPrompt;
    std::string lastError;
    std::string currentSessionId;
    
    // Async execution state
    FILE* activePipe = nullptr;
    std::string outputBuffer;
    ResponseCallback pendingCallback;
    
    // Claude Code execution
    ClaudeResponse executeClaudeCommand(const std::string& query);
    bool startAsyncCommand(const std::string& query, ResponseCallback callback);
    std::string buildClaudeCommand(const std::string& query) const;
    
    // JSON parsing
    ClaudeResponse parseClaudeResponse(const std::string& json) const;
    std::string extractJsonField(const std::string& json, const std::string& field) const;
    bool extractJsonBool(const std::string& json, const std::string& field) const;
    double extractJsonNumber(const std::string& json, const std::string& field) const;
    
    // Error handling
    void setError(const std::string& error);
    void clearError();
};

#endif // WIBWOB_ENGINE_H