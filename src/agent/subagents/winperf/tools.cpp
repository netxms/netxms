/*
** Windows Performance NetXMS subagent
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
**
**/

#include "winperf.h"


//
// Get error text for PDH functions
//

TCHAR *GetPdhErrorText(DWORD dwError, TCHAR *pszBuffer, int iBufferSize)
{
   TCHAR *pszMsg;

   if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                     FORMAT_MESSAGE_FROM_HMODULE | 
                     FORMAT_MESSAGE_IGNORE_INSERTS,
                     GetModuleHandle(_T("PDH.DLL")), dwError,
                     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                     (LPTSTR)&pszMsg, 0, NULL)>0)
   {
      TranslateStr(pszMsg, _T("\r\n"), _T(""));
      _tcsncpy(pszBuffer, pszMsg, iBufferSize);
      LocalFree(pszMsg);
   }
   else
   {
      GetSystemErrorText(dwError, pszBuffer, iBufferSize);
   }
   return pszBuffer;
}


//
// Report PDH error to master agent's log
//

void ReportPdhError(TCHAR *pszFunction, TCHAR *pszPdhCall, PDH_STATUS dwError)
{
   TCHAR szBuffer[1024];

   NxWriteAgentLog(EVENTLOG_WARNING_TYPE, _T("%s: PDH Error %08X in call to %s (%s)"), 
                   pszFunction, dwError, pszPdhCall, GetPdhErrorText(dwError, szBuffer, 1024));
}
