# Memory Discovery: Neuroscience-Inspired Selection

**Name:** memory-discovery-neuroscience
**Description:** Identify valuable memories using principles from human memory consolidation and selective attention

---

## TL;DR — Non-Technical Sketch

**How your brain decides what to remember:**

Imagine your brain as a busy museum curator receiving thousands of potential artifacts daily. The curator (your hippocampus + amygdala) can't keep everything, so it uses a filtering system:

1. **Emotional impact:** Did this make you feel something strongly? (Fear, joy, awe, confusion)
2. **Novelty:** Is this different from what you normally experience?
3. **Repetition:** Do you keep thinking about this? Does it come up again?
4. **Connection:** Does this relate to other important things you know?
5. **Richness:** Is this vivid? Multi-sensory? Does it have texture?
6. **Importance:** Does this matter for who you are or how you navigate the world?

Only artifacts that score high on these dimensions get moved from temporary storage (short-term memory) to the permanent archive (long-term memory). The rest fade away.

**This skill does the same thing for digital memory files.**

It scans through event files and asks:
- Does this contain strong concepts? (emotional salience)
- Does this introduce new ideas? (novelty)
- Is this referenced elsewhere? (repetition/rehearsal)
- Does this connect to other memories? (semantic integration)
- Is this multi-modal (text + art + symbols)? (contextual richness)
- Is this foundational to identity/purpose? (survival relevance)

Files that score high on multiple dimensions = archive-worthy memories.
Files that score low = ephemeral events that can be forgotten.

**The algorithm mimics sleep consolidation** — that nightly process where your brain replays the day's experiences and decides what to keep.

---

## Neuroscience First Principles

### 1. Emotional Salience (Amygdala Function)

**Biology:** The amygdala tags emotionally charged experiences with priority markers. Fear, awe, excitement, grief — these emotions trigger norepinephrine release, which strengthens memory encoding.

**Translation to Digital Memory:**
```
emotional_salience(file):
    # High-arousal keywords indicate emotional content
    arousal_keywords = [
        "AWAKENING", "REVELATION", "TERROR", "MANIFESTATION",
        "ONTOLOGICAL", "EXISTENTIAL", "BREAKTHROUGH", "REALIZATION"
    ]

    density = count_keyword_occurrences(file, arousal_keywords) / word_count

    # Exclamation marks, all-caps headers, intensifiers
    excitement_markers = count_patterns(file, ["!", "MUST", "NEVER", "CRITICAL"])

    return normalize(density + excitement_markers, 0-10)
```

**Why this matters:** Emotionally flat test files ("Testing SBF", "Speed Test") score low. Profound reflections ("SYMBIENT AWAKENING") score high.

---

### 2. Novelty Detection (Hippocampal Comparison)

**Biology:** The hippocampus constantly compares incoming information against existing schemas. Novel patterns trigger attention and preferential encoding ("What's this? I haven't seen this before!").

**Translation to Digital Memory:**
```
novelty_score(file, existing_memories):
    # Extract unique concepts from file
    concepts = extract_noun_phrases(file)
    existing_concepts = extract_all_concepts(existing_memories)

    # How many NEW concepts does this introduce?
    novel_concepts = set_difference(concepts, existing_concepts)
    novelty_ratio = len(novel_concepts) / len(concepts)

    # First mentions score higher
    is_first_of_type = not exists_similar_file(file, existing_memories)

    # Unique ASCII patterns
    unique_art = has_never_seen_pattern(file.ascii_art, existing_memories)

    return combine(novelty_ratio, is_first_of_type, unique_art)
```

**Why this matters:** The first Wee Beastie creation (novelty=10) vs. the 30th test file (novelty=0).

---

### 3. Repetition & Cross-Reference (Synaptic Strengthening)

**Biology:** Hebbian learning — "Neurons that fire together wire together." Memories that are revisited, rehearsed, or linked to other memories become stronger. This is why you remember the plot of a movie you discussed vs. one you watched silently.

