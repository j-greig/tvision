# Terminal Resize Glitch Effects Implementation

**tl;dr:** Replicate and programmatically generate terminal resize visual artifacts (character scatter, color bleeding, buffer corruption) observed in TVision TUI applications, with exportable glitch frame capture system.

## Context
Terminal resize operations in TVision applications create visual artifacts due to timing mismatches between screen buffer updates, terminal dimension changes, and drawing operations. These glitches produce aesthetically interesting effects including scattered characters, color bleeding, and display corruption that can be artistically valuable.

## Objective
Develop a system to intentionally replicate, control, and capture terminal resize glitch effects for artistic/visual purposes, mimicking the natural corruption that occurs during rapid window resizing.

## Technical Analysis Requirements

### 1. Root Cause Investigation
• Analyze TVision's screen buffer management during resize events (`TScreen::screenResize()`)
• Identify timing windows where draw operations conflict with dimension changes
• Document how `TDrawBuffer` and screen update cycles create corruption patterns
• Map relationship between terminal dimension changes and character positioning errors
• Study ncurses buffer handling during `SIGWINCH` signal processing

### 2. TVision Resize Behavior Analysis
• Trace `TProgram::getEvent()` handling of `evCommand` with `cmScreenChanged`
• Document `TView::changeBounds()` propagation during resize
• Analyze how gradient drawing algorithms interact with changing screen dimensions
• Identify buffer overflow/underflow conditions that create scatter effects
• Map color attribute persistence across resize operations

## Implementation Requirements

### 3. Glitch Effect Engine
• **Intentional Buffer Corruption**: Simulate timing race conditions programmatically
• **Character Scatter Algorithm**: Randomly offset character positions based on resize patterns
• **Color Bleeding System**: Preserve and redistribute color attributes incorrectly
• **Partial Draw Corruption**: Interrupt drawing operations mid-cycle
• **Dimension Mismatch Simulation**: Apply old coordinates to new screen dimensions

### 4. Glitch Capture System
• **Frame State Freezing**: Capture corrupted `TDrawBuffer` states at glitch moments
• **Text Export Functionality**: Export complete TUI interface as clipboard-ready text
• **Artifact Preservation**: Maintain color codes, positioning errors, and character corruption
• **Multiple Format Support**: Plain text, ANSI escape sequences, HTML with styling
• **Timing Control**: Trigger captures at specific moments in resize cycle

### 5. Programmable Glitch Generation
• **Synthetic Resize Events**: Generate fake resize signals without actual terminal changes
• **Controlled Corruption Parameters**: Adjustable scatter radius, color bleed intensity, corruption probability
• **Pattern-Based Glitches**: Specific corruption algorithms (diagonal scatter, radial blur, etc.)
• **Animation Integration**: Glitch effects that evolve over time with existing gradient animations

## Technical Specifications

### Core Components
• **GlitchEngine Class**: Main controller for glitch effect generation
• **CorruptedDrawBuffer**: Extended `TDrawBuffer` with intentional corruption methods  
• **FrameCapture System**: Export corrupted frames to text/clipboard
• **ResizeSimulator**: Generate synthetic resize events and timing conflicts

### Integration Points
• Extend existing `test_pattern` application with glitch modes
• Hook into gradient animation update cycles for corruption injection
• Integrate with clipboard export functionality for text capture
• Add API endpoints for programmatic glitch control

### Output Formats
• **Raw Text**: Character-by-character export with positioning preserved
• **ANSI Formatted**: Color codes and escape sequences intact
• **Structured Data**: JSON format with character positions, colors, corruption metadata
• **Visual Documentation**: Screenshots of glitch states for comparison

## Success Criteria
□ Successfully reproduce terminal resize artifacts programmatically
□ Generate controllable glitch effects that match observed corruption patterns
□ Export corrupted frame states as clipboard-ready text with formatting preserved
□ Provide API control over glitch parameters and timing
□ Document technical mechanisms behind TVision resize corruption
□ Create reusable glitch generation system for artistic applications

## Constraints
• Must work within existing TVision architecture without breaking core functionality
• Glitch effects should be toggleable (normal operation still available)
• Export system must handle Unicode and color attribute preservation
• Performance impact should be minimal when glitch mode is disabled
• Maintain compatibility with existing test-tui applications and API server

## Technical Approach
Focus on intercepting and corrupting the normal TVision drawing pipeline rather than relying on actual terminal resizing, enabling controlled and reproducible glitch effects that can be captured and exported reliably.