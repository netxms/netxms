/* 
** NetXMS subagent for NetBSD
** Copyright (C) 2004 Alex Kirhenshtein
** Copyright (C) 2008 Mark Ibell
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
	{ "Agent.SourcePackageSupport",   H_SourcePkgSupport,NULL,
		DCI_DT_INT,		DCIDESC_AGENT_SOURCEPACKAGESUPPORT },

	{ "Disk.Avail(*)",                H_DiskInfo,        (char *)DISK_AVAIL,
		DCI_DT_UINT64,	DCIDESC_DISK_AVAIL },
	{ "Disk.AvailPerc(*)",            H_DiskInfo,        (char *)DISK_AVAIL_PERC,
		DCI_DT_FLOAT,	DCIDESC_DISK_AVAILPERC },
	{ "Disk.Free(*)",                 H_DiskInfo,        (char *)DISK_FREE,
		DCI_DT_UINT64,	DCIDESC_DISK_FREE },
	{ "Disk.FreePerc(*)",             H_DiskInfo,        (char *)DISK_FREE_PERC,
		DCI_DT_FLOAT,	DCIDESC_DISK_FREEPERC },
	{ "Disk.Total(*)",                H_DiskInfo,        (char *)DISK_TOTAL,
		DCI_DT_UINT64,	DCIDESC_DISK_TOTAL },
	{ "Disk.Used(*)",                 H_DiskInfo,        (char *)DISK_USED,
		DCI_DT_UINT64,	DCIDESC_DISK_USED },
	{ "Disk.UsedPerc(*)",             H_DiskInfo,        (char *)DISK_USED_PERC,
		DCI_DT_FLOAT,	DCIDESC_DISK_USEDPERC },

	{ "Net.IP.Forwarding",            H_NetIpForwarding, (char *)4,
		DCI_DT_INT,		DCIDESC_NET_IP_FORWARDING },
	{ "Net.IP6.Forwarding",           H_NetIpForwarding, (char *)6,
		DCI_DT_INT,		DCIDESC_NET_IP6_FORWARDING },
	{ "Net.Interface.AdminStatus(*)", H_NetIfAdmStatus,  NULL,
		DCI_DT_INT,		DCIDESC_NET_INTERFACE_ADMINSTATUS },
	{ "Net.Interface.Link(*)",        H_NetIfLink,       NULL,
		DCI_DT_INT,		DCIDESC_NET_INTERFACE_LINK },
	{ "Net.Interface.BytesIn(*)",     H_NetIfInfoFromKVM, (char *)IF_INFO_BYTES_IN,
		DCI_DT_UINT64,		DCIDESC_NET_INTERFACE_BYTESIN },
	{ "Net.Interface.BytesOut(*)",    H_NetIfInfoFromKVM, (char *)IF_INFO_BYTES_OUT,
		DCI_DT_UINT64,		DCIDESC_NET_INTERFACE_BYTESOUT },
	{ "Net.Interface.InErrors(*)",    H_NetIfInfoFromKVM, (char *)IF_INFO_IN_ERRORS,
		DCI_DT_UINT64,		DCIDESC_NET_INTERFACE_INERRORS },
	{ "Net.Interface.OutErrors(*)",   H_NetIfInfoFromKVM, (char *)IF_INFO_OUT_ERRORS,
		DCI_DT_UINT64,		DCIDESC_NET_INTERFACE_OUTERRORS },
	{ "Net.Interface.PacketsIn(*)",   H_NetIfInfoFromKVM, (char *)IF_INFO_PACKETS_IN,
		DCI_DT_UINT64,		DCIDESC_NET_INTERFACE_PACKETSIN },
	{ "Net.Interface.PacketsOut(*)",  H_NetIfInfoFromKVM, (char *)IF_INFO_PACKETS_OUT,
		DCI_DT_UINT64,		DCIDESC_NET_INTERFACE_PACKETSOUT },

	{ "Process.Count(*)",             H_ProcessCount,    "P",			DCI_DT_UINT,	DCIDESC_PROCESS_COUNT },
	{ "Process.CountEx(*)",           H_ProcessCount,    "E",			DCI_DT_UINT,	DCIDESC_PROCESS_COUNTEX },
#if HAVE_STRUCT_KINFO_PROC2
	{ "Process.CPUTime(*)",           H_ProcessInfo,     CAST_TO_POINTER(PROCINFO_CPUTIME, const char *),
		DCI_DT_INT64,	DCIDESC_PROCESS_CPUTIME },
	{ "Process.KernelTime(*)",        H_ProcessInfo,     CAST_TO_POINTER(PROCINFO_KTIME, const char *),
		DCI_DT_INT64,	DCIDESC_PROCESS_CPUTIME },
	{ "Process.PageFaults(*)",        H_ProcessInfo,     CAST_TO_POINTER(PROCINFO_PAGEFAULTS, const char *),
		DCI_DT_INT64,	DCIDESC_PROCESS_PAGEFAULTS },
	{ "Process.Threads(*)",           H_ProcessInfo,     CAST_TO_POINTER(PROCINFO_THREADS, const char *),
		DCI_DT_INT64,	DCIDESC_PROCESS_THREADS },
	{ "Process.UserTime(*)",          H_ProcessInfo,     CAST_TO_POINTER(PROCINFO_UTIME, const char *),
		DCI_DT_INT64,	DCIDESC_PROCESS_CPUTIME },
	{ "Process.VMSize(*)",            H_ProcessInfo,     CAST_TO_POINTER(PROCINFO_VMSIZE, const char *),
		DCI_DT_INT64,	DCIDESC_PROCESS_VMSIZE },
	{ "Process.WkSet(*)",             H_ProcessInfo,     CAST_TO_POINTER(PROCINFO_WKSET, const char *),
		DCI_DT_INT64,	DCIDESC_PROCESS_WKSET },
#endif

	{ "System.CPU.Count",             H_CpuCount,        NULL,
		DCI_DT_UINT,	DCIDESC_PROCESS_COUNT },

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

	{ "System.ProcessCount",          H_ProcessCount,    "S",
		DCI_DT_UINT,	DCIDESC_SYSTEM_PROCESSCOUNT },
#if HAVE_STRUCT_KINFO_PROC2
	{ "System.ThreadCount",           H_ProcessCount,    "T",
		DCI_DT_UINT,	DCIDESC_SYSTEM_THREADCOUNT },
#endif

	{ "System.Uname",                 H_Uname,           NULL,
		DCI_DT_STRING,	DCIDESC_SYSTEM_UNAME },
	{ "System.Uptime",                H_Uptime,          NULL,
		DCI_DT_UINT,	DCIDESC_SYSTEM_UPTIME },
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
	"NetBSD",
	NETXMS_VERSION_STRING,
	NULL, NULL, NULL,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	sizeof(m_enums) / sizeof(NETXMS_SUBAGENT_ENUM),
	m_enums,
	0,
	NULL
};

//
// Entry point for NetXMS agent
//

DECLARE_SUBAGENT_ENTRY_POINT(NETBSD)
{
	*ppInfo = &m_info;
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.5  2007/06/08 13:49:49  alk
fixed NETXMS_SUBAGENT_INFO init

Revision 1.4  2007/06/08 00:02:36  alk
DECLARE_SUBAGENT_INIT replaced with DECLARE_SUBAGENT_ENTRY_POINT

NETXMS_SUBAGENT_INFO initialization fixed (actions)

Revision 1.3  2007/06/07 22:06:53  alk
1) descriptions changed to defines
2) params structure was filled incorrectly

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
