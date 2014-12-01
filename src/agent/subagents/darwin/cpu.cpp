/* 
** netxms subagent for darwin
** copyright (c) 2012 alex kirhenshtein
**
** this program is free software; you can redistribute it and/or modify
** it under the terms of the gnu general public license as published by
** the free software foundation; either version 2 of the license, or
** (at your option) any later version.
**
** this program is distributed in the hope that it will be useful,
** but without any warranty; without even the implied warranty of
** merchantability or fitness for a particular purpose.  see the
** gnu general public license for more details.
**
** you should have received a copy of the gnu general public license
** along with this program; if not, write to the free software
** foundation, inc., 675 mass ave, cambridge, ma 02139, usa.
**
**/

//#undef _XOPEN_SOURCE

#include <nms_common.h>
#include <nms_agent.h>

#include <sys/sysctl.h>
#include <sys/utsname.h>
#include "cpu.h"


#define CPU_USAGE_SLOTS			900 // 60 sec * 15 min => 900 sec
// 64 + 1 for overal
#define MAX_CPU					(64 + 1)

static THREAD m_cpuUsageCollector = INVALID_THREAD_HANDLE;
static MUTEX m_cpuUsageMutex = INVALID_MUTEX_HANDLE;
static bool volatile m_stopCollectorThread = false;
static uint64_t m_user[MAX_CPU];
static uint64_t m_system[MAX_CPU];
static uint64_t m_idle[MAX_CPU];
static float *m_cpuUsage;
static float *m_cpuUsageUser;
static float *m_cpuUsageSystem;
static float *m_cpuUsageIdle;
static int m_currentSlot = 0;
static int m_maxCPU = 0;


static void CpuUsageCollector();
static THREAD_RESULT THREAD_CALL CpuUsageCollectorThread(void *pArg);



LONG H_CpuUsage(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
  return SYSINFO_RC_ERROR;
}

LONG H_CpuUsageEx(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
  return SYSINFO_RC_ERROR;
}

void StartCpuUsageCollector(void)
{
	int i, j;

	m_cpuUsageMutex = MutexCreate();

#define SIZE sizeof(float) * CPU_USAGE_SLOTS * MAX_CPU
#define ALLOCATE_AND_CLEAR(x) x = (float *)malloc(SIZE); memset(x, 0, SIZE);
	ALLOCATE_AND_CLEAR(m_cpuUsage);
	ALLOCATE_AND_CLEAR(m_cpuUsageUser);
	ALLOCATE_AND_CLEAR(m_cpuUsageSystem);
	ALLOCATE_AND_CLEAR(m_cpuUsageIdle);
#undef ALLOCATE_AND_CLEAR
#undef SIZE

#define CLEAR(x) memset(x, 0, sizeof(uint64_t) * MAX_CPU);
	CLEAR(m_user)
	CLEAR(m_system)
	CLEAR(m_idle)
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
	free(m_cpuUsageSystem);
}

static void CpuUsageCollector()
{
  return; // disable for now

	FILE *hStat = fopen("/proc/stat", "r");

	if (hStat == NULL)
	{
		// TODO: logging
		return;
	}

	uint64_t user = 0, system = 0, idle = 0;
	unsigned int cpu = 0;
	unsigned int maxCpu = 0;
	char buffer[1024];

	MutexLock(m_cpuUsageMutex);
	if (m_currentSlot == CPU_USAGE_SLOTS)
	{
		m_currentSlot = 0;
	}

	// scan for all CPUs
	while (1)
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
			//ret = sscanf(buffer, "cpu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
			//		&user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal, &guest);
      ret = -1;
		}
		else
		{
			//ret = sscanf(buffer, "cpu%u %llu %llu %llu %llu %llu %llu %llu %llu %llu",
			//		&cpu, &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal, &guest);
      ret = -1;
			cpu++;
		}

		if (ret < 4)
			continue;

		maxCpu = max(cpu, maxCpu);

		uint64_t userDelta, systemDelta, idleDelta;

		// TODO: handle overflow
		userDelta = user - m_user[cpu];
		systemDelta = system - m_system[cpu];
		idleDelta = idle - m_idle[cpu];

		uint64_t totalDelta = userDelta + systemDelta + idleDelta;

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
		m_system[cpu] = system;
		m_idle[cpu] = idle;
	}

	/* go to the next slot */
	m_currentSlot++;
	MutexUnlock(m_cpuUsageMutex);

	fclose(hStat);
	m_maxCPU = maxCpu - 1;
}

static THREAD_RESULT THREAD_CALL CpuUsageCollectorThread(void *pArg)
{
	//setlocale(LC_NUMERIC, "C");
	while(m_stopCollectorThread == false)
	{
		CpuUsageCollector();
		ThreadSleepMs(1000); // sleep 1 second
	}

	return THREAD_OK;
}
