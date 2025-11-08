/*----------------------------------------------------------*/
/*                                                          */
/*   webcam.cpp : Webcam ASCII Art Implementation          */
/*                                                          */
/*----------------------------------------------------------*/

#define Uses_TRect
#define Uses_TWindow
#define Uses_TView
#define Uses_TDrawBuffer
#define Uses_TEvent
#define Uses_TKeys
#define Uses_TPoint

#include <tvision/tv.h>
#include "webcam.h"
#include "tvcmds.h"

#include <cmath>
#include <cstring>
#include <ctime>
#include <sys/time.h>

//
// AsciiConverter Implementation
//

AsciiConverter::AsciiConverter() {
    setCharSet(STANDARD);
}

void AsciiConverter::initCharSet(CharSet set) {
    switch (set) {
        case MINIMAL:
            charSet = " .:+#@";
            break;
        case SIMPLE:
            charSet = " .:-=+*#%@";
            break;
        case STANDARD:
            charSet = "$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`'. ";
            break;
        case BLOCKS:
            charSet = " .:-=+*#%@";  // Use simple as fallback for blocks (Unicode might not render)
            break;
    }
}

void AsciiConverter::setCharSet(CharSet set) {
    initCharSet(set);
}

int AsciiConverter::getBrightness(unsigned char r, unsigned char g, unsigned char b) {
    // Standard luminance calculation
    return (int)(0.299 * r + 0.587 * g + 0.114 * b);
}

char AsciiConverter::pixelToChar(unsigned char r, unsigned char g, unsigned char b) {
    int brightness = getBrightness(r, g, b);
    int charIndex = (brightness * (charSet.length() - 1)) / 255;
    return charSet[charIndex];
}

std::string AsciiConverter::frameToAscii(const unsigned char* data, int width, int height,
                                         int targetWidth, int targetHeight) {
    std::string result;
    result.reserve(targetWidth * targetHeight + targetHeight); // preallocate

    float xRatio = (float)width / (float)targetWidth;
    float yRatio = (float)height / (float)targetHeight;

    for (int ty = 0; ty < targetHeight; ty++) {
        for (int tx = 0; tx < targetWidth; tx++) {
            // Sample from source image
            int sx = (int)(tx * xRatio);
            int sy = (int)(ty * yRatio);

            // Ensure we're within bounds
            if (sx >= width) sx = width - 1;
            if (sy >= height) sy = height - 1;

            int offset = (sy * width + sx) * 3;
            unsigned char b = data[offset];
            unsigned char g = data[offset + 1];
            unsigned char r = data[offset + 2];

            result += pixelToChar(r, g, b);
        }
        result += '\n';
    }

    return result;
}

//
// TWebcamView Implementation
//

TWebcamView::TWebcamView(const TRect& bounds) :
    TView(bounds), frameWidth(0), frameHeight(0), statusText("Initializing...")
{
    growMode = gfGrowHiX | gfGrowHiY;
}

void TWebcamView::setAsciiData(const std::string& ascii, int width, int height) {
    asciiLines.clear();
    frameWidth = width;
    frameHeight = height;

    // Split ASCII string into lines
    std::string line;
    for (char c : ascii) {
        if (c == '\n') {
            asciiLines.push_back(line);
            line.clear();
        } else {
            line += c;
        }
    }
    if (!line.empty()) {
        asciiLines.push_back(line);
    }

    drawView();
}

void TWebcamView::setStatus(const std::string& status) {
    statusText = status;
    drawView();
}

void TWebcamView::draw() {
    TDrawBuffer b;
    ushort color = getColor(0x01);  // Normal text color

    if (asciiLines.empty()) {
        // Show status message
        for (int i = 0; i < size.y; i++) {
            b.moveChar(0, ' ', color, size.x);
            if (i == size.y / 2) {
                const char* msg = statusText.c_str();
                int msgLen = strlen(msg);
                int startX = (size.x - msgLen) / 2;
                if (startX < 0) startX = 0;
                b.moveStr(startX, msg, color);
            }
            writeLine(0, i, size.x, 1, b);
        }
    } else {
        // Draw ASCII art
        int lineCount = asciiLines.size();
        if (lineCount > size.y) lineCount = size.y;

        for (int i = 0; i < lineCount; i++) {
            b.moveChar(0, ' ', color, size.x);
            const char* line = asciiLines[i].c_str();
            int lineLen = strlen(line);
            if (lineLen > size.x) lineLen = size.x;
            b.moveStr(0, line, color, lineLen);
            writeLine(0, i, size.x, 1, b);
        }

        // Fill remaining lines with spaces
        for (int i = lineCount; i < size.y; i++) {
            b.moveChar(0, ' ', color, size.x);
            writeLine(0, i, size.x, 1, b);
        }
    }
}

//
// WebcamCapture Implementation
//

#ifdef HAVE_OPENCV

WebcamCapture::WebcamCapture() : capturing(false) {
}

WebcamCapture::~WebcamCapture() {
    stop();
}

bool WebcamCapture::start(int deviceId) {
    if (capturing) return true;

    capture.open(deviceId);
    if (!capture.isOpened()) {
        return false;
    }

    // Set camera properties for better performance
    capture.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    capture.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    capture.set(cv::CAP_PROP_FPS, 30);

    capturing = true;
    return true;
}

void WebcamCapture::stop() {
    if (capturing) {
        capture.release();
        capturing = false;
    }
}

