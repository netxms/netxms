/*
** NetXMS subagent for Darwin (macOS)
** Copyright (C) 2012-2026 Alex Kirhenshtein, Victor Kirhenshtein
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

#include <nms_common.h>
#include <nms_agent.h>

#include <sys/sysctl.h>
#include <sys/proc.h>
#include <libproc.h>
#include <pwd.h>

#include "darwin.h"
#include "system.h"

#define MAX_PROCESS_NAME_LEN (MAXCOMLEN + 1)
#define MAX_USER_NAME_LEN    256
#define MAX_CMD_LINE_LEN     ARG_MAX

/**
 * Get physical footprint of a process - the memory the system charges to it, which excludes
 * pages shared with other processes. Unlike the resident set size it can be summed across
 * processes without counting shared mappings once per process. This is the same value
 * Activity Monitor shows in its "Memory" column.
 */
static bool GetProcessPhysFootprint(pid_t pid, int64_t *footprint)
{
   struct rusage_info_v2 ri;
   if (proc_pid_rusage(pid, RUSAGE_INFO_V2, reinterpret_cast<rusage_info_t*>(&ri)) != 0)
      return false;
   *footprint = static_cast<int64_t>(ri.ri_phys_footprint);
   return true;
}

/**
 * Read process command line using sysctl(KERN_PROCARGS2)
 */
static bool ReadProcCmdLine(pid_t pid, char *cmdLine, size_t maxLen)
{
   int mib[3] = { CTL_KERN, KERN_PROCARGS2, pid };
   char *buffer = static_cast<char*>(MemAlloc(ARG_MAX));
   if (buffer == nullptr)
      return false;

   size_t size = ARG_MAX;
   if (sysctl(mib, 3, buffer, &size, nullptr, 0) != 0)
   {
      MemFree(buffer);
      return false;
   }

   if (size < sizeof(int))
   {
      MemFree(buffer);
      return false;
   }

   int argc;
   memcpy(&argc, buffer, sizeof(int));
   char *p = buffer + sizeof(int);
   char *end = buffer + size;

   // Skip executable path
   while (p < end && *p != 0)
      p++;
   // Skip padding nulls
   while (p < end && *p == 0)
      p++;

   // Reconstruct command line from argv[0..argc-1]
   size_t pos = 0;
   for (int i = 0; i < argc && p < end; i++)
   {
      size_t argLen = strnlen(p, end - p);
      if (i > 0 && pos < maxLen - 1)
         cmdLine[pos++] = ' ';
      size_t copyLen = std::min(argLen, maxLen - 1 - pos);
      memcpy(cmdLine + pos, p, copyLen);
      pos += copyLen;
      p += argLen + 1;
   }
   cmdLine[pos] = 0;

   MemFree(buffer);
   return pos > 0;
}

/**
 * Get username for a given UID
 */
static bool GetProcessUserName(uid_t uid, char *userName, size_t maxLen)
{
   struct passwd pw;
   struct passwd *result = nullptr;
   char buffer[1024];

   if (getpwuid_r(uid, &pw, buffer, sizeof(buffer), &result) == 0 && result != nullptr)
   {
      strlcpy(userName, pw.pw_name, maxLen);
      return true;
   }
   return false;
}

/**
 * Check if given process matches filter
 */
static bool MatchProcess(struct kinfo_proc *p, bool extMatch, const char *nameFilter, const char *cmdLineFilter,
                          const char *userNameFilter)
{
   if (extMatch)
   {
      if (nameFilter != nullptr && *nameFilter != 0)
         if (!RegexpMatchA(p->kp_proc.p_comm, nameFilter, false))
            return false;

      if (userNameFilter != nullptr && *userNameFilter != 0)
      {
         char userName[MAX_USER_NAME_LEN];
         if (!GetProcessUserName(p->kp_eproc.e_ucred.cr_uid, userName, sizeof(userName)) ||
             !RegexpMatchA(userName, userNameFilter, false))
            return false;
      }

      if (cmdLineFilter != nullptr && *cmdLineFilter != 0)
      {
         char *cmdLine = static_cast<char*>(MemAlloc(MAX_CMD_LINE_LEN));
         bool match = ReadProcCmdLine(p->kp_proc.p_pid, cmdLine, MAX_CMD_LINE_LEN) && RegexpMatchA(cmdLine, cmdLineFilter, true);
         MemFree(cmdLine);
         if (!match)
            return false;
      }
      return true;
   }
   else
   {
      return strcasecmp(p->kp_proc.p_comm, nameFilter) == 0;
   }
}

