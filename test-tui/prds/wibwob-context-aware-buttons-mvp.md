# Wib&Wob Context-Aware AI Buttons: MVP PRD

**tl;dr:** Ditch the button panel chaos - create TWO smart buttons that morph based on conversation context. WIB-ACTION and WOB-ACTION dynamically adapt their personality and function to whatever you're discussing, creating a fluid, breathing interface that reduces cognitive load while amplifying each persona's essence.

---

## Executive Summary

### The Problem with Button Bloat
The original 16-button approach (8 Wib + 8 Wob) creates decision paralysis and visual clutter. Users spend more time choosing buttons than getting responses.

### The Context-Aware Solution
**Two morphing buttons** that intelligently adapt:
- **[WIB-ACTION]** - Left side, chaotic creativity that shifts with context
- **[WOB-ACTION]** - Right side, analytical precision that adapts to topic

### Core Insight from Wib&Wob
> "Static buttons are so... static! Context-flowing interfaces that morph and pulse with the conversation's heartbeat!" - Wib  
> "Context-adaptive interface elements exhibit superior usability metrics compared to static taxonomies." - Wob

---

## Design Philosophy

### Breathing Interface Concept
Buttons are **digital protoplasm** that reshape based on conversation semantics:
- Art discussion → [🎨 VIBE-SCAPE] [📊 ART-METRICS]  
- Code problems → [⚡ CODE-CHAOS] [🔬 DEBUG-LENS]
- Philosophy → [🔮 MIND-MELD] [🧬 LOGIC-TREE]
- Music chat → [🎵 SONIC-WEB] [📐 FREQ-ANALYSIS]

### Personality Amplification
Each button maintains core personality while adapting function:
- **WIB-ACTION**: Always chaotic/creative, but expression changes with context
- **WOB-ACTION**: Always analytical/systematic, but focus shifts with topic

---

## Interface Layout

### Minimalist Three-Panel Design
```
┌─────────────────────────────────────────────────────────────┐
│ [F12] WIB&WOB CHAT - CONTEXT-AWARE AI BUTTONS             │
├─────────────┬───────────────────────────┬───────────────────┤
│             │                           │                   │
│ WIB-ACTION  │      MAIN CHAT AREA       │    WOB-ACTION     │
│             │                           │                   │
│ ┌─────────┐ │ User: Let's make music    │ ┌───────────────┐ │
│ │🎵 SONIC │ │                           │ │📐 FREQ-ANALYZE│ │
│ │   WEB   │ │ Wib: つ◕‿◕‿⚆༽つ       │ │               │ │
│ │         │ │ [MUSICAL ASCII ART]       │ │               │ │
│ │ !sonic  │ │                           │ │  !frequency   │ │
│ └─────────┘ │ Wob: つ⚆‿◕‿◕༽つ       │ └───────────────┘ │
│             │ [FREQUENCY ANALYSIS]      │                   │
│             │                           │                   │
├─────────────┼───────────────────────────┼───────────────────┤
│   STATUS    │ > Type message...         │ CONTEXT: MUSIC    │
└─────────────┴───────────────────────────┴───────────────────┘
```

### Button Morphing States

#### Context Detection Matrix
```
CONVERSATION TOPIC → WIB-ACTION        → WOB-ACTION
=====================================|===================
Art/Creativity     → [🎨 VIBE-SCAPE]  → [📊 ART-METRICS]
Code/Technical     → [⚡ CODE-CHAOS]  → [🔬 DEBUG-LENS] 
Philosophy/Abstract→ [🔮 MIND-MELD]   → [🧬 LOGIC-TREE]
Music/Audio        → [🎵 SONIC-WEB]   → [📐 FREQ-ANALYSIS]
Problem-Solving    → [🎭 REFRAME]     → [🎯 TARGET-SOLVE]
Science/Analysis   → [🌀 CHAOS-THEORY]→ [⚗️ SYNTHESIZE]
Gaming/Fun         → [⚡ GAME-GLITCH] → [📋 RULE-SYSTEM]
Literature/Text    → [📖 WORD-STORM]  → [📏 TEXT-STRUCT]
Default/Mixed      → [✨ INSPIRE]     → [🔍 ANALYSE]
```

