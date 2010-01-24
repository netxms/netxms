/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Victor Kirhenshtein
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
** File: win32.cpp
** Win32 specific parameters or Win32 implementation of common parameters
**
**/

#include "nxagentd.h"


//
// Handler for System.Memory.XXX parameters
//

LONG H_MemoryInfo(const char *cmd, const char *arg, char *value)
{
   if (imp_GlobalMemoryStatusEx != NULL)
   {
      MEMORYSTATUSEX mse;

      mse.dwLength = sizeof(MEMORYSTATUSEX);
      if (!imp_GlobalMemoryStatusEx(&mse))
         return SYSINFO_RC_ERROR;

      switch(CAST_FROM_POINTER(arg, int))
      {
         case MEMINFO_PHYSICAL_FREE:
            ret_uint64(value, mse.ullAvailPhys);
            break;
         case MEMINFO_PHYSICAL_FREE_PCT:
            ret_uint(value, (DWORD)(mse.ullAvailPhys * 100 / mse.ullTotalPhys));
            break;
         case MEMINFO_PHYSICAL_TOTAL:
            ret_uint64(value, mse.ullTotalPhys);
            break;
         case MEMINFO_PHYSICAL_USED:
            ret_uint64(value, mse.ullTotalPhys - mse.ullAvailPhys);
            break;
         case MEMINFO_PHYSICAL_USED_PCT:
            ret_uint(value, (DWORD)((mse.ullTotalPhys - mse.ullAvailPhys) * 100 / mse.ullTotalPhys));
            break;
         case MEMINFO_VIRTUAL_FREE:
            ret_uint64(value, mse.ullAvailPageFile);
            break;
         case MEMINFO_VIRTUAL_FREE_PCT:
            ret_uint(value, (DWORD)(mse.ullAvailPageFile * 100 / mse.ullTotalPageFile));
            break;
         case MEMINFO_VIRTUAL_TOTAL:
            ret_uint64(value, mse.ullTotalPageFile);
            break;
         case MEMINFO_VIRTUAL_USED:
            ret_uint64(value, mse.ullTotalPageFile - mse.ullAvailPageFile);
            break;
         case MEMINFO_VIRTUAL_USED_PCT:
            ret_uint(value, (DWORD)((mse.ullTotalPageFile - mse.ullAvailPageFile) * 100 / mse.ullTotalPageFile));
            break;
         default:
            return SYSINFO_RC_UNSUPPORTED;
      }
   }
   else
   {
      MEMORYSTATUS ms;

      GlobalMemoryStatus(&ms);
      switch(CAST_FROM_POINTER(arg, int))
      {
         case MEMINFO_PHYSICAL_FREE:
            ret_uint64(value, ms.dwAvailPhys);
            break;
         case MEMINFO_PHYSICAL_FREE_PCT:
            ret_uint(value, (DWORD)((QWORD)ms.dwAvailPhys * 100 / (QWORD)ms.dwTotalPhys));
            break;
         case MEMINFO_PHYSICAL_TOTAL:
            ret_uint64(value, ms.dwTotalPhys);
            break;
         case MEMINFO_PHYSICAL_USED:
            ret_uint64(value, ms.dwTotalPhys - ms.dwAvailPhys);
            break;
         case MEMINFO_PHYSICAL_USED_PCT:
            ret_uint(value, (DWORD)((QWORD)(ms.dwTotalPhys - ms.dwAvailPhys) * 100 / (QWORD)ms.dwTotalPhys));
            break;
         case MEMINFO_VIRTUAL_FREE:
            ret_uint64(value, ms.dwAvailPageFile);
            break;
         case MEMINFO_VIRTUAL_FREE_PCT:
            ret_uint(value, (DWORD)((QWORD)ms.dwAvailPageFile * 100 / (QWORD)ms.dwTotalPageFile));
            break;
         case MEMINFO_VIRTUAL_TOTAL:
            ret_uint64(value, ms.dwTotalPageFile);
            break;
         case MEMINFO_VIRTUAL_USED:
            ret_uint64(value, ms.dwTotalPageFile - ms.dwAvailPageFile);
            break;
         case MEMINFO_VIRTUAL_USED_PCT:
            ret_uint(value, (DWORD)((QWORD)(ms.dwTotalPageFile - ms.dwAvailPageFile) * 100 / (QWORD)ms.dwTotalPageFile));
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

LONG H_HostName(const char *pszCmd, const char *pArg, char *pValue)
{
   DWORD dwSize;
   char szBuffer[MAX_COMPUTERNAME_LENGTH + 1];

   dwSize = MAX_COMPUTERNAME_LENGTH + 1;
   GetComputerName(szBuffer, &dwSize);
   ret_string(pValue, szBuffer);
   return SYSINFO_RC_SUCCESS;
}


//
// Handler for disk space information parameters
//

LONG H_DiskInfo(const char *pszCmd, const char *pArg, char *pValue)
{
   char szPath[MAX_PATH];
   ULARGE_INTEGER availBytes, freeBytes, totalBytes;
   LONG nRet = SYSINFO_RC_SUCCESS;

   if (!AgentGetParameterArg(pszCmd, 1, szPath, MAX_PATH))
      return SYSINFO_RC_UNSUPPORTED;
   if (GetDiskFreeSpaceEx(szPath, &availBytes, &totalBytes, &freeBytes))
   {
      switch(CAST_FROM_POINTER(pArg, int))
      {
         case DISKINFO_FREE_BYTES:
            ret_uint64(pValue, freeBytes.QuadPart);
            break;
         case DISKINFO_USED_BYTES:
            ret_uint64(pValue, totalBytes.QuadPart - freeBytes.QuadPart);
            break;
         case DISKINFO_TOTAL_BYTES:
            ret_uint64(pValue, totalBytes.QuadPart);
            break;
         case DISKINFO_FREE_SPACE_PCT:
            ret_double(pValue, 100.0 * (INT64)freeBytes.QuadPart / (INT64)totalBytes.QuadPart);
            break;
         case DISKINFO_USED_SPACE_PCT:
            ret_double(pValue, 100.0 * (INT64)(totalBytes.QuadPart - freeBytes.QuadPart) / (INT64)totalBytes.QuadPart);
            break;
         default:
            nRet = SYSINFO_RC_UNSUPPORTED;
            break;
      }
   }
   else
   {
		TCHAR error[256];

		DebugPrintf(INVALID_INDEX, 2, _T("%s: GetDiskFreeSpaceEx failed: %s"), pszCmd,
		            GetSystemErrorText(GetLastError(), error, 256));
      nRet = SYSINFO_RC_ERROR;
   }

   return nRet;
}


//
// Handler for System.Uname parameter
//

LONG H_SystemUname(const char *cmd, const char *arg, char *value)
{
   DWORD dwSize;
   char *cpuType, computerName[MAX_COMPUTERNAME_LENGTH + 1], osVersion[256];
   OSVERSIONINFO versionInfo;
	SYSTEM_INFO sysInfo;

   dwSize = MAX_COMPUTERNAME_LENGTH + 1;
   GetComputerName(computerName, &dwSize);

 	versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&versionInfo);

	if (!GetWindowsVersionString(osVersion, 256))
	{
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
	}

	GetSystemInfo(&sysInfo);
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
      case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
         cpuType = "IA-32 on IA-64";
         break;
      case PROCESSOR_ARCHITECTURE_AMD64:
         cpuType = "AMD-64";
         break;
      default:
         cpuType = "unknown";
         break;
   }

   sprintf(value, "Windows %s %d.%d.%d %s %s", computerName, versionInfo.dwMajorVersion,
           versionInfo.dwMinorVersion, versionInfo.dwBuildNumber, osVersion, cpuType);
   return SYSINFO_RC_SUCCESS;
}


//
// Handler for System.CPU.Count parameter
//

LONG H_CPUCount(const char *pszCmd, const char *pArg, char *pValue)
{
   SYSTEM_INFO sysInfo;

   GetSystemInfo(&sysInfo);
   ret_uint(pValue, sysInfo.dwNumberOfProcessors);
   return SYSINFO_RC_SUCCESS;
}
