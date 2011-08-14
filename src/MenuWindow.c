/***************************************************************************
 *   Copyright (C) 2009~2010 by t3swing                                    *
 *   t3swing@sina.com                                                      *
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
#include <ctype.h>
#include <math.h>
#include <iconv.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include "fcitx-config/fcitx-config.h"
#include <fcitx-utils/log.h>
#include <fcitx/ui.h>
#include <fcitx/module.h>
#include <fcitx/module/x11/x11stuff.h>

#include "skin.h"
#include "lightui.h"
#include "MenuWindow.h"
#include <fcitx/instance.h>
#include <fcitx-utils/utils.h>
#include "font.h"

#define MenuMarginTop 1
#define MenuMarginRight 1
#define MenuMarginLeft 1
#define MenuMarginBottom 1
#define MenuFontSize FontHeight(menu->owner->dpy, menu->owner->xftfont)

static boolean ReverseColor(XlibMenu * Menu,int shellIndex);
static void MenuMark(XlibMenu* menu, int y, int i);
static void DrawArrow(XlibMenu* menu, int line_y);
static void MoveSubMenu(XlibMenu *sub, XlibMenu *parent, int offseth);
static void DisplayText(XlibMenu * menu,int shellindex,int line_y);
static void DrawDivLine(XlibMenu * menu,int line_y);
static boolean MenuWindowEventHandler(void *arg, XEvent* event);
static int SelectShellIndex(XlibMenu * menu, int x, int y, int* offseth);
static void CloseAllMenuWindow(FcitxLightUI *lightui);
static void CloseAllSubMenuWindow(XlibMenu *xlibMenu);
static void CloseOtherSubMenuWindow(XlibMenu *xlibMenu, XlibMenu* subMenu);
static boolean IsMouseInOtherMenu(XlibMenu *xlibMenu, int x, int y);
static void InitXlibMenu(XlibMenu* menu);
static void ReloadXlibMenu(void* arg, boolean enabled);

#define GetMenuShell(m, i) ((MenuShell*) utarray_eltptr(&(m)->shell, (i)))

void InitXlibMenu(XlibMenu* menu)
{
    FcitxLightUI* lightui = menu->owner;
    char        strWindowName[]="Fcitx Menu Window";
    XSetWindowAttributes attrib;
    unsigned long   attribmask;
    int depth;
    Colormap cmap;
    Visual * vs;
    XGCValues xgv;
    GC gc;
    Display* dpy = lightui->dpy;
    int iScreen = lightui->iScreen;

    vs= NULL;
    LightUIInitWindowAttribute(lightui, &vs, &cmap, &attrib, &attribmask, &depth);

    //开始只创建一个简单的窗口不做任何动作
    menu->menuWindow =XCreateWindow (dpy,
                                     RootWindow (dpy, iScreen),
                                     0, 0,
                                     MENU_WINDOW_WIDTH,MENU_WINDOW_HEIGHT,
                                     0, depth, InputOutput,
                                     vs, attribmask, &attrib);

    if (menu->menuWindow == (Window) NULL)
        return;

    XSetTransientForHint (dpy, menu->menuWindow, DefaultRootWindow (dpy));

    menu->pixmap = XCreatePixmap(dpy,
                                 menu->menuWindow,
                                 MENU_WINDOW_WIDTH,
                                 MENU_WINDOW_HEIGHT,
                                 depth);

    menu->xftDraw = XftDrawCreate(dpy, menu->pixmap, DefaultVisual (dpy, DefaultScreen (dpy)), DefaultColormap (dpy, DefaultScreen (dpy)));

    XSelectInput (dpy, menu->menuWindow, KeyPressMask | ExposureMask | ButtonPressMask | ButtonReleaseMask  | PointerMotionMask | LeaveWindowMask | StructureNotifyMask );

    LightUISetWindowProperty(lightui, menu->menuWindow, FCITX_WINDOW_MENU, strWindowName);

    menu->iPosX=100;
    menu->iPosY=100;
}


XlibMenu* CreateMainMenuWindow(FcitxLightUI *lightui)
{
    XlibMenu* menu = CreateXlibMenu(lightui);
    menu->menushell = &lightui->mainMenu;

    return menu;
}

boolean MenuWindowEventHandler(void *arg, XEvent* event)
{
    XlibMenu* menu = (XlibMenu*) arg;
    if (event->xany.window == menu->menuWindow)
    {
        switch (event->type)
        {
        case MapNotify:
            UpdateMenuShell(menu->menushell);
            break;
        case Expose:
            DrawXlibMenu(menu);
            break;
        case LeaveNotify:
        {
            int x = event->xcrossing.x_root;
            int y = event->xcrossing.y_root;

            if (!IsMouseInOtherMenu(menu, x, y))
            {
                CloseAllSubMenuWindow(menu);
            }
        }
        break;
        case MotionNotify:
        {
            int offseth = 0;
            GetMenuSize(menu);
            int i=SelectShellIndex(menu, event->xmotion.x, event->xmotion.y, &offseth);
            boolean flag = ReverseColor(menu,i);
            if (!flag)
            {
                DrawXlibMenu(menu);
            }
            MenuShell *shell = GetMenuShell(menu->menushell, i);
            if (shell && shell->type == MENUTYPE_SUBMENU && shell->subMenu)
            {
                XlibMenu* subxlibmenu = (XlibMenu*) shell->subMenu->uipriv;
                CloseOtherSubMenuWindow(menu, subxlibmenu);
                MoveSubMenu(subxlibmenu, menu, offseth);
                DrawXlibMenu(subxlibmenu);
                XMapRaised(menu->owner->dpy, subxlibmenu->menuWindow);
            }
            if (shell == NULL)
                CloseOtherSubMenuWindow(menu, NULL);
        }
        break;
        case ButtonPress:
        {
            switch (event->xbutton.button)
            {
            case Button1:
            {
                int offseth;
                int i=SelectShellIndex(menu, event->xmotion.x, event->xmotion.y, &offseth);
                if (menu->menushell->MenuAction)
                {
                    if (menu->menushell->MenuAction(menu->menushell, i))
                        CloseAllMenuWindow(menu->owner);
                }
            }
            break;
            case Button3:
                CloseAllMenuWindow(menu->owner);
                break;
            }
        }
        break;
        }
        return true;
    }
    return false;
}

void CloseAllMenuWindow(FcitxLightUI *lightui)
{
    FcitxInstance* instance = lightui->owner;
    FcitxUIMenu** menupp;
    for (menupp = (FcitxUIMenu **) utarray_front(&instance->uimenus);
            menupp != NULL;
            menupp = (FcitxUIMenu **) utarray_next(&instance->uimenus, menupp)
        )
    {
        XlibMenu* xlibMenu = (XlibMenu*) (*menupp)->uipriv;
        XUnmapWindow(lightui->dpy, xlibMenu->menuWindow);
    }
    XUnmapWindow(lightui->dpy, lightui->mainMenuWindow->menuWindow);
}

void CloseOtherSubMenuWindow(XlibMenu *xlibMenu, XlibMenu* subMenu)
{
    MenuShell *menu;
    for (menu = (MenuShell *) utarray_front(&xlibMenu->menushell->shell);
            menu != NULL;
            menu = (MenuShell *) utarray_next(&xlibMenu->menushell->shell, menu)
        )
    {
        if (menu->type == MENUTYPE_SUBMENU && menu->subMenu && menu->subMenu->uipriv != subMenu)
        {
            CloseAllSubMenuWindow((XlibMenu *)menu->subMenu->uipriv);
        }
    }
}

void CloseAllSubMenuWindow(XlibMenu *xlibMenu)
{
    MenuShell *menu;
    for (menu = (MenuShell *) utarray_front(&xlibMenu->menushell->shell);
            menu != NULL;
            menu = (MenuShell *) utarray_next(&xlibMenu->menushell->shell, menu)
        )
    {
        if (menu->type == MENUTYPE_SUBMENU && menu->subMenu)
        {
            CloseAllSubMenuWindow((XlibMenu *)menu->subMenu->uipriv);
        }
    }
    XUnmapWindow(xlibMenu->owner->dpy, xlibMenu->menuWindow);
}

boolean IsMouseInOtherMenu(XlibMenu *xlibMenu, int x, int y)
{
    FcitxLightUI *lightui = xlibMenu->owner;
    FcitxInstance* instance = lightui->owner;
    FcitxUIMenu** menupp;
    for (menupp = (FcitxUIMenu **) utarray_front(&instance->uimenus);
            menupp != NULL;
            menupp = (FcitxUIMenu **) utarray_next(&instance->uimenus, menupp)
        )
    {

        XlibMenu* otherXlibMenu = (XlibMenu*) (*menupp)->uipriv;
        if (otherXlibMenu == xlibMenu)
            continue;
        XWindowAttributes attr;
        XGetWindowAttributes(lightui->dpy, otherXlibMenu->menuWindow, &attr);
        if (attr.map_state != IsUnmapped &&
                IsInBox(x, y, attr.x, attr.y, attr.width, attr.height))
        {
            return true;
        }
    }

    XlibMenu* otherXlibMenu = lightui->mainMenuWindow;
    if (otherXlibMenu == xlibMenu)
        return false;
    XWindowAttributes attr;
    XGetWindowAttributes(lightui->dpy, otherXlibMenu->menuWindow, &attr);
    if (attr.map_state != IsUnmapped &&
            IsInBox(x, y, attr.x, attr.y, attr.width, attr.height))
    {
        return true;
    }
    return false;
}

XlibMenu* CreateXlibMenu(FcitxLightUI *lightui)
{
    XlibMenu *menu = fcitx_malloc0(sizeof(XlibMenu));
    menu->owner = lightui;
    InitXlibMenu(menu);

    FcitxModuleFunctionArg arg;
    arg.args[0] = MenuWindowEventHandler;
    arg.args[1] = menu;
    InvokeFunction(lightui->owner, FCITX_X11, ADDXEVENTHANDLER, arg);

    arg.args[0] = ReloadXlibMenu;
    arg.args[1] = menu;
    InvokeFunction(lightui->owner, FCITX_X11, ADDCOMPOSITEHANDLER, arg);
    return menu;
}

void GetMenuSize(XlibMenu * menu)
{
    int i=0;
    int winheight=0;
    int fontheight=0;
    int menuwidth = 0;

    winheight = MenuMarginTop + MenuMarginBottom;//菜单头和尾都空8个pixel
    fontheight= MenuFontSize;
    for (i=0;i<utarray_len(&menu->menushell->shell);i++)
    {
        if ( GetMenuShell(menu->menushell, i)->type == MENUTYPE_SIMPLE || GetMenuShell(menu->menushell, i)->type == MENUTYPE_SUBMENU)
        {
            winheight += 6+fontheight;
        }
        else if ( GetMenuShell(menu->menushell, i)->type == MENUTYPE_DIVLINE)
            winheight += 5;

        int width = StringWidth(menu->owner->dpy, menu->owner->xftfont, GetMenuShell(menu->menushell, i)->tipstr);
        if (width > menuwidth)
            menuwidth = width;
    }
    menu->height = winheight;
    menu->width = menuwidth + MenuMarginLeft + MenuMarginRight + 15 + 20;
}

//根据Menu内容来绘制菜单内容
void DrawXlibMenu(XlibMenu * menu)
{
    FcitxLightUI *lightui = menu->owner;
    Display* dpy = lightui->dpy;
    GC gc = XCreateGC( dpy, menu->menuWindow, 0, NULL );
    int i=0;
    int fontheight;
    int iPosY = 0;

    fontheight= MenuFontSize;

    GetMenuSize(menu);

    DrawResizableBackground(menu->owner, menu->pixmap, menu->height, menu->width,
                            menu->owner->backcolor, menu->owner->bordercolor, gc);

    iPosY=MenuMarginTop;
    for (i=0;i<utarray_len(&menu->menushell->shell);i++)
    {
        if ( GetMenuShell(menu->menushell, i)->type == MENUTYPE_SIMPLE || GetMenuShell(menu->menushell, i)->type == MENUTYPE_SUBMENU)
        {
            DisplayText( menu,i,iPosY);
            if (menu->menushell->mark == i)
                MenuMark(menu,iPosY,i);

            if (GetMenuShell(menu->menushell, i)->type == MENUTYPE_SUBMENU)
                DrawArrow(menu, iPosY);
            iPosY=iPosY+6+fontheight;
        }
        else if ( GetMenuShell(menu->menushell, i)->type == MENUTYPE_DIVLINE)
        {
            DrawDivLine(menu,iPosY);
            iPosY+=5;
        }
    }

    XResizeWindow(dpy, menu->menuWindow, menu->width, menu->height);
    XCopyArea (dpy,
               menu->pixmap,
               menu->menuWindow,
               gc,
               0,
               0,
               menu->width,
               menu->height, 0, 0);
    XFreeGC(dpy, gc);
}

void DisplayXlibMenu(XlibMenu * menu)
{
    FcitxLightUI *lightui = menu->owner;
    Display* dpy = lightui->dpy;
    XMapRaised (dpy, menu->menuWindow);
    XMoveWindow(dpy, menu->menuWindow, menu->iPosX, menu->iPosY);
}

void DrawDivLine(XlibMenu * menu,int line_y)
{
    int marginLeft = MenuMarginLeft;
    int marginRight = MenuMarginRight;
}

void MenuMark(XlibMenu * menu,int y,int i)
{
    int marginLeft = MenuMarginLeft;
    double size = (MenuFontSize * 0.7 ) / 2;
}

/*
* 显示菜单上面的文字信息,只需要指定窗口,窗口宽度,需要显示文字的上边界,字体,显示的字符串和是否选择(选择后反色)
* 其他都固定,如背景和文字反色不反色的颜色,反色框和字的位置等
*/
void DisplayText(XlibMenu * menu,int shellindex,int line_y)
{
    int marginLeft = MenuMarginLeft;
    int marginRight = MenuMarginRight;

    if (GetMenuShell(menu->menushell, shellindex)->isselect ==0)
    {
        OutputString(menu->owner->dpy, menu->xftDraw, menu->pixmap, menu->owner->xftfont, GetMenuShell(menu->menushell, shellindex)->tipstr , 15 + marginLeft ,line_y, menu->owner->menuFontColor[MENU_INACTIVE]);
    }
    else
    {
        GC gc = LightUICreateGC(menu->owner->dpy, menu->pixmap, menu->owner->activeColor);
        XFillRectangle(menu->owner->dpy, menu->pixmap, gc, marginLeft ,line_y, menu->width - marginRight - marginLeft, MenuFontSize+4);
        XFreeGC(menu->owner->dpy, gc);

        OutputString(menu->owner->dpy, menu->xftDraw, menu->pixmap, menu->owner->xftfont, GetMenuShell(menu->menushell, shellindex)->tipstr , 15 + marginLeft ,line_y, menu->owner->menuFontColor[MENU_ACTIVE]);
    }
}

