/*
** NetXMS subagent for AIX
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
** File: proc.cpp
**
**/

#define _H_XCOFF

#include "aix_subagent.h"
#include <procinfo.h>
#include <sys/vminfo.h>
#include <sys/resource.h>
#include <sys/time.h>

//
// Possible consolidation types of per-process information
//

#define INFOTYPE_MIN 0
#define INFOTYPE_MAX 1
#define INFOTYPE_AVG 2
#define INFOTYPE_SUM 3

//
// Select 32-bit or 64-bit interface
// At least at AIX 5.1 getprocs() and getprocs64() are not defined, so we
// define them here
//

#if HAVE_GETPROCS64
typedef struct procentry64 PROCENTRY;
#define GETPROCS getprocs64
#if !HAVE_DECL_GETPROCS64
extern "C" int getprocs64(struct procentry64 *, int, struct fdsinfo64 *, int, pid_t *, int);
#endif
#else
typedef struct procsinfo PROCENTRY;
#define GETPROCS getprocs
#if !HAVE_DECL_GETPROCS
extern "C" int getprocs(struct procsinfo *, int, struct fdsinfo *, int, pid_t *, int);
#endif
#endif

/**
 * Maximum possible length of process name and user name
 */
#define MAX_PROCESS_NAME_LEN (MAXCOMLEN + 1)
#define MAX_USER_NAME_LEN 256
#define MAX_CMD_LINE_LEN PRARGSZ

/**
 * Reads process psinfo file from /proc
 * T = psinfo OR pstatus
 * fileName = psinfo OR status
 */
template <typename T>
static bool ReadProcInfo(const char *fileName, pid_t pid, T *buff)
{
   bool result = false;
   char fullFileName[MAX_PATH];
   snprintf(fullFileName, MAX_PATH, "/proc/%i/%s", static_cast<int>(pid), fileName);
   int hFile = _open(fullFileName, O_RDONLY);
   if (hFile != -1)
   {
      result = _read(hFile, buff, sizeof(T)) == sizeof(T);
      _close(hFile);
   }
   return result;
}

/**
 * Reads initial characters of arg list (cmd line) of process.
 * Buffer must be MAX_CMD_LINE_LEN bytes long.
 */
