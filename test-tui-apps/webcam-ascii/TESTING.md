# Testing Guide - Webcam ASCII Art

## Quick Test on MacBook Air 2020

### Method 1: Direct File Access (Fastest)

1. Open Finder and navigate to: `test-tui-apps/webcam-ascii/`
2. Double-click `index.html`
3. Safari will open the file
4. Click "Start Camera"
5. Grant camera permissions when prompted
6. You should see your webcam feed converted to ASCII art!

### Method 2: Local Server (If Direct Access Fails)

```bash
cd test-tui-apps/webcam-ascii
python3 -m http.server 8000
```

Then open Safari and go to: `http://localhost:8000`

## Testing Checklist for MacBook Air 2020

### Basic Functionality Tests

- [ ] **Camera Permission**
  - Click "Start Camera"
  - Permission dialog appears
  - Granting permission starts the feed
  - Denying permission shows error message

- [ ] **ASCII Conversion**
  - Video feed appears as ASCII characters
  - ASCII updates in real-time
  - Characters change based on movement/lighting
  - Display is smooth (no excessive flickering)

- [ ] **Resolution Controls**
  - Move "Width" slider (30-200)
    - Display width changes immediately
    - No lag or freezing
  - Move "Height" slider (20-150)
    - Display height changes immediately
    - Aspect ratio adjusts

- [ ] **Font Size Control**
  - Move "Font Size" slider (6-16)
    - Text size changes immediately
    - ASCII characters remain legible
    - No layout breaking

- [ ] **Character Set Selection**
  - Test "Simple" set: Should see basic characters
  - Test "Standard" set: Should see detailed characters
  - Test "Blocks" set: Should see block elements
  - Test "Minimal" set: Should see minimal character set
  - Switching updates display immediately

- [ ] **Start/Stop Functionality**
  - Click "Stop" button
    - Camera indicator light turns off
    - ASCII display stops updating
    - Message shows "Camera stopped"
  - Click "Start Camera" again
    - Camera restarts successfully
    - ASCII conversion resumes

- [ ] **Hide/Show Controls**
  - Click "Hide Controls"
    - Controls panel becomes semi-transparent
    - More screen space for ASCII display
  - Hover over controls area
    - Controls become visible again
  - Click "Show Controls"
    - Controls fully visible again

### Performance Tests

- [ ] **FPS Counter**
  - Should show 15+ FPS with default settings
  - Should stay above 10 FPS with max resolution (200x150)
  - Should reach 25-30 FPS with low resolution (50x30)

- [ ] **Responsiveness**
  - Sliders move smoothly
  - No lag when adjusting controls
  - Display updates without freezing

- [ ] **Extended Use**
  - Run for 5 minutes continuously
    - No memory issues
    - FPS remains stable
    - No browser slowdown

### Browser Compatibility (macOS)

- [ ] **Safari** (Primary browser for MacBook Air 2020)
  - All features work
  - Smooth performance
  - No console errors

- [ ] **Chrome** (If installed)
  - All features work
  - Performance comparison with Safari
  - No compatibility issues

### Error Handling

- [ ] **No Camera Available**
  - Disconnect/disable camera (if possible)
  - App shows appropriate error message

- [ ] **Permission Denied**
  - Deny camera permission
  - App shows error: "Camera access failed"
  - Can retry by refreshing page

- [ ] **Browser Not Supported**
  - Check on old browser (if available)
  - Should show compatibility message

## Running Automated Tests

1. Open `test/test.html` in your browser
2. Click "Run All Tests"
3. Verify all tests pass (green)
4. Check performance metrics:
   - Average conversion time should be <50ms
   - Theoretical FPS should be >20

### Expected Test Results

```
✓ All unit tests should pass
✓ Brightness calculation tests: 4/4 passed
✓ Character mapping tests: 7/7 passed
✓ Image data conversion tests: 4/4 passed
✓ Character set switching tests: 4/4 passed
✓ Visual gradient test: 3/3 passed
✓ Performance tests: 2/2 passed

Total: ~24 tests, all passing
```

## Performance Benchmarks for MacBook Air 2020

Expected performance on MacBook Air 2020 (M1 or Intel):

| Resolution | Character Set | Expected FPS | Notes |
|------------|---------------|--------------|-------|
| 50x30      | Any          | 30+          | Very smooth |
| 100x60     | Simple       | 25-30        | Smooth (default) |
| 100x60     | Standard     | 20-25        | Good detail |
| 150x90     | Simple       | 18-22        | Acceptable |
| 200x150    | Standard     | 12-18        | Maximum quality |

## Common Issues & Solutions

### Issue: "Camera access failed"

**Cause**: Permission denied or camera in use

**Fix**:
1. Check Safari → Preferences → Websites → Camera
2. Ensure camera isn't used by another app (Zoom, FaceTime, etc.)
3. Refresh page and grant permission again

### Issue: Low FPS (<10)

**Cause**: Resolution too high or browser performance

**Fix**:
1. Reduce width and height sliders
2. Use "Simple" character set
3. Close other browser tabs
4. Restart browser

### Issue: Distorted ASCII output

**Cause**: Font rendering or aspect ratio

**Fix**:
1. Ensure browser zoom is 100%
2. Try different font sizes
3. Adjust height to maintain aspect ratio (~0.6 × width)

### Issue: Page won't load via file://

**Cause**: Browser security restrictions

**Fix**:
1. Use local server method (see above)
2. Use Safari instead of Chrome (less restrictive for file://)

## Visual Quality Testing

### Good Lighting Test
- Use well-lit environment
- Should see good contrast in ASCII
- Details should be visible

### Low Light Test
- Dim lighting or cover camera partially
- Should see mostly darker characters
- No crashes or errors

### Movement Test
- Wave hand in front of camera
- ASCII should update smoothly
- No lag or ghosting

### Detail Test
- Hold up text or detailed object
- Higher resolutions should show more detail
- "Standard" character set should show best detail

## Accessibility Testing

- [ ] Can navigate with keyboard (Tab key)
- [ ] Buttons are clearly labeled
- [ ] Controls are easy to understand
- [ ] Error messages are clear

## Security/Privacy Testing

- [ ] Verify no network requests (check browser network tab)
- [ ] Confirm no data is stored (check localStorage/sessionStorage)
- [ ] Camera turns off when stopping
- [ ] No recording or saving functionality (as expected)

## Final Verification

After completing all tests, verify:

✅ Camera works on MacBook Air 2020
✅ Real-time ASCII conversion is smooth
✅ All controls function properly
✅ Performance meets expectations (>15 FPS)
✅ No console errors
✅ No memory leaks
✅ Privacy maintained (no network/storage)

## Reporting Issues

If you encounter any issues, note:
- Browser version (Safari/Chrome)
- macOS version
- MacBook Air model/year
- Specific steps to reproduce
- Console error messages (if any)

---

**Happy Testing!** 🧪
