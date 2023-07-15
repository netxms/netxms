/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2021 Victor Kirhenshtein
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

#define DEBUG_TAG_WATCHDOG _T("watchdog")

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
static uint32_t s_numThreads = 0;
static Mutex s_watchdogMutex(MutexType::FAST);

/**
 * Add thread to watch list
 */
uint32_t WatchdogAddThread(const TCHAR *name, time_t notifyInterval)
{
   s_watchdogMutex.lock();
   _tcscpy(s_threadInfo[s_numThreads].name, name);
   s_threadInfo[s_numThreads].notifyInterval = notifyInterval;
   s_threadInfo[s_numThreads].lastReport = time(nullptr);
   s_threadInfo[s_numThreads].state = WATCHDOG_RUNNING;
   uint32_t id = s_numThreads;
   s_numThreads++;
   s_watchdogMutex.unlock();
   return id;
}

/**
 * Set thread timestamp
 */
void WatchdogNotify(uint32_t id)
{
   if (IsShutdownInProgress())
      return;

   s_watchdogMutex.lock();
   if (id < s_numThreads)
   {
      if (s_threadInfo[id].state == WATCHDOG_NOT_RESPONDING)
      {
         nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_WATCHDOG, _T("Thread \"%s\" returned to running state"), s_threadInfo[id].name);
         EventBuilder(EVENT_THREAD_RUNNING, g_dwMgmtNode)
            .param(_T("threadName"), s_threadInfo[id].name)
            .post();
      }
      s_threadInfo[id].lastReport = time(nullptr);
      s_threadInfo[id].state = WATCHDOG_RUNNING;
   }
   s_watchdogMutex.unlock();
}

/**
 * Set thread sleep mode
 */
void WatchdogStartSleep(uint32_t id)
{
   if (IsShutdownInProgress())
      return;

   s_watchdogMutex.lock();
   if (id < s_numThreads)
   {
      if (s_threadInfo[id].state == WATCHDOG_NOT_RESPONDING)
      {
         nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_WATCHDOG, _T("Thread \"%s\" returned to running state"), s_threadInfo[id].name);
         EventBuilder(EVENT_THREAD_RUNNING, g_dwMgmtNode)
            .param(_T("threadName"), s_threadInfo[id].name)
            .post();
      }
      s_threadInfo[id].lastReport = time(nullptr);
      s_threadInfo[id].state = WATCHDOG_SLEEPING;
   }
   s_watchdogMutex.unlock();
}

/**
 * Print current thread status
 */
void WatchdogPrintStatus(CONSOLE_CTX console)
{
   static const TCHAR *statusColors[] = { _T("32"), _T("34"), _T("31") };
   static const TCHAR *statusNames[] = { _T("Running"), _T("Sleeping"), _T("Not responding") };

   ConsolePrintf(console, _T("\x1b[1m%-48s Interval Status\x1b[0m\n----------------------------------------------------------------------------\n"), _T("Thread"));
   s_watchdogMutex.lock();
   for(uint32_t i = 0; i < s_numThreads; i++)
      ConsolePrintf(console, _T("%-48s %-8ld \x1b[%s;1m%s\x1b[0m\n"), s_threadInfo[i].name, (long)s_threadInfo[i].notifyInterval,
                    statusColors[s_threadInfo[i].state], statusNames[s_threadInfo[i].state]);
   s_watchdogMutex.unlock();
}

/**
 * Get state of given thread
 */
WatchdogState WatchdogGetState(const TCHAR *name)
{
   WatchdogState state = WATCHDOG_UNKNOWN;

   s_watchdogMutex.lock();
   for(uint32_t i = 0; i < s_numThreads; i++)
      if (!_tcsicmp(s_threadInfo[i].name, name))
      {
         state = s_threadInfo[i].state;
         break;
      }
   s_watchdogMutex.unlock();
   return state;
}

/**
 * Get all watchdog monitored threads
 */
void WatchdogGetThreads(StringList *out)
{
   s_watchdogMutex.lock();
   for(UINT32 i = 0; i < s_numThreads; i++)
      out->add(s_threadInfo[i].name);
   s_watchdogMutex.unlock();
}

/**
 * Watchdog thread
 */
static void WatchdogThread()
{
   ThreadSetName("Watchdog");
   nxlog_debug_tag(DEBUG_TAG_WATCHDOG, 1, _T("Watchdog thread started"));

   while(!SleepAndCheckForShutdown(20))
   {
      // Walk through threads and check if they are alive
      s_watchdogMutex.lock();
      time_t currTime = time(nullptr);
      for(UINT32 i = 0; i < s_numThreads; i++)
         if ((currTime - s_threadInfo[i].lastReport > s_threadInfo[i].notifyInterval) &&
             (s_threadInfo[i].state == WATCHDOG_RUNNING))
         {
            EventBuilder(EVENT_THREAD_HANGS, g_dwMgmtNode)
               .param(_T("threadName"), s_threadInfo[i].name)
               .post();
            nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_WATCHDOG, _T("Thread \"%s\" does not respond to watchdog thread"), s_threadInfo[i].name);
            s_threadInfo[i].state = WATCHDOG_NOT_RESPONDING;
         }
      s_watchdogMutex.unlock();
   }

   nxlog_debug_tag(DEBUG_TAG_WATCHDOG, 1, _T("Watchdog thread terminated"));
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
   s_watchdogThread = ThreadCreateEx(WatchdogThread);
}

/**
 * Shutdown watchdog
 */
void WatchdogShutdown()
{
   ThreadJoin(s_watchdogThread);
}
