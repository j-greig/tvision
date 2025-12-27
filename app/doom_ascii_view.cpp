/*---------------------------------------------------------*/
/*                                                         */
/*   doom_ascii_view.cpp - DOOM ASCII TUI Implementation   */
/*                                                         */
/*---------------------------------------------------------*/

#include "doom_ascii_view.h"

#define Uses_TWindow
#define Uses_TFrame
#define Uses_TEvent
#include <tvision/tv.h>

// DOOM key codes
extern "C" {
#include "doomkeys.h"
}

//
// Unicode block character gradient for luminance mapping
// 5 levels from dark to bright
//
static const char* UNICODE_BLOCKS = " ░▒▓█";

//
// Global pointer to active DOOM view (for C code to access input queue)
//
static TDoomAsciiView* g_activeDoomView = nullptr;

//
// Map Turbo Vision key codes to DOOM key codes
//
static unsigned char mapTVKeyToDoom(ushort tvKey)
{
    switch (tvKey) {
        // Arrow keys
        case kbUp:        return KEY_UPARROW;
        case kbDown:      return KEY_DOWNARROW;
        case kbLeft:      return KEY_LEFTARROW;
        case kbRight:     return KEY_RIGHTARROW;

        // Control keys
        case kbCtrlA:     return KEY_FIRE;        // Ctrl = shoot
        case ' ':         return KEY_USE;         // Space = use/open doors
        case kbEsc:       return KEY_ESCAPE;      // ESC = menu
        case kbEnter:     return KEY_ENTER;

        // Function keys
        case kbF1:        return KEY_F1;
        case kbF2:        return KEY_F2;
        case kbF3:        return KEY_F3;
        case kbF4:        return KEY_F4;
        case kbF5:        return KEY_F5;
        case kbF6:        return KEY_F6;
        case kbF7:        return KEY_F7;
        case kbF8:        return KEY_F8;
        case kbF9:        return KEY_F9;
        case kbF10:       return KEY_F10;

        // Other useful keys
        case kbTab:       return KEY_TAB;
        case kbShiftTab:  return KEY_TAB;

        default:
            // Pass through alphanumeric keys directly (for weapon selection, etc.)
            if (tvKey >= '1' && tvKey <= '9')
                return (unsigned char)tvKey;
            if (tvKey >= 'a' && tvKey <= 'z')
                return (unsigned char)tvKey;
            if (tvKey >= 'A' && tvKey <= 'Z')
                return (unsigned char)(tvKey - 'A' + 'a');  // Convert to lowercase

            return 0;  // Unmapped key
    }
}

//
// TDoomAsciiView constructor
//
TDoomAsciiView::TDoomAsciiView(const TRect &bounds, const char* wadPath_)
    : TView(bounds), wadPath(wadPath_)
{
    // Anchor to top-left and grow to the right and bottom
    growMode = gfGrowHiX | gfGrowHiY;

    // Receive timer expirations via broadcast events
    eventMask |= evBroadcast;

    // Set global pointer for C code access
    g_activeDoomView = this;

    // Get DOOM screen dimensions
    getDoomScreenDimensions(&gameWidth, &gameHeight);

    // Initialize DOOM engine with WAD path
    DG_Init(wadPath);
}

//
// Destructor
//
TDoomAsciiView::~TDoomAsciiView()
{
    stopTimer();

    // Clear global pointer
    if (g_activeDoomView == this)
        g_activeDoomView = nullptr;
}

//
// Start timer for animation
//
void TDoomAsciiView::startTimer()
{
    if (timerId == 0) {
        timerId = setTimer(periodMs, (int)periodMs);
        fprintf(stderr, "[DOOM] Timer started: timerId=%p, period=%ums\n", (void*)timerId, periodMs);
    }
}

//
// Stop timer
//
void TDoomAsciiView::stopTimer()
{
    if (timerId != 0) {
        killTimer(timerId);
        timerId = 0;
    }
}

