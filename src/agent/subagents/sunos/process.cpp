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
			if (hFile != NULL)
			{
				psinfo_t psi;

				if (read(hFile, &psi, sizeof(psinfo_t)) == sizeof(psinfo_t))
				{
					if ((pszPattern == NULL) || (!strcmp(psi.pr_fname, CHECK_NULL_EX(pszPattern))))
					{
						if (pEnt != NULL)
						{
							(*pEnt)[nFound].nPid = strtoul(pNameList[nCount]->d_name, NULL, 10);
							strncpy((*pEnt)[nFound].szProcName, psi.pr_fname,
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

LONG H_ProcessList(char *pszParam, char *pArg, NETXMS_VALUES_LIST *pValue)
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

LONG H_ProcessCount(char *pszParam, char *pArg, char *pValue)
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

LONG H_SysProcCount(char *pszParam, char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
	int nCount;

	nCount = ProcRead(NULL, NULL);
	if (nCount >= 0)
	{
		ret_int(pValue, nCount);
		nRet = SYSINFO_RC_SUCCESS;
	}

	return nRet;
}


//
// Handler for Process.XXX parameters
//

LONG H_ProcessInfo(char *pszParam, char *pArg, char *pValue)
{
   int nRet = SYSINFO_RC_ERROR;
   char szArg[128] = "";
	int nCount;

	NxGetParameterArg(pszParam, 1, szArg, sizeof(szArg));
	nCount = ProcRead(NULL, szArg);
	if (nCount >= 0)
	{
		nRet = SYSINFO_RC_SUCCESS;
	}

	return nRet;
}
