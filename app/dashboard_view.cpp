#include "dashboard_view.h"

#include <sstream>
#include <cstdlib>
#include <cstring>
#include <algorithm>

// ─── Color Palette ────────────────────────────────────────────
// Classic Turbo Vision light theme — matches TV frame colors.
// bg: light cyan/blue (168,168,210) to sit with default TV palette.

static const TColorRGB bgMain(168, 184, 210);   // Soft blue-grey
static const TColorRGB bgAltA(158, 174, 200);   // Table row A
static const TColorRGB bgAltB(148, 164, 190);   // Table row B
static const TColorRGB bgHeader(80, 100, 150);   // Table header

static TColorAttr cBackground()   { return TColorAttr(TColorRGB(30, 30, 50),    bgMain); }
static TColorAttr cLabel()        { return TColorAttr(TColorRGB(20, 20, 100),    bgMain); }
static TColorAttr cProgressFill() { return TColorAttr(TColorRGB(20, 140, 40),    bgMain); }
static TColorAttr cProgressEmpty(){ return TColorAttr(TColorRGB(140, 150, 170),  bgMain); }
static TColorAttr cProgressText() { return TColorAttr(TColorRGB(30, 30, 60),     bgMain); }
static TColorAttr cKvKey()        { return TColorAttr(TColorRGB(20, 50, 120),    bgMain); }
static TColorAttr cKvValue()      { return TColorAttr(TColorRGB(10, 10, 30),     bgMain); }
static TColorAttr cSeparator()    { return TColorAttr(TColorRGB(130, 145, 170),  bgMain); }
static TColorAttr cTableHeader()  { return TColorAttr(TColorRGB(255, 255, 255),  bgHeader); }
static TColorAttr cTableRowA()    { return TColorAttr(TColorRGB(20, 20, 40),     bgAltA); }
static TColorAttr cTableRowB()    { return TColorAttr(TColorRGB(20, 20, 40),     bgAltB); }
static TColorAttr cStatusRun()    { return TColorAttr(TColorRGB(20, 140, 40),    bgMain); }
static TColorAttr cStatusIdle()   { return TColorAttr(TColorRGB(160, 140, 20),   bgMain); }
static TColorAttr cStatusErr()    { return TColorAttr(TColorRGB(180, 30, 30),    bgMain); }

// ─── Helpers ──────────────────────────────────────────────────

std::vector<std::string> TDashboardView::split(const std::string& s, char delim) {
    std::vector<std::string> parts;
    std::istringstream iss(s);
    std::string tok;
    while (std::getline(iss, tok, delim)) {
        parts.push_back(tok);
    }
    return parts;
}

// ─── TDashboardView ──────────────────────────────────────────

TDashboardView::TDashboardView(const TRect& bounds)
    : TView(bounds)
{
    growMode = gfGrowHiX | gfGrowHiY;
    options |= ofFramed;
}

void TDashboardView::parseContent(const std::string& content) {
    items_.clear();
    std::istringstream stream(content);
    std::string line;

    while (std::getline(stream, line)) {
        if (line.empty()) continue;
        // Trim \r if present
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        std::vector<std::string> parts = split(line, '|');
        if (parts.empty()) continue;

        DashItem item;
        std::string type = parts[0];

        if (type == "LABEL" && parts.size() >= 2) {
            item.type = DashItemType::Label;
            item.label = parts[1];
        } else if (type == "PROGRESS" && parts.size() >= 4) {
            item.type = DashItemType::ProgressBar;
            item.label = parts[1];
            item.numValue = std::atoi(parts[2].c_str());
            item.maxValue = std::atoi(parts[3].c_str());
        } else if (type == "KV" && parts.size() >= 3) {
            item.type = DashItemType::KeyValue;
            item.label = parts[1];
            item.value = parts[2];
        } else if (type == "SEPARATOR") {
            item.type = DashItemType::Separator;
        } else if (type == "STATUS" && parts.size() >= 3) {
            item.type = DashItemType::Status;
            item.label = parts[1];
            item.value = parts[2]; // "running", "idle", "error"
        } else if (type == "TABLE_HDR" && parts.size() >= 2) {
            item.type = DashItemType::TableHeader;
            item.columns.assign(parts.begin() + 1, parts.end());
        } else if (type == "TABLE_ROW" && parts.size() >= 2) {
            item.type = DashItemType::TableRow;
            item.columns.assign(parts.begin() + 1, parts.end());
        } else {
            // Unknown type, treat as label
            item.type = DashItemType::Label;
            item.label = line;
        }

        items_.push_back(item);
    }

    drawView();
}

void TDashboardView::setItems(const std::vector<DashItem>& items) {
    items_ = items;
    drawView();
}

