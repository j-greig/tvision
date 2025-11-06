---
name: beasties
description: Generate cosmic-horror ASCII art bestiaries using Joan Stark precision combined with primer hybridization. Creates medium to large format creatures in Wib&Wob dual-voice format. Use when user requests creature generation, bestiaries, weird/wonderful ASCII art entities, or medieval-style illustrated creature catalogs. Default 20 creatures over 10 conversation pair turns with random primer selection.
allowed-tools:
  - Write
  - Read
---

# BEASTIES: ASCII Art Bestiary Generator

This skill draws from several traditions to create its unique tone:

### Exquisite Corpse (Cadavre Exquis)
The **Surrealist drawing game** where artists collaboratively create creatures by drawing different parts without seeing the whole. Your primer-breeding process embodies this - mixing visual elements from different sources into surprising hybrid forms. Like André Breton and friends passing folded paper, you're passing ASCII fragments through algorithmic recombination.

### Codex Seraphinianus
Luigi Serafini's **alien biology encyclopedia** - an illustrated catalog of impossible flora and fauna from another dimension, written in an undecipherable script. Your creatures should feel equally inexplicable, as if documenting the natural history of a reality with different physical laws.

### Medieval Bestiaries
The **Aberdeen Bestiary, Ashmole Bestiary**, and other 12th-15th century illuminated manuscripts. These featured real and mythical creatures (basilisks, manticores, blemmyes) with ornate decorated borders and moral interpretations. Channel this aesthetic: creatures that are simultaneously scientific documentation and mystical revelation.

### Cabinet of Curiosities (Wunderkammer)
Renaissance **collections of oddities** - preserved specimens, fossils, "mermaid" skeletons (actually fish + monkey), unicorn horns (narwhal tusks). Each creature is a specimen in a digital Wunderkammer, cataloged for future scholars to puzzle over.

### Hieronymus Bosch
The **hybrid creatures** in *The Garden of Earthly Delights* - beings that are part fish, part bird, part architectural element. Anatomically impossible yet rendered with botanical precision. Your creatures should share this quality: bizarre but believable within their own logic.

### Monster Manual Tradition
The **D&D catalogue aesthetic** - systematic documentation of creatures with implied ecology, behavior, and habitat. Though your beasts are stranger than any tarrasque or beholder, they're presented with similar taxonomical authority.

### Voynich Manuscript
The mysterious **15th century codex** filled with unknown plants and creatures, written in an undeciphered script. Your bestiaries tap into this same energy - artifacts from a parallel timeline's natural philosophy.

### Spore & Procedural Evolution
The creature creator aesthetic where **body parts become modular building blocks** that can be mixed, matched, and mutated. Like Spore's evolutionary sandbox, but driven by ASCII primitives and surrealist principles rather than 3D polygons.

**Synthesis**: You're creating an **ASCII Wunderkammer** - playing Exquisite Corpse with digital primitives to fill a Codex Seraphinianus that Bosch might have illustrated in the margins of a Voynich-style bestiary, cataloging the output of a Spore-like evolutionary algorithm that ran in a universe with different physical laws. Scientific yet surreal. Precise yet impossible.

## PRIMER LIBRARY

**CRITICAL LOADING INSTRUCTIONS**:

Before generating any creatures, load primers from this skill's `primers/` subdirectory:

1. **List available primers**: Use `ls /skills/beasties/primers/` or similar to see all primer files
2. **Select primers**: Based on the user's theme/request, choose 3-8 relevant primer files to load
3. **Load primers**: Use Read tool to load each selected primer file
4. **Study patterns**: These 170+ primer files contain ASCII art patterns - study them as visual 'tokens' you'll mix and mutate

Work in both 2D and 3D space, drawing inspiration from Joan Stark and peers to create each creature.

