/*
** NetXMS PING subagent
** Copyright (C) 2004, 2005, 2006, 2007 Victor Kirhenshtein
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
** File: ping.cpp
**
**/

#include "ping.h"


//
// Static data
//

static CONDITION m_hCondShutdown = INVALID_CONDITION_HANDLE;
#ifdef _NETWARE
static CONDITION m_hCondTerminate = INVALID_CONDITION_HANDLE;
#endif
static BOOL m_bShutdown = FALSE;
static DWORD m_dwNumTargets = 0;
static PING_TARGET *m_pTargetList = NULL;
static DWORD m_dwTimeout = 3000;    // Default timeout is 3 seconds
static DWORD m_dwDefPacketSize = 46;
static DWORD m_dwPollsPerMinute = 4;


//
// Poller thread
//

static THREAD_RESULT THREAD_CALL PollerThread(void *pArg)
{
	QWORD qwStartTime;
	DWORD i, dwSum, dwCount, dwInterval, dwElapsedTime;
	BOOL bUnreachable;

	while(!m_bShutdown)
	{
		bUnreachable = FALSE;
		qwStartTime = GetCurrentTimeMs();
		if (IcmpPing(((PING_TARGET *)pArg)->dwIpAddr, 1, m_dwTimeout,
					&((PING_TARGET *)pArg)->dwLastRTT, 
					((PING_TARGET *)pArg)->dwPacketSize) != ICMP_SUCCESS)
		{
			((PING_TARGET *)pArg)->dwLastRTT = 10000;
			bUnreachable = TRUE;
		}

		((PING_TARGET *)pArg)->pdwHistory[((PING_TARGET *)pArg)->iBufPos++] = ((PING_TARGET *)pArg)->dwLastRTT;
		if (((PING_TARGET *)pArg)->iBufPos == (int)m_dwPollsPerMinute)
			((PING_TARGET *)pArg)->iBufPos = 0;
		if (bUnreachable)
		{
			((PING_TARGET *)pArg)->dwAvgRTT = 10000;
		}
		else
		{
			for(i = 0, dwSum = 0, dwCount = 0; i < m_dwPollsPerMinute; i++)
			{
				if (((PING_TARGET *)pArg)->pdwHistory[i] < 10000)
				{
					dwSum += ((PING_TARGET *)pArg)->pdwHistory[i];
					dwCount++;
				}
			}
			((PING_TARGET *)pArg)->dwAvgRTT = dwSum / dwCount;
		}

		dwElapsedTime = (DWORD)(GetCurrentTimeMs() - qwStartTime);

		dwInterval = 60000 / m_dwPollsPerMinute;
		if (ConditionWait(m_hCondShutdown, (dwInterval > dwElapsedTime + 1000) ? dwInterval - dwElapsedTime : 1000))
			break;
	}
	return THREAD_OK;
}


//
// Hanlder for immediate ping request
//

static LONG H_IcmpPing(TCHAR *pszParam, TCHAR *pArg, TCHAR *pValue)
{
	TCHAR szHostName[256], szTimeOut[32], szPacketSize[32];
	DWORD dwAddr, dwTimeOut = m_dwTimeout, dwRTT, dwPacketSize = m_dwDefPacketSize;

	if (!NxGetParameterArg(pszParam, 1, szHostName, 256))
		return SYSINFO_RC_UNSUPPORTED;
	StrStrip(szHostName);
	if (!NxGetParameterArg(pszParam, 2, szTimeOut, 256))
		return SYSINFO_RC_UNSUPPORTED;
	StrStrip(szTimeOut);
	if (!NxGetParameterArg(pszParam, 3, szPacketSize, 256))
		return SYSINFO_RC_UNSUPPORTED;
	StrStrip(szPacketSize);

	dwAddr = _t_inet_addr(szHostName);
	if (szTimeOut[0] != 0)
	{
		dwTimeOut = _tcstoul(szTimeOut, NULL, 0);
		if (dwTimeOut < 500)
			dwTimeOut = 500;
		if (dwTimeOut > 5000)
			dwTimeOut = 5000;
	}
	if (szPacketSize[0] != 0)
	{
		dwPacketSize = _tcstoul(szPacketSize, NULL, 0);
	}

	if (IcmpPing(dwAddr, 1, dwTimeOut, &dwRTT, dwPacketSize) != ICMP_SUCCESS)
		dwRTT = 10000;
	ret_uint(pValue, dwRTT);
	return SYSINFO_RC_SUCCESS;
}


