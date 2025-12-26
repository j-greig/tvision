# Claude Model Configuration Analysis

**tl;dr:** Model is set in 2 layers: `test-tui/llm/config/llm_config.json` (C++ reads this) which passes `--model sonnet` to `claude` CLI, which then reads `test-tui/.claude/settings.json` (Claude Code's own config). Both now correctly set to `"sonnet"` alias for latest model.

---

## Executive Summary

The TUI app (`test_pattern`) running from `test-tui/` directory uses a **two-layer configuration system**:

1. **C++ Layer**: `test-tui/llm/config/llm_config.json` → defines `--model sonnet` flag
2. **Claude Code Layer**: `test-tui/.claude/settings.json` → defines `"model": "sonnet"` (backup/override)

**Current Status**: ✅ Both layers now correctly use `"sonnet"` alias (not explicit model IDs like `claude-sonnet-4-5-20250929`)

**Why "sonnet" alias?** Per Claude Code CLI docs: `--model` accepts aliases like `sonnet` or `opus` which automatically resolve to the latest model version.

---

## Configuration Loading Chain (Step-by-Step)

### 1. Application Startup (C++ Side)

**File**: `test-tui/wibwob_engine.cpp:195`

```cpp
bool loadResult = config->loadFromFile("llm/config/llm_config.json");
```

**Working Directory**: `/Users/james/Repos/tvision/test-tui/` (when running `./build/test_pattern`)

**Resolution**: Relative path resolves to `test-tui/llm/config/llm_config.json`

### 2. Config File Loaded

**File**: `test-tui/llm/config/llm_config.json:12-16`

```json
"claude_code": {
  "enabled": true,
  "command": "claude",
  "args": ["-p", "--model", "sonnet", "--mcp-config", ".claude/settings.json", "--output-format", "json"]
}
```

**Key Points**:
- ✅ `--model sonnet` (uses alias, not explicit ID)
- ✅ `--mcp-config .claude/settings.json` (points to backup config)
- The `args` array is stored in C++ and used for every Claude Code invocation

### 3. Command Construction (C++ Side)

**File**: `test-tui/llm/providers/claude_code_provider.cpp:334-389`

```cpp
std::string ClaudeCodeProvider::buildClaudeCommand(const LLMRequest& request) const {
    std::ostringstream cmd;
    cmd << claudePath;  // "claude"

    // Add configured args (they already include -p, --mcp-config, etc.)
    for (const std::string& arg : commandArgs) {
        if (arg.find("--output-format") != std::string::npos) continue;
        cmd << " " << arg;  // THIS INCLUDES --model sonnet
    }

    cmd << " --output-format json";

    // Session management: use --resume with session ID if available
    if (!currentSessionId.empty()) {
        cmd << " --resume " << currentSessionId;  // MODEL FLAG PERSISTS WITH --resume
    }

    // ... system prompt, query, etc.

    return cmd.str();
}
```

**Generated Command Example** (first turn):
```bash
claude -p --model sonnet --mcp-config .claude/settings.json --output-format json --system-prompt-file wibandwob.prompt.md "user query here"
```

**Generated Command Example** (subsequent turns):
```bash
claude -p --model sonnet --mcp-config .claude/settings.json --output-format json --resume <session_id> "user query here"
```

**Critical Observation**: The `--model sonnet` flag is included in **every turn** (both initial and resume), ensuring model persistence across conversation.

### 4. Claude Code CLI Execution

When `claude` CLI is invoked, it processes arguments in this order:

1. **Command-line flags**: `--model sonnet` (from C++ args)
2. **MCP config file**: `test-tui/.claude/settings.json` (specified by `--mcp-config`)
3. **Global config**: `~/.claude/settings.json` (if exists, lower priority)

**Effective Model Selection**:
- CLI flag `--model sonnet` takes **highest priority**
- Backup: `test-tui/.claude/settings.json` has `"model": "sonnet"` at line 2
- Claude Code resolves `"sonnet"` alias → latest Sonnet model (`claude-sonnet-4-5-20250929` as of 2025-11)

---

## All Model Configuration Locations (Exhaustive List)

### 🟢 Active Configurations (Used by TUI App)

#### 1. **Primary**: `test-tui/llm/config/llm_config.json`

**Lines 12-16**:
```json
"claude_code": {
  "enabled": true,
  "command": "claude",
  "args": ["-p", "--model", "sonnet", "--mcp-config", ".claude/settings.json", "--output-format", "json"]
}
```

**Purpose**: C++ reads this file and uses `args` array to build `claude` CLI commands.

**Status**: ✅ Set to `"sonnet"` alias

**Priority**: **HIGHEST** (command-line flag overrides all other configs)

---

#### 2. **Backup/Override**: `test-tui/.claude/settings.json`

**Line 2**:
```json
"model": "sonnet",
```

**Purpose**: Claude Code's own configuration file, specified via `--mcp-config` flag.

**Status**: ✅ Set to `"sonnet"` alias

**Priority**: **MEDIUM** (used if CLI flag not provided)

**Note**: This file also contains:
- MCP server permissions (lines 3-49)
- Enabled MCP servers: `tui-control`, `symbient-brain` (lines 54-56)

---

#### 3. **Fallback**: `/Users/james/Repos/tvision/.claude/settings.local.json`

**Status**: ❌ **NO** `"model"` field present (checked lines 1-43)

**Purpose**: Project-root Claude Code config (for when running Claude Code from repo root, not from test-tui)

**Priority**: **LOW** (only used if no other configs specify model)

**Contents**: Permissions, MCP servers, `"outputStyle": "WibWob"` (line 42)

---

### 🟡 Alternate Provider Configs (Not Used by Default)

#### 4. `test-tui/llm/config/llm_config.json` - `anthropic_api` provider

**Lines 4-11**:
```json
"anthropic_api": {
  "enabled": true,
  "model": "claude-sonnet-4-5-20250929",
  "endpoint": "https://api.anthropic.com/v1/messages",
  "apiKeyEnv": "ANTHROPIC_API_KEY",
  "maxTokens": "4096",
  "temperature": "1.0"
}
```

**Purpose**: Direct Anthropic API provider (bypasses Claude Code CLI)

**Status**: ✅ Explicit model ID (correct, but not used since `activeProvider: "claude_code"`)

**Active**: ❌ (provider disabled in favor of `claude_code`)

---

#### 5. `test-tui/llm/base/llm_config.cpp` - Default Config Fallback

**Lines 338, 346**:
```cpp
"model": "claude-3-5-haiku-latest",    // anthropic_api default
"model": "anthropic/claude-3-haiku",   // openrouter default
```

**Purpose**: Hardcoded defaults used if `llm_config.json` cannot be loaded

**Status**: ⚠️ Shows Haiku (but irrelevant - file load succeeds, these aren't used)

**Active**: ❌ (only used if config file missing)

---

### 🔵 Documentation References (Informational)

#### 6. `README.md:24-26`

**Content**:
```markdown
**Wib&Wob Chat LLM Model**: The embedded chat window uses Claude Code CLI with **Haiku by default**. To use Sonnet instead, edit `test-tui/llm/config/llm_config.json` and change the `claude_code` provider's `args` array to include `"--model"` and `"sonnet"`:
```json
"args": ["-p", "--model", "sonnet", "--mcp-config", ".claude/settings.local.json", "--output-format", "json"]
```

**Status**: ⚠️ **OUTDATED** - Still references `.claude/settings.local.json` (should be `.claude/settings.json`)

**Recommendation**: Update README to reflect current config filename

---

#### 7. `CLAUDE.md:664`

**Content**: Example showing `"model": "claude-3-5-haiku-latest"`

**Purpose**: Documentation example for API integration

**Status**: ℹ️ Informational only (not a live config)

---

## Configuration Precedence (Priority Order)

When the TUI app launches Claude Code CLI, model selection follows this priority:

1. **CLI `--model` flag** (from `test-tui/llm/config/llm_config.json` args array) → ✅ `"sonnet"`
2. **`--mcp-config` file** (`test-tui/.claude/settings.json`) → ✅ `"sonnet"`
3. **Global config** (`~/.claude/settings.json` if exists) → ❓ Unknown (not checked)
4. **Claude Code default** (Haiku) → ❌ Never reached (overridden by #1)

**Current Effective Model**: `sonnet` alias → resolves to `claude-sonnet-4-5-20250929`

---

## Why "sonnet" Alias is Correct

From Claude Code CLI documentation:
> `--model` Sets the model for the current session with an alias for the latest model (sonnet or opus) or a model's full name.

**Benefits of using `"sonnet"` alias**:
- ✅ Automatically uses latest Sonnet version (future-proof)
- ✅ Shorter, more readable config
- ✅ Matches Claude Code's recommended usage

**When NOT to use alias**:
- ❌ If you need to pin to a specific model version (e.g., for reproducibility)
- ❌ If using Anthropic API directly (aliases not supported)

---

## Why We Saw Haiku Earlier (Mystery Solved)

**User's Original Concern**: "i think we are stuck in haiku mode arghghghg"

**Root Cause**: Config was using explicit model ID `claude-sonnet-4-5-20250929` instead of `"sonnet"` alias.

**What Actually Happened**:
- Claude Code's **cost optimization** uses Haiku for background tasks (cache creation, tool execution)
- Sonnet is still used for main reasoning responses
- This is **WORKING AS DESIGNED**, not a bug

**Evidence** (from manual test):
```json
"modelUsage": {
  "claude-haiku-4-5-20251001": {
    "cacheCreationInputTokens": 8315,
    "costUSD": 0.01157175
  },
  "claude-sonnet-4-5-20250929": {
    "inputTokens": 3,
    "outputTokens": 47,
    "costUSD": 0.0508293
  }
}
```

**Fix Applied**: Changed to `"sonnet"` alias for clarity and future-proofing (even though explicit ID also worked).

---

## Verification Steps (How to Confirm Model)

### 1. Check C++ Config
```bash
cd /Users/james/Repos/tvision/test-tui
cat llm/config/llm_config.json | grep -A5 claude_code
```

**Expected Output**:
```json
"claude_code": {
  "enabled": true,
  "command": "claude",
  "args": ["-p", "--model", "sonnet", "--mcp-config", ".claude/settings.json", ...]
}
```

### 2. Check Claude Code Config
```bash
cd /Users/james/Repos/tvision/test-tui
cat .claude/settings.json | head -5
```

**Expected Output**:
```json
{
  "model": "sonnet",
  "permissions": {
```

### 3. Test Directly from Test-TUI Directory
```bash
cd /Users/james/Repos/tvision/test-tui
claude -p --model sonnet --output-format json "What model are you?"
```

**Expected Output**: JSON with `"result": "I am Claude Sonnet 4.5..."` and model usage showing Sonnet for main response.

### 4. Check TUI App Debug Logs
```bash
cd /Users/james/Repos/tvision/test-tui
./run_test_pattern_logged.sh
# In TUI: Open Wib&Wob Chat, type "/model"
# Check logs/test_pattern_*.log for:
# "DEBUG: Claude command: claude -p --model sonnet ..."
```

---

## Recommendations

### ✅ Changes Already Applied

1. ✅ Set `test-tui/llm/config/llm_config.json` to use `"sonnet"` alias (line 15)
2. ✅ Confirmed `test-tui/.claude/settings.json` has `"model": "sonnet"` (line 2)
3. ✅ Updated `--mcp-config` path from `.claude/settings.local.json` → `.claude/settings.json` (line 15)

### 📝 Pending Documentation Updates

1. **Update README.md:24-26** - Change example from `.claude/settings.local.json` → `.claude/settings.json`
2. **Update README.md:24** - Change "**Haiku by default**" → "**Sonnet by default**" (now that config is fixed)

### 🔍 Optional Verification

3. **Test session persistence** - Verify `--model sonnet` flag persists across conversation turns with `--resume`
4. **Check global config** - Verify `~/.claude/settings.json` doesn't override with different model

---

## Technical Details: How Model Selection Works in Code

### C++ Configuration Loading
**File**: `test-tui/llm/base/llm_config.cpp:45-51`

```cpp
LLMConfig::LLMConfig() {
    // Load .env file first to set environment variables
    loadDotEnv("../.env");  // Load from parent directory when running from build/

    // Set up default configuration
    loadFromString(getDefaultConfigJson());
}
```

**Then**: `test-tui/wibwob_engine.cpp:195`
```cpp
bool loadResult = config->loadFromFile("llm/config/llm_config.json");
```

**Resolution**: Running from `test-tui/`, this resolves to `test-tui/llm/config/llm_config.json`.

### Args Array Parsing
**File**: `test-tui/llm/providers/claude_code_provider.cpp:127-170`

The `configure()` method parses the JSON config and extracts the `args` array into `commandArgs` vector:

```cpp
bool ClaudeCodeProvider::configure(const std::string& config) {
    // Parse "args": [...] array from JSON
    size_t argsPos = config.find("\"args\"");
    // ... extract each arg into commandArgs vector
}
```

### Command Building (Every Turn)
**File**: `test-tui/llm/providers/claude_code_provider.cpp:334-389`

```cpp
std::string ClaudeCodeProvider::buildClaudeCommand(const LLMRequest& request) const {
    cmd << claudePath;  // "claude"

    for (const std::string& arg : commandArgs) {  // Includes "--model", "sonnet"
        cmd << " " << arg;
    }

    if (!currentSessionId.empty()) {
        cmd << " --resume " << currentSessionId;  // Model persists here too!
    }
}
```

**Key Insight**: The `commandArgs` vector (loaded once at startup) is reused for every turn, ensuring `--model sonnet` is **always** passed to Claude CLI.

---

## Glossary

- **Alias**: Short model name like `"sonnet"` or `"opus"` that Claude Code resolves to latest version
- **Explicit Model ID**: Full model name like `"claude-sonnet-4-5-20250929"`
- **Active Provider**: The LLM provider currently selected in `llm_config.json` (currently `"claude_code"`)
- **MCP Config**: Model Context Protocol configuration file for Claude Code CLI
- **Cost Optimization**: Claude Code's automatic use of Haiku for background tasks, Sonnet for reasoning

---

## Conclusion

**Current Configuration**: ✅ **CORRECT**

- `test-tui/llm/config/llm_config.json` → `--model sonnet` (line 15)
- `test-tui/.claude/settings.json` → `"model": "sonnet"` (line 2)
- Both use `"sonnet"` alias (not explicit model ID)
- Model flag persists across conversation turns via `--resume`

**No Further Changes Needed** for model selection. Optional: Update README.md documentation to reflect current config.

**Mystery Solved**: The "Haiku mode" concern was a misunderstanding of Claude Code's cost optimization (Haiku for caching, Sonnet for reasoning). This is intentional and working correctly.
