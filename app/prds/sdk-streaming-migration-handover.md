# SDK Streaming Chat Migration - Handover to Opus

**tl;dr**: Cherry-pick of streaming SDK chat (commit 7ca9f2b) into develop is 80% complete. Config conflicts resolved, one remaining conflict in wibwob_engine.cpp getCurrentModel(), then need to accept new files, move SDK bridge to app/ structure, update CMakeLists, build and test.

## Current State (2024-12-02)

**Branch**: `~/Repos/tvision-develop` (worktree pointing to origin/develop, detached HEAD)
**Operation**: Cherry-picking commit `7ca9f2b` (Claude Code SDK streaming integration)
**Status**: In progress with merge conflicts

### What's Been Done ✓

1. **Worktree created**: `~/Repos/tvision-develop` pointing to origin/develop
2. **Cherry-pick initiated**: `git cherry-pick 7ca9f2b`
3. **Config conflicts RESOLVED**:
   - `app/llm/config/llm_config.json` - merged model + SDK params
   - `app/llm/base/llm_config.cpp` - added debug line + merged default config with claude_code_sdk provider
4. **Auto-merged files** (already staged):
   - `app/llm/base/llm_config.h`
   - `app/wibwob_engine.h`
   - SDK bridge files added to `test-tui/llm/sdk_bridge/` (need to move to `app/`)

### What's Left TODO

#### 1. Resolve Final Conflict in wibwob_engine.cpp

**File**: `app/wibwob_engine.cpp` line ~153
**Issue**: getCurrentModel() method needs to handle claude_code_sdk provider

**Resolution**: Take the incoming version (from 7ca9f2b) which adds this logic:

```cpp
std::string WibWobEngine::getCurrentModel() const {
    if (!config) return "unknown";

    std::string provider = getCurrentProvider();
    if (provider == "none") return "unknown";

    ProviderConfig providerConfig = config->getProviderConfig(provider);

    // For claude_code and claude_code_sdk, return appropriate model name
    if (provider == "claude_code") {
        return "Claude Code";
    } else if (provider == "claude_code_sdk") {
        return "sonnet";  // Show the actual alias we send to Claude Code
    }

    return providerConfig.model.empty() ? "unknown" : providerConfig.model;
}
```

**Command**:
```bash
cd ~/Repos/tvision-develop
# Edit app/wibwob_engine.cpp to resolve conflict (remove markers, take incoming version)
# OR use: git checkout --theirs app/wibwob_engine.cpp (but verify logic first)
```

#### 2. Accept New SDK Provider Files

**Files marked UA (unmerged, added by them)**:
- `app/llm/providers/claude_code_sdk_provider.cpp`
- `app/llm/providers/claude_code_sdk_provider.h`
- `app/prds/wibwob-claude-code-sdk-modernisation.md`

These are already in the correct location (git moved them from test-tui/ to app/ during merge).

**Command**:
```bash
cd ~/Repos/tvision-develop
git add app/llm/providers/claude_code_sdk_provider.{cpp,h}
git add app/prds/wibwob-claude-code-sdk-modernisation.md
```

#### 3. Handle Old test-tui Files

**Files marked DU (deleted by us)**:
- `test-tui/wibwob_view.cpp`
- `test-tui/wibwob_view.h`

These were moved to `app/` in the develop refactor. Remove them:

```bash
cd ~/Repos/tvision-develop
git rm test-tui/wibwob_view.{cpp,h}
```

#### 4. Move SDK Bridge to app/ Structure

The SDK bridge files were added to `test-tui/llm/sdk_bridge/` but need to be in `app/llm/sdk_bridge/`:

**Current** (wrong location):
```
test-tui/llm/sdk_bridge/
├── claude_sdk_bridge.js
├── package.json
└── package-lock.json
```

**Target** (correct location):
```
app/llm/sdk_bridge/
├── claude_sdk_bridge.js
├── package.json
└── package-lock.json
```

**Commands**:
```bash
cd ~/Repos/tvision-develop
mkdir -p app/llm/sdk_bridge
git mv test-tui/llm/sdk_bridge/* app/llm/sdk_bridge/
rmdir test-tui/llm/sdk_bridge test-tui/llm test-tui  # if empty
```

#### 5. Update Config Path Reference

Update `app/llm/config/llm_config.json` nodeScriptPath:

**Change from**: `"nodeScriptPath": "llm/sdk_bridge/claude_sdk_bridge.js"`
**Change to**: `"nodeScriptPath": "app/llm/sdk_bridge/claude_sdk_bridge.js"`

#### 6. Update CMakeLists.txt

Add SDK provider to build:

**File**: `app/CMakeLists.txt` (or wherever LLM sources are defined)

Add:
```cmake
    llm/providers/claude_code_sdk_provider.cpp
    llm/providers/claude_code_sdk_provider.h
```

#### 7. Stage All Resolved Files

```bash
cd ~/Repos/tvision-develop
git add app/llm/config/llm_config.json
git add app/llm/base/llm_config.cpp
git add app/wibwob_engine.cpp  # after resolving conflict
# ... all other resolved files
```

#### 8. Complete Cherry-Pick

```bash
cd ~/Repos/tvision-develop
git cherry-pick --continue
# Will prompt for commit message - keep the original or edit
```

#### 9. Build and Test