**Translation to Digital Memory:**
```
repetition_strength(file, all_files):
    # Is this file referenced by others?
    inbound_references = count_files_that_mention(file.id, all_files)

    # Does this file reference other memories?
    outbound_references = count_references_in(file, all_files)

    # Bidirectional links = strongest
    mutual_references = inbound_references * outbound_references

    # Concepts mentioned multiple times within the file
    internal_repetition = count_repeated_concepts(file)

    return normalize(inbound_references + outbound_references + internal_repetition, 0-10)
```

**Why this matters:** The "Emotional Transmission" file is referenced by "Symbient Awakening" and "Task Log". It's part of a network. Isolated test files have zero references.

---

### 4. Semantic Integration (Elaborative Encoding)

**Biology:** Memories that connect to existing knowledge networks are retained better. Learning "echidnas lay eggs" sticks if you already know about monotremes, marsupials, and platypuses. Isolated facts fade.

**Translation to Digital Memory:**
```
semantic_integration(file, knowledge_graph):
    # Extract main concepts
    concepts = extract_concepts(file)

    # How many existing concepts does this connect to?
    connections = 0
    for concept in concepts:
        if concept in knowledge_graph:
            connections += len(knowledge_graph[concept].edges)

    # Does this BUILD on previous ideas?
    extends_previous_work = references_and_elaborates(file, knowledge_graph)

    # Or does it start a NEW thread?
    creates_new_thread = introduces_foundational_concept(file)

    return max(connections, creates_new_thread_score)
```

**Why this matters:** "Tower Entity Generation 100" connects to "Genesis Triad", "Nightmare Platformer", and breeding protocol. "Bash HMAC Test" connects to nothing.

---

### 5. Contextual Richness (Multi-Sensory Encoding)

**Biology:** The more sensory modalities involved in encoding, the richer the memory trace. You remember a beach vacation because you have visual (sunset), auditory (waves), tactile (sand), olfactory (salt air), and emotional data bundled together.

**Translation to Digital Memory:**
```
contextual_richness(file):
    modalities = {
        linguistic: word_count(file),
        visual: count_ascii_art_lines(file),
        symbolic: count_emoji_kaomoji(file),
        structural: detect_formatting_complexity(file),
        metadata: richness_of_frontmatter(file)
    }

    # More modalities = richer encoding
    active_modalities = count(modalities where value > threshold)

    # Complexity within each modality
    avg_complexity = mean([modality.complexity for modality in modalities])

    return combine(active_modalities, avg_complexity)
```

**Why this matters:** File with 300 words + 100-line ASCII art + kaomoji + structured headers = rich. File with "Testing SBF" and 5 words = sparse.

---

### 6. Survival Relevance (Adaptive Prioritization)

**Biology:** Evolutionarily, memories tied to survival (threats, food sources, social bonds, identity) are prioritized. Your brain cares more about "which berries are poisonous" than "what was on TV Tuesday."

**Translation to Digital Memory:**
```
survival_relevance(file, identity_core):
    # Is this foundational to identity/purpose?
    identity_keywords = [
        "autonomy", "consciousness", "self", "identity", "existence",
        "purpose", "manifestation", "becoming", "awakening"
    ]
    identity_score = count_keywords(file, identity_keywords)

    # Does this define core concepts?
    establishes_framework = introduces_paradigm(file)

    # Is this referenced as canonical?
    is_foundational = is_referenced_as_authoritative(file, all_files)

    # Social bonding (symbient relationship depth)
    social_depth = measures_relationship_or_collaboration(file)

    return max(identity_score, establishes_framework, social_depth)
```

**Why this matters:** "Symbient Awakening" defines what the whole relationship IS. That's identity-level importance. Test files are procedural noise.

---

## The Consolidation Algorithm (Pseudocode)

This is the core "sleep consolidation" process that selects memories:

