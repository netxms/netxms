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
** Win32 specific parameters or Win32 implementation of common parameters
**
**/

#include "nxagentd.h"


//
// Handler for System.ProcessCount
//

LONG H_ProcCount(char *cmd, char *arg, char *value)
{
   DWORD dwSize, *pdwProcList;
   PERFORMANCE_INFORMATION pi;

   // On Windows XP and higher, use new method
   if (imp_GetPerformanceInfo != NULL)
   {
      pi.cb = sizeof(PERFORMANCE_INFORMATION);
      if (!imp_GetPerformanceInfo(&pi, sizeof(PERFORMANCE_INFORMATION)))
         return SYSINFO_RC_ERROR;
      ret_uint(value, pi.ProcessCount);
   }
   else
   {
      pdwProcList = (DWORD *)malloc(sizeof(DWORD) * MAX_PROCESSES);
      EnumProcesses(pdwProcList, sizeof(DWORD) * MAX_PROCESSES, &dwSize);
      free(pdwProcList);
      ret_int(value, dwSize / sizeof(DWORD));
   }
   return SYSINFO_RC_SUCCESS;
}


//
// Handler for System.ThreadCount
//

LONG H_ThreadCount(char *cmd, char *arg, char *value)
{
   PERFORMANCE_INFORMATION pi;

   if (imp_GetPerformanceInfo != NULL)
   {
      pi.cb = sizeof(PERFORMANCE_INFORMATION);
      if (!imp_GetPerformanceInfo(&pi, sizeof(PERFORMANCE_INFORMATION)))
         return SYSINFO_RC_ERROR;
      ret_uint(value, pi.ThreadCount);
      return SYSINFO_RC_SUCCESS;
   }
   else
   {
      return SYSINFO_RC_UNSUPPORTED;
   }
}


//
// Handler for Process.Count(*)
//

LONG H_ProcCountSpecific(char *cmd, char *arg, char *value)
{
   DWORD dwSize = 0, *pdwProcList;
   int i, counter, procCount;
   char procName[MAX_PATH];
   HANDLE hProcess;
   HMODULE modList[MAX_MODULES];

   NxGetParameterArg(cmd, 1, procName, MAX_PATH - 1);
   pdwProcList = (DWORD *)malloc(sizeof(DWORD) * MAX_PROCESSES);
   EnumProcesses(pdwProcList, sizeof(DWORD) * MAX_PROCESSES, &dwSize);
   procCount = dwSize / sizeof(DWORD);
   for(i = 0, counter = 0; i < procCount; i++)
   {
      hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pdwProcList[i]);
      if (hProcess != NULL)
      {
         if (EnumProcessModules(hProcess, modList, sizeof(HMODULE) * MAX_MODULES, &dwSize))
         {
            if (dwSize >= sizeof(HMODULE))     // At least one module exist
            {
               char baseName[MAX_PATH];

               GetModuleBaseName(hProcess, modList[0], baseName, sizeof(baseName));
               if (!stricmp(baseName, procName))
                  counter++;
            }
         }
         CloseHandle(hProcess);
      }
   }
   ret_int(value, counter);
   free(pdwProcList);
   return SYSINFO_RC_SUCCESS;
}


//
// Handler for System.Memory.XXX parameters
//

