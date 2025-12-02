# LLM Config Debug: Systematic Fix Plan

**tl;dr:** Multiple hardcoded anthropic_api overrides + config file loading failure + provider registration issues preventing claude_code_sdk from working. Need systematic search-and-destroy of all forcing mechanisms plus config loading fix.

## Problem Analysis

The debug output reveals multiple systemic issues:
1. **Multiple forcing locations** - "DEBUG: Forced activeProvider to: anthropic_api" appears repeatedly
2. **Config loading failure** - "Config file load result: FAILED" 
3. **Provider availability failure** - "Active provider 'claude_code_sdk' is not available or disabled"
4. **Cascading defaults** - Even after fixing default config, still falls back to anthropic_api

## Step-by-Step Fix Plan

### Phase 1: Complete Search & Inventory
Find ALL locations where anthropic_api is hardcoded or forced:

```bash
# Search for all anthropic_api references
rg "anthropic_api" --type cpp --type h test-tui/

# Search for all "Forced activeProvider" debug messages
rg "Forced activeProvider" --type cpp --type h test-tui/

# Search for provider forcing logic patterns
rg "activeProvider.*=" --type cpp --type h test-tui/

# Search for config loading and fallback logic
rg "default.*config\|fallback.*config\|config.*default" --type cpp --type h test-tui/
```

### Phase 2: Fix Config File Loading
Priority fix - config loading is completely failing:

1. **Identify config loading code**:
   ```bash
   rg "Config file load result\|loading.*config\|load.*config" --type cpp test-tui/
   ```

2. **Check file paths and permissions**:
   - Verify config file exists and is readable
   - Check absolute vs relative path issues
   - Validate JSON syntax

3. **Add detailed logging** to config loading:
   - File existence checks
   - Permission checks
   - JSON parsing error details
   - Path resolution logging

### Phase 3: Provider Registration Audit
Fix "provider not available" error:

1. **Find provider registration code**:
   ```bash
   rg "claude_code_sdk.*register\|register.*claude_code_sdk" --type cpp test-tui/
   ```

2. **Verify provider initialization**:
   - Check claude_code_sdk_provider.cpp is being compiled
   - Verify provider constructor is being called
   - Check provider registration in provider factory/registry

3. **Debug provider availability check**:
   - Add logging to provider availability logic
   - Verify provider name matching (case sensitivity, typos)
   - Check provider enabled/disabled flags

### Phase 4: Systematic Forcing Elimination
Remove ALL hardcoded overrides:

1. **For each forcing location found**:
   - Document why it was added (git blame/history)
   - Determine if it's debug code that can be removed
   - Replace with proper config-driven logic
   - Add comments explaining the change

2. **Common forcing patterns to eliminate**:
   - `activeProvider = "anthropic_api"` assignments
   - Conditional logic that defaults to anthropic_api
   - Debug/testing code that overrides user config
   - Environment variable overrides

### Phase 5: Config Precedence Chain
Establish clear, predictable config loading order:

1. **Define precedence hierarchy**:
   ```
   1. Command line arguments (--provider=X)
   2. Environment variables (LLM_PROVIDER=X)
   3. JSON config file (llm_config.json)
   4. Compiled defaults (claude_code_sdk)
   ```

2. **Implement with logging**:
   - Log each step of precedence checking
   - Show what value wins and why
   - Make debugging transparent

### Phase 6: Validation & Testing
Ensure fix is complete and robust:

1. **Test config loading scenarios**:
   - Valid JSON config file
   - Invalid/malformed JSON
   - Missing config file
   - Permissions issues

2. **Test provider scenarios**:
   - claude_code_sdk provider available
   - Provider not available
   - Multiple providers registered
   - No providers available

3. **Test precedence scenarios**:
   - Each level of precedence hierarchy
   - Conflicting values at different levels
   - Missing values at higher precedence levels

## Implementation Checklist

### Phase 1 Tasks
- [ ] Search for all anthropic_api hardcoded references
- [ ] Search for all "Forced activeProvider" debug locations
- [ ] Search for provider assignment patterns
- [ ] Document all forcing locations found

### Phase 2 Tasks
- [ ] Find and examine config loading code
- [ ] Check config file path and permissions
- [ ] Add detailed config loading logging
- [ ] Test config file parsing

### Phase 3 Tasks
- [ ] Verify claude_code_sdk provider compilation
- [ ] Check provider registration code
- [ ] Debug provider availability logic
- [ ] Verify provider name matching

### Phase 4 Tasks
- [ ] Remove/fix each forcing location
- [ ] Replace with proper config logic
- [ ] Document changes made
- [ ] Test each fix individually

### Phase 5 Tasks
- [ ] Implement config precedence chain
- [ ] Add precedence logging
- [ ] Document precedence rules
- [ ] Test precedence scenarios

### Phase 6 Tasks
- [ ] Test all config loading scenarios
- [ ] Test all provider scenarios  
- [ ] Test all precedence scenarios
- [ ] Verify no more forcing debug messages

## Expected Outcome

After completion:
- No "DEBUG: Forced activeProvider to: anthropic_api" messages
- "Config file load result: SUCCESS" message
- claude_code_sdk provider properly available
- Clear, logged precedence chain working correctly
- User config respected instead of hardcoded overrides

## Files to Modify

Based on search results, likely candidates:
- Config loading code (find with searches)
- Provider registration code
- Default config initialization
- Any debug/test code with hardcoded values
- Provider availability checking code

## Success Criteria

1. Config file loads successfully
2. No forcing debug messages appear
3. claude_code_sdk provider is available and working
4. User config values are respected
5. Clear precedence chain logging visible
6. System works with all test scenarios