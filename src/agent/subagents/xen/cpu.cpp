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

#define MAX_CPU_USAGE_SLOTS   900

/**
 * CPU usage structure
 */
class CpuUsageData
{
public:
   uint64_t prevTime;
   INT32 usage[MAX_CPU_USAGE_SLOTS];
   int currPos;

   CpuUsageData()
   {
      prevTime = 0;
      memset(usage, 0, sizeof(usage));
      currPos = 0;
   }

   void update(uint64_t timePeriod, uint64_t cpuTime, int cpuCount)
   {
      if (prevTime != 0)
      {
         usage[currPos++] = (INT32)((cpuTime - prevTime) * 1000 / timePeriod) / cpuCount;
         if (currPos == MAX_CPU_USAGE_SLOTS)
            currPos = 0;
      }
      prevTime = cpuTime;
   }

   UINT32 getCurrentUsage()
   {
      return usage[currPos > 0 ? currPos - 1 : MAX_CPU_USAGE_SLOTS - 1];
   }

   UINT32 getAverageUsage(int samples)
   {
      UINT32 sum = 0;
      for(int i = 0, pos = currPos; i < samples; i++)
      {
         pos--;
         if (pos < 0)
            pos = MAX_CPU_USAGE_SLOTS - 1;
         sum += usage[pos];
      }
      return sum / samples;
   }
};

/**
 * Collected data
 */
static HashMap<uint32_t, CpuUsageData> s_vmCpuUsage(true);
static CpuUsageData s_hostCpuUsage;
static Mutex s_dataLock;

/**
 * Query CPU usage for domain
 */
bool XenQueryDomainCpuUsage(uint32_t domId, INT32 *curr, INT32 *avg1min)
{
   s_dataLock.lock();
   CpuUsageData *data = s_vmCpuUsage.get(domId);
   if (data != NULL)
   {
      *curr = data->getCurrentUsage();
      *avg1min = data->getAverageUsage(60);
   }
   s_dataLock.unlock();
   return data != NULL;
}

/**
 * Collect CPU usage data
 */
static bool CollectData(libxl_ctx *ctx, struct timespec *prevClock)
{
   bool success = false;
   uint64_t totalTime = 0;

   int count;
   libxl_dominfo *domains = libxl_list_domain(ctx, &count);
   if (domains != NULL)
   {
      struct timespec currClock;
      clock_gettime(CLOCK_MONOTONIC_RAW, &currClock);
      uint64_t tdiff = (currClock.tv_sec - prevClock->tv_sec) * _ULL(1000000000) + (currClock.tv_nsec - prevClock->tv_nsec);

      int cpuCount = libxl_get_online_cpus(ctx);

      s_dataLock.lock();
      for(int i = 0; i < count; i++)
      {
         CpuUsageData *u = s_vmCpuUsage.get(domains[i].domid);
         if (u == NULL)
         {
            u = new CpuUsageData();
            s_vmCpuUsage.set(domains[i].domid, u);
         }
         u->update(tdiff, domains[i].cpu_time, cpuCount);
         totalTime += domains[i].cpu_time;
      }
      libxl_dominfo_list_free(domains, count);

      s_hostCpuUsage.update(tdiff, totalTime, cpuCount);
      s_dataLock.unlock();

      memcpy(prevClock, &currClock, sizeof(struct timespec));
      success = true;
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

   struct timespec currClock;
   while(!AgentSleepAndCheckForShutdown(1000))
   {
      if (!connected)
      {
         connected = (libxl_ctx_alloc(&ctx, LIBXL_VERSION, 0, &g_xenLogger) == 0);
         if (connected)
            CollectData(ctx, &currClock);
         continue;
      }
      CollectData(ctx, &currClock);
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

/**
 * Handler for XEN.Host.CPU.Usage parameters
 */
LONG H_XenHostCPUUsage(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   UINT32 usage;
   s_dataLock.lock();
   switch(*arg)
   {
      case '0':
         usage = s_hostCpuUsage.getCurrentUsage();
         break;
      case '1':
         usage = s_hostCpuUsage.getAverageUsage(60);
         break;
      case '5':
         usage = s_hostCpuUsage.getAverageUsage(300);
         break;
      case 'F':
         usage = s_hostCpuUsage.getAverageUsage(900);
         break;
   }
   s_dataLock.unlock();
   _sntprintf(value, MAX_RESULT_LENGTH, _T("%d.%d"), usage / 10, usage % 10);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for XEN.Domain.CPU.Usage parameters
 */
LONG H_XenDomainCPUUsage(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   char domName[256];
   if (!AgentGetParameterArgA(param, 1, domName, 256))
      return SYSINFO_RC_UNSUPPORTED;

   char *eptr;
   uint32_t domId = strtoul(domName, &eptr, 0);
   if (*eptr != 0)
   {
      LONG rc = XenResolveDomainName(domName, &domId);
      if (rc != SYSINFO_RC_SUCCESS)
         return rc;
   }

   LONG rc = SYSINFO_RC_SUCCESS;
   UINT32 usage;
   s_dataLock.lock();
   CpuUsageData *data = s_vmCpuUsage.get(domId);
   if (data != NULL)
   {
      switch(*arg)
      {
         case '0':
            usage = data->getCurrentUsage();
            break;
         case '1':
            usage = data->getAverageUsage(60);
            break;
         case '5':
            usage = data->getAverageUsage(300);
            break;
         case 'F':
            usage = data->getAverageUsage(900);
            break;
      }
      _sntprintf(value, MAX_RESULT_LENGTH, _T("%d.%d"), usage / 10, usage % 10);
   }
   else
   {
      rc = SYSINFO_RC_ERROR;
   }
   s_dataLock.unlock();
   return rc;
}
