/* 
** NetXMS - Network Management System
** Windows Console
** Copyright (C) 2004 Victor Kirhenshtein
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
** $module: tools.cpp
** Various tools and helper functions
**
**/

#include "stdafx.h"
#include "nxcon.h"


//
// Format time stamp
//

char *FormatTimeStamp(DWORD dwTimeStamp, char *pszBuffer, int iType)
{
   struct tm *pTime;
   static char *pFormat[] = { "%d-%b-%Y %H:%M:%S", "%H:%M:%S" };

   pTime = localtime((const time_t *)&dwTimeStamp);
   strftime(pszBuffer, 32, pFormat[iType], pTime);
   return pszBuffer;
}


//
// Get size of a window
//

CSize GetWindowSize(CWnd *pWnd)
{
   RECT rect;
   CSize size;

   pWnd->GetWindowRect(&rect);
   size.cx = rect.right - rect.left + 1;
   size.cy = rect.bottom - rect.top + 1;
   return size;
}
