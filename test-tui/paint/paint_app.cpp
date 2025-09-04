/*---------------------------------------------------------*/
/*                                                         */
/*   paint_app.cpp - Minimal TVision paint app (MVP)       */
/*                                                         */
/*---------------------------------------------------------*/

#define Uses_TApplication
#define Uses_TWindow
#define Uses_TRect
#define Uses_TFrame
#define Uses_TDeskTop
#define Uses_TMenuBar
#define Uses_TMenu
#define Uses_TSubMenu
#define Uses_TMenuItem
#define Uses_TStatusLine
#define Uses_TStatusDef
#define Uses_TStatusItem
#define Uses_TKeys
#define Uses_MsgBox
#include <tvision/tv.h>

#include "paint_canvas.h"

class PaintWindow : public TWindow {
public:
    PaintWindow(const TRect &bounds, const char *title)
        : TWindow(bounds, title, wnNoNumber), TWindowInit(&PaintWindow::initFrame)
    {
        options |= ofTileable;
        TRect client = getExtent(); client.grow(-1, -1);
        TPaintCanvasView *canvas = new TPaintCanvasView(client, client.b.x, client.b.y);
        insert(canvas);
        canvas->select();
    }
private:
    static TFrame *initFrame(TRect r) { return new TFrame(r); }
};

class PaintApp : public TApplication {
public:
    PaintApp() : TProgInit(&PaintApp::initStatusLine, &PaintApp::initMenuBar, &PaintApp::initDeskTop) {}

    static TStatusLine *initStatusLine(TRect r) {
        r.a.y = r.b.y - 1;
        return new TStatusLine(r,
            *new TStatusDef(0, 0xFFFF) +
            *new TStatusItem("~F10~ Menu", kbF10, cmMenu) +
            *new TStatusItem("~Alt-X~ Exit", kbAltX, cmQuit)
        );
    }
    static TMenuBar *initMenuBar(TRect r) {
        r.b.y = r.a.y + 1;
        return new TMenuBar(r,
            *new TSubMenu("~F~ile", kbAltF) +
                *new TMenuItem("~N~ew", cmNew, kbCtrlN) +
                *new TMenuItem("E~x~it", cmQuit, kbAltX, hcNoContext, "Alt-X")
        );
    }
    static TDeskTop *initDeskTop(TRect r) { return new TDeskTop(r); }

    virtual void handleEvent(TEvent &ev) override {
        TApplication::handleEvent(ev);
        if (ev.what == evCommand) {
            switch (ev.message.command) {
                case cmNew: {
                    TRect r(2, 1, 82, 26);
                    PaintWindow *w = new PaintWindow(r, "Paint");
                    deskTop->insert(w);
                    clearEvent(ev);
                    break;
                }
                default: break;
            }
        }
    }
};

int main() {
    PaintApp app;
    // Open initial window
    TRect r(2, 1, 82, 26);
    PaintWindow *w = new PaintWindow(r, "Paint");
    app.deskTop->insert(w);
    app.run();
    return 0;
}
