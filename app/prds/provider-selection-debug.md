# Provider Selection Debug Analysis & Fix Plan

**tl;dr:** Config loads `anthropic_api` but JSON parser ignores it, hardcoded defaults override user config, fallback to `claude_code` makes UI show wrong provider. Need proper JSON parsing, config validation, and initialization flow.

## Root Cause Analysis

### 1. **Critical Bug: JSON Parser Ignores Provider Configurations**
**Location:** `llm_config.cpp:119-161` (parseJson method)
**Issue:** The parser reads `activeProvider` from JSON but completely ignores the `providers` section and hardcodes default configs.

```cpp
// Current broken behavior:
bool LLMConfig::parseJson(const std::string& json) {
    // Reads activeProvider correctly ✓
    activeProvider = json.substr(start, end - start);
    
    // BUT THEN ignores user config and hardcodes defaults ✗
    ProviderConfig anthropicApi;
    anthropicApi.model = "claude-3-haiku-20240307";  // Wrong model!
    // User config says "claude-3-5-haiku-latest"
}
```

### 2. **Fallback Logic Issue**
**Location:** `wibwob_engine.cpp:165-167`
**Issue:** When config parsing fails (it always does), system falls back to `claude_code` instead of respecting user's `anthropic_api` choice.

### 3. **Deferred Loading Race Condition**
**Location:** `wibwob_engine.cpp:92-96` (getCurrentProvider)
**Issue:** UI calls `getCurrentProvider()` before `sendQuery()`, triggering const_cast config loading that may fail and return wrong provider.

### 4. **Config Path Assumptions**
**Location:** `wibwob_engine.cpp:138`
**Issue:** Hardcoded relative path `"llm/config/llm_config.json"` may not resolve correctly in all execution contexts.

### 5. **Validation vs Parsing Confusion**
**Location:** `wibwob_engine.cpp:147-153`
**Issue:** Config load failure prints validation errors but then loads default config anyway, masking the real parsing problem.

## User Experience Problems

1. **Status shows wrong provider**: "Thinking with claude-code-cli (claude_code)" when user set `anthropic_api`
2. **Loading gets stuck**: API calls fail because wrong provider is initialized
3. **Silent failures**: Config errors are logged to stderr, not visible to user
4. **Model mismatch**: User config has "claude-3-5-haiku-latest" but parser uses "claude-3-haiku-20240307"

## Fix Implementation Plan

### Phase 1: Fix JSON Parsing (Critical)

#### 1.1 Implement Proper JSON Parser
```cpp
// Replace hardcoded parseJson with actual JSON parsing
bool LLMConfig::parseJson(const std::string& json) {
    // Parse entire JSON structure including providers section
    // Extract model, endpoint, apiKeyEnv for each provider
    // Respect user configuration instead of hardcoding
}
```

#### 1.2 Add JSON Validation
```cpp
// Validate that parsed config matches file structure
// Check for required fields per provider type
// Report specific parsing errors
```

### Phase 2: Fix Initialization Flow

#### 2.1 Eager Config Loading
```cpp
// Load config in WibWobEngine constructor, not deferred
// Fail fast with clear error messages
// No const_cast hacks
```

#### 2.2 Provider Fallback Logic
```cpp
// If activeProvider fails to init, try other enabled providers
// Only fall back to claude_code if no API providers work
// Update UI status with fallback information
```

### Phase 3: Improve Error Handling

#### 3.1 User-Visible Error Reporting
```cpp
// Replace stderr with user-visible error messages
// Show config problems in UI status
// Guide user to fix configuration issues
```

#### 3.2 Config Path Resolution
```cpp
// Use absolute paths or proper working directory resolution
// Check file existence before attempting to parse
// Provide clear file not found errors
```

## Testing Strategy

### Test Cases
1. **Valid config with anthropic_api**: Should show correct provider/model
2. **Missing config file**: Should create default and show fallback message
3. **Malformed JSON**: Should show parsing error, not silent failure
4. **Missing API key**: Should show environment variable error
5. **Provider unavailable**: Should try fallbacks and report final choice

### Verification Points
- [ ] UI status shows correct provider name from config file
- [ ] Model name matches user's config exactly
- [ ] Config parsing errors are visible to user
- [ ] API calls succeed when provider is properly configured
- [ ] Graceful fallback when provider initialization fails

## Implementation Files to Modify

1. **`llm_config.cpp`**: Rewrite parseJson method to actually parse JSON
2. **`wibwob_engine.cpp`**: Fix initialization flow and error handling
3. **`wibwob_view.cpp`**: Add config error display in UI
4. **`llm_config.h`**: Add proper error reporting methods

## Risk Assessment

**High Risk**: JSON parsing is completely broken - all users affected
**Medium Risk**: Fallback logic makes it seem like it works (silent failure)
**Low Risk**: Fix is straightforward once root cause is identified

## Success Metrics

- User sets `"activeProvider": "anthropic_api"` → UI shows "Thinking with claude-3-5-haiku-latest (anthropic_api)"
- Config errors are visible in UI, not just stderr
- No more "Loading..." hangs when provider is properly configured
- Fallback behavior is transparent to user