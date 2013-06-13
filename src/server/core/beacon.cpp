/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Victor Kirhenshtein
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


//
// Constants
//

#define MAX_BEACON_HOSTS	32


//
// Beacon checking thread
//

THREAD_RESULT THREAD_CALL BeaconPoller(void *arg)
{
	UINT32 i, interval, timeout, packetSize, hostCount, hostList[MAX_BEACON_HOSTS];
	TCHAR *curr, *next, hosts[1024];

	interval = ConfigReadULong(_T("BeaconPollingInterval"), 1000);
	timeout = ConfigReadULong(_T("BeaconTimeout"), 1000);
	packetSize = ConfigReadULong(_T("IcmpPingSize"), 46);
	
	ConfigReadStr(_T("BeaconHosts"), hosts, 1024, _T(""));
	StrStrip(hosts);
	if (hosts[0] == 0)	// Empty list
	{
		DbgPrintf(1, _T("Beacon poller will not start because beacon host list is empty"));
		return THREAD_OK;
	}
	
	for(curr = hosts, hostCount = 0; (curr != NULL) && (hostCount < MAX_BEACON_HOSTS); curr = next)
	{
		next = _tcschr(curr, _T(','));
		if (next != NULL)
		{
			*next = 0;
			next++;
		}
		StrStrip(curr);
		hostList[hostCount] = ResolveHostName(curr);
		if ((hostList[hostCount] == INADDR_NONE) || (hostList[hostCount] == INADDR_ANY))
		{
			nxlog_write(MSG_INVALID_BEACON, EVENTLOG_WARNING_TYPE, "s", curr);
		}
		else
		{
			hostCount++;
		}
	}

	if (hostCount == 0)
	{
		DbgPrintf(1, _T("Beacon poller will not start because no valid host names was found in beacon list"));
		return THREAD_OK;
	}

	DbgPrintf(1, _T("Beacon poller thread started"));
	while(!(g_dwFlags & AF_SHUTDOWN))
	{
		for(i = 0; i < hostCount; i++)
		{
			if (IcmpPing(hostList[i], 1, timeout, NULL, packetSize) == ICMP_SUCCESS)
				break;	// At least one beacon responds, no need to check others
		}
		if ((i == hostCount) && (!(g_dwFlags & AF_NO_NETWORK_CONNECTIVITY)))
		{
			// All beacons are lost, consider NetXMS server network conectivity loss
			g_dwFlags |= AF_NO_NETWORK_CONNECTIVITY;
			PostEvent(EVENT_NETWORK_CONNECTION_LOST, g_dwMgmtNode, "d", hostCount);
		}
		else if ((i < hostCount) && (g_dwFlags & AF_NO_NETWORK_CONNECTIVITY))
		{
			g_dwFlags &= ~AF_NO_NETWORK_CONNECTIVITY;
			PostEvent(EVENT_NETWORK_CONNECTION_RESTORED, g_dwMgmtNode, "d", hostCount);
		}
		ThreadSleepMs(interval);
	}
	DbgPrintf(1, _T("Beacon poller thread terminated"));
	return THREAD_OK;
}
