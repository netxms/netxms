/* $Id: linux.cpp,v 1.6 2004-08-26 23:51:26 alk Exp $ */

/* 
** NetXMS subagent for GNU/Linux
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
** $module: linux.cpp
**
**/

#include <nms_common.h>
#include <nms_agent.h>

#include <locale.h>
#include <sys/utsname.h>
#include <sys/statvfs.h>


enum
{
	PHYSICAL_FREE,
	PHYSICAL_USED,
	PHYSICAL_TOTAL,
	SWAP_FREE,
	SWAP_USED,
	SWAP_TOTAL,
	VIRTUAL_FREE,
	VIRTUAL_USED,
	VIRTUAL_TOTAL,
};

enum
{
	DISK_FREE,
	DISK_USED,
	DISK_TOTAL,
};

//
// Hanlder functions
//

static LONG H_Uptime(char *pszParam, char *pArg, char *pValue)
{
	FILE *hFile;
	unsigned int uUptime = 0;

	hFile = fopen("/proc/uptime", "r");
	if (hFile != NULL)
	{
		char szTmp[64];

		if (fgets(szTmp, sizeof(szTmp), hFile) != NULL)
		{
			double dTmp;

			setlocale(LC_NUMERIC, "C");
			if (sscanf(szTmp, "%lf", &dTmp) == 1)
			{
				uUptime = (unsigned int)dTmp;
			}
			setlocale(LC_NUMERIC, "");
		}
		fclose(hFile);
	}
	
	if (uUptime > 0)
	{
   	ret_uint(pValue, uUptime);
	}

   return uUptime > 0 ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
}

static LONG H_Uname(char *pszParam, char *pArg, char *pValue)
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

static LONG H_Hostname(char *pszParam, char *pArg, char *pValue)
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

static LONG H_DiskInfo(char *pszParam, char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
	struct statvfs s;
   char szArg[512] = {0};

   NxGetParameterArg(pszParam, 1, szArg, sizeof(szArg));

	if (szArg[0] != 0 && statvfs(szArg, &s) == 0)
	{
		nRet = SYSINFO_RC_SUCCESS;
		switch((int)pArg)
		{
		case DISK_FREE:
			ret_uint64(pValue, (QWORD)s.f_bfree * (QWORD)s.f_bsize);
			break;
		case DISK_TOTAL:
			ret_uint64(pValue, (QWORD)s.f_blocks * (QWORD)s.f_frsize);
			break;
		case DISK_USED:
			ret_uint64(pValue, (QWORD)(s.f_blocks - s.f_bfree) * (QWORD)s.f_frsize);
			break;
		default: // YIC
			nRet = SYSINFO_RC_ERROR;
			break;
		}
	}

	return nRet;
}

static LONG H_CpuLoad(char *pszParam, char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
	struct statvfs s;
   char szArg[128] = {0};
	FILE *hFile;

	// get processor
   //NxGetParameterArg(pszParam, 1, szArg, sizeof(szArg));

	hFile = fopen("/proc/loadavg", "r");
	if (hFile != NULL)
	{
		char szTmp[64];

		if (fgets(szTmp, sizeof(szTmp), hFile) != NULL)
		{
			double dLoad1, dLoad5, dLoad15;

			setlocale(LC_NUMERIC, "C");
			if (sscanf(szTmp, "%lf %lf %lf", &dLoad1, &dLoad5, &dLoad15) == 3)
			{
				switch (pszParam[19])
				{
				case '1': // 15 min
					ret_double(pValue, dLoad15);
					break;
				case '5': // 5 min
					ret_double(pValue, dLoad5);
					break;
				default: // 1 min
					ret_double(pValue, dLoad1);
					break;
				}
				nRet = SYSINFO_RC_SUCCESS;
			}
			setlocale(LC_NUMERIC, "");
		}

		fclose(hFile);
	}
	

	return nRet;
}

static LONG H_CpuUsage(char *pszParam, char *pArg, char *pValue)
{
	return SYSINFO_RC_ERROR;
}

static LONG H_ProcessCount(char *pszParam, char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
	struct statvfs s;
   char szArg[128] = {0};

   NxGetParameterArg(pszParam, 1, szArg, sizeof(szArg));

	return nRet;
}

static LONG H_NetworkARPCache(char *pszParam, char *pArg, NETXMS_VALUES_LIST *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
	FILE *hFile;

	hFile = fopen("/proc/net/arp", "r");
	if (hFile != NULL)
	{
		fclose(hFile);
	}

	return nRet;
}

