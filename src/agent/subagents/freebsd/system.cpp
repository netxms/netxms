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

#undef _XOPEN_SOURCE

#if __FreeBSD__ < 8
#define _SYS_LOCK_PROFILE_H_	/* prevent include of sys/lock_profile.h which can be C++ incompatible) */
#endif

#include <nms_common.h>
#include <nms_agent.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/utsname.h>
#include <sys/param.h>

#if __FreeBSD__ < 5
#include <sys/proc.h>
#endif

#include <sys/user.h>
#include <fcntl.h>
#include <kvm.h>
#include <paths.h>


#include "system.h"

LONG H_Uptime(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
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

LONG H_Uname(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
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

   	ret_mbstring(pValue, szBuff);

		nRet = SYSINFO_RC_SUCCESS;
	}

   return nRet;
}

LONG H_Hostname(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
	int nRet = SYSINFO_RC_ERROR;
	char szBuff[128];

	if (gethostname(szBuff, sizeof(szBuff)) == 0)
	{
   	ret_mbstring(pValue, szBuff);
		nRet = SYSINFO_RC_SUCCESS;
	}

   return nRet;
}

LONG H_CpuLoad(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
	int nRet = SYSINFO_RC_ERROR;
   char szArg[128] = {0};
	FILE *hFile;
	double dLoad[3];

	// get processor
	//AgentGetParameterArg(pszParam, 1, szArg, sizeof(szArg));

	if (getloadavg(dLoad, 3) == 3)
	{
		switch (pszParam[18])
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

LONG H_CpuUsage(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
	return SYSINFO_RC_UNSUPPORTED;
}

LONG H_CpuCount(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
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

LONG H_MemoryInfo(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
	int nRet = SYSINFO_RC_ERROR;
	FILE *hFile;
	int nPageCount, nFreeCount;
	int64_t nSwapTotal, nSwapUsed;
	char *pTag;
	int mib[4];
	size_t nSize;
	int i;
	char *pOid;
	int nPageSize;
	kvm_t *kd;
	int type = CAST_FROM_POINTER(pArg, int);
#if HAVE_KVM_GETSWAPINFO
	struct kvm_swap swap[16];
#endif

	nPageCount = nFreeCount = 0;
	nSwapTotal = nSwapUsed = 0;

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

	nPageSize = getpagesize();

	// Phisical memory

	DOIT("vm.stats.vm.v_page_count", nPageCount);

	if (nRet == SYSINFO_RC_SUCCESS)
	{
		nRet = SYSINFO_RC_ERROR;
		DOIT("vm.stats.vm.v_free_count", nFreeCount);
	}

#undef DOIT

	// Swap
#if HAVE_KVM_GETSWAPINFO
	kd = kvm_open(NULL, NULL, NULL, O_RDONLY, "kvm_open");
	if (kd != NULL)
	{
		i = kvm_getswapinfo(kd, swap, 16, 0);
		while (i > 0)
		{
			i--;
			nSwapTotal += swap[i].ksw_total;
			nSwapUsed += swap[i].ksw_used;
		}
		kvm_close(kd);
	}
	else
	{
		if ((type != PHYSICAL_FREE) &&
		    (type != PHYSICAL_TOTAL) &&
		    (type != PHYSICAL_USED))
		nRet = SYSINFO_RC_ERROR;
	}
#endif

	if (nRet == SYSINFO_RC_SUCCESS)
	{
		switch(type)
		{
			case PHYSICAL_FREE: // ph-free
				ret_uint64(pValue, ((int64_t)nFreeCount) * nPageSize);
				break;
			case PHYSICAL_FREE_PCT: // ph-free %
				ret_uint(pValue, (DWORD)(((int64_t)nFreeCount * 100) / (int64_t)nPageCount));
				break;
			case PHYSICAL_TOTAL: // ph-total
				ret_uint64(pValue, ((int64_t)nPageCount) * nPageSize);
				break;
			case PHYSICAL_USED: // ph-used
				ret_uint64(pValue, ((int64_t)(nPageCount - nFreeCount)) * nPageSize);
				break;
			case PHYSICAL_USED_PCT: // ph-used %
				ret_uint(pValue, (DWORD)(((int64_t)(nPageCount - nFreeCount) * 100) / (int64_t)nPageCount));
				break;
			case SWAP_FREE: // sw-free
				ret_uint64(pValue, (nSwapTotal - nSwapUsed) * nPageSize);
				break;
			case SWAP_FREE_PCT:
				if (nSwapTotal > 0)
					ret_uint(pValue, (DWORD)(((nSwapTotal - nSwapUsed) * 100) / nSwapTotal));
				else
					ret_uint(pValue, 100);
				break;
			case SWAP_TOTAL: // sw-total
				ret_uint64(pValue, nSwapTotal * nPageSize);
				break;
			case SWAP_USED: // sw-used
				ret_uint64(pValue, nSwapUsed * nPageSize);
				break;
			case SWAP_USED_PCT:
				if (nSwapTotal > 0)
					ret_uint(pValue, (DWORD)((nSwapUsed * 100) / nSwapTotal));
				else
					ret_uint(pValue, 0);
				break;
			case VIRTUAL_FREE: // vi-free
				ret_uint64(pValue, ((nSwapTotal - nSwapUsed) * nPageSize) +
						(((int64_t)nFreeCount) * nPageSize));
				break;
			case VIRTUAL_FREE_PCT:
				ret_uint(pValue, (DWORD)(((int64_t)nFreeCount + (nSwapTotal - nSwapUsed) * 100) / ((int64_t)nPageCount + nSwapTotal)));
				break;
			case VIRTUAL_TOTAL: // vi-total
				ret_uint64(pValue, (nSwapTotal * nPageSize) +
						(((int64_t)nPageCount) * nPageSize));
				break;
			case VIRTUAL_USED: // vi-used
				ret_uint64(pValue, (nSwapUsed * nPageSize) +
						(((int64_t)(nPageCount - nFreeCount)) * nPageSize));
				break;
			case VIRTUAL_USED_PCT:
				ret_uint(pValue, (DWORD)((((int64_t)(nPageCount - nFreeCount) + nSwapUsed) * 100) / ((int64_t)nPageCount + nSwapTotal)));
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
LONG H_SourcePkgSupport(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
	ret_int(pValue, 1);
	return SYSINFO_RC_SUCCESS;
}
