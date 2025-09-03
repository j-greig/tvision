/*---------------------------------------------------------*/
/*                                                         */
/*   test_pattern_app.cpp - Test Pattern Window Spawner   */
/*   Unlimited resizable windows with test patterns       */
/*                                                         */
/*---------------------------------------------------------*/

#define Uses_TKeys
#define Uses_TApplication
#define Uses_TEvent
#define Uses_TRect
#define Uses_TDialog
#define Uses_TStaticText
#define Uses_TButton
#define Uses_TMenuBar
#define Uses_TSubMenu
#define Uses_TMenuItem
#define Uses_TMenu
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
#define Uses_MsgBox
#define Uses_cmTile
#define Uses_cmCascade
#include <tvision/tv.h>

#include "test_pattern.h"
#include "gradient.h"
#include "wallpaper.h"
#include <sstream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <sys/stat.h>

// Configuration - Toggle pattern display mode
// true  = Continuous mode (pattern flows like text, wraps at line ends creating diagonals)
// false = Tiled mode (pattern resets at start of each line, crops at edges)
bool USE_CONTINUOUS_PATTERN = true;  // Made non-const so it can be changed at runtime

// Command constants
const ushort cmNewWindow = 100;
const ushort cmScreenshot = 101;
const ushort cmNewGradientH = 102;
const ushort cmNewGradientV = 103;
const ushort cmNewGradientR = 104;
const ushort cmNewGradientD = 105;
const ushort cmPatternContinuous = 106;
const ushort cmPatternTiled = 107;

// Forward declarations
class TTestPatternView;
class TTestPatternWindow;
class TGradientWindow;
class TTestPatternApp;
class TCustomMenuBar;

/*---------------------------------------------------------*/
/* TCustomMenuBar - Menu bar with right-aligned kaomoji   */
/*---------------------------------------------------------*/
class TCustomMenuBar : public TMenuBar
{
public:
    TCustomMenuBar(const TRect& bounds, TMenu* aMenu) : TMenuBar(bounds, aMenu) {}
    TCustomMenuBar(const TRect& bounds, TSubMenu& aMenu) : TMenuBar(bounds, aMenu) {}
    
    virtual void draw()
    {
        // Draw the menu bar with custom content
        TDrawBuffer b;
        TMenuItem *p;
        short x;
        
        TAttrPair cNormal = getColor(0x0301);
        TAttrPair cSelect = getColor(0x0604);
        TAttrPair cNormDisabled = getColor(0x0202);
        TAttrPair cSelDisabled = getColor(0x0505);
        
        // Fill background
        b.moveChar(0, ' ', cNormal, size.x);
        
        // Draw menu items from left
        if (menu != 0) {
            x = 1;
            p = menu->items;
            while (p != 0) {
                if (p->name != 0) {
                    int l = cstrlen(p->name);
                    if (x + l < size.x) {
                        TAttrPair color;
                        if (p->disabled)
                            color = (p == current) ? cSelDisabled : cNormDisabled;
                        else
                            color = (p == current) ? cSelect : cNormal;
                        
                        b.moveChar(x, ' ', color, 1);
                        b.moveCStr(x + 1, p->name, color);
                        b.moveChar(x + l + 1, ' ', color, 1);
                    }
                    x += l + 2;
                }
                p = p->next;
            }
        }
        
        // Add kaomoji at right side
        const char* kaomoji = "つ◕‿◕‿◕༽つ";
        int kaomojiWidth = 12;
        int xPos = size.x - kaomojiWidth; // Removed the -1 to move it right to the edge
        
        if (xPos > x) { // Only draw if there's space
            b.moveStr(xPos, kaomoji, cNormal);
        }
        
        writeBuf(0, 0, size.x, 1, b);
    }
};

/*---------------------------------------------------------*/
/* TTestPatternView - The interior view showing pattern   */
/*---------------------------------------------------------*/
class TTestPatternView : public TView
{
    
public:
    TTestPatternView(const TRect& bounds) : TView(bounds)
    {
        options |= ofFramed;
        growMode = gfGrowHiX | gfGrowHiY;
    }
    
    virtual void draw()
    {
        TDrawBuffer b;
        int patternHeight = TTestPattern::getPatternHeight();
        
        for (int y = 0; y < size.y; y++)
        {
            int patternRow = y % patternHeight;
            int offset = 0;
            
            // Calculate offset for continuous patterns
            if (USE_CONTINUOUS_PATTERN) {
                offset = (y / patternHeight) * size.x;
            }
            
            TTestPattern::drawPatternRow(b, patternRow, size.x, offset);
            writeLine(0, y, size.x, 1, b);
        }
    }
};

