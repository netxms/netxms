/* 
** NetXMS subagent for GNU/Linux
** Copyright (C) 2004-2022 Raden Solutions
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

#include "linux_subagent.h"
#include <mntent.h>
#include <paths.h>
#include <sys/vfs.h>

/**
 * Find mount point by device
 */
static void FindMountpointByDevice(char *dev, size_t size)
{
	FILE *f = setmntent(_PATH_MOUNTED, "r");
	if (f == nullptr)
	   return;

	struct mntent *mnt, mntbuffer;
	char textbuffer[4096];
   while ((mnt = getmntent_r(f, &mntbuffer, textbuffer, sizeof(textbuffer))) != nullptr)
   {
      if (!strcmp(mnt->mnt_fsname, dev))
      {
         strlcpy(dev, mnt->mnt_dir, size);
         break;
      }
   }
   endmntent(f);
}

/**
 * Handler for FileSystem.* parameters
 */
LONG H_FileSystemInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   char mountPoint[512] = "";
   AgentGetParameterArgA(param, 1, mountPoint, sizeof(mountPoint));
   if (mountPoint[0] == 0)
      return SYSINFO_RC_UNSUPPORTED;

   FindMountpointByDevice(mountPoint, sizeof(mountPoint));

   struct statfs s;
   if (statfs(mountPoint, &s) != 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("H_FileSystemInfo: Call to statfs(\"%hs\") failed (%hs)"), mountPoint, strerror(errno));
      return (errno == ENOENT) ? SYSINFO_RC_NO_SUCH_INSTANCE : SYSINFO_RC_ERROR;
   }

   uint64_t usedBlocks = static_cast<uint64_t>(s.f_blocks - s.f_bfree);
   uint64_t totalBlocks = static_cast<uint64_t>(s.f_blocks);
   uint64_t blockSize = static_cast<uint64_t>(s.f_bsize);
   uint64_t freeBlocks = static_cast<uint64_t>(s.f_bfree);
   uint64_t availableBlocks = static_cast<uint64_t>(s.f_bavail);

   nxlog_debug_tag(DEBUG_TAG, 6, _T("H_FileSystemInfo: statfs(\"%hs\"): blockSize=") UINT64_FMT _T(" totalBlocks=") UINT64_FMT _T(" freeBlocks=") UINT64_FMT _T(" availableBlocks=") UINT64_FMT,
      mountPoint, blockSize, totalBlocks, freeBlocks, availableBlocks);

   switch(CAST_FROM_POINTER(arg, int))
   {
      case DISK_TOTAL:
         ret_uint64(value, totalBlocks * blockSize);
         break;
      case DISK_USED:
         ret_uint64(value, usedBlocks * blockSize);
         break;
      case DISK_FREE:
         ret_uint64(value, freeBlocks * blockSize);
         break;
      case DISK_AVAIL:
         ret_uint64(value, availableBlocks * blockSize);
         break;
      case DISK_USED_PERC:
         if (totalBlocks > 0)
         {
            ret_double(value, (usedBlocks * 100.0) / totalBlocks);
         }
         else
         {
            ret_double(value, 0.0);
         }
         break;
      case DISK_AVAIL_PERC:
         if (totalBlocks > 0)
         {
            ret_double(value, (availableBlocks * 100.0) / totalBlocks);
         }
         else
         {
            ret_double(value, 0.0);
         }
         break;
      case DISK_FREE_PERC:
         if (totalBlocks > 0)
         {
            ret_double(value, (freeBlocks * 100.0) / totalBlocks);
         }
         else
         {
            ret_double(value, 0.0);
         }
         break;
      case DISK_TOTAL_INODES:
         ret_uint64(value, s.f_files);
         break;
      case DISK_FREE_INODES:
      case DISK_AVAIL_INODES:
         ret_uint64(value, s.f_ffree);
         break;
      case DISK_FREE_INODES_PERC:
      case DISK_AVAIL_INODES_PERC:
         ret_double(value, (s.f_files > 0) ? s.f_ffree * 100.0 / s.f_files : 0);
         break;
      case DISK_USED_INODES:
         ret_uint64(value, s.f_files - s.f_ffree);
         break;
      case DISK_USED_INODES_PERC:
         ret_double(value, (s.f_files > 0) ? (s.f_files - s.f_ffree) * 100.0 / s.f_files : 0);
         break;
      default:
         return SYSINFO_RC_UNSUPPORTED;
   }
	return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for FileSystem.Volumes table
 */
