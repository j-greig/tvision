/*---------------------------------------------------------*/
/*                                                         */
/*   frame_file_player_view.h - ASCII frame-file player   */
/*                                                         */
/*   How it works (MVP):                                  */
/*   - Loads a text file once into 'fileData'.            */
/*   - Builds 'frames' as byte spans split by lines       */
/*     exactly equal to "----" (CRLF safe).               */
/*   - Starts a periodic UI timer; on each tick advances  */
/*     'frameIndex' and requests redraw (no threads).     */
/*   - draw(): blits current frame, truncating/padding to */
/*     view width/height.                                 */
/*   - FPS sources (precedence): ctor param > header >    */
/*     default (300 ms).                                   */
/*                                                         */
/*---------------------------------------------------------*/

#ifndef FRAME_FILE_PLAYER_VIEW_H
#define FRAME_FILE_PLAYER_VIEW_H

#define Uses_TView
#define Uses_TRect
#define Uses_TDrawBuffer
#define Uses_TEvent
#define Uses_TColorAttr
#include <tvision/tv.h>

#include <string>
#include <vector>

struct Span { size_t start; size_t end; }; // [start, end)

class FrameFilePlayerView : public TView {
public:
    explicit FrameFilePlayerView(const TRect &bounds, const std::string &path, unsigned periodMs = 300);
    virtual ~FrameFilePlayerView();

    // TView overrides
    virtual void draw() override;
    virtual void handleEvent(TEvent &ev) override;
    virtual void setState(ushort aState, Boolean enable) override;

    bool ok() const { return loadOk; }
    const std::string &error() const { return errorMsg; }

private:
    // Data
    std::string fileData;
    std::vector<Span> frames;
    size_t frameIndex {0};
    TTimerId timerId {0};
    unsigned periodMs {300};
    bool loadOk {false};
    std::string errorMsg;

    // Helpers
    void startTimer();
    void stopTimer();
    void advanceFrame();
    void loadAndIndex(const std::string &path);
    static size_t nextLineStart(const std::string &s, size_t pos);
    static size_t findLineEnd(const std::string &s, size_t pos, size_t limit);
};

#endif // FRAME_FILE_PLAYER_VIEW_H
