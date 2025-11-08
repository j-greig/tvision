/*---------------------------------------------------------*/
/*                                                         */
/*   wibwob_view.cpp - Wib&Wob AI Chat Interface          */
/*                                                         */
/*---------------------------------------------------------*/

#include "wibwob_view.h"
#include "wibwob_engine.h"

#define Uses_TKeys
#define Uses_TDrawBuffer
#define Uses_TColorAttr
#define Uses_TTimerEvent
#define Uses_MsgBox
#define Uses_TWindow
#define Uses_TFrame
#include <tvision/tv.h>

#include <ctime>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <fstream>
#include <random>
#include <sys/stat.h>
#include <iterator>

TWibWobView::TWibWobView(const TRect& bounds) : TView(bounds) {
    options |= ofSelectable;
    eventMask |= evKeyDown | evBroadcast;
    
    engine = nullptr; // Lazy initialization
    
    statusText = "Type a message and press Enter";
    
    // Initialize spinner
    showSpinner = false;
    spinnerFrame = 0;
    spinnerTimerId = nullptr;
    
    // Defer logging initialization and welcome message until first access
}

void TWibWobView::ensureEngineInitialized() {
    if (!engineInitialized) {
        // Initialize logging first
        if (logFilePath.empty()) {
            initializeLogging();
        }
        
        engine = new WibWobEngine();
        
        // Load system prompt from file or use fallback
        std::ifstream promptFile("wibandwob.prompt.md");
        if (promptFile.is_open()) {
            // Read the entire prompt file
            std::string customPrompt((std::istreambuf_iterator<char>(promptFile)),
                                   std::istreambuf_iterator<char>());
            promptFile.close();
            
            engine->setSystemPrompt(customPrompt);
            addMessage("System", "Step into WibWobWorld, human.");
            logMessage("System", "Loaded custom prompt from wibandwob.prompt.md");
        } else {
            // Fallback system prompt
            engine->setSystemPrompt(
                "You are wib&wob, a dual-minded artist/scientist AI assistant integrated into a Turbo Vision TUI application. "
                "Respond as both Wib (chaotic, artistic) and Wob (precise, scientific). "
                "Help with TVision framework, C++ development, and creative projects. "
                "Use British English and maintain your distinctive personalities."
            );
            addMessage("Wib", "Wotcher! I'm wib&wob, your AI assistant for this TVision app. (Note: wibandwob.prompt.md not found - using fallback prompt)");
        }
        
        // Defer provider availability checks to first send to avoid UI stalls.
        statusText = "Ready - Type a message and press Enter";
        
        // Do not touch provider/config here; keep init instant
        logMessage("System", "Chat engine initialized (provider loads on first send)");
        
        engineInitialized = true;
        drawView(); // Refresh to show the welcome message
    }
}

TWibWobView::~TWibWobView() {
    stopSpinner();
    delete engine;
}

void TWibWobView::draw() {
    // REMOVED: ensureEngineInitialized() - now deferred to first user input to avoid UI freeze

    TDrawBuffer buf;
    TColorAttr normalColor = getColor(1);
    TColorAttr focusedColor = getColor(2);
    TColorAttr statusColor = getColor(3);
    
    // Calculate layout - this updates when window is resized
    maxVisibleLines = getMessageAreaHeight();
    
    // Clear the entire view first
    buf.moveChar(0, ' ', normalColor, size.x);
    for (int y = 0; y < size.y; y++) {
        writeLine(0, y, size.x, 1, buf);
    }
    
    // Draw messages area
    drawMessages();
    
    // Draw status line
    drawStatus();
    
    // Draw input line
    drawInputLine();
}

void TWibWobView::drawMessages() {
    TDrawBuffer buf;
    TColorAttr normalColor = getColor(1);
    TColorAttr userColor = getColor(2);  
    TColorAttr wibColor = getColor(3);
    TColorAttr errorColor = getColor(4);
    
    int y = 0;
    int maxY = getMessageAreaHeight();
    int startMsg = std::max(0, (int)messages.size() - maxY + scrollOffset);
    
    for (int i = startMsg; i < (int)messages.size() && y < maxY; ++i) {
        const auto& msg = messages[i];
        
        // Choose color based on sender and error state
        TColorAttr msgColor = normalColor;
        if (msg.is_error) {
            msgColor = errorColor;
        } else if (msg.sender == "User") {
            msgColor = userColor;
        } else if (msg.sender == "Wib") {
            msgColor = wibColor;
        }
        
        // Format message: "Sender: Content"
        std::string displayText = msg.sender + ": " + msg.content;
        
        // Word wrap if needed
        auto wrappedLines = wrapText(displayText, size.x);
        for (const auto& line : wrappedLines) {
            if (y >= maxY) break;
            
            buf.moveChar(0, ' ', msgColor, size.x);
            buf.moveStr(0, line.c_str(), msgColor);
            writeLine(0, y, size.x, 1, buf);
            y++;
        }
    }
    
    // Fill remaining area
    while (y < maxY) {
        buf.moveChar(0, ' ', normalColor, size.x);
        writeLine(0, y, size.x, 1, buf);
        y++;
    }
}