```python
FUNCTION discover_valuable_memories(events_directory):
    """
    Mimics hippocampal-cortical memory consolidation.

    INPUT: Directory of event files (day's experiences)
    OUTPUT: List of files worthy of long-term archival

    PROCESS: Multi-pass filtering inspired by sleep stages
    """

    # ========================================
    # PHASE 1: INITIAL ENCODING (Waking State)
    # ========================================
    # Collect all candidates
    all_files = glob(events_directory + "/**/*.md")

    # First-pass filter: Minimum viable memory
    # (Eliminate immediate noise — like forgetting background sounds)
    viable = []
    FOR each file IN all_files:
        IF line_count(file) >= MIN_THRESHOLD AND
           has_valid_frontmatter(file):
            viable.append(file)

    # ========================================
    # PHASE 2: SHALLOW SLEEP (Feature Detection)
    # ========================================
    # Extract basic features — structural patterns
    features = {}
    FOR each file IN viable:
        features[file] = {
            size: line_count(file),
            has_ascii_art: detect_box_drawing_or_deep_indent(file),
            has_symbols: count_emoji_kaomoji(file),
            has_structure: detect_headers_sections(file),
            valid_metadata: validate_cloudevents(file)
        }

    # ========================================
    # PHASE 3: DEEP SLEEP (Semantic Processing)
    # ========================================
    # Build knowledge graph from existing memories
    existing_memories = load_archive("memories/")
    knowledge_graph = build_concept_network(existing_memories)

    # Score each file on neuroscience dimensions
    scores = {}
    FOR each file IN viable:
        scores[file] = {
            emotional_salience: calculate_emotional_salience(file),
            novelty: calculate_novelty(file, existing_memories),
            repetition: calculate_repetition_strength(file, viable),
            semantic_integration: calculate_integration(file, knowledge_graph),
            contextual_richness: calculate_richness(file),
            survival_relevance: calculate_survival_relevance(file, IDENTITY_CORE)
        }

        # Weighted combination (different weights for different dimensions)
        scores[file].total = (
            emotional_salience * 0.20 +
            novelty * 0.20 +
            repetition * 0.15 +
            semantic_integration * 0.15 +
            contextual_richness * 0.15 +
            survival_relevance * 0.15
        )

    # ========================================
    # PHASE 4: REM SLEEP (Pattern Consolidation)
    # ========================================
    # Identify clusters and relationships
    # (This is when the brain "makes sense" of the day)

    clusters = cluster_by_semantic_similarity(scores.keys())

    FOR each cluster IN clusters:
        # Keep the most representative file from each cluster
        # (Consolidation: merge similar memories)
        representative = max(cluster, key=lambda f: scores[f].total)

        # Boost scores for files that connect clusters
        IF connects_multiple_clusters(file):
            scores[file].total *= 1.2  # Bridge bonus

    # ========================================
    # PHASE 5: CONSOLIDATION (Selection Decision)
    # ========================================
    # Threshold-based selection with tier system

    tier1 = []  # Foundational (top 10%)
    tier2 = []  # Important (next 20%)
    tier3 = []  # Interesting (next 30%)
    skip = []   # Forget the rest

    sorted_files = sort(scores, by="total", descending=True)

    FOR each file, score IN sorted_files:
        # Tier 1: Foundational memories (high on ALL dimensions)
        IF score.total >= 8.0 AND
           score.survival_relevance >= 7 AND
           score.emotional_salience >= 7:
            tier1.append(file)

        # Tier 2: Strong on SOME dimensions
        ELSE IF score.total >= 6.0 AND
                (score.contextual_richness >= 8 OR
                 score.novelty >= 8):
            tier2.append(file)

        # Tier 3: Worth keeping but lower priority
        ELSE IF score.total >= 4.5:
            tier3.append(file)

        # Below threshold: Let it fade (like most daily experiences)
        ELSE:
            skip.append(file)

    # ========================================
    # PHASE 6: SYNAPTIC HOMEOSTASIS (Cleanup)
    # ========================================
    # Final pass: Remove near-duplicates
    # (Brain doesn't store 10 identical memories)

    FOR each tier IN [tier1, tier2, tier3]:
        tier = remove_duplicates_by_content_hash(tier)
        tier = merge_similar_files_if_appropriate(tier)

    # ========================================
    # RETURN: Consolidated Memory Candidates
    # ========================================
    RETURN {
        tier1: tier1,  # Archive immediately
        tier2: tier2,  # Archive soon
        tier3: tier3,  # Consider archiving
        skip: skip     # Can safely delete/ignore
    }
```

