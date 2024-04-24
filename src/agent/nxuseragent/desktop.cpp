/*
** NetXMS user agent
** Copyright (C) 2009-2024 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: desktop.cpp
**/

#include "nxuseragent.h"

/**
 * Draw message on desktop
 */
void DrawTextOnDesktop(const TCHAR *text)
{
   HDC hdc = GetDC(NULL);

   RECT rect;
   rect.left = 10;
   rect.top = 10;
   rect.right = 200;
   rect.bottom = 200;
   DrawText(hdc, text, static_cast<int>(_tcslen(text)), &rect, DT_LEFT | DT_NOPREFIX);

   ReleaseDC(NULL, hdc);
}

/**
 * Update desktop wallpaper
 */
void UpdateDesktopWallpaper()
{
   if (g_desktopWallpaper == nullptr)
      return;

   TCHAR wallpaper[MAX_PATH];
   ExpandEnvironmentStrings(g_desktopWallpaper, wallpaper, MAX_PATH);

   nxlog_debug(2, _T("Setting desktop wallpaper to \"%s\""), wallpaper);
   SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, wallpaper, SPIF_SENDCHANGE);
}
