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

#include "linux_subagent.h"


//
// Filter for reading only pid directories from /proc
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
// Read process information from /proc system
// Parameters:
//    pEnt - If not NULL, ProcRead() will return pointer to dynamically
//           allocated array of process information structures for
//           matched processes. Caller should free it with free().
//    szProcName - If not NULL, only processes with matched name will
//                 be counted and read. If szCmdLine is NULL, then exact
//                 match required to pass filter; otherwise szProcName can
//                 be a regular expression.
//    szCmdLine - If not NULL, only processes with command line matched to
//                regular expression will be counted and read.
// Return value: number of matched processes or -1 in case of error.
//

int ProcRead(PROC_ENT **pEnt, char *szProcName, char *szCmdLine)
{
	struct dirent **pNameList;
	int nCount, nFound;
	BOOL bProcFound, bCmdFound;

	nFound = -1;
	if (pEnt != NULL)
		*pEnt = NULL;
	AgentWriteDebugLog(5, "ProcRead(%p,\"%s\",\"%s\")", pEnt, CHECK_NULL(szProcName), CHECK_NULL(szCmdLine));

	nCount = scandir("/proc", &pNameList, &ProcFilter, alphasort);
	// if everything is null we can simply return nCount!!!
	if (nCount < 0 )
	{
		//perror("scandir");
	}
	else if( pEnt == NULL && szProcName == NULL && szCmdLine == NULL ) // get process count, we can skip long loop
	{
		nFound = nCount;
		// clean up
		while(nCount--)
		{
			free(pNameList[nCount]);
		}
		free(pNameList);
	}
	else 
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
			bProcFound = bCmdFound = FALSE;
			char szFileName[512];
			FILE *hFile;
			char szProcStat[1024] = {0}; 
			char szBuff[1024] = {0}; 
			char *pProcName = NULL, *pProcStat = NULL;
			unsigned long nPid;

			snprintf(szFileName, sizeof(szFileName),
					"/proc/%s/stat", pNameList[nCount]->d_name);
			hFile = fopen(szFileName, "r");
			if (hFile != NULL)
			{
				if (fgets(szProcStat, sizeof(szProcStat), hFile) != NULL)
				{
					if (sscanf(szProcStat, "%lu ", &nPid) == 1)
					{
						pProcName = strchr(szProcStat, ')');
						if (pProcName != NULL)
						{
							pProcStat = pProcName + 1;
							*pProcName = 0;							
							pProcName =  strchr(szProcStat, '(');
							if (pProcName != NULL)
							{
								pProcName++;
								if ((szProcName != NULL) && (*szProcName != 0))
								{
									if (szCmdLine == NULL) // use old style compare
										bProcFound = strcmp(pProcName, szProcName) == 0;
									else
										bProcFound = RegexpMatch(pProcName, szProcName, FALSE);
								}
								else
								{
									bProcFound = TRUE;
								}
							}
						} // pProcName != NULL
					}
				} // fgets   
				fclose(hFile);
			} // hFile

			if ((szCmdLine != NULL) && (*szCmdLine != 0))
			{
				snprintf(szFileName, sizeof(szFileName),
						"/proc/%s/cmdline", pNameList[nCount]->d_name);
				hFile = fopen(szFileName, "r");
				if (hFile != NULL)
				{
					char processCmdLine[1024];
					memset(processCmdLine, 0, sizeof(processCmdLine));

					int len = fread(processCmdLine, 1, sizeof(processCmdLine) - 1, hFile);
					if (len > 0) // got a valid record in format: argv[0]\x00argv[1]\x00...
					{
						int j;
						//char *pArgs;

						/* Commented out by victor: to behave identicaly on different platforms,
						   argv[0] should be matched as well
						j = strlen(szBuff) + 1;
						pArgs = szBuff + j; // skip first (argv[0])
						len -= j;*/
						// replace 0x00 with spaces
						for(j = 0; j < len - 1; j++)
						{
							if (processCmdLine[j] == 0)
							{
								processCmdLine[j] = ' ';
							}
						}
						bCmdFound = RegexpMatch(processCmdLine, szCmdLine, TRUE);
					}
					else
					{
						bCmdFound = RegexpMatch("", szCmdLine, TRUE);
					}
					fclose(hFile);
				} // hFile != NULL
				else
				{
					bCmdFound = RegexpMatch("", szCmdLine, TRUE);
				}
			} // szCmdLine
			else
			{
				bCmdFound = TRUE;
			}

			if (bProcFound && bCmdFound)
			{
				if (pEnt != NULL && pProcName != NULL)
				{
					(*pEnt)[nFound].nPid = nPid;
					nx_strncpy((*pEnt)[nFound].szProcName, pProcName, sizeof((*pEnt)[nFound].szProcName));

					// Parse rest of /proc/pid/stat file
					if (pProcStat != NULL)
					{
						if (sscanf(pProcStat, " %c %d %d %*d %*d %*d %*u %lu %*u %lu %*u %lu %lu %*u %*u %*d %*d %ld %*d %*u %lu %ld ",
						           &(*pEnt)[nFound].state, &(*pEnt)[nFound].parent, &(*pEnt)[nFound].group,
						           &(*pEnt)[nFound].minflt, &(*pEnt)[nFound].majflt,
						           &(*pEnt)[nFound].utime, &(*pEnt)[nFound].ktime, &(*pEnt)[nFound].threads,
						           &(*pEnt)[nFound].vmsize, &(*pEnt)[nFound].rss) != 10)
						{
							AgentWriteDebugLog(2, "Error parsing /proc/%d/stat", nPid);
						}
					}
				}
				nFound++;
			}

			free(pNameList[nCount]);
		}
		free(pNameList);
	}

	if ((nFound < 0) && (pEnt != NULL))
	{
		safe_free(*pEnt);
		*pEnt = NULL;
	}

	return nFound;
}


