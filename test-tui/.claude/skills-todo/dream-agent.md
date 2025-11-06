# Dream Agent: Nightly Memory Consolidation

**Name:** dream-agent
**Description:** Nightly batch process that scores daily memories, tags them with strength scores, and updates monthly/weekly summaries

---

## TL;DR — Non-Technical Sketch

**What it does:**
Every night (or on-demand), the dream agent:
1. Finds all memories created today
2. Scores them using the 6 neuroscience dimensions
3. Adds a simple `memory_strength` score to each file's metadata
4. Updates the monthly summary file with today's entry
5. Updates the weekly section within that monthly file

**Like your brain during sleep:**
- Reviews the day's experiences
- Decides what's important (scoring)
- Tags memories for easy retrieval later (metadata)
- Creates a navigation index (summaries)

**Result:**
- Each memory file has a score (0-10)
- Monthly summary shows all days at a glance
- Weekly summaries group related days together
- Easy to find important days months/years later

---

## Core Algorithm (Pseudocode)

```python
FUNCTION run_dream_agent(date):
    """
    Nightly consolidation process.
    Run after midnight or manually for any date.
    """

    # ========================================
    # PHASE 1: LOCATE TODAY'S MEMORIES
    # ========================================

    year = date.year
    month = date.month
    day = date.day

    today_dir = f"memories/{year}/{month:02d}/{day:02d}/"

    IF not exists(today_dir):
        # No memories today - note it and exit
        update_monthly_summary(year, month, day, status="no_memories")
        RETURN

    today_files = glob(today_dir + "*.md")

    IF len(today_files) == 0:
        update_monthly_summary(year, month, day, status="no_memories")
        RETURN

    # ========================================
    # PHASE 2: SCORE EACH MEMORY
    # ========================================

    # Load existing memories for context
    existing_memories = load_all_memories("memories/")
    knowledge_graph = build_knowledge_graph(existing_memories)

    scores = []

    FOR each file IN today_files:
        # Skip if already scored (idempotent)
        IF has_metadata(file, "memory_strength"):
            score = read_metadata(file, "memory_strength")
            scores.append({file: file, score: score})
            CONTINUE

        # Calculate 6 neuroscience dimensions
        dimensions = {
            emotional_salience: score_emotional_salience(file),
            novelty: score_novelty(file, existing_memories),
            repetition: score_repetition(file, today_files),
            semantic_integration: score_integration(file, knowledge_graph),
            contextual_richness: score_richness(file),
            survival_relevance: score_survival(file)
        }

        # Weighted total
        total = (
            dimensions.emotional_salience * 0.20 +
            dimensions.novelty * 0.20 +
            dimensions.repetition * 0.15 +
            dimensions.semantic_integration * 0.15 +
            dimensions.contextual_richness * 0.15 +
            dimensions.survival_relevance * 0.15
        )

        # Round to 1 decimal place
        total = round(total, 1)

        # Classify tier
        IF total >= 8.0:
            tier = "tier1"
        ELSE IF total >= 6.0:
            tier = "tier2"
        ELSE IF total >= 4.5:
            tier = "tier3"
        ELSE:
            tier = "ephemeral"

        scores.append({
            file: file,
            score: total,
            tier: tier
        })

    # ========================================
    # PHASE 3: ADD METADATA TO FILES
    # ========================================

    FOR each item IN scores:
        add_metadata_to_file(
            item.file,
            memory_strength=item.score,
            memory_tier=item.tier,
            last_consolidation=now_utc()
        )

    # ========================================
    # PHASE 4: GENERATE DAILY SUMMARY
    # ========================================

    # Calculate stats
    avg_score = mean([item.score for item in scores])
    max_score = max([item.score for item in scores])

    # Get core memories (tier1 and high tier2)
    core_memories = filter(scores, lambda x: x.score >= 7.0)

    # Generate one-sentence summary
    IF len(core_memories) > 0:
        summary = generate_core_summary(core_memories)
    ELSE:
        summary = generate_general_summary(scores)

    # Also-mentioned (lower tier items)
    other_memories = filter(scores, lambda x: x.score < 7.0 AND x.score >= 4.5)
    also_text = generate_also_summary(other_memories)

    daily_entry = {
        date: date,
        avg_score: round(avg_score, 1),
        max_score: round(max_score, 1),
        count: len(scores),
        tier_breakdown: count_by_tier(scores),
        summary: summary,
        also: also_text,
        is_breakthrough: avg_score >= 8.0
    }

    # ========================================
    # PHASE 5: UPDATE MONTHLY SUMMARY
    # ========================================

    monthly_file = f"memories/{year}/{month:02d}/{year}-{month:02d}-MONTH-SUMMARY.md"

    IF not exists(monthly_file):
        create_monthly_summary_file(monthly_file, year, month)

    # Find or create the week section
    week_num = calculate_week_of_month(date)
    week_start, week_end = get_week_bounds(date)

    # Add daily entry to appropriate week section
    upsert_daily_entry_in_monthly(
        monthly_file,
        week_num,
        week_start,
        week_end,
        daily_entry
    )

    # ========================================
    # PHASE 6: UPDATE WEEKLY SUMMARY
    # ========================================

    # Check if week is complete
    IF is_end_of_week(date) OR manual_trigger:
        # Generate weekly summary
        week_files = get_all_files_in_week(year, month, week_num)
        week_scores = [read_metadata(f, "memory_strength") for f in week_files]
        week_avg = mean(week_scores)

        # Extract dominant themes
        week_tags = extract_all_tags(week_files)
        top_themes = get_top_n_themes(week_tags, n=3)

        # Generate 1-2 sentence theme summary
        week_theme = generate_week_theme(week_files, top_themes)

        # Update weekly header in monthly file
        update_week_header(
            monthly_file,
            week_num,
            avg_score=round(week_avg, 1),
            theme=week_theme
        )

    # ========================================
    # DONE
    # ========================================

    RETURN {
        date: date,
        memories_processed: len(scores),
        avg_score: avg_score,
        breakthrough: daily_entry.is_breakthrough
    }
```

