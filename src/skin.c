/***************************************************************************
 *   Copyright (C) 2009~2010 by t3swing                                    *
 *   t3swing@gmail.com                                                     *
 *   Copyright (C) 2009~2010 by Yuking                                     *
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
 * @file   skin.c
 * @author Yuking yuking_net@sohu.com  t3swing  t3swing@sina.com
 *
 * @date   2009-10-9
 *
 * @brief  皮肤设置相关定义及初始化加载工作
 *
 *
 */
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <libintl.h>
#include <X11/xpm.h>

#include "fcitx/fcitx.h"

#include "fcitx-config/fcitx-config.h"
#include "fcitx-config/xdg.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/utarray.h"

#include "lightui.h"
#include "skin.h"
#include "MenuWindow.h"
#include "InputWindow.h"
#include "MainWindow.h"
#include "TrayWindow.h"
#include "fcitx/ui.h"
#include "fcitx/frontend.h"
#include "fcitx-utils/utils.h"
#include "fcitx/instance.h"
#include "fcitx/hook.h"
#include "fcitx/candidate.h"
#include "font.h"

#include "active.xpm"
#include "bingchan.xpm"
#include "cangjie.xpm"
#include "chttrans_active.xpm"
#include "chttrans_inactive.xpm"
#include "cn.xpm"
#include "dianbaoma.xpm"
#include "en.xpm"
#include "erbi.xpm"
#include "fullwidth_active.xpm"
#include "fullwidth_inactive.xpm"
#include "inactive.xpm"
#include "logo.xpm"
#include "next.xpm"
#include "pinyin.xpm"
#include "prev.xpm"
#include "punc_active.xpm"
#include "punc_inactive.xpm"
#include "quwei.xpm"
#include "remind_active.xpm"
#include "remind_inactive.xpm"
#include "shuangpin.xpm"
#include "vk_active.xpm"
#include "vk_inactive.xpm"
#include "wanfeng.xpm"
#include "wbpy.xpm"
#include "wubi.xpm"
#include "ziranma.xpm"


#define MarginTop 4
#define MarginLeft 4
#define MarginBottom 4
#define MarginRight 4
#define FONTHEIGHT FontHeight(inputWindow->dpy, inputWindow->owner->xftfont)
#define InputPos (fontSize)
#define OutputPos (fontSize * 2 + 8)

struct _ImageTable
{
    char* name;
    char** data;
} builtInImage[] =
{
    { "tray_active", active_xpm },
    { "tray_inactive", inactive_xpm },
    { "bingchan", bingchan_xpm },
    { "bingchan", bingchan_xpm },
    { "cangjie", cangjie_xpm },
    { "chttrans_active", chttrans_active_xpm },
    { "chttrans_inactive", chttrans_inactive_xpm },
    { "active", cn_xpm },
    { "dianbaoma", dianbaoma_xpm },
    { "en", en_xpm },
    { "erbi", erbi_xpm },
    { "fullwidth_active", fullwidth_active_xpm },
    { "fullwidth_inactive", fullwidth_inactive_xpm },
    { "logo", logo_xpm },
    { "next", next_xpm },
    { "pinyin", pinyin_xpm },
    { "prev", prev_xpm },
    { "punc_active", punc_active_xpm },
    { "punc_inactive", punc_inactive_xpm },
    { "quwei", quwei_xpm },
    { "remind_active", remind_active_xpm },
    { "remind_inactive", remind_inactive_xpm },
    { "shuangpin", shuangpin_xpm },
    { "vk_active", vk_active_xpm },
    { "vk_inactive", vk_inactive_xpm },
    { "wanfeng", wanfeng_xpm },
    { "wbpy", wbpy_xpm },
    { "wubi", wubi_xpm },
    { "ziranma", ziranma_xpm },
    { NULL, NULL }
};

void DrawResizableBackground(FcitxLightUI* lightui,
                             Drawable drawable,
                             int height,
                             int width,
                             ConfigColor background,
                             ConfigColor border,
                             GC gc
                            )
{
    int marginLeft = 1;
    int marginTop = 1;
    int marginRight = 1;
    int marginBottom = 1;
    XClearArea (lightui->dpy, drawable, 0, 0, width, height, False);

    LightUISetGC(lightui->dpy, gc, background);

    XFillRectangle (lightui->dpy, drawable, gc, marginLeft, marginTop, width - marginLeft - marginRight, height - marginTop - marginTop);

    LightUISetGC(lightui->dpy, gc, border);
    XFillRectangle (lightui->dpy, drawable, gc, 0, 0, width, marginTop);
    XFillRectangle (lightui->dpy, drawable, gc, 0, 0, marginLeft, height);
    XFillRectangle (lightui->dpy, drawable, gc, width - marginRight, 0, marginRight, height);
    XFillRectangle (lightui->dpy, drawable, gc, 0, height - marginBottom, width, marginBottom);
}