bool WebcamCapture::captureFrame(unsigned char* &data, int &width, int &height) {
    if (!capturing) return false;

    capture >> frame;
    if (frame.empty()) return false;

    // Convert from BGR to RGB
    cv::cvtColor(frame, frameRGB, cv::COLOR_BGR2RGB);

    width = frameRGB.cols;
    height = frameRGB.rows;
    data = frameRGB.data;

    return true;
}

#else

// Mock implementation for testing without OpenCV
WebcamCapture::WebcamCapture() : capturing(false), frameCount(0), patternWidth(160), patternHeight(120) {
}

WebcamCapture::~WebcamCapture() {
    stop();
}

bool WebcamCapture::start(int deviceId) {
    if (capturing) return true;

    generateTestPattern();
    capturing = true;
    return true;
}

void WebcamCapture::stop() {
    capturing = false;
}

void WebcamCapture::generateTestPattern() {
    // Generate a simple test pattern (gradient)
    testPattern.resize(patternWidth * patternHeight * 3);

    for (int y = 0; y < patternHeight; y++) {
        for (int x = 0; x < patternWidth; x++) {
            int offset = (y * patternWidth + x) * 3;

            // Create animated gradient pattern
            int frame = frameCount % 256;
            unsigned char r = (x * 255 / patternWidth + frame) % 256;
            unsigned char g = (y * 255 / patternHeight) % 256;
            unsigned char b = ((x + y) * 255 / (patternWidth + patternHeight) + frame) % 256;

            testPattern[offset] = r;
            testPattern[offset + 1] = g;
            testPattern[offset + 2] = b;
        }
    }
}

bool WebcamCapture::captureFrame(unsigned char* &data, int &width, int &height) {
    if (!capturing) return false;

    generateTestPattern();  // Update pattern each frame
    frameCount++;

    width = patternWidth;
    height = patternHeight;
    data = testPattern.data();

    return true;
}

#endif

//
// TWebcamAsciiWindow Implementation
//

TWebcamAsciiWindow::TWebcamAsciiWindow() :
    TWindow(TRect(0, 0, 60, 20), "Webcam ASCII Art", wnNoNumber),
    TWindowInit(&TWebcamAsciiWindow::initFrame),
    isCapturing(false),
    lastUpdateTime(0),
    frameCount(0),
    targetFPS(15)
{
    options |= ofTileable;
    flags &= ~(wfZoom | wfGrow);

    // Create the ASCII view
    TRect r = getExtent();
    r.grow(-1, -1);
    asciiView = new TWebcamView(r);
    insert(asciiView);

    // Create capture and converter
    capture = new WebcamCapture();
    converter = new AsciiConverter();

    // Start capture automatically
    startCapture();
}

TWebcamAsciiWindow::~TWebcamAsciiWindow() {
    stopCapture();
    delete capture;
    delete converter;
}

void TWebcamAsciiWindow::sizeLimits(TPoint& min, TPoint& max) {
    TWindow::sizeLimits(min, max);
    min.x = 30;
    min.y = 15;
}

unsigned long TWebcamAsciiWindow::getCurrentTimeMs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

void TWebcamAsciiWindow::startCapture() {
    if (isCapturing) return;

    #ifdef HAVE_OPENCV
    asciiView->setStatus("Starting camera...");
    #else
    asciiView->setStatus("Starting test pattern (OpenCV not available)...");
    #endif

    if (capture->start(0)) {
        isCapturing = true;
        lastUpdateTime = getCurrentTimeMs();
        #ifdef HAVE_OPENCV
        asciiView->setStatus("Camera active - Press ESC to close");
        #else
        asciiView->setStatus("Test pattern active - Build with OpenCV for webcam");
        #endif
    } else {
        isCapturing = false;
        #ifdef HAVE_OPENCV
        asciiView->setStatus("Failed to open camera - Press ESC to close");
        #else
        asciiView->setStatus("Failed to start test pattern");
        #endif
    }
}

void TWebcamAsciiWindow::stopCapture() {
    if (isCapturing) {
        capture->stop();
        isCapturing = false;
        asciiView->setStatus("Camera stopped");
    }
}

void TWebcamAsciiWindow::updateFrame() {
    if (!isCapturing) return;

    // Throttle to target FPS
    unsigned long currentTime = getCurrentTimeMs();
    unsigned long frameDuration = 1000 / targetFPS;

    if (currentTime - lastUpdateTime < frameDuration) {
        return;  // Not time for next frame yet
    }

    lastUpdateTime = currentTime;

    // Capture frame
    unsigned char* frameData;
    int frameWidth, frameHeight;

    if (!capture->captureFrame(frameData, frameWidth, frameHeight)) {
        asciiView->setStatus("Frame capture failed");
        return;
    }

    // Get view dimensions for ASCII conversion
    TRect r = asciiView->getBounds();
    int targetWidth = r.b.x - r.a.x;
    int targetHeight = r.b.y - r.a.y;

    if (targetWidth <= 0 || targetHeight <= 0) return;

    // Convert to ASCII
    std::string ascii = converter->frameToAscii(frameData, frameWidth, frameHeight,
                                                 targetWidth, targetHeight);

    // Update view
    asciiView->setAsciiData(ascii, targetWidth, targetHeight);

    frameCount++;
}

void TWebcamAsciiWindow::handleEvent(TEvent& event) {
    TWindow::handleEvent(event);

    if (event.what == evKeyDown) {
        switch (event.keyDown.keyCode) {
            case kbEsc:
                // Close window
                event.what = evCommand;
                event.message.command = cmClose;
                putEvent(event);
                clearEvent(event);
                break;
        }
    }
}

void TWebcamAsciiWindow::idle() {
    TWindow::idle();
    updateFrame();
}
