/*
** NetXMS Session Agent
** Copyright (C) 2003-2024 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be usefu,,
** but ITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: screenshot.cpp
**
**/

#include "nxsagent.h"

/**
 * Take screenshot
 */
void TakeScreenshot(NXCPMessage *response)
{
   uint32_t rcc = ERR_INTERNAL_ERROR;

   HDC dc = CreateDC(_T("DISPLAY"), nullptr, nullptr, nullptr);
   if (dc != nullptr)
   {
      HDC memdc = CreateCompatibleDC(dc);
      if (memdc != nullptr)
      {
         int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
         int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
         int cx = GetSystemMetrics(SM_CXVIRTUALSCREEN);
         int cy = GetSystemMetrics(SM_CYVIRTUALSCREEN);

         HBITMAP bitmap = CreateCompatibleBitmap(dc, cx, cy);
         if (bitmap != nullptr)
         {
            SelectObject(memdc, bitmap);
            BitBlt(memdc, 0, 0, cx, cy, dc, x, y, SRCCOPY | CAPTUREBLT);
         }

         DeleteDC(memdc);

         ByteStream *png = SaveBitmapToPng(bitmap);
         if (png != nullptr)
         {
            rcc = ERR_SUCCESS;
            response->setField(VID_FILE_DATA, png->buffer(), png->size());
            delete png;
         }
         DeleteObject(bitmap);
      }
      DeleteDC(dc);
   }
   
   response->setField(VID_RCC, rcc);
}
