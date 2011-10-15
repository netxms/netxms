/* 
** NetXMS subagent for GNU/Linux
** Copyright (C) 2004 - 2007 Alex Kirhenshtein
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


static void findMountpointByDevice(char *dev, int size)
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

LONG H_DiskInfo(const char *pszParam, const char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
	struct statfs s;
	char szArg[512] = {0};

	AgentGetParameterArg(pszParam, 1, szArg, sizeof(szArg));

	if (szArg[0] != 0)
	{
		findMountpointByDevice(szArg, sizeof(szArg));

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
	}

	return nRet;
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.6  2007/09/27 09:16:41  alk
disk usage fixed (%), should work for up to 1T mountpoints

Revision 1.5  2007/09/17 18:55:04  alk
freespace(%) fixed for large partitions

Revision 1.4  2007/04/24 12:04:10  alk
code reformat

Revision 1.3  2006/03/02 12:17:05  victor
Removed various warnings related to 64bit platforms

Revision 1.2  2005/08/19 15:23:50  victor
Added new parameters

Revision 1.1  2004/10/22 22:08:34  alk
source restructured;
implemented:
	Net.IP.Forwarding
	Net.IP6.Forwarding
	Process.Count(*)
	Net.ArpCache
	Net.InterfaceList (if-type not implemented yet)
	System.ProcessList


*/
