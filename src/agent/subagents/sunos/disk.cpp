/* $Id: disk.cpp,v 1.2 2006-05-15 22:11:22 alk Exp $ */

/*
 ** NetXMS subagent for SunOS/Solaris
 ** Copyright (C) 2004 Victor Kirhenshtein
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
 ** $module: disk.cpp
 **
 **/

#include "sunos_subagent.h"
#include <sys/statvfs.h>


//
// Disk used/free space information
//

LONG H_DiskInfo(char *pszParam, char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
	struct statvfs sv;
	char szPath[512] = "";

	NxGetParameterArg(pszParam, 1, szPath, sizeof(szPath));

	if ((szPath[0] != 0) && (statvfs(szPath, &sv) == 0))
	{
		nRet = SYSINFO_RC_SUCCESS;
		switch((int)pArg)
		{
			case DISK_FREE:
				ret_uint64(pValue, (QWORD)sv.f_bfree * (QWORD)sv.f_frsize);
				break;
			case DISK_TOTAL:
				ret_uint64(pValue, (QWORD)sv.f_blocks * (QWORD)sv.f_frsize);
				break;
			case DISK_USED:
				ret_uint64(pValue, (QWORD)(sv.f_blocks - sv.f_bfree) * (QWORD)sv.f_frsize);
				break;
			default: // YIC
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
