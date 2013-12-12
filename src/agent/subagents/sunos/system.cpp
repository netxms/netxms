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
#include <sys/sysinfo.h>
#include <sys/systeminfo.h>

/**
 * Handler for System.Uname parameter
 */
LONG H_Uname(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
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
LONG H_Uptime(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
{
	kstat_ctl_t *kc;
	kstat_t *kp;
	kstat_named_t *kn;
	DWORD hz, secs;
	LONG nRet = SYSINFO_RC_ERROR;

	hz = sysconf(_SC_CLK_TCK);

	// Open kstat
	kstat_lock();
	kc = kstat_open();
	if (kc != NULL)
	{
		// read uptime counter
		kp = kstat_lookup(kc, (char *)"unix", 0, (char *)"system_misc");
		if (kp != NULL)
		{
			if(kstat_read(kc, kp, 0) != -1)
			{
				kn = (kstat_named_t *)kstat_data_lookup(kp, (char *)"clk_intr");
				if (kn != NULL)
				{
					secs = kn->value.ul / hz;
					ret_uint(pValue, secs);
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
 * Handler for System.Hostname parameter
 */
LONG H_Hostname(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
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
LONG H_LoadAvg(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
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
LONG H_KStat(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
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
LONG H_CPUCount(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
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
static MUTEX s_swapInfoMutex = MutexCreate();
static UINT64 s_swapUsed = 0;
static UINT64 s_swapFree = 0;
static UINT64 s_swapTotal = 0;

/**
 * Update swap info
 */
static void UpdateSwapInfo()
{
	kstat_lock();
	kstat_ctl_t *kc = kstat_open();
	if (kc != NULL)
   {
      kstat_unlock();
      
      struct vminfo v1, v2;
      if (ReadVMInfo(kc, &v1))
      {
         ThreadSleep(1);
         if (ReadVMInfo(kc, &v2))
         {
            s_swapUsed = v2.swap_alloc - v1.swap_alloc;
            s_swapFree = v2.swap_free - v1.swap_free;
            s_swapTotal = s_swapUsed + s_swapFree;
         }
      }
      
      kstat_lock();
      kstat_close(kc);
   }
   else
	{
      AgentWriteDebugLog(6, _T("SunOS: kstat_open failed in UpdateSwapInfo"));
      return;
   }
	kstat_unlock();
}

/**
 * Get swap info counter, calling update if needed
 */
static UINT64 GetSwapCounter(UINT64 *cnt)
{
   MutexLock(s_swapInfoMutex);
   time_t now = time(NULL);
   if (now - s_lastSwapInfoUpdate > 10)   // older then 10 seconds
   {
      UpdateSwapInfo();
      s_lastSwapInfoUpdate = now;
   }
   INT64 result = *cnt;
   MutexUnlock(s_swapInfoMutex);
   return result;
}

/**
 * Handler for System.Memory.* parameters
 */
LONG H_MemoryInfo(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
{
	LONG nRet = SYSINFO_RC_SUCCESS;
	kstat_named_t kn;
	QWORD qwPageSize;

	qwPageSize = sysconf(_SC_PAGESIZE);

	switch(CAST_FROM_POINTER(pArg, int))
	{
		case MEMINFO_PHYSICAL_TOTAL:
			ret_uint64(pValue, (QWORD)sysconf(_SC_PHYS_PAGES) * qwPageSize);
			break;
		case MEMINFO_PHYSICAL_FREE:
			nRet = ReadKStatValue("unix", 0, "system_pages", "freemem", NULL, &kn);
			if (nRet == SYSINFO_RC_SUCCESS)
			{
				ret_uint64(pValue, (QWORD)kn.value.ul * qwPageSize);
			}
			break;
		case MEMINFO_PHYSICAL_FREEPCT:
			nRet = ReadKStatValue("unix", 0, "system_pages", "freemem", NULL, &kn);
			if (nRet == SYSINFO_RC_SUCCESS)
			{
				ret_double(pValue, (double)kn.value.ul * 100.0 / (double)sysconf(_SC_PHYS_PAGES));
			}
			break;
		case MEMINFO_PHYSICAL_USED:
			nRet = ReadKStatValue("unix", 0, "system_pages", "freemem", NULL, &kn);
			if (nRet == SYSINFO_RC_SUCCESS)
			{
				ret_uint64(pValue, (QWORD)(sysconf(_SC_PHYS_PAGES) - kn.value.ul) * qwPageSize);
			}
			break;
		case MEMINFO_PHYSICAL_USEDPCT:
			nRet = ReadKStatValue("unix", 0, "system_pages", "freemem", NULL, &kn);
			if (nRet == SYSINFO_RC_SUCCESS)
			{
				ret_double(pValue, (double)(sysconf(_SC_PHYS_PAGES) - kn.value.ul) * 100.0 /  (double)sysconf(_SC_PHYS_PAGES));
			}
			break;
		case MEMINFO_SWAP_TOTAL:
			ret_uint64(pValue, GetSwapCounter(&s_swapTotal) * qwPageSize);
			break;
		case MEMINFO_SWAP_FREE:
         ret_uint64(pValue, GetSwapCounter(&s_swapFree) * qwPageSize);
			break;
		case MEMINFO_SWAP_FREEPCT:
         ret_double(pValue, (double)GetSwapCounter(&s_swapFree) * 100.0 / (double)GetSwapCounter(&s_swapTotal));
			break;
		case MEMINFO_SWAP_USED:
         ret_uint64(pValue, GetSwapCounter(&s_swapUsed) * qwPageSize);
			break;
		case MEMINFO_SWAP_USEDPCT:
         ret_double(pValue, (double)GetSwapCounter(&s_swapUsed) * 100.0 / (double)GetSwapCounter(&s_swapTotal));
			break;
		case MEMINFO_VIRTUAL_TOTAL:
			ret_uint64(pValue, ((QWORD)sysconf(_SC_PHYS_PAGES) + GetSwapCounter(&s_swapTotal)) * qwPageSize);
			break;
		case MEMINFO_VIRTUAL_FREE:
			nRet = ReadKStatValue("unix", 0, "system_pages", "freemem", NULL, &kn);
			if (nRet == SYSINFO_RC_SUCCESS)
			{
				ret_uint64(pValue, ((QWORD)kn.value.ul + GetSwapCounter(&s_swapFree)) * qwPageSize);
			}
			break;
		case MEMINFO_VIRTUAL_FREEPCT:
			nRet = ReadKStatValue("unix", 0, "system_pages", "freemem", NULL, &kn);
			if (nRet == SYSINFO_RC_SUCCESS)
			{
				ret_double(pValue, ((double)kn.value.ul + (double)GetSwapCounter(&s_swapFree)) * 100.0 / ((double)sysconf(_SC_PHYS_PAGES) + (double)GetSwapCounter(&s_swapTotal)));
			}
			break;
		case MEMINFO_VIRTUAL_USED:
			nRet = ReadKStatValue("unix", 0, "system_pages", "freemem", NULL, &kn);
			if (nRet == SYSINFO_RC_SUCCESS)
			{
				ret_uint64(pValue, ((QWORD)(sysconf(_SC_PHYS_PAGES) - kn.value.ul) + GetSwapCounter(&s_swapUsed)) * qwPageSize);
			}
			break;
		case MEMINFO_VIRTUAL_USEDPCT:
			nRet = ReadKStatValue("unix", 0, "system_pages", "freemem", NULL, &kn);
			if (nRet == SYSINFO_RC_SUCCESS)
			{
				ret_double(pValue, ((double)(sysconf(_SC_PHYS_PAGES) - kn.value.ul) + (double)GetSwapCounter(&s_swapUsed)) * 100.0 /  ((double)sysconf(_SC_PHYS_PAGES) + (double)GetSwapCounter(&s_swapTotal)));
			}
			break;
		default:
			nRet = SYSINFO_RC_UNSUPPORTED;
			break;
	}

	return nRet;
}
