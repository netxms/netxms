/*
** NetXMS PING subagent
** Copyright (C) 2004-2018 Victor Kirhenshtein
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
static ThreadPool *s_pollers = NULL;
static ObjectArray<PING_TARGET> s_targets(16, 16, true);
static Mutex s_targetLock;
static UINT32 m_timeout = 3000;    // Default timeout is 3 seconds
static UINT32 m_defaultPacketSize = 46;
static UINT32 m_pollsPerMinute = 4;
static UINT32 s_maxTargetInactivityTime = 86400;
static UINT32 s_options = PING_OPT_ALLOW_AUTOCONFIGURE;

/**
 * Poller
 */
static void Poller(void *arg)
{
   PING_TARGET *target = (PING_TARGET *)arg;

	bool unreachable = false;
	INT64 startTime = GetCurrentTimeMs();

   if (target->automatic && (startTime / 1000 - target->lastDataRead > s_maxTargetInactivityTime))
   {
      nxlog_debug_tag(DEBUG_TAG, 3, _T("Target %s (%s) removed because of inactivity"), target->name, (const TCHAR *)target->ipAddr.toString());
      s_targetLock.lock();
      s_targets.remove(target);
      s_targetLock.unlock();
      return;
   }

retry:
   if (IcmpPing(target->ipAddr, 1, m_timeout, &target->lastRTT, target->packetSize, target->dontFragment) != ICMP_SUCCESS)
   {
      InetAddress ip = InetAddress::resolveHostName(target->dnsName);
      if (!ip.equals(target->ipAddr))
      {
         TCHAR ip1[64], ip2[64];
         nxlog_debug_tag(DEBUG_TAG, 6, _T("IP address for target %s changed from %s to %s"), target->name,
                         target->ipAddr.toString(ip1), ip.toString(ip2));
         target->ipAddr = ip;
         goto retry;
      }
      target->lastRTT = 10000;
      unreachable = TRUE;
   }

   target->history[target->bufPos++] = target->lastRTT;
   if (target->bufPos == (int)m_pollsPerMinute)
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
            nxlog_debug_tag(DEBUG_TAG, 6, _T("IP address for target %s changed from %s to %s"), target->name,
                            target->ipAddr.toString(ip1), ip.toString(ip2));
            target->ipAddr = ip;
         }
         target->ipAddrAge = 0;
      }
   }

   UINT32 sum = 0, count = 0, lost = 0, stdDev = 0;
   for(UINT32 i = 0; i < m_pollsPerMinute; i++)
   {
      if (target->history[i] < 10000)
      {
         sum += target->history[i];
         if (target->history[i] > 0)
         {
            stdDev += (target->avgRTT - target->history[i]) * (target->avgRTT - target->history[i]);
         }
         count++;
      }
      else
      {
         lost++;
      }
   }
   target->avgRTT = unreachable ? 10000 : (sum / count);
   target->packetLoss = lost * 100 / m_pollsPerMinute;

   if (count > 0)
   {
      target->stdDevRTT = (UINT32)sqrt((double)stdDev / (double)count);
   }
   else
   {
      target->stdDevRTT = 0;
   }

   UINT32 elapsedTime = static_cast<UINT32>(GetCurrentTimeMs() - startTime);
   UINT32 interval = 60000 / m_pollsPerMinute;

   ThreadPoolScheduleRelative(s_pollers, (interval > elapsedTime) ? interval - elapsedTime : 1, Poller, arg);
}

/**
 * Hanlder for immediate ping request
 */
