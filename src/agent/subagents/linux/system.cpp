/*
** NetXMS subagent for GNU/Linux
** Copyright (C) 2004-2016 Raden Solutions
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

#include "linux_subagent.h"
#include <sys/ipc.h>
#include <sys/msg.h>

/**
 * Handler for System.ConnectedUsers parameter
 */
LONG H_ConnectedUsers(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
	LONG nRet = SYSINFO_RC_ERROR;
	FILE *f;
	struct utmp rec;
	int nCount = 0;

	f = fopen(UTMP_FILE, "r");
	if (f != NULL)
	{
		nRet = SYSINFO_RC_SUCCESS;
		while(fread(&rec, sizeof(rec), 1, f) == 1)
		{
			if (rec.ut_type == USER_PROCESS)
			{
				nCount++;
			}
		}
		fclose(f);
		ret_uint(pValue, nCount);
	}

	return nRet;
}

/**
 * Handler for System.ActiveUserSessions enum
 */
LONG H_ActiveUserSessions(const TCHAR *pszParam, const TCHAR *pArg, StringList *pValue, AbstractCommSession *session)
{
	LONG nRet = SYSINFO_RC_ERROR;
	FILE *f;
	struct utmp rec;
	TCHAR szBuffer[1024];

	f = fopen(UTMP_FILE, "r");
	if (f != NULL)
	{
		nRet = SYSINFO_RC_SUCCESS;
		while(fread(&rec, sizeof(rec), 1, f) == 1)
		{
			if (rec.ut_type == USER_PROCESS)
			{
				_sntprintf(szBuffer, 1024, _T("\"%hs\" \"%hs\" \"%hs\""), rec.ut_user,
				           rec.ut_line, rec.ut_host);
				pValue->add(szBuffer);
			}
		}
		fclose(f);
	}

	return nRet;
}

/**
 * Handler for System.Uptime parameter
 */
LONG H_Uptime(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
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

			if (sscanf(szTmp, "%lf", &dTmp) == 1)
			{
				uUptime = (unsigned int)dTmp;
			}
		}
		fclose(hFile);
	}

	if (uUptime > 0)
	{
		ret_uint(pValue, uUptime);
	}

	return uUptime > 0 ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
}

/**
 * Handler for System.Uname parameter
 */
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

/**
 * Handler for System.CPU.LoadAvg parameters
 */
LONG H_CpuLoad(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
	int nRet = SYSINFO_RC_ERROR;
	struct statvfs s;
	FILE *hFile;

	hFile = fopen("/proc/loadavg", "r");
	if (hFile != NULL)
	{
		char szTmp[64];

		if (fgets(szTmp, sizeof(szTmp), hFile) != NULL)
		{
			double dLoad1, dLoad5, dLoad15;

			if (sscanf(szTmp, "%lf %lf %lf", &dLoad1, &dLoad5, &dLoad15) == 3)
			{
				switch (CAST_FROM_POINTER(pArg, int))
				{
					case INTERVAL_5MIN:
						ret_double(pValue, dLoad5);
						break;
					case INTERVAL_15MIN:
						ret_double(pValue, dLoad15);
						break;
					default: // 1 min
						ret_double(pValue, dLoad1);
						break;
				}
				nRet = SYSINFO_RC_SUCCESS;
			}
		}

		fclose(hFile);
	}

	return nRet;
}

/**
 * Get memory zones low watermark
 */
static long GetMemoryZonesLowWatermark()
{
   FILE *hFile = fopen("/proc/zoneinfo", "r");
   if (hFile == NULL)
      return 0;

   long total = 0;
   bool zoneHeaderScanned = false;
   bool lowFieldScaned = false;
   char buffer[256];
   while(fgets(buffer, sizeof(buffer), hFile) != NULL)
   {
      long v;
      if (sscanf(buffer, "Node %ld, zone %*s\n", &v) == 1)
      {
         zoneHeaderScanned = true;
         lowFieldScaned = false;
         continue;
      }

      if (sscanf(buffer, " low %ld\n", &v) == 1)
      {
         if (zoneHeaderScanned && !lowFieldScaned)
         {
            total += v;
            lowFieldScaned = true;
         }
         continue;
      }
   }
   fclose(hFile);

   return total * getpagesize() / 1024;   // convert pages to KB
}

