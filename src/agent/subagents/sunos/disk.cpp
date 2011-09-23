/* $Id$ */

/*
 ** NetXMS subagent for SunOS/Solaris
 ** Copyright (C) 2004, 2005, 2006, 2007 Victor Kirhenshtein
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
 ** File: disk.cpp
 **
 **/

#include "sunos_subagent.h"
#include <sys/statvfs.h>


//
// Disk used/free space information
//

LONG H_DiskInfo(const char *pszParam, const char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
	struct statvfs sv;
	char szPath[512] = "";

	AgentGetParameterArg(pszParam, 1, szPath, sizeof(szPath));

	if ((szPath[0] != 0) && (statvfs(szPath, &sv) == 0))
	{
		nRet = SYSINFO_RC_SUCCESS;
		
		QWORD usedBlocks = (QWORD)(sv.f_blocks - sv.f_bfree);
		QWORD totalBlocks = (QWORD)sv.f_blocks;
		QWORD blockSize = (QWORD)sv.f_bsize;
		QWORD freeBlocks = (QWORD)sv.f_bfree;
		QWORD availableBlocks = (QWORD)sv.f_bavail;
		
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
