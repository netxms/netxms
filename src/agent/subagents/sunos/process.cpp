/* $Id$ */

/*
 ** NetXMS subagent for SunOS/Solaris
 ** Copyright (C) 2004 Victor Kirhenshtein
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
 ** $module: process.cpp
 **
 **/

#include "sunos_subagent.h"
#include <procfs.h>


//
// Constants
//

#define INFOTYPE_MIN             0
#define INFOTYPE_MAX             1
#define INFOTYPE_AVG             2
#define INFOTYPE_SUM             3


//
// Filter function for scandir() in ProcRead()
//

static int ProcFilter(const struct dirent *pEnt)
{
	char *pTmp;

	if (pEnt == NULL)
	{
		return 0; // ignore file
	}

	pTmp = (char *)pEnt->d_name;
	while (*pTmp != 0)
	{
		if (*pTmp < '0' || *pTmp > '9')
		{
			return 0; // skip
		}
		pTmp++;
	}

	return 1;
}


//
// Read process list from /proc filesystem
// Returns only processes with names matched to pszPattern
// (currently with strcmp) ot, if pszPattern is NULL, all
// processes in the system.
//

static int ProcRead(PROC_ENT **pEnt, char *pszPattern)
{
	struct dirent **pNameList;
	int nCount, nFound;

	nFound = -1;
	if (pEnt != NULL)
		*pEnt = NULL;

	nCount = scandir("/proc", &pNameList, &ProcFilter, alphasort);
	if (nCount >= 0)
	{
		nFound = 0;

		if (nCount > 0 && pEnt != NULL)
		{
			*pEnt = (PROC_ENT *)malloc(sizeof(PROC_ENT) * (nCount + 1));

			if (*pEnt == NULL)
			{
				nFound = -1;
				// cleanup
				while(nCount--)
				{
					free(pNameList[nCount]);
				}
			}
			else
			{
				memset(*pEnt, 0, sizeof(PROC_ENT) * (nCount + 1));
			}
		}

		while(nCount--)
		{
			char szFileName[256];
			int hFile;

			snprintf(szFileName, sizeof(szFileName),
					"/proc/%s/psinfo", pNameList[nCount]->d_name);
			hFile = open(szFileName, O_RDONLY);
			if (hFile != -1)
			{
				psinfo_t psi;

				if (read(hFile, &psi, sizeof(psinfo_t)) == sizeof(psinfo_t))
				{
					if ((pszPattern == NULL) || (!strcmp(psi.pr_fname, CHECK_NULL_EX(pszPattern))))
					{
						if (pEnt != NULL)
						{
							(*pEnt)[nFound].nPid = strtoul(pNameList[nCount]->d_name, NULL, 10);
							nx_strncpy((*pEnt)[nFound].szProcName, psi.pr_fname,
									sizeof((*pEnt)[nFound].szProcName));
						}
						nFound++;
					}
				}
				close(hFile);
			}
			free(pNameList[nCount]);
		}
		free(pNameList);
	}

	return nFound;
}


//
// Process list
//

LONG H_ProcessList(const char *pszParam, const char *pArg, NETXMS_VALUES_LIST *pValue)
{
	LONG nRet = SYSINFO_RC_SUCCESS;
	PROC_ENT *pList;
	int i, nProc;
	char szBuffer[256];

	nProc = ProcRead(&pList, NULL);
	if (nProc != -1)
	{
		for(i = 0; i < nProc; i++)
		{
			snprintf(szBuffer, sizeof(szBuffer), "%d %s", pList[i].nPid,
					pList[i].szProcName);
			NxAddResultString(pValue, szBuffer);
		}
	}
	else
	{
		nRet = SYSINFO_RC_ERROR;
	}
	safe_free(pList);

	return nRet;
}


//
// Handler for Process.Count(*) parameter
//

LONG H_ProcessCount(const char *pszParam, const char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
	char szArg[128] = "";
	int nCount;

	NxGetParameterArg(pszParam, 1, szArg, sizeof(szArg));
	nCount = ProcRead(NULL, szArg);
	if (nCount >= 0)
	{
		ret_int(pValue, nCount);
		nRet = SYSINFO_RC_SUCCESS;
	}

	return nRet;
}


//
// Handler for System.ProcessCount parameter
//

