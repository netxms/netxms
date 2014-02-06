/*
** NetXMS subagent for SunOS/Solaris
** Copyright (C) 2004-2014 Victor Kirhenshtein
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
** File: process.cpp
**
**/

#include "sunos_subagent.h"
#include <procfs.h>
#include <sys/proc.h>


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
// Read process information from /proc system
// Parameters:
//    pEnt - If not NULL, ProcRead() will return pointer to dynamically
//           allocated array of process information structures for
//           matched processes. Caller should free it with free().
//    pszProcName - If not NULL, only processes with matched name will
//                  be counted and read. If szCmdLine is NULL, then exact
//                  match required to pass filter; otherwise szProcName can
//                  be a regular expression.
//    pszCmdLine - If not NULL, only processes with command line matched to
//                 regular expression will be counted and read.
//    lwpState   - If non-zero, only processes with pstatus_t.pr_lwp.pr_state
//                 (as defined in proc.h: SSLEEP, SRUN, SZOMB, SSTOP,
//                 SIDL, SONPROC) will be counted
// Return value: number of matched processes or -1 in case of error.
//

static int ProcRead(PROC_ENT **pEnt, char *pszProcName, char *pszCmdLine, int lwpState)
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
               BOOL nameMatch = TRUE, cmdLineMatch = TRUE, lwpStateMatch = TRUE;

               if (pszProcName != NULL)
               {
                  if (pszCmdLine == NULL)		// Match like in Process.Count()
                     nameMatch = !strcmp(psi.pr_fname, pszProcName);
                  else
                     nameMatch = RegexpMatchA(psi.pr_fname, pszProcName, FALSE);
               }

               if ((pszCmdLine != NULL) && (*pszCmdLine != 0))
               {
                  cmdLineMatch = RegexpMatchA(psi.pr_psargs, pszCmdLine, TRUE);
               }

               if (lwpState != 0)
               {
                  lwpStateMatch = psi.pr_lwp.pr_state == lwpState;
               }

               if (nameMatch && cmdLineMatch && lwpStateMatch)
               {
                  if (pEnt != NULL)
                  {
                     (*pEnt)[nFound].nPid = strtoul(pNameList[nCount]->d_name, NULL, 10);
                     strncpy((*pEnt)[nFound].szProcName, psi.pr_fname,
                           sizeof((*pEnt)[nFound].szProcName));
                     (*pEnt)[nFound].szProcName[sizeof((*pEnt)[nFound].szProcName) - 1] = 0;
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

   if ((nFound < 0) && (pEnt != NULL))
   {
      safe_free(*pEnt);
      *pEnt = NULL;
   }
   return nFound;
}

/**
 * Process list
 */
LONG H_ProcessList(const TCHAR *pszParam, const TCHAR *pArg, StringList *pValue)
{
   LONG nRet = SYSINFO_RC_SUCCESS;
   PROC_ENT *pList;
   int i, nProc;
   TCHAR szBuffer[256];

   nProc = ProcRead(&pList, NULL, NULL, 0);
   if (nProc != -1)
   {
      for(i = 0; i < nProc; i++)
      {
         _sntprintf(szBuffer, 256, _T("%d %hs"), pList[i].nPid, pList[i].szProcName);
         pValue->add(szBuffer);
      }
   }
   else
   {
      nRet = SYSINFO_RC_ERROR;
   }
   safe_free(pList);

   return nRet;
}

/**
 * Handler for Process.Count(*) and Process.CountEx(*) parameters
 */
LONG H_ProcessCount(const TCHAR *param, const TCHAR *arg, TCHAR *value)
{
   int nRet = SYSINFO_RC_ERROR;
   char procName[256] = "", cmdLine[256] = "";
   int nCount;

   AgentGetParameterArgA(param, 1, procName, sizeof(procName));
   if (*arg == _T('E'))	// Process.CountEx
      AgentGetParameterArgA(param, 2, cmdLine, sizeof(cmdLine));

   switch (*arg)
   {
      case 'E':
         nCount = ProcRead(NULL, (procName[0] != 0) ? procName : NULL, cmdLine, 0);
         break;
      case 'Z':
         nCount = ProcRead(NULL, (procName[0] != 0) ? procName : NULL, NULL, SZOMB);
         break;
      default:
         nCount = ProcRead(NULL, (procName[0] != 0) ? procName : NULL, cmdLine, 0);
         // 'S'
         break;
   }

   if (nCount >= 0)
   {
      ret_int(value, nCount);
      nRet = SYSINFO_RC_SUCCESS;
   }

   return nRet;
}

/**
 * Handler for System.ProcessCount parameter
 */
LONG H_SysProcCount(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
{
   return ReadKStatValue("unix", 0, "system_misc", "nproc", pValue, NULL);
}

/**
 * Read process information file from /proc file system
 */
static BOOL ReadProcFile(pid_t nPid, const char *pszFile, void *pData, size_t nDataLen)
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

/**
 * Get specific process attribute
 */
static BOOL GetProcessAttribute(pid_t nPid, int nAttr, int nType, int nCount, QWORD *pqwValue)
{
   QWORD qwValue = 0;  
   char szFileName[MAX_PATH];
   prusage_t usage;
   pstatus_t status;
   psinfo_t info;
   BOOL bResult = TRUE;

   // Get value for current process instance
   switch(nAttr)
   {
      case PROCINFO_VMSIZE:
         if (ReadProcFile(nPid, "psinfo", &info, sizeof(psinfo_t)))
         {
            qwValue = info.pr_size * 1024;
         }
         else
         {
            bResult = FALSE;
         }
         break;
      case PROCINFO_WKSET:
         if (ReadProcFile(nPid, "psinfo", &info, sizeof(psinfo_t)))
         {
            qwValue = info.pr_rssize * 1024;
         }
         else
         {
            bResult = FALSE;
         }
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
      case PROCINFO_SYSCALLS:
         if (ReadProcFile(nPid, "usage", &usage, sizeof(prusage_t)))
         {
            qwValue = usage.pr_sysc;
         }
         else
         {
            bResult = FALSE;
         }
         break;
      case PROCINFO_THREADS:
         if (ReadProcFile(nPid, "status", &status, sizeof(pstatus_t)))
         {
            qwValue = status.pr_nlwp;
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
      case PROCINFO_CPUTIME:
         if (ReadProcFile(nPid, "status", &status, sizeof(pstatus_t)))
         {
            qwValue = status.pr_utime.tv_sec * 1000 + 
               status.pr_utime.tv_nsec / 1000000 +
               status.pr_stime.tv_sec * 1000 + 
               status.pr_stime.tv_nsec / 1000000;
         }
         else
         {
            bResult = FALSE;
         }
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

/**
 * Handler for Process.XXX parameters
 */
LONG H_ProcessInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value)
{
   int nRet = SYSINFO_RC_ERROR;
   char szBuffer[256] = "", procName[256] = "", cmdLine[256] = "";
   int i, nCount, nType;
   PROC_ENT *pList;
   QWORD qwValue;
   static const char *pszTypeList[]={ "min", "max", "avg", "sum", NULL };

   // Get parameter type arguments
   AgentGetParameterArgA(param, 2, szBuffer, sizeof(szBuffer));
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

   // Get process name
   AgentGetParameterArgA(param, 1, procName, MAX_PATH);
   AgentGetParameterArgA(param, 3, cmdLine, MAX_PATH);
   StrStripA(cmdLine);

   nCount = ProcRead(&pList, (procName[0] != 0) ? procName : NULL, (cmdLine[0] != 0) ? cmdLine : NULL, 0);
   if (nCount > 0)
   {
      for(i = 0, qwValue = 0; i < nCount; i++)
         if (!GetProcessAttribute(pList[i].nPid, CAST_FROM_POINTER(arg, int), nType, i, &qwValue))
            break;
      if (i == nCount)
      {
         ret_uint64(value, qwValue);
         nRet = SYSINFO_RC_SUCCESS;
      }
   }
   safe_free(pList);

   return nRet;
}