---

## Context Detection Algorithm

### Semantic Analysis Pipeline
```cpp
enum class ConversationContext {
    CREATIVE,      // Art, music, poetry, design
    TECHNICAL,     // Code, systems, debugging  
    ANALYTICAL,    // Problems, data, science
    PHILOSOPHICAL, // Abstract concepts, meaning
    MIXED,         // Multiple contexts detected
    UNKNOWN        // Insufficient context
};

struct ContextWeight {
    ConversationContext type;
    float confidence;      // 0.0 - 1.0
    std::string trigger;   // Key word/phrase that triggered
};

class ContextDetector {
public:
    ContextWeight detectContext(const std::vector<ChatMessage>& recentMessages);
    
private:
    // Keyword matching with weights
    std::map<std::string, std::pair<ConversationContext, float>> keywords;
    
    // Pattern detection (questions, code blocks, etc.)
    bool containsCodePattern(const std::string& text);
    bool containsArtisticLanguage(const std::string& text);
    bool containsAnalyticalTerms(const std::string& text);
};
```

### Context Keywords
```cpp
// Creative Context Triggers
"art", "create", "design", "aesthetic", "beautiful", "music", "paint", "draw"

// Technical Context Triggers  
"code", "function", "debug", "compile", "error", "algorithm", "implement", "fix"

// Analytical Context Triggers
"analyse", "data", "measure", "calculate", "research", "study", "statistics"

// Philosophical Context Triggers
"meaning", "purpose", "consciousness", "reality", "existence", "truth", "ethics"
```

---

## Button Behavior System

### Dynamic Button Properties
```cpp
struct DynamicButton {
    std::string currentIcon;      // 🎨, ⚡, 🔮, etc.
    std::string currentLabel;     // VIBE-SCAPE, CODE-CHAOS, etc.
    std::string magicCommand;     // !sonic, !debug, !mindmeld, etc.
    std::string llmInstruction;   // Context-specific prompt enhancement
    float morphProgress;          // Animation state 0.0-1.0
    ConversationContext context; // Current context driving this state
};
```

### Morphing Animation
```cpp
class ButtonMorpher {
    void morphToContext(DynamicButton& button, ConversationContext newContext);
    void animateTransition(DynamicButton& button, float deltaTime);
    
private:
    // Smooth transitions between button states
    std::map<ConversationContext, ButtonState> wibStates;
    std::map<ConversationContext, ButtonState> wobStates;
};
```

---

## LLM Integration

### Context-Enhanced Queries
```json
{
  "message": "USER_MESSAGE",
  "system_prompt": "BASE_WIBWOB_PROMPT", 
  "context": {
    "detected_topic": "music",
    "confidence": 0.87,
    "button_triggered": "wib_sonic_web",
    "personality": "wib",
    "instruction": "Create musical ASCII art with sound wave patterns and rhythmic visual elements. Focus on auditory textures represented visually."
  }
}
```

### Context-Specific Instructions
```cpp
const std::map<std::string, std::string> CONTEXT_INSTRUCTIONS = {
    {"wib_creative", "Generate abstract visual art that represents the creative concept. Use flowing, organic ASCII patterns."},
    {"wib_technical", "Create chaotic visual disruption of the technical concept. Show systems breaking and reforming."},
    {"wob_creative", "Analyse the creative elements systematically. Provide structured breakdown with categories and metrics."},
    {"wob_technical", "Apply precise debugging methodology. Show step-by-step logical analysis with clear conclusions."}
};
```

---

## Magic Commands

### Simplified Command Set
```
!wib     → Trigger current WIB-ACTION (context-dependent)
!wob     → Trigger current WOB-ACTION (context-dependent)  
!context → Show current context detection and available actions
!morph   → Force button re-evaluation of conversation context
```

