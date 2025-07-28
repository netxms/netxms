/*
** NetXMS subagent for AIX
** Copyright (C) 2004-2023 Victor Kirhenshtein
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

#include "aix_subagent.h"


//
// Collector data
//

#define CPU_USAGE_SLOTS			900 		/* 60 sec * 15 min => 900 sec */

static THREAD m_cpuUsageCollector = INVALID_THREAD_HANDLE;
static Mutex m_cpuUsageMutex(MutexType::FAST);
static bool volatile m_stopCollectorThread = false;
static uint64_t *m_lastUser;
static uint64_t *m_lastSystem;
static uint64_t *m_lastIdle;
static uint64_t *m_lastIoWait;
static double *m_cpuUsage;
static double *m_cpuUsageUser;
static double *m_cpuUsageSystem;
static double *m_cpuUsageIdle;
static double *m_cpuUsageIoWait;

static u_longlong_t m_lastPhysicalUser;
static u_longlong_t m_lastPhysicalSystem;
static u_longlong_t m_lastPhysicalIdle;
static u_longlong_t m_lastPhysicalIoWait;
static double *m_cpuPhysicalUsage;
static double *m_cpuPhysicalUsageUser;
static double *m_cpuPhysicalUsageSystem;
static double *m_cpuPhysicalUsageIdle;
static double *m_cpuPhysicalUsageIoWait;

static perfstat_cpu_t *m_cpuStats;

static int m_currentSlot = 0;
static int m_maxCPU = 0;

// update detailed stats
#define UPDATE(delta, target) { \
      if (delta > 0) { *(target + (cpu * CPU_USAGE_SLOTS) + m_currentSlot) = (double)delta / onePercent; } \
      else { *(target + (cpu * CPU_USAGE_SLOTS) + m_currentSlot) = 0; } \
   }

/**
 * Stats collector
 */
