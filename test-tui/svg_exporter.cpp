/*---------------------------------------------------------*/
/*                                                         */
/*   svg_exporter.cpp - Standalone TVision SVG Exporter    */
/*                                                         */
/*---------------------------------------------------------*/

#include "svg_exporter.h"

#include <sstream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <algorithm>

// Helpers to work with TVision color API.
static inline std::string cssHexFromRGB(TColorRGB rgb)
{
    uint32_t v = (uint32_t) rgb; // 0xRRGGBB
    std::ostringstream oss;
    oss << '#' << std::hex << std::setw(6) << std::setfill('0') << (v & 0xFFFFFF);
    return oss.str();
}

static inline std::string cssHexFore(const TColorAttr &attr)
{
    return cssHexFromRGB(getFore(attr).asRGB());
}

static inline std::string cssHexBack(const TColorAttr &attr)
{
    return cssHexFromRGB(getBack(attr).asRGB());
}

static inline bool isWideTrail(const TScreenCell &cell)
{
    return cell._ch.isWideCharTrail();
}

// XML escaper that handles TVision's extended character set safely.
static inline std::string xmlEscape(const std::string &s)
{
    std::string out;
    out.reserve(s.size() + 8);
    
    for (size_t i = 0; i < s.size(); ++i)
    {
        unsigned char c = s[i];
        
        // Check for UTF-8 replacement character sequence (0xEF 0xBF 0xBD = �)
        if (c == 0xEF && i + 2 < s.size() && 
            (unsigned char)s[i+1] == 0xBF && 
            (unsigned char)s[i+2] == 0xBD)
        {
            out += ' '; // Replace with space
            i += 2; // Skip the next 2 bytes
            continue;
        }
        
        // Handle control characters
        if (c < 0x20 && c != 0x09 && c != 0x0A && c != 0x0D)
        {
            out += ' '; // Replace control chars with space
        }
        // Handle problematic single-byte characters (0x80-0xFF range)
        // These are likely CP437/CP850 or other legacy encodings, not UTF-8
        else if (c >= 0x80)
        {
            // Common problematic characters that cause XML parsing errors
            switch (c)
            {
                case 0xB0: out += "°"; break;  // Degree symbol (often causes issues)
                case 0xB1: out += "░"; break;  // Light shade
                case 0xB2: out += "▒"; break;  // Medium shade  
                case 0xB3: out += "▓"; break;  // Dark shade
                case 0xC4: out += "─"; break;  // Horizontal line
                case 0xC3: out += "├"; break;  // T-junction left
                case 0xB4: out += "┤"; break;  // T-junction right
                case 0xC2: out += "┬"; break;  // T-junction top
                case 0xC1: out += "┴"; break;  // T-junction bottom
                case 0xC5: out += "┼"; break;  // Cross
                case 0xC0: out += "└"; break;  // Bottom left corner
                case 0xD9: out += "┘"; break;  // Bottom right corner
                case 0xC9: out += "┌"; break;  // Top left corner
                case 0xBB: out += "┐"; break;  // Top right corner
                case 0xBA: out += "│"; break;  // Vertical line
                default:
                    // For other high-bit chars, replace with safe character
                    out += '?'; 
                    break;
            }
        }
        // Handle XML special characters
        else
        {
            switch (c)
            {
                case '&': out += "&amp;"; break;
                case '<': out += "&lt;"; break;
                case '>': out += "&gt;"; break;
                case '"': out += "&quot;"; break;
                case '\'': out += "&apos;"; break;
                default: out.push_back((char) c); break;
            }
        }
    }
    return out;
}

// Render background rectangles per row with run-length merging by background color.
static void renderBackgrounds(std::ostringstream &svg,
                              const SvgExportOptions &opt,
                              int cols, int rows,
                              const TScreenCell *buffer)
{
    for (int y = 0; y < rows; ++y)
    {
        int x = 0;
        while (x < cols)
        {
            const TScreenCell &cell = buffer[y * cols + x];
            std::string bg = cssHexBack(cell.attr);
            int runStart = x;
            int runEnd = x + 1;
            if (opt.mergeBackgroundRuns)
            {
                while (runEnd < cols)
                {
                    const TScreenCell &c2 = buffer[y * cols + runEnd];
                    if (cssHexBack(c2.attr) != bg)
                        break;
                    ++runEnd;
                }
            }
            float rx = runStart * opt.cellWidth;
            float ry = y * opt.cellHeight;
            float rw = (runEnd - runStart) * opt.cellWidth;
            float rh = opt.cellHeight;
            svg << "  <rect x=\"" << rx << "\" y=\"" << ry
                << "\" width=\"" << rw << "\" height=\"" << rh
                << "\" fill=\"" << bg << "\"/>\n";
            x = runEnd;
        }
    }
}

