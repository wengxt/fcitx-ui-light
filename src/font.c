#include <X11/Xlib.h>
#include <string.h>
#include <locale.h>
#include "lightui.h"


void CreateFont (FcitxLightUI* lightui)
{
    char          **missing_charsets;
    int             num_missing_charsets = 0;
    char           *default_string;
    char            strFont[256];
    if (lightui->strUserLocale[0])
        setlocale (LC_CTYPE, lightui->strUserLocale);

    if (lightui->fontset)
        XFreeFontSet (lightui->dpy, lightui->fontset);

    int iFontSize = 16;
    sprintf (strFont, "-*-*-medium-r-normal--%d-*-*-*-*-*-*-*,-*-*-medium-r-normal--%d-*-*-*-*-*-*-*", iFontSize, iFontSize);
    lightui->fontset = XCreateFontSet (lightui->dpy, strFont, &missing_charsets, &num_missing_charsets, &default_string);
    if (num_missing_charsets > 0)
        fprintf (stderr, "Error: Cannot Create Chinese Fonts!\n\n");
    setlocale (LC_CTYPE, "");
}

void OutputString (Display* dpy, Drawable window, XFontSet font, char *str, int x, int y, GC gc)
{
    if (!font || !str)
        return;

    y += FontHeight(font);

    Xutf8DrawString (dpy, window, font, gc, x, y, str, strlen (str));
}


int StringWidth (char *str, XFontSet font)
{
    XRectangle      InkBox, LogicalBox;

    if (!font || !str)
        return 0;

    Xutf8TextExtents (font, str, strlen (str), &InkBox, &LogicalBox);

    return LogicalBox.width;
}

int FontHeight (XFontSet font)
{
    XRectangle      InkBox, LogicalBox;
    char            str[] = "Ay\xe4\xb8\xad";

    if (!font)
        return 0;

    Xutf8TextExtents (font, str, strlen (str), &InkBox, &LogicalBox);

    return LogicalBox.height;
}
