# Repository Guidelines

## Project Structure & Module Organization
- Library: `source/tvision` (engine) and `source/platform` (platform glue).
- Public API: `include/tvision/**` (exported headers only).
- App (primary): `test-tui/` — MS‑DOS‑style ASCII art apps and tools.
- Examples: `examples/**` (classic Turbo Vision demos).
- Tests: `test/**` (GoogleTest). Web tooling: `web-terminal/`.
- Build output: `build/` (created by CMake commands below).

## Build, Test, and Development Commands
- Configure + build (Release):
  - `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`
  - `cmake --build build`
- Run library demos: see README. Primary app commands below.
- Enable and run C++ tests (GoogleTest):
  - `cmake -S . -B build -DTV_BUILD_TESTS=ON && cmake --build build --target tvision-test-run`
  - Or run the binary: `./build/tvision-test`
- Web terminal (browser-hosted TUI):
  - `cd web-terminal && python3 -m venv venv && source venv/bin/activate && pip install -r requirements.txt && python main.py`

## test-tui App Focus
- Purpose: playground for MS‑DOS‑era, ASCII/ANSI art TUIs (patterns, gradients, animation, painting).
- Build (from `test-tui/`):
  - `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build`
- Run targets:
  - `./build/test_pattern` — multi-window patterns/gradients, ASCII wallpaper.
  - `./build/simple_tui` — minimal reference app.
  - `./build/frame_file_player --file frames_demo.txt [--fps NN]` — plays ASCII frames (see `FRAME_PLAYER.md`).
  - Optional: `ansi_viewer_main.cpp` builds an ANSI art viewer; assets under `test-tui/ansi/`.
- Layout & naming:
  - Entrypoints: `*_main.cpp`; views: `*_view.{h,cpp}`; helpers: `lower_snake_case.{h,cpp}`.
  - Painting MVP: `test-tui/paint/*` (canvas, palette, tools).

## Coding Style & Naming Conventions
- Language: C++14. Follow existing patterns in `source/tvision`.
- Indentation: spaces; keep file-local style consistent.
- Naming: classes prefixed `T*` (e.g., `TEditor`), functions `camelCase`, files `lower_snake_case.cpp`.
- Public API goes under `include/tvision`; internal-only headers stay local to implementation.

## Testing Guidelines
- Framework: GoogleTest. Place tests under `test/<area>/<name>.test.cpp` (see existing files for examples).
- Coverage: add unit tests for new behavior and regressions; prefer fast, deterministic tests.
- Run: enable `TV_BUILD_TESTS=ON` and build `tvision-test-run` (or execute `./build/tvision-test`).

## Commit & Pull Request Guidelines
- Commit messages: Conventional Commits style (`feat:`, `fix:`, `docs:`); emoji optional. Use present tense and clear scope (e.g., `feat(platform): add OSC52 clipboard`).
- PRs must include: concise description, rationale, build/test steps, and linked issues. Add screenshots/GIFs for TUI changes when helpful.
- Scope PRs narrowly; update docs/README sections if behavior or flags change.

## Security & Configuration Tips
- Linux build deps: `libncursesw` (and optionally `libgpm`). Runtime clipboard: `xsel/xclip` (X11) or `wl-clipboard` (Wayland).
- Windows (MSVC): set `/permissive- /Zc:__cplusplus`. Keep RTL linkage (`/MD` vs `/MT`) consistent across app and library.
