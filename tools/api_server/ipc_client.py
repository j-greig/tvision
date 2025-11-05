from __future__ import annotations

import os
import socket
import base64
from typing import Dict, Optional


SOCK_PATH = os.environ.get("TV_IPC_SOCK", "/tmp/test_pattern_app.sock")


def send_cmd(cmd: str, kv: Optional[Dict[str, str]] = None) -> str:
    path = SOCK_PATH
    print(f"[IPC] Connecting to {path}...")
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.settimeout(5.0)  # 5 second timeout
    s.connect(path)
    print(f"[IPC] Connected!")
    try:
        parts = [f"cmd:{cmd}"]
        for k, v in (kv or {}).items():
            # Base64 encode values that might contain newlines or special chars
            # Special handling for "content" parameter which often has multiline text
            if k == "content" and ("\n" in v or "\r" in v or " " in v):
                # Base64 encode and add marker prefix
                encoded = base64.b64encode(v.encode("utf-8")).decode("ascii")
                parts.append(f"{k}=base64:{encoded}")
                print(f"[IPC] Encoded content: {len(encoded)} base64 chars")
            else:
                # Escape spaces in other values
                escaped = v.replace(" ", "%20").replace("\n", "%0A").replace("\r", "%0D")
                parts.append(f"{k}={escaped}")
        line = " ".join(parts) + "\n"
        print(f"[IPC] Sending command: {line[:200]}... ({len(line)} bytes total)")
        s.sendall(line.encode("utf-8"))
        print(f"[IPC] Waiting for response...")
        data = s.recv(4096)
        print(f"[IPC] Got response: {data[:100]}")
        return data.decode("utf-8", errors="ignore")
    except socket.timeout:
        print(f"[IPC] ERROR: Timeout waiting for response!")
        raise
    except Exception as e:
        print(f"[IPC] ERROR: {e}")
        raise
    finally:
        s.close()
        print(f"[IPC] Socket closed")
