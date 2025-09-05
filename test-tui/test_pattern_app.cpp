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
#define Uses_TFileDialog
#include <tvision/tv.h>

#include "test_pattern.h"
#include "gradient.h"
#include "wallpaper.h"
#include "frame_file_player_view.h"
#include "mech_window.h"
#include <sstream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <sys/stat.h>
#include <cstdio>
#include <fstream>
#include <vector>
#include <cstring>
#include <map>
// Local API IPC bridge (Unix domain socket)
#include "api_ipc.h"

// Configuration - Toggle pattern display mode
// true  = Continuous mode (pattern flows like text, wraps at line ends creating diagonals)
// false = Tiled mode (pattern resets at start of each line, crops at edges)
bool USE_CONTINUOUS_PATTERN = true;  // Made non-const so it can be changed at runtime

// Command constants
// File menu commands
const ushort cmNewWindow = 100;
const ushort cmNewGradientH = 102;
const ushort cmNewGradientV = 103;
const ushort cmNewGradientR = 104;
const ushort cmNewGradientD = 105;
const ushort cmNewDonut = 108;
const ushort cmOpenAnimation = 109;
const ushort cmSaveWorkspace = 110;
const ushort cmNewMechs = 111;
const ushort cmOpenWorkspace = 115;
// Future File commands
const ushort cmOpenAnsiArt = 112;
const ushort cmNewPaintCanvas = 113;
const ushort cmOpenImageFile = 114;

// Edit menu commands
const ushort cmScreenshot = 101;
const ushort cmPatternContinuous = 106;
const ushort cmPatternTiled = 107;
// Edit menu commands
const ushort cmSettings = 117;

// View menu commands
const ushort cmWallpaperWibwob = 118;
const ushort cmWallpaperCustom = 119;
const ushort cmWallpaperDisable = 120;
const ushort cmZoomIn = 121;
const ushort cmZoomOut = 122;
const ushort cmActualSize = 123;
const ushort cmFullScreen = 124;

// Tools menu commands (future)
const ushort cmAnsiEditor = 125;
const ushort cmPaintTools = 126;
const ushort cmAnimationStudio = 127;
const ushort cmQuantumPrinter = 128;

// Help menu commands
const ushort cmAbout = 129;
const ushort cmKeyboardShortcuts = 130;
const ushort cmDebugInfo = 131;

// Forward declarations
class TTestPatternView;
class TTestPatternWindow;
class TGradientWindow;
class TFrameAnimationWindow;
class TTestPatternApp;
class TCustomMenuBar;
class TCustomStatusLine;

/*---------------------------------------------------------*/
/* TCustomMenuBar - Menu bar with right-aligned kaomoji   */
/*---------------------------------------------------------*/
class TCustomMenuBar : public TMenuBar
{
public:
    TCustomMenuBar(const TRect& bounds, TMenu* aMenu) : TMenuBar(bounds, aMenu) {}
    TCustomMenuBar(const TRect& bounds, TSubMenu& aMenu) : TMenuBar(bounds, aMenu) {}
    
    virtual TColorAttr mapColor(uchar index) noexcept override
    {
        TColorRGB trueBlack(0, 0, 0);
        TColorRGB trueWhite(255, 255, 255);
        
        // Experiment with different indices to find which controls hotkeys
        // getColor(0x0301) uses indices 1 and 3, getColor(0x0604) uses indices 4 and 6
        switch(index) {
            case 1:  // First try index 1 (might be hotkey for getColor(0x0301))
            case 3:  // Or index 3 (might be hotkey for getColor(0x0301)) 
            case 4:  // Or index 4 (might be hotkey for getColor(0x0604))
            case 6:  // Or index 6 (might be hotkey for getColor(0x0604))
                return TColorAttr(trueBlack, trueWhite);  // BLACK ON TRUE WHITE
            default:
                return TMenuBar::mapColor(index);  // Use parent's mapping for others
        }
    }
    
    virtual void draw() override
    {
        // Use standard TMenuBar drawing with our custom palette
        TMenuBar::draw();
        
        // Add kaomoji at right side with proper background fill
        TDrawBuffer b;
        const char* kaomoji = "つ◕‿◕‿◕༽つ";
        int kaomojiWidth = 12;
        int xPos = size.x - kaomojiWidth;
        
        if (xPos > 1) { // Only draw if there's space
            TAttrPair cNormal = getColor(0x0301);
            // Fill entire kaomoji area with background first
            b.moveChar(0, ' ', cNormal, kaomojiWidth);
            // Then write kaomoji text
            b.moveStr(0, kaomoji, cNormal);
            writeBuf(xPos, 0, kaomojiWidth, 1, b);
        }
    }
};

/*---------------------------------------------------------*/
/* TCustomStatusLine - Status line with white hotkeys     */
/*---------------------------------------------------------*/
class TCustomStatusLine : public TStatusLine
{
public:
    TCustomStatusLine(const TRect& bounds, TStatusDef& aDefs) : TStatusLine(bounds, aDefs) {}
    
