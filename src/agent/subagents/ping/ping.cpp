/*
** NetXMS PING subagent
** Copyright (C) 2004-2015 Victor Kirhenshtein
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

/**
 * Static data
 */
static CONDITION m_hCondShutdown = INVALID_CONDITION_HANDLE;
#ifdef _NETWARE
static CONDITION m_hCondTerminate = INVALID_CONDITION_HANDLE;
#endif
static BOOL m_bShutdown = FALSE;
static ObjectArray<PING_TARGET> s_targets(16, 16, true);
static UINT32 m_dwTimeout = 3000;    // Default timeout is 3 seconds
static UINT32 m_dwDefPacketSize = 46;
static UINT32 m_dwPollsPerMinute = 4;

/**
 * Poller thread
 */
static THREAD_RESULT THREAD_CALL PollerThread(void *arg)
{
	QWORD qwStartTime;
	UINT32 i, dwSum, dwLost, dwCount, dwInterval, dwElapsedTime, dwStdDev;
	BOOL bUnreachable;
	PING_TARGET *target = (PING_TARGET *)arg;
	while(!m_bShutdown)
	{
		bUnreachable = FALSE;
		qwStartTime = GetCurrentTimeMs();
retry:
		if (IcmpPing(target->ipAddr, 1, m_dwTimeout, &target->lastRTT, target->packetSize) != ICMP_SUCCESS)
		{
         InetAddress ip = InetAddress::resolveHostName(target->dnsName);
         if (!ip.equals(target->ipAddr))
			{
				TCHAR ip1[64], ip2[64];
				AgentWriteDebugLog(6, _T("PING: IP address for target %s changed from %s to %s"), target->name,
                               target->ipAddr.toString(ip1), ip.toString(ip2));
				target->ipAddr = ip;
				goto retry;
			}
			target->lastRTT = 10000;
			bUnreachable = TRUE;
		}

		target->history[target->bufPos++] = target->lastRTT;
		if (target->bufPos == (int)m_dwPollsPerMinute)
		{
			target->bufPos = 0;

			// recheck IP every 5 minutes
			target->ipAddrAge++;
			if (target->ipAddrAge >= 1)
			{
            InetAddress ip = InetAddress::resolveHostName(target->dnsName);
            if (!ip.equals(target->ipAddr))
				{
					TCHAR ip1[64], ip2[64];
					AgentWriteDebugLog(6, _T("PING: IP address for target %s changed from %s to %s"), target->name,
											 target->ipAddr.toString(ip1), ip.toString(ip2));
					target->ipAddr = ip;
				}
				target->ipAddrAge = 0;
			}
		}

		for(i = 0, dwSum = 0, dwLost = 0, dwCount = 0, dwStdDev = 0; i < m_dwPollsPerMinute; i++)
		{
			if (target->history[i] < 10000)
			{
				dwSum += target->history[i];
				if (target->history[i] > 0)
				{
					dwStdDev += (target->avgRTT - target->history[i]) * (target->avgRTT - target->history[i]);
				}
				dwCount++;
			}
			else
			{
				dwLost++;
			}
		}
		target->avgRTT = bUnreachable ? 10000 : (dwSum / dwCount);
		target->packetLoss = dwLost * 100 / m_dwPollsPerMinute;

		if (dwCount > 0)
		{
			target->stdDevRTT = (UINT32)sqrt((double)dwStdDev / (double)dwCount);
		}
		else
		{
			target->stdDevRTT = 0;
		}
		
		dwElapsedTime = (UINT32)(GetCurrentTimeMs() - qwStartTime);
		dwInterval = 60000 / m_dwPollsPerMinute;

		if (ConditionWait(m_hCondShutdown, (dwInterval > dwElapsedTime + 1000) ? dwInterval - dwElapsedTime : 1000))
			break;
	}
	return THREAD_OK;
}

/**
 * Hanlder for immediate ping request
 */
