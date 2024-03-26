/* 
** NetXMS subagent for GNU/Linux
** Copyright (C) 2004-2022 Raden Solutions
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
static Mutex m_cpuUsageMutex(MutexType::FAST);
static bool volatile m_stopCollectorThread = false;
static uint64_t *m_user;
static uint64_t *m_nice;
static uint64_t *m_system;
static uint64_t *m_idle;
static uint64_t *m_iowait;
static uint64_t *m_irq;
static uint64_t *m_softirq;
static uint64_t *m_steal;
static uint64_t *m_guest;
static uint64_t m_cpuInterrupts;
static uint64_t m_cpuContextSwitches;
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
	if (hStat == nullptr)
	{
		nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot open /proc/stat"));
		return;
	}

	uint64_t user, nice, system, idle;
	uint64_t iowait = 0, irq = 0, softirq = 0; // 2.6
	uint64_t steal = 0; // 2.6.11
	uint64_t guest = 0; // 2.6.24
	uint32_t cpu = 0;
	uint32_t maxCpu = 0;
	char buffer[1024];

	m_cpuUsageMutex.lock();
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
            ret = sscanf(buffer, "cpu " UINT64_FMTA " " UINT64_FMTA " " UINT64_FMTA " " UINT64_FMTA " " UINT64_FMTA " " UINT64_FMTA " " UINT64_FMTA " " UINT64_FMTA " " UINT64_FMTA,
                  &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal, &guest);
         }
         else
         {
            ret = sscanf(buffer, "cpu%u " UINT64_FMTA " " UINT64_FMTA " " UINT64_FMTA " " UINT64_FMTA " " UINT64_FMTA " " UINT64_FMTA " " UINT64_FMTA " " UINT64_FMTA " " UINT64_FMTA,
                  &cpu, &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal, &guest);
            cpu++;
         }
		}
		else if (buffer[0] == 'i' && buffer[1] == 'n' && buffer[2] == 't' && buffer[3] == 'r')
		{
         ret = sscanf(buffer, "intr " UINT64_FMTA, &m_cpuInterrupts);
		}
		else if (buffer[0] == 'c' && buffer[1] == 't' && buffer[2] == 'x' && buffer[3] == 't')
      {
         ret = sscanf(buffer, "ctxt " UINT64_FMTA, &m_cpuContextSwitches);
      }
		else
		{
		   continue;
		}

		if (ret < 4)
			continue;

		maxCpu = std::max(cpu, maxCpu);

		uint64_t userDelta, niceDelta, systemDelta, idleDelta;
		uint64_t iowaitDelta, irqDelta, softirqDelta, stealDelta;
		uint64_t guestDelta;

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

		uint64_t totalDelta = userDelta + niceDelta + systemDelta + idleDelta + iowaitDelta + irqDelta + softirqDelta + stealDelta + guestDelta;
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
	m_cpuUsageMutex.unlock();

	fclose(hStat);
	m_maxCPU = maxCpu;
}

/**
 * CPU usage collector thread
 */
static void CpuUsageCollectorThread()
{
   nxlog_debug_tag(DEBUG_TAG, 2, _T("CPU usage collector thread started"));
	while(m_stopCollectorThread == false)
	{
		CpuUsageCollector();
		ThreadSleepMs(1000); // sleep 1 second
	}
   nxlog_debug_tag(DEBUG_TAG, 2, _T("CPU usage collector thread stopped"));
}

/**
 * Get number of CPU from /proc/stat
 */
static uint32_t GetCpuCountFromStat()
{
   uint32_t count = 0;

   FILE *hStat = fopen("/proc/stat", "r");
   if (hStat == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot open /proc/stat"));
      return count;
   }

   char buffer[1024];
   while(true)
   {
      if (fgets(buffer, sizeof(buffer), hStat) == nullptr)
         break;

      if (buffer[0] == 'c' && buffer[1] == 'p' && buffer[2] == 'u' && buffer[3] != ' ')
      {
         uint32_t cpuIndex = 0;
         int r = sscanf(buffer, "cpu%u ", &cpuIndex);
         assert(cpuIndex + 1 > count);
         count = cpuIndex + 1;
      }
   }

   fclose(hStat);
   return count;
}

/**
 * Start CPU usage collector
 */