LONG H_FileSystems(const TCHAR *cmd, const TCHAR *arg, Table *table, AbstractCommSession *session)
{
   LONG rc = SYSINFO_RC_SUCCESS;

   FILE *in = fopen("/etc/mtab", "r");
   if (in != nullptr)
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

      while(1)
      {
         char line[4096];
         if (fgets(line, 4096, in) == nullptr)
            break;
         if (!strncmp(line, "rootfs /", 8))
            continue;

         table->addRow();

         char device[512], mountPoint[512], fsType[256];
         const char *next = ExtractWordA(line, device);
         next = ExtractWordA(next, mountPoint);
         ExtractWordA(next, fsType);

#ifdef UNICODE
         table->setPreallocated(0, WideStringFromMBString(mountPoint));
         table->setPreallocated(1, WideStringFromMBString(device));
         table->setPreallocated(3, WideStringFromMBString(fsType));
#else
         table->set(0, mountPoint);
         table->set(1, device);
         table->set(3, fsType);
#endif

         struct statfs s;
         if (statfs(mountPoint, &s) == 0)
         {
            uint64_t usedBlocks = static_cast<uint64_t>(s.f_blocks - s.f_bfree);
            uint64_t totalBlocks = static_cast<uint64_t>(s.f_blocks);
            uint64_t blockSize = static_cast<uint64_t>(s.f_bsize);
            uint64_t freeBlocks = static_cast<uint64_t>(s.f_bfree);
            uint64_t availableBlocks = static_cast<uint64_t>(s.f_bavail);

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
            nxlog_debug_tag(DEBUG_TAG, 4, _T("H_FileSystems: Call to statfs(\"%hs\") failed (%hs)"), mountPoint, strerror(errno));

            table->set(4, static_cast<uint64_t>(0));
            table->set(5, static_cast<uint64_t>(0));
            table->set(6, static_cast<uint64_t>(0));
            table->set(7, static_cast<uint64_t>(0));
            table->set(8, static_cast<uint64_t>(0));
            table->set(9, static_cast<uint64_t>(0));
            table->set(10, static_cast<uint64_t>(0));
         }
      }
      fclose(in);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("H_FileSystems: cannot open /etc/mtab"));
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

   FILE *in = fopen("/etc/mtab", "r");
   if (in != nullptr)
   {
      while(true)
      {
         char line[4096];
         if (fgets(line, 4096, in) == nullptr)
            break;
         if (!strncmp(line, "rootfs /", 8))
            continue;
         char *ptr = strchr(line, ' ');
         if (ptr != nullptr)
         {
            ptr++;
            char *mp = ptr;
            ptr = strchr(mp, ' ');
            if (ptr != nullptr)
               *ptr = 0;
#ifdef UNICODE
            value->addPreallocated(WideStringFromMBString(mp));
#else
            value->add(mp);
#endif
         }
      }
      fclose(in);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("H_MountPoints: cannot open /etc/mtab"));
      rc = SYSINFO_RC_ERROR;
   }
   return rc;
}

/**
 * Handler for FileSystem.Type(*)
 */
LONG H_FileSystemType(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   char path[MAX_PATH];
   if (!AgentGetParameterArgA(cmd, 1, path, MAX_PATH))
      return SYSINFO_RC_UNSUPPORTED;
   
   LONG rc = SYSINFO_RC_NO_SUCH_INSTANCE;

	FILE *f = setmntent(_PATH_MOUNTED, "r");
	if (f != nullptr)
   {
	   struct mntent *mnt, mntbuffer;
	   char textbuffer[4096];
		while((mnt = getmntent_r(f, &mntbuffer, textbuffer, sizeof(textbuffer))) != nullptr)
		{
		   if (!strcmp(mnt->mnt_type, "rootfs"))
		      continue;  // ignore rootfs entries

			if (!strcmp(mnt->mnt_fsname, path) || !strcmp(mnt->mnt_dir, path))
			{
            ret_mbstring(value, mnt->mnt_type);
            rc = SYSINFO_RC_SUCCESS;
				break;
			}
		}
		endmntent(f);
	}
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("H_FileSystemType: call to setmntent failed"));
      rc = SYSINFO_RC_ERROR;
   }
   return rc;
}
