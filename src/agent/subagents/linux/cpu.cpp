/* 
** NetXMS subagent for GNU/Linux
** Copyright (C) 2004-2024 Raden Solutions
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

#include "cpu.h"

/**
 * Collector object
 */
static Collector s_collector;

/**
 * Collector access mutex.
 * s_cpuUsageMutex must be held to access `s_collector` and its internals
 */
static Mutex s_cpuUsageMutex(MutexType::FAST);

/**
 * CPU usage collector thread
 */
static void CpuUsageCollectorThread()
{
   nxlog_debug_tag(DEBUG_TAG, 3, _T("CPU usage collector thread started"));
   do
   {
      s_cpuUsageMutex.lock();
      s_collector.collect();
      s_cpuUsageMutex.unlock();
   } while(!AgentSleepAndCheckForShutdown(1000));
   nxlog_debug_tag(DEBUG_TAG, 3, _T("CPU usage collector thread stopped"));
}

/**
 * Collector thread
 */
static THREAD s_collectorThread = INVALID_THREAD_HANDLE;

/**
 * Start CPU usage collector
 */
void StartCpuUsageCollector()
{
   s_collectorThread = ThreadCreateEx(CpuUsageCollectorThread);
}

/**
 * Shutdown CPU usage collector
 */
void ShutdownCpuUsageCollector()
{
   ThreadJoin(s_collectorThread);
}

/**
 * Handler for System.CPU.Usage* metrics
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
	CpuUsageSource source = (CpuUsageSource)CPU_USAGE_PARAM_SOURCE(pArg);

   s_cpuUsageMutex.lock();
   ret_double(pValue, s_collector.getTotalUsage(source, count));
   s_cpuUsageMutex.unlock();

   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.CPU.Usage* metrics for specific CPU
 */
LONG H_CpuUsageEx(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   LONG ret;
   TCHAR buffer[256] = {0,}, *eptr;
   if (!AgentGetParameterArg(pszParam, 1, buffer, 256))
      return SYSINFO_RC_UNSUPPORTED;

   int cpu = _tcstol(buffer, &eptr, 0);
   if ((*eptr != 0) || (cpu < 0))
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
   CpuUsageSource source = (CpuUsageSource)CPU_USAGE_PARAM_SOURCE(pArg);

   s_cpuUsageMutex.lock();
   if (cpu < s_collector.m_perCore.size())
   {
      ret_double(pValue, s_collector.getCoreUsage(source, cpu, count));
      ret = SYSINFO_RC_SUCCESS;
   }
   else
   {
      ret = SYSINFO_RC_UNSUPPORTED;
   }
   s_cpuUsageMutex.unlock();
   return ret;
}

/**
 * Handler for System.CPU.Count* parameters
 */
LONG H_CpuCount(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   uint32_t retval;
   LONG status = CountRangesFile(reinterpret_cast<const char*>(arg), &retval);
   ret_uint(value, retval);
   return status;
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
   int64_t frequency;
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
   int cpuId = (int)_tcstol(buffer, nullptr, 0);

   CPU_INFO *cpu = nullptr;
   for(int i = 0; i < count; i++)
   {
      if (cpuInfo[i].id == cpuId)
      {
         cpu = &cpuInfo[i];
         break;
      }
   }
   if (cpu == nullptr)
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
   s_cpuUsageMutex.lock();
   ret_uint(value, s_collector.m_cpuContextSwitches);
   s_cpuUsageMutex.unlock();
   return SYSINFO_RC_SUCCESS;
}

/*
 * Handler for CPU Interrupts parameter
 */
LONG H_CpuInterrupts(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   s_cpuUsageMutex.lock();
   ret_uint(value, s_collector.m_cpuInterrupts);
   s_cpuUsageMutex.unlock();
   return SYSINFO_RC_SUCCESS;
}
