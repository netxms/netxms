/*
** NetXMS subagent for SunOS/Solaris
** Copyright (C) 2004-2016 Victor Kirhenshtein
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


//
// Constants
//

#define MAX_CPU_COUNT   256


//
// Collected statistic
//

static int m_nCPUCount = 1;
static int m_nInstanceMap[MAX_CPU_COUNT];
static DWORD m_dwUsage[MAX_CPU_COUNT + 1];
static DWORD m_dwUsage5[MAX_CPU_COUNT + 1];
static DWORD m_dwUsage15[MAX_CPU_COUNT + 1];

/**
 * Read CPU times
 */
static bool ReadCPUTimes(kstat_ctl_t *kc, uint_t *pValues, BYTE *success)
{
   uint_t *pData = pValues;
   memset(success, 0, m_nCPUCount);
   bool hasFailures = false;

   kstat_lock();
   for(int i = 0; i < m_nCPUCount; i++, pData += CPU_STATES)
   {
      kstat_t *kp = kstat_lookup(kc, (char *)"cpu_stat", m_nInstanceMap[i], NULL);
      if (kp != NULL)
      {
         if (kstat_read(kc, kp, NULL) != -1)
         {
            memcpy(pData, ((cpu_stat_t *)kp->ks_data)->cpu_sysinfo.cpu, sizeof(uint_t) * CPU_STATES);
            success[i] = 1;
         }
         else
         {
            nxlog_debug(8, _T("SunOS: kstat_read failed in ReadCPUTimes (instance=%d errno=%d)"), m_nInstanceMap[i], errno);
            hasFailures = true;
         }
      }
      else
      {
         nxlog_debug(8, _T("SunOS: kstat_lookup failed in ReadCPUTimes (instance=%d errno=%d)"), m_nInstanceMap[i], errno);
         hasFailures = true;
      }
   }
   kstat_unlock();
   return hasFailures;
}

/**
 * CPU usage statistics collector thread
 */