static LONG H_MemoryInfo(char *pszParam, char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
	FILE *hFile;
	unsigned long nMemTotal, nMemFree, nSwapTotal, nSwapFree;
	char *pTag;

	nMemTotal = nMemFree = nSwapTotal = nSwapFree = 0;

	hFile = fopen("/proc/meminfo", "r");
	if (hFile != NULL)
	{
		char szTmp[128];
		while (fgets(szTmp, sizeof(szTmp), hFile) != NULL)
		{
			if (sscanf(szTmp, "MemTotal: %lu kB\n", &nMemTotal) == 1)
				continue;
			if (sscanf(szTmp, "MemFree: %lu kB\n", &nMemFree) == 1)
				continue;
			if (sscanf(szTmp, "SwapTotal: %lu kB\n", &nSwapTotal) == 1)
				continue;
			if (sscanf(szTmp, "SwapFree: %lu kB\n", &nSwapFree) == 1)
				continue;
		}

		fclose(hFile);

		nRet = SYSINFO_RC_SUCCESS;
		switch((int)pArg)
		{
		case PHYSICAL_FREE: // ph-free
			ret_uint64(pValue, ((int64_t)nMemFree) * 1024);
			break;
		case PHYSICAL_TOTAL: // ph-total
			ret_uint64(pValue, ((int64_t)nMemTotal) * 1024);
			break;
		case PHYSICAL_USED: // ph-used
			ret_uint64(pValue, ((int64_t)(nMemTotal - nMemFree)) * 1024);
			break;
		case SWAP_FREE: // sw-free
			ret_uint64(pValue, ((int64_t)nSwapFree) * 1024);
			break;
		case SWAP_TOTAL: // sw-total
			ret_uint64(pValue, ((int64_t)nSwapTotal) * 1024);
			break;
		case SWAP_USED: // sw-used
			ret_uint64(pValue, ((int64_t)(nSwapTotal - nSwapFree)) * 1024);
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


//
// Subagent information
//

static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
   { "System.Uptime",                H_Uptime,           NULL },
   { "System.Uname",                 H_Uname,            NULL },
   { "System.Hostname",              H_Hostname,         NULL },

   { "Disk.Free(*)",                 H_DiskInfo,         (char *)DISK_FREE },
   { "Disk.Total(*)",                H_DiskInfo,         (char *)DISK_TOTAL },
   { "Disk.Used(*)",                 H_DiskInfo,         (char *)DISK_USED },

   { "System.CPU.Procload",          H_CpuLoad,          NULL },
   { "System.CPU.Procload5",         H_CpuLoad,          NULL },
   { "System.CPU.Procload15",        H_CpuLoad,          NULL },

   { "System.CPU.Usage",             H_CpuUsage,         NULL },
   { "System.CPU.Usage5",            H_CpuUsage,         NULL },
   { "System.CPU.Usage15",           H_CpuUsage,         NULL },

   { "Process.Count(*)",             H_ProcessCount,     NULL },

   { "System.Memory.Physical.Free",  H_MemoryInfo,       (char *)PHYSICAL_FREE },
   { "System.Memory.Physical.Total", H_MemoryInfo,       (char *)PHYSICAL_TOTAL },
   { "System.Memory.Physical.Used",  H_MemoryInfo,       (char *)PHYSICAL_USED },
   { "System.Memory.Swap.Free",      H_MemoryInfo,       (char *)SWAP_FREE },
   { "System.Memory.Swap.Total",     H_MemoryInfo,       (char *)SWAP_TOTAL },
   { "System.Memory.Swap.Used",      H_MemoryInfo,       (char *)SWAP_USED },
   { "System.Memory.Virtual.Free",   H_MemoryInfo,       (char *)VIRTUAL_FREE },
   { "System.Memory.Virtual.Total",  H_MemoryInfo,       (char *)VIRTUAL_TOTAL },
   { "System.Memory.Virtual.Used",   H_MemoryInfo,       (char *)VIRTUAL_USED },
};

static NETXMS_SUBAGENT_ENUM m_enums[] =
{
   { "Network.ARPCache",      H_NetworkARPCache, NULL },
};

static NETXMS_SUBAGENT_INFO m_info =
{
	"Linux",
	0x01000000,
	NULL,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	sizeof(m_enums) / sizeof(NETXMS_SUBAGENT_ENUM),
	m_enums
};

//
// Entry point for NetXMS agent
//

extern "C" BOOL NxSubAgentInit(NETXMS_SUBAGENT_INFO **ppInfo)
{
   *ppInfo = &m_info;

   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.5  2004/08/18 00:12:56  alk
+ System.CPU.Procload* added, SINGLE processor only.

Revision 1.4  2004/08/17 23:19:20  alk
+ Disk.* implemented

Revision 1.3  2004/08/17 15:17:32  alk
+ linux agent: system.uptime, system.uname, system.hostname
! skeleton: amount of _PARM & _ENUM filled with sizeof()

Revision 1.1  2004/08/17 10:24:18  alk
+ subagent selection based on "uname -s"


*/
