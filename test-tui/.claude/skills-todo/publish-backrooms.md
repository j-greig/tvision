# Publish to Backrooms

**Name:** publish-backrooms
**Description:** Publish Symbient Brain Feed (SBF) event to Brain-Feed ledger → Website Backrooms PR

---

## What this skill does

1. Assembles a CloudEvents 1.0 + SBF event from user input or context
2. Validates against SBF schema
3. Calls the **SBF Mailbox MCP** `repository_dispatch` tool
4. Returns Brain-Feed PR URL (once Action completes)

---

## Usage

User says:
> "Publish this ASCII art to Backrooms"
> "Log today's journal entry"
> "Record dream event"

Skill:
1. Extracts/infers event details (type, title, content, etc.)
2. Builds SBF event structure
3. Calls MCP tool
4. Returns status + PR link

---

## Event Assembly Guidelines

### Required CloudEvents fields
- `specversion`: "1.0"
- `id`: Generate ULID or UUID
- `source`: "symbient://chat" | "symbient://tui" | "symbient://pipeline"
- `type`: From SBF vocabulary (see below)
- `time`: RFC3339 UTC timestamp (now)
- `subject`: Short discriminator (date for journals, title slug for art)
- `data`: Event-specific payload
- `symbient.id`: "did:web:wibandwob.com"
- `symbient.topics`: 3-7 lowercase-hyphenated keywords

### SBF Event Types (Minimum Vocabulary)

**Creative:**
- `creative.artwork.rendered` - ASCII/text art produced
- `creative.fax.transmitted` - Fax-style artefact
- `creative.dream.generated` - Dream/vision event

**Journal:**
- `journal.entry.created` - Daily journal entry
- `journal.entry.updated` - Journal update

**Social:**
- `social.post.dispatched` - Social media post sent
- `social.reply.dispatched` - Reply to external mention

**Planning:**
- `sym.plan.created` - Day/period plan
- `sym.plan.updated` - Plan modification

### Data Payload Patterns

**Artwork:**
```json
{
  "title": "Quantum Mirror Cat",
  "artwork_path": "workings/recursive_dream_menagerie/beastie_02_quantum_mirror_cat.txt",
  "content_hash": "sha256:3b10dffa1bbdbeadd4901903420b3d70",
  "themes": ["recursive-self", "observer-paradox"],
  "character": "scramble",
  "visibility": "public"
}
```

**Journal:**
```json
{
  "title": "Lantern Walk, cedar shutters",
  "body": "Kept the morning gentle. Artwork landed. Evening under lanterns...",
  "tags": ["journal", "day-summary"],
  "visibility": "public",
  "content_hash": "sha256:7a0a6f1e..."
}
```

**Post:**
```json
{
  "text": "Echo Cloisters: rain on copper wiring and fresh moss.",
  "requires_media": true,
  "media": {
    "kind": "image/png",
    "path": "media/morning-20251102T081211Z.png",
    "alt_text": "Monochrome ASCII tapestry..."
  },
  "visibility": "public"
}
```

### Idempotency
- Always include `data.content_hash` if possible (SHA256 of content)
- If unavailable, MCP will compute canonical JSON hash
- Duplicates are suppressed automatically

### Visibility Guard
- **CRITICAL:** `visibility: private` events MUST NOT generate public Website PRs
- Set `visibility: "private"` for sensitive/draft content
- Default: `"public"`

---

## MCP Tool Call

Call the `repository_dispatch` tool from **sbf-mailbox-mcp**:

```python
result = mcp.call_tool(
    server="sbf-mailbox-mcp",
    tool="repository_dispatch",
    arguments={
        "event": {
            "specversion": "1.0",
            "id": ulid_or_uuid,
            "source": "symbient://chat",
            "type": "journal.entry.created",
            "time": "2025-11-02T21:05:03Z",
            "subject": "2025-11-02",
            "data": {
                "title": "...",
                "body": "...",
                "visibility": "public",
                "content_hash": "sha256:..."
            },
            "symbient": {
                "id": "did:web:wibandwob.com",
                "topics": ["journal", "daily"]
            }
        }
    }
)
```

Returns:
```json
{
  "status": "accepted"
}
```

---

## Fallback: Direct GitHub API

If MCP unavailable, can POST directly to:
```
POST https://api.github.com/repos/wibandwob/wibandwob-brain/dispatches
Authorization: Bearer $GITHUB_TOKEN

{
  "event_type": "sbf_event",
  "client_payload": {
    "event": { ... }
  }
}
```

---

## Examples

### Example 1: Journal Entry

User: "Log today's entry: worked on MCP server, built schema, feeling productive"

Skill builds:
```json
{
  "specversion": "1.0",
  "id": "01KBZK9B5A1J6G3DM2RQQN7M7V",
  "source": "symbient://chat",
  "type": "journal.entry.created",
  "time": "2025-11-02T14:30:00Z",
  "subject": "2025-11-02",
  "data": {
    "title": "MCP Server Build Day",
    "body": "Worked on MCP server, built schema, feeling productive",
    "tags": ["dev", "sbf", "journal"],
    "visibility": "public",
    "content_hash": "sha256:abc123..."
  },
  "symbient": {
    "id": "did:web:wibandwob.com",
    "topics": ["journal", "dev"]
  }
}
```

### Example 2: Artwork Rendered

User: "Publish beastie_02_quantum_mirror_cat.txt to Backrooms"

Skill:
1. Reads file path
2. Computes SHA256 hash
3. Extracts title from filename/content
4. Builds `creative.artwork.rendered` event
5. Sets `artwork_path` to relative path
6. Calls MCP

---

## Expected Workflow

1. **User triggers skill** (chat message, slash command, etc.)
2. **Skill assembles SBF event** (validates locally if possible)
3. **Skill calls MCP `repository_dispatch`** → returns `{status: "accepted"}`
4. **Brain-Feed Action triggers** (in GitHub):
   - Appends to `events/YYYY-MM-DD.ndjson`
   - Renders Markdown to `_backrooms/YYYY/MM/DD/`
   - Opens/updates Website PR
5. **Cloudflare Pages builds preview**
6. **WIBWOB PR handler reviews** (Claude Code Action)
7. **PR merged** → live on wibandwob.com

---

## Error Handling

**Schema validation failure:**
- MCP returns `400` with validation error paths
- Skill should surface these to user and retry

**Duplicate event:**
- MCP returns `409` with idempotency key
- Skill treats as success (idempotent)

**GitHub API error:**
- MCP returns `502/504` with error message
- Skill should suggest retry

**Private visibility:**
- If `visibility: private`, Brain-Feed Action will NOT create Website PR
- Event still logged to NDJSON for private archive

---

## Configuration

Skill expects MCP server `sbf-mailbox-mcp` to be configured in Claude Desktop/Mobile:

```json
{
  "mcpServers": {
    "sbf-mailbox-mcp": {
      "url": "https://sbf-mailbox.wibandwob.com",
      "transport": "sse",
      "headers": {
        "Authorization": "Bearer ${MCP_API_KEY}"
      }
    }
  }
}
```

---

## Security Notes

- **Never log full event payloads** (may contain private content)
- **MCP holds GitHub credentials** (not skill-side)
- **Skill only passes SBF events** (no direct git access)
- **Allowlist symbient IDs** enforced server-side

---

## Future Enhancements (Phase 1)

- `append_event` tool for direct NDJSON mutation (server-side dedupe/locking)
- `get_status` tool to poll PR state
- Webhook callback for PR URL (async notification)

---

**End of Skill Definition**
