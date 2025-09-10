/*---------------------------------------------------------*/
/*                                                         */
/*   llm_config.cpp - LLM Configuration Management        */
/*                                                         */
/*---------------------------------------------------------*/

#include "llm_config.h"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <algorithm>

std::string ProviderConfig::getParameter(const std::string& key, const std::string& defaultValue) const {
    auto it = parameters.find(key);
    return it != parameters.end() ? it->second : defaultValue;
}

int ProviderConfig::getParameterInt(const std::string& key, int defaultValue) const {
    std::string value = getParameter(key);
    if (value.empty()) return defaultValue;
    try {
        return std::stoi(value);
    } catch (...) {
        return defaultValue;
    }
}

double ProviderConfig::getParameterDouble(const std::string& key, double defaultValue) const {
    std::string value = getParameter(key);
    if (value.empty()) return defaultValue;
    try {
        return std::stod(value);
    } catch (...) {
        return defaultValue;
    }
}

bool ProviderConfig::getParameterBool(const std::string& key, bool defaultValue) const {
    std::string value = getParameter(key);
    if (value.empty()) return defaultValue;
    std::transform(value.begin(), value.end(), value.begin(), ::tolower);
    return value == "true" || value == "1" || value == "yes";
}

LLMConfig::LLMConfig() {
    // Set up default configuration
    loadFromString(getDefaultConfigJson());
}

