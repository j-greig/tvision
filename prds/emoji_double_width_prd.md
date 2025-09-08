# Emoji Double-Width in Text Views — PRD

## Problem
- Opening text files with emoji renders misaligned columns: many terminals render emoji grapheme clusters 2 columns wide, but our width logic is codepoint-based (wcwidth), not grapheme-aware.
- Effects: wrapped lines break early/late; cursor hit-testing is off; horizontal scrolling and selection drift around emoji.

## Goals
- Display and navigate text such that screen columns match terminal rendering when emoji are present.
- Preserve performance and backwards compatibility for non-emoji content.
- No heavy external runtime deps by default; keep portability (macOS/Linux/Windows/WSL).

## Non‑Goals
- Bitmap/image emoji; color emoji rendering.
- Full Unicode compliance for all grapheme rules across every version (we target the high‑value emoji cases).

## Background
- Today: `TText::next/width` measure a single Unicode scalar and call `Platform::charWidth` (wcwidth/WinWidth). This is not grapheme‑aware and undercounts multi‑codepoint emoji (e.g., skin tone modifiers, ZWJ sequences, flags).
- Terminals commonly render:
  - Extended pictographic/emoji grapheme clusters: width = 2
  - Combining marks: width = 0 (attach to base)
  - Regional indicator pairs (flags): width = 2 as a single cluster

## Options (ranked)
1) Grapheme‑aware width in TText (recommended)
   - Add a lightweight grapheme stepper to `TText` that detects clusters we care about:
     - Base emoji (Extended_Pictographic) plus optional VS‑16 (U+FE0F), modifiers (skin tones), sequences joined by ZWJ (U+200D), flag pairs (Regional_Indicator x2), keycap sequences.
   - Treat such clusters as width 2; treat combining marks as width 0.
   - Update `TText::nextImpl` to step a cluster and return {bytes, width}.
   - Update `drawOne` to emit all codepoints of a cluster into consecutive cells but only advance the visible column count by the cluster width (2). For columns beyond cluster width, set char to spaces and attributes to match (so buffers remain rectangular).
   - Pros: Good fidelity for common emoji; no external deps; contained change (TText + callers).
   - Cons: Incomplete vs full UAX#29; ongoing table updates (emoji ranges).

2) Heuristic double‑width by codepoint range (fallback)
   - If first codepoint ∈ emoji ranges (e.g., U+1F1E6..U+1FAFF) or has Emoji_Presentation, return width 2; ignore sequences.
   - Pros: Tiny and fast.
   - Cons: Wrong for ZWJ/flags/modifiers; still better than 1‑cell emoji.

3) Integrate utf8proc/ICU (feature‑flag)
   - Use libutf8proc (MIT) or ICU to segment graphemes and query properties.
   - Pros: Accurate and future‑proof.
   - Cons: New dependency, footprint, packaging complexity (esp. Windows).

4) App‑level fallback (not preferred)
   - Preprocess loaded text to replace emoji with 2‑cell placeholders.
   - Pros: No core changes.
   - Cons: Loses actual emoji, surprises users, non‑portable across viewers.

## Proposed Design (Option 1 + 2 as fallback)
- Add internal module: `tvision/internal/grapheme.h/.cpp` (no public API).
  - `struct Cluster { size_t byteLen; int width; };`
  - `Cluster nextCluster(TStringView s)` implements a small DFA:
    - Decode first scalar (existing UTF‑8 DFA).
    - Accumulate: VS‑16; Emoji_Modifier; ZWJ + next Extended_Pictographic; Regional_Indicator pairs; keycap (U+0023/U+002A/U+0030..U+0039 + VS‑16 + U+20E3).
    - width = 2 if cluster has Extended_Pictographic or RI pair; else width from wcwidth of base (with 0 for combining).
- Wire into `TText`:
  - Change `TText::nextImpl(TStringView)` to call `nextCluster` (behind feature flag `TV_TEXT_GRAPHEME_AWARE` default ON).
  - Keep existing `TText::nextImpl(TSpan<const uint32_t>)` for legacy paths.
  - Ensure `TText::drawOneImpl` repeats writing within available cells and advances by returned width (pad any extra to keep grid consistent).
- Configuration
  - Env/option: `TV_EMOJI_WIDTH={auto,force2,off}`
    - auto: treat Extended_Pictographic/RI clusters as width 2 (default).
    - force2: treat any codepoint with Emoji property (even alone) as width 2.
    - off: fall back to wcwidth only.

