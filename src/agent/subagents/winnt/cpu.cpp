/*
** NetXMS platform subagent for Windows
** Copyright (C) 2003-2017 Victor Kirhenshtein
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

static int s_cpuCount = 0;
static SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *s_cpuTimes = NULL;
static UINT32 *s_usage = NULL;
static UINT32 *s_idle = NULL;
static UINT32 *s_kernel = NULL;
static UINT32 *s_user = NULL;
static UINT32 *s_interrupt = NULL;
static UINT32 *s_interruptCount = NULL;
static int s_bpos = 0;
static CRITICAL_SECTION s_lock;

/**
 * CPU collector thread
 */
static THREAD_RESULT THREAD_CALL CPUStatCollector(void *arg)
{
   nxlog_debug(3, _T("CPU stat collector started (%d CPUs)"), s_cpuCount);

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

      UINT64 sysIdle = 0;
      UINT64 sysKernel = 0;
      UINT64 sysUser = 0;
      UINT64 sysInterrupt = 0;

      EnterCriticalSection(&s_lock);

      s_interruptCount[s_cpuCount] = 0;

      int idx = s_bpos;
      for(int i = 0; i < s_cpuCount; i++, idx++)
      {
         UINT64 idle = curr[i].IdleTime.QuadPart - prev[i].IdleTime.QuadPart;
         UINT64 kernel = curr[i].KernelTime.QuadPart - prev[i].KernelTime.QuadPart;
         UINT64 user = curr[i].UserTime.QuadPart - prev[i].UserTime.QuadPart;
         UINT64 interrupt = curr[i].Reserved1[1].QuadPart - prev[i].Reserved1[1].QuadPart;
         UINT64 total = kernel + user;  // kernel time includes idle time

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
            s_usage[idx] = (UINT32)((total - idle) * 10000 / total);
            s_idle[idx] = (UINT32)(idle * 10000 / total);
            s_kernel[idx] = (UINT32)((kernel - idle) * 10000 / total);
            s_user[idx] = (UINT32)(user * 10000 / total);
            s_interrupt[idx] = (UINT32)(interrupt * 10000 / total);
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

      UINT64 sysTotal = sysKernel + sysUser;
      if (sysTotal > 0)
      {
         s_usage[idx] = (UINT32)((sysTotal - sysIdle) * 10000 / sysTotal);
         s_idle[idx] = (UINT32)(sysIdle * 10000 / sysTotal);
         s_kernel[idx] = (UINT32)((sysKernel - sysIdle) * 10000 / sysTotal);
         s_user[idx] = (UINT32)(sysUser * 10000 / sysTotal);
         s_interrupt[idx] = (UINT32)(sysInterrupt * 10000 / sysTotal);
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

      LeaveCriticalSection(&s_lock);
      
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
   nxlog_debug(3, _T("CPU stat collector stopped"));
   return THREAD_OK;
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
   SYSTEM_INFO si;
   GetSystemInfo(&si);
   s_cpuCount = (int)si.dwNumberOfProcessors;
   s_cpuTimes = (SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *)malloc(sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * s_cpuCount * 2);
   s_usage = (UINT32 *)calloc(900, sizeof(UINT32) * (s_cpuCount + 1));
   s_idle = (UINT32 *)calloc(900, sizeof(UINT32) * (s_cpuCount + 1));
   s_kernel = (UINT32 *)calloc(900, sizeof(UINT32) * (s_cpuCount + 1));
   s_user = (UINT32 *)calloc(900, sizeof(UINT32) * (s_cpuCount + 1));
   s_interrupt = (UINT32 *)calloc(900, sizeof(UINT32) * (s_cpuCount + 1));
   s_interruptCount = (UINT32 *)calloc(sizeof(UINT32), s_cpuCount + 1);
   InitializeCriticalSectionAndSpinCount(&s_lock, 1000);
   s_collectorThread = ThreadCreateEx(CPUStatCollector, 0, NULL);
}

/**
 * Stop collector
 */
void StopCPUStatCollector()
{
   ThreadJoin(s_collectorThread);
   free(s_cpuTimes);
   free(s_usage);
   free(s_idle);
   free(s_kernel);
   free(s_user);
   DeleteCriticalSection(&s_lock);
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

   UINT32 usage = 0;
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

   UINT32 *data;
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

   EnterCriticalSection(&s_lock);
   for(int p = s_bpos - step, i = 0; i < count; i++, p -= step)
   {
      if (p < 0)
         p = s_cpuCount * 900 - step;
      usage += data[p + cpuIndex];
   }
   LeaveCriticalSection(&s_lock);

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
   ret_uint(value, *((UINT32 *)interrupts.Reserved1));
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

   EnterCriticalSection(&s_lock);
   ret_uint(value, s_interruptCount[cpuIndex]);
   LeaveCriticalSection(&s_lock);

   return SYSINFO_RC_SUCCESS;
}