void TWibWobView::drawStatus() {
    TDrawBuffer buf;
    TColorAttr statusColor = getColor(5);
    
    int statusY = size.y - 2;  // Second to last line
    
    buf.moveChar(0, ' ', statusColor, size.x);
    
    std::string status;
    if (showSpinner) {
        // Spinner characters
        const char spinnerChars[] = {'|', '/', '-', '\\'};
        char spinnerChar = spinnerChars[spinnerFrame % 4];
        status = "[" + statusText + " " + spinnerChar + "]";
    } else {
        status = "[" + statusText + "]";
    }
    
    if (status.length() > (size_t)size.x) {
        status = status.substr(0, size.x - 3) + "...";
    }
    buf.moveStr(0, status.c_str(), statusColor);
    writeLine(0, statusY, size.x, 1, buf);
}

void TWibWobView::drawInputLine() {
    TDrawBuffer buf;
    TColorAttr inputColor = (state & sfFocused) ? getColor(6) : getColor(1);
    
    int inputY = size.y - 1;  // Last line
    
    buf.moveChar(0, ' ', inputColor, size.x);
    
    std::string prompt = "> ";
    std::string display = prompt + currentInput;
    
    // Handle cursor and scrolling if input is too long
    if (display.length() > (size_t)size.x) {
        display = display.substr(display.length() - size.x);
    }
    
    buf.moveStr(0, display.c_str(), inputColor);
    
    // Show cursor if focused
    if ((state & sfFocused) && inputActive) {
        int cursorPos = std::min((int)display.length(), size.x - 1);
        buf.moveChar(cursorPos, display[cursorPos], inputColor | 0x80, 1);  // Reverse video for cursor
    }
    
    writeLine(0, inputY, size.x, 1, buf);
}

void TWibWobView::handleEvent(TEvent& event) {
    TView::handleEvent(event);

    if (event.what == evKeyDown) {
        handleKeyDown(event);
        clearEvent(event);
    } else if (event.what == evBroadcast && event.message.command == cmTimerExpired) {
        if (event.message.infoPtr == spinnerTimerId) {
            updateSpinner();

            // Also poll engine for async responses
            if (engineInitialized && engine) {
                engine->poll();
            }

            clearEvent(event);
        }
    }
    // REMOVED: cmScrollBarChanged handler (causing segfaults on line 248)
    // Scrollbar still works via keyboard navigation (Up/Down/PgUp/PgDn)
}

void TWibWobView::handleKeyDown(TEvent& event) {
    switch (event.keyDown.keyCode) {
        case kbEnter:
            if (!currentInput.empty()) {
                processInput();
            }
            break;
            
        case kbBack:
            if (!currentInput.empty()) {
                currentInput.pop_back();
                drawView();
            }
            break;
            
        case kbUp:
            scrollUp();
            break;
            
        case kbDown:
            scrollDown();
            break;
            
        case kbPgUp:
            scrollUp(maxVisibleLines / 2);
            break;
            
        case kbPgDn:
            scrollDown(maxVisibleLines / 2);
            break;
            
        case kbHome:
            scrollOffset = 0;
            drawView();
            break;
            
        case kbEnd:
            scrollOffset = -(int)messages.size();
            drawView();
            break;
            
        case kbEsc:
            ensureEngineInitialized();
            if (engine && engine->isBusy()) {
                engine->cancel();
                setStatus("Request cancelled - Type a message and press Enter");
                inputActive = true;
                drawView();
            }
            break;
            
        default:
            if (event.keyDown.charScan.charCode >= 32 && event.keyDown.charScan.charCode < 127) {
                currentInput += (char)event.keyDown.charScan.charCode;
                drawView();
            }
            break;
    }
}