LONG H_SysProcCount(const char *pszParam, const char *pArg, char *pValue)
{
	return ReadKStatValue("unix", 0, "system_misc", "nproc", pValue, NULL);
}


//
// Read process information file from /proc file system
//

static BOOL ReadProcFile(pid_t nPid, char *pszFile, void *pData, size_t nDataLen)
{
	char szFileName[256];
	int hFile;
	BOOL bResult = FALSE;

	snprintf(szFileName, sizeof(szFileName), "/proc/%d/%s", nPid, pszFile);
	hFile = open(szFileName, O_RDONLY);
	if (hFile != -1)
	{
		psinfo_t psi;

		if (read(hFile, pData, nDataLen) == nDataLen)
		{
			bResult = TRUE;
		}
		close(hFile);
	}
	return bResult;
}


//
// Get specific process attribute
//

static BOOL GetProcessAttribute(pid_t nPid, int nAttr, int nType,
                                int nCount, QWORD *pqwValue)
{
	QWORD qwValue;  
	char szFileName[MAX_PATH];
	prusage_t usage;
	pstatus_t status;
	BOOL bResult = TRUE;

	// Get value for current process instance
	switch(nAttr)
	{
		case PROCINFO_VMSIZE:
			break;
		case PROCINFO_WKSET:
			break;
		case PROCINFO_PF:
			if (ReadProcFile(nPid, "usage", &usage, sizeof(prusage_t)))
			{
				qwValue = usage.pr_minf + usage.pr_majf;
			}
			else
			{
				bResult = FALSE;
			}
			break;
		case PROCINFO_KTIME:
			if (ReadProcFile(nPid, "status", &status, sizeof(pstatus_t)))
			{
				qwValue = status.pr_stime.tv_sec * 1000 + 
					status.pr_stime.tv_nsec / 1000000;
			}
			else
			{
				bResult = FALSE;
			}
			break;
		case PROCINFO_UTIME:
			if (ReadProcFile(nPid, "status", &status, sizeof(pstatus_t)))
			{
				qwValue = status.pr_utime.tv_sec * 1000 + 
					status.pr_utime.tv_nsec / 1000000;
			}
			else
			{
				bResult = FALSE;
			}
			break;
		case PROCINFO_IO_READ_B:
			break;
		case PROCINFO_IO_READ_OP:
			break;
		case PROCINFO_IO_WRITE_B:
			break;
		case PROCINFO_IO_WRITE_OP:
			break;
		default:
			bResult = FALSE;
			break;
	}

	// Recalculate final value according to selected type
	if (nCount == 0)     // First instance
	{
		*pqwValue = qwValue;
	}
	else
	{
		switch(nType)
		{
			case INFOTYPE_MIN:
				*pqwValue = min(*pqwValue, qwValue);
				break;
			case INFOTYPE_MAX:
				*pqwValue = max(*pqwValue, qwValue);
				break;
			case INFOTYPE_AVG:
				*pqwValue = (*pqwValue * nCount + qwValue) / (nCount + 1);
				break;
			case INFOTYPE_SUM:
				*pqwValue = *pqwValue + qwValue;
				break;
			default:
				bResult = FALSE;
				break;
		}
	}
	return bResult;
}


//
// Handler for Process.XXX parameters
//

LONG H_ProcessInfo(const char *pszParam, const char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
	char szBuffer[256] = "";
	int i, nCount, nType;
	PROC_ENT *pList;
	QWORD qwValue;
	static char *pszTypeList[]={ "min", "max", "avg", "sum", NULL };

	// Get parameter type arguments
	NxGetParameterArg(pszParam, 2, szBuffer, sizeof(szBuffer));
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

	NxGetParameterArg(pszParam, 1, szBuffer, sizeof(szBuffer));
	nCount = ProcRead(&pList, szBuffer);
	if (nCount > 0)
	{
		for(i = 0, qwValue = 0; i < nCount; i++)
			if (!GetProcessAttribute(pList[i].nPid, (int)pArg, nType, i, &qwValue))
				break;
		if (i == nCount)
		{
			ret_uint64(pValue, qwValue);
			nRet = SYSINFO_RC_SUCCESS;
		}
	}

	return nRet;
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.9  2006/05/15 22:11:22  alk
+ Net.Interface.Link() workaround; trying kstat() first, then
IFF_RUNNING it kstat's link_up failed.
- code reformated


*/
