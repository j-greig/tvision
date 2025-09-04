/*---------------------------------------------------------*/
/*                                                         */
/*   paint_canvas.h - Simple paint canvas view (MVP)       */
/*                                                         */
/*   TL;DR                                                 */
/*   - TPaintCanvasView is a TView that owns a 2D cell     */
/*     buffer {ch, fg, bg} and draws it row-wise using     */
/*     TDrawBuffer + writeLine.                            */
/*   - Keyboard: arrows move cursor, Space draws,          */
/*     Shift+arrows draws while moving.                    */
/*   - Colors: 16-color BIOS indices for fg/bg.            */
/*   - Intended for MVP; extend with tools later.          */
/*                                                         */
/*---------------------------------------------------------*/

#ifndef TVISION_PAINT_CANVAS_H
#define TVISION_PAINT_CANVAS_H

#define Uses_TView
#define Uses_TRect
#define Uses_TDrawBuffer
#define Uses_TEvent
#define Uses_TColorAttr
#define Uses_TKeys
#include <tvision/tv.h>

#include <vector>

struct PaintCell {
    char ch;
    uint8_t fg;
    uint8_t bg;
};

class TPaintCanvasView : public TView {
public:
    TPaintCanvasView(const TRect &bounds, int cols, int rows);

    virtual void draw() override;
    virtual void handleEvent(TEvent &ev) override;
    virtual void sizeLimits(TPoint &min, TPoint &max) override;
    virtual void setState(ushort aState, Boolean enable) override;

    void clear();

private:
    int cols, rows;
    std::vector<PaintCell> buffer;
    int curX = 0, curY = 0;
    uint8_t fg = 15; // white
    uint8_t bg = 0;  // black

    void put(int x, int y, char ch, uint8_t f, uint8_t b);
    PaintCell &cell(int x, int y);
    void moveCursor(int dx, int dy, bool drawWhileMoving);
    void toggleDraw();
};

#endif
