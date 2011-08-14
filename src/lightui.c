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

#include <X11/Xutil.h>
#include <X11/extensions/Xrender.h>
#include <X11/Xatom.h>
#include <unistd.h>
#include <limits.h>
#include <libintl.h>
#include <errno.h>


#include "fcitx/fcitx.h"
#include "fcitx/ui.h"
#include "fcitx/module.h"
#include "fcitx/module/x11/x11stuff.h"

#include "lightui.h"
#include "fcitx-config/xdg.h"
#include "fcitx-utils/log.h"
#include "fcitx/instance.h"
#include "fcitx/frontend.h"
#include "InputWindow.h"
#include "MainWindow.h"
#include "TrayWindow.h"
#include "MenuWindow.h"
#include "MessageWindow.h"
#include "fcitx/hook.h"
#include <fcitx-utils/utils.h>

struct _FcitxSkin;
boolean MainMenuAction(FcitxUIMenu* menu, int index);

static void* LightUICreate(FcitxInstance* instance);
static void LightUICloseInputWindow(void* arg);
static void LightUIShowInputWindow(void* arg);
static void LightUIMoveInputWindow(void* arg);
static void LightUIRegisterMenu(void *arg, FcitxUIMenu* menu);
static void LightUIUpdateStatus(void *arg, FcitxUIStatus* status);
static void LightUIRegisterStatus(void *arg, FcitxUIStatus* status);
static void LightUIOnInputFocus(void *arg);
static void LightUIOnInputUnFocus(void *arg);
static void LightUIOnTriggerOn(void *arg);
static void LightUIOnTriggerOff(void *arg);
static void LightUIDisplayMessage(void *arg, char *title, char **msg, int length);
static void LightUIInputReset(void *arg);
static void ReloadConfigLightUI(void *arg);
static ConfigFileDesc* GetLightUIDesc();
static void LightUIMainWindowSizeHint(void *arg, int* x, int* y, int* w, int* h);

FCITX_EXPORT_API
FcitxUI ui = {
    LightUICreate,
    LightUICloseInputWindow,
    LightUIShowInputWindow,
    LightUIMoveInputWindow,
    LightUIUpdateStatus,
    LightUIRegisterStatus,
    LightUIRegisterMenu,
    LightUIOnInputFocus,
    LightUIOnInputUnFocus,
    LightUIOnTriggerOn,
    LightUIOnTriggerOff,
    LightUIDisplayMessage,
    LightUIMainWindowSizeHint,
    ReloadConfigLightUI
};

void* LightUICreate(FcitxInstance* instance)
{
    FcitxAddon* lightuiaddon = GetAddonByName(&instance->addons, FCITX_LIGHT_UI_NAME);
    FcitxModuleFunctionArg arg;
    FcitxLightUI* lightui = fcitx_malloc0(sizeof(FcitxLightUI));
    lightui->owner = instance;
    if (!LoadLightUIConfig(lightui))
    {
        free(lightui);
        return NULL;
    }
    lightui->dpy = InvokeFunction(instance, FCITX_X11, GETDISPLAY, arg);
    if (lightui->dpy == NULL)
    {
        free(lightui);
        return NULL;
    }

    CreateFont(lightui);
    lightui->iScreen = DefaultScreen(lightui->dpy);

    lightui->protocolAtom = XInternAtom (lightui->dpy, "WM_PROTOCOLS", False);
    lightui->killAtom = XInternAtom (lightui->dpy, "WM_DELETE_WINDOW", False);

    /* Main Menu Initial */
    utarray_init(&lightui->mainMenu.shell, &menuICD);

    FcitxUIMenu **menupp;
    for (menupp = (FcitxUIMenu **) utarray_front(&instance->uimenus);
            menupp != NULL;
            menupp = (FcitxUIMenu **) utarray_next(&instance->uimenus, menupp)
        )
    {
        FcitxUIMenu * menup = *menupp;
        if (!menup->isSubMenu)
            AddMenuShell(&lightui->mainMenu, menup->name, MENUTYPE_SUBMENU, menup);
    }
    AddMenuShell(&lightui->mainMenu, NULL, MENUTYPE_DIVLINE, NULL);
    AddMenuShell(&lightui->mainMenu, _("Configure"), MENUTYPE_SIMPLE, NULL);
    AddMenuShell(&lightui->mainMenu, _("Exit"), MENUTYPE_SIMPLE, NULL);
    lightui->mainMenu.MenuAction = MainMenuAction;
    lightui->mainMenu.priv = lightui;
    lightui->mainMenu.mark = -1;


    lightui->inputWindow = CreateInputWindow(lightui);
    lightui->mainWindow = CreateMainWindow(lightui);
    lightui->trayWindow = CreateTrayWindow(lightui);
    lightui->messageWindow = CreateMessageWindow(lightui);
    lightui->mainMenuWindow = CreateMainMenuWindow(lightui);

    FcitxIMEventHook resethk;
    resethk.arg = lightui;
    resethk.func = LightUIInputReset;
    RegisterResetInputHook(instance, resethk);
    return lightui;
}

