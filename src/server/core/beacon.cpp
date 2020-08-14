/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
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
** File: beacon.cpp
**
**/

#include "nxcore.h"

#define DEBUG_TAG _T("beacon")

/**
 * Beacon hosts poller
 */
void BeaconPoller()
{
	uint32_t interval = ConfigReadULong(_T("Beacon.PollingInterval"), 1000);
	uint32_t timeout = ConfigReadULong(_T("Beacon.Timeout"), 1000);
	uint32_t packetSize = ConfigReadULong(_T("IcmpPingSize"), 46);

	TCHAR hosts[MAX_CONFIG_VALUE];
	ConfigReadStr(_T("Beacon.Hosts"), hosts, MAX_CONFIG_VALUE, _T(""));
	StrStrip(hosts);
	if (hosts[0] == 0)	// Empty list
	{
	   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Beacon poller will not start because beacon host list is empty"));
		return;
	}

   InetAddressList hostList;
	for(TCHAR *curr = hosts, *next = NULL; curr != NULL; curr = next)
	{
		next = _tcschr(curr, _T(','));
		if (next != NULL)
		{
			*next = 0;
			next++;
		}
		StrStrip(curr);
      InetAddress addr = InetAddress::resolveHostName(curr);
      if (addr.isValidUnicast())
      {
         hostList.add(addr);
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Beacon host %s added"), (const TCHAR *)addr.toString());
      }
      else
		{
			nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Invalid beacon host name or address %s - host will be excluded from beacon list"), curr);
		}
	}

   if (hostList.size() == 0)
	{
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Beacon poller will not start because no valid host names was found in beacon list"));
		return;
	}

	nxlog_debug_tag(DEBUG_TAG, 1, _T("Beacon poller thread started"));
	while(!(g_flags & AF_SHUTDOWN))
	{
      int i;
      for(i = 0; i < hostList.size(); i++)
		{
         nxlog_debug_tag(DEBUG_TAG, 7, _T("Beacon poller: checking host %s"), hostList.get(i).toString(hosts));
         if (IcmpPing(hostList.get(i), 1, timeout, NULL, packetSize, false) == ICMP_SUCCESS)
				break;	// At least one beacon responds, no need to check others
		}
      if ((i == hostList.size()) && (!(g_flags & AF_NO_NETWORK_CONNECTIVITY)))
		{
			// All beacons are lost, consider NetXMS server network conectivity loss
			g_flags |= AF_NO_NETWORK_CONNECTIVITY;
         PostSystemEvent(EVENT_NETWORK_CONNECTION_LOST, g_dwMgmtNode, "d", hostList.size());
		}
      else if ((i < hostList.size()) && (g_flags & AF_NO_NETWORK_CONNECTIVITY))
		{
			g_flags &= ~AF_NO_NETWORK_CONNECTIVITY;
			PostSystemEvent(EVENT_NETWORK_CONNECTION_RESTORED, g_dwMgmtNode, "d", hostList.size());
		}
		ThreadSleepMs(interval);
	}
	nxlog_debug_tag(DEBUG_TAG, 1, _T("Beacon poller thread terminated"));
}
