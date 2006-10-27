/* $Id: system.cpp,v 1.8 2006-10-27 10:00:51 victor Exp $ */

/* 
** NetXMS subagent for HP-UX
** Copyright (C) 2006 Alex Kirhenshtein
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
*/

#include <nms_common.h>
#include <nms_agent.h>

#include <locale.h>
#include <sys/utsname.h>
#include <sys/statvfs.h>
#include <sys/pstat.h>
#include <sys/dk.h>
#include <utmpx.h>
#include <utmp.h>

#include "system.h"


//
// Handler for System.Uptime parameter
//

LONG H_Uptime(char *pszParam, char *pArg, char *pValue)
{
	FILE *hFile;
	unsigned int uptime = 0;

	struct utmpx id;
	struct utmpx* u;
	setutxent();
	id.ut_type = BOOT_TIME;
	u = getutxid(&id);
	if (u != NULL)
	{
		uptime = time(NULL) - u->ut_tv.tv_sec;
	}

	if (uptime > 0)
	{
		ret_uint(pValue, uptime);
	}

	return uptime > 0 ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
}


//
// Handler for System.Uname parameter
//

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


//
// Handler for System.Hostname parameter
//

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


//
// Handler for System.ConnectedUsers
//

LONG H_ConnectedUsers(char *pszParam, char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
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
// Handler for System.CPU.LoadAvg parameter
//

LONG H_CpuLoad(char *pszParam, char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
	struct pst_dynamic info;

	if(pstat_getdynamic(&info, sizeof(info), 0, 0) >= 0)
	{
		switch (pszParam[19])
		{
			case '1': // 15 min
				ret_double(pValue, info.psd_avg_15_min);
				break;
			case '5': // 5 min
				ret_double(pValue, info.psd_avg_5_min);
				break;
			default: // 1 min
				ret_double(pValue, info.psd_avg_1_min);
				break;
		}

		nRet = SYSINFO_RC_SUCCESS;
	}

	return nRet;
}


//
// Handler for System.Memory.* parameters
//

LONG H_MemoryInfo(char *pszParam, char *pArg, char *pValue)
{
	LONG nRet = SYSINFO_RC_ERROR;
	int mode = CAST_FROM_POINTER(pArg, int);
	QWORD qwSwapTotal, qwSwapFree;
	struct pst_dynamic psd;
	struct pst_static pss;
	struct pst_swapinfo pssw;
	int i;

	// Get physical memory info
	if ((mode != SWAP_FREE) && (mode != SWAP_TOTAL) && (mode != SWAP_USED))
	{
		if (!pstat_getstatic(&pss, sizeof(pss), (size_t)1, 0) ||
		    !pstat_getdynamic(&psd, sizeof(psd), (size_t)1, 0))
		return SYSINFO_RC_ERROR;
	}

	// Get swap info
	if ((mode != PHYSICAL_FREE) && (mode != PHYSICAL_TOTAL) && (mode != PHYSICAL_USED))
	{
		qwSwapTotal = 0;
		qwSwapFree = 0;
		for (i = 0; pstat_getswap(&pssw, sizeof(pssw), (size_t)1, i); i++)
		{
			/* I was unable to find any explanation what pss_swapchunk actually
			 * means - in header file it's commented as "block size", but on
			 * my test system it's equal 2048, but swapinfo shows information
			 * as block size equal 1024 bytes, and every example I was able to
			 * find also uses * 1024 instead of * pssw.pss_swapchunk. */
			if (pssw.pss_flags & SW_BLOCK)
			{
			   qwSwapTotal += (QWORD)pssw.pss_nblksenabled * 1024; //(QWORD)pssw.pss_swapchunk;
			   qwSwapFree += (QWORD)pssw.pss_nblksavail * 1024; //(QWORD)pssw.pss_swapchunk;
			}
			else if (pssw.pss_flags & SW_FS)
			{
			   qwSwapTotal += (QWORD)pssw.pss_limit * 1024; //(QWORD)pssw.pss_swapchunk;
			   qwSwapFree += (QWORD)(pssw.pss_limit - pssw.pss_allocated - pssw.pss_reserve) * 1024; //(QWORD)pssw.pss_swapchunk;
			}
		}
	}

	nRet = SYSINFO_RC_SUCCESS;
	switch(mode)
	{
		case PHYSICAL_FREE: // ph-free
			ret_uint64(pValue, (QWORD)psd.psd_free * (QWORD)pss.page_size);
			break;
		case PHYSICAL_TOTAL: // ph-total
			ret_uint64(pValue, (QWORD)pss.physical_memory * (QWORD)pss.page_size);
			break;
		case PHYSICAL_USED: // ph-used
			ret_uint64(pValue, (QWORD)(pss.physical_memory - psd.psd_free) * (QWORD)pss.page_size);
			break;
		case SWAP_FREE: // sw-free
			ret_uint64(pValue, qwSwapFree);
			break;
		case SWAP_TOTAL: // sw-total
			ret_uint64(pValue, qwSwapTotal);
			break;
		case SWAP_USED: // sw-used
			ret_uint64(pValue, qwSwapTotal - qwSwapFree);
			break;
		case VIRTUAL_FREE: // vi-free
			ret_uint64(pValue, (QWORD)psd.psd_free * (QWORD)pss.page_size + qwSwapFree);
			break;
		case VIRTUAL_TOTAL: // vi-total
			ret_uint64(pValue, (QWORD)pss.physical_memory * (QWORD)pss.page_size + qwSwapTotal);
			break;
		case VIRTUAL_USED: // vi-used
			ret_uint64(pValue, (QWORD)(pss.physical_memory - psd.psd_free) *
                                 (QWORD)pss.page_size + (qwSwapTotal - qwSwapFree));
			break;
		default: // error
			nRet = SYSINFO_RC_ERROR;
			break;
	}

	return nRet;
}


//
// Get process list
//


static struct pst_status *GetProcessList(int *pnNumProcs)
{
	int nSize = 0, nCount;
	struct pst_status *pBuffer = NULL;
	
	do
	{
		nSize += 100;
		pBuffer = (pst_status *)realloc(pBuffer, sizeof(struct pst_status) * nSize);
		nCount = pstat_getproc(pBuffer, sizeof(struct pst_status), nSize, 0);
	} while(nCount == nSize);

	if (nCount <= 0)
	{
		free(pBuffer);
		pBuffer = NULL;
	}
	else
	{
		*pnNumProcs = nCount;
	}

	return pBuffer;
}


//
// Handler for System.ProcessCount parameter
//

LONG H_SysProcessCount(char *pszParam, char *pArg, char *pValue)
{
	struct pst_status *pst;
	int nCount;
	LONG nRet = SYSINFO_RC_ERROR;

	pst = GetProcessList(&nCount);
	if (pst != NULL)
	{
		free(pst);
		nRet = SYSINFO_RC_SUCCESS;
		ret_int(pValue, nCount);
	}
	return nRet;
}


//
// Handler for Process.Count(*) parameter
//

LONG H_ProcessCount(char *pszParam, char *pArg, char *pValue)
{
	struct pst_status *pst;
	int i, nCount, nTotal;
	char szArg[256];
	LONG nRet = SYSINFO_RC_ERROR;

	if (!NxGetParameterArg(pszParam, 1, szArg, sizeof(szArg)))
		return SYSINFO_RC_UNSUPPORTED;

	pst = GetProcessList(&nCount);
	if (pst != NULL)
	{
		for (i = 0, nTotal = 0; i < nCount; i++)
		{
			if (!strcasecmp(pst[i].pst_ucomm, szArg))
				nTotal++;
		}
		free(pst);
		ret_int(pValue, nTotal);
		nRet = SYSINFO_RC_SUCCESS;
	}

	return nRet;
}


//
// Handler for System.ProcessList enum
//

LONG H_ProcessList(char *pszParam, char *pArg, NETXMS_VALUES_LIST *pValue)
{
	LONG nRet = SYSINFO_RC_ERROR;
	int i, nCount;
	struct pst_status *pst;
	char szBuff[256];

	pst = GetProcessList(&nCount);
	if (pst != NULL)
	{
		for (i = 0; i < nCount; i++)
		{
			snprintf(szBuff, sizeof(szBuff), "%d %s", (DWORD)pst[i].pst_pid, pst[i].pst_ucomm);
			NxAddResultString(pValue, szBuff);
		}
		free(pst);
		nRet = SYSINFO_RC_SUCCESS;
	}

	return nRet;
}


///////////////////////////////////////////////////////////////////////////////
// CPU Usage
//

static THREAD m_cpuUsageCollector = INVALID_THREAD_HANDLE;
static MUTEX m_cpuUsageMutex = INVALID_MUTEX_HANDLE;
static bool m_stopCollectorThread = false;
static uint64_t m_total = 0;
static uint64_t m_idle = 0;
// 60 sec * 15 min => 900 sec
#define CPU_USAGE_SLOTS 900
static float m_cpuUsage[CPU_USAGE_SLOTS];
static int m_currentSlot = 0;

static void CpuUsageCollector()
{
	struct pst_dynamic psd;
	uint64_t total, delta;
	int i;

	if (pstat_getdynamic(&psd, sizeof(struct pst_dynamic), 1, 0) == 1)
	{
		for(i = 0, total = 0; i < PST_MAX_CPUSTATES; i++)
			total += psd.psd_cpu_time[i];
		delta = total - m_total;

		if (m_currentSlot == CPU_USAGE_SLOTS)
		{
			m_currentSlot = 0;
		}

		if (delta > 0)
		{
			m_cpuUsage[m_currentSlot++] =
				100 - ((float)((psd.psd_cpu_time[CP_IDLE] - m_idle) * 100) / delta);
		}
		else
		{
			if (m_currentSlot == 0)
			{
				m_cpuUsage[m_currentSlot++] = m_cpuUsage[CPU_USAGE_SLOTS - 1];
			}
			else
			{
				m_cpuUsage[m_currentSlot] = m_cpuUsage[m_currentSlot - 1];
				m_currentSlot++;
			}
		}

		m_idle = psd.psd_cpu_time[CP_IDLE];
		m_total = total;
	}
}

static THREAD_RESULT THREAD_CALL CpuUsageCollectorThread(void *pArg)
{
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

	m_cpuUsageMutex = MutexCreate();

	for (i = 0; i < CPU_USAGE_SLOTS; i++)
	{
		m_cpuUsage[i] = 0;
	}

	// get initial count of user/system/idle time
	m_currentSlot = 0;
	CpuUsageCollector();

	ThreadSleepMs(1000);

	// fill first slot with u/s/i delta
	m_currentSlot = 0;
	CpuUsageCollector();

	// fill all slots with current cpu usage
	for (i = 1; i < CPU_USAGE_SLOTS; i++)
	{
		m_cpuUsage[i] = m_cpuUsage[0];
	}

	// start collector
	m_cpuUsageCollector = ThreadCreateEx(CpuUsageCollectorThread, 0, NULL);
}

void ShutdownCpuUsageCollector(void)
{
	MutexLock(m_cpuUsageMutex, INFINITE);
	m_stopCollectorThread = true;
	MutexUnlock(m_cpuUsageMutex);

	ThreadJoin(m_cpuUsageCollector);
	MutexDestroy(m_cpuUsageMutex);

}

LONG H_CpuUsage(char *pszParam, char *pArg, char *pValue)
{
	double usage = 0;
	int i, count;

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

	MutexLock(m_cpuUsageMutex, INFINITE);

	for (i = 0; i < count; i++)
	{
		int position = m_currentSlot - i - 1;

		if (position < 0)
		{
			position += CPU_USAGE_SLOTS;
		}

		usage += m_cpuUsage[position];
	}

	MutexUnlock(m_cpuUsageMutex);

	usage /= count;
	ret_double(pValue, usage);

	return SYSINFO_RC_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.7  2006/10/26 17:46:22  victor
System.CPU.Usage almost complete

Revision 1.6  2006/10/26 15:19:39  victor
Fixed problems with Process.Count and System.ProcessCount on HP-UX 11.23

Revision 1.5  2006/10/26 06:55:17  victor
Minor changes

Revision 1.4  2006/10/25 22:12:05  victor
System.Memory.xxx seems to be working

Revision 1.3  2006/10/25 16:13:37  victor
Implemented Memory.xxx parameters

Revision 1.2  2006/10/05 00:34:24  alk
HPUX: minor cleanup; added System.LoggedInCount (W(1) | wc -l equivalent)

Revision 1.1  2006/10/04 14:59:14  alk
initial version of HPUX subagent


*/