void DrawInputBar(InputWindow* inputWindow, int iCursorPos, Messages * msgup, Messages *msgdown ,unsigned int * iheight, unsigned int *iwidth)
{
    int i;
    char *strUp[MAX_MESSAGE_COUNT];
    char *strDown[MAX_MESSAGE_COUNT];
    int posUpX[MAX_MESSAGE_COUNT], posUpY[MAX_MESSAGE_COUNT];
    int posDownX[MAX_MESSAGE_COUNT], posDownY[MAX_MESSAGE_COUNT];
    int oldHeight = *iheight, oldWidth = *iwidth;
    int newHeight = 0, newWidth = 0;
    int cursor_pos=0;
    int inputWidth = 0, outputWidth = 0;
    int outputHeight = 0;
    FcitxInputState* input = &inputWindow->owner->owner->input;
    FcitxInstance* instance = inputWindow->owner->owner;
    int iChar = iCursorPos;
    int strWidth = 0, strHeight = 0;
    int fontSize = FONTHEIGHT;

    if (!IsMessageChanged(msgup) && !IsMessageChanged(msgdown))
        return;

    inputWidth = 0;
    strHeight = FONTHEIGHT;

    for (i = 0; i < GetMessageCount(msgup) ; i++)
    {
        char *trans = ProcessOutputFilter(instance, GetMessageString(msgup, i));
        if (trans)
            strUp[i] = trans;
        else
            strUp[i] = GetMessageString(msgup, i);
        posUpX[i] = MarginLeft + inputWidth;

        strWidth = StringWidth(inputWindow->dpy, inputWindow->owner->xftfont, strUp[i]);

        posUpY[i] = MarginTop + InputPos - strHeight;
        inputWidth += strWidth;
        if (input->bShowCursor)
        {
            int length = strlen(GetMessageString(msgup, i));
            if (iChar >= 0)
            {
                if (iChar < length)
                {
                    char strTemp[MESSAGE_MAX_LENGTH];
                    char *strGBKT = NULL;
                    strncpy(strTemp, strUp[i], iChar);
                    strTemp[iChar] = '\0';
                    strGBKT = strTemp;
                    strWidth = StringWidth(inputWindow->dpy, inputWindow->owner->xftfont, strGBKT);
                    cursor_pos= posUpX[i]
                                + strWidth + 2;
                }
                iChar -= length;
            }
        }

    }

    if (iChar >= 0)
        cursor_pos = inputWidth + MarginLeft;

    outputWidth = 0;
    outputHeight = 0;
    int currentX = 0;
    for (i = 0; i < GetMessageCount(msgdown) ; i++)
    {
        char *trans = ProcessOutputFilter(instance, GetMessageString(msgdown, i));
        if (trans)
            strDown[i] = trans;
        else
            strDown[i] = GetMessageString(msgdown, i);

        if (inputWindow->owner->bVerticalList) /* vertical */
        {
            if (GetMessageType(msgdown, i) == MSG_INDEX)
            {
                if (currentX > outputWidth)
                    outputWidth = currentX;
                if (i != 0)
                {
                    outputHeight += fontSize + 2;
                    currentX = 0;
                }
            }
            posDownX[i] = MarginLeft + currentX;
            strWidth = StringWidth(inputWindow->dpy, inputWindow->owner->xftfont, strDown[i]);
            currentX += strWidth;
            posDownY[i] =  MarginTop + OutputPos + outputHeight - strHeight;
        }
        else /* horizontal */
        {
            posDownX[i] = MarginLeft + outputWidth;
            strWidth = StringWidth(inputWindow->dpy, inputWindow->owner->xftfont, strDown[i]);
            posDownY[i] = MarginTop + OutputPos - strHeight;
            outputWidth += strWidth;
        }
    }
    if (inputWindow->owner->bVerticalList && currentX > outputWidth)
        outputWidth = currentX;

    newHeight = MarginTop + OutputPos + outputHeight + MarginBottom;

    newWidth = (inputWidth<outputWidth)?outputWidth:inputWidth;
    newWidth+=MarginLeft+MarginRight;

    /* round to ROUND_SIZE in order to decrease resize */
    newWidth =  (newWidth / ROUND_SIZE) * ROUND_SIZE + ROUND_SIZE;

    //输入条长度应该比背景图长度要长,比最大长度要短
    newWidth=(newWidth>=INPUT_BAR_MAX_WIDTH)?INPUT_BAR_MAX_WIDTH:newWidth;
    if (inputWindow->owner->bVerticalList) /* vertical */
    {
        newWidth = (newWidth < INPUT_BAR_VMIN_WIDTH)?INPUT_BAR_VMIN_WIDTH:newWidth;
    }
    else
    {
        newWidth = (newWidth < INPUT_BAR_HMIN_WIDTH)?INPUT_BAR_HMIN_WIDTH:newWidth;
    }

    *iwidth = newWidth;
    *iheight = newHeight;

    if (oldHeight != newHeight || oldWidth != newWidth)
    {
        DrawResizableBackground(inputWindow->owner, inputWindow->pixmap2, newHeight, newWidth,
                                inputWindow->owner->backcolor,
                                inputWindow->owner->bordercolor,
                                inputWindow->pixmap2_gc
                               );
    }
    XGCValues gcvalues;
    GC gc = XCreateGC(inputWindow->dpy, inputWindow->pixmap, 0, &gcvalues);
    XCopyArea(
            inputWindow->dpy,
            inputWindow->pixmap2,
            inputWindow->pixmap,
            gc,
            0, 0,
            inputWindow->iInputWindowWidth,
            inputWindow->iInputWindowHeight,
            0, 0
             );
    XFreeGC(inputWindow->dpy, gc);
    if (input->bShowCursor )
    {
        //画向前向后箭头
    }

    for (i = 0; i < GetMessageCount(msgup) ; i++)
    {
        OutputString(inputWindow->dpy, inputWindow->xftDraw, inputWindow->pixmap, inputWindow->owner->xftfont, strUp[i], posUpX[i], posUpY[i], inputWindow->owner->fontColor[GetMessageType(msgup, i)]);
        if (strUp[i] != GetMessageString(msgup, i))
            free(strUp[i]);
    }

    for (i = 0; i < GetMessageCount(msgdown) ; i++)
    {
        OutputString(inputWindow->dpy, inputWindow->xftDraw, inputWindow->pixmap, inputWindow->owner->xftfont, strDown[i], posDownX[i], posDownY[i], inputWindow->owner->fontColor[GetMessageType(msgdown, i)]);
        if (strDown[i] != GetMessageString(msgdown, i))
            free(strDown[i]);
    }

    //画光标
    if (input->bShowCursor )
    {
        GC gc = LightUICreateGC(inputWindow->dpy, inputWindow->pixmap, inputWindow->owner->cursorColor);
        XDrawLine(inputWindow->dpy, inputWindow->pixmap, gc, cursor_pos, MarginTop + InputPos,
                  cursor_pos, MarginTop + InputPos - fontSize - 4);
        XFreeGC(inputWindow->dpy, gc);
    }

    SetMessageChanged(msgup, false);
    SetMessageChanged(msgdown, false);
}


