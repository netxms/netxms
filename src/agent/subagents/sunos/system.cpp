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
#include <sys/systeminfo.h>


//
// Handler for System.Uname parameter
//

LONG H_Uname(const char *pszParam, const char *pArg, char *pValue)
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
		snprintf(pValue, MAX_RESULT_LENGTH, "%s %s %s %s %s %s %s",
				szSysStr[0], szSysStr[1], szSysStr[2], szSysStr[3],
				szSysStr[4], szSysStr[5], szSysStr[6]);
	}

	return nRet;
}


//
// Handler for System.Uptime parameter
//

LONG H_Uptime(const char *pszParam, const char *pArg, char *pValue)
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
		kp = kstat_lookup(kc, "unix", 0, "system_misc");
		if (kp != NULL)
		{
			if(kstat_read(kc, kp, 0) != -1)
			{
				kn = (kstat_named_t *)kstat_data_lookup(kp, "clk_intr");
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

//
// Handler for System.Hostname parameter
//

LONG H_Hostname(const char *pszParam, const char *pArg, char *pValue)
{
	return (sysinfo(SI_HOSTNAME, pValue, MAX_RESULT_LENGTH) == -1) ?
		SYSINFO_RC_ERROR : SYSINFO_RC_SUCCESS;
}


//
// Handler for System.CPU.LoadAvg
//

LONG H_LoadAvg(const char *pszParam, const char *pArg, char *pValue)
{
	kstat_ctl_t *kc;
	kstat_t *kp;
	kstat_named_t *kn;
	LONG nRet = SYSINFO_RC_ERROR;
	static char *szParam[] = { "avenrun_1min", "avenrun_5min", "avenrun_15min" };

	kstat_lock();
	kc = kstat_open();
	if (kc != NULL)
	{
		kp = kstat_lookup(kc, "unix", 0, "system_misc");
		if (kp != NULL)
		{
			if(kstat_read(kc, kp, 0) != -1)
			{
				kn = (kstat_named_t *)kstat_data_lookup(kp, szParam[(int)pArg]);
				if (kn != NULL)
				{
					sprintf(pValue, "%.2f0000", (double)kn->value.ul / 256.0);
					nRet = SYSINFO_RC_SUCCESS;
				}
			}
		}
		kstat_close(kc);
	}
	kstat_unlock();

	return nRet;
}


//
// Handler for System.KStat(*)
//

LONG H_KStat(const char *pszParam, const char *pArg, char *pValue)
{
	char *eptr, szModule[128], szName[128], szInstance[16], szStat[128];
	LONG nInstance;

	// Read parameters
	if ((!AgentGetParameterArg(pszParam, 1, szModule, 128)) ||
			(!AgentGetParameterArg(pszParam, 2, szInstance, 16)) ||
			(!AgentGetParameterArg(pszParam, 3, szName, 128)) ||
			(!AgentGetParameterArg(pszParam, 4, szStat, 128)))
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


//
// Handler for System.CPU.Count
//

LONG H_CPUCount(const char *pszParam, const char *pArg, char *pValue)
{
	return ReadKStatValue("unix", 0, "system_misc", "ncpus", pValue, NULL);
}


//
// Handler for generic kstat parameter
//

LONG ReadKStatValue(char *pszModule, LONG nInstance, char *pszName,
		char *pszStat, char *pValue, kstat_named_t *pRawValue)
{
	kstat_ctl_t *kc;
	kstat_t *kp;
	kstat_named_t *kn;
	LONG nRet = SYSINFO_RC_ERROR;

	kstat_lock();
	kc = kstat_open();
	if (kc != NULL)
	{
		kp = kstat_lookup(kc, pszModule, nInstance, pszName);
		if (kp != NULL)
		{
			if(kstat_read(kc, kp, 0) != -1)
			{
				kn = (kstat_named_t *)kstat_data_lookup(kp, pszStat);
				if (kn != NULL)
				{
					if (pValue != NULL)
					{
						switch(kn->data_type)
						{
							case KSTAT_DATA_CHAR:
								ret_string(pValue, kn->value.c);
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
					AgentWriteDebugLog(6, "SunOS::ReadKStatValue(%s,%d,%s,%s): kstat_data_lookup failed (%s)", pszModule, nInstance, pszName, pszStat, strerror(errno));
				}
			}
			else
			{
				AgentWriteDebugLog(6, "SunOS::ReadKStatValue(%s,%d,%s,%s): kstat_read failed (%s)", pszModule, nInstance, pszName, pszStat, strerror(errno));
			}
		}
		else
		{
			AgentWriteDebugLog(6, "SunOS::ReadKStatValue(%s,%d,%s,%s): kstat_lookup failed (%s)", pszModule, nInstance, pszName, pszStat, strerror(errno));
		}
		kstat_close(kc);
	}
	kstat_unlock();

	return nRet;
}


//
// Handler for System.CPU.Count
//

LONG H_MemoryInfo(const char *pszParam, const char *pArg, char *pValue)
{
	LONG nRet = SYSINFO_RC_SUCCESS;
	kstat_named_t kn;
	QWORD qwPageSize;

	qwPageSize = sysconf(_SC_PAGESIZE);

	switch((int)pArg)
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
		default:
			nRet = SYSINFO_RC_UNSUPPORTED;
			break;
	}

	return nRet;
}
