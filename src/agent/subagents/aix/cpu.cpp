/*
** NetXMS subagent for AIX
** Copyright (C) 2004-2011 Victor Kirhenshtein
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
#include <libperfstat.h>


//
// Collector data
//

#define CPU_USAGE_SLOTS			900 		/* 60 sec * 15 min => 900 sec */
#define MAX_CPU					(64 + 1)	/* 64 + 1 for overal */

static THREAD m_cpuUsageCollector = INVALID_THREAD_HANDLE;
static MUTEX m_cpuUsageMutex = INVALID_MUTEX_HANDLE;
static bool volatile m_stopCollectorThread = false;
static uint64_t m_user[MAX_CPU];
static uint64_t m_system[MAX_CPU];
static uint64_t m_idle[MAX_CPU];
static uint64_t m_iowait[MAX_CPU];
static float *m_cpuUsage;
static float *m_cpuUsageUser;
static float *m_cpuUsageSystem;
static float *m_cpuUsageIdle;
static float *m_cpuUsageIoWait;
static int m_currentSlot = 0;
static int m_maxCPU = 0;


//
// Stats collector
//

static void CpuUsageCollector()
{
	uint64_t user = 0, system = 0, idle = 0, iowait = 0;
	unsigned int cpu = 0;

	MutexLock(m_cpuUsageMutex, INFINITE);
	if (m_currentSlot == CPU_USAGE_SLOTS)
	{
		m_currentSlot = 0;
	}

	// Get data
	perfstat_cpu_total_t cpuTotals;
	perfstat_cpu_t *cpuStats = NULL;
	if (perfstat_cpu_total(NULL, &cpuTotals, sizeof(perfstat_cpu_total_t), 1) == 1)
	{
		perfstat_id_t firstcpu;
		strcpy(firstcpu.name, FIRST_CPU);
		cpuStats = (perfstat_cpu_t *)malloc(sizeof(perfstat_cpu_t) * cpuTotals.ncpus);
		int cpuCount = perfstat_cpu(&firstcpu, cpuStats, sizeof(perfstat_cpu_t), cpuTotals.ncpus);
		if (cpuCount > 0)
		{
			m_maxCPU = cpuCount;
		}
		else
		{
			AgentWriteDebugLog(4, "AIX: call to perfstat_cpu failed (%s)", strerror(errno));
			m_maxCPU = 0;
		}
		
		// update collected data
		for(cpu = 0; cpu <= m_maxCPU; cpu++)
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
				user = cpuStats[cpu - 1].user;
				system = cpuStats[cpu - 1].sys;
				idle = cpuStats[cpu - 1].idle;
				iowait = cpuStats[cpu - 1].wait;
			}
			
			uint64_t userDelta = user - m_user[cpu];
			uint64_t systemDelta = system - m_system[cpu];
			uint64_t idleDelta = idle - m_idle[cpu];
			uint64_t iowaitDelta = iowait - m_iowait[cpu];
			uint64_t totalDelta = userDelta + systemDelta + idleDelta + iowaitDelta;
			
			float onePercent = (float)totalDelta / 100.0; // 1% of total
			if (onePercent == 0)
			{
				onePercent = 1; // TODO: why 1?
			}

			// update detailed stats
#define UPDATE(delta, target) { \
				if (delta > 0) { *(target + (cpu * CPU_USAGE_SLOTS) + m_currentSlot) = (float)delta / onePercent; } \
				else { *(target + (cpu * CPU_USAGE_SLOTS) + m_currentSlot) = 0; } \
			}

			UPDATE(userDelta, m_cpuUsageUser);
			UPDATE(systemDelta, m_cpuUsageSystem);
			UPDATE(idleDelta, m_cpuUsageIdle);
			UPDATE(iowaitDelta, m_cpuUsageIoWait);

			// update overal cpu usage
			if (totalDelta > 0)
			{
				*(m_cpuUsage + (cpu * CPU_USAGE_SLOTS) + m_currentSlot) = 100.0 - ((float)idleDelta / onePercent);
			}
			else
			{
				*(m_cpuUsage + (cpu * CPU_USAGE_SLOTS) + m_currentSlot) = 0;
			}

			m_user[cpu] = user;
			m_system[cpu] = system;
			m_idle[cpu] = idle;
			m_iowait[cpu] = iowait;
		}
		free(cpuStats);
	}
	else
	{
		AgentWriteDebugLog(4, "AIX: call to perfstat_cpu_total failed (%s)", strerror(errno));
	}

	// go to the next slot
	m_currentSlot++;
	MutexUnlock(m_cpuUsageMutex);
}


//
// Collector thread
//

