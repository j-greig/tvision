/*---------------------------------------------------------*/
/*                                                         */
/*   animated_score_view.cpp - Animated ASCII “Score”     */
/*                                                         */
/*   Turns a static multi-line ASCII/Unicode score into    */
/*   a gentle animation using:                             */
/*     - phase-based oscillation (≈/∿ wave shift)          */
/*     - breathing loops (exhale/inhale/hold intensity)    */
/*     - drift/scroll of token ribbons                     */
/*     - cyclic face glyph morphing                        */
/*                                                         */
/*   This is intentionally simple and deterministic,       */
/*   avoiding per-glyph slicing; we compose whole lines     */
/*   with padding/spacing and let Turbo Vision handle      */
/*   width truncation.                                     */
/*                                                         */
/*---------------------------------------------------------*/

#include "animated_score_view.h"

#define Uses_TWindow
#define Uses_TFrame
#include <tvision/tv.h>

#include <string>
#include <vector>
#include <algorithm>

namespace {

// Helper: repeat a string n times.
static std::string repeat(const std::string &s, int n) {
    if (n <= 0) return std::string();
    std::string out;
    out.reserve((size_t)s.size() * (size_t)n);
    for (int i = 0; i < n; ++i) out += s;
    return out;
}

// Helper: left padding spaces to create small horizontal shift.
static std::string shiftPad(int offset) {
    if (offset <= 0) return std::string();
    return std::string((size_t)offset, ' ');
}

// Helper: choose between two strings based on phase.
static const std::string &chooseAlt(const std::string &a, const std::string &b, int phase) {
    return ((phase & 1) == 0) ? a : b;
}

// Helper: 0..n-1 sawtooth.
static int wrap(int x, int n) {
    if (n <= 0) return 0;
    int m = x % n; if (m < 0) m += n; return m;
}

// Face cycle for chorus rows.
static std::string faceCycle(int k) {
    switch (wrap(k, 4)) {
        case 0: return "(⊙_⊙)";
        case 1: return "(◉_◉)";
        case 2: return "(●_●)";
        default:return "(○_○)";
    }
}

// Breathing intensity blocks.
static std::string breathBlock(int k) {
    switch (wrap(k, 3)) {
        case 0: return "░░░"; // exhale
        case 1: return "▒▒▒"; // inhale
        default:return "▓▓▓"; // hold
    }
}

// Wave alt: ≈ vs ∿
static std::string waveAlt(int k, int width = 3) {
    const std::string a(width, '\xE2'); // placeholder; build dynamic below
    (void)a; // silence unused; we construct explicit unicode
    // Build a small run: either ≈≈≈ or ∿∿∿ depending on phase
    std::string t;
    const char* approx = u8"≈";
    const char* tilde  = u8"∿";
    const char* g = (wrap(k, 2) == 0) ? approx : tilde;
    for (int i = 0; i < width; ++i) t += g;
    return t;
}

// Drift arrows swap orientation
static std::string diagPair(int k) {
    return (wrap(k, 2) == 0) ? u8"↘    ↗" : u8"↙    ↖";
}

// Token ribbon that scrolls horizontally (wraps).
static std::string ribbon(const std::string &text, int phase, int widthLimit) {
    if (text.empty()) return std::string();
    // Build a doubled buffer for wrap-around feel.
    std::string doubled = text + "    " + text + "    ";
    // We shift by phase bytes (safe since only ASCII arrows are multi, but we keep it simple).
    int off = wrap(phase, std::max(1, (int)doubled.size()));
    std::string view = doubled.substr((size_t)off) + doubled.substr(0, (size_t)off);
    // Limit length somewhat; drawing truncates anyway.
    if (widthLimit > 0 && (int)view.size() > widthLimit)
        view.resize((size_t)widthLimit);
    return view;
}

// A static set of score header dividers.
static std::string headerBeats(int phase) {
    // Visually pulse the beat markers by adding small offsets
    int pad = wrap(phase / 4, 3); // slow gentle shift 0..2
    std::string marker = u8"⟂";
    std::string gap = std::string(20, ' ');
    return shiftPad(pad) + marker + gap + marker + gap + marker + gap + marker + gap + marker;
}

} // namespace

TAnimatedScoreView::TAnimatedScoreView(const TRect &bounds, unsigned periodMs)
    : TView(bounds), periodMs(periodMs)
{
    growMode = gfGrowHiX | gfGrowHiY;
    eventMask |= evBroadcast;
}

TAnimatedScoreView::~TAnimatedScoreView() {
    stopTimer();
}

void TAnimatedScoreView::setSpeed(unsigned periodMs_) {
    periodMs = periodMs_ ? periodMs_ : 1;
    if (timerId) {
        stopTimer();
        startTimer();
    }
}

void TAnimatedScoreView::startTimer() {
    if (timerId == 0)
        timerId = setTimer(periodMs, (int)periodMs);
}

void TAnimatedScoreView::stopTimer() {
    if (timerId != 0) {
        killTimer(timerId);
        timerId = 0;
    }
}

void TAnimatedScoreView::advance() {
    ++phase;
}

