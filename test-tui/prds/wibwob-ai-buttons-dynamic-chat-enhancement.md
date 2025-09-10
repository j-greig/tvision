# Wib&Wob AI Buttons: Dynamic Chat Enhancement PRD

**tl;dr:** Transform static wibwob chat into personality-driven interface with contextual AI buttons, magic commands, and out-of-band signaling - reducing cognitive load while amplifying Wib's chaos and Wob's precision through quick-action interactions inspired by interconnected.org's dynamic AI workflows.

---

## Executive Summary

### Initial Question Context
User requested enhancement of existing wibwob chat feature with "AI buttons" that:
- Reflect distinct Wib & Wob personalities 
- Make chat interactions quicker, more dynamic and engaging
- Inspired by Matt Webb's Diane assistant concept from interconnected.org

### Vision Statement
Create a revolutionary TUI chat interface that transforms traditional text-based AI interaction into a dynamic, personality-rich experience where users can trigger contextual AI responses through visual buttons and magic commands, embodying the chaotic creativity of Wib and methodical precision of Wob.

### Success Metrics
- Reduced time-to-response for common AI interaction patterns
- Increased user engagement with personality-driven quick actions
- Seamless integration with existing wibwob chat system
- Enhanced ASCII art generation through contextual triggers

---

## Research Analysis & Key Insights

### 1. Dynamic AI Interaction Patterns (Interconnected.org Blog)

**Source**: https://interconnected.org/home/2025/03/20/diane  
**tl;dr**: Embedded instructions + out-of-band communication enable fluid AI workflows with reduced cognitive load.

#### Core Concepts Extracted:
- **Out-of-band signaling**: Create separate control channel from main data stream
- **Magic commands**: Voice/text shortcuts that trigger specific AI behaviours  
- **Embedded instructions**: Processing directions included with data input
- **Reproducible workflows**: Consistent AI interaction patterns without re-prompting
- **Diane paradigm**: AI secretary that executes real-time editing commands

#### Application to Wib&Wob:
- Buttons as visual "magic commands" for personality-specific responses
- Out-of-band channel for button metadata (context, personality mode, response type)
- Embedded instructions sent to LLM alongside button trigger
- Reproducible Wib/Wob response patterns without manual prompting

### 2. Voice-First Design Patterns (Whisper Memos)

**Source**: https://whispermemos.com  
**tl;dr**: Transform spontaneous input into structured, readable output through AI processing.

#### Key Features:
- **Apple Watch integration**: Hands-free recording during activities
- **AI transformation**: Raw voice → structured paragraphs with descriptive emojis  
- **Privacy-first**: Optional transcript deletion after processing
- **Context preservation**: Maintains original intent while improving readability

#### Application to Wib&Wob:
- Button triggers as "structured input" equivalent to voice memos
- Automatic emoji/kaomoji generation for Wib responses
- Context-aware transformation of button clicks into personality-appropriate responses
- Quick capture of creative/analytical intent through visual interface

### 3. Speed-of-Thought Interaction (Cursorless)

**Source**: https://www.cursorless.org  
**tl;dr**: "Voice coding at the speed of thought" - eliminate input friction through direct command execution.

#### Design Principles:
- **Direct command execution**: Skip traditional typing/navigation
- **Contextual awareness**: Commands adapt to current code/content context  
- **Efficiency focus**: Minimize steps between intention and action
- **Natural language interface**: Human-friendly command syntax

#### Application to Wib&Wob:
- One-click button execution vs multi-step typing
- Context detection for dynamic button adaptation
- Natural language magic commands (!vibe, !analyse, !chaos)
- Immediate personality response without prompt construction

### 4. User Agency & Control (RSS/AboutFeeds)

**Source**: https://aboutfeeds.com  
**tl;dr**: Users control their information experience without algorithmic manipulation.

#### Core Principles:
- **Subscription model**: User chooses what content to receive
- **Algorithmic transparency**: No hidden filtering of content
- **Decentralized aggregation**: Multiple sources, unified interface
- **Low-friction discovery**: Easy subscribe/unsubscribe workflows