/*---------------------------------------------------------*/
/* TTestPatternWindow - Window containing test pattern    */
/*---------------------------------------------------------*/
class TTestPatternWindow : public TWindow
{
private:
    TTestPatternView* patternView;
    
public:
    TTestPatternWindow(const TRect& bounds, const char* aTitle) :
        TWindow(bounds, aTitle, wnNoNumber),
        TWindowInit(&TTestPatternWindow::initFrame)
    {
        options |= ofTileable;  // Enable cascade/tile functionality
        
        // Get the interior bounds (excluding frame)
        TRect interior = getExtent();
        interior.grow(-1, -1);
        
        // Insert the test pattern view
        patternView = new TTestPatternView(interior);
        insert(patternView);
    }
    
    TTestPatternView* getPatternView() { return patternView; }
    
};

/*---------------------------------------------------------*/
/* TGradientWindow - Window containing gradient           */
/*---------------------------------------------------------*/
class TGradientWindow : public TWindow
{
public:
    enum GradientType {
        gtHorizontal,
        gtVertical,
        gtRadial,
        gtDiagonal
    };
    
    TGradientWindow(const TRect& bounds, const char* aTitle, GradientType type) :
        TWindow(bounds, aTitle, wnNoNumber),
        TWindowInit(&TGradientWindow::initFrame)
    {
        options |= ofTileable;  // Enable cascade/tile functionality
        
        // Get the interior bounds (excluding frame)
        TRect interior = getExtent();
        interior.grow(-1, -1);
        
        // Insert the appropriate gradient view
        TGradientView* gradientView = nullptr;
        switch (type)
        {
            case gtHorizontal:
                gradientView = new THorizontalGradientView(interior);
                break;
            case gtVertical:
                gradientView = new TVerticalGradientView(interior);
                break;
            case gtRadial:
                gradientView = new TRadialGradientView(interior);
                break;
            case gtDiagonal:
                gradientView = new TDiagonalGradientView(interior);
                break;
        }
        
        if (gradientView)
            insert(gradientView);
    }
};

/*---------------------------------------------------------*/
/* TTestPatternApp - Main application class               */
/*---------------------------------------------------------*/
class TTestPatternApp : public TApplication
{
public:
    TTestPatternApp();
    virtual void handleEvent(TEvent& event);
    virtual void idle();
    virtual TPalette& getPalette() const;
    static TMenuBar* initMenuBar(TRect);
    static TStatusLine* initStatusLine(TRect);
    static TDeskTop* initDeskTop(TRect);
    
private:
    void newTestWindow();
    void newGradientWindow(TGradientWindow::GradientType type);
    void cascade();
    void tile();
    void closeAll();
    void takeScreenshot();
    void setPatternMode(bool continuous);
    
    int windowNumber;
    static const int maxWindows = 99;
};

TTestPatternApp::TTestPatternApp() :
    TProgInit(&TTestPatternApp::initStatusLine,
              &TTestPatternApp::initMenuBar,
              &TTestPatternApp::initDeskTop),
    windowNumber(0)
{
    // Create first window automatically
    newTestWindow();
}

void TTestPatternApp::handleEvent(TEvent& event)
{
    TApplication::handleEvent(event);
    
    if (event.what == evCommand)
    {
        switch (event.message.command)
        {
            case cmNewWindow:
                newTestWindow();
                clearEvent(event);
                break;
            case cmNewGradientH:
                newGradientWindow(TGradientWindow::gtHorizontal);
                clearEvent(event);
                break;
            case cmNewGradientV:
                newGradientWindow(TGradientWindow::gtVertical);
                clearEvent(event);
                break;
            case cmNewGradientR:
                newGradientWindow(TGradientWindow::gtRadial);
                clearEvent(event);
                break;
            case cmNewGradientD:
                newGradientWindow(TGradientWindow::gtDiagonal);
                clearEvent(event);
                break;
            case cmPatternContinuous:
                setPatternMode(true);
                clearEvent(event);
                break;
            case cmPatternTiled:
                setPatternMode(false);
                clearEvent(event);
                break;
            case cmScreenshot:
                takeScreenshot();
                clearEvent(event);
                break;
            case cmCascade:
                cascade();
                clearEvent(event);
                break;
            case cmTile:
                tile();
                clearEvent(event);
                break;
            case cmCloseAll:
                closeAll();
                clearEvent(event);
                break;
            default:
                break;
        }
    }
}