void TWibWobView::processInput() {
    ensureEngineInitialized();

    if (currentInput.empty() || engine->isBusy()) {
        return;
    }

    std::string userMessage = currentInput;
    currentInput.clear();

    // Handle slash commands
    if (userMessage == "/clear") {
        clearChat();
        addMessage("System", "Chat cleared");
        drawView();
        return;
    }

    if (userMessage == "/model") {
        std::string providerInfo = "Provider: " + engine->getCurrentProvider() +
                                   "\nModel: " + engine->getCurrentModel();
        addMessage("System", providerInfo);
        drawView();
        return;
    }

    if (userMessage == "/help") {
        std::string helpText = "Available commands:\n"
                              "/clear - Clear chat history\n"
                              "/model - Show current provider and model\n"
                              "/help - Show this help message";
        addMessage("System", helpText);
        drawView();
        return;
    }

    // Add user message to chat
    addMessage("User", userMessage);
    
    // Set status and start spinner with provider info
    std::string providerName = engine->getCurrentProvider();
    std::string modelName = engine->getCurrentModel();
    static const char* statusOptions[] = {
        "Wibbling ...", "Wobbling ...", "Scrambling ...", "Reticulating ...", "Whizzing ...", "Puttering ..."
    };
    std::string statusMsg = statusOptions[rand() % (sizeof(statusOptions) / sizeof(statusOptions[0]))];
    //std::string statusMsg = "Thinking with " + modelName + " (" + providerName + ")...";
    setStatus(statusMsg);
    inputActive = false;
    startSpinner();
    drawView();
    
    // Send to LLM provider
    auto start = std::chrono::high_resolution_clock::now();
    
    // Log provider info to chat log
    logMessage("System", "Using provider: " + engine->getCurrentProvider() + ", model: " + engine->getCurrentModel());
    
    engine->sendQuery(userMessage, [this, start](const ClaudeResponse& response) {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        stopSpinner();
        inputActive = true;
        if (response.is_error) {
            addMessage("System", "Error (" + std::to_string(duration.count()) + "ms): " + response.error_message, true);
            
            // Log raw API response on errors too
            if (!response.session_id.empty() && response.session_id.find("RAW_API_RESPONSE:") == 0) {
                logMessage("API_RAW_ERROR", response.session_id.substr(17));
            }
            
            setStatus("Error - Try again");
        } else {
            // Log debug info including raw API response
            logMessage("Debug", "Response length: " + std::to_string(response.result.length()) + " chars");
            logMessage("Debug", "Provider: " + response.provider_name + ", Model: " + response.model_used);
            
            // Log raw API response if available
            if (!response.session_id.empty() && response.session_id.find("RAW_API_RESPONSE:") == 0) {
                logMessage("API_RAW", response.session_id.substr(17)); // Remove "RAW_API_RESPONSE: " prefix
            }
            
            addMessage("Wib&Wob", response.result);
            setStatus("Ready (" + std::to_string(duration.count()) + "ms) - Type a message and press Enter");
        }
        drawView();
        setState(sfExposed, True);  // Force redraw to ensure UI updates

        // Bring chat window back to front after MCP commands complete
        // (MCP may have spawned windows that now cover the chat)
        if (owner && owner->owner) {
            // owner = TWibWobWindow, owner->owner = deskTop
            owner->select();
        }
    });
}

void TWibWobView::sendMessage(const std::string& message) {
    currentInput = message;
    processInput();
}

void TWibWobView::addMessage(const std::string& sender, const std::string& content, bool is_error) {
    ChatMessage msg;
    msg.sender = sender;
    msg.content = content;
    msg.timestamp = getCurrentTime();
    msg.is_error = is_error;

    messages.push_back(msg);

    // Log the message
    logMessage(sender, content, is_error);

    // Auto-scroll to bottom
    ensureInputVisible();

    // Update scrollbar when message added
    notifyScrollBarUpdate();
    drawView();
}

void TWibWobView::setStatus(const std::string& status) {
    statusText = status;
}

void TWibWobView::clearChat() {
    messages.clear();
    scrollOffset = 0;
    addMessage("Wib", "Chat cleared. What can I help you with?");
}

void TWibWobView::scrollUp(int lines) {
    scrollOffset = std::max(scrollOffset - lines, -(int)messages.size() + 1);
    notifyScrollBarUpdate();
    drawView();
}

