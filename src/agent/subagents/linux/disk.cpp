/* 
** NetXMS subagent for GNU/Linux
** Copyright (C) 2004-2013 Raden Solutions
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

/**
 * Find mount point by device
 */
static void FindMountpointByDevice(char *dev, int size)
{
	char *name;
	struct mntent *mnt;
	FILE *f;

	f = setmntent(_PATH_MOUNTED, "r");
	if (f != NULL) {
		while ((mnt = getmntent(f)) != NULL)
		{
			if (!strcmp(mnt->mnt_fsname, dev))
			{
				strncpy(dev, mnt->mnt_dir, size);
				break;
			}
		}
		endmntent(f);
	}
}

/**
 * Handler for FileSystem.* parameters
 */
LONG H_DiskInfo(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
	char szArg[512] = {0};

	AgentGetParameterArgA(pszParam, 1, szArg, sizeof(szArg));

	if (szArg[0] != 0)
	{
		FindMountpointByDevice(szArg, sizeof(szArg));

	   struct statfs s;
		if (statfs(szArg, &s) == 0)
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
               if (totalBlocks > 0)
               {
                  ret_double(pValue, (usedBlocks * 100) / totalBlocks);
               }
               else
               {
                  ret_double(pValue, 0.0);
               }
					break;
				case DISK_AVAIL_PERC:
               if (totalBlocks > 0)
               {
                  ret_double(pValue, (availableBlocks * 100) / totalBlocks);
               }
               else
               {
                  ret_double(pValue, 0.0);
               }
					break;
				case DISK_FREE_PERC:
               if (totalBlocks > 0)
               {
                  ret_double(pValue, (freeBlocks * 100) / totalBlocks);
               }
               else
               {
                  ret_double(pValue, 0.0);
               }
					break;
				default:
					nRet = SYSINFO_RC_ERROR;
					break;
			}
		}
	}

	return nRet;
}

/**
 * Handler for FileSystem.Volumes table
 */
LONG H_FileSystems(const TCHAR *cmd, const TCHAR *arg, Table *table)
{
   LONG rc = SYSINFO_RC_SUCCESS;

   FILE *in = fopen("/etc/mtab", "r");
   if (in != NULL)
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

      while(1)
      {
         char line[256];
         if (fgets(line, 256, in) == NULL)
            break;

         table->addRow();

         char device[256], mountPoint[256], fsType[256];
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
            AgentWriteDebugLog(4, _T("Linux: H_FileSystems: Call to statfs(\"%hs\") failed (%hs)"), mountPoint, strerror(errno));

            table->set(4, (QWORD)0);
            table->set(5, (QWORD)0);
            table->set(6, (QWORD)0);
            table->set(7, (QWORD)0);
            table->set(8, (QWORD)0);
            table->set(9, (QWORD)0);
            table->set(10, (QWORD)0);
         }
      }
      fclose(in);
   }
   else
   {
      AgentWriteDebugLog(4, _T("Linux: H_FileSystems: cannot open /etc/mtab"));
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

   FILE *in = fopen("/etc/mtab", "r");
   if (in != NULL)
   {
      while(1)
      {
         char line[256];
         if (fgets(line, 256, in) == NULL)
            break;
         char *ptr = strchr(line, ' ');
         if (ptr != NULL)
         {
            ptr++;
            char *mp = ptr;
            ptr = strchr(mp, ' ');
            if (ptr != NULL)
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
      AgentWriteDebugLog(4, _T("Linux: H_MountPoints: cannot open /etc/mtab"));
      rc = SYSINFO_RC_ERROR;
   }
   return rc;
}
