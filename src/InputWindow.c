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

#include <string.h>
#include <stdlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <libintl.h>

#include "fcitx/ui.h"
#include "fcitx/module.h"
#include "fcitx/profile.h"
#include "fcitx/frontend.h"
#include "fcitx/configfile.h"
#include "fcitx/instance.h"
#include "fcitx-utils/utils.h"

#include "InputWindow.h"
#include "lightui.h"
#include "skin.h"
#include <fcitx/module/x11/x11stuff.h>
#include "MainWindow.h"
#include <fcitx-utils/log.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>

static boolean InputWindowEventHandler(void *arg, XEvent* event);
static void InitInputWindow(InputWindow* inputWindow);
static void ReloadInputWindow(void* arg, boolean enabled);

void InitInputWindow(InputWindow* inputWindow)
{
    XSetWindowAttributes    attrib;
    unsigned long   attribmask;
    char        strWindowName[]="Fcitx Input Window";
    int depth;
    Colormap cmap;
    Visual * vs;
    FcitxLightUI* lightui = inputWindow->owner;
    int iScreen = lightui->iScreen;
    Display* dpy = lightui->dpy;
    inputWindow->window = None;
    inputWindow->iInputWindowHeight = INPUTWND_HEIGHT;
    inputWindow->iInputWindowWidth = INPUTWND_WIDTH;
    inputWindow->iOffsetX = 0;
    inputWindow->iOffsetY = 8;
    inputWindow->dpy = dpy;
    inputWindow->iScreen = iScreen;

    inputWindow->iInputWindowHeight= INPUTWND_HEIGHT;
    vs= NULL;
    LightUIInitWindowAttribute(lightui, &vs, &cmap, &attrib, &attribmask, &depth);

    inputWindow->window=XCreateWindow (dpy,
                                       RootWindow(dpy, iScreen),
                                       lightui->iMainWindowOffsetX,
                                       lightui->iMainWindowOffsetY,
                                       inputWindow->iInputWindowWidth,
                                       inputWindow->iInputWindowHeight,
                                       0,
                                       depth,InputOutput,
                                       vs,attribmask,
                                       &attrib);

    inputWindow->pixmap = XCreatePixmap(dpy,
                                 inputWindow->window,
                                 INPUT_BAR_MAX_WIDTH,
                                 INPUT_BAR_MAX_HEIGHT,
                                 depth);
    inputWindow->pixmap2 = XCreatePixmap(dpy,
                                 inputWindow->pixmap,
                                 INPUT_BAR_MAX_WIDTH,
                                 INPUT_BAR_MAX_HEIGHT,
                                 depth);

    XGCValues gcvalues;
    inputWindow->window_gc = XCreateGC(inputWindow->dpy, inputWindow->window, 0, &gcvalues);
    inputWindow->pixmap_gc = XCreateGC(inputWindow->dpy, inputWindow->pixmap, 0, &gcvalues);
    inputWindow->pixmap2_gc = XCreateGC(inputWindow->dpy, inputWindow->pixmap2, 0, &gcvalues);
    inputWindow->xftDraw = XftDrawCreate(inputWindow->dpy, inputWindow->pixmap, DefaultVisual (dpy, DefaultScreen (dpy)), DefaultColormap (dpy, DefaultScreen (dpy)));

    XSelectInput (dpy, inputWindow->window, ButtonPressMask | ButtonReleaseMask  | PointerMotionMask | ExposureMask);

    LightUISetWindowProperty(lightui, inputWindow->window, FCITX_WINDOW_DOCK, strWindowName);
}

InputWindow* CreateInputWindow(FcitxLightUI *lightui)
{
    InputWindow* inputWindow;

    inputWindow = fcitx_malloc0(sizeof(InputWindow));
    inputWindow->owner = lightui;
    InitInputWindow(inputWindow);

    FcitxModuleFunctionArg arg;
    arg.args[0] = InputWindowEventHandler;
    arg.args[1] = inputWindow;
    InvokeFunction(lightui->owner, FCITX_X11, ADDXEVENTHANDLER, arg);

    arg.args[0] = ReloadInputWindow;
    arg.args[1] = inputWindow;
    InvokeFunction(lightui->owner, FCITX_X11, ADDCOMPOSITEHANDLER, arg);

    inputWindow->msgUp = InitMessages();
    inputWindow->msgDown = InitMessages();
    return inputWindow;
}

