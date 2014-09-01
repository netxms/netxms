/*
** NetXMS Session Agent
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
void TakeScreenshot(CSCPMessage *response)
{
   UINT32 rcc = ERR_INTERNAL_ERROR;

   HDC dc = GetDC(NULL);
   if (dc != NULL)
   {
      HDC memdc = CreateCompatibleDC(dc);
      if (memdc != NULL)
      {
         RECT desktopRect;
         GetWindowRect(GetDesktopWindow(), &desktopRect);
         int cx = desktopRect.right - desktopRect.left;
         int cy = desktopRect.bottom - desktopRect.top;

         HBITMAP bitmap = CreateCompatibleBitmap(dc, cx, cy);
         if (bitmap != NULL)
         {
            SelectObject(memdc, bitmap);
            BitBlt(memdc, 0, 0, cx, cy, dc, 0, 0, SRCCOPY);
         }

         DeleteDC(memdc);

         TCHAR tempPath[MAX_PATH];
         GetTempPath(MAX_PATH, tempPath);

         TCHAR tempFile[MAX_PATH];
         GetTempFileName(tempPath, _T("nx"), 0, tempFile);
         if (SaveBitmapToPng(bitmap, tempFile))
         {
            rcc = ERR_SUCCESS;
            response->setFieldFromFile(VID_FILE_DATA, tempFile);
         }
         else
         {
            AgentWriteDebugLog(4, _T("Cannot save bitmap as PNG"));
         }
         DeleteObject(bitmap);
         DeleteFile(tempFile);
      }
      ReleaseDC(NULL, dc);
   }
   
   response->SetVariable(VID_RCC, rcc);
}