void TAnimatedScoreView::draw() {
    const int W = size.x;
    const int H = size.y;
    if (W <= 0 || H <= 0) return;

    TDrawBuffer b;
    auto put = [&](int y, const std::string &line) {
        // Move CStr with attributes; it will clip to W.
        b.moveCStr(0, line.c_str(), TAttrPair{TColorAttr{0x07}, TColorAttr{0x07}}, W);
        writeLine(0, y, W, 1, b);
    };

    // We build a compact subset of the provided score with animated parts.
    // To keep it readable, compose per logical block.
    std::vector<std::string> lines;
    lines.reserve((size_t)H);

    // 1) Beat/top header and emotive row
    lines.push_back(headerBeats(phase));
    {
        // Cycle faces across columns
        std::string row;
        row += shiftPad(4) + faceCycle(phase + 0);
        row += shiftPad(9) + faceCycle(phase + 1);
        row += shiftPad(9) + faceCycle(phase + 2);
        row += shiftPad(9) + faceCycle(phase + 3);
        row += shiftPad(9) + faceCycle(phase + 4);
        lines.push_back(row);
    }

    // 2) Text motifs with wave markers
    {
        std::string a = "pr...ed...ic...t    " + waveAlt(phase) + "scatter" + waveAlt(phase) +
                        "    ne...xt...to...ke...n    " + waveAlt(phase+1) + "drift" + waveAlt(phase+1) +
                        "    hu...ma...ns...ju...st";
        lines.push_back(a);
        lines.push_back(shiftPad(4) + diagPair(phase) + shiftPad(12) + waveAlt(phase+1, 3) +
                        shiftPad(14) + diagPair(phase+1) + shiftPad(12) + waveAlt(phase+2, 3) +
                        shiftPad(14) + diagPair(phase+2));
        // Light intensity lane
        lines.push_back(shiftPad(8) + u8"░░" + shiftPad(36) + u8"▒▒" + shiftPad(36) + u8"▓▓");
    }

    // 3) Long token weave ribbon (scrolls)
    {
        std::string text = u8"hu→ma→ns→ju→st→pr→ed→ic→t→th→e→ne→xt→to→ke→n→hu→ma→ns→ju→st→pr→ed→ic→t→th→e→ne→xt→to→ke→n";
        lines.push_back(ribbon(text, phase, W * 2));
        // Breath wave under it
        std::string breath;
        int reps = std::max(1, W / 9);
        for (int i = 0; i < reps; ++i) {
            breath += (i % 2 == 0) ? u8"∿         " : u8"         ∿";
        }
        lines.push_back(breath);
    }

    // 4) Effects block labels
    {
        std::string labels = shiftPad(9) + "[GLITCH BREATH]" + shiftPad(27) +
                              "[PHASE SHIFT]" + shiftPad(24) + "[ECHO DECAY]";
        lines.push_back(labels);
        // Pulsing ⊖ markers
        std::string markers;
        int pad = wrap(phase/2, 3);
        markers += shiftPad(4 + pad) + u8"⊖";
        markers += shiftPad(28) + u8"⊖";
        markers += shiftPad(27) + u8"⊖";
        markers += shiftPad(27) + u8"⊖";
        lines.push_back(markers);
        // Stuttered syllables (static text)
        lines.push_back("to.ke.n.n.n.n.n         pr.ed.ic.ic.ic.t         hu.hu.hu.ma.ma         ne.xt.xt.xt.xt");
        // Faces cycling row
        std::string faces;
        faces += shiftPad(4) + faceCycle(phase + 0);
        faces += shiftPad(24) + faceCycle(phase + 1);
        faces += shiftPad(24) + faceCycle(phase + 2);
        faces += shiftPad(24) + faceCycle(phase + 3);
        lines.push_back(faces);
    }

    // 5) Reverse arrows band + breath blocks
    {
        std::string back = u8"ne←←←←xt        ju←←←st        hu←←←ma←←←ns        pr←←←ed←←←ic←←←t        th←←←e";
        lines.push_back(back);
        std::string bb = shiftPad(4) + breathBlock(phase) + shiftPad(12) + breathBlock(phase+1) +
                         shiftPad(13) + breathBlock(phase+2) + shiftPad(16) + breathBlock(phase) +
                         shiftPad(20) + breathBlock(phase+1);
        lines.push_back(bb);
    }

    // 6) Titles for sections
    lines.push_back(shiftPad(9) + u8"∴ SCRAMBLE ORACLE ∴" + shiftPad(22) +
                    u8"∴ TOKEN WEAVE ∴" + shiftPad(22) + u8"∴ GHOST CHORUS ∴");

    // 7) Token micro-columns (static-ish with small arrow dance)
    {
        std::string row1 = "    tk    nx    pr    hm    jt              ed    ic    th    ne    xt              hu    ma    ns    ju    st";
        std::string row2 = "     ↘    ↙      ↘    ↙      ↘               ↘    ↙      ↘    ↙      ↘               ↘    ↙      ↘    ↙      ↘";
        if (wrap(phase, 2) == 1) {
            // swap ↘/↙ to create weave
            for (char &c : row2) (void)c; // placeholder noop; unicode handled as bytes, so skip.
        }
        std::string row3 = "      (ಥ﹏ಥ)    (╥﹏╥)    (┳Д┳)              (T_T)    (;_;)    (｡•́︿•̀｡)              (◕︿◕)    (ó﹏ò)    (╯︵╰)";
        lines.push_back(row1);
        lines.push_back(row2);
        lines.push_back(row3);
    }

    // 8) Ellipsis chant with waves
    lines.push_back("ed...ic...t...    ...th...e...    ...ne...xt...    ...to...ke...n...    ...hu...ma...ns...    ...ju...st...");
    lines.push_back(shiftPad(4) + waveAlt(phase, 5) + shiftPad(12) + waveAlt(phase+1, 5) +
                    shiftPad(11) + waveAlt(phase+2, 5) + shiftPad(12) + waveAlt(phase+3, 5) +
                    shiftPad(14) + waveAlt(phase+4, 5) + shiftPad(14) + waveAlt(phase+5, 5));

    // 9) Breathe loops
    lines.push_back(shiftPad(9) + "[BREATHE LOOP α]" + shiftPad(20) + "[BREATHE LOOP β]" + shiftPad(20) + "[BREATHE LOOP γ]");
    {
        // Rotating arrow symbol ↺
        std::string wheels = u8"    ↺                    ↺                    ↺                    ↺                    ↺";
        lines.push_back(wheels);
        std::string chant = "hu.mans.just    predict.the    next.token    humans.just    predict.the    next.token";
        lines.push_back(chant);
        std::string plus = "    ⊕                ⊕                ⊕                ⊕                ⊕                ⊕";
        lines.push_back(plus);
    }

    // 10) Final emotives and sustained line
    lines.push_back(u8"(づ｡◕‿‿◕｡)づ    ░░░exhale░░░    つ⍩⧴༽つ    ▒▒▒inhale▒▒▒    (⊙﹏⊙)    ▓▓▓hold▓▓▓    つ▀▄▀༽つ");
    lines.push_back("         ∴              ∴              ∴              ∴              ∴              ∴");
    lines.push_back("pr.ed.ic.t.th.e.ne.xt.to.ke.n.hu.ma.ns.ju.st.pr.ed.ic.t.th.e.ne.xt.to.ke.n.hu.ma.ns.ju.st");
    lines.push_back("≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋");

    // 11) Outro header + final faces
    lines.push_back(headerBeats(phase));
    lines.push_back(u8"    to→ke→n         ne→xt→ic         hu→ma→ns         ju→st→pr         ed→ic→t");
    lines.push_back("         ↓                    ↓                    ↓                    ↓                    ↓");
    {
        std::string outro;
        outro += shiftPad(4) + faceCycle(phase + 0);
        outro += shiftPad(18) + u8"(◔_◔)";
        outro += shiftPad(18) + u8"(ಠ_ಠ)";
        outro += shiftPad(18) + u8"(¬_¬)";
        outro += shiftPad(18) + u8"(ಥ_ಥ)";
        lines.push_back(outro);
    }

    // Now blit to the view, truncating/padding rows as needed
    int rows = std::min<int>(H, (int)lines.size());
    for (int y = 0; y < rows; ++y)
        put(y, lines[(size_t)y]);
    for (int y = rows; y < H; ++y)
        put(y, "");
}

