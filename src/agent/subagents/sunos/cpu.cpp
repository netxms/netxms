/*
** NetXMS subagent for SunOS/Solaris
** Copyright (C) 2004-2021 Victor Kirhenshtein
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

#include "sunos_subagent.h"
#include <sys/sysinfo.h>

/**
 * Maximum supported numebr of CPUs
 */
#define MAX_CPU_COUNT   256

/**
 * Fixed point arithmetics multiplier
 */
#define FP_MULTIPLIER	(1000)

/**
 * CPU usage data
 */
struct CPU_USAGE_DATA
{
   uint32_t states[CPU_STATES];
};

/**
 * Average CPU usage data
 */
struct CPU_USAGE_DATA_AVG
{
   double states[CPU_STATES];
};

/**
 * Collected statistic
 */
static int s_cpuCount = 1;
static int s_instanceMap[MAX_CPU_COUNT];
static CPU_USAGE_DATA_AVG s_usage[MAX_CPU_COUNT + 1];
static CPU_USAGE_DATA_AVG s_usage5[MAX_CPU_COUNT + 1];
static CPU_USAGE_DATA_AVG s_usage15[MAX_CPU_COUNT + 1];
static Mutex s_usageDataLock(MutexType::FAST);

/**
 * Read CPU times
 */
static bool ReadCPUTimes(kstat_ctl_t *kc, uint_t *pValues, BYTE *success)
{
   uint_t *pData = pValues;
   memset(success, 0, s_cpuCount);
   bool hasFailures = false;

   kstat_lock();
   for(int i = 0; i < s_cpuCount; i++, pData += CPU_STATES)
   {
      kstat_t *kp = kstat_lookup(kc, (char *)"cpu_stat", s_instanceMap[i], nullptr);
      if (kp != nullptr)
      {
         if (kstat_read(kc, kp, nullptr) != -1)
         {
            memcpy(pData, ((cpu_stat_t *)kp->ks_data)->cpu_sysinfo.cpu, sizeof(uint_t) * CPU_STATES);
            success[i] = 1;
         }
         else
         {
            nxlog_debug(8, _T("SunOS: kstat_read failed in ReadCPUTimes (instance=%d errno=%d)"), s_instanceMap[i], errno);
            hasFailures = true;
         }
      }
      else
      {
         nxlog_debug(8, _T("SunOS: kstat_lookup failed in ReadCPUTimes (instance=%d errno=%d)"), s_instanceMap[i], errno);
         hasFailures = true;
      }
   }
   kstat_unlock();
   return hasFailures;
}

/**
 * CPU usage statistics collector thread
 */