    virtual TColorAttr mapColor(uchar index) noexcept override
    {
        TColorRGB trueBlack(0, 0, 0);
        TColorRGB trueWhite(255, 255, 255);
        
        // Status line uses different indices than menu bar
        // Try common status line color indices for hotkeys
        switch(index) {
            case 1:  // Try index 1
            case 2:  // Try index 2  
            case 3:  // Try index 3
            case 4:  // Try index 4
                return TColorAttr(trueBlack, trueWhite);  // BLACK ON TRUE WHITE
            default:
                return TStatusLine::mapColor(index);  // Use parent's mapping for others
        }
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
/* TFrameAnimationWindow - Window containing animation    */
/*---------------------------------------------------------*/
class TFrameAnimationWindow : public TWindow
{
public:
    TFrameAnimationWindow(const TRect& bounds, const char* aTitle, const std::string& filePath) :
        TWindow(bounds, aTitle, wnNoNumber),
        TWindowInit(&TFrameAnimationWindow::initFrame)
    {
        options |= ofTileable;  // Enable cascade/tile functionality
        
        // Get the interior bounds (excluding frame)
        TRect interior = getExtent();
        interior.grow(-1, -1);
        
        // Check if file has frame delimiters to decide which view to use
        if (hasFrameDelimiters(filePath)) {
            // Animation file - use frame player
            FrameFilePlayerView* animView = new FrameFilePlayerView(interior, filePath);
            insert(animView);
        } else {
            // Regular text file - use scrollable text viewer
            TTextFileView* textView = new TTextFileView(interior, filePath);
            insert(textView);
        }
    }
    
    // Override changeBounds to fix tile redraw issue
    virtual void changeBounds(const TRect& bounds) override
    {
        TWindow::changeBounds(bounds);
        
        // Force complete redraw after window is resized/moved (e.g., by tile operations)
        setState(sfExposed, True);
        redraw();
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
    virtual void run();
    virtual TPalette& getPalette() const;
    static TMenuBar* initMenuBar(TRect);
    static TStatusLine* initStatusLine(TRect);
    static TDeskTop* initDeskTop(TRect);
    
private:
    void newTestWindow();
    void newTestWindow(const TRect& bounds);
    void newGradientWindow(TGradientWindow::GradientType type);
    void newGradientWindow(TGradientWindow::GradientType type, const TRect& bounds);
    void newMechWindow();
    void newDonutWindow();
    void openAnimationFile();
    void openAnimationFilePath(const std::string& path);
    void openAnimationFilePath(const std::string& path, const TRect& bounds);
    void openWorkspace();
    bool openWorkspacePath(const std::string& path);
    void cascade();
    void tile();
    void closeAll();
    void takeScreenshot();
    void setPatternMode(bool continuous);
    void saveWorkspace();
    TRect calculateWindowBounds(const std::string& filePath);
    std::string buildWorkspaceJson();
    static std::string jsonEscape(const std::string& s);
    bool loadWorkspaceFromFile(const std::string& path);
    static bool parseBool(const std::string &s, size_t &pos, bool &out);
    static bool parseString(const std::string &s, size_t &pos, std::string &out);
    static bool parseNumber(const std::string &s, size_t &pos, int &out);
    static void skipWs(const std::string &s, size_t &pos);
    static bool consume(const std::string &s, size_t &pos, char ch);
    static bool parseKeyedString(const std::string &s, size_t objStart, const char *key, std::string &out);
    static bool parseKeyedBool(const std::string &s, size_t objStart, const char *key, bool &out);
    static bool parseBounds(const std::string &s, size_t objStart, int &x,int &y,int &w,int &h);
    
    int windowNumber;
    static const int maxWindows = 99;
    
    // API/IPC registry for per-window control
    int apiIdCounter = 1;
    std::map<TWindow*, std::string> winToId;
    std::map<std::string, TWindow*> idToWin;
    
    std::string registerWindow(TWindow* w) {
        if (!w) return std::string();
        auto it = winToId.find(w);
        if (it != winToId.end()) return it->second;
        char buf[32];
        std::snprintf(buf, sizeof(buf), "w%d", apiIdCounter++);
        std::string id(buf);
        winToId[w] = id;
        idToWin[id] = w;
        return id;
    }
    
    TWindow* findWindowById(const std::string& id) {
        auto it = idToWin.find(id);
        if (it != idToWin.end()) return it->second;
        // Fallback: scan desktop to refresh mapping if needed
        // Rebuild maps for current windows
        winToId.clear();
        idToWin.clear();
        TView *start = deskTop->first();
        if (start) {
            TView *v = start;
            do {
                TWindow *w = dynamic_cast<TWindow*>(v);
                if (w) {
                    registerWindow(w);
                }
                v = v->next;
            } while (v != start);
        }
        it = idToWin.find(id);
        if (it != idToWin.end()) return it->second;
        return nullptr;
    }
    
    // IPC server
    ApiIpcServer* ipcServer = nullptr;
    
    // Friend API helper functions implemented below to bridge IPC calls.
    friend void api_spawn_test(TTestPatternApp&);
    friend void api_spawn_gradient(TTestPatternApp&, const std::string&);
    friend void api_open_animation_path(TTestPatternApp&, const std::string&);
    friend void api_spawn_test(TTestPatternApp&, const TRect* bounds);
    friend void api_spawn_gradient(TTestPatternApp&, const std::string&, const TRect* bounds);
    friend void api_open_animation_path(TTestPatternApp&, const std::string&, const TRect* bounds);
    friend void api_cascade(TTestPatternApp&);
    friend void api_tile(TTestPatternApp&);
    friend void api_close_all(TTestPatternApp&);
    friend void api_set_pattern_mode(TTestPatternApp&, const std::string&);
    friend void api_save_workspace(TTestPatternApp&);
    friend void api_open_workspace_path(TTestPatternApp&, const std::string&);
    friend void api_screenshot(TTestPatternApp&);
    friend std::string api_get_state(TTestPatternApp&);
    friend std::string api_move_window(TTestPatternApp&, const std::string&, int, int);
    friend std::string api_resize_window(TTestPatternApp&, const std::string&, int, int);
    friend std::string api_focus_window(TTestPatternApp&, const std::string&);
    friend std::string api_close_window(TTestPatternApp&, const std::string&);
};

TTestPatternApp::TTestPatternApp() :
    TProgInit(&TTestPatternApp::initStatusLine,
              &TTestPatternApp::initMenuBar,
              &TTestPatternApp::initDeskTop),
    windowNumber(0)
{
    // Start IPC server for local API control (best-effort; ignore failures)
    ipcServer = new ApiIpcServer(this);
    ipcServer->start("/tmp/test_pattern_app.sock");
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
            case cmNewMechs:
                newMechWindow();
                clearEvent(event);
                break;
            case cmNewDonut:
                newDonutWindow();
                clearEvent(event);
                break;
            case cmOpenAnimation:
                openAnimationFile();
                clearEvent(event);
                break;
            case cmOpenWorkspace:
                openWorkspace();
                clearEvent(event);
                break;
            case cmSaveWorkspace:
                saveWorkspace();
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
                
            // Edit menu commands
                
            // View menu commands  
            case cmWallpaperWibwob:
                messageBox("WIBWOBWORLD wallpaper is already active!", mfInformation | mfOKButton);
                clearEvent(event);
                break;
            case cmWallpaperCustom:
                messageBox("Custom wallpaper loading coming soon!", mfInformation | mfOKButton);
                clearEvent(event);
                break;
            case cmWallpaperDisable:
                messageBox("Wallpaper disable coming soon!", mfInformation | mfOKButton);
                clearEvent(event);
                break;
            case cmZoomIn:
                messageBox("Zoom In coming soon!", mfInformation | mfOKButton);
                clearEvent(event);
                break;
            case cmZoomOut:
                messageBox("Zoom Out coming soon!", mfInformation | mfOKButton);
                clearEvent(event);
                break;
            case cmActualSize:
                messageBox("Actual Size coming soon!", mfInformation | mfOKButton);
                clearEvent(event);
                break;
            case cmFullScreen:
                messageBox("Full Screen mode coming soon!", mfInformation | mfOKButton);
                clearEvent(event);
                break;
                
            // Tools menu commands
            case cmAnsiEditor:
                messageBox("ANSI Editor coming soon!", mfInformation | mfOKButton);
                clearEvent(event);
                break;
            case cmPaintTools:
                messageBox("Paint Tools coming soon!", mfInformation | mfOKButton);
                clearEvent(event);
                break;
            case cmAnimationStudio:
                messageBox("Animation Studio coming soon!", mfInformation | mfOKButton);
                clearEvent(event);
                break;
            case cmQuantumPrinter:
                messageBox("🚀 QUANTUM PRINTER ACTIVATED! 🚀\n\nPrinting reality at 42Hz...", mfInformation | mfOKButton);
                clearEvent(event);
                break;
                
            // Help menu commands
            case cmAbout:
                messageBox("WIBWOBWORLD Test Pattern Generator\n\nBuilt with Turbo Vision\nつ◕‿◕‿◕༽つ", mfInformation | mfOKButton);
                clearEvent(event);
                break;
                
            // Future File commands
            case cmOpenAnsiArt:
                messageBox("ANSI Art file opening coming soon!", mfInformation | mfOKButton);
                clearEvent(event);
                break;
            case cmNewPaintCanvas:
                messageBox("Paint Canvas creation coming soon!", mfInformation | mfOKButton);
                clearEvent(event);
                break;
            case cmOpenImageFile:
                messageBox("Image file opening coming soon!", mfInformation | mfOKButton);
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

void TTestPatternApp::newTestWindow(const TRect& bounds)
{
    // Create window title
    windowNumber++;
    std::stringstream title;
    title << "Test Pattern " << windowNumber;
    
    // Create and insert window with provided bounds
    TTestPatternWindow* window = new TTestPatternWindow(bounds, title.str().c_str());
    deskTop->insert(window);
    registerWindow(window);
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
    // Close all regular windows on the desktop (iterating safely over circular list)
    std::vector<TWindow*> toClose;
    TView *start = deskTop->first();
    if (start) {
        TView *v = start;
        do {
            TView *nextV = v->next; // cache next to avoid invalidation issues
            if (TWindow *w = dynamic_cast<TWindow*>(v)) {
                // Skip non-user windows if any (none expected here)
                toClose.push_back(w);
            }
            v = nextV;
        } while (v != start);
    }
    for (auto *w : toClose) {
        if (w && (w->flags & wfClose))
            w->close();
    }
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
    registerWindow(window);
}

void TTestPatternApp::newGradientWindow(TGradientWindow::GradientType type, const TRect& bounds)
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
    
    // Create and insert window with provided bounds
    TGradientWindow* window = new TGradientWindow(bounds, title.str().c_str(), type);
    deskTop->insert(window);
    registerWindow(window);
}

void TTestPatternApp::newMechWindow()
{
    // Create window title
    windowNumber++;
    std::stringstream title;
    title << "Mechs Grid " << windowNumber;
    
    // Calculate window position (cascade effect)
    int offset = (windowNumber - 1) % 10;
    TRect bounds(
        2 + offset * 2,           // left
        1 + offset,               // top
        70 + offset * 2,          // right (wider for mech grid)
        30 + offset               // bottom (taller for mech grid)
    );
    
    // Create and insert window
    TMechWindow* window = new TMechWindow(bounds, title.str().c_str(), windowNumber);
    deskTop->insert(window);
    registerWindow(window);
}

void TTestPatternApp::newDonutWindow()
{
    // Create window title
    windowNumber++;
    std::stringstream title;
    title << "Donut Animation " << windowNumber;
    
    // Calculate window position (cascade effect)
    int offset = (windowNumber - 1) % 10;
    TRect bounds(
        2 + offset * 2,           // left
        1 + offset,               // top
        50 + offset * 2,          // right
        15 + offset               // bottom
    );
    
    // Create and insert window with donut.txt file
    TFrameAnimationWindow* window = new TFrameAnimationWindow(bounds, title.str().c_str(), "donut.txt");
    deskTop->insert(window);
    registerWindow(window);
}

void TTestPatternApp::openAnimationFile()
{
    char fileName[MAXPATH];
    strcpy(fileName, "primers/*.txt");
    
    TFileDialog* dialog = new TFileDialog("primers/*.txt", "Open Text/Animation File", "~N~ame", fdOpenButton, 100);
    if (executeDialog(dialog, fileName) != cmCancel)
    {
        // Determine file type and create appropriate title
        windowNumber++;
        std::stringstream title;
        
        if (hasFrameDelimiters(fileName)) {
            title << "Animation " << windowNumber;
        } else {
            // Extract filename without path for text files
            std::string fileStr(fileName);
            size_t lastSlash = fileStr.find_last_of("/\\");
            std::string baseName = (lastSlash != std::string::npos) ? fileStr.substr(lastSlash + 1) : fileStr;
            title << baseName << " - Text " << windowNumber;
        }
        
        // Calculate window position (cascade effect)
        int offset = (windowNumber - 1) % 10;
        TRect bounds(
            2 + offset * 2,           // left
            1 + offset,               // top
            50 + offset * 2,          // right
            15 + offset               // bottom
        );
        
        // Create and insert window with selected file
        TFrameAnimationWindow* window = new TFrameAnimationWindow(bounds, title.str().c_str(), fileName);
        deskTop->insert(window);
        registerWindow(window);
    }
}

void TTestPatternApp::openAnimationFilePath(const std::string& filePath)
{
    // Determine file type and create appropriate title
    windowNumber++;
    std::stringstream title;
    
    if (hasFrameDelimiters(filePath)) {
        title << "Animation " << windowNumber;
    } else {
        // Extract filename without path for text files
        size_t lastSlash = filePath.find_last_of("/\\");
        std::string baseName = (lastSlash != std::string::npos) ? filePath.substr(lastSlash + 1) : filePath;
        title << baseName << " - Text " << windowNumber;
    }
    
    // Calculate window position (cascade effect)
    int offset = (windowNumber - 1) % 10;
    TRect bounds(
        2 + offset * 2,           // left
        1 + offset,               // top
        50 + offset * 2,          // right
        15 + offset               // bottom
    );
    
    // Create and insert window with selected file
    TFrameAnimationWindow* window = new TFrameAnimationWindow(bounds, title.str().c_str(), filePath);
    deskTop->insert(window);
    registerWindow(window);
}

void TTestPatternApp::openAnimationFilePath(const std::string& filePath, const TRect& bounds)
{
    // Determine file type and create appropriate title
    windowNumber++;
    std::stringstream title;
    
    if (hasFrameDelimiters(filePath)) {
        title << "Animation " << windowNumber;
    } else {
        // Extract filename without path for text files
        size_t lastSlash = filePath.find_last_of("/\\");
        std::string baseName = (lastSlash != std::string::npos) ? filePath.substr(lastSlash + 1) : filePath;
        title << baseName << " - Text " << windowNumber;
    }
    
    // Create and insert window with provided bounds
    TFrameAnimationWindow* window = new TFrameAnimationWindow(bounds, title.str().c_str(), filePath);
    deskTop->insert(window);
    registerWindow(window);
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
    "\x70\x70\x0F\x07\x70\x70\x70\x07\x0F\x07\x07\x07\x70\x07\x0F" \
    "\x70\x0F\x70\x07\x07\x70\x07\x0F\x70\x7F\x7F\x70\x07\x70\x07\x0F" \
    "\x70\x7F\x7F\x70\x07\x70\x70\x7F\x7F\x07\x70\x0F\x70\x0F\x70\x07" \
    "\x0F\x0F\x0F\x70\x0F\x07\x70\x70\x70\x07\x70\x0F\x07\x07\x78\x00" \
    "\x70\xF0\x0F\x70\x07\x70\x70\x0F\x0F\x07\xF0\x7F\x08\x7F\xF0\x70" \
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
            *new TMenuItem("New ~T~est Pattern", cmNewWindow, kbCtrlN) +
            *new TMenuItem("New ~H~-Gradient", cmNewGradientH, kbNoKey) +
            *new TMenuItem("New ~V~-Gradient", cmNewGradientV, kbNoKey) +
            *new TMenuItem("New ~R~adial Gradient", cmNewGradientR, kbNoKey) +
            *new TMenuItem("New ~D~iagonal Gradient", cmNewGradientD, kbNoKey) +
            *new TMenuItem("New ~M~echs Grid", cmNewMechs, kbCtrlM) +
            *new TMenuItem("New ~A~nimation", cmNewDonut, kbCtrlD) +
            *new TMenuItem("New A~N~SI Art", cmOpenAnsiArt, kbNoKey) +
            *new TMenuItem("New ~P~aint Canvas", cmNewPaintCanvas, kbNoKey) +
            newLine() +
            *new TMenuItem("~O~pen Text/Animation...", cmOpenAnimation, kbCtrlO) +
            *new TMenuItem("Open ANS~I~ Art...", cmOpenAnsiArt, kbNoKey) +
            *new TMenuItem("Open I~m~age...", cmOpenImageFile, kbNoKey) +
            newLine() +
            *new TMenuItem("~S~ave Workspace", cmSaveWorkspace, kbCtrlS) +
            *new TMenuItem("Open ~W~orkspace...", cmOpenWorkspace, kbNoKey) +
            newLine() +
            *new TMenuItem("E~x~it", cmQuit, cmQuit, hcNoContext, "Alt-X") +
        *new TSubMenu("~E~dit", kbAltE) +
            *new TMenuItem("Sc~r~eenshot", cmScreenshot, kbCtrlP) +
            newLine() +
            (TMenuItem&) (
                *new TSubMenu("Pattern ~M~ode", kbNoKey) +
                    *new TMenuItem(USE_CONTINUOUS_PATTERN ? "\x04 ~C~ontinuous (Diagonal)" : "  ~C~ontinuous (Diagonal)", 
                                  cmPatternContinuous, kbNoKey) +
                    *new TMenuItem(!USE_CONTINUOUS_PATTERN ? "\x04 ~T~iled (Cropped)" : "  ~T~iled (Cropped)", 
                                  cmPatternTiled, kbNoKey)
            ) +
        *new TSubMenu("~V~iew", kbAltV) +
            (TMenuItem&) (
                *new TSubMenu("~W~allpaper", kbNoKey) +
                    *new TMenuItem("WIBWO~B~WORLD", cmWallpaperWibwob, kbNoKey) +
                    *new TMenuItem("Load ~C~ustom...", cmWallpaperCustom, kbNoKey) +
                    *new TMenuItem("~D~isable", cmWallpaperDisable, kbNoKey)
            ) +
            newLine() +
            *new TMenuItem("Zoom ~I~n", cmZoomIn, kbNoKey) +
            *new TMenuItem("Zoom ~O~ut", cmZoomOut, kbNoKey) +
            *new TMenuItem("~A~ctual Size", cmActualSize, kbNoKey) +
            *new TMenuItem("~F~ull Screen", cmFullScreen, kbF11) +
        *new TSubMenu("~W~indow", kbAltW) +
            *new TMenuItem("~C~ascade", cmCascade, kbNoKey) +
            *new TMenuItem("~T~ile", cmTile, kbNoKey) +
            newLine() +
            *new TMenuItem("~N~ext", cmNext, kbF6) +
            *new TMenuItem("~P~revious", cmPrev, kbShiftF6) +
            newLine() +
            *new TMenuItem("Close", cmClose, kbAltF3) +
            *new TMenuItem("C~l~ose All", cmCloseAll, kbNoKey) +
        *new TSubMenu("~T~ools", kbAltT) +
            *new TMenuItem("~A~NSI Editor", cmAnsiEditor, kbNoKey) +
            *new TMenuItem("~P~aint Tools", cmPaintTools, kbNoKey) +
            *new TMenuItem("Animation ~S~tudio", cmAnimationStudio, kbNoKey) +
            newLine() +
            *new TMenuItem("~Q~uantum Printer", cmQuantumPrinter, kbF11) +
        *new TSubMenu("~H~elp", kbAltH) +
            *new TMenuItem("~A~bout WIBWOBWORLD", cmAbout, kbNoKey)
    );
}

TStatusLine* TTestPatternApp::initStatusLine(TRect r)
{
    r.a.y = r.b.y - 1;
    return new TCustomStatusLine(r,
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
    // Create desktop with standard constructor (plain background)
    TDeskTop* desktop = new TDeskTop(r);
    
    return desktop;
}


void TTestPatternApp::run()
{
    // Call parent run to initialize everything first
    TApplication::run();
}

TRect TTestPatternApp::calculateWindowBounds(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open()) {
        // Fallback size if file can't be read
        return TRect(2, 1, 50, 15);
    }
    
    int maxWidth = 0;
    int height = 0;
    std::string line;
    
    while (std::getline(file, line)) {
        // Skip frame delimiter lines if present
        if (line == "----") continue;
        
        int lineWidth = line.length();
        if (lineWidth > maxWidth) {
            maxWidth = lineWidth;
        }
        height++;
    }
    file.close();
    
    // Add padding for window frame (2 chars horizontal, 2 lines vertical)
    int windowWidth = maxWidth + 2;
    int windowHeight = height + 2;
    
    // Get screen dimensions
    TRect screenBounds = deskTop->getExtent();
    int screenWidth = screenBounds.b.x;
    int screenHeight = screenBounds.b.y;
    
    // Limit to screen size minus margins
    if (windowWidth > screenWidth - 4) windowWidth = screenWidth - 4;
    if (windowHeight > screenHeight - 2) windowHeight = screenHeight - 2;
    
    // Center the window
    int x = (screenWidth - windowWidth) / 2;
    int y = (screenHeight - windowHeight) / 2;
    
    return TRect(x, y, x + windowWidth, y + windowHeight);
}

void TTestPatternApp::idle()
{
    TApplication::idle();
    // Poll IPC server for incoming API commands
    if (ipcServer) ipcServer->poll();
    
    // Create spore monster window on first idle call (after full init)
    static bool firstRun = true;
    if (firstRun) {
        firstRun = false;
        
        // Auto-size window to file content
        std::string filePath = "primers/spore-monster.txt";
        TRect bounds = calculateWindowBounds(filePath);
        
        std::string title = "Spore Monster";
        TFrameAnimationWindow* window = new TFrameAnimationWindow(bounds, title.c_str(), filePath.c_str());
        deskTop->insert(window);
        registerWindow(window);
        // Force immediate redraw
        redraw();
    }
}

int main()
{
    TTestPatternApp app;
    app.run();
    return 0;
}

// ---- IPC API helper functions (friend) ----
// Backward compatibility overloads
void api_spawn_test(TTestPatternApp& app) { app.newTestWindow(); }
void api_spawn_gradient(TTestPatternApp& app, const std::string& kind) {
    if (kind == "horizontal") app.newGradientWindow(TGradientWindow::gtHorizontal);
    else if (kind == "vertical") app.newGradientWindow(TGradientWindow::gtVertical);
    else if (kind == "radial") app.newGradientWindow(TGradientWindow::gtRadial);
    else if (kind == "diagonal") app.newGradientWindow(TGradientWindow::gtDiagonal);
    else app.newGradientWindow(TGradientWindow::gtHorizontal);
}
void api_open_animation_path(TTestPatternApp& app, const std::string& path) {
    app.openAnimationFilePath(path);
}

// New overloads with bounds support
void api_spawn_test(TTestPatternApp& app, const TRect* bounds) { 
    if (bounds) {
        app.newTestWindow(*bounds);
    } else {
        app.newTestWindow();
    }
}

void api_spawn_gradient(TTestPatternApp& app, const std::string& kind, const TRect* bounds) {
    TGradientWindow::GradientType type = TGradientWindow::gtHorizontal;
    if (kind == "horizontal") type = TGradientWindow::gtHorizontal;
    else if (kind == "vertical") type = TGradientWindow::gtVertical;
    else if (kind == "radial") type = TGradientWindow::gtRadial;
    else if (kind == "diagonal") type = TGradientWindow::gtDiagonal;
    
    if (bounds) {
        app.newGradientWindow(type, *bounds);
    } else {
        app.newGradientWindow(type);
    }
}

void api_open_animation_path(TTestPatternApp& app, const std::string& path, const TRect* bounds) {
    if (bounds) {
        app.openAnimationFilePath(path, *bounds);
    } else {
        app.openAnimationFilePath(path);
    }
}

void api_cascade(TTestPatternApp& app) { app.cascade(); }
void api_tile(TTestPatternApp& app) { app.tile(); }
void api_close_all(TTestPatternApp& app) { app.closeAll(); }

void api_set_pattern_mode(TTestPatternApp& app, const std::string& mode) {
    bool continuous = (mode == "continuous");
    app.setPatternMode(continuous);
}

void api_save_workspace(TTestPatternApp& app) { app.saveWorkspace(); }

void api_open_workspace_path(TTestPatternApp& app, const std::string& path) {
    app.openWorkspacePath(path);
}

void api_screenshot(TTestPatternApp& app) { app.takeScreenshot(); }

std::string api_get_state(TTestPatternApp& app) {
    // Rebuild window registry to sync with current desktop state
    app.winToId.clear();
    app.idToWin.clear();
    
    std::stringstream json;
    json << "{\"windows\":[";
    
    bool first = true;
    TView *start = app.deskTop->first();
    if (start) {
        TView *v = start;
        do {
            TWindow *w = dynamic_cast<TWindow*>(v);
            if (w) {
                std::string id = app.registerWindow(w);
                
                if (!first) json << ",";
                json << "{\"id\":\"" << id << "\""
                     << ",\"x\":" << w->origin.x
                     << ",\"y\":" << w->origin.y  
                     << ",\"width\":" << w->size.x
                     << ",\"height\":" << w->size.y
                     << ",\"title\":\"";
                
                // Safely escape title
                if (w->title) {
                    std::string title(w->title);
                    for (char c : title) {
                        if (c == '"') json << "\\\"";
                        else if (c == '\\') json << "\\\\";
                        else json << c;
                    }
                }
                json << "\"}";
                first = false;
            }
            v = v->next;
        } while (v != start);
    }
    
    json << "]}";
    return json.str();
}

std::string api_move_window(TTestPatternApp& app, const std::string& id, int x, int y) {
    TWindow* w = app.findWindowById(id);
    if (!w) return "{\"error\":\"Window not found\"}";
    
    TRect newBounds = w->getBounds();
    newBounds.move(x - newBounds.a.x, y - newBounds.a.y);
    w->locate(newBounds);
    
    return "{\"success\":true}";
}

std::string api_resize_window(TTestPatternApp& app, const std::string& id, int width, int height) {
    TWindow* w = app.findWindowById(id);
    if (!w) return "{\"error\":\"Window not found\"}";
    
    TRect newBounds = w->getBounds();
    newBounds.b.x = newBounds.a.x + width;
    newBounds.b.y = newBounds.a.y + height;
    w->locate(newBounds);
    
    return "{\"success\":true}";
}

std::string api_focus_window(TTestPatternApp& app, const std::string& id) {
    TWindow* w = app.findWindowById(id);
    if (!w) return "{\"error\":\"Window not found\"}";
    
    app.deskTop->setCurrent(w, TDeskTop::normalSelect);
    return "{\"success\":true}";
}

std::string api_close_window(TTestPatternApp& app, const std::string& id) {
    TWindow* w = app.findWindowById(id);
    if (!w) return "{\"error\":\"Window not found\"}";
    
    // Remove from registry
    app.winToId.erase(w);
    app.idToWin.erase(id);
    
    // Close the window
    w->close();
    return "{\"success\":true}";
}

// --- Minimal JSON parsing helpers (subset tailored to our schema) ---
void TTestPatternApp::skipWs(const std::string &s, size_t &pos)
{
    while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\n' || s[pos] == '\r' || s[pos] == '\t')) ++pos;
}

bool TTestPatternApp::consume(const std::string &s, size_t &pos, char ch)
{
    skipWs(s, pos);
    if (pos < s.size() && s[pos] == ch) { ++pos; return true; }
    return false;
}

bool TTestPatternApp::parseString(const std::string &s, size_t &pos, std::string &out)
{
    skipWs(s, pos);
    if (pos >= s.size() || s[pos] != '"') return false;
    ++pos;
    std::string res;
    while (pos < s.size()) {
        char c = s[pos++];
        if (c == '"') { out = res; return true; }
        if (c == '\\') {
            if (pos >= s.size()) return false;
            char e = s[pos++];
            switch (e) {
                case '"': res.push_back('"'); break;
                case '\\': res.push_back('\\'); break;
                case 'n': res.push_back('\n'); break;
                case 'r': res.push_back('\r'); break;
                case 't': res.push_back('\t'); break;
                default: res.push_back(e); break;
            }
        } else res.push_back(c);
    }
    return false;
}

bool TTestPatternApp::parseNumber(const std::string &s, size_t &pos, int &out)
{
    skipWs(s, pos);
    bool neg = false;
    if (pos < s.size() && (s[pos] == '-' || s[pos] == '+')) { neg = (s[pos] == '-'); ++pos; }
    long val = 0; bool any=false;
    while (pos < s.size() && s[pos] >= '0' && s[pos] <= '9') { any=true; val = val*10 + (s[pos]-'0'); ++pos; }
    if (!any) return false;
    out = neg ? -int(val) : int(val);
    return true;
}

bool TTestPatternApp::parseBool(const std::string &s, size_t &pos, bool &out)
{
    skipWs(s, pos);
    if (s.compare(pos, 4, "true") == 0) { out = true; pos += 4; return true; }
    if (s.compare(pos, 5, "false") == 0) { out = false; pos += 5; return true; }
    return false;
}

bool TTestPatternApp::parseKeyedString(const std::string &s, size_t objStart, const char *key, std::string &out)
{
    size_t pos = objStart;
    while (pos < s.size()) {
        skipWs(s, pos);
        if (s[pos] == '}' || s[pos] == ']') return false;
        std::string k; size_t kpos = pos;
        if (!parseString(s, kpos, k)) { ++pos; continue; }
        pos = kpos; skipWs(s, pos);
        if (!consume(s, pos, ':')) continue;
        if (k == key) {
            return parseString(s, pos, out);
        }
        // Skip value
        skipWs(s, pos);
        if (pos>=s.size()) break;
        if (s[pos] == '"') { std::string tmp; parseString(s, pos, tmp); }
        else if ((s[pos] >= '0' && s[pos] <= '9') || s[pos]=='-' || s[pos]=='+') { int dummy; parseNumber(s, pos, dummy); }
        else if (s[pos] == 't' || s[pos] == 'f') { bool db; parseBool(s, pos, db); }
        else if (s[pos] == '{') { int depth=1; ++pos; while (pos<s.size()&&depth){ if(s[pos]=='"'){ ++pos; while(pos<s.size()&&s[pos]!='"'){ if(s[pos]=='\\') ++pos; ++pos;} ++pos; continue;} if(s[pos]=='{')depth++; else if(s[pos]=='}')depth--; ++pos; } }
        else if (s[pos] == '[') { int depth=1; ++pos; while (pos<s.size()&&depth){ if(s[pos]=='"'){ ++pos; while(pos<s.size()&&s[pos]!='"'){ if(s[pos]=='\\') ++pos; ++pos;} ++pos; continue;} if(s[pos]=='[')depth++; else if(s[pos]==']')depth--; ++pos; } }
        skipWs(s, pos); if (pos<s.size() && s[pos]==',') ++pos;
    }
    return false;
}

bool TTestPatternApp::parseKeyedBool(const std::string &s, size_t objStart, const char *key, bool &out)
{
    size_t pos = objStart;
    while (pos < s.size()) {
        skipWs(s, pos);
        if (s[pos] == '}' || s[pos] == ']') return false;
        std::string k; size_t kpos = pos;
        if (!parseString(s, kpos, k)) { ++pos; continue; }
        pos = kpos; skipWs(s, pos);
        if (!consume(s, pos, ':')) continue;
        if (k == key) {
            return parseBool(s, pos, out);
        }
        // Skip value
        skipWs(s, pos);
        if (pos>=s.size()) break;
        if (s[pos] == '"') { std::string tmp; parseString(s, pos, tmp); }
        else if ((s[pos] >= '0' && s[pos] <= '9') || s[pos]=='-' || s[pos]=='+') { int dummy; parseNumber(s, pos, dummy); }
        else if (s[pos] == 't' || s[pos] == 'f') { bool db; parseBool(s, pos, db); }
        else if (s[pos] == '{') { int depth=1; ++pos; while (pos<s.size()&&depth){ if(s[pos]=='"'){ ++pos; while(pos<s.size()&&s[pos]!='"'){ if(s[pos]=='\\') ++pos; ++pos;} ++pos; continue;} if(s[pos]=='{')depth++; else if(s[pos]=='}')depth--; ++pos; } }
        else if (s[pos] == '[') { int depth=1; ++pos; while (pos<s.size()&&depth){ if(s[pos]=='"'){ ++pos; while(pos<s.size()&&s[pos]!='"'){ if(s[pos]=='\\') ++pos; ++pos;} ++pos; continue;} if(s[pos]=='[')depth++; else if(s[pos]==']')depth--; ++pos; } }
        skipWs(s, pos); if (pos<s.size() && s[pos]==',') ++pos;
    }
    return false;
}

bool TTestPatternApp::parseBounds(const std::string &s, size_t objStart, int &x,int &y,int &w,int &h)
{
    size_t pos = objStart;
    while (pos < s.size()) {
        skipWs(s, pos);
        if (s[pos] == '}' || s[pos] == ']') return false;
        std::string k; size_t kpos = pos;
        if (!parseString(s, kpos, k)) { ++pos; continue; }
        pos = kpos; skipWs(s, pos);
        if (!consume(s, pos, ':')) continue;
        if (k == "bounds") {
            skipWs(s, pos);
            if (!consume(s, pos, '{')) return false;
            int tx=0,ty=0,tw=0,th=0; bool okX=false,okY=false,okW=false,okH=false;
            while (pos < s.size()) {
                skipWs(s, pos);
                if (s[pos] == '}') { ++pos; break; }
                std::string bk; if (!parseString(s, pos, bk)) return false; if (!consume(s,pos,':')) return false;
                if (bk == "x") { okX = parseNumber(s,pos,tx); }
                else if (bk == "y") { okY = parseNumber(s,pos,ty); }
                else if (bk == "w") { okW = parseNumber(s,pos,tw); }
                else if (bk == "h") { okH = parseNumber(s,pos,th); }
                skipWs(s,pos); if (pos<s.size() && s[pos]==',') ++pos;
            }
            if (okX && okY && okW && okH) { x=tx; y=ty; w=tw; h=th; return true; }
            return false;
        }
        // Skip value
        skipWs(s, pos);
        if (pos>=s.size()) break;
        if (s[pos] == '"') { std::string tmp; parseString(s, pos, tmp); }
        else if ((s[pos] >= '0' && s[pos] <= '9') || s[pos]=='-' || s[pos]=='+') { int dummy; parseNumber(s, pos, dummy); }
        else if (s[pos] == 't' || s[pos] == 'f') { bool db; parseBool(s, pos, db); }
        else if (s[pos] == '{') { int depth=1; ++pos; while (pos<s.size()&&depth){ if(s[pos]=='"'){ ++pos; while(pos<s.size()&&s[pos]!='"'){ if(s[pos]=='\\') ++pos; ++pos;} ++pos; continue;} if(s[pos]=='{')depth++; else if(s[pos]=='}')depth--; ++pos; } }
        else if (s[pos] == '[') { int depth=1; ++pos; while (pos<s.size()&&depth){ if(s[pos]=='"'){ ++pos; while(pos<s.size()&&s[pos]!='"'){ if(s[pos]=='\\') ++pos; ++pos;} ++pos; continue;} if(s[pos]=='[')depth++; else if(s[pos]==']')depth--; ++pos; } }
        skipWs(s, pos); if (pos<s.size() && s[pos]==',') ++pos;
    }
    return false;
}

bool TTestPatternApp::loadWorkspaceFromFile(const std::string& path)
{
    std::ifstream in(path);
    if (!in) {
        std::string msg = std::string("Failed to open ") + path;
        messageBox(msg.c_str(), mfError | mfOKButton);
        return false;
    }
    std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    if (data.find("\"version\"") == std::string::npos || data.find("\"windows\"") == std::string::npos) {
        messageBox("Invalid workspace file.", mfError | mfOKButton);
        return false;
    }

    // Extract globals.patternMode
    bool continuous = USE_CONTINUOUS_PATTERN;
    size_t globalsPos = data.find("\"globals\"");
    if (globalsPos != std::string::npos) {
        size_t pos = data.find('{', globalsPos);
        if (pos != std::string::npos) {
            std::string pm;
            if (parseKeyedString(data, pos+1, "patternMode", pm))
                continuous = (pm == "continuous");
        }
    }

    // Locate windows array and extract each object substring
    size_t winKey = data.find("\"windows\"");
    if (winKey == std::string::npos) {
        messageBox("No windows in workspace.", mfError | mfOKButton);
        return false;
    }
    size_t arrPos = data.find('[', winKey);
    if (arrPos == std::string::npos) return false;
    std::vector<std::string> objects;
    size_t p = arrPos+1; bool inStr=false; int depth=0;
    while (p < data.size()) {
        char c = data[p];
        if (c == '"') { inStr = !inStr; ++p; continue; }
        if (!inStr) {
            if (c == '{') {
                int d=1; size_t q=p+1;
                while (q<data.size() && d) {
                    if (data[q] == '"') { ++q; while (q<data.size() && data[q] != '"') { if (data[q]=='\\') ++q; ++q; } ++q; continue; }
                    if (data[q] == '{') d++; else if (data[q] == '}') d--; ++q;
                }
                objects.emplace_back(data.substr(p, q-p));
                p = q; continue;
            }
            if (c == ']') break;
        }
        ++p;
    }

    // Close current windows
    closeAll();

    // Apply globals
    USE_CONTINUOUS_PATTERN = continuous;

    // Restore windows
    std::vector<TWindow*> created;
    for (const auto &obj : objects) {
        std::string type; if (!parseKeyedString(obj, 0, "type", type)) continue;
        std::string title; parseKeyedString(obj, 0, "title", title);
        int x=2,y=1,w=50,h=15; parseBounds(obj, 0, x,y,w,h);
        bool zoomed=false; parseKeyedBool(obj, 0, "zoomed", zoomed);

        // Clamp
        TRect ext = deskTop->getExtent();
        int maxW = ext.b.x - ext.a.x;
        int maxH = ext.b.y - ext.a.y;
        if (w < 16) w = 16; if (h < 6) h = 6;
        if (w > maxW) w = maxW; if (h > maxH) h = maxH;
        if (x < 0) x = 0; if (y < 0) y = 0;
        if (x + w > maxW) x = std::max(0, maxW - w);
        if (y + h > maxH) y = std::max(0, maxH - h);
        TRect bounds(x,y,x+w,y+h);

        TWindow *win = nullptr;
        if (type == "test_pattern") {
            win = new TTestPatternWindow(bounds, title.c_str());
        } else if (type == "gradient") {
            std::string gtype; // props.gradientType preferred
            size_t propsPos = obj.find("\"props\"");
            if (propsPos != std::string::npos) {
                size_t brace = obj.find('{', propsPos);
                if (brace != std::string::npos)
                    parseKeyedString(obj, brace+1, "gradientType", gtype);
            }
            TGradientWindow::GradientType gt = TGradientWindow::gtHorizontal;
            if (gtype == "vertical") gt = TGradientWindow::gtVertical;
            else if (gtype == "radial") gt = TGradientWindow::gtRadial;
            else if (gtype == "diagonal") gt = TGradientWindow::gtDiagonal;
            win = new TGradientWindow(bounds, title.c_str(), gt);
        } else {
            continue;
        }
        deskTop->insert(win);
        if (zoomed) win->zoom();
        created.push_back(win);
    }

    // Focus saved
    int focusedIdx = -1;
    size_t fpos = data.find("\"focusedIndex\"");
    if (fpos != std::string::npos) { size_t pos = data.find(':', fpos); if (pos != std::string::npos) { ++pos; parseNumber(data, pos, focusedIdx); } }
    if (focusedIdx >= 0 && focusedIdx < (int)created.size()) created[focusedIdx]->select();

    return true;
}

void TTestPatternApp::openWorkspace()
{
    // Open a dialog rooted at workspaces/ listing JSON files
    char fileName[260];
    std::strncpy(fileName, "workspaces/*.json", sizeof(fileName));
    fileName[sizeof(fileName)-1] = '\0';
    TFileDialog *dlg = new TFileDialog("workspaces/*.json", "Open Workspace", "~N~ame", fdOpenButton, 101);
    ushort res = deskTop->execView(dlg);
    std::string path;
    if (res != cmCancel) {
        dlg->getData(fileName);
        path = fileName;
        // Normalize: if only a filename, prepend workspaces/
        if (!path.empty() && path.find('/') == std::string::npos)
            path = std::string("workspaces/") + path;
    } else {
        path = "workspaces/last_workspace.json";
    }
    destroy(dlg);

    // Fallback if file missing: try default
    std::ifstream test(path.c_str());
    if (!test.good()) {
        test.close();
        path = "workspaces/last_workspace.json";
    } else test.close();

    if (!loadWorkspaceFromFile(path))
        return;
    messageBox("Workspace loaded.", mfInformation | mfOKButton);
}

bool TTestPatternApp::openWorkspacePath(const std::string& path)
{
    return loadWorkspaceFromFile(path);
}

// Minimal JSON helpers
std::string TTestPatternApp::jsonEscape(const std::string& s)
{
    std::string out;
    out.reserve(s.size() + 8);
    for (unsigned char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    char buf[7];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else out += char(c);
        }
    }
    return out;
}

std::string TTestPatternApp::buildWorkspaceJson()
{
    // Screen size
    TRect ext = deskTop->getExtent();
    int sw = ext.b.x - ext.a.x;
    int sh = ext.b.y - ext.a.y;

    // Timestamp (basic)
    char ts[64];
    std::time_t t = std::time(nullptr);
    std::tm *lt = std::localtime(&t);
    std::strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%S", lt);

    std::string json;
    json += "{\n";
    json += "  \"version\": 1,\n";
    json += "  \"app\": \"test_pattern\",\n";
    json += std::string("  \"timestamp\": \"") + ts + "\",\n";
    json += "  \"screen\": { \"width\": " + std::to_string(sw) + ", \"height\": " + std::to_string(sh) + " },\n";
    json += std::string("  \"globals\": { \"patternMode\": \"") + (USE_CONTINUOUS_PATTERN ? "continuous" : "tiled") + "\" },\n";
    json += "  \"windows\": [\n";

    // Collect windows in current z-order (child list is circular)
    int idx = 0;
    int focusedIndex = -1;
    TView *vStart = deskTop->first();
    if (vStart) {
    TView *v = vStart;
    do {
        TView *nextV = v->next; // Always advance even on skips
        TWindow *w = dynamic_cast<TWindow*>(v);
        if (!w) { v = nextV; continue; } // Skip non-window views (e.g., wallpaper)
        if (!w->getState(sfVisible)) { v = nextV; continue; }

        // Determine type and props
        std::string type = "custom";
        std::string props = "{}";

        if (dynamic_cast<TTestPatternWindow*>(w)) {
            type = "test_pattern";
            props = "{}"; // Pattern mode is global in MVP
        } else {
            // Try to detect gradient by scanning child views (circular list)
            bool isGradient = false;
            TView *cStart = w->first();
            if (cStart) {
            TView *c = cStart;
            do {
                if (dynamic_cast<THorizontalGradientView*>(c)) {
                    type = "gradient"; props = "{\"gradientType\": \"horizontal\"}"; isGradient = true; break;
                } else if (dynamic_cast<TVerticalGradientView*>(c)) {
                    type = "gradient"; props = "{\"gradientType\": \"vertical\"}"; isGradient = true; break;
                } else if (dynamic_cast<TRadialGradientView*>(c)) {
                    type = "gradient"; props = "{\"gradientType\": \"radial\"}"; isGradient = true; break;
                } else if (dynamic_cast<TDiagonalGradientView*>(c)) {
                    type = "gradient"; props = "{\"gradientType\": \"diagonal\"}"; isGradient = true; break;
                }
                c = c->next;
            } while (c != cStart);
            }
            if (!isGradient) {
                // Unknown window type: keep as 'custom' with empty props
            }
        }

        // Bounds (outer window rect)
        TRect b = w->getBounds();
        int x = b.a.x, y = b.a.y, ww = b.b.x - b.a.x, hh = b.b.y - b.a.y;

        // Zoomed: compare to max size from sizeLimits
        TPoint minSz, maxSz;
        w->sizeLimits(minSz, maxSz);
        bool zoomed = (w->size.x == maxSz.x && w->size.y == maxSz.y && w->origin.x == 0 && w->origin.y == 0);

        // Track focused window index (selected)
        if (w->getState(sfSelected))
            focusedIndex = idx; // zero-based

        if (idx++ > 0) json += ",\n";
        json += "    {\n";
        json += "      \"id\": \"w" + std::to_string(idx) + "\",\n";
        json += "      \"type\": \"" + type + "\",\n";
        const char *title = w->getTitle(0);
        std::string safeTitle = title ? jsonEscape(title) : std::string("");
        json += "      \"title\": \"" + safeTitle + "\",\n";
        json += "      \"bounds\": { \"x\": " + std::to_string(x) + ", \"y\": " + std::to_string(y) + ", \"w\": " + std::to_string(ww) + ", \"h\": " + std::to_string(hh) + " },\n";
        json += std::string("      \"zoomed\": ") + (zoomed ? "true" : "false") + ",\n";
        json += "      \"props\": " + props + "\n";
        json += "    }";
        v = nextV;
    } while (v != vStart);
    }

    json += "\n  ]";
    if (focusedIndex >= 0)
        json += ",\n  \"focusedIndex\": " + std::to_string(focusedIndex);
    json += "\n}";
    return json;
}

void TTestPatternApp::saveWorkspace()
{
    // Ensure directory exists
    mkdir("workspaces", 0755);

    std::string json = buildWorkspaceJson();
    const char *path = "workspaces/last_workspace.json";
    const char *tmpPath = "workspaces/last_workspace.json.tmp";
    std::ofstream out(tmpPath, std::ios::out | std::ios::trunc);
    if (!out) {
        std::string msg = std::string("Failed to open ") + tmpPath + " for writing";
        messageBox(msg.c_str(), mfError | mfOKButton);
        return;
    }
    out << json;
    out.close();
    if (!out.good()) {
        std::string msg = std::string("Error writing ") + tmpPath;
        messageBox(msg.c_str(), mfError | mfOKButton);
        return;
    }
    // Atomic replace
    std::remove(path); // ignore errors
    std::rename(tmpPath, path);
    // Also write a timestamped snapshot: YYMMDD_HHMM
    char tsName[32];
    std::time_t t = std::time(nullptr);
    std::tm *lt = std::localtime(&t);
    std::strftime(tsName, sizeof(tsName), "%y%m%d_%H%M", lt);
    std::string snapPath = std::string("workspaces/last_workspace_") + tsName + ".json";
    std::ofstream snap(snapPath.c_str(), std::ios::out | std::ios::trunc);
    if (snap) {
        snap << json;
        snap.close();
    }
    std::string ok = std::string("Workspace saved to ") + path + "\nSnapshot: " + snapPath;
    messageBox(ok.c_str(), mfInformation | mfOKButton);
}
