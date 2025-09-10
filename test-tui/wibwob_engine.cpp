/*---------------------------------------------------------*/
/*                                                         */
/*   wibwob_engine.cpp - LLM Provider Integration Engine  */
/*                                                         */
/*---------------------------------------------------------*/

#include "wibwob_engine.h"
#include "llm/base/llm_provider_factory.h"

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <algorithm>
#include <chrono>

WibWobEngine::WibWobEngine() {
    // Default system prompt for wib&wob
    systemPrompt = "You are wib&wob, a helpful AI assistant integrated into a Turbo Vision TUI application.";
    
    // Defer configuration loading until first use
}

WibWobEngine::~WibWobEngine() {
    // Cancel any pending request
    cancel();
}

bool WibWobEngine::sendQuery(const std::string& query, ResponseCallback callback) {
    // Load configuration on first use
    if (!currentProvider) {
        loadConfiguration();
    }
    
    if (!currentProvider || query.empty()) {
        if (callback) {
            LLMResponse response;
            response.is_error = true;
            response.error_message = "No provider available or empty query";
            callback(response);
        }
        return false;
    }
    
    // Build the request
    LLMRequest request;
    request.message = query;
    request.system_prompt = systemPrompt;
    
    // Send to current provider
    return currentProvider->sendQuery(request, callback);
}

void WibWobEngine::poll() {
    if (currentProvider) {
        currentProvider->poll();
    }
}

void WibWobEngine::cancel() {
    if (currentProvider) {
        currentProvider->cancel();
    }
}

bool WibWobEngine::isClaudeAvailable() const {
    // Load configuration on first check
    if (!currentProvider) {
        const_cast<WibWobEngine*>(this)->loadConfiguration();
    }
    return currentProvider && currentProvider->isAvailable();
}

void WibWobEngine::setSystemPrompt(const std::string& prompt) {
    systemPrompt = prompt;
}

void WibWobEngine::setClaudePath(const std::string& path) {
    // Legacy compatibility - update claude_code provider configuration if active
    claudePath = path;
    
    if (currentProvider && currentProvider->getProviderName() == "claude_code") {
        // Would need to reconfigure the provider - for now just store the path
        // In production, this would update the provider's configuration
    }
}

bool WibWobEngine::switchProvider(const std::string& providerName) {
    return initializeProvider(providerName);
}

std::string WibWobEngine::getCurrentProvider() const {
    // Load configuration on first check
    if (!currentProvider) {
        const_cast<WibWobEngine*>(this)->loadConfiguration();
    }
    if (currentProvider) {
        return currentProvider->getProviderName();
    }
    return "none";
}

std::string WibWobEngine::getCurrentModel() const {
    if (!config) return "unknown";
    
    std::string provider = getCurrentProvider();
    if (provider == "none") return "unknown";
    
    ProviderConfig providerConfig = config->getProviderConfig(provider);
    
    // For claude_code, we don't know the exact model, just return generic name
    if (provider == "claude_code") {
        return "claude-code-cli";
    }
    
    return providerConfig.model.empty() ? "unknown" : providerConfig.model;
}

std::vector<std::string> WibWobEngine::getAvailableProviders() const {
    return LLMProviderFactory::getInstance().getAvailableProviders();
}

bool WibWobEngine::isBusy() const {
    return currentProvider && currentProvider->isBusy();
}

std::string WibWobEngine::getLastError() const {
    if (currentProvider) {
        return currentProvider->getLastError();
    }
    return "No provider initialized";
}

void WibWobEngine::loadConfiguration() {
    config = std::make_unique<LLMConfig>();
    
    // Try to load from config file
    bool loadResult = config->loadFromFile("llm/config/llm_config.json");
    if (loadResult) {
        // Initialize the active provider
        std::string activeProvider = config->getActiveProvider();
        if (!activeProvider.empty()) {
            initializeProvider(activeProvider);
        }
    } else {
        // Config file missing or invalid - create default but DON'T overwrite existing file
        fprintf(stderr, "ERROR: Failed to load llm/config/llm_config.json\n");
        
        // Check validation errors
        auto errors = config->getValidationErrors();
        for (const auto& error : errors) {
            fprintf(stderr, "Config error: %s\n", error.c_str());
        }
        std::string defaultJson = LLMConfig::getDefaultConfigJson();
        config->loadFromString(defaultJson);
        
        // Only save if file doesn't exist at all
        FILE* check = fopen("llm/config/llm_config.json", "r");
        if (!check) {
            config->saveToFile("llm/config/llm_config.json");
        } else {
            fclose(check);
        }
        
        // Try claude_code as default
        initializeProvider("claude_code");
    }
}

bool WibWobEngine::initializeProvider(const std::string& providerName) {
    // Create new provider instance
    auto provider = LLMProviderFactory::getInstance().createProvider(providerName);
    if (!provider) {
        return false;
    }
    
    // Get configuration for this provider
    if (config) {
        ProviderConfig providerConfig = config->getProviderConfig(providerName);
        if (providerConfig.enabled) {
            // Convert ProviderConfig to JSON string for provider configuration
            std::string configJson = generateProviderConfigJson(providerConfig);
            if (!provider->configure(configJson)) {
                return false;
            }
        }
    }
    
    // Check if provider is available
    if (!provider->isAvailable()) {
        return false;
    }
    
    // Switch to new provider
    currentProvider = std::move(provider);
    
    // Update active provider in config (but don't save - respect user's file)
    if (config) {
        config->setActiveProvider(providerName);
        // Don't auto-save to avoid overwriting user's manual edits
    }
    
    return true;
}

std::string WibWobEngine::generateProviderConfigJson(const ProviderConfig& config) const {
    std::ostringstream json;
    json << "{";
    
    if (!config.model.empty()) {
        json << "\"model\":\"" << config.model << "\",";
    }
    if (!config.endpoint.empty()) {
        json << "\"endpoint\":\"" << config.endpoint << "\",";
    }
    if (!config.apiKeyEnv.empty()) {
        json << "\"apiKeyEnv\":\"" << config.apiKeyEnv << "\",";
    }
    if (!config.command.empty()) {
        json << "\"command\":\"" << config.command << "\",";
    }
    
    // Add generic parameters
    for (const auto& param : config.parameters) {
        json << "\"" << param.first << "\":\"" << param.second << "\",";
    }
    
    std::string result = json.str();
    if (result.back() == ',') {
        result.pop_back(); // Remove trailing comma
    }
    result += "}";
    
    return result;
}