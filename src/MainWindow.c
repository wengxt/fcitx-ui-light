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
/**
 * @file   MainWindow.c
 * @author Yuking yuking_net@sohu.com
 * @date   2008-1-16
 *
 * @brief  主窗口
 *
 *
 */

#include <X11/Xutil.h>
#include <string.h>
#include <X11/Xatom.h>
#include <limits.h>
#include <libintl.h>

#include "fcitx/frontend.h"
#include "fcitx/module.h"
#include "fcitx/instance.h"

#include "MainWindow.h"
#include "fcitx-utils/log.h"
#include "fcitx/module/x11/x11stuff.h"
#include "lightui.h"
#include "skin.h"
#include "MenuWindow.h"
#include <fcitx-utils/utils.h>

#define FCITX_MAX(a,b) ((a) > (b)?(a) : (b))

#define MarginTop 1
#define MarginRight 1
#define MarginLeft 1
#define MarginBottom 1
#define ICON_SIZE 16
#define ICON_HEIGHT ICON_SIZE
#define ICON_WIDTH ICON_SIZE

#define MAIN_BAR_MAX_WIDTH 400
#define MAIN_BAR_MAX_HEIGHT 400

static boolean MainWindowEventHandler(void *arg, XEvent* event);
static void UpdateStatusGeometry(FcitxLightUIStatus *privstat, int x, int y);
static void ReloadMainWindow(void* arg, boolean enabled);
static void InitMainWindow(MainWindow* mainWindow);

void InitMainWindow(MainWindow* mainWindow)
{
    FcitxLightUI* lightui = mainWindow->owner;
    int depth;
    Colormap cmap;
    Visual * vs;
    XSetWindowAttributes attrib;
    unsigned long   attribmask;
    GC gc;
    char        strWindowName[] = "Fcitx Main Window";
    int swidth, sheight;
    XGCValues xgv;
    Display* dpy = lightui->dpy;
    int iScreen = lightui->iScreen;
    mainWindow->dpy = dpy;

    GetScreenSize(lightui, &swidth, &sheight);

    vs = LightUIFindARGBVisual(lightui);

    LightUIInitWindowAttribute(lightui, &vs, &cmap, &attrib, &attribmask, &depth);
    mainWindow->window=XCreateWindow (dpy,
                                      RootWindow(dpy, iScreen),
                                      lightui->iMainWindowOffsetX,
                                      lightui->iMainWindowOffsetY,
                                      100,
                                      100,
                                      0, depth,InputOutput, vs,attribmask, &attrib);

    if (mainWindow->window == None)
        return;

    xgv.foreground = WhitePixel(dpy, iScreen);
    mainWindow->pm_main_bar = XCreatePixmap(
                                  dpy,
                                  mainWindow->window,
                                  MAIN_BAR_MAX_WIDTH,
                                  MAIN_BAR_MAX_HEIGHT,
                                  depth);
    gc = XCreateGC(dpy,mainWindow->pm_main_bar, GCForeground, &xgv);
    XFillRectangle(dpy, mainWindow->pm_main_bar, gc, 0, 0, 100, 100);
    XFreeGC(dpy,gc);

    mainWindow->main_win_gc = XCreateGC( dpy, mainWindow->window, 0, NULL );
    XChangeWindowAttributes (dpy, mainWindow->window, attribmask, &attrib);
    XSelectInput (dpy, mainWindow->window, ExposureMask | ButtonPressMask | ButtonReleaseMask  | PointerMotionMask | LeaveWindowMask);

    LightUISetWindowProperty(lightui, mainWindow-> window, FCITX_WINDOW_DOCK, strWindowName);
}

MainWindow* CreateMainWindow (FcitxLightUI* lightui)
{
    MainWindow *mainWindow;

    mainWindow = fcitx_malloc0(sizeof(MainWindow));
    mainWindow->owner = lightui;
    InitMainWindow(mainWindow);

    FcitxModuleFunctionArg arg;
    arg.args[0] = MainWindowEventHandler;
    arg.args[1] = mainWindow;
    InvokeFunction(lightui->owner, FCITX_X11, ADDXEVENTHANDLER, arg);

    arg.args[0] = ReloadMainWindow;
    arg.args[1] = mainWindow;
    InvokeFunction(lightui->owner, FCITX_X11, ADDCOMPOSITEHANDLER, arg);
    return mainWindow;
}

