**Turbo Vision ASCII Image Viewer**

**Status**: PRD — POC and v2 roadmap

**Owners**: TUI/Graphics

**Summary**
- Build an image-to-ASCII renderer with an optional Turbo Vision `TView` front-end.
- POC ships as a single-file package to keep iteration fast and portable.
- v2 evolves into a reusable view and helpers with richer color, scaling, and UX.

**Background & Rationale**
- The repo focuses on ASCII/ANSI art and Turbo Vision apps. We have pattern and frame players but no direct way to visualize real images in the TUI.
- An image→ASCII viewer enables quick previews, demos, and educational examples for dithering, palette quantization, and glyph mapping.
- The POC must be easy to compile both inside and outside the repo (stdout fallback) to maximize adoption.

**Goals (POC)**
- Convert PNG/JPEG into a grid of cells: `(glyph, fg, bg)`.
- Provide an ordered-dithered luminance-to-glyph mapping and ANSI 16-color quantization.
- Render grid through Turbo Vision (`TDrawBuffer`) when available; otherwise dump to stdout.
- Single-file, minimal dependencies; image loader via `stb_image`.
- Controls in TV app: zoom (aspect tweak), toggle dither, cycle glyph ramps.

**Non-Goals (POC)**
- No video import, no animated formats.
- No palette editing UI, no complex error diffusion, no HDR/ICC handling.
- No changes to core `source/tvision` API.

**Users**
- Contributors prototyping ASCII effects in Turbo Vision.
- Artists wanting to preview artworks in terminal constraints.
- Educators demonstrating dithering and quantization.

**User Experience (POC)**
- Binary: `tv_ascii_view` (Turbo Vision build) or `ascii_dump` (stdout fallback).
- CLI: `./tv_ascii_view image.png [cols rows]` or `./ascii_dump image.png [cols rows]`.
- Keys: `+/-` zoom (aspect), `d` toggle dither, `g` cycle glyph ramp; help dialog on start.

**Functional Requirements (POC)**
- Load image as RGBA.
- Rasterize to `cols×rows` grid with aspect correction per text cell (non-square pixels).
- Ordered Bayer 4×4 dithering on luma before glyph selection (toggleable).
- Glyph ramps: block, Unicode, ASCII.
- ANSI 16-color quantization; black background by default.
- Turbo Vision rendering via `TDrawBuffer`; alternative stdout dump.

**Non-Functional Requirements (POC)**
- C++14 compatible; no changes to `include/tvision`.
- Build in `test-tui` via CMake; optionally compile standalone with `-DTVISION_AVAILABLE`.
- Deterministic output for given inputs, platform-agnostic within terminal color differences.

**Tech Overview (POC)**
- Single source: `test-tui/tvision_ascii_view_poc.cpp`.
- Dependency: `stb_image.h` (PNG/JPEG). If unavailable system-wide, place next to the file.
- Build targets:
  - `tv_ascii_view`: links `tvision`, defines `TVISION_AVAILABLE`.
  - `ascii_dump`: no `tvision` linkage; uses stdout rendering.

**Acceptance Criteria (POC)**
- Load and display a PNG/JPG at default 80×24 with visible glyph shading.
- Dithering toggle visibly changes output.
- Glyph ramp cycling updates display without crash.
- CLI fallback prints UTF-8 and ANSI colors without control-code leakage.

**Risks (POC)**
- Availability of `stb_image.h` in developer environments.
- Terminal/font differences affect perceived quality.

---

**V2 Enhancements (Roadmap)**

**Goals (v2)**
- Better color and quality while staying fast and portable.
- Reusable, testable components integrated with `test-tui` patterns and painting utilities.

**Key Features (v2)**
- 256-color and truecolor output paths with configurable palette policies.
- Error-diffusion dithering (Floyd–Steinberg, Sierra-2-4A) with clamp-safe edges.
- Adaptive glyph selection using a glyph atlas and per-glyph error metrics.
- Smart background color inference per cell or 2×1 blocks to reduce color bleeding.
- Fit/scale policies: fit width/height, preserve aspect by glyph cell metrics.
- Simple file dialog to open images from within Turbo Vision apps.
- Config siderbar/status line: current ramp, color mode, FPS for live resizing.

**Architecture (v2)**
- Extract modules:
  - `ascii_image_rasterizer.{h,cpp}`: core grid generation API.
  - `ascii_palette.{h,cpp}`: palette/quantization strategies (ANSI16, 256, truecolor).
  - `ascii_glyphs.{h,cpp}`: glyph ramps, atlas, metrics.
  - `ascii_image_view.{h,cpp}`: Turbo Vision `TView` wrapper with params/state.
- Keep CLI sample using the same library to preserve stdout path.

**Non-Functional (v2)**
- Benchmarks for 80×24 and 160×50 sizes (<16 ms/frame on modern CPUs).
- Unit tests for rasterizer determinism, palette mapping, and glyph selection.
- Documented extension points for plug-in ramps/palettes.

**Acceptance Criteria (v2)**
- Visual parity or improvement vs. POC on sample set (natural images, line art).
- 256-color path shows measurable SSIM/PSNR gains vs. ANSI16.
- Error diffusion does not introduce artifacts at borders; no crashes on odd sizes.
- Public headers under `include/tvision` remain untouched (internal-only components under `test-tui/`).

**Milestones**
- M1: Extract core into `ascii_image_rasterizer` + tests.
- M2: Add 256-color quantizer + benchmarks.
- M3: Error diffusion + clamp handling + tests.
- M4: Glyph atlas scoring + toggleable policies.
- M5: TV `TView` component + config UI.
- M6: Docs + examples + sample images.

**Open Questions**
- Truecolor path: keep as demo only or integrate with engine attributes?
- Unicode coverage: depend on terminal font; should we ship an ASCII-only default to avoid tofu?
- Optional dependency policy for `stb_image` within the repo (vendor vs. system include).

