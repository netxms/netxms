/* $Id: disk.cpp,v 1.1 2006-09-27 04:33:36 victor Exp $ */

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


//
// Handler for Disk.xxx parameters
//

LONG H_DiskInfo(char *pszParam, char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
	struct statvfs s;
	char szArg[512] = "";

	if (!NxGetParameterArg(pszParam, 1, szArg, sizeof(szArg)))
		return SYSINFO_RC_UNSUPPORTED;

	if (szArg[0] != 0 && statvfs(szArg, &s) == 0)
	{
		nRet = SYSINFO_RC_SUCCESS;
		switch((long)pArg)
		{
			case DISK_FREE:
				ret_uint64(pValue, (QWORD)s.f_bfree * (QWORD)s.f_bsize);
				break;
			case DISK_FREE_PERC:
				ret_double(pValue, 100.0 * s.f_bfree / s.f_blocks);
				break;
			case DISK_AVAIL:
				ret_uint64(pValue, (QWORD)s.f_bavail * (QWORD)s.f_bsize);
				break;
			case DISK_AVAIL_PERC:
				ret_double(pValue, 100.0 * s.f_bavail / s.f_blocks);
				break;
			case DISK_TOTAL:
				ret_uint64(pValue, (QWORD)s.f_blocks * (QWORD)s.f_frsize);
				break;
			case DISK_USED:
				ret_uint64(pValue, (QWORD)(s.f_blocks - s.f_bfree) * (QWORD)s.f_frsize);
				break;
			case DISK_USED_PERC:
				ret_double(pValue, 100.0 * (s.f_blocks - s.f_bfree) / s.f_blocks);
				break;
			default: // YIC
				nRet = SYSINFO_RC_ERROR;
				break;
		}
	}

	return nRet;
}
