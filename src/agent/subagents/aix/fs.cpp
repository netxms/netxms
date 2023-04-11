/* 
** NetXMS subagent for AIX
** Copyright (C) 2004-2023 Victor Kirhenshtein
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
 * Handler for FileSystem.xxx parameters
 */
LONG H_FileSystemInfo(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   char fsname[512] = "";
   if (!AgentGetParameterArgA(pszParam, 1, fsname, sizeof(fsname)))
      return SYSINFO_RC_UNSUPPORTED;

   int nRet = SYSINFO_RC_ERROR;
   struct statvfs s;
   if ((fsname[0] != 0) && statvfs(fsname, &s) == 0)
   {
      nRet = SYSINFO_RC_SUCCESS;
		
      uint64_t usedBlocks = (s.f_blocks == (fsblkcnt_t)(-1)) ? _ULL(0) : (uint64_t)(s.f_blocks - s.f_bfree);
      uint64_t totalBlocks = (s.f_blocks == (fsblkcnt_t)(-1)) ? _ULL(0) : (uint64_t)s.f_blocks;
      uint64_t blockSize = (uint64_t)s.f_frsize;
      uint64_t freeBlocks = (uint64_t)s.f_bfree;
      uint64_t availableBlocks = (uint64_t)s.f_bavail;
		
      switch((long)pArg)
      {
			case FS_TOTAL:
				ret_uint64(pValue, totalBlocks * blockSize);
				break;
			case FS_USED:
				ret_uint64(pValue, usedBlocks * blockSize);
				break;
			case FS_FREE:
				ret_uint64(pValue, freeBlocks * blockSize);
				break;
			case FS_AVAIL:
				ret_uint64(pValue, availableBlocks * blockSize);
				break;
			case FS_USED_PERC:
				ret_double(pValue, (totalBlocks > 0) ? (usedBlocks * 100.0) / totalBlocks : 0);
				break;
			case FS_FREE_PERC:
				ret_double(pValue, (totalBlocks > 0) ? (freeBlocks * 100.0) / totalBlocks : 0);
				break;
			case FS_AVAIL_PERC:
				ret_double(pValue, (totalBlocks > 0) ? (availableBlocks * 100.0) / totalBlocks : 0);
				break;
			case FS_TOTAL_INODES:
				ret_uint64(pValue, s.f_files);
				break;
			case FS_USED_INODES:
				ret_uint64(pValue, s.f_files - s.f_ffree);
				break;
			case FS_FREE_INODES:
				ret_uint64(pValue, s.f_ffree);
				break;
			case FS_AVAIL_INODES:
				ret_uint64(pValue, s.f_favail);
				break;
			case FS_USED_INODES_PERC:
				ret_double(pValue, (s.f_files > 0) ? ((s.f_files - s.f_ffree) * 100.0 / s.f_files) : 0);
				break;
			case FS_FREE_INODES_PERC:
				ret_double(pValue, (s.f_files > 0) ? (s.f_ffree * 100.0 / s.f_files) : 0);
				break;
         case FS_AVAIL_INODES_PERC:
            ret_double(pValue, (s.f_files > 0) ? (s.f_favail * 100.0 / s.f_files) : 0);
            break;
         case FS_FSTYPE:
            ret_mbstring(pValue, s.f_basetype);
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
   char *buffer = (char *)MemAlloc(size);
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
      buffer = (char *)MemRealloc(buffer, size);
   }
   MemFree(buffer);
   return nullptr;
}

/**
 * Handler for FileSystem.Volumes table
 */
LONG H_FileSystems(const TCHAR *cmd, const TCHAR *arg, Table *table, AbstractCommSession *session)
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
#ifdef UNICODE
         table->setPreallocated(0, WideStringFromMBString(mountPoint));
         table->setPreallocated(1, WideStringFromMBString((char *)vmt2dataptr(curr, VMT_OBJECT)));
#else
         table->set(0, mountPoint);
         table->set(1, (char *)vmt2dataptr(curr, VMT_OBJECT));
#endif

         const TCHAR *fstype = nullptr;
         switch(curr->vmt_gfstype)
         {
            case MNT_AHAFS:
               fstype = _T("ahafs");
               break;
            case MNT_CACHEFS:
               fstype = _T("cachefs");
               break;
            case MNT_CDROM:
               fstype = _T("cdrom");
               break;
            case MNT_CIFS:
               fstype = _T("cifs");
               break;
            case MNT_J2:
               fstype = _T("jfs2");
               break;
            case MNT_JFS:
               fstype = _T("jfs");
               break;
            case MNT_NAMEFS:
               fstype = _T("namefs");
               break;
            case MNT_NFS:
               fstype = _T("nfs");
               break;
            case MNT_NFS3:
               fstype = _T("nfs3");
               break;
            case MNT_NFS4:
               fstype = _T("nfs4");
               break;
            case MNT_POOLFS:
               fstype = _T("poolfs");
               break;
            case MNT_PROCFS:
               fstype = _T("procfs");
               break;
            case MNT_UDF:
               fstype = _T("udfs");
               break;
            case MNT_VXFS:
               fstype = _T("vxfs");
               break;
            default:
               break;
         }

         struct statvfs s;
         if (statvfs(mountPoint, &s) == 0)
         {
            if (fstype != nullptr)
               table->set(3, fstype);
            else
               table->set(3, s.f_basetype);

            uint64_t usedBlocks = (s.f_blocks == (fsblkcnt_t)(-1)) ? _ULL(0) : (uint64_t)(s.f_blocks - s.f_bfree);
            uint64_t totalBlocks = (s.f_blocks == (fsblkcnt_t)(-1)) ? _ULL(0) : (uint64_t)s.f_blocks;
            uint64_t blockSize = (uint64_t)s.f_frsize;
            uint64_t freeBlocks = (uint64_t)s.f_bfree;
            uint64_t availableBlocks = (uint64_t)s.f_bavail;

            table->set(4, totalBlocks * blockSize);
            table->set(5, freeBlocks * blockSize);
            table->set(6, (totalBlocks > 0) ? (freeBlocks * 100.0) / totalBlocks : 0);
            table->set(7, availableBlocks * blockSize);
            table->set(8, (totalBlocks > 0) ? (availableBlocks * 100.0) / totalBlocks : 0);
            table->set(9, usedBlocks * blockSize);
            table->set(10, (totalBlocks > 0) ? (usedBlocks * 100.0) / totalBlocks : 0);
         }
         else
         {
            nxlog_debug_tag(AIX_DEBUG_TAG, 4, _T("H_FileSystems: Call to statvfs(\"%hs\") failed (%s)"), mountPoint, _tcserror(errno));

            if (fstype != nullptr)
               table->set(3, fstype);
            else
               table->set(3, (uint32_t)curr->vmt_gfstype);
            table->set(4, (uint64_t)0);
            table->set(5, (uint64_t)0);
            table->set(6, (uint64_t)0);
            table->set(7, (uint64_t)0);
            table->set(8, (uint64_t)0);
            table->set(9, (uint64_t)0);
            table->set(10, (uint64_t)0);
         }

         curr = (struct vmount *)((char *)curr + curr->vmt_length);
      }
      MemFree(mountPoints);
   }
   else
   {
      nxlog_debug_tag(AIX_DEBUG_TAG, 4, _T("H_FileSystems: Call to GetMountPoints failed"));
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

	int count;
	struct vmount *mountPoints = GetMountPoints(&count);
	if (mountPoints != NULL)
	{
	   struct vmount *curr = mountPoints;
	   for(int i = 0; i < count; i++)
	   {
#ifdef UNICODE
	      value->addPreallocated(WideStringFromMBString((char *)vmt2dataptr(curr, VMT_STUB)));
#else
	      value->add((char *)vmt2dataptr(curr, VMT_STUB));
#endif
	      curr = (struct vmount *)((char *)curr + curr->vmt_length);
	   }
	   free(mountPoints);
	}
	else
	{
		nxlog_debug_tag(AIX_DEBUG_TAG, 4, _T("H_MountPoints: Call to GetMountPoints failed"));
		rc = SYSINFO_RC_ERROR;
	}
	return rc;
}
