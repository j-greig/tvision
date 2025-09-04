/*---------------------------------------------------------*/
/*                                                         */
/*   wallpaper.cpp - Desktop Wallpaper Implementation     */
/*   WIBWOBWORLD ASCII Art Background                     */
/*                                                         */
/*---------------------------------------------------------*/

#include "wallpaper.h"
#include <cstring>

// ASCII art for WIBWOBWORLD - stored exactly as provided
const char* TWallpaperView::asciiArt[] = {
    "  \xDB     \xDB\xB0 \xDB\xDB\xDB \xDC\xDC\xDC\xDC    \xDB     \xDB\xB0 \xB2\xDB\xDB\xDB\xDB\xDB   \xDC\xDC\xDC\xDC   ",
    "\xDB\xDB\xB0 \xDB \xB0\xDB\xB0\xDB\xDB\xDB\xB2\xDB\xDB\xDB\xDB\xDB\xDC \xDB\xDB\xB0 \xDB \xB0\xDB\xB0\xB2\xDB\xDB\xB2  \xDB\xDB\xB2\xDB\xDB\xDB\xDB\xDB\xDC ",
    "\xB2\xDB\xB0 \xDB \xB0\xDB \xB2\xDB\xDB\xB2\xB2\xDB\xDB\xB2 \xDC\xDB\xDB\xB2\xDB\xB0 \xDB \xB0\xDB \xB2\xDB\xDB\xB0  \xDB\xDB\xB2\xB2\xDB\xDB\xB2 \xDC\xDB\xDB",
    "\xB0\xDB\xB0 \xDB \xB0\xDB \xB0\xDB\xDB\xB0\xB2\xDB\xDB\xB0\xDB\xDF  \xB0\xDB\xB0 \xDB \xB0\xDB \xB2\xDB\xDB   \xDB\xDB\xB0\xB2\xDB\xDB\xB0\xDB\xDF  ",
    "\xB0\xB0\xDB\xDB\xB2\xDB\xDB\xDB \xB0\xDB\xDB\xB0\xB0\xDB\xDB  \xDF\xDB\xDB\xB0\xB0\xDB\xDB\xB2\xDB\xDB\xDB \xB0 \xDB\xDB\xDB\xDB\xDB\xB2\xB0\xB0\xDB\xDB  \xDF\xDB\xDB",
    "\xB0 \xDB\xB0\xB2 \xB2  \xB0\xDB  \xB0\xB2\xDB\xDB\xDB\xDB\xDF\xB2\xB0 \xDB\xB0\xB2 \xB2  \xB0 \xB2\xB0\xB2\xB0\xB2\xB0 \xB0\xB2\xDB\xDB\xDB\xDB\xDF\xB2",
    "  \xB2 \xB0 \xB0   \xB2 \xB0\xB2\xB0\xB2   \xB0   \xB2 \xB0 \xB0    \xB0 \xB2 \xB2\xB0 \xB2\xB0\xB2   \xB0 ",
    "  \xB0   \xB0   \xB2 \xB0 \xB0    \xB0   \xB0   \xB0  \xB0 \xB0 \xB0 \xB2   \xB0    \xB0 ",
    " \xDB     \xDB\xB0 \xB2\xDB\xDB\xDB\xDB\xDB   \xDB\xDB\xDF\xDB\xDB\xDB   \xDB\xDB\xDB    \xDB\xDB\xDB\xDB\xDB\xDB\xDC     ",
    "\xDB\xDB\xB0 \xDB \xB0\xDB\xB0\xB2\xDB\xDB\xB2  \xDB\xDB\xB2\xDB\xDB\xDB \xB2 \xDB\xDB\xB2\xDB\xDB\xDB\xB2    \xB2\xDB\xDB\xDF \xDB\xDB\xDC    ",
    "\xB2\xDB\xB0 \xDB \xB0\xDB \xB2\xDB\xDB\xB0  \xDB\xDB\xB2\xDB\xDB\xDB \xB0\xDC\xDB \xB2\xB2\xDB\xDB\xB0    \xB0\xDB\xDB   \xDB\xDC    ",
    "\xB0\xDB\xB0 \xDB \xB0\xDB \xB2\xDB\xDB   \xDB\xDB\xB0\xB2\xDB\xDB\xDF\xDF\xDB\xDC  \xB2\xDB\xDB\xB0    \xB0\xDB\xDC\xDC   \xDC    ",
    "\xB0\xB0\xDB\xDB\xB2\xDB\xDB\xDB \xB0 \xDB\xDB\xDB\xDB\xDB\xB2\xB0\xB0\xDB\xDB\xDB \xB2\xDB\xDB\xB2\xB0\xDB\xDB\xDB\xDB\xDB\xDB\xB2\xB0\xB2\xDB\xDB\xDB\xDB\xDB     ",
    "\xB0 \xDB\xB0\xB2 \xB2  \xB0 \xB2\xB0\xB2\xB0\xB2\xB0 \xB0 \xB2\xDB \xB0\xB2\xDB\xB0\xB0 \xB2\xB0\xDB  \xB0 \xB2\xB2\xDB  \xB2     ",
    "  \xB2 \xB0 \xB0    \xB0 \xB2 \xB2\xB0   \xB0\xB2 \xB0 \xB2\xB0\xB0 \xB0 \xB2  \xB0 \xB0 \xB2  \xB2     ",
    "  \xB0   \xB0  \xB0 \xB0 \xB0 \xB2    \xB0\xB0   \xB0   \xB0 \xB0    \xB0 \xB0  \xB0     ",
    "    \xB0        \xB0 \xB0     \xB0         \xB0       \xB0       "
};

