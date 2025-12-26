/*---------------------------------------------------------*/
/*                                                         */
/*   ascii_cam_view.cpp - ASCII Art Webcam View           */
/*                                                         */
/*   Reads ASCII art frames from /tmp/ascii_cam.sock      */
/*   (Python/OpenCV worker) and renders with grayscale    */
/*   colors. Window size dictates ASCII resolution.       */
/*                                                         */
/*   Controls:                                             */
/*     Space: pause/resume                                 */
/*     +/-  : adjust frame cadence                         */
/*     r    : reset buffers                                */
/*     v    : toggle HUD                                   */
/*                                                         */
/*---------------------------------------------------------*/

#include "ascii_cam_view.h"
#include "notitle_frame.h"

#define Uses_TWindow
#define Uses_TFrame
#define Uses_TColorAttr
#define Uses_TDrawBuffer
#include <tvision/tv.h>

#include <cmath>
#include <cstring>
#include <algorithm>
#include <string>
#include <vector>
#include <chrono>

#ifndef _WIN32
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#endif

TAsciiCamView::TAsciiCamView(const TRect &bounds, unsigned periodMs)
    : TView(bounds), periodMs(periodMs)
{
    options |= ofSelectable;
    growMode = gfGrowHiX | gfGrowHiY;
    eventMask |= evBroadcast | evKeyboard;
}

TAsciiCamView::~TAsciiCamView(){
    stopTimer();
#ifndef _WIN32
    if (sockFd>=0) { close(sockFd); sockFd=-1; }
#endif
}

void TAsciiCamView::startTimer(){ if (!timerId) timerId = setTimer(periodMs, (int)periodMs); }
void TAsciiCamView::stopTimer(){ if (timerId){ killTimer(timerId); timerId = 0; } }

void TAsciiCamView::advance(){
    ++frame;
    pollSocket();
}

void TAsciiCamView::sendWindowSize(){
#ifndef _WIN32
    if (sockFd < 0) return;

    int W = size.x, H = size.y;

    // Only send if size changed
    if (W == lastSentW && H == lastSentH) return;

    // Send resize command as JSON line
    char buf[256];
    int len = std::snprintf(buf, sizeof(buf), "{\"cmd\":\"resize\",\"cols\":%d,\"rows\":%d}\n", W, H);
    if (len > 0) {
        ::write(sockFd, buf, len);
        lastSentW = W;
        lastSentH = H;
    }
#endif
}

bool TAsciiCamView::pollSocket(){
#ifndef _WIN32
    // Lazy connect with retry
    if (sockFd < 0){
        static unsigned long lastTryMs = 0;
        auto nowMs = (unsigned long) std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();

        if (nowMs - lastTryMs < 500) return false;
        lastTryMs = nowMs;

        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd >= 0){
            sockaddr_un addr;
            std::memset(&addr, 0, sizeof(addr));
            addr.sun_family = AF_UNIX;
            std::snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", "/tmp/ascii_cam.sock");
            if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == 0){
                int flags = fcntl(fd, F_GETFL, 0);
                fcntl(fd, F_SETFL, flags | O_NONBLOCK);
                sockFd = fd;
                inHdr.clear();
                inPayload.clear();
                needBytes = 0;
                connectionStatus = "connected";
                // Send initial window size
                sendWindowSize();
            } else {
                close(fd);
                connectionStatus = "failed";
            }
        } else {
            connectionStatus = "socket_error";
        }
    }
    if (sockFd < 0) return false;

    // Read header line
    if (needBytes == 0){
        char buf[512];
        int n = (int)::read(sockFd, buf, sizeof(buf));
        if (n <= 0) {
            if (n < 0 && errno == EAGAIN) return false;
            close(sockFd); sockFd = -1; connectionStatus = "disconnected";
            return false;
        }
        inHdr.append(buf, buf + n);
        auto pos = inHdr.find('\n');
        if (pos != std::string::npos){
            std::string header = inHdr.substr(0, pos);
            inHdr.erase(0, pos+1);

            // Parse JSON header for width, height, line count
            int w=0, h=0;
            auto findInt=[&](const char* key, int &out){
                size_t k=header.find(key);
                if(k==std::string::npos) return;
                size_t c=header.find(':', k);
                if(c!=std::string::npos){
                    out=std::atoi(header.c_str()+c+1);
                }
            };
            findInt("\"cols\"", w);
            findInt("\"rows\"", h);

            if (w>0 && h>0){
                camW = w;
                camH = h;
                asciiLines.clear();
                asciiLines.reserve(h);
                needBytes = h; // Expecting h lines
            }
            return true;
        }
    }

    // Read ASCII lines
    if (needBytes > 0){
        char buf[4096];
        int n = (int)::read(sockFd, buf, sizeof(buf));
        if (n < 0 && errno == EAGAIN) return false;
        if (n <= 0) {
            close(sockFd); sockFd = -1; connectionStatus = "disconnected";
            return false;
        }

        inHdr.append(buf, buf+n);

        // Extract complete lines
        while (needBytes > 0) {
            auto pos = inHdr.find('\n');
            if (pos == std::string::npos) break;

            std::string line = inHdr.substr(0, pos);
            inHdr.erase(0, pos+1);
            asciiLines.push_back(line);
            needBytes--;
        }

        if (needBytes == 0) {
            ++framesRx;
            ++framesSinceTick;
            return true;
        }
    }