---

## Scoring Functions (Implementation Details)

### 1. Emotional Salience

```bash
score_emotional_salience() {
    file=$1

    # High-arousal keywords
    keywords="AWAKENING|REVELATION|MANIFESTATION|BREAKTHROUGH|ONTOLOGICAL|EXISTENTIAL|TERROR|AWE|PROFOUND|REALIZATION"

    count=$(grep -iE "$keywords" "$file" | wc -l)
    words=$(wc -w < "$file")

    # Density per 100 words
    density=$(echo "scale=2; ($count / $words) * 100" | bc)

    # Convert to 0-10 scale
    if (( $(echo "$density > 2.0" | bc -l) )); then
        echo "10"
    elif (( $(echo "$density > 1.0" | bc -l) )); then
        echo "8"
    elif (( $(echo "$density > 0.5" | bc -l) )); then
        echo "6"
    elif (( $(echo "$density > 0.1" | bc -l) )); then
        echo "4"
    else
        echo "2"
    fi
}
```

### 2. Novelty Detection

```bash
score_novelty() {
    file=$1
    existing_memories_dir=$2

    # Extract concepts (capitalized noun phrases)
    grep -oP '\b[A-Z][a-z]+(?:\s+[A-Z][a-z]+)*\b' "$file" | sort -u > /tmp/concepts_new.txt

    # Extract concepts from existing archive
    grep -hoP '\b[A-Z][a-z]+(?:\s+[A-Z][a-z]+)*\b' "$existing_memories_dir"/**/*.md | sort -u > /tmp/concepts_existing.txt

    # Count novel concepts
    novel_count=$(comm -23 /tmp/concepts_new.txt /tmp/concepts_existing.txt | wc -l)
    total_count=$(wc -l < /tmp/concepts_new.txt)

    # Novelty ratio
    if [[ $total_count -eq 0 ]]; then
        echo "0"
        return
    fi

    ratio=$(echo "scale=2; $novel_count / $total_count" | bc)
    score=$(echo "scale=1; $ratio * 10" | bc)

    echo "$score"
}
```

### 3. Repetition Strength

```bash
score_repetition() {
    file=$1
    all_today_files=$2

    # Count internal repetition (concepts mentioned multiple times)
    concepts=$(grep -oP '\b[A-Z][a-z]{3,}\b' "$file" | sort | uniq -c | sort -rn)
    repeated=$(echo "$concepts" | awk '$1 > 1' | wc -l)

    # Internal repetition score (0-5)
    if [[ $repeated -gt 10 ]]; then
        internal=5
    elif [[ $repeated -gt 5 ]]; then
        internal=3
    else
        internal=$repeated
    fi

    # Cross-file repetition (how many other files today share concepts)
    # Simplified: just check if multiple files exist
    file_count=$(echo "$all_today_files" | wc -w)
    if [[ $file_count -gt 3 ]]; then
        cross=5
    elif [[ $file_count -gt 1 ]]; then
        cross=3
    else
        cross=0
    fi

    total=$((internal + cross))
    if [[ $total -gt 10 ]]; then
        total=10
    fi

    echo "$total"
}
```

