#ifndef FONT_H
#define FONT_H

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

struct _FcitxLightUI;

void GetValidFont(const char* strUserLocale, char **font);
void CreateFont(struct _FcitxLightUI* lightui);
void OutputString (Display* dpy, XftDraw* xftDraw, Drawable window, XftFont* font, char* str, int x, int y, ConfigColor color);
int StringWidth (Display* dpy, XftFont* font, char* str);
int FontHeight (Display* dpy, XftFont* font);

#endif