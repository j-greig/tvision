/*---------------------------------------------------------*/
/*                                                         */
/*   doom_ascii_view.h - DOOM ASCII TUI Window             */
/*                                                         */
/*---------------------------------------------------------*/

#ifndef DOOM_ASCII_VIEW_H
#define DOOM_ASCII_VIEW_H

#define Uses_TView
#define Uses_TDrawBuffer
#define Uses_TRect
#define Uses_TEvent
#define Uses_TKeys
#include <tvision/tv.h>
#include <vector>
#include <cstdint>

// Forward declarations
extern "C" {
    void DG_Init(const char* wadPath);
    void D_DoomTick(void);
    uint32_t* getDoomScreenBuffer(void);
    void getDoomScreenDimensions(unsigned* width, unsigned* height);
}

// C-compatible input queue accessors (called by doomgeneric_tvision.c)
extern "C" {
    int doomInputQueueEmpty(void);
    unsigned char doomInputQueuePop(void);
}

//
// TDoomAsciiView
//
// Timer-based DOOM renderer using Unicode block characters
// - Extends TView (fixed viewport, no scrollbar)
// - 30 FPS animation via 33ms timer ticks
// - RGB → Unicode block conversion
// - Double-width rendering for aspect ratio
//
class TDoomAsciiView : public TView {
public:
    explicit TDoomAsciiView(const TRect &bounds, const char* wadPath);
    virtual ~TDoomAsciiView();

    // TView overrides
    virtual void draw() override;
    virtual void handleEvent(TEvent &ev) override;
    virtual void setState(ushort aState, Boolean enable) override;
    virtual void changeBounds(const TRect& bounds) override;

private:
    void startTimer();
    void stopTimer();
    void advance();              // Calls D_DoomTick() + render buffer
    void convertFrameBuffer();   // RGB → Unicode blocks

    // Timer state
    unsigned periodMs = 33;      // 30 FPS
    TTimerId timerId = 0;

    // Frame buffer (rendered ASCII)
    std::vector<TScreenCell> frameBuffer;

    // DOOM game dimensions
    unsigned gameWidth = 320;
    unsigned gameHeight = 200;

    // WAD file path
    const char* wadPath;

public:
    // Input queue (DOOM key codes) - public for getDoomInputQueue() accessor
    std::vector<uint8_t> inputQueue;
};

// Factory function
class TWindow;
TWindow* createDoomAsciiWindow(const TRect &bounds, const char* wadPath);

#endif // DOOM_ASCII_VIEW_H