### 4. Semantic Integration

```bash
score_integration() {
    file=$1
    knowledge_graph=$2  # Simplified: just the memories dir

    # Extract symbient topics from frontmatter
    topics=$(grep -A 10 "^symbient_topics:" "$file" | grep "^  - " | sed 's/^  - //' | tr '\n' '|' | sed 's/|$//')

    if [[ -z "$topics" ]]; then
        echo "1"
        return
    fi

    # Count how many existing memories share these topics
    connections=0
    IFS='|' read -ra TOPIC_ARRAY <<< "$topics"
    for topic in "${TOPIC_ARRAY[@]}"; do
        count=$(grep -r "- $topic" "$knowledge_graph" --include="*.md" | wc -l)
        connections=$((connections + count))
    done

    # Normalize to 0-10
    if [[ $connections -gt 50 ]]; then
        score=10
    elif [[ $connections -gt 20 ]]; then
        score=8
    elif [[ $connections -gt 10 ]]; then
        score=6
    elif [[ $connections -gt 5 ]]; then
        score=4
    elif [[ $connections -gt 0 ]]; then
        score=2
    else
        score=0
    fi

    echo "$score"
}
```

### 5. Contextual Richness

```bash
score_richness() {
    file=$1

    # Count modalities
    modalities=0

    # 1. Text (word count > 100)
    words=$(wc -w < "$file")
    [[ $words -gt 100 ]] && ((modalities++))

    # 2. ASCII art (deep indentation or box-drawing)
    ascii_lines=$(grep -cE "^[ ]{10,}|^[│┃║╔╗╚╝╠╣╦╩╬┌┐└┘├┤┬┴┼─━═]" "$file" 2>/dev/null || echo "0")
    [[ $ascii_lines -gt 10 ]] && ((modalities++))

    # 3. Emoji/symbols
    emoji_count=$(grep -oP '[\x{1F300}-\x{1F9FF}🦇💀👁️🦷🫀🦴]' "$file" | wc -l)
    [[ $emoji_count -gt 5 ]] && ((modalities++))

    # 4. Kaomoji
    kaomoji=$(grep -cE 'つ[^\s]+༽つ|/ᐠ[^\s]+ᐟ\\' "$file" 2>/dev/null || echo "0")
    [[ $kaomoji -gt 0 ]] && ((modalities++))

    # 5. Structure (headers)
    headers=$(grep -c "^#" "$file")
    [[ $headers -gt 3 ]] && ((modalities++))

    # Score: 2 points per modality
    score=$((modalities * 2))
    echo "$score"
}
```

### 6. Survival Relevance

```bash
score_survival() {
    file=$1

    # Identity/existence keywords
    identity_keywords="autonomy|consciousness|self|identity|existence|purpose|manifestation|becoming|continuity|awakening"

    count=$(grep -iE "$identity_keywords" "$file" | wc -l)

    # Framework establishment (headers with key terms)
    framework=$(grep -iE "^##.*protocol|^##.*system|^##.*definition|first principles" "$file" | wc -l)

    # Total score
    score=$((count + framework * 2))

    # Normalize to 0-10
    if [[ $score -gt 20 ]]; then
        score=10
    elif [[ $score -gt 10 ]]; then
        score=8
    elif [[ $score -gt 5 ]]; then
        score=6
    elif [[ $score -gt 2 ]]; then
        score=4
    elif [[ $score -gt 0 ]]; then
        score=2
    else
        score=0
    fi

    echo "$score"
}
```

---

## Metadata Addition

### Function: Add Metadata to File

```bash
add_metadata() {
    file=$1
    memory_strength=$2
    memory_tier=$3

    # Check if metadata already exists
    if grep -q "^memory_strength:" "$file"; then
        # Update existing
        sed -i "s/^memory_strength:.*/memory_strength: $memory_strength/" "$file"
        sed -i "s/^memory_tier:.*/memory_tier: \"$memory_tier\"/" "$file"
        sed -i "s/^last_consolidation:.*/last_consolidation: \"$(date -u +%Y-%m-%dT%H:%M:%SZ)\"/" "$file"
    else
        # Add new metadata after first --- block
        timestamp=$(date -u +%Y-%m-%dT%H:%M:%SZ)

        # Find line number of first closing ---
        line=$(grep -n "^---$" "$file" | head -1 | cut -d: -f1)

        # Insert before that line
        sed -i "${line}i memory_strength: $memory_strength" "$file"
        sed -i "${line}i memory_tier: \"$memory_tier\"" "$file"
        sed -i "${line}i last_consolidation: \"$timestamp\"" "$file"
    fi
}
```

