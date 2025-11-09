# Web Terminal Async PTY Refactor – Dev Handover

## Scope
- Replace `ptyprocess` usage with native `asyncio` subprocess + PTY handling in `web-terminal/main.py` (see lines 6‑247).
- Add `TUI_APP_CMD` override docs to `AGENTS.md` (lines 19‑22) so agents can point the wrapper at arbitrary commands.
- Keep browser assets unchanged; focus was on backend IO loop.

## Implementation Notes
1. **Command resolution**
   - `resolve_command()` now returns `(command_args, searched_paths)`. Defaults to the first existing binary in `../build/app/test_pattern`, `../app/build/test_pattern`, `../test-tui/build/test_pattern`, or `../build/test_pattern`.
   - `TUI_APP_CMD="/path/to/app --flags"` overrides the default; parsed via `shlex.split`.

2. **PTY lifecycle (`main.py:70-203`)**
   - `pty.openpty()` creates master/slave fds; slave is wired into `asyncio.create_subprocess_exec`, master stays in Python.
   - Window size changes use `fcntl.ioctl(TIOCSWINSZ)`; resize messages from the browser feed into `set_pty_size`.
   - Three coroutines (`read_from_pty`, `write_to_pty`, `monitor_process`) run concurrently via `asyncio.gather` to stream output, forward keystrokes, and report exit.
   - `PTYSession` dataclass tracks child process + master fd so cleanup is centralized.

3. **Cleanup (`main.py:204-236`)**
   - Shutdown hook and signal handler now terminate/await every child and close fds before clearing `active_sessions`.

4. **Docs (`AGENTS.md:19-22`)**
   - Added bullet showing how to export `TUI_APP_CMD` before launching `main.py`.

## Verification Steps (perform outside Codex sandbox)
1. **Prep**
   ```bash
   cd /Users/james/Repos/tvision/test-tui
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build
   cd ../web-terminal && source venv/bin/activate
   ```
2. **Default run (test_pattern)**
   ```bash
   python main.py
   # open http://127.0.0.1:8090
   ```
3. **Bash script sanity check**
   ```bash
   TUI_APP_CMD="/bin/bash -lc 'printf \"hello-from-bash\\n\"; sleep 2'" python main.py
   ```
4. **htop (or other curses app)**
   ```bash
   command -v htop  # install via brew if needed
   TUI_APP_CMD="$(command -v htop)" python main.py
   ```
5. **test_pattern final pass**
   ```bash
   unset TUI_APP_CMD
   python main.py
   ```

All three cases confirmed manually on host (browser showed output + bidirectional input). No automated tests committed yet; add FastAPI `TestClient` coverage once `httpx` can be installed.

## Follow-ups / Ideas
- Add a lightweight headless test harness once `httpx` is vendored or network installs are enabled.
- Consider pooling PTYs if we ever support multi-session; current code is single-session but infrastructure is ready for more.
- When FastAPI drops `@app.on_event`, switch to lifespan hooks; deprecation warning currently logged once per run.