void TTestPatternApp::newTestWindow()
{
    // Create window title
    windowNumber++;
    std::stringstream title;
    title << "Test Pattern " << windowNumber;
    
    // Calculate window position (cascade effect)
    int offset = (windowNumber - 1) % 10;
    TRect bounds(
        2 + offset * 2,           // left
        1 + offset,               // top
        50 + offset * 2,          // right
        15 + offset               // bottom
    );
    
    // Create and insert window
    TTestPatternWindow* window = new TTestPatternWindow(bounds, title.str().c_str());
    deskTop->insert(window);
}

void TTestPatternApp::cascade()
{
    deskTop->cascade(deskTop->getExtent());
}

void TTestPatternApp::tile()
{
    deskTop->tile(deskTop->getExtent());
}

void TTestPatternApp::closeAll()
{
    // Close all windows on desktop
    message(deskTop, evCommand, cmCloseAll, 0);
}

void TTestPatternApp::newGradientWindow(TGradientWindow::GradientType type)
{
    // Create window title
    windowNumber++;
    std::stringstream title;
    
    switch (type)
    {
        case TGradientWindow::gtHorizontal:
            title << "Horizontal Gradient " << windowNumber;
            break;
        case TGradientWindow::gtVertical:
            title << "Vertical Gradient " << windowNumber;
            break;
        case TGradientWindow::gtRadial:
            title << "Radial Gradient " << windowNumber;
            break;
        case TGradientWindow::gtDiagonal:
            title << "Diagonal Gradient " << windowNumber;
            break;
    }
    
    // Calculate window position (cascade effect)
    int offset = (windowNumber - 1) % 10;
    TRect bounds(
        2 + offset * 2,           // left
        1 + offset,               // top
        50 + offset * 2,          // right
        15 + offset               // bottom
    );
    
    // Create and insert window
    TGradientWindow* window = new TGradientWindow(bounds, title.str().c_str(), type);
    deskTop->insert(window);
}

void TTestPatternApp::setPatternMode(bool continuous)
{
    USE_CONTINUOUS_PATTERN = continuous;
    
    // Show confirmation message
    std::string mode = continuous ? "Continuous (Diagonal)" : "Tiled (Cropped)";
    std::stringstream msg;
    msg << "Pattern mode set to: " << mode;
    messageBox(msg.str().c_str(), mfInformation | mfOKButton);
}


void TTestPatternApp::takeScreenshot()
{
    // Create screenshots directory if it doesn't exist
    mkdir("screenshots", 0755);
    
    // Generate timestamp for filename
    time_t rawtime;
    struct tm* timeinfo;
    char timestamp[80];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", timeinfo);
    
    // Build screenshot command for macOS
    std::stringstream cmd;
    
    // Try to detect terminal application
    const char* termProgram = getenv("TERM_PROGRAM");
    std::string appName = "Terminal"; // default
    
    if (termProgram) {
        std::string term(termProgram);
        if (term == "iTerm.app") {
            appName = "iTerm2";
        } else if (term == "Apple_Terminal") {
            appName = "Terminal";
        }
    }
    
    // Build the screenshot command
    cmd << "screencapture -x -l$(osascript -e 'tell app \"" 
        << appName 
        << "\" to id of window 1' 2>/dev/null) "
        << "screenshots/tui_" << timestamp << ".png 2>/dev/null";
    
    // Execute the screenshot command
    int result = system(cmd.str().c_str());
    
    // Show result message
    if (result == 0) {
        std::stringstream msg;
        msg << "Screenshot saved to screenshots/tui_" << timestamp << ".png";
        messageBox(msg.str().c_str(), mfInformation | mfOKButton);
    } else {
        messageBox("Screenshot failed. Try manual capture with Cmd+Shift+4", 
                   mfError | mfOKButton);
    }
}

// Custom monochrome palette with reversed main areas
// Palette indices:
// 0-7:   Desktop/Background (black background with pattern)
// 8-15:  Window frame colors 
// 16-23: Cyan window (alternate)
// 24-31: Gray window (alternate)
// 32-39: Dialog colors
// 40-47: Menu colors (reversed to black bg)
// 48-55: Status line colors (reversed to black bg)
// 56-63: Help colors
// 64+:   Additional elements

#define cpMonochrome \
    "\x07\x07\x0F\x07\x70\x70\x70\x07\x0F\x07\x07\x07\x70\x07\x0F" \
    "\x70\x0F\x70\x07\x07\x70\x07\x0F\x70\x7F\x7F\x70\x07\x70\x07\x0F" \
    "\x70\x7F\x7F\x70\x07\x70\x70\x7F\x7F\x07\x70\x0F\x70\x0F\x70\x07" \
    "\x0F\x0F\x0F\x70\x0F\x07\x70\x70\x70\x07\x70\x0F\x07\x07\x78\x00" \
    "\x70\x0F\x0F\x70\x07\x70\x70\x0F\x0F\x07\x0F\x7F\x08\x7F\x08\x70" \
    "\x7F\x7F\x7F\x0F\x70\x70\x07\x70\x70\x70\x07\x7F\x70\x07\x08\x00" \
    "\x70\x7F\x7F\x70\x07\x70\x70\x7F\x7F\x07\x0F\x0F\x78\x0F\x78\x07" \
    "\x0F\x0F\x0F\x70\x0F\x07\x70\x70\x70\x07\x70\x0F\x07\x07\x78\x00" \
    "\x07\x0F\x07\x70\x70\x07\x0F\x70"

