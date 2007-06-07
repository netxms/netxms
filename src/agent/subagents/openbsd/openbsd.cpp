/* $Id: openbsd.cpp,v 1.3 2007-06-07 22:06:53 alk Exp $ */

/* 
** NetXMS subagent for FreeBSD
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

#include <nms_common.h>
#include <nms_agent.h>

#include "net.h"
#include "system.h"
#include "disk.h"

//
// Subagent information
//

static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
	{ "System.CPU.Count",             H_CpuCount,        NULL,
		DCI_DT_UINT,	DCIDESC_PROCESS_COUNT },

	{ "Disk.Free(*)",                 H_DiskInfo,        (char *)DISK_FREE,
		DCI_DT_UINT64,	DCIDESC_DISK_FREE },
	{ "Disk.Total(*)",                H_DiskInfo,        (char *)DISK_TOTAL,
		DCI_DT_UINT64,	DCIDESC_DISK_TOTAL },
	{ "Disk.Used(*)",                 H_DiskInfo,        (char *)DISK_USED,
		DCI_DT_UINT64,	DCIDESC_DISK_USED },

	{ "Net.IP.Forwarding",            H_NetIpForwarding, (char *)4,
		DCI_DT_INT,		DCIDESC_NET_IP_FORWARDING },
	{ "Net.IP6.Forwarding",           H_NetIpForwarding, (char *)6,
		DCI_DT_INT,		DCIDESC_NET_IP6_FORWARDING },
	{ "Net.Interface.AdminStatus(*)", H_NetIfAdmStatus,  NULL,
		DCI_DT_INT,		DCIDESC_NET_INTERFACE_ADMINSTATUS },
	{ "Net.Interface.Link(*)",        H_NetIfLink,       NULL,
		DCI_DT_INT,		DCIDESC_NET_INTERFACE_LINK },

	{ "Process.Count(*)",             H_ProcessCount,    (char *)0,
		DCI_DT_UINT,	DCIDESC_PROCESS_COUNT },
	{ "System.ProcessCount",          H_ProcessCount,    (char *)1,
		DCI_DT_UINT,	DCIDESC_SYSTEM_PROCESSCOUNT },

	{ "System.CPU.LoadAvg",           H_CpuLoad,         NULL,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_LOADAVG },
	{ "System.CPU.LoadAvg5",          H_CpuLoad,         NULL,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_LOADAVG5 },
	{ "System.CPU.LoadAvg15",         H_CpuLoad,         NULL,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_LOADAVG15 },
/*
	{ "System.CPU.Usage",             H_CpuUsage,        NULL,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE },
	{ "System.CPU.Usage5",            H_CpuUsage,        NULL,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5 },
	{ "System.CPU.Usage15",           H_CpuUsage,        NULL,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15 },
*/
	{ "System.Hostname",              H_Hostname,        NULL,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_HOSTNAME },
	{ "System.Memory.Physical.Free",  H_MemoryInfo,      (char *)PHYSICAL_FREE,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE },
	{ "System.Memory.Physical.Total", H_MemoryInfo,      (char *)PHYSICAL_TOTAL,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_PHYSICAL_TOTAL },
	{ "System.Memory.Physical.Used",  H_MemoryInfo,      (char *)PHYSICAL_USED,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED },
	{ "System.Memory.Swap.Free",      H_MemoryInfo,      (char *)SWAP_FREE,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_SWAP_FREE },
	{ "System.Memory.Swap.Total",     H_MemoryInfo,      (char *)SWAP_TOTAL,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_SWAP_TOTAL },
	{ "System.Memory.Swap.Used",      H_MemoryInfo,      (char *)SWAP_USED,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_SWAP_USED },
	{ "System.Memory.Virtual.Free",   H_MemoryInfo,      (char *)VIRTUAL_FREE,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_VIRTUAL_FREE },
	{ "System.Memory.Virtual.Total",  H_MemoryInfo,      (char *)VIRTUAL_TOTAL,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_VIRTUAL_TOTAL },
	{ "System.Memory.Virtual.Used",   H_MemoryInfo,      (char *)VIRTUAL_USED,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_VIRTUAL_USED },
	{ "System.Uname",                 H_Uname,           NULL,
		DCI_DT_STRING,	DCIDESC_SYSTEM_UNAME },
	{ "System.Uptime",                H_Uptime,          NULL,
		DCI_DT_UINT,	DCIDESC_SYSTEM_UPTIME },

	{ "Agent.SourcePackageSupport",   H_SourcePkgSupport,NULL,
		DCI_DT_INT,		DCIDESC_AGENT_SOURCEPACKAGESUPPORT },
};

static NETXMS_SUBAGENT_ENUM m_enums[] =
{
	{ "Net.ArpCache",                 H_NetArpCache,     NULL },
	{ "Net.InterfaceList",            H_NetIfList,       NULL },
	{ "Net.IP.RoutingTable",          H_NetRoutingTable, NULL },
	{ "System.ProcessList",           H_ProcessList,     NULL },
};

static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	"FreeBSD",
	NETXMS_VERSION_STRING,
	NULL, NULL,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	sizeof(m_enums) / sizeof(NETXMS_SUBAGENT_ENUM),
	m_enums
};

//
// Entry point for NetXMS agent
//

DECLARE_SUBAGENT_INIT(FREEBSD)
{
	*ppInfo = &m_info;
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.2  2007/05/23 10:46:59  victor
Minor changes

Revision 1.1  2006/03/07 09:42:48  alk
OpenBSD subagent incorporated
flex params changed in libnxsl's makefile

Revision 1.7  2005/09/15 21:47:02  victor
Added macro DECLARE_SUBAGENT_INIT to simplify initialization function declaration

Revision 1.6  2005/08/22 23:00:05  alk
Net.IP.RoutingTable added

Revision 1.5  2005/03/10 19:04:07  alk
implemented:
	Net.Interface.AdminStatus(*)
	Net.Interface.Link(*)

Revision 1.4  2005/01/24 19:51:16  alk
reurn types/comments added
Process.Count(*)/System.ProcessCount fixed

Revision 1.3  2005/01/23 05:08:06  alk
+ System.CPU.Count
+ System.Memory.Physical.*
+ System.ProcessCount
+ System.ProcessList

Revision 1.2  2005/01/17 23:25:47  alk
Agent.SourcePackageSupport added

Revision 1.1  2005/01/17 17:14:32  alk
freebsd agent, incomplete (but working)


*/
