/* 
** netxms subagent for darwin
** copyright (c) 2021 Raden Solutions
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

#include <nms_agent.h>
#include <nms_util.h>

#include <mach/mach_host.h>
#include <mach/vm_map.h>

#include <sys/sysctl.h>
#include <sys/utsname.h>
#include <algorithm>

#include "cpu.h"
#include "darwin.h"

#define CPU_USAGE_SLOTS			900 // 60 sec * 15 min => 900 sec

// Per-core CPU usage history
typedef struct
{
   uint32_t prevUserUsage;
   uint32_t prevSystemUsage;
   uint32_t prevIdleUsage;
   uint32_t prevNiceUsage;
   uint32_t prevTotalUsage;

   // % of total usage
   float avgUserUsage[CPU_USAGE_SLOTS];
   float avgSystemUsage[CPU_USAGE_SLOTS];
   float avgIdleUsage[CPU_USAGE_SLOTS];
   float avgNiceUsage[CPU_USAGE_SLOTS];
   float avgBusyUsage[CPU_USAGE_SLOTS]; // User+System+Nice usage
} cpuUsageHistory;

static THREAD m_cpuUsageCollector = INVALID_THREAD_HANDLE;
static Mutex s_cpuUsageMutex(MutexType::FAST);
static bool volatile m_stopCollectorThread = false;
static cpuUsageHistory *s_cpuUsageHistory;
static unsigned int s_maxCPU;
static int s_currentSlot;

/**
 * Calculate average usage of selected type for specific CPU core over a number of iterations. Assumes s_cpuUsageMutex is locked when called
 */
static float AverageUsage(const float *const avgUsage, int iterationCount)
{
   float usage = 0;
   int counter = iterationCount;

   for (int i = (s_currentSlot - 1); counter > 0; i--)
   {
      if (i < 0)
      {
         i += CPU_USAGE_SLOTS;
      }

      usage += avgUsage[i];

      counter--;
   }

   return (usage / iterationCount);
}

/**
 * Parser function for CPU usage stats request, targetting all cores
 */
LONG H_CpuUsage(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   if (m_cpuUsageCollector == INVALID_THREAD_HANDLE)
   {
      return SYSINFO_RC_ERROR;
   }

   int count;

   switch(CPU_USAGE_PARAM_INTERVAL(pArg))
   {
      case INTERVAL_5MIN:
         count = 5 * 60;
         break;
      case INTERVAL_15MIN:
         count = 15 * 60;
         break;
      case INTERVAL_1MIN:
         count = 60;
         break;
      default:
         return SYSINFO_RC_UNSUPPORTED;
   }

   s_cpuUsageMutex.lock();

   // Calculate CPU usage estimate
   float usage = 0;

   for (int coreId = 0; coreId < s_maxCPU; coreId++)
   {
      // Get offset to desired memory type used
      switch (CPU_USAGE_PARAM_SOURCE(pArg))
      {
         case CPU_USAGE_OVERAL:
            usage += AverageUsage(s_cpuUsageHistory[coreId].avgBusyUsage, count);
            break;
         case CPU_USAGE_USER:
            usage += AverageUsage(s_cpuUsageHistory[coreId].avgUserUsage, count);
            break;
         case CPU_USAGE_NICE:
            usage += AverageUsage(s_cpuUsageHistory[coreId].avgNiceUsage, count);
            break;
         case CPU_USAGE_SYSTEM:
            usage += AverageUsage(s_cpuUsageHistory[coreId].avgSystemUsage, count);
            break;;
         case CPU_USAGE_IDLE:
            usage += AverageUsage(s_cpuUsageHistory[coreId].avgIdleUsage, count);
            break;
         default:
            return SYSINFO_RC_UNSUPPORTED;
      }
   }

   s_cpuUsageMutex.unlock();

   usage /= s_maxCPU;
   ret_double(pValue, usage);

   return SYSINFO_RC_SUCCESS;
}

/**
 * Parser function for CPU usage stats request, targetting specific core
 */