void TDashboardView::draw() {
    int w = size.x;
    int h = size.y;
    int tableRowIdx = 0;

    int row = 0;
    for (size_t i = 0; i < items_.size() && row < h; ++i) {
        TDrawBuffer b;
        // Clear line with background
        b.moveChar(0, ' ', cBackground(), w);

        switch (items_[i].type) {
            case DashItemType::Label:
                drawLabel(b, items_[i]);
                break;
            case DashItemType::ProgressBar:
                drawProgressBar(b, items_[i]);
                break;
            case DashItemType::KeyValue:
                drawKeyValue(b, items_[i]);
                break;
            case DashItemType::Separator:
                drawSeparator(b);
                break;
            case DashItemType::Status:
                drawStatus(b, items_[i]);
                break;
            case DashItemType::TableHeader:
                drawTableHeader(b, items_[i]);
                tableRowIdx = 0;
                break;
            case DashItemType::TableRow:
                drawTableRow(b, items_[i], (tableRowIdx % 2) == 1);
                tableRowIdx++;
                break;
        }

        writeLine(0, row, w, 1, b);
        row++;
    }

    // Fill remaining rows with background
    for (; row < h; ++row) {
        TDrawBuffer b;
        b.moveChar(0, ' ', cBackground(), w);
        writeLine(0, row, w, 1, b);
    }
}

// ─── Per-Type Renderers ───────────────────────────────────────

void TDashboardView::drawLabel(TDrawBuffer& b, const DashItem& item) {
    int w = size.x;
    // Centered bold label
    int textLen = (int)item.label.size();
    int pad = std::max(0, (w - textLen) / 2);
    b.moveStr(pad, item.label.c_str(), cLabel());
}

void TDashboardView::drawProgressBar(TDrawBuffer& b, const DashItem& item) {
    int w = size.x;
    int barWidth = w - 4; // 2 padding each side

    // Label on left
    int labelLen = std::min((int)item.label.size(), barWidth / 3);
    b.moveStr(2, item.label.c_str(), cProgressText());

    // Bar starts after label
    int barStart = labelLen + 4;
    int barLen = barWidth - labelLen - 10; // leave room for percentage
    if (barLen < 4) barLen = 4;

    double ratio = (item.maxValue > 0) ? (double)item.numValue / item.maxValue : 0.0;
    if (ratio > 1.0) ratio = 1.0;
    int filled = (int)(ratio * barLen);
    int empty = barLen - filled;

    // Draw bar: [====----]
    b.moveChar(barStart, '[', cProgressText(), 1);
    if (filled > 0) {
        b.moveChar(barStart + 1, '=', cProgressFill(), filled);
    }
    if (empty > 0) {
        b.moveChar(barStart + 1 + filled, '-', cProgressEmpty(), empty);
    }
    b.moveChar(barStart + 1 + barLen, ']', cProgressText(), 1);

    // Percentage text
    int pct = (int)(ratio * 100);
    char pctStr[16];
    std::snprintf(pctStr, sizeof(pctStr), " %d/%d %d%%", item.numValue, item.maxValue, pct);
    b.moveStr(barStart + barLen + 2, pctStr, cProgressText());
}

void TDashboardView::drawKeyValue(TDrawBuffer& b, const DashItem& item) {
    b.moveStr(2, item.label.c_str(), cKvKey());
    int valueStart = std::max((int)item.label.size() + 4, 16);
    b.moveStr(valueStart, item.value.c_str(), cKvValue());
}

void TDashboardView::drawSeparator(TDrawBuffer& b) {
    int w = size.x;
    b.moveChar(1, '-', cSeparator(), w - 2);
}

void TDashboardView::drawStatus(TDrawBuffer& b, const DashItem& item) {
    TColorAttr dotColor = cStatusIdle();
    char dot = '*';
    if (item.value == "running" || item.value == "ok" || item.value == "done") {
        dotColor = cStatusRun();
    } else if (item.value == "error" || item.value == "fail") {
        dotColor = cStatusErr();
    }

    b.moveChar(2, dot, dotColor, 1);
    b.moveStr(4, item.label.c_str(), cKvKey());

    int stateStart = std::max((int)item.label.size() + 6, 20);
    TColorAttr stateColor = dotColor;
    b.moveStr(stateStart, item.value.c_str(), stateColor);
}

void TDashboardView::drawTableHeader(TDrawBuffer& b, const DashItem& item) {
    int w = size.x;
    // Fill entire row with header background
    b.moveChar(0, ' ', cTableHeader(), w);

    int colWidth = (w - 2) / std::max(1, (int)item.columns.size());
    for (size_t c = 0; c < item.columns.size(); ++c) {
        int x = 2 + (int)c * colWidth;
        b.moveStr(x, item.columns[c].c_str(), cTableHeader());
    }
}

void TDashboardView::drawTableRow(TDrawBuffer& b, const DashItem& item, bool alt) {
    int w = size.x;
    TColorAttr rowColor = alt ? cTableRowB() : cTableRowA();
    b.moveChar(0, ' ', rowColor, w);

    int colWidth = (w - 2) / std::max(1, (int)item.columns.size());
    for (size_t c = 0; c < item.columns.size(); ++c) {
        int x = 2 + (int)c * colWidth;
        b.moveStr(x, item.columns[c].c_str(), rowColor);
    }
}

// ─── TDashboardWindow ────────────────────────────────────────

TDashboardWindow::TDashboardWindow(const TRect& bounds, const char* title)
    : TWindow(bounds, title, wnNoNumber),
      TWindowInit(&TDashboardWindow::initFrame),
      dashView_(nullptr)
{
    // Interior rect (inside frame)
    TRect interior = getExtent();
    interior.grow(-1, -1);

    dashView_ = new TDashboardView(interior);
    insert(dashView_);
}

void TDashboardWindow::setContent(const std::string& content) {
    if (dashView_) {
        dashView_->parseContent(content);
    }
}