//
// Handler for System.ProcessCount, Process.Count() and Process.CountEx() parameters
//

LONG H_ProcessCount(const char *pszParam, const char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
	char szArg[128] = "", szCmdLine[128] = "";
	int nCount = -1;

	if (*pArg != 'T')
	{
		AgentGetParameterArg(pszParam, 1, szArg, sizeof(szArg));
		if (*pArg == 'E')
		{
			AgentGetParameterArg(pszParam, 2, szCmdLine, sizeof(szCmdLine));
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


//
// Handler for System.ThreadCount parameter
//

LONG H_ThreadCount(const char *param, const char *arg, char *value)
{
	int i, sum, count, ret = SYSINFO_RC_ERROR;
	PROC_ENT *procList;

	count = ProcRead(&procList, NULL, NULL);
	if (count >= 0)
	{
		for(i = 0, sum = 0; i < count; i++)
			sum += procList[i].threads;
		ret_int(value, sum);
		ret = SYSINFO_RC_SUCCESS;
		free(procList);
	}

	return ret;
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

LONG H_ProcessDetails(const char *param, const char *arg, char *value)
{
	int i, count, type;
	long pageSize, ticksPerSecond;
	INT64 currVal, finalVal;
	PROC_ENT *procList;
	char procName[MAX_PATH], cmdLine[MAX_PATH], buffer[256];
   static const char *typeList[]={ "min", "max", "avg", "sum", NULL };

   // Get parameter type arguments
   AgentGetParameterArg(param, 2, buffer, 256);
   if (buffer[0] == 0)     // Omited type
   {
      type = INFOTYPE_SUM;
   }
   else
   {
      for(type = 0; typeList[type] != NULL; type++)
         if (!stricmp(typeList[type], buffer))
            break;
      if (typeList[type] == NULL)
         return SYSINFO_RC_UNSUPPORTED;     // Unsupported type
   }

	// Get process name
	AgentGetParameterArg(param, 1, procName, MAX_PATH);
	AgentGetParameterArg(param, 3, cmdLine, MAX_PATH);
	StrStrip(cmdLine);

	count = ProcRead(&procList, procName, (cmdLine[0] != 0) ? cmdLine : NULL);
	AgentWriteDebugLog(5, "H_ProcessDetails(\"%s\"): ProcRead() returns %d", param, count);
	if (count == -1)
		return SYSINFO_RC_ERROR;

	pageSize = getpagesize();
	ticksPerSecond = sysconf(_SC_CLK_TCK);
	for(i = 0, finalVal = 0; i < count; i++)
	{
		switch(CAST_FROM_POINTER(arg, int))
		{
			case PROCINFO_CPUTIME:
				currVal = (procList[i].ktime + procList[i].utime) * 1000 / ticksPerSecond;
				break;
			case PROCINFO_KTIME:
				currVal = procList[i].ktime * 1000 / ticksPerSecond;
				break;
			case PROCINFO_UTIME:
				currVal = procList[i].utime * 1000 / ticksPerSecond;
				break;
			case PROCINFO_PAGEFAULTS:
				currVal = procList[i].majflt + procList[i].minflt;
				break;
			case PROCINFO_THREADS:
				currVal = procList[i].threads;
				break;
			case PROCINFO_VMSIZE:
				currVal = procList[i].vmsize;
				break;
			case PROCINFO_WKSET:
				currVal = procList[i].rss * pageSize;
				break;
			default:
				currVal = 0;
				break;
		}

		switch(type)
		{
			case INFOTYPE_SUM:
			case INFOTYPE_AVG:
				finalVal += currVal;
				break;
			case INFOTYPE_MIN:
				finalVal = min(currVal, finalVal);
				break;
			case INFOTYPE_MAX:
				finalVal = max(currVal, finalVal);
				break;
		}
	}

	safe_free(procList);
	if (type == INFOTYPE_AVG)
		finalVal /= count;

	ret_int64(value, finalVal);
	return SYSINFO_RC_SUCCESS;
}