void TWibWobView::scrollDown(int lines) {
    scrollOffset = std::min(scrollOffset + lines, 0);
    notifyScrollBarUpdate();
    drawView();
}

void TWibWobView::ensureInputVisible() {
    scrollOffset = 0;  // Always show the most recent messages
    notifyScrollBarUpdate();
}

std::vector<std::string> TWibWobView::wrapText(const std::string& text, int width) const {
    std::vector<std::string> lines;
    if (width <= 0) {
        lines.emplace_back("");
        return lines;
    }

    size_t lineStart = 0;
    const size_t length = text.size();

    while (lineStart <= length) {
        size_t newlinePos = text.find('\n', lineStart);
        std::string segment;
        if (newlinePos == std::string::npos) {
            segment = text.substr(lineStart);
        } else {
            segment = text.substr(lineStart, newlinePos - lineStart);
        }

        if (!segment.empty() && segment.back() == '\r') {
            segment.pop_back();
        }

        if (segment.empty()) {
            lines.emplace_back("");
        } else {
            size_t pos = 0;
            while (pos < segment.size()) {
                size_t remaining = segment.size() - pos;
                size_t slice = remaining > static_cast<size_t>(width) ? static_cast<size_t>(width) : remaining;
                bool trimmedSpace = false;

                if (remaining > static_cast<size_t>(width)) {
                    size_t breakPos = segment.find_last_of(" \t", pos + width - 1);
                    if (breakPos != std::string::npos && breakPos >= pos) {
                        size_t candidate = breakPos - pos;
                        if (candidate > 0) {
                            slice = candidate;
                            trimmedSpace = true;
                        }
                    }
                }

                lines.push_back(segment.substr(pos, slice));
                pos += slice;

                if (trimmedSpace) {
                    while (pos < segment.size() && segment[pos] == ' ') {
                        ++pos;
                    }
                }
            }
        }

        if (newlinePos == std::string::npos) {
            break;
        }

        lineStart = newlinePos + 1;
        if (lineStart == length) {
            lines.emplace_back("");
            break;
        }
    }

    if (lines.empty()) {
        lines.emplace_back("");
    }

    return lines;
}

std::string TWibWobView::getCurrentTime() const {
    std::time_t now = std::time(nullptr);
    std::tm* local = std::localtime(&now);
    
    std::ostringstream oss;
    oss << std::put_time(local, "%H:%M:%S");
    return oss.str();
}

int TWibWobView::calculateTotalWrappedLines() const {
    int totalLines = 0;
    int viewWidth = size.x - 1;  // Account for scrollbar

    for (const auto& msg : messages) {
        std::string displayText = msg.sender + ": " + msg.content;
        auto wrappedLines = wrapText(displayText, viewWidth);
        totalLines += wrappedLines.size();
    }

    return totalLines;
}

void TWibWobView::notifyScrollBarUpdate() {
    // Find parent window and trigger scrollbar update
    // Walk up the parent chain to find TWibWobWindow
    TView* parent = owner;
    while (parent) {
        auto* window = dynamic_cast<TWibWobWindow*>(parent);
        if (window) {
            window->updateScrollBarLimit();
            break;
        }
        parent = parent->owner;
    }
}

int TWibWobView::getMessageAreaHeight() const {
    return size.y - getInputAreaHeight() - getStatusAreaHeight();
}

void TWibWobView::setState(ushort aState, Boolean enable) {
    TView::setState(aState, enable);
    if (aState & sfFocused) {
        drawView();
    }
}

void TWibWobView::changeBounds(const TRect& bounds) {
    TView::changeBounds(bounds);
    // Force a redraw when bounds change to handle resize
    // Also update scrollbar limits when the view is resized
    notifyScrollBarUpdate();
    drawView();
}

void TWibWobView::startSpinner() {
    if (showSpinner) return; // Already running
    
    showSpinner = true;
    spinnerFrame = 0;
    
    // Set a timer for 200ms intervals
    spinnerTimerId = setTimer(200, 200);
}

void TWibWobView::stopSpinner() {
    if (!showSpinner) return; // Not running
    
    showSpinner = false;
    
    if (spinnerTimerId) {
        killTimer(spinnerTimerId);
        spinnerTimerId = nullptr;
    }
}

void TWibWobView::updateSpinner() {
    if (!showSpinner) return;
    
    spinnerFrame++;
    drawView(); // Redraw to show new spinner frame
}