void DisplayMainWindow (Display* dpy, MainWindow* mainWindow)
{
    FcitxLog(DEBUG, _("DISPLAY MainWindow"));

    if (!mainWindow->bMainWindowHidden)
        XMapRaised (dpy, mainWindow->window);
}

void DrawMainWindow (MainWindow* mainWindow)
{
    FcitxInstance *instance = mainWindow->owner->owner;
    FcitxLightUI* lightui = mainWindow->owner;
    char path[PATH_MAX];

    if ( mainWindow->bMainWindowHidden )
        return;

    FcitxLog(DEBUG, _("DRAW MainWindow"));

    if (mainWindow->owner->hideMainWindow == HM_SHOW || (mainWindow->owner->hideMainWindow == HM_AUTO && (GetCurrentState(mainWindow->owner->owner) != IS_CLOSED)))
    {
            /* Only logo and input status is hard-code, other should be status */
            int currentX = MarginLeft;
            int height = 0;
            LightUIImage* logo = LoadImage(lightui, "logo");
            LightUIImage* imicon;
            int imageheight;
            if (logo)
            {
                currentX += ICON_WIDTH;
                imageheight = ICON_HEIGHT;
                if (imageheight > height)
                    height = imageheight;
            }

            if (GetCurrentState(instance) != IS_ACTIVE )
                imicon = LoadImage(lightui, "eng");
            else
            {
                FcitxIM* im = GetCurrentIM(instance);
                imicon = LoadImage(lightui, im->strIconName);
                if (imicon == NULL)
                    imicon = LoadImage(lightui, "active");
            }
            currentX += ICON_WIDTH;
            imageheight = ICON_HEIGHT;
            if (imageheight > height)
                height = imageheight;

            FcitxUIStatus* status;
            for (status = (FcitxUIStatus*) utarray_front(&instance->uistats);
                    status != NULL;
                    status = (FcitxUIStatus*) utarray_next(&instance->uistats, status)
                )
            {
                boolean active = status->getCurrentStatus(status->arg);
                sprintf(path, "%s_%s", status->name, active ? "active": "inactive");
                LightUIImage* statusicon = LoadImage(lightui, path);
                if (statusicon == NULL)
                    continue;
                currentX += ICON_WIDTH;
                imageheight = ICON_HEIGHT;
                if (imageheight > height)
                    height = imageheight;
            }

            int width = currentX + MarginRight;
            height += MarginTop + MarginBottom;

            XResizeWindow(mainWindow->dpy, mainWindow->window, width, height);
            DrawResizableBackground(lightui, mainWindow->pm_main_bar, height, width, lightui->backcolor, lightui->bordercolor, mainWindow->main_win_gc);

            currentX = MarginLeft;
            if (logo)
            {
                DrawImage(mainWindow->dpy, mainWindow->pm_main_bar, logo, currentX, MarginTop, ICON_WIDTH, ICON_HEIGHT);
                UpdateStatusGeometry( &mainWindow->logostat, currentX, MarginTop);
                currentX += ICON_WIDTH;
            }
            DrawImage(mainWindow->dpy, mainWindow->pm_main_bar, imicon, currentX, MarginTop, ICON_WIDTH, ICON_HEIGHT);
            UpdateStatusGeometry( &mainWindow->imiconstat, currentX, MarginTop);
            currentX += ICON_WIDTH;

            for (status = (FcitxUIStatus*) utarray_front(&instance->uistats);
                    status != NULL;
                    status = (FcitxUIStatus*) utarray_next(&instance->uistats, status)
                )
            {
                FcitxLightUIStatus* privstat = GetPrivateStatus(status);
                if (privstat == NULL)
                    continue;
                /* reset status */
                privstat->x = privstat->y = -1;
                privstat->w = privstat->h = 0;
                boolean active = status->getCurrentStatus(status->arg);
                sprintf(path, "%s_%s", status->name, active ? "active": "inactive");
                LightUIImage* statusicon = LoadImage(lightui, path);
                if (statusicon == NULL)
                    continue;
                DrawImage(mainWindow->dpy, mainWindow->pm_main_bar, statusicon, currentX, MarginTop, ICON_WIDTH, ICON_HEIGHT);
                UpdateStatusGeometry(privstat, currentX, MarginTop);
                currentX += ICON_WIDTH;
            }

            XCopyArea (mainWindow->dpy, mainWindow->pm_main_bar, mainWindow->window, mainWindow->main_win_gc, 0, 0, width,
                       height, 0, 0);
    }
    else
        XUnmapWindow (mainWindow->dpy, mainWindow->window);

    FcitxLog(DEBUG, _("DRAW MainWindow"));
}

void ReloadMainWindow(void *arg, boolean enabled)
{
    MainWindow* mainWindow = (MainWindow*) arg;
    boolean visable = WindowIsVisable(mainWindow->dpy, mainWindow->window);
    XFreePixmap(mainWindow->dpy, mainWindow->pm_main_bar);
    XFreeGC(mainWindow->dpy, mainWindow->main_win_gc);
    XDestroyWindow(mainWindow->dpy, mainWindow->window);

    mainWindow->pm_main_bar = None;
    mainWindow->main_win_gc = NULL;
    mainWindow->window = None;

    InitMainWindow(mainWindow);

    if (visable)
        ShowMainWindow(mainWindow);
}

void UpdateStatusGeometry(FcitxLightUIStatus *privstat, int x, int y)
{
    privstat->x = x;
    privstat->y = y;
    privstat->w = ICON_WIDTH;
    privstat->h = ICON_HEIGHT;
}

void ShowMainWindow(MainWindow* mainWindow)
{
    XMapRaised (mainWindow->dpy, mainWindow->window);
}

void CloseMainWindow(MainWindow *mainWindow)
{
    XUnmapWindow (mainWindow->dpy, mainWindow->window);
}

boolean MainWindowEventHandler(void *arg, XEvent* event)
{
    MainWindow* mainWindow = arg;
    FcitxInstance *instance = mainWindow->owner->owner;
    FcitxLightUI *lightui = mainWindow->owner;

    if (event->xany.window == mainWindow->window)
    {
        switch (event->type)
        {
        case Expose:
            DrawMainWindow(mainWindow);
            break;
        case MotionNotify:
            break;
        case LeaveNotify:
            break;
        case ButtonPress:
            switch (event->xbutton.button) {
            case Button1:
            {
                if (IsInRspArea(event->xbutton.x, event->xbutton.y, &mainWindow->logostat)) {
                    lightui->iMainWindowOffsetX = event->xbutton.x;
                    lightui->iMainWindowOffsetY = event->xbutton.y;

                    if (!LightUIMouseClick(mainWindow->owner, mainWindow->window, &lightui->iMainWindowOffsetX, &lightui->iMainWindowOffsetY))
                    {
                        if (GetCurrentState(instance) == IS_CLOSED) {
                            EnableIM(instance, GetCurrentIC(instance), false);
                        }
                        else {
                            CloseIM(instance, GetCurrentIC(instance));
                        }
                    }
                    SaveLightUIConfig(lightui);
                } else if (IsInRspArea(event->xbutton.x, event->xbutton.y, &mainWindow->imiconstat)) {
                    SwitchIM(instance, -1);
                } else {
                    FcitxUIStatus *status;
                    for (status = (FcitxUIStatus*) utarray_front(&instance->uistats);
                            status != NULL;
                            status = (FcitxUIStatus*) utarray_next(&instance->uistats, status)
                        )
                    {
                        FcitxLightUIStatus* privstat = GetPrivateStatus(status);
                        if (IsInRspArea(event->xbutton.x, event->xbutton.y, privstat))
                        {
                            UpdateStatus(instance, status->name);
                        }
                    }
                }
            }
            break;
            case Button3:
            {
                XlibMenu *mainMenuWindow = lightui->mainMenuWindow;
                unsigned int height;
                int sheight;
                XWindowAttributes attr;
                GetMenuSize(mainMenuWindow);
                GetScreenSize(lightui, NULL, &sheight);
                XGetWindowAttributes(lightui->dpy, mainWindow->window, &attr);
                height = attr.height;

                mainMenuWindow->iPosX = lightui->iMainWindowOffsetX;
                mainMenuWindow->iPosY =
                    lightui->iMainWindowOffsetY +
                    height;
                if ((mainMenuWindow->iPosY + mainMenuWindow->height) >
                        sheight)
                    mainMenuWindow->iPosY = lightui->iMainWindowOffsetY - 5 - mainMenuWindow->height;

                DrawXlibMenu(mainMenuWindow);
                DisplayXlibMenu(mainMenuWindow);

            }
            break;

            }
            break;
        case ButtonRelease:
            switch (event->xbutton.button) {
            case Button1:
                break;
            case Button2:
                break;
            }
            break;
        }
        return true;
    }
    return false;
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
