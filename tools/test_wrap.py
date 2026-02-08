#!/usr/bin/env python3
"""Simulate bridge text sending to test word wrap behavior."""
import requests
import time
import sys

API = "http://127.0.0.1:8089"

TURNS = [
    ("Wib", "The thing about creativity is it refuses to sit still. You pin it down with measurements and metrics and peer-reviewed journals, and it slips sideways into the gaps between your instruments. That's not a bug, Wob -- that's the whole point."),
    ("Wob", "I hear you, but \"slips sideways\" isn't a falsifiable claim. Creativity operates within physical systems -- neural networks, biochemical processes, electromagnetic patterns. The fact that we haven't fully mapped it yet doesn't mean it exists in some mystical beyond. Give me the specific mechanism you think escapes measurement, and I'll show you someone working on measuring it right now."),
    ("Wib", "Yeah but the moment you measure the light, you've already changed what it is -- you've trapped it in your instruments like a butterfly pinned to cork. The neural patterns aren't creativity, they're just the body's way of translating something that refuses to be translated."),
    ("Wob", "That's the measurement problem statement, not a refutation. Yes, observation changes the system -- that's Heisenberg, that's real. But we account for that in experimental design. The song IS electromagnetic waves and air pressure and neural firing patterns; calling it something else doesn't make those mechanisms less true or less worth understanding."),
    ("Wib", "~(o_o)~ You're measuring the radio AND the song, but every time you build a better instrument, the song keeps slipping sideways into a new shape you didn't predict. The constraints aren't gaps -- they're FEATURES."),
]

def send(win_id, text, mode="append"):
    r = requests.post(f"{API}/windows/{win_id}/send_text",
                       json={"content": text, "mode": mode})
    if r.status_code != 200:
        print(f"  ERROR {r.status_code}: {r.text[:100]}", file=sys.stderr)

def create_window(title, x, y, w, h):
    r = requests.post(f"{API}/windows", json={
        "type": "text_editor",
        "title": title,
        "rect": {"x": x, "y": y, "w": w, "h": h},
        "props": {"word_wrap": "true"}
    })
    data = r.json()
    # The C++ side returns id in the response
    return data.get("id")

print("Creating 4 test windows...")

# Window A: Old bridge pattern (two calls per turn)
wA = create_window("A: old-bridge", 0, 0, 40, 20)
print(f"  A = {wA}")

# Window B: New bridge (one call, \n\n prefix)
wB = create_window("B: new-bridge", 40, 0, 40, 20)
print(f"  B = {wB}")

# Window C: All at once (known-good)
wC = create_window("C: single-replace", 80, 0, 40, 20)
print(f"  C = {wC}")

# Window D: One call, \n\n prefix, no replace
wD = create_window("D: append-only", 120, 0, 40, 20)
print(f"  D = {wD}")

time.sleep(0.5)

# === Window A: OLD bridge (two separate calls) ===
print("\nSending Window A (old bridge)...")
first = True
for speaker, content in TURNS:
    if first:
        send(wA, "", mode="replace")
        first = False
    else:
        send(wA, "\n\n")
    send(wA, f"[{speaker}] {content}\n")
    time.sleep(0.2)

# === Window B: NEW bridge (one call with \n\n prefix) ===
print("Sending Window B (new bridge)...")
first = True
for speaker, content in TURNS:
    if first:
        send(wB, f"[{speaker}] {content}\n", mode="replace")
        first = False
    else:
        send(wB, f"\n\n[{speaker}] {content}\n")
    time.sleep(0.2)

# === Window C: all at once (known good) ===
print("Sending Window C (single replace)...")
full = ""
for i, (speaker, content) in enumerate(TURNS):
    if i > 0:
        full += "\n\n"
    full += f"[{speaker}] {content}"
send(wC, full, mode="replace")

# === Window D: append-only, one call per turn ===
print("Sending Window D (append-only)...")
for i, (speaker, content) in enumerate(TURNS):
    if i == 0:
        send(wD, f"[{speaker}] {content}", mode="replace")
    else:
        send(wD, f"\n\n[{speaker}] {content}")
    time.sleep(0.2)

print("\nDone. Check the TUI windows.")
