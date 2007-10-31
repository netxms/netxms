/* $Id: system.cpp,v 1.20 2007-10-31 08:14:33 victor Exp $ */

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
#include <utmp.h>

#include "system.h"
#include "proc.h"


//
// Handler for System.ConnectedUsers parameter
//

LONG H_ConnectedUsers(char *pszParam, char *pArg, char *pValue)
{
	LONG nRet = SYSINFO_RC_ERROR;
	FILE *f;
	struct utmp rec;
	int nCount = 0;

	f = fopen(UTMP_FILE, "r");
	if (f != NULL)
	{
		nRet = SYSINFO_RC_SUCCESS;
		while(fread(&rec, sizeof(rec), 1, f) == 1)
		{
			if (rec.ut_type == USER_PROCESS)
			{
				nCount++;
			}
		}
		fclose(f);
		ret_uint(pValue, nCount);
	}

	return nRet;
}


//
// Handler for System.ActiveUserSessions enum
//

LONG H_ActiveUserSessions(char *pszParam, char *pArg, NETXMS_VALUES_LIST *pValue)
{
	LONG nRet = SYSINFO_RC_ERROR;
	FILE *f;
	struct utmp rec;
	char szBuffer[1024];

	f = fopen(UTMP_FILE, "r");
	if (f != NULL)
	{
		nRet = SYSINFO_RC_SUCCESS;
		while(fread(&rec, sizeof(rec), 1, f) == 1)
		{
			if (rec.ut_type == USER_PROCESS)
			{
				snprintf(szBuffer, 1024, "\"%s\" \"%s\" \"%s\"", rec.ut_user,
						rec.ut_line, rec.ut_host);
				NxAddResultString(pValue, szBuffer);
			}
		}
		fclose(f);
	}

	return nRet;
}


