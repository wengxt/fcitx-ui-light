#include <X11/Xlib.h>
#include <string.h>
#include <locale.h>
#include <fontconfig/fontconfig.h>
#include <fcitx-utils/log.h>
#include <libintl.h>
#include <X11/Xft/Xft.h>
#include "lightui.h"
#include "font.h"

/**
 * @brief Get Usable Font
 *
 * @param strUserLocale font language
 * @param font input as a malloc-ed font name, out put as new malloc-ed font name.
 * @return void
 **/
void GetValidFont(const char* strUserLocale, char **font)
{
    FcFontSet   *fs = NULL;
    FcPattern   *pat = NULL;
    FcObjectSet *os = NULL;

    if (!FcInit())
    {
        FcitxLog(ERROR, _("Error: Load fontconfig failed"));
        return;
    }
    char locale[3];

    if (strUserLocale)
        strncpy(locale, strUserLocale, 2);
    else
        strcpy(locale, "zh");
    locale[2]='\0';
reloadfont:
    if (strcmp(*font, "") == 0)
    {
        FcChar8 strpat[9];
        sprintf((char*)strpat, ":lang=%s", locale);
        pat = FcNameParse(strpat);
    }
    else
    {
        pat = FcNameParse ((FcChar8*)(*font));
    }

    os = FcObjectSetBuild(FC_FAMILY, FC_STYLE, (char*)0);
    fs = FcFontList(0, pat, os);
    if (os)
        FcObjectSetDestroy(os);
    os = NULL;

    FcPatternDestroy(pat);
    pat = NULL;

    if (!fs || fs->nfont <= 0)
        goto nofont;

    FcChar8* family;
    if (FcPatternGetString (fs->fonts[0], FC_FAMILY, 0, &family) != FcResultMatch)
        goto nofont;
    if (*font)
        free(*font);

    *font = strdup((const char*) family);

    FcFontSetDestroy(fs);

    FcitxLog(INFO, _("your current font is: %s"), *font);
    return;

nofont:
    if (strcmp(*font, "") != 0)
    {
        strcpy(*font, "");
        if (pat)
            FcPatternDestroy(pat);
        if (os)
            FcObjectSetDestroy(os);
        if (fs)
            FcFontSetDestroy(fs);

        goto reloadfont;
    }

    FcitxLog(FATAL, _("no valid font."));
    return;
}

void CreateFont (FcitxLightUI* lightui)
{
    GetValidFont(lightui->strUserLocale, &lightui->font);

    if (lightui->xftfont)
        XftFontClose (lightui->dpy, lightui->xftfont);

    lightui->xftfont = XftFontOpen (lightui->dpy, lightui->iScreen, XFT_FAMILY, XftTypeString, lightui->font, XFT_SIZE, XftTypeDouble, (double) lightui->fontSize, XFT_ANTIALIAS, XftTypeBool, True, NULL);

}

void OutputString (Display* dpy, XftDraw* xftDraw, Drawable window, XftFont* font, char *str, int x, int y, ConfigColor color)
{
    if (!font || !str)
        return;

    y += FontHeight(dpy, font);

    XftColor        xftColor;
    XRenderColor    renderColor;

    if (!font || !str)
        return;

    renderColor.red = color.r * 65535;
    renderColor.green = color.g * 65535;
    renderColor.blue = color.b * 65535;
    renderColor.alpha = 0xFFFF;

    XftColorAllocValue (dpy, DefaultVisual (dpy, DefaultScreen (dpy)), DefaultColormap (dpy, DefaultScreen (dpy)), &renderColor, &xftColor);
    XftDrawChange (xftDraw, window);
    XftDrawStringUtf8 (xftDraw, &xftColor, font, x, y, (FcChar8 *) str, strlen (str));

    XftColorFree (dpy, DefaultVisual (dpy, DefaultScreen (dpy)), DefaultColormap (dpy, DefaultScreen (dpy)), &xftColor);
}


int StringWidth (Display* dpy, XftFont * font, char *str)
{
    XGlyphInfo      extents;

    if (!font || !str)
        return 0;

    XftTextExtentsUtf8 (dpy, font, (FcChar8 *) str, strlen (str), &extents);

    return extents.xOff;
}

int FontHeight (Display* dpy, XftFont * font)
{
    XGlyphInfo      extents;
    char            str[] = "Ay\xe4\xb8\xad";

    if (!font)
        return 0;

    XftTextExtentsUtf8 (dpy, font, (FcChar8 *) str, strlen (str), &extents);

    return extents.height;
}
