#ifndef FONT_H
#define FONT_H

#include <X11/Xlib.h>

struct _FcitxLightUI;

void CreateFont(struct _FcitxLightUI* lightui);
void OutputString (Display* dpy, Drawable window, XFontSet font, char *str, int x, int y, GC gc);
int StringWidth (char *str, XFontSet font);
int FontHeight (XFontSet font);

#endif