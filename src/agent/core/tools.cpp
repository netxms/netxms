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


//
// Print message to the console if allowed to do so
//

void ConsolePrintf(char *szFormat, ...)
{
   if (!(g_dwFlags & AF_DAEMON))
   {
      va_list args;

      va_start(args, szFormat);
      vprintf(szFormat, args);
      va_end(args);
   }
}


/**********************************************************
 Following functions are Windows specific
**********************************************************/

#ifdef _WIN32


//
// Get system error string by call to FormatMessage
//

char *GetSystemErrorText(DWORD dwError)
{
   char *msgBuf;
   static char staticBuffer[1024];

   if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                     FORMAT_MESSAGE_FROM_SYSTEM | 
                     FORMAT_MESSAGE_IGNORE_INSERTS,
                     NULL, dwError,
                     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                     (LPSTR)&msgBuf, 0, NULL) > 0)
   {
      msgBuf[strcspn(msgBuf, "\r\n")] = 0;
      strncpy(staticBuffer, msgBuf, 1023);
      LocalFree(msgBuf);
   }
   else
   {
      sprintf(staticBuffer, "MSG 0x%08X - Unable to find message text", dwError);
   }

   return staticBuffer;
}


//
// Get error text for PDH functions
//

char *GetPdhErrorText(DWORD dwError)
{
   char *msgBuf;
   static char staticBuffer[1024];

   if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                     FORMAT_MESSAGE_FROM_HMODULE | 
                     FORMAT_MESSAGE_IGNORE_INSERTS,
                     GetModuleHandle("PDH.DLL"), dwError,
                     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                     (LPSTR)&msgBuf, 0, NULL)>0)
   {
      msgBuf[strcspn(msgBuf, "\r\n")] = 0;
      strncpy(staticBuffer, msgBuf, 1023);
      LocalFree(msgBuf);
      return staticBuffer;
   }
   else
   {
      return GetSystemErrorText(dwError);
   }
}

#endif   /* _WIN32 */
