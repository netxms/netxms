/*
** NetXMS subagent for SunOS/Solaris
** Copyright (C) 2004-2024 Victor Kirhenshtein
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
#include <pwd.h>
#include <sys/proc.h>

/**
 * Constants
 */
#define INFOTYPE_MIN 0
#define INFOTYPE_MAX 1
#define INFOTYPE_AVG 2
#define INFOTYPE_SUM 3

/**
 * Maximum possible length of process name and user name
 */
#define MAX_PROCESS_NAME_LEN PRFNSZ
#define MAX_USER_NAME_LEN 256
#define MAX_CMD_LINE_LEN PRARGSZ

/**
 * Process list entry structure
 */
struct t_ProcEnt
{
   pid_t pid;
   char name[PRFNSZ];
};

/**
 * Reads process psinfo file from /proc
 * T = psinfo_t OR pstatus_t OR prusage_t
 * fileName = psinfo OR status OR usage
 */
template <typename T>
static bool ReadProcInfo(const char *fileName, const char *pidAsChar, T *buff)
{
   bool result = false;
   char fullFileName[MAX_PATH];
   snprintf(fullFileName, MAX_PATH, "/proc/%s/%s", pidAsChar, fileName);
   int hFile = _open(fullFileName, O_RDONLY);
   if (hFile != -1)
   {
      result = _read(hFile, buff, sizeof(T)) == sizeof(T);
      _close(hFile);
   }
   return result;
}

/**
 * Reads process user name.
 */
static bool ReadProcUser(uid_t uid, char *userName)
{
   passwd resultbuf;
   char buffer[512];
   passwd *userInfo;
   getpwuid_r(uid, &resultbuf, buffer, sizeof(buffer), &userInfo);
   if (userInfo != nullptr)
   {
      strlcpy(userName, userInfo->pw_name, MAX_USER_NAME_LEN);
      return true;
   }
   return false;
}

/**
 * Count handles
 */
static uint32_t CountProcessHandles(pid_t pid)
{
   char path[MAX_PATH];
   snprintf(path, MAX_PATH, "/proc/%d/fd", static_cast<int>(pid));
   int result = CountFilesInDirectoryA(path);
   if (result < 0)
      return 0;
   return result;
}

/**
 * Filter for reading only pid directories from /proc
 */
static int ProcFilter(const struct dirent *d)
{
   const char *c = d->d_name;
   while (*c != 0)
   {
      if (!isdigit(*c))
         return 0; // skip
      c++;
   }
   return 1;
}

/**
 * Read process information from /proc system
 *
 * @param pEnt - If not NULL, ProcRead() will return pointer to dynamically
 *        allocated array of process information structures for
 *        matched processes. Caller should free it with MemFree().
 * @param extended - If FALSE, only process name filter will be used and will require exact
 *            match insted of regular expression.
 * @param procNameFilter - If not NULL, only processes with name matched to
 *                  regular expression will be counted and read.
 * @param cmdLineFilter - If not NULL, only processes with command line matched to
 *                 regular expression will be counted and read.
 * @param userNameFilter - If not NULL, only processes with user name matched to
 *                  regular expression will be counted and read.
 * @param state - If non-zero, only processes with pstatus_t.pr_lwp.pr_state
 *         (as defined in proc.h: SSLEEP, SRUN, SZOMB, SSTOP,
 *         SIDL, SONPROC) will be counted
 * @return number of matched processes or -1 in case of error.
 */
static int ProcRead(t_ProcEnt **pEnt, bool extended, char *procNameFilter, char *cmdLineFilter, char *userNameFilter, int state)
{
   int procFound = -1;
   if (pEnt != nullptr)
      *pEnt = nullptr;

   struct dirent **procList;
   int procCount = scandir("/proc", &procList, ProcFilter, alphasort);
   if (procCount >= 0)
   {
      procFound = 0;

      if (procCount > 0 && pEnt != nullptr)
      {
         *pEnt = MemAllocArray<t_ProcEnt>(procCount + 1);
         if (*pEnt == nullptr)
         {
            procFound = -1;
            // cleanup
            while (procCount--)
            {
               free(procList[procCount]);
            }
         }
      }

      while (procCount--)
      {
         psinfo_t psi;
         if (ReadProcInfo<psinfo_t>("psinfo", procList[procCount]->d_name, &psi))
         {
            bool nameMatch = true, cmdLineMatch = true, userMatch = true, stateMatch = true;

            if (extended)
            {
               // Match process name
               if ((procNameFilter != nullptr) && (*procNameFilter != 0))
               {
                  if (!RegexpMatchA(psi.pr_fname, procNameFilter, true))
                     continue;
               }
               // Match cmd line
               if ((cmdLineFilter != nullptr) && (*cmdLineFilter != 0))
               {
                  if (!RegexpMatchA(psi.pr_psargs, cmdLineFilter, true))
                     continue;
               }
               // Match user name
               if ((userNameFilter != nullptr) && (*userNameFilter != 0))
               {
                  char userName[MAX_USER_NAME_LEN];
                  if (!ReadProcUser(psi.pr_uid, userName) || !RegexpMatchA(userName, userNameFilter, true))
                  {
                        continue;
                  }
               }
            }
            else
            {
               // Match process name
               if ((procNameFilter != nullptr) && (*procNameFilter != 0))
               {
                  if (strcmp(psi.pr_fname, procNameFilter))
                     continue;
               }
            }

            // Match state
            if (state != 0)
            {
               if (psi.pr_lwp.pr_state != state)
                  continue;
            }

            if (pEnt != nullptr)
            {
               (*pEnt)[procFound].pid = strtoul(procList[procCount]->d_name, nullptr, 10);
               strncpy((*pEnt)[procFound].name, psi.pr_fname, sizeof((*pEnt)[procFound].name));
               (*pEnt)[procFound].name[sizeof((*pEnt)[procFound].name) - 1] = 0;
            }
            procFound++;
         }
         free(procList[procCount]);
      }
      free(procList);
   }

   if ((procFound < 0) && (pEnt != nullptr))
   {
      MemFree(*pEnt);
      *pEnt = nullptr;
   }
   return procFound;
}

/**
 * Handler for System.ProcessList and System.Processes lists
 */
LONG H_ProcessList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   t_ProcEnt *pList;
   int nProc = ProcRead(&pList, false, nullptr, nullptr, nullptr, 0);
   if (nProc == -1)
      return SYSINFO_RC_ERROR;

   bool useCmdLine = (*arg == '2');
   char buffer[4096];
   for (int i = 0; i < nProc; i++)
   {
      if (useCmdLine)
      {
         char pid[16];
         IntegerToString(pList[i].pid, pid);
         psinfo_t pi;
         if (ReadProcInfo<psinfo_t>("psinfo", pid, &pi) && (pi.pr_psargs[0] != 0))
         {
            snprintf(buffer, sizeof(buffer), "%d %s", pList[i].pid, pi.pr_psargs);
         }
         else
         {
            snprintf(buffer, sizeof(buffer), "%d %s", pList[i].pid, pList[i].name);
         }
      }
      else
      {
         snprintf(buffer, sizeof(buffer), "%d %s", pList[i].pid, pList[i].name);
      }
      value->addMBString(buffer);
   }
   MemFree(pList);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Process.Count(*) and Process.CountEx(*) parameters
 */
LONG H_ProcessCount(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   int nRet = SYSINFO_RC_ERROR;
   int nCount;

   char procName[256] = "";
   AgentGetParameterArgA(param, 1, procName, sizeof(procName));

   if (*arg == _T('Z')) // Process.ZombieCount
   {
      nCount = ProcRead(nullptr, false, procName, nullptr, nullptr, SZOMB);
   }
   else
   {
      char cmdLine[256] = "", userName[256] = "";
      if (*arg == _T('E')) // Process.CountEx()
      {
         AgentGetParameterArgA(param, 2, cmdLine, sizeof(cmdLine));
         AgentGetParameterArgA(param, 3, userName, sizeof(userName));
         nCount = ProcRead(nullptr, true, procName, cmdLine, userName, 0);
      }
      else // Process.Count()
      {
         nCount = ProcRead(nullptr, false, procName, nullptr, nullptr, 0);
      }
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
LONG H_SysProcCount(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   return ReadKStatValue("unix", 0, "system_misc", "nproc", pValue, NULL);
}

/**
 * Get specific process attribute
 */
static bool GetProcessAttribute(pid_t nPid, int nAttr, int nType, int nCount, uint64_t *pvalue)
{
   uint64_t value = 0;

   char szFileName[MAX_PATH];
   prusage_t usage;
   pstatus_t status;
   psinfo_t info;
   uint64_t totalMemory = 0;
   bool success = true;

   char pidAsChar[10];
   IntegerToString(static_cast<int32_t>(nPid), pidAsChar);

   // Get value for current process instance
   switch (nAttr)
   {
      case PROCINFO_VMSIZE:
         if (ReadProcInfo<psinfo_t>("psinfo", pidAsChar, &info))
         {
            value = info.pr_size * 1024;
         }
         else
         {
            success = false;
         }
         break;
      case PROCINFO_RSS:
         if (ReadProcInfo<psinfo_t>("psinfo", pidAsChar, &info))
         {
            value = info.pr_rssize * 1024;
         }
         else
         {
            success = false;
         }
         break;
      case PROCINFO_MEMPERC:
         if (ReadProcInfo<psinfo_t>("psinfo", pidAsChar, &info))
         {
            if (totalMemory == 0)
               totalMemory = static_cast<uint64_t>(sysconf(_SC_PHYS_PAGES)) * sysconf(_SC_PAGESIZE) / 1024;
            value = static_cast<uint64_t>(info.pr_rssize) * 10000 / totalMemory;
         }
         else
         {
            success = false;
         }
         break;
      case PROCINFO_PF:
         if (ReadProcInfo<prusage_t>("usage", pidAsChar, &usage))
         {
            value = usage.pr_minf + usage.pr_majf;
         }
         else
         {
            success = false;
         }
         break;
      case PROCINFO_SYSCALLS:
         if (ReadProcInfo<prusage_t>("usage", pidAsChar, &usage))
         {
            value = usage.pr_sysc;
         }
         else
         {
            success = false;
         }
         break;
      case PROCINFO_THREADS:
         if (ReadProcInfo<pstatus_t>("status", pidAsChar, &status))
         {
            value = status.pr_nlwp;
         }
         else
         {
            success = false;
         }
         break;
      case PROCINFO_KTIME:
         if (ReadProcInfo<pstatus_t>("status", pidAsChar, &status))
         {
            value = status.pr_stime.tv_sec * 1000 + status.pr_stime.tv_nsec / 1000000;
         }
         else
         {
            success = false;
         }
         break;
      case PROCINFO_UTIME:
         if (ReadProcInfo<pstatus_t>("status", pidAsChar, &status))
         {
            value = status.pr_utime.tv_sec * 1000 + status.pr_utime.tv_nsec / 1000000;
         }
         else
         {
            success = false;
         }
         break;
      case PROCINFO_CPUTIME:
         if (ReadProcInfo<pstatus_t>("status", pidAsChar, &status))
         {
            value = status.pr_utime.tv_sec * 1000 + status.pr_utime.tv_nsec / 1000000 +
                      status.pr_stime.tv_sec * 1000 + status.pr_stime.tv_nsec / 1000000;
         }
         else
         {
            success = FALSE;
         }
         break;
      case PROCINFO_HANDLES:
         value = CountProcessHandles(nPid);
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
         success = false;
         break;
   }

   // Recalculate final value according to selected type
   if (nCount == 0) // First instance
   {
      *pvalue = value;
   }
   else
   {
      switch (nType)
      {
         case INFOTYPE_MIN:
            *pvalue = std::min(*pvalue, value);
            break;
         case INFOTYPE_MAX:
            *pvalue = std::max(*pvalue, value);
            break;
         case INFOTYPE_AVG:
            *pvalue = (*pvalue * nCount + value) / (nCount + 1);
            break;
         case INFOTYPE_SUM:
            *pvalue = *pvalue + value;
            break;
         default:
            success = false;
            break;
      }
   }
   return success;
}

/**
 * Handler for Process.XXX parameters
 */
LONG H_ProcessInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   int nRet = SYSINFO_RC_ERROR;
   char buffer[256] = "", procName[256] = "", cmdLine[256] = "", userName[256] = "";
   int i, nCount, nType;
   t_ProcEnt *pList;
   uint64_t qwValue;

   // Get parameter type arguments
   AgentGetParameterArgA(param, 2, buffer, sizeof(buffer));
   if (buffer[0] == 0) // Omited type
   {
      nType = INFOTYPE_SUM;
   }
   else
   {
      static const char *typeList[] = { "min", "max", "avg", "sum", nullptr };
      for (nType = 0; typeList[nType] != nullptr; nType++)
         if (!stricmp(typeList[nType], buffer))
            break;
      if (typeList[nType] == nullptr)
         return SYSINFO_RC_UNSUPPORTED; // Unsupported type
   }

   // Get process name
   AgentGetParameterArgA(param, 1, procName, MAX_PATH);
   AgentGetParameterArgA(param, 3, cmdLine, MAX_PATH);
   AgentGetParameterArgA(param, 4, userName, MAX_PATH);
   TrimA(cmdLine);

   nCount = ProcRead(&pList, true, procName, cmdLine, userName, 0);
   if (nCount > 0)
   {
      for (i = 0, qwValue = 0; i < nCount; i++)
         if (!GetProcessAttribute(pList[i].pid, CAST_FROM_POINTER(arg, int), nType, i, &qwValue))
            break;
      if (i == nCount)
      {
         if (CAST_FROM_POINTER(arg, int) == PROCINFO_MEMPERC)
         {
            ret_double(value, static_cast<double>(qwValue) / 100, 2);
         }
         else
         {
            ret_uint64(value, qwValue);
         }
         nRet = SYSINFO_RC_SUCCESS;
      }
   }
   MemFree(pList);

   return nRet;
}

/**
 * Handler for System.Processes table
 */
LONG H_ProcessTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   value->addColumn(_T("PID"), DCI_DT_UINT, _T("PID"), true);
   value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"));
   value->addColumn(_T("USER"), DCI_DT_STRING, _T("User"));
   value->addColumn(_T("THREADS"), DCI_DT_UINT, _T("Threads"));
   value->addColumn(_T("HANDLES"), DCI_DT_UINT, _T("Handles"));
   value->addColumn(_T("KTIME"), DCI_DT_UINT64, _T("Kernel Time"));
   value->addColumn(_T("UTIME"), DCI_DT_UINT64, _T("User Time"));
   value->addColumn(_T("VMSIZE"), DCI_DT_UINT64, _T("VM Size"));
   value->addColumn(_T("RSS"), DCI_DT_UINT64, _T("RSS"));
   value->addColumn(_T("MEMORY_USAGE"), DCI_DT_FLOAT, _T("Memory Usage"));
   value->addColumn(_T("PAGE_FAULTS"), DCI_DT_UINT64, _T("Page Faults"));
   value->addColumn(_T("CMDLINE"), DCI_DT_STRING, _T("Command Line"));

   int rc = SYSINFO_RC_ERROR;

   t_ProcEnt *procs;
   int procCount = ProcRead(&procs, false, nullptr, nullptr, nullptr, 0);
   if (procs != nullptr)
   {
      rc = SYSINFO_RC_SUCCESS;
      uint64_t totalMemory = static_cast<uint64_t>(sysconf(_SC_PHYS_PAGES)) * sysconf(_SC_PAGESIZE) / 1024;
      for (int i = 0; i < procCount; i++)
      {
         value->addRow();
         value->set(0, procs[i].pid);

         char pidAsChar[10];
         IntegerToString(procs[i].pid, pidAsChar);

         char user[MAX_USER_NAME_LEN];
         psinfo_t pi;
         if (ReadProcInfo<psinfo_t>("psinfo", pidAsChar, &pi))
         {
            if (!ReadProcUser(pi.pr_uid, user))
               *user = 0;
            value->set(7, pi.pr_size * 1024);
            value->set(8, pi.pr_rssize * 1024);
            value->set(9, static_cast<double>(static_cast<uint64_t>(pi.pr_rssize) * 10000 / totalMemory) / 100, 2);
            value->set(11, pi.pr_psargs);
         }
         else
         {
            *user = 0;
            value->set(7, 0);
            value->set(8, 0);
            value->set(9, _T("0.00"));
            value->set(11, _T(""));
         }

#ifdef UNICODE
         value->setPreallocated(1, WideStringFromMBString(procs[i].name));
         value->setPreallocated(2, WideStringFromMBString(user));
#else
         value->set(1, procs[i].name);
         value->set(2, user);
#endif
         value->set(4, CountProcessHandles(procs[i].pid));

         pstatus_t ps;
         if (ReadProcInfo<pstatus_t>("status", pidAsChar, &ps))
         {
            value->set(3, ps.pr_nlwp);
            // tv_sec are seconds, tv_nsec are nanoseconds => converting both to milliseconds
            value->set(5, (ps.pr_stime.tv_sec * 1000 + ps.pr_stime.tv_nsec / 1000000));
            value->set(6, (ps.pr_utime.tv_sec * 1000 + ps.pr_utime.tv_nsec / 1000000));
         }
         else
         {
            value->set(3, 0);
            value->set(5, 0);
            value->set(6, 0);
         }

         prusage_t pu;
         if (ReadProcInfo<prusage_t>("usage", pidAsChar, &pu))
         {
            value->set(10, pu.pr_majf + pu.pr_minf);
         }
         else
         {
            value->set(10, 0);
         }
      }
      MemFree(procs);
   }
   return rc;
}

/**
 * Handler for System.HandleCount parameter
 */
LONG H_HandleCount(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   int ret = SYSINFO_RC_ERROR;
   t_ProcEnt *procs;
   int procCount = ProcRead(&procs, false, nullptr, nullptr, nullptr, 0);
   if (procs != nullptr)
   {
      int sum = 0;
      for (int i = 0; i < procCount; i++)
      {
         sum += CountProcessHandles(procs[i].pid);
      }
      ret_int(value, sum);
      ret = SYSINFO_RC_SUCCESS;
   }
   return ret;
}
