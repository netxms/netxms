/* $Id: system.cpp,v 1.4 2006-03-01 22:13:09 alk Exp $ */

/* 
** NetXMS subagent for GNU/Linux
** Copyright (C) 2004 Alex Kirhenshtein
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

#include <nms_common.h>
#include <nms_agent.h>

#include <locale.h>
#include <sys/utsname.h>
#include <sys/statvfs.h>

#include "system.h"
#include "proc.h"

LONG H_Uptime(char *pszParam, char *pArg, char *pValue)
{
	FILE *hFile;
	unsigned int uUptime = 0;

	hFile = fopen("/proc/uptime", "r");
	if (hFile != NULL)
	{
		char szTmp[64];

		if (fgets(szTmp, sizeof(szTmp), hFile) != NULL)
		{
			double dTmp;

			setlocale(LC_NUMERIC, "C");
			if (sscanf(szTmp, "%lf", &dTmp) == 1)
			{
				uUptime = (unsigned int)dTmp;
			}
			setlocale(LC_NUMERIC, "");
		}
		fclose(hFile);
	}
	
	if (uUptime > 0)
	{
   	ret_uint(pValue, uUptime);
	}

   return uUptime > 0 ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
}

LONG H_Uname(char *pszParam, char *pArg, char *pValue)
{
	struct utsname utsName;
	int nRet = SYSINFO_RC_ERROR;

	if (uname(&utsName) == 0)
	{
		char szBuff[1024];

		snprintf(szBuff, sizeof(szBuff), "%s %s %s %s %s", utsName.sysname,
				utsName.nodename, utsName.release, utsName.version,
				utsName.machine);
		// TODO: processor & platform

   	ret_string(pValue, szBuff);

		nRet = SYSINFO_RC_SUCCESS;
	}

   return nRet;
}

LONG H_Hostname(char *pszParam, char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
	char szBuff[128];

	if (gethostname(szBuff, sizeof(szBuff)) == 0)
	{
   	ret_string(pValue, szBuff);
		nRet = SYSINFO_RC_SUCCESS;
	}

   return nRet;
}

LONG H_CpuLoad(char *pszParam, char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
	struct statvfs s;
   char szArg[128] = {0};
	FILE *hFile;

	// get processor
   //NxGetParameterArg(pszParam, 1, szArg, sizeof(szArg));

	hFile = fopen("/proc/loadavg", "r");
	if (hFile != NULL)
	{
		char szTmp[64];

		if (fgets(szTmp, sizeof(szTmp), hFile) != NULL)
		{
			double dLoad1, dLoad5, dLoad15;

			setlocale(LC_NUMERIC, "C");
			if (sscanf(szTmp, "%lf %lf %lf", &dLoad1, &dLoad5, &dLoad15) == 3)
			{
				switch (pszParam[19])
				{
				case '1': // 15 min
					ret_double(pValue, dLoad15);
					break;
				case '5': // 5 min
					ret_double(pValue, dLoad5);
					break;
				default: // 1 min
					ret_double(pValue, dLoad1);
					break;
				}
				nRet = SYSINFO_RC_SUCCESS;
			}
			setlocale(LC_NUMERIC, "");
		}

		fclose(hFile);
	}
	

	return nRet;
}

LONG H_ProcessCount(char *pszParam, char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
	struct statvfs s;
   char szArg[128] = {0};
	int nCount = -1;

   NxGetParameterArg(pszParam, 1, szArg, sizeof(szArg));
	if (pArg == NULL)
	{
		nCount = ProcRead(NULL, szArg);
	}
	else
	{
		nCount = ProcRead(NULL, NULL);
	}
	if (nCount >= 0)
	{
		ret_int(pValue, nCount);
		nRet = SYSINFO_RC_SUCCESS;
	}

	return nRet;
}

LONG H_MemoryInfo(char *pszParam, char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
	FILE *hFile;
	unsigned long nMemTotal, nMemFree, nSwapTotal, nSwapFree;
	char *pTag;

	nMemTotal = nMemFree = nSwapTotal = nSwapFree = 0;

	hFile = fopen("/proc/meminfo", "r");
	if (hFile != NULL)
	{
		char szTmp[128];
		while (fgets(szTmp, sizeof(szTmp), hFile) != NULL)
		{
			if (sscanf(szTmp, "MemTotal: %lu kB\n", &nMemTotal) == 1)
				continue;
			if (sscanf(szTmp, "MemFree: %lu kB\n", &nMemFree) == 1)
				continue;
			if (sscanf(szTmp, "SwapTotal: %lu kB\n", &nSwapTotal) == 1)
				continue;
			if (sscanf(szTmp, "SwapFree: %lu kB\n", &nSwapFree) == 1)
				continue;
		}

		fclose(hFile);

		nRet = SYSINFO_RC_SUCCESS;
		switch((int)pArg)
		{
		case PHYSICAL_FREE: // ph-free
			ret_uint64(pValue, ((int64_t)nMemFree) * 1024);
			break;
		case PHYSICAL_TOTAL: // ph-total
			ret_uint64(pValue, ((int64_t)nMemTotal) * 1024);
			break;
		case PHYSICAL_USED: // ph-used
			ret_uint64(pValue, ((int64_t)(nMemTotal - nMemFree)) * 1024);
			break;
		case SWAP_FREE: // sw-free
			ret_uint64(pValue, ((int64_t)nSwapFree) * 1024);
			break;
		case SWAP_TOTAL: // sw-total
			ret_uint64(pValue, ((int64_t)nSwapTotal) * 1024);
			break;
		case SWAP_USED: // sw-used
			ret_uint64(pValue, ((int64_t)(nSwapTotal - nSwapFree)) * 1024);
			break;
		case VIRTUAL_FREE: // vi-free
			ret_uint64(pValue, 0);
			break;
		case VIRTUAL_TOTAL: // vi-total
			ret_uint64(pValue, 0);
			break;
		case VIRTUAL_USED: // vi-used
			ret_uint64(pValue, 0);
			break;
		default: // error
			nRet = SYSINFO_RC_ERROR;
			break;
		}
	}

	return nRet;
}

LONG H_ProcessList(char *pszParam, char *pArg, NETXMS_VALUES_LIST *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
	PROC_ENT *pEnt;
	int nCount;

	nCount = ProcRead(&pEnt, NULL);

	if (nCount >= 0)
	{
		nRet = SYSINFO_RC_SUCCESS;

		for (int i = 0; i < nCount; i++)
		{
			char szBuff[128];

			snprintf(szBuff, sizeof(szBuff), "%d %s",
					pEnt[i].nPid, pEnt[i].szProcName);
			NxAddResultString(pValue, szBuff);
		}
	}

	return nRet;
}

//
// stub
//
LONG H_SourcePkgSupport(char *pszParam, char *pArg, char *pValue)
{
	ret_int(pValue, 1);
	return SYSINFO_RC_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// CPU Usage
//

//
// CPU Usage data
//

static THREAD m_cpuUsageCollector = INVALID_THREAD_HANDLE;
static MUTEX m_cpuUsageMutex = INVALID_MUTEX_HANDLE;
static bool m_stopCollectorThread = false;
static int m_user = 0;
static int m_system = 0;
static int m_idle = 0;
static float m_cpuUsage[60];
static int m_currentSlot = 0;

static void CpuUsageCollector()
{
	FILE *hStat = fopen("/proc/stat", "r");

	if (hStat != NULL)
	{
		int user, nice, system, idle;
		if (fscanf(hStat, "cpu %d %d %d %d", &user, &nice, &system, &idle) == 4)
		{
			long total = (user - m_user) + (system - m_system) + (idle - m_idle);

			if (m_currentSlot == 60)
			{
				m_currentSlot = 0;
			}

			printf(">>> %d-%d-%d ||| %d-%d-%d\n",
					user, system, idle,
					m_user, m_system, m_idle);
			printf("--- {{%ld}} ---\n", total);

			m_cpuUsage[m_currentSlot++] =
				100 - ((float)((idle - m_idle) * 100) / total);
			printf("usage[%02d] == (%f) %f\n",
					m_currentSlot - 1,
					((float)((idle - m_idle) * 100) / total),
					m_cpuUsage[m_currentSlot - 1]);

			m_user = user;
			m_system = system;
			m_idle = idle;
		}
		fclose(hStat);
	}
}

static THREAD_RESULT THREAD_CALL CpuUsageCollectorThread(void *pArg)
{
	setlocale(LC_NUMERIC, "C");
	printf("Collector started\n"); fflush(stdout);
	while (m_stopCollectorThread == false)
	{
		MutexLock(m_cpuUsageMutex, INFINITE);

		CpuUsageCollector();
		
		MutexUnlock(m_cpuUsageMutex);
		ThreadSleepMs(1000); // sleep 1 second
	}

	return THREAD_OK;
}

void StartCpuUsageCollector(void)
{
	int i;

	printf("GO UP\n"); fflush(stdout);
	m_cpuUsageMutex = MutexCreate();

	for (i = 0; i < 60; i++)
	{
		m_cpuUsage[i] = 0;
	}

	m_currentSlot = 0;
	CpuUsageCollector();
	printf("2: %f\n", m_cpuUsage[0]);

	sleep(1);

	m_currentSlot = 0;
	CpuUsageCollector();
	printf("3: %f\n", m_cpuUsage[0]);

	for (i = 1; i < 60; i++)
	{
		m_cpuUsage[i] = m_cpuUsage[0];
	}

	m_cpuUsageCollector = ThreadCreateEx(CpuUsageCollectorThread, 0, NULL);
}

void ShutdownCpuUsageCollector(void)
{
	printf("GO DOWN\n"); fflush(stdout);
	MutexLock(m_cpuUsageMutex, INFINITE);
	m_stopCollectorThread = true;
	MutexUnlock(m_cpuUsageMutex);

	ThreadJoin(m_cpuUsageCollector);
	MutexDestroy(m_cpuUsageMutex);

	printf("DIED\n"); fflush(stdout);
}

LONG H_CpuUsage(char *pszParam, char *pArg, char *pValue)
{
	float usage = 0;

	MutexLock(m_cpuUsageMutex, INFINITE);
	for (int i = 0; i < 60; i++)
	{
		printf("usage[%0d]=%f\n", i, usage);
		usage += m_cpuUsage[i];
	}

	ret_double(pValue, usage);

	MutexUnlock(m_cpuUsageMutex);

	return SYSINFO_RC_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.3  2005/01/24 19:40:31  alk
return type/comments added for command list

System.ProcessCount/Process.Count(*) misunderstanding resolved

Revision 1.2  2005/01/17 23:31:01  alk
Agent.SourcePackageSupport added

Revision 1.1  2004/10/22 22:08:35  alk
source restructured;
implemented:
	Net.IP.Forwarding
	Net.IP6.Forwarding
	Process.Count(*)
	Net.ArpCache
	Net.InterfaceList (if-type not implemented yet)
	System.ProcessList


*/
