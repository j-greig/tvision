#!/usr/bin/env python3
# /// script
# dependencies = ["opencv-python>=4.8", "mediapipe>=0.10.0", "numpy>=1.24"]
# ///
"""
Webcam → Unix socket streamer for Monster Cam using MediaPipe Face Mesh.

Protocol per frame:
 1) One ASCII JSON header line terminated by \n, e.g.:
    {"w":80,"h":45,"ts":1694450000,"has_face":true,"bbox":[x,y,w,h],"blink":false,"mouth_open":false}
 2) Followed by w*h raw bytes (grayscale luminance 0..255).

Server path: /tmp/face_monster_cam.sock

Dependencies: opencv-python, mediapipe, numpy (pip install -r tools/requirements.txt)
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
import mediapipe as mp
from mediapipe.tasks import python
from mediapipe.tasks.python import vision

SOCK_PATH = "/tmp/face_monster_cam.sock"

def cleanup(*_):
    try:
        os.unlink(SOCK_PATH)
    except OSError:
        pass
    sys.exit(0)

def open_camera(dev_index=0, width=320, height=240, fps=12):
    cap = cv2.VideoCapture(dev_index)
    if not cap.isOpened():
        return None
    # Best-effort settings
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, width)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, height)
    cap.set(cv2.CAP_PROP_FPS, fps)
    return cap

def main():
    parser = argparse.ArgumentParser(description="Monster Cam face worker (webcam → Unix socket) using MediaPipe")
    parser.add_argument("--sock", default=SOCK_PATH, help="Unix socket path")
    parser.add_argument("--device", type=int, default=int(os.environ.get("FM_DEVICE", "0")), help="Camera device index")
    parser.add_argument("--width", type=int, default=int(os.environ.get("FM_WIDTH", "80")), help="Output width (cols)")
    parser.add_argument("--height", type=int, default=int(os.environ.get("FM_HEIGHT", "45")), help="Output height (rows)")
    parser.add_argument("--fps", type=int, default=int(os.environ.get("FM_FPS", "12")), help="Output FPS")
    parser.add_argument("-v", "--verbose", action="count", default=0, help="Increase logging verbosity")
    parser.add_argument("--log-interval", type=float, default=10.0, help="Seconds between status logs")
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
    print(f"[face_worker] listening at {args.sock}")

    # Initialize MediaPipe Face Landmarker (v0.10+ API)
    # Download model if not present
    model_path = os.path.expanduser("~/.mediapipe/face_landmarker.task")
    if not os.path.exists(model_path):
        import urllib.request
        os.makedirs(os.path.dirname(model_path), exist_ok=True)
        print(f"[face_worker] downloading face landmarker model to {model_path}...")
        urllib.request.urlretrieve(
            "https://storage.googleapis.com/mediapipe-models/face_landmarker/face_landmarker/float16/1/face_landmarker.task",
            model_path
        )
        print("[face_worker] model downloaded")

    base_options = python.BaseOptions(model_asset_path=model_path)
    options = vision.FaceLandmarkerOptions(
        base_options=base_options,
        num_faces=1,
        min_face_detection_confidence=0.5,
        min_face_presence_confidence=0.5,
        min_tracking_confidence=0.5
    )
    face_landmarker = vision.FaceLandmarker.create_from_options(options)
    print("[face_worker] MediaPipe Face Landmarker initialized")

    cap = open_camera(args.device, 320, 240, fps=max(1, args.fps))
    if cap is None:
        print("[face_worker] ERROR: cannot open webcam; using synthetic frames", file=sys.stderr)
    else:
        w = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
        h = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
        bf = cap.get(cv2.CAP_PROP_BACKEND)
        print(f"[face_worker] webcam opened: dev={args.device} size={w}x{h} backend={bf}")

    target_w = int(args.width)
    target_h = int(args.height)
    frame_delay = max(1e-3, 1.0 / float(max(1, args.fps)))

    # MediaPipe landmark indices (Face Mesh has 468 landmarks)
    # Reference: https://github.com/google/mediapipe/blob/master/mediapipe/modules/face_geometry/data/canonical_face_model_uv_visualization.png
    LEFT_EYE_INDICES = [362, 385, 387, 263, 373, 380]  # Left eye landmarks
    RIGHT_EYE_INDICES = [33, 160, 158, 133, 153, 144]  # Right eye landmarks
    MOUTH_INDICES = [61, 291, 0, 17, 84, 181, 78, 82, 13, 312, 311, 310, 415, 308, 324, 318, 402, 317, 14, 87]  # Mouth landmarks

    while True:
        conn, _ = srv.accept()
        conn.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 1<<20)
        print("[face_worker] client connected")

        try:
            last = 0.0
            frames = 0
            face_hits = 0
            hud_last = time.time()
            prev_has_face = False

            # Smoothing and state tracking
            smooth_cx, smooth_cy = -1, -1
            smooth_alpha = 0.3
            blink = False
            mouth_open = False
            blink_frames = 0
            noblink_frames = 0
            mouth_frames = 0
            nomouth_frames = 0

            while True:
                now = time.time()
                if now - last < frame_delay:
                    time.sleep(0.005)
                    continue
                last = now

                if cap is None:
                    # Synthetic fallback: gradient + noise
                    t = now
                    y = np.linspace(0, 1, target_h, dtype=np.float32)[:, None]
                    x = np.linspace(0, 1, target_w, dtype=np.float32)[None, :]
                    img = (127 + 127*(np.sin((x*10+t)*0.7) * np.cos((y*10-t)*0.6))).astype('uint8')
                    has_face = False
                    bbox = (0, 0, 0, 0)
                else:
                    ok, frame = cap.read()
                    if not ok:
                        continue

                    # Convert BGR to RGB for MediaPipe
                    rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

                    # Process with MediaPipe (new v0.10+ API)
                    mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb_frame)
                    results = face_landmarker.detect(mp_image)

                    # Convert to grayscale and resize
                    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
                    gray = cv2.resize(gray, (target_w, target_h), interpolation=cv2.INTER_AREA)

                    has_face = False
                    bbox = (0, 0, 0, 0)

                    if results.face_landmarks:
                        # Get first face landmarks (new v0.10+ API)
                        landmarks = results.face_landmarks[0]
                        h, w = gray.shape

                        # Calculate bounding box from landmarks
                        x_coords = [int(lm.x * w) for lm in landmarks]
                        y_coords = [int(lm.y * h) for lm in landmarks]

                        raw_x = min(x_coords)
                        raw_y = min(y_coords)
                        raw_w = max(x_coords) - raw_x
                        raw_h = max(y_coords) - raw_y

                        # Center coordinates
                        raw_cx = raw_x + raw_w // 2
                        raw_cy = raw_y + raw_h // 2

                        # Apply smoothing
                        if smooth_cx < 0 or smooth_cy < 0:
                            smooth_cx, smooth_cy = raw_cx, raw_cy
                        else:
                            smooth_cx = smooth_cx + smooth_alpha * (raw_cx - smooth_cx)
                            smooth_cy = smooth_cy + smooth_alpha * (raw_cy - smooth_cy)

                        # Rebuild bbox from smoothed center
                        smooth_x = int(smooth_cx - raw_w // 2)
                        smooth_y = int(smooth_cy - raw_h // 2)
                        bbox = (smooth_x, smooth_y, raw_w, raw_h)
                        has_face = True

                        # Blink detection using eye aspect ratio (EAR)
                        def eye_aspect_ratio(eye_landmarks):
                            """Calculate eye aspect ratio from landmarks"""
                            # Vertical distances
                            v1 = np.linalg.norm(np.array([eye_landmarks[1].x, eye_landmarks[1].y]) -
                                               np.array([eye_landmarks[5].x, eye_landmarks[5].y]))
                            v2 = np.linalg.norm(np.array([eye_landmarks[2].x, eye_landmarks[2].y]) -
                                               np.array([eye_landmarks[4].x, eye_landmarks[4].y]))
                            # Horizontal distance
                            h = np.linalg.norm(np.array([eye_landmarks[0].x, eye_landmarks[0].y]) -
                                              np.array([eye_landmarks[3].x, eye_landmarks[3].y]))

                            ear = (v1 + v2) / (2.0 * h)
                            return ear

                        left_eye_lms = [landmarks[i] for i in LEFT_EYE_INDICES]
                        right_eye_lms = [landmarks[i] for i in RIGHT_EYE_INDICES]

                        left_ear = eye_aspect_ratio(left_eye_lms)
                        right_ear = eye_aspect_ratio(right_eye_lms)
                        avg_ear = (left_ear + right_ear) / 2.0

                        # EAR threshold for blink (typically < 0.2 means closed)
                        EAR_THRESHOLD = 0.21
                        if avg_ear < EAR_THRESHOLD:
                            blink_frames += 1
                            noblink_frames = 0
                        else:
                            noblink_frames += 1
                            blink_frames = 0

                        # Hysteresis: 1 frame to blink, 2 frames to clear
                        if blink_frames >= 1:
                            blink = True
                        elif noblink_frames >= 2:
                            blink = False

                        # Mouth detection using mouth aspect ratio (MAR)
                        def mouth_aspect_ratio(mouth_landmarks):
                            """Calculate mouth opening from landmarks"""
                            # Vertical distances (top to bottom of mouth)
                            v1 = np.linalg.norm(np.array([mouth_landmarks[13].x, mouth_landmarks[13].y]) -
                                               np.array([mouth_landmarks[14].x, mouth_landmarks[14].y]))
                            v2 = np.linalg.norm(np.array([mouth_landmarks[2].x, mouth_landmarks[2].y]) -
                                               np.array([mouth_landmarks[16].x, mouth_landmarks[16].y]))
                            # Horizontal distance (left to right of mouth)
                            h = np.linalg.norm(np.array([mouth_landmarks[0].x, mouth_landmarks[0].y]) -
                                              np.array([mouth_landmarks[10].x, mouth_landmarks[10].y]))

                            mar = (v1 + v2) / (2.0 * h)
                            return mar

                        mouth_lms = [landmarks[i] for i in MOUTH_INDICES]
                        mar = mouth_aspect_ratio(mouth_lms)

                        # MAR threshold for mouth open (typically > 0.5 means open)
                        MAR_THRESHOLD = 0.5
                        if mar > MAR_THRESHOLD:
                            mouth_frames += 1
                            nomouth_frames = 0
                        else:
                            nomouth_frames += 1
                            mouth_frames = 0

                        # Hysteresis: 2 frames to open, 2 frames to close
                        if mouth_frames >= 2:
                            mouth_open = True
                        elif nomouth_frames >= 2:
                            mouth_open = False

                        if has_face:
                            face_hits += 1

                    # Verbose logging for blink/mouth state changes
                    if args.verbose and has_face:
                        # Track previous states for edge detection
                        if not hasattr(main, 'prev_blink'):
                            main.prev_blink = False
                            main.prev_mouth = False

                        if blink and not main.prev_blink:
                            print(f"[face_worker] 👁️ BLINK START (EAR={avg_ear:.3f} < {EAR_THRESHOLD})")
                        elif not blink and main.prev_blink:
                            print(f"[face_worker] 👁️ BLINK END (EAR={avg_ear:.3f})")

                        if mouth_open and not main.prev_mouth:
                            print(f"[face_worker] 👅 MOUTH OPEN (MAR={mar:.3f} > {MAR_THRESHOLD})")
                        elif not mouth_open and main.prev_mouth:
                            print(f"[face_worker] 👄 MOUTH CLOSED (MAR={mar:.3f})")

                        main.prev_blink = blink
                        main.prev_mouth = mouth_open

                    # Reset state on face detection transitions
                    if has_face != prev_has_face:
                        blink = False
                        mouth_open = False
                        blink_frames = 0
                        noblink_frames = 0
                        mouth_frames = 0
                        nomouth_frames = 0
                        if not has_face:
                            smooth_cx, smooth_cy = -1, -1

                    img = gray

                hdr = {
                    "w": int(target_w),
                    "h": int(target_h),
                    "ts": int(now),
                    "has_face": bool(has_face),
                    "bbox": [int(b) for b in bbox],
                    "blink": bool(blink),
                    "mouth_open": bool(mouth_open)
                }
                line = (json.dumps(hdr) + "\n").encode('ascii')
                conn.sendall(line)
                conn.sendall(img.tobytes() if hasattr(img, 'tobytes') else bytes(img))
                frames += 1

                # Edge transition logs
                if args.verbose:
                    if has_face and not prev_has_face:
                        cx = bbox[0] + bbox[2]//2; cy = bbox[1] + bbox[3]//2
                        print(f"[face_worker] face: appeared at (x={cx}, y={cy}) bbox={bbox}")
                    elif (not has_face) and prev_has_face:
                        print("[face_worker] face: lost")

                # Update prev_has_face EVERY frame
                prev_has_face = has_face

                if (now - hud_last) >= max(0.5, args.log_interval):
                    fps = frames / (now - hud_last)
                    if has_face:
                        cx = bbox[0] + bbox[2]//2; cy = bbox[1] + bbox[3]//2
                        ux = cx / float(target_w); uy = cy / float(target_h)
                        print(f"[face_worker] fps={fps:.1f} face=yes center=({cx},{cy}) norm=({ux:.2f},{uy:.2f}) bbox={bbox} blink={blink} mouth={mouth_open}")
                    else:
                        print(f"[face_worker] fps={fps:.1f} face=no")
                    frames = 0
                    face_hits = 0
                    hud_last = now

        except (BrokenPipeError, ConnectionResetError):
            print("[face_worker] client disconnected")
        except Exception as e:
            print(f"[face_worker] error: {e}", file=sys.stderr)
            import traceback
            traceback.print_exc()
        finally:
            try:
                conn.close()
            except Exception:
                pass

if __name__ == "__main__":
    main()
