# Kaomoji Menu Enhancement & Terminal Screenshot Implementation

**tl;dr:** Add `つ◕‿◕‿◕ ༽つ` kaomoji to menu bar with optional Unifont rendering, implement terminal screenshot capability for TUI apps, research cross-platform capture methods.

## Context
Working with Turbo Vision TUI framework on macOS with existing test pattern app using monochrome theme. Need to enhance menu with Unicode kaomoji while addressing font rendering limitations and implementing screenshot functionality.

## Objective
1. Successfully render kaomoji `つ◕‿◕‿◕ ༽つ` in Turbo Vision menu bar
2. Implement terminal screenshot capability for TUI applications
3. Research and evaluate multiple screenshot approaches

## Requirements

### Kaomoji Menu Enhancement
• **Unicode Support**: Verify Turbo Vision's Unicode rendering capabilities
• **Font Selection**: Investigate Unifont integration for specific menu elements
• **Positioning**: Implement right-aligned OR first-item placement in menu bar
• **Rendering Quality**: Ensure proper character display across terminal emulators
• **Fallback Handling**: Graceful degradation if Unicode/font unsupported

### Screenshot Implementation
• **Cross-Platform Compatibility**: Primary focus macOS, consider Windows/Linux
• **Multiple Capture Methods**: Research and implement various approaches
• **Integration**: Seamless screenshot triggering within TUI application
• **Output Format**: Standard image formats (PNG preferred)
• **Error Handling**: Robust failure management and user feedback

## Technical Challenges

### Turbo Vision Unicode Limitations
• **Character Encoding**: Assess current Unicode support level
• **Font Rendering**: Determine if custom font loading possible
• **Terminal Compatibility**: Test across macOS Terminal, iTerm2, VSCode terminal
• **Memory Management**: Handle Unicode strings properly in C++

### Screenshot Technical Approaches
• **In-Application Capture**: Direct terminal buffer to image conversion
• **System-Level Screenshots**: Shell command integration (`screencapture` on macOS)
• **VSCode Extension APIs**: Leverage editor screenshot capabilities
• **Third-Party Tools**: Integration with existing screenshot utilities

## Research Requirements

### Terminal Screenshot Methods Investigation
1. **Native Terminal Buffer Capture**
   - Analyse Turbo Vision's screen buffer access
   - Convert character/colour matrix to image format
   - Handle special characters and Unicode rendering

2. **System Command Integration**
   - macOS `screencapture` command integration
   - Window detection and targeting
   - Coordinate calculation for terminal bounds

3. **VSCode Terminal Extension APIs**
   - Research available screenshot extensions
   - Command palette integration possibilities
   - Automation potential for TUI apps

4. **Cross-Platform Solutions**
   - Linux: `gnome-screenshot`, `scrot`, `import`
   - Windows: PowerShell screenshot methods
   - Abstraction layer for platform differences

## Implementation Strategy

### Phase 1: Kaomoji Menu Integration
1. **Unicode Assessment**
   - Test current kaomoji rendering in existing menu
   - Document character display issues
   - Identify terminal-specific rendering differences

2. **Font Investigation**
   - Research Unifont integration methods
   - Test font loading in Turbo Vision context
   - Evaluate performance impact

3. **Menu Modification**
   - Implement right-aligned menu item placement
   - Add kaomoji as menu element with proper spacing
   - Test positioning across window resize events

### Phase 2: Screenshot Research & Prototyping
1. **Method Evaluation**
   - Create proof-of-concept for each screenshot approach
   - Performance benchmarking
   - Quality assessment of captured images

2. **Integration Planning**
   - Design screenshot trigger mechanism (hotkey/menu)
   - File naming and storage strategy
   - User notification system

### Phase 3: Implementation & Testing
1. **Core Screenshot Functionality**
   - Implement chosen screenshot method(s)
   - Add error handling and user feedback
   - Create configuration options

2. **Quality Assurance**
   - Test across multiple terminal emulators
   - Verify Unicode rendering consistency
   - Validate screenshot accuracy and quality

## Expected Deliverables

### Code Components
• Modified menu bar with kaomoji integration
• Screenshot capture functionality
• Cross-platform compatibility layer
• Error handling and user feedback systems

### Documentation
• Technical implementation notes
• Platform-specific setup instructions
• Troubleshooting guide for Unicode/font issues

## Success Criteria
□ Kaomoji displays correctly in menu bar across supported terminals
□ Screenshot functionality captures TUI application accurately
□ No performance degradation in menu rendering
□ Graceful fallback when Unicode/fonts unavailable
□ Clean integration with existing codebase architecture
□ Cross-platform compatibility maintained where possible

## Technical Constraints
• Must work within Turbo Vision framework limitations
• Cannot break existing monochrome theme compatibility  
• Should maintain consistent performance characteristics
• Unicode support dependent on terminal emulator capabilities