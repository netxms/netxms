/* $Id: disk.cpp,v 1.1 2006-10-04 14:59:13 alk Exp $ */

/* 
** NetXMS subagent for HP-UX
** Copyright (C) 2006 Alex Kirhenshtein
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
*/

#include <nms_common.h>
#include <nms_agent.h>

#include <sys/statvfs.h>

#include "disk.h"

LONG H_DiskInfo(char *pszParam, char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
	struct statvfs s;
	char szArg[512] = {0};

	NxGetParameterArg(pszParam, 1, szArg, sizeof(szArg));

	if (szArg[0] != 0 && statvfs(szArg, &s) == 0)
	{
		nRet = SYSINFO_RC_SUCCESS;

		switch((long)pArg)
		{
			case DISK_FREE:
				ret_uint64(pValue, (QWORD)s.f_bfree * (QWORD)s.f_frsize);
				break;
			case DISK_FREE_PERC:
				ret_double(pValue, (double)s.f_bfree / ((double)s.f_size / 100));
				break;
			case DISK_AVAIL:
				ret_uint64(pValue, (QWORD)s.f_bavail * (QWORD)s.f_frsize);
				break;
			case DISK_AVAIL_PERC:
				ret_double(pValue, (double)s.f_bavail / ((double)s.f_size / 100));
				break;
			case DISK_TOTAL:
				ret_uint64(pValue, (QWORD)s.f_size * (QWORD)s.f_frsize);
				break;
			case DISK_USED:
				ret_uint64(pValue, (QWORD)(s.f_size - s.f_bfree) * (QWORD)s.f_frsize);
				break;
			case DISK_USED_PERC:
				ret_double(pValue, (double)(s.f_size - s.f_bfree) / ((double)s.f_size / 100));
				break;
			default: // JIC
				nRet = SYSINFO_RC_ERROR;
				break;
		}
	}

	return nRet;
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $

*/