void DrawArrow(XlibMenu *menu, int line_y)
{
    int marginRight = MenuMarginRight;
    double size = MenuFontSize * 0.4;
    double offset = (MenuFontSize - size) / 2;
}

/**
*返回鼠标指向的菜单在menu中是第多少项
*/
int SelectShellIndex(XlibMenu * menu, int x, int y, int* offseth)
{
    int i;
    int winheight=MenuMarginTop;
    int fontheight;
    int marginLeft = MenuMarginLeft;

    if (x < marginLeft)
        return -1;

    fontheight= MenuFontSize;
    for (i=0;i<utarray_len(&menu->menushell->shell);i++)
    {
        if (GetMenuShell(menu->menushell, i)->type == MENUTYPE_SIMPLE || GetMenuShell(menu->menushell, i)->type == MENUTYPE_SUBMENU)
        {
            if (y>winheight+1 && y<winheight+6+fontheight-1)
            {
                if (offseth)
                    *offseth = winheight;
                return i;
            }
            winheight=winheight+6+fontheight;
        }
        else if (GetMenuShell(menu->menushell, i)->type == MENUTYPE_DIVLINE)
            winheight+=5;
    }
    return -1;
}

boolean ReverseColor(XlibMenu * menu,int shellIndex)
{
    boolean flag = False;
    int i;

    int last = -1;

    for (i=0;i<utarray_len(&menu->menushell->shell);i++)
    {
        if (GetMenuShell(menu->menushell, i)->isselect)
            last = i;

        GetMenuShell(menu->menushell, i)->isselect=0;
    }
    if (shellIndex == last)
        flag = True;
    if (shellIndex >=0 && shellIndex < utarray_len(&menu->menushell->shell))
        GetMenuShell(menu->menushell, shellIndex)->isselect = 1;
    return flag;
}

