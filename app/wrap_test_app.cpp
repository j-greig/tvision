/*---------------------------------------------------------*/
/*  wrap_test_app.cpp - Compare 5 word-wrap approaches     */
/*  Spawns 5 windows with identical fake conversation      */
/*  text using different wrapping strategies.               */
/*---------------------------------------------------------*/

#define Uses_TKeys
#define Uses_TApplication
#define Uses_TEvent
#define Uses_TRect
#define Uses_TStaticText
#define Uses_TMenuBar
#define Uses_TSubMenu
#define Uses_TMenuItem
#define Uses_TStatusLine
#define Uses_TStatusItem
#define Uses_TStatusDef
#define Uses_TDeskTop
#define Uses_TWindow
#define Uses_TFrame
#define Uses_TScroller
#define Uses_TScrollBar
#define Uses_TView
#define Uses_TDrawBuffer
#define Uses_TText
#define Uses_MsgBox
#include <tvision/tv.h>

#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstring>

// ============================================================
// FAKE CONVERSATION - 5 turns, some chunky
// ============================================================
static const char* FAKE_CONVO =
    "[Wib] The thing about creativity is it refuses to sit still. "
    "You pin it down with measurements and metrics and peer-reviewed "
    "journals, and it slips sideways into the gaps between your "
    "instruments. That's not a bug, Wob -- that's the whole point.\n"
    "\n"
    "[Wob] I hear you, but \"slips sideways\" isn't a falsifiable claim. "
    "Creativity operates within physical systems -- neural networks, "
    "biochemical processes, electromagnetic patterns. The fact that we "
    "haven't fully mapped it yet doesn't mean it exists in some mystical "
    "beyond. Give me the specific mechanism you think escapes measurement, "
    "and I'll show you someone working on measuring it right now.\n"
    "\n"
    "[Wib] Yeah but the moment you measure the light, you've already "
    "changed what it is -- you've trapped it in your instruments like a "
    "butterfly pinned to cork. The neural patterns aren't creativity, "
    "they're just the body's way of translating something that refuses "
    "to be translated. You're measuring the radio, not the song.\n"
    "\n"
    "[Wob] That's the measurement problem statement, not a refutation. "
    "Yes, observation changes the system -- that's Heisenberg, that's "
    "real. But we account for that in experimental design. The song IS "
    "electromagnetic waves and air pressure and neural firing patterns; "
    "calling it something else doesn't make those mechanisms less true "
    "or less worth understanding. I'm not saying we've finished mapping "
    "creativity -- we haven't. But \"refuses to be translated\" sounds "
    "like poetry, not physics.\n"
    "\n"
    "[Wib] ~(o_o)~ You're measuring the radio AND the song, but every "
    "time you build a better instrument, the song keeps slipping sideways "
    "into a new shape you didn't predict. The constraints aren't gaps -- "
    "they're FEATURES. The universe is fundamentally more creative than "
    "any net we throw over it.\n";


// ============================================================
// V1: Current approach - TView + vector<string> pre-wrap
// Wraps when content is set or window resizes.
// ============================================================
class TWrapV1View : public TView {
    std::vector<std::string> wrappedLines;
    std::string rawText;
    size_t scrollTop;
    TColorAttr color;

    void rewrap() {
        wrappedLines.clear();
        int W = size.x;
        if (W <= 0) return;

        std::istringstream iss(rawText);
        std::string line;
        while (std::getline(iss, line)) {
            if (line.empty()) {
                wrappedLines.push_back("");
                continue;
            }
            size_t pos = 0;
            while (pos < line.size()) {
                size_t remaining = line.size() - pos;
                size_t slice = std::min(remaining, (size_t)W);
                if (remaining > (size_t)W) {
                    size_t bp = line.find_last_of(" \t", pos + W - 1);
                    if (bp != std::string::npos && bp >= pos && (bp - pos) > 0)
                        slice = bp - pos;
                }
                wrappedLines.push_back(line.substr(pos, slice));
                pos += slice;
                while (pos < line.size() && line[pos] == ' ') ++pos;
            }
        }
        if (wrappedLines.empty()) wrappedLines.push_back("");
    }

public:
    TWrapV1View(const TRect& r, const std::string& text)
        : TView(r), rawText(text), scrollTop(0)
    {
        growMode = gfGrowHiX | gfGrowHiY;
        eventMask |= evKeyboard;
        color = TColorAttr(TColorRGB(220, 220, 220), TColorRGB(0, 0, 0));
        rewrap();
    }