void TWibWobView::initializeLogging() {
    // Generate unique session ID
    sessionId = generateSessionId();
    
    // Create logs directory if it doesn't exist
    mkdir("logs", 0755);
    
    // Create log file with timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ss;
    ss << "logs/chat_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S") 
       << "_" << sessionId << ".log";
    logFilePath = ss.str();
    
    // Write session header
    std::ofstream logFile(logFilePath, std::ios::app);
    if (logFile.is_open()) {
        logFile << "=== WibWob Chat Session ===" << std::endl;
        logFile << "Session ID: " << sessionId << std::endl;
        logFile << "Started: " << getTimestamp() << std::endl;
        logFile << "Provider: [To be determined]" << std::endl;
        logFile << "============================" << std::endl;
        logFile.close();
    }
}

void TWibWobView::logMessage(const std::string& sender, const std::string& content, bool is_error) {
    if (logFilePath.empty()) return;
    
    std::ofstream logFile(logFilePath, std::ios::app);
    if (logFile.is_open()) {
        std::string timestamp = getTimestamp();
        std::string status = is_error ? " [ERROR]" : "";
        
        logFile << "[" << timestamp << "] " << sender << status << ": " << content << std::endl;
        
        // Add extra metadata for errors
        if (is_error) {
            logFile << "    ^^ Error occurred during message processing" << std::endl;
        }
        
        logFile.close();
    }
}

std::string TWibWobView::generateSessionId() const {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(100000, 999999);
    return std::to_string(dis(gen));
}

std::string TWibWobView::getTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

TWibWobWindow::TWibWobWindow(const TRect& bounds, const std::string& title)
    : TWindow(bounds, title.c_str(), wnNoNumber)
    , TWindowInit(&TWibWobWindow::initFrame)
    , baseTitle(title)
{
    options |= ofTileable;
    growMode = gfGrowHiX | gfGrowHiY;

    TRect client = getExtent();
    client.grow(-1, -1);

    // Create vertical scrollbar on the right edge
    TRect scrollBarRect = client;
    scrollBarRect.a.x = scrollBarRect.b.x - 1;
    vScrollBar = new TScrollBar(scrollBarRect);
    vScrollBar->growMode = gfGrowLoY | gfGrowHiY;
    insert(vScrollBar);

    // Adjust client area to make room for scrollbar
    client.b.x -= 1;

    chatView = new TWibWobView(client);
    chatView->growMode = gfGrowHiX | gfGrowHiY;
    insert(chatView);
}

void TWibWobWindow::updateTitleWithSession(const std::string& sessionId) {
    if (!sessionId.empty()) {
        // Show first 8 chars of session ID
        std::string shortId = sessionId.length() > 8 ? sessionId.substr(0, 8) : sessionId;
        std::string newTitle = baseTitle + " [" + shortId + "]";

        // Update window title
        if (frame) {
            delete[] (char*)title;
            title = newStr(newTitle.c_str());
            frame->drawView();
        }
    }
}

void TWibWobWindow::updateScrollBarLimit()
{
    if (!vScrollBar || !chatView) return;

    // Calculate total wrapped lines in the chat
    int totalWrappedLines = chatView->calculateTotalWrappedLines();
    int messageAreaHeight = chatView->getMessageAreaHeight();

    // Calculate max scrollable range
    int maxScrollableLines = std::max(0, totalWrappedLines - messageAreaHeight);

    // Get current scroll offset from chatView
    int currentScroll = -chatView->getScrollOffset();  // Negate because scrollOffset is negative

    // Update scrollbar parameters
    vScrollBar->setParams(currentScroll, 0, maxScrollableLines, messageAreaHeight, 1);
}

void TWibWobWindow::changeBounds(const TRect& bounds)
{
    TWindow::changeBounds(bounds);

    // TWindow::changeBounds() already handles child view resizing via growMode
    // The scrollbar has growMode gfGrowLoY | gfGrowHiY (grows with bottom edge)
    // The chatView has growMode gfGrowHiX | gfGrowHiY (grows with right and bottom edges)
    // No manual repositioning needed - just trigger redraws

    setState(sfExposed, True);

    // Update scrollbar limits when window is resized
    updateScrollBarLimit();

    redraw();
}

TFrame* TWibWobWindow::initFrame(TRect r)
{
    return new TFrame(r);
}

TWindow* createWibWobWindow(const TRect& bounds, const std::string& title)
{
    return new TWibWobWindow(bounds, title);
}