static void CpuUsageCollector()
{
   uint64_t user = 0, system = 0, idle = 0, iowait = 0;
   unsigned int cpu = 0;

   m_cpuUsageMutex.lock();
	if (m_currentSlot == CPU_USAGE_SLOTS)
	{
		m_currentSlot = 0;
	}

	// Get data
	perfstat_cpu_total_t cpuTotals;
	if (perfstat_cpu_total(NULL, &cpuTotals, sizeof(perfstat_cpu_total_t), 1) == 1)
	{
		perfstat_id_t firstcpu;
		strcpy(firstcpu.name, FIRST_CPU);
		int cpuCount = perfstat_cpu(&firstcpu, m_cpuStats, sizeof(perfstat_cpu_t), m_maxCPU);
		if (cpuCount > m_maxCPU)
		{
			nxlog_debug_tag(AIX_DEBUG_TAG, 6, _T("perfstat_cpu return code (%d) > maxCPU (%d)"), cpuCount, m_maxCPU);
			cpuCount = m_maxCPU;
		}
		else if (cpuCount <= 0)
		{
			nxlog_debug_tag(AIX_DEBUG_TAG, 6, _T("Call to perfstat_cpu failed (%s)"), _tcserror(errno));
			cpuCount = 0;
		}
		
		// update collected data
		for(cpu = 0; cpu <= cpuCount; cpu++)
		{
			if (cpu == 0)
			{
				user = cpuTotals.user;
				system = cpuTotals.sys;
				idle = cpuTotals.idle;
				iowait = cpuTotals.wait;
			}
			else
			{
				user = m_cpuStats[cpu - 1].user;
				system = m_cpuStats[cpu - 1].sys;
				idle = m_cpuStats[cpu - 1].idle;
				iowait = m_cpuStats[cpu - 1].wait;
			}
			
			uint64_t userDelta = user - m_lastUser[cpu];
			uint64_t systemDelta = system - m_lastSystem[cpu];
			uint64_t idleDelta = idle - m_lastIdle[cpu];
			uint64_t iowaitDelta = iowait - m_lastIoWait[cpu];
			uint64_t totalDelta = userDelta + systemDelta + idleDelta + iowaitDelta;
			
         if (totalDelta > 0)
         {
            double onePercent = (double)totalDelta / 100.0; // 1% of total

            UPDATE(userDelta, m_cpuUsageUser);
            UPDATE(systemDelta, m_cpuUsageSystem);
            UPDATE(idleDelta, m_cpuUsageIdle);
            UPDATE(iowaitDelta, m_cpuUsageIoWait);

				*(m_cpuUsage + (cpu * CPU_USAGE_SLOTS) + m_currentSlot) = 100.0 - ((double)idleDelta / onePercent);
			}
			else
			{
            *(m_cpuUsageUser + (cpu * CPU_USAGE_SLOTS) + m_currentSlot) = 0;
            *(m_cpuUsageSystem + (cpu * CPU_USAGE_SLOTS) + m_currentSlot) = 0;
            *(m_cpuUsageIdle + (cpu * CPU_USAGE_SLOTS) + m_currentSlot) = 100;
            *(m_cpuUsageIoWait + (cpu * CPU_USAGE_SLOTS) + m_currentSlot) = 0;
				*(m_cpuUsage + (cpu * CPU_USAGE_SLOTS) + m_currentSlot) = 0;
			}

			m_lastUser[cpu] = user;
			m_lastSystem[cpu] = system;
			m_lastIdle[cpu] = idle;
			m_lastIoWait[cpu] = iowait;
		}
	}
	else
	{
		nxlog_debug_tag(AIX_DEBUG_TAG, 6, _T("Call to perfstat_cpu_total failed (%s)"), _tcserror(errno));
	}

   perfstat_partition_total_t lparstats;
   if (perfstat_partition_total(NULL, &lparstats, sizeof(perfstat_partition_total_t), 1))
   {
      u_longlong_t deltaUser, deltaSystem, deltaIdle, deltaIoWait, deltaTotal;

      deltaUser = lparstats.puser - m_lastPhysicalUser;
      deltaSystem = lparstats.psys - m_lastPhysicalSystem;
      deltaIdle = lparstats.pidle - m_lastPhysicalIdle;
      deltaIoWait = lparstats.pwait - m_lastPhysicalIoWait;
      deltaTotal = deltaUser + deltaSystem + deltaIdle + deltaIoWait;

      m_lastPhysicalUser = lparstats.puser;
      m_lastPhysicalSystem = lparstats.psys;
      m_lastPhysicalIdle = lparstats.pidle;
      m_lastPhysicalIoWait = lparstats.pwait;

      m_cpuPhysicalUsageUser[m_currentSlot] = (double)deltaUser * 100.0 / (double)deltaTotal;
      m_cpuPhysicalUsageSystem[m_currentSlot] = (double)deltaSystem * 100.0 / (double)deltaTotal;
      m_cpuPhysicalUsageIdle[m_currentSlot] = (double)deltaIdle * 100.0 / (double)deltaTotal;
      m_cpuPhysicalUsageIoWait[m_currentSlot] = (double)deltaIoWait * 100.0 / (double)deltaTotal;

      m_cpuPhysicalUsage[m_currentSlot] = 100.0 - m_cpuPhysicalUsageIdle[m_currentSlot];
   }
   else
   {
      nxlog_debug_tag(AIX_DEBUG_TAG, 6, _T("Call to perfstat_partition_total failed (%s)"), _tcserror(errno));
   }

   // go to the next slot
   m_currentSlot++;
   m_cpuUsageMutex.unlock();
}

/**
 * Collector thread
 */
static void CpuUsageCollectorThread()
{
   nxlog_debug_tag(AIX_DEBUG_TAG, 1, _T("CPU usage collector thread started"));
   while(m_stopCollectorThread == false)
   {
      CpuUsageCollector();
      ThreadSleepMs(1000); // sleep 1 second
   }
   nxlog_debug_tag(AIX_DEBUG_TAG, 1, _T("CPU usage collector thread stopped"));
}

