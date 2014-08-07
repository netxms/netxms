/* 
** NetXMS subagent for GNU/Linux
** Copyright (C) 2004 - 2007 Alex Kirhenshtein
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

#include "linux_subagent.h"


#define CPU_USAGE_SLOTS			900 // 60 sec * 15 min => 900 sec
// 64 + 1 for overal
#define MAX_CPU					(64 + 1)

static THREAD m_cpuUsageCollector = INVALID_THREAD_HANDLE;
static MUTEX m_cpuUsageMutex = INVALID_MUTEX_HANDLE;
static bool volatile m_stopCollectorThread = false;
static UINT64 m_user[MAX_CPU];
static UINT64 m_nice[MAX_CPU];
static UINT64 m_system[MAX_CPU];
static UINT64 m_idle[MAX_CPU];
static UINT64 m_iowait[MAX_CPU];
static UINT64 m_irq[MAX_CPU];
static UINT64 m_softirq[MAX_CPU];
static UINT64 m_steal[MAX_CPU];
static UINT64 m_guest[MAX_CPU];
static float *m_cpuUsage;
static float *m_cpuUsageUser;
static float *m_cpuUsageNice;
static float *m_cpuUsageSystem;
static float *m_cpuUsageIdle;
static float *m_cpuUsageIoWait;
static float *m_cpuUsageIrq;
static float *m_cpuUsageSoftIrq;
static float *m_cpuUsageSteal;
static float *m_cpuUsageGuest;
static int m_currentSlot = 0;
static int m_maxCPU = 0;

static void CpuUsageCollector()
{
	FILE *hStat = fopen("/proc/stat", "r");

	if (hStat == NULL)
	{
		// TODO: logging
		return;
	}

	UINT64 user, nice, system, idle;
	UINT64 iowait = 0, irq = 0, softirq = 0; // 2.6
	UINT64 steal = 0; // 2.6.11
	UINT64 guest = 0; // 2.6.24
	unsigned int cpu = 0;
	unsigned int maxCpu = 0;
	char buffer[1024];

	MutexLock(m_cpuUsageMutex);
	if (m_currentSlot == CPU_USAGE_SLOTS)
	{
		m_currentSlot = 0;
	}

	// scan for all CPUs
	while(true)
	{
		if (fgets(buffer, sizeof(buffer), hStat) == NULL)
			break;

		if (buffer[0] != 'c' || buffer[1] != 'p' || buffer[2] != 'u')
			continue;

		int ret;
		if (buffer[3] == ' ')
		{
			// "cpu ..." - Overal
			cpu = 0;
			ret = sscanf(buffer, "cpu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
					&user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal, &guest);
		}
		else
		{
			ret = sscanf(buffer, "cpu%u %llu %llu %llu %llu %llu %llu %llu %llu %llu",
					&cpu, &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal, &guest);
			cpu++;
		}

		if (ret < 4)
			continue;

		maxCpu = max(cpu, maxCpu);

		UINT64 userDelta, niceDelta, systemDelta, idleDelta;
		UINT64 iowaitDelta, irqDelta, softirqDelta, stealDelta;
		UINT64 guestDelta;

#define DELTA(x, y) (((x) > (y)) ? ((x) - (y)) : 0)
		userDelta = DELTA(user, m_user[cpu]);
		niceDelta = DELTA(nice, m_nice[cpu]);
		systemDelta = DELTA(system, m_system[cpu]);
		idleDelta = DELTA(idle, m_idle[cpu]);
		iowaitDelta = DELTA(iowait, m_iowait[cpu]);
		irqDelta = DELTA(irq, m_irq[cpu]);
		softirqDelta = DELTA(softirq, m_softirq[cpu]);
		stealDelta = DELTA(steal, m_steal[cpu]); // steal=time spent in virtualization stuff (xen).
		guestDelta = DELTA(guest, m_guest[cpu]); //
#undef DELTA

		UINT64 totalDelta = userDelta + niceDelta + systemDelta + idleDelta + iowaitDelta + irqDelta + softirqDelta + stealDelta + guestDelta;
		float onePercent = (float)totalDelta / 100.0; // 1% of total
		if (onePercent == 0)
		{
			onePercent = 1; // TODO: why 1?
		}
			
		/* update detailed stats */
