/*
** NetXMS platform subagent for Windows
** Copyright (C) 2003-2024 Victor Kirhenshtein
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

#include "winnt_subagent.h"
#include <winternl.h>

static uint32_t s_cpuCount = 0;
static SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *s_cpuTimes = nullptr;
static uint32_t *s_usage = nullptr;
static uint32_t *s_idle = nullptr;
static uint32_t *s_kernel = nullptr;
static uint32_t *s_user = nullptr;
static uint32_t *s_interrupt = nullptr;
static uint32_t *s_interruptCount = nullptr;
static int s_bpos = 0;
static win_mutex_t s_lock;

/**
 * Get total number of logical processors in the system
 */
static uint32_t GetTotalProcessorCount()
{
   uint32_t totalProcessors = 0;
   WORD numGroups = GetActiveProcessorGroupCount();
   for (WORD group = 0; group < numGroups; ++group)
      totalProcessors += GetActiveProcessorCount(group);
   return totalProcessors;
}

/**
 * CPU collector thread
 */
static void CPUStatCollector()
{
   ThreadSetName("CPUStatCollector");
   nxlog_debug_tag(DEBUG_TAG, 3, _T("CPU stat collector started (%d CPUs)"), s_cpuCount);

   SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *prev = s_cpuTimes;
   SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *curr = &s_cpuTimes[s_cpuCount];

   ULONG cpuTimesLen = sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * s_cpuCount;
   ULONG size;
   NtQuerySystemInformation(SystemProcessorPerformanceInformation, s_cpuTimes, cpuTimesLen, &size);

   while(!AgentSleepAndCheckForShutdown(1000))
   {
      if (NtQuerySystemInformation(SystemProcessorPerformanceInformation, curr, cpuTimesLen, &size) != 0)
      {
         memcpy(curr, prev, cpuTimesLen);
      }

      uint64_t sysIdle = 0;
      uint64_t sysKernel = 0;
      uint64_t sysUser = 0;
      uint64_t sysInterrupt = 0;

      LockMutex(&s_lock, INFINITE);

      s_interruptCount[s_cpuCount] = 0;

      int idx = s_bpos;
      for(int i = 0; i < s_cpuCount; i++, idx++)
      {
         uint64_t idle = curr[i].IdleTime.QuadPart - prev[i].IdleTime.QuadPart;
         uint64_t kernel = curr[i].KernelTime.QuadPart - prev[i].KernelTime.QuadPart;
         uint64_t user = curr[i].UserTime.QuadPart - prev[i].UserTime.QuadPart;
         uint64_t interrupt = curr[i].Reserved1[1].QuadPart - prev[i].Reserved1[1].QuadPart;
         uint64_t total = kernel + user;  // kernel time includes idle time

         // There were reports of agent reporting extremely high CPU usage
         // That could happen when idle count is greater than total count
         if (idle > kernel)
            idle = kernel;

         s_interruptCount[i] = curr[i].Reserved2;
         s_interruptCount[s_cpuCount] += curr[i].Reserved2;

         sysIdle += idle;
         sysKernel += kernel;
         sysUser += user;
         sysInterrupt += interrupt;

         if (total > 0)
         {
            s_usage[idx] = (uint32_t)((total - idle) * 10000 / total);
            s_idle[idx] = (uint32_t)(idle * 10000 / total);
            s_kernel[idx] = (uint32_t)((kernel - idle) * 10000 / total);
            s_user[idx] = (uint32_t)(user * 10000 / total);
            s_interrupt[idx] = (uint32_t)(interrupt * 10000 / total);
         }
         else
         {
            s_usage[idx] = 0;
            s_idle[idx] = 0;
            s_kernel[idx] = 0;
            s_user[idx] = 0;
            s_interrupt[idx] = 0;
         }
      }

      uint64_t sysTotal = sysKernel + sysUser;
      if (sysTotal > 0)
      {
         s_usage[idx] = (uint32_t)((sysTotal - sysIdle) * 10000 / sysTotal);
         s_idle[idx] = (uint32_t)(sysIdle * 10000 / sysTotal);
         s_kernel[idx] = (uint32_t)((sysKernel - sysIdle) * 10000 / sysTotal);
         s_user[idx] = (uint32_t)(sysUser * 10000 / sysTotal);
         s_interrupt[idx] = (uint32_t)(sysInterrupt * 10000 / sysTotal);
      }
      else
      {
         s_usage[idx] = 0;
         s_idle[idx] = 0;
         s_kernel[idx] = 0;
         s_user[idx] = 0;
         s_interrupt[idx] = 0;
      }

      s_bpos += s_cpuCount + 1;
      if (s_bpos >= (s_cpuCount + 1) * 900)
         s_bpos = 0;

      UnlockMutex(&s_lock);
      
      // swap buffers
      if (curr == s_cpuTimes)
      {
         curr = prev;
         prev = s_cpuTimes;
      }
      else
      {
         prev = curr;
         curr = s_cpuTimes;
      }
   }
   nxlog_debug_tag(DEBUG_TAG, 3, _T("CPU stat collector stopped"));
}

