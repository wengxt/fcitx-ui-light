/***************************************************************************
 *   Copyright (C) 2002~2005 by Yuking                                     *
 *   yuking_net@sohu.com                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef _LIGHT_UI_H
#define _LIGHT_UI_H

#include "config.h"

#include "fcitx/fcitx.h"
#include "fcitx/ui.h"
#include "fcitx-config/fcitx-config.h"
#include "fcitx-utils/utarray.h"

#include <X11/Xlib.h>

#ifdef _ENABLE_PANGO
#include <pango/pangocairo.h>
#endif

#include "skin.h"
#include <fcitx/module/x11/x11stuff.h>
#include <X11/Xft/Xft.h>

#define FCITX_LIGHT_UI_NAME "fcitx-light-ui"

#define _(x) dgettext("fcitx-light-ui", (x))

struct _MainWindow;
struct _AboutWindow;
struct _FcitxLightUIStatus;

typedef enum _HIDE_MAINWINDOW {
    HM_SHOW = 0,
    HM_AUTO = 1,
    HM_HIDE = 2
} HIDE_MAINWINDOW;

/**
 * @brief Config and Global State for Light UI
 **/
typedef struct _FcitxLightUI {
    GenericConfig gconfig;
    Display* dpy;
    int iScreen;
    Atom protocolAtom;
    Atom killAtom;
    struct _InputWindow* inputWindow;
    struct _MainWindow* mainWindow;
    struct _MessageWindow* messageWindow;
    struct _TrayWindow* trayWindow;
    struct _AboutWindow* aboutWindow;

    struct _FcitxInstance *owner;

    char* font;
    char* strUserLocale;
    int fontSize;
    boolean bUseTrayIcon;
    boolean bUseTrayIcon_;
    HIDE_MAINWINDOW hideMainWindow;
    boolean bVerticalList;
    char* skinType;
    int iMainWindowOffsetX;
    int iMainWindowOffsetY;
    ConfigColor backcolor;
    ConfigColor bordercolor;
    ConfigColor fontColor[MESSAGE_TYPE_COUNT];
    ConfigColor menuFontColor[2];
    ConfigColor activeColor;
    ConfigColor lineColor;
    ConfigColor cursorColor;

    UT_array status;
    struct _XlibMenu* mainMenuWindow;
    FcitxUIMenu mainMenu;
    XftFont* xftfont;
    LightUIImage* imageTable;
} FcitxLightUI;

void GetScreenSize(FcitxLightUI* lightui, int* width, int* height);
void
LightUIInitWindowAttribute(FcitxLightUI* lightui, Visual ** vs, Colormap * cmap,
                             XSetWindowAttributes * attrib,
                             unsigned long *attribmask, int *depth);
Visual * LightUIFindARGBVisual (FcitxLightUI* lightui);
boolean LightUIMouseClick(FcitxLightUI* lightui, Window window, int *x, int *y);
boolean IsInRspArea(int x0, int y0, struct _FcitxLightUIStatus* status);
void LightUISetWindowProperty(FcitxLightUI* lightui, Window window, FcitxXWindowType type, char *windowTitle);
void ActivateWindow(Display *dpy, int iScreen, Window window);
boolean LoadLightUIConfig(FcitxLightUI* lightui);
void SaveLightUIConfig(FcitxLightUI* lightui);
boolean WindowIsVisable(Display* dpy, Window window);
GC LightUICreateGC(Display* dpy, Drawable drawable, ConfigColor color);
void LightUISetGC(Display* dpy, GC gc, ConfigColor color);

#define GetPrivateStatus(status) ((FcitxLightUIStatus*)(status)->priv)

CONFIG_BINDING_DECLARE(FcitxLightUI);
#endif

// kate: indent-mode cstyle; space-indent on; indent-width 0;
