/*
** NetXMS subagent for AIX
** Copyright (C) 2004-2016 Victor Kirhenshtein
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
#include <sys/ipc.h>
#include <sys/msg.h>
#include <nlist.h>
#include <utmp.h>
#include <libperfstat.h>

/**
 * Hander for System.CPU.Count parameter
 */
LONG H_CPUCount(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
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

/**
 * Handler for System.Uname parameter
 */
LONG H_Uname(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
	LONG nRet;
	struct utsname un;

	if (uname(&un) == 0)
	{
#ifdef UNICODE
		swprintf(pValue, MAX_RESULT_LENGTH, L"%hs %hs %hs %hs %hs", un.sysname, un.nodename, un.release, un.version, un.machine);
#else
		snprintf(pValue, MAX_RESULT_LENGTH, "%s %s %s %s %s", un.sysname, un.nodename, un.release, un.version, un.machine);
#endif
		nRet = SYSINFO_RC_SUCCESS;
	}
	else
	{
		nRet = SYSINFO_RC_ERROR;
	}
	return nRet;
}

/**
 * Handler for System.Uptime parameter
 */
LONG H_Uptime(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
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

/**
 * Handler for System.Hostname parameter
 */
LONG H_Hostname(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
	LONG nRet;
	struct utsname un;

	if (uname(&un) == 0)
	{
		ret_mbstring(pValue, un.nodename);
		nRet = SYSINFO_RC_SUCCESS;
	}
	else
	{
		nRet = SYSINFO_RC_ERROR;
	}
	return nRet;
}

/**
 * Handler for System.Memory.Physical.xxx parameters
 */
LONG H_MemoryInfo(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
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
				ret_double(pValue, (double)vmi.numfrb * 100.0 / vmi.memsizepgs, 2);
				break;
			case MEMINFO_PHYSICAL_USED:
				ret_uint64(pValue, (vmi.memsizepgs - vmi.numfrb) * getpagesize());
				break;
			case MEMINFO_PHYSICAL_USED_PERC:
				ret_double(pValue, ((double)vmi.memsizepgs - vmi.numfrb) * 100.0 / vmi.memsizepgs, 2);
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

/**
 * Handler for virtual and swap memory parameters
 */
LONG H_VirtualMemoryInfo(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
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
				ret_double(pValue, (double)memStats.pgsp_free * 100.0 / memStats.pgsp_total, 2);
				break;
			case MEMINFO_SWAP_USED:
				ret_uint64(pValue, (memStats.pgsp_total - memStats.pgsp_free) * 4096);
				break;
			case MEMINFO_SWAP_USED_PERC:
				ret_double(pValue, ((double)memStats.pgsp_total - memStats.pgsp_free) * 100.0 / memStats.pgsp_total);
				break;
			case MEMINFO_SWAP_TOTAL:
				ret_uint64(pValue, memStats.pgsp_total * 4096);
				break;
			case MEMINFO_VIRTUAL_ACTIVE:
				ret_uint64(pValue, memStats.virt_active * 4096);
				break;
			case MEMINFO_VIRTUAL_ACTIVE_PERC:
				ret_double(pValue, (double)memStats.virt_active * 100.0 / memStats.virt_total);
				break;
			case MEMINFO_VIRTUAL_FREE:
				ret_uint64(pValue, (memStats.real_free + memStats.pgsp_free) * 4096);
				break;
			case MEMINFO_VIRTUAL_FREE_PERC:
				ret_double(pValue, ((double)memStats.real_free + memStats.pgsp_free) * 100.0 / memStats.virt_total);
				break;
			case MEMINFO_VIRTUAL_USED:
				ret_uint64(pValue, (memStats.virt_total - memStats.real_free - memStats.pgsp_free) * 4096);
				break;
			case MEMINFO_VIRTUAL_USED_PERC:
				ret_double(pValue, ((double)memStats.virt_total - memStats.real_free - memStats.pgsp_free) * 100.0 / memStats.virt_total);
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

/**
 * Read from /dev/kmem
 */
static BOOL kread(int kmem, off_t offset, void *buffer, size_t buflen)
{
	if (lseek(kmem, offset, SEEK_SET) != -1)
	{
		return (read(kmem, buffer, buflen) == buflen);
	}
	return FALSE;
}

/**
 * Handler for System.LoadAvg parameters
 */
LONG H_LoadAvg(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
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

/**
 * Handler for System.MsgQueue.* parameters
 */
LONG H_SysMsgQueue(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR buffer[64];
   if (!AgentGetParameterArg(param, 1, buffer, 64))
      return SYSINFO_RC_UNSUPPORTED;

   int queueId = -1;
   if (buffer[0] == _T('@'))  // queue identified by ID
   {
      TCHAR *eptr;
      queueId = (int)_tcstol(&buffer[1], &eptr, 0);
      if ((queueId < 0) || (*eptr != 0))
         return SYSINFO_RC_UNSUPPORTED;   // Invalid queue ID
   }
   else  // queue identified by key
   {
      TCHAR *eptr;
      key_t key = (key_t)_tcstoul(buffer, &eptr, 0);
      if (*eptr != 0)
         return SYSINFO_RC_UNSUPPORTED;   // Invalid key
      queueId = msgget(key, 0);
      if (queueId < 0)
         return (errno == ENOENT) ? SYSINFO_RC_NO_SUCH_INSTANCE : SYSINFO_RC_ERROR;
   }

   struct msqid_ds data;
   if (msgctl(queueId, IPC_STAT, &data) != 0)
      return ((errno == EIDRM) || (errno == EINVAL)) ? SYSINFO_RC_NO_SUCH_INSTANCE : SYSINFO_RC_ERROR;

   switch((char)*arg)
   {
      case 'b':
         ret_uint64(value, (UINT64)data.msg_cbytes);
         break;
      case 'B':
         ret_uint64(value, (UINT64)data.msg_qbytes);
         break;
      case 'c':
         ret_uint64(value, (UINT64)data.msg_ctime);
         break;
      case 'm':
         ret_uint64(value, (UINT64)data.msg_qnum);
         break;
      case 'r':
         ret_uint64(value, (UINT64)data.msg_rtime);
         break;
      case 's':
         ret_uint64(value, (UINT64)data.msg_stime);
         break;
      default:
         return SYSINFO_RC_UNSUPPORTED;
   }
   return SYSINFO_RC_SUCCESS;
}
