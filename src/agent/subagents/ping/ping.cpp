/*
** NetXMS PING subagent
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
** File: ping.cpp
**
**/

#include "ping.h"
#include <netxms-version.h>

/**
 * Static data
 */
static ThreadPool *s_pollers = nullptr;
static ObjectArray<PING_TARGET> s_targets(16, 16, Ownership::True);
static Mutex s_targetLock;
static uint32_t s_timeout = 3000;    // Default timeout is 3 seconds
static uint32_t s_defaultPacketSize = 46;
static uint32_t s_pollsPerMinute = 4;
static uint32_t s_maxTargetInactivityTime = 86400;
static uint32_t s_movingAverageTimePeriod = 3600;
static uint32_t s_options = PING_OPT_ALLOW_AUTOCONFIGURE;

/**
 * Poller
 */
static void Poller(PING_TARGET *target)
{
	bool unreachable = false;
	int64_t startTime = GetCurrentTimeMs();

   if (target->automatic && (startTime / 1000 - target->lastDataRead > s_maxTargetInactivityTime))
   {
      nxlog_debug_tag(DEBUG_TAG, 3, _T("Target %s (%s) removed because of inactivity"), target->name, (const TCHAR *)target->ipAddr.toString());
      s_targetLock.lock();
      s_targets.remove(target);
      s_targetLock.unlock();
      return;
   }

   // recheck IP every 5 minutes
   target->ipAddrAge++;
   if (target->ipAddrAge >= s_pollsPerMinute * 5)
   {
      InetAddress ip = InetAddress::resolveHostName(target->dnsName);
      if (!ip.equals(target->ipAddr))
      {
         TCHAR ip1[64], ip2[64];
         nxlog_debug_tag(DEBUG_TAG, 6, _T("IP address for target %s changed from %s to %s"), target->name, target->ipAddr.toString(ip1), ip.toString(ip2));
         target->ipAddr = ip;
      }
      target->ipAddrAge = 0;
   }

retry:
   if (IcmpPing(target->ipAddr, 1, s_timeout, &target->lastRTT, target->packetSize, target->dontFragment) != ICMP_SUCCESS)
   {
      InetAddress ip = InetAddress::resolveHostName(target->dnsName);
      if (!ip.equals(target->ipAddr))
      {
         TCHAR ip1[64], ip2[64];
         nxlog_debug_tag(DEBUG_TAG, 6, _T("IP address for target %s changed from %s to %s"), target->name, target->ipAddr.toString(ip1), ip.toString(ip2));
         target->ipAddr = ip;
         goto retry;
      }
      target->lastRTT = 10000;
      unreachable = true;
   }
   nxlog_debug_tag(DEBUG_TAG, 7, _T("Poller: completed for host=%s timeout=%d packetSize=%d dontFragment=%s unreachable=%s time=%d"),
      target->dnsName, s_timeout, target->packetSize, target->dontFragment ? _T("true") : _T("false"), unreachable ? _T("true") : _T("false"), target->lastRTT);

   target->rttHistory[target->bufPos] = target->lastRTT;

   uint32_t sum = 0, count = 0, lost = 0, localMin = 0x7FFFFFFF, localMax = 0;
   for(uint32_t i = 0; i < s_pollsPerMinute; i++)
   {
      if (target->rttHistory[i] < 10000)
      {
         sum += target->rttHistory[i];
         if (target->rttHistory[i] < localMin)
         {
            localMin = target->rttHistory[i];
         }
         if (target->rttHistory[i] > localMax)
         {
            localMax = target->rttHistory[i];
         }
         count++;
      }
      else if (target->rttHistory[i] == 10000)
      {
         lost++;
      }
   }
   target->averageRTT = unreachable ? 10000 : (sum / count);
   target->minRTT = localMin;
   target->maxRTT = localMax;
   target->packetLoss = lost * 100 / s_pollsPerMinute;

   if (target->lastRTT != 10000)
   {
      if (target->cumulativeMinRTT > target->lastRTT)
      {
         target->cumulativeMinRTT = target->lastRTT;
      }
      if (target->cumulativeMaxRTT < target->lastRTT)
      {
         target->cumulativeMaxRTT = target->lastRTT;
      }
   }

   if (count > 1)
   {
      uint32_t stdDev = 0;
      for(uint32_t i = 0; i < s_pollsPerMinute; i++)
      {
         if ((target->rttHistory[i] > 0) && (target->rttHistory[i] < 10000))
         {
            uint32_t delta = target->averageRTT - target->rttHistory[i];
            stdDev += delta * delta;
         }
      }
      target->stdDevRTT = static_cast<uint32_t>(sqrt((double)stdDev / (double)count));
   }
   else
   {
      target->stdDevRTT = 0;
   }

   if (target->lastRTT != 10000)
   {
      if (target->movingAverageRTT == 0xFFFFFFFF)
      {
         target->movingAverageRTT = target->lastRTT * EMA_FP_1;
      }
      else
      {
         UpdateExpMovingAverage(target->movingAverageRTT, target->movingAverageExp, target->lastRTT);
      }

      if (target->prevRTT != 0xFFFFFFFF)
      {
         uint32_t jitter = abs(static_cast<int32_t>(target->lastRTT) - static_cast<int32_t>(target->prevRTT));
         target->jitterHistory[target->bufPos] = jitter;
         sum = 0;
         for(uint32_t i = 0; i < s_pollsPerMinute; i++)
            sum += target->jitterHistory[i];
         target->averageJitter = sum / s_pollsPerMinute;

         if (target->movingAverageJitter == 0xFFFFFFFF)
         {
            target->movingAverageJitter = jitter * EMA_FP_1;
         }
         else
         {
            UpdateExpMovingAverage(target->movingAverageJitter, target->movingAverageExp, jitter);
         }
      }
      target->prevRTT = target->lastRTT;
   }
   else
   {
      // Keep jitter value when target is unreachable
      target->jitterHistory[target->bufPos] = target->averageJitter;
   }

   target->bufPos++;
   if (target->bufPos == (int)s_pollsPerMinute)
      target->bufPos = 0;

   uint32_t elapsedTime = static_cast<uint32_t>(GetCurrentTimeMs() - startTime);
   uint32_t interval = 60000 / s_pollsPerMinute;

   ThreadPoolScheduleRelative(s_pollers, (interval > elapsedTime) ? interval - elapsedTime : 1, Poller, target);
}