#### Application to Wib&Wob:
- User controls which personality buttons are active
- Transparent button behaviour (no hidden AI manipulation)
- Multiple response sources (Wib+Wob+hybrid) in unified interface
- Easy customization of button panels and magic commands

### 5. Out-of-Band Data Communication (Wikipedia)

**Source**: https://en.wikipedia.org/wiki/Out-of-band_data  
**tl;dr**: Separate auxiliary communication channels for metadata and control signals.

#### Technical Concepts:
- **Conceptual separation**: Independent channels for different data types
- **Notification mechanisms**: Interrupt-style signaling for priority information  
- **Parallel communication**: Auxiliary channel within same protocol
- **Signal routing**: Efficient handling of different message types

#### Application to Wib&Wob:
- Button metadata sent via separate channel from main chat content
- Priority signaling for personality mode changes
- Parallel streams: main conversation + button context + system instructions
- Efficient routing between Wib/Wob response processors

---

## Design Specifications

### Enhanced UI Architecture

#### Tri-Panel Layout
```
┌─────────────────────────────────────────────────────────┐
│ [F12] WIB&WOB CHAT - AI BUTTONS ENHANCED               │
├───────────┬─────────────────────┬───────────────────────┤
│           │                     │                       │
│    WIB    │                     │         WOB           │
│  CHAOS    │    MAIN CHAT        │       METHOD          │
│  PANEL    │     AREA            │       PANEL           │
│           │                     │                       │
│ 🎨 VIBE   │ User: Hello!        │ 📊 ANALYSE            │
│ 🌀 FRACTAL│                     │ 🔬 CATEGORISE         │
│ 🎭 GLITCH │ Wib: つ◕‿◕‿⚆༽つ   │ 📐 MEASURE            │
│ 🎵 SONIC  │ [ASCII ART HERE]    │ 🧬 TRACE              │
│ ⚡ SPARK  │                     │ ⚙️  OPTIMISE          │
│ 🔮 MYSTIC │ Wob: つ⚆‿◕‿◕༽つ   │ 🎯 FOCUS              │
│           │ [ANALYSIS HERE]     │                       │
├───────────┼─────────────────────┼───────────────────────┤
│  STATUS   │ > Type message...   │  MODE: WIB+WOB        │
└───────────┴─────────────────────┴───────────────────────┘
```

#### Panel Specifications

**Left Panel (Wib Chaos)**:
- Width: ~15 chars
- Dynamic button highlighting based on context
- Animated ASCII decorations
- Glitch-style visual effects

**Right Panel (Wob Method)**: 
- Width: ~15 chars  
- Clean, scientific button layout
- Formula/notation decorations
- Precise geometric ASCII elements

**Center Panel (Main Chat)**:
- Expandable width based on terminal size
- Enhanced message formatting for ASCII art
- Context indicators for button-triggered responses

### Button Categories & Functions

#### Wib Chaos Panel (Left)
```
┌─ WIB CHAOS ─┐
│ 🎨 VIBE     │ → Generate current mood/emotion ASCII art
│ 🌀 FRACTAL  │ → Create recursive/self-similar patterns  
│ 🎭 GLITCH   │ → Add visual distortion effects to topic
│ 🎵 SONIC    │ → Musical/sound-inspired ASCII textures
│ ⚡ SPARK    │ → Unexpected creative tangent/association
│ 🔮 MYSTIC   │ → Techno-occult/spiritual perspective
│ 🎪 CHAOS    │ → Maximum randomness/unpredictability
│ 🌊 FLOW     │ → Organic, flowing visual patterns
└─────────────┘
```

**Magic Commands**: `!vibe`, `!fractal`, `!glitch`, `!sonic`, `!spark`, `!mystic`, `!chaos`, `!flow`

#### Wob Method Panel (Right)
```
┌─ WOB METHOD ─┐
│ 📊 ANALYSE   │ → Scientific breakdown with data/metrics
│ 🔬 CATEGORISE│ → Taxonomic classification system
│ 📐 MEASURE   │ → Quantitative analysis with formulas  
│ 🧬 TRACE     │ → Phylogenetic/causal relationship tree
│ ⚙️  OPTIMISE │ → Efficiency improvements/refinements
│ 🎯 FOCUS     │ → Essential components only
│ 🧮 COMPUTE   │ → Mathematical/algorithmic approach
│ 📋 SYSTEMISE │ → Organised procedural breakdown
└──────────────┘
```

