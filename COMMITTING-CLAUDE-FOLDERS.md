# Committing .claude/ Folders to Git

**tl;dr:** `.claude/` folders contain project-specific Claude Code config (MCP servers, permissions, custom skills). Commit `test-tui/.claude/` with team configs, keep root `.claude/` private with personal settings. Use `git add -f` to bypass gitignore, remove personal paths before committing.

## The Problem

Claude Code creates `.claude/` folders for configuration, but they're usually gitignored by default (via `.*` patterns). This creates a dilemma:

- **Personal settings** (root `.claude/settings.local.json`) - shouldn't be committed (API keys, personal paths)
- **Project settings** (subdirectory `.claude/settings.json`) - should be committed (team MCP servers, shared workflows)

Without clear guidance, teams either:
1. Accidentally commit personal credentials
2. Miss committing valuable project configs
3. Struggle to reproduce AI-assisted workflows

## Non-Technical Sketch

Think of it like IDE settings:

```
your-project/
├── .vscode/               ← Team settings (committed)
│   ├── settings.json      ← Shared editor config
│   └── extensions.json    ← Recommended extensions
└── .vscode.local/         ← Personal settings (gitignored)
    └── settings.json      ← Your custom keybindings
```

Claude Code's `.claude/` folders work the same way - some belong to the project, some belong to you.

## What Goes Where

### ✅ Commit These (Project-Level)
**Location:** `<project-subdir>/.claude/` (e.g., `test-tui/.claude/`)

**Contains:**
- `settings.json` - MCP server configs for the embedded chat
- `skills/` - Custom AI skills specific to this project
- `commands/` - Slash commands for project workflows

**Why commit:**
- Enables reproducible AI-assisted development
- Documents which MCP servers the project uses
- Shares custom skills with the team
- Makes onboarding easier ("just run the chat window")

**Example:** `test-tui/.claude/settings.json`
```json
{
  "enableAllProjectMcpServers": true,
  "enabledMcpjsonServers": [
    "tui-control",
    "symbient-brain"
  ]
}
```

### ❌ Don't Commit These (Personal)
**Location:** Root `.claude/settings.local.json`

**Contains:**
- Personal API keys in `apiKeyEnv` references
- User-specific file paths (`/Users/james/...`)
- Personal tool permissions from your workflows
- IDE integration settings

**Why not commit:**
- Leaks personal paths and preferences
- May expose credentials
- Clutters diffs with session-specific changes
- Not relevant to other developers

## Git Workflow

### Step 1: Check What's Ignored

```bash
# Check if .claude is gitignored
git check-ignore -v test-tui/.claude/settings.json
# Output: .gitignore:25:.*	test-tui/.claude/settings.json
```

If ignored, you'll need to force-add it.

### Step 2: Clean Personal Data

Before committing project configs, scrub personal info:

```bash
# Bad: Hardcoded username
"Read(//Users/james/Repos/**)"

# Good: Relative path
"Read(test-tui/**)"

# Bad: Specific file with username
"Bash(cat /Users/james/Repos/tvision/test-tui/file.json)"

# Good: Relative from project root
"Bash(cat test-tui/file.json)"
```

**Quick check:**
```bash
grep -r "Users/$(whoami)" test-tui/.claude/
```

If this returns matches, clean them before committing.

### Step 3: Force Add and Commit

```bash
# Force add bypasses .gitignore
git add -f test-tui/.claude/

# Check what's staged
git status test-tui/.claude/

# Commit with descriptive message
git commit -m "📦 config: Add Claude Code MCP settings for embedded chat

Enables tui-control and symbient-brain MCP servers for AI-assisted
window management and memory persistence in the embedded Wib&Wob chat.

Includes custom skills:
- beasties/ - ASCII art generator with 180+ primers
- dream-protocol/ - Dream logging
- fax/ - Fax machine aesthetic
- music-notation/ - ASCII notation system"
```

### Step 4: Verify and Push

```bash
# Verify no credentials leaked
git show --stat

# Check file contents
git show test-tui/.claude/settings.json

# Push when safe
git push origin your-branch
```

## What to Exclude

Add to `.gitignore` if not already present:

```gitignore
# Personal Claude settings (root level)
.claude/settings.local.json
.claude/**/logs/

# Session data
.claude/**/sessions/
.claude/**/.DS_Store

# But allow project configs (they'll need git add -f)
# Project-level .claude/ folders must be force-added
```

