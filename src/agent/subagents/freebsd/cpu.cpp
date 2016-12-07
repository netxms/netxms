/*
** NetXMS subagent for FreeBSD
** Copyright (C) 2004 - 2016 Raden Solutions
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
**/
#include "freebsd_subagent.h"

#define CPU_USAGE_SLOTS       900 // 60 sec * 15 min => 900 sec
// 64 + 1 for overal
#define MAX_CPU               (64 + 1)

static THREAD m_cpuUsageCollector = INVALID_THREAD_HANDLE;
static MUTEX m_cpuUsageMutex = INVALID_MUTEX_HANDLE;
static bool volatile m_stopCollectorThread = false;
static UINT64 m_user[MAX_CPU];
static UINT64 m_nice[MAX_CPU];
static UINT64 m_system[MAX_CPU];
static UINT64 m_idle[MAX_CPU];
static UINT64 m_cpuInterrupts;
static float *m_cpuUsage;
static float *m_cpuUsageUser;
static float *m_cpuUsageNice;
static float *m_cpuUsageSystem;
static float *m_cpuUsageIdle;
static int m_currentSlot = 0;
static int m_cpuCount = 0;

static int ExecSysctl(const char *param, void *buffer, size_t buffSize)
{
   int ret = SYSINFO_RC_ERROR;
   int mib[2];
   size_t nSize = sizeof(mib);

   if (sysctlnametomib(param, mib, &nSize) == 0)
   {
      if (sysctl(mib, nSize, buffer, &buffSize, NULL, 0) == 0)
      {
         ret = SYSINFO_RC_SUCCESS;
      }
   }

   return ret;
}

/**
 * CPU usage collector
 */
static void CpuUsageCollector()
{
   UINT64 buffer[1024];
   size_t buffSize = sizeof(buffer);

   if (ExecSysctl("kern.cp_times", buffer, buffSize) != SYSINFO_RC_SUCCESS)
      return;

   MutexLock(m_cpuUsageMutex);
   if (m_currentSlot == CPU_USAGE_SLOTS)
   {
      m_currentSlot = 0;
   }

   UINT64 user, nice, system, interrupts = 0, idle, n = 0;
   // scan for all CPUs
   for(int cpu = 0; cpu < m_cpuCount; cpu++)
   {
      user = buffer[n];
      nice = buffer[n+1];
      system = buffer[n+2];
      interrupts += buffer[n+3];
      idle = buffer[n+4];

      UINT64 userDelta, niceDelta, systemDelta, idleDelta;

#define DELTA(x, y) (((x) > (y)) ? ((x) - (y)) : 0)
      userDelta = DELTA(user, m_user[cpu]);
      niceDelta = DELTA(nice, m_nice[cpu]);
      systemDelta = DELTA(system, m_system[cpu]);
      idleDelta = DELTA(idle, m_idle[cpu]);
#undef DELTA

      UINT64 totalDelta = userDelta + niceDelta + systemDelta + idleDelta;
      float onePercent = (float)totalDelta / 100.0; // 1% of total
      if (onePercent == 0)
      {
         onePercent = 1; // TODO: why 1?
      }

      /* update detailed stats */
#define UPDATE(delta, target) { \
         if (delta > 0) { *(target + (cpu * CPU_USAGE_SLOTS) + m_currentSlot) = (float)delta / onePercent; \
         } \
         else { *(target + (cpu * CPU_USAGE_SLOTS) + m_currentSlot) = 0; } \
      }

      UPDATE(userDelta, m_cpuUsageUser);
      UPDATE(niceDelta, m_cpuUsageNice);
      UPDATE(systemDelta, m_cpuUsageSystem);
      UPDATE(idleDelta, m_cpuUsageIdle);

      /* update overal cpu usage */
      if (totalDelta > 0)
      {
         *(m_cpuUsage + (cpu * CPU_USAGE_SLOTS) + m_currentSlot) = 100.0 - ((float)idleDelta / onePercent);
      }
      else
      {
         *(m_cpuUsage + (cpu * CPU_USAGE_SLOTS) + m_currentSlot) = 0;
      }

      m_user[cpu] = user;
      m_nice[cpu] = nice;
      m_system[cpu] = system;
      m_idle[cpu] = idle;

      n += 5;
   }

   m_cpuInterrupts = interrupts;

   /* go to the next slot */
   m_currentSlot++;
   MutexUnlock(m_cpuUsageMutex);
}

/**
 * CPU usage collector thread
 *
 */
static THREAD_RESULT THREAD_CALL CpuUsageCollectorThread(void *pArg)
{
   while(m_stopCollectorThread == false)
   {
      CpuUsageCollector();
      ThreadSleepMs(1000); // sleep 1 second
   }
   return THREAD_OK;
}

/**
 * Start CPU usage collector
 */