bool LLMConfig::loadFromFile(const std::string& configPath) {
    std::ifstream file(configPath);
    if (!file.is_open()) {
        validationErrors.push_back("Could not open config file: " + configPath);
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    
    return loadFromString(buffer.str());
}

bool LLMConfig::loadFromString(const std::string& jsonConfig) {
    validationErrors.clear();
    return parseJson(jsonConfig);
}

bool LLMConfig::saveToFile(const std::string& configPath) const {
    std::ofstream file(configPath);
    if (!file.is_open()) {
        return false;
    }
    
    file << generateJson();
    return file.good();
}

ProviderConfig LLMConfig::getProviderConfig(const std::string& provider) const {
    auto it = providers.find(provider);
    return it != providers.end() ? it->second : ProviderConfig{};
}

void LLMConfig::setProviderConfig(const std::string& provider, const ProviderConfig& config) {
    providers[provider] = config;
}

std::vector<std::string> LLMConfig::getAvailableProviders() const {
    std::vector<std::string> result;
    for (const auto& pair : providers) {
        if (pair.second.enabled) {
            result.push_back(pair.first);
        }
    }
    return result;
}

bool LLMConfig::hasProvider(const std::string& provider) const {
    auto it = providers.find(provider);
    return it != providers.end() && it->second.enabled;
}

std::string LLMConfig::resolveApiKey(const std::string& envVar) const {
    if (envVar.empty()) return "";
    const char* value = std::getenv(envVar.c_str());
    return value ? std::string(value) : std::string();
}

bool LLMConfig::isValid() const {
    validationErrors.clear();
    const_cast<LLMConfig*>(this)->validateConfiguration();
    return validationErrors.empty();
}

std::vector<std::string> LLMConfig::getValidationErrors() const {
    return validationErrors;
}

// Simple JSON parsing for basic configuration (production would use proper JSON library)
bool LLMConfig::parseJson(const std::string& json) {
    // This is a very basic JSON parser - in production we'd use a proper JSON library
    // For now, implement basic parsing for our known configuration structure
    
    // Find activeProvider
    size_t pos = json.find("\"activeProvider\"");
    if (pos != std::string::npos) {
        size_t start = json.find("\"", pos + 16);  // Skip past "activeProvider":
        if (start != std::string::npos) {
            start++; // Skip opening quote
            size_t end = json.find("\"", start);
            if (end != std::string::npos) {
                activeProvider = json.substr(start, end - start);
            }
        }
    }
    
    // Set up default providers (simplified for POC)
    ProviderConfig claudeCode;
    claudeCode.enabled = true;
    claudeCode.command = "claude";
    claudeCode.args = {"-p"};
    providers["claude_code"] = claudeCode;
    
    ProviderConfig anthropicApi;
    anthropicApi.enabled = true;
    anthropicApi.model = "claude-3-haiku-20240307";
    anthropicApi.endpoint = "https://api.anthropic.com/v1/messages";
    anthropicApi.apiKeyEnv = "ANTHROPIC_API_KEY";
    anthropicApi.parameters["maxTokens"] = "4096";
    anthropicApi.parameters["temperature"] = "0.7";
    providers["anthropic_api"] = anthropicApi;
    
    ProviderConfig openRouter;
    openRouter.enabled = true;
    openRouter.model = "anthropic/claude-3-haiku";
    openRouter.endpoint = "https://openrouter.ai/api/v1/chat/completions";
    openRouter.apiKeyEnv = "OPENROUTER_API_KEY";
    providers["openrouter"] = openRouter;
    
    validateConfiguration();
    return validationErrors.empty();
}

std::string LLMConfig::generateJson() const {
    std::ostringstream json;
    json << "{\n";
    json << "  \"activeProvider\": \"" << activeProvider << "\",\n";
    json << "  \"providers\": {\n";
    
    bool first = true;
    for (const auto& pair : providers) {
        if (!first) json << ",\n";
        first = false;
        
        const std::string& name = pair.first;
        const ProviderConfig& config = pair.second;
        
        json << "    \"" << name << "\": {\n";
        json << "      \"enabled\": " << (config.enabled ? "true" : "false") << ",\n";
        
        if (!config.model.empty()) {
            json << "      \"model\": \"" << config.model << "\",\n";
        }
        if (!config.endpoint.empty()) {
            json << "      \"endpoint\": \"" << config.endpoint << "\",\n";
        }
        if (!config.apiKeyEnv.empty()) {
            json << "      \"apiKeyEnv\": \"" << config.apiKeyEnv << "\",\n";
        }
        if (!config.command.empty()) {
            json << "      \"command\": \"" << config.command << "\",\n";
            if (!config.args.empty()) {
                json << "      \"args\": [";
                for (size_t i = 0; i < config.args.size(); ++i) {
                    if (i > 0) json << ", ";
                    json << "\"" << config.args[i] << "\"";
                }
                json << "],\n";
            }
        }
        
        // Add parameters
        for (const auto& param : config.parameters) {
            json << "      \"" << param.first << "\": \"" << param.second << "\",\n";
        }
        
        // Remove trailing comma
        std::string line = json.str();
        if (line.back() == '\n' && line[line.size()-2] == ',') {
            json.seekp(-2, std::ios_base::cur);
            json << "\n";
        }
        
        json << "    }";
    }
    
    json << "\n  }\n";
    json << "}\n";
    
    return json.str();
}

void LLMConfig::validateConfiguration() {
    if (activeProvider.empty()) {
        validationErrors.push_back("No active provider specified");
        return;
    }
    
    if (!hasProvider(activeProvider)) {
        validationErrors.push_back("Active provider '" + activeProvider + "' is not available or disabled");
    }
    
    // Validate provider configurations
    for (const auto& pair : providers) {
        const std::string& name = pair.first;
        const ProviderConfig& config = pair.second;
        
        if (!config.enabled) continue;
        
        // Validate API-based providers
        if (name == "anthropic_api" || name == "openrouter") {
            if (config.endpoint.empty()) {
                validationErrors.push_back("Provider '" + name + "' missing endpoint");
            }
            if (!config.apiKeyEnv.empty()) {
                std::string apiKey = resolveApiKey(config.apiKeyEnv);
                if (apiKey.empty()) {
                    validationErrors.push_back("Provider '" + name + "' API key not found in environment variable '" + config.apiKeyEnv + "'");
                }
            }
        }
        
        // Validate command-based providers
        if (name == "claude_code") {
            if (config.command.empty()) {
                validationErrors.push_back("Provider '" + name + "' missing command");
            }
        }
    }
}

std::string LLMConfig::getDefaultConfigJson() {
    return R"({
  "activeProvider": "claude_code",
  "providers": {
    "claude_code": {
      "enabled": true,
      "command": "claude",
      "args": ["-p"]
    },
    "anthropic_api": {
      "enabled": true,
      "model": "claude-3-haiku-20240307",
      "endpoint": "https://api.anthropic.com/v1/messages",
      "apiKeyEnv": "ANTHROPIC_API_KEY",
      "maxTokens": "4096",
      "temperature": "0.7"
    },
    "openrouter": {
      "enabled": true,
      "model": "anthropic/claude-3-haiku",
      "endpoint": "https://openrouter.ai/api/v1/chat/completions",
      "apiKeyEnv": "OPENROUTER_API_KEY"
    }
  }
})";
}