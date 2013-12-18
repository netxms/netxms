/* 
** NetXMS client proxy
** Copyright (C) 2003-2013 Victor Kirhenshtein
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

#include "nxcproxy.h"

/**
 * Print message to the console if allowed to do so
 */
void ConsolePrintf(const TCHAR *pszFormat, ...)
{
   if (!(g_flags & AF_DAEMON))
   {
      va_list args;

      va_start(args, pszFormat);
      _vtprintf(pszFormat, args);
      va_end(args);
   }
}

/**
 * Print debug messages
 */
void DebugPrintf(int level, const TCHAR *pszFormat, ...)
{
   if (level <= (int)g_debugLevel)
   {
      va_list args;
      TCHAR szBuffer[4096];

      va_start(args, pszFormat);
      _vsntprintf(szBuffer, 4096, pszFormat, args);
      va_end(args);
      
      nxlog_write(MSG_DEBUG, EVENTLOG_DEBUG_TYPE, "s", szBuffer);
   }
}
