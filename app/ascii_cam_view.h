/*---------------------------------------------------------*/
/*                                                         */
/*   ascii_cam_view.h - ASCII Art Webcam View             */
/*                                                         */
/*   Real-time ASCII art conversion of webcam feed        */
/*   with automatic window size scaling.                  */
/*                                                         */
/*---------------------------------------------------------*/

#ifndef ASCII_CAM_VIEW_H
#define ASCII_CAM_VIEW_H

#define Uses_TView
#define Uses_TRect
#define Uses_TDrawBuffer
#define Uses_TEvent
#define Uses_TColorAttr
#include <tvision/tv.h>

#include <vector>
#include <string>

class TAsciiCamView : public TView {
public:
    explicit TAsciiCamView(const TRect &bounds, unsigned periodMs = 80);
    virtual ~TAsciiCamView();

    virtual void draw() override;
    virtual void handleEvent(TEvent &ev) override;
    virtual void setState(ushort aState, Boolean enable) override;
    virtual void changeBounds(const TRect& bounds) override;

private:
    void startTimer();
    void stopTimer();
    void advance();
    bool pollSocket();
    void sendWindowSize();

    // Rendering params
    unsigned periodMs;
    TTimerId timerId {0};
    int frame {0};
    bool debugHud {true};

    // ASCII gradient (10 levels, dark to light)
    std::string gradient {" .:-=+*#%@"};

    // Latest ASCII frame
    int camW {0}, camH {0};
    std::vector<std::string> asciiLines;

    // Socket state
    int sockFd {-1};
    std::string inHdr;
    std::vector<unsigned char> inPayload;
    int needBytes {0};
    std::string connectionStatus {"connecting"};

    // Stats
    unsigned long framesRx {0};
    unsigned long framesSinceTick {0};
    unsigned long lastTickMs {0};
    float rxFps {0.f};

    // Track last sent dimensions to avoid redundant updates
    int lastSentW {0};
    int lastSentH {0};
};

class TWindow;
TWindow* createAsciiCamWindow(const TRect &bounds);

#endif // ASCII_CAM_VIEW_H