//
// Handler for poller information
//

static LONG H_PollResult(TCHAR *pszParam, TCHAR *pArg, TCHAR *pValue)
{
	TCHAR szTarget[MAX_DB_STRING];
	DWORD i, dwIpAddr;
	BOOL bUseName = FALSE;

	if (!NxGetParameterArg(pszParam, 1, szTarget, MAX_DB_STRING))
		return SYSINFO_RC_UNSUPPORTED;
	StrStrip(szTarget);

	dwIpAddr = _t_inet_addr(szTarget);
	if ((dwIpAddr == INADDR_ANY) || (dwIpAddr == INADDR_NONE))
		bUseName = TRUE;

	for(i = 0; i < m_dwNumTargets; i++)
	{
		if (bUseName)
		{
			if (!_tcsicmp(m_pTargetList[i].szName, szTarget))
				break;
		}
		else
		{
			if (m_pTargetList[i].dwIpAddr == dwIpAddr)
				break;
		}
	}

	if (i == m_dwNumTargets)
		return SYSINFO_RC_UNSUPPORTED;   // No such target

	if (*pArg == _T('A'))
		ret_uint(pValue, m_pTargetList[i].dwAvgRTT);
	else
		ret_uint(pValue, m_pTargetList[i].dwLastRTT);

	return SYSINFO_RC_SUCCESS;
}


//
// Handler for configured target list
//

static LONG H_TargetList(TCHAR *pszParam, TCHAR *pArg, NETXMS_VALUES_LIST *pValue)
{
	DWORD i;
	TCHAR szBuffer[MAX_DB_STRING + 64], szIpAddr[16];

	for(i = 0; i < m_dwNumTargets; i++)
	{
		_stprintf(szBuffer, _T("%s %u %u %u %s"), IpToStr(ntohl(m_pTargetList[i].dwIpAddr), szIpAddr),
				m_pTargetList[i].dwLastRTT, m_pTargetList[i].dwAvgRTT, 
				m_pTargetList[i].dwPacketSize, m_pTargetList[i].szName);
		NxAddResultString(pValue, szBuffer);
	}

	return SYSINFO_RC_SUCCESS;
}


//
// Called by master agent at unload
//

static void SubagentShutdown(void)
{
	DWORD i;

	m_bShutdown = TRUE;
	if (m_hCondShutdown != INVALID_CONDITION_HANDLE)
		ConditionSet(m_hCondShutdown);

	for(i = 0; i < m_dwNumTargets; i++)
		ThreadJoin(m_pTargetList[i].hThread);
	safe_free(m_pTargetList);

#ifdef _NETWARE
	// Notify main thread that NLM can exit
	ConditionSet(m_hCondTerminate);
#endif
}


//
// Add target from configuration file parameter
// Parameter value should be <ip_address>:<name>
// Name part is optional and can be missing
//

static BOOL AddTargetFromConfig(TCHAR *pszCfg)
{
	TCHAR *ptr, *pszLine, *pszName = NULL;
	DWORD dwIpAddr, dwPacketSize = m_dwDefPacketSize;
	BOOL bResult = FALSE;

	pszLine = _tcsdup(pszCfg);
	ptr = _tcschr(pszLine, _T(':'));
	if (ptr != NULL)
	{
		*ptr = 0;
		ptr++;
		StrStrip(ptr);
		pszName = ptr;

		// Packet size
		ptr = _tcschr(pszName, _T(':'));
		if (ptr != NULL)
		{
			*ptr = 0;
			ptr++;
			StrStrip(ptr);
			StrStrip(pszName);
			dwPacketSize = _tcstoul(ptr, NULL, 0);
		}
	}
	StrStrip(pszLine);

	dwIpAddr = _t_inet_addr(pszLine);
	if ((dwIpAddr != INADDR_ANY) && (dwIpAddr != INADDR_NONE))
	{
		m_pTargetList = (PING_TARGET *)realloc(m_pTargetList, sizeof(PING_TARGET) * (m_dwNumTargets + 1));
		memset(&m_pTargetList[m_dwNumTargets], 0, sizeof(PING_TARGET));
		m_pTargetList[m_dwNumTargets].dwIpAddr = dwIpAddr;
		if (pszName != NULL)
			nx_strncpy(m_pTargetList[m_dwNumTargets].szName, pszName, MAX_DB_STRING);
		else
			IpToStr(ntohl(dwIpAddr), m_pTargetList[m_dwNumTargets].szName);
		m_pTargetList[m_dwNumTargets].dwPacketSize = dwPacketSize;
		m_dwNumTargets++;
		bResult = TRUE;
	}

	free(pszLine);
	return bResult;
}