#define UPDATE(delta, target) { \
			if (delta > 0) { *(target + (cpu * CPU_USAGE_SLOTS) + m_currentSlot) = (float)delta / onePercent; } \
			else { *(target + (cpu * CPU_USAGE_SLOTS) + m_currentSlot) = 0; } \
		}

		UPDATE(userDelta, m_cpuUsageUser);
		UPDATE(niceDelta, m_cpuUsageNice);
		UPDATE(systemDelta, m_cpuUsageSystem);
		UPDATE(idleDelta, m_cpuUsageIdle);
		UPDATE(iowaitDelta, m_cpuUsageIoWait);
		UPDATE(irqDelta, m_cpuUsageIrq);
		UPDATE(softirqDelta, m_cpuUsageSoftIrq);
		UPDATE(stealDelta, m_cpuUsageSteal);
		UPDATE(guestDelta, m_cpuUsageGuest);

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
		m_iowait[cpu] = iowait;
		m_irq[cpu] = irq;
		m_softirq[cpu] = softirq;
		m_steal[cpu] = steal;
		m_guest[cpu] = guest;
	}

	/* go to the next slot */
	m_currentSlot++;
	MutexUnlock(m_cpuUsageMutex);

	fclose(hStat);
	m_maxCPU = maxCpu;
}

static THREAD_RESULT THREAD_CALL CpuUsageCollectorThread(void *pArg)
{
	while(m_stopCollectorThread == false)
	{
		CpuUsageCollector();
		ThreadSleepMs(1000); // sleep 1 second
	}

	return THREAD_OK;
}

void StartCpuUsageCollector(void)
{
	int i, j;

	m_cpuUsageMutex = MutexCreate();

#define SIZE sizeof(float) * CPU_USAGE_SLOTS * MAX_CPU
#define ALLOCATE_AND_CLEAR(x) x = (float *)malloc(SIZE); memset(x, 0, SIZE);
	ALLOCATE_AND_CLEAR(m_cpuUsage);
	ALLOCATE_AND_CLEAR(m_cpuUsageUser);
	ALLOCATE_AND_CLEAR(m_cpuUsageNice);
	ALLOCATE_AND_CLEAR(m_cpuUsageSystem);
	ALLOCATE_AND_CLEAR(m_cpuUsageIdle);
	ALLOCATE_AND_CLEAR(m_cpuUsageIoWait);
	ALLOCATE_AND_CLEAR(m_cpuUsageIrq);
	ALLOCATE_AND_CLEAR(m_cpuUsageSoftIrq);
	ALLOCATE_AND_CLEAR(m_cpuUsageSteal);
	ALLOCATE_AND_CLEAR(m_cpuUsageGuest);
#undef ALLOCATE_AND_CLEAR
#undef SIZE

#define CLEAR(x) memset(x, 0, sizeof(UINT64) * MAX_CPU);
	CLEAR(m_user)
	CLEAR(m_nice)
	CLEAR(m_system)
	CLEAR(m_idle)
	CLEAR(m_iowait)
	CLEAR(m_irq)
	CLEAR(m_softirq)
	CLEAR(m_steal)
	CLEAR(m_guest)
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
			FILL(m_cpuUsageNice);
			FILL(m_cpuUsageSystem);
			FILL(m_cpuUsageIdle);
			FILL(m_cpuUsageIoWait);
			FILL(m_cpuUsageIrq);
			FILL(m_cpuUsageSoftIrq);
			FILL(m_cpuUsageSteal);
			FILL(m_cpuUsageGuest);
	}
#undef FILL

	// start collector
	m_cpuUsageCollector = ThreadCreateEx(CpuUsageCollectorThread, 0, NULL);
}

void ShutdownCpuUsageCollector(void)
{
	m_stopCollectorThread = true;
	ThreadJoin(m_cpuUsageCollector);
	MutexDestroy(m_cpuUsageMutex);

	free(m_cpuUsage);
	free(m_cpuUsageUser);
	free(m_cpuUsageNice);
	free(m_cpuUsageSystem);
	free(m_cpuUsageIdle);
	free(m_cpuUsageIoWait);
	free(m_cpuUsageIrq);
	free(m_cpuUsageSoftIrq);
	free(m_cpuUsageSteal);
	free(m_cpuUsageGuest);
}

static void GetUsage(int source, int cpu, int count, TCHAR *value)
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
		case CPU_USAGE_NICE:
			table = (float *)m_cpuUsageNice;
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
		case CPU_USAGE_IRQ:
			table = (float *)m_cpuUsageIrq;
			break;
		case CPU_USAGE_SOFTIRQ:
			table = (float *)m_cpuUsageSoftIrq;
			break;
		case CPU_USAGE_STEAL:
			table = (float *)m_cpuUsageSteal;
			break;
		case CPU_USAGE_GUEST:
			table = (float *)m_cpuUsageGuest;
			break;
		default:
			table = (float *)m_cpuUsage;
	}

	table += cpu * CPU_USAGE_SLOTS;

	float usage = 0;

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

LONG H_CpuUsage(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
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

LONG H_CpuUsageEx(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
{
	int count, cpu;
	TCHAR buffer[256], *eptr;
	struct CpuUsageParam *p = (struct CpuUsageParam *)pValue;

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

LONG H_CpuCount(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
{
	ret_uint(pValue, m_maxCPU);
	return SYSINFO_RC_SUCCESS;
}
