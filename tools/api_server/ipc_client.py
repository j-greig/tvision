from __future__ import annotations

import os
import socket
from typing import Dict, Optional


SOCK_PATH = os.environ.get("TV_IPC_SOCK", "/tmp/test_pattern_app.sock")


def send_cmd(cmd: str, kv: Optional[Dict[str, str]] = None) -> str:
    path = SOCK_PATH
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.connect(path)
    try:
        parts = [f"cmd:{cmd}"]
        for k, v in (kv or {}).items():
            parts.append(f"{k}={v}")
        line = " ".join(parts) + "\n"
        s.sendall(line.encode("utf-8"))
        data = s.recv(4096)
        return data.decode("utf-8", errors="ignore")
    finally:
        s.close()
