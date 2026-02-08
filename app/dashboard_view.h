#pragma once

#define Uses_TView
#define Uses_TWindow
#define Uses_TScrollBar
#define Uses_TDrawBuffer
#define Uses_TRect
#define Uses_TColorAttr
#include <tvision/tv.h>

#include <string>
#include <vector>

// ─── Dashboard Item Types ─────────────────────────────────────
// Each line of the content format maps to one of these.
// Format: TYPE|field1|field2|...
//
// LABEL|text              → Bold heading text
// PROGRESS|label|value|max → Colored progress bar
// KV|key|value            → Key-value pair
// SEPARATOR               → Horizontal line
// STATUS|label|state      → Status indicator (running/idle/error)
// TABLE_HDR|col1|col2|... → Table header row
// TABLE_ROW|val1|val2|... → Table data row

enum class DashItemType {
    Label,
    ProgressBar,
    KeyValue,
    Separator,
    Status,
    TableHeader,
    TableRow
};

struct DashItem {
    DashItemType type;
    std::string label;
    std::string value;
    std::vector<std::string> columns;  // for table rows
    int numValue;
    int maxValue;

    DashItem() : type(DashItemType::Label), numValue(0), maxValue(100) {}
};

// ─── TDashboardView ───────────────────────────────────────────
// Custom TView that renders dashboard components using TDrawBuffer.
// Each item type gets its own color scheme and layout.

class TDashboardView : public TView {
public:
    TDashboardView(const TRect& bounds);
    virtual void draw() override;

    // Parse pipe-delimited content format into items
    void parseContent(const std::string& content);

    // Direct item manipulation
    void setItems(const std::vector<DashItem>& items);

private:
    std::vector<DashItem> items_;

    // Per-type renderers
    void drawLabel(TDrawBuffer& b, const DashItem& item);
    void drawProgressBar(TDrawBuffer& b, const DashItem& item);
    void drawKeyValue(TDrawBuffer& b, const DashItem& item);
    void drawSeparator(TDrawBuffer& b);
    void drawStatus(TDrawBuffer& b, const DashItem& item);
    void drawTableHeader(TDrawBuffer& b, const DashItem& item);
    void drawTableRow(TDrawBuffer& b, const DashItem& item, bool alt);

    // Helper: split string by delimiter
    static std::vector<std::string> split(const std::string& s, char delim);
};

// ─── TDashboardWindow ────────────────────────────────────────
// TWindow containing a TDashboardView. Created via API.

class TDashboardWindow : public TWindow {
public:
    TDashboardWindow(const TRect& bounds, const char* title);

    TDashboardView* dashView() { return dashView_; }
    void setContent(const std::string& content);

private:
    TDashboardView* dashView_;
};
