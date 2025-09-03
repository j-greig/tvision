/*---------------------------------------------------------*/
/*                                                         */
/*   frame_file_player_view.cpp - ASCII frame player      */
/*   Reads file, splits by '----' lines, timer-advances   */
/*                                                         */
/*---------------------------------------------------------*/

#include "frame_file_player_view.h"

#include <fstream>
#include <sstream>

FrameFilePlayerView::FrameFilePlayerView(const TRect &bounds, const std::string &path, unsigned periodMs)
    : TView(bounds), periodMs(periodMs) {
    growMode = gfGrowHiX | gfGrowHiY;
    // Receive timer expirations via broadcast events (cmTimerExpired).
    // See TProgram::idle() and TTimerQueue in the library.
    eventMask |= evBroadcast;
    loadAndIndex(path);
}

FrameFilePlayerView::~FrameFilePlayerView() {
    stopTimer();
}

void FrameFilePlayerView::startTimer() {
    if (timerId == 0)
        // Periodic, UI-thread timer. First timeout == period.
        timerId = setTimer(periodMs, (int)periodMs);
}

void FrameFilePlayerView::stopTimer() {
    if (timerId != 0) {
        killTimer(timerId);
        timerId = 0;
    }
}

void FrameFilePlayerView::advanceFrame() {
    if (!frames.empty())
        frameIndex = (frameIndex + 1) % frames.size();
}

size_t FrameFilePlayerView::nextLineStart(const std::string &s, size_t pos) {
    // Advance to index after '\n' (or EOF)
    size_t n = s.size();
    while (pos < n) {
        char c = s[pos++];
        if (c == '\n') break;
    }
    return pos;
}

size_t FrameFilePlayerView::findLineEnd(const std::string &s, size_t pos, size_t limit) {
    // Return index of '\n' or limit (exclusive). Does not skip '\n'.
    size_t i = pos;
    limit = std::min(limit, s.size());
    while (i < limit && s[i] != '\n') ++i;
    return i; // points at '\n' or limit
}

void FrameFilePlayerView::loadAndIndex(const std::string &path) {
    // Read whole file
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in) {
        loadOk = false;
        errorMsg = "Failed to open file: " + path;
        return;
    }
    std::ostringstream oss;
    oss << in.rdbuf();
    fileData = oss.str();

    frames.clear();
    size_t n = fileData.size();
    size_t pos = 0;

    // Optional header FPS=... on first line.
    // If present, use it to refine the period unless the caller already
    // provided a ctor 'periodMs' override (the caller controls precedence).
    if (pos < n) {
        size_t lineEnd = findLineEnd(fileData, pos, n);
        size_t pureEnd = lineEnd;
        if (pureEnd > pos && fileData[pureEnd - 1] == '\r') --pureEnd;
        if (pureEnd - pos >= 4 && fileData.compare(pos, 4, "FPS=") == 0) {
            // Parse simple integer value
            int fps = 0;
            try {
                fps = std::stoi(fileData.substr(pos + 4, pureEnd - (pos + 4)));
            } catch (...) {
                fps = 0;
            }
            if (fps > 0) {
                unsigned p = (unsigned)(1000 / fps);
                if (p == 0) p = 1;
                periodMs = p;
            }
            pos = nextLineStart(fileData, pos);
        }
    }

    // Build frames using delimiter lines exactly equal to "----" (allow CR before LF)
    const std::string delim = "----";
    const size_t npos = std::string::npos;
    size_t curStart = npos;

    while (pos < n) {
        size_t lineStart = pos;
        size_t lineEnd = findLineEnd(fileData, pos, n);    // index of '\n' or n
        size_t pureEnd = lineEnd;
        if (pureEnd > lineStart && fileData[pureEnd - 1] == '\r') --pureEnd;

        bool isDelim = (pureEnd - lineStart == delim.size() &&
                        fileData.compare(lineStart, delim.size(), delim) == 0);

        if (isDelim) {
            if (curStart == npos) {
                // Frame begins after this delimiter
                curStart = nextLineStart(fileData, lineStart);
            } else {
                // Close previous frame before this delimiter line
                frames.push_back(Span{curStart, lineStart});
                curStart = nextLineStart(fileData, lineStart);
            }
        }

        pos = nextLineStart(fileData, pos);
    }

    if (curStart != npos) {
        // If there is trailing content after last delimiter, include it.
        if (curStart <= n)
            frames.push_back(Span{curStart, n});
    } else {
        // No delimiter present: whole file is one frame (may be empty)
        frames.push_back(Span{0, n});
    }

    // Ensure at least one frame to render
    if (frames.empty())
        frames.push_back(Span{0, 0});

    frameIndex = 0;
    loadOk = true;
}

void FrameFilePlayerView::draw() {
    TDrawBuffer buf;
    const int W = size.x, H = size.y;

    // Fallback: fill with spaces if nothing to show
    auto fillRow = [&](int y) {
        buf.moveChar(0, ' ', TColorAttr{0x07}, W);
        writeLine(0, y, W, 1, buf);
    };

    if (W <= 0 || H <= 0) return;

    if (!loadOk || frames.empty()) {
        for (int y = 0; y < H; ++y) fillRow(y);
        return;
    }

    Span s = frames[frameIndex];
    size_t p = s.start;
    size_t end = std::min(s.end, fileData.size());

    for (int y = 0; y < H; ++y) {
        if (p >= end) {
            fillRow(y);
            continue;
        }

        size_t lineEnd = findLineEnd(fileData, p, end);
        size_t pureEnd = lineEnd;
        if (pureEnd > p && fileData[pureEnd - 1] == '\r') --pureEnd;
        size_t len = pureEnd > p ? pureEnd - p : 0;

        // Truncate or pad to width
        int n = (int)std::min<size_t>(len, (size_t)W);
        if (n > 0) {
            std::string line(fileData.data() + p, fileData.data() + p + n);
            buf.moveStr(0, line.c_str(), TColorAttr{0x07});
        }
        if (W > n)
            buf.moveChar(n, ' ', TColorAttr{0x07}, W - n);

        writeLine(0, y, W, 1, buf);

        // Advance to next line (skip '\n' if present)
        p = (lineEnd < end && fileData[lineEnd] == '\n') ? lineEnd + 1 : lineEnd;
    }
}

