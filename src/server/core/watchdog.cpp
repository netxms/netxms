/* 
** Project X - Network Management System
** Copyright (C) 2003 Victor Kirhenshtein
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
   time_t tLastCheck;
   BOOL bNotResponding;
} m_threadInfo[MAX_THREADS];
static DWORD m_dwNumThreads = 0;
static MUTEX m_hWatchdogAccess;


//
// Add thread to watch list
//

DWORD WatchdogAddThread(char *szName)
{
   DWORD dwId;

   MutexLock(m_hWatchdogAccess, INFINITE);
   strcpy(m_threadInfo[m_dwNumThreads].szName, szName);
   m_threadInfo[m_dwNumThreads].tLastCheck = time(NULL);
   m_threadInfo[m_dwNumThreads].bNotResponding = FALSE;
   dwId = m_dwNumThreads;
   m_dwNumThreads++;
   MutexUnlock(m_hWatchdogAccess);
   return dwId;
}


//
// Initialize watchdog
//

void WatchdogInit(void)
{
   m_hWatchdogAccess = MutexCreate();
}


//
// Set thread timestamp
//

void WatchdogNotify(DWORD dwId)
{
   MutexLock(m_hWatchdogAccess, INFINITE);
   if (dwId < m_dwNumThreads)
   {
      m_threadInfo[dwId].tLastCheck = time(NULL);
      m_threadInfo[dwId].bNotResponding = FALSE;
   }
   MutexUnlock(m_hWatchdogAccess);
}


//
// Print current thread status
//

void WatchdogPrintStatus(void)
{
   DWORD i;

   if (!IsStandalone())
      return;

   printf("%-48s Status\n----------------------------------------------------------------------------\n", "Thread");
   MutexLock(m_hWatchdogAccess, INFINITE);
   for(i = 0; i < m_dwNumThreads; i++)
      printf("%-48s %s\n", m_threadInfo[i].szName, 
             m_threadInfo[i].bNotResponding ? "Not responding" : "Running");
   MutexUnlock(m_hWatchdogAccess);
}


//
// Watchdog thread
//

void WatchdogThread(void *arg)
{
   DWORD i;
   time_t currTime;

   while(!ShutdownInProgress())
   {
      if (SleepAndCheckForShutdown(10))
         break;      // Shutdown has arrived

      // Walk through threads and check if they are alive
      MutexLock(m_hWatchdogAccess, INFINITE);
      currTime = time(NULL);
      for(i = 0; i < m_dwNumThreads; i++)
         if ((currTime - m_threadInfo[i].tLastCheck > 60) &&
             (!m_threadInfo[i].bNotResponding))
         {
            PostEvent(EVENT_THREAD_HANGS, g_dwMgmtNode, "s", m_threadInfo[i].szName);
            WriteLog(MSG_THREAD_HANGS, EVENTLOG_ERROR_TYPE, "s", m_threadInfo[i].szName);
            m_threadInfo[i].bNotResponding = TRUE;
         }
      MutexUnlock(m_hWatchdogAccess);
   }
}
