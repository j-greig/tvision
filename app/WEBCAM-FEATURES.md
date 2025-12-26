# Webcam Features

Two real-time webcam visualization modes with different aesthetics.

## Quick Start

### ASCII Webcam (Recommended - Simpler Setup)

**Real-time ASCII art that scales with window size**

```bash
# Start worker (from project root)
uv run app/tools/ascii_cam_worker.py --device 1

# Open TUI in another terminal
./build/test_pattern
# Menu: View → ASCII Webcam (or hotkey: W)
```

**What you get:**
- Webcam feed converted to ASCII characters (` .:-=+*#%@`)
- Auto-scales: small window = chunky, large window = detailed
- Controls: `v` (toggle HUD), `Space` (pause), `+/-` (speed), `r` (reset)

---

### Monster Cam (Emoji Sprite with Face Tracking)

**Minimal emoji face sprite that tracks your face position, blinks, and tongue**

```bash
# Start worker (from project root)
uv run app/tools/face_worker.py --device 1 -v

# Open TUI in another terminal
./build/test_pattern
# Menu: View → Monster Cam (Emoji) (or hotkey: C)
```

**What you get:**
- 3-line emoji sprite: `👁️═👁️` / `∿👃∿` / `👄` or `👅`
- Tracks face position with smoothing
- Blinks when you blink (eyes disappear)
- Shows tongue `👅` when mouth opens, lips `👄` when closed
- Controls: `v` (toggle HUD), `Space` (pause), `+/-` (speed), `r` (reset)

---

## Comparison

| Feature | ASCII Webcam | Monster Cam (Emoji) |
|---------|--------------|---------------------|
| **Aesthetic** | Retro ASCII art | Minimal emoji sprite |
| **Detail** | High-res realistic | Stylized abstract |
| **Dependencies** | opencv, numpy | opencv, numpy, **mediapipe** |
| **Setup** | ✅ Simple | ⚠️ Needs MediaPipe model download |
| **Worker Script** | `ascii_cam_worker.py` | `face_worker.py` |
| **Socket** | `/tmp/ascii_cam.sock` | `/tmp/face_monster_cam.sock` |
| **Menu Command** | View → ASCII Webcam (W) | View → Monster Cam (C) |
| **Features** | Grayscale conversion | Face tracking, blink, tongue |

---

## Troubleshooting

### "webcam:failed" in window

**For ASCII Webcam:**
- Check worker is running: `ps aux | grep ascii_cam_worker`
- Restart worker: `uv run app/tools/ascii_cam_worker.py --device 1`

**For Monster Cam:**
- Check worker is running: `ps aux | grep face_worker`
- Restart worker: `uv run app/tools/face_worker.py --device 1 -v`

### "ModuleNotFoundError: No module named 'mediapipe'"

Only needed for Monster Cam. The `uv run` command handles this automatically, but if you see this error:

```bash
# Let uv handle dependencies (recommended)
uv run app/tools/face_worker.py --device 1 -v

# Or install manually
pip3 install --break-system-packages mediapipe>=0.10.0
```

### Wrong camera device

List available cameras:
```bash
python3 -c "
import cv2
for i in range(5):
    cap = cv2.VideoCapture(i)
    if cap.isOpened():
        w = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
        h = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
        print(f'Device {i}: {w}x{h}')
        cap.release()
"
```

Then use the correct device number:
```bash
uv run app/tools/ascii_cam_worker.py --device 0  # or 1, 2, etc.
```

### Both windows simultaneously

You can run both workers at the same time (different sockets, different cameras if you have multiple):

```bash
# Terminal 1: ASCII worker on device 1
uv run app/tools/ascii_cam_worker.py --device 1

# Terminal 2: Face worker on device 0 (if you have 2 cameras)
uv run app/tools/face_worker.py --device 0 -v

# Terminal 3: TUI
./build/test_pattern
# Open both windows: View → ASCII Webcam + View → Monster Cam
```

---

## Technical Details

### ASCII Webcam Protocol

**Socket:** `/tmp/ascii_cam.sock`

**Bidirectional:**
- TUI → Worker: `{"cmd":"resize","cols":80,"rows":24}\n`
- Worker → TUI: `{"cols":80,"rows":24,"ts":123456}\n` + ASCII lines

**Aspect ratio correction:** Characters are ~2:1 tall/wide, so worker samples 2x width

### Monster Cam Protocol

**Socket:** `/tmp/face_monster_cam.sock`

**Unidirectional (Worker → TUI):**
- Header: `{"w":80,"h":45,"ts":123456,"has_face":true,"bbox":[x,y,w,h],"blink":false,"mouth_open":false}\n`
- Payload: `w*h` grayscale bytes

**Face tracking:**
- MediaPipe Face Mesh: 468 3D landmarks
- Eye Aspect Ratio (EAR < 0.21) = blink
- Mouth Aspect Ratio (MAR > 0.5) = mouth open

---

## Files

- `app/ascii_cam_view.{h,cpp}` - ASCII Webcam TUI view
- `app/generative_monster_cam_view.{h,cpp}` - Monster Cam TUI view
- `app/tools/ascii_cam_worker.py` - ASCII art converter worker
- `app/tools/face_worker.py` - Face tracking worker (MediaPipe)
- `app/tools/requirements.txt` - Python dependencies