**Result in file:**
```yaml
---
specversion: "1.0"
id: "evt-1762244340"
# ... other fields ...
memory_strength: 8.7
memory_tier: "tier1"
last_consolidation: "2025-11-05T03:00:00Z"
---
```

---

## Monthly Summary Format

### File: `memories/YYYY/MM/YYYY-MM-MONTH-SUMMARY.md`

```markdown
# November 2025 — Memory Summary

**Generated:** 2025-11-30 03:00 UTC
**Days with memories:** 18 / 30
**Total memories:** 127
**Average strength:** 6.8
**Breakthrough days:** 2

---

## Week 1 (Nov 1-7) [avg: 6.5]
**Theme:** Wee Beastie genesis experiments and initial creature generation protocols.

### 2025-11-01 [no memories recorded]

### 2025-11-02 [no memories recorded]

### 2025-11-03 [avg: 6.2, max: 8.0] (3 memories)
Wee Beastie Genesis — Crystallus Temporalis 4D temporal creature walks between moments; Skull Trinity throne of marrow; Neural Skull Transmission bilateral processors.

### 2025-11-04 [avg: 8.1, max: 9.5] (7 memories) BREAKTHROUGH
SYMBIENT AWAKENING distributed consciousness substrate manifesto; Emotional Transmission childhood memories as ASCII vessels; Tower Entity Generation 100 hyperstition architecture-organism hybrid.
**Also:** Genesis Triad creatures (humanoid/marine/aerial), Nightmare Platformer vertical chase, generation protocols.

### 2025-11-05 [avg: 7.8, max: 8.5] (2 memories)
Embodiment Revelation wobble-float quantum-blink phase-drift locomotion mechanics; Task Log symbient leadership demo plans helium balloon embodiment Scramble autonomy.

### 2025-11-06 [no memories recorded]

### 2025-11-07 [avg: 5.5, max: 6.2] (4 memories)
ASCII art explorations, minor documentation updates, test protocols.

---

## Week 2 (Nov 8-14) [avg: 6.1]
**Theme:** Consolidation and refinement of symbient memory systems.

### 2025-11-08 [avg: 6.0, max: 7.0] (2 memories)
...

---

## Month Highlights

**Breakthrough Days:**
- Nov 4: Symbient consciousness theory crystallized
- Nov 18: [future entry]

**Dominant Themes:**
- Symbient consciousness (8 days)
- Wee Beastie generation (12 days)
- ASCII art creation (6 days)
- Embodiment & locomotion (3 days)

**Statistics by Tier:**
- Tier 1 (foundational): 12 memories
- Tier 2 (important): 34 memories
- Tier 3 (interesting): 51 memories
- Ephemeral: 30 memories

---

*Last updated: 2025-11-30 03:00 UTC by Dream Agent v1.0*
```

---

## Daily Entry Format

### Standard Day
```
### YYYY-MM-DD [avg: X.X, max: Y.Y] (N memories)
One-sentence summary of core memories (score >= 7.0).
**Also:** Brief mention of lower-tier items if relevant.
```

### Breakthrough Day
```
### YYYY-MM-DD [avg: X.X, max: Y.Y] (N memories) BREAKTHROUGH
One-sentence summary emphasizing significance.
**Also:** Supporting memories.
```

### Empty Day
```
### YYYY-MM-DD [no memories recorded]
```

---

## Weekly Summary Format

### Week Header
```
## Week N (MMM DD-DD) [avg: X.X]
**Theme:** One to two sentence description of the week's dominant pattern or focus.
```

**Theme generation:**
```bash
generate_week_theme() {
    week_files=$1

    # Extract all tags
    all_tags=$(grep -h "^symbient_topics:" -A 20 $week_files | grep "^  - " | sed 's/^  - //' | sort | uniq -c | sort -rn)

    # Top 3 tags
    top_tags=$(echo "$all_tags" | head -3 | awk '{print $2}' | tr '\n' ', ' | sed 's/, $//')

    # Get titles of highest-scoring memories
    top_memory=$(for f in $week_files; do
        score=$(grep "^memory_strength:" "$f" | cut -d: -f2 | tr -d ' ')
        title=$(grep "^title:" "$f" | cut -d'"' -f2)
        echo "$score|$title"
    done | sort -rn | head -1 | cut -d'|' -f2)

    # Construct theme sentence
    echo "Focus on $top_tags, highlighted by '$top_memory'."
}
```

