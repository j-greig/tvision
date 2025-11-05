/*---------------------------------------------------------*/
/*                                                         */
/*   wibwob_view.h - Wib&Wob AI Chat Interface            */
/*                                                         */
/*---------------------------------------------------------*/

#ifndef WIBWOB_VIEW_H
#define WIBWOB_VIEW_H

#define Uses_TView
#define Uses_TRect
#define Uses_TEvent
#define Uses_TKeys
#define Uses_TDrawBuffer
#define Uses_TWindow
#define Uses_TFrame
#define Uses_TScrollBar
#include <tvision/tv.h>

#include <string>
#include <vector>

// Forward declaration
class WibWobEngine;

struct ChatMessage {
    std::string sender;  // "User" or "Wib"
    std::string content;
    std::string timestamp;
    bool is_error = false;
};

class TWibWobView : public TView {
public:
    TWibWobView(const TRect& bounds);
    virtual ~TWibWobView();

    virtual void draw() override;
    virtual void handleEvent(TEvent& event) override;
    virtual void setState(ushort aState, Boolean enable) override;
    virtual void changeBounds(const TRect& bounds) override;

    // Chat operations
    void sendMessage(const std::string& message);
    void addMessage(const std::string& sender, const std::string& content, bool is_error = false);
    void setStatus(const std::string& status);
    void clearChat();

private:
    // UI state
    std::vector<ChatMessage> messages;
    std::string currentInput;
    std::string statusText;
    int scrollOffset = 0;
    int maxVisibleLines = 0;
    bool inputActive = true;
    
    // Spinner animation
    bool showSpinner = false;
    int spinnerFrame = 0;
    void* spinnerTimerId = nullptr;
    
    // Engine
    WibWobEngine* engine = nullptr;
    bool engineInitialized = false;
    
    // Logging
    std::string sessionId;
    std::string logFilePath;
    
    // Lazy initialization
    void ensureEngineInitialized();
    
    // Logging
    void initializeLogging();
    void logMessage(const std::string& sender, const std::string& content, bool is_error = false);
    std::string generateSessionId() const;
    std::string getTimestamp() const;
    
    // Drawing helpers
    void drawMessages();
    void drawInputLine();
    void drawStatus();
    void scrollUp(int lines = 1);
    void scrollDown(int lines = 1);
    void ensureInputVisible();
    
    // Input handling
    void handleKeyDown(TEvent& event);
    void handleChar(TEvent& event);
    void processInput();
    
    // Animation
    void startSpinner();
    void stopSpinner();
    void updateSpinner();
    
    // Text formatting
    std::vector<std::string> wrapText(const std::string& text, int width) const;
    std::string getCurrentTime() const;
    
    // Layout calculations
    int getMessageAreaHeight() const;
    int getInputAreaHeight() const { return 1; }
    int getStatusAreaHeight() const { return 1; }
};

class TWibWobWindow : public TWindow {
public:
    TWibWobWindow(const TRect& bounds, const std::string& title);

    virtual void changeBounds(const TRect& bounds) override;

private:
    static TFrame* initFrame(TRect r);
    TWibWobView* chatView {nullptr};
    TScrollBar* vScrollBar {nullptr};
};

TWindow* createWibWobWindow(const TRect& bounds, const std::string& title);

#endif // WIBWOB_VIEW_H
