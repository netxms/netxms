/* $Id: system.cpp,v 1.3 2006-09-28 20:00:03 victor Exp $ */
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
** File: system.cpp
**
**/

#include "aix_subagent.h"
#include <sys/utsname.h>
#include <sys/vminfo.h>


//
// Hander for System.CPU.Count parameter
//

LONG H_CPUCount(char *pszParam, char *pArg, char *pValue)
{
	struct vario v;
	LONG nRet;

	if (sys_parm(SYSP_GET, SYSP_V_NCPUS_CFG, &v) == 0)
	{
		ret_int(pValue, v.v.v_ncpus_cfg.value);
		nRet = SYSINFO_RC_SUCCESS;
	}
	else
	{
		nRet = SYSINFO_RC_ERROR;
	}
	return nRet;
}


//
// Handler for System.Uname parameter
//

LONG H_Uname(char *pszParam, char *pArg, char *pValue)
{
	LONG nRet;
	struct utsname un;

	if (uname(&un) == 0)
	{
		sprintf(pValue, "%s %s %s %s %s", un.sysname, un.nodename, un.release,
			un.version, un.machine);
		nRet = SYSINFO_RC_SUCCESS;
	}
	else
	{
		nRet = SYSINFO_RC_ERROR;
	}
	return nRet;
}


//
// Handler for System.Uptime parameter
//

LONG H_Uptime(char *pszParam, char *pArg, char *pValue)
{
	LONG nRet = SYSINFO_RC_ERROR;
	return nRet;
}


//
// Handler for System.Hostname parameter
//

LONG H_Hostname(char *pszParam, char *pArg, char *pValue)
{
	LONG nRet;
	struct utsname un;

	if (uname(&un) == 0)
	{
		ret_string(pValue, un.nodename);
		nRet = SYSINFO_RC_SUCCESS;
	}
	else
	{
		nRet = SYSINFO_RC_ERROR;
	}
	return nRet;
}


//
// Handler for System.Memory.Physical.xxx parameters
//

LONG H_MemoryInfo(char *pszParam, char *pArg, char *pValue)
{
	struct vminfo vmi;
	LONG nRet;

	if (vmgetinfo(&vmi, VMINFO, sizeof(struct vminfo)) == 0)
	{
		switch((int)pArg)
		{
			case MEMINFO_PHYSICAL_FREE:
				ret_uint64(pValue, vmi.numfrb * getpagesize());
				break;
			case MEMINFO_PHYSICAL_USED:
				ret_uint64(pValue, (vmi.memsizepgs - vmi.numfrb) * getpagesize());
				break;
			case MEMINFO_PHYSICAL_TOTAL:
				ret_uint64(pValue, vmi.memsizepgs * getpagesize());
				break;
		}
		nRet = SYSINFO_RC_SUCCESS;
	}
	else
	{
		nRet = SYSINFO_RC_ERROR;
	}
	return nRet;
}