/**
 * Memory counters
 */
static long s_memTotal = 0;
static long s_memFree = 0;
static long s_memAvailable = 0;
static long s_memBuffers = 0;
static long s_memCached = 0;
static long s_memFileActive = 0;
static long s_memFileInactive = 0;
static long s_memSlabReclaimable = 0;
static long s_swapTotal = 0;
static long s_swapFree = 0;
static INT64 s_memStatTimestamp = 0;
static MUTEX s_memStatLock = MutexCreate();

/**
 * Collect memory usage info
 */
static bool CollectMemoryUsageInfo()
{
   FILE *hFile = fopen("/proc/meminfo", "r");
   if (hFile == NULL)
      return false;

   bool memAvailPresent = false;
   char buffer[256];
   while(fgets(buffer, sizeof(buffer), hFile) != NULL)
   {
      if (sscanf(buffer, "MemTotal: %lu kB\n", &s_memTotal) == 1)
         continue;
      if (sscanf(buffer, "MemFree: %lu kB\n", &s_memFree) == 1)
         continue;
      if (sscanf(buffer, "MemAvailable: %lu kB\n", &s_memAvailable) == 1)
      {
         memAvailPresent = true;
         continue;
      }
      if (sscanf(buffer, "SwapTotal: %lu kB\n", &s_swapTotal) == 1)
         continue;
      if (sscanf(buffer, "SwapFree: %lu kB\n", &s_swapFree) == 1)
         continue;
      if (sscanf(buffer, "Buffers: %lu kB\n", &s_memBuffers) == 1)
         continue;
      if (sscanf(buffer, "Cached: %lu kB\n", &s_memCached) == 1)
         continue;
      if (sscanf(buffer, "Active(file): %lu kB\n", &s_memFileActive) == 1)
         continue;
      if (sscanf(buffer, "Inactive(file): %lu kB\n", &s_memFileInactive) == 1)
         continue;
      if (sscanf(buffer, "SReclaimable: %lu kB\n", &s_memSlabReclaimable) == 1)
         continue;
   }

   fclose(hFile);

   if (!memAvailPresent)
   {
      // Available memory calculation taken from the following kernel patch:
      // https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/commit/?id=34e431b0ae398fc54ea69ff85ec700722c9da773
      long lowWatermark = GetMemoryZonesLowWatermark();
      s_memAvailable = s_memFree - lowWatermark;
      long pageCache = s_memFileActive + s_memFileInactive;
      s_memAvailable += pageCache - std::min(pageCache / 2, lowWatermark);
      s_memAvailable += s_memSlabReclaimable - std::min(s_memSlabReclaimable / 2, lowWatermark);
      if (s_memAvailable < 0)
         s_memAvailable = 0;
   }

   s_memStatTimestamp = GetCurrentTimeMs();
   return true;
}

/**
 * Handler for System.Memory.* parameters
 */
