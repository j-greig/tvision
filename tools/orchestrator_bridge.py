#!/usr/bin/env python3
"""Bridge: llm-orchestrator --jsonl → TUI conversation window.

Launches llm-orchestrator as a subprocess with --jsonl, reads JSONL events
from stdout, and routes all conversation messages to a single centered
TUI text editor window via the wibwob-dos HTTP API.

Layout on boot:
  - Single conversation window: centered, 65% width, full height
  - ASCII art primer window alongside for decoration

Usage:
    uv run tools/orchestrator_bridge.py \
        --config examples/orchestrator/conversation.toml \
        --actors examples/orchestrator/actors/ \
        --api http://127.0.0.1:8089
"""

import argparse
import json
import random
import subprocess
import sys
from pathlib import Path

import requests

DEFAULT_API = "http://127.0.0.1:8089"
ORCHESTRATOR_BIN = "llm-orchestrator"

# Primer files (~15-25 lines, good for sidebar). Relative to repo root.
PRIMER_CANDIDATES = [
    "app/primers/wibwob-ascii-process.txt",
    "app/primers/wibwob-portrait-1.txt",
    "app/primers/wibwob-portrait-2.txt",
    "app/primers/cat-in-space.txt",
    "app/primers/cat-rainbow-factory.txt",
    "app/primers/wibbwob-dual-portrait.txt",
]


class TUIBridge:
    """Routes orchestrator JSONL events to a single TUI conversation window."""

    def __init__(self, api_base: str):
        self.api = api_base.rstrip("/")
        self.conv_window_id: str | None = None
        self.canvas_w = 160
        self.canvas_h = 40
        self.first_message = True

    def _get(self, path: str) -> dict | None:
        try:
            r = requests.get(f"{self.api}{path}", timeout=5)
            r.raise_for_status()
            return r.json()
        except requests.RequestException as e:
            print(f"[bridge] API error: {e}", file=sys.stderr)
            return None

    def _post(self, path: str, payload: dict | None = None) -> dict | None:
        try:
            r = requests.post(f"{self.api}{path}", json=payload or {}, timeout=5)
            r.raise_for_status()
            return r.json()
        except requests.RequestException as e:
            print(f"[bridge] API error: {e}", file=sys.stderr)
            return None

    def get_canvas_size(self):
        """Fetch terminal dimensions from the TUI app."""
        state = self._get("/state")
        if state and "canvas" in state:
            self.canvas_w = state["canvas"].get("width", 160)
            self.canvas_h = state["canvas"].get("height", 40)
        print(f"[bridge] canvas: {self.canvas_w}x{self.canvas_h}", file=sys.stderr)

    def setup_layout(self):
        """Create the initial window layout: conversation + art primer."""
        self.get_canvas_size()

        # Conversation window: 65% width, full height, centered
        conv_w = max(int(self.canvas_w * 0.65), 50)
        conv_h = self.canvas_h - 2  # leave room for menu/status bars
        conv_x = (self.canvas_w - conv_w) // 2
        conv_y = 1

        # Art primer: left side, ~20 lines tall
        primer_path = self._pick_primer()
        if primer_path:
            primer_w = conv_x - 1
            primer_h = min(22, conv_h)
            if primer_w >= 15:
                self._post("/windows", {
                    "type": "text_view",
                    "title": Path(primer_path).stem,
                    "rect": {"x": 0, "y": 1, "w": primer_w, "h": primer_h},
                    "props": {"path": primer_path},
                })
                print(f"[bridge] primer: {Path(primer_path).name} ({primer_w}x{primer_h})", file=sys.stderr)

        # Create conversation window (after primer so it gets focus)
        resp = self._post("/windows", {
            "type": "text_editor",
            "title": "Wib & Wob",
            "rect": {"x": conv_x, "y": conv_y, "w": conv_w, "h": conv_h},
            "props": {"word_wrap": True},
        })
        if resp and "id" in resp:
            self.conv_window_id = resp["id"]
            print(f"[bridge] conversation window: {self.conv_window_id} "
                  f"({conv_w}x{conv_h} at {conv_x},{conv_y})", file=sys.stderr)

        # Send intro
        self.send_text(
            "Wib & Wob are thinking...\n"
            "\u1d55( \u141b )\u1d55\n\n"
        )

    def _pick_primer(self) -> str | None:
        """Pick a random primer file that exists."""
        candidates = [p for p in PRIMER_CANDIDATES if Path(p).exists()]
        return random.choice(candidates) if candidates else None

    def send_text(self, text: str, mode: str = "append"):
        """Send text to the conversation window."""
        if not self.conv_window_id:
            return
        self._post(f"/windows/{self.conv_window_id}/send_text", {
            "content": text,
            "mode": mode,
        })

    def handle_event(self, event: dict):
        """Route a single JSONL event to the conversation window."""
        etype = event.get("type", "")
        speaker = event.get("speaker_name", "")

        if etype == "message":
            content = event.get("content", "")
            # Clear intro on first real message
            if self.first_message:
                self.send_text(f"[{speaker}] {content}\n", mode="replace")
                self.first_message = False
            else:
                self.send_text(f"\n\n[{speaker}] {content}\n")
            print(f"[{speaker}] {content[:80]}{'...' if len(content) > 80 else ''}")

        elif etype == "feel_result":
            emotion = event.get("dominant_emotion", "?")
            intensity = event.get("intensity", 0)
            self.send_text(f"  [{speaker} feels {emotion} {intensity:.0%}]\n")

        elif etype == "conversation_ended":
            self.send_text("\n--- conversation ended ---\n")
            print("[bridge] conversation ended", file=sys.stderr)

        elif etype == "actor_exited":
            reason = event.get("reason", "")
            self.send_text(f"  [{speaker} exited: {reason}]\n")

        elif etype == "turn_started":
            pass  # Don't clutter with turn markers

        elif etype == "system_notice":
            level = event.get("level", "info")
            message = event.get("message", "")
            self.send_text(f"  [{level}] {message}\n")
            print(f"  [{level}] {message}", file=sys.stderr)


