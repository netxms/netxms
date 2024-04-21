/*
** NetXMS subagent for FreeBSD
** Copyright (C) 2004-2024 Alex Kirhenshtein, Victor Kirhenshtein
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

#undef _XOPEN_SOURCE

#if __FreeBSD__ < 8
#define _SYS_LOCK_PROFILE_H_ /* prevent include of sys/lock_profile.h which can be C++ incompatible) */
#endif

#include <nms_agent.h>
#include <nms_common.h>
#include <nms_util.h>

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/utsname.h>

#if __FreeBSD__ < 5
#include <sys/proc.h>
#endif

#include <kvm.h>
#include <sys/user.h>

#include "freebsd_subagent.h"

#ifndef KERN_PROC_PROC
#define KERN_PROC_PROC KERN_PROC_ALL
#endif

/**
 * Maximum possible length of process name and user name
 */
#define MAX_PROCESS_NAME_LEN (COMMLEN + 1)
#define MAX_USER_NAME_LEN (LOGNAMELEN + 1)
#define MAX_CMD_LINE_LEN ARG_MAX

/**
 * Build process command line
 */
static bool ReadProcCmdLine(const pid_t pid, char *buff)
{
   bool result = false;
   char fullFileName[MAX_PATH];
   snprintf(fullFileName, MAX_PATH, "/proc/%i/cmdline", static_cast<int>(pid));
   int hFile = _open(fullFileName, O_RDONLY);
   if (hFile != -1)
   {
      result = _read(hFile, buff, MAX_CMD_LINE_LEN) > 0;
      _close(hFile);
   }
   return result;
}

/**
 * Check if given process matches filter
 */
static bool MatchProcess(struct kinfo_proc *p, bool extMatch, const char *nameFilter, const char *cmdLineFilter,
                         const char *userNameFilter)
{
   if (extMatch)
   {
      // Proc name filter
      if (nameFilter != nullptr && *nameFilter != 0)
         if (!RegexpMatchA(p->ki_comm, nameFilter, false))
            return false;

      // User filter
      if (userNameFilter != nullptr && *userNameFilter != 0)
      {
         if (!RegexpMatchA(p->ki_login, userNameFilter, false))
            return false;
      }
      // Cmd line filter
      if (cmdLineFilter != nullptr && *cmdLineFilter != 0)
      {
         char cmdLine[MAX_CMD_LINE_LEN];
         if (!ReadProcCmdLine(p->ki_pid, cmdLine) || !RegexpMatchA(cmdLine, cmdLineFilter, true))
            return false;
      }
      return true;
   }
   else
   {
      return strcasecmp(p->ki_comm, nameFilter) == 0;
   }
}

/**
 * Handler for Process.Count, Process.CountEx, System.ProcessCount and System.ThreadCount parameters
 */
LONG H_ProcessCount(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   int ret = SYSINFO_RC_ERROR;
   char name[128] = "", cmdLine[128] = "", userName[128] = "";
   int procCount;
   int result = -1;

   if ((*arg != 'S') && (*arg != 'T')) // Not System.ProcessCount nor System.ThreadCount
   {
      AgentGetParameterArgA(param, 1, name, sizeof(name));
      if (*arg == 'E') // Process.CountEx
      {
         AgentGetParameterArgA(param, 2, cmdLine, sizeof(cmdLine));
         AgentGetParameterArgA(param, 3, userName, sizeof(userName));
      }
   }
   kvm_t *kd = kvm_openfiles(NULL, "/dev/null", NULL, O_RDONLY, NULL);
   if (kd != nullptr)
   {
      kinfo_proc *kp = kvm_getprocs(kd, KERN_PROC_PROC, 0, &procCount);
      if (kp != nullptr)
      {
         if (*arg != 'S') // Not System.ProcessCount
         {
            result = 0;
            if (*arg == 'T') // System.ThreadCount
            {
               for (int i = 0; i < procCount; i++)
               {
                  result += kp[i].ki_numthreads;
               }
            }
            else // Process.CountEx
            {
               for (int i = 0; i < procCount; i++)
               {
                  if (MatchProcess(&kp[i], *arg == 'E', name, cmdLine, userName))
                  {
                     result++;
                  }
               }
            }
         }
         else
         {
            result = procCount;
         }
      }
      if (result >= 0)
      {
         ret_int(value, result);
         ret = SYSINFO_RC_SUCCESS;
      }
      kvm_close(kd);
   }
   return ret;
}

/**
 * Handler for Process.* parameters
 */