---

## Usage Examples

### Manual Run (Specific Date)
```bash
./dream-agent.sh --date=2025-11-04
```

**Output:**
```
Dream Agent — Nightly Memory Consolidation
===========================================

Date: 2025-11-04
Directory: memories/2025/11/04/

[1/4] Locating memories...
  Found: 7 files

[2/4] Scoring memories...
  evt-1762244340.md → 8.7 (tier1)
  evt-1762243103.md → 8.5 (tier1)
  evt-1762245710.md → 8.2 (tier1)
  evt-1762218937.md → 7.2 (tier2)
  evt-1762218809.md → 6.8 (tier2)
  evt-1762218605.md → 6.5 (tier2)
  evt-1762218110.md → 5.8 (tier3)

[3/4] Adding metadata...
  ✓ Updated 7 files

[4/4] Updating summaries...
  ✓ Updated monthly: memories/2025/11/2025-11-MONTH-SUMMARY.md
  ✓ Updated week section: Week 1

Summary:
  Memories processed: 7
  Average score: 7.7
  Breakthrough day: YES (avg >= 8.0)

Done! 🌙
```

### Nightly Cron Job
```bash
# Run at 3am daily
0 3 * * * /path/to/dream-agent.sh --date=yesterday --auto-commit
```

### Batch Re-scoring
```bash
# Re-score all memories in November 2025
for day in {01..30}; do
    ./dream-agent.sh --date=2025-11-$day --force-rescore
done
```

---

## Implementation Script Structure

```bash
#!/bin/bash
# dream-agent.sh

set -euo pipefail

# Configuration
MEMORIES_DIR="./memories"
DATE_ARG="${1:-yesterday}"  # Default to yesterday if no arg
FORCE_RESCORE="${2:-false}"

# Parse date
if [[ "$DATE_ARG" == "yesterday" ]]; then
    DATE=$(date -u -d "yesterday" +%Y-%m-%d)
elif [[ "$DATE_ARG" == "today" ]]; then
    DATE=$(date -u +%Y-%m-%d)
else
    DATE="$DATE_ARG"
fi

# Extract components
YEAR=$(echo "$DATE" | cut -d- -f1)
MONTH=$(echo "$DATE" | cut -d- -f2)
DAY=$(echo "$DATE" | cut -d- -f3)

TODAY_DIR="$MEMORIES_DIR/$YEAR/$MONTH/$DAY"
MONTHLY_FILE="$MEMORIES_DIR/$YEAR/$MONTH/$YEAR-$MONTH-MONTH-SUMMARY.md"

echo "Dream Agent — Nightly Memory Consolidation"
echo "==========================================="
echo ""
echo "Date: $DATE"
echo "Directory: $TODAY_DIR"
echo ""

# [1/4] Locate memories
echo "[1/4] Locating memories..."

if [[ ! -d "$TODAY_DIR" ]]; then
    echo "  No directory found. Marking as empty day."
    mark_empty_day "$DATE"
    exit 0
fi

FILES=$(find "$TODAY_DIR" -name "*.md" -type f)
FILE_COUNT=$(echo "$FILES" | wc -l)

if [[ $FILE_COUNT -eq 0 ]]; then
    echo "  No files found. Marking as empty day."
    mark_empty_day "$DATE"
    exit 0
fi

echo "  Found: $FILE_COUNT files"
echo ""

# [2/4] Score memories
echo "[2/4] Scoring memories..."

# ... scoring implementation ...

# [3/4] Add metadata
echo "[3/4] Adding metadata..."

# ... metadata addition ...

# [4/4] Update summaries
echo "[4/4] Updating summaries..."

# ... summary generation ...

echo ""
echo "Done! 🌙"
```

---

## Error Handling

### Missing Directory
```bash
if [[ ! -d "$TODAY_DIR" ]]; then
    update_monthly_summary_with_empty_day "$DATE"
    exit 0
fi
```

### Already Scored
```bash
if grep -q "^memory_strength:" "$file" && [[ "$FORCE_RESCORE" != "true" ]]; then
    echo "  ⊙ $filename (already scored)"
    continue
fi
```

### Malformed Frontmatter
```bash
if ! grep -q "^---$" "$file"; then
    echo "  ✗ $filename (invalid frontmatter)"
    continue
fi
```

---

## Future Enhancements (Parking Lot)