LightUIImage* LoadImage(struct _FcitxLightUI* lightui, const char* name)
{
    XImage* xpm = NULL, *mask = NULL;
    LightUIImage *image = NULL;

    HASH_FIND_STR(lightui->imageTable, name, image);
    if (image != NULL)
        return image;
    if ( strlen(name) > 0 )
    {
        int i = 0;
        while (builtInImage[i].name != NULL)
        {
            if (strcmp(builtInImage[i].name, name) == 0)
            {
                XpmAttributes   attrib;

                attrib.valuemask = 0;

                XpmCreateImageFromData (lightui->dpy, builtInImage[i].data , &xpm, &mask, &attrib);
                break;
            }
            i ++;
        }
    }

    if (xpm != NULL)
    {
        image = fcitx_malloc0(sizeof(LightUIImage));
        image->name = strdup(name);
        image->image = xpm;
        image->mask = mask;
        image->pixmask = XCreatePixmap(lightui->dpy, DefaultRootWindow(lightui->dpy), mask->width, mask->height, mask->depth);
        GC gc = XCreateGC (lightui->dpy, image->pixmask, 0, NULL);
        XPutImage(lightui->dpy, image->pixmask, gc, image->mask, 0, 0, 0, 0, mask->width, mask->height);
        XFreeGC(lightui->dpy, gc);

        HASH_ADD_KEYPTR(hh, lightui->imageTable, image->name, strlen(image->name), image);
        return image;
    }
    return NULL;
}

void DrawImage(Display* dpy, Drawable drawable, LightUIImage* image, int x, int y, int w, int h)
{
    GC gc = XCreateGC(dpy, drawable, 0, NULL);
    XSetClipMask(dpy, gc, image->pixmask);
    XSetClipOrigin(dpy, gc, x, y);
    XPutImage(dpy, drawable, gc, image->image, 0, 0, x, y, w, h);
    XFreeGC(dpy, gc);
}


// kate: indent-mode cstyle; space-indent on; indent-width 0;
