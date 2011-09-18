/* $Id$ */
/*
** NetXMS subagent for AIX
** Copyright (C) 2004-2009 Victor Kirhenshtein
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
** File: proc.cpp
**
**/

#define _H_XCOFF

#include "aix_subagent.h"
#include <procinfo.h>


//
// Possible consolidation types of per-process information
//

#define INFOTYPE_MIN             0
#define INFOTYPE_MAX             1
#define INFOTYPE_AVG             2
#define INFOTYPE_SUM             3


//
// Select 32-bit or 64-bit interface
// At least at AIX 5.1 getprocs() and getprocs64() are not defined, so we
// define them here
//

#if HAVE_GETPROCS64
typedef struct procentry64 PROCENTRY;
#define GETPROCS getprocs64
extern "C" int getprocs64(struct procentry64 *, int, struct fdsinfo64 *, int, pid_t *, int);
#else
typedef struct procsinfo PROCENTRY;
#define GETPROCS getprocs
extern "C" int getprocs(struct procsinfo *, int, struct fdsinfo *, int, pid_t *, int);
#endif


//
// Get process list and number of processes
//

static PROCENTRY *GetProcessList(int *pnprocs)
{
	PROCENTRY *pBuffer;
	int nprocs, nrecs = 100;
	pid_t index;

retry_getprocs:
	pBuffer = (PROCENTRY *)malloc(sizeof(PROCENTRY) * nrecs);
	index = 0;
	nprocs = GETPROCS(pBuffer, sizeof(PROCENTRY), NULL, 0, &index, nrecs);
	if (nprocs != -1)
	{
		if (nprocs == nrecs)
		{
			free(pBuffer);
			nrecs += 100;
			goto retry_getprocs;
		}
		*pnprocs = nprocs;
	}
	else
	{
		free(pBuffer);
		pBuffer = NULL;
	}
	return pBuffer;
}


//
// Handler for System.ProcessList enum
//

LONG H_ProcessList(const char *pszParam, const char *pArg, StringList *pValue)
{
	LONG nRet;
	PROCENTRY *pList;
	int i, nProcCount;
	char szBuffer[256];

	pList = GetProcessList(&nProcCount);
	if (pList != NULL)
	{
		for(i = 0; i < nProcCount; i++)
		{
			snprintf(szBuffer, 256, "%d %s", pList[i].pi_pid, pList[i].pi_comm);
			pValue->add(szBuffer);
		}
		free(pList);
		nRet = SYSINFO_RC_SUCCESS;
	}
	else
	{
		nRet = SYSINFO_RC_ERROR;
	}

	return nRet;
}


//
// Handler for System.ProcessCount parameter
//

LONG H_SysProcessCount(const char *pszParam, const char *pArg, char *pValue)
{
	int nprocs;
	pid_t index = 0;

	nprocs = GETPROCS(NULL, 0, NULL, 0, &index, 65535);
	if (nprocs == -1)
		return SYSINFO_RC_ERROR;
	
	ret_uint(pValue, nprocs);
	return SYSINFO_RC_SUCCESS;
}


//
// Handler for System.ThreadCount parameter
//

LONG H_SysThreadCount(const char *pszParam, const char *pArg, char *pValue)
{
	LONG nRet;
	PROCENTRY *pList;
	int i, nProcCount, nThreadCount;

	pList = GetProcessList(&nProcCount);
	if (pList != NULL)
	{
		for(i = 0, nThreadCount = 0; i < nProcCount; i++)
		{
			nThreadCount += pList[i].pi_thcount;
		}
		free(pList);
		ret_uint(pValue, nThreadCount);
		nRet = SYSINFO_RC_SUCCESS;
	}
	else
	{
		nRet = SYSINFO_RC_ERROR;
	}

	return nRet;
}


//
// Handler for Process.Count(*) parameter
//

LONG H_ProcessCount(const char *pszParam, const char *pArg, char *pValue)
{
	LONG nRet;
	PROCENTRY *pList;
	int i, nSysProcCount, nProcCount;
	char szProcessName[256];

	if (!AgentGetParameterArg(pszParam, 1, szProcessName, 256))
		return SYSINFO_RC_UNSUPPORTED;

	pList = GetProcessList(&nSysProcCount);
	if (pList != NULL)
	{
		for(i = 0, nProcCount = 0; i < nSysProcCount; i++)
		{
			if (!strcmp(pList[i].pi_comm, szProcessName))
				nProcCount++;
		}
		free(pList);
		ret_uint(pValue, nProcCount);
		nRet = SYSINFO_RC_SUCCESS;
	}
	else
	{
		nRet = SYSINFO_RC_ERROR;
	}

	return nRet;
}


//
// Handler for Process.xxx(*) parameters
//

LONG H_ProcessInfo(const char *pszParam, const char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_SUCCESS;
	char szBuffer[256] = "";
	int i, nProcCount, nType, nCount;
	PROCENTRY *pList;
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
	pList = GetProcessList(&nProcCount);
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
						qwCurrVal = pList[i].pi_ru.ru_inblock;
						break;
//					case PROCINFO_IO_WRITE_B:
					case PROCINFO_IO_WRITE_OP:
						qwCurrVal = pList[i].pi_ru.ru_oublock;
						break;
					case PROCINFO_KTIME:
						qwCurrVal = pList[i].pi_ru.ru_stime.tv_sec * 1000 + pList[i].pi_ru.ru_stime.tv_usec / 1000;
						break;
					case PROCINFO_PF:
						qwCurrVal = pList[i].pi_majflt + pList[i].pi_minflt;
						break;
					case PROCINFO_THREADS:
						qwCurrVal = pList[i].pi_thcount;
						break;
					case PROCINFO_UTIME:
						qwCurrVal = pList[i].pi_ru.ru_utime.tv_sec * 1000 + pList[i].pi_ru.ru_utime.tv_usec / 1000;
						break;
					case PROCINFO_VMSIZE:
						qwCurrVal = pList[i].pi_size * getpagesize();
						break;
//					case PROCINFO_WKSET:
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

	return nRet;
}
