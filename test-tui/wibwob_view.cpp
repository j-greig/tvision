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
#include <tvision/tv.h>

#include <ctime>
#include <sstream>
#include <iomanip>
#include <chrono>

TWibWobView::TWibWobView(const TRect& bounds) : TView(bounds) {
    options |= ofSelectable;
    eventMask |= evKeyDown | evBroadcast;
    
    engine = new WibWobEngine();
    
    // Set up fallback system prompt (used only if wibandwob.prompt.md is not found)
    engine->setSystemPrompt(
        "You are wib&wob, a dual-minded artist/scientist AI assistant integrated into a Turbo Vision TUI application. "
        "Respond as both Wib (chaotic, artistic) and Wob (precise, scientific). "
        "Help with TVision framework, C++ development, and creative projects. "
        "Use British English and maintain your distinctive personalities."
    );
    
    statusText = engine->isClaudeAvailable() ? "Ready - Type a message and press Enter" : "Claude Code not available";
    
    // Add welcome message - check if custom prompt file exists
    FILE* promptCheck = fopen("wibandwob.prompt.md", "r");
    if (promptCheck) {
        fclose(promptCheck);
        addMessage("System", "Wib&Wob loaded with custom prompt file! Ask them anything...");
    } else {
        addMessage("Wib", "Wotcher! I'm wib&wob, your AI assistant for this TVision app. (Note: wibandwob.prompt.md not found - using fallback prompt)");
    }
}

TWibWobView::~TWibWobView() {
    delete engine;
}

void TWibWobView::draw() {
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
    std::string status = "[" + statusText + "]";
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
    }
    
    // Always poll the engine for async responses
    if (engine) {
        engine->poll();
    }
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
    if (currentInput.empty() || engine->isBusy()) {
        return;
    }
    
    std::string userMessage = currentInput;
    currentInput.clear();
    
    // Add user message to chat
    addMessage("User", userMessage);
    
    // Set status to thinking
    setStatus("Calling Claude Code...");
    inputActive = false;
    drawView();
    
    // Send to Claude (synchronous for POC)
    auto start = std::chrono::high_resolution_clock::now();
    engine->sendQuery(userMessage, [this, start](const ClaudeResponse& response) {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        inputActive = true;
        if (response.is_error) {
            addMessage("System", "Error (" + std::to_string(duration.count()) + "ms): " + response.error_message, true);
            setStatus("Error - Try again");
        } else {
            addMessage("Wib&Wob", response.result);
            setStatus("Ready (" + std::to_string(duration.count()) + "ms) - Type a message and press Enter");
        }
        drawView();
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
    
    // Auto-scroll to bottom
    ensureInputVisible();
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
    drawView();
}

void TWibWobView::scrollDown(int lines) {
    scrollOffset = std::min(scrollOffset + lines, 0);
    drawView();
}

void TWibWobView::ensureInputVisible() {
    scrollOffset = 0;  // Always show the most recent messages
}

std::vector<std::string> TWibWobView::wrapText(const std::string& text, int width) const {
    std::vector<std::string> lines;
    if (width <= 0) return lines;
    
    std::istringstream words(text);
    std::string word;
    std::string currentLine;
    
    while (words >> word) {
        if (currentLine.empty()) {
            currentLine = word;
        } else if (currentLine.length() + 1 + word.length() <= (size_t)width) {
            currentLine += " " + word;
        } else {
            lines.push_back(currentLine);
            currentLine = word;
        }
    }
    
    if (!currentLine.empty()) {
        lines.push_back(currentLine);
    }
    
    return lines.empty() ? std::vector<std::string>{""} : lines;
}

std::string TWibWobView::getCurrentTime() const {
    std::time_t now = std::time(nullptr);
    std::tm* local = std::localtime(&now);
    
    std::ostringstream oss;
    oss << std::put_time(local, "%H:%M:%S");
    return oss.str();
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
    drawView();
}