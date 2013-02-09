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
**/

#include "linux_subagent.h"

/**
 * Handler for System.ConnectedUsers parameter
 */
LONG H_ConnectedUsers(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
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
LONG H_ActiveUserSessions(const TCHAR *pszParam, const TCHAR *pArg, StringList *pValue)
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
LONG H_Uptime(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
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

/**
 * Handler for System.Uname parameter
 */
LONG H_Uname(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
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
 * Handler for System.Hostname parameter
 */
LONG H_Hostname(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
	char szBuff[128];

	if (gethostname(szBuff, sizeof(szBuff)) == 0)
	{
		ret_mbstring(pValue, szBuff);
		nRet = SYSINFO_RC_SUCCESS;
	}

	return nRet;
}

/**
 * Handler for System.CPU.LoadAvg parameters
 */
LONG H_CpuLoad(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
	struct statvfs s;
	char szArg[128] = {0};
	FILE *hFile;

	// get processor
	//AgentGetParameterArg(pszParam, 1, szArg, sizeof(szArg));

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
			setlocale(LC_NUMERIC, "");
		}

		fclose(hFile);
	}

	return nRet;
}

/**
 * Handler for System.Memory.* parameters
 */
LONG H_MemoryInfo(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
	FILE *hFile;
	unsigned long nMemTotal, nMemFree, nSwapTotal, nSwapFree, nBuffers;
	char *pTag;

	nMemTotal = nMemFree = nSwapTotal = nSwapFree = nBuffers = 0;

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
			if (sscanf(szTmp, "Buffers: %lu kB\n", &nBuffers) == 1)
				continue;
		}

		fclose(hFile);

		nRet = SYSINFO_RC_SUCCESS;
		switch((long)pArg)
		{
			case PHYSICAL_FREE: // ph-free
				ret_uint64(pValue, ((QWORD)nMemFree) * 1024);
				break;
			case PHYSICAL_FREE_PCT: // ph-free percentage
				ret_uint(pValue, (DWORD)((QWORD)nMemFree * 100 / (QWORD)nMemTotal));
				break;
			case PHYSICAL_TOTAL: // ph-total
				ret_uint64(pValue, ((QWORD)nMemTotal) * 1024);
				break;
			case PHYSICAL_USED: // ph-used
				ret_uint64(pValue, ((QWORD)(nMemTotal - nMemFree)) * 1024);
				break;
			case PHYSICAL_USED_PCT: // ph-used percentage
				ret_uint(pValue, (DWORD)((QWORD)(nMemTotal - nMemFree) * 100 / (QWORD)nMemTotal));
				break;
			case PHYSICAL_AVAILABLE:
				ret_uint64(pValue, ((QWORD)nMemFree + (QWORD)nBuffers) * 1024);
				break;
			case PHYSICAL_AVAILABLE_PCT:
				ret_uint(pValue, (DWORD)(((QWORD)nMemFree + (QWORD)nBuffers) * 100 / (QWORD)nMemTotal));
				break;
			case SWAP_FREE: // sw-free
				ret_uint64(pValue, ((QWORD)nSwapFree) * 1024);
				break;
			case SWAP_FREE_PCT: // sw-free percentage
				if (nSwapTotal > 0)
					ret_uint(pValue, (DWORD)((QWORD)nSwapFree * 100 / (QWORD)nSwapTotal));
				else
					ret_uint(pValue, 100);
				break;
			case SWAP_TOTAL: // sw-total
				ret_uint64(pValue, ((QWORD)nSwapTotal) * 1024);
				break;
			case SWAP_USED: // sw-used
				ret_uint64(pValue, ((QWORD)(nSwapTotal - nSwapFree)) * 1024);
				break;
			case SWAP_USED_PCT: // sw-used percentage
				if (nSwapTotal > 0)
					ret_uint(pValue, (DWORD)((QWORD)(nSwapTotal - nSwapFree) * 100 / (QWORD)nSwapTotal));
				else
					ret_uint(pValue, 0);
				break;
			case VIRTUAL_FREE: // vi-free
				ret_uint64(pValue, ((QWORD)nMemFree + (QWORD)nSwapFree) * 1024);
				break;
			case VIRTUAL_FREE_PCT: // vi-free percentage
				ret_uint(pValue, (DWORD)(((QWORD)nMemFree + (QWORD)nSwapFree) * 100 / ((QWORD)nMemTotal + (QWORD)nSwapTotal)));
				break;
			case VIRTUAL_TOTAL: // vi-total
				ret_uint64(pValue, ((QWORD)nMemTotal + (QWORD)nSwapTotal) * 1024);
				break;
			case VIRTUAL_USED: // vi-used
				ret_uint64(pValue, ((QWORD)(nMemTotal - nMemFree) + (QWORD)(nSwapTotal - nSwapFree)) * 1024);
				break;
			case VIRTUAL_USED_PCT: // vi-used percentage
				ret_uint(pValue, (DWORD)(((QWORD)(nMemTotal - nMemFree) + (QWORD)(nSwapTotal - nSwapFree)) * 100 / ((QWORD)nMemTotal + (QWORD)nSwapTotal)));
				break;
			case VIRTUAL_AVAILABLE:
				ret_uint64(pValue, ((QWORD)nMemFree + (QWORD)nSwapFree + (QWORD)nBuffers) * 1024);
				break;
			case VIRTUAL_AVAILABLE_PCT:
				ret_uint(pValue, (DWORD)(((QWORD)nMemFree + (QWORD)nSwapFree + (QWORD)nBuffers) * 100 / ((QWORD)nMemTotal + (QWORD)nSwapTotal)));
				break;
			default: // error
				nRet = SYSINFO_RC_ERROR;
				break;
		}
	}

	return nRet;
}

/**
 * Handler for System.ProcessList list
 */
LONG H_ProcessList(const TCHAR *pszParam, const TCHAR *pArg, StringList *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
	PROC_ENT *pEnt;
	int nCount;

	nCount = ProcRead(&pEnt, NULL, NULL);

	if (nCount >= 0)
	{
		nRet = SYSINFO_RC_SUCCESS;

		for (int i = 0; i < nCount; i++)
		{
			TCHAR szBuff[128];

			_sntprintf(szBuff, sizeof(szBuff), _T("%d %hs"),
					pEnt[i].nPid, pEnt[i].szProcName);
			pValue->add(szBuff);
		}
	}

	return nRet;
}

/**
 * Handler for Agent.SourcePackageSupport parameter
 */
LONG H_SourcePkgSupport(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
{
	ret_int(pValue, 1);
	return SYSINFO_RC_SUCCESS;
}
