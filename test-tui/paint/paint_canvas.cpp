/*---------------------------------------------------------*/
/*                                                         */
/*   paint_canvas.cpp - Simple paint canvas view (MVP)     */
/*                                                         */
/*---------------------------------------------------------*/

#include "paint_canvas.h"

TPaintCanvasView::TPaintCanvasView(const TRect &bounds, int cols, int rows)
    : TView(bounds), cols(cols), rows(rows), buffer(cols * rows)
{
    options |= ofFramed | ofSelectable;
    growMode = gfGrowAll;
    eventMask |= evKeyboard | evMouseDown | evMouseAuto | evMouseMove;
    clear();
}

void TPaintCanvasView::clear()
{
    for (auto &c : buffer) { c.ch = ' '; c.fg = fg; c.bg = bg; }
    drawView();
}

PaintCell &TPaintCanvasView::cell(int x, int y)
{
    if (x < 0) x = 0; if (x >= cols) x = cols - 1;
    if (y < 0) y = 0; if (y >= rows) y = rows - 1;
    return buffer[y * cols + x];
}

void TPaintCanvasView::put(int x, int y, char ch, uint8_t f, uint8_t b)
{
    if (x < 0 || x >= cols || y < 0 || y >= rows) return;
    auto &c = buffer[y * cols + x];
    c.ch = ch; c.fg = f; c.bg = b;
}

void TPaintCanvasView::draw()
{
    TDrawBuffer b;
    const int W = size.x;
    const int H = size.y;

    for (int y = 0; y < H; ++y) {
        int by = y;
        if (by >= rows) {
            b.moveChar(0, ' ', TColorAttr{0x07}, W);
            writeLine(0, y, W, 1, b);
            continue;
        }
        int filled = 0;
        while (filled < W) {
            int x = filled;
            if (x >= cols) {
                b.moveChar(filled, ' ', TColorAttr{0x07}, W - filled);
                break;
            }
            const auto &c = cell(x, by);
            // Create 16-color attribute from fg/bg (BIOS)
            uint8_t attr = (c.bg << 4) | (c.fg & 0x0F);
            TColorAttr a{attr};
            if ((state & sfFocused) && x == curX && by == curY)
                a = reverseAttribute(a);
            b.moveChar(filled, c.ch, a, 1);
            ++filled;
        }
        writeLine(0, y, W, 1, b);
    }
}

void TPaintCanvasView::moveCursor(int dx, int dy, bool drawWhileMoving)
{
    int nx = curX + dx;
    int ny = curY + dy;
    if (nx < 0) nx = 0; if (nx >= cols) nx = cols - 1;
    if (ny < 0) ny = 0; if (ny >= rows) ny = rows - 1;
    if (drawWhileMoving) put(nx, ny, '\xDB', fg, bg); // draw full block
    curX = nx; curY = ny;
    drawView();
}

void TPaintCanvasView::toggleDraw()
{
    auto &c = cell(curX, curY);
    if (c.ch == ' ') c.ch = '\xDB'; else c.ch = ' ';
    drawView();
}

void TPaintCanvasView::handleEvent(TEvent &ev)
{
    TView::handleEvent(ev);
    if (ev.what == evKeyDown) {
        bool shift = (ev.keyDown.controlKeyState & 0x03) != 0; // kbShift
        switch (ev.keyDown.keyCode) {
            case kbLeft:  moveCursor(-1, 0, shift); clearEvent(ev); break;
            case kbRight: moveCursor( 1, 0, shift); clearEvent(ev); break;
            case kbUp:    moveCursor( 0,-1, shift); clearEvent(ev); break;
            case kbDown:  moveCursor( 0, 1, shift); clearEvent(ev); break;
            default:
                if (ev.keyDown.charScan.charCode == ' ') { toggleDraw(); clearEvent(ev); }
                break;
        }
    }
    else if (ev.what == evMouseDown) {
        TPoint m = makeLocal(ev.mouse.where);
        bool shift = (ev.mouse.eventFlags & (kbShift)) != 0;
        curX = std::max(0, std::min(cols - 1, m.x));
        curY = std::max(0, std::min(rows - 1, m.y));
        if (shift || (ev.mouse.buttons & mbLeftButton))
            put(curX, curY, '\xDB', fg, bg);
        drawView();
        clearEvent(ev);
    }
}

void TPaintCanvasView::sizeLimits(TPoint &min, TPoint &max)
{
    TView::sizeLimits(min, max);
    min.x = 16; min.y = 6;
}

void TPaintCanvasView::setState(ushort aState, Boolean enable)
{
    TView::setState(aState, enable);
    if (aState & (sfFocused | sfActive))
        drawView();
}
