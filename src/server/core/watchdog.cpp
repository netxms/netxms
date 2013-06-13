/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: watchdog.cpp
**
**/

#include "nxcore.h"


//
// Constants
//

#define MAX_THREAD_NAME    64
#define MAX_THREADS        64


//
// Static data
//

static struct
{
   TCHAR szName[MAX_THREAD_NAME];
   time_t tNotifyInterval;
   time_t tLastCheck;
   BOOL bNotResponding;
} m_threadInfo[MAX_THREADS];
static UINT32 m_dwNumThreads = 0;
static MUTEX m_mutexWatchdogAccess = INVALID_MUTEX_HANDLE;


//
// Add thread to watch list
//

UINT32 WatchdogAddThread(const TCHAR *szName, time_t tNotifyInterval)
{
   UINT32 dwId;

   MutexLock(m_mutexWatchdogAccess);
   _tcscpy(m_threadInfo[m_dwNumThreads].szName, szName);
   m_threadInfo[m_dwNumThreads].tNotifyInterval = tNotifyInterval;
   m_threadInfo[m_dwNumThreads].tLastCheck = time(NULL);
   m_threadInfo[m_dwNumThreads].bNotResponding = FALSE;
   dwId = m_dwNumThreads;
   m_dwNumThreads++;
   MutexUnlock(m_mutexWatchdogAccess);
   return dwId;
}


//
// Initialize watchdog
//

void WatchdogInit()
{
   m_mutexWatchdogAccess = MutexCreate();
}


//
// Set thread timestamp
//

void WatchdogNotify(UINT32 dwId)
{
   if (IsShutdownInProgress())
      return;

   MutexLock(m_mutexWatchdogAccess);
   if (dwId < m_dwNumThreads)
   {
      if (m_threadInfo[dwId].bNotResponding)
         PostEvent(EVENT_THREAD_RUNNING, g_dwMgmtNode, "s", m_threadInfo[dwId].szName);
      m_threadInfo[dwId].tLastCheck = time(NULL);
      m_threadInfo[dwId].bNotResponding = FALSE;
   }
   MutexUnlock(m_mutexWatchdogAccess);
}


//
// Print current thread status
//

void WatchdogPrintStatus(CONSOLE_CTX pCtx)
{
   UINT32 i;

   ConsolePrintf(pCtx, _T("\x1b[1m%-48s Interval Status\x1b[0m\n----------------------------------------------------------------------------\n"), _T("Thread"));
   MutexLock(m_mutexWatchdogAccess);
   for(i = 0; i < m_dwNumThreads; i++)
      ConsolePrintf(pCtx, _T("%-48s %-8ld \x1b[%s;1m%s\x1b[0m\n"), m_threadInfo[i].szName, (long)m_threadInfo[i].tNotifyInterval,
		              (m_threadInfo[i].bNotResponding ? _T("31") : _T("32")),
						  (m_threadInfo[i].bNotResponding ? _T("Not responding") : _T("Running")));
   MutexUnlock(m_mutexWatchdogAccess);
}


//
// Watchdog thread
//

THREAD_RESULT THREAD_CALL WatchdogThread(void *arg)
{
   UINT32 i;
   time_t currTime;

   while(!IsShutdownInProgress())
   {
      if (SleepAndCheckForShutdown(20))
         break;      // Shutdown has arrived

      // Walk through threads and check if they are alive
      MutexLock(m_mutexWatchdogAccess);
      currTime = time(NULL);
      for(i = 0; i < m_dwNumThreads; i++)
         if ((currTime - m_threadInfo[i].tLastCheck > m_threadInfo[i].tNotifyInterval) &&
             (!m_threadInfo[i].bNotResponding))
         {
            PostEvent(EVENT_THREAD_HANGS, g_dwMgmtNode, "s", m_threadInfo[i].szName);
            nxlog_write(MSG_THREAD_HANGS, EVENTLOG_ERROR_TYPE, "s", m_threadInfo[i].szName);
            m_threadInfo[i].bNotResponding = TRUE;
         }
      MutexUnlock(m_mutexWatchdogAccess);
   }

   MutexDestroy(m_mutexWatchdogAccess);
   m_mutexWatchdogAccess = NULL;
   DbgPrintf(1, _T("Watchdog thread terminated"));
   return THREAD_OK;
}
