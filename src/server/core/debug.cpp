/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
 * Get total number of DCIs
 */
static int GetDciCount()
{
   int dciCount = 0;
   g_idxObjectById.forEach(
      [&dciCount] (NetObj *object) -> EnumerationCallbackResult
      {
         if (object->isDataCollectionTarget())
            dciCount += static_cast<DataCollectionTarget*>(object)->getItemCount();
         return _CONTINUE;
      });
   return dciCount;
}

/**
 * Get total number of interfaces
 */
static int GetInterfaceCount()
{
   int ifCount = 0;
   g_idxObjectById.forEach(
      [&ifCount] (NetObj *object) -> EnumerationCallbackResult
      {
         if (object->getObjectClass() == OBJECT_INTERFACE)
            ifCount++;
         return _CONTINUE;
      });
   return ifCount;
}

/**
 * Get server statistics as JSON document
 */
json_t NXCORE_EXPORTABLE *GetServerStats()
{
   uint32_t s = static_cast<uint32_t>(time(nullptr) - g_serverStartTime);
   uint32_t d = s / 86400;
   s -= d * 86400;
   uint32_t h = s / 3600;
   s -= h * 3600;
   uint32_t m = s / 60;
   s -= m * 60;

   char uptime[128];
   snprintf(uptime, 128, "%u days, %2u:%02u:%02u", d, h, m, s);

   json_t *stats = json_object();
   json_object_set_new(stats, "total_object_count", json_integer(g_idxObjectById.size()));
   json_object_set_new(stats, "node_object_count", json_integer(g_idxNodeById.size()));
   json_object_set_new(stats, "interface_object_count", json_integer(GetInterfaceCount()));
   json_object_set_new(stats, "access_point_object_count", json_integer(g_idxAccessPointById.size()));
   json_object_set_new(stats, "sensor_object_count", json_integer(g_idxSensorById.size()));
   json_object_set_new(stats, "metric_count", json_integer(GetDciCount()));
   json_object_set_new(stats, "alarm_count", json_integer(GetAlarmCount()));
   json_object_set_new(stats, "uptime", json_string(uptime));
   return stats;
}

/**
 * Show server statistics
 */
void ShowServerStats(CONSOLE_CTX console)
{
	uint32_t s = static_cast<uint32_t>(time(nullptr) - g_serverStartTime);
	uint32_t d = s / 86400;
   s -= d * 86400;
   uint32_t h = s / 3600;
   s -= h * 3600;
   uint32_t m = s / 60;
   s -= m * 60;

   wchar_t uptime[128];
   _sntprintf(uptime, 128, L"%u days, %2u:%02u:%02u", d, h, m, s);

   ConsolePrintf(console,
      _T("Objects............: %d\n")
      _T("   Nodes...........: %d\n")
      _T("   Interfaces......: %d\n")
      _T("   Access points...: %d\n")
      _T("   Sensors.........: %d\n")
      _T("Collectible DCIs...: %d\n")
      _T("Active alarms......: %u\n")
      _T("Uptime.............: %s\n")
      _T("\n"),
      g_idxObjectById.size(), g_idxNodeById.size(), GetInterfaceCount(), g_idxAccessPointById.size(), g_idxSensorById.size(), GetDciCount(), GetAlarmCount(), uptime);
}

/**
 * Show queue stats from function
 */
void ShowQueueStats(ServerConsole *console, int64_t size, const TCHAR *name)
{
   console->printf(_T("%-32s : %u\n"), name, static_cast<unsigned int>(size));
}

/**
 * Show queue stats
 */
void ShowQueueStats(ServerConsole *console, const Queue *queue, const TCHAR *name)
{
   if (queue != nullptr)
      ShowQueueStats(console, queue->size(), name);
}

/**
 * Show queue stats
 */
void ShowThreadPoolPendingQueue(ServerConsole *console, ThreadPool *p, const TCHAR *name)
{
   int size;
   if (p != nullptr)
   {
      ThreadPoolInfo info;
      ThreadPoolGetInfo(p, &info);
      size = (info.activeRequests > info.curThreads) ? info.activeRequests - info.curThreads : 0;
   }
   else
   {
      size = 0;
   }
   console->printf( _T("%-32s : %d\n"), name, size);
}

/**
 * Show thread pool stats
 */
void ShowThreadPool(ServerConsole *console, const TCHAR *p)
{
   ThreadPoolInfo info;
   if (ThreadPoolGetInfo(p, &info))
   {
      console->printf(
            _T("\x1b[1m%s\x1b[0m\n")
           _T("   Threads.............. %d (%d/%d)\n")
           _T("   Load average......... %0.2f %0.2f %0.2f\n")
           _T("   Current load......... %d%%\n")
           _T("   Usage................ %d%%\n")
           _T("   Active requests...... %d\n")
           _T("   Scheduled requests... %d\n")
           _T("   Total requests....... ") UINT64_FMT _T("\n")
           _T("   Thread starts........ ") UINT64_FMT _T("\n")
           _T("   Thread stops......... ") UINT64_FMT _T("\n")
           _T("   Wait time EMA........ %u ms\n")
           _T("   Wait time SMA........ %u ms\n")
           _T("   Wait time SD......... %u ms\n")
           _T("   Queue size EMA....... %u\n")
           _T("   Queue size SMA....... %u\n")
           _T("   Queue size SD........ %u\n\n"),
        info.name, info.curThreads, info.minThreads, info.maxThreads,
        info.loadAvg[0], info.loadAvg[1], info.loadAvg[2],
        info.load, info.usage, info.activeRequests, info.scheduledRequests,
        info.totalRequests, info.threadStarts, info.threadStops,
        info.waitTimeEMA, info.waitTimeSMA, info.waitTimeSD,
        info.queueSizeEMA, info.queueSizeSMA, info.queueSizeSD);
   }
   else
   {
      console->printf(_T("Cannot get information for thread pool \x1b[1m%s\x1b[0m\n"), p);
   }
}