```bash
cd ~/Repos/tvision-develop/app
mkdir -p build && cd build
cmake ..
make

# Test the streaming chat
cd /Users/james/Repos/tvision-develop
# Find and run the appropriate test app
```

#### 10. Verify Streaming Works

**Test procedure**:
1. Run the TUI app with wibwob chat
2. Send a query: "Count to 20 slowly"
3. **During response**, try to drag/move windows
4. **Expected**: Windows should move smoothly (UI responsive)
5. **Expected**: Response appears word-by-word (streaming)
6. Compare with origin/develop behavior (should freeze UI)

#### 11. Push to Develop (if tests pass)

```bash
cd ~/Repos/tvision-develop
git checkout -b sdk-streaming-migration
git push origin sdk-streaming-migration
# Create PR to develop
```

## Architecture Context

### The Problem
**Origin/develop** has blocking chat that freezes entire UI during LLM queries (uses synchronous `popen()` in `claude_code_provider.cpp`).

### The Solution
**Commit 7ca9f2b** introduces streaming SDK chat via:
- Background thread for async I/O
- Thread-safe queue for stream chunks
- Main thread polling (non-blocking)
- Incremental callbacks as data arrives

### Key Files in This Migration

**SDK Provider** (NEW):
- `app/llm/providers/claude_code_sdk_provider.{cpp,h}` - Non-blocking streaming provider

**SDK Bridge** (NEW):
- `app/llm/sdk_bridge/claude_sdk_bridge.js` - Node.js bridge to Claude Code SDK
- `app/llm/sdk_bridge/package.json` - Dependencies (axios, zod)

**Modified**:
- `app/llm/config/llm_config.json` - Defaults to claude_code_sdk
- `app/llm/base/llm_config.cpp` - Default config includes SDK provider
- `app/wibwob_engine.cpp` - getCurrentModel() handles SDK provider
- `app/wibwob_engine.h` - Header changes

**UI Layer** (might need updates):
- Check if `app/wibwob_view.{cpp,h}` needs streaming callback support
- Original commit modified `test-tui/wibwob_view.cpp` for streaming
- May need to port those changes to `app/wibwob_view.cpp`

## Reference Documents

1. **Detailed architectural comparison**: `/Users/james/Repos/tvision/test-tui/prds/blocking-vs-streaming-chat-comparison.md`
2. **Original SDK modernisation PRD**: Will be at `app/prds/wibwob-claude-code-sdk-modernisation.md` after merge
3. **Original commit**: `7ca9f2b` in test-tui-imac-unknown-state branch

## Git Status Reference

```
UU app/llm/base/llm_config.cpp           # RESOLVED (remove markers, stage)
M  app/llm/base/llm_config.h             # Auto-merged (stage)
UU app/llm/config/llm_config.json        # RESOLVED (remove markers, stage)
UA app/llm/providers/claude_code_sdk_provider.cpp  # Accept (stage)
UA app/llm/providers/claude_code_sdk_provider.h    # Accept (stage)
UA app/prds/wibwob-claude-code-sdk-modernisation.md # Accept (stage)
UU app/wibwob_engine.cpp                 # NEEDS RESOLUTION (one conflict remains)
M  app/wibwob_engine.h                   # Auto-merged (stage)
A  test-tui/llm/sdk_bridge/*             # Move to app/llm/sdk_bridge/
DU test-tui/wibwob_view.{cpp,h}         # Remove (git rm)
```

## Potential Issues to Watch

### 1. wibwob_view Streaming Support
The original commit modified `test-tui/wibwob_view.cpp` to handle streaming callbacks:
- `startStreamingMessage()`
- `appendToStreamingMessage()`
- `finishStreamingMessage()`

Check if `app/wibwob_view.cpp` has these methods or if they need to be ported from the old branch.

### 2. CMake Build Configuration
Develop branch restructured build system. May need to:
- Update CMakeLists to include SDK provider
- Ensure SDK bridge path is correct
- Check if Node.js is available on build system

### 3. Node.js Dependencies
SDK bridge requires:
```bash
cd app/llm/sdk_bridge
npm install  # Installs axios, zod
```

### 4. Thread Safety
SDK provider uses `std::thread`, `std::mutex`, `std::condition_variable`. Ensure Turbo Vision event loop plays nicely with background threads (should be fine with polling approach).

## Success Criteria

✅ Build completes without errors
✅ TUI app launches successfully
✅ Wibwob chat connects to SDK provider
✅ Queries stream word-by-word
✅ UI remains responsive during queries (can drag windows, etc)
✅ No crashes or thread safety issues
✅ Performance acceptable (~50ms overhead vs blocking version)

## Rollback Plan

If migration fails:
```bash
cd ~/Repos/tvision-develop
git cherry-pick --abort
# Develop branch remains untouched
```

## Next Steps After This Migration

Once SDK streaming works in develop:
1. Consider merging other features from test-tui-imac-unknown-state:
   - `26b66de` - MCP tools module (if useful)
   - `508256e` - SVG export (if useful)
2. Remove or archive test-tui-imac-unknown-state branch
3. Make develop the canonical branch for future work

## Handover Complete

**Current working directory**: `~/Repos/tvision-develop`
**Git state**: Mid-cherry-pick with conflicts
**Next action**: Resolve wibwob_engine.cpp conflict, then follow steps 2-11 above

Good luck, Opus. The hard architectural analysis is done, just need mechanical merge completion and testing.
