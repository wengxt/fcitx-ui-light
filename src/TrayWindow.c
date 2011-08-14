/***************************************************************************
 *   Copyright (C) 2002~2010 by Yuking                                     *
 *   yuking_net@sohu.com                                                   *
 *   Copyright (C) 2010~2010 by CSSlayer                                   *
 *   wengxt@gmail.com                                                      *
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

#include "fcitx/fcitx.h"

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xmd.h>
#include <X11/xpm.h>
#include <X11/extensions/Xrender.h>

#include "TrayWindow.h"
#include "tray.h"
#include "skin.h"
#include "lightui.h"
#include "fcitx/module/x11/x11stuff.h"
#include "fcitx-utils/log.h"
#include "fcitx/frontend.h"
#include "fcitx/module.h"
#include "MenuWindow.h"
#include "fcitx/instance.h"
#include <fcitx-utils/utils.h>
static boolean TrayEventHandler(void *arg, XEvent* event);

void InitTrayWindow(TrayWindow *trayWindow)
{
    FcitxLightUI *lightui = trayWindow->owner;
    Display *dpy = lightui->dpy;
    int iScreen = lightui->iScreen;
    char   strWindowName[]="Fcitx Tray Window";
    if ( !lightui->bUseTrayIcon )
        return;

    InitTray(dpy, trayWindow);

    XVisualInfo* vi = TrayGetVisual(dpy, trayWindow);
    if (vi && vi->visual) {
        Window p = DefaultRootWindow (dpy);
        Colormap colormap = XCreateColormap(dpy, p, vi->visual, AllocNone);
        XSetWindowAttributes wsa;
        wsa.background_pixmap = 0;
        wsa.colormap = colormap;
        wsa.background_pixel = 0;
        wsa.border_pixel = 0;
        trayWindow->window = XCreateWindow(dpy, p, -1, -1, 1, 1,
                                           0, vi->depth, InputOutput, vi->visual,
                                           CWBackPixmap|CWBackPixel|CWBorderPixel|CWColormap, &wsa);
    }
    else {
        trayWindow->window = XCreateSimpleWindow (dpy, DefaultRootWindow(dpy),
                             -1, -1, 1, 1, 0,
                             BlackPixel (dpy, DefaultScreen (dpy)),
                             WhitePixel (dpy, DefaultScreen (dpy)));
        XSetWindowBackgroundPixmap(dpy, trayWindow->window, ParentRelative);
    }
    if (trayWindow->window == (Window) NULL)
        return;

    XSizeHints size_hints;
    size_hints.flags = PWinGravity | PBaseSize;
    size_hints.base_width = trayWindow->size;
    size_hints.base_height = trayWindow->size;
    XSetWMNormalHints(dpy, trayWindow->window, &size_hints);


    XSelectInput (dpy, trayWindow->window, ExposureMask | KeyPressMask |
                  ButtonPressMask | ButtonReleaseMask | StructureNotifyMask
                  | EnterWindowMask | PointerMotionMask | LeaveWindowMask | VisibilityChangeMask);

    LightUISetWindowProperty(lightui, trayWindow->window, FCITX_WINDOW_DOCK, strWindowName);

    TrayFindDock(dpy, trayWindow);
}

TrayWindow* CreateTrayWindow(FcitxLightUI *lightui) {
    TrayWindow *trayWindow = fcitx_malloc0(sizeof(TrayWindow));
    trayWindow->owner = lightui;
    FcitxModuleFunctionArg arg;
    arg.args[0] = TrayEventHandler;
    arg.args[1] = trayWindow;
    InvokeFunction(lightui->owner, FCITX_X11, ADDXEVENTHANDLER, arg);
    InitTrayWindow(trayWindow);
    return trayWindow;
}

void ReleaseTrayWindow(TrayWindow *trayWindow)
{
    FcitxLightUI *lightui = trayWindow->owner;
    Display *dpy = lightui->dpy;
    if (trayWindow->window == None)
        return;
    XDestroyWindow(dpy, trayWindow->window);
    trayWindow->window = None;
}

void DrawTrayWindow(TrayWindow* trayWindow) {
    FcitxLightUI *lightui = trayWindow->owner;
    Display *dpy = lightui->dpy;
    char *name = NULL;
    XImage* tray = NULL;
    XImage* mask = NULL;

    int f_state;
    if ( !lightui->bUseTrayIcon )
        return;

    if (GetCurrentState(lightui->owner) == IS_ACTIVE)
        name = "tray_active";
    else
        name = "tray_inactive";

    LightUIImage* image = LoadImage(lightui, name);
    if (image)
    {
        DrawImage(dpy, trayWindow->window, image, 0, 0, trayWindow->size, trayWindow->size);
    }

    if (!trayWindow->bTrayMapped)
        return;

}

boolean TrayEventHandler(void *arg, XEvent* event)
{
    TrayWindow *trayWindow = arg;
    FcitxLightUI *lightui = trayWindow->owner;
    FcitxInstance* instance = lightui->owner;
    Display *dpy = lightui->dpy;
    if (!lightui->bUseTrayIcon)
        return false;
    switch (event->type) {
    case ClientMessage:
        if (event->xclient.message_type == trayWindow->atoms[ATOM_MANAGER]
                && event->xclient.data.l[1] == trayWindow->atoms[ATOM_SELECTION])
        {
            if (trayWindow->window == None)
                InitTrayWindow(trayWindow);
            TrayFindDock(dpy, trayWindow);
            return true;
        }
        break;

    case Expose:
        if (event->xexpose.window == trayWindow->window) {
            DrawTrayWindow (trayWindow);
        }
        break;
    case ConfigureNotify:
        if (trayWindow->window == event->xconfigure.window)
        {
            int size = event->xconfigure.height;
            if (size != trayWindow->size)
            {
                trayWindow->size = size;
                XSizeHints size_hints;
                size_hints.flags = PWinGravity | PBaseSize;
                size_hints.base_width = trayWindow->size;
                size_hints.base_height = trayWindow->size;
                XSetWMNormalHints(dpy, trayWindow->window, &size_hints);
            }

            DrawTrayWindow (trayWindow);
            return true;
        }
        break;
    case ButtonPress:
    {
        if (event->xbutton.window == trayWindow->window)
        {
            switch (event->xbutton.button)
            {
            case Button1:
                if (GetCurrentState(instance) == IS_CLOSED) {
                    EnableIM(instance, GetCurrentIC(instance), false);
                }
                else {
                    CloseIM(instance, GetCurrentIC(instance));
                }
                break;
            case Button3:
            {
                XlibMenu *mainMenuWindow = lightui->mainMenuWindow;
                int dwidth, dheight;
                GetScreenSize(lightui, &dwidth, &dheight);
                GetMenuSize(mainMenuWindow);
                if (event->xbutton.x_root - event->xbutton.x +
                        mainMenuWindow->width >= dwidth)
                    mainMenuWindow->iPosX = dwidth - mainMenuWindow->width - event->xbutton.x;
                else
                    mainMenuWindow->iPosX =
                        event->xbutton.x_root - event->xbutton.x;

                // 面板的高度是可以变动的，需要取得准确的面板高度，才能准确确定右键菜单位置。
                if (event->xbutton.y_root + mainMenuWindow->height -
                        event->xbutton.y >= dheight)
                    mainMenuWindow->iPosY =
                        dheight - mainMenuWindow->height -
                        event->xbutton.y - 15;
                else
                    mainMenuWindow->iPosY = event->xbutton.y_root - event->xbutton.y + 25;     // +sc.skin_tray_icon.active_img.height;

                DrawXlibMenu(mainMenuWindow);
                DisplayXlibMenu(mainMenuWindow);
            }
            break;
            }
            return true;
        }
    }
    break;
    case DestroyNotify:
        if (event->xdestroywindow.window == trayWindow->dockWindow)
        {
            trayWindow->dockWindow = None;
            trayWindow->bTrayMapped = False;
            ReleaseTrayWindow(trayWindow);
            return true;
        }
        break;

    case ReparentNotify:
        if (event->xreparent.parent == DefaultRootWindow(dpy) && event->xreparent.window == trayWindow->window)
        {
            trayWindow->bTrayMapped = False;
            ReleaseTrayWindow(trayWindow);
            return true;
        }
        break;
    }
    return false;
}
// kate: indent-mode cstyle; space-indent on; indent-width 0;