/**
 * Get all processes via sysctl
 * Caller must MemFree() the returned pointer
 */
static struct kinfo_proc *GetProcessList(int *count)
{
   int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };

   for (int retry = 0; retry < 3; retry++)
   {
      size_t size = 0;
      if (sysctl(mib, 4, nullptr, &size, nullptr, 0) != 0)
         return nullptr;

      // Add extra space for processes that may appear between calls
      size += size / 8;
      struct kinfo_proc *procs = static_cast<struct kinfo_proc*>(MemAlloc(size));
      if (procs == nullptr)
         return nullptr;

      if (sysctl(mib, 4, procs, &size, nullptr, 0) == 0)
      {
         *count = static_cast<int>(size / sizeof(struct kinfo_proc));
         return procs;
      }

      MemFree(procs);
      if (errno != ENOMEM)
         return nullptr;
   }
   return nullptr;
}

/**
 * Get total physical memory size in bytes
 */
static uint64_t GetTotalPhysicalMemory()
{
   int mib[2] = { CTL_HW, HW_MEMSIZE };
   uint64_t memSize = 0;
   size_t len = sizeof(memSize);
   if (sysctl(mib, 2, &memSize, &len, nullptr, 0) != 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("sysctl(HW_MEMSIZE) failed (%s)"), _tcserror(errno));
      return 0;
   }
   return memSize;
}

/**
 * Handler for Process.* parameters (Process.CPUTime, Process.RSS, Process.VMSize, etc.)
 */