LONG H_ProcessInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   int ret = SYSINFO_RC_ERROR;
   char name[128] = "", cmdLine[128] = "", userName[128] = "", buffer[64] = "";
   int procCount, matched;
   INT64 currValue, result;
   int i, type;
   static const char *typeList[] = { "min", "max", "avg", "sum", NULL };

   // Get parameter type arguments
   AgentGetParameterArgA(param, 2, buffer, sizeof(buffer));
   if (buffer[0] == 0) // Omited type
   {
      type = INFOTYPE_SUM;
   }
   else
   {
      for (type = 0; typeList[type] != NULL; type++)
         if (!stricmp(typeList[type], buffer))
            break;
      if (typeList[type] == NULL)
         return SYSINFO_RC_UNSUPPORTED; // Unsupported type
   }

   AgentGetParameterArgA(param, 1, name, sizeof(name));
   AgentGetParameterArgA(param, 3, cmdLine, sizeof(cmdLine));
   AgentGetParameterArgA(param, 4, userName, sizeof(userName));

   kvm_t *kd = kvm_openfiles(NULL, "/dev/null", NULL, O_RDONLY, NULL);
   if (kd != nullptr)
   {
      kinfo_proc *kp = kvm_getprocs(kd, KERN_PROC_PROC, 0, &procCount);
      if (kp != nullptr)
      {
         result = 0;
         matched = 0;
         for (i = 0; i < procCount; i++)
         {
            if (MatchProcess(&kp[i], *cmdLine != 0, name, cmdLine, userName))
            {
               matched++;
               switch (CAST_FROM_POINTER(arg, int))
               {
                  case PROCINFO_CPUTIME:
                     currValue = kp[i].ki_runtime / 1000; // microsec -> millisec
                     break;
                  case PROCINFO_THREADS:
                     currValue = kp[i].ki_numthreads;
                     break;
                  case PROCINFO_VMSIZE:
                     currValue = kp[i].ki_size;
                     break;
                  case PROCINFO_WKSET:
                     currValue = kp[i].ki_rssize * getpagesize();
                     break;
               }

               switch (type)
               {
                  case INFOTYPE_SUM:
                  case INFOTYPE_AVG:
                     result += currValue;
                     break;
                  case INFOTYPE_MIN:
                     result = std::min(result, currValue);
                     break;
                  case INFOTYPE_MAX:
                     result = std::max(result, currValue);
                     break;
               }
            }
         }
         if ((type == INFOTYPE_AVG) && (matched > 0))
            result /= matched;
         ret_int64(value, result);
         ret = SYSINFO_RC_SUCCESS;
      }
      kvm_close(kd);
   }
   return ret;
}

/**
 * Handler for System.ProcessList enum
 */
LONG H_ProcessList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   int ret = SYSINFO_RC_ERROR;
   int procCount = -1;
   int i;

   kvm_t *kd = kvm_openfiles(NULL, "/dev/null", NULL, O_RDONLY, NULL);
   if (kd != nullptr)
   {
      kinfo_proc *kp = kvm_getprocs(kd, KERN_PROC_PROC, 0, &procCount);
      if (kp != nullptr)
      {
         for (i = 0; i < procCount; i++)
         {
            char szBuff[128];

            snprintf(szBuff, sizeof(szBuff), "%d %s", kp[i].ki_pid, kp[i].ki_comm);
#ifdef UNICODE
            value->addPreallocated(WideStringFromMBString(szBuff));
#else
            value->add(szBuff);
#endif
         }
      }
      kvm_close(kd);
      if (procCount >= 0)
      {
         ret = SYSINFO_RC_SUCCESS;
      }
   }
   return ret;
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
   value->addColumn(_T("PAGE_FAULTS"), DCI_DT_UINT64, _T("Page Faults"));
   value->addColumn(_T("CMDLINE"), DCI_DT_STRING, _T("Command Line"));

   int rc = SYSINFO_RC_ERROR;

   kvm_t *kd = kvm_openfiles(NULL, "/dev/null", NULL, O_RDONLY, NULL);
   if (kd != nullptr)
   {
      int procCount;
      kinfo_proc *procs = kvm_getprocs(kd, KERN_PROC_PROC, 0, &procCount);
      if (procs != nullptr)
      {

         rc = SYSINFO_RC_SUCCESS;
         for (int i = 0; i < procCount; i++)
         {
            value->addRow();
            value->set(0, procs[i].ki_pid);
#ifdef UNICODE
            value->setPreallocated(1, WideStringFromMBString(procs[i].ki_comm));
            value->setPreallocated(2, WideStringFromMBString(procs[i].ki_login));
#else
            value->set(1, procs[i].ki_comm);
            value->set(2, procs[i].ki_login);
#endif
            value->set(3, procs[i].ki_numthreads);
            // value->set(4, p->fd);
            // tv_sec are seconds, tv_usec are microseconds => converting both to milliseconds
            value->set(5, procs[i].ki_rusage.ru_stime.tv_sec * 1000 + procs[i].ki_rusage.ru_stime.tv_usec / 1000);
            value->set(6, procs[i].ki_rusage.ru_utime.tv_sec * 1000 + procs[i].ki_rusage.ru_utime.tv_usec / 1000);
            value->set(7, procs[i].ki_size);
            value->set(8, procs[i].ki_rssize * 1024);
            value->set(9, procs[i].ki_rusage.ru_minflt + procs[i].ki_rusage.ru_majflt);
            char cmdLine[MAX_CMD_LINE_LEN];
            if (ReadProcCmdLine(procs[i].ki_pid, cmdLine))
               value->set(10, cmdLine);
         }
      }
      kvm_close(kd);
   }
   return rc;
}