/**
 * Hanlder for immediate ping request
 */
static LONG H_IcmpPing(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
	TCHAR szHostName[256], szTimeOut[32], szPacketSize[32], dontFragmentFlag[32], retryCountText[32];
	uint32_t dwTimeOut = s_timeout, dwRTT, dwPacketSize = s_defaultPacketSize;
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
   if (!AgentGetParameterArg(pszParam, 5, retryCountText, 32))
      return SYSINFO_RC_UNSUPPORTED;
   Trim(retryCountText);

   InetAddress addr = InetAddress::resolveHostName(szHostName);
	if (szTimeOut[0] != 0)
	{
		dwTimeOut = _tcstoul(szTimeOut, nullptr, 0);
		if (dwTimeOut < 100)
			dwTimeOut = 100;
		if (dwTimeOut > 5000)
			dwTimeOut = 5000;
	}
	if (szPacketSize[0] != 0)
	{
		dwPacketSize = _tcstoul(szPacketSize, nullptr, 0);
	}
	if (dontFragmentFlag[0] != 0)
	{
	   dontFragment = (_tcstol(dontFragmentFlag, nullptr, 0) != 0);
	}

	int retryCount = (retryCountText[0] != 0) ? _tcstol(retryCountText, nullptr, 0) : 1;
	if (retryCount < 1)
	   retryCount = 1;

	TCHAR ipAddrText[64];
	nxlog_debug_tag(DEBUG_TAG, 7, _T("IcmpPing: start for host=%s addr=%s retryCount=%d"), szHostName, addr.toString(ipAddrText), retryCount);
	uint32_t result = IcmpPing(addr, retryCount, dwTimeOut, &dwRTT, dwPacketSize, dontFragment);
	nxlog_debug_tag(DEBUG_TAG, 7, _T("IcmpPing: completed for host=%s timeout=%d packetSize=%d dontFragment=%s result=%d time=%d"),
	      szHostName, dwTimeOut, dwPacketSize, dontFragment ? _T("true") : _T("false"), result, dwRTT);

   if (result == ICMP_SUCCESS)
   {
      ret_uint(pValue, dwRTT);
      return SYSINFO_RC_SUCCESS;
   }
   else if ((result == ICMP_TIMEOUT) || (result == ICMP_UNREACHABLE))
	{
      ret_uint(pValue, 10000);
      return SYSINFO_RC_SUCCESS;
	}
   return SYSINFO_RC_ERROR;
}