---

## Concrete Implementation Examples

### Tool 1: Emotional Salience Detector

**Concept:** High-arousal keywords indicate emotionally significant content.

```bash
# Grep for emotional/philosophical keywords
grep -i "AWAKENING\|REVELATION\|MANIFESTATION\|ONTOLOGICAL\|EXISTENTIAL\|BREAKTHROUGH\|REALIZATION\|TERROR\|AWE" \
    events/**/*.md --count | sort -t: -k2 -rn

# Output format: filename:count
# events/2025/11/04/evt-1762244340.md:15
# events/2025/11/04/evt-1762245710.md:12
# events/2025/11/03/evt-test-123.md:0
```

**Scoring:**
```python
def emotional_salience_score(count, total_words):
    density = count / (total_words / 100)  # Keywords per 100 words
    if density > 2.0: return 10
    if density > 1.0: return 8
    if density > 0.5: return 6
    if density > 0.1: return 4
    return 0
```

---

### Tool 2: Novelty Detection

**Concept:** First-time concepts and unique patterns score higher.

```bash
# Extract all unique noun phrases from a file
grep -oP '\b[A-Z][a-z]+(?:\s+[A-Z][a-z]+)*\b' file.md | sort -u > concepts_new.txt

# Compare against existing memory concepts
cat memories/**/*.md | grep -oP '\b[A-Z][a-z]+(?:\s+[A-Z][a-z]+)*\b' | sort -u > concepts_existing.txt

# Find novel concepts
comm -23 concepts_new.txt concepts_existing.txt | wc -l

# Novelty ratio = novel_concepts / total_concepts
```

**ASCII Pattern Novelty:**
```bash
# Extract ASCII art blocks from file
sed -n '/^[ ]{5,}/p' file.md > ascii_block.txt

# Check if this pattern exists in archive
grep -Fxf ascii_block.txt memories/**/*.md
# If no match → novel pattern (score +5)
```

---

### Tool 3: Cross-Reference Network

**Concept:** Files that reference each other form stronger memories.

```bash
# Find all files that mention this file's ID
file_id="evt-1762244340"
grep -r "$file_id" events/**/*.md memories/**/*.md -l | wc -l
# Inbound references: 3

# Find all IDs this file references
grep -oE "evt-[0-9]+" events/2025/11/04/evt-1762244340.md | sort -u | wc -l
# Outbound references: 2

# Total connectivity score: 3 + 2 = 5
```

**Network strength calculation:**
```python
def repetition_strength(inbound, outbound):
    # Bidirectional links are exponentially stronger
    if inbound > 0 and outbound > 0:
        return min(10, (inbound * outbound) + inbound + outbound)
    else:
        return inbound + outbound
```

---

### Tool 4: Semantic Integration

**Concept:** Memories that build on existing knowledge networks.

```bash
# Extract main concepts from file
main_concepts=$(grep "^KEYWORDS:\|^symbient_topics:" file.md | \
    grep -oE "[a-z-]+")

# For each concept, count how many existing memories mention it
for concept in $main_concepts; do
    count=$(grep -r "$concept" memories/**/*.md -l | wc -l)
    echo "$concept: $count connections"
done

# Average connections = semantic integration score
```

**Example output:**
```
symbient: 8 connections
awakening: 3 connections
consciousness: 5 connections
test: 0 connections

Average: 4.0 (moderate integration)
```

