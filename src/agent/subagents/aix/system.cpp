/*
** NetXMS subagent for AIX
** Copyright (C) 2004-2025 Victor Kirhenshtein
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
#include <sys/cfgodm.h>
#include <odmi.h>

/**
 * Hander for System.CPU.Count parameter
 */
LONG H_CPUCount(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *value, AbstractCommSession *session)
{
   LONG rc;
   struct vario v;
   if (sys_parm(SYSP_GET, SYSP_V_NCPUS_CFG, &v) == 0)
   {
      ret_int(value, v.v.v_ncpus_cfg.value);
      rc = SYSINFO_RC_SUCCESS;
   }
   else
   {
      rc = SYSINFO_RC_ERROR;
   }
   return rc;
}

/**
 * Handler for System.Uname parameter
 */
LONG H_Uname(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   struct utsname un;
   if (uname(&un) != 0)
      return SYSINFO_RC_ERROR;
#ifdef UNICODE
   swprintf(value, MAX_RESULT_LENGTH, L"%hs %hs %hs %hs %hs", un.sysname, un.nodename, un.release, un.version, un.machine);
#else
   snprintf(value, MAX_RESULT_LENGTH, "%s %s %s %s %s", un.sysname, un.nodename, un.release, un.version, un.machine);
#endif
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.Uptime parameter
 */
LONG H_Uptime(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *value, AbstractCommSession *session)
{
   LONG rc = SYSINFO_RC_ERROR;
   int fd = open(UTMP_FILE, O_RDONLY);
   if (fd != -1)
   {
      while(true)
      {
         struct utmp ut;
         if (read(fd, &ut, sizeof(struct utmp)) != sizeof(struct utmp))
            break; // Read error
         if (ut.ut_type == BOOT_TIME)
         {
            ret_uint(value, time(nullptr) - ut.ut_time);
            rc = SYSINFO_RC_SUCCESS;
            break;
         }
      }
      close(fd);
   }
   return rc;
}

/**
 * Handler for System.Hostname parameter
 */
LONG H_Hostname(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *value, AbstractCommSession *session)
{
	LONG nRet;
	struct utsname un;

	if (uname(&un) == 0)
	{
		ret_mbstring(value, un.nodename);
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
LONG H_MemoryInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   struct vminfo vmi;
   LONG rc;

   if (vmgetinfo(&vmi, VMINFO, sizeof(struct vminfo)) == 0)
   {
      switch(CAST_FROM_POINTER(arg, int))
      {
         case MEMINFO_PHYSICAL_AVAILABLE:
            ret_uint64(value, vmi.memavailable);
            break;
         case MEMINFO_PHYSICAL_AVAILABLE_PERC:
            ret_double(value, (double)(vmi.memavailable / getpagesize()) * 100.0 / vmi.memsizepgs, 2);
            break;
         case MEMINFO_PHYSICAL_CACHED:
            ret_uint64(value, vmi.numperm * getpagesize());
            break;
         case MEMINFO_PHYSICAL_CACHED_PERC:
            ret_double(value, (double)vmi.numperm * 100.0 / vmi.memsizepgs, 2);
            break;
         case MEMINFO_PHYSICAL_CLIENT:
            ret_uint64(value, vmi.numperm * getpagesize());
            break;
         case MEMINFO_PHYSICAL_CLIENT_PERC:
            ret_double(value, (double)vmi.numperm * 100.0 / vmi.memsizepgs, 2);
            break;
         case MEMINFO_PHYSICAL_COMP:
            ret_uint64(value, (vmi.memsizepgs - vmi.numfrb - vmi.numperm) * getpagesize());
            break;
         case MEMINFO_PHYSICAL_COMP_PERC:
            ret_double(value, static_cast<double>(vmi.memsizepgs - vmi.numfrb - vmi.numperm) * 100.0 / vmi.memsizepgs, 2);
            break;
         case MEMINFO_PHYSICAL_FREE:
            ret_uint64(value, vmi.numfrb * getpagesize());
            break;
         case MEMINFO_PHYSICAL_FREE_PERC:
            ret_double(value, (double)vmi.numfrb * 100.0 / vmi.memsizepgs, 2);
            break;
         case MEMINFO_PHYSICAL_USED:
            ret_uint64(value, (vmi.memsizepgs - vmi.numfrb) * getpagesize());
            break;
         case MEMINFO_PHYSICAL_USED_PERC:
            ret_double(value, static_cast<double>(vmi.memsizepgs - vmi.numfrb) * 100.0 / vmi.memsizepgs, 2);
            break;
         case MEMINFO_PHYSICAL_TOTAL:
            ret_uint64(value, vmi.memsizepgs * getpagesize());
            break;
      }
      rc = SYSINFO_RC_SUCCESS;
   }
   else
   {
      rc = SYSINFO_RC_ERROR;
   }
   return rc;
}

/**
 * Handler for virtual and swap memory parameters
 */
LONG H_VirtualMemoryInfo(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *value, AbstractCommSession *session)
{
   perfstat_memory_total_t memStats;
   LONG nRet;

	if (perfstat_memory_total(NULL, &memStats, sizeof(perfstat_memory_total_t), 1) == 1)
	{
		switch(CAST_FROM_POINTER(pArg, int))
		{
			case MEMINFO_SWAP_FREE:
				ret_uint64(value, memStats.pgsp_free * 4096);
				break;
			case MEMINFO_SWAP_FREE_PERC:
				ret_double(value, (double)memStats.pgsp_free * 100.0 / memStats.pgsp_total, 2);
				break;
			case MEMINFO_SWAP_USED:
				ret_uint64(value, (memStats.pgsp_total - memStats.pgsp_free) * 4096);
				break;
			case MEMINFO_SWAP_USED_PERC:
				ret_double(value, ((double)memStats.pgsp_total - memStats.pgsp_free) * 100.0 / memStats.pgsp_total);
				break;
			case MEMINFO_SWAP_TOTAL:
				ret_uint64(value, memStats.pgsp_total * 4096);
				break;
			case MEMINFO_VIRTUAL_ACTIVE:
				ret_uint64(value, memStats.virt_active * 4096);
				break;
			case MEMINFO_VIRTUAL_ACTIVE_PERC:
				ret_double(value, (double)memStats.virt_active * 100.0 / memStats.virt_total);
				break;
			case MEMINFO_VIRTUAL_FREE:
				ret_uint64(value, (memStats.real_free + memStats.pgsp_free) * 4096);
				break;
			case MEMINFO_VIRTUAL_FREE_PERC:
				ret_double(value, ((double)memStats.real_free + memStats.pgsp_free) * 100.0 / memStats.virt_total);
				break;
			case MEMINFO_VIRTUAL_USED:
				ret_uint64(value, (memStats.virt_total - memStats.real_free - memStats.pgsp_free) * 4096);
				break;
			case MEMINFO_VIRTUAL_USED_PERC:
				ret_double(value, ((double)memStats.virt_total - memStats.real_free - memStats.pgsp_free) * 100.0 / memStats.virt_total);
				break;
			case MEMINFO_VIRTUAL_TOTAL:
				ret_uint64(value, memStats.virt_total * 4096);
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
static bool kread(int kmem, off_t offset, void *buffer, size_t buflen)
{
   if (lseek(kmem, offset, SEEK_SET) != -1)
      return (read(kmem, buffer, buflen) == buflen);
   return false;
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

/**
 * Read attribute from sys0 device using ODM
 */
static bool ReadAttributeFromSysDevice(const char *attribute, char *buffer)
{
   char query[256];
   snprintf(query, 256, "name='sys0' and attribute='%s'", attribute);

   bool success = false;
   struct CuAt object;
   long rc = CAST_FROM_POINTER(odm_get_obj(CuAt_CLASS, query, &object, ODM_FIRST), long);
   while((rc != 0) && (rc != -1))
   {
      // Sanity check - some versions of AIX return all attributes despite filter in odm_get_obj
      if (!strcmp(attribute, object.attribute))
      {
         strlcpy(buffer, object.value, MAX_RESULT_LENGTH);
         success = true;
         break;
      }
      rc = CAST_FROM_POINTER(odm_get_obj(CuAt_CLASS, query, &object, ODM_NEXT), long);
   }
   return success;
}

/**
 * Handler for Hardware.System.Manufacturer
 */
LONG H_HardwareManufacturer(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   char buffer[MAX_RESULT_LENGTH];
   if (!ReadAttributeFromSysDevice("modelname", buffer))
      return SYSINFO_RC_ERROR;
   char *s = strchr(buffer, ',');
   if (s == nullptr)
      return SYSINFO_RC_UNSUPPORTED;
   *s = 0;
   ret_mbstring(value, buffer);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Hardware.System.Product
 */
LONG H_HardwareProduct(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   char buffer[MAX_RESULT_LENGTH];
   if (!ReadAttributeFromSysDevice("modelname", buffer))
      return SYSINFO_RC_ERROR;
   char *s = strchr(buffer, ',');
   if (s != nullptr)
   {
      s++;
      ret_mbstring(value, s);
   }
   else
   {
      ret_mbstring(value, buffer);
   }
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Hardware.System.MachineId parameter
 */
LONG H_HardwareMachineId(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   struct utsname un;
   if (uname(&un) != 0)
      return SYSINFO_RC_ERROR;
   ret_mbstring(value, un.machine);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.OS.* parameters
 */
LONG H_OSInfo(const TCHAR* cmd, const TCHAR* arg, TCHAR* value, AbstractCommSession* session)
{
   struct utsname un;
   if (uname(&un) != 0)
   {
      nxlog_debug_tag(AIX_DEBUG_TAG, 4, _T("Failed to load info from uname"));
      return SYSINFO_RC_ERROR;
   }

   switch (*arg)
   {
      case 'N': // ProductName
         ret_mbstring(value, un.sysname);
         break;
      case 'V': // Version
         char version[8];
         snprintf(version, 8, "%s.%s", un.version, un.release);
         ret_mbstring(value, version);
         break;
      default:
         return SYSINFO_RC_ERROR;
   }

   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.OS.ServicePack parameter
 */
LONG H_OSServicePack(const TCHAR* cmd, const TCHAR* arg, TCHAR* value, AbstractCommSession* session)
{

   LineOutputProcessExecutor pe(_T("oslevel -s"));
   if (!pe.execute())
   {
      nxlog_debug_tag(AIX_DEBUG_TAG, 4, _T("Failed to execute oslevel command"));
      return SYSINFO_RC_ERROR;
   }
   if (!pe.waitForCompletion(10000))
   {
      nxlog_debug_tag(AIX_DEBUG_TAG, 4, _T("Failed to execute oslevel: command timed out"));
      return SYSINFO_RC_ERROR;
   }
   ret_string(value, pe.getData().get(0));

   return SYSINFO_RC_SUCCESS;
}
