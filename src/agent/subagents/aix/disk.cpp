/* 
** NetXMS subagent for AIX
** Copyright (C) 2004, 2005, 2006 Victor Kirhenshtein
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

#include "aix_subagent.h"
#include <sys/statvfs.h>

/**
 * Handler for Disk.xxx parameters
 */
LONG H_DiskInfo(const char *pszParam, const char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
	struct statvfs s;
	char szArg[512] = "";

	if (!AgentGetParameterArg(pszParam, 1, szArg, sizeof(szArg)))
		return SYSINFO_RC_UNSUPPORTED;

	if (szArg[0] != 0 && statvfs(szArg, &s) == 0)
	{
		nRet = SYSINFO_RC_SUCCESS;
		
		QWORD usedBlocks = (QWORD)(s.f_blocks - s.f_bfree);
		QWORD totalBlocks = (QWORD)s.f_blocks;
		QWORD blockSize = (QWORD)s.f_bsize;
		QWORD freeBlocks = (QWORD)s.f_bfree;
		QWORD availableBlocks = (QWORD)s.f_bavail;
		
		switch((long)pArg)
		{
			case DISK_TOTAL:
				ret_uint64(pValue, totalBlocks * blockSize);
				break;
			case DISK_USED:
				ret_uint64(pValue, usedBlocks * blockSize);
				break;
			case DISK_FREE:
				ret_uint64(pValue, freeBlocks * blockSize);
				break;
			case DISK_AVAIL:
				ret_uint64(pValue, availableBlocks * blockSize);
				break;
			case DISK_USED_PERC:
				ret_double(pValue, (usedBlocks * 100) / totalBlocks);
				break;
			case DISK_AVAIL_PERC:
				ret_double(pValue, (availableBlocks * 100) / totalBlocks);
				break;
			case DISK_FREE_PERC:
				ret_double(pValue, (freeBlocks * 100) / totalBlocks);
				break;
			default:
				nRet = SYSINFO_RC_ERROR;
				break;
		}
	}

	return nRet;
}

/**
 * Get current mount points
 *
 * @param count pointer to buffer where number of mount points will be stored
 * @return pointer to list of vmount structures or NULL on error
 */
static struct vmount *GetMountPoints(int *count)
{
   int size = 8192;
   char *buffer = (char *)malloc(size);
   while(size <= 65536)
   {
      int rc = mntctl(MCTL_QUERY, size, buffer);

      if (rc < 0)
         break;

      if (rc > 0)
      {
         *count = rc;
         return (struct vmount *)buffer;
      }

      size += 8192;
      buffer = (char *)realloc(buffer, size);
   }
   free(buffer);
   return NULL;
}

/**
 * Handler for FileSystem.Stats table
 */
LONG H_FileSystems(const TCHAR *cmd, const TCHAR *arg, Table *table)
{
	LONG rc = SYSINFO_RC_SUCCESS;

	/*
	TCHAR volName[MAX_PATH];
	HANDLE hVol = FindFirstVolume(volName, MAX_PATH);
	if (hVol != INVALID_HANDLE_VALUE)
	{
		table->addColumn(_T("MOUNTPOINT"), DCI_DT_STRING);
		table->addColumn(_T("VOLUME"), DCI_DT_STRING);
		table->addColumn(_T("LABEL"), DCI_DT_STRING);
		table->addColumn(_T("FSTYPE"), DCI_DT_STRING);
		table->addColumn(_T("SIZE.TOTAL"), DCI_DT_UINT64);
		table->addColumn(_T("SIZE.FREE"), DCI_DT_UINT64);
		table->addColumn(_T("SIZE.FREE.PCT"), DCI_DT_FLOAT);
		table->addColumn(_T("SIZE.AVAIL"), DCI_DT_UINT64);
		table->addColumn(_T("SIZE.AVAIL.PCT"), DCI_DT_FLOAT);
		table->addColumn(_T("SIZE.USED"), DCI_DT_UINT64);
		table->addColumn(_T("SIZE.USED.PCT"), DCI_DT_FLOAT);

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
	*/
	return rc;
}

/**
 * Handler for FileSystem.MountPoints list
 */
LONG H_MountPoints(const TCHAR *cmd, const TCHAR *arg, StringList *value)
{
	LONG rc = SYSINFO_RC_SUCCESS;

	int count;
	struct vmount *mountPoints = GetMountPoints(&count);
	if (mountPoints != NULL)
	{
	   struct vmount *curr = mountPoints;
	   for(int i = 0; i < count; i++)
	   {
	      value->add((char *)curr + curr->vmt_data[VMT_STUB].vmt_off);
	      curr = (struct vmount *)((char *)curr + curr->vmt_length);
	   }
	   free(mountPoints);
	}
	else
	{
		AgentWriteDebugLog(4, _T("AIX: H_MountPoints: Call to GetMountPoints failed"));
		rc = SYSINFO_RC_ERROR;
	}
	return rc;
}
