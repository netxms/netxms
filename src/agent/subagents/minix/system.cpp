/* 
** NetXMS subagent for NetBSD
** Copyright (C) 2006 C.T.Co 
** Copyright (C) 2008 Mark Ibell
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

#include <sys/time.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/utsname.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/vmmeter.h>
#include <fcntl.h>
#include <kvm.h>
#include <paths.h>


#include "system.h"

LONG H_Uptime(const char *pszParam, const char *pArg, char *pValue, AbstractCommSession *session)
{
	int mib[2] = { CTL_KERN, KERN_BOOTTIME };
	time_t nNow;
	size_t nSize;
	struct timeval bootTime;
	time_t nUptime = 0;

	nSize = sizeof(bootTime);

	time(&nNow);
	if (sysctl(mib, 2, &bootTime, &nSize, NULL, 0) != (-1))
	{
		nUptime = (time_t)(nNow - bootTime.tv_sec);
	}
	else
	{
		perror("uptime()");
	}

	if (nUptime > 0)
	{
   	ret_uint(pValue, nUptime);
	}

   return nUptime > 0 ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
}

LONG H_Uname(const char *pszParam, const char *pArg, char *pValue, AbstractCommSession *session)
{
	struct utsname utsName;
	int nRet = SYSINFO_RC_ERROR;

	if (uname(&utsName) == 0)
	{
		char szBuff[1024];

		snprintf(szBuff, sizeof(szBuff), "%s %s %s %s %s", utsName.sysname,
				utsName.nodename, utsName.release, utsName.version,
				utsName.machine);
		// TODO: processor & platform

   		ret_string(pValue, szBuff);

		nRet = SYSINFO_RC_SUCCESS;
	}

   return nRet;
}

LONG H_Hostname(const char *pszParam, const char *pArg, char *pValue, AbstractCommSession *session)
{
	int nRet = SYSINFO_RC_ERROR;
	char szBuff[128];

	if (gethostname(szBuff, sizeof(szBuff)) == 0)
	{
   	ret_string(pValue, szBuff);
		nRet = SYSINFO_RC_SUCCESS;
	}

   return nRet;
}

LONG H_CpuLoad(const char *pszParam, const char *pArg, char *pValue, AbstractCommSession *session)
{
	int nRet = SYSINFO_RC_ERROR;
   	char szArg[128] = {0};
	FILE *hFile;
	double dLoad[3];

	// get processor
   	//AgentGetParameterArg(pszParam, 1, szArg, sizeof(szArg));

	if (getloadavg(dLoad, 3) == 3)
	{
		switch (pszParam[19])
		{
		case '1': // 15 min
			ret_double(pValue, dLoad[2]);
			break;
		case '5': // 5 min
			ret_double(pValue, dLoad[1]);
			break;
		default: // 1 min
			ret_double(pValue, dLoad[0]);
			break;
		}
		nRet = SYSINFO_RC_SUCCESS;
	}

	return nRet;
}

LONG H_CpuUsage(const char *pszParam, const char *pArg, char *pValue, AbstractCommSession *session)
{
	return SYSINFO_RC_UNSUPPORTED;
}

LONG H_CpuCount(const char *pszParam, const char *pArg, char *pValue, AbstractCommSession *session)
{
	int nRet = SYSINFO_RC_ERROR;
	int mib[] = {CTL_HW, HW_NCPU};
	size_t  nValSize;
	int nVal;

	nValSize = sizeof(nVal);
	if (sysctl(mib, 2, &nVal, &nValSize, NULL, 0) == 0)
	{
   		ret_int(pValue, nVal);
		nRet = SYSINFO_RC_SUCCESS;
	}

	return nRet;
}

LONG H_MemoryInfo(const char *pszParam, const char *pArg, char *pValue, AbstractCommSession *session)
{
	int nRet = SYSINFO_RC_ERROR;
	FILE *hFile;
	int nUsedPhysCount, nFreePhysCount;
	int nUsedVirtCount;
	int64_t nSwapTotal, nSwapUsed;
	char *pTag;
	int mib[] = {CTL_VM, VM_METER};
	int i;
	char *pOid;
	char szArg[16] = {0};
	kvm_t *kd;
	struct vmtotal vm;
	size_t nValSize = sizeof(vmtotal);

	nUsedPhysCount = nFreePhysCount = 0;
	nUsedVirtCount = 0;
	nSwapTotal = nSwapUsed = 0;

   	AgentGetParameterArg(pszParam, 1, szArg, sizeof(szArg));

	// Physical memory

        if (sysctl(mib, 2, &vm, &nValSize, NULL, 0) == -1)
        	perror("VM_METER : sysctl()");
	else
	{
		nUsedPhysCount = vm.t_rm;	// physical memory in use
		nFreePhysCount = vm.t_free; 	// free physical memory
		nUsedVirtCount = vm.t_vm; 	// virtual memory in use
		nRet = SYSINFO_RC_SUCCESS;
	}

#define ctoqb(x)	ctob(int64_t(x))
#define dbtoqb(x)	dbtob(int64_t(x))

	if (nRet == SYSINFO_RC_SUCCESS)
	{
		switch((int)pArg)
		{
		case PHYSICAL_FREE: // ph-free
			ret_uint64(pValue, ctoqb(nFreePhysCount));
			break;
		case PHYSICAL_TOTAL: // ph-total
			ret_uint64(pValue, ctoqb(nFreePhysCount + nUsedPhysCount));
			break;
		case PHYSICAL_USED: // ph-used
			ret_uint64(pValue, ctoqb(nUsedPhysCount));
			break;
		default: // error
			nRet = SYSINFO_RC_ERROR;
			break;
		}
	}

	return nRet;
}

//
// stub
//
LONG H_SourcePkgSupport(const char *pszParam, const char *pArg, char *pValue, AbstractCommSession *session)
{
	ret_int(pValue, 1);
	return SYSINFO_RC_SUCCESS;
}