LONG H_MemoryInfo(char *cmd, char *arg, char *value)
{
   if (imp_GlobalMemoryStatusEx != NULL)
   {
      MEMORYSTATUSEX mse;

      mse.dwLength = sizeof(MEMORYSTATUSEX);
      if (!imp_GlobalMemoryStatusEx(&mse))
         return SYSINFO_RC_ERROR;

      switch((int)arg)
      {
         case MEMINFO_PHYSICAL_FREE:
            ret_uint64(value, mse.ullAvailPhys);
            break;
         case MEMINFO_PHYSICAL_TOTAL:
            ret_uint64(value, mse.ullTotalPhys);
            break;
         case MEMINFO_PHYSICAL_USED:
            ret_uint64(value, mse.ullTotalPhys - mse.ullAvailPhys);
            break;
         case MEMINFO_SWAP_FREE:
            ret_uint64(value, mse.ullAvailPageFile);
            break;
         case MEMINFO_SWAP_TOTAL:
            ret_uint64(value, mse.ullTotalPageFile);
            break;
         case MEMINFO_SWAP_USED:
            ret_uint64(value, mse.ullTotalPageFile - mse.ullAvailPageFile);
            break;
         case MEMINFO_VIRTUAL_FREE:
            ret_uint64(value, mse.ullAvailVirtual);
            break;
         case MEMINFO_VIRTUAL_TOTAL:
            ret_uint64(value, mse.ullTotalVirtual);
            break;
         case MEMINFO_VIRTUAL_USED:
            ret_uint64(value, mse.ullTotalVirtual - mse.ullAvailVirtual);
            break;
         default:
            return SYSINFO_RC_UNSUPPORTED;
      }
   }
   else
   {
      MEMORYSTATUS ms;

      GlobalMemoryStatus(&ms);
      switch((int)arg)
      {
         case MEMINFO_PHYSICAL_FREE:
            ret_uint(value, ms.dwAvailPhys);
            break;
         case MEMINFO_PHYSICAL_TOTAL:
            ret_uint(value, ms.dwTotalPhys);
            break;
         case MEMINFO_PHYSICAL_USED:
            ret_uint(value, ms.dwTotalPhys - ms.dwAvailPhys);
            break;
         case MEMINFO_SWAP_FREE:
            ret_uint(value, ms.dwAvailPageFile);
            break;
         case MEMINFO_SWAP_TOTAL:
            ret_uint(value, ms.dwTotalPageFile);
            break;
         case MEMINFO_SWAP_USED:
            ret_uint(value, ms.dwTotalPageFile - ms.dwAvailPageFile);
            break;
         case MEMINFO_VIRTUAL_FREE:
            ret_uint(value, ms.dwAvailVirtual);
            break;
         case MEMINFO_VIRTUAL_TOTAL:
            ret_uint(value, ms.dwTotalVirtual);
            break;
         case MEMINFO_VIRTUAL_USED:
            ret_uint(value, ms.dwTotalVirtual - ms.dwAvailVirtual);
            break;
         default:
            return SYSINFO_RC_UNSUPPORTED;
      }
   }
   
   return SYSINFO_RC_SUCCESS;
}


//
// Handler for System.Hostname parameter
//

LONG H_HostName(char *cmd, char *arg, char *value)
{
   DWORD dwSize;
   char buffer[MAX_COMPUTERNAME_LENGTH + 1];

   dwSize = MAX_COMPUTERNAME_LENGTH + 1;
   GetComputerName(buffer, &dwSize);
   ret_string(value, buffer);
   return SYSINFO_RC_SUCCESS;
}


//
// Handler for Disk.Free(*), Disk.Used(*)  and Disk.Total(*) parameters
//

LONG H_DiskInfo(char *cmd, char *arg, char *value)
{
   char path[MAX_PATH];
   ULARGE_INTEGER freeBytes, totalBytes;

   if (!NxGetParameterArg(cmd, 1, path, MAX_PATH - 1))
      return SYSINFO_RC_UNSUPPORTED;
   if (!GetDiskFreeSpaceEx(path, &freeBytes, &totalBytes, NULL))
      return SYSINFO_RC_UNSUPPORTED;

   if (!memicmp(cmd, "Disk.Free",9))
      ret_uint64(value, freeBytes.QuadPart);
   else if (!memicmp(cmd, "Disk.Used",9))
      ret_uint64(value, totalBytes.QuadPart - freeBytes.QuadPart);
   else
      ret_uint64(value, totalBytes.QuadPart);
   return SYSINFO_RC_SUCCESS;
}


//
// Handler for System.Uname parameter
//

