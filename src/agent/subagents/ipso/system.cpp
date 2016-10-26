/* $Id$ */

/*
** NetXMS subagent for FreeBSD
** Copyright (C) 2004 Alex Kirhenshtein
** Copyright (C) 2006 Victor Kirhenshtein
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

#include "ipso.h"
#include <sys/sysctl.h>
#include <sys/utsname.h>
#include <vm/vm_param.h>
#include <sys/vmmeter.h>
#include <kvm.h>


//
// Defines for getting data from process information structure
//

#define KP_PID(x)    (*((pid_t *)(kp + 48)))
#define KP_PNAME(x)  (x + 0xCB)


//
// Handler for System.Uptime
//

LONG H_Uptime(char *pszParam, char *pArg, char *pValue, AbstractCommSession *session)
{
	static int mib[2] = { CTL_KERN, KERN_BOOTTIME };
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


//
// Handler for System.Uname
//

LONG H_Uname(char *pszParam, char *pArg, char *pValue, AbstractCommSession *session)
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

LONG H_Hostname(char *pszParam, char *pArg, char *pValue, AbstractCommSession *session)
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

LONG H_CpuLoad(char *pszParam, char *pArg, char *pValue, AbstractCommSession *session)
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

LONG H_CpuCount(char *pszParam, char *pArg, char *pValue, AbstractCommSession *session
{
	int nRet = SYSINFO_RC_ERROR;
	static int mib[2] = { CTL_HW, HW_NCPU };
	size_t nSize = sizeof(mib), nValSize;
	int nVal;

	nValSize = sizeof(nVal);
	if (sysctl(mib, nSize, &nVal, &nValSize, NULL, 0) == 0)
	{
		ret_int(pValue, nVal);
		nRet = SYSINFO_RC_SUCCESS;
	}

	return nRet;
}


//
// Handler for System.ProcessCount and Process.Count(*) parameters
//

LONG H_ProcessCount(char *pszParam, char *pArg, char *pValue, AbstractCommSession *session)
{
	char *kp, szArg[128] = "";
	int i, nCount, nResult = -1;
	kvm_t *kd;
	LONG nRet = SYSINFO_RC_ERROR;

	AgentGetParameterArg(pszParam, 1, szArg, sizeof(szArg));

	kd = kvm_openfiles(NULL, NULL, NULL, O_RDONLY, NULL);
	if (kd != NULL)
	{
		kp = (char *)kvm_getprocs(kd, KERN_PROC_ALL, 0, &nCount);

		if (kp != NULL)
		{
			if (szArg[0] != 0)
			{
				nResult = 0;
				for(i = 0; i < nCount; i++, kp += 648)
				{
					if (!stricmp(KP_PNAME(kp), szArg))
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


//
// Handler for System.Memory.* parameters
//

LONG H_MemoryInfo(char *pszParam, char *pArg, char *pValue, AbstractCommSession *session)
{
	struct vmtotal vmStat;
	DWORD dwPageSize, dwPhysMem;
	size_t nSize;
	int mib[2] = { CTL_HW, HW_PAGESIZE };
	int nRet = SYSINFO_RC_SUCCESS;

	nSize = sizeof(int);
	if (sysctl(mib, 2, &dwPageSize, &nSize, NULL, 0) == -1)
	{
		return SYSINFO_RC_ERROR;
	}

	nSize = sizeof(int);
	mib[1] = HW_PHYSMEM;
	if (sysctl(mib, 2, &dwPhysMem, &nSize, NULL, 0) == -1)
	{
		return SYSINFO_RC_ERROR;
	}

	nSize = sizeof(struct vmtotal);
	mib[0] = CTL_VM;
	mib[1] = VM_METER;
	if (sysctl(mib, 2, &vmStat, &nSize, NULL, 0) != -1)
	{
/*
#define XX(a) a,a*dwPageSize,(a*dwPageSize)/1024
printf("t_avm    = %d %d %dK\n",XX(vmStat.t_avm));
printf("t_rm     = %d %d %dK\n",XX(vmStat.t_rm));
printf("t_arm    = %d %d %dK\n",XX(vmStat.t_arm));
printf("t_vmshr  = %d %d %dK\n",XX(vmStat.t_vmshr));
printf("t_avmshr = %d %d %dK\n",XX(vmStat.t_avmshr));
printf("t_rmshr  = %d %d %dK\n",XX(vmStat.t_rmshr));
printf("t_armshr = %d %d %dK\n",XX(vmStat.t_armshr));
printf("t_free   = %d %d %dK\n",XX(vmStat.t_free));
printf("PageSize = %d\n",dwPageSize);
*/

/* TODO: check what to use: t_arm or t_free for free memory size */
		switch((int)pArg)
		{
			case PHYSICAL_FREE:
				ret_uint64(pValue, (QWORD)vmStat.t_free * dwPageSize);
				break;
			case PHYSICAL_FREE_PCT:
				ret_double(pValue, (((double)vmStat.t_free * 100.0 * dwPageSize) / dwPhysMem), 2);
				break;
			case PHYSICAL_TOTAL:
				ret_uint64(pValue, (QWORD)dwPhysMem);
				break;
			case PHYSICAL_USED:
				ret_uint64(pValue, (QWORD)dwPhysMem - (QWORD)vmStat.t_arm * dwPageSize);
				break;
			case PHYSICAL_USED_PCT:
				ret_double(pValue, ((((double)dwPhysMem - vmStat.t_arm * dwPageSize) * 100.0) / dwPhysMem), 2);
				break;
			default:
				nRet = SYSINFO_RC_UNSUPPORTED;
				break;
		}
	}
	else
	{
		nRet = SYSINFO_RC_ERROR;
	}
	return nRet;
}


//
// Handler for System.ProcessList enum
//

LONG H_ProcessList(char *pszParam, char *pArg, StringList *pValue, AbstractCommSession *session)
{
	int i, nCount = -1;
	char *kp;
	kvm_t *kd;
	LONG nRet = SYSINFO_RC_ERROR;

	kd = kvm_openfiles(NULL, NULL, NULL, O_RDONLY, NULL);
	if (kd != 0)
	{
		kp = (char *)kvm_getprocs(kd, KERN_PROC_ALL, 0, &nCount);
		if (kp != NULL)
		{
			for (i = 0; i < nCount; i++, kp += 648)
			{
				char szBuff[128];

				snprintf(szBuff, sizeof(szBuff), "%d %s",
					KP_PID(kp), KP_PNAME(kp));
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


///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.4  2006/08/16 22:26:09  victor
- Most of Net.Interface.XXX functions implemented on IPSO
- Added function MACToStr

Revision 1.3  2006/07/24 06:49:48  victor
- Process and physical memory parameters are working
- Various other changes

Revision 1.2  2006/07/21 16:22:44  victor
Some parameters are working

Revision 1.1  2006/07/21 11:48:35  victor
Initial commit

*/