static LONG H_IcmpPing(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
	TCHAR szHostName[256], szTimeOut[32], szPacketSize[32], dontFragmentFlag[32];
	UINT32 dwTimeOut = m_timeout, dwRTT, dwPacketSize = m_defaultPacketSize;
	bool dontFragment = ((s_options & PING_OPT_DONT_FRAGMENT) != 0);

	if (!AgentGetParameterArg(pszParam, 1, szHostName, 256))
		return SYSINFO_RC_UNSUPPORTED;
	Trim(szHostName);
	if (!AgentGetParameterArg(pszParam, 2, szTimeOut, 32))
		return SYSINFO_RC_UNSUPPORTED;
	Trim(szTimeOut);
	if (!AgentGetParameterArg(pszParam, 3, szPacketSize, 32))
		return SYSINFO_RC_UNSUPPORTED;
	Trim(szPacketSize);
   if (!AgentGetParameterArg(pszParam, 4, dontFragmentFlag, 32))
      return SYSINFO_RC_UNSUPPORTED;
   Trim(dontFragmentFlag);

   InetAddress addr = InetAddress::resolveHostName(szHostName);
	if (szTimeOut[0] != 0)
	{
		dwTimeOut = _tcstoul(szTimeOut, NULL, 0);
		if (dwTimeOut < 100)
			dwTimeOut = 100;
		if (dwTimeOut > 5000)
			dwTimeOut = 5000;
	}
	if (szPacketSize[0] != 0)
	{
		dwPacketSize = _tcstoul(szPacketSize, NULL, 0);
	}
	if (dontFragmentFlag[0] != 0)
	{
	   dontFragment = (_tcstol(dontFragmentFlag, NULL, 0) != 0);
	}

	nxlog_debug_tag(DEBUG_TAG, 7, _T("IcmpPing started for host %s"), szHostName);
	UINT32 result = IcmpPing(addr, 1, dwTimeOut, &dwRTT, dwPacketSize, dontFragment);
	nxlog_debug_tag(DEBUG_TAG, 7, _T("IcmpPing: hostName=%s timeout=%d packetSize=%d dontFragment=%s result=%d time=%d"),
	      szHostName, dwTimeOut, dwPacketSize, dontFragment ? _T("true") : _T("false"), result, dwRTT);

	if (result == ICMP_TIMEOUT || result == ICMP_UNREACHEABLE)
		dwRTT = 10000;

	if(result == ICMP_TIMEOUT || result == ICMP_UNREACHEABLE || result == ICMP_SUCCESS)
	{
	   ret_uint(pValue, dwRTT);
	   return SYSINFO_RC_SUCCESS;
	}
	else
	   return SYSINFO_RC_ERROR;
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
   s_targetLock.lock();
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
   {
      if (s_options & PING_OPT_ALLOW_AUTOCONFIGURE)
      {
         InetAddress addr = bUseName ? InetAddress::resolveHostName(szTarget) : ipAddr;
         if (!addr.isValid())
         {
            s_targetLock.unlock();
            return SYSINFO_RC_UNSUPPORTED;   // Invalid hostname
         }

         t = new PING_TARGET;
         memset(t, 0, sizeof(PING_TARGET));
         t->ipAddr = addr;
         nx_strncpy(t->dnsName, szTarget, MAX_DB_STRING);
         nx_strncpy(t->name, szTarget, MAX_DB_STRING);
         t->packetSize = m_defaultPacketSize;
         t->dontFragment = ((s_options & PING_OPT_DONT_FRAGMENT) != 0);
         t->automatic = true;
         s_targets.add(t);

         nxlog_debug_tag(DEBUG_TAG, 3, _T("New ping target %s (%s) created from request"), t->name, (const TCHAR *)t->ipAddr.toString());

         ThreadPoolExecute(s_pollers, Poller, t);
      }
      else
      {
         s_targetLock.unlock();
         return SYSINFO_RC_UNSUPPORTED;   // No such target
      }
   }
   s_targetLock.unlock();

   t->lastDataRead = time(NULL);
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
	TCHAR szBuffer[MAX_DB_STRING + 128];

   s_targetLock.lock();
	for(int i = 0; i < s_targets.size(); i++)
	{
      PING_TARGET *t = s_targets.get(i);
      _sntprintf(szBuffer, MAX_DB_STRING + 128, _T("%s"), t->name);
		value->add(szBuffer);
	}
   s_targetLock.unlock();

	return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for configured target table
 */
static LONG H_TargetTable(const TCHAR *pszParam, const TCHAR *pArg, Table *value, AbstractCommSession *session)
{
    value->addColumn(_T("IP"), DCI_DT_STRING, _T("IP"), true);
    value->addColumn(_T("LAST_RTT"), DCI_DT_UINT, _T("Last response time"));
    value->addColumn(_T("AVERAGE_RTT"), DCI_DT_UINT, _T("Average response time"));
    value->addColumn(_T("PACKET_LOSS"), DCI_DT_UINT, _T("Packet loss"));
    value->addColumn(_T("PACKET_SIZE"), DCI_DT_UINT, _T("Packet size"));
    value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"));
    value->addColumn(_T("DNS_NAME"), DCI_DT_STRING, _T("DNS name"));
    value->addColumn(_T("IS_AUTO"), DCI_DT_INT, _T("Automatic"));

    s_targetLock.lock();
    for(int i = 0; i < s_targets.size(); i++)
    {
        value->addRow();
        PING_TARGET *t = s_targets.get(i);
        value->set(0, t->ipAddr.toString());
        value->set(1, t->lastRTT);
        value->set(2, t->avgRTT);
        value->set(3, t->packetLoss);
        value->set(4, t->packetSize);
        value->set(5, t->name);
        value->set(6, t->dnsName);
        value->set(7, t->automatic);
    }
    s_targetLock.unlock();
    return SYSINFO_RC_SUCCESS;
}

/**
 * Called by master agent at unload
 */
static void SubagentShutdown()
{
   ThreadPoolDestroy(s_pollers);
   nxlog_debug_tag(DEBUG_TAG, 2, _T("Poller thread pool destroyed"));
}

/**
 * Add target from configuration file parameter
 * Parameter value should be <ip_address>:<name>:<packet_size>
 * Name and size parts are optional and can be missing
 */
static BOOL AddTargetFromConfig(TCHAR *pszCfg)
{
	TCHAR *ptr, *pszLine, *pszName = NULL;
	UINT32 dwPacketSize = m_defaultPacketSize;
	bool dontFragment = ((s_options & PING_OPT_DONT_FRAGMENT) != 0);
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

			// Options
	      TCHAR *options = _tcschr(ptr, _T(':'));
	      if (options != NULL)
	      {
	         *options = 0;
	         options++;
	         StrStrip(ptr);
	         StrStrip(options);
	         dontFragment = (_tcsicmp(options, _T("DF")) != 0);
	      }

	      if (*ptr != 0)
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
		t->dontFragment = dontFragment;
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
   { _T("AutoConfigureTargets"), CT_BOOLEAN, 0, 0, PING_OPT_ALLOW_AUTOCONFIGURE, 0, &s_options },
	{ _T("DefaultPacketSize"), CT_LONG, 0, 0, 0, 0, &m_defaultPacketSize },
   { _T("DefaultDoNotFragmentFlag"), CT_BOOLEAN, 0, 0, PING_OPT_DONT_FRAGMENT, 0, &s_options },
   { _T("MaxTargetInactivityTime"), CT_LONG, 0, 0, 0, 0, &s_maxTargetInactivityTime },
	{ _T("PacketRate"), CT_LONG, 0, 0, 0, 0, &m_pollsPerMinute },
	{ _T("Target"), CT_STRING_LIST, _T('\n'), 0, 0, 0, &m_pszTargetList },
	{ _T("Timeout"), CT_LONG, 0, 0, 0, 0, &m_timeout },
	{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
};

/**
 * Subagent initialization
 */
static bool SubagentInit(Config *config)
{
	// Parse configuration
	bool success = config->parseTemplate(_T("Ping"), m_cfgTemplate);
	if (!success)
	{
	   free(m_pszTargetList);
	   return false;
	}

	s_pollers = ThreadPoolCreate(_T("PING"), 1, 1024);

   if (m_pollsPerMinute == 0)
      m_pollsPerMinute = 1;
   else if (m_pollsPerMinute > MAX_POLLS_PER_MINUTE)
      m_pollsPerMinute = MAX_POLLS_PER_MINUTE;
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Packet rate set to %d packets per minute (%d ms between packets)"), m_pollsPerMinute, 60000 / m_pollsPerMinute);

   // Parse target list
   if (m_pszTargetList != NULL)
   {
      TCHAR *pItem = m_pszTargetList;
      TCHAR *pEnd = _tcschr(pItem, _T('\n'));
      while(pEnd != NULL)
      {
         *pEnd = 0;
         StrStrip(pItem);
         if (!AddTargetFromConfig(pItem))
            AgentWriteLog(NXLOG_WARNING,
                  _T("Unable to add ICMP ping target from configuration file. ")
                  _T("Original configuration record: %s"), pItem);
         pItem = pEnd +1;
         pEnd = _tcschr(pItem, _T('\n'));
      }
      free(m_pszTargetList);
   }

   // First poll
   for(int i = 0; i < s_targets.size(); i++)
   {
      PING_TARGET *t = s_targets.get(i);
      ThreadPoolExecute(s_pollers, Poller, t);
   }

	return true;
}

/**
 * Parameters
 */
static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
	{ _T("Icmp.AvgPingTime(*)"), H_PollResult, _T("A"), DCI_DT_UINT, _T("Average response time of ICMP ping to {instance} for last minute") },
	{ _T("Icmp.LastPingTime(*)"), H_PollResult, _T("L"), DCI_DT_UINT, _T("Response time of last ICMP ping to {instance}") },
	{ _T("Icmp.PacketLoss(*)"), H_PollResult, _T("P"), DCI_DT_UINT, _T("Packet loss for ICMP ping to {instance}") },
 	{ _T("Icmp.PingStdDev(*)"), H_PollResult, _T("D"), DCI_DT_UINT, _T("Standard deviation for ICMP ping to {instance}") },
	{ _T("Icmp.Ping(*)"), H_IcmpPing, NULL, DCI_DT_UINT, _T("ICMP ping response time for {instance}") }
};

/**
 * Lists
 */
static NETXMS_SUBAGENT_LIST m_lists[] =
{
	{ _T("Icmp.Targets"), H_TargetList, NULL }
};

/**
 * Tables
 */
static NETXMS_SUBAGENT_TABLE m_table[] =
{
    { _T("Icmp.Targets"), H_TargetTable, NULL, _T("IP"), _T("ICMP ping targets") }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("PING"), NETXMS_BUILD_TAG,
	SubagentInit, SubagentShutdown, NULL, NULL,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	sizeof(m_lists) / sizeof(NETXMS_SUBAGENT_LIST),
	m_lists,
	sizeof(m_table) / sizeof(NETXMS_SUBAGENT_TABLE),
	m_table,	// tables
   0, NULL,	// actions
	0, NULL	// push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(PING)
{
	*ppInfo = &m_info;
	return true;
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