void FrameFilePlayerView::handleEvent(TEvent &ev) {
    TView::handleEvent(ev);
    if (ev.what == evBroadcast && ev.message.command == cmTimerExpired) {
        // Only act on our own timer; 'infoPtr' carries the TTimerId that fired.
        if (timerId != 0 && ev.message.infoPtr == timerId) {
            advanceFrame();
            drawView();
            clearEvent(ev);
        }
    }
}

void FrameFilePlayerView::setState(ushort aState, Boolean enable) {
    TView::setState(aState, enable);
    if ((aState & sfExposed) != 0) {
        if (enable) {
            frameIndex = 0;
            // Start periodic animation strictly from the UI/event loop.
            // No threads; timer callbacks arrive as cmTimerExpired broadcasts.
            startTimer();
            drawView();
        } else {
            // Pause when hidden to avoid unnecessary work.
            stopTimer();
        }
    }
}

// TTextFileView implementation
TTextFileView::TTextFileView(const TRect &bounds, const std::string &path) : TGroup(bounds)
{
    growMode = gfGrowHiX | gfGrowHiY;
    options |= ofSelectable;
    
    // Create vertical scrollbar on right edge
    TRect r = getExtent();
    r.a.x = r.b.x - 1;
    vScrollBar = new TScrollBar(r);
    insert(vScrollBar);
    
    loadFile(path);
    setLimit();
}

TTextFileView::~TTextFileView()
{
    // ScrollBar will be destroyed by TView destructor
}

void TTextFileView::loadFile(const std::string &path)
{
    std::ifstream file(path);
    if (!file) {
        loadOk = false;
        errorMsg = "Failed to open file: " + path;
        return;
    }
    
    lines.clear();
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    
    loadOk = true;
}

void TTextFileView::setLimit()
{
    if (vScrollBar) {
        int maxLines = std::max(0, (int)lines.size() - size.y);
        vScrollBar->setParams(topLine, 0, maxLines, size.y - 1, 1);
    }
}

void TTextFileView::draw()
{
    if (!needsRedraw) {
        return; // Skip unnecessary redraws during window dragging
    }
    
    TDrawBuffer buf;
    int viewHeight = size.y;
    int viewWidth = size.x - 1; // Leave space for scrollbar
    
    for (int y = 0; y < viewHeight; y++) {
        int lineIndex = topLine + y;
        
        if (lineIndex < (int)lines.size()) {
            // Display line using efficient moveCStr with maxWidth to avoid substr allocation
            const std::string& line = lines[lineIndex];
            TAttrPair attrs{TColorAttr{0x07}, TColorAttr{0x07}};
            
            ushort written = buf.moveCStr(0, line.c_str(), attrs, viewWidth);
            if (written < viewWidth) {
                buf.moveChar(written, ' ', TColorAttr{0x07}, viewWidth - written);
            }
        } else {
            // Empty line
            buf.moveChar(0, ' ', TColorAttr{0x07}, viewWidth);
        }
        
        writeLine(0, y, viewWidth, 1, buf);
    }
    
    needsRedraw = false;
}

void TTextFileView::handleEvent(TEvent &ev)
{
    TGroup::handleEvent(ev);
    
    if (ev.what == evKeyDown) {
        switch (ev.keyDown.keyCode) {
            case kbUp:
                if (topLine > 0) {
                    topLine--;
                    needsRedraw = true;
                    setLimit();
                    drawView();
                }
                clearEvent(ev);
                break;
                
            case kbDown:
                if (topLine + size.y < (int)lines.size()) {
                    topLine++;
                    needsRedraw = true;
                    setLimit();
                    drawView();
                }
                clearEvent(ev);
                break;
                
            case kbPgUp:
                topLine = std::max(0, topLine - size.y);
                needsRedraw = true;
                setLimit();
                drawView();
                clearEvent(ev);
                break;
                
            case kbPgDn:
                topLine = std::min((int)lines.size() - size.y, topLine + size.y);
                if (topLine < 0) topLine = 0;
                needsRedraw = true;
                setLimit();
                drawView();
                clearEvent(ev);
                break;
                
            case kbHome:
                topLine = 0;
                needsRedraw = true;
                setLimit();
                drawView();
                clearEvent(ev);
                break;
                
            case kbEnd:
                topLine = std::max(0, (int)lines.size() - size.y);
                needsRedraw = true;
                setLimit();
                drawView();
                clearEvent(ev);
                break;
        }
    } else if (ev.what == evBroadcast && ev.message.command == cmScrollBarChanged) {
        if (ev.message.infoPtr == vScrollBar) {
            topLine = vScrollBar->value;
            needsRedraw = true;
            drawView();
        }
    }
}

void TTextFileView::changeBounds(const TRect& bounds)
{
    TGroup::changeBounds(bounds);
    needsRedraw = true;  // Trigger redraw when window is resized
    setLimit();          // Update scrollbar limits
}

// Helper function to detect if file contains frame delimiters
bool hasFrameDelimiters(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file) return false;
    
    std::string line;
    while (std::getline(file, line)) {
        // Remove trailing \r if present (CRLF handling)
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line == "----") {
            return true;
        }
    }
    return false;
}