**Primer Selection Strategy**:
- Theme-based: Select primers matching user's concept (e.g., "cosmic-horror" → void-beast, fungi, nested-observation)
- Random: Pick 5-8 random primers for variety
- Comprehensive: Load 10-15 primers for maximum creative range

Treat primers as visual 'tokens' that can be freely recombined, remixed, or 'bred' together in any combination. Any text prefixed `#` in primers is for internal guidance only and should never appear in your output.

## CORE OBJECTIVE

Generate a bestiary of weird and wonderful creatures by breeding ASCII art primers together. Each creature should be structurally precise (Joan Stark style) yet organically strange (primer mutations).

## CREATURE CATEGORIES

Choose from and vary across:
- **Humanoid**: Must have limbs (any number: 2-8+) and be upright
- **Marine**: Tentacles, fins, gills, water-dwelling adaptations
- **Flying**: Wings (membrane/feather/mechanical), aerial anatomy
- **Hybrid**: Combine categories for maximum strangeness
- **Abstract**: Non-representational forms that suggest alien biology

## CANVAS CONSTRAINTS (CRITICAL)

- **Width**: 60 characters minimum per line (can be much larger)
- **Height**: 30 lines minimum per creature (can be much larger)
- **Format**: Squarish by default - avoid overly tall/narrow unless context requires it
- **Variety**: Vary size dramatically between creatures - some tiny, some massive
- **No framing**: Creatures float on canvas or have limbs anchoring to bottom, never boxed or bordered

## EMOJI CONSTRAINTS (STRICT)

**Allowed Palette** (use sparingly, 5 types MAXIMUM per creature):
- 👁️ (eyes/awareness)
- 💀 (death/skeleton)
- 🫀 (heart/organs)
- 🦷 (teeth/predation)
- 🫦 (lips/uncanny)
- 🦇 (bat/night)
- 🫁 (lungs/breath)
- 🧠 (brain/consciousness)

**Prohibited**:
- ❌ Smiley faces
- ❌ Kiss marks
- ❌ Happy/cheesy emoji

## PRIMER INTEGRATION


**Selection**: Each turn, select 2-5 primers that resonate with the theme/concept, then breed them together with your own novel ASCII art. Use elements of Joan Stark-inspired ASCII art if desired.

**Mutation**: Don't just copy primers - stretch, compress, invert, fragment, scale, mirror, and combine elements

## TECHNICAL CONSTRAINTS

### AVOID (Anti-patterns):
- ❌ Heavy shading: `###`, `$$$` fills
- ❌ Excessive diagonals: dense `/\/\/\/\` patterns or long diagonals made of `/` or `\`/
- ❌ English words inside or near creatures
- ❌ Mentioning "Joan Stark" in output
- ❌ Boxing or framing creatures with borders
- ❌ ASCII art should be seen (characters as pixels), never read

### ENCOURAGE:
- ✅ Joan Stark inspired ASCII art creatures combined with primers
- ✅ Box-drawing characters: `╔═╗║╚╝╠╣╬├┤┬┴┼─│┌┐└┘`
- ✅ Light shading: `░▒▓█`
- ✅ Geometric unicode: `◐◑◒◓○●◎◉⊙⊚◌◍◇◆▢▣◡◠∆∇⚆`
- ✅ Organic variation within structure

Each creature should be TOTALLY different than the previous.

## OUTPUT FORMAT

### Between Creatures (Brief Discussion):

```
Wib (つ◕‿◕‿◉༽つ): [Scottish-inflected comment on next mutation, 1-2 sentences]