    void draw() override {
        int W = size.x, H = size.y;
        for (int y = 0; y < H; ++y) {
            TDrawBuffer b;
            size_t li = scrollTop + y;
            if (li < wrappedLines.size()) {
                std::string vis = wrappedLines[li].substr(0, W);
                int col = 0;
                if (!vis.empty()) {
                    ushort w = b.moveCStr(col, vis.c_str(),
                        TAttrPair{color, color}, W);
                    col += (w > 0 ? w : 0);
                }
                if (col < W) b.moveChar(col, ' ', color, W - col);
            } else {
                b.moveChar(0, ' ', color, W);
            }
            writeLine(0, y, W, 1, b);
        }
    }

    void handleEvent(TEvent& ev) override {
        TView::handleEvent(ev);
        if (ev.what == evKeyDown) {
            bool handled = true;
            switch (ev.keyDown.keyCode) {
                case kbDown:
                    if (scrollTop + size.y < (int)wrappedLines.size()) { scrollTop++; }
                    break;
                case kbUp:
                    if (scrollTop > 0) scrollTop--;
                    break;
                case kbPgDn:
                    scrollTop = std::min(scrollTop + size.y,
                        wrappedLines.size() > (size_t)size.y ? wrappedLines.size() - size.y : (size_t)0);
                    break;
                case kbPgUp:
                    scrollTop = (scrollTop > (size_t)size.y) ? scrollTop - size.y : 0;
                    break;
                default: handled = false;
            }
            if (handled) { drawView(); clearEvent(ev); }
        }
    }

    void changeBounds(const TRect& b) override {
        TView::changeBounds(b);
        rewrap();
        drawView();
    }
};


// ============================================================
// V2: TStaticText-style draw-time wrap using TText::scroll()
// No pre-wrap - wraps on every draw. Uses TV's own word algo.
// ============================================================
class TWrapV2View : public TView {
    std::string rawText;
    size_t scrollTop;
    TColorAttr color;

public:
    TWrapV2View(const TRect& r, const std::string& text)
        : TView(r), rawText(text), scrollTop(0)
    {
        growMode = gfGrowHiX | gfGrowHiY;
        eventMask |= evKeyboard;
        color = TColorAttr(TColorRGB(220, 220, 220), TColorRGB(0, 0, 0));
    }

    void draw() override {
        int W = size.x, H = size.y;
        TStringView s = rawText;
        int l = (int)s.size();
        int p = 0;
        int y = 0;

        // Skip to scrollTop
        int skipped = 0;
        while (p < l && skipped < (int)scrollTop) {
            int i = p;
            int last = i + TText::scroll(s.substr(i), W, False);
            // Word-boundary walk (TStaticText algo)
            int j;
            do {
                j = p;
                while (p < l && s[p] == ' ') ++p;
                while (p < l && s[p] != ' ' && s[p] != '\n')
                    p += TText::next(s.substr(p));
            } while (p < l && p < last && s[p] != '\n');
            if (p > last) {
                p = (j > i) ? j : last;
            }
            while (p < l && s[p] == ' ') ++p;
            if (p < l && s[p] == '\n') ++p;
            ++skipped;
        }

        while (y < H) {
            TDrawBuffer b;
            b.moveChar(0, ' ', color, W);
            if (p < l) {
                int i = p;
                int last = i + TText::scroll(s.substr(i), W, False);
                int j;
                do {
                    j = p;
                    while (p < l && s[p] == ' ') ++p;
                    while (p < l && s[p] != ' ' && s[p] != '\n')
                        p += TText::next(s.substr(p));
                } while (p < l && p < last && s[p] != '\n');
                if (p > last) {
                    p = (j > i) ? j : last;
                }
                int width = strwidth(s.substr(i, p - i));
                b.moveStr(0, s.substr(i), color, (ushort)width);
                while (p < l && s[p] == ' ') ++p;
                if (p < l && s[p] == '\n') ++p;
            }
            writeLine(0, y++, W, 1, b);
        }
    }

    void handleEvent(TEvent& ev) override {
        TView::handleEvent(ev);
        if (ev.what == evKeyDown) {
            bool handled = true;
            switch (ev.keyDown.keyCode) {
                case kbDown: scrollTop++; break;
                case kbUp: if (scrollTop > 0) scrollTop--; break;
                case kbPgDn: scrollTop += size.y; break;
                case kbPgUp: scrollTop = (scrollTop > (size_t)size.y) ? scrollTop - size.y : 0; break;
                default: handled = false;
            }
            if (handled) { drawView(); clearEvent(ev); }
        }
    }
};