LONG H_ProcessInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   char name[128] = "", cmdLine[128] = "", userName[128] = "", buffer[64] = "";
   static const char *typeList[] = { "min", "max", "avg", "sum", nullptr };

   AgentGetParameterArgA(param, 2, buffer, sizeof(buffer));
   int type;
   if (buffer[0] == 0)
   {
      type = INFOTYPE_SUM;
   }
   else
   {
      for (type = 0; typeList[type] != nullptr; type++)
         if (!stricmp(typeList[type], buffer))
            break;
      if (typeList[type] == nullptr)
         return SYSINFO_RC_UNSUPPORTED;
   }

   AgentGetParameterArgA(param, 1, name, sizeof(name));
   AgentGetParameterArgA(param, 3, cmdLine, sizeof(cmdLine));
   AgentGetParameterArgA(param, 4, userName, sizeof(userName));

   int procCount = 0;
   struct kinfo_proc *procs = GetProcessList(&procCount);
   if (procs == nullptr)
      return SYSINFO_RC_ERROR;

   uint64_t totalMemory = 0;
   int64_t result = 0;
   int matched = 0;

   for (int i = 0; i < procCount; i++)
   {
      if (!MatchProcess(&procs[i], *cmdLine != 0 || *userName != 0, name, cmdLine, userName))
         continue;

      struct proc_taskinfo ti;
      int bytes = proc_pidinfo(procs[i].kp_proc.p_pid, PROC_PIDTASKINFO, 0, &ti, sizeof(ti));
      if (bytes != static_cast<int>(sizeof(ti)))
         continue;  // Skip processes where task info is unavailable (likely exited mid-scan)

      int64_t currValue = 0;
      switch (CAST_FROM_POINTER(arg, int))
      {
         case PROCINFO_CPUTIME:
            currValue = static_cast<int64_t>((ti.pti_total_user + ti.pti_total_system) / 1000000);  // nanoseconds -> milliseconds
            break;
         case PROCINFO_MEMPERC:
            if (totalMemory == 0)
            {
               totalMemory = GetTotalPhysicalMemory();
               if (totalMemory == 0)
               {
                  MemFree(procs);
                  return SYSINFO_RC_ERROR;
               }
            }
            currValue = static_cast<int64_t>(static_cast<double>(ti.pti_resident_size) * 10000.0 / totalMemory);
            break;
         case PROCINFO_PRIVATE_MEMPERC:
            // Accumulated as bytes and converted to percentage after aggregation - converting
            // per process would truncate each process to whole hundredths of a percent, which
            // rounds down to zero for processes with a small footprint
            if (totalMemory == 0)
            {
               totalMemory = GetTotalPhysicalMemory();
               if (totalMemory == 0)
               {
                  MemFree(procs);
                  return SYSINFO_RC_ERROR;
               }
            }
            if (!GetProcessPhysFootprint(procs[i].kp_proc.p_pid, &currValue))
               currValue = 0;
            break;
         case PROCINFO_PRIVATE_RSS:
            if (!GetProcessPhysFootprint(procs[i].kp_proc.p_pid, &currValue))
               currValue = 0;
            break;
         case PROCINFO_RSS:
            currValue = static_cast<int64_t>(ti.pti_resident_size);
            break;
         case PROCINFO_THREADS:
            currValue = ti.pti_threadnum;
            break;
         case PROCINFO_VMSIZE:
            currValue = static_cast<int64_t>(ti.pti_virtual_size);
            break;
      }

      matched++;
      switch (type)
      {
         case INFOTYPE_SUM:
         case INFOTYPE_AVG:
            result += currValue;
            break;
         case INFOTYPE_MIN:
            result = (matched == 1) ? currValue : std::min(result, currValue);
            break;
         case INFOTYPE_MAX:
            result = (matched == 1) ? currValue : std::max(result, currValue);
            break;
      }
   }

   MemFree(procs);

   if ((type == INFOTYPE_AVG) && (matched > 0))
      result /= matched;

   int attribute = CAST_FROM_POINTER(arg, int);
   if (attribute == PROCINFO_PRIVATE_MEMPERC)
      result = (totalMemory > 0) ? static_cast<int64_t>(static_cast<double>(result) * 10000.0 / totalMemory) : 0;
   if ((attribute == PROCINFO_MEMPERC) || (attribute == PROCINFO_PRIVATE_MEMPERC))
   {
      _sntprintf(value, MAX_RESULT_LENGTH, _T("%d.%02d"), static_cast<int>(result) / 100, static_cast<int>(result) % 100);
   }
   else
   {
      ret_int64(value, result);
   }

   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.ProcessList and System.Processes enumerations
 */