void StartCpuUsageCollector()
{
   uint32_t cpuCount = GetCpuCountFromStat();

   m_cpuUsage = MemAllocArray<float>(CPU_USAGE_SLOTS * (cpuCount + 1));
   m_cpuUsageUser = MemAllocArray<float>(CPU_USAGE_SLOTS * (cpuCount + 1));
   m_cpuUsageNice = MemAllocArray<float>(CPU_USAGE_SLOTS * (cpuCount + 1));
   m_cpuUsageSystem = MemAllocArray<float>(CPU_USAGE_SLOTS * (cpuCount + 1));
   m_cpuUsageIdle = MemAllocArray<float>(CPU_USAGE_SLOTS * (cpuCount + 1));
   m_cpuUsageIoWait = MemAllocArray<float>(CPU_USAGE_SLOTS * (cpuCount + 1));
   m_cpuUsageIrq = MemAllocArray<float>(CPU_USAGE_SLOTS * (cpuCount + 1));
   m_cpuUsageSoftIrq = MemAllocArray<float>(CPU_USAGE_SLOTS * (cpuCount + 1));
   m_cpuUsageSteal = MemAllocArray<float>(CPU_USAGE_SLOTS * (cpuCount + 1));
   m_cpuUsageGuest = MemAllocArray<float>(CPU_USAGE_SLOTS * (cpuCount + 1));

   m_user = MemAllocArray<uint64_t>(CPU_USAGE_SLOTS * (cpuCount + 1));
   m_nice = MemAllocArray<uint64_t>(CPU_USAGE_SLOTS * (cpuCount + 1));
   m_system = MemAllocArray<uint64_t>(CPU_USAGE_SLOTS * (cpuCount + 1));
   m_idle = MemAllocArray<uint64_t>(CPU_USAGE_SLOTS * (cpuCount + 1));
   m_iowait = MemAllocArray<uint64_t>(CPU_USAGE_SLOTS * (cpuCount + 1));
   m_irq = MemAllocArray<uint64_t>(CPU_USAGE_SLOTS * (cpuCount + 1));
   m_softirq = MemAllocArray<uint64_t>(CPU_USAGE_SLOTS * (cpuCount + 1));
   m_steal = MemAllocArray<uint64_t>(CPU_USAGE_SLOTS * (cpuCount + 1));
   m_guest = MemAllocArray<uint64_t>(CPU_USAGE_SLOTS * (cpuCount + 1));

	// get initial count of user/system/idle time
	m_currentSlot = 0;
	CpuUsageCollector();

	sleep(1);

	// fill first slot with u/s/i delta
	m_currentSlot = 0;
	CpuUsageCollector();

	// fill all slots with current cpu usage
#define FILL(x) memcpy(x + i, x, sizeof(float));
	for (uint32_t i = 0; i < (CPU_USAGE_SLOTS * cpuCount) - 1; i++)
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
	m_cpuUsageCollector = ThreadCreateEx(CpuUsageCollectorThread);
}

/**
 * Shutdown CPU usage collector
 */
void ShutdownCpuUsageCollector()
{
	m_stopCollectorThread = true;
	ThreadJoin(m_cpuUsageCollector);

	MemFree(m_cpuUsage);
	MemFree(m_cpuUsageUser);
	MemFree(m_cpuUsageNice);
	MemFree(m_cpuUsageSystem);
	MemFree(m_cpuUsageIdle);
	MemFree(m_cpuUsageIoWait);
	MemFree(m_cpuUsageIrq);
	MemFree(m_cpuUsageSoftIrq);
	MemFree(m_cpuUsageSteal);
	MemFree(m_cpuUsageGuest);
	MemFree(m_user);
	MemFree(m_nice);
	MemFree(m_system);
	MemFree(m_idle);
	MemFree(m_iowait);
	MemFree(m_irq);
	MemFree(m_softirq);
	MemFree(m_steal);
	MemFree(m_guest);
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
		case CPU_USAGE_IOWAIT:
			table = m_cpuUsageIoWait;
			break;
		case CPU_USAGE_IRQ:
			table = m_cpuUsageIrq;
			break;
		case CPU_USAGE_SOFTIRQ:
			table = m_cpuUsageSoftIrq;
			break;
		case CPU_USAGE_STEAL:
			table = m_cpuUsageSteal;
			break;
		case CPU_USAGE_GUEST:
			table = m_cpuUsageGuest;
			break;
		default:
			table = m_cpuUsage;
	}

   table += cpu * CPU_USAGE_SLOTS;
   float usage = 0;
   m_cpuUsageMutex.lock();

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

   m_cpuUsageMutex.unlock();

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
	TCHAR buffer[256], *eptr;
	if (!AgentGetParameterArg(pszParam, 1, buffer, 256))
		return SYSINFO_RC_UNSUPPORTED;
		
	int cpu = _tcstol(buffer, &eptr, 0);
	if ((*eptr != 0) || (cpu < 0) || (cpu >= m_maxCPU))
		return SYSINFO_RC_UNSUPPORTED;

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
   INT64 frequency;
   int cacheSize;
};

/**
 * Read /proc/cpuinfo
 */
static int ReadCpuInfo(CPU_INFO *info, int size)
{
   FILE *f = fopen("/proc/cpuinfo", "r");
   if (f == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot open /proc/cpuinfo"));
      return -1;
   }

   int count = -1;
   char buffer[256];
   while(!feof(f))
   {
      if (fgets(buffer, sizeof(buffer), f) == nullptr)
         break;
      char *s = strchr(buffer, '\n');
      if (s != nullptr)
         *s = 0;

      s = strchr(buffer, ':');
      if (s == nullptr)
         continue;

      *s = 0;
      s++;
      TrimA(buffer);
      TrimA(s);

      if (!strcmp(buffer, "processor"))
      {
         count++;
         memset(&info[count], 0, sizeof(CPU_INFO));
         info[count].id = (int)strtol(s, nullptr, 10);
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
         info[count].frequency = strtoll(s, &eptr, 10) * _LL(1000);
         if (*eptr == '.')
         {
            eptr[4] = 0;
            info[count].frequency += strtoll(eptr + 1, nullptr, 10);
         }
      }
      else if (!strcmp(buffer, "cache size"))
      {
         info[count].cacheSize = (int)strtol(s, nullptr, 10);
      }
      else if (!strcmp(buffer, "physical id"))
      {
         info[count].physicalId = (int)strtol(s, nullptr, 10);
      }
      else if (!strcmp(buffer, "core id"))
      {
         info[count].coreId = (int)strtol(s, nullptr, 10);
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
         _sntprintf(value, MAX_RESULT_LENGTH, _T("%d.%03d"),
               static_cast<int>(cpu->frequency / 1000), static_cast<int>(cpu->frequency % 1000));
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
