/* $Id: tools.cpp,v 1.1 2007-05-07 11:35:42 victor Exp $ */
/* 
** NetXMS - Network Management System
** HTTP Server
** Copyright (C) 2006, 2007 Alex Kirhenshtein and Victor Kirhenshtein
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
** File: tools.cpp
**
**/

#include "nxhttpd.h"


//
// Print debug messages
//

void DebugPrintf(DWORD dwSessionId, char *pszFormat, ...)
{
   if (g_dwFlags & AF_DEBUG)
   {
      va_list args;
      char szBuffer[4096];

      va_start(args, pszFormat);
      _vsntprintf(szBuffer, 4096, pszFormat, args);
      va_end(args);
      
      if (dwSessionId != INVALID_INDEX)
         WriteLog(MSG_DEBUG_SESSION, EVENTLOG_INFORMATION_TYPE, "ds", dwSessionId, szBuffer);
      else
         WriteLog(MSG_DEBUG, EVENTLOG_INFORMATION_TYPE, "s", szBuffer);
   }
}