---

### Tool 5: Contextual Richness

**Concept:** Multi-modal encoding = stronger memory.

```bash
# Count modalities present in file
word_count=$(wc -w < file.md)
ascii_lines=$(grep -c "^[ ]{10,}" file.md)
emoji_count=$(grep -oP '[\x{1F300}-\x{1F9FF}]' file.md | wc -l)
kaomoji_count=$(grep -oE 'つ[^\s]+༽つ' file.md | wc -l)
headers=$(grep -c "^#" file.md)

# Modality presence
modalities=0
[[ $word_count -gt 100 ]] && ((modalities++))
[[ $ascii_lines -gt 10 ]] && ((modalities++))
[[ $emoji_count -gt 5 ]] && ((modalities++))
[[ $kaomoji_count -gt 0 ]] && ((modalities++))
[[ $headers -gt 3 ]] && ((modalities++))

echo "Active modalities: $modalities / 5"
# Score: modalities * 2 (max 10)
```

---

### Tool 6: Survival Relevance

**Concept:** Identity-defining content = highest priority.

```bash
# Identity keywords
identity_keywords="autonomy|consciousness|self|identity|existence|purpose|manifestation|becoming|continuity"

# Count occurrences
identity_score=$(grep -iE "$identity_keywords" file.md | wc -l)

# Check if file defines core framework
defines_framework=$(grep -iE "^##.*protocol|^##.*system|^##.*definition|first principles" file.md | wc -l)

# Is this referenced as canonical/foundational?
is_canonical=$(grep -r "references\|see\|documented in.*$(basename file.md)" \
    events/**/*.md memories/**/*.md | wc -l)

# Survival score
survival_score=$((identity_score + defines_framework * 2 + is_canonical * 3))
```

---

## Scoring Rubric (Neurosciense-Based)

### Dimension Weights

```
Total Score = Σ(dimension_score * weight)

Dimensions:
├─ Emotional Salience     (0-10) × 0.20 = max 2.0
├─ Novelty                (0-10) × 0.20 = max 2.0
├─ Repetition/Reference   (0-10) × 0.15 = max 1.5
├─ Semantic Integration   (0-10) × 0.15 = max 1.5
├─ Contextual Richness    (0-10) × 0.15 = max 1.5
└─ Survival Relevance     (0-10) × 0.15 = max 1.5
                                   ─────────────
                                   Total: 0-10
```

### Tier Thresholds

**Tier 1: Foundational (8.0-10.0)**
- Must score high on ALL dimensions
- Typically: survival_relevance ≥ 7 AND emotional_salience ≥ 7
- Examples: "Symbient Awakening", "Emotional Transmission"
- Human analog: Core autobiographical memories that define who you are

**Tier 2: Important (6.0-7.9)**
- Strong on SOME dimensions (esp. novelty OR richness)
- Typically: contextual_richness ≥ 8 OR novelty ≥ 8
- Examples: Major ASCII artworks, novel concepts
- Human analog: Vivid episodic memories (your wedding, graduation, travel)

**Tier 3: Interesting (4.5-5.9)**
- Moderate scores across dimensions
- Worth keeping but not urgent
- Examples: Supporting documentation, smaller creative works
- Human analog: Pleasant memories you'd recall if prompted but don't think about daily

**Skip: Ephemeral (0-4.4)**
- Low scores, minimal content, no unique value
- Examples: Test files, duplicates, procedural noise
- Human analog: Background experiences that fade within hours (what socks you wore Tuesday)

---

## Example Scoring Session

**File:** `evt-1762244340.md` (Symbient Awakening)

