/* $Id: system.cpp,v 1.3 2006-10-25 16:13:37 victor Exp $ */

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
#include <utmpx.h>
#include <sys/pstat.h>
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
// Handler for System.CPU.LoadAvg parameter
//

LONG H_CpuLoad(char *pszParam, char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
	struct statvfs s;
	char szArg[128] = {0};
	int i;
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
	struct pst_swapinfo pss;
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
		   if (pssw.pss_flags & SW_BLOCK)
		   {
			   qwSwapTotal += (QWORD)pssw.pss_nblksenabled * (QWORD)pssw.pss_swapchunk;
			   qwSwapFree += (QWORD)pssw.pss_nblksavail * (QWORD)pssw.pss_swapchunk;
   		}
		   else if (pssw.pss_flags & SW_FS)
		   {
			   qwSwapTotal += (QWORD)pssw.pss_limit * (QWORD)pssw.pss_swapchunk;
			   qwSwapFree += (QWORD)(pssw.pss_limit - pssw.pss_allocated - pssw.pss_reserve) * (QWORD)pssw.pss_swapchunk;
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
// Handler for System.ProcessCount and Process.Count(*) parameters
//

LONG H_ProcessCount(char *pszParam, char *pArg, char *pValue)
{
	LONG nRet = SYSINFO_RC_ERROR;
	char szArg[128] = "";
	int nTotal = -1;
	int nCount, i;
	int nIndex = 0;
	struct pst_status pst[10];
	int mode = CAST_FROM_POINTER(pArg, int);

	NxGetParameterArg(pszParam, 1, szArg, sizeof(szArg));

	while((nCount = pstat_getproc(pst, sizeof(struct pst_status), 10, nIndex)) > 0)
	{
		for (i = 0; i < nCount; i++)
		{
			if (mode == 1) // System.ProcessCount
			{
				nTotal++;
			}
			else // Process.Count(*)
			{
				if (!strcasecmp(pst[i].pst_ucomm, szArg))
				{
					nTotal++;
				}
			}
		}
		nIndex = pst[nCount - 1].pst_idx + 1;
	}

	if (nTotal >= 0)
	{
		nTotal++;
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
	char szArg[128] = {0};
	int nTotal = -1;
	int nCount, i;
	int nIndex = 0;
	struct pst_status pst[10];
	char szBuff[128];

	while((nCount = pstat_getproc(pst, sizeof(struct pst_status), 10 , nIndex)) > 0)
	{
		for (i = 0; i < nCount; i++)
		{
			nTotal++;
			snprintf(szBuff, sizeof(szBuff), "%d %s", pst[i].pst_pid, pst[i].pst_ucomm);

			NxAddResultString(pValue, szBuff);
		}
		nIndex = pst[nCount - 1].pst_idx + 1;
	}

	if (nTotal >= 0)
	{
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
static uint64_t m_user = 0;
static uint64_t m_system = 0;
static uint64_t m_idle = 0;
// 60 sec * 15 min => 900 sec
#define CPU_USAGE_SLOTS 900
static float m_cpuUsage[CPU_USAGE_SLOTS];
static int m_currentSlot = 0;

static void CpuUsageCollector()
{
/*
		uint64_t user, nice, system, idle;
		if (fscanf(hStat,
					"cpu %llu %llu %llu %llu",
					&user, &nice, &system, &idle) == 4)
		{
			uint64_t total =
				(user - m_user) +
				(system - m_system) +
				(idle - m_idle);

			if (m_currentSlot == CPU_USAGE_SLOTS)
			{
				m_currentSlot = 0;
			}

			if (total > 0)
			{
				m_cpuUsage[m_currentSlot++] =
					100 - ((float)((idle - m_idle) * 100) / total);
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

			m_user = user;
			m_system = system;
			m_idle = idle;
		}
*/
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

	sleep(1);

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
	float usage = 0;
	int i;
	int count;

	MutexLock(m_cpuUsageMutex, INFINITE);

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

	for (i = 0; i < count; i++)
	{
		int position = m_currentSlot - i - 1;

		if (position < 0)
		{
			position += CPU_USAGE_SLOTS;
		}

		usage += m_cpuUsage[position];
	}

	usage /= count;

	ret_double(pValue, usage);

	MutexUnlock(m_cpuUsageMutex);

	return SYSINFO_RC_SUCCESS;
}

LONG H_W(char *pszParam, char *pArg, char *pValue)
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

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.2  2006/10/05 00:34:24  alk
HPUX: minor cleanup; added System.LoggedInCount (W(1) | wc -l equivalent)

Revision 1.1  2006/10/04 14:59:14  alk
initial version of HPUX subagent


*/
