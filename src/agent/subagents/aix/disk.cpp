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
 * Handler for FileSystem.Volumes table
 */
LONG H_FileSystems(const TCHAR *cmd, const TCHAR *arg, Table *table)
{
	LONG rc = SYSINFO_RC_SUCCESS;

   int count;
   struct vmount *mountPoints = GetMountPoints(&count);
   if (mountPoints != NULL)
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

      struct vmount *curr = mountPoints;
      for(int i = 0; i < count; i++)
      {
         table->addRow();
         char *mountPoint = (char *)vmt2dataptr(curr, VMT_STUB);
         table->set(0, mountPoint);
         table->set(1, (char *)vmt2dataptr(curr, VMT_OBJECT));

         switch(curr->vmt_gfstype)
         {
            case MNT_CACHEFS:
               table->set(3, "cachefs");
               break;
            case MNT_CDROM:
               table->set(3, "cdrom");
               break;
            case MNT_CIFS:
               table->set(3, "cifs");
               break;
            case MNT_J2:
               table->set(3, "jfs2");
               break;
            case MNT_JFS:
               table->set(3, "jfs");
               break;
            case MNT_NAMEFS:
               table->set(3, "namefs");
               break;
            case MNT_NFS:
               table->set(3, "nfs");
               break;
            case MNT_NFS3:
               table->set(3, "nfs3");
               break;
            case MNT_NFS4:
               table->set(3, "nfs4");
               break;
            case MNT_PROCFS:
               table->set(3, "procfs");
               break;
            case MNT_UDF:
               table->set(3, "udfs");
               break;
            case MNT_VXFS:
               table->set(3, "vxfs");
               break;
            default:
               table->set(3, (DWORD)curr->vmt_gfstype);
               break;
         }

         struct statvfs s;
         if (statvfs(mountPoint, &s) == 0)
         {
            QWORD usedBlocks = (QWORD)(s.f_blocks - s.f_bfree);
            QWORD totalBlocks = (QWORD)s.f_blocks;
            QWORD blockSize = (QWORD)s.f_bsize;
            QWORD freeBlocks = (QWORD)s.f_bfree;
            QWORD availableBlocks = (QWORD)s.f_bavail;

            table->set(4, totalBlocks * blockSize);
            table->set(5, freeBlocks * blockSize);
            table->set(6, (totalBlocks > 0) ? (freeBlocks * 100) / totalBlocks : 0);
            table->set(7, availableBlocks * blockSize);
            table->set(8, (totalBlocks > 0) ? (availableBlocks * 100) / totalBlocks : 0);
            table->set(9, usedBlocks * blockSize);
            table->set(10, (totalBlocks > 0) ? (usedBlocks * 100) / totalBlocks : 0);
         }
         else
         {
            TCHAR buffer[1024];
            AgentWriteDebugLog(4, "AIX: H_FileSystems: Call to statvfs(\"%s\") failed (%s)", mountPoint, strerror(errno));

            table->set(4, (QWORD)0);
            table->set(5, (QWORD)0);
            table->set(6, (QWORD)0);
            table->set(7, (QWORD)0);
            table->set(8, (QWORD)0);
            table->set(9, (QWORD)0);
            table->set(10, (QWORD)0);
         }

         curr = (struct vmount *)((char *)curr + curr->vmt_length);
      }
      free(mountPoints);
   }
   else
   {
      AgentWriteDebugLog(4, _T("AIX: H_FileSystems: Call to GetMountPoints failed"));
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

	int count;
	struct vmount *mountPoints = GetMountPoints(&count);
	if (mountPoints != NULL)
	{
	   struct vmount *curr = mountPoints;
	   for(int i = 0; i < count; i++)
	   {
	      value->add((char *)vmt2dataptr(curr, VMT_STUB));
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
