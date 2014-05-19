/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2012 Victor Kirhenshtein
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
** File: debug.cpp
**
**/

#include "nxcore.h"

#ifdef _WIN32
#include <dbghelp.h>
#endif

/**
 * Test read/write lock state and print to stdout
 */
void DbgTestRWLock(RWLOCK hLock, const TCHAR *szName, CONSOLE_CTX pCtx)
{
   ConsolePrintf(pCtx, _T("  %s: "), szName);
   if (RWLockWriteLock(hLock, 100))
   {
      ConsolePrintf(pCtx, _T("unlocked\n"));
      RWLockUnlock(hLock);
   }
   else
   {
      if (RWLockReadLock(hLock, 100))
      {
         ConsolePrintf(pCtx, _T("locked for reading\n"));
         RWLockUnlock(hLock);
      }
      else
      {
         ConsolePrintf(pCtx, _T("locked for writing\n"));
      }
   }
}

/**
 * Print message to console, either local or remote
 */
void ConsolePrintf(CONSOLE_CTX pCtx, const TCHAR *pszFormat, ...)
{
   va_list args;
   TCHAR szBuffer[8192];

   va_start(args, pszFormat);
   _vsntprintf(szBuffer, 8191, pszFormat, args);
	szBuffer[8191] = 0;
   va_end(args);

	if ((pCtx->hSocket == -1) && (pCtx->session == NULL) && (pCtx->output == NULL))
   {
		WriteToTerminal(szBuffer);
   }
   else if (pCtx->output != NULL)
   {
      // remove possible escape sequences
      for(int i = 0; szBuffer[i] != 0; i++)
      {
         if (szBuffer[i] == 27)
         {
            int start = i++;
            if (szBuffer[i] == '[')
            {
               for(i++; (szBuffer[i] != 0) && (szBuffer[i] != 'm'); i++);
               if (szBuffer[i] == 'm')
                  i++;
            }
            memmove(&szBuffer[start], &szBuffer[i], (_tcslen(&szBuffer[i]) + 1) * sizeof(TCHAR));
            i = start - 1;
         }
      }

      MutexLock(pCtx->socketMutex);
      *pCtx->output += szBuffer;
      MutexUnlock(pCtx->socketMutex);
   }
   else
   {
      pCtx->pMsg->SetVariable(VID_MESSAGE, szBuffer);
		if (pCtx->session != NULL)
		{
			pCtx->session->postMessage(pCtx->pMsg);
		}
		else
		{
			CSCP_MESSAGE *pRawMsg = pCtx->pMsg->createMessage();
			SendEx(pCtx->hSocket, pRawMsg, ntohl(pRawMsg->dwSize), 0, pCtx->socketMutex);
			free(pRawMsg);
		}
   }
}

/**
 * Show server statistics
 */
static void DciCountCallback(NetObj *object, void *data)
{
	*((int *)data) += (int)((Node *)object)->getItemCount();
}

void ShowServerStats(CONSOLE_CTX pCtx)
{
	int dciCount = 0;
	g_idxNodeById.forEach(DciCountCallback, &dciCount);
   ConsolePrintf(pCtx, _T("Total number of objects:     %d\n")
                       _T("Number of monitored nodes:   %d\n")
                       _T("Number of collectable DCIs:  %d\n\n"),
	              g_idxObjectById.getSize(), g_idxNodeById.getSize(), dciCount);
}

/**
 * Show queue stats
 */
void ShowQueueStats(CONSOLE_CTX pCtx, Queue *pQueue, const TCHAR *pszName)
{
   if (pQueue != NULL)
      ConsolePrintf(pCtx, _T("%-32s : %d\n"), pszName, pQueue->Size());
}

/**
 * Write process coredump
 */
#ifdef _WIN32

void DumpProcess(CONSOLE_CTX pCtx)
{
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	char cmdLine[64];

	ConsolePrintf(pCtx, _T("Dumping process to disk...\n"));

	sprintf(cmdLine, "netxmsd.exe --dump %d", GetCurrentProcessId());
	memset(&si, 0, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	if (CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE,
	                   (g_dwFlags & AF_DAEMON) ? CREATE_NO_WINDOW : 0, NULL, NULL, &si, &pi))
	{
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		
		ConsolePrintf(pCtx, _T("Done.\n"));
	}
	else
	{
		TCHAR buffer[256];
		ConsolePrintf(pCtx, _T("Dump error: CreateProcess() failed (%s)\n"), GetSystemErrorText(GetLastError(), buffer, 256));
	}
}

#else

void DumpProcess(CONSOLE_CTX pCtx)
{
	ConsolePrintf(pCtx, _T("DUMP command is not supported for current operating system\n"));
}

#endif
