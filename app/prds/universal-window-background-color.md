# Universal Window Background Color Customization

**tl;dr:** Extend existing Score BG Color functionality to all window types via Window menu, starting with Text/Animation windows as MVP. Enables user-customizable backgrounds for better visibility and personalization.

## Context

The test-tui application currently has background color customization through `View => Score BG Color` for specific window types. Users opening Text/Animation files via `File => Open Text/Animation` lack this customization capability, creating an inconsistent user experience.

## Objective

Create a universal background color customization system accessible via the `Window` menu that works for all window types, with Text/Animation windows as the initial implementation target (MVP).

## Problem Statement

- **Inconsistent UX**: Some windows support background color changes, others don't
- **Limited scope**: Current implementation tied to specific window types via View menu
- **Poor discoverability**: Users may not find color options for newly opened windows
- **Accessibility**: Text/Animation content may be hard to read on default backgrounds

## Requirements

### Functional Requirements
• Universal background color picker accessible via `Window` menu
• Apply to currently focused/active window
• Support Text/Animation windows (frame_player, text_view types)
• Persist color settings per window instance (session-only for MVP)
• Real-time preview of color changes
• Reset to default background option

### Technical Requirements  
• Extend existing TColorDialog/color picker UI component
• Modify Window menu structure to include "Background Color..." option
• Add background color property to base window classes
• Update rendering pipeline to respect per-window background colors
• Ensure compatibility with existing View=>Score BG Color functionality

### UI/UX Requirements
• Menu item: `Window => Background Color...`
• Standard Turbo Vision color picker dialog
• Menu item only enabled when applicable window is focused  
• Visual feedback showing current background color in picker
• Cancel/OK buttons with proper state management

## Expected Output

**Menu Structure:**
```
Window
├── Cascade
├── Tile  
├── Close All
├── ──────────
└── Background Color...    [NEW - enabled when applicable window focused]
```

**Dialog Interface:**
- Standard TColorDialog with current background pre-selected
- Real-time preview (if feasible) or apply-on-OK
- "Default" button to reset to system background

**Code Changes:**
- Extend base window classes with `bgColor` property
- Modify window rendering to use custom background
- Add Window menu handler for color picker
- Update Text/Animation window implementations

## Success Criteria

□ Window menu includes "Background Color..." option
□ Menu item properly enabled/disabled based on focused window type
□ Color picker dialog opens and functions correctly
□ Text/Animation windows display chosen background color immediately
□ Multiple windows can have different background colors simultaneously  
□ No regression in existing View=>Score BG Color functionality
□ Clean fallback to default background when no custom color set

## Implementation Approach

### Phase 1: Base Infrastructure
1. Add `bgColor` member to relevant base window classes
2. Modify rendering pipeline to check for custom background colors
3. Create Window menu item with proper enable/disable logic

### Phase 2: Text/Animation Integration  
1. Update `text_view` and `frame_player` window types to support background color
2. Ensure proper redraws when background color changes
3. Test with various text content and animation frames

### Phase 3: Dialog Integration
1. Wire Window menu to open existing TColorDialog
2. Apply selected color to focused window
3. Handle cancel/OK states properly

## Edge Cases & Considerations

**Technical Edge Cases:**
- What happens if no window is focused when menu selected?
- How to handle color picker for non-supporting window types?
- Memory management for color properties
- Performance impact of custom backgrounds on animation windows

**UX Edge Cases:**  
- Very dark text on dark backgrounds (readability)
- High contrast accessibility requirements
- Color picker dismissed vs cancelled
- Window closed while color picker is open

**Compatibility:**
- Ensure no conflicts with existing View=>Score BG Color
- Terminal color depth limitations (256 vs truecolor)
- Different terminal emulator color handling

## Testing Requirements

**Manual Testing:**
- Create Text/Animation windows via File menu
- Verify Window menu shows Background Color option  
- Test color picker opens and applies colors correctly
- Verify each window maintains its own background color
- Test with multiple Text/Animation windows simultaneously

**Regression Testing:**
- Existing View=>Score BG Color still functions
- Other Window menu items unaffected
- Window focus/cascade/tile operations work with custom backgrounds
- No visual artifacts or rendering issues

## Technical Notes

- Leverage existing TColorDialog implementation
- Consider using TColorAttr for RGB color support beyond basic palette
- Ensure proper cleanup when windows are destroyed
- May need to modify base TView drawing methods to respect background color property

This MVP provides the foundation for expanding background customization to all window types in future iterations.