static THREAD_RESULT THREAD_CALL CpuUsageCollectorThread(void *arg)
{
	AgentWriteDebugLog(1, "CPU usage collector thread started");
	while(m_stopCollectorThread == false)
	{
		CpuUsageCollector();
		ThreadSleepMs(1000); // sleep 1 second
	}
	AgentWriteDebugLog(1, "CPU usage collector thread stopped");
	return THREAD_OK;
}


//
// Start CPU usage collector
//

void StartCpuUsageCollector()
{
	int i, j;

	m_cpuUsageMutex = MutexCreate();

#define SIZE sizeof(float) * CPU_USAGE_SLOTS * MAX_CPU
#define ALLOCATE_AND_CLEAR(x) x = (float *)malloc(SIZE); memset(x, 0, SIZE);
	ALLOCATE_AND_CLEAR(m_cpuUsage);
	ALLOCATE_AND_CLEAR(m_cpuUsageUser);
	ALLOCATE_AND_CLEAR(m_cpuUsageSystem);
	ALLOCATE_AND_CLEAR(m_cpuUsageIdle);
	ALLOCATE_AND_CLEAR(m_cpuUsageIoWait);
#undef ALLOCATE_AND_CLEAR
#undef SIZE

#define CLEAR(x) memset(x, 0, sizeof(uint64_t) * MAX_CPU);
	CLEAR(m_user)
	CLEAR(m_system)
	CLEAR(m_idle)
	CLEAR(m_iowait)
#undef CLEAR

	// get initial count of user/system/idle time
	m_currentSlot = 0;
	CpuUsageCollector();

	sleep(1);

	// fill first slot with u/s/i delta
	m_currentSlot = 0;
	CpuUsageCollector();

	// fill all slots with current cpu usage
#define FILL(x) memcpy(x + i, x, sizeof(float));
	for (i = 0; i < (CPU_USAGE_SLOTS * MAX_CPU) - 1; i++)
	{
		FILL(m_cpuUsage);
		FILL(m_cpuUsageUser);
		FILL(m_cpuUsageSystem);
		FILL(m_cpuUsageIdle);
		FILL(m_cpuUsageIoWait);
	}
#undef FILL

	// start collector
	m_cpuUsageCollector = ThreadCreateEx(CpuUsageCollectorThread, 0, NULL);
}


//
// Shutdown CPU usage collector
//

void ShutdownCpuUsageCollector()
{
	m_stopCollectorThread = true;
	ThreadJoin(m_cpuUsageCollector);
	MutexDestroy(m_cpuUsageMutex);

	free(m_cpuUsage);
	free(m_cpuUsageUser);
	free(m_cpuUsageSystem);
	free(m_cpuUsageIdle);
	free(m_cpuUsageIoWait);
}


//
// Get usage value for given CPU, metric, and interval
//

static void GetUsage(int source, int cpu, int count, char *value)
{
	float *table;
	switch (source)
	{
		case CPU_USAGE_OVERAL:
			table = (float *)m_cpuUsage;
			break;
		case CPU_USAGE_USER:
			table = (float *)m_cpuUsageUser;
			break;
		case CPU_USAGE_SYSTEM:
			table = (float *)m_cpuUsageSystem;
			break;
		case CPU_USAGE_IDLE:
			table = (float *)m_cpuUsageIdle;
			break;
		case CPU_USAGE_IOWAIT:
			table = (float *)m_cpuUsageIoWait;
			break;
		default:
			table = (float *)m_cpuUsage;
			break;
	}

	table += cpu * CPU_USAGE_SLOTS;

	float usage = 0;
	float *p = table + m_currentSlot - 1;

	MutexLock(m_cpuUsageMutex, INFINITE);
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


//
// Handler for System.CPU.Usage.* parameters
//

LONG H_CpuUsage(const char *pszParam, const char *pArg, char *pValue)
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


//
// Handler for System.CPU.Usage.*(*) parameters (CPU-specific versions)
//

LONG H_CpuUsageEx(const char *pszParam, const char *pArg, char *pValue)
{
	int count, cpu;
	char buffer[256], *eptr;
	struct CpuUsageParam *p = (struct CpuUsageParam *)pValue;

	if (!AgentGetParameterArg(pszParam, 1, buffer, 256))
		return SYSINFO_RC_UNSUPPORTED;
		
	cpu = strtol(buffer, &eptr, 0);
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


//
// Handler for System.CPU.Count parameter
//

LONG H_CpuCount(const char *pszParam, const char *pArg, char *pValue)
{
	ret_uint(pValue, (m_maxCPU > 0) ? m_maxCPU : 1);
	return SYSINFO_RC_SUCCESS;
}
