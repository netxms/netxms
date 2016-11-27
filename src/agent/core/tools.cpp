/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2016 Victor Kirhenshtein
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
** File: tools.cpp
**
**/

#include "nxagentd.h"
#include <stdarg.h>

/**
 * Print message to the console if allowed to do so
 */
void ConsolePrintf(const TCHAR *format, ...)
{
   if (!(g_dwFlags & AF_DAEMON))
   {
      va_list args;

      va_start(args, format);
      _vtprintf(format, args);
      va_end(args);
   }
}

/**
 * Print debug messages
 */
void DebugPrintf(int level, const TCHAR *format, ...)
{
   if (level > nxlog_get_debug_level())
      return;

   va_list args;
   TCHAR buffer[4096];

   va_start(args, format);
   _vsntprintf(buffer, 4096, format, args);
   va_end(args);

   nxlog_write(MSG_DEBUG, EVENTLOG_DEBUG_TYPE, "s", buffer);
}

/**
 * Build full path for file in file store
 */
void BuildFullPath(TCHAR *pszFileName, TCHAR *pszFullPath)
{
   int i, nLen;

   // Strip path from original file name, if any
   for(i = (int)_tcslen(pszFileName) - 1;
       (i >= 0) && (pszFileName[i] != '\\') && (pszFileName[i] != '/'); i--);

   // Create full path to the file store
   _tcscpy(pszFullPath, g_szFileStore);
   nLen = (int)_tcslen(pszFullPath);
   if ((pszFullPath[nLen - 1] != '\\') &&
       (pszFullPath[nLen - 1] != '/'))
   {
      _tcscat(pszFullPath, FS_PATH_SEPARATOR);
      nLen++;
   }
   nx_strncpy(&pszFullPath[nLen], &pszFileName[i + 1], MAX_PATH - nLen);
}

/**
 * Wait for specific process
 */
bool WaitForProcess(const TCHAR *name)
{
	TCHAR param[MAX_PATH], value[MAX_RESULT_LENGTH];
	bool success = false;
	VirtualSession session(0);
	UINT32 rc;

	_sntprintf(param, MAX_PATH, _T("Process.Count(%s)"), name);
	while(true)
	{
		rc = GetParameterValue(param, value, &session);
		switch(rc)
		{
			case ERR_SUCCESS:
				if (_tcstol(value, NULL, 0) > 0)
				{
					success = true;
					goto done;
				}
				break;
			case ERR_UNKNOWN_PARAMETER:
				goto done;
			default:
				break;
		}
		ThreadSleep(1);
	}

done:
	return success;
}


/**********************************************************
 Following functions are Windows specific
**********************************************************/

#ifdef _WIN32

/**
 * Get error text for PDH functions
 */
TCHAR *GetPdhErrorText(DWORD dwError, TCHAR *pszBuffer, int iBufSize)
{
   TCHAR *msgBuf;

   if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                     FORMAT_MESSAGE_FROM_HMODULE |
                     FORMAT_MESSAGE_IGNORE_INSERTS,
                     GetModuleHandle(_T("PDH.DLL")), dwError,
                     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                     (LPTSTR)&msgBuf, 0, NULL)>0)
   {
      msgBuf[_tcscspn(msgBuf, _T("\r\n"))] = 0;
      nx_strncpy(pszBuffer, msgBuf, iBufSize);
      LocalFree(msgBuf);
   }
   else
   {
      GetSystemErrorText(dwError, pszBuffer, iBufSize);
   }
   return pszBuffer;
}

#endif   /* _WIN32 */
