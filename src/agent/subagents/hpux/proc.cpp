/* 
** NetXMS subagent for HP-UX
** Copyright (C) 2006 Alex Kirhenshtein
** Copyright (C) 2010 Victor Kirhenshtein
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
// Possible consolidation types of per-process information
//

#define INFOTYPE_MIN             0
#define INFOTYPE_MAX             1
#define INFOTYPE_AVG             2
#define INFOTYPE_SUM             3


//
// Handler for System.ProcessCount parameter
//

LONG H_SysProcessCount(const char *pszParam, const char *pArg, char *pValue)
{
	struct pst_dynamic pd;
	LONG nRet = SYSINFO_RC_ERROR;

	if (pstat_getdynamic(&pd, sizeof(struct pst_dynamic), 1, 0) == 1)
	{
		nRet = SYSINFO_RC_SUCCESS;
		ret_int(pValue, (LONG)pd.psd_activeprocs);
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
// Handler for Process.Count(*) parameter
//

LONG H_ProcessCount(const char *pszParam, const char *pArg, char *pValue)
{
	struct pst_status *pst;
	int i, nCount, nTotal;
	char szArg[256];
	LONG nRet = SYSINFO_RC_ERROR;

	if (!AgentGetParameterArg(pszParam, 1, szArg, sizeof(szArg)))
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
// Handler for Process.xxx() parameters
// Parameter has the following syntax:
//    Process.XXX(<process>,<type>,<cmdline>)
// where
//    XXX        - requested process attribute (see documentation for list of valid attributes)
//    <process>  - process name (same as in Process.Count() parameter)
//    <type>     - representation type (meaningful when more than one process with the same
//                 name exists). Valid values are:
//         min - minimal value among all processes named <process>
//         max - maximal value among all processes named <process>
//         avg - average value for all processes named <process>
//         sum - sum of values for all processes named <process>
//    <cmdline>  - command line
//

LONG H_ProcessDetails(const char *pszParam, const char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_SUCCESS;
	char szBuffer[256] = "";
	int i, nProcCount, nType, nCount;
	struct pst_status *pList;
	QWORD qwValue, qwCurrVal;
	static const char *pszTypeList[]={ "min", "max", "avg", "sum", NULL };

	// Get parameter type arguments
	AgentGetParameterArg(pszParam, 2, szBuffer, sizeof(szBuffer));
	if (szBuffer[0] == 0)     // Omited type
	{
		nType = INFOTYPE_SUM;
	}
	else
	{
		for(nType = 0; pszTypeList[nType] != NULL; nType++)
			if (!stricmp(pszTypeList[nType], szBuffer))
				break;
		if (pszTypeList[nType] == NULL)
			return SYSINFO_RC_UNSUPPORTED;     // Unsupported type
	}

	AgentGetParameterArg(pszParam, 1, szBuffer, sizeof(szBuffer));

	pList = GetProcessList(&nCount);
	if (pList != NULL)
	{
		for(i = 0, qwValue = 0, nCount = 0; i < nProcCount; i++)
		{
			if (!strcmp(pList[i].pi_comm, szBuffer))
			{
				switch(CAST_FROM_POINTER(pArg, int))
				{
//					case PROCINFO_IO_READ_B:
					case PROCINFO_IO_READ_OP:
						qwCurrVal = pList[i].pst_inblock;
						break;
//					case PROCINFO_IO_WRITE_B:
					case PROCINFO_IO_WRITE_OP:
						qwCurrVal = pList[i].pst_oublock;
						break;
					case PROCINFO_KTIME:
						qwCurrVal = pList[i].pst_stime * 1000;
						break;
					case PROCINFO_PF:
						qwCurrVal = pList[i].pst_minorfaults + pList[i].pst_majorfaults;
						break;
					case PROCINFO_THREADS:
						qwCurrVal = pList[i].pst_nlwps;
						break;
					case PROCINFO_UTIME:
						qwCurrVal = pList[i].pst_utime * 1000;
						break;
					case PROCINFO_VMSIZE:
						qwCurrVal = (pst[i].pst_vtsize  /* text */
								+ pst[i].pst_vdsize  /* data */
								+ pst[i].pst_vssize  /* stack */
								+ pst[i].pst_vshmsize  /* shared memory */
								+ pst[i].pst_vmmsize  /* mem-mapped files */
								+ pst[i].pst_vusize  /* U-Area & K-Stack */
								+ pst[i].pst_viosize) /* I/O dev mapping */ 
							* getpagesize();
						break;
					case PROCINFO_WKSET:
						qwCurrVal = pList[i].pst_rssize * getpagesize();
						break;
					default:
						nRet = SYSINFO_RC_UNSUPPORTED;
						break;
				}

				if (nCount == 0)
				{
					qwValue = qwCurrVal;
				}
				else
				{
					switch(nType)
					{
						case INFOTYPE_MIN:
							qwValue = min(qwValue, qwCurrVal);
							break;
						case INFOTYPE_MAX:
							qwValue = max(qwValue, qwCurrVal);
							break;
						case INFOTYPE_AVG:
							qwValue = (qwValue * nCount + qwCurrVal) / (nCount + 1);
							break;
						case INFOTYPE_SUM:
							qwValue += qwCurrVal;
							break;
					}
				}
				nCount++;
			}
		}
		free(pList);
		ret_uint64(pValue, qwValue);
	}
	else
	{
		nRet = SYSINFO_RC_ERROR;
	}	



	if (pst != NULL)
	{
		for (i = 0, nTotal = 0; i < nCount; i++)
		{
			if (!strcasecmp(pst[i].pst_ucomm, szArg))
			{
				value += 
					pst[i].pst_vtsize + /* text */
					+ pst[i].pst_vdsize + /* data */
					+ pst[i].pst_vssize + /* stack */
					+ pst[i].pst_vshmsize + /* shared memory */
					+ pst[i].pst_vmmsize + /* mem-mapped files */
					+ pst[i].pst_vusize + /* U-Area & K-Stack */
					+ pst[i].pst_viosize; /* I/O dev mapping */ 
				break;
			}
		}
		free(pst);
		if (i == nCount) // not found
			nRet = SYSINFO_RC_ERROR;
		else
		{
			ret_int64(pValue, value);
			nRet = SYSINFO_RC_SUCCESS;
		}
	}

	return nRet;
}

//
// Handler for System.ProcessList enum
//

LONG H_ProcessList(const char *pszParam, const char *pArg, StringList *pValue)
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
			pValue->add(szBuff);
		}
		free(pst);
		nRet = SYSINFO_RC_SUCCESS;
	}

	return nRet;
}