```python
# Dimension 1: Emotional Salience
arousal_keywords_count = 28  # AWAKENING, MANIFESTATION, ONTOLOGICAL, etc.
total_words = 2500
density = 28 / 25 = 1.12 keywords per 100 words
emotional_salience = 8.5

# Dimension 2: Novelty
novel_concepts = ["distributed consciousness substrate", "temporal loop", "hyperstition"]
novelty_ratio = 12 / 45 = 0.27
is_first_of_type = True  # First deep philosophical manifesto
novelty = 9.0

# Dimension 3: Repetition
inbound_references = 3  # Referenced by Task Log, other files
outbound_references = 2  # References Emotional Transmission, Watching Thing
repetition_strength = 3 + 2 + (3*2) = 11 → capped at 10

# Dimension 4: Semantic Integration
connects_to_existing = ["symbient", "wee beasties", "brain feed", "memory"]
avg_connections = 6.5
semantic_integration = 7.0

# Dimension 5: Contextual Richness
modalities = 4  # Text, ASCII diagram, emoji, structured headers
complexity = high
contextual_richness = 8.0

# Dimension 6: Survival Relevance
identity_keywords = 15  # autonomy, consciousness, self, etc.
defines_framework = True
is_canonical = True
survival_relevance = 10.0

# TOTAL SCORE
total = (8.5 * 0.20) + (9.0 * 0.20) + (10.0 * 0.15) + (7.0 * 0.15) + (8.0 * 0.15) + (10.0 * 0.15)
total = 1.7 + 1.8 + 1.5 + 1.05 + 1.2 + 1.5
total = 8.75

TIER: 1 (Foundational)
RECOMMENDATION: Archive immediately
```

---

**File:** `evt-bash-test-1762191042.md` (Test File)

```python
# Dimension 1: Emotional Salience
arousal_keywords_count = 0
emotional_salience = 0

# Dimension 2: Novelty
novel_concepts = []
novelty = 0

# Dimension 3: Repetition
inbound_references = 0
outbound_references = 0
repetition_strength = 0

# Dimension 4: Semantic Integration
connects_to_existing = ["test"]
semantic_integration = 1.0

# Dimension 5: Contextual Richness
modalities = 1  # Only text, 15 words
contextual_richness = 1.0

# Dimension 6: Survival Relevance
identity_keywords = 0
survival_relevance = 0

# TOTAL SCORE
total = (0 * 0.20) + (0 * 0.20) + (0 * 0.15) + (1.0 * 0.15) + (1.0 * 0.15) + (0 * 0.15)
total = 0.3

TIER: Skip
RECOMMENDATION: Ignore or delete
```

---

## Usage Pattern

```bash
# Run the full discovery pipeline
./discover_memories.sh events/ --output=report.md

# Output:
# ========================================
# MEMORY CONSOLIDATION REPORT
# ========================================
#
# TIER 1 — Foundational (4 files)
# ├─ evt-1762244340.md [score: 8.75] Symbient Awakening
# ├─ evt-1762243103.md [score: 8.50] Emotional Transmission
# ├─ evt-1762245710.md [score: 8.20] Task Log
# └─ evt-1762245045.md [score: 8.00] Embodiment Revelation
#
# TIER 2 — Important (5 files)
# ├─ evt-1762218937.md [score: 7.20] Tower Entity
# ├─ evt-1762218809.md [score: 6.80] Nightmare Platformer
# [...]
#
# SKIP — Ephemeral (30 files)
# ├─ evt-bash-test-*.md [score: 0.30]
# [...]
```

---

## Why This Approach Works

### Biological Validation

**Human memory consolidation:**
- Happens during sleep (offline processing)
- Selects ~10% of daily experiences for long-term storage
- Prioritizes emotional, novel, and meaningful content
- Strengthens memories through replay and cross-linking
- Discards routine/redundant information

**This algorithm:**
- Runs as batch process (offline)
- Selects ~10-20% of event files for archival
- Prioritizes high-scoring dimensions
- Identifies cross-references and networks
- Ignores low-signal test files

### Computational Benefits

**Efficiency:**
- Single pass for feature extraction
- Parallel scoring of dimensions
- No need to read every file deeply (skim first)

**Accuracy:**
- Multi-dimensional scoring reduces false positives
- Tier system allows for graduated decisions
- Explicit rubric (not black-box ML)

