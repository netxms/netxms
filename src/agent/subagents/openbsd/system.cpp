/* 
** NetXMS subagent for OpenBSD
** Copyright (C) 2006 C.T.Co 
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
#include <fcntl.h>
#include <kvm.h>
#include <paths.h>


#include "system.h"

LONG H_Uptime(const char *pszParam, const char *pArg, char *pValue)
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

LONG H_Uname(const char *pszParam, const char *pArg, char *pValue)
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

LONG H_Hostname(const char *pszParam, const char *pArg, char *pValue)
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

LONG H_CpuLoad(const char *pszParam, const char *pArg, char *pValue)
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

LONG H_CpuUsage(const char *pszParam, const char *pArg, char *pValue)
{
	return SYSINFO_RC_UNSUPPORTED;
}

LONG H_CpuCount(const char *pszParam, const char *pArg, char *pValue)
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

LONG H_ProcessCount(const char *pszParam, const char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
   	char szArg[128] = {0};
	int nCount = -1;
	int nResult = -1;
	int i;
	kvm_t *kd;
	struct kinfo_proc *kp;

   	AgentGetParameterArg(pszParam, 1, szArg, sizeof(szArg));

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

LONG H_MemoryInfo(const char *pszParam, const char *pArg, char *pValue)
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
		if ((int)pArg != PHYSICAL_FREE &&
		    (int)pArg != PHYSICAL_TOTAL &&
		    (int)pArg != PHYSICAL_USED &&
		    (int)pArg != VIRTUAL_USED)
		nRet = SYSINFO_RC_ERROR;
	}

	long pageSize = sysconf(_SC_PAGESIZE);

#define PAGES_TO_BYTES(x) ((QWORD)(x) * (QWORD)pageSize)
#define BLOCKS_TO_BYTES(x) ((QWORD)(x) * (QWORD)(DEV_BSIZE))

	if (nRet == SYSINFO_RC_SUCCESS)
	{
		switch((int)pArg)
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

LONG H_ProcessList(const char *pszParam, const char *pArg, StringList *pValue)
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
				pValue->add(szBuff);
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
LONG H_SourcePkgSupport(const char *pszParam, const char *pArg, char *pValue)
{
	ret_int(pValue, 1);
	return SYSINFO_RC_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.8  2005/05/30 14:39:32  alk
* process list now works via kvm, compatible with freebsd 5+

Revision 1.7  2005/05/29 22:44:59  alk
* configure: pthreads & fbsd5+; detection code should be rewriten!
* another ugly hack: agent's process info disabled for fbsd5+: struct kinfo_proc changed; m/b fix it tomorow
* server/nxadm & fbsd5.1: a**holes, in 5.1 there no define with version in readline.h...

Revision 1.6  2005/01/24 19:51:16  alk
reurn types/comments added
Process.Count(*)/System.ProcessCount fixed

Revision 1.5  2005/01/23 05:36:11  alk
+ System.Memory.Swap.*
+ System.Memory.Virtual.*

NB! r/o access to /dev/mem required! (e.g. chgrp kmem ; chmod g+s)

Revision 1.4  2005/01/23 05:14:49  alk
System's PageSize used instead of Hardware PageSize

Revision 1.3  2005/01/23 05:08:06  alk
+ System.CPU.Count
+ System.Memory.Physical.*
+ System.ProcessCount
+ System.ProcessList

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