// ============================================================
// V3: TScroller with pre-wrapped lines (CLAUDE.md pattern)
// Uses standardScrollBar, delta.y, setLimit.
// ============================================================
class TWrapV3View : public TScroller {
    std::vector<std::string> wrappedLines;
    std::string rawText;
    TColorAttr color;

    void rewrap() {
        wrappedLines.clear();
        int W = size.x;
        if (W <= 0) return;

        std::istringstream iss(rawText);
        std::string line;
        while (std::getline(iss, line)) {
            if (line.empty()) {
                wrappedLines.push_back("");
                continue;
            }
            size_t pos = 0;
            while (pos < line.size()) {
                size_t remaining = line.size() - pos;
                size_t slice = std::min(remaining, (size_t)W);
                if (remaining > (size_t)W) {
                    size_t bp = line.find_last_of(" \t", pos + W - 1);
                    if (bp != std::string::npos && bp >= pos && (bp - pos) > 0)
                        slice = bp - pos;
                }
                wrappedLines.push_back(line.substr(pos, slice));
                pos += slice;
                while (pos < line.size() && line[pos] == ' ') ++pos;
            }
        }
        if (wrappedLines.empty()) wrappedLines.push_back("");
        setLimit(0, (int)wrappedLines.size());
    }

public:
    TWrapV3View(const TRect& r, TScrollBar* vs, const std::string& text)
        : TScroller(r, nullptr, vs), rawText(text)
    {
        growMode = gfGrowHiX | gfGrowHiY;
        color = TColorAttr(TColorRGB(220, 220, 220), TColorRGB(0, 0, 0));
        rewrap();
    }

    void draw() override {
        int W = size.x, H = size.y;
        for (int y = 0; y < H; ++y) {
            TDrawBuffer b;
            size_t li = delta.y + y;
            if (li < wrappedLines.size()) {
                std::string vis = wrappedLines[li].substr(0, W);
                int col = 0;
                if (!vis.empty()) {
                    ushort w = b.moveCStr(col, vis.c_str(),
                        TAttrPair{color, color}, W);
                    col += (w > 0 ? w : 0);
                }
                if (col < W) b.moveChar(col, ' ', color, W - col);
            } else {
                b.moveChar(0, ' ', color, W);
            }
            writeLine(0, y, W, 1, b);
        }
    }

    void changeBounds(const TRect& b) override {
        TScroller::changeBounds(b);
        rewrap();
    }
};


// ============================================================
// V4: Draw-time wrap using TText::scroll() + paragraph model
// Stores paragraphs (split by \n\n), wraps each at draw time.
// More robust for resize - no stale cache.
// ============================================================
class TWrapV4View : public TView {
    std::string rawText;
    size_t scrollTop;
    TColorAttr color;
    TColorAttr speakerColor;

    // Count total wrapped lines for given width (for scroll bounds)
    int countLines(int W) {
        if (W <= 0) return 0;
        TStringView s = rawText;
        int total = 0;
        int p = 0, l = (int)s.size();
        while (p < l) {
            int i = p;
            int last = i + TText::scroll(s.substr(i), W, False);
            int j;
            do {
                j = p;
                while (p < l && s[p] == ' ') ++p;
                while (p < l && s[p] != ' ' && s[p] != '\n')
                    p += TText::next(s.substr(p));
            } while (p < l && p < last && s[p] != '\n');
            if (p > last) p = (j > i) ? j : last;
            while (p < l && s[p] == ' ') ++p;
            if (p < l && s[p] == '\n') ++p;
            ++total;
        }
        return total;
    }

public:
    TWrapV4View(const TRect& r, const std::string& text)
        : TView(r), rawText(text), scrollTop(0)
    {
        growMode = gfGrowHiX | gfGrowHiY;
        eventMask |= evKeyboard;
        color = TColorAttr(TColorRGB(220, 220, 220), TColorRGB(0, 0, 0));
        speakerColor = TColorAttr(TColorRGB(120, 220, 255), TColorRGB(0, 0, 0));
    }