LONG H_MemoryInfo(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   MutexLock(s_memStatLock);
   if (s_memStatTimestamp < GetCurrentTimeMs() - 1000)
   {
      if (!CollectMemoryUsageInfo())
      {
         MutexUnlock(s_memStatLock);
         return SYSINFO_RC_ERROR;
      }
   }

   LONG rc = SYSINFO_RC_SUCCESS;
   switch((long)pArg)
   {
      case PHYSICAL_FREE: // ph-free
         ret_uint64(pValue, ((QWORD)s_memFree) * 1024);
         break;
      case PHYSICAL_FREE_PCT: // ph-free percentage
         ret_double(pValue, ((double)s_memFree * 100.0 / s_memTotal), 2);
         break;
      case PHYSICAL_TOTAL: // ph-total
         ret_uint64(pValue, ((QWORD)s_memTotal) * 1024);
         break;
      case PHYSICAL_USED: // ph-used
         ret_uint64(pValue, ((QWORD)(s_memTotal - s_memFree)) * 1024);
         break;
      case PHYSICAL_USED_PCT: // ph-used percentage
         ret_double(pValue, (((double)s_memTotal - s_memFree) * 100.0 / s_memTotal), 2);
         break;
      case PHYSICAL_AVAILABLE:
         ret_uint64(pValue, ((QWORD)s_memAvailable) * 1024);
         break;
      case PHYSICAL_AVAILABLE_PCT:
         ret_double(pValue, ((double)s_memAvailable * 100.0 / s_memTotal), 2);
         break;
      case PHYSICAL_CACHED:
         ret_uint64(pValue, ((QWORD)s_memCached) * 1024);
         break;
      case PHYSICAL_CACHED_PCT:
         ret_double(pValue, ((double)s_memCached * 100.0 / s_memTotal), 2);
         break;
      case PHYSICAL_BUFFERS:
         ret_uint64(pValue, ((QWORD)s_memBuffers) * 1024);
         break;
      case PHYSICAL_BUFFERS_PCT:
         ret_double(pValue, ((double)s_memBuffers * 100.0 / s_memTotal), 2);
         break;
      case SWAP_FREE: // sw-free
         ret_uint64(pValue, ((QWORD)s_swapFree) * 1024);
         break;
      case SWAP_FREE_PCT: // sw-free percentage
         if (s_swapTotal > 0)
            ret_double(pValue, ((double)s_swapFree * 100.0 / s_swapTotal), 2);
         else
            ret_double(pValue, 100.0, 2);
         break;
      case SWAP_TOTAL: // sw-total
         ret_uint64(pValue, ((QWORD)s_swapTotal) * 1024);
         break;
      case SWAP_USED: // sw-used
         ret_uint64(pValue, ((QWORD)(s_swapTotal - s_swapFree)) * 1024);
         break;
      case SWAP_USED_PCT: // sw-used percentage
         if (s_swapTotal > 0)
            ret_double(pValue, (((double)s_swapTotal - s_swapFree) * 100.0 / s_swapTotal), 2);
         else
            ret_double(pValue, 0.0, 2);
         break;
      case VIRTUAL_FREE: // vi-free
         ret_uint64(pValue, ((QWORD)s_memFree + (QWORD)s_swapFree) * 1024);
         break;
      case VIRTUAL_FREE_PCT: // vi-free percentage
         ret_double(pValue, (((double)s_memFree + s_swapFree) * 100.0 / (s_memTotal + s_swapTotal)), 2);
         break;
      case VIRTUAL_TOTAL: // vi-total
         ret_uint64(pValue, ((QWORD)s_memTotal + (QWORD)s_swapTotal) * 1024);
         break;
      case VIRTUAL_USED: // vi-used
         ret_uint64(pValue, ((QWORD)(s_memTotal - s_memFree) + (QWORD)(s_swapTotal - s_swapFree)) * 1024);
         break;
      case VIRTUAL_USED_PCT: // vi-used percentage
         ret_double(pValue, ((((double)s_memTotal - s_memFree) + (s_swapTotal - s_swapFree)) * 100.0 / (s_memTotal + s_swapTotal)), 2);
         break;
      case VIRTUAL_AVAILABLE:
         ret_uint64(pValue, ((QWORD)s_memAvailable + (QWORD)s_swapFree) * 1024);
         break;
      case VIRTUAL_AVAILABLE_PCT:
         ret_double(pValue, (((double)s_memAvailable + s_swapFree) * 100.0 / (s_memTotal + s_swapTotal)), 2);
         break;
      default: // error
         rc = SYSINFO_RC_UNSUPPORTED;
         break;
   }
   MutexUnlock(s_memStatLock);
	return rc;
}

/**
 * Handler for Agent.SourcePackageSupport parameter
 */
LONG H_SourcePkgSupport(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
	ret_int(pValue, 1);
	return SYSINFO_RC_SUCCESS;
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
         ret_uint64(value, (UINT64)data.__msg_cbytes);
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
