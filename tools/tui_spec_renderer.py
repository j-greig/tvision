#!/usr/bin/env python3
# /// script
# requires-python = ">=3.11"
# dependencies = ["requests>=2.31.0"]
# ///
"""
TUI Spec Renderer — JSON spec → Turbo Vision windows.

Reads a JSON spec file describing a dashboard layout, then creates
windows in wibwob-dos via the REST API and injects content.

Supports two render modes:
  --mode=dashboard  → Uses native TDashboardView (colored progress bars,
                      styled tables, status indicators). Requires C++ build.
  --mode=text       → Uses text_editor windows (plain text). No C++ changes needed.

Usage:
    uv run tools/tui_spec_renderer.py specs/test_dashboard.json
    uv run tools/tui_spec_renderer.py specs/test_dashboard.json --mode=text
    uv run tools/tui_spec_renderer.py specs/test_dashboard.json --api http://localhost:8089

Requires: wibwob-dos TUI running + API server on :8089
"""

import argparse
import json
import sys
import time

import requests

DEFAULT_API = "http://localhost:8089"


# ─── Dashboard Content Builder ─────────────────────────────────
# Converts JSON element specs into the pipe-delimited format
# that TDashboardView::parseContent() expects.

def build_dashboard_content(elem: dict) -> str:
    """Convert a JSON element spec into dashboard content format."""
    elem_type = elem.get("type", "text")
    lines = []

    if elem_type == "heading":
        lines.append(f"LABEL|{elem.get('text', '')}")

    elif elem_type == "text":
        content = elem.get("content", "")
        for line in content.split("\n"):
            lines.append(f"LABEL|{line}")

    elif elem_type == "meter":
        lines.append(f"PROGRESS|{elem.get('label', '')}|{elem.get('value', 0)}|{elem.get('max', 100)}")

    elif elem_type == "table":
        headers = elem.get("headers", [])
        rows = elem.get("rows", [])
        lines.append("TABLE_HDR|" + "|".join(headers))
        for row in rows:
            lines.append("TABLE_ROW|" + "|".join(str(c) for c in row))

    elif elem_type == "status":
        lines.append(f"STATUS|{elem.get('label', '')}|{elem.get('state', 'idle')}")

    elif elem_type == "kv":
        lines.append(f"KV|{elem.get('key', '')}|{elem.get('value', '')}")

    elif elem_type == "panel":
        # Composite: a panel with multiple sub-items
        if "title" in elem:
            lines.append(f"LABEL|{elem['title']}")
            lines.append("SEPARATOR")
        for sub in elem.get("items", []):
            lines.append(build_dashboard_content(sub))

    return "\n".join(lines)


def build_composite_dashboard(elements: list[dict]) -> str:
    """Build a single dashboard content string from multiple elements.
    Used when rendering all elements into one dashboard window."""
    parts = []
    for elem in elements:
        parts.append(build_dashboard_content(elem))
        parts.append("SEPARATOR")
    return "\n".join(parts)


# ─── Text Mode Renderers (Fallback) ────────────────────────────

def render_text_heading(api: str, win_id: str, text: str, font: str = "slant"):
    """Inject FIGlet heading into text_editor window."""
    requests.post(f"{api}/windows/{win_id}/send_figlet", json={
        "text": text, "font": font, "mode": "replace",
    }).raise_for_status()


def render_text_content(api: str, win_id: str, content: str):
    """Inject plain text into text_editor window."""
    requests.post(f"{api}/windows/{win_id}/send_text", json={
        "content": content, "mode": "replace",
    }).raise_for_status()


def render_text_meter(api: str, win_id: str, label: str, value: int, max_val: int):
    """ASCII progress bar for text mode."""
    bar_width = 20
    filled = round((value / max_val) * bar_width) if max_val > 0 else 0
    empty = bar_width - filled
    pct = round((value / max_val) * 100) if max_val > 0 else 0
    bar = f" {label}\n\n [{chr(9608) * filled}{chr(9617) * empty}]\n\n {value}/{max_val}  ({pct}%)"
    render_text_content(api, win_id, bar)


