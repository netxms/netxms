/*
** NetXMS subagent for AIX
** Copyright (C) 2004-2011 Victor Kirhenshtein
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
#include <nlist.h>
#include <utmp.h>
#include <libperfstat.h>


//
// Hander for System.CPU.Count parameter
//

LONG H_CPUCount(const char *pszParam, const char *pArg, char *pValue)
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

LONG H_Uname(const char *pszParam, const char *pArg, char *pValue)
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

LONG H_Uptime(const char *pszParam, const char *pArg, char *pValue)
{
	LONG nRet = SYSINFO_RC_ERROR;
	int fd;
	struct utmp ut;

	fd = open(UTMP_FILE, O_RDONLY);
	if (fd != -1)
	{
		while(1)
		{
			if (read(fd, &ut, sizeof(struct utmp)) != sizeof(struct utmp))
				break;	// Read error
			if (ut.ut_type == BOOT_TIME)
			{
				ret_uint(pValue, time(NULL) - ut.ut_time);
				nRet = SYSINFO_RC_SUCCESS;
				break;
			}
		}
		close(fd);
	}
	return nRet;
}


//
// Handler for System.Hostname parameter
//

LONG H_Hostname(const char *pszParam, const char *pArg, char *pValue)
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

LONG H_MemoryInfo(const char *pszParam, const char *pArg, char *pValue)
{
	struct vminfo vmi;
	LONG nRet;

	if (vmgetinfo(&vmi, VMINFO, sizeof(struct vminfo)) == 0)
	{
		switch(CAST_FROM_POINTER(pArg, int))
		{
			case MEMINFO_PHYSICAL_FREE:
				ret_uint64(pValue, vmi.numfrb * getpagesize());
				break;
			case MEMINFO_PHYSICAL_FREE_PERC:
				ret_uint(pValue, vmi.numfrb * 100 / vmi.memsizepgs);
				break;
			case MEMINFO_PHYSICAL_USED:
				ret_uint64(pValue, (vmi.memsizepgs - vmi.numfrb) * getpagesize());
				break;
			case MEMINFO_PHYSICAL_USED_PERC:
				ret_uint(pValue, (vmi.memsizepgs - vmi.numfrb) * 100 / vmi.memsizepgs);
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


//
// Handler for virtual and swap memory parameters
//

LONG H_VirtualMemoryInfo(const char *pszParam, const char *pArg, char *pValue)
{
	perfstat_memory_total_t memStats;
	LONG nRet;

	if (perfstat_memory_total(NULL, &memStats, sizeof(perfstat_memory_total_t), 1) == 1)
	{
		switch(CAST_FROM_POINTER(pArg, int))
		{
			case MEMINFO_SWAP_FREE:
				ret_uint64(pValue, memStats.pgsp_free * 4096);
				break;
			case MEMINFO_SWAP_FREE_PERC:
				ret_uint(pValue, memStats.pgsp_free * 100 / memStats.pgsp_total);
				break;
			case MEMINFO_SWAP_USED:
				ret_uint64(pValue, (memStats.pgsp_total - memStats.pgsp_free) * 4096);
				break;
			case MEMINFO_SWAP_USED_PERC:
				ret_uint(pValue, (memStats.pgsp_total - memStats.pgsp_free) * 100 / memStats.pgsp_total);
				break;
			case MEMINFO_SWAP_TOTAL:
				ret_uint64(pValue, memStats.pgsp_total * 4096);
				break;
			case MEMINFO_VIRTUAL_ACTIVE:
				ret_uint64(pValue, memStats.virt_active * 4096);
				break;
			case MEMINFO_VIRTUAL_ACTIVE_PERC:
				ret_uint(pValue, memStats.virt_active * 100 / memStats.virt_total);
				break;
			case MEMINFO_VIRTUAL_FREE:
				ret_uint64(pValue, (memStats.real_free + memStats.pgsp_free) * 4096);
				break;
			case MEMINFO_VIRTUAL_FREE_PERC:
				ret_uint(pValue, (memStats.real_free + memStats.pgsp_free) * 100 / memStats.virt_total);
				break;
			case MEMINFO_VIRTUAL_USED:
				ret_uint64(pValue, (memStats.virt_total - memStats.real_free - memStats.pgsp_free) * 4096);
				break;
			case MEMINFO_VIRTUAL_USED_PERC:
				ret_uint(pValue, (memStats.virt_total - memStats.real_free - memStats.pgsp_free) * 100 / memStats.virt_total);
				break;
			case MEMINFO_VIRTUAL_TOTAL:
				ret_uint64(pValue, memStats.virt_total * 4096);
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


//
// Read from /dev/kmem
//

static BOOL kread(int kmem, off_t offset, void *buffer, size_t buflen)
{
	if (lseek(kmem, offset, SEEK_SET) != -1)
	{
		return (read(kmem, buffer, buflen) == buflen);
	}
	return FALSE;
}


//
// Handler for System.LoadAvg parameters
//

static LONG LoadAvgFromPerflib(const char *arg, char *value)
{
	LONG rc;
	perfstat_cpu_total_t info;

	if (perfstat_cpu_total(NULL, &info, sizeof(perfstat_cpu_total_t), 1) == 1)
	{
		ret_double(value, (double)info.loadavg[*arg - '0'] / (double)(1 << SBITS));
		rc = SYSINFO_RC_SUCCESS;
	}
	else
	{
		rc = SYSINFO_RC_ERROR;
	}
	return rc;
}

LONG H_LoadAvg(const char *param, const char *arg, char *value)
{
	int kmem;
	LONG rc = SYSINFO_RC_ERROR;
	struct nlist nl[] = { (char *)"avenrun" }; 
	long avenrun[3];

	kmem = open("/dev/kmem", O_RDONLY);
	if (kmem == -1)
	{
		return LoadAvgFromPerflib(arg, value);
	}

	if (knlist(nl, 1, sizeof(struct nlist)) == 0)
	{
		if (nl[0].n_value != NULL)
		{
			if (kread(kmem, nl[0].n_value, avenrun, sizeof(avenrun)))
			{
				ret_double(value, (double)avenrun[*arg - '0'] / 65536.0);
				rc = SYSINFO_RC_SUCCESS;
			}
		}
	}	

	close(kmem);
	return rc;
}