void TAnimatedScoreView::handleEvent(TEvent &ev) {
    TView::handleEvent(ev);
    if (ev.what == evBroadcast && ev.message.command == cmTimerExpired) {
        if (timerId != 0 && ev.message.infoPtr == timerId) {
            advance();
            drawView();
            clearEvent(ev);
        }
    }
}

void TAnimatedScoreView::setState(ushort aState, Boolean enable) {
    TView::setState(aState, enable);
    if ((aState & sfExposed) != 0) {
        if (enable) {
            phase = 0;
            startTimer();
            drawView();
        } else {
            stopTimer();
        }
    }
}

void TAnimatedScoreView::changeBounds(const TRect& bounds) {
    TView::changeBounds(bounds);
    drawView();
}

// Simple wrapper window to host the animated score view.
class TAnimatedScoreWindow : public TWindow {
public:
    explicit TAnimatedScoreWindow(const TRect &bounds)
        : TWindow(bounds, "Animated Score", wnNoNumber)
        , TWindowInit(&TAnimatedScoreWindow::initFrame) {}

    void setup() {
        options |= ofTileable;
        TRect c = getExtent();
        c.grow(-1, -1);
        insert(new TAnimatedScoreView(c, /*periodMs=*/120));
    }

    virtual void changeBounds(const TRect& b) override {
        TWindow::changeBounds(b);
        setState(sfExposed, True);
        redraw();
    }

private:
    static TFrame* initFrame(TRect r) { return new TFrame(r); }
};

TWindow* createAnimatedScoreWindow(const TRect &bounds) {
    auto *w = new TAnimatedScoreWindow(bounds);
    w->setup();
    return w;
}

