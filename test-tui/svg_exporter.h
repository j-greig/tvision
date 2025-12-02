/*---------------------------------------------------------*/
/*                                                         */
/*   svg_exporter.h - Standalone TVision SVG Exporter      */
/*   Exports current TScreen buffer to a single SVG file   */
/*                                                         */
/*   NOTE: This module is self-contained and not wired     */
/*   into any app menus yet. Safe to compile separately.   */
/*                                                         */
/*---------------------------------------------------------*/

#ifndef TV_TEST_TUI_SVG_EXPORTER_H
#define TV_TEST_TUI_SVG_EXPORTER_H

#define Uses_TScreen
#define Uses_TScreenCell
#define Uses_TColorAttr
#include <tvision/tv.h>

#include <string>

struct SvgExportOptions {
    // Cell metrics (pixels)
    float cellWidth = 9.0f;
    float cellHeight = 16.0f;
    // Baseline offset from the top of the cell (pixels)
    float baseline = 12.0f;
    // CSS font family string
    std::string fontFamily = "Courier New, monospace";
    // Optimization flags
    bool mergeBackgroundRuns = true;
    bool groupTextRuns = true;
    bool skipWideCharTrails = true; // always recommended
    bool preserveSpaces = true;     // set xml:space="preserve" on <text>
};

// Saves the current TVision screen buffer to an SVG file.
// Returns true on success; false on I/O error or missing screen buffer.
bool saveCurrentScreenAsSvg(const std::string &filePath,
                            const SvgExportOptions &opt = {});

// Converts the current screen buffer into an SVG string without writing to disk.
// Useful for tests or piping to other systems.
std::string screenToSvgString(const SvgExportOptions &opt = {});

#endif // TV_TEST_TUI_SVG_EXPORTER_H

