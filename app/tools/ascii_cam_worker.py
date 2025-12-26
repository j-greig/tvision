#!/usr/bin/env python3
# /// script
# dependencies = ["opencv-python>=4.8", "numpy>=1.24"]
# ///
"""
ASCII Art Webcam Worker

Converts webcam feed to ASCII art and streams via Unix socket.
Resolution automatically adapts to client window size.

Protocol per frame:
1) JSON header line: {"cols":80,"rows":24}\n
2) Followed by 'rows' lines of ASCII art text (each line terminated by \n)

Server path: /tmp/ascii_cam.sock

Dependencies: opencv-python, numpy (pip install -r tools/requirements.txt)
"""

import cv2
import os
import sys
import time
import json
import socket
import signal
import argparse
import numpy as np

SOCK_PATH = "/tmp/ascii_cam.sock"

# ASCII gradient (10 levels, dark to light)
ASCII_GRADIENT = " .:-=+*#%@"

def cleanup(*_):
    try:
        os.unlink(SOCK_PATH)
    except OSError:
        pass
    sys.exit(0)

def open_camera(dev_index=0, width=640, height=480, fps=30):
    cap = cv2.VideoCapture(dev_index)
    if not cap.isOpened():
        return None
    # Best-effort settings
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, width)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, height)
    cap.set(cv2.CAP_PROP_FPS, fps)
    return cap

def convert_to_ascii(gray_frame, cols, rows, gradient=ASCII_GRADIENT):
    """
    Convert grayscale frame to ASCII art.

    Args:
        gray_frame: numpy array (H, W) uint8 grayscale
        cols: target ASCII width (columns)
        rows: target ASCII height (rows)
        gradient: string of chars from dark to light

    Returns:
        List of strings (ASCII art lines)
    """
    # Correct for terminal aspect ratio: chars are ~2x taller than wide
    # So multiply width by 2 for proper aspect ratio
    ascii_width = cols * 2
    ascii_height = rows

    # Resize frame to target dimensions
    resized = cv2.resize(gray_frame, (ascii_width, ascii_height), interpolation=cv2.INTER_AREA)

    # Map pixel values (0-255) to gradient indices
    num_chars = len(gradient)
    ascii_frame = []

    for y in range(ascii_height):
        row = ""
        for x in range(0, ascii_width, 2):  # Process 2 pixels at a time (aspect ratio correction)
            # Average 2 horizontal pixels to get 1 char
            if x + 1 < ascii_width:
                pixel_val = (int(resized[y, x]) + int(resized[y, x+1])) // 2
            else:
                pixel_val = resized[y, x]

            char_idx = int((pixel_val / 255.0) * (num_chars - 1))
            char_idx = max(0, min(num_chars - 1, char_idx))  # Clamp
            row += gradient[char_idx]
        ascii_frame.append(row)

    return ascii_frame