def run_bridge(config: Path, actors: Path, api: str, orchestrator: str, extra_args: list[str]):
    bridge = TUIBridge(api)
    bridge.setup_layout()

    cmd = [orchestrator, "--jsonl", "--config", str(config), "--actors", str(actors)] + extra_args
    print(f"[bridge] launching: {' '.join(cmd)}", file=sys.stderr)

    proc = subprocess.Popen(
        cmd,
        stdin=subprocess.PIPE,   # keep stdin open (prevent EOF exit)
        stdout=subprocess.PIPE,
        stderr=sys.stderr,
        text=True,
        bufsize=1,  # line-buffered
    )

    try:
        for line in proc.stdout:
            line = line.strip()
            if not line:
                continue
            try:
                event = json.loads(line)
            except json.JSONDecodeError:
                print(f"[bridge] non-JSON line: {line}", file=sys.stderr)
                continue
            bridge.handle_event(event)
    except KeyboardInterrupt:
        print("\n[bridge] interrupted, stopping orchestrator...", file=sys.stderr)
        proc.terminate()
    finally:
        proc.wait()
        print(f"[bridge] orchestrator exited with code {proc.returncode}", file=sys.stderr)


def main():
    parser = argparse.ArgumentParser(
        description="Bridge llm-orchestrator conversations to TUI windows",
    )
    parser.add_argument("--config", "-c", type=Path, required=True,
                        help="Path to conversation TOML config")
    parser.add_argument("--actors", "-a", type=Path, required=True,
                        help="Path to actors directory")
    parser.add_argument("--api", default=DEFAULT_API,
                        help=f"TUI API base URL (default: {DEFAULT_API})")
    parser.add_argument("--orchestrator", default=ORCHESTRATOR_BIN,
                        help=f"Path to llm-orchestrator binary (default: {ORCHESTRATOR_BIN})")
    args, extra = parser.parse_known_args()

    # Verify API is reachable
    try:
        r = requests.get(f"{args.api}/state", timeout=3)
        r.raise_for_status()
        print(f"[bridge] connected to TUI API at {args.api}", file=sys.stderr)
    except requests.RequestException as e:
        print(f"[bridge] ERROR: cannot reach TUI API at {args.api}: {e}", file=sys.stderr)
        print("[bridge] Is the API server running? (./start_api_server.sh)", file=sys.stderr)
        sys.exit(1)

    run_bridge(args.config, args.actors, args.api, args.orchestrator, extra)


if __name__ == "__main__":
    main()