    void draw() override {
        int W = size.x, H = size.y;
        TStringView s = rawText;
        int l = (int)s.size();
        int p = 0;
        int lineNum = 0;
        int y = 0;

        while (p < l && y < H) {
            // Empty line: emit blank row
            if (s[p] == '\n') {
                if (lineNum >= (int)scrollTop) {
                    TDrawBuffer b;
                    b.moveChar(0, ' ', color, W);
                    writeLine(0, y++, W, 1, b);
                }
                ++p;
                ++lineNum;
                continue;
            }

            int i = p;
            int last = i + TText::scroll(s.substr(i), W, False);
            int j;
            do {
                j = p;
                while (p < l && s[p] == ' ') ++p;
                while (p < l && s[p] != ' ' && s[p] != '\n')
                    p += TText::next(s.substr(p));
            } while (p < l && p < last && s[p] != '\n');
            if (p > last) p = (j > i) ? j : last;

            if (lineNum >= (int)scrollTop) {
                TDrawBuffer b;
                b.moveChar(0, ' ', color, W);
                // Color [Wib] and [Wob] tags
                TStringView seg = s.substr(i, p - i);
                TColorAttr lineColor = color;
                if (seg.size() > 5 && seg[0] == '[' &&
                    (seg[1] == 'W') && (seg[4] == ']')) {
                    lineColor = speakerColor;
                }
                int width = strwidth(seg);
                b.moveStr(0, seg, lineColor, (ushort)std::min(width, W));
                writeLine(0, y++, W, 1, b);
            }

            while (p < l && s[p] == ' ') ++p;
            if (p < l && s[p] == '\n') ++p;
            ++lineNum;
        }
        // Fill remaining rows
        while (y < H) {
            TDrawBuffer b;
            b.moveChar(0, ' ', color, W);
            writeLine(0, y++, W, 1, b);
        }
    }

    void handleEvent(TEvent& ev) override {
        TView::handleEvent(ev);
        if (ev.what == evKeyDown) {
            bool handled = true;
            switch (ev.keyDown.keyCode) {
                case kbDown: scrollTop++; break;
                case kbUp: if (scrollTop > 0) scrollTop--; break;
                case kbPgDn: scrollTop += size.y; break;
                case kbPgUp: scrollTop = (scrollTop > (size_t)size.y) ? scrollTop - size.y : 0; break;
                default: handled = false;
            }
            if (handled) { drawView(); clearEvent(ev); }
        }
    }
};


// ============================================================
// V5: Hard-break at width (no word boundary - control case)
// Shows what it looks like WITHOUT word-aware breaking.
// ============================================================
class TWrapV5View : public TView {
    std::vector<std::string> wrappedLines;
    std::string rawText;
    size_t scrollTop;
    TColorAttr color;

    void rewrap() {
        wrappedLines.clear();
        int W = size.x;
        if (W <= 0) return;
        std::istringstream iss(rawText);
        std::string line;
        while (std::getline(iss, line)) {
            if (line.empty()) {
                wrappedLines.push_back("");
                continue;
            }
            // Hard break at exactly W chars - no word boundary
            for (size_t pos = 0; pos < line.size(); pos += W) {
                wrappedLines.push_back(line.substr(pos, W));
            }
        }
        if (wrappedLines.empty()) wrappedLines.push_back("");
    }

public:
    TWrapV5View(const TRect& r, const std::string& text)
        : TView(r), rawText(text), scrollTop(0)
    {
        growMode = gfGrowHiX | gfGrowHiY;
        eventMask |= evKeyboard;
        color = TColorAttr(TColorRGB(220, 220, 220), TColorRGB(0, 0, 0));
        rewrap();
    }

    void draw() override {
        int W = size.x, H = size.y;
        for (int y = 0; y < H; ++y) {
            TDrawBuffer b;
            size_t li = scrollTop + y;
            if (li < wrappedLines.size()) {
                std::string vis = wrappedLines[li].substr(0, W);
                int col = 0;
                if (!vis.empty()) {
                    ushort w = b.moveCStr(col, vis.c_str(),
                        TAttrPair{color, color}, W);
                    col += (w > 0 ? w : 0);
                }
                if (col < W) b.moveChar(col, ' ', color, W - col);
            } else {
                b.moveChar(0, ' ', color, W);
            }
            writeLine(0, y, W, 1, b);
        }
    }

    void handleEvent(TEvent& ev) override {
        TView::handleEvent(ev);
        if (ev.what == evKeyDown) {
            bool handled = true;
            switch (ev.keyDown.keyCode) {
                case kbDown:
                    if (scrollTop + size.y < (int)wrappedLines.size()) scrollTop++;
                    break;
                case kbUp: if (scrollTop > 0) scrollTop--; break;
                case kbPgDn:
                    scrollTop = std::min(scrollTop + size.y,
                        wrappedLines.size() > (size_t)size.y ? wrappedLines.size() - size.y : (size_t)0);
                    break;
                case kbPgUp:
                    scrollTop = (scrollTop > (size_t)size.y) ? scrollTop - size.y : 0;
                    break;
                default: handled = false;
            }
            if (handled) { drawView(); clearEvent(ev); }
        }
    }