/**
 * Collector thread handle
 */
static THREAD s_collectorThread = INVALID_THREAD_HANDLE;

/**
 * Start collector
 */
void StartCPUStatCollector()
{
   s_cpuCount = GetTotalProcessorCount();
   s_cpuTimes = MemAllocArray<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION>(s_cpuCount * 2);
   s_usage = MemAllocArray<uint32_t>(900 * (s_cpuCount + 1));
   s_idle = MemAllocArray<uint32_t>(900 * (s_cpuCount + 1));
   s_kernel = MemAllocArray<uint32_t>(900 * (s_cpuCount + 1));
   s_user = MemAllocArray<uint32_t>(900 * (s_cpuCount + 1));
   s_interrupt = MemAllocArray<uint32_t>(900 * (s_cpuCount + 1));
   s_interruptCount = MemAllocArray<uint32_t>(s_cpuCount + 1);
   InitializeMutex(&s_lock, 1000);
   s_collectorThread = ThreadCreateEx(CPUStatCollector);
}

/**
 * Stop collector
 */
void StopCPUStatCollector()
{
   ThreadJoin(s_collectorThread);
   MemFree(s_cpuTimes);
   MemFree(s_usage);
   MemFree(s_idle);
   MemFree(s_kernel);
   MemFree(s_user);
   DestroyMutex(&s_lock);
}

/**
 * Handler for System.CPU.Usage parameters
 */
LONG H_CpuUsage(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   int cpuIndex;
   if (arg[0] == 'T')   // Total
   {
      cpuIndex = s_cpuCount;
   }
   else
   {
      TCHAR buffer[64];
      if (!AgentGetParameterArg(param, 1, buffer, 64))
         return SYSINFO_RC_UNSUPPORTED;
      cpuIndex = _tcstol(buffer, NULL, 10);
      if ((cpuIndex < 0) || (cpuIndex >= s_cpuCount))
         return SYSINFO_RC_UNSUPPORTED;
   }

   uint32_t usage = 0;
   int step = s_cpuCount + 1;
   int count;
   switch(arg[1])
   {
      case '0':
         count = 1;
         break;
      case '1':
         count = 60;
         break;
      case '2':
         count = 300;
         break;
      case '3':
         count = 900;
         break;
      default:
         return SYSINFO_RC_UNSUPPORTED;
         break;
   }

   uint32_t *data;
   switch(arg[2])
   {
      case 'U':
         data = s_usage;
         break;
      case 'I':
         data = s_idle;
         break;
      case 'q':
         data = s_interrupt;
         break;
      case 's':
         data = s_kernel;
         break;
      case 'u':
         data = s_user;
         break;
      default:
         return SYSINFO_RC_UNSUPPORTED;
         break;
   }

   LockMutex(&s_lock, INFINITE);
   for(int p = s_bpos - step, i = 0; i < count; i++, p -= step)
   {
      if (p < 0)
         p = s_cpuCount * 900 - step;
      usage += data[p + cpuIndex];
   }
   UnlockMutex(&s_lock);

   usage /= count;
   _sntprintf(value, MAX_RESULT_LENGTH, _T("%d.%02d"), usage / 100, usage % 100);
   return SYSINFO_RC_SUCCESS;
}

/*
 * Handler for System.CPU.ContextSwitches parameter
 */
LONG H_CpuContextSwitches(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   SYSTEM_INTERRUPT_INFORMATION interrupts;
   ULONG size;
   if (NtQuerySystemInformation(SystemInterruptInformation, &interrupts, sizeof(SYSTEM_INTERRUPT_INFORMATION), &size) != 0)
      return SYSINFO_RC_ERROR;

   // First 4 bytes in SYSTEM_INTERRUPT_INFORMATION is context switch count
   // (according to http://www.geoffchappell.com/studies/windows/km/ntoskrnl/api/ex/sysinfo/interrupt.htm)
   ret_uint(value, *((uint32_t *)interrupts.Reserved1));
   return SYSINFO_RC_SUCCESS;
}

/*
 * Handler for System.CPU.Interrupts parameters
 */
LONG H_CpuInterrupts(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   int cpuIndex;
   if (arg[0] == 'T')   // Total
   {
      cpuIndex = s_cpuCount;
   }
   else
   {
      TCHAR buffer[64];
      if (!AgentGetParameterArg(param, 1, buffer, 64))
         return SYSINFO_RC_UNSUPPORTED;
      cpuIndex = _tcstol(buffer, NULL, 10);
      if ((cpuIndex < 0) || (cpuIndex >= s_cpuCount))
         return SYSINFO_RC_UNSUPPORTED;
   }

   LockMutex(&s_lock, INFINITE);
   ret_uint(value, s_interruptCount[cpuIndex]);
   UnlockMutex(&s_lock);

   return SYSINFO_RC_SUCCESS;
}

/**
* Handler for System.CPU.Count parameter
*/
LONG H_CpuCount(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   ret_uint(value, GetTotalProcessorCount());
   return SYSINFO_RC_SUCCESS;
}