def main():
    parser = argparse.ArgumentParser(description="ASCII Art Webcam Worker (webcam → Unix socket)")
    parser.add_argument("--sock", default=SOCK_PATH, help="Unix socket path")
    parser.add_argument("--device", type=int, default=int(os.environ.get("FM_DEVICE", "0")), help="Camera device index")
    parser.add_argument("--fps", type=int, default=int(os.environ.get("FM_FPS", "12")), help="Output FPS")
    parser.add_argument("-v", "--verbose", action="count", default=0, help="Increase logging verbosity")
    parser.add_argument("--log-interval", type=float, default=10.0, help="Seconds between status logs")
    parser.add_argument("--gradient", default=ASCII_GRADIENT, help="ASCII gradient string (dark to light)")
    args = parser.parse_args()

    signal.signal(signal.SIGINT, cleanup)
    signal.signal(signal.SIGTERM, cleanup)

    # Remove stale socket
    try:
        os.unlink(args.sock)
    except FileNotFoundError:
        pass

    srv = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    srv.bind(args.sock)
    srv.listen(1)
    os.chmod(args.sock, 0o666)
    print(f"[ascii_cam_worker] listening at {args.sock}")

    cap = open_camera(args.device, 640, 480, fps=max(1, args.fps))
    if cap is None:
        print("[ascii_cam_worker] ERROR: cannot open webcam; using synthetic frames", file=sys.stderr)
    else:
        w = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
        h = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
        bf = cap.get(cv2.CAP_PROP_BACKEND)
        print(f"[ascii_cam_worker] webcam opened: dev={args.device} size={w}x{h} backend={bf}")

    # Default target dimensions (will be updated by client)
    target_cols = 80
    target_rows = 24
    frame_delay = max(1e-3, 1.0 / float(max(1, args.fps)))

    while True:
        conn, _ = srv.accept()
        conn.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 1<<20)
        print("[ascii_cam_worker] client connected")

        try:
            last = 0.0
            frames = 0
            hud_last = time.time()

            # Buffer for incoming resize commands
            resize_buffer = ""

            while True:
                now = time.time()

                # Check for resize command from client (non-blocking)
                try:
                    conn.setblocking(False)
                    data = conn.recv(256).decode('utf-8')
                    if data:
                        resize_buffer += data
                        # Look for complete JSON line
                        while '\n' in resize_buffer:
                            line, resize_buffer = resize_buffer.split('\n', 1)
                            try:
                                cmd = json.loads(line)
                                if cmd.get('cmd') == 'resize':
                                    target_cols = cmd.get('cols', target_cols)
                                    target_rows = cmd.get('rows', target_rows)
                                    if args.verbose:
                                        print(f"[ascii_cam_worker] client requested resize: {target_cols}x{target_rows}")
                            except json.JSONDecodeError:
                                pass
                except BlockingIOError:
                    pass
                except Exception as e:
                    # Connection error
                    break
                finally:
                    conn.setblocking(True)

                # Frame rate limiting
                if now - last < frame_delay:
                    time.sleep(0.005)
                    continue
                last = now

                if cap is None:
                    # Synthetic fallback: gradient + noise
                    t = now
                    y = np.linspace(0, 1, target_rows * 2, dtype=np.float32)[:, None]
                    x = np.linspace(0, 1, target_cols * 2, dtype=np.float32)[None, :]
                    img = (127 + 127*(np.sin((x*10+t)*0.7) * np.cos((y*10-t)*0.6))).astype('uint8')
                else:
                    ok, frame = cap.read()
                    if not ok:
                        continue

                    # Convert to grayscale
                    img = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

                # Convert to ASCII art
                ascii_lines = convert_to_ascii(img, target_cols, target_rows, args.gradient)

                # Send header
                hdr = {
                    "cols": target_cols,
                    "rows": target_rows,
                    "ts": int(now)
                }
                header_line = (json.dumps(hdr) + "\n").encode('ascii')
                conn.sendall(header_line)

                # Send ASCII lines
                for line in ascii_lines:
                    # Ensure line is exactly target_cols wide (pad or truncate)
                    if len(line) < target_cols:
                        line = line + ' ' * (target_cols - len(line))
                    elif len(line) > target_cols:
                        line = line[:target_cols]
                    conn.sendall((line + "\n").encode('utf-8'))

                frames += 1

                # Status logging
                if (now - hud_last) >= max(0.5, args.log_interval):
                    fps = frames / (now - hud_last)
                    print(f"[ascii_cam_worker] fps={fps:.1f} size={target_cols}x{target_rows}")
                    frames = 0
                    hud_last = now

        except (BrokenPipeError, ConnectionResetError):
            print("[ascii_cam_worker] client disconnected")
        except Exception as e:
            print(f"[ascii_cam_worker] error: {e}", file=sys.stderr)
            import traceback
            traceback.print_exc()
        finally:
            try:
                conn.close()
            except Exception:
                pass

if __name__ == "__main__":
    main()
