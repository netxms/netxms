/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003-2011 Victor Kirhenshtein
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

/**
 * Handler for System.Memory.XXX parameters
 */
LONG H_MemoryInfo(const TCHAR *cmd, const TCHAR *arg, TCHAR *value)
{
   MEMORYSTATUSEX mse;

   mse.dwLength = sizeof(MEMORYSTATUSEX);
   if (!GlobalMemoryStatusEx(&mse))
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
   
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.Hostname parameter
 */ 
LONG H_HostName(const TCHAR *pszCmd, const TCHAR *pArg, TCHAR *pValue)
{
   DWORD dwSize;
   TCHAR szBuffer[MAX_COMPUTERNAME_LENGTH + 1];

   dwSize = MAX_COMPUTERNAME_LENGTH + 1;
   GetComputerName(szBuffer, &dwSize);
   ret_string(pValue, szBuffer);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for FileSystem.Stats table
 */
LONG H_FileSystems(const TCHAR *cmd, const TCHAR *arg, Table *table)
{
	LONG rc = SYSINFO_RC_SUCCESS;

	TCHAR volName[MAX_PATH];
	HANDLE hVol = FindFirstVolume(volName, MAX_PATH);
	if (hVol != INVALID_HANDLE_VALUE)
	{
		table->addColumn(_T("MOUNTPOINT"), DCI_DT_STRING, true, _T("Mount Point"));
		table->addColumn(_T("VOLUME"), DCI_DT_STRING, false, _T("Volume"));
		table->addColumn(_T("LABEL"), DCI_DT_STRING, false, _T("Label"));
		table->addColumn(_T("FSTYPE"), DCI_DT_STRING, false, _T("FS Type"));
		table->addColumn(_T("SIZE.TOTAL"), DCI_DT_UINT64, false, _T("Total"));
		table->addColumn(_T("SIZE.FREE"), DCI_DT_UINT64, false, _T("Free"));
		table->addColumn(_T("SIZE.FREE.PCT"), DCI_DT_FLOAT, false, _T("Free %"));
		table->addColumn(_T("SIZE.AVAIL"), DCI_DT_UINT64, false, _T("Available"));
		table->addColumn(_T("SIZE.AVAIL.PCT"), DCI_DT_FLOAT, false, _T("Available %"));
		table->addColumn(_T("SIZE.USED"), DCI_DT_UINT64, false, _T("Used"));
		table->addColumn(_T("SIZE.USED.PCT"), DCI_DT_FLOAT, false, _T("Used %"));

		do
		{
			TCHAR mountPoints[MAX_PATH];
			DWORD size;
			if (GetVolumePathNamesForVolumeName(volName, mountPoints, MAX_PATH, &size))
			{
				table->addRow();
				table->set(0, mountPoints);
				table->set(1, volName);

				TCHAR label[MAX_PATH], fsType[MAX_PATH];
				if (GetVolumeInformation(volName, label, MAX_PATH, NULL, NULL, NULL, fsType, MAX_PATH))
				{
					table->set(2, label);
					table->set(3, fsType);
				}
				else
				{
					TCHAR buffer[1024];
					DebugPrintf(INVALID_INDEX, 4, _T("H_FileSystems: Call to GetVolumeInformation(\"%s\") failed (%s)"), volName, GetSystemErrorText(GetLastError(), buffer, 1024));
				}

				ULARGE_INTEGER availBytes, freeBytes, totalBytes;
				if (GetDiskFreeSpaceEx(volName, &availBytes, &totalBytes, &freeBytes))
				{
	            table->set(4, totalBytes.QuadPart);
					table->set(5, freeBytes.QuadPart);
					table->set(6, 100.0 * (INT64)freeBytes.QuadPart / (INT64)totalBytes.QuadPart);
					table->set(7, freeBytes.QuadPart);
					table->set(8, 100.0 * (INT64)freeBytes.QuadPart / (INT64)totalBytes.QuadPart);
					table->set(9, totalBytes.QuadPart - freeBytes.QuadPart);
					table->set(10, 100.0 * (INT64)(totalBytes.QuadPart - freeBytes.QuadPart) / (INT64)totalBytes.QuadPart);
				}
				else
				{
					TCHAR buffer[1024];
					DebugPrintf(INVALID_INDEX, 4, _T("H_FileSystems: Call to GetDiskFreeSpaceEx(\"%s\".\"%s\") failed (%s)"), volName, mountPoints, GetSystemErrorText(GetLastError(), buffer, 1024));

					table->set(4, (QWORD)0);
					table->set(5, (QWORD)0);
					table->set(6, (QWORD)0);
					table->set(7, (QWORD)0);
					table->set(8, (QWORD)0);
					table->set(9, (QWORD)0);
					table->set(10, (QWORD)0);
				}
			}
			else
			{
				TCHAR buffer[1024];
				DebugPrintf(INVALID_INDEX, 4, _T("H_FileSystems: Call to GetVolumePathNamesForVolumeName(\"%s\") failed (%s)"), volName, GetSystemErrorText(GetLastError(), buffer, 1024));
			}
		} while(FindNextVolume(hVol, volName, MAX_PATH));
		FindVolumeClose(hVol);
	}
	else
	{
		TCHAR buffer[1024];
		DebugPrintf(INVALID_INDEX, 4, _T("H_FileSystems: Call to FindFirstVolume failed (%s)"), GetSystemErrorText(GetLastError(), buffer, 1024));
		rc = SYSINFO_RC_ERROR;
	}
	return rc;
}

/**
 * Handler for FileSystem.MountPoints list
 */
LONG H_MountPoints(const TCHAR *cmd, const TCHAR *arg, StringList *value)
{
	LONG rc = SYSINFO_RC_SUCCESS;

	TCHAR volName[MAX_PATH];
	HANDLE hVol = FindFirstVolume(volName, MAX_PATH);
	if (hVol != INVALID_HANDLE_VALUE)
	{
		do
		{
			TCHAR mountPoints[MAX_PATH];
			DWORD size;
			if (GetVolumePathNamesForVolumeName(volName, mountPoints, MAX_PATH, &size))
			{
				if (mountPoints[0] != 0)
					value->add(mountPoints);
				else
					value->add(volName);
			}
			else
			{
				TCHAR buffer[1024];
				DebugPrintf(INVALID_INDEX, 4, _T("H_MountPoints: Call to GetVolumePathNamesForVolumeName(\"%s\") failed (%s)"), volName, GetSystemErrorText(GetLastError(), buffer, 1024));
			}
		} while(FindNextVolume(hVol, volName, MAX_PATH));
		FindVolumeClose(hVol);
	}
	else
	{
		TCHAR buffer[1024];
		DebugPrintf(INVALID_INDEX, 4, _T("H_MountPoints: Call to FindFirstVolume failed (%s)"), GetSystemErrorText(GetLastError(), buffer, 1024));
		rc = SYSINFO_RC_ERROR;
	}
	return rc;
}

/**
 * Handler for disk space information parameters
 */
LONG H_DiskInfo(const TCHAR *pszCmd, const TCHAR *pArg, TCHAR *pValue)
{
   TCHAR szPath[MAX_PATH];
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

/**
 * Handler for System.Uname parameter
 * by LWX
 */
LONG H_SystemUname(const TCHAR *cmd, const TCHAR *arg, TCHAR *value)
{
   DWORD dwSize;
   TCHAR *cpuType, computerName[MAX_COMPUTERNAME_LENGTH + 1], osVersion[256];
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
				_sntprintf(osVersion, 256, _T("Windows %s-%s"), versionInfo.dwMinorVersion == 0 ? _T("95") :
					(versionInfo.dwMinorVersion == 10 ? _T("98") :
						(versionInfo.dwMinorVersion == 90 ? _T("Me") : _T("Unknown"))), versionInfo.szCSDVersion);
				break;
			case VER_PLATFORM_WIN32_NT:
				_sntprintf(osVersion, 256, _T("Windows NT %d.%d %s"), versionInfo.dwMajorVersion,
						  versionInfo.dwMinorVersion, versionInfo.szCSDVersion);
				break;
			default:
				_tcscpy(osVersion, _T("Windows [Unknown Version]"));
				break;
		}
	}

	GetSystemInfo(&sysInfo);
   switch(sysInfo.wProcessorArchitecture)
   {
      case PROCESSOR_ARCHITECTURE_INTEL:
         cpuType = _T("Intel IA-32");
         break;
      case PROCESSOR_ARCHITECTURE_MIPS:
         cpuType = _T("MIPS");
         break;
      case PROCESSOR_ARCHITECTURE_ALPHA:
         cpuType = _T("Alpha");
         break;
      case PROCESSOR_ARCHITECTURE_PPC:
         cpuType = _T("PowerPC");
         break;
      case PROCESSOR_ARCHITECTURE_IA64:
         cpuType = _T("Intel IA-64");
         break;
      case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
         cpuType = _T("IA-32 on IA-64");
         break;
      case PROCESSOR_ARCHITECTURE_AMD64:
         cpuType = _T("AMD-64");
         break;
      default:
         cpuType = _T("unknown");
         break;
   }

	_sntprintf(value, MAX_RESULT_LENGTH, _T("Windows %s %d.%d.%d %s %s"), computerName, versionInfo.dwMajorVersion,
	           versionInfo.dwMinorVersion, versionInfo.dwBuildNumber, osVersion, cpuType);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.CPU.Count parameter
 */
LONG H_CPUCount(const TCHAR *pszCmd, const TCHAR *pArg, TCHAR *pValue)
{
   SYSTEM_INFO sysInfo;

   GetSystemInfo(&sysInfo);
   ret_uint(pValue, sysInfo.dwNumberOfProcessors);
   return SYSINFO_RC_SUCCESS;
}
