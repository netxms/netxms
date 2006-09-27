/* $Id: proc.cpp,v 1.2 2006-09-27 19:09:17 victor Exp $ */
/*
** NetXMS subagent for AIX
** Copyright (C) 2004, 2005, 2006 Victor Kirhenshtein
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

#include "aix_subagent.h"
#include <procinfo.h>


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

LONG H_ProcessList(char *pszParam, char *pArg, NETXMS_VALUES_LIST *pValue)
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
			NxAddResultString(pValue, szBuffer);
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

LONG H_SysProcessCount(char *pszParam, char *pArg, char *pValue)
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

LONG H_SysThreadCount(char *pszParam, char *pArg, char *pValue)
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