static LONG H_IcmpPing(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
	TCHAR szHostName[256], szTimeOut[32], szPacketSize[32];
	UINT32 dwTimeOut = m_dwTimeout, dwRTT, dwPacketSize = m_dwDefPacketSize;

	if (!AgentGetParameterArg(pszParam, 1, szHostName, 256))
		return SYSINFO_RC_UNSUPPORTED;
	StrStrip(szHostName);
	if (!AgentGetParameterArg(pszParam, 2, szTimeOut, 256))
		return SYSINFO_RC_UNSUPPORTED;
	StrStrip(szTimeOut);
	if (!AgentGetParameterArg(pszParam, 3, szPacketSize, 256))
		return SYSINFO_RC_UNSUPPORTED;
	StrStrip(szPacketSize);

   InetAddress addr = InetAddress::resolveHostName(szHostName);
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

	if (IcmpPing(addr, 1, dwTimeOut, &dwRTT, dwPacketSize) != ICMP_SUCCESS)
		dwRTT = 10000;
	ret_uint(pValue, dwRTT);
	return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for poller information
 */
static LONG H_PollResult(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
	TCHAR szTarget[MAX_DB_STRING];
	BOOL bUseName = FALSE;

	if (!AgentGetParameterArg(pszParam, 1, szTarget, MAX_DB_STRING))
		return SYSINFO_RC_UNSUPPORTED;
	StrStrip(szTarget);

   InetAddress ipAddr = InetAddress::parse(szTarget);
   if (!ipAddr.isValid())
		bUseName = TRUE;

   int i;
   PING_TARGET *t = NULL;
   for(i = 0; i < s_targets.size(); i++)
	{
      t = s_targets.get(i);
		if (bUseName)
		{
			if (!_tcsicmp(t->name, szTarget) || !_tcsicmp(t->dnsName, szTarget))
				break;
		}
		else
		{
			if (t->ipAddr.equals(ipAddr))
				break;
		}
	}

   if (i == s_targets.size())
		return SYSINFO_RC_UNSUPPORTED;   // No such target

	switch(*pArg)
	{
		case _T('A'):
			ret_uint(pValue, t->avgRTT);
			break;
		case _T('L'):
			ret_uint(pValue, t->lastRTT);
			break;
		case _T('P'):
			ret_uint(pValue, t->packetLoss);
			break;
		case _T('D'):
			ret_uint(pValue, t->stdDevRTT);
			break;
		default:
			return SYSINFO_RC_UNSUPPORTED;
	}

	return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for configured target list
 */
static LONG H_TargetList(const TCHAR *pszParam, const TCHAR *pArg, StringList *value, AbstractCommSession *session)
{
	TCHAR szBuffer[MAX_DB_STRING + 128], szIpAddr[64];

	for(int i = 0; i < s_targets.size(); i++)
	{
      PING_TARGET *t = s_targets.get(i);
      _sntprintf(szBuffer, MAX_DB_STRING + 128, _T("%s %u %u %u %u %s"), t->ipAddr.toString(szIpAddr),
				     t->lastRTT, t->avgRTT, t->packetLoss, t->packetSize, t->name);
		value->add(szBuffer);
	}

	return SYSINFO_RC_SUCCESS;
}

/**
 * Called by master agent at unload
 */
static void SubagentShutdown()
{
	m_bShutdown = TRUE;
	if (m_hCondShutdown != INVALID_CONDITION_HANDLE)
		ConditionSet(m_hCondShutdown);

   for(int i = 0; i < s_targets.size(); i++)
      ThreadJoin(s_targets.get(i)->hThread);

#ifdef _NETWARE
	// Notify main thread that NLM can exit
	ConditionSet(m_hCondTerminate);
#endif
}

/**
 * Add target from configuration file parameter
 * Parameter value should be <ip_address>:<name>:<packet_size>
 * Name and size parts are optional and can be missing
 */
static BOOL AddTargetFromConfig(TCHAR *pszCfg)
{
	TCHAR *ptr, *pszLine, *pszName = NULL;
	UINT32 dwPacketSize = m_dwDefPacketSize;
	BOOL bResult = FALSE;

	pszLine = _tcsdup(pszCfg);
   StrStrip(pszLine);
   TCHAR *addrStart = pszLine;
   TCHAR *scanStart = pszLine;

   if (pszLine[0] == _T('['))
   {
      addrStart++;
	   ptr = _tcschr(addrStart, _T(']'));
	   if (ptr != NULL)
	   {
         *ptr = 0;
         scanStart = ptr + 1;
      }
   }

	ptr = _tcschr(scanStart, _T(':'));
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
	StrStrip(addrStart);

   InetAddress addr = InetAddress::resolveHostName(addrStart);
   if (addr.isValid())
	{
      PING_TARGET *t = new PING_TARGET;
		memset(t, 0, sizeof(PING_TARGET));
		t->ipAddr = addr;
		nx_strncpy(t->dnsName, addrStart, MAX_DB_STRING);
		if (pszName != NULL)
			nx_strncpy(t->name, pszName, MAX_DB_STRING);
		else
         addr.toString(t->name);
		t->packetSize = dwPacketSize;
      s_targets.add(t);
		bResult = TRUE;
	}

	free(pszLine);
	return bResult;
}

/**
 * Configuration file template
 */
static TCHAR *m_pszTargetList = NULL;
static NX_CFG_TEMPLATE m_cfgTemplate[] =
{
	{ _T("DefaultPacketSize"), CT_LONG, 0, 0, 0, 0, &m_dwDefPacketSize },
	{ _T("PacketRate"), CT_LONG, 0, 0, 0, 0, &m_dwPollsPerMinute },
	{ _T("Target"), CT_STRING_LIST, _T('\n'), 0, 0, 0, &m_pszTargetList },
	{ _T("Timeout"), CT_LONG, 0, 0, 0, 0, &m_dwTimeout },
	{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
};

/**
 * Subagent initialization
 */
static BOOL SubagentInit(Config *config)
{
	bool success;

	// Parse configuration
	success = config->parseTemplate(_T("Ping"), m_cfgTemplate);
	if (success)
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
					AgentWriteLog(EVENTLOG_WARNING_TYPE,
							_T("Unable to add ICMP ping target from configuration file. ")
							_T("Original configuration record: %s"), pItem);
			}
			free(m_pszTargetList);
		}

		// Create shutdown condition and start poller threads
		m_hCondShutdown = ConditionCreate(TRUE);
      for(int i = 0; i < s_targets.size(); i++)
      {
         PING_TARGET *t = s_targets.get(i);
         t->hThread = ThreadCreateEx(PollerThread, 0, t);
      }
	}
	else
	{
		safe_free(m_pszTargetList);
	}

	return success;
}

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
	{ _T("Icmp.AvgPingTime(*)"), H_PollResult, _T("A"), DCI_DT_UINT, _T("Average response time of ICMP ping to {instance} for last minute") },
	{ _T("Icmp.LastPingTime(*)"), H_PollResult, _T("L"), DCI_DT_UINT, _T("Response time of last ICMP ping to {instance}") },
	{ _T("Icmp.PacketLoss(*)"), H_PollResult, _T("P"), DCI_DT_UINT, _T("Packet loss for ICMP ping to {instance}") },
 	{ _T("Icmp.PingStdDev(*)"), H_PollResult, _T("D"), DCI_DT_UINT, _T("Standard deviation for ICMP ping to {instance}") },
	{ _T("Icmp.Ping(*)"), H_IcmpPing, NULL, DCI_DT_UINT, _T("ICMP ping response time for {instance}") }
};
static NETXMS_SUBAGENT_LIST m_enums[] =
{
	{ _T("Icmp.TargetList"), H_TargetList, NULL }
};

static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("PING"), NETXMS_VERSION_STRING,
	SubagentInit, SubagentShutdown, NULL,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	sizeof(m_enums) / sizeof(NETXMS_SUBAGENT_LIST),
	m_enums,
	0, NULL,	// tables
   0, NULL,	// actions
	0, NULL	// push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(PING)
{
	*ppInfo = &m_info;
	return TRUE;
}

#ifdef _WIN32

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif

/**
 * NetWare entry point
 * We use main() instead of _init() and _fini() to implement
 * automatic unload of the subagent after unload handler is called
 */
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
