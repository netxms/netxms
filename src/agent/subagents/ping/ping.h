/*
** NetXMS PING subagent
** Copyright (C) 2004-2013 Victor Kirhenshtein
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
** File: ping.h
**
**/

#ifndef _ping_h_
#define _ping_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_agent.h>
#include <math.h>

/**
 * Constants
 */
#define MAX_POLLS_PER_MINUTE     60

/**
 * Target information structure
 */
struct PING_TARGET
{
   InetAddress ipAddr;
   TCHAR dnsName[MAX_DB_STRING];
   TCHAR name[MAX_DB_STRING];
   UINT32 packetSize;
   UINT32 avgRTT;
   UINT32 lastRTT;
   UINT32 stdDevRTT;
   UINT32 packetLoss;
   UINT32 history[MAX_POLLS_PER_MINUTE];
   int bufPos;
	int ipAddrAge;
   THREAD hThread;
};

#endif
