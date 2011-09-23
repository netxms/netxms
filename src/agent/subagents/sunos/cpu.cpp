/*
 ** NetXMS subagent for SunOS/Solaris
 ** Copyright (C) 2004-2011 Victor Kirhenshtein
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


//
// Read CPU times
//

static void ReadCPUTimes(kstat_ctl_t *kc, uint_t *pValues)
{
	kstat_t *kp;
	int i;
	uint_t *pData;

	kstat_lock();
	for(i = 0, pData = pValues; i < m_nCPUCount; i++, pData += CPU_STATES)
	{
		kp = kstat_lookup(kc, "cpu_stat", m_nInstanceMap[i], NULL);
		if (kp != NULL)
		{
			if (kstat_read(kc, kp, NULL) != -1)
			{
				memcpy(pData, ((cpu_stat_t *)kp->ks_data)->cpu_sysinfo.cpu, sizeof(uint_t) * CPU_STATES);
			}
			else 
			{
				AgentWriteDebugLog(6, "SunOS: kstat_read failed in ReadCPUTimes");
			}
		}
	}
	kstat_unlock();
}


//
// CPU usage statistics collector thread
//

THREAD_RESULT THREAD_CALL CPUStatCollector(void *arg)
{
	kstat_ctl_t *kc;
	kstat_t *kp;
	kstat_named_t *kn;
	int i, j, iIdleTime, iLimit;
	DWORD *pdwHistory, dwHistoryPos, dwCurrPos, dwIndex;
	DWORD dwSum[MAX_CPU_COUNT + 1];
	uint_t *pnLastTimes, *pnCurrTimes, *pnTemp;
	uint_t nSum, nSysSum, nSysCurrIdle, nSysLastIdle;

	// Open kstat
	kstat_lock();
	kc = kstat_open();
	if (kc == NULL)
	{
		kstat_unlock();
		AgentWriteLog(EVENTLOG_ERROR_TYPE,
				"SunOS: Unable to open kstat() context (%s), CPU statistics will not be collected", 
				strerror(errno));
		return THREAD_OK;
	}

	// Read number of CPUs
	kp = kstat_lookup(kc, "unix", 0, "system_misc");
	if (kp != NULL)
	{
		if(kstat_read(kc, kp, 0) != -1)
		{
			kn = (kstat_named_t *)kstat_data_lookup(kp, "ncpus");
			if (kn != NULL)
			{
				m_nCPUCount = kn->value.ui32;
			}
		}
	}

	// Read CPU instance numbers
	memset(m_nInstanceMap, 0xFF, sizeof(int) * MAX_CPU_COUNT);
	for(i = 0, j = 0; i < m_nCPUCount; i++)
	{
		while(kstat_lookup(kc, "cpu_stat", j, NULL) == NULL)
			j++;
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
	AgentWriteDebugLog(1, "CPU stat collector thread started");

	// Do first read
	ReadCPUTimes(kc, pnLastTimes);
	ThreadSleepMs(1000);

	// Collection loop
	while(!g_bShutdown)
	{
		ReadCPUTimes(kc, pnCurrTimes);

		// Calculate utilization for last second for each CPU
		dwIndex = dwHistoryPos * (m_nCPUCount + 1);
		for(i = 0, j = 0, nSysSum = 0, nSysCurrIdle = 0, nSysLastIdle = 0;
				i < m_nCPUCount; i++)
		{
			iIdleTime = j + CPU_IDLE;
			iLimit = j + CPU_STATES;
			for(nSum = 0; j < iLimit; j++)
				nSum += pnCurrTimes[j] - pnLastTimes[j];
			nSysSum += nSum;
			nSysCurrIdle += pnCurrTimes[iIdleTime];
			nSysLastIdle += pnLastTimes[iIdleTime];
			pdwHistory[dwIndex++] = 
				1000 - ((pnCurrTimes[iIdleTime] - pnLastTimes[iIdleTime]) * 1000 / nSum);
		}

		// Average utilization for last second for all CPUs
		pdwHistory[dwIndex] = 
			1000 - ((nSysCurrIdle - nSysLastIdle) * 1000 / nSysSum);

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

		ThreadSleepMs(1000);
	}

	// Cleanup
	free(pnLastTimes);
	free(pnCurrTimes);
	free(pdwHistory);
	kstat_lock();
	kstat_close(kc);
	kstat_unlock();
	AgentWriteDebugLog(1, "CPU stat collector thread stopped");
	return THREAD_OK;
}


//
// Handlers for System.CPU.Usage parameters
//

LONG H_CPUUsage(const char *pszParam, const char *pArg, char *pValue)
{
	LONG nRet = SYSINFO_RC_SUCCESS;

	if (pArg[0] == 'T')
	{
		switch(pArg[1])
		{
			case '0':
				sprintf(pValue, "%d.%d00000",
						m_dwUsage[MAX_CPU_COUNT] / 10,
						m_dwUsage[MAX_CPU_COUNT] % 10);
				break;
			case '1':
				sprintf(pValue, "%d.%d00000",
						m_dwUsage5[MAX_CPU_COUNT] / 10,
						m_dwUsage5[MAX_CPU_COUNT] % 10);
				break;
			case '2':
				sprintf(pValue, "%d.%d00000",
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
		char *eptr, szBuffer[32] = "error";

		// Get CPU number
		AgentGetParameterArg(pszParam, 1, szBuffer, 32);
		nInstance = strtol(szBuffer, &eptr, 0);
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
					sprintf(pValue, "%d.%d00000",
							m_dwUsage[nCPU] / 10,
							m_dwUsage[nCPU] % 10);
					break;
				case '1':
					sprintf(pValue, "%d.%d00000",
							m_dwUsage5[nCPU] / 10,
							m_dwUsage5[nCPU] % 10);
					break;
				case '2':
					sprintf(pValue, "%d.%d00000",
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
