/* $Id: disk.cpp,v 1.2 2007-04-18 20:26:29 victor Exp $ */

/* 
** NetXMS subagent for FreeBSD
** Copyright (C) 2004 Alex Kirhenshtein
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

#include <nms_common.h>
#include <nms_agent.h>

#include <sys/param.h>
#include <sys/mount.h>

#include "disk.h"

LONG H_DiskInfo(char *pszParam, char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
   char szArg[512] = {0};
	struct statfs s;

   NxGetParameterArg(pszParam, 1, szArg, sizeof(szArg));

	if (szArg[0] != 0 && statfs(szArg, &s) == 0)
	{
		nRet = SYSINFO_RC_SUCCESS;
		switch((int)pArg)
		{
		case DISK_AVAIL:
			ret_uint64(pValue, (QWORD)s.f_bavail * (QWORD)s.f_bsize);
			break;
		case DISK_AVAIL_PERC:
			ret_double(pValue, 100.0 * s.f_bavail / s.f_blocks);
			break;
		case DISK_FREE:
			ret_uint64(pValue, (QWORD)s.f_bfree * (QWORD)s.f_bsize);
			break;
		case DISK_FREE_PERC:
			ret_double(pValue, 100.0 * s.f_bfree / s.f_blocks);
			break;
		case DISK_TOTAL:
			ret_uint64(pValue, (QWORD)s.f_blocks * (QWORD)s.f_bsize);
			break;
		case DISK_USED:
			ret_uint64(pValue, (QWORD)(s.f_blocks - s.f_bfree) * (QWORD)s.f_bsize);
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

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.1  2005/01/17 17:14:32  alk
freebsd agent, incomplete (but working)

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