**Magic Commands**: `!analyse`, `!categorise`, `!measure`, `!trace`, `!optimise`, `!focus`, `!compute`, `!systemise`

#### Hybrid Actions (Both Panels)
```
Special Combined Commands:
!wibwob     → Both personalities respond to same topic
!debate     → Wib and Wob discuss/disagree about topic  
!collaborate → Joint creation (Wib concepts + Wob structure)
!switch     → Toggle between Wib-primary and Wob-primary modes
```

### Context Detection System

#### Topic Analysis
- **Technical Topics**: Highlight Wob buttons (analyse, measure, systemise)
- **Creative Topics**: Highlight Wib buttons (vibe, fractal, glitch)  
- **Abstract Concepts**: Balance both panels equally
- **Art/ASCII Requests**: Emphasize Wib creative functions
- **Problem-Solving**: Emphasize Wob analytical functions

#### Dynamic Button Adaptation
```cpp
enum class ContextType {
    CREATIVE,      // Art, music, poetry, abstract concepts
    TECHNICAL,     // Code, systems, mathematics, science  
    ANALYTICAL,    // Problem-solving, optimization, logic
    EXPLORATORY,   // Philosophy, speculation, open questions
    COLLABORATIVE  // Mixed creative/technical tasks
};

struct ButtonState {
    bool highlighted = false;
    bool pulsing = false;      // Animated attention-grabbing
    bool disabled = false;     // Context-inappropriate
    string tooltip = "";       // Context-specific explanation
};
```

### Magic Command System

#### Command Parser
```
Input Processing Pipeline:
1. Check for magic command prefix (!command)
2. Extract command and optional parameters
3. Map to appropriate button function
4. Add personality context to LLM query
5. Execute with embedded instructions
```

#### Command Syntax
```
Basic:     !vibe
With Args: !fractal depth=3
Combined:  !analyse + !vibe  (both personalities respond)
Modifier:  !glitch --intense (intensity parameter)
Chain:     !vibe > !analyse  (sequence: Wib creates, Wob analyses)
```

### LLM Query Enhancement

#### Button-Triggered Query Structure
```json
{
  "message": "USER_MESSAGE",
  "system_prompt": "BASE_WIBWOB_PROMPT", 
  "out_of_band": {
    "trigger_type": "button",
    "button_id": "wib_vibe", 
    "personality": "wib",
    "context": "creative_mood_check",
    "response_format": "ascii_art_heavy",
    "instructions": "Generate ASCII art representing current conversational mood. Focus on visual over textual explanation. Minimum 40x30 character size."
  }
}
```

#### Embedded Instructions Per Button
```cpp
const std::map<std::string, std::string> BUTTON_INSTRUCTIONS = {
    {"wib_vibe", "Create ASCII art showing the emotional/aesthetic mood of the current conversation. Use visual metaphors and abstract patterns."},
    {"wib_fractal", "Generate recursive, self-similar ASCII patterns that relate to the topic. Show mathematical beauty through character art."},
    {"wob_analyse", "Provide systematic breakdown with categories, metrics, and scientific notation. Use structured lists and precise language."},
    {"wob_measure", "Quantify aspects of the topic using formulas, ratios, and mathematical expressions in ASCII format."}
    // ... more button instructions
};
```

---

## Technical Implementation Plan

### Phase 1: Core Button System
1. **Enhanced TWibWobView**: Modify existing chat view for tri-panel layout
2. **Button Panel Classes**: Create `WibButtonPanel` and `WobButtonPanel` 
3. **Button State Management**: Track highlighting, animation, enabled states
4. **Basic Click Handling**: Route button presses to appropriate LLM queries

### Phase 2: Magic Command Integration  
1. **Command Parser**: Detect and parse magic commands from text input
2. **Button Mapping**: Map text commands to visual button equivalents
3. **Parameter Handling**: Support command arguments and modifiers
4. **Command History**: Track and suggest recently used commands

