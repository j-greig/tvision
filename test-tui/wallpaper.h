/*---------------------------------------------------------*/
/*                                                         */
/*   wallpaper.h - Desktop Wallpaper Component            */
/*   WIBWOBWORLD ASCII Art Background                     */
/*                                                         */
/*---------------------------------------------------------*/

#ifndef WALLPAPER_H
#define WALLPAPER_H

#define Uses_TBackground
#define Uses_TRect
#define Uses_TDrawBuffer
#include <tvision/tv.h>

/*---------------------------------------------------------*/
/* TWallpaperView - Custom desktop background with art    */
/*---------------------------------------------------------*/
class TWallpaperView : public TBackground
{
public:
    TWallpaperView(const TRect& bounds);
    virtual void draw();
    virtual TPalette& getPalette() const;
    
private:
    static const char* asciiArt[];
    static const int artWidth;
    static const int artHeight;
    
    void drawCenteredArt(TDrawBuffer& b, int y);
    TAttrPair getArtColor();
    TAttrPair getBgColor();
};

#endif // WALLPAPER_H