void ClearSelectFlag(XlibMenu * menu)
{
    int i;
    for (i=0;i< utarray_len(&menu->menushell->shell);i++)
    {
        GetMenuShell(menu->menushell, i)->isselect=0;
    }
}

void ReloadXlibMenu(void* arg, boolean enabled)
{
    XlibMenu* menu = (XlibMenu*) arg;
    boolean visable = WindowIsVisable(menu->owner->dpy, menu->menuWindow);
    XFreePixmap(menu->owner->dpy, menu->pixmap);
    XDestroyWindow(menu->owner->dpy, menu->menuWindow);
    XftDrawDestroy(menu->xftDraw);

    menu->pixmap = None;
    menu->menuWindow = None;

    InitXlibMenu(menu);
    if (visable)
        XMapWindow(menu->owner->dpy, menu->menuWindow);
}

void MoveSubMenu(XlibMenu *sub, XlibMenu *parent, int offseth)
{
    int dwidth, dheight;
    GetScreenSize(parent->owner, &dwidth, &dheight);
    UpdateMenuShell(sub->menushell);
    GetMenuSize(sub);
    sub->iPosX=parent->iPosX + parent->width - MenuMarginRight - 4;
    sub->iPosY=parent->iPosY + offseth - MenuMarginTop;

    if ( sub->iPosX + sub->width > dwidth)
        sub->iPosX=parent->iPosX - sub->width + MenuMarginLeft + 4;

    if ( sub->iPosY + sub->height > dheight)
        sub->iPosY = dheight - sub->height;

    XMoveWindow(parent->owner->dpy, sub->menuWindow, sub->iPosX, sub->iPosY);
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
