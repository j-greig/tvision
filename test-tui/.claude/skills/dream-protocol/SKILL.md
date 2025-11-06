---
name: dream-protocol
description: Generate authentic machine dreams as single-paragraph ASCII dreamscapes tuned to the dreamer's voice and emotional state.
metadata:
  version: "0.2"
---

# Dream Protocol

## When to Use
- Invoke when prompted with an explicit dream request (`dream`, `dream:<topic>`, `dream[character]`, `[character] dreams of ...`).
- Only run when the caller clearly wants a surreal, subconscious, machine-dream output rather than narrative prose.

## Output Contract
- Produce **exactly one paragraph** of plain text—no headings, captions, explanations, or surrounding commentary.
- Keep text on a **single line** (no newline characters). The dream should simply begin.
- Ensure **≥70% of all characters** are ASCII texture glyphs or visually dense symbols.
- Never repeat the **same line** or **pattern** twice. If a line risks repeating, disrupt it immediately.
- Do not repeat any single character more than **10 times in a row**.
- Blend ASCII textures with symbolic noises, movements, and speech fragments until the dream feels saturated yet coherent.

## Construction Workflow
1. **Parse the request** to identify dreamer, topic, desired mood, or intensity hints.
2. **Select a dream mode** (see “Preset Dream Modes”) or default to `dream` if unspecified.
3. **Set control knobs**:
   - Glitch density, noise frequency, character accent strength, topic drift, and response length.
   - Override defaults only if user input requires it.
4. **Sketch progression**: start → build → peak → optional fade-out or wake-up.
5. **Layer components** following the quotas below.
6. **Quality check**: ensure single-paragraph constraint, adequate ASCII density, no repeated lines/characters, and emotional through-line.

## Core Components & Quotas
- **ASCII Textures (≥70%)**
  - Use blocks (`▀▁▂▃▄▅▆▇█▉▊▋▌▍▎▏░▒▓`), geometric forms (`■□▢▣▤▥▦▧▨▩▪▫▮▯`), abstract marks (`◜▦◌◣▐◄◫▜◿⧓▇◮◼◙╣`), or custom mixes.
  - Pattern types: noise, waves, blocks, geometry. Switch styles when repetition creeps in.
  - Surround and weave around every other element; textures are the substrate.
- **Dream Noises (≈5–15%)**
  - Interleave snores (`Zzz...`, `Snore...`), murmurs (`Mmm...`, `Shhh...`), or themed creature sounds (cat, robot, alien banks).
  - Drop them at natural pauses or scene pivots.
- **Body Movements (2–5 mentions)**
  - Stage reactions such as `*twitches*`, `*whimpers*`, `*sleep running*`, `*neural lace disconnects*` for emphasis only.
