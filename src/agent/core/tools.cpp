/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: tools.cpp
**
**/

#include "nxagentd.h"
#include <stdarg.h>


//
// Print message to the console if allowed to do so
//

void ConsolePrintf(char *pszFormat, ...)
{
   if (!(g_dwFlags & AF_DAEMON))
   {
      va_list args;

      va_start(args, pszFormat);
      vprintf(pszFormat, args);
      va_end(args);
   }
}


//
// Print debug messages
//

void DebugPrintf(char *pszFormat, ...)
{
   if (g_dwFlags & AF_DEBUG)
   {
      va_list args;
      char szBuffer[1024];

      va_start(args, pszFormat);
      vsnprintf(szBuffer, 1024, pszFormat, args);
      va_end(args);
      
      WriteLog(MSG_DEBUG, EVENTLOG_INFORMATION_TYPE, "s", szBuffer);
   }
}


/**********************************************************
 Following functions are Windows specific
**********************************************************/

#ifdef _WIN32

//
// Get error text for PDH functions
//

char *GetPdhErrorText(DWORD dwError, char *pszBuffer, int iBufSize)
{
   char *msgBuf;

   if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                     FORMAT_MESSAGE_FROM_HMODULE | 
                     FORMAT_MESSAGE_IGNORE_INSERTS,
                     GetModuleHandle("PDH.DLL"), dwError,
                     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                     (LPSTR)&msgBuf, 0, NULL)>0)
   {
      msgBuf[strcspn(msgBuf, "\r\n")] = 0;
      strncpy(pszBuffer, msgBuf, iBufSize);
      LocalFree(msgBuf);
   }
   else
   {
      GetSystemErrorText(dwError, pszBuffer, iBufSize);
   }
   return pszBuffer;
}

#endif   /* _WIN32 */
