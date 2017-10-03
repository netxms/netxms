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

static THREAD m_cpuUsageCollector = INVALID_THREAD_HANDLE;
static MUTEX m_cpuUsageMutex = INVALID_MUTEX_HANDLE;
static bool volatile m_stopCollectorThread = false;
static UINT64 *m_user;
static UINT64 *m_nice;
static UINT64 *m_system;
static UINT64 *m_idle;
static UINT64 *m_iowait;
static UINT64 *m_irq;
static UINT64 *m_softirq;
static UINT64 *m_steal;
static UINT64 *m_guest;
static UINT64 m_cpuInterrupts;
static UINT64 m_cpuContextSwitches;
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

/**
 * CPU usage collector
 */
static void CpuUsageCollector()
{
	FILE *hStat = fopen("/proc/stat", "r");

	if (hStat == NULL)
	{
		AgentWriteDebugLog(2, _T("Cannot open /proc/stat"));
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

		int ret;
		if (buffer[0] == 'c' && buffer[1] == 'p' && buffer[2] == 'u')
		{
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
		}
		else if (buffer[0] == 'i' && buffer[1] == 'n' && buffer[2] == 't' && buffer[3] == 'r')
		{
         ret = sscanf(buffer, "intr %llu", &m_cpuInterrupts);
		}
		else if (buffer[0] == 'c' && buffer[1] == 't' && buffer[2] == 'x' && buffer[3] == 't')
      {
         ret = sscanf(buffer, "ctxt %llu", &m_cpuContextSwitches);
      }
		else
		   continue;

		if (ret < 4)
			continue;

		maxCpu = std::max(cpu, maxCpu);

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

/**
 * CPU usage collector thread
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
	int i, j;

	m_cpuUsageMutex = MutexCreate();

   UINT32 cpuCount = sysconf(_SC_NPROCESSORS_ONLN);
   m_cpuUsage = (float *)calloc(CPU_USAGE_SLOTS, sizeof(float) * (cpuCount + 1));
   m_cpuUsageUser = (float *)calloc(CPU_USAGE_SLOTS, sizeof(float) * (cpuCount + 1));
   m_cpuUsageNice = (float *)calloc(CPU_USAGE_SLOTS, sizeof(float) * (cpuCount + 1));
   m_cpuUsageSystem = (float *)calloc(CPU_USAGE_SLOTS, sizeof(float) * (cpuCount + 1));
   m_cpuUsageIdle = (float *)calloc(CPU_USAGE_SLOTS, sizeof(float) * (cpuCount + 1));
   m_cpuUsageIoWait = (float *)calloc(CPU_USAGE_SLOTS, sizeof(float) * (cpuCount + 1));
   m_cpuUsageIrq = (float *)calloc(CPU_USAGE_SLOTS, sizeof(float) * (cpuCount + 1));
   m_cpuUsageSoftIrq = (float *)calloc(CPU_USAGE_SLOTS, sizeof(float) * (cpuCount + 1));
   m_cpuUsageSteal = (float *)calloc(CPU_USAGE_SLOTS, sizeof(float) * (cpuCount + 1));
   m_cpuUsageGuest = (float *)calloc(CPU_USAGE_SLOTS, sizeof(float) * (cpuCount + 1));

   m_user = (UINT64 *)calloc(CPU_USAGE_SLOTS, sizeof(UINT64) * (cpuCount + 1));
   m_nice = (UINT64 *)calloc(CPU_USAGE_SLOTS, sizeof(UINT64) * (cpuCount + 1));
   m_system = (UINT64 *)calloc(CPU_USAGE_SLOTS, sizeof(UINT64) * (cpuCount + 1));
   m_idle = (UINT64 *)calloc(CPU_USAGE_SLOTS, sizeof(UINT64) * (cpuCount + 1));
   m_iowait = (UINT64 *)calloc(CPU_USAGE_SLOTS, sizeof(UINT64) * (cpuCount + 1));
   m_irq = (UINT64 *)calloc(CPU_USAGE_SLOTS, sizeof(UINT64) * (cpuCount + 1));
   m_softirq = (UINT64 *)calloc(CPU_USAGE_SLOTS, sizeof(UINT64) * (cpuCount + 1));
   m_steal = (UINT64 *)calloc(CPU_USAGE_SLOTS, sizeof(UINT64) * (cpuCount + 1));
   m_guest = (UINT64 *)calloc(CPU_USAGE_SLOTS, sizeof(UINT64) * (cpuCount + 1));

	// get initial count of user/system/idle time
	m_currentSlot = 0;
	CpuUsageCollector();

	sleep(1);

	// fill first slot with u/s/i delta
	m_currentSlot = 0;
	CpuUsageCollector();

	// fill all slots with current cpu usage
#define FILL(x) memcpy(x + i, x, sizeof(float));
	for (i = 0; i < (CPU_USAGE_SLOTS * cpuCount) - 1; i++)
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
	free(m_cpuUsageIoWait);
	free(m_cpuUsageIrq);
	free(m_cpuUsageSoftIrq);
	free(m_cpuUsageSteal);
	free(m_cpuUsageGuest);
	free(m_user);
	free(m_nice);
	free(m_system);
	free(m_idle);
	free(m_iowait);
	free(m_irq);
	free(m_softirq);
	free(m_steal);
	free(m_guest);
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

/**
 * Handler for System.CPU.Count parameter
 */
LONG H_CpuCount(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
	ret_uint(pValue, m_maxCPU);
	return SYSINFO_RC_SUCCESS;
}

/**
 * CPU info structure
 */
struct CPU_INFO
{
   int id;
   int coreId;
   int physicalId;
   char model[64];
   long frequency;
   int cacheSize;
};

/**
 * Read /proc/cpuinfo
 */
static int ReadCpuInfo(CPU_INFO *info, int size)
{
   FILE *f = fopen("/proc/cpuinfo", "r");
   if (f == NULL)
   {
      AgentWriteDebugLog(2, _T("Cannot open /proc/cpuinfo"));
      return -1;
   }

   int count = -1;
   char buffer[256];
   while(!feof(f))
   {
      if (fgets(buffer, sizeof(buffer), f) == NULL)
         break;
      char *s = strchr(buffer, '\n');
      if (s != NULL)
         *s = 0;

      s = strchr(buffer, ':');
      if (s == NULL)
         continue;

      *s = 0;
      s++;
      StrStripA(buffer);
      StrStripA(s);

      if (!strcmp(buffer, "processor"))
      {
         count++;
         memset(&info[count], 0, sizeof(CPU_INFO));
         info[count].id = (int)strtol(s, NULL, 10);
         continue;
      }

      if (count == -1)
         continue;

      if (!strcmp(buffer, "model name"))
      {
         strncpy(info[count].model, s, 63);
      }
      else if (!strcmp(buffer, "cpu MHz"))
      {
         char *eptr;
         info[count].frequency = strtol(s, &eptr, 10) * 1000;
         if (*eptr == '.')
         {
            eptr[4] = 0;
            info[count].frequency += strtol(eptr + 1, NULL, 10);
         }
      }
      else if (!strcmp(buffer, "cache size"))
      {
         info[count].cacheSize = (int)strtol(s, NULL, 10);
      }
      else if (!strcmp(buffer, "physical id"))
      {
         info[count].physicalId = (int)strtol(s, NULL, 10);
      }
      else if (!strcmp(buffer, "core id"))
      {
         info[count].coreId = (int)strtol(s, NULL, 10);
      }
   }

   fclose(f);
   return count + 1;
}

/**
 * Handler for CPU info parameters
 */
LONG H_CpuInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   CPU_INFO cpuInfo[256];
   int count = ReadCpuInfo(cpuInfo, 256);
   if (count <= 0)
      return SYSINFO_RC_ERROR;

   TCHAR buffer[32];
   AgentGetParameterArg(param, 1, buffer, 32);
   int cpuId = (int)_tcstol(buffer, NULL, 0);

   CPU_INFO *cpu = NULL;
   for(int i = 0; i < count; i++)
   {
      if (cpuInfo[i].id == cpuId)
      {
         cpu = &cpuInfo[i];
         break;
      }
   }
   if (cpu == NULL)
      return SYSINFO_RC_NO_SUCH_INSTANCE;

   switch(*arg)
   {
      case 'C':   // Core ID
         ret_int(value, cpu->coreId);
         break;
      case 'F':   // Frequency
         _sntprintf(value, MAX_RESULT_LENGTH, _T("%d.%03d"), cpu->frequency / 1000, cpu->frequency % 1000);
         break;
      case 'M':   // Model
         ret_mbstring(value, cpu->model);
         break;
      case 'P':   // Physical ID
         ret_int(value, cpu->physicalId);
         break;
      case 'S':   // Cache size
         ret_int(value, cpu->cacheSize);
         break;
      default:
         return SYSINFO_RC_UNSUPPORTED;
   }

   return SYSINFO_RC_SUCCESS;
}

/*
 * Handler for CPU Context Switch parameter
 */
LONG H_CpuCswitch(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   ret_uint(value, m_cpuContextSwitches);
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