//
// Handler for System.Uptime parameter
//

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
				switch (pszParam[18])
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
	char szArg[128] = "", szCmdLine[128] = "";
	int nCount = -1;

	if (*pArg != 'T')
	{
		NxGetParameterArg(pszParam, 1, szArg, sizeof(szArg));
		if (*pArg == 'E')
		{
			NxGetParameterArg(pszParam, 2, szCmdLine, sizeof(szCmdLine));
		}
	}

	nCount = ProcRead(NULL, (*pArg != 'T') ? szArg : NULL, (*pArg == 'E') ? szCmdLine : NULL);

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
		switch((long)pArg)
		{
			case PHYSICAL_FREE: // ph-free
				ret_uint64(pValue, ((QWORD)nMemFree) * 1024);
				break;
			case PHYSICAL_TOTAL: // ph-total
				ret_uint64(pValue, ((QWORD)nMemTotal) * 1024);
				break;
			case PHYSICAL_USED: // ph-used
				ret_uint64(pValue, ((QWORD)(nMemTotal - nMemFree)) * 1024);
				break;
			case SWAP_FREE: // sw-free
				ret_uint64(pValue, ((QWORD)nSwapFree) * 1024);
				break;
			case SWAP_TOTAL: // sw-total
				ret_uint64(pValue, ((QWORD)nSwapTotal) * 1024);
				break;
			case SWAP_USED: // sw-used
				ret_uint64(pValue, ((QWORD)(nSwapTotal - nSwapFree)) * 1024);
				break;
			case VIRTUAL_FREE: // vi-free
				ret_uint64(pValue, ((QWORD)nMemFree + (QWORD)nSwapFree) * 1024);
				break;
			case VIRTUAL_TOTAL: // vi-total
				ret_uint64(pValue, ((QWORD)nMemTotal + (QWORD)nSwapTotal) * 1024);
				break;
			case VIRTUAL_USED: // vi-used
				ret_uint64(pValue, ((QWORD)(nMemTotal - nMemFree) + (QWORD)(nSwapTotal - nSwapFree)) * 1024);
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

	nCount = ProcRead(&pEnt, NULL, NULL);

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

#define CPU_USAGE_SLOTS			900
#define MAX_CPU					64

static THREAD m_cpuUsageCollector = INVALID_THREAD_HANDLE;
static MUTEX m_cpuUsageMutex = INVALID_MUTEX_HANDLE;
static bool volatile m_stopCollectorThread = false;
static uint64_t m_user[MAX_CPU + 1];
static uint64_t m_nice[MAX_CPU + 1];
static uint64_t m_system[MAX_CPU + 1];
static uint64_t m_idle[MAX_CPU + 1];
static uint64_t m_iowait[MAX_CPU + 1];
static uint64_t m_irq[MAX_CPU + 1];
static uint64_t m_softirq[MAX_CPU + 1];
static uint64_t m_steal[MAX_CPU + 1];
// 60 sec * 15 min => 900 sec
static float m_cpuUsage[MAX_CPU + 1][CPU_USAGE_SLOTS];
static int m_currentSlot = 0;
static int m_maxCPU = 0;

static void CpuUsageCollector()
{
	FILE *hStat = fopen("/proc/stat", "r");

	if (hStat != NULL)
	{
		uint64_t user, nice, system, idle;
		uint64_t iowait = 0, irq = 0, softirq = 0, steal = 0;
		int cpu;
		char pattern[128];
		
		strcpy(pattern, "cpu %llu %llu %llu %llu %llu %llu %llu %llu\n");
		for(cpu = 0; fscanf(hStat, pattern,
					           &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal) >= 4; cpu++)
		{
			int64_t userDelta, niceDelta, systemDelta, idleDelta;
			int64_t iowaitDelta, irqDelta, softirqDelta, stealDelta;

			// inspired by top from procps-3.2.7
			userDelta = user - m_user[cpu];
			niceDelta = nice - m_nice[cpu];
			systemDelta = system - m_system[cpu];
			idleDelta = idle - m_idle[cpu];
			if (idleDelta < 0)
			{
				idleDelta = 0;
			}
			iowaitDelta = iowait - m_iowait[cpu];
			irqDelta = irq - m_irq[cpu];
			softirqDelta = softirq - m_softirq[cpu];
			stealDelta = steal - m_steal[cpu]; // steal=time spent in virtualization stuff (xen).

			int64_t total = userDelta + niceDelta + systemDelta + idleDelta +
				iowaitDelta + irqDelta + softirqDelta + stealDelta;
			if (total < 1)
			{
				total = 1;
			}

			float scale = 100.0 / (float)total; // 100 -> HZ (from asm/param.h)

			MutexLock(m_cpuUsageMutex, INFINITE);

			if (m_currentSlot == CPU_USAGE_SLOTS)
			{
				m_currentSlot = 0;
			}

			if (total > 0)
			{
				m_cpuUsage[cpu][m_currentSlot++] = 100.0 - (float)idleDelta * scale;
			}
			else
			{
				if (m_currentSlot == 0)
				{
					m_cpuUsage[cpu][m_currentSlot++] = m_cpuUsage[cpu][CPU_USAGE_SLOTS - 1];
				}
				else
				{
					m_cpuUsage[cpu][m_currentSlot] = m_cpuUsage[cpu][m_currentSlot - 1];
					m_currentSlot++;
				}
			}

			MutexUnlock(m_cpuUsageMutex);

			m_user[cpu] = user;
			m_nice[cpu] = nice;
			m_system[cpu] = system;
			m_idle[cpu] = idle;
			m_iowait[cpu] = iowait;
			m_irq[cpu] = irq;
			m_softirq[cpu] = softirq;
			m_steal[cpu] = steal;
			
			sprintf(pattern, "cpu%d %%llu %%llu %%llu %%llu %%llu %%llu %%llu %%llu\n", cpu);
		}
		fclose(hStat);
		m_maxCPU = cpu - 1;
	}
}

static THREAD_RESULT THREAD_CALL CpuUsageCollectorThread(void *pArg)
{
	setlocale(LC_NUMERIC, "C");
	while(m_stopCollectorThread == false)
	{
		CpuUsageCollector();
		ThreadSleepMs(1000); // sleep 1 second
	}

	return THREAD_OK;
}

void StartCpuUsageCollector(void)
{
	int i, j;

	m_cpuUsageMutex = MutexCreate();
	memset(m_cpuUsage, 0, sizeof(float) * CPU_USAGE_SLOTS * (MAX_CPU + 1));

	memset(m_user, 0, sizeof(uint64_t) * (MAX_CPU + 1));
	memset(m_nice, 0, sizeof(uint64_t) * (MAX_CPU + 1));
	memset(m_system, 0, sizeof(uint64_t) * (MAX_CPU + 1));
	memset(m_idle, 0, sizeof(uint64_t) * (MAX_CPU + 1));
	memset(m_iowait, 0, sizeof(uint64_t) * (MAX_CPU + 1));
	memset(m_irq, 0, sizeof(uint64_t) * (MAX_CPU + 1));
	memset(m_softirq, 0, sizeof(uint64_t) * (MAX_CPU + 1));
	memset(m_steal, 0, sizeof(uint64_t) * (MAX_CPU + 1));

	// get initial count of user/system/idle time
	m_currentSlot = 0;
	CpuUsageCollector();

	sleep(1);

	// fill first slot with u/s/i delta
	m_currentSlot = 0;
	CpuUsageCollector();

	// fill all slots with current cpu usage
	for (i = 1; i < CPU_USAGE_SLOTS; i++)
	{
		for(j = 0; j < MAX_CPU + 1; j++)
		{
			m_cpuUsage[j][i] = m_cpuUsage[j][0];
		}
	}

	// start collector
	m_cpuUsageCollector = ThreadCreateEx(CpuUsageCollectorThread, 0, NULL);
}

void ShutdownCpuUsageCollector(void)
{
	m_stopCollectorThread = true;
	ThreadJoin(m_cpuUsageCollector);
	MutexDestroy(m_cpuUsageMutex);
}

static void GetUsage(int slot, int count, char *value)
{
	float usage = 0;
	int i;

	MutexLock(m_cpuUsageMutex, INFINITE);

	for (i = 0; i < count; i++)
	{
		int position = m_currentSlot - i - 1;

		if (position < 0)
		{
			position += CPU_USAGE_SLOTS;
		}

		usage += m_cpuUsage[slot][position];
	}

	MutexUnlock(m_cpuUsageMutex);

	usage /= count;
	ret_double(value, usage);
}

LONG H_CpuUsage(char *pszParam, char *pArg, char *pValue)
{
	int count;

	switch (CAST_FROM_POINTER(pArg, int))
	{
		case 5: // last 5 min
			count = 5 * 60;
			break;
		case 15: // last 15 min
			count = 15 * 60;
			break;
		default: // should be 0, but jic
			count = 60;
			break;
	}

	GetUsage(0, count, pValue);

	return SYSINFO_RC_SUCCESS;
}

LONG H_CpuUsageEx(char *pszParam, char *pArg, char *pValue)
{
	int count, cpu;
	char buffer[256], *eptr;

	switch (CAST_FROM_POINTER(pArg, int))
	{
		case 5: // last 5 min
			count = 5 * 60;
			break;
		case 15: // last 15 min
			count = 15 * 60;
			break;
		default: // should be 0, but jic
			count = 60;
			break;
	}
	
	if (!NxGetParameterArg(pszParam, 1, buffer, 256))
		return SYSINFO_RC_UNSUPPORTED;
		
	cpu = strtol(buffer, &eptr, 0);
	if ((*eptr != 0) || (cpu < 0) || (cpu >= m_maxCPU))
		return SYSINFO_RC_UNSUPPORTED;

	GetUsage(cpu + 1, count, pValue);

	return SYSINFO_RC_SUCCESS;
}

LONG H_CpuCount(char *pszParam, char *pArg, char *pValue)
{
	ret_uint(pValue, (m_maxCPU > 0) ? m_maxCPU : 1);
	return SYSINFO_RC_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.19  2007/10/30 15:41:50  victor
Fixed bug with CPU usage computation

Revision 1.18  2007/08/28 22:24:14  alk
CPU usage fixed

Revision 1.17  2007/04/25 07:44:09  victor
- Linux and HPUX subagents changed to new model
- ODBCQUERY subagent code cleaned

Revision 1.16  2007/04/24 12:04:10  alk
code reformat

Revision 1.15  2007/02/05 11:28:21  alk
Fixed "Conditional jump or move depends on uninitialised value(s)" in CpuLoad

Revision 1.14  2007/02/05 11:27:49  alk
code reformatted

Revision 1.13  2007/01/15 00:16:07  victor
Implemented Process.CountEx for Linux

Revision 1.12  2006/11/03 13:39:47  victor
Minor changes

Revision 1.11  2006/10/30 17:25:10  victor
Implemented System.ConnectedUsers and System.ActiveUserSessions

Revision 1.10  2006/10/25 16:41:23  victor
- Implemented System.Memory.Virtual.xxx parameters
- Added passing of --disable-iconv to agent upgrade script if needed

Revision 1.9  2006/08/17 20:10:00  alk
fixed gcc4 issues

Revision 1.8  2006/03/02 21:08:21  alk
implemented:
	System.CPU.Usage5
	System.CPU.Usage15

Revision 1.7  2006/03/02 12:17:05  victor
Removed various warnings related to 64bit platforms

Revision 1.6  2006/03/01 23:38:57  alk
times switched to cputime64_t -> u64 -> uint64_t (and %llu in sscanf)

Revision 1.5  2006/03/01 23:26:21  alk
System.CPU.Usage fixed

Revision 1.4  2006/03/01 22:13:09  alk
added System.CPU.Usage [broken]

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
