/*
** NetXMS subagent for GNU/Linux
** Copyright (C) 2004-2025 Raden Solutions
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
#include <sys/statvfs.h>
#include <sys/utsname.h>
#include <utmp.h>

/**
 * Handler for System.ConnectedUsers parameter
 */
LONG H_ConnectedUsers(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
	FILE *f = fopen(UTMP_FILE, "r");
	if (f == nullptr)
	   return SYSINFO_RC_ERROR;

   int count = 0;
   struct utmp rec;
   while(fread(&rec, sizeof(rec), 1, f) == 1)
   {
      if (rec.ut_type == USER_PROCESS)
         count++;
   }
   fclose(f);
   ret_uint(value, count);
	return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.ActiveUserSessions list
 */
LONG H_UserSessionList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
	FILE *f = fopen(UTMP_FILE, "r");
	if (f == nullptr)
	   return SYSINFO_RC_ERROR;

   struct utmp rec;
   while(fread(&rec, sizeof(rec), 1, f) == 1)
   {
      if (rec.ut_type == USER_PROCESS)
      {
         TCHAR buffer[1024];
         _sntprintf(buffer, 1024, _T("\"%hs\" \"%hs\" \"%hs\""), rec.ut_user, rec.ut_line, rec.ut_host);
         value->add(buffer);
      }
   }
   fclose(f);

	return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.ActiveUserSessions table
 */
LONG H_UserSessionTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   FILE *f = fopen(UTMP_FILE, "r");
   if (f == nullptr)
      return SYSINFO_RC_ERROR;

   value->addColumn(_T("ID"), DCI_DT_UINT, _T("ID"), true);
   value->addColumn(_T("USER_NAME"), DCI_DT_STRING, _T("User name"));
   value->addColumn(_T("TERMINAL"), DCI_DT_STRING, _T("Terminal"));
   value->addColumn(_T("STATE"), DCI_DT_STRING, _T("State"));
   value->addColumn(_T("CLIENT_NAME"), DCI_DT_STRING, _T("Client name"));
   value->addColumn(_T("CLIENT_ADDRESS"), DCI_DT_STRING, _T("Client address"));
   value->addColumn(_T("CLIENT_DISPLAY"), DCI_DT_STRING, _T("Client display"));
   value->addColumn(_T("CONNECT_TIMESTAMP"), DCI_DT_UINT64, _T("Connect time"));
   value->addColumn(_T("LOGON_TIMESTAMP"), DCI_DT_UINT64, _T("Logon time"));
   value->addColumn(_T("IDLE_TIME"), DCI_DT_UINT, _T("Idle for"));
   value->addColumn(_T("AGENT_TYPE"), DCI_DT_INT, _T("Agent type"));
   value->addColumn(_T("AGENT_PID"), DCI_DT_UINT, _T("Agent PID"));

   char tty[128] = "/dev/";
   struct utmp rec;
   while(fread(&rec, sizeof(rec), 1, f) == 1)
   {
      if (rec.ut_type != USER_PROCESS)
         continue;

      value->addRow();
      value->set(0, rec.ut_pid);
      value->set(1, rec.ut_user);
      value->set(2, rec.ut_line);
      value->set(3, _T("Active"));
      value->set(4, rec.ut_host);

      InetAddress addr = InetAddress::parse(rec.ut_host);
      if (addr.isValid())
         value->set(5, addr.toString());

      value->set(8, static_cast<int64_t>(rec.ut_tv.tv_sec));

      strlcpy(&tty[5], rec.ut_line, 123);
      struct stat st;
      if (stat(tty, &st) == 0)
         value->set(9, static_cast<int64_t>(time(nullptr) - st.st_atime));

      value->set(10, -1);
      value->set(11, 0);
   }
   fclose(f);

   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.Uptime parameter
 */
LONG H_Uptime(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   struct timespec ts;
   if (clock_gettime(CLOCK_BOOTTIME, &ts) != 0)
      return SYSINFO_RC_ERROR;

   ret_uint(value, static_cast<uint32_t>(ts.tv_sec));
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.Uname parameter
 */
LONG H_Uname(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
	struct utsname utsName;
	if (uname(&utsName) != 0)
	   return SYSINFO_RC_ERROR;

   char buffer[1024];
   snprintf(buffer, sizeof(buffer), "%s %s %s %s %s", utsName.sysname, utsName.nodename, utsName.release, utsName.version, utsName.machine);
   ret_mbstring(pValue, buffer);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.CPU.LoadAvg parameters
 */
LONG H_CpuLoad(const TCHAR *metric, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   char buffer[64];
   if (!ReadLineFromFileA("/proc/loadavg", buffer, sizeof(buffer)))
	   return SYSINFO_RC_ERROR;

   double load1, load5, load15;
   if (sscanf(buffer, "%lf %lf %lf", &load1, &load5, &load15) != 3)
      return SYSINFO_RC_ERROR;

   switch (CAST_FROM_POINTER(arg, int))
   {
      case INTERVAL_5MIN:
         ret_double(value, load5);
         break;
      case INTERVAL_15MIN:
         ret_double(value, load15);
         break;
      default: // 1 min
         ret_double(value, load1);
         break;
   }
	return SYSINFO_RC_SUCCESS;
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
static int64_t s_memStatTimestamp = 0;
static Mutex s_memStatLock(MutexType::FAST);

/**
 * Collect memory usage info
 */
static bool CollectMemoryUsageInfo()
{
   FILE *hFile = fopen("/proc/meminfo", "r");
   if (hFile == nullptr)
      return false;

   bool memAvailPresent = false;
   char buffer[256];
   while(fgets(buffer, sizeof(buffer), hFile) != nullptr)
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

   s_memStatTimestamp = GetMonotonicClockTime();
   return true;
}

/**
 * Get total memory size (in kb)
 */
uint64_t GetTotalMemorySize()
{
   if (s_memTotal == 0)
   {
      s_memStatLock.lock();
      CollectMemoryUsageInfo();
      s_memStatLock.unlock();
   }
   return s_memTotal;
}

/**
 * Handler for System.Memory.* parameters
 */
LONG H_MemoryInfo(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   s_memStatLock.lock();
   if (s_memStatTimestamp < GetMonotonicClockTime() - 1000)
   {
      if (!CollectMemoryUsageInfo())
      {
         s_memStatLock.unlock();
         return SYSINFO_RC_ERROR;
      }
   }

   LONG rc = SYSINFO_RC_SUCCESS;
   switch((long)pArg)
   {
      case PHYSICAL_FREE: // ph-free
         ret_uint64(pValue, ((uint64_t)s_memFree) * 1024);
         break;
      case PHYSICAL_FREE_PCT: // ph-free percentage
         ret_double(pValue, ((double)s_memFree * 100.0 / s_memTotal), 2);
         break;
      case PHYSICAL_TOTAL: // ph-total
         ret_uint64(pValue, ((uint64_t)s_memTotal) * 1024);
         break;
      case PHYSICAL_USED: // ph-used
         ret_uint64(pValue, ((uint64_t)(s_memTotal - s_memFree)) * 1024);
         break;
      case PHYSICAL_USED_PCT: // ph-used percentage
         ret_double(pValue, (((double)s_memTotal - s_memFree) * 100.0 / s_memTotal), 2);
         break;
      case PHYSICAL_AVAILABLE:
         ret_uint64(pValue, ((uint64_t)s_memAvailable) * 1024);
         break;
      case PHYSICAL_AVAILABLE_PCT:
         ret_double(pValue, ((double)s_memAvailable * 100.0 / s_memTotal), 2);
         break;
      case PHYSICAL_CACHED:
         ret_uint64(pValue, ((uint64_t)s_memCached) * 1024);
         break;
      case PHYSICAL_CACHED_PCT:
         ret_double(pValue, ((double)s_memCached * 100.0 / s_memTotal), 2);
         break;
      case PHYSICAL_BUFFERS:
         ret_uint64(pValue, ((uint64_t)s_memBuffers) * 1024);
         break;
      case PHYSICAL_BUFFERS_PCT:
         ret_double(pValue, ((double)s_memBuffers * 100.0 / s_memTotal), 2);
         break;
      case SWAP_FREE: // sw-free
         ret_uint64(pValue, ((uint64_t)s_swapFree) * 1024);
         break;
      case SWAP_FREE_PCT: // sw-free percentage
         if (s_swapTotal > 0)
            ret_double(pValue, ((double)s_swapFree * 100.0 / s_swapTotal), 2);
         else
            ret_double(pValue, 100.0, 2);
         break;
      case SWAP_TOTAL: // sw-total
         ret_uint64(pValue, ((uint64_t)s_swapTotal) * 1024);
         break;
      case SWAP_USED: // sw-used
         ret_uint64(pValue, ((uint64_t)(s_swapTotal - s_swapFree)) * 1024);
         break;
      case SWAP_USED_PCT: // sw-used percentage
         if (s_swapTotal > 0)
            ret_double(pValue, (((double)s_swapTotal - s_swapFree) * 100.0 / s_swapTotal), 2);
         else
            ret_double(pValue, 0.0, 2);
         break;
      case VIRTUAL_FREE: // vi-free
         ret_uint64(pValue, ((uint64_t)s_memFree + (uint64_t)s_swapFree) * 1024);
         break;
      case VIRTUAL_FREE_PCT: // vi-free percentage
         ret_double(pValue, (((double)s_memFree + s_swapFree) * 100.0 / (s_memTotal + s_swapTotal)), 2);
         break;
      case VIRTUAL_TOTAL: // vi-total
         ret_uint64(pValue, ((uint64_t)s_memTotal + (uint64_t)s_swapTotal) * 1024);
         break;
      case VIRTUAL_USED: // vi-used
         ret_uint64(pValue, ((uint64_t)(s_memTotal - s_memFree) + (uint64_t)(s_swapTotal - s_swapFree)) * 1024);
         break;
      case VIRTUAL_USED_PCT: // vi-used percentage
         ret_double(pValue, ((((double)s_memTotal - s_memFree) + (s_swapTotal - s_swapFree)) * 100.0 / (s_memTotal + s_swapTotal)), 2);
         break;
      case VIRTUAL_AVAILABLE:
         ret_uint64(pValue, ((uint64_t)s_memAvailable + (uint64_t)s_swapFree) * 1024);
         break;
      case VIRTUAL_AVAILABLE_PCT:
         ret_double(pValue, (((double)s_memAvailable + s_swapFree) * 100.0 / (s_memTotal + s_swapTotal)), 2);
         break;
      default: // error
         rc = SYSINFO_RC_UNSUPPORTED;
         break;
   }
   s_memStatLock.unlock();
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

/**
 * Handler for System.OS.* parameters
 */
LONG H_OSInfo(const TCHAR* cmd, const TCHAR* arg, TCHAR* value, AbstractCommSession* session)
{
   Config parser;
   if (!parser.loadIniConfig(_T("/etc/os-release"), _T("os")))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Failed to parse /etc/os-release file"));
      return SYSINFO_RC_ERROR;
   }

   const TCHAR* data = nullptr;
   switch (*arg)
   {
      case 'B': // Build
         data = parser.getValue(_T("/os/BUILD_ID"));
         break;

      case 'N': // ProductName
         data = parser.getValue(_T("/os/PRETTY_NAME"));
         if (data == nullptr)
         {
            data = parser.getValue(_T("/os/NAME"));
            if (data == nullptr)
               data = parser.getValue(_T("/os/ID"));
         }
         break;

      case 'T': // ProductType
         data = parser.getValue(_T("/os/VARIANT"));
         break;

      case 'V': // Version
         data = parser.getValue(_T("/os/VERSION"));
         if (data == nullptr)
            data = parser.getValue(_T("/os/VERSION_ID"));
         break;
   }

   if (data == nullptr)
      return SYSINFO_RC_UNSUPPORTED;

   ret_string(value, data);

   return SYSINFO_RC_SUCCESS;
}