TPalette& TTestPatternApp::getPalette() const
{
    static TPalette palette(cpMonochrome, sizeof(cpMonochrome)-1);
    return palette;
}

TMenuBar* TTestPatternApp::initMenuBar(TRect r)
{
    r.b.y = r.a.y + 1;
    
    return new TCustomMenuBar(r,
        *new TSubMenu("~F~ile", kbAltF) +
            *new TMenuItem("~N~ew Test Window", cmNewWindow, kbCtrlN) +
            (TMenuItem&) (
                *new TSubMenu("New ~G~radient", kbNoKey) +
                    *new TMenuItem("~H~orizontal", cmNewGradientH, kbNoKey) +
                    *new TMenuItem("~V~ertical", cmNewGradientV, kbNoKey) +
                    *new TMenuItem("~R~adial", cmNewGradientR, kbNoKey) +
                    *new TMenuItem("~D~iagonal", cmNewGradientD, kbNoKey)
            ) +
            newLine() +
            *new TMenuItem("~S~creenshot", cmScreenshot, kbCtrlS) +
            newLine() +
            *new TMenuItem("~C~ascade", cmCascade, kbNoKey) +
            *new TMenuItem("~T~ile", cmTile, kbNoKey) +
            *new TMenuItem("C~l~ose All", cmCloseAll, kbNoKey) +
            newLine() +
            *new TMenuItem("E~x~it", cmQuit, cmQuit, hcNoContext, "Alt-X") +
        *new TSubMenu("~P~attern", kbAltP) +
            *new TMenuItem(USE_CONTINUOUS_PATTERN ? "\x04 Continuous (Diagonal)" : "  Continuous (Diagonal)", 
                          cmPatternContinuous, kbNoKey) +
            *new TMenuItem(!USE_CONTINUOUS_PATTERN ? "\x04 Tiled (Cropped)" : "  Tiled (Cropped)", 
                          cmPatternTiled, kbNoKey) +
        *new TSubMenu("~W~indow", kbAltW) +
            *new TMenuItem("~M~ove", cmResize, kbCtrlF5) +
            *new TMenuItem("~Z~oom", cmZoom, kbF5) +
            *new TMenuItem("~N~ext", cmNext, kbF6) +
            *new TMenuItem("~P~revious", cmPrev, kbShiftF6) +
            *new TMenuItem("~C~lose", cmClose, kbAltF3)
    );
}

TStatusLine* TTestPatternApp::initStatusLine(TRect r)
{
    r.a.y = r.b.y - 1;
    return new TStatusLine(r,
        *new TStatusDef(0, 0xFFFF) +
            *new TStatusItem("~Alt-X~ Exit", kbAltX, cmQuit) +
            *new TStatusItem("~Ctrl-N~ New Window", kbCtrlN, cmNewWindow) +
            *new TStatusItem("~F5~ Zoom", kbF5, cmZoom) +
            *new TStatusItem("~F6~ Next", kbF6, cmNext) +
            *new TStatusItem("~Alt-F3~ Close", kbAltF3, cmClose) +
            *new TStatusItem("~F10~ Menu", kbF10, cmMenu) +
            *new TStatusItem("~F11~ Quantum Printer", kbF11, cmMenu)
    );
}

TDeskTop* TTestPatternApp::initDeskTop(TRect r)
{
    r.a.y = 1;
    r.b.y--;
    // Create desktop with standard constructor
    TDeskTop* desktop = new TDeskTop(r);
    
    // Remove the default background if it exists
    if (desktop->background != 0)
    {
        desktop->remove(desktop->background);
        destroy(desktop->background);
        desktop->background = 0;
    }
    
    // Insert our custom wallpaper as the new background
    // Use desktop->getExtent() to match the desktop's internal coordinate system
    TWallpaperView* wallpaper = new TWallpaperView(desktop->getExtent());
    desktop->insert(wallpaper);
    desktop->background = wallpaper;
    
    return desktop;
}


void TTestPatternApp::idle()
{
    TApplication::idle();
    
    // No animated views anymore
}

int main()
{
    TTestPatternApp app;
    app.run();
    return 0;
}