/**
 * Handler for poller information
 */
static LONG H_PollResult(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR target[MAX_DB_STRING];
	if (!AgentGetParameterArg(param, 1, target, MAX_DB_STRING))
		return SYSINFO_RC_UNSUPPORTED;
	Trim(target);

   InetAddress ipAddr = InetAddress::parse(target);
   bool useName = !ipAddr.isValid();

   int i;
   PING_TARGET *t = nullptr;
   s_targetLock.lock();
   for(i = 0; i < s_targets.size(); i++)
	{
      t = s_targets.get(i);
		if (useName)
		{
			if (!_tcsicmp(t->name, target) || !_tcsicmp(t->dnsName, target))
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
      s_targetLock.unlock();
      if (s_options & PING_OPT_ALLOW_AUTOCONFIGURE)
      {
         InetAddress addr = useName ? InetAddress::resolveHostName(target) : ipAddr;
         if (!addr.isValid())
         {
            return SYSINFO_RC_UNSUPPORTED;   // Invalid hostname
         }

         t = new PING_TARGET;
         memset(t, 0, sizeof(PING_TARGET));
         t->ipAddr = addr;
         _tcslcpy(t->dnsName, target, MAX_DB_STRING);
         _tcslcpy(t->name, target, MAX_DB_STRING);
         t->packetSize = s_defaultPacketSize;
         t->dontFragment = ((s_options & PING_OPT_DONT_FRAGMENT) != 0);
         t->prevRTT = 0xFFFFFFFF;
         t->cumulativeMinRTT = 0x7FFFFFFF;
         t->movingAverageRTT = 0xFFFFFFFF;
         t->movingAverageExp = EMA_EXP(60 / s_pollsPerMinute, s_movingAverageTimePeriod);
         t->movingAverageJitter = 0xFFFFFFFF;
         t->automatic = true;
         t->lastDataRead = time(nullptr);
         for(uint32_t i = 0; i < s_pollsPerMinute; i++)
            t->rttHistory[i] = 10001;  // Indicate unused slot

         s_targetLock.lock();
         s_targets.add(t);

         nxlog_debug_tag(DEBUG_TAG, 3, _T("New ping target %s (%s) created from request"), t->name, (const TCHAR *)t->ipAddr.toString());

         ThreadPoolExecute(s_pollers, Poller, t);
      }
      else
      {
         return SYSINFO_RC_UNSUPPORTED;   // No such target
      }
   }
   s_targetLock.unlock();

   t->lastDataRead = time(nullptr);
	switch(*arg)
	{
		case _T('A'):
			ret_uint(value, t->averageRTT);
			break;
      case _T('a'):
         if (t->movingAverageRTT == 0xFFFFFFFF)
            return SYSINFO_RC_ERROR;   // No data yet
         ret_uint(value, static_cast<uint32_t>(round(GetExpMovingAverageValue(t->movingAverageRTT))));
         break;
      case _T('c'):
         ret_uint(value, t->cumulativeMinRTT);
         break;
      case _T('C'):
         ret_uint(value, t->cumulativeMaxRTT);
         break;
      case _T('D'):
         ret_uint(value, t->stdDevRTT);
         break;
      case _T('J'):
         ret_uint(value, t->averageJitter);
         break;
      case _T('j'):
         if (t->movingAverageJitter == 0xFFFFFFFF)
            return SYSINFO_RC_ERROR;   // No data yet
         ret_uint(value, static_cast<uint32_t>(round(GetExpMovingAverageValue(t->movingAverageJitter))));
         break;
		case _T('L'):
			ret_uint(value, t->lastRTT);
			break;
      case _T('m'):
         ret_uint(value, t->minRTT);
         break;
      case _T('M'):
         ret_uint(value, t->maxRTT);
         break;
		case _T('P'):
			ret_uint(value, t->packetLoss);
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
    value->addColumn(_T("MIN_RTT"), DCI_DT_UINT, _T("Minimum response time"));
    value->addColumn(_T("MAX_RTT"), DCI_DT_UINT, _T("Maximum response time"));
    value->addColumn(_T("MOVING_AVERAGE_RTT"), DCI_DT_UINT, _T("Moving average of response time"));
    value->addColumn(_T("STD_DEV"), DCI_DT_UINT, _T("Standard deviation of response time"));
    value->addColumn(_T("JITTER"), DCI_DT_UINT, _T("Jitter"));
    value->addColumn(_T("MOVING_AVERAGE_JITTER"), DCI_DT_UINT, _T("Moving average of jitter"));
    value->addColumn(_T("CMIN_RTT"), DCI_DT_UINT, _T("Cumulative minimum response time"));
    value->addColumn(_T("CMAX_RTT"), DCI_DT_UINT, _T("Cumulative maximum response time"));
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
        value->set(2, t->averageRTT);
        value->set(3, t->minRTT);
        value->set(4, t->maxRTT);
        value->set(5, (t->movingAverageRTT == 0xFFFFFFFF) ? 0 : static_cast<uint32_t>(round(GetExpMovingAverageValue(t->movingAverageRTT))));
        value->set(6, t->stdDevRTT);
        value->set(7, t->averageJitter);
        value->set(8, (t->movingAverageJitter == 0xFFFFFFFF) ? 0 : static_cast<uint32_t>(round(GetExpMovingAverageValue(t->movingAverageJitter))));
        value->set(9, t->cumulativeMinRTT);
        value->set(10, t->cumulativeMaxRTT);
        value->set(11, t->packetLoss);
        value->set(12, t->packetSize);
        value->set(13, t->name);
        value->set(14, t->dnsName);
        value->set(15, t->automatic);
    }
    s_targetLock.unlock();
    return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for scanning address range
 * Arguments: start address, end address, timeout
 */
static LONG H_ScanRange(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   char startAddr[128], endAddr[128];
   TCHAR timeoutText[64];
   if (!AgentGetParameterArgA(param, 1, startAddr, 128) ||
       !AgentGetParameterArgA(param, 2, endAddr, 128) ||
       !AgentGetParameterArg(param, 3, timeoutText, 64))
   {
      return SYSINFO_RC_UNSUPPORTED;
   }

   InetAddress start = InetAddress::parse(startAddr);
   InetAddress end = InetAddress::parse(endAddr);
   uint32_t timeout = (timeoutText[0] != 0) ? _tcstoul(timeoutText, nullptr, 0) : 1000;
   if (!start.isValid() || !end.isValid() || (timeout == 0))
   {
      return SYSINFO_RC_UNSUPPORTED;
   }

   StructArray<InetAddress> *results = ScanAddressRange(start, end, timeout);
   if (results == nullptr)
      return SYSINFO_RC_ERROR;

   TCHAR buffer[128];
   for(int i = 0; i < results->size(); i++)
      value->add(results->get(i)->toString(buffer));
   delete results;
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
	TCHAR *ptr, *pszLine, *pszName = nullptr;
	uint32_t dwPacketSize = s_defaultPacketSize;
	bool dontFragment = ((s_options & PING_OPT_DONT_FRAGMENT) != 0);
	BOOL bResult = FALSE;

	pszLine = Trim(MemCopyString(pszCfg));
   TCHAR *addrStart = pszLine;
   TCHAR *scanStart = pszLine;

   if (pszLine[0] == _T('['))
   {
      addrStart++;
	   ptr = _tcschr(addrStart, _T(']'));
	   if (ptr != nullptr)
	   {
         *ptr = 0;
         scanStart = ptr + 1;
      }
   }

	ptr = _tcschr(scanStart, _T(':'));
	if (ptr != nullptr)
	{
		*ptr = 0;
		ptr++;
		Trim(ptr);
		pszName = ptr;

		// Packet size
		ptr = _tcschr(pszName, _T(':'));
		if (ptr != nullptr)
		{
			*ptr = 0;
			ptr++;
			Trim(ptr);
			Trim(pszName);

			// Options
	      TCHAR *options = _tcschr(ptr, _T(':'));
	      if (options != nullptr)
	      {
	         *options = 0;
	         options++;
	         Trim(ptr);
	         Trim(options);
	         dontFragment = (_tcsicmp(options, _T("DF")) != 0);
	      }

	      if (*ptr != 0)
	         dwPacketSize = _tcstoul(ptr, nullptr, 0);
		}
	}
	Trim(addrStart);

   InetAddress addr = InetAddress::resolveHostName(addrStart);
   if (addr.isValid())
	{
      PING_TARGET *t = new PING_TARGET;
		memset(t, 0, sizeof(PING_TARGET));
		t->ipAddr = addr;
		_tcslcpy(t->dnsName, addrStart, MAX_DB_STRING);
		if (pszName != nullptr)
			_tcslcpy(t->name, pszName, MAX_DB_STRING);
		else
         addr.toString(t->name);
		t->packetSize = dwPacketSize;
		t->dontFragment = dontFragment;
      t->prevRTT = 0xFFFFFFFF;
		t->cumulativeMinRTT = 0x7FFFFFFF;
		t->movingAverageRTT = 0xFFFFFFFF;
      t->movingAverageExp = EMA_EXP(60 / s_pollsPerMinute, s_movingAverageTimePeriod);
      t->movingAverageJitter = 0xFFFFFFFF;
      for(uint32_t i = 0; i < s_pollsPerMinute; i++)
         t->rttHistory[i] = 10001;  // Indicate unused slot
      s_targets.add(t);
		bResult = TRUE;
	}

   MemFree(pszLine);
	return bResult;
}

/**
 * Configuration file template
 */
static TCHAR *m_pszTargetList = nullptr;
static uint32_t s_poolMaxSize = 1024;
static uint32_t s_poolMinSize = 1;
static NX_CFG_TEMPLATE m_cfgTemplate[] =
{
   { _T("AutoConfigureTargets"), CT_BOOLEAN_FLAG_32, 0, 0, PING_OPT_ALLOW_AUTOCONFIGURE, 0, &s_options, nullptr },
	{ _T("DefaultPacketSize"), CT_LONG, 0, 0, 0, 0, &s_defaultPacketSize, nullptr },
   { _T("DefaultDoNotFragmentFlag"), CT_BOOLEAN_FLAG_32, 0, 0, PING_OPT_DONT_FRAGMENT, 0, &s_options, nullptr },
   { _T("MaxTargetInactivityTime"), CT_LONG, 0, 0, 0, 0, &s_maxTargetInactivityTime, nullptr },
   { _T("MovingAverageTimePeriod"), CT_LONG, 0, 0, 0, 0, &s_movingAverageTimePeriod, nullptr },
	{ _T("PacketRate"), CT_LONG, 0, 0, 0, 0, &s_pollsPerMinute, nullptr },
	{ _T("Target"), CT_STRING_CONCAT, _T('\n'), 0, 0, 0, &m_pszTargetList, nullptr },
   { _T("ThreadPoolMaxSize"), CT_LONG, 0, 0, 0, 0, &s_poolMaxSize, nullptr },
   { _T("ThreadPoolMinSize"), CT_LONG, 0, 0, 0, 0, &s_poolMinSize, nullptr },
	{ _T("Timeout"), CT_LONG, 0, 0, 0, 0, &s_timeout, nullptr },
	{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr, nullptr }
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
	   MemFree(m_pszTargetList);
	   return false;
	}

	s_pollers = ThreadPoolCreate(_T("PING"), s_poolMinSize, s_poolMaxSize);

   if (s_pollsPerMinute == 0)
      s_pollsPerMinute = 1;
   else if (s_pollsPerMinute > MAX_POLLS_PER_MINUTE)
      s_pollsPerMinute = MAX_POLLS_PER_MINUTE;
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Packet rate set to %u packets per minute (%u ms between packets)"), s_pollsPerMinute, 60000 / s_pollsPerMinute);

   // Parse target list
   if (m_pszTargetList != nullptr)
   {
      TCHAR *pItem = m_pszTargetList;
      TCHAR *pEnd = _tcschr(pItem, _T('\n'));
      while(pEnd != nullptr)
      {
         *pEnd = 0;
         Trim(pItem);
         if (!AddTargetFromConfig(pItem))
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG,
                  _T("Unable to add ICMP ping target from configuration file. ")
                  _T("Original configuration record: %s"), pItem);
         pItem = pEnd +1;
         pEnd = _tcschr(pItem, _T('\n'));
      }
      MemFree(m_pszTargetList);
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
   { _T("Icmp.CumulativeMaxPingTime(*)"), H_PollResult, _T("M"), DCI_DT_UINT, _T("Cumulative maximum response time of ICMP ping to {instance}") },
   { _T("Icmp.CumulativeMinPingTime(*)"), H_PollResult, _T("M"), DCI_DT_UINT, _T("Cumulative minimum response time of ICMP ping to {instance}") },
   { _T("Icmp.Jitter(*)"), H_PollResult, _T("J"), DCI_DT_UINT, _T("Jitter of ICMP ping to {instance}") },
   { _T("Icmp.LastPingTime(*)"), H_PollResult, _T("L"), DCI_DT_UINT, _T("Response time of last ICMP ping to {instance}") },
   { _T("Icmp.MaxPingTime(*)"), H_PollResult, _T("M"), DCI_DT_UINT, _T("Maximum response time of ICMP ping to {instance} for last minute") },
   { _T("Icmp.MinPingTime(*)"), H_PollResult, _T("M"), DCI_DT_UINT, _T("Minimum response time of ICMP ping to {instance} for last minute") },
   { _T("Icmp.MovingAvgJitter(*)"), H_PollResult, _T("j"), DCI_DT_UINT, _T("Moving average of jitter of ICMP ping to {instance}") },
   { _T("Icmp.MovingAvgPingTime(*)"), H_PollResult, _T("a"), DCI_DT_UINT, _T("Moving average of response time of ICMP ping to {instance}") },
   { _T("Icmp.PacketLoss(*)"), H_PollResult, _T("P"), DCI_DT_UINT, _T("Packet loss for ICMP ping to {instance} for last minute") },
   { _T("Icmp.PingStdDev(*)"), H_PollResult, _T("D"), DCI_DT_UINT, _T("Standard deviation for ICMP ping to {instance} for last minute") },
   { _T("Icmp.Ping(*)"), H_IcmpPing, NULL, DCI_DT_UINT, _T("ICMP ping response time for {instance}") }
};

/**
 * Lists
 */
static NETXMS_SUBAGENT_LIST m_lists[] =
{
   { _T("Icmp.ScanRange(*)"), H_ScanRange, nullptr },
	{ _T("Icmp.Targets"), H_TargetList, nullptr }
};

/**
 * Tables
 */
static NETXMS_SUBAGENT_TABLE m_table[] =
{
    { _T("Icmp.Targets"), H_TargetTable, nullptr, _T("IP"), _T("ICMP ping targets") }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("PING"), NETXMS_VERSION_STRING,
	SubagentInit, SubagentShutdown, nullptr, nullptr, nullptr,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	sizeof(m_lists) / sizeof(NETXMS_SUBAGENT_LIST),
	m_lists,
	sizeof(m_table) / sizeof(NETXMS_SUBAGENT_TABLE),
	m_table,	// tables
   0, nullptr,	// actions
	0, nullptr	// push parameters
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
