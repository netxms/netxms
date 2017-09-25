/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2016 Victor Kirhenshtein
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
** File: watchdog.cpp
**
**/

#include "nxcore.h"

/**
 * Max thread name length
 */
#define MAX_THREAD_NAME    64

/**
 * Max number of threads
 */
#define MAX_THREADS        64

/**
 * Thread list
 */
static struct
{
   TCHAR name[MAX_THREAD_NAME];
   time_t notifyInterval;
   time_t lastReport;
   WatchdogState state;
} s_threadInfo[MAX_THREADS];
static UINT32 s_numThreads = 0;
static MUTEX s_watchdogLock = INVALID_MUTEX_HANDLE;

/**
 * Add thread to watch list
 */
UINT32 WatchdogAddThread(const TCHAR *name, time_t notifyInterval)
{
   MutexLock(s_watchdogLock);
   _tcscpy(s_threadInfo[s_numThreads].name, name);
   s_threadInfo[s_numThreads].notifyInterval = notifyInterval;
   s_threadInfo[s_numThreads].lastReport = time(NULL);
   s_threadInfo[s_numThreads].state = WATCHDOG_RUNNING;
   UINT32 id = s_numThreads;
   s_numThreads++;
   MutexUnlock(s_watchdogLock);
   return id;
}

/**
 * Set thread timestamp
 */
void WatchdogNotify(UINT32 id)
{
   if (IsShutdownInProgress())
      return;

   MutexLock(s_watchdogLock);
   if (id < s_numThreads)
   {
      if (s_threadInfo[id].state == WATCHDOG_NOT_RESPONDING)
      {
         nxlog_write(MSG_THREAD_RUNNING, NXLOG_INFO, "s", s_threadInfo[id].name);
         PostEvent(EVENT_THREAD_RUNNING, g_dwMgmtNode, "s", s_threadInfo[id].name);
      }
      s_threadInfo[id].lastReport = time(NULL);
      s_threadInfo[id].state = WATCHDOG_RUNNING;
   }
   MutexUnlock(s_watchdogLock);
}

/**
 * Set thread sleep mode
 */
void WatchdogStartSleep(UINT32 id)
{
   if (IsShutdownInProgress())
      return;

   MutexLock(s_watchdogLock);
   if (id < s_numThreads)
   {
      if (s_threadInfo[id].state == WATCHDOG_NOT_RESPONDING)
      {
         nxlog_write(MSG_THREAD_RUNNING, NXLOG_INFO, "s", s_threadInfo[id].name);
         PostEvent(EVENT_THREAD_RUNNING, g_dwMgmtNode, "s", s_threadInfo[id].name);
      }
      s_threadInfo[id].lastReport = time(NULL);
      s_threadInfo[id].state = WATCHDOG_SLEEPING;
   }
   MutexUnlock(s_watchdogLock);
}

/**
 * Print current thread status
 */
void WatchdogPrintStatus(CONSOLE_CTX console)
{
   static const TCHAR *statusColors[] = { _T("32"), _T("34"), _T("31") };
   static const TCHAR *statusNames[] = { _T("Running"), _T("Sleeping"), _T("Not responding") };

   ConsolePrintf(console, _T("\x1b[1m%-48s Interval Status\x1b[0m\n----------------------------------------------------------------------------\n"), _T("Thread"));
   MutexLock(s_watchdogLock);
   for(UINT32 i = 0; i < s_numThreads; i++)
      ConsolePrintf(console, _T("%-48s %-8ld \x1b[%s;1m%s\x1b[0m\n"), s_threadInfo[i].name, (long)s_threadInfo[i].notifyInterval,
                    statusColors[s_threadInfo[i].state], statusNames[s_threadInfo[i].state]);
   MutexUnlock(s_watchdogLock);
}

/**
 * Get state of given thread
 */
WatchdogState WatchdogGetState(const TCHAR *name)
{
   WatchdogState state = WATCHDOG_UNKNOWN;

   MutexLock(s_watchdogLock);
   for(UINT32 i = 0; i < s_numThreads; i++)
      if (!_tcsicmp(s_threadInfo[i].name, name))
      {
         state = s_threadInfo[i].state;
         break;
      }
   MutexUnlock(s_watchdogLock);
   return state;
}

/**
 * Get all watchdog monitored threads
 */
void WatchdogGetThreads(StringList *out)
{
   MutexLock(s_watchdogLock);
   for(UINT32 i = 0; i < s_numThreads; i++)
      out->add(s_threadInfo[i].name);
   MutexUnlock(s_watchdogLock);
}

/**
 * Watchdog thread
 */
THREAD_RESULT THREAD_CALL WatchdogThread(void *arg)
{
   ThreadSetName("Watchdog");
   nxlog_debug(1, _T("Watchdog thread started"));
   while(!SleepAndCheckForShutdown(20))
   {
      // Walk through threads and check if they are alive
      MutexLock(s_watchdogLock);
      time_t currTime = time(NULL);
      for(UINT32 i = 0; i < s_numThreads; i++)
         if ((currTime - s_threadInfo[i].lastReport > s_threadInfo[i].notifyInterval) &&
             (s_threadInfo[i].state == WATCHDOG_RUNNING))
         {
            PostEvent(EVENT_THREAD_HANGS, g_dwMgmtNode, "s", s_threadInfo[i].name);
            nxlog_write(MSG_THREAD_NOT_RESPONDING, NXLOG_ERROR, "s", s_threadInfo[i].name);
            s_threadInfo[i].state = WATCHDOG_NOT_RESPONDING;
         }
      MutexUnlock(s_watchdogLock);
   }

   nxlog_debug(1, _T("Watchdog thread terminated"));
   return THREAD_OK;
}

/**
 * Watchdog thread handle
 */
static THREAD s_watchdogThread = INVALID_THREAD_HANDLE;

/**
 * Initialize watchdog
 */
void WatchdogInit()
{
   s_watchdogLock = MutexCreate();
   s_watchdogThread = ThreadCreateEx(WatchdogThread, 0, NULL);
}

/**
 * Shutdown watchdog
 */
void WatchdogShutdown()
{
   ThreadJoin(s_watchdogThread);
   MutexDestroy(s_watchdogLock);
}
