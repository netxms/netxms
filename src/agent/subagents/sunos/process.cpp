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
//

int ProcRead(PROC_ENT **pEnt, char *szPatern)
{
	struct dirent **pNameList;
	int nCount, nFound;
	char *pPatern;

	if (szPatern != NULL)
	{
		pPatern = szPatern;
	}
	else
	{
		pPatern = "";
	}

	nFound = -1;

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
					if (strstr(psi.pr_fname, pPatern) != NULL)
					{
						nFound++;
						if (pEnt != NULL)
						{
							(*pEnt)[nFound].nPid = atou(pNameList[nCount]->d_name);
							strncpy((*pEnt)[nFound].szProcName, pProcName,
									  sizeof((*pEnt)[nFound].szProcName));
						}
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

	return nRet;
}