boolean InputWindowEventHandler(void *arg, XEvent* event)
{
    InputWindow* inputWindow = arg;
    if (event->xany.window == inputWindow->window)
    {
        switch (event->type)
        {
        case Expose:
            DrawInputWindow(inputWindow);
            break;
        case ButtonPress:
            switch (event->xbutton.button) {
            case Button1:
            {
                int             x,
                y;
                x = event->xbutton.x;
                y = event->xbutton.y;
                LightUIMouseClick(inputWindow->owner, inputWindow->window, &x, &y);

                FcitxInputContext* ic = GetCurrentIC(inputWindow->owner->owner);

                if (ic)
                    SetWindowOffset(inputWindow->owner->owner, ic, x, y);

                DrawInputWindow(inputWindow);
            }
            break;
            }
            break;
        }
        return true;
    }
    return false;
}

void DisplayInputWindow (InputWindow* inputWindow)
{
    FcitxLog(DEBUG, _("DISPLAY InputWindow"));
    MoveInputWindowInternal(inputWindow);
    XMapRaised (inputWindow->dpy, inputWindow->window);
}

void DrawInputWindow(InputWindow* inputWindow)
{
    int lastW = inputWindow->iInputWindowWidth, lastH = inputWindow->iInputWindowHeight;
    int cursorPos = NewMessageToOldStyleMessage(inputWindow->owner->owner, inputWindow->msgUp, inputWindow->msgDown);
    DrawInputBar(inputWindow, cursorPos, inputWindow->msgUp, inputWindow->msgDown, &inputWindow->iInputWindowHeight ,&inputWindow->iInputWindowWidth);

    /* Resize Window will produce Expose Event, so there is no need to draw right now */
    if (lastW != inputWindow->iInputWindowWidth || lastH != inputWindow->iInputWindowHeight)
    {
        XResizeWindow(
            inputWindow->dpy,
            inputWindow->window,
            inputWindow->iInputWindowWidth,
            inputWindow->iInputWindowHeight);
        MoveInputWindowInternal(inputWindow);
    }

    XCopyArea(
            inputWindow->dpy,
            inputWindow->pixmap,
            inputWindow->window,
            inputWindow->window_gc,
            0, 0,
            inputWindow->iInputWindowWidth,
            inputWindow->iInputWindowHeight,
            0, 0
             );

    XFlush(inputWindow->dpy);
}

void MoveInputWindowInternal(InputWindow* inputWindow)
{
    int dwidth, dheight;
    int x = 0, y = 0;
    GetScreenSize(inputWindow->owner, &dwidth, &dheight);

    FcitxInputContext* ic = GetCurrentIC(inputWindow->owner->owner);
    GetWindowPosition(inputWindow->owner->owner, ic, &x, &y);

    int iTempInputWindowX, iTempInputWindowY;

    if (x < 0)
        iTempInputWindowX = 0;
    else
        iTempInputWindowX = x + inputWindow->iOffsetX;

    if (y < 0)
        iTempInputWindowY = 0;
    else
        iTempInputWindowY = y + inputWindow->iOffsetY;

    if ((iTempInputWindowX + inputWindow->iInputWindowWidth) > dwidth)
        iTempInputWindowX = dwidth - inputWindow->iInputWindowWidth;

    if ((iTempInputWindowY + inputWindow->iInputWindowHeight) > dheight) {
        if ( iTempInputWindowY > dheight )
            iTempInputWindowY = dheight - 2 * inputWindow->iInputWindowHeight;
        else
            iTempInputWindowY = iTempInputWindowY - 2 * inputWindow->iInputWindowHeight;
    }
    XMoveWindow (inputWindow->dpy, inputWindow->window, iTempInputWindowX, iTempInputWindowY);
}

void CloseInputWindowInternal(InputWindow* inputWindow)
{
    XUnmapWindow (inputWindow->dpy, inputWindow->window);
}

void ReloadInputWindow(void* arg, boolean enabled)
{
    InputWindow* inputWindow = (InputWindow*) arg;
    boolean visable = WindowIsVisable(inputWindow->dpy, inputWindow->window);

    XFreeGC(inputWindow->dpy, inputWindow->window_gc);
    XFreeGC(inputWindow->dpy, inputWindow->pixmap_gc);
    XFreeGC(inputWindow->dpy, inputWindow->pixmap2_gc);

    XFreePixmap(inputWindow->dpy, inputWindow->pixmap2);
    XFreePixmap(inputWindow->dpy, inputWindow->pixmap);

    XDestroyWindow(inputWindow->dpy, inputWindow->window);
    XftDrawDestroy(inputWindow->xftDraw);

    inputWindow->window = None;

    InitInputWindow(inputWindow);

    if (visable)
        ShowInputWindowInternal(inputWindow);
}

void ShowInputWindowInternal(InputWindow* inputWindow)
{
    XMapRaised(inputWindow->dpy, inputWindow->window);
    DrawInputWindow(inputWindow);
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