void LightUISetWindowProperty(FcitxLightUI* lightui, Window window, FcitxXWindowType type, char *windowTitle)
{
    FcitxModuleFunctionArg arg;
    arg.args[0] = &window;
    arg.args[1] = &type;
    arg.args[2] = windowTitle;
    InvokeFunction(lightui->owner, FCITX_X11, SETWINDOWPROP, arg);
}

static void LightUIInputReset(void *arg)
{
    FcitxLightUI* lightui = (FcitxLightUI*) arg;
    DrawMainWindow(lightui->mainWindow);
}

static void LightUICloseInputWindow(void *arg)
{
    FcitxLightUI* lightui = (FcitxLightUI*) arg;
    CloseInputWindowInternal(lightui->inputWindow);
}

static void LightUIShowInputWindow(void *arg)
{
    FcitxLightUI* lightui = (FcitxLightUI*) arg;
    ShowInputWindowInternal(lightui->inputWindow);
}

static void LightUIMoveInputWindow(void *arg)
{
    FcitxLightUI* lightui = (FcitxLightUI*) arg;
    MoveInputWindowInternal(lightui->inputWindow);
}

static void LightUIUpdateStatus(void *arg, FcitxUIStatus* status)
{
    FcitxLightUI* lightui = (FcitxLightUI*) arg;
    DrawMainWindow(lightui->mainWindow);
}

static void LightUIRegisterMenu(void *arg, FcitxUIMenu* menu)
{
    FcitxLightUI* lightui = (FcitxLightUI*) arg;
    XlibMenu* xlibMenu = CreateXlibMenu(lightui);
    menu->uipriv = xlibMenu;
    xlibMenu->menushell = menu;
}

static void LightUIRegisterStatus(void *arg, FcitxUIStatus* status)
{
    FcitxLightUI* lightui = (FcitxLightUI*) arg;
    status->priv = fcitx_malloc0(sizeof(FcitxLightUIStatus));
    char activename[PATH_MAX], inactivename[PATH_MAX];
    sprintf(activename, "%s_active.png", status->name);
    sprintf(inactivename, "%s_inactive.png", status->name);
}

static void LightUIOnInputFocus(void *arg)
{
    FcitxLightUI* lightui = (FcitxLightUI*) arg;
    FcitxInstance *instance = lightui->owner;
    DrawMainWindow(lightui->mainWindow);
    if (GetCurrentState(instance) == IS_ACTIVE)
    {
        ShowMainWindow(lightui->mainWindow);
    }
    DrawTrayWindow(lightui->trayWindow);
}

static void LightUIOnInputUnFocus(void *arg)
{
    FcitxLightUI* lightui = (FcitxLightUI*) arg;
    DrawMainWindow(lightui->mainWindow);
    DrawTrayWindow(lightui->trayWindow);
}
Bool
IsWindowVisible(Display* dpy, Window window)
{
    XWindowAttributes attrs;

    XGetWindowAttributes(dpy, window, &attrs);

    if (attrs.map_state == IsUnmapped)
        return False;

    return True;
}

