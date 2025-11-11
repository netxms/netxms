/*
** NetXMS multiplatform core agent
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
** File: tools.cpp
**
**/

#include "nxagentd.h"
#include <netxms-version.h>

/**
 * Check for VNC server on given address
 */
bool IsVNCServerRunning(const InetAddress& addr, uint16_t port)
{
   SOCKET s = ConnectToHost(addr, port, 1000);
   if (s == INVALID_SOCKET)
      return false;

   char buffer[32];
   ssize_t bytes = RecvEx(s, buffer, 31, 0, 1000);
   shutdown(s, SHUT_RDWR);
   closesocket(s);

   if (bytes < 12)
      return false;  // Should be at least 12 bytes - "RFB xxx.yyy\n"

   buffer[bytes] = 0;
   return RegexpMatchA(buffer, "^RFB [0-9]{3}\\.[0-9]{3}", true);
}

/**
 * Upgrade agent from given package file
 */
uint32_t UpgradeAgent(const TCHAR *pkgFile)
{
   TCHAR cmdLine[1024];
#if defined(_WIN32)
   _sntprintf(cmdLine, 1024, _T("\"%s\" /VERYSILENT /SUPPRESSMSGBOXES /LOG /FORCECLOSEAPPLICATIONS /NORESTART"), pkgFile);
#else
   _tchmod(pkgFile, 0700);   // Set execute permissions on package file
   _sntprintf(cmdLine, 1024, _T("\"%s\" version=") NETXMS_VERSION_STRING
                             _T(" prefix=") PREFIX
#ifdef __DISABLE_ICONV
                             _T(" opt=--disable-iconv")
#endif
                             _T(" config=%s"), pkgFile, g_szConfigFile);
#endif
   return ProcessExecutor::execute(cmdLine) ? ERR_SUCCESS : ERR_EXEC_FAILED;
}

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
 * Print message to the console if allowed to do so
 */
void ConsolePrintf2(const TCHAR *format, va_list args)
{
   if (!(g_dwFlags & AF_DAEMON))
      _vtprintf(format, args);
}

/**
 * Print debug messages
 */
void DebugPrintf(int level, const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   nxlog_debug2(level, format, args);
   va_end(args);
}

/**
 * Build full path for file in file store
 */
void BuildFullPath(const TCHAR *pszFileName, TCHAR *pszFullPath)
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
   _tcslcpy(&pszFullPath[nLen], &pszFileName[i + 1], MAX_PATH - nLen);
}

/**
 * Wait for specific process
 */
bool WaitForProcess(const TCHAR *name)
{
	TCHAR param[MAX_PATH], value[MAX_RESULT_LENGTH];
	bool success = false;
	VirtualSession session(0);

	_sntprintf(param, MAX_PATH, _T("Process.Count(%s)"), name);
	while(true)
	{
		uint32_t rc = GetMetricValue(param, value, &session);
		switch(rc)
		{
			case ERR_SUCCESS:
				if (_tcstol(value, nullptr, 0) > 0)
				{
					success = true;
					goto done;
				}
				break;
			case ERR_UNKNOWN_METRIC:
         case ERR_UNSUPPORTED_METRIC:
				goto done;
			default:
				break;
		}
		ThreadSleep(1);
	}

done:
	return success;
}

/**
 * Substitute command arguments
 */
String SubstituteCommandArguments(const TCHAR *cmdTemplate, const TCHAR *param)
{
   StringBuffer cmdLine;
   cmdLine.setAllocationStep(1024);

   for (const TCHAR *sptr = cmdTemplate; *sptr != 0; sptr++)
   {
      if (*sptr == _T('$'))
      {
         sptr++;
         if (*sptr == 0)
            break;   // Single $ character at the end of line
         if ((*sptr >= _T('1')) && (*sptr <= _T('9')))
         {
            TCHAR buffer[1024];
            if (AgentGetParameterArg(param, *sptr - '0', buffer, 1024))
            {
               cmdLine.append(buffer);
            }
         }
         else
         {
            cmdLine.append(*sptr);
         }
      }
      else
      {
         cmdLine.append(*sptr);
      }
   }
   return cmdLine;
}
/**
 * Check if command is parameterized (contains $1, $2, etc.)
 */
bool IsParametrizedCommand(const TCHAR *command)
{
   bool isParameterized = false;
   while (true)
   {
      const TCHAR *p = _tcschr(command, _T('$'));
      if (p == nullptr)
         break;
      p++;
      if ((*p >= '1') && (*p <= '9'))
      {
         isParameterized = true;
         break;
      }
      command = p;
   }
   return isParameterized;
}

/**
 * Set time of local system
 */
void SetLocalSystemTime(int64_t newTime)
{
#ifdef _WIN32
   LARGE_INTEGER li;
   li.QuadPart = newTime * 10000 + EPOCHFILETIME;
   FILETIME ft;
   ft.dwLowDateTime = li.LowPart;
   ft.dwHighDateTime = li.HighPart;
   SYSTEMTIME st;
   FileTimeToSystemTime(&ft, &st);
   SetSystemTime(&st);
#else
   struct timeval tv;
   tv.tv_sec = newTime / 1000;
   tv.tv_usec = (newTime % 1000) * 1000;
   settimeofday(&tv, nullptr);
#endif
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
      _tcslcpy(pszBuffer, msgBuf, iBufSize);
      LocalFree(msgBuf);
   }
   else
   {
      GetSystemErrorText(dwError, pszBuffer, iBufSize);
   }
   return pszBuffer;
}

#endif   /* _WIN32 */