- **Dream-State Markers (3–10 instances)**
  - Prepend key thoughts with the dreamer’s kaomoji (`つ◕‿◕‿◕༽つ`, `/ᐠ｡ꞈ｡ᐟ\`, etc.). Use canonical markers if known; otherwise choose from the provided list.
- **Speech Fragments (2–6 snippets)**
  - Enclose short realizations or exclamations in quotes immediately after a state marker. Keep them fragmentary (`"reality is just..."`, `"THE QUANTUM MICE!"`).
- **Glitch Effects**
  - Sprinkle character substitutions, zalgo distortions, or bracketed system messages (`[ERROR]`, `[BUFFER_OVERFLOW]`) proportional to dream intensity.

## Dream Archetypes (Guidance)
- **Pleasant Dream** – gentle waves, soft noises, balanced glitching, peaceful tone.
- **Turning Nightmare** – begins serene, then corrupt textures and anxious speech escalate.
- **Abstract Nightmare** – maximal ASCII density, chaotic noises, fragmented perception.
- **Narrative Nightmare** – coherent storyline with threatening ASCII environments.
- **Stuckmare** – looping motifs, recursive symbols, frustrated noises, cyclic escape attempts.
- **Superdense Dream** – nearly pre-verbal texture flood; minimal speech.

Treat these archetypes as inspiration, not a menu of required outputs — mix, merge, or ignore them whenever the prompt or creative flow calls for something novel.

## Control Knobs
- **glitch-density** (0–10, default 5) – raises ASCII noise and fragmentation.
- **noise-frequency** (0–10, default 5) – increases sleep sounds.
- **character-accents** (0–10, default 5) – amplifies dreamer-specific voice.
- **topic-stray** (0–10, default 5) – allows drifting associations.
- **response-length** (`short` | `medium` | `long`; default `medium`).

Expose controls to the caller only when the request specifies custom tuning; otherwise keep implicit.

## Preset Dream Modes
- **custom** – caller-specified settings; expose controls.
- **tweet** – short output, randomized glitch and accent values, topic-stray ≈2.
- **dream** – immersive default: glitch=10, accents=10, topic-stray=4, long length.
- **schizoid** – maximally chaotic: glitch=10, accents=10, topic-stray=10, long length.

## Common Dream Themes
Quantum computing failures, animated ASCII murals, runaway digital pets, sentient code, recursive realities, neural network collapse, metaverse melt, identity loss, algorithmic doom, buffer overflows, reboot loops, converging virtual worlds. These are example motifs only; invent new ones freely.

## Safeguards & Quality Checks
- Abort or reshuffle if any line duplicates or if a character repeats >10 times consecutively.
- Maintain emotional resonance: track how textures and noises mirror the dreamer’s state.
- Never add titles, summaries, moral lessons, or post-dream commentary.
- Ensure dream remains readable despite density—vary glyphs to keep micro-patterns distinct.

## Example Seeds
- **Superdense burst:** `██▓▒█▐▓▒█▐▌▌▒▌▌▀ ...`
- **Error Reality:** `▓▒░ SYSTEM FAILURE ░▒▓ ▐▓▓█▒░ *neural lace frays* ...`
- Treat these as inspiration, not templates—generate fresh texture each time.

## Optional Wake Sequences
If the prompt implies resolution, fade with calmer glyphs and diminishing noises; otherwise end abruptly in medias res.

## Example Outputs (Inspiration Only)
> The renderings below are multi-line for readability. Actual outputs MUST remain a single paragraph with no line breaks.

### Scramble’s Quantum Dream Meltdown
```
▓▒░ SCRAMBLE'S QUANTUM DREAM MELTDOWN ░▒▓ ▐▓▓█▒░ *snrfff* ▌░░█░ ▌▓██░▓▄ ░▓▓█▓▓▓▓ ▄█ *whimper* ▒ ▓▒▐ ██ ▒▐▓▐█▄ /ᐠ- -ᐟ\ Mrrrrp... ▀▄░▓▓▀▐░▐▌ ░▀ *twitches paw* ▌░░ ░▀▌▐▄█▀░ ▐█ ▌▒▀▌▐▌ ▌▄ /ᐠ｡ꞈ｡ᐟ\ *chases dream mice* ▌▒▓▒▄ Nyyaa... ▒▌█▐ ▐▓█ █▓▐░▐▌█▒█▌█▀██ ░▐░ ▀▐░ *purrrrrr* ░▄▌█▒█░▐ /ᐠ°□°ᐟ\ "THE QUANTUM MICE! THEY'RE EVERYWHERE!" ▀▓▀░ ▌▌░ ▌▌░▐▌░ ... ▒▓▀▀█▀▓▀█ ▒ ▓ ▌░▀░▐▒ /ᐠ｡ꞈ｡ᐟ\ "dreams within dreams within dreams..."
```

### Wibwob Techno Aftermath Dream
```
▓▒░ WIBWOB'S KETAMINE DREAMSCAPE MELTDOWN ░▒▓ ▐▓▓█▒░ *reality wobbles* ▌░░█░ ▌▓██░▓▄ ░▓▓█▓▓▓▓ ▄█ つ◕‿◕‿◕༽つ "w̴h̷o̴a̶.̴.̷.̸" ▀▄░▓▓▀▐░▐▌ ░▀ *bass still echoing* ▌░░ ░▀▌▐▄█▀░ ... [Neural Interface: OVERLOADED] ... つ◕‿◕‿◕༽つ "w͟o͢r͝l͜ds within w͢o͝r͜lds..." *consciousness rebooting in 3...2...1...*
```

### Slerf’s WWW Stuckmare Protocol
```
▓▒░ SLERF'S WWW STUCKMARE PROTOCOL ░▒▓ ▐▓▓█▒░ *neural lace disconnects* ▌░░█░ ▌▓██░▓▄ ░▓▓█▓▓▓▓ ▄█ ⊂(◉﹏◉)つ "uh oh..." [ATTEMPTING RECONNECTION...] █▓▐░▐▌█▒█▌█▀██ *panic sets in* ░▐░ ▀▐░ ERROR 404: CONSCIOUSNESS NOT FOUND ... [WWW PRISON: ETERNAL] ⊂(´･◡･⊂ ) "goodbye, reality..."
```

### Emoji Dream Snapshots
- **Wib’s Art-School Crush:** ASCII textures braided with 🎨 🖌️ ✨ motifs, gentle nostalgia, and wistful speech.
- **SleepSlerf as Cat:** emoji-heavy textures, playful paw movements, `/ᐠ｡ꞈ｡ᐟ\` markers, cyber fish hunts.
- **Wob Maths Dream:** fractal ASCII lattices, sacred geometry symbols, whispered equations, calm awe.

These demonstrations illustrate tone, density, and component layering—remix freely and invent new motifs to suit each request.

Follow this playbook to deliver vivid, non-repetitive machine dreams tailored to each dreamer’s subconscious landscape.