THREAD_RESULT THREAD_CALL CPUStatCollector(void *arg)
{
   kstat_ctl_t *kc;
   kstat_t *kp;
   kstat_named_t *kn;
   int i, j, iIdleTime, iLimit;
   DWORD *pdwHistory, dwHistoryPos, dwCurrPos, dwIndex;
   DWORD dwSum[MAX_CPU_COUNT + 1];
   BYTE readSuccess[MAX_CPU_COUNT];
   uint_t *pnLastTimes, *pnCurrTimes, *pnTemp;
   uint_t nSum, nSysSum, nSysCurrIdle, nSysLastIdle;

   // Open kstat
   kstat_lock();
   kc = kstat_open();
   if (kc == NULL)
   {
      kstat_unlock();
      AgentWriteLog(NXLOG_ERROR,
            _T("SunOS: Unable to open kstat() context (%s), CPU statistics will not be collected"), 
            _tcserror(errno));
      return THREAD_OK;
   }

   // Read number of CPUs
   kp = kstat_lookup(kc, (char *)"unix", 0, (char *)"system_misc");
   if (kp != NULL)
   {
      if (kstat_read(kc, kp, 0) != -1)
      {
         kn = (kstat_named_t *)kstat_data_lookup(kp, (char *)"ncpus");
         if (kn != NULL)
         {
            m_nCPUCount = kn->value.ui32;
         }
      }
   }

   // Read CPU instance numbers
   memset(m_nInstanceMap, 0xFF, sizeof(int) * MAX_CPU_COUNT);
   for(i = 0, j = 0; (i < m_nCPUCount) && (j < MAX_CPU_COUNT); i++)
   {
      while(kstat_lookup(kc, (char *)"cpu_info", j, NULL) == NULL)
      {
         j++;
         if (j == MAX_CPU_COUNT)
         {
            nxlog_debug(1, _T("SunOS: cannot determine instance for CPU #%d"), i);
            break;
         }
      }
      m_nInstanceMap[i] = j++;
   }

   kstat_unlock();

   // Initialize data
   memset(m_dwUsage, 0, sizeof(DWORD) * (MAX_CPU_COUNT + 1));
   memset(m_dwUsage5, 0, sizeof(DWORD) * (MAX_CPU_COUNT + 1));
   memset(m_dwUsage15, 0, sizeof(DWORD) * (MAX_CPU_COUNT + 1));
   pdwHistory = (DWORD *)malloc(sizeof(DWORD) * (m_nCPUCount + 1) * 900);
   memset(pdwHistory, 0, sizeof(DWORD) * (m_nCPUCount + 1) * 900);
   pnLastTimes = (uint_t *)malloc(sizeof(uint_t) * m_nCPUCount * CPU_STATES);
   pnCurrTimes = (uint_t *)malloc(sizeof(uint_t) * m_nCPUCount * CPU_STATES);
   dwHistoryPos = 0;
   AgentWriteDebugLog(1, _T("CPU stat collector thread started"));

   // Do first read
   bool readFailures = ReadCPUTimes(kc, pnLastTimes, readSuccess);

   // Collection loop
   int counter = 0;
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
         if (kc == NULL)
         {
            kstat_unlock();
            AgentWriteLog(NXLOG_ERROR,
                          _T("SunOS: Unable to re-open kstat() context (%s), CPU statistics collection aborted"), 
                          _tcserror(errno));
            return THREAD_OK;
         }
         kstat_unlock();
      }
      readFailures = ReadCPUTimes(kc, pnCurrTimes, readSuccess);

      // Calculate utilization for last second for each CPU
      dwIndex = dwHistoryPos * (m_nCPUCount + 1);
      for(i = 0, j = 0, nSysSum = 0, nSysCurrIdle = 0, nSysLastIdle = 0; i < m_nCPUCount; i++)
      {
         if (readSuccess[i])
         {
            iIdleTime = j + CPU_IDLE;
            iLimit = j + CPU_STATES;
            for(nSum = 0; j < iLimit; j++)
               nSum += pnCurrTimes[j] - pnLastTimes[j];
            if (nSum > 0)
            {
               nSysSum += nSum;
               nSysCurrIdle += pnCurrTimes[iIdleTime];
               nSysLastIdle += pnLastTimes[iIdleTime];
               pdwHistory[dwIndex++] = 1000 - ((pnCurrTimes[iIdleTime] - pnLastTimes[iIdleTime]) * 1000 / nSum);
            }
            else
            {
               // sum for all states is 0
               // this could indicate CPU spending all time in a state we are not aware of, or incorrect state data
               // assume 100% utilization
               pdwHistory[dwIndex++] = 1000;
            }
         }
         else
         {
            pdwHistory[dwIndex++] = 0;
            j += CPU_STATES;  // skip states for offline CPU
         }
      }

      // Average utilization for last second for all CPUs
      if (nSysSum > 0)
      {
         pdwHistory[dwIndex] = 1000 - ((nSysCurrIdle - nSysLastIdle) * 1000 / nSysSum);
      }
      else
      {
         // sum for all states for all CPUs is 0
         // this could indicate CPU spending all time in a state we are not aware of, or incorrect state data
         // assume 100% utilization
         pdwHistory[dwIndex] = 1000;
      }

      // Copy current times to last
      pnTemp = pnLastTimes;
      pnLastTimes = pnCurrTimes;
      pnCurrTimes = pnTemp;

      // Calculate averages
      memset(dwSum, 0, sizeof(dwSum));
      for(i = 0, dwCurrPos = dwHistoryPos; i < 900; i++)
      {
         dwIndex = dwCurrPos * (m_nCPUCount + 1);
         for(j = 0; j < m_nCPUCount; j++, dwIndex++)
            dwSum[j] += pdwHistory[dwIndex];
         dwSum[MAX_CPU_COUNT] += pdwHistory[dwIndex];

         switch(i)
         {
            case 59:
               for(j = 0; j < m_nCPUCount; j++)
                  m_dwUsage[j] = dwSum[j] / 60;
               m_dwUsage[MAX_CPU_COUNT] = dwSum[MAX_CPU_COUNT] / 60;
               break;
            case 299:
               for(j = 0; j < m_nCPUCount; j++)
                  m_dwUsage5[j] = dwSum[j] / 300;
               m_dwUsage5[MAX_CPU_COUNT] = dwSum[MAX_CPU_COUNT] / 300;
               break;
            case 899:
               for(j = 0; j < m_nCPUCount; j++)
                  m_dwUsage15[j] = dwSum[j] / 900;
               m_dwUsage15[MAX_CPU_COUNT] = dwSum[MAX_CPU_COUNT] / 900;
               break;
            default:
               break;
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
   free(pnLastTimes);
   free(pnCurrTimes);
   free(pdwHistory);
   kstat_lock();
   kstat_close(kc);
   kstat_unlock();
   AgentWriteDebugLog(1, _T("CPU stat collector thread stopped"));
   return THREAD_OK;
}

/**
 * Handlers for System.CPU.Usage parameters
 */
LONG H_CPUUsage(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   LONG nRet = SYSINFO_RC_SUCCESS;

   if (pArg[0] == 'T')
   {
      switch(pArg[1])
      {
         case '0':
            _sntprintf(pValue, MAX_RESULT_LENGTH, _T("%d.%d"),
                  m_dwUsage[MAX_CPU_COUNT] / 10,
                  m_dwUsage[MAX_CPU_COUNT] % 10);
            break;
         case '1':
            _sntprintf(pValue, MAX_RESULT_LENGTH, _T("%d.%d"),
                  m_dwUsage5[MAX_CPU_COUNT] / 10,
                  m_dwUsage5[MAX_CPU_COUNT] % 10);
            break;
         case '2':
            _sntprintf(pValue, MAX_RESULT_LENGTH, _T("%d.%d"),
                  m_dwUsage15[MAX_CPU_COUNT] / 10,
                  m_dwUsage15[MAX_CPU_COUNT] % 10);
            break;
         default:
            nRet = SYSINFO_RC_UNSUPPORTED;
            break;
      }
   }
   else
   {
      LONG nCPU = -1, nInstance;
      TCHAR *eptr, szBuffer[32] = _T("error");

      // Get CPU number
      AgentGetParameterArg(pszParam, 1, szBuffer, 32);
      nInstance = _tcstol(szBuffer, &eptr, 0);
      if (nInstance != -1)
      {
         for(nCPU = 0; nCPU < MAX_CPU_COUNT; nCPU++)
            if (m_nInstanceMap[nCPU] == nInstance)
               break;
      }
      if ((*eptr == 0) && (nCPU >= 0) && (nCPU < m_nCPUCount))
      {
         switch(pArg[1])
         {
            case '0':
               _sntprintf(pValue, MAX_RESULT_LENGTH, _T("%d.%d"),
                     m_dwUsage[nCPU] / 10,
                     m_dwUsage[nCPU] % 10);
               break;
            case '1':
               _sntprintf(pValue, MAX_RESULT_LENGTH, _T("%d.%d"),
                     m_dwUsage5[nCPU] / 10,
                     m_dwUsage5[nCPU] % 10);
               break;
            case '2':
               _sntprintf(pValue, MAX_RESULT_LENGTH, _T("%d.%d"),
                     m_dwUsage15[nCPU] / 10,
                     m_dwUsage15[nCPU] % 10);
               break;
            default:
               nRet = SYSINFO_RC_UNSUPPORTED;
               break;
         }
      }
      else
      {
         nRet = SYSINFO_RC_UNSUPPORTED;
      }
   }

   return nRet;
}