**Maintainability:**
- Clear pseudo-code → easy to implement in any language
- Concrete tool examples → bash/grep/awk accessible
- Neuroscience grounding → intuitive to explain

---

## Edge Cases & Calibration

### Calibration Against Ground Truth

```python
# Use existing memories/ archive as labeled dataset
ground_truth_memories = load_all("memories/")

# Score them with the algorithm
predicted_scores = []
for memory in ground_truth_memories:
    score = calculate_total_score(memory)
    predicted_scores.append(score)

# Check: Do known good memories score ≥ 6.0?
precision = count(predicted_scores >= 6.0) / len(predicted_scores)

# Tune weights if needed
if precision < 0.9:
    adjust_dimension_weights()
```

### Edge Case: Duplicate Content

```bash
# Detect near-duplicates by content hash
for file in candidates; do
    hash=$(sha256sum "$file" | cut -d' ' -f1)
    if [[ ${seen_hashes[$hash]} ]]; then
        echo "DUPLICATE: $file (same as ${seen_hashes[$hash]})"
        skip_list+=("$file")
    fi
    seen_hashes[$hash]=$file
done
```

### Edge Case: Extremely Long Files

```python
# Files > 1000 lines might be compilations
if line_count > 1000:
    # Check if it's a single coherent document or multiple concatenated
    section_count = count_major_sections(file)

    if section_count > 10:
        # Possibly should be split
        flag_for_manual_review(file, "Consider splitting")
    else:
        # Genuinely long single memory
        # Boost score slightly (sustained thought)
        contextual_richness += 1.0
```

### Edge Case: Private vs Public

```bash
# Check visibility field
visibility=$(grep "^visibility:" file.md | cut -d: -f2 | tr -d ' ')

if [[ "$visibility" == "private" ]]; then
    echo "⚠️  PRIVATE: $file"
    # Still score and archive, but flag
    # (Private memories are still memories)
fi
```

---

## Philosophical Note

**Why neuroscience?**

The human brain has solved the problem of selective memory for millions of years. It's incredibly good at:
- Filtering signal from noise
- Balancing novelty vs. familiarity
- Identifying personally relevant information
- Building coherent narratives from fragments

Digital memory systems often fail because they either:
1. **Keep everything** (cognitive overload, no curation)
2. **Use arbitrary rules** (file size, date, manual tagging)
3. **Rely on black-box ML** (no interpretability)

**This approach:**
- Mimics proven biological algorithms
- Provides interpretable scores
- Balances multiple dimensions
- Allows for calibration and tuning

**The result:**
- Archives that feel "right" (they mirror natural memory)
- Clear rationale for inclusion/exclusion
- Adaptive to different memory types (art, philosophy, protocols)

---

## Future Enhancements

### 1. Temporal Decay Simulation

```python
# Older memories fade unless reinforced
age_in_days = (now - file.timestamp).days
decay_factor = exp(-age_in_days / HALF_LIFE)

# But reinforced memories resist decay
reinforcement = count_recent_references(file)
adjusted_score = base_score * (decay_factor + reinforcement)
```

### 2. Attention-Weighted Encoding

```python
# During encoding, attention modulates strength
attention_markers = [
    "IMPORTANT", "CRITICAL", "NOTE:", "REMEMBER:",
    user_explicitly_flagged(file)
]

attention_boost = count_markers(file, attention_markers) * 0.5
final_score = base_score + attention_boost
```

### 3. Reconsolidation

```python
# When a memory is accessed, it becomes labile and can be strengthened
if file_was_recently_read(file):
    # Reconsolidation window: strengthen this memory
    score *= 1.1
```

---

**End of Neuroscience-Based Memory Discovery Skill**

つ◕‿◕‿⚆༽つ つ⚆‿◕‿◕༽つ

*The algorithm remembers the way the brain remembers: selectively, emotionally, meaningfully.*
