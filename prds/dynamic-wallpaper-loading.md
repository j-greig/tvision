# Dynamic Wallpaper Loading from Text Files - PRD

**tl;dr:** Replace hardcoded CP437 hex sequences with dynamic UTF-8 file loading from test-tui/primers/*.txt, leveraging Turbo Vision's native UTF-8 support for cleaner, editable ASCII art wallpapers.

## Problem

Currently, wallpaper ASCII art in `test-tui/wallpaper.cpp` is hardcoded as CP437 hex escape sequences:

```cpp
const char* TWallpaperView::asciiArt[] = {
    "\xDB     \xDB\xB0 \xDB\xDB\xDB \xDC\xDC\xDC\xDC...",
    // 17 more lines of hex-encoded madness
};
```

**Issues:**
- Unreadable and unmaintainable hex encoding
- Can't easily edit ASCII art
- No support for Unicode/emoji art (despite TV having UTF-8 support)
- Hardcoded single wallpaper option

## Solution Overview

Dynamically load ASCII art from `test-tui/primers/*.txt` files using Turbo Vision's built-in UTF-8 support.

## Technical Approach

### Current Turbo Vision UTF-8 Support

TV already has comprehensive UTF-8 infrastructure:
- `TText::drawStr()` - renders UTF-8 strings to screen cells
- `TText::measure()` - calculates text width/metrics
- Native UTF-8 to screen cell conversion

### Implementation Plan

#### 1. Create WallpaperLoader Utility

```cpp
class WallpaperLoader {
public:
    static std::vector<std::string> loadFromFile(const std::string& path);
    static std::vector<std::string> getAvailableWallpapers();
};
```

#### 2. Modify TWallpaperView

**Current:**
```cpp
const char* asciiArt[17]; // Hardcoded hex
```

**New:**
```cpp
std::vector<std::string> artLines;
void loadFromFile(const std::string& path);
void loadRandom();
```

#### 3. Replace Draw Logic

**Current:** Manual char/attribute placement with hex decoding
**New:** Use `TText::drawStr()` for native UTF-8 rendering

```cpp
void TWallpaperView::draw() {
    // Use TText::drawStr() instead of manual putChar/putAttribute
    for (int y = 0; y < artLines.size(); y++) {
        TText::drawStr(cells, artLines[y], artColor);
    }
}
```

### File Structure

```
test-tui/primers/
├── wibwob-kaomoji.txt       # Unicode kaomoji art
├── symbient.txt             # Complex ASCII scenes  
├── monster-emoji.txt        # Emoji-based art
└── wireframe-isometric.txt  # Traditional ASCII
```

### Benefits

1. **Editable:** Direct text file editing instead of hex encoding
2. **Unicode Support:** Full emoji/kaomoji wallpapers via TV's UTF-8 support
3. **Dynamic:** Cycle through different wallpapers
4. **Maintainable:** No more CP437 hex sequences
5. **Leverages Existing:** Uses TV's proven UTF-8 infrastructure

## Implementation Steps

1. **Create WallpaperLoader class**
   - Use `std::ifstream` (already used by `frame_file_player`)
   - Load files as UTF-8 strings
   - Handle file errors gracefully

2. **Modify TWallpaperView constructor**
   - Accept file path parameter
   - Replace static array initialization
   - Default to `wibwob-kaomoji.txt` for backwards compatibility

3. **Update draw() method**
   - Replace manual char placement with `TText::drawStr()`
   - Maintain centering logic
   - Handle UTF-8 width calculations via `TText::measure()`

4. **Integration**
   - Update `test_pattern_app.cpp` to specify wallpaper file
   - Add menu option for wallpaper selection

## Testing

```bash
cd test-tui && cmake --build build
./build/test_pattern  # Should load default wibwob-kaomoji.txt
```

Test with various primer files to ensure UTF-8 rendering works correctly.

## Future Extensions

- Wallpaper selection menu in test_pattern
- Animation support (cycle through multiple files)
- Custom wallpaper directories
- Color mapping for monochrome ASCII art

## Files to Modify

- `test-tui/wallpaper.h` - Add dynamic loading interface
- `test-tui/wallpaper.cpp` - Replace hex arrays with file loading + TText rendering
- `test-tui/test_pattern_app.cpp` - Update wallpaper initialization

## Schema Example

**Input file** (`test-tui/primers/wibwob-kaomoji.txt`):
```
つ◕‿◕‿◕༽つ
つ🧠‿🧠‿🧠༽つ
つ👁️‿👁️‿👁️༽つ
```

**Runtime:** Loaded as `std::vector<std::string>`, rendered via `TText::drawStr()` with proper UTF-8 handling.