LONG H_CpuUsageEx(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   if (m_cpuUsageCollector == INVALID_THREAD_HANDLE)
   {
      return SYSINFO_RC_ERROR;
   }

   // Get core target parameter
   TCHAR buffer[256];

   if (!AgentGetParameterArg(pszParam, 1, buffer, 256))
   {
      return SYSINFO_RC_UNSUPPORTED;
   }

   TCHAR *eptr;
   int coreId = _tcstol(buffer, &eptr, 0);

   if ((*eptr != 0) || (coreId > (s_maxCPU - 1)) || (coreId < 0))
   {
      return SYSINFO_RC_UNSUPPORTED;
   }

   // Get desired history iteration count
   int count;

   switch(CPU_USAGE_PARAM_INTERVAL(pArg))
   {
      case INTERVAL_5MIN:
         count = 5 * 60;
         break;
      case INTERVAL_15MIN:
         count = 15 * 60;
         break;
      case INTERVAL_1MIN:
         count = 60;
         break;
      default:
         return SYSINFO_RC_UNSUPPORTED;
   }

   s_cpuUsageMutex.lock();

   // Calculate CPU usage estimate
   switch (CPU_USAGE_PARAM_SOURCE(pArg))
   {
      case CPU_USAGE_OVERAL:
         ret_double(pValue, AverageUsage(s_cpuUsageHistory[coreId].avgBusyUsage, count));
         break;
      case CPU_USAGE_USER:
         ret_double(pValue, AverageUsage(s_cpuUsageHistory[coreId].avgUserUsage, count));
         break;
      case CPU_USAGE_NICE:
         ret_double(pValue, AverageUsage(s_cpuUsageHistory[coreId].avgNiceUsage, count));
         break;
      case CPU_USAGE_SYSTEM:
         ret_double(pValue, AverageUsage(s_cpuUsageHistory[coreId].avgSystemUsage, count));
         break;
      case CPU_USAGE_IDLE:
         ret_double(pValue, AverageUsage(s_cpuUsageHistory[coreId].avgIdleUsage, count));
         break;
      default:
         return SYSINFO_RC_UNSUPPORTED;
   }

   s_cpuUsageMutex.unlock();

   return SYSINFO_RC_SUCCESS;
}

/**
 * CPU useage collection function. Queries all cores in all modes.
 */
static void CpuUsageCollector()
{
   natural_t cpuCount;
   processor_info_array_t infoArray;
   mach_msg_type_number_t count = PROCESSOR_CPU_LOAD_INFO_COUNT;

   s_cpuUsageMutex.lock();

   // Get data for all cores
   if (host_processor_info(mach_host_self(), PROCESSOR_CPU_LOAD_INFO, &cpuCount, &infoArray, &count) == KERN_SUCCESS)
   {
      // Assuming core count can't change during runtime
      /*if (cpuCount != s_maxCPU)
      {
         // @todo: Add additional core initialization/de-initialization
         s_maxCPU = cpuCount;
      }*/

      if (s_currentSlot == CPU_USAGE_SLOTS)
      {
         s_currentSlot = 0;
      }

      // Get array of individual CPU core data
      processor_cpu_load_info_data_t* cpuUsageInfo = reinterpret_cast<processor_cpu_load_info_data_t*>(infoArray);

      for (int coreId = 0; coreId < s_maxCPU; coreId++)
      {
         // Process current CPU usage for the core and add it to history
         uint32_t totalUsage = 0;

         for(int i = 0; i < CPU_STATE_MAX; i++)
         {
            totalUsage += cpuUsageInfo[coreId].cpu_ticks[i];
         }

         uint32_t deltaUserUsage = (cpuUsageInfo[coreId].cpu_ticks[CPU_STATE_USER] - s_cpuUsageHistory[coreId].prevUserUsage);
         uint32_t deltaSystemUsage = (cpuUsageInfo[coreId].cpu_ticks[CPU_STATE_SYSTEM] - s_cpuUsageHistory[coreId].prevSystemUsage);
         uint32_t deltaIdleUsage = (cpuUsageInfo[coreId].cpu_ticks[CPU_STATE_IDLE] - s_cpuUsageHistory[coreId].prevIdleUsage);
         uint32_t deltaNiceUsage = (cpuUsageInfo[coreId].cpu_ticks[CPU_STATE_NICE] - s_cpuUsageHistory[coreId].prevNiceUsage);
         uint32_t deltaTotalUsage = (totalUsage - s_cpuUsageHistory[coreId].prevTotalUsage);  // Results in roughly 100 ticks per core, per second

         if (deltaTotalUsage > 0)
         {
            s_cpuUsageHistory[coreId].avgUserUsage[s_currentSlot] = (((float)(deltaUserUsage) / (float)(deltaTotalUsage)) * 100);
            s_cpuUsageHistory[coreId].avgSystemUsage[s_currentSlot] = (((float)(deltaSystemUsage) / (float)(deltaTotalUsage)) * 100);
            s_cpuUsageHistory[coreId].avgIdleUsage[s_currentSlot] = (((float)(deltaIdleUsage) / (float)(deltaTotalUsage)) * 100);
            s_cpuUsageHistory[coreId].avgNiceUsage[s_currentSlot] = (((float)(deltaNiceUsage) / (float)(deltaTotalUsage)) * 100);
            s_cpuUsageHistory[coreId].avgBusyUsage[s_currentSlot] = s_cpuUsageHistory[coreId].avgUserUsage[s_currentSlot] + s_cpuUsageHistory[coreId].avgSystemUsage[s_currentSlot] +
                                                                     s_cpuUsageHistory[coreId].avgNiceUsage[s_currentSlot];
         }
         else
         {
            s_cpuUsageHistory[coreId].avgUserUsage[s_currentSlot] = 0;
            s_cpuUsageHistory[coreId].avgSystemUsage[s_currentSlot] = 0;
            s_cpuUsageHistory[coreId].avgIdleUsage[s_currentSlot] = 0;
            s_cpuUsageHistory[coreId].avgNiceUsage[s_currentSlot] = 0;
            s_cpuUsageHistory[coreId].avgBusyUsage[s_currentSlot] = 0;
         }

         s_currentSlot++;

         s_cpuUsageHistory[coreId].prevUserUsage = cpuUsageInfo[coreId].cpu_ticks[CPU_STATE_USER];
         s_cpuUsageHistory[coreId].prevSystemUsage = cpuUsageInfo[coreId].cpu_ticks[CPU_STATE_SYSTEM];
         s_cpuUsageHistory[coreId].prevIdleUsage = cpuUsageInfo[coreId].cpu_ticks[CPU_STATE_IDLE];
         s_cpuUsageHistory[coreId].prevNiceUsage = cpuUsageInfo[coreId].cpu_ticks[CPU_STATE_NICE];
         s_cpuUsageHistory[coreId].prevTotalUsage = totalUsage;
      }

      vm_deallocate(mach_task_self(), (vm_address_t)infoArray, count);
   }

   s_cpuUsageMutex.unlock();
}

