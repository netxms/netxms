/* $Id: system.cpp,v 1.3 2005-01-23 05:08:06 alk Exp $ */

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

#include <sys/time.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/utsname.h>
#include <sys/param.h>
#include <sys/user.h>

#include "system.h"

LONG H_Uptime(char *pszParam, char *pArg, char *pValue)
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

LONG H_Uname(char *pszParam, char *pArg, char *pValue)
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

LONG H_Hostname(char *pszParam, char *pArg, char *pValue)
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

LONG H_CpuLoad(char *pszParam, char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
   char szArg[128] = {0};
	FILE *hFile;
	double dLoad[3];

	// get processor
   //NxGetParameterArg(pszParam, 1, szArg, sizeof(szArg));

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

LONG H_CpuUsage(char *pszParam, char *pArg, char *pValue)
{
	return SYSINFO_RC_UNSUPPORTED;
}

LONG H_CpuCount(char *pszParam, char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
	int mib[2];
	size_t nSize = sizeof(mib), nValSize;
	int nVal;

	if (sysctlnametomib("hw.ncpu", mib, &nSize) != 0)
	{
		return SYSINFO_RC_ERROR;
	}

	nValSize = sizeof(nVal);
	if (sysctl(mib, nSize, &nVal, &nValSize, NULL, 0) == 0)
	{
   	ret_int(pValue, nVal);
		nRet = SYSINFO_RC_SUCCESS;
	}

	return nRet;
}

LONG H_ProcessCount(char *pszParam, char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
//	struct statvfs s;
   char szArg[128] = {0};
	int nCount = -1;
	struct kinfo_proc *pKInfo;
	int mib[3];
	size_t nSize;
	int i;

   NxGetParameterArg(pszParam, 1, szArg, sizeof(szArg));

	nSize = sizeof(mib);
	if (sysctlnametomib("kern.proc.all", mib, &nSize) == 0)
	{
		if (sysctl(mib, 3, NULL, &nSize, NULL, 0) == 0)
		{
			{
				pKInfo = (struct kinfo_proc *)malloc(nSize);
				if (pKInfo != NULL)
				{
					if (sysctl(mib, 3, pKInfo, &nSize, NULL, 0) == 0)
					{
						nCount = 0;
						for (i = 0; i < (nSize / sizeof(struct kinfo_proc)); i++)
						{
							if (szArg[0] == 0 ||
									strcasecmp(pKInfo[i].kp_proc.p_comm, szArg) == 0)
							{
								nCount++;
							}
						}
					}
					free(pKInfo);
				}
			}
		}
	}

	if (nCount >= 0)
	{
		ret_int(pValue, nCount);
		nRet = SYSINFO_RC_SUCCESS;
	}

	return nRet;
}

LONG H_MemoryInfo(char *pszParam, char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
	FILE *hFile;
	int nPageCount, nFreeCount;
	char *pTag;
	int mib[4];
	size_t nSize;
	int i;
	char *pOid;
	int nPageSize;
	char szArg[16] = {0};

	nPageCount = nFreeCount = 0;

   NxGetParameterArg(pszParam, 1, szArg, sizeof(szArg));

#define DOIT(x, y) { \
	nSize = sizeof(mib); \
	if (sysctlnametomib(x, mib, &nSize) == 0) { \
		size_t __nTmp = sizeof(y); \
		if (sysctl(mib, nSize, &y, &__nTmp, NULL, 0) == 0) { \
			nRet = SYSINFO_RC_SUCCESS; \
		} else { \
			perror(x ": sysctl()"); \
		} \
	} else { \
		perror(x ": sysctlnametomib()"); \
	} \
}

	DOIT("hw.pagesize", nPageSize);

	if (nRet == SYSINFO_RC_SUCCESS)
	{
		nRet = SYSINFO_RC_ERROR;
		DOIT("vm.stats.vm.v_page_count", nPageCount);
	}

	if (nRet == SYSINFO_RC_SUCCESS)
	{
		nRet = SYSINFO_RC_ERROR;
		DOIT("vm.stats.vm.v_free_count", nFreeCount);
	}

#undef DOIT

	if (nRet == SYSINFO_RC_SUCCESS)
	{
		switch((int)pArg)
		{
		case PHYSICAL_FREE: // ph-free
			ret_uint64(pValue, ((int64_t)nFreeCount) * nPageSize);
			break;
		case PHYSICAL_TOTAL: // ph-total
			ret_uint64(pValue, ((int64_t)nPageCount) * nPageSize);
			break;
		case PHYSICAL_USED: // ph-used
			ret_uint64(pValue, ((int64_t)(nPageCount - nFreeCount)) * nPageSize);
			break;
		case SWAP_FREE: // sw-free
			ret_uint64(pValue, 0);
			break;
		case SWAP_TOTAL: // sw-total
			ret_uint64(pValue, 0);
			break;
		case SWAP_USED: // sw-used
			ret_uint64(pValue, 0);
			break;
		case VIRTUAL_FREE: // vi-free
			ret_uint64(pValue, 0);
			break;
		case VIRTUAL_TOTAL: // vi-total
			ret_uint64(pValue, 0);
			break;
		case VIRTUAL_USED: // vi-used
			ret_uint64(pValue, 0);
			break;
		default: // error
			nRet = SYSINFO_RC_ERROR;
			break;
		}
	}

	return nRet;
}

LONG H_ProcessList(char *pszParam, char *pArg, NETXMS_VALUES_LIST *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
	int nCount = -1;
	struct kinfo_proc *pKInfo;
	int mib[3];
	size_t nSize;
	int i;

	nSize = sizeof(mib);
	if (sysctlnametomib("kern.proc.all", mib, &nSize) == 0)
	{
		if (sysctl(mib, 3, NULL, &nSize, NULL, 0) == 0)
		{
			{
				pKInfo = (struct kinfo_proc *)malloc(nSize);
				if (pKInfo != NULL)
				{
					if (sysctl(mib, 3, pKInfo, &nSize, NULL, 0) == 0)
					{
						nCount = 0;
						for (i = 0; i < (nSize / sizeof(struct kinfo_proc)); i++)
						{
							char szBuff[128];

							snprintf(szBuff, sizeof(szBuff), "%d %s",
									pKInfo[i].kp_proc.p_pid, pKInfo[i].kp_proc.p_comm);
							NxAddResultString(pValue, szBuff);

							nCount++;
						}
					}
					free(pKInfo);
				}
			}
		}
	}

	if (nCount >= 0)
	{
		nRet = SYSINFO_RC_SUCCESS;
	}

	return nRet;
}

//
// stub
//
LONG H_SourcePkgSupport(char *pszParam, char *pArg, char *pValue)
{
	ret_int(pValue, 1);
	return SYSINFO_RC_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.2  2005/01/17 23:25:47  alk
Agent.SourcePackageSupport added

Revision 1.1  2005/01/17 17:14:32  alk
freebsd agent, incomplete (but working)

Revision 1.1  2004/10/22 22:08:35  alk
source restructured;
implemented:
	Net.IP.Forwarding
	Net.IP6.Forwarding
	Process.Count(*)
	Net.ArpCache
	Net.InterfaceList (if-type not implemented yet)
	System.ProcessList


*/
