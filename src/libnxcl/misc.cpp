/* 
** NetXMS - Network Management System
** Client Library
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
** $module: misc.cpp
**
**/

#include "libnxcl.h"
#include <stdarg.h>


//
// Change library state and notify client
//

void ChangeState(DWORD dwState)
{
   g_dwState = dwState;
   CallEventHandler(NXC_EVENT_STATE_CHANGED, dwState, NULL);
}


//
// Print debug messages
//

void DebugPrintf(char *szFormat, ...)
{
   va_list args;
   char szBuffer[4096];

   if (g_pDebugCallBack == NULL)
      return;

   va_start(args, szFormat);
   vsprintf(szBuffer, szFormat, args);
   va_end(args);
   g_pDebugCallBack(szBuffer);
}