## Directory Structure Examples

### Good: Separated Personal/Project

```
tvision/                           ← Project root
├── .claude/                       ← Personal (gitignored)
│   └── settings.local.json        ← Your settings for working on repo
├── test-tui/                      ← Subproject
│   ├── .claude/                   ← Project-specific (committed)
│   │   ├── settings.json          ← MCP servers for embedded chat
│   │   └── skills/                ← Custom AI skills
│   │       ├── beasties/
│   │       └── dream-protocol/
│   └── build/
└── .gitignore                     ← Ignores root .claude/
```

### Bad: Mixed Personal/Project

```
project/
├── .claude/                       ← DANGER: Mixed use
│   ├── settings.json              ← Has team MCP servers
│   ├── my-api-keys.env            ← LEAKED CREDENTIALS
│   └── /Users/james/secrets/      ← LEAKED PATHS
```

## Real-World Example: This Project

**Committed:**
- `test-tui/.claude/settings.json` - MCP config for embedded chat
- `test-tui/.claude/skills/` - 180+ ASCII art primers
- `test-tui/.claude/skills-todo/` - Planned features

**Not Committed:**
- `.claude/settings.local.json` - Personal permissions for repo work
- Contains my username in paths
- Contains git commit templates from my workflow

**Commit:**
```bash
cd /Users/james/Repos/tvision
git add -f test-tui/.claude/
git commit -m "🎨🤖📦 feat: Add Claude Code MCP settings and custom skills library"
git push myfork test-tui-apps
```

Result: Team can now run the embedded chat with the same MCP servers.

## Common Gotchas

### 1. "Git add doesn't stage my .claude files"
**Problem:** Caught by `.gitignore` pattern `.*`

**Solution:** Use `git add -f` (force)
```bash
git add -f test-tui/.claude/settings.json
```

### 2. "Teammate can't use my MCP servers"
**Problem:** Forgot to commit `.claude/settings.json`

**Solution:** Commit the project-level config
```bash
git add -f test-tui/.claude/settings.json
git commit -m "config: Add MCP server config for embedded chat"
```

### 3. "Accidentally committed my username"
**Problem:** Hardcoded paths leaked

**Solution:** Amend the commit if not pushed, or create fix commit
```bash
# Fix the file
vim test-tui/.claude/settings.json
# Remove /Users/james paths

# Amend if not pushed
git add test-tui/.claude/settings.json
git commit --amend --no-edit

# Or create new commit if already pushed
git add test-tui/.claude/settings.json
git commit -m "fix: Remove hardcoded usernames from Claude config"
```

### 4. ".DS_Store files in my commit"
**Problem:** macOS metadata files

**Solution:** Add to `.gitignore` and remove
```bash
echo ".DS_Store" >> .gitignore
git rm --cached test-tui/.claude/**/.DS_Store
git commit -m "chore: Remove .DS_Store files"
```

## Quick Reference

```bash
# Check if ignored
git check-ignore -v path/to/.claude/file

# Clean personal data
grep -r "Users/$(whoami)" path/to/.claude/

# Force add project config
git add -f test-tui/.claude/

# Verify no leaks
git diff --staged

# Commit
git commit -m "config: Add Claude settings"

# Push
git push origin branch-name
```

## When to Commit .claude/

**✅ Commit when:**
- Project has embedded AI features (like our TUI chat)
- MCP servers are part of the development workflow
- Custom skills are project-specific
- Team needs reproducible AI tooling

**❌ Don't commit when:**
- It's your personal Claude Code workspace config
- Contains API keys or credentials
- Has user-specific paths
- Is temporary/experimental

## Best Practice

1. **Separate locations** - Root `.claude/` for personal, subdir `.claude/` for project
2. **Scrub before commit** - Remove personal paths and credentials
3. **Force add** - Use `git add -f` to bypass gitignore
4. **Document why** - Commit message should explain what the configs enable
5. **Review diffs** - Check staged changes for leaks before pushing

## References

- Claude Code docs: https://docs.claude.com/en/docs/claude-code
- MCP protocol: https://modelcontextprotocol.io
- This project's config: [test-tui/.claude/settings.json](test-tui/.claude/settings.json)