//
// Configuration file template
//

static TCHAR *m_pszTargetList = NULL;
static NX_CFG_TEMPLATE cfgTemplate[] =
{
	{ _T("DefaultPacketSize"), CT_LONG, 0, 0, 0, 0, &m_dwDefPacketSize },
	{ _T("PacketRate"), CT_LONG, 0, 0, 0, 0, &m_dwPollsPerMinute },
	{ _T("Target"), CT_STRING_LIST, _T('\n'), 0, 0, 0, &m_pszTargetList },
	{ _T("Timeout"), CT_LONG, 0, 0, 0, 0, &m_dwTimeout },
	{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
};


//
// Subagent initialization
//

static BOOL SubagentInit(TCHAR *pszConfigFile)
{
	DWORD i, dwResult;

	// Load configuration
	dwResult = NxLoadConfig(pszConfigFile, _T("Ping"), cfgTemplate, FALSE);
	if (dwResult == NXCFG_ERR_OK)
	{
		TCHAR *pItem, *pEnd;

		if (m_dwPollsPerMinute == 0)
			m_dwPollsPerMinute = 1;
		if (m_dwPollsPerMinute > MAX_POLLS_PER_MINUTE)
			m_dwPollsPerMinute = MAX_POLLS_PER_MINUTE;

		// Parse target list
		if (m_pszTargetList != NULL)
		{
			for(pItem = m_pszTargetList; *pItem != 0; pItem = pEnd + 1)
			{
				pEnd = _tcschr(pItem, _T('\n'));
				if (pEnd != NULL)
					*pEnd = 0;
				StrStrip(pItem);
				if (!AddTargetFromConfig(pItem))
					NxWriteAgentLog(EVENTLOG_WARNING_TYPE,
							_T("Unable to add ICMP ping target from configuration file. ")
							_T("Original configuration record: %s"), pItem);
			}
			free(m_pszTargetList);
		}

		// Create shutdown condition and start poller threads
		m_hCondShutdown = ConditionCreate(TRUE);
		for(i = 0; i < m_dwNumTargets; i++)
			m_pTargetList[i].hThread = ThreadCreateEx(PollerThread, 0, &m_pTargetList[i]);
	}
	else
	{
		safe_free(m_pszTargetList);
	}

	return dwResult == NXCFG_ERR_OK;
}


//
// Subagent information
//

static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
	{ _T("Icmp.AvgPingTime(*)"), H_PollResult, _T("A"), DCI_DT_UINT, _T("Average response time of ICMP ping to {instance} for last minute") },
	{ _T("Icmp.LastPingTime(*)"), H_PollResult, _T("L"), DCI_DT_UINT, _T("Response time of last ICMP ping to {instance}") },
	{ _T("Icmp.Ping(*)"), H_IcmpPing, NULL, DCI_DT_UINT, _T("ICMP ping response time for {instance}") }
};
static NETXMS_SUBAGENT_ENUM m_enums[] =
{
	{ _T("Icmp.TargetList"), H_TargetList, NULL }
};

static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("PING"), _T(NETXMS_VERSION_STRING),
	SubagentInit, SubagentShutdown, NULL,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	sizeof(m_enums) / sizeof(NETXMS_SUBAGENT_ENUM),
	m_enums,
	0, NULL
};


//
// Entry point for NetXMS agent
//

DECLARE_SUBAGENT_ENTRY_POINT(PING)
{
	*ppInfo = &m_info;
	return TRUE;
}


//
// DLL entry point
//

#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif


//
// NetWare entry point
// We use main() instead of _init() and _fini() to implement
// automatic unload of the subagent after unload handler is called
//

#ifdef _NETWARE

int main(int argc, char *argv[])
{
   m_hCondTerminate = ConditionCreate(TRUE);
   ConditionWait(m_hCondTerminate, INFINITE);
   ConditionDestroy(m_hCondTerminate);
   sleep(1);
   return 0;
}

#endif
