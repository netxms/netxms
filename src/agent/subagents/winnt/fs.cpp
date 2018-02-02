/*
** Windows platform subagent
** Copyright (C) 2003-2018 Victor Kirhenshtein
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
** File: fs.cpp
**/

#include "winnt_subagent.h"

/**
 * Handler for FileSystem.Stats table
 */
LONG H_FileSystems(const TCHAR *cmd, const TCHAR *arg, Table *table, AbstractCommSession *session)
{
	LONG rc = SYSINFO_RC_SUCCESS;

	TCHAR volName[MAX_PATH];
	HANDLE hVol = FindFirstVolume(volName, MAX_PATH);
	if (hVol != INVALID_HANDLE_VALUE)
	{
		table->addColumn(_T("MOUNTPOINT"), DCI_DT_STRING, _T("Mount Point"), true);
		table->addColumn(_T("VOLUME"), DCI_DT_STRING, _T("Volume"));
		table->addColumn(_T("LABEL"), DCI_DT_STRING, _T("Label"));
		table->addColumn(_T("FSTYPE"), DCI_DT_STRING, _T("FS Type"));
		table->addColumn(_T("SIZE.TOTAL"), DCI_DT_UINT64, _T("Total"));
		table->addColumn(_T("SIZE.FREE"), DCI_DT_UINT64, _T("Free"));
		table->addColumn(_T("SIZE.FREE.PCT"), DCI_DT_FLOAT, _T("Free %"));
		table->addColumn(_T("SIZE.AVAIL"), DCI_DT_UINT64, _T("Available"));
		table->addColumn(_T("SIZE.AVAIL.PCT"), DCI_DT_FLOAT, _T("Available %"));
		table->addColumn(_T("SIZE.USED"), DCI_DT_UINT64, _T("Used"));
		table->addColumn(_T("SIZE.USED.PCT"), DCI_DT_FLOAT, _T("Used %"));

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
					nxlog_debug(4, _T("H_FileSystems: Call to GetVolumeInformation(\"%s\") failed (%s)"), volName, GetSystemErrorText(GetLastError(), buffer, 1024));
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
					nxlog_debug(4, _T("H_FileSystems: Call to GetDiskFreeSpaceEx(\"%s\".\"%s\") failed (%s)"), volName, mountPoints, GetSystemErrorText(GetLastError(), buffer, 1024));

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
				nxlog_debug(4, _T("H_FileSystems: Call to GetVolumePathNamesForVolumeName(\"%s\") failed (%s)"), volName, GetSystemErrorText(GetLastError(), buffer, 1024));
			}
		} while(FindNextVolume(hVol, volName, MAX_PATH));
		FindVolumeClose(hVol);
	}
	else
	{
		TCHAR buffer[1024];
		nxlog_debug(4, _T("H_FileSystems: Call to FindFirstVolume failed (%s)"), GetSystemErrorText(GetLastError(), buffer, 1024));
		rc = SYSINFO_RC_ERROR;
	}
	return rc;
}

/**
 * Handler for FileSystem.MountPoints list
 */
LONG H_MountPoints(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
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
				nxlog_debug(4, _T("H_MountPoints: Call to GetVolumePathNamesForVolumeName(\"%s\") failed (%s)"), volName, GetSystemErrorText(GetLastError(), buffer, 1024));
			}
		} while(FindNextVolume(hVol, volName, MAX_PATH));
		FindVolumeClose(hVol);
	}
	else
	{
		TCHAR buffer[1024];
		nxlog_debug(4, _T("H_MountPoints: Call to FindFirstVolume failed (%s)"), GetSystemErrorText(GetLastError(), buffer, 1024));
		rc = SYSINFO_RC_ERROR;
	}
	return rc;
}

/**
 * Handler for FileSystem.Type(*) parameter
 */
LONG H_FileSystemType(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR path[MAX_PATH];
   if (!AgentGetParameterArg(cmd, 1, path, MAX_PATH))
      return SYSINFO_RC_UNSUPPORTED;

   if (path[_tcslen(path) - 1] != _T('\\'))
      _tcscat(path, _T("\\"));

   return GetVolumeInformation(path, NULL, 0, NULL, NULL, NULL, value, MAX_RESULT_LENGTH) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
}

/**
 * Handler for disk space information parameters
 */
LONG H_FileSystemInfo(const TCHAR *pszCmd, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
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
         case FSINFO_FREE_BYTES:
            ret_uint64(pValue, freeBytes.QuadPart);
            break;
         case FSINFO_USED_BYTES:
            ret_uint64(pValue, totalBytes.QuadPart - freeBytes.QuadPart);
            break;
         case FSINFO_TOTAL_BYTES:
            ret_uint64(pValue, totalBytes.QuadPart);
            break;
         case FSINFO_FREE_SPACE_PCT:
            ret_double(pValue, 100.0 * (INT64)freeBytes.QuadPart / (INT64)totalBytes.QuadPart);
            break;
         case FSINFO_USED_SPACE_PCT:
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
		nxlog_debug(2, _T("%s: GetDiskFreeSpaceEx failed: %s"), pszCmd, GetSystemErrorText(GetLastError(), error, 256));
      nRet = SYSINFO_RC_ERROR;
   }
   return nRet;
}
