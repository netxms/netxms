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

#include "nms_core.h"


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
   char szName[MAX_THREAD_NAME];
   time_t tNotifyInterval;
   time_t tLastCheck;
   BOOL bNotResponding;
} m_threadInfo[MAX_THREADS];
static DWORD m_dwNumThreads = 0;
static MUTEX m_mutexWatchdogAccess;


//
// Add thread to watch list
//

DWORD WatchdogAddThread(char *szName, time_t tNotifyInterval)
{
   DWORD dwId;

   MutexLock(m_mutexWatchdogAccess, INFINITE);
   strcpy(m_threadInfo[m_dwNumThreads].szName, szName);
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

void WatchdogInit(void)
{
   m_mutexWatchdogAccess = MutexCreate();
}


//
// Set thread timestamp
//

void WatchdogNotify(DWORD dwId)
{
   if (ShutdownInProgress())
      return;

   MutexLock(m_mutexWatchdogAccess, INFINITE);
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

void WatchdogPrintStatus(void)
{
   DWORD i;

   if (!IsStandalone())
      return;

   printf("%-48s Interval Status\n----------------------------------------------------------------------------\n", "Thread");
   MutexLock(m_mutexWatchdogAccess, INFINITE);
   for(i = 0; i < m_dwNumThreads; i++)
      printf("%-48s %-8ld %s\n", m_threadInfo[i].szName, m_threadInfo[i].tNotifyInterval,
             m_threadInfo[i].bNotResponding ? "Not responding" : "Running");
   MutexUnlock(m_mutexWatchdogAccess);
}


//
// Watchdog thread
//

THREAD_RESULT THREAD_CALL WatchdogThread(void *arg)
{
   DWORD i;
   time_t currTime;

   while(!ShutdownInProgress())
   {
      if (SleepAndCheckForShutdown(20))
         break;      // Shutdown has arrived

      // Walk through threads and check if they are alive
      MutexLock(m_mutexWatchdogAccess, INFINITE);
      currTime = time(NULL);
      for(i = 0; i < m_dwNumThreads; i++)
         if ((currTime - m_threadInfo[i].tLastCheck > m_threadInfo[i].tNotifyInterval) &&
             (!m_threadInfo[i].bNotResponding))
         {
            PostEvent(EVENT_THREAD_HANGS, g_dwMgmtNode, "s", m_threadInfo[i].szName);
            WriteLog(MSG_THREAD_HANGS, EVENTLOG_ERROR_TYPE, "s", m_threadInfo[i].szName);
            m_threadInfo[i].bNotResponding = TRUE;
         }
      MutexUnlock(m_mutexWatchdogAccess);
   }

   MutexDestroy(m_mutexWatchdogAccess);
   m_mutexWatchdogAccess = NULL;
   DbgPrintf(AF_DEBUG_MISC, "Watchdog thread terminated\n");
   return THREAD_OK;
}