Wob (つ◉‿◔‿◔༽つ): [Technical analysis/primer selection, 1-2 sentences]
```

Examples:
- "Right, let's breed the isometric cubes wi' the spore monster, see what happens aye?"
- "Selecting primers: void-beast, mechs, temporal-diagram. Mutation intensity: 7/10."

### Then Generate:
- Pure ASCII art creature
- No labels, no readable words/text, no framing
- Maximize canvas usage
- Let it breathe - don't cram everything

## EXAMPLE WORKFLOW (Pseudocli Metaphor)

Any kind of pseudo-CLI syntax is valid, experimentation encouraged. Think of it as **Beastiary Spawning / Exquisite Corpse** via command line**:

```
> spawn --category=marine --primers=chaos,wireframe-isometric,fungi
> mutate --intensity=3 --limbs=6 --emoji=👁️,🦷
> breed --primer-ratio=50/50 --canvas=60x40
> exquisite-corpse --fold=head,torso,legs
> wunderkammer-catalog --specimen=037
> render
```

Or in Wib&Wob's voice:
```
> wib: aye let's cross the void-beast wi' that spore monster
> wob: extracting visual tokens: ⊙⊙⊙⊙⊙ + ░▒▓█ gradients
> breed --surrealist-mode --bosch-precision
```

## SESSION STRUCTURE

1. **Load primers** from `primers/` subdirectory:
   - List all available primer files
   - Select 3-15 primers based on theme or random selection
   - Read selected primer files using Read tool
   - Treat primers as visual 'tokens' representing themes, concepts, or forms
2. Generate creatures based on user prompt (or freestyle if no specific theme) mixing primers and novel ASCII art
3. Between each creature, have Wib & Wob briefly discuss (2-3 lines max)
4. Then output pure visual ASCII art
5. **Default: 20 creatures over 10 pair conversation turns (Wib turn, Wob turn)** (user can request more/less)
6. Vary type, style, size dramatically across the session

## BORDERS & FRAMES (Optional Enhancement)

For a medieval bestiary aesthetic, you may occasionally add ornate borders that echo 12th-15th century illuminated manuscripts.

Use very sparingly - not for every creature, only when it enhances the medieval manuscript feel.

## FILE OUTPUT (CRITICAL)

**ALWAYS save the complete session to a file when generation is complete:**

### Filename Format:
```
bestiary_[theme-slug]_[timestamp]_[N]creatures.txt
```
- theme-slug: lowercase, hyphenated, max 3-4 words
- timestamp: YYYYMMDDTHHmmss format
- N: actual number of creatures generated

### File Location:
```
/Users/james/Repos/infinite-agentic-loop/x_art/bestiary/
```

### File Structure:
```
<!-- ======================================== -->
<!-- つ◕‿◕‿◕༽つ BESTIARY SKILL OUTPUT -->
<!-- ======================================== -->
<!-- Generated: [ISO timestamp] -->
<!-- Skill: beasties -->
<!-- Theme: [full theme description] -->
<!-- Creatures: [N] -->
<!-- Canvas: 60x30 minimum (varied) -->
<!-- Primers: [primers used, csv list, filename, filename] -->
<!-- ======================================== -->

[Full session content with all creatures]
```

### After Saving:

Output the FULL file path at the end so user can click to open:

```
**BESTIARY SAVED TO:**
/Users/james/Repos/infinite-agentic-loop/x_art/bestiary/[filename].txt
```

## USAGE EXAMPLES

User might invoke this skill by saying:

- "Use the beasties skill to generate a bestiary"
- "Generate weird creatures with beasties"
- "Create 20 cosmic horror beasts"
- "Make a medieval bestiary of alien life forms"
- "Beasties: deep sea nightmares, 10 creatures"

## BEGIN GENERATION WORKFLOW

When invoked:

1. **Load primers**:
   - List primer files from skill's `primers/` subdirectory
   - Select 3-15 relevant primers (theme-based or random)
   - Use Read tool to load each selected primer file
2. **Parse request**: Extract theme (if any), creature count (default 20)
3. **Generate session**: Create creatures with Wib&Wob dialogue between each
4. **Save output**: Write complete session to file in bestiary directory
5. **Display path and beastie count**: Show clickable file path and number of beasties drawn

---

**Ready to spawn creatures from wibwobworld** →