//
// advance - Execute one DOOM frame
//
void TDoomAsciiView::advance()
{
    static int tickCount = 0;
    if (tickCount < 5) {
        fprintf(stderr, "[DOOM] advance() called, tick %d\n", tickCount++);
    }

    // Call DOOM tick (one frame iteration)
    D_DoomTick();

    // Auto-release keys after delay
    for (auto it = keyPressedFrames.begin(); it != keyPressedFrames.end(); ) {
        if (it->second > 0) {
            it->second++;  // Increment frame counter

            if (it->second >= KEY_RELEASE_DELAY) {
                // Generate release event
                uint16_t data = (0 << 8) | it->first;  // pressed=0
                inputQueue.push_back(data);
                it = keyPressedFrames.erase(it);  // Remove from map
                continue;
            }
        }
        ++it;
    }

    // Convert RGB buffer to ASCII
    convertFrameBuffer();
}

//
// convertFrameBuffer - RGB → Unicode blocks
//
void TDoomAsciiView::convertFrameBuffer()
{
    static int convertCount = 0;
    if (convertCount < 3) {
        fprintf(stderr, "[DOOM] convertFrameBuffer() called %d\n", convertCount++);
    }

    uint32_t* src = getDoomScreenBuffer();
    if (!src) {
        fprintf(stderr, "[DOOM ERROR] getDoomScreenBuffer() returned NULL!\n");
        return;
    }

    const int W = size.x;
    const int H = size.y;
    if (W <= 0 || H <= 0) return;

    // Resize frame buffer if needed
    if ((int)frameBuffer.size() < W * H)
        frameBuffer.resize(W * H);

    // Calculate scaled dimensions
    // Double-width: each pixel = 2 cells
    int scaledW = W / 2;
    int scaledH = H;

    // Clamp to valid range
    if (scaledW < 1) scaledW = 1;
    if (scaledH < 1) scaledH = 1;

    // Convert each pixel
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < scaledW; ++x) {
            // Sample from game buffer with scaling
            int srcX = (x * (int)gameWidth) / scaledW;
            int srcY = (y * (int)gameHeight) / scaledH;

            // Clamp source coordinates
            if (srcX >= (int)gameWidth) srcX = gameWidth - 1;
            if (srcY >= (int)gameHeight) srcY = gameHeight - 1;

            uint32_t rgb = src[srcY * gameWidth + srcX];

            // Extract RGB components
            uint8_t r = (rgb >> 16) & 0xFF;
            uint8_t g = (rgb >> 8) & 0xFF;
            uint8_t b = rgb & 0xFF;

            // Convert to luminance (ITU-R BT.601)
            int lum = (r * 299 + g * 587 + b * 114) / 1000;

            // Map to Unicode block (5 levels)
            int level = (lum * 4) / 255;
            if (level < 0) level = 0;
            if (level > 4) level = 4;

            char ch = UNICODE_BLOCKS[level];

            // Create color attribute (full RGB for best quality)
            TColorRGB fg(r, g, b);
            TColorRGB bg(0, 0, 0);  // Pure black background

            // Double-width: write 2 cells per pixel
            int cellX = x * 2;
            if (cellX < W && y < H) {
                setCell(frameBuffer[y * W + cellX], ch, TColorAttr(fg, bg));
            }
            if (cellX + 1 < W && y < H) {
                setCell(frameBuffer[y * W + cellX + 1], ch, TColorAttr(fg, bg));
            }
        }
    }
}

//
// draw - Blit frame buffer to screen
//
void TDoomAsciiView::draw()
{
    static int drawCount = 0;
    if (drawCount < 3) {
        fprintf(stderr, "[DOOM] draw() called %d, size=%dx%d, frameBuf size=%zu\n",
                drawCount++, size.x, size.y, frameBuffer.size());
    }

    const int W = size.x;
    const int H = size.y;
    if (W <= 0 || H <= 0) return;

    // Ensure frame buffer is allocated
    if (frameBuffer.size() < (size_t)(W * H)) {
        fprintf(stderr, "[DOOM] WARNING: frameBuffer too small, resizing from %zu to %d\n",
                frameBuffer.size(), W * H);
        frameBuffer.resize(W * H);
    }

    // Blit each line
    for (int y = 0; y < H; ++y) {
        if (y >= (int)frameBuffer.size() / W) break;
        writeLine(0, y, W, 1, &frameBuffer[y * W]);
    }
}