void CPUStatCollector()
{
   int i, j;
   uint32_t dwCurrPos, dwIndex;

   // Open kstat
   kstat_lock();
   kstat_ctl_t *kc = kstat_open();
   if (kc == nullptr)
   {
      kstat_unlock();
      AgentWriteLog(NXLOG_ERROR,
            _T("SunOS: Unable to open kstat() context (%s), CPU statistics will not be collected"), 
            _tcserror(errno));
      return;
   }

   // Read number of CPUs
   kstat_t *kp = kstat_lookup(kc, (char *)"unix", 0, (char *)"system_misc");
   if (kp != nullptr)
   {
      if (kstat_read(kc, kp, 0) != -1)
      {
         kstat_named_t *kn = (kstat_named_t *)kstat_data_lookup(kp, (char *)"ncpus");
         if (kn != nullptr)
         {
            s_cpuCount = kn->value.ui32;
         }
      }
   }

   // Read CPU instance numbers
   memset(s_instanceMap, 0xFF, sizeof(int) * MAX_CPU_COUNT);
   for(i = 0, j = 0; (i < s_cpuCount) && (j < MAX_CPU_COUNT); i++)
   {
      while(kstat_lookup(kc, (char *)"cpu_info", j, nullptr) == nullptr)
      {
         j++;
         if (j == MAX_CPU_COUNT)
         {
            nxlog_debug(1, _T("SunOS: cannot determine instance for CPU #%d"), i);
            break;
         }
      }
      s_instanceMap[i] = j++;
   }

   kstat_unlock();

   // Initialize data
   memset(s_usage, 0, sizeof(s_usage));
   memset(s_usage5, 0, sizeof(s_usage5));
   memset(s_usage15, 0, sizeof(s_usage15));
   CPU_USAGE_DATA *history = MemAllocArray<CPU_USAGE_DATA>((s_cpuCount + 1) * 900);
   for(i = 0; i < (s_cpuCount + 1) * 900; i++)
      history[i].states[CPU_IDLE] = FP_MULTIPLIER * 100; // Pre-fill history with 100% idle
   uint_t *pnLastTimes = MemAllocArray<uint_t>(s_cpuCount * CPU_STATES);
   uint_t *pnCurrTimes = MemAllocArray<uint_t>(s_cpuCount * CPU_STATES);
   uint32_t dwHistoryPos = 0;
   BYTE readSuccess[MAX_CPU_COUNT];
   AgentWriteDebugLog(1, _T("CPU stat collector thread started"));

   // Do first read
   bool readFailures = ReadCPUTimes(kc, pnLastTimes, readSuccess);

   // Collection loop
   int counter = 0;
   uint_t sysDelta[CPU_STATES];
   while(!AgentSleepAndCheckForShutdown(1000))
   {
      counter++;
      if (counter == 60)
         counter = 0;
      
      // Re-open kstat handle if some processor data cannot be read
      if (readFailures && (counter == 0))
      {
         kstat_lock();
         kstat_close(kc);
         kc = kstat_open();
         if (kc == nullptr)
         {
            kstat_unlock();
            AgentWriteLog(NXLOG_ERROR,
                          _T("SunOS: Unable to re-open kstat() context (%s), CPU statistics collection aborted"), 
                          _tcserror(errno));
            return;
         }
         kstat_unlock();
      }
      readFailures = ReadCPUTimes(kc, pnCurrTimes, readSuccess);

      // Calculate utilization for last second for each CPU
      memset(sysDelta, 0, sizeof(sysDelta));
      uint_t sysTotal = 0;
      dwIndex = dwHistoryPos * (s_cpuCount + 1);
      for(i = 0, j = 0; i < s_cpuCount; i++, dwIndex++)
      {
         if (readSuccess[i])
         {
            uint_t sum = 0;
            for(int state = 0; state < CPU_STATES; state++, j++)
            {
               uint_t delta = pnCurrTimes[j] - pnLastTimes[j];
               sysDelta[state] += delta;
               sum += delta;
            }

            if (sum > 0)
            {
               sysTotal += sum;
               j -= CPU_STATES;
               for(int state = 0; state < CPU_STATES; state++, j++)
                  history[dwIndex].states[state] = (pnCurrTimes[j] - pnLastTimes[j]) * FP_MULTIPLIER * 100 / sum;
            }
            else
            {
               // sum for all states is 0
               // this could indicate CPU spending all time in a state we are not aware of, or incorrect state data
               // assume 100% utilization
               memset(&history[dwIndex], 0, sizeof(CPU_USAGE_DATA));
            }
         }
         else
         {
            memset(&history[dwIndex], 0, sizeof(CPU_USAGE_DATA));
            j += CPU_STATES;  // skip states for offline CPU
         }
      }

      // Average utilization for last second for all CPUs
      if (sysTotal > 0)
      {
         for(int state = 0; state < CPU_STATES; state++)
            history[dwIndex].states[state] = sysDelta[state] * FP_MULTIPLIER * 100 / sysTotal;
      }
      else
      {
         // sum for all states for all CPUs is 0
         // this could indicate CPU spending all time in a state we are not aware of, or incorrect state data
         // assume 100% utilization
         memset(&history[dwIndex], 0, sizeof(CPU_USAGE_DATA));
      }

      // Copy current times to last
      uint_t *pnTemp = pnLastTimes;
      pnLastTimes = pnCurrTimes;
      pnCurrTimes = pnTemp;

      // Calculate averages
      CPU_USAGE_DATA usageSum[MAX_CPU_COUNT + 1];
      memset(usageSum, 0, sizeof(usageSum));
      for(i = 0, dwCurrPos = dwHistoryPos; i < 900; i++)
      {
         for(int state = 0; state < CPU_STATES; state++)
         {
            dwIndex = dwCurrPos * (s_cpuCount + 1);
            for(j = 0; j < s_cpuCount; j++, dwIndex++)
               usageSum[j].states[state] += history[dwIndex].states[state];
            usageSum[MAX_CPU_COUNT].states[state] += history[dwIndex].states[state];

            switch(i)
            {
               case 59:
                  s_usageDataLock.lock();
                  for(j = 0; j < s_cpuCount; j++)
                     s_usage[j].states[state] = static_cast<double>(usageSum[j].states[state]) / (60.0 * FP_MULTIPLIER);
                  s_usage[MAX_CPU_COUNT].states[state] = static_cast<double>(usageSum[MAX_CPU_COUNT].states[state]) / (60.0 * FP_MULTIPLIER);
                  s_usageDataLock.unlock();
                  break;
               case 299:
                  s_usageDataLock.lock();
                  for(j = 0; j < s_cpuCount; j++)
                     s_usage5[j].states[state] = static_cast<double>(usageSum[j].states[state]) / (300.0 * FP_MULTIPLIER);
                  s_usage5[MAX_CPU_COUNT].states[state] = static_cast<double>(usageSum[MAX_CPU_COUNT].states[state]) / (300.0 * FP_MULTIPLIER);
                  s_usageDataLock.unlock();
                  break;
               case 899:
                  s_usageDataLock.lock();
                  for(j = 0; j < s_cpuCount; j++)
                     s_usage15[j].states[state] = static_cast<double>(usageSum[j].states[state]) / (900.0 * FP_MULTIPLIER);
                  s_usage15[MAX_CPU_COUNT].states[state] = static_cast<double>(usageSum[MAX_CPU_COUNT].states[state]) / (900.0 * FP_MULTIPLIER);
                  s_usageDataLock.unlock();
                  break;
               default:
                  break;
            }
         }

         if (dwCurrPos > 0)
            dwCurrPos--;
         else
            dwCurrPos = 899;
      }

      // Increment history buffer position
      dwHistoryPos++;
      if (dwHistoryPos == 900)
         dwHistoryPos = 0;
   }

   // Cleanup
   MemFree(pnLastTimes);
   MemFree(pnCurrTimes);
   MemFree(history);
   kstat_lock();
   kstat_close(kc);
   kstat_unlock();
   AgentWriteDebugLog(1, _T("CPU stat collector thread stopped"));
}