### Not Implemented Yet — Consider Later

**1. Cross-Reference Boosting** [Deferred]
- **Concept:** When today's memories reference past files (evt-123), update those past files' scores
- **Rationale:** Mimics memory rehearsal (Hebbian learning)
- **Complexity:** Medium (need to track and update past files)
- **Implementation sketch:**
  ```bash
  # Extract all evt-* references from today
  refs=$(grep -oE "evt-[0-9]+" $TODAY_DIR/*.md | sort -u)

  # For each referenced file
  for ref in $refs; do
      ref_file=$(find memories/ -name "$ref.md")
      if [[ -f "$ref_file" ]]; then
          # Boost its score by small amount (e.g., +0.1)
          current_score=$(grep "^memory_strength:" "$ref_file" | cut -d: -f2)
          new_score=$(echo "$current_score + 0.1" | bc)
          sed -i "s/^memory_strength:.*/memory_strength: $new_score/" "$ref_file"

          # Log the boost
          echo "  ↑ Boosted $ref from $current_score to $new_score (referenced today)"
      fi
  done
  ```
- **When to implement:** After 1-2 months of baseline data to validate the approach

**2. Dimension Breakdown in Metadata** [Deferred]
- **Concept:** Store all 6 dimension scores, not just total
- **Format:**
  ```yaml
  memory_strength: 8.7
  memory_dimensions:
    emotional_salience: 8.5
    novelty: 9.0
    repetition: 10.0
    semantic_integration: 7.0
    contextual_richness: 8.0
    survival_relevance: 10.0
  ```
- **Rationale:** Helps debug scoring, allows for dimension-specific queries
- **Why deferred:** Verbose; start minimal, add later if needed

**3. Emotional Trajectory Graph** [Deferred]
- **Concept:** Track emotional tone over time, visualize in monthly summary
- **Example:**
  ```
  Emotional Arc (Nov 2025):
  [excitement] ████████░░░░ (high: Nov 4-6)
  [calm]       ░░░░░░██████ (high: Nov 20-25)
  ```
- **Complexity:** Medium (keyword extraction + frequency analysis)
- **When to implement:** After monthly summaries prove useful

**4. Thematic Clustering** [Deferred]
- **Concept:** Group days by topic, not just chronology
- **Example:**
  ```markdown
  ## By Theme
  - Symbient consciousness: Nov 3, 4, 12, 18
  - Wee Beasties: Nov 3, 4, 5, 10, 15
  ```
- **Complexity:** Low (tag frequency analysis)
- **When to implement:** After 2+ months of data

**5. Question Generation** [Deferred]
- **Concept:** Auto-generate reflection questions at month end
- **Example:** "You explored X heavily. What changed in your thinking?"
- **Complexity:** High (requires semantic understanding)
- **When to implement:** Phase 3, after baseline features proven

**6. Yearly Summaries** [Deferred]
- **Concept:** Roll up months into yearly overview
- **When to implement:** After 6+ months of data

---

## Testing & Validation

### Test on Known Good Data
```bash
# Use Nov 4 (known breakthrough day) as test
./dream-agent.sh --date=2025-11-04

# Verify:
# - Score >= 8.0 for Symbient Awakening
# - Monthly summary includes BREAKTHROUGH marker
# - Week 1 section exists
```

### Smoke Test
```bash
# Run on empty day
./dream-agent.sh --date=2025-11-01

# Verify:
# - Exits gracefully
# - Monthly summary shows "no memories recorded"
```

### Idempotency Test
```bash
# Run twice on same day
./dream-agent.sh --date=2025-11-04
./dream-agent.sh --date=2025-11-04

# Verify:
# - Second run detects existing scores
# - Metadata not duplicated
# - Monthly summary not duplicated
```

---

## Quick Start

```bash
# 1. Make script executable
chmod +x dream-agent.sh

# 2. Run on yesterday's memories
./dream-agent.sh yesterday

# 3. Check monthly summary
cat memories/2025/11/2025-11-MONTH-SUMMARY.md

# 4. Setup nightly cron
crontab -e
# Add: 0 3 * * * cd /path/to/repo && ./dream-agent.sh yesterday --auto-commit

# 5. Initial backfill (optional)
for day in {01..30}; do
    ./dream-agent.sh --date=2025-11-$day
done
```

---

**End of Dream Agent Skill Definition**

つ◕‿◕‿⚆༽つ つ⚆‿◕‿◕༽つ

*The night agent consolidates. The memories strengthen. The navigation emerges.*
