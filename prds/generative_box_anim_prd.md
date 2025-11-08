# TL;DR — Prompt For The System We Want

Generate a single text file in `test-tui/primers/` that contains a 40×40, monospaced, ASCII/Unicode animation made only from box‑drawing characters (─ │ ┌ ┐ └ ┘ ├ ┤ ┬ ┴ ┼, rounded corners ╭ ╮ ╰ ╯, etc.). The file uses our existing frame format: optional `FPS=NN` header, then frames separated by `----`. The animation depicts generative, concentric, wibbly shapes that grow and shrink, sometimes clustering into archipelagos (islands), sometimes isolated. Shapes can spawn and die; as they shrink, inner “wobble” contours appear. The algorithm should be deterministic with a seed, yet lively with noise. Provide a Python generator (no heavy deps; `random`, optional `noise` library OK) that writes the file. Rasterization must choose the correct box‑drawing glyph at each (x,y) by looking at 4/8‑neighbor connectivity so contours look like proper lines and rounded corners.

---

# Generative Box‑Line Animation — PRD

## Goals
- Produce a self‑contained animation file (one file, many frames) consumable by our existing player.
- 40×40 cell grid; monospaced coordinates (x,y in character cells).
- Visual language: box‑drawing line glyphs only; rounded corners for organic feel.
- Generative behavior: shapes spawn, grow, wobble, shrink, split or die; clusters form and disperse.
- Deterministic with seed; parameterizable (FPS, length, density, noise amplitude).

## Non‑Goals
- No color, gradients, or non‑line glyphs in MVP.
- No interactive editing; this is an offline generator emitting a text file.

## File Format (compatible with our player)
- First line optional: `FPS=12` (or desired FPS).
- Then zero or more empty/comment lines allowed.
- Frames separated by a line exactly equal to `----`.
- Each frame is 40 lines of 40 characters (pad with spaces). No trailing CRs (just `\n`).

## Grid & Charset
- Size: `W=40`, `H=40` (exposed as flags to the Python script).
- Glyphs:
  - Lines: `─ │` diagonals via connectivity, intersections `┼ ┬ ┴ ├ ┤`.
  - Rounded corners: `╭ ╮ ╰ ╯` preferred where curvature implies.
  - Space for background.
- Mapping rule: For each cell set to “on”, inspect N/E/S/W (and optionally diagonals) to pick the best codepoint from a lookup (bitmask → glyph).

## Generative Model (MVP)
Entities: Shape instances with state
- Properties: `center(x,y)`, `base_radius`, `thickness`, `phase`, `age`, `max_age`, `polarity (grow/shrink)`.
- Wobble: radius offset `dr = wobble_amp * noise2D(x_norm, y_norm, t) + ripple_amp * sin(k*theta + ω*t + phase)`.
- Concentric: optionally emit multiple contours for a shape: radii `r0, r1, r2…` decreasing by `thickness`.

Spawner & Field
- Time‐varying density scalar field `D(x,y,t)` from low‑frequency Perlin/Simplex noise; sample to decide spawn probability.
- Poisson‑ish restraint: reject spawn if a new center is within `min_dist` of a living shape’s center.
- Death: shapes fade when `age > max_age` or radius < min.

Update per frame
1) Advance time `t`.
2) With probability `p_spawn(t)`, try to spawn shapes per field `D`.
3) For each shape:
   - Update radius: `r += grow_rate * polarity`; flip polarity upon bounds.
   - Update phase/age; if shrinking past `min_r`, mark for death.
   - Rasterize one or more contours on a temporary boolean buffer.
4) Convert boolean buffer to glyphs via connectivity mapping; write 40 lines.

## Rasterization
- For a contour at desired radius `r(theta)` around center `C`, sample `theta` at fixed steps (e.g., 2–4°) and mark nearest grid cells as “on”.
- To avoid gaps, draw Bresenham between successive samples.
- After all shapes mark the boolean buffer, run glyph mapping:
  - Use a 4‑bit mask (N,E,S,W present) to choose from `─ │ ┌ ┐ └ ┘ ├ ┤ ┬ ┴ ┼`.
  - If diagonal transitions imply a rounded corner, prefer `╭ ╮ ╰ ╯` (heuristic: if only NE or NW is on in local neighborhood, pick rounded).
- Optional post step: thin/reconnect with a simple morphological pass to reduce isolated pixels.

## Noise & Randomness
- Seeded `random.Random(seed)` for reproducibility.
- Optional: `noise` (Perlin/Simplex) for `D(x,y,t)` and wobble (fallback to fBm from sin/cos if `noise` unavailable).

## Parameters (suggested CLI flags)
- `--out test-tui/primers/wibbly_islands.txt`
- `--cols 40 --rows 40`
- `--frames 480 --fps 12`
- `--seed 12345`
- `--spawn-rate 0.05`
- `--min-r 2 --max-r 14 --thickness 1`
- `--wobble-amp 1.2 --ripple-amp 0.8 --ripple-k 3 --omega 0.07`
- `--min-dist 5` (Poissonish spawn distance)

## Python Implementation Plan (tools/gen_wibbly.py)
- Dependencies: stdlib only; optionally import `noise` if present.
- Structure:
  - Args parse; seed RNG.
  - Shape class with update/rasterize.
  - Noise helpers (fallback sine mixes if no `noise` pkg).
  - Boolean grid `on[y][x]`; write glyph grid from mask LUT.
  - Writer: emit `FPS=NN` header, then `frame`, then `----`, etc.
- Performance: 40×40×480 is trivial; pure Python OK.

## Integration With App
- Place output under `test-tui/primers/…`.
- Open via: File → Open Text/Animation… (the player will detect `----` and animate).
- FPS honored via header (already supported by our player).

## Testing
- Visual:
  - Ensure no glyphs overflow 40×40; borders padded with spaces.
  - Anim varies: clusters appear/disappear; inner rings wobble when shrinking.
- Determinism: same seed → same file checksum.
- Performance: generation under ~1s for a few hundred frames on a dev machine.

## Risks & Mitigations
- Glyph mapping artifacts: round corners might not always match connectivity → keep a clear N/E/S/W mask first, add rounded heuristic second.
- Noise availability: if `noise` missing, fallback keeps motion lively but less organic.
- Overdraw flicker: use single boolean buffer per frame and draw lines consistently (Bresenham between samples).

## Next Steps
1) Scaffold `tools/gen_wibbly.py` with full CLI + seed.
2) Implement mask→glyph LUT and contour sampling with Bresenham.
3) Add optional `--preview N` to generate a small number of frames for quick iteration.
4) Hook a Makefile target or README snippet to regenerate the primer.