## Affected Areas
- Measuring: wrapping, scrolling, selection, cursor movement in `TTextFileView`, `TEditor`, status hints.
- Drawing: `TText::draw*` paths; screen buffer widths.
- Tests: add platform‑agnostic unit tests under `test/platform/ttext.test.cpp` covering:
  - 🙂, 👍🏽, 👨‍👩‍👧‍👦, 🇺🇸, á (combining), keycaps (#️⃣), ZWJ chains.

## Risks & Mitigations
- Terminal variance (some treat emoji as width 1): provide runtime toggle and app setting.
- Performance: cluster stepping adds overhead; mitigate with simple state machine and early exits; only run on UTF‑8 builds (default).
- Data drift: emoji ranges evolve; keep minimal table for Extended_Pictographic + RI; document update path.

## Rollout Plan
1. Implement core cluster stepper + width in `internal/grapheme.*`.
2. Gate via `TV_TEXT_GRAPHEME_AWARE` (default ON); add env override.
3. Update `TText::*` and verify `test-tui` text rendering, wrapping, and cursoring.
4. Add unit tests; document setting in README “Unicode support”.

## Open Questions
- Should we expose a public hook to override width policy per app?
- Do we need per-terminal heuristics (e.g., respect `wt`, `iTerm2`, `xterm`) for ‘auto’?
- Should we extend to general UAX#29 grapheme boundaries later (beyond emoji)?

## Current Findings (MVP Results)
- Improvements: flags (RI pairs), keycaps, and many pictographs now align better (width=2).
- Regressions observed: some sequences like U+1F441 U+FE0F (👁️) still show artifacts (e.g., stray ASCII like `15_`), or appear width=1 on some lines.

Hypotheses:
- Terminal/font variance: with Unifont (monochrome), some emoji rely on Variation Selector-16 for emoji-presentational glyphs. If the font lacks a dedicated emoji glyph, the terminal may fall back to a text glyph (width=1) even if we treat it as width=2.
- Cell payload limits: `TCellChar` stores up to 15 bytes total. Appending VS16 + modifiers may exceed limits in rare chains; truncated bytes could leak into subsequent cells if not fully consumed.
- Cluster handling order: writing base first and then appending VS16 may not always produce the intended glyph selection in some terminals; some terminals expect the exact byte order and may ignore appended zero-width if we mark the cell as wide.

Constraints in Turbo Vision:
- A cell is a grid unit with a text payload (<=15 bytes) and width metadata (1 or 2 columns) implemented via a wide-char trail cell.
- Rendering uses UTF-8 text written through the console adapter (ncurses/Win32), not shaping engines; terminals do grapheme/wcwidth decisions.

## Research Plan (Methodical)
1) Repro Harness
   - Add a developer toggle: `TV_EMOJI_DEBUG=1` to overlay per-glyph diagnostics under the cursor (codepoints, bytes, cluster length, reported width).
   - Add a micro-sample file `primers/emoji_matrix.txt` with rows for: base emoji, emoji+VS16, emoji+modifier, RI flags, ZWJ family, keycaps, combining marks.

2) Instrumentation
   - Log (to stderr or a side panel) for each drawn grapheme: `[bytes=N width=W hex=F0 9F ...]`.
   - Confirm no residual bytes write into subsequent cells when cluster length > 4.

3) Terminal/Font Matrix
   - Terminals: Apple Terminal, iTerm2, Kitty, Alacritty, VS Code integrated terminal, xterm.
   - Fonts: Unifont (monochrome), Apple Color Emoji, Noto Color Emoji.
   - Measure: Does 👁️ render width 1 or 2? Does VS16 change presentation? Does `wcwidth` (or platform adapter) agree?

4) Policy Refinement
   - Auto policy: detect terminals that follow Unicode 9 emoji width rules (heuristics via env: `VTE_VERSION`, `TERM_PROGRAM`, `WT_SESSION`) and default emoji clusters to width 2; otherwise default to width 1 for ambiguous emoji unless VS16 present.
   - VS16 handling: consume VS16 always; append to the same cell only if total bytes <= 15. Otherwise, consume without appending to avoid spill.
   - ZWJ chains: consume full cluster; draw base + VS16/modifier only; skip writing post‑ZWJ bases (avoid inconsistent shaping across terminals).

5) Fallback Modes
   - `TV_EMOJI_WIDTH=off`: revert to wcwidth only.
   - `TV_EMOJI_WIDTH=force2`: force width 2 for Extended_Pictographic + VS16 regardless of terminal.
   - App-level override hook: allow the application to supply a custom width callback.

6) Validation
   - Visual: the `|` bars in paired lines remain vertically aligned.
   - Cursor: left/right moves skip 2 columns over emoji clusters; selection does not split clusters.
   - No stray bytes: no occurrences of spurious ASCII around emoji.

## Why Unifont Matters Here
- Unifont provides monochrome coverage and typically maps emoji to BW glyphs. Some terminals still treat these emoji as ambiguous/width=2; others render them as width=1 unless VS16 forces emoji presentation.
- Our code must be conservative: do not assume color emoji behaviors; prefer consuming control bytes (VS16/ZWJ) and only append when safe. Default to terminal-friendly presentation to avoid showing raw UTF‑8 tail bytes.

## Next Steps (Engineering Tasks)
- Add `TV_EMOJI_DEBUG` diagnostics overlay (cursor-infos).
- Add `primers/emoji_matrix.txt` with curated samples.
- Implement env-driven policy switch (`TV_EMOJI_WIDTH=off|auto|force2`).
- Harden cell packing: strict bound checks; never append beyond per-cell budget; consume remainder safely.
- Add unit tests for 👁️ + VS16 and ensure no extra bytes render.
