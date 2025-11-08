/*---------------------------------------------------------*/
/*                                                         */
/*   webcam.h : Webcam ASCII Art Window for TVDemo        */
/*                                                         */
/*---------------------------------------------------------*/

#if !defined( __WEBCAM_H )
#define __WEBCAM_H

#include <string>
#include <vector>

// Forward declarations
class TRect;
class TEvent;

// ASCII Converter Class
class AsciiConverter {
public:
    AsciiConverter();

    enum CharSet {
        MINIMAL,    // 8 chars: ' .:+#@'
        SIMPLE,     // 12 chars: ' .:-=+*#%@'
        STANDARD,   // 70 chars: full range
        BLOCKS      // Block elements: ' ░▒▓█'
    };

    void setCharSet(CharSet set);
    std::string frameToAscii(const unsigned char* data, int width, int height, int targetWidth, int targetHeight);

private:
    std::string charSet;
    char pixelToChar(unsigned char r, unsigned char g, unsigned char b);
    int getBrightness(unsigned char r, unsigned char g, unsigned char b);
    void initCharSet(CharSet set);
};

// Webcam View Class - displays the ASCII art
class TWebcamView : public TView {
public:
    TWebcamView(const TRect& bounds);

    virtual void draw();
    void setAsciiData(const std::string& ascii, int width, int height);
    void setStatus(const std::string& status);

private:
    std::vector<std::string> asciiLines;
    std::string statusText;
    int frameWidth;
    int frameHeight;
};

#ifdef HAVE_OPENCV
// OpenCV-based webcam capture
#include <opencv2/opencv.hpp>

class WebcamCapture {
public:
    WebcamCapture();
    ~WebcamCapture();

    bool start(int deviceId = 0);
    void stop();
    bool isCapturing() const { return capturing; }

    bool captureFrame(unsigned char* &data, int &width, int &height);

private:
    cv::VideoCapture capture;
    cv::Mat frame;
    cv::Mat frameRGB;
    bool capturing;
};

#else
// Mock/Test Pattern Generator (fallback without OpenCV)
class WebcamCapture {
public:
    WebcamCapture();
    ~WebcamCapture();

    bool start(int deviceId = 0);
    void stop();
    bool isCapturing() const { return capturing; }

    bool captureFrame(unsigned char* &data, int &width, int &height);

private:
    std::vector<unsigned char> testPattern;
    bool capturing;
    int frameCount;
    int patternWidth;
    int patternHeight;

    void generateTestPattern();
};
#endif

// Main Webcam ASCII Window
class TWebcamAsciiWindow : public TWindow {
public:
    TWebcamAsciiWindow();
    ~TWebcamAsciiWindow();

    virtual void handleEvent(TEvent& event);
    virtual void sizeLimits(TPoint& min, TPoint& max);

    void idle();  // Called from app idle() to update frames

private:
    TWebcamView *asciiView;
    WebcamCapture *capture;
    AsciiConverter *converter;
    bool isCapturing;
    unsigned long lastUpdateTime;
    int frameCount;
    int targetFPS;

    void startCapture();
    void stopCapture();
    void updateFrame();
    unsigned long getCurrentTimeMs();
};

#endif // __WEBCAM_H