void ActivateWindow(Display *dpy, int iScreen, Window window)
{
    XEvent ev;

    memset(&ev, 0, sizeof(ev));

    Atom _NET_ACTIVE_WINDOW = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);

    ev.xclient.type = ClientMessage;
    ev.xclient.window = window;
    ev.xclient.message_type = _NET_ACTIVE_WINDOW;
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = 1;
    ev.xclient.data.l[1] = CurrentTime;
    ev.xclient.data.l[2] = 0;

    XSendEvent(dpy, RootWindow(dpy, iScreen), False, SubstructureNotifyMask, &ev);
    XSync(dpy, False);
}

void GetScreenSize(FcitxLightUI* lightui, int* width, int* height)
{
    FcitxModuleFunctionArg arg;
    arg.args[0] = width;
    arg.args[1] = height;
    InvokeFunction(lightui->owner, FCITX_X11, GETSCREENSIZE, arg);
}

CONFIG_DESC_DEFINE(GetLightUIDesc, "fcitx-light-ui.desc")

boolean LoadLightUIConfig(FcitxLightUI* lightui)
{
    ConfigFileDesc* configDesc = GetLightUIDesc();
    if (configDesc == NULL)
        return false;
    FILE *fp;
    char *file;
    fp = GetXDGFileUserWithPrefix("conf", "fcitx-light-ui.config", "rt", &file);
    FcitxLog(INFO, _("Load Config File %s"), file);
    free(file);
    if (!fp) {
        if (errno == ENOENT)
            SaveLightUIConfig(lightui);
    }

    ConfigFile *cfile = ParseConfigFileFp(fp, configDesc);

    FcitxLightUIConfigBind(lightui, cfile, configDesc);
    ConfigBindSync(&lightui->gconfig);

    if (fp)
        fclose(fp);
    return true;
}

void SaveLightUIConfig(FcitxLightUI *lightui)
{
    ConfigFileDesc* configDesc = GetLightUIDesc();
    char *file;
    FILE *fp = GetXDGFileUserWithPrefix("conf", "fcitx-light-ui.config", "wt", &file);
    FcitxLog(INFO, "Save Config to %s", file);
    SaveConfigFileFp(fp, &lightui->gconfig, configDesc);
    free(file);
    if (fp)
        fclose(fp);
}

Visual * FindARGBVisual (FcitxLightUI* lightui)
{
    FcitxModuleFunctionArg arg;
    return InvokeFunction(lightui->owner, FCITX_X11, FINDARGBVISUAL, arg);
}

boolean IsInRspArea(int x0, int y0, FcitxLightUIStatus* status)
{
    return IsInBox(x0, y0, status->x, status->y, status->w, status->h);
}

boolean
LightUIMouseClick(FcitxLightUI* lightui, Window window, int *x, int *y)
{
    boolean            bMoved = false;
    FcitxModuleFunctionArg arg;
    arg.args[0] = &window;
    arg.args[1] = x;
    arg.args[2] = y;
    arg.args[3] = &bMoved;
    InvokeFunction(lightui->owner, FCITX_X11, MOUSECLICK, arg);

    return bMoved;
}

void LightUIOnTriggerOn(void* arg)
{
    FcitxLightUI* lightui = (FcitxLightUI*) arg;
    FcitxInstance *instance = lightui->owner;
    if (GetCurrentState(instance) == IS_ACTIVE)
    {
        DrawMainWindow(lightui->mainWindow);
        ShowMainWindow(lightui->mainWindow);
    }
    DrawTrayWindow(lightui->trayWindow);
}

void LightUIOnTriggerOff(void* arg)
{
    FcitxLightUI* lightui = (FcitxLightUI*) arg;
    DrawMainWindow(lightui->mainWindow);
    DrawTrayWindow(lightui->trayWindow);
}

