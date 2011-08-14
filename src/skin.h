/***************************************************************************
 *   Copyright (C) 2009-2010 by t3swing                                    *
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
 * @file   skin.h
 * @author t3swing  t3swing@sina.com
 *
 * @date   2009-10-9
 *
 * @brief  皮肤设置相关定义及初始化加载工作
 *
 *
 */

#ifndef _SKIN_H
#define _SKIN_H

#define SIZEX 800
#define SIZEY 200
#include <X11/Xlib.h>
#include "fcitx-utils/uthash.h"
#include "fcitx-config/fcitx-config.h"
#include "fcitx/ui.h"

struct _XlibMenu;
struct _InputWindow;
struct _Messages;
struct _FcitxLightUI;

typedef struct _LightUIImage
{
    char *name;
    XImage *image;
    UT_hash_handle hh;
    XImage* mask;
    Pixmap pixmask;
} LightUIImage;

void DrawInputBar(struct _InputWindow* inputWindow, int cursorPos, struct _Messages * msgup, struct _Messages *msgdown ,unsigned int * iheight, unsigned int *iwidth);
void LoadInputMessage(struct _InputWindow* inputWindow, const char* font);
void DrawImage(Display* dpy, Drawable drawable, LightUIImage* image, int x, int y, int w, int h);
void ParsePlacement(UT_array* sps, char* placment);
void DrawResizableBackground(struct _FcitxLightUI* lightui,
                             Drawable drawable,
                             int height,
                             int width,
                             ConfigColor background,
                             ConfigColor border,
                             GC gc
                            );

LightUIImage* LoadImage(struct _FcitxLightUI* lightui, const char* name);

#endif



// kate: indent-mode cstyle; space-indent on; indent-width 0;