LONG H_ProcessList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   int procCount = 0;
   struct kinfo_proc *procs = GetProcessList(&procCount);
   if (procs == nullptr)
      return SYSINFO_RC_ERROR;

   bool useCmdLine = (*arg == '2');
   size_t bufferSize = useCmdLine ? MAX_CMD_LINE_LEN + 32 : 256;
   char *buffer = static_cast<char*>(MemAlloc(bufferSize));
   char *cmdLine = useCmdLine ? static_cast<char*>(MemAlloc(MAX_CMD_LINE_LEN)) : nullptr;

   for (int i = 0; i < procCount; i++)
   {
      if (useCmdLine)
      {
         if (ReadProcCmdLine(procs[i].kp_proc.p_pid, cmdLine, MAX_CMD_LINE_LEN) && (cmdLine[0] != 0))
         {
            snprintf(buffer, bufferSize, "%d %s", procs[i].kp_proc.p_pid, cmdLine);
         }
         else
         {
            snprintf(buffer, bufferSize, "%d %s", procs[i].kp_proc.p_pid, procs[i].kp_proc.p_comm);
         }
      }
      else
      {
         snprintf(buffer, bufferSize, "%d %s", procs[i].kp_proc.p_pid, procs[i].kp_proc.p_comm);
      }
      value->addMBString(buffer);
   }

   MemFree(cmdLine);
   MemFree(buffer);
   MemFree(procs);
   return SYSINFO_RC_SUCCESS;
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

   uint64_t totalMemory = GetTotalPhysicalMemory();
   if (totalMemory == 0)
      return SYSINFO_RC_ERROR;

   int procCount = 0;
   struct kinfo_proc *procs = GetProcessList(&procCount);
   if (procs == nullptr)
      return SYSINFO_RC_ERROR;

   char *cmdLine = static_cast<char*>(MemAlloc(MAX_CMD_LINE_LEN));

   for (int i = 0; i < procCount; i++)
   {
      value->addRow();
      value->set(0, procs[i].kp_proc.p_pid);
#ifdef UNICODE
      value->setPreallocated(1, WideStringFromMBString(procs[i].kp_proc.p_comm));
#else
      value->set(1, procs[i].kp_proc.p_comm);
#endif

      char userName[MAX_USER_NAME_LEN];
      if (GetProcessUserName(procs[i].kp_eproc.e_ucred.cr_uid, userName, sizeof(userName)))
      {
#ifdef UNICODE
         value->setPreallocated(2, WideStringFromMBString(userName));
#else
         value->set(2, userName);
#endif
      }

      struct proc_taskinfo ti;
      int bytes = proc_pidinfo(procs[i].kp_proc.p_pid, PROC_PIDTASKINFO, 0, &ti, sizeof(ti));
      bool hasTaskInfo = (bytes == static_cast<int>(sizeof(ti)));

      value->set(3, hasTaskInfo ? ti.pti_threadnum : 0);
      // Column 4 (HANDLES) - not available on Darwin
      if (hasTaskInfo)
      {
         value->set(5, static_cast<uint64_t>(ti.pti_total_system / 1000000));  // nanoseconds -> milliseconds
         value->set(6, static_cast<uint64_t>(ti.pti_total_user / 1000000));
         value->set(7, static_cast<uint64_t>(ti.pti_virtual_size));
         value->set(8, static_cast<uint64_t>(ti.pti_resident_size));
         value->set(9, static_cast<double>(ti.pti_resident_size) * 100.0 / totalMemory, 2);
         value->set(10, static_cast<uint64_t>(ti.pti_faults));
      }

      if (ReadProcCmdLine(procs[i].kp_proc.p_pid, cmdLine, MAX_CMD_LINE_LEN))
      {
#ifdef UNICODE
         value->setPreallocated(11, WideStringFromMBString(cmdLine));
#else
         value->set(11, cmdLine);
#endif
      }
   }

   MemFree(cmdLine);
   MemFree(procs);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Process.Count, Process.CountEx, System.ProcessCount and System.ThreadCount parameters
 */
LONG H_ProcessCount(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   char name[128] = "", cmdLine[128] = "", userName[128] = "";

   if ((*arg != 'S') && (*arg != 'T'))
   {
      AgentGetParameterArgA(param, 1, name, sizeof(name));
      if (*arg == 'E')
      {
         AgentGetParameterArgA(param, 2, cmdLine, sizeof(cmdLine));
         AgentGetParameterArgA(param, 3, userName, sizeof(userName));
      }
   }

   int procCount = 0;
   struct kinfo_proc *procs = GetProcessList(&procCount);
   if (procs == nullptr)
      return SYSINFO_RC_ERROR;

   int result;
   if (*arg == 'S')
   {
      result = procCount;
   }
   else
   {
      result = 0;
      if (*arg == 'T')
      {
         for (int i = 0; i < procCount; i++)
         {
            struct proc_taskinfo ti;
            int bytes = proc_pidinfo(procs[i].kp_proc.p_pid, PROC_PIDTASKINFO, 0, &ti, sizeof(ti));
            if (bytes == static_cast<int>(sizeof(ti)))
               result += ti.pti_threadnum;
         }
      }
      else
      {
         for (int i = 0; i < procCount; i++)
         {
            if (MatchProcess(&procs[i], *arg == 'E', name, cmdLine, userName))
               result++;
         }
      }
   }

   MemFree(procs);
   ret_int(value, result);
   return SYSINFO_RC_SUCCESS;
}