// Text run key groups by foreground + style bits.
struct TextKey {
    TColorDesired fg;
    ushort style;
};

static inline bool sameKey(const TextKey &a, const TextKey &b)
{
    return a.style == b.style && a.fg == b.fg;
}

// Map style flags to CSS inline style.
static inline std::string styleToCss(ushort style)
{
    std::string css;
    if (style & slBold) css += "font-weight:bold;";
    if (style & slItalic) css += "font-style:italic;";
    if (style & slUnderline) css += "text-decoration:underline;";
    return css;
}

static void renderText(std::ostringstream &svg,
                       const SvgExportOptions &opt,
                       int cols, int rows,
                       const TScreenCell *buffer)
{
    // One <text> element per contiguous run with same fg+style in a row.
    for (int y = 0; y < rows; ++y)
    {
        int x = 0;
        while (x < cols)
        {
            const TScreenCell &cell = buffer[y * cols + x];

            if (opt.skipWideCharTrails && isWideTrail(cell))
            {
                ++x; // Skip trail cells
                continue;
            }

            // Build a run
            TextKey key { getFore(cell.attr), getStyle(cell.attr) };
            std::string runText;
            int runStart = x;
            int advance = 0;

            auto emitCell = [&](const TScreenCell &c) {
                // Fetch UTF-8 text (may include combining marks).
                TStringView tv = c._ch.getText();
                std::string s(tv.begin(), tv.size());
                runText += s;
                advance += c.isWide() ? 2 : 1;
            };

            emitCell(cell);
            int xi = x + 1;
            if (opt.groupTextRuns)
            {
                while (xi < cols)
                {
                    const TScreenCell &c2 = buffer[y * cols + xi];
                    if (opt.skipWideCharTrails && isWideTrail(c2))
                    {
                        ++xi;
                        continue;
                    }
                    TextKey key2 { getFore(c2.attr), getStyle(c2.attr) };
                    if (!sameKey(key, key2))
                        break;
                    emitCell(c2);
                    ++xi;
                }
            }

            // Emit SVG <text> for the run.
            float tx = runStart * opt.cellWidth;
            float ty = y * opt.cellHeight + opt.baseline;
            std::string fill = cssHexFromRGB(key.fg.asRGB());
            std::string css = styleToCss(key.style);

            svg << "  <text x=\"" << tx << "\" y=\"" << ty << "\" fill=\"" << fill
                << "\" font-family=\"" << xmlEscape(opt.fontFamily) << "\" font-size=\""
                << (opt.cellHeight * 0.85f) << "\"";
            if (opt.preserveSpaces)
                svg << " xml:space=\"preserve\"";
            if (!css.empty())
                svg << " style=\"" << css << "\"";
            svg << ">" << xmlEscape(runText) << "</text>\n";

            x = xi;
        }
    }
}

std::string screenToSvgString(const SvgExportOptions &opt)
{
    if (!TScreen::screenBuffer || TScreen::screenWidth == 0 || TScreen::screenHeight == 0)
        return std::string();

    const int cols = TScreen::screenWidth;
    const int rows = TScreen::screenHeight;
    const TScreenCell *buffer = TScreen::screenBuffer;

    std::ostringstream svg;
    const float W = opt.cellWidth * cols;
    const float H = opt.cellHeight * rows;

    svg << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << W
        << "\" height=\"" << H << "\" viewBox=\"0 0 " << W << ' ' << H << "\">\n";

    // Optional: solid base background (fallback if rows*cols are empty).
    svg << "  <rect x=\"0\" y=\"0\" width=\"" << W << "\" height=\"" << H
        << "\" fill=\"" << cssHexFromRGB(getBack(TColorAttr{}).asRGB()) << "\"/>\n";

    renderBackgrounds(svg, opt, cols, rows, buffer);
    renderText(svg, opt, cols, rows, buffer);

    svg << "</svg>\n";
    return svg.str();
}

bool saveCurrentScreenAsSvg(const std::string &filePath, const SvgExportOptions &opt)
{
    if (!TScreen::screenBuffer || TScreen::screenWidth == 0 || TScreen::screenHeight == 0)
        return false;
    std::string svg = screenToSvgString(opt);
    if (svg.empty())
        return false;
    std::ofstream ofs(filePath, std::ios::out | std::ios::trunc);
    if (!ofs.is_open())
        return false;
    ofs << svg;
    return ofs.good();
}