/**
 * Start CPU usage collector
 */
void StartCpuUsageCollector()
{
   perfstat_cpu_total_t cpuTotals;
   if (perfstat_cpu_total(NULL, &cpuTotals, sizeof(perfstat_cpu_total_t), 1) == 1)
   {
      m_maxCPU = cpuTotals.ncpus_cfg;
   }
   else
   {
      m_maxCPU = 1;
   }
   nxlog_debug_tag(AIX_DEBUG_TAG, 2, _T("CPU count set to %d"), m_maxCPU);

   m_cpuStats = (perfstat_cpu_t *)malloc(sizeof(perfstat_cpu_t) * m_maxCPU);

#define SIZE sizeof(uint64_t) * (m_maxCPU + 1)
#define ALLOCATE_AND_CLEAR(x) x = (uint64_t *)malloc(SIZE); memset(x, 0, SIZE);
   ALLOCATE_AND_CLEAR(m_lastUser);
   ALLOCATE_AND_CLEAR(m_lastSystem);
   ALLOCATE_AND_CLEAR(m_lastIdle);
   ALLOCATE_AND_CLEAR(m_lastIoWait);
#undef ALLOCATE_AND_CLEAR
#undef SIZE

#define SIZE sizeof(double) * CPU_USAGE_SLOTS * (m_maxCPU + 1)
#define ALLOCATE_AND_CLEAR(x) x = (double *)malloc(SIZE); memset(x, 0, SIZE);
   ALLOCATE_AND_CLEAR(m_cpuUsage);
   ALLOCATE_AND_CLEAR(m_cpuUsageUser);
   ALLOCATE_AND_CLEAR(m_cpuUsageSystem);
   ALLOCATE_AND_CLEAR(m_cpuUsageIdle);
   ALLOCATE_AND_CLEAR(m_cpuUsageIoWait);
#undef ALLOCATE_AND_CLEAR
#undef SIZE

#define SIZE sizeof(double) * CPU_USAGE_SLOTS
#define ALLOCATE_AND_CLEAR(x) x = (double *)malloc(SIZE); memset(x, 0, SIZE);
   ALLOCATE_AND_CLEAR(m_cpuPhysicalUsage);
   ALLOCATE_AND_CLEAR(m_cpuPhysicalUsageUser);
   ALLOCATE_AND_CLEAR(m_cpuPhysicalUsageSystem);
   ALLOCATE_AND_CLEAR(m_cpuPhysicalUsageIoWait);
   ALLOCATE_AND_CLEAR(m_cpuPhysicalUsageIdle);
#undef ALLOCATE_AND_CLEAR
#undef SIZE

   // get initial count of user/system/idle time
   m_currentSlot = 0;
   CpuUsageCollector();

   sleep(1);

   // fill first slot with u/s/i delta
   m_currentSlot = 0;
   CpuUsageCollector();

   // fill all slots with current cpu usage
   for (int i = 1; i < CPU_USAGE_SLOTS * (m_maxCPU + 1); i++)
   {
      m_cpuUsage[i] = m_cpuUsage[0];
      m_cpuUsageUser[i] = m_cpuUsageUser[0];
      m_cpuUsageSystem[i] = m_cpuUsageSystem[0];
      m_cpuUsageIdle[i] = m_cpuUsageIdle[0];
      m_cpuUsageIoWait[i] = m_cpuUsageIoWait[0];
   }
   for (int i = 1; i < CPU_USAGE_SLOTS; i++)
   {
      m_cpuPhysicalUsage[i] = m_cpuPhysicalUsage[0];
      m_cpuPhysicalUsageUser[i] = m_cpuPhysicalUsageUser[0];
      m_cpuPhysicalUsageSystem[i] = m_cpuPhysicalUsageSystem[0];
      m_cpuPhysicalUsageIdle[i] = m_cpuPhysicalUsageIdle[0];
      m_cpuPhysicalUsageIoWait[i] = m_cpuPhysicalUsageIoWait[0];
   }

   // start collector
   m_cpuUsageCollector = ThreadCreateEx(CpuUsageCollectorThread);
}