const int TWallpaperView::artWidth = 51;
const int TWallpaperView::artHeight = 17;

TWallpaperView::TWallpaperView(const TRect& bounds) : TBackground(bounds, ' ')
{
    // Use plain space for pure black background
    pattern = ' ';
}

void TWallpaperView::draw()
{
    TDrawBuffer b;
    TColorRGB trueBlack(0, 0, 0);  // Pure RGB black
    TColorAttr bgColor(trueBlack, trueBlack);  // Pure black background
    TAttrPair artColor = getArtColor(); // White
    
    // Calculate centering offsets
    int xOffset = (size.x > artWidth) ? (size.x - artWidth) / 2 : 0;
    int yOffset = (size.y > artHeight) ? (size.y - artHeight) / 2 : 0;
    
    for (int y = 0; y < size.y; y++)
    {
        // First, fill entire line with black spaces
        for (int x = 0; x < size.x; x++)
        {
            b.putChar(x, ' ');
            b.putAttribute(x, bgColor);
        }
        
        // Check if we're in the art region
        int artY = y - yOffset;
        if (artY >= 0 && artY < artHeight && xOffset < size.x)
        {
            // Draw the art line exactly as stored
            const char* line = asciiArt[artY];
            int lineLen = strlen(line);
            
            for (int x = 0; x < lineLen && (x + xOffset) < size.x; x++)
            {
                if (x + xOffset >= 0)
                {
                    unsigned char ch = (unsigned char)line[x];
                    if (ch != ' ')  // Don't draw spaces
                    {
                        TColorAttr color;
                        TColorRGB trueBlack(0, 0, 0);  // Pure RGB black
                        
                        // Use RGB colors to bypass terminal palette interpretation
                        if (ch == 0xB0)  // Light shade ░
                        {
                            TColorRGB darkGray(64, 64, 64);
                            color = TColorAttr(darkGray, trueBlack);
                        }
                        else if (ch == 0xB1)  // Medium shade ▒
                        {
                            TColorRGB lightGray(128, 128, 128);
                            color = TColorAttr(lightGray, trueBlack);
                        }
                        else if (ch == 0xB2)  // Dark shade ▓
                        {
                            TColorRGB mediumGray(96, 96, 96);
                            color = TColorAttr(mediumGray, trueBlack);
                        }
                        else if (ch == 0xDB)  // Full block █
                        {
                            TColorRGB white(255, 255, 255);
                            color = TColorAttr(white, trueBlack);
                        }
                        else if (ch == 0xDC)  // Lower half block ▄
                        {
                            TColorRGB mediumGray(96, 96, 96);
                            color = TColorAttr(mediumGray, trueBlack);
                        }
                        else if (ch == 0xDF)  // Upper half block ▀
                        {
                            TColorRGB mediumGray(96, 96, 96);
                            color = TColorAttr(mediumGray, trueBlack);
                        }
                        else
                        {
                            TColorRGB white(255, 255, 255);
                            color = TColorAttr(white, trueBlack);
                        }
                        
                        b.putChar(x + xOffset, ch);
                        b.putAttribute(x + xOffset, color);
                    }
                }
            }
        }
        
        writeLine(0, y, size.x, 1, b);
    }
}

void TWallpaperView::drawCenteredArt(TDrawBuffer& b, int y)
{
    // This method is called by draw() for each line
    // Implementation merged into main draw() method for better control
}

TAttrPair TWallpaperView::getArtColor()
{
    // Bright white for the art
    return getColor(0x0F);
}

TAttrPair TWallpaperView::getBgColor()
{
    // Use RGB true black instead of palette color 0 to bypass terminal color scheme
    TColorRGB trueBlack(0, 0, 0);  // Pure black RGB
    TAttrPair black;
    black = TColorAttr(trueBlack, trueBlack);  // Both foreground and background black
    return black;
}

TPalette& TWallpaperView::getPalette() const
{
    // Use desktop palette
    static TPalette palette("\x07\x07\x0F\x07\x70\x70\x70\x07", 8);
    return palette;
}