void LightUIDisplayMessage(void* arg, char* title, char** msg, int length)
{
    FcitxLightUI* lightui = (FcitxLightUI*) arg;
    XMapRaised(lightui->dpy, lightui->messageWindow->window);
    DrawMessageWindow(lightui->messageWindow, title, msg, length);
}

boolean MainMenuAction(FcitxUIMenu* menu, int index)
{
    FcitxLightUI* lightui = (FcitxLightUI*) menu->priv;
    int length = utarray_len(&menu->shell);
    if (index == 0)
    {
    }
    else if (index == length - 1) /* Exit */
    {
        EndInstance(lightui->owner);
    }
    else if (index == length - 2) /* Configuration */
    {
        pid_t id;

        id = fork();

        if (id < 0)
            FcitxLog(ERROR, _("Unable to create process"));
        else if (id == 0)
        {
            id = fork();

            if (id < 0)
            {
                FcitxLog(ERROR, _("Unable to create process"));
                exit(1);
            }
            else if (id > 0)
                exit(0);
            else
            {
                execl(BINDIR "/fcitx-configtool", "fcitx-configtool", NULL);
                exit(0);
            }
        }
    }
    return true;
}

void
LightUIInitWindowAttribute(FcitxLightUI* lightui, Visual ** vs, Colormap * cmap,
                             XSetWindowAttributes * attrib,
                             unsigned long *attribmask, int *depth)
{
    FcitxModuleFunctionArg arg;
    arg.args[0] = vs;
    arg.args[1] = cmap;
    arg.args[2] = attrib;
    arg.args[3] = attribmask;
    arg.args[4] = depth;
    InvokeFunction(lightui->owner, FCITX_X11, INITWINDOWATTR, arg);
}

Visual * LightUIFindARGBVisual (FcitxLightUI* lightui)
{
    FcitxModuleFunctionArg arg;
    return InvokeFunction(lightui->owner, FCITX_X11, FINDARGBVISUAL, arg);
}

void LightUIMainWindowSizeHint(void* arg, int* x, int* y, int* w, int* h)
{
    FcitxLightUI* lightui = (FcitxLightUI*) arg;
    if (x)
    {
        *x = lightui->iMainWindowOffsetX;
    }
    if (y)
    {
        *y = lightui->iMainWindowOffsetY;
    }

    XWindowAttributes attr;
    XGetWindowAttributes(lightui->dpy, lightui->mainWindow->window, &attr);
    if (w)
    {
        *w = attr.width;
    }
    if (h)
    {
        *h = attr.height;
    }

}

void ReloadConfigLightUI(void* arg)
{
    FcitxLightUI* lightui = (FcitxLightUI*) arg;
    LoadLightUIConfig(lightui);
}

boolean WindowIsVisable(Display* dpy, Window window)
{
    XWindowAttributes attr;
    XGetWindowAttributes(dpy, window, &attr);
    return attr.map_state == IsViewable;
}

GC LightUICreateGC(Display* dpy, Drawable drawable, ConfigColor color)
{
    XGCValues gcvalues;
    GC gc = XCreateGC(dpy, drawable, 0, &gcvalues);
    LightUISetGC(dpy, gc, color);

    return gc;
}

void LightUISetGC(Display* dpy, GC gc, ConfigColor color)
{
    XGCValues gcvalues;
    XColor xcolor;
    xcolor.red = color.r * 65535;
    xcolor.green = color.g * 65535;
    xcolor.blue = color.b * 65535;
    long unsigned int iPixel;
    if (XAllocColor(dpy, DefaultColormap (dpy, DefaultScreen (dpy)), &xcolor))
        iPixel = xcolor.pixel;
    else
        iPixel = WhitePixel (dpy, DefaultScreen (dpy));

    XSetForeground (dpy, gc, iPixel);
}
// kate: indent-mode cstyle; space-indent on; indent-width 0;