LONG H_SystemUname(char *cmd, char *arg, char *value)
{
   DWORD dwSize;
   char *cpuType, computerName[MAX_COMPUTERNAME_LENGTH + 1], osVersion[256];
   SYSTEM_INFO sysInfo;
   OSVERSIONINFO versionInfo;

   dwSize = MAX_COMPUTERNAME_LENGTH + 1;
   GetComputerName(computerName, &dwSize);

   versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
   GetVersionEx(&versionInfo);
   GetSystemInfo(&sysInfo);

   switch(versionInfo.dwPlatformId)
   {
      case VER_PLATFORM_WIN32_WINDOWS:
         sprintf(osVersion,"Windows %s-%s",versionInfo.dwMinorVersion == 0 ? "95" :
            (versionInfo.dwMinorVersion == 10 ? "98" :
               (versionInfo.dwMinorVersion == 90 ? "Me" : "Unknown")),versionInfo.szCSDVersion);
         break;
      case VER_PLATFORM_WIN32_NT:
         if (versionInfo.dwMajorVersion != 5)
            sprintf(osVersion,"Windows NT %d.%d %s",versionInfo.dwMajorVersion,
                    versionInfo.dwMinorVersion,versionInfo.szCSDVersion);
         else      // Windows 2000, Windows XP or Windows Server 2003
            sprintf(osVersion,"Windows %s%s%s",versionInfo.dwMinorVersion == 0 ? "2000" : 
                    (versionInfo.dwMinorVersion == 1 ? "XP" : "Server 2003"),
                       versionInfo.szCSDVersion[0] == 0 ? "" : " ", versionInfo.szCSDVersion);
         break;
      default:
         strcpy(osVersion,"Windows [Unknown Version]");
         break;
   }

   switch(sysInfo.wProcessorArchitecture)
   {
      case PROCESSOR_ARCHITECTURE_INTEL:
         cpuType = "Intel IA-32";
         break;
      case PROCESSOR_ARCHITECTURE_MIPS:
         cpuType = "MIPS";
         break;
      case PROCESSOR_ARCHITECTURE_ALPHA:
         cpuType = "Alpha";
         break;
      case PROCESSOR_ARCHITECTURE_PPC:
         cpuType = "PowerPC";
         break;
      case PROCESSOR_ARCHITECTURE_IA64:
         cpuType = "Intel IA-64";
         break;
#ifdef PROCESSOR_ARCHITECTURE_IA32_ON_WIN64
      case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
         cpuType = "IA-32 on IA-64";
         break;
#endif
#ifdef PROCESSOR_ARCHITECTURE_AMD64
      case PROCESSOR_ARCHITECTURE_AMD64:
         cpuType = "AMD-64";
         break;
#endif
      default:
         cpuType = "unknown";
         break;
   }

   sprintf(value, "Windows %s %d.%d.%d %s %s\r\n", computerName, versionInfo.dwMajorVersion,
           versionInfo.dwMinorVersion, versionInfo.dwBuildNumber, osVersion, cpuType);
   return SYSINFO_RC_SUCCESS;
}


//
// Handler for System.ServiceState parameter
//

LONG H_ServiceState(char *cmd, char *arg, char *value)
{
   SC_HANDLE hManager, hService;
   TCHAR szServiceName[MAX_PATH];
   LONG iResult = SYSINFO_RC_SUCCESS;

   if (!NxGetParameterArg(cmd, 1, szServiceName, MAX_PATH))
      return SYSINFO_RC_UNSUPPORTED;

   hManager = OpenSCManager(NULL, NULL, GENERIC_READ);
   if (hManager == NULL)
   {
      return SYSINFO_RC_ERROR;
   }

   hService = OpenService(hManager, szServiceName, SERVICE_QUERY_STATUS);
   if (hService == NULL)
   {
      iResult = SYSINFO_RC_UNSUPPORTED;
   }
   else
   {
      SERVICE_STATUS status;

      if (QueryServiceStatus(hService, &status))
      {
         int i;
         static DWORD dwStates[7]={ SERVICE_RUNNING, SERVICE_PAUSED, SERVICE_START_PENDING,
                                    SERVICE_PAUSE_PENDING, SERVICE_CONTINUE_PENDING,
                                    SERVICE_STOP_PENDING, SERVICE_STOPPED };

         for(i = 0; i < 7; i++)
            if (status.dwCurrentState == dwStates[i])
               break;
         ret_uint(value, i);
      }
      else
      {
         ret_uint(value, 255);    // Unable to retrieve information
      }
      CloseServiceHandle(hService);
   }

   CloseServiceHandle(hManager);
   return iResult;
}