/**
 * Get thread pool stat (for internal DCI)
 */
DataCollectionError GetThreadPoolStat(ThreadPoolStat stat, const TCHAR *param, TCHAR *value)
{
   TCHAR poolName[64], options[64];
   if (!AgentGetParameterArg(param, 1, poolName, 64) ||
       !AgentGetParameterArg(param, 2, options, 64))
      return DCE_NOT_SUPPORTED;

   ThreadPoolInfo info;
   if (!ThreadPoolGetInfo(poolName, &info))
      return DCE_NOT_SUPPORTED;

   switch(stat)
   {
      case THREAD_POOL_CURR_SIZE:
         ret_int(value, info.curThreads);
         break;
      case THREAD_POOL_LOAD:
         ret_int(value, info.load);
         break;
      case THREAD_POOL_LOADAVG_1:
         if ((options[0] != 0) && _tcstol(options, NULL, 10))
            ret_double(value, info.loadAvg[0] / info.maxThreads, 2);
         else
            ret_double(value, info.loadAvg[0], 2);
         break;
      case THREAD_POOL_LOADAVG_5:
         if ((options[0] != 0) && _tcstol(options, NULL, 10))
            ret_double(value, info.loadAvg[1] / info.maxThreads, 2);
         else
            ret_double(value, info.loadAvg[1], 2);
         break;
      case THREAD_POOL_LOADAVG_15:
         if ((options[0] != 0) && _tcstol(options, NULL, 10))
            ret_double(value, info.loadAvg[2] / info.maxThreads, 2);
         else
            ret_double(value, info.loadAvg[2], 2);
         break;
      case THREAD_POOL_MAX_SIZE:
         ret_int(value, info.maxThreads);
         break;
      case THREAD_POOL_MIN_SIZE:
         ret_int(value, info.minThreads);
         break;
      case THREAD_POOL_ACTIVE_REQUESTS:
         ret_int(value, info.activeRequests);
         break;
      case THREAD_POOL_SCHEDULED_REQUESTS:
         ret_int(value, info.scheduledRequests);
         break;
      case THREAD_POOL_USAGE:
         ret_int(value, info.usage);
         break;
      case THREAD_POOL_AVERAGE_WAIT_TIME:
         ret_uint(value, info.waitTimeEMA);
         break;
      default:
         return DCE_NOT_SUPPORTED;
   }
   return DCE_SUCCESS;
}

/**
 * Run load test in thread pool
 */
void RunThreadPoolLoadTest(const TCHAR *poolName, int numTasks, ServerConsole *console)
{
   ThreadPool *pool = ThreadPoolGetByName(poolName);
   if (pool != nullptr)
   {
      console->printf(_T("Starting %d test tasks in thread pool %s\n"), numTasks, poolName);
      for(int i = 0; i < numTasks; i++)
      {
         ThreadPoolExecute(pool,
            [i] () -> void
            {
               ThreadSleepMs(GenerateRandomNumber(0, 500));
               nxlog_debug_tag(_T("threads.pool"), 6, _T("Test task #%d completed"), i + 1);
            });
      }
   }
   else
   {
      console->print(_T("Invalid thread pool name"));
   }
}

/**
 * Write process coredump
 */
#ifdef _WIN32

void DumpProcess(CONSOLE_CTX console)
{
	ConsolePrintf(console, _T("Dumping process to disk...\n"));

	TCHAR cmdLine[MAX_PATH + 64];
	_sntprintf(cmdLine, MAX_PATH + 64, _T("netxmsd.exe --dump-dir \"%s\" --dump %u"), g_szDumpDir, GetCurrentProcessId());

	PROCESS_INFORMATION pi;
   STARTUPINFO si;
	memset(&si, 0, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	if (CreateProcess(NULL, cmdLine, NULL, NULL, FALSE,
            (g_flags & AF_DAEMON) ? CREATE_NO_WINDOW : 0, NULL, NULL, &si, &pi))
	{
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		
		ConsolePrintf(console, _T("Done.\n"));
	}
	else
	{
		TCHAR buffer[256];
		ConsolePrintf(console, _T("Dump error: CreateProcess() failed (%s)\n"), GetSystemErrorText(GetLastError(), buffer, 256));
	}
}

#else

void DumpProcess(CONSOLE_CTX console)
{
	console->print(_T("DUMP command is not supported for current operating system\n"));
}

#endif
