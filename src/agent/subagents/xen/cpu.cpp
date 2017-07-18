/*
** NetXMS XEN hypervisor subagent
** Copyright (C) 2017 Raden Solutions
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
** File: cpu.cpp
**
**/

#include "xen.h"

struct CPU_USAGE
{
   uint64_t prevTime;
};

static HashMap<INT32, CPU_USAGE> s_spuUsage;

static bool CollectCPUTime(libxl_ctx *ctx)
{
   bool success = false;
   uint64_t totalTime = 0;
   int count;
   libxl_dominfo *domains = libxl_list_domain(ctx, &count);
   if (domains != NULL)
   {
      for(int i = 0; i < count; i++)
      {
         totalTime += domains[i].cpu_time;
      }
      libxl_dominfo_list_free(domains, count);
   }
   else
   {
      nxlog_debug(4, _T("XEN: call to libxl_list_domain failed"));
   }
   return success;
}

/**
 * CPU collector thread
 */
static THREAD_RESULT THREAD_CALL CollectorThread(void *arg)
{
   nxlog_debug(1, _T("XEN: CPU collector thread started"));
   libxl_ctx *ctx = NULL;
   bool connected = false;
   uint64_t prevTime = 0;
   while(!AgentSleepAndCheckForShutdown(1000))
   {
      if (!connected)
      {
         connected = (libxl_ctx_alloc(&ctx, LIBXL_VERSION, 0, &g_xenLogger) == 0);
         if (connected)
            prevTime = CollectCPUTime(ctx);
         continue;
      }

      uint64_t currTime = CollectCPUTime(ctx);
      nxlog_debug(1, _T("TIME = %llu"), currTime - prevTime);
      prevTime = currTime;
   }
   if (ctx != NULL)
      libxl_ctx_free(ctx);
   nxlog_debug(1, _T("XEN: CPU collector thread stopped"));
   return THREAD_OK;
}

/**
 * CPU collector thread handle
 */
static THREAD s_cpuCollectorThread = INVALID_THREAD_HANDLE;

/**
 * Start CPU collector
 */
void XenStartCPUCollector()
{
   s_cpuCollectorThread = ThreadCreateEx(CollectorThread, 0, NULL);
}

/**
 * Stop CPU collector
 */
void XenStopCPUCollector()
{
   ThreadJoin(s_cpuCollectorThread);
}
