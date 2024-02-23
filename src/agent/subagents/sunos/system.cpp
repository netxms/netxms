/*
** NetXMS subagent for SunOS/Solaris
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

#include "sunos_subagent.h"

// one of system headers includes conflicting declaration of struct uuid
// the following define will hide it
#define uuid ___uuid

#include <sys/sysinfo.h>
#include <sys/systeminfo.h>
#include <sys/swap.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#undef uuid

/**
 * Handler for System.Uname parameter
 */
LONG H_Uname(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   char szSysStr[7][64];
   int i;
   LONG nRet = SYSINFO_RC_SUCCESS;
   static int nSysCode[7] =
   {
      SI_SYSNAME,
      SI_HOSTNAME,
      SI_RELEASE,
      SI_VERSION,
      SI_MACHINE,
      SI_ARCHITECTURE,
      SI_PLATFORM
   };

   for(i = 0; i < 7; i++)
      if (sysinfo(nSysCode[i], szSysStr[i], 64) == -1)
      {
         nRet = SYSINFO_RC_ERROR;
         break;
      }

   if (nRet == SYSINFO_RC_SUCCESS)
   {
      _sntprintf(pValue, MAX_RESULT_LENGTH, _T("%hs %hs %hs %hs %hs %hs %hs"),
            szSysStr[0], szSysStr[1], szSysStr[2], szSysStr[3],
            szSysStr[4], szSysStr[5], szSysStr[6]);
   }

   return nRet;
}

/**
 * Handler for System.Uptime parameter
 */