#endif
    return false;
}

void TAsciiCamView::draw(){
    int W=size.x, H=size.y;
    if (W<=0 || H<=0) return;

    // Update FPS
    auto nowMs = (unsigned long) std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    if (lastTickMs == 0) lastTickMs = nowMs;
    unsigned long dt = nowMs - lastTickMs;
    if (dt >= 900) {
        rxFps = dt ? (framesSinceTick * 1000.0f / (float)dt) : 0.f;
        framesSinceTick = 0;
        lastTickMs = nowMs;
    }

    // Draw ASCII art
    int numLines = std::min(H, (int)asciiLines.size());
    for (int y = 0; y < numLines; ++y) {
        TDrawBuffer b;
        const std::string& line = asciiLines[y];

        for (int x = 0; x < W && x < (int)line.length(); ++x) {
            char ch = line[x];

            // Map char to brightness (for gradient coloring)
            float brightness = 0.0f;
            for (size_t i = 0; i < gradient.length(); ++i) {
                if (gradient[i] == ch) {
                    brightness = (float)i / (float)(gradient.length() - 1);
                    break;
                }
            }

            // Grayscale color based on character brightness
            uint8_t gray = (uint8_t)(brightness * 255);
            TColorRGB fg(gray, gray, gray);
            TColorRGB bg(0, 0, 0); // Pure black background
            TColorAttr attr(fg, bg);

            b.moveChar(x, ch, attr, 1);
        }
        writeLine(0, y, W, 1, b);
    }

    // Fill remaining lines with black
    TColorAttr black(TColorRGB(0,0,0), TColorRGB(0,0,0));
    for (int y = numLines; y < H; ++y) {
        TDrawBuffer b;
        b.moveChar(0, ' ', black, W);
        writeLine(0, y, W, 1, b);
    }

    // Draw HUD if enabled
    if (debugHud) {
        TColorAttr hudAttr(TColorRGB(255,255,0), TColorRGB(0,0,0));
        char buf[256];

        // Line 0: Connection status and FPS
        std::snprintf(buf, sizeof(buf), "webcam:%s | fps:%.1f", connectionStatus.c_str(), rxFps);
        TDrawBuffer b0;
        b0.moveChar(0, ' ', black, W);
        b0.moveCStr(1, buf, hudAttr, W-1);
        writeLine(0, 0, W, 1, b0);

        // Line 1: Resolution info
        std::snprintf(buf, sizeof(buf), "ascii:%dx%d | window:%dx%d", camW, camH, W, H);
        TDrawBuffer b1;
        b1.moveChar(0, ' ', black, W);
        b1.moveCStr(1, buf, hudAttr, W-1);
        if (H > 1) writeLine(0, 1, W, 1, b1);
    }
}

void TAsciiCamView::handleEvent(TEvent &ev){
    TView::handleEvent(ev);
    if (ev.what == evKeyDown){
        int k = ev.keyDown.keyCode;
        if (k == ' ') { stopTimer(); if (!timerId) startTimer(); clearEvent(ev); }
        else if (k == '+') { periodMs = std::max(10u, periodMs - 10); stopTimer(); startTimer(); clearEvent(ev); }
        else if (k == '-') { periodMs = std::min(500u, periodMs + 10); stopTimer(); startTimer(); clearEvent(ev); }
        else if (k == 'r') { inHdr.clear(); inPayload.clear(); asciiLines.clear(); needBytes = 0; frame=0; clearEvent(ev); }
        else if (k == 'v') { debugHud = !debugHud; drawView(); clearEvent(ev); }
    }
    else if (ev.what == evBroadcast && ev.message.command == cmTimerExpired){
        if (timerId && ev.message.infoPtr == timerId){
            advance(); drawView(); clearEvent(ev);
        }
    }
}

void TAsciiCamView::setState(ushort aState, Boolean enable){
    TView::setState(aState, enable);
    if (aState == sfActive || aState == sfSelected || aState == sfExposed){
        if (enable) startTimer(); else stopTimer();
    }
}

void TAsciiCamView::changeBounds(const TRect& bounds){
    TView::changeBounds(bounds);
    // Send new size to Python worker
    sendWindowSize();
}

TWindow* createAsciiCamWindow(const TRect &bounds){
    TRect client = bounds;
    client.grow(-1,-1);
    auto* view = new TAsciiCamView(client, 80);
    auto* win = new TWindow(bounds, "ASCII Webcam", wnNoNumber);
    win->insert(view);
    win->flags &= ~wfClose;
    win->growMode = gfGrowAll | gfGrowRel;
    return win;
}
