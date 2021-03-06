/***************************************************************************
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

#include "MessageWindow.h"
#include "fcitx-utils/log.h"

#include <ctype.h>

#include <iconv.h>
#include <X11/Xatom.h>
#include <string.h>
#include "lightui.h"
#include "fcitx/module.h"
#include <X11/Xutil.h>
#include "fcitx/module/x11/x11stuff.h"
#include <fcitx-utils/utils.h>


#define MESSAGE_WINDOW_MARGIN 20
#define MESSAGE_WINDOW_LINESPACE 2
static void            InitMessageWindowProperty (MessageWindow* messageWindow);
static boolean MessageWindowEventHandler(void *arg, XEvent* event);
static void DisplayMessageWindow (MessageWindow *messageWindow);

MessageWindow* CreateMessageWindow (FcitxLightUI * lightui)
{
    MessageWindow* messageWindow = fcitx_malloc0(sizeof(MessageWindow));
    Display *dpy = lightui->dpy;
    int iScreen= lightui->iScreen;
    messageWindow->owner = lightui;

    messageWindow->color.r = messageWindow->color.g = messageWindow->color.b = 220.0 / 256;
    messageWindow->fontColor.r = messageWindow->fontColor.g = messageWindow->fontColor.b = 0;
    messageWindow->fontSize = 15;
    messageWindow->width = 1;
    messageWindow->height = 1;

    messageWindow->window =
        XCreateSimpleWindow (dpy, DefaultRootWindow (dpy), 0, 0, 1, 1, 0, WhitePixel (dpy, DefaultScreen (dpy)), WhitePixel (dpy, DefaultScreen (dpy)));

    if (messageWindow->window == None)
        return False;

    InitMessageWindowProperty (messageWindow);
    XSelectInput (dpy, messageWindow->window, ExposureMask | ButtonPressMask | ButtonReleaseMask  | PointerMotionMask );

    FcitxModuleFunctionArg arg;
    arg.args[0] = MessageWindowEventHandler;
    arg.args[1] = messageWindow;
    InvokeFunction(lightui->owner, FCITX_X11, ADDXEVENTHANDLER, arg);

    return messageWindow;
}

boolean MessageWindowEventHandler(void *arg, XEvent* event)
{
    MessageWindow* messageWindow = (MessageWindow*) arg;
    if (event->type == ClientMessage
            && event->xclient.data.l[0] == messageWindow->owner->killAtom
            && event->xclient.window == messageWindow->window
       )
    {
        XUnmapWindow(messageWindow->owner->dpy, messageWindow->window);
        return true;
    }

    if (event->xany.window == messageWindow->window)
    {
        switch (event->type)
        {
        case Expose:
            DrawMessageWindow(messageWindow, NULL, NULL, 0);
            DisplayMessageWindow(messageWindow);
            break;
        case ButtonRelease:
        {
            switch (event->xbutton.button)
            {
            case Button1:
                XUnmapWindow(messageWindow->owner->dpy, messageWindow->window);
                break;
            }
        }
        break;
        }
        return true;
    }
    return false;
}
void InitMessageWindowProperty (MessageWindow *messageWindow)
{
    FcitxLightUI* lightui = messageWindow->owner;
    Display *dpy = lightui->dpy;
    XSetTransientForHint (dpy, messageWindow->window, DefaultRootWindow (dpy));

    LightUISetWindowProperty(lightui, messageWindow->window, FCITX_WINDOW_DIALOG, "Fcitx - Message");

    XSetWMProtocols(dpy, messageWindow->window, &lightui->killAtom, 1);
}

void DisplayMessageWindow (MessageWindow *messageWindow)
{
    FcitxLightUI* lightui = messageWindow->owner;
    Display *dpy = lightui->dpy;
    int dwidth, dheight;
    GetScreenSize(lightui, &dwidth, &dheight);
    XMapRaised (dpy, messageWindow->window);
    XMoveWindow (dpy, messageWindow->window, (dwidth - messageWindow->width) / 2, (dheight - messageWindow->height) / 2);
}

void DrawMessageWindow (MessageWindow* messageWindow, char *title, char **msg, int length)
{
    FcitxLightUI* lightui = messageWindow->owner;
    Display *dpy = lightui->dpy;
    int i = 0;
    if (title)
    {
        if (messageWindow->title)
            free(messageWindow->title);
        messageWindow->title = strdup(title);
    }
    else
        if (!messageWindow->title)
            return;

    title = messageWindow->title;
    FcitxLog(DEBUG, "%s", title);

    XTextProperty   tp;
    Xutf8TextListToTextProperty(dpy, &title, 1, XUTF8StringStyle, &tp);
    XSetWMName(dpy, messageWindow->window, &tp);
    XFree(tp.value);

    if (msg)
    {
        if (messageWindow->msg)
        {
            for (i =0 ;i<messageWindow->length; i++)
                free(messageWindow->msg[i]);
            free(messageWindow->msg);
        }
        messageWindow->length = length;
        messageWindow->msg = malloc(sizeof(char*) * length);
        for (i = 0; i < messageWindow->length; i++)
            messageWindow->msg[i] = strdup(msg[i]);
    }
    else
    {
        if (!messageWindow->msg)
            return;
    }
    msg = messageWindow->msg;
    length = messageWindow->length;

    if (!msg || length == 0)
        return;

    messageWindow->height = MESSAGE_WINDOW_MARGIN * 2 + length *(messageWindow->fontSize + MESSAGE_WINDOW_LINESPACE);
    messageWindow->width = 0;

    for (i = 0; i< length ;i ++)
    {
        int width;
        if (width > messageWindow->width)
            messageWindow->width = width;
    }

    messageWindow->width += MESSAGE_WINDOW_MARGIN * 2;
    XResizeWindow(dpy, messageWindow->window, messageWindow->width, messageWindow->height);

    int x, y;
    x = MESSAGE_WINDOW_MARGIN;
    y = MESSAGE_WINDOW_MARGIN;
    for (i = 0; i< length ;i ++)
    {
        y += messageWindow->fontSize + MESSAGE_WINDOW_LINESPACE;
    }

    ActivateWindow(dpy, lightui->iScreen, messageWindow->window);
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
