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

/**
 * Filter for reading only pid directories from /proc
 */
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

/**
 * Read process information from /proc system
 * Parameters:
 *    pEnt - If not NULL, ProcRead() will return pointer to dynamically
 *           allocated array of process information structures for
 *           matched processes. Caller should free it with free().
 *    szProcName - If not NULL, only processes with matched name will
 *                 be counted and read. If szCmdLine is NULL, then exact
 *                 match required to pass filter; otherwise szProcName can
 *                 be a regular expression.
 *    szCmdLine - If not NULL, only processes with command line matched to
 *                regular expression will be counted and read.
 * Return value: number of matched processes or -1 in case of error.
 */
int ProcRead(PROC_ENT **pEnt, char *szProcName, char *szCmdLine)
{
	struct dirent **pNameList;
	int nCount, nFound;
	BOOL bProcFound, bCmdFound;

	nFound = -1;
	if (pEnt != NULL)
		*pEnt = NULL;
	AgentWriteDebugLog(5, _T("ProcRead(%p,\"%hs\",\"%hs\")"), pEnt, CHECK_NULL_A(szProcName), CHECK_NULL_A(szCmdLine));

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
										bProcFound = RegexpMatchA(pProcName, szProcName, FALSE);
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
               size_t len = 0, pos = 0;
					char *processCmdLine = (char *)malloc(1024);
               while(true)
               {
					   int bytes = fread(&processCmdLine[pos], 1, 1024, hFile);
                  if (bytes < 0)
                     bytes = 0;
                  len += bytes;
                  if (bytes < 1024)
                  {
                     processCmdLine[len] = 0;
                     break;
                  }
                  pos += bytes;
                  processCmdLine = (char *)realloc(processCmdLine, pos + 1024);
               }
					if (len > 0)
					{
						// got a valid record in format: argv[0]\x00argv[1]\x00...
						// Note: to behave identicaly on different platforms,
						// full command line including argv[0] should be matched
						// replace 0x00 with spaces
						for(int j = 0; j < len - 1; j++)
						{
							if (processCmdLine[j] == 0)
							{
								processCmdLine[j] = ' ';
							}
						}
						bCmdFound = RegexpMatchA(processCmdLine, szCmdLine, TRUE);
					}
					else
					{
						bCmdFound = RegexpMatchA("", szCmdLine, TRUE);
					}
					fclose(hFile);
               free(processCmdLine);
				} // hFile != NULL
				else
				{
					bCmdFound = RegexpMatchA("", szCmdLine, TRUE);
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
					strncpy((*pEnt)[nFound].szProcName, pProcName, sizeof((*pEnt)[nFound].szProcName));

					// Parse rest of /proc/pid/stat file
					if (pProcStat != NULL)
					{
						if (sscanf(pProcStat, " %c %d %d %*d %*d %*d %*u %lu %*u %lu %*u %lu %lu %*u %*u %*d %*d %ld %*d %*u %lu %ld ",
						           &(*pEnt)[nFound].state, &(*pEnt)[nFound].parent, &(*pEnt)[nFound].group,
						           &(*pEnt)[nFound].minflt, &(*pEnt)[nFound].majflt,
						           &(*pEnt)[nFound].utime, &(*pEnt)[nFound].ktime, &(*pEnt)[nFound].threads,
						           &(*pEnt)[nFound].vmsize, &(*pEnt)[nFound].rss) != 10)
						{
							AgentWriteDebugLog(2, _T("Error parsing /proc/%d/stat"), nPid);
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

/**
 * Handler for System.ProcessCount, Process.Count() and Process.CountEx() parameters
 */
LONG H_ProcessCount(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
	char szArg[128] = "", szCmdLine[128] = "";
	int nCount = -1;

	if (*pArg != _T('T'))
	{
		AgentGetParameterArgA(pszParam, 1, szArg, sizeof(szArg));
		if (*pArg == _T('E'))
		{
			AgentGetParameterArgA(pszParam, 2, szCmdLine, sizeof(szCmdLine));
		}
	}

	nCount = ProcRead(NULL, (*pArg != _T('T')) ? szArg : NULL, (*pArg == _T('E')) ? szCmdLine : NULL);

	if (nCount >= 0)
	{
		ret_int(pValue, nCount);
		nRet = SYSINFO_RC_SUCCESS;
	}

	return nRet;
}

/**
 * Handler for System.ThreadCount parameter
 */
LONG H_ThreadCount(const TCHAR *param, const TCHAR *arg, TCHAR *value)
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

/**
 * Handler for Process.xxx() parameters
 * Parameter has the following syntax:
 *    Process.XXX(<process>,<type>,<cmdline>)
 * where
 *    XXX        - requested process attribute (see documentation for list of valid attributes)
 *    <process>  - process name (same as in Process.Count() parameter)
 *    <type>     - representation type (meaningful when more than one process with the same
 *                 name exists). Valid values are:
 *         min - minimal value among all processes named <process>
 *         max - maximal value among all processes named <process>
 *         avg - average value for all processes named <process>
 *         sum - sum of values for all processes named <process>
 *    <cmdline>  - command line
 */
LONG H_ProcessDetails(const TCHAR *param, const TCHAR *arg, TCHAR *value)
{
	int i, count, type;
	INT64 currVal, finalVal;
	PROC_ENT *procList;
	char procName[MAX_PATH], cmdLine[MAX_PATH], buffer[256];
   static const char *typeList[]={ "min", "max", "avg", "sum", NULL };

   // Get parameter type arguments
   AgentGetParameterArgA(param, 2, buffer, 256);
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
	AgentGetParameterArgA(param, 1, procName, MAX_PATH);
	AgentGetParameterArgA(param, 3, cmdLine, MAX_PATH);
	StrStripA(cmdLine);

	count = ProcRead(&procList, procName, (cmdLine[0] != 0) ? cmdLine : NULL);
	AgentWriteDebugLog(5, _T("H_ProcessDetails(\"%hs\"): ProcRead() returns %d"), param, count);
	if (count == -1)
		return SYSINFO_RC_ERROR;

	long pageSize = getpagesize();
	long ticksPerSecond = sysconf(_SC_CLK_TCK);
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

/**
 * Handler for System.ProcessList list
 */
LONG H_ProcessList(const TCHAR *pszParam, const TCHAR *pArg, StringList *pValue)
{
   int nRet = SYSINFO_RC_ERROR;

   PROC_ENT *plist;
   int nCount = ProcRead(&plist, NULL, NULL);
   if (nCount >= 0)
   {    
      nRet = SYSINFO_RC_SUCCESS;

      for(int i = 0; i < nCount; i++)
      {         
         TCHAR szBuff[128];
         _sntprintf(szBuff, sizeof(szBuff), _T("%d %hs"), plist[i].nPid, plist[i].szProcName);
         pValue->add(szBuff);
      }         
   }    
	safe_free(plist);
   return nRet;
}

/**
 * Handler for System.Processes table
 */
LONG H_ProcessTable(const TCHAR *cmd, const TCHAR *arg, Table *value)
{
   value->addColumn(_T("PID"), DCI_DT_UINT, _T("PID"), true);
   value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"));
   value->addColumn(_T("THREADS"), DCI_DT_UINT, _T("Threads"));
   value->addColumn(_T("KTIME"), DCI_DT_UINT64, _T("Kernel Time"));
   value->addColumn(_T("UTIME"), DCI_DT_UINT64, _T("User Time"));
   value->addColumn(_T("VMSIZE"), DCI_DT_UINT64, _T("VM Size"));
   value->addColumn(_T("RSS"), DCI_DT_UINT64, _T("RSS"));
   value->addColumn(_T("PAGE_FAULTS"), DCI_DT_UINT64, _T("Page Faults"));
   value->addColumn(_T("CMDLINE"), DCI_DT_STRING, _T("Command Line"));

   int rc = SYSINFO_RC_ERROR;

   PROC_ENT *plist;
   int nCount = ProcRead(&plist, NULL, NULL);
   if (nCount >= 0)
   {    
      rc = SYSINFO_RC_SUCCESS;

      UINT64 pageSize = getpagesize();
      UINT64 ticksPerSecond = sysconf(_SC_CLK_TCK);
      for(int i = 0; i < nCount; i++)
      {         
         value->addRow();
         value->set(0, plist[i].nPid);
#ifdef UNICODE
         value->setPreallocated(1, WideStringFromMBString(plist[i].szProcName));
#else
         value->set(1, plist[i].szProcName);
#endif
         value->set(2, (UINT32)plist[i].threads);
         value->set(3, (UINT64)plist[i].ktime * 1000 / ticksPerSecond);
         value->set(4, (UINT64)plist[i].utime * 1000 / ticksPerSecond);
         value->set(5, (UINT64)plist[i].vmsize);
         value->set(6, (UINT64)plist[i].rss * pageSize);
         value->set(7, (UINT64)plist[i].minflt + (UINT64)plist[i].majflt);
      }         
   }    
   safe_free(plist);
   return rc;
}