/**
 * Shutdown CPU usage collector
 */
void ShutdownCpuUsageCollector()
{
   m_stopCollectorThread = true;
   ThreadJoin(m_cpuUsageCollector);

   MemFree(m_cpuStats);

   MemFree(m_cpuUsage);
   MemFree(m_cpuUsageUser);
   MemFree(m_cpuUsageSystem);
   MemFree(m_cpuUsageIdle);
   MemFree(m_cpuUsageIoWait);

   MemFree(m_cpuPhysicalUsage);
   MemFree(m_cpuPhysicalUsageUser);
   MemFree(m_cpuPhysicalUsageSystem);
   MemFree(m_cpuPhysicalUsageIoWait);
   MemFree(m_cpuPhysicalUsageIdle);

   MemFree(m_lastUser);
   MemFree(m_lastSystem);
   MemFree(m_lastIdle);
   MemFree(m_lastIoWait);
}

/**
 * Get usage value for given CPU, metric, and interval
 */
static void GetUsage(int source, int cpu, int count, TCHAR *value)
{
   double *table;
   switch (source)
   {
      case CPU_USAGE_OVERALL:
         table = m_cpuUsage;
         break;
      case CPU_USAGE_USER:
         table = m_cpuUsageUser;
         break;
		case CPU_USAGE_SYSTEM:
			table = m_cpuUsageSystem;
			break;
		case CPU_USAGE_IDLE:
			table = m_cpuUsageIdle;
			break;
		case CPU_USAGE_IOWAIT:
			table = m_cpuUsageIoWait;
			break;
      case CPU_PA_OVERALL:
         table = m_cpuPhysicalUsage;
         break;
      case CPU_PA_USER:
         table = m_cpuPhysicalUsageUser;
         break;
      case CPU_PA_SYSTEM:
         table = m_cpuPhysicalUsageSystem;
         break;
      case CPU_PA_IDLE:
         table = m_cpuPhysicalUsageIdle;
         break;
      case CPU_PA_IOWAIT:
         table = m_cpuPhysicalUsageIoWait;
         break;
      default:
         table = m_cpuUsage;
         break;
   }

   table += cpu * CPU_USAGE_SLOTS;

   double usage = 0;

   m_cpuUsageMutex.lock();

   // m_currentSlot points to next slot to be filled
   // it is never zero because collector thread always increment it
   // before releasing mutex
   double *p = table + m_currentSlot - 1;
	for (int i = 0; i < count; i++)
	{
		usage += *p;
		if (p == table)
		{
			p += CPU_USAGE_SLOTS;
		}
		p--;
	}

   m_cpuUsageMutex.unlock();

   usage /= count;
   ret_double(value, usage);
}

/**
 * Handler for System.CPU.Usage.* parameters
 */
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

/**
 * Handler for System.CPU.Usage.*(*) parameters (CPU-specific versions)
 */
LONG H_CpuUsageEx(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
	int count, cpu;
	TCHAR buffer[256], *eptr;

	if (!AgentGetParameterArg(pszParam, 1, buffer, 256))
		return SYSINFO_RC_UNSUPPORTED;
		
	cpu = _tcstol(buffer, &eptr, 0);
	if ((*eptr != 0) || (cpu < 0) || (cpu >= m_maxCPU))
		return SYSINFO_RC_UNSUPPORTED;

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
 * Handler for System.CPU.Count parameter
 */
LONG H_CpuCount(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   perfstat_cpu_total_t cpuTotals;
   if (perfstat_cpu_total(NULL, &cpuTotals, sizeof(perfstat_cpu_total_t), 1) != 1)
      return SYSINFO_RC_ERROR;
   ret_uint(pValue, cpuTotals.ncpus);
   return SYSINFO_RC_SUCCESS;
}
