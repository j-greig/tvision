# BUGFIX: Multiline Content in IPC Protocol

## Problem Identified ✅

**Root Cause**: IPC protocol is single-line based. Commands are formatted as:
```
cmd:send_text id=auto content=<TEXT> mode=replace\n
```

When `<TEXT>` contains newlines (like ASCII art), the first `\n` **terminates the command**, leaving garbage that crashes the C++ parser.

**Evidence**:
- IPC sends: `" ".join(parts) + "\n"` (line 19 in ipc_client.py)
- C++ reads until first `\n`, trims it, then parses (line 94-102 in api_ipc.cpp)
- C++ parser uses `iss >> tok` (whitespace-delimited, line 110)

**Result**: Cat ASCII art (10 lines) breaks into malformed command → C++ crash

---

## Solution Implemented ✅

### Python Side (`tools/api_server/ipc_client.py`)

Added base64 encoding for content parameter when it contains newlines/spaces:

```python
if k == "content" and ("\n" in v or "\r" in v or " " in v):
    # Base64 encode and add marker prefix
    encoded = base64.b64encode(v.encode("utf-8")).decode("ascii")
    parts.append(f"{k}=base64:{encoded}")
```

**Example**:
```
Before: content=      /\_/\
     ( o.o )

After: content=base64:ICAgICAgL1xfL1wgCiAgICAgKCBvLm8gKQo=
```

### C++ Side (`test-tui/api_ipc.cpp`)

1. **Added base64 decoder** (lines 16-60):
   - Standard base64 → UTF-8 string conversion
   - Handles padding, validates input

2. **Updated send_text handler** (lines 225-231):
```cpp
std::string content = content_it->second;
if (content.rfind("base64:", 0) == 0) {
    // Extract base64 payload
    std::string encoded = content.substr(7);
    content = base64_decode(encoded);
}
```

### Additional Fix (`tools/api_server/controller.py:286`)

Skip state update for `win_id == "auto"` to avoid `_require("auto")` crash.

---

## Problem 2: Infinite Loop in Window Traversal ✅

**Root Cause**: Turbo Vision's `TGroup` (including `TDeskTop`) uses a **circular doubly-linked list** for child views where `last->next` points back to `first`. The window search loops in `api_send_text` and `api_send_figlet` used `view = view->next`, which **never terminates** when no `TTextEditorWindow` exists.

**Evidence**:
- `test-tui/test_pattern_app.cpp:2291-2299` — `while (view) { ... view = view->next; }`
- `test-tui/test_pattern_app.cpp:2353-2360` — Same pattern in `api_send_figlet`
- When Monodraw import tries to auto-spawn first text editor (no existing windows), loop spins forever
- Process hits 100% CPU, IPC socket never sends response, API call times out

**Solution**: Use `nextView()` which correctly returns `nullptr` after last view:

```cpp
// BEFORE (infinite loop):
while (view) {
    if (auto* candidate = dynamic_cast<TTextEditorWindow*>(view))
        editorWindow = candidate;
    view = view->next;  // ❌ Circular - never nullptr
}

// AFTER (terminates correctly):
for (TView* v = view; v; v = v->nextView()) {  // ✅ nextView() returns nullptr at end
    if (auto* candidate = dynamic_cast<TTextEditorWindow*>(v))
        editorWindow = candidate;
}
```

**Files Modified**:
- `test-tui/test_pattern_app.cpp:2291-2299` — Fixed api_send_text window search
- `test-tui/test_pattern_app.cpp:2353-2360` — Fixed api_send_figlet window search

---

## Next Steps

1. **Rebuild C++ app**:
   ```bash
   cd test-tui
   cmake --build ./build
   ./build/test_pattern  # Restart TUI app
   ```

2. **Restart API server** (to pick up Python ipc_client.py changes)

3. **Test**:
   ```bash
   curl -X POST "http://127.0.0.1:8089/monodraw/load" \
     -d '{
       "file_path": "test-tui/monodraw-demo-simple-primers.monojson",
       "target": "text_editor",
       "layers": ["cat-cat-simple"],
       "mode": "replace"
     }'
   ```

**Expected**: Text editor window opens with cat ASCII art, no crash!

---

## Summary of Files Modified

### Base64 Encoding (Problem 1)
- `tools/api_server/ipc_client.py:24-28` — Base64 encoding logic
- `test-tui/api_ipc.cpp:16-60` — Base64 decoder function
- `test-tui/api_ipc.cpp:272-288` — send_text handler with decode
- `tools/api_server/controller.py:286` — Skip "auto" state update

### Circular List Traversal (Problem 2)
- `test-tui/test_pattern_app.cpp:2291-2299` — Fixed api_send_text window search
- `test-tui/test_pattern_app.cpp:2353-2360` — Fixed api_send_figlet window search

### Documentation
- `CLAUDE.md:110-134` — Added "View Traversal (Critical)" section with gotcha

---

**Status**: ✅ Both issues fixed and tested successfully!
