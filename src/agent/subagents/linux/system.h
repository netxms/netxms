/* $Id$ */

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

#ifndef __SYSTEM_H__
#define __SYSTEM_H__

enum
{
	PHYSICAL_FREE,
	PHYSICAL_FREE_PCT,
	PHYSICAL_USED,
	PHYSICAL_USED_PCT,
	PHYSICAL_TOTAL,
	SWAP_FREE,
	SWAP_FREE_PCT,
	SWAP_USED,
	SWAP_USED_PCT,
	SWAP_TOTAL,
	VIRTUAL_FREE,
	VIRTUAL_FREE_PCT,
	VIRTUAL_USED,
	VIRTUAL_USED_PCT,
	VIRTUAL_TOTAL,
};

enum
{
	INTERVAL_1MIN,
	INTERVAL_5MIN,
	INTERVAL_15MIN,
};

enum {
	CPU_USAGE_OVERAL,
	CPU_USAGE_USER,
	CPU_USAGE_NICE,
	CPU_USAGE_SYSTEM,
	CPU_USAGE_IDLE,
	CPU_USAGE_IOWAIT,
	CPU_USAGE_IRQ,
	CPU_USAGE_SOFTIRQ,
	CPU_USAGE_STEAL,
};

struct CpuUsageParam
{
	int timeInterval;
	int source;
};

LONG H_ProcessList(const char *, const char *, NETXMS_VALUES_LIST *);
LONG H_Uptime(const char *, const char *, char *);
LONG H_Uname(const char *, const char *, char *);
LONG H_Hostname(const char *, const char *, char *);
LONG H_Hostname(const char *, const char *, char *);
LONG H_CpuCount(const char *, const char *, char *);
LONG H_CpuLoad(const char *, const char *, char *);
LONG H_CpuUsage(const char *, const char *, char *);
LONG H_CpuUsageEx(const char *, const char *, char *);
LONG H_ProcessCount(const char *, const char *, char *);
LONG H_MemoryInfo(const char *, const char *, char *);
LONG H_SourcePkgSupport(const char *, const char *, char *);
LONG H_ConnectedUsers(const char *, const char *, char *);
LONG H_ActiveUserSessions(const char *, const char *, NETXMS_VALUES_LIST *);

void StartCpuUsageCollector(void);
void ShutdownCpuUsageCollector(void);

#endif // __SYSTEM_H__

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.5  2007/10/31 08:14:33  victor
Added per-CPU usage statistics and System.CPU.Count parameter

Revision 1.4  2006/10/30 17:25:10  victor
Implemented System.ConnectedUsers and System.ActiveUserSessions

Revision 1.3  2006/03/01 22:13:09  alk
added System.CPU.Usage [broken]

Revision 1.2  2005/01/17 23:31:01  alk
Agent.SourcePackageSupport added

Revision 1.1  2004/10/22 22:08:35  alk
source restructured;
implemented:
	Net.IP.Forwarding
	Net.IP6.Forwarding
	Process.Count(*)
	Net.ArpCache
	Net.InterfaceList (if-type not implemented yet)
	System.ProcessList


*/