/**
 * Handlers for System.CPU.Usage parameters
 */
LONG H_CPUUsage(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   LONG nRet = SYSINFO_RC_SUCCESS;

   CPU_USAGE_DATA_AVG *dataSet;
   switch(arg[1])
   {
      case '0':
         dataSet = s_usage;
         break;
      case '1':
         dataSet = s_usage5;
         break;
      case '2':
         dataSet = s_usage15;
         break;
      default:
         return SYSINFO_RC_UNSUPPORTED;
   }

   CPU_USAGE_DATA_AVG *data;
   if (arg[0] == 'T')
   {
      data = &dataSet[MAX_CPU_COUNT];
   }
   else
   {
      // Get CPU number
      char buffer[32] = "error";
      AgentGetParameterArgA(param, 1, buffer, 32);

      char *eptr;
      int32_t instance = strtol(buffer, &eptr, 0);
      int32_t cpu = -1;
      if (instance != -1)
      {
         for(cpu = 0; cpu < MAX_CPU_COUNT; cpu++)
            if (s_instanceMap[cpu] == instance)
               break;
      }
      if ((*eptr != 0) || (cpu < 0) || (cpu >= s_cpuCount))
         return SYSINFO_RC_UNSUPPORTED;

      data = &dataSet[cpu];
   }

   int state;
   bool invert = false;
   switch(arg[2])
   {
      case 'I':
         state = CPU_IDLE;
         break;
      case 'S':
         state = CPU_KERNEL;
         break;
      case 'T':
         state = CPU_IDLE;
         invert = true;
         break;
      case 'U':
         state = CPU_USER;
         break;
      case 'W':
         state = CPU_WAIT;
         break;
      default:
         return SYSINFO_RC_UNSUPPORTED;
   }

   s_usageDataLock.lock();
   double usage = data->states[state];
   s_usageDataLock.unlock();
   if (invert)
      usage = 100.0 - usage;
   _sntprintf(value, MAX_RESULT_LENGTH, _T("%.2f"), usage);
   return SYSINFO_RC_SUCCESS;
}
