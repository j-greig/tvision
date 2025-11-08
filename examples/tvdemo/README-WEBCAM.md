# Webcam ASCII Art for TVDemo

Real-time webcam-to-ASCII art conversion integrated into the Turbo Vision demo application.

## Features

- Real-time webcam capture and ASCII conversion
- Displays in a resizable Turbo Vision window
- Multiple ASCII character sets
- Runs at 10-15 FPS in terminal
- Fallback test pattern when OpenCV is not available

## Building

### With OpenCV (Real Webcam Support)

#### macOS

```bash
# Install OpenCV
brew install opencv

# Build tvision
cmake . -B build
cmake --build build
```

#### Linux (Ubuntu/Debian)

```bash
# Install OpenCV
sudo apt-get update
sudo apt-get install libopencv-dev

# Build tvision
cmake . -B build
cmake --build build
```

#### Linux (Fedora)

```bash
# Install OpenCV
sudo dnf install opencv-devel

# Build tvision
cmake . -B build
cmake --build build
```

### Without OpenCV (Test Pattern Mode)

If OpenCV is not installed, the application will still compile and run with a test pattern:

```bash
cmake . -B build
cmake --build build
```

The build system will display:
```
-- OpenCV not found - webcam will use test pattern
-- To enable webcam: install OpenCV and rebuild
```

## Running

```bash
# Run tvdemo
./build/examples/tvdemo/tvdemo

# In the application:
# 1. Open the system menu (click the icon in top-left or press F10)
# 2. Select "Webcam ASCII" or press Alt-W
```

## Usage

### Opening the Webcam Window

- **Menu**: System Menu → Webcam ASCII
- **Keyboard**: Alt-W

### Controls

- **ESC**: Close the webcam window
- **Alt-F3**: Close the window (standard TV close)
- **Ctrl-F5**: Resize/move the window
- **F5**: Zoom window

### Window Behavior

- The window updates automatically via the application idle() loop
- ASCII conversion adapts to window size automatically
- Frame rate is throttled to 15 FPS to avoid terminal overload
- Camera starts automatically when window opens
- Camera stops automatically when window closes

## Camera Permissions

### macOS

macOS requires camera permissions for terminal applications:

1. When you first run the webcam feature, macOS will prompt for camera access
2. If denied, go to System Preferences → Security & Privacy → Camera
3. Enable camera access for Terminal.app (or iTerm2, etc.)
4. Restart the application

### Linux

Most Linux systems allow direct camera access. If you encounter permission issues:

```bash
# Add your user to the video group
sudo usermod -a -G video $USER

# Log out and log back in for changes to take effect
```

## Architecture

### Class Structure

```
TWebcamAsciiWindow
├── TWebcamView (displays ASCII art)
├── WebcamCapture (OpenCV or test pattern)
└── AsciiConverter (pixel to ASCII conversion)
```

### Files

```
examples/tvdemo/
├── webcam.h         - Header with class declarations
├── webcam.cpp       - Implementation
├── tvcmds.h         - Command constant (cmWebcamAsciiCmd)
├── tvdemo.h         - Method declaration
├── tvdemo2.cpp      - Event handler and webcamAscii() method
├── tvdemo3.cpp      - Menu integration and idle() updates
└── CMakeLists.txt   - OpenCV detection and linking
```

### Integration Points

1. **Command Constant**: `cmWebcamAsciiCmd = 116` in tvcmds.h
2. **Menu Item**: Added to system menu in tvdemo3.cpp::initMenuBar()
3. **Event Handler**: Case in tvdemo2.cpp::handleEvent()
4. **Idle Update**: Called from tvdemo3.cpp::idle() to update frames
5. **Method**: TVDemo::webcamAscii() creates and inserts window

## Performance

### Target Specs

- **Frame Rate**: 15 FPS (terminal-friendly)
- **Resolution**: Adapts to window size (typically 60x20 chars)
- **CPU**: ~10-20% on modern systems
- **Latency**: <100ms camera-to-display

### Optimization

The code includes several performance optimizations:

- Frame rate throttling to avoid overwhelming terminal
- Efficient brightness calculation (standard luminance formula)
- Minimal memory allocations in render loop
- Pre-allocated string buffers

## Test Pattern Mode

When OpenCV is not available, a test pattern is displayed:

- Animated gradient pattern
- Updates at same frame rate as real camera
- Useful for testing ASCII conversion without hardware
- Clearly labeled as "test pattern" in status

## Troubleshooting

### Camera Not Opening

**Symptom**: "Failed to open camera" message

**Solutions**:
1. Check camera permissions (macOS: System Preferences)
2. Ensure camera is not in use by another application
3. Try restarting the application
4. Check camera works with other apps (Photo Booth, etc.)

### Build Fails with OpenCV Errors

**Symptom**: Compilation errors related to OpenCV

**Solutions**:
1. Verify OpenCV is installed: `pkg-config --modversion opencv4`
2. Check OpenCV path: `pkg-config --cflags opencv4`
3. Try rebuilding: `rm -rf build && cmake . -B build && cmake --build build`
4. If problems persist, build without OpenCV (test pattern mode)

### Low Frame Rate

**Symptom**: Choppy or slow updates

**Solutions**:
1. This is normal in some terminals (limited refresh rate)
2. Try a different terminal emulator (iTerm2, Alacritty, etc.)
3. Reduce window size (smaller = faster)
4. Check CPU usage - if high, close other applications

### Distorted ASCII

**Symptom**: ASCII output looks stretched or squashed

**Solutions**:
1. This is normal - terminal characters are ~2:1 aspect ratio
2. Resize window to adjust aspect ratio
3. Try different font sizes in your terminal
4. Use a monospace font (Courier, Menlo, Monaco, etc.)

## Development

### Adding New Character Sets

Edit `webcam.cpp::AsciiConverter::initCharSet()`:

```cpp
case MY_CUSTOM_SET:
    charSet = " .:-=+*#MY_CHARS";
    break;
```

Add enum value to `webcam.h::AsciiConverter::CharSet`.

### Adjusting Frame Rate

Edit `webcam.cpp::TWebcamAsciiWindow` constructor:

```cpp
targetFPS(20)  // Change from 15 to desired FPS
```

Note: Higher FPS may overwhelm terminal refresh rate.

### Changing Default Window Size

Edit `webcam.cpp::TWebcamAsciiWindow` constructor:

```cpp
TWindow(TRect(0, 0, 80, 30), ...)  // Change from 60x20 to 80x30
```

## Known Limitations

- Terminal refresh rate limits practical FPS to ~20-30
- Color ASCII not yet supported (monochrome only)
- Single camera only (no multi-camera support)
- No recording/snapshot capability
- Character aspect ratio is fixed (2:1 typical for terminals)

## Future Enhancements

Potential improvements for future versions:

- Color ASCII art (using ANSI colors)
- Character set selection dialog
- Resolution presets
- Camera selection dialog (multiple cameras)
- Frame capture/save to file
- Adjustable aspect ratio
- Edge detection mode
- Motion detection

## Credits

- Based on web version in test-tui-apps/webcam-ascii/
- Uses OpenCV for cross-platform webcam access
- Integrates with Turbo Vision framework
- ASCII conversion algorithm from standard luminance formula

## See Also

- Main tvdemo README
- Turbo Vision documentation
- OpenCV documentation: https://docs.opencv.org/

## Support

For issues specific to the webcam feature:

1. Check this README first
2. Verify OpenCV installation
3. Test with test pattern mode (no OpenCV)
4. Check terminal compatibility

For general Turbo Vision issues, see the main project documentation.

## License

Same as Turbo Vision project.
