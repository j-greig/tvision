#!/usr/bin/env python3
"""
MP Step 1: Render a single 40x40 (default) box-line frame to a file/stdout.

- Uses simple generative concentric wobbly contours (no animation yet)
- Emits exactly ROWS lines of COLS characters (spaces padded)
"""

import argparse
import math
import random
import sys
from typing import List


def bresenham_line(x0, y0, x1, y1, mark):
    dx = abs(x1 - x0)
    dy = -abs(y1 - y0)
    sx = 1 if x0 < x1 else -1
    sy = 1 if y0 < y1 else -1
    err = dx + dy
    x, y = x0, y0
    while True:
        mark(x, y)
        if x == x1 and y == y1:
            break
        e2 = 2 * err
        if e2 >= dy:
            err += dy
            x += sx
        if e2 <= dx:
            err += dx
            y += sy


def mask_to_glyph(n, e, s, w, rounded=True):
    """Map cardinal connectivity to box-drawing glyph."""
    m = (1 if n else 0) | (2 if e else 0) | (4 if s else 0) | (8 if w else 0)
    if m == 0:
        return ' '
    if m in (1, 4):  # N or S only
        return '│'
    if m in (2, 8):  # E or W only
        return '─'
    if m == 5:  # N+S
        return '│'
    if m == 10:  # E+W
        return '─'
    # Two-way corners
    if rounded:
        if m == (4 | 2):  # S+E
            return '╭'
        if m == (4 | 8):  # S+W
            return '╮'
        if m == (1 | 2):  # N+E
            return '╰'
        if m == (1 | 8):  # N+W
            return '╯'
    else:
        if m == (4 | 2):
            return '┌'
        if m == (4 | 8):
            return '┐'
        if m == (1 | 2):
            return '└'
        if m == (1 | 8):
            return '┘'
    # T-junctions & crossings
    if m == (1 | 2 | 4):
        return '├'  # N+E+S
    if m == (1 | 4 | 8):
        return '┤'  # N+S+W
    if m == (2 | 8 | 1):
        return '┴'  # N+E+W
    if m == (2 | 8 | 4):
        return '┬'  # S+E+W
    if m == (1 | 2 | 4 | 8):
        return '┼'
    # Fallback
    return '┼'


def build_frame(cols: int, rows: int, seed: int) -> List[str]:
    rnd = random.Random(seed)

    # Boolean grid for edge occupancy
    on = [[False for _ in range(cols)] for _ in range(rows)]

    # Choose 2-4 shapes
    shape_count = rnd.randint(2, 4)
    shapes = []
    for _ in range(shape_count):
        cx = rnd.randint(cols // 4, 3 * cols // 4)
        cy = rnd.randint(rows // 4, 3 * rows // 4)
        base_r = rnd.uniform(min(cols, rows) * 0.12, min(cols, rows) * 0.35)
        wobble_amp = rnd.uniform(0.6, 1.6)
        ripple_amp = rnd.uniform(0.3, 1.0)
        ripple_k = rnd.choice([2, 3, 4])
        phase = rnd.uniform(0, math.tau)
        thickness = rnd.choice([1.0, 1.5])
        rings = rnd.choice([1, 2, 3])
        shapes.append((cx, cy, base_r, wobble_amp, ripple_amp, ripple_k, phase, thickness, rings))

    def mark(x, y):
        if 0 <= x < cols and 0 <= y < rows:
            on[y][x] = True

    # Rasterize shapes
    for (cx, cy, base_r, wobble_amp, ripple_amp, ripple_k, phase, thickness, rings) in shapes:
        for ring in range(rings):
            r0 = base_r - ring * (thickness + 1.5)
            if r0 < 1.5:
                continue
            prev = None
            steps = max(36, int(2 * math.pi * r0))  # more points for larger radii
            for i in range(steps + 1):
                th = (i / steps) * 2 * math.pi
                # Wobble radius
                dr = wobble_amp * math.sin(3 * th + phase) + ripple_amp * math.sin(ripple_k * th + phase * 0.7)
                r = max(1.0, r0 + dr)
                x = int(round(cx + r * math.cos(th)))
                y = int(round(cy + r * math.sin(th)))
                if prev is None:
                    prev = (x, y)
                    mark(x, y)
                else:
                    x0, y0 = prev
                    bresenham_line(x0, y0, x, y, mark)
                    prev = (x, y)

    # Convert to glyphs
    out_rows: List[str] = []
    for y in range(rows):
        line_chars = []
        for x in range(cols):
            if not on[y][x]:
                line_chars.append(' ')
                continue
            n = on[y - 1][x] if y - 1 >= 0 else False
            s = on[y + 1][x] if y + 1 < rows else False
            w = on[y][x - 1] if x - 1 >= 0 else False
            e = on[y][x + 1] if x + 1 < cols else False
            g = mask_to_glyph(n, e, s, w, rounded=True)
            line_chars.append(g)
        # Ensure exact width
        out_rows.append(''.join(line_chars)[:cols].ljust(cols))
    return out_rows


def main():
    ap = argparse.ArgumentParser(description="Generate a single 40x40 box-line frame")
    ap.add_argument("--cols", type=int, default=40)
    ap.add_argument("--rows", type=int, default=40)
    ap.add_argument("--seed", type=int, default=12345)
    ap.add_argument("--out", type=str, default="-")
    args = ap.parse_args()

    frame = build_frame(args.cols, args.rows, args.seed)
    data = "\n".join(frame) + "\n"
    if args.out == "-":
        sys.stdout.write(data)
    else:
        with open(args.out, "w", encoding="utf-8") as f:
            f.write(data)


if __name__ == "__main__":
    main()

