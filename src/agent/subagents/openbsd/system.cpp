/* 
** NetXMS subagent for OpenBSD
** Copyright (C) 2006 C.T.Co 
** Copyright (C) 2017 Raden Solutions SIA
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
#include <sys/swap.h>
#include <sys/vmmeter.h>
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

LONG H_CpuUsage(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
	return SYSINFO_RC_UNSUPPORTED;
}

LONG H_CpuCount(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
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

LONG H_ProcessCount(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
	int nRet = SYSINFO_RC_ERROR;
   	char szArg[128] = {0};
	int nCount = -1;
	int nResult = -1;
	int i;
	kvm_t *kd;
	struct kinfo_proc *kp;

   	AgentGetParameterArgA(pszParam, 1, szArg, sizeof(szArg));

	kd = kvm_openfiles(NULL, NULL, NULL, O_RDONLY, NULL);
	if (kd != 0)
	{
#if KVM_GETPROCS_REQUIRES_SIZEOF
		kp = kvm_getprocs(kd, KERN_PROC_ALL, 0, sizeof(struct kinfo_proc), &nCount);
#else
		kp = kvm_getprocs(kd, KERN_PROC_ALL, 0, &nCount);
#endif

		if (kp != NULL)
		{
			if (szArg[0] != 0)
			{
				for (nResult = 0, i = 0; i < nCount; i++)
				{
#if KINFO_PROC_HAS_KP_PROC
						if (strcasecmp(kp[i].kp_proc.p_comm, szArg) == 0)
#else
						if (strcasecmp(kp[i].p_comm, szArg) == 0)
#endif
						{
							nResult++;
						}
				}
			}
			else
			{
				nResult = nCount;
			}
		}

		kvm_close(kd);
	}

	if (nResult >= 0)
	{
		ret_int(pValue, nResult);
		nRet = SYSINFO_RC_SUCCESS;
	}

	return nRet;
}

LONG H_MemoryInfo(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
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
	struct swapent swap[16];
	struct vmtotal vm;
	size_t nValSize = sizeof(vmtotal);

	nUsedPhysCount = nFreePhysCount = 0;
	nUsedVirtCount = 0;
	nSwapTotal = nSwapUsed = 0;

   	AgentGetParameterArgA(pszParam, 1, szArg, sizeof(szArg));

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

	// Swap

	if ((i = swapctl(SWAP_STATS, swap, 16)) != -1)
	{
		while (i-- > 0)
		{
			nSwapTotal += swap[i].se_nblks;
			nSwapUsed += swap[i].se_inuse;
		}
	}
	else
	{
		if (CAST_FROM_POINTER(pArg, int) != PHYSICAL_FREE &&
		    CAST_FROM_POINTER(pArg, int) != PHYSICAL_TOTAL &&
		    CAST_FROM_POINTER(pArg, int) != PHYSICAL_USED &&
		    CAST_FROM_POINTER(pArg, int) != VIRTUAL_USED)
		nRet = SYSINFO_RC_ERROR;
	}

	long pageSize = sysconf(_SC_PAGESIZE);

#define PAGES_TO_BYTES(x) ((QWORD)(x) * (QWORD)pageSize)
#define BLOCKS_TO_BYTES(x) ((QWORD)(x) * (QWORD)(DEV_BSIZE))

	if (nRet == SYSINFO_RC_SUCCESS)
	{
		switch(CAST_FROM_POINTER(pArg, int))
		{
		case PHYSICAL_FREE: // ph-free
			ret_uint64(pValue, PAGES_TO_BYTES(nFreePhysCount));
			break;
		case PHYSICAL_TOTAL: // ph-total
			ret_uint64(pValue, PAGES_TO_BYTES(nFreePhysCount + nUsedPhysCount));
			break;
		case PHYSICAL_USED: // ph-used
			ret_uint64(pValue, PAGES_TO_BYTES(nUsedPhysCount));
			break;
		case SWAP_FREE: // sw-free
			ret_uint64(pValue, BLOCKS_TO_BYTES(nSwapTotal - nSwapUsed));
			break;
		case SWAP_TOTAL: // sw-total
			ret_uint64(pValue, BLOCKS_TO_BYTES(nSwapTotal));
			break;
		case SWAP_USED: // sw-used
			ret_uint64(pValue, BLOCKS_TO_BYTES(nSwapUsed));
			break;
		case VIRTUAL_FREE: // vi-free : phys free + swap free
			ret_uint64(pValue, BLOCKS_TO_BYTES(nSwapTotal - nSwapUsed) +
					   PAGES_TO_BYTES(nFreePhysCount));
			break;
		case VIRTUAL_TOTAL: // vi-total : phys total + swap total
			ret_uint64(pValue, BLOCKS_TO_BYTES(nSwapTotal) +
					   PAGES_TO_BYTES(nFreePhysCount + nUsedPhysCount));
			break;
		case VIRTUAL_USED: // vi-used
			ret_uint64(pValue, PAGES_TO_BYTES(nUsedVirtCount));
			break;
		default: // error
			nRet = SYSINFO_RC_ERROR;
			break;
		}
	}

	return nRet;
}

LONG H_ProcessList(const TCHAR *pszParam, const TCHAR *pArg, StringList *pValue, AbstractCommSession *session)
{
	int nRet = SYSINFO_RC_ERROR;
	int nCount = -1;
	int i;
	struct kinfo_proc *kp;
	kvm_t *kd;

	kd = kvm_openfiles(NULL, NULL, NULL, O_RDONLY, NULL);
	if (kd != 0)
	{
#if KVM_GETPROCS_REQUIRES_SIZEOF
		kp = kvm_getprocs(kd, KERN_PROC_ALL, 0, sizeof(struct kinfo_proc), &nCount);
#else
		kp = kvm_getprocs(kd, KERN_PROC_ALL, 0, &nCount);
#endif

		if (kp != NULL)
		{
			for (i = 0; i < nCount; i++)
			{
				char szBuff[128];

				snprintf(szBuff, sizeof(szBuff), "%d %s",
#if KINFO_PROC_HAS_KP_PROC
						kp[i].kp_proc.p_pid, kp[i].kp_proc.p_comm
#else
						kp[i].p_pid, kp[i].p_comm
#endif
						);
				pValue->addMBString(szBuff);
			}
		}

		kvm_close(kd);
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
LONG H_SourcePkgSupport(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
	ret_int(pValue, 1);
	return SYSINFO_RC_SUCCESS;
}
