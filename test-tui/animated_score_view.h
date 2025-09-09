/*---------------------------------------------------------*/
/*                                                         */
/*   animated_score_view.h - Animated ASCII “Score” View  */
/*                                                         */
/*   A timer-driven view that renders a multi-line         */
/*   Unicode ASCII score with subtle, musical-like         */
/*   animation: phase shifts, breathing, drift, and        */
/*   cyclic glyph changes.                                 */
/*                                                         */
/*   Implementation borrows patterns from                   */
/*   animated_blocks_view and animated_gradient_view:      */
/*   - UI-thread timer via setTimer/killTimer              */
/*   - cmTimerExpired broadcast handling                   */
/*   - draw() composes each frame based on phase           */
/*                                                         */
/*---------------------------------------------------------*/

#ifndef ANIMATED_SCORE_VIEW_H
#define ANIMATED_SCORE_VIEW_H

#define Uses_TView
#define Uses_TRect
#define Uses_TDrawBuffer
#define Uses_TEvent
#define Uses_TColorAttr
#include <tvision/tv.h>

class TAnimatedScoreView : public TView {
public:
    explicit TAnimatedScoreView(const TRect &bounds, unsigned periodMs = 120);
    virtual ~TAnimatedScoreView();

    virtual void draw() override;
    virtual void handleEvent(TEvent &ev) override;
    virtual void setState(ushort aState, Boolean enable) override;
    virtual void changeBounds(const TRect& bounds) override;

    void setSpeed(unsigned periodMs_);

private:
    void startTimer();
    void stopTimer();
    void advance();

    unsigned periodMs;
    TTimerId timerId {0};
    int phase {0};
};

class TWindow; TWindow* createAnimatedScoreWindow(const TRect &bounds);

#endif // ANIMATED_SCORE_VIEW_H