    void changeBounds(const TRect& b) override {
        TView::changeBounds(b);
        rewrap();
        drawView();
    }
};


// ============================================================
// Window wrappers for each approach
// ============================================================

class TWrapTestWindow : public TWindow {
public:
    TWrapTestWindow(const TRect& r, const char* title)
        : TWindow(r, title, wnNoNumber),
          TWindowInit(&TWindow::initFrame)
    {
        options |= ofTileable;
    }
};

// V3 needs special construction for scrollbar
class TWrapV3Window : public TWindow {
public:
    TWrapV3Window(const TRect& r)
        : TWindow(r, "V3: TScroller pre-wrap", wnNoNumber),
          TWindowInit(&TWindow::initFrame)
    {
        options |= ofTileable;
        TRect client = getExtent();
        client.grow(-1, -1);
        TScrollBar* vs = standardScrollBar(sbVertical | sbHandleKeyboard);
        auto* view = new TWrapV3View(client, vs, FAKE_CONVO);
        insert(view);
    }
};


// ============================================================
// Application
// ============================================================

class TWrapTestApp : public TApplication {
public:
    TWrapTestApp();
    static TMenuBar* initMenuBar(TRect r);
    static TStatusLine* initStatusLine(TRect r);
    void handleEvent(TEvent& ev) override;

private:
    void spawnAll();
};

TWrapTestApp::TWrapTestApp()
    : TApplication(),
      TProgInit(&TWrapTestApp::initStatusLine,
                &TWrapTestApp::initMenuBar,
                &TApplication::initDeskTop)
{
    spawnAll();
}

void TWrapTestApp::spawnAll() {
    int dw = deskTop->size.x;
    int dh = deskTop->size.y;

    // Calculate window dimensions - 3 across top, 2 across bottom
    int winW = dw / 3;
    int winH = dh / 2;

    // V1: top-left
    {
        TRect r(0, 0, winW, winH);
        auto* w = new TWrapTestWindow(r, "V1: pre-wrap vector");
        TRect c = w->getExtent(); c.grow(-1, -1);
        w->insert(new TWrapV1View(c, FAKE_CONVO));
        deskTop->insert(w);
    }
    // V2: top-center
    {
        TRect r(winW, 0, winW * 2, winH);
        auto* w = new TWrapTestWindow(r, "V2: TStaticText algo");
        TRect c = w->getExtent(); c.grow(-1, -1);
        w->insert(new TWrapV2View(c, FAKE_CONVO));
        deskTop->insert(w);
    }
    // V3: top-right (TScroller)
    {
        TRect r(winW * 2, 0, dw, winH);
        auto* w = new TWrapV3Window(r);
        deskTop->insert(w);
    }
    // V4: bottom-left
    {
        TRect r(0, winH, dw / 2, dh);
        auto* w = new TWrapTestWindow(r, "V4: draw-time TText::scroll");
        TRect c = w->getExtent(); c.grow(-1, -1);
        w->insert(new TWrapV4View(c, FAKE_CONVO));
        deskTop->insert(w);
    }
    // V5: bottom-right (hard break control)
    {
        TRect r(dw / 2, winH, dw, dh);
        auto* w = new TWrapTestWindow(r, "V5: hard-break (control)");
        TRect c = w->getExtent(); c.grow(-1, -1);
        w->insert(new TWrapV5View(c, FAKE_CONVO));
        deskTop->insert(w);
    }
}

TMenuBar* TWrapTestApp::initMenuBar(TRect r) {
    r.b.y = r.a.y + 1;
    return new TMenuBar(r,
        *new TSubMenu("~F~ile", kbAltF) +
            *new TMenuItem("E~x~it", cmQuit, kbAltX)
    );
}

TStatusLine* TWrapTestApp::initStatusLine(TRect r) {
    r.a.y = r.b.y - 1;
    return new TStatusLine(r,
        *new TStatusDef(0, 0xFFFF) +
            *new TStatusItem("~Alt-X~ Exit", kbAltX, cmQuit) +
            *new TStatusItem("~Up/Down~ Scroll", kbNoKey, 0) +
            *new TStatusItem("~PgUp/PgDn~ Page", kbNoKey, 0)
    );
}

void TWrapTestApp::handleEvent(TEvent& ev) {
    TApplication::handleEvent(ev);
}

int main() {
    TWrapTestApp app;
    app.run();
    app.shutDown();
    return 0;
}