//
// handleEvent - Process timer and keyboard events
//
void TDoomAsciiView::handleEvent(TEvent &ev)
{
    TView::handleEvent(ev);

    // Ensure view is selectable for keyboard focus
    if (!(options & ofSelectable)) {
        options |= ofSelectable;
    }

    // Keyboard input - queue press events and track state
    if (ev.what == evKeyDown) {
        unsigned char doomKey = mapTVKeyToDoom(ev.keyDown.keyCode);
        if (doomKey != 0) {
            // Only queue press if key wasn't already down (no repeat)
            if (keyPressedFrames[doomKey] == 0) {
                uint16_t data = (1 << 8) | doomKey;  // pressed=1
                inputQueue.push_back(data);
                keyPressedFrames[doomKey] = 1;  // Mark as pressed
            }
            clearEvent(ev);
        }
    }

    // Timer expiration - advance one frame
    if (ev.what == evBroadcast && ev.message.command == cmTimerExpired) {
        static int timerEventCount = 0;
        if (timerEventCount < 3) {
            fprintf(stderr, "[DOOM] Timer event received %d, timerId=%p, infoPtr=%p\n",
                    timerEventCount++, (void*)timerId, ev.message.infoPtr);
        }
        if (timerId != 0 && ev.message.infoPtr == timerId) {
            advance();
            drawView();
            clearEvent(ev);
        }
    }
}

//
// setState - Start/stop timer on expose/hide
//
void TDoomAsciiView::setState(ushort aState, Boolean enable)
{
    TView::setState(aState, enable);

    if ((aState & sfExposed) != 0) {
        if (enable) {
            fprintf(stderr, "[DOOM] setState: window exposed, starting timer\n");
            startTimer();
            drawView();
        } else {
            fprintf(stderr, "[DOOM] setState: window hidden, stopping timer\n");
            stopTimer();
        }
    }
}

//
// changeBounds - Recalculate scaling on resize
//
void TDoomAsciiView::changeBounds(const TRect& bounds)
{
    TView::changeBounds(bounds);

    // Recreate frame buffer with new dimensions
    frameBuffer.clear();

    // Re-render immediately
    drawView();
}

//
// Window wrapper
//
class TDoomAsciiWindow : public TWindow {
public:
    explicit TDoomAsciiWindow(const TRect &bounds, const char* wadPath)
        : TWindow(bounds, "DOOM ASCII", wnNoNumber)
        , TWindowInit(&TDoomAsciiWindow::initFrame)
    {
        options |= ofTileable;
        TRect c = getExtent();
        c.grow(-1, -1);
        insert(new TDoomAsciiView(c, wadPath));
    }

    virtual void changeBounds(const TRect& b) override
    {
        TWindow::changeBounds(b);
        // Force a full redraw after tiling/resizing
        setState(sfExposed, True);
        redraw();
    }

    static TFrame* initFrame(TRect r) {
        return new TFrame(r);
    }
};

//
// Factory function
//
TWindow* createDoomAsciiWindow(const TRect &bounds, const char* wadPath)
{
    return new TDoomAsciiWindow(bounds, wadPath);
}

//
// C-compatible input queue accessors
//
extern "C" {

int doomInputQueueEmpty(void)
{
    if (!g_activeDoomView)
        return 1;  // Empty if no view
    return g_activeDoomView->inputQueue.empty() ? 1 : 0;
}

uint16_t doomInputQueuePop(void)
{
    if (!g_activeDoomView || g_activeDoomView->inputQueue.empty())
        return 0;

    uint16_t data = g_activeDoomView->inputQueue.front();
    g_activeDoomView->inputQueue.erase(g_activeDoomView->inputQueue.begin());
    return data;
}

} // extern "C"