LONG H_Uptime(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   ret_uint(pValue, gethrtime()/1000000000);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.Hostname parameter
 */
LONG H_Hostname(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
#ifdef UNICODE
   char buffer[MAX_RESULT_LENGTH];
   if (sysinfo(SI_HOSTNAME, buffer, MAX_RESULT_LENGTH) == -1)
      return SYSINFO_RC_ERROR;
   ret_mbstring(pValue, buffer);
   return SYSINFO_RC_SUCCESS;
#else
   return (sysinfo(SI_HOSTNAME, pValue, MAX_RESULT_LENGTH) == -1) ?
      SYSINFO_RC_ERROR : SYSINFO_RC_SUCCESS;
#endif
}

/**
 * Handler for System.CPU.LoadAvg
 */
LONG H_LoadAvg(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   kstat_ctl_t *kc;
   kstat_t *kp;
   kstat_named_t *kn;
   LONG nRet = SYSINFO_RC_ERROR;
   static char *szParam[] = { (char *)"avenrun_1min", (char *)"avenrun_5min", (char *)"avenrun_15min" };

   kstat_lock();
   kc = kstat_open();
   if (kc != NULL)
   {
      kp = kstat_lookup(kc, (char *)"unix", 0, (char *)"system_misc");
      if (kp != NULL)
      {
         if(kstat_read(kc, kp, 0) != -1)
         {
            kn = (kstat_named_t *)kstat_data_lookup(kp, szParam[CAST_FROM_POINTER(pArg, int)]);
            if (kn != NULL)
            {
               _sntprintf(pValue, MAX_RESULT_LENGTH, _T("%0.02f"), (double)kn->value.ui32 / 256.0);
               nRet = SYSINFO_RC_SUCCESS;
            }
         }
      }
      kstat_close(kc);
   }
   kstat_unlock();

   return nRet;
}

/**
 * Handler for System.KStat(*)
 */
LONG H_KStat(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   char *eptr, szModule[128], szName[128], szInstance[16], szStat[128];
   LONG nInstance;

   // Read parameters
   if ((!AgentGetParameterArgA(pszParam, 1, szModule, 128)) ||
         (!AgentGetParameterArgA(pszParam, 2, szInstance, 16)) ||
         (!AgentGetParameterArgA(pszParam, 3, szName, 128)) ||
         (!AgentGetParameterArgA(pszParam, 4, szStat, 128)))
      return SYSINFO_RC_UNSUPPORTED;

   if (szInstance[0] != 0)
   {
      nInstance = strtol(szInstance, &eptr, 0);
      if (*eptr != 0)
         return SYSINFO_RC_UNSUPPORTED;
   }
   else
   {
      nInstance = 0;
   }

   return ReadKStatValue(szModule, nInstance, szName, szStat, pValue, NULL);
}

/**
 * Handler for System.CPU.Count
 */
LONG H_CPUCount(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   return ReadKStatValue("unix", 0, "system_misc", "ncpus", pValue, NULL);
}

/**
 * Handler for generic kstat parameter
 */
LONG ReadKStatValue(const char *pszModule, LONG nInstance, const char *pszName,
      const char *pszStat, TCHAR *pValue, kstat_named_t *pRawValue)
{
   kstat_ctl_t *kc;
   kstat_t *kp;
   kstat_named_t *kn;
   LONG nRet = SYSINFO_RC_ERROR;

   kstat_lock();
   kc = kstat_open();
   if (kc != NULL)
   {
      kp = kstat_lookup(kc, (char *)pszModule, nInstance, (char *)pszName);
      if (kp != NULL)
      {
         if(kstat_read(kc, kp, 0) != -1)
         {
            kn = (kstat_named_t *)kstat_data_lookup(kp, (char *)pszStat);
            if (kn != NULL)
            {
               if (pValue != NULL)
               {
                  switch(kn->data_type)
                  {
                     case KSTAT_DATA_CHAR:
                        ret_mbstring(pValue, kn->value.c);
                        break;
                     case KSTAT_DATA_INT32:
                        ret_int(pValue, kn->value.i32);
                        break;
                     case KSTAT_DATA_UINT32:
                        ret_uint(pValue, kn->value.ui32);
                        break;
                     case KSTAT_DATA_INT64:
                        ret_int64(pValue, kn->value.i64);
                        break;
                     case KSTAT_DATA_UINT64:
                        ret_uint64(pValue, kn->value.ui64);
                        break;
                     case KSTAT_DATA_FLOAT:
                        ret_double(pValue, kn->value.f);
                        break;
                     case KSTAT_DATA_DOUBLE:
                        ret_double(pValue, kn->value.d);
                        break;
                     default:
                        ret_int(pValue, 0);
                        break;
                  }
               }

               if (pRawValue != NULL)
               {
                  memcpy(pRawValue, kn, sizeof(kstat_named_t));
               }

               nRet = SYSINFO_RC_SUCCESS;
            }
            else
            {
               AgentWriteDebugLog(6, _T("SunOS::ReadKStatValue(%hs,%d,%hs,%hs): kstat_data_lookup failed (%hs)"), pszModule, nInstance, pszName, pszStat, strerror(errno));
            }
         }
         else
         {
            AgentWriteDebugLog(6, _T("SunOS::ReadKStatValue(%hs,%d,%hs,%hs): kstat_read failed (%hs)"), pszModule, nInstance, pszName, pszStat, strerror(errno));
         }
      }
      else
      {
         AgentWriteDebugLog(6, _T("SunOS::ReadKStatValue(%hs,%d,%hs,%hs): kstat_lookup failed (%hs)"), pszModule, nInstance, pszName, pszStat, strerror(errno));
      }
      kstat_close(kc);
   }
   kstat_unlock();

   return nRet;
}

/**
 * Read vminfo structure from kstat
 */
static bool ReadVMInfo(kstat_ctl_t *kc, struct vminfo *info)
{
   kstat_t *kp;
   int i;
   uint_t *pData;
   bool success = false;

   kstat_lock();
   kp = kstat_lookup(kc, (char *)"unix", 0, (char *)"vminfo");
   if (kp != NULL)
   {
      if (kstat_read(kc, kp, NULL) != -1)
      {
         memcpy(info, kp->ks_data, sizeof(struct vminfo));
         success = true;
      }
      else
      {
         AgentWriteDebugLog(6, _T("SunOS: kstat_read failed in ReadVMInfo"));
      }
   }
   else
   {
      AgentWriteDebugLog(6, _T("SunOS: kstat_lookup failed in ReadVMInfo"));
   }
   kstat_unlock();
   return success;
}

/**
 * Last swap info update time
 */
static time_t s_lastSwapInfoUpdate = 0;
static Mutex s_swapInfoMutex;

/**
 * All swap counters are in blocks
 */
static uint64_t s_swapUsed = 0;
static uint64_t s_swapFree = 0;
static uint64_t s_swapTotal = 0;

/**
 * Update swap info
 */
static void UpdateSwapInfo()
{
   static TCHAR METHOD_NAME[] = _T("UpdateSwapInfo");

   int num = swapctl(SC_GETNSWP, nullptr);
   if (num == -1)
   {
      AgentWriteDebugLog(6, _T("%s: %s: call to swapctl(SC_GETNSWP) failed"), AGENT_NAME, METHOD_NAME);
      return;
   }

   swaptbl_t *swapTable = (swaptbl_t *)MemAlloc(num * sizeof(swapent_t) + sizeof(swaptbl_t));
   if (swapTable == nullptr)
   {
      AgentWriteDebugLog(6, _T("%s: %s: failed to allocate the swap table"), AGENT_NAME, METHOD_NAME);
      return;
   }
   swapTable->swt_n = num;
   char buffer[MAXPATHLEN];
   for (int i = 0; i < num; i++)
   {
      swapTable->swt_ent[i].ste_path = buffer;
   }

   int ret = swapctl(SC_LIST, swapTable);
   if (ret == -1)
   {
      AgentWriteDebugLog(6, _T("%s: %s: call to swapctl(SC_LIST) failed"), AGENT_NAME, METHOD_NAME);
      MemFree(swapTable);
      return;
   }

   uint64_t totalBytes = 0;
   uint64_t freeBytes = 0;

   swapent *swapEntry = swapTable->swt_ent;
   for(int i = 0; i < num; i++)
   {
      totalBytes += static_cast<uint64_t>(swapEntry[i].ste_pages);
      freeBytes += static_cast<uint64_t>(swapEntry[i].ste_free);
   }

   MemFree(swapTable);

   s_swapTotal = totalBytes;
   s_swapFree = freeBytes;
   s_swapUsed = totalBytes - freeBytes;
}

/**
 * Get swap info counter, calling update if needed
 */
static uint64_t GetSwapCounter(uint64_t *cnt)
{
   s_swapInfoMutex.lock();
   time_t now = time(nullptr);
   if (now - s_lastSwapInfoUpdate > 10)   // older then 10 seconds
   {
      UpdateSwapInfo();
      s_lastSwapInfoUpdate = now;
   }
   uint64_t result = *cnt;
   s_swapInfoMutex.unlock();
   return result;
}

/**
 * Handler for System.Memory.* parameters
 */
LONG H_MemoryInfo(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   LONG nRet = SYSINFO_RC_SUCCESS;
   kstat_named_t kn;
   uint64_t pageSize = sysconf(_SC_PAGESIZE);

   switch(CAST_FROM_POINTER(pArg, int))
   {
      case MEMINFO_PHYSICAL_TOTAL:
         ret_uint64(pValue, static_cast<uint64_t>(sysconf(_SC_PHYS_PAGES)) * pageSize);
         break;
      case MEMINFO_PHYSICAL_FREE:
         nRet = ReadKStatValue("unix", 0, "system_pages", "freemem", nullptr, &kn);
         if (nRet == SYSINFO_RC_SUCCESS)
         {
            ret_uint64(pValue, (QWORD)kn.value.ul * pageSize);
         }
         break;
      case MEMINFO_PHYSICAL_FREEPCT:
         nRet = ReadKStatValue("unix", 0, "system_pages", "freemem", nullptr, &kn);
         if (nRet == SYSINFO_RC_SUCCESS)
         {
            ret_double(pValue, (double)kn.value.ul * 100.0 / (double)sysconf(_SC_PHYS_PAGES), 2);
         }
         break;
      case MEMINFO_PHYSICAL_USED:
         nRet = ReadKStatValue("unix", 0, "system_pages", "freemem", nullptr, &kn);
         if (nRet == SYSINFO_RC_SUCCESS)
         {
            ret_uint64(pValue, (QWORD)(sysconf(_SC_PHYS_PAGES) - kn.value.ul) * pageSize);
         }
         break;
      case MEMINFO_PHYSICAL_USEDPCT:
         nRet = ReadKStatValue("unix", 0, "system_pages", "freemem", nullptr, &kn);
         if (nRet == SYSINFO_RC_SUCCESS)
         {
            ret_double(pValue, (double)(sysconf(_SC_PHYS_PAGES) - kn.value.ul) * 100.0 / sysconf(_SC_PHYS_PAGES), 2);
         }
         break;
      case MEMINFO_SWAP_TOTAL:
         ret_uint64(pValue, GetSwapCounter(&s_swapTotal) * pageSize);
         break;
      case MEMINFO_SWAP_FREE:
         ret_uint64(pValue, GetSwapCounter(&s_swapFree) * pageSize);
         break;
      case MEMINFO_SWAP_FREEPCT:
         {
            GetSwapCounter(&s_swapTotal);
            ret_double(pValue, s_swapTotal == 0 ? 0 : (double)GetSwapCounter(&s_swapFree) * 100.0 / s_swapTotal, 2);
         }
         break;
      case MEMINFO_SWAP_USED:
         ret_uint64(pValue, GetSwapCounter(&s_swapUsed) * pageSize);
         break;
      case MEMINFO_SWAP_USEDPCT:
         {
            GetSwapCounter(&s_swapTotal);
            ret_double(pValue, s_swapTotal == 0 ? 0 : (double)GetSwapCounter(&s_swapUsed) * 100.0 / s_swapTotal, 2);
         }
         break;
      case MEMINFO_VIRTUAL_TOTAL:
         ret_uint64(pValue, ((UINT64)sysconf(_SC_PHYS_PAGES) + GetSwapCounter(&s_swapTotal)) * pageSize);
         break;
      case MEMINFO_VIRTUAL_FREE:
         nRet = ReadKStatValue("unix", 0, "system_pages", "freemem", nullptr, &kn);
         if (nRet == SYSINFO_RC_SUCCESS)
         {
            ret_uint64(pValue, ((UINT64)kn.value.ul + GetSwapCounter(&s_swapFree)) * pageSize);
         }
         break;
      case MEMINFO_VIRTUAL_FREEPCT:
         nRet = ReadKStatValue("unix", 0, "system_pages", "freemem", nullptr, &kn);
         if (nRet == SYSINFO_RC_SUCCESS)
         {
            UINT64 freeMem = (UINT64)kn.value.ul + GetSwapCounter(&s_swapFree);
            UINT64 totalMem = (UINT64)sysconf(_SC_PHYS_PAGES) + GetSwapCounter(&s_swapTotal);

            ret_double(pValue, (double)(freeMem * 100) / totalMem, 2);
         }
         break;
      case MEMINFO_VIRTUAL_USED:
         nRet = ReadKStatValue("unix", 0, "system_pages", "freemem", nullptr, &kn);
         if (nRet == SYSINFO_RC_SUCCESS)
         {
            ret_uint64(pValue, (UINT64)(sysconf(_SC_PHYS_PAGES) - kn.value.ul + GetSwapCounter(&s_swapUsed)) * pageSize);
         }
         break;
      case MEMINFO_VIRTUAL_USEDPCT:
         nRet = ReadKStatValue("unix", 0, "system_pages", "freemem", nullptr, &kn);
         if (nRet == SYSINFO_RC_SUCCESS)
         {
            UINT64 freeMem = (UINT64)kn.value.ul + GetSwapCounter(&s_swapFree);
            UINT64 totalMem = (UINT64)sysconf(_SC_PHYS_PAGES) + GetSwapCounter(&s_swapTotal);

            ret_double(pValue, (double)(totalMem - freeMem) * 100.0 / totalMem, 2);
         }
         break;
      default:
         nRet = SYSINFO_RC_UNSUPPORTED;
         break;
   }

   return nRet;
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

#ifdef __sparc

/**
 * Handler for hardware information parameters available via sysinfo on Sparc platform
 */
LONG H_SystemHardwareInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   char buffer[256];
   if (sysinfo(CAST_FROM_POINTER(arg, int), buffer, 256) >= 0)
   {
      ret_mbstring(value, buffer);
      return SYSINFO_RC_SUCCESS;
   }
   return SYSINFO_RC_UNSUPPORTED;
}

#endif

bool GetPrettyOSName(char* buff, size_t buflen)
{
   int fd = open("/etc/release", O_RDONLY);
   if (fd == -1)
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Failed to open /etc/release file"));
      return false;
   }

   ssize_t bytes = read(fd, buff, buflen - 1);
   if (bytes < 1)
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Failed to read from /etc/release file"));
      close(fd);
      return false;
   }

   close(fd);

   buff[bytes] = 0;

   char* eol = strchr(buff, '\n');
   if (eol == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Failed to read OS name from /etc/release file - buffer too short"));
      return false;
   }

   *eol = 0;

   TrimA(buff);

   return true;
}

/**
 * Handler for System.OS.* parameters
 */
LONG H_OSInfo(const TCHAR* cmd, const TCHAR* arg, TCHAR* value, AbstractCommSession* session)
{
   char buff[128];
   switch (*arg)
   {
      case 'N': // ProductName
         if (!GetPrettyOSName(buff, 128) && sysinfo(SI_SYSNAME, buff, 128) == -1)
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("Failed to get sysname from sysinfo"));
            return SYSINFO_RC_ERROR;
         }
         ret_mbstring(value, buff);
         break;

      case 'V': // Version
         if (sysinfo(SI_VERSION, buff, 128) == -1)
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("Failed to get version from sysinfo"));
            return SYSINFO_RC_ERROR;
         }
         ret_mbstring(value, buff);
         break;

      default:
         return SYSINFO_RC_ERROR;
   }
   return SYSINFO_RC_SUCCESS;
}