def render_text_table(api: str, win_id: str, headers: list[str], rows: list[list[str]]):
    """Formatted table for text mode."""
    all_rows = [headers] + rows
    col_widths = []
    for col_idx in range(len(headers)):
        max_w = max(len(str(row[col_idx])) for row in all_rows if col_idx < len(row))
        col_widths.append(max_w + 2)

    def format_row(row):
        cells = []
        for i, cell in enumerate(row):
            w = col_widths[i] if i < len(col_widths) else 10
            cells.append(str(cell).ljust(w))
        return " ".join(cells)

    lines = [" " + format_row(headers)]
    lines.append(" " + "-" * sum(col_widths + [len(col_widths) - 1]))
    for row in rows:
        lines.append(" " + format_row(row))
    render_text_content(api, win_id, "\n".join(lines))


# ─── Layout Engine ──────────────────────────────────────────────

def compute_layout(spec: dict, canvas: dict) -> list[dict]:
    """Compute window positions from grid layout spec.

    Elements with an explicit "height" field get that many rows.
    Remaining vertical space is split equally among auto-height rows.
    """
    layout = spec.get("layout", {})
    cols = layout.get("cols", 2)
    rows = layout.get("rows", 2)
    gap = layout.get("gap", 1)
    origin_x = layout.get("origin", {}).get("x", 1)
    origin_y = layout.get("origin", {}).get("y", 1)

    avail_w = canvas.get("width", 80) - origin_x - 1
    avail_h = canvas.get("height", 25) - origin_y - 2  # leave room for status bar

    cell_w = max((avail_w - (cols - 1) * gap) // cols, 10)

    # First pass: assign elements to rows and find explicit heights
    elements = spec.get("elements", [])
    row_assignments = {}  # row_idx -> list of elements
    occupied = set()
    cursor_row, cursor_col = 0, 0
    elem_rows = []  # which row each element lands on

    for elem in elements:
        while (cursor_row, cursor_col) in occupied:
            cursor_col += 1
            if cursor_col >= cols:
                cursor_col = 0
                cursor_row += 1

        span_cols = 1
        span = elem.get("span")
        if span and len(span) >= 2:
            span_cols = span[1] - span[0] + 1

        for sc in range(span_cols):
            occupied.add((cursor_row, cursor_col + sc))

        elem_rows.append((cursor_row, cursor_col, span_cols))
        row_assignments.setdefault(cursor_row, []).append(elem)

        cursor_col += span_cols
        if cursor_col >= cols:
            cursor_col = 0
            cursor_row += 1

    # Compute row heights: explicit heights first, then split remainder
    num_rows = max(r for r, _, _ in elem_rows) + 1 if elem_rows else rows
    row_heights = [None] * num_rows
    fixed_total = 0

    for row_idx, elems_in_row in row_assignments.items():
        for e in elems_in_row:
            h = e.get("height")
            if h and row_idx < num_rows:
                row_heights[row_idx] = h
                fixed_total += h
                break

    auto_rows = sum(1 for h in row_heights if h is None)
    remaining = avail_h - fixed_total - (num_rows - 1) * gap
    auto_h = max(remaining // auto_rows, 4) if auto_rows > 0 else 4

    for i in range(num_rows):
        if row_heights[i] is None:
            row_heights[i] = auto_h

    # Compute cumulative Y positions per row
    row_y = [0] * num_rows
    row_y[0] = origin_y
    for i in range(1, num_rows):
        row_y[i] = row_y[i - 1] + row_heights[i - 1] + gap

    # Second pass: compute positions
    positions = []
    for idx, (row_idx, col_idx, span_cols) in enumerate(elem_rows):
        x = origin_x + col_idx * (cell_w + gap)
        y = row_y[row_idx]
        w = cell_w * span_cols + gap * (span_cols - 1)
        h = row_heights[row_idx]
        positions.append({"x": x, "y": y, "w": w, "h": h})

    return positions


# ─── Main Renderer ──────────────────────────────────────────────

def render_spec(spec: dict, api: str = DEFAULT_API, mode: str = "dashboard"):
    """Render a full JSON spec to TUI windows."""

    # 1. Get canvas size (with sensible fallback)
    state = requests.get(f"{api}/state").json()
    canvas = state.get("canvas", {"width": 80, "height": 25})
    # Fallback if TUI reports invalid size (e.g. running without terminal)
    if canvas.get("width", 0) < 10 or canvas.get("height", 0) < 10:
        canvas = {"width": 112, "height": 54}
        print(f"Canvas: using fallback {canvas['width']}x{canvas['height']}")
    else:
        print(f"Canvas: {canvas['width']}x{canvas['height']}")

    # 2. Clear existing windows
    requests.post(f"{api}/windows/close_all")
    time.sleep(0.3)

    # 3. Compute layout
    positions = compute_layout(spec, canvas)
    elements = spec.get("elements", [])
    print(f"Rendering {len(elements)} elements in {mode} mode...")

    # 4. Create windows and inject content
    window_content = {}  # win_id → content, for second-pass refresh
    for idx, elem in enumerate(elements):
        if idx >= len(positions):
            break

        pos = positions[idx]
        elem_type = elem.get("type", "text")
        title = elem.get("title", elem.get("label", elem_type))

        # Choose window type based on render mode
        win_type = "dashboard" if mode == "dashboard" else "text_editor"

        resp = requests.post(f"{api}/windows", json={
            "type": win_type,
            "title": title,
            "rect": pos,
        })
        resp.raise_for_status()
        win = resp.json()
        win_id = win["id"]
        time.sleep(0.15)

        try:
            if mode == "dashboard":
                # Build structured content for TDashboardView
                content = build_dashboard_content(elem)
                window_content[win_id] = content
                # Retry once on failure (IPC can be briefly busy)
                for attempt in range(2):
                    try:
                        requests.post(f"{api}/windows/{win_id}/send_text", json={
                            "content": content,
                            "mode": "replace",
                        }).raise_for_status()
                        break
                    except requests.exceptions.RequestException:
                        if attempt == 0:
                            time.sleep(0.3)
                        else:
                            raise
            else:
                # Text mode fallback
                if elem_type == "heading":
                    render_text_heading(api, win_id, elem.get("text", ""), elem.get("font", "slant"))
                elif elem_type == "text":
                    render_text_content(api, win_id, elem.get("content", ""))
                elif elem_type == "meter":
                    render_text_meter(api, win_id, elem.get("label", ""), elem.get("value", 0), elem.get("max", 100))
                elif elem_type == "table":
                    render_text_table(api, win_id, elem.get("headers", []), elem.get("rows", []))
                else:
                    render_text_content(api, win_id, f"Unknown type: {elem_type}")

            print(f"  [{idx+1}/{len(elements)}] {elem_type}: {title} at ({pos['x']},{pos['y']} {pos['w']}x{pos['h']})")
        except requests.exceptions.RequestException as e:
            print(f"  [{idx+1}/{len(elements)}] {elem_type}: FAILED - {e}", file=sys.stderr)

    # 5. Second pass: re-send content to all dashboard windows
    #    Fixes intermittent empty renders caused by drawView() racing
    #    with window initialization on the IPC thread.
    if mode == "dashboard" and window_content:
        time.sleep(0.3)
        print("Confirming content (pass 2)...")
        for win_id, content in window_content.items():
            try:
                requests.post(f"{api}/windows/{win_id}/send_text", json={
                    "content": content,
                    "mode": "replace",
                }).raise_for_status()
            except requests.exceptions.RequestException:
                pass  # Best effort
        print(f"  {len(window_content)} windows refreshed.")

    print(f"\nDone. {len(elements)} elements rendered.")


def main():
    parser = argparse.ArgumentParser(description="Render JSON spec to TUI windows")
    parser.add_argument("spec_file", help="Path to JSON spec file")
    parser.add_argument("--api", default=DEFAULT_API, help=f"API base URL (default: {DEFAULT_API})")
    parser.add_argument("--mode", choices=["dashboard", "text"], default="dashboard",
                        help="Render mode: dashboard (native widgets) or text (text_editor fallback)")
    args = parser.parse_args()

    with open(args.spec_file) as f:
        spec = json.load(f)

    print(f"Spec: {spec.get('title', 'untitled')} ({len(spec.get('elements', []))} elements)")

    try:
        health = requests.get(f"{args.api}/health", timeout=2).json()
        if not health.get("ok"):
            print("API server not healthy", file=sys.stderr)
            sys.exit(1)
    except requests.exceptions.ConnectionError:
        print(f"Cannot connect to API at {args.api}", file=sys.stderr)
        sys.exit(1)

    render_spec(spec, args.api, args.mode)


if __name__ == "__main__":
    main()