### Phase 3: Context Detection
1. **Topic Classifier**: Analyse conversation for creative vs technical content
2. **Dynamic Highlighting**: Automatically highlight relevant buttons  
3. **Adaptive Tooltips**: Context-specific button explanations
4. **Smart Suggestions**: Recommend buttons based on conversation flow

### Phase 4: Advanced Features
1. **Button Animations**: Visual feedback for button states and triggers
2. **Command Chaining**: Support sequential button execution
3. **Custom Button Sets**: User-defined button configurations
4. **Analytics**: Track button usage patterns for optimization

### File Structure
```
test-tui/
├── wibwob_view.h              (enhanced with button panels)
├── wibwob_view.cpp            (tri-panel layout implementation)  
├── wibwob_buttons.h           (button panel base classes)
├── wibwob_buttons.cpp         (button rendering and interaction)
├── wibwob_magic_commands.h    (command parser and mapping)  
├── wibwob_magic_commands.cpp  (magic command implementation)
├── wibwob_context.h           (topic detection and adaptation)
├── wibwob_context.cpp         (context analysis algorithms)
└── prds/
    └── wibwob-ai-buttons-dynamic-chat-enhancement.md (this file)
```

---

## User Experience Scenarios

### Scenario 1: Creative ASCII Art Generation
```
User: "Show me a castle"
[User clicks 🎨 VIBE button]
→ Wib generates whimsical castle ASCII art with emotional flourishes
[User clicks 🔬 CATEGORISE button] 
→ Wob analyses architectural elements and structural components
```

### Scenario 2: Magic Command Workflow
```
User types: "How do computers work? !analyse"
→ System detects magic command, highlights 📊 ANALYSE button
→ Wob provides systematic breakdown with technical diagrams
User types: "!vibe"  
→ Wib creates abstract ASCII art representing "computational essence"
```

### Scenario 3: Context-Aware Adaptation
```
Conversation about music composition:
→ System highlights Wib creative buttons (🎵 SONIC, 🌊 FLOW)
→ Dims Wob technical buttons (📐 MEASURE still available)
→ User clicks 🎵 SONIC
→ Wib generates music-inspired ASCII waveforms and notation
```

---

## Success Metrics & Evaluation

### Quantitative Metrics
- **Response Time**: Average seconds from button click to AI response
- **Engagement Rate**: Percentage of sessions using buttons vs text-only
- **Command Usage**: Most popular magic commands and button combinations
- **Session Length**: Average duration of button-enhanced vs traditional chat

### Qualitative Metrics  
- **Personality Expression**: User feedback on Wib/Wob characterization strength
- **Creative Output**: Quality and variety of ASCII art generation
- **Workflow Efficiency**: User perception of reduced cognitive load
- **Interface Intuitiveness**: Ease of learning button functions and magic commands

### Technical Metrics
- **Performance Impact**: UI responsiveness with button panels active
- **Memory Usage**: Additional memory overhead from button system
- **Error Rates**: Frequency of button/command parsing failures
- **Integration Stability**: Compatibility with existing wibwob chat features

---

## Future Enhancements

### Advanced Personality Features
- **Scramble Integration**: Cat-cat appears in button panels with context-sensitive reactions
- **Mood Evolution**: Button appearances change based on conversation history
- **Personality Learning**: Buttons adapt to user's preferred Wib/Wob balance

### Extended Interaction Patterns
- **Button Combinations**: Multi-button presses for hybrid responses
- **Gesture Sequences**: Button press patterns that trigger special modes
- **Voice Integration**: Voice magic commands via terminal microphone input
- **API Extensions**: External applications can trigger wibwob buttons via MCP

### Community Features
- **Button Sharing**: Export/import custom button configurations
- **Command Libraries**: Community-contributed magic command collections  
- **Personality Mods**: User-created Wib/Wob personality variations
- **ASCII Art Gallery**: Share button-generated artwork

---

*🤖 Generated with [Claude Code](https://claude.ai/code)*  
*Co-Authored-By: Claude <noreply@anthropic.com>*
