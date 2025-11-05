#!/usr/bin/env python3
"""
Monodraw Parser CLI — Standalone tool for parsing Monodraw JSON files.

Usage:
    monodraw_parser_cli.py <file.monojson>

Output:
    JSON to stdout with structure:
    {
        "ok": true,
        "layers": [
            {"name": "...", "origin_x": 10, "origin_y": 5, "width": 40, "height": 20, "text_content": "...", "object_id": "..."},
            ...
        ],
        "canvas_bounds": {"width": 250, "height": 120}
    }

Exit codes:
    0 - Success
    1 - File not found or parse error
    2 - Invalid arguments
"""

import sys
import json
import os

# Add parent directory to path to import monodraw_parser module
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'api_server'))

try:
    from monodraw_parser import MonodrawParser
except ImportError:
    # Fallback: try direct import if running from api_server directory
    try:
        from tools.api_server.monodraw_parser import MonodrawParser
    except ImportError:
        print(json.dumps({
            "ok": False,
            "error": "Failed to import MonodrawParser module"
        }), file=sys.stderr)
        sys.exit(1)


def main():
    if len(sys.argv) != 2:
        print(json.dumps({
            "ok": False,
            "error": "Usage: monodraw_parser_cli.py <file.monojson>"
        }), file=sys.stderr)
        sys.exit(2)

    file_path = sys.argv[1]

    # Check file exists
    if not os.path.exists(file_path):
        print(json.dumps({
            "ok": False,
            "error": f"File not found: {file_path}"
        }), file=sys.stderr)
        sys.exit(1)

    try:
        # Parse Monodraw file
        layers = MonodrawParser.parse_file(file_path)
        canvas_w, canvas_h = MonodrawParser.get_canvas_bounds(layers)

        # Convert to JSON-serializable format
        output = {
            "ok": True,
            "layers": [
                {
                    "name": layer.name,
                    "origin_x": layer.origin[0],
                    "origin_y": layer.origin[1],
                    "width": layer.frame_size[0],
                    "height": layer.frame_size[1],
                    "text_content": layer.text_content,
                    "object_id": layer.object_id
                }
                for layer in layers
            ],
            "canvas_bounds": {
                "width": canvas_w,
                "height": canvas_h
            }
        }

        # Output JSON to stdout
        print(json.dumps(output, ensure_ascii=False, indent=2))
        sys.exit(0)

    except Exception as e:
        print(json.dumps({
            "ok": False,
            "error": f"Parse error: {str(e)}"
        }), file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
