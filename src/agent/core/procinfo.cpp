/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: win32.cpp
** Win32 specific process information parameters
**
**/

#include "nxagentd.h"


//
// Convert process time from FILETIME structure (100-nanosecond units) to __uint64 (milliseconds)
//

static unsigned __int64 ConvertProcessTime(FILETIME *lpft)
{
   unsigned __int64 i;

   memcpy(&i, lpft, sizeof(unsigned __int64));
   i /= 10000;      // Convert 100-nanosecond units to milliseconds
   return i;
}


//
// Check if attribute supported or not
//

static BOOL IsAttributeSupported(int attr)
{
   switch(attr)
   {
      case PROCINFO_GDI_OBJ:
      case PROCINFO_USER_OBJ:
         if (imp_GetGuiResources == NULL)
            return FALSE;     // No appropriate function available, probably we are running on NT4
         break;
      case PROCINFO_IO_READ_B:
      case PROCINFO_IO_READ_OP:
      case PROCINFO_IO_WRITE_B:
      case PROCINFO_IO_WRITE_OP:
      case PROCINFO_IO_OTHER_B:
      case PROCINFO_IO_OTHER_OP:
         if (imp_GetProcessIoCounters == NULL)
            return FALSE;     // No appropriate function available, probably we are running on NT4
         break;
      default:
         break;
   }

   return TRUE;
}


//
// Get specific process attribute
//

static unsigned __int64 GetProcessAttribute(HANDLE hProcess, int attr, int type, int count,
                                            unsigned __int64 lastValue)
{
   unsigned __int64 value;  
   PROCESS_MEMORY_COUNTERS mc;
   IO_COUNTERS ioCounters;
   FILETIME ftCreate, ftExit, ftKernel, ftUser;

   // Get value for current process instance
   switch(attr)
   {
      case PROCINFO_VMSIZE:
         GetProcessMemoryInfo(hProcess, &mc, sizeof(PROCESS_MEMORY_COUNTERS));
         value = mc.PagefileUsage/1024;   // Convert to Kbytes
         break;
      case PROCINFO_WKSET:
         GetProcessMemoryInfo(hProcess, &mc, sizeof(PROCESS_MEMORY_COUNTERS));
         value = mc.WorkingSetSize/1024;   // Convert to Kbytes
         break;
      case PROCINFO_PF:
         GetProcessMemoryInfo(hProcess, &mc, sizeof(PROCESS_MEMORY_COUNTERS));
         value = mc.PageFaultCount;
         break;
      case PROCINFO_KTIME:
      case PROCINFO_UTIME:
         GetProcessTimes(hProcess, &ftCreate, &ftExit, &ftKernel, &ftUser);
         value = ConvertProcessTime(attr == PROCINFO_KTIME ? &ftKernel : &ftUser);
         break;
      case PROCINFO_GDI_OBJ:
      case PROCINFO_USER_OBJ:
         value = imp_GetGuiResources(hProcess, attr == PROCINFO_GDI_OBJ ? GR_GDIOBJECTS : GR_USEROBJECTS);
         break;
      case PROCINFO_IO_READ_B:
         imp_GetProcessIoCounters(hProcess, &ioCounters);
         value = ioCounters.ReadTransferCount;
         break;
      case PROCINFO_IO_READ_OP:
         imp_GetProcessIoCounters(hProcess, &ioCounters);
         value = ioCounters.ReadOperationCount;
         break;
      case PROCINFO_IO_WRITE_B:
         imp_GetProcessIoCounters(hProcess, &ioCounters);
         value = ioCounters.WriteTransferCount;
         break;
      case PROCINFO_IO_WRITE_OP:
         imp_GetProcessIoCounters(hProcess, &ioCounters);
         value = ioCounters.WriteOperationCount;
         break;
      case PROCINFO_IO_OTHER_B:
         imp_GetProcessIoCounters(hProcess, &ioCounters);
         value = ioCounters.OtherTransferCount;
         break;
      case PROCINFO_IO_OTHER_OP:
         imp_GetProcessIoCounters(hProcess, &ioCounters);
         value = ioCounters.OtherOperationCount;
         break;
      default:       // Unknown attribute
         WriteLog(MSG_UNEXPECTED_ATTRIBUTE, EVENTLOG_ERROR_TYPE, "x", attr);
         value = 0;
   }

   // Recalculate final value according to selected type
   if (count == 1)     // First instance
   {
      return value;
   }

   switch(type)
   {
      case INFOTYPE_MIN:
         return min(lastValue, value);
      case INFOTYPE_MAX:
         return max(lastValue, value);
      case INFOTYPE_AVG:
         return (lastValue * (count - 1) + value) / count;
      case INFOTYPE_SUM:
         return lastValue + value;
      default:
         WriteLog(MSG_UNEXPECTED_TYPE, EVENTLOG_ERROR_TYPE, "x", type);
         return 0;
   }
}


//
// Get process-specific information
// Parameter has the following syntax:
//    Process.XXX(<process>,<type>)
// where
//    XXX        - requested process attribute (see documentation for list of valid attributes)
//    <process>  - process name (same as in Process.Count() parameter)
//    <type>     - representation type (meaningful when more than one process with the same
//                 name exists). Valid values are:
//         min - minimal value among all processes named <process>
//         max - maximal value among all processes named <process>
//         avg - average value for all processes named <process>
//         sum - sum of values for all processes named <process>
//

LONG H_ProcInfo(char *cmd, char *arg, char *value)
{
   char buffer[256];
   int attr, type, i, procCount, counter;
   unsigned __int64 attrVal;
   DWORD *procList, dwSize;
   HMODULE *modList;
   static char *typeList[]={ "min", "max", "avg", "sum" };

   // Check attribute
   attr = (int)arg;
   if (!IsAttributeSupported(attr))
      return SYSINFO_RC_UNSUPPORTED;     // Unsupported attribute

   // Get parameter type arguments
   NxGetParameterArg(cmd, 2, buffer, 255);
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
   NxGetParameterArg(cmd, 1, buffer, 255);

   // Gather information
   attrVal = 0;
   procList = (DWORD *)malloc(MAX_PROCESSES * sizeof(DWORD));
   modList = (HMODULE *)malloc(MAX_MODULES * sizeof(HMODULE));
   EnumProcesses(procList, sizeof(DWORD) * MAX_PROCESSES, &dwSize);
   procCount = dwSize / sizeof(DWORD);
   for(i = 0, counter = 0; i < procCount; i++)
   {
      HANDLE hProcess;

      hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, procList[i]);
      if (hProcess != NULL)
      {
         if (EnumProcessModules(hProcess, modList, sizeof(HMODULE) * MAX_MODULES, &dwSize))
         {
            if (dwSize >= sizeof(HMODULE))     // At least one module exist
            {
               char baseName[MAX_PATH];

               GetModuleBaseName(hProcess, modList[0], baseName, sizeof(baseName));
               if (!stricmp(baseName, buffer))
               {
                  counter++;  // Number of processes with specific name
                  attrVal = GetProcessAttribute(hProcess, attr, type, counter, attrVal);
               }
            }
         }
         CloseHandle(hProcess);
      }
   }

   // Cleanup
   free(procList);
   free(modList);

   if (counter == 0)    // No processes with given name
      return SYSINFO_RC_ERROR;

   ret_uint64(value, attrVal);
   return SYSINFO_RC_SUCCESS;
}
