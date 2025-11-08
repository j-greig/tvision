# Webcam ASCII Art

A lightweight, standalone web application that converts your webcam feed into real-time ASCII art.

![ASCII Art Demo](demo-screenshot.png)

## Features

- **Real-time Conversion**: Live webcam feed converted to ASCII characters at 15-30 FPS
- **Adjustable Resolution**: Control character dimensions (width and height)
- **Multiple Character Sets**: Choose from 4 different ASCII character sets
  - Simple (12 chars)
  - Standard (70 chars) - Default
  - Block Elements
  - Minimal (8 chars)
- **Customizable Display**: Adjust font size for optimal viewing
- **Dark Theme**: Black background with white text for comfortable viewing
- **Zero Dependencies**: Pure vanilla JavaScript, no build process required
- **Privacy First**: All processing happens locally in your browser

## Quick Start

### Option 1: Direct File Access

1. Download or clone this repository
2. Open `index.html` in your web browser (Chrome or Safari recommended)
3. Click "Start Camera" and grant camera permissions
4. Enjoy your ASCII art webcam feed!

### Option 2: Local Server (Recommended for HTTPS requirements)

Some browsers require HTTPS for camera access. You can run a local server:

```bash
# Using Python 3
cd test-tui-apps/webcam-ascii
python3 -m http.server 8000

# Using Node.js (with http-server installed)
npx http-server -p 8000

# Using PHP
php -S localhost:8000
```

Then open: `http://localhost:8000`

## Usage

### Controls

1. **Start Camera**: Initiates webcam capture and ASCII conversion
2. **Stop**: Stops the camera feed
3. **Width Slider**: Adjusts horizontal character count (30-200)
4. **Height Slider**: Adjusts vertical character count (20-150)
5. **Font Size Slider**: Changes display text size (6-16px)
6. **Character Set Selector**: Switches between different ASCII character sets
7. **Hide/Show Controls**: Toggles control panel visibility for fullscreen effect

### Tips for Best Results

- **Resolution**: Start with default settings (100x60) and adjust based on performance
- **Character Set**:
  - Use "Standard" for detailed images
  - Use "Simple" or "Minimal" for faster performance
  - Use "Blocks" for a retro look
- **Font Size**: Smaller font sizes (6-8px) work well with higher resolutions
- **Lighting**: Well-lit environments produce better contrast and detail

## Browser Compatibility

### Tested Platforms

- ✅ **macOS Safari 14+** (MacBook Air 2020 tested)
- ✅ **Chrome 90+** (macOS, Windows, Linux)
- ✅ **Firefox 88+**
- ✅ **Edge 90+**

### Requirements

- Modern browser with `getUserMedia` API support
- Webcam/camera device
- JavaScript enabled

### Known Limitations

- Some browsers require HTTPS for camera access (use localhost for testing)
- Mobile browsers supported but may have performance limitations
- Safari may show a one-time camera permission dialog

## Technical Details

### Architecture

The application uses three main modules:

1. **AsciiConverter**: Handles pixel-to-character conversion
   - Brightness calculation using standard luminance formula
   - Character mapping based on brightness values
   - Configurable character sets

2. **WebcamManager**: Manages camera stream
   - getUserMedia API integration
   - Canvas-based frame capture
   - Stream lifecycle management

3. **AsciiCameraApp**: Main application controller
   - UI event handling
   - Render loop using `requestAnimationFrame`
   - FPS tracking and display

### Performance

- **Frame Processing**: ~10-30ms per frame (100x60 resolution)
- **Target FPS**: 20-30 FPS
- **Memory Usage**: <50MB typical
- **Bundle Size**: ~15KB (single HTML file)

### Algorithm

```
1. Capture video frame from webcam
2. Draw frame to canvas at target resolution
3. Extract pixel data from canvas
4. For each pixel:
   a. Calculate brightness: 0.299*R + 0.587*G + 0.114*B
   b. Map brightness to character from selected set
5. Render ASCII string to DOM
6. Repeat via requestAnimationFrame
```

## Testing

### Automated Tests

Run the test suite by opening `test/test.html` in your browser.

**Test Coverage**:
- ✅ Brightness calculation accuracy
- ✅ Character mapping correctness
- ✅ Image data to ASCII conversion
- ✅ Character set switching
- ✅ Visual gradient rendering
- ✅ Performance benchmarks

### Manual Testing

See `test/test.html` for a complete manual testing checklist.

**Key Test Scenarios**:
1. Camera permission flow
2. Start/stop functionality
3. Resolution control responsiveness
4. Cross-browser compatibility
5. Error handling (no camera, denied permissions)

## Development

### Project Structure

```
webcam-ascii/
├── index.html          # Main application (single-file, all-in-one)
├── README.md           # This file
└── test/
    └── test.html       # Test suite with unit and visual tests
```

### Customization

#### Adding New Character Sets

Edit the `charSets` object in `index.html`:

```javascript
this.charSets = {
    simple: ' .:-=+*#%@',
    custom: 'YOUR_CHARS_HERE',  // Add your set
    // ... existing sets
};
```

Then add option to the select element:

```html
<option value="custom">Custom Set</option>
```

#### Modifying Brightness Calculation

The `getBrightness()` method uses standard luminance weights:

```javascript
getBrightness(r, g, b) {
    return 0.299 * r + 0.587 * g + 0.114 * b;
}
```

You can adjust weights for different effects.

#### Performance Tuning

To limit frame rate for better performance:

```javascript
// Add FPS limit in render() method
const targetFPS = 20;
const frameDelay = 1000 / targetFPS;

// Use setTimeout with requestAnimationFrame
setTimeout(() => {
    this.animationId = requestAnimationFrame(() => this.render());
}, frameDelay);
```

## Troubleshooting

### Camera Not Working

**Problem**: "Camera access failed" error

**Solutions**:
1. Check browser permissions (Safari: Preferences → Websites → Camera)
2. Ensure no other app is using the camera
3. Try using HTTPS or localhost
4. Refresh the page and grant permissions when prompted

### Poor Performance

**Problem**: Low FPS or laggy display

**Solutions**:
1. Reduce resolution (lower width/height values)
2. Use simpler character set ("Simple" or "Minimal")
3. Close other browser tabs/applications
4. Try a different browser (Chrome often performs better)

### Display Issues

**Problem**: ASCII art looks distorted

**Solutions**:
1. Ensure browser zoom is 100%
2. Try different font sizes
3. Adjust aspect ratio by changing height relative to width
4. Some fonts may work better than others (Courier New is recommended)

## Privacy

- **No Data Collection**: All processing happens locally in your browser
- **No Server Communication**: No video or images are sent anywhere
- **No Storage**: No data is saved or cached
- **Camera Access**: Only used while the app is running and explicitly started

## License

See the main TVision project license.

## Credits

- Inspired by classic ASCII art projects
- Built with vanilla JavaScript and HTML5 Canvas API
- Uses standard luminance formula for brightness calculation

## Contributing

Contributions welcome! Some ideas:

- [ ] Color ASCII mode (using ANSI colors or CSS)
- [ ] Video recording/GIF export
- [ ] Preset resolution profiles
- [ ] Keyboard shortcuts
- [ ] Mirroring/flip options
- [ ] Edge detection mode
- [ ] Mobile optimization

## Support

Tested and developed for MacBook Air 2020 running macOS.

For issues or questions, please refer to the main TVision repository.

---

**Enjoy your ASCII adventures!** 🎨
