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

#include "cpu.h"

static Collector *collector = nullptr;
// m_cpuUsageMutex must be held to access `collector`, thread and its internals
static Mutex m_cpuUsageMutex(MutexType::FAST);

/**
 * CPU usage collector thread
 */
static void CpuUsageCollectorThread()
{
   nxlog_debug_tag(DEBUG_TAG, 9, _T("CPU usage collector thread started"));

   m_cpuUsageMutex.lock();
   while(collector->m_stopThread == false)
   {
      collector->Collect();
      m_cpuUsageMutex.unlock();
      ThreadSleepMs(1000); // sleep 1 second
      m_cpuUsageMutex.lock();
   }
   m_cpuUsageMutex.unlock();
   nxlog_debug_tag(DEBUG_TAG, 9, _T("CPU usage collector thread stopped"));
}

/**
 * Start CPU usage collector
 */
void StartCpuUsageCollector()
{
   m_cpuUsageMutex.lock();
   if (collector != nullptr)
   {
      nxlog_write(NXLOG_ERROR, _T("CPU Usage Collector extraneous initialization detected!"));
   }
   assert(collector == nullptr);
   collector = new Collector();

   // start collector
   collector->m_thread = ThreadCreateEx(CpuUsageCollectorThread);
   m_cpuUsageMutex.unlock();
}

/**
 * Shutdown CPU usage collector
 */
void ShutdownCpuUsageCollector()
{
   m_cpuUsageMutex.lock();
   collector->m_stopThread = true;
   m_cpuUsageMutex.unlock();
   ThreadJoin(collector->m_thread);
   m_cpuUsageMutex.lock();
   delete collector;
   collector = nullptr;
   m_cpuUsageMutex.unlock();
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
        enum CpuUsageSource source = (enum CpuUsageSource)CPU_USAGE_PARAM_SOURCE(pArg);
        m_cpuUsageMutex.lock();
        float usage = collector->GetTotalUsage(source, count);
        ret_double(pValue, usage);
        m_cpuUsageMutex.unlock();
	return SYSINFO_RC_SUCCESS;
}

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
   enum CpuUsageSource source = (enum CpuUsageSource)CPU_USAGE_PARAM_SOURCE(pArg);
   m_cpuUsageMutex.lock();
   if (cpu >= collector->m_perCore.size())
   {
      ret = SYSINFO_RC_UNSUPPORTED;
   }
   else
   {
      float usage = collector->GetCoreUsage(source, cpu, count);
      ret_double(pValue, usage);
      ret = SYSINFO_RC_SUCCESS;
   }
   m_cpuUsageMutex.unlock();
   return ret;
}

/**
 * Handler for System.CPU.Count parameter
 */
LONG H_CpuCount(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   m_cpuUsageMutex.lock();
   ret_uint(pValue, collector->m_perCore.size());
   m_cpuUsageMutex.unlock();
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
   m_cpuUsageMutex.lock();
   ret_uint(value, collector->m_cpuContextSwitches);
   m_cpuUsageMutex.unlock();
   return SYSINFO_RC_SUCCESS;
}

/*
 * Handler for CPU Interrupts parameter
 */
LONG H_CpuInterrupts(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   m_cpuUsageMutex.lock();
   ret_uint(value, collector->m_cpuInterrupts);
   m_cpuUsageMutex.unlock();
   return SYSINFO_RC_SUCCESS;
}