static void CpuUsageCollectorThread()
{
   while(!m_stopCollectorThread)
   {
      CpuUsageCollector();
      ThreadSleepMs(1000); // sleep 1 second
   }
}

/**
 * Initializer for CPU collector thread
 */
void StartCpuUsageCollector(void)
{
   // Get core count
   natural_t cpuCount;
   processor_info_array_t infoArray;
   mach_msg_type_number_t count = PROCESSOR_CPU_LOAD_INFO_COUNT;

   if (host_processor_info(mach_host_self(), PROCESSOR_CPU_LOAD_INFO, &cpuCount, &infoArray, &count) == KERN_SUCCESS)
   {
      s_maxCPU = cpuCount;
   }
   else
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Call to host_processor_info() failed"));
      return;
   }

   // Set up data arrays
   s_cpuUsageHistory = new cpuUsageHistory[s_maxCPU];
   memset(s_cpuUsageHistory, 0, (sizeof(cpuUsageHistory) * s_maxCPU));

   // Get initial deltas
   s_currentSlot = 0;
   CpuUsageCollector();
   s_currentSlot = 0;
   CpuUsageCollector();
   s_currentSlot = 0;

   // Fill data arrays with initial value
   for (int coreId = 0; coreId < s_maxCPU; coreId++)
   {
      for (int i = 1; i < CPU_USAGE_SLOTS; i++)
      {
         s_cpuUsageHistory[coreId].avgUserUsage[i] = s_cpuUsageHistory[coreId].avgUserUsage[0];
         s_cpuUsageHistory[coreId].avgSystemUsage[i] = s_cpuUsageHistory[coreId].avgSystemUsage[0];
         s_cpuUsageHistory[coreId].avgIdleUsage[i] = s_cpuUsageHistory[coreId].avgIdleUsage[0];
         s_cpuUsageHistory[coreId].avgNiceUsage[i] = s_cpuUsageHistory[coreId].avgNiceUsage[0];
         s_cpuUsageHistory[coreId].avgBusyUsage[i] = s_cpuUsageHistory[coreId].avgBusyUsage[0];
      }
   }

   // Start collector
   m_cpuUsageCollector = ThreadCreateEx(CpuUsageCollectorThread);
}

/**
 * Destructor for CPU usage collector thread
 */
void ShutdownCpuUsageCollector(void)
{
   m_stopCollectorThread = true;
   ThreadJoin(m_cpuUsageCollector);
   delete s_cpuUsageHistory;
}