void StartCpuUsageCollector()
{
   size_t size = sizeof(m_cpuCount);
   if (ExecSysctl("hw.ncpu", &m_cpuCount, size) != SYSINFO_RC_SUCCESS)
      return;

   int i, j;

   m_cpuUsageMutex = MutexCreate();

#define SIZE sizeof(float) * CPU_USAGE_SLOTS * m_cpuCount
#define ALLOCATE_AND_CLEAR(x) x = (float *)malloc(SIZE); memset(x, 0, SIZE);
   ALLOCATE_AND_CLEAR(m_cpuUsage);
   ALLOCATE_AND_CLEAR(m_cpuUsageUser);
   ALLOCATE_AND_CLEAR(m_cpuUsageNice);
   ALLOCATE_AND_CLEAR(m_cpuUsageSystem);
   ALLOCATE_AND_CLEAR(m_cpuUsageIdle);
#undef ALLOCATE_AND_CLEAR
#undef SIZE
   // get initial count of user/system/idle time
   CpuUsageCollector();

   sleep(1);

   // fill first slot with u/s/i delta
   CpuUsageCollector();

   // fill all slots with current cpu usage
#define FILL(x) memcpy(x + i, x, sizeof(float));
   for (i = 0; i < (CPU_USAGE_SLOTS * m_cpuCount) - 1; i++)
   {
         FILL(m_cpuUsage);
         FILL(m_cpuUsageUser);
         FILL(m_cpuUsageNice);
         FILL(m_cpuUsageSystem);
         FILL(m_cpuUsageIdle);
   }
#undef FILL

   // start collector
   m_cpuUsageCollector = ThreadCreateEx(CpuUsageCollectorThread, 0, NULL);
}

/**
 * Shutdown CPU usage collector
 */
void ShutdownCpuUsageCollector()
{
   m_stopCollectorThread = true;
   ThreadJoin(m_cpuUsageCollector);
   MutexDestroy(m_cpuUsageMutex);

   free(m_cpuUsage);
   free(m_cpuUsageUser);
   free(m_cpuUsageNice);
   free(m_cpuUsageSystem);
   free(m_cpuUsageIdle);
}

static void GetUsage(int source, int cpu, int count, TCHAR *value)
{
   float *table;
   switch (source)
   {
      case CPU_USAGE_OVERAL:
         table = m_cpuUsage;
         break;
      case CPU_USAGE_USER:
         table = m_cpuUsageUser;
         break;
      case CPU_USAGE_NICE:
         table = m_cpuUsageNice;
         break;
      case CPU_USAGE_SYSTEM:
         table = m_cpuUsageSystem;
         break;
      case CPU_USAGE_IDLE:
         table = m_cpuUsageIdle;
         break;
      default:
         table = (float *)m_cpuUsage;
   }

   table += cpu * CPU_USAGE_SLOTS;
   double usage = 0;
   MutexLock(m_cpuUsageMutex);

   float *p = table + m_currentSlot - 1;
   for (int i = 0; i < count; i++)
   {
      usage += *p;
      if (p == table)
      {
         p += CPU_USAGE_SLOTS;
      }
      p--;
   }

   MutexUnlock(m_cpuUsageMutex);

   usage /= count;

   ret_double(value, usage);
}

LONG H_CpuUsage(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   int count;

   switch(CPU_USAGE_PARAM_INTERVAL(pArg))
   {
      case INTERVAL_5MIN:
         count = 5 * 60;
         break;
      case INTERVAL_15MIN:
         count = 15 * 60;
         break;
      default:
         count = 60;
         break;
   }

   GetUsage(CPU_USAGE_PARAM_SOURCE(pArg), 0, count, pValue);
   return SYSINFO_RC_SUCCESS;
}

LONG H_CpuUsageEx(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   int count, cpu;

   switch(CPU_USAGE_PARAM_INTERVAL(pArg))
   {
      case INTERVAL_5MIN:
         count = 5 * 60;
         break;
      case INTERVAL_15MIN:
         count = 15 * 60;
         break;
      default:
         count = 60;
         break;
   }

   GetUsage(CPU_USAGE_PARAM_SOURCE(pArg), cpu + 1, count, pValue);

   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for CPU info parameters
 */
LONG H_CpuInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   size_t size;
   int ret;
   switch(*arg)
   {
      case 'C':   // CPU count
         UINT32 count;
         size = sizeof(count);
         ret = ExecSysctl("hw.ncpu", &count, size);
         ret_uint(value, count);
         break;
      case 'F':   // Frequency
         UINT32 freq;
         size = sizeof(freq);
         ret = ExecSysctl("hw.clockrate", &freq, size);
         ret_uint(value, freq);
         break;
      case 'M':   // Model
         char buffer[MAX_NAME];
         size = sizeof(buffer);
         ret = ExecSysctl("hw.model", buffer, size);
         ret_mbstring(value, buffer);
         break;
      default:
         return SYSINFO_RC_UNSUPPORTED;
   }

   return ret;
}

LONG H_CpuLoad(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   int nRet = SYSINFO_RC_ERROR;
   char szArg[128] = {0};
   FILE *hFile;
   double dLoad[3];

   struct loadavg sysload;
   size_t size = sizeof(loadavg);
   if (ExecSysctl("vm.loadavg", &sysload, size) != SYSINFO_RC_SUCCESS)
      return nRet;

   switch (pszParam[18])
   {
   case '1': // 15 min
      ret_double(pValue, (float)sysload.ldavg[2] / sysload.fscale);
      break;
   case '5': // 5 min
      ret_double(pValue, (float)sysload.ldavg[1] / sysload.fscale);
      break;
   default: // 1 min
      ret_double(pValue, (float)sysload.ldavg[0] / sysload.fscale);
      break;
   }
   nRet = SYSINFO_RC_SUCCESS;

   return nRet;
}

/*
 * Handler for CPU Context Switch parameter
 */
LONG H_CpuCswitch(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   UINT64 buffer;
   size_t size = sizeof(buffer);

   if (sysctlbyname("vm.stats.sys.v_swtch", &buffer, &size, NULL, 0) != 0)
      return SYSINFO_RC_ERROR;

   ret_uint(value, buffer);
   return SYSINFO_RC_SUCCESS;
}

/*
 * Handler for CPU Interrupts parameter
 */
LONG H_CpuInterrupts(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   ret_uint(value, m_cpuInterrupts);
   return SYSINFO_RC_SUCCESS;
}