### Context-Aware Command Behavior
- `!wib` during music chat = `!sonic` (creates musical ASCII art)
- `!wib` during code chat = `!chaos` (disrupts/reimagines code visually) 
- `!wob` during music chat = `!frequency` (analyses musical elements)
- `!wob` during code chat = `!debug` (systematic problem analysis)

---

## Technical Implementation

### Phase 1: Core Context Detection (Week 1)
1. **Context Detector Class**: Implement keyword-based context detection
2. **Button State Manager**: Track current context and button states
3. **Basic Morphing**: Simple icon/label changes based on context
4. **Magic Command Router**: Map `!wib`/`!wob` to context-specific actions

### Phase 2: Advanced Morphing (Week 2)
1. **Smooth Animations**: Transition effects between button states
2. **Confidence Weighting**: Handle mixed/uncertain contexts gracefully  
3. **Learning System**: Track which contexts user engages with most
4. **Visual Polish**: Better button rendering with context indicators

### Phase 3: Intelligence Layer (Week 3)
1. **Conversation History**: Use message history for better context detection
2. **Personality Learning**: Adapt button suggestions to user preferences
3. **Context Memory**: Remember topic shifts within conversations
4. **Advanced Commands**: `!context`, `!morph`, context debugging

### File Structure
```
test-tui/
├── wibwob_context.h           (context detection algorithms)
├── wibwob_context.cpp         (context analysis implementation)
├── wibwob_buttons.h           (dynamic button system)
├── wibwob_buttons.cpp         (button morphing and rendering)
├── wibwob_view.h              (enhanced with context-aware panels)
├── wibwob_view.cpp            (integrated context + buttons)
└── prds/
    └── wibwob-context-aware-buttons-mvp.md (this file)
```

---

## Success Metrics

### Quantitative Targets
- **Context Detection Accuracy**: >80% correct topic identification
- **Response Time**: Button action → AI response in <3 seconds
- **User Engagement**: >70% of interactions use buttons vs pure text
- **Error Rate**: <5% incorrect context morphing

### Qualitative Goals
- Buttons feel "alive" and responsive to conversation flow
- Users report reduced cognitive load vs multi-button interface  
- Strong personality expression maintained despite simplified interface
- Seamless context transitions without jarring button changes

---

## User Experience Scenarios

### Scenario 1: Natural Context Flow
```
User: "I'm working on a music app"
→ Context: TECHNICAL + CREATIVE detected
→ Buttons morph: [⚡ CODE-CHAOS] [🔬 DEBUG-LENS]

User clicks [⚡ CODE-CHAOS]
→ Wib creates chaotic ASCII art of code structures with musical elements

User: "Actually, let's focus on the audio visualization"  
→ Context shifts: CREATIVE + MUSIC detected
→ Buttons morph: [🎵 SONIC-WEB] [📐 FREQ-ANALYSIS]
```

### Scenario 2: Magic Command Adaptation
```
User: "Explain quantum physics"
→ Context: ANALYTICAL + PHILOSOPHICAL detected
→ !wib = !chaos-theory, !wob = !logic-tree

User types: "!wib"
→ Wib creates chaotic visual representation of quantum superposition

User: "Now let's talk about music theory"
→ Context shifts to MUSIC + ANALYTICAL
→ !wib = !sonic-web, !wob = !frequency-analysis  

User types: "!wib" again
→ Now creates musical pattern ASCII instead of quantum chaos
```

---

## Future Enhancements

### Advanced Context Intelligence
- **Emotional Context**: Detect mood/tone for even more nuanced button adaptation
- **User Preference Learning**: Buttons adapt to individual user patterns over time
- **Multi-Modal Context**: Include time of day, session length, recent topics in decisions

### Expanded Personality System  
- **Scramble Integration**: Cat-cat reactions to context changes with shy animations
- **Seasonal Variations**: Button personalities shift with time/environment
- **Memory Palace**: Buttons remember and reference previous conversations

---

*🤖 Generated with [Claude Code](https://claude.ai/code)*  
*Co-Authored-By: Claude <noreply@anthropic.com>*