static bool ReadProcCmdLine(pid_t pid, char *buff)
{
   psinfo pi;
   if (ReadProcInfo<psinfo>("psinfo", pid, &pi))
   {
      strcpy(buff, pi.pr_psargs);
      return true;
   }
   return false;
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
 * Read handles
 */
static uint32_t CountProcessHandles(uint32_t pid)
{
   char path[MAX_PATH];
   snprintf(path, MAX_PATH, "/proc/%u/fd", pid);
   int result = CountFilesInDirectoryA(path);
   if (result < 0)
      return 0;
   return result;
}

/**
 * Get process list and number of processes
 */
static PROCENTRY *GetProcessList(int *pnprocs)
{
   pid_t index = 0;
   int nprocs = GETPROCS(nullptr, 0, nullptr, 0, &index, 65535);
   if (nprocs == -1)
      return nullptr;

   PROCENTRY *pBuffer;
   int nrecs = nprocs + 64;

retry_getprocs:
   pBuffer = MemAllocArray<PROCENTRY>(nrecs);
   index = 0;
   nprocs = GETPROCS(pBuffer, sizeof(PROCENTRY), nullptr, 0, &index, nrecs);
   if (nprocs != -1)
   {
      if (nprocs == nrecs)
      {
         free(pBuffer);
         nrecs += 1024;
         goto retry_getprocs;
      }
      *pnprocs = nprocs;
   }
   else
   {
      MemFreeAndNull(pBuffer);
   }
   return pBuffer;
}

/**
 * Handler for lists System.Processes and System.ProcessList
 */
LONG H_ProcessList(const TCHAR *metric, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   int procCount;
   PROCENTRY *plist = GetProcessList(&procCount);
   if (plist == nullptr)
      return SYSINFO_RC_ERROR;

   bool useCmdLine = (*arg == '2');
   for (int i = 0; i < procCount; i++)
   {
      if (useCmdLine)
      {
         char cmdLine[MAX_CMD_LINE_LEN + 16];
         IntegerToString(plist[i].pi_pid, cmdLine);
         size_t pos = strlen(cmdLine);
         cmdLine[pos++] = ' ';
         if (!ReadProcCmdLine(plist[i].pi_pid, &cmdLine[pos]) || (cmdLine[pos] == 0))
         {
            strcpy(&cmdLine[pos], plist[i].pi_comm);
         }
         value->addMBString(cmdLine);
      }
      else
      {
         char buffer[256];
         snprintf(buffer, 256, "%d %s", plist[i].pi_pid, plist[i].pi_comm);
         value->addMBString(buffer);
      }
   }
   MemFree(plist);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.ProcessCount parameter
 */
LONG H_SysProcessCount(const TCHAR *metric, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   int nprocs;
   pid_t index = 0;

   nprocs = GETPROCS(NULL, 0, NULL, 0, &index, 65535);
   if (nprocs == -1)
      return SYSINFO_RC_ERROR;

   ret_uint(pValue, nprocs);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.ThreadCount parameter
 */
LONG H_SysThreadCount(const TCHAR *metric, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   LONG nRet;
   int nProcCount;
   PROCENTRY *pList = GetProcessList(&nProcCount);
   if (pList != NULL)
   {
      int nThreadCount = 0;
      for (int i = 0; i < nProcCount; i++)
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

/**
 * Check if given process matches filter
 */
static bool MatchProcess(PROCENTRY *proc, const char *processNameFilter, const char *cmdLineFilter, const char *userNameFilter)
{
   // Proc name filter
   if (processNameFilter != nullptr && *processNameFilter != 0)
   {
      if (!RegexpMatchA(proc->pi_comm, processNameFilter, true))
         return false;
   }
   // User filter
   if (userNameFilter != nullptr && *userNameFilter != 0)
   {
      char userName[MAX_USER_NAME_LEN];
      if (!ReadProcUser(proc->pi_uid, userName) || !RegexpMatchA(userName, userNameFilter, true))
         return false;
   }
   // Cmd line filter
   if (cmdLineFilter != nullptr && *cmdLineFilter != 0)
   {
      char cmdLine[MAX_CMD_LINE_LEN];
      if (!ReadProcCmdLine(proc->pi_pid, cmdLine) || !RegexpMatchA(cmdLine, cmdLineFilter, true))
         return false;
   }
   return true;
}

/**
 * Handler for Process.Count(*), Process.CountEx(*) parameters
 */
LONG H_ProcessCount(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   char processNameFilter[256] = "", cmdLineFilter[256] = "", userNameFilter[256] = "";
   if (!AgentGetParameterArgA(param, 1, processNameFilter, sizeof(processNameFilter)))
      return SYSINFO_RC_UNSUPPORTED;

   bool extended = (*arg == _T('E'));
   if (extended)
   {
      AgentGetParameterArgA(param, 2, cmdLineFilter, sizeof(cmdLineFilter));
      AgentGetParameterArgA(param, 3, userNameFilter, sizeof(userNameFilter));
   }

   int sysProcCount;
   PROCENTRY *pList = GetProcessList(&sysProcCount);
   if (pList == nullptr)
      return SYSINFO_RC_ERROR;

   if (!extended && (*processNameFilter == 0))
   {
      MemFree(pList);
      ret_uint(value, sysProcCount);
      return SYSINFO_RC_SUCCESS;
   }

   int procCount = 0;
   for (int i = 0; i < sysProcCount; i++)
   {
      if (extended) // Process.CountEx()
      {
         if (MatchProcess(&pList[i], processNameFilter, cmdLineFilter, userNameFilter))
            procCount++;
      }
      else // Process.Count()
      {
         if (!strcmp(pList[i].pi_comm, processNameFilter))
            procCount++;
      }
   }
   MemFree(pList);
   ret_uint(value, procCount);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Process.xxx(*) parameters
 */
LONG H_ProcessInfo(const TCHAR *metric, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   int nRet = SYSINFO_RC_SUCCESS;
   char szBuffer[256] = "";
   char processNameFilter[128] = "", cmdLineFilter[128] = "", userNameFilter[128] = "";

   // Get parameter type arguments
   int nType;
   AgentGetParameterArgA(metric, 2, szBuffer, sizeof(szBuffer));
   if (szBuffer[0] == 0) // Omited type
   {
      nType = INFOTYPE_SUM;
   }
   else
   {
      static const char *typeList[] = { "min", "max", "avg", "sum", nullptr };
      for (nType = 0; typeList[nType] != nullptr; nType++)
         if (!stricmp(typeList[nType], szBuffer))
            break;
      if (typeList[nType] == nullptr)
         return SYSINFO_RC_UNSUPPORTED; // Unsupported type
   }

   AgentGetParameterArgA(metric, 1, processNameFilter, sizeof(processNameFilter));
   AgentGetParameterArgA(metric, 3, cmdLineFilter, sizeof(cmdLineFilter));
   AgentGetParameterArgA(metric, 4, userNameFilter, sizeof(userNameFilter));

   int nProcCount;
   PROCENTRY *pList = GetProcessList(&nProcCount);
   if (pList == nullptr)
      return SYSINFO_RC_ERROR;

   uint64_t totalMemory = 0;
   uint64_t totalValue = 0;
   int nCount = 0;
   for (int i = 0; i < nProcCount; i++)
   {
      if (MatchProcess(&pList[i], processNameFilter, cmdLineFilter, userNameFilter))
      {
         uint64_t currValue;
         switch (CAST_FROM_POINTER(arg, int))
         {
            case PROCINFO_CPUTIME:
               // tv_usec contains nanoseconds, not microseconds
               currValue = pList[i].pi_ru.ru_stime.tv_sec * 1000 + pList[i].pi_ru.ru_stime.tv_usec / 1000000 +
                           pList[i].pi_ru.ru_utime.tv_sec * 1000 + pList[i].pi_ru.ru_utime.tv_usec / 1000000;
               break;
            case PROCINFO_HANDLES:
               currValue = CountProcessHandles(pList[i].pi_pid);
               break;
            case PROCINFO_IO_READ_OP:
               currValue = pList[i].pi_ru.ru_inblock;
               break;
            case PROCINFO_IO_WRITE_OP:
               currValue = pList[i].pi_ru.ru_oublock;
               break;
            case PROCINFO_KTIME:
               // tv_usec contains nanoseconds, not microseconds
               currValue = pList[i].pi_ru.ru_stime.tv_sec * 1000 + pList[i].pi_ru.ru_stime.tv_usec / 1000000;
               break;
            case PROCINFO_MEMPERC:
               if (totalMemory == 0)
               {
                  struct vminfo vmi;
                  if (vmgetinfo(&vmi, VMINFO, sizeof(struct vminfo)) == 0)
                  {
                     totalMemory = vmi.memsizepgs;
                  }
                  else
                  {
                     MemFree(pList);
                     return SYSINFO_RC_ERROR;
                  }
               }
               currValue = (pList[i].pi_drss + pList[i].pi_trss) * 10000 / totalMemory;
               break;
            case PROCINFO_PF:
               currValue = pList[i].pi_majflt + pList[i].pi_minflt;
               break;
            case PROCINFO_RSS:
               currValue = (pList[i].pi_drss + pList[i].pi_trss) * getpagesize();
               break;
            case PROCINFO_THREADS:
               currValue = pList[i].pi_thcount;
               break;
            case PROCINFO_UTIME:
               // tv_usec contains nanoseconds, not microseconds
               currValue = pList[i].pi_ru.ru_utime.tv_sec * 1000 + pList[i].pi_ru.ru_utime.tv_usec / 1000000;
               break;
            case PROCINFO_VMSIZE:
               currValue = pList[i].pi_size * getpagesize();
               break;
            default:
               MemFree(pList);
               return SYSINFO_RC_UNSUPPORTED;
         }

         if (nCount == 0)
         {
            totalValue = currValue;
         }
         else
         {
            switch (nType)
            {
               case INFOTYPE_MIN:
                  totalValue = std::min(totalValue, currValue);
                  break;
               case INFOTYPE_MAX:
                  totalValue = std::max(totalValue, currValue);
                  break;
               case INFOTYPE_AVG:
                  totalValue = (totalValue * nCount + currValue) / (nCount + 1);
                  break;
               case INFOTYPE_SUM:
                  totalValue += currValue;
                  break;
            }
         }
         nCount++;
      }
   }
   MemFree(pList);

   if (CAST_FROM_POINTER(arg, int) == PROCINFO_MEMPERC)
   {
      _sntprintf(value, MAX_RESULT_LENGTH, _T("%d.%02d"), static_cast<int>(totalValue) / 100, static_cast<int>(totalValue) % 100);
   }
   else
   {
      ret_uint64(value, totalValue);
   }
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

   struct vminfo vmi;
   if (vmgetinfo(&vmi, VMINFO, sizeof(struct vminfo)) != 0)
      return SYSINFO_RC_ERROR;
   uint64_t totalMemory = vmi.memsizepgs * getpagesize() / 1024;

   int rc = SYSINFO_RC_ERROR;

   int procCount;
   PROCENTRY *procs = GetProcessList(&procCount);
   if (procs != nullptr)
   {
      rc = SYSINFO_RC_SUCCESS;
      for (int i = 0; i < procCount; i++)
      {
         value->addRow();
         value->set(0, procs[i].pi_pid);
#ifdef UNICODE
         value->setPreallocated(1, WideStringFromMBString(procs[i].pi_comm));

         char userName[MAX_USER_NAME_LEN];
         if (!ReadProcUser(procs[i].pi_uid, userName))
            *userName = 0;
         value->setPreallocated(2, WideStringFromMBString(userName));
#else
         value->set(1, procs[i].pi_comm);
         char userName[MAX_USER_NAME_LEN];
         if (!ReadProcUser(procs[i].pi_uid, userName))
            *userName = 0;
         value->set(2, userName);
#endif
         value->set(4, CountProcessHandles(procs[i].pi_pid));

         pstatus ps;
         if (ReadProcInfo<pstatus>("status", procs[i].pi_pid, &ps))
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

         value->set(7, static_cast<uint32_t>(procs[i].pi_dvm * 1024));

         psinfo pi;
         if (ReadProcInfo<psinfo>("psinfo", procs[i].pi_pid, &pi))
         {
            value->set(8, pi.pr_rssize * 1024);
            value->set(9, static_cast<double>(pi.pr_rssize * 10000 / totalMemory) / 100, 2);
         }
         else
         {
            value->set(8, 0);
            value->set(9, _T("0.00"));
         }

         value->set(10, static_cast<uint32_t>(procs[i].pi_majflt + procs[i].pi_minflt));

         char cmdLine[MAX_CMD_LINE_LEN];
         if (ReadProcCmdLine(procs[i].pi_pid, cmdLine))
            value->set(11, cmdLine);
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
   int procCount, ret = SYSINFO_RC_ERROR;
   PROCENTRY *procs = GetProcessList(&procCount);
   if (procs != nullptr)
   {
      int sum = 0;
      for (int i = 0; i < procCount; i++)
      {
         sum += CountProcessHandles(procs[i].pi_pid);
      }
      ret_int(value, sum);
      ret = SYSINFO_RC_SUCCESS;
      MemFree(procs);
   }
   return ret;
}
