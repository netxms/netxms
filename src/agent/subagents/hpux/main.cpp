/* $Id: main.cpp,v 1.10 2007-06-08 00:02:36 alk Exp $ */

/*
** NetXMS subagent for HP-UX
** Copyright (C) 2006 Alex Kirhenshtein
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
**
**/

#include "hpux.h"


//
// Detect support for source packages
//

static LONG H_SourcePkg(char *pszParam, char *pArg, char *pValue)
{
	ret_int(pValue, 1); // assume that we have sane build env
	return SYSINFO_RC_SUCCESS;
}


//
// Initialization callback
//

static BOOL SubAgentInit(TCHAR *pszConfigFile)
{
	StartCpuUsageCollector();
	return TRUE;
}


//
// Called by master agent at unload
//

static void SubAgentShutdown(void)
{
	ShutdownCpuUsageCollector();
}


//
// Subagent information
//

static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
	{ "Agent.SourcePackageSupport",	H_SourcePkg,		NULL,
		DCI_DT_INT, DCIDESC_AGENT_SOURCEPACKAGESUPPORT },

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

	{ "Net.Interface.AdminStatus(*)", H_NetIfInfoFromIOCTL, (char *)IF_INFO_ADMIN_STATUS,
		DCI_DT_INT,		DCIDESC_NET_INTERFACE_ADMINSTATUS },
	{ "Net.Interface.BytesIn(*)",     H_NetIfInfoFromProc, (char *)IF_INFO_BYTES_IN,
		DCI_DT_UINT,	DCIDESC_NET_INTERFACE_BYTESIN },
	{ "Net.Interface.BytesOut(*)",    H_NetIfInfoFromProc, (char *)IF_INFO_BYTES_OUT,
		DCI_DT_UINT,	DCIDESC_NET_INTERFACE_BYTESOUT },
	{ "Net.Interface.Description(*)", H_NetIfInfoFromIOCTL, (char *)IF_INFO_DESCRIPTION,
		DCI_DT_STRING, DCIDESC_NET_INTERFACE_DESCRIPTION },
	{ "Net.Interface.InErrors(*)",    H_NetIfInfoFromProc, (char *)IF_INFO_IN_ERRORS,
		DCI_DT_UINT,	DCIDESC_NET_INTERFACE_INERRORS },
	{ "Net.Interface.Link(*)",        H_NetIfInfoFromIOCTL, (char *)IF_INFO_OPER_STATUS,
		DCI_DT_INT,		DCIDESC_NET_INTERFACE_LINK },
	{ "Net.Interface.OutErrors(*)",   H_NetIfInfoFromProc, (char *)IF_INFO_OUT_ERRORS,
		DCI_DT_UINT,	DCIDESC_NET_INTERFACE_OUTERRORS },
	{ "Net.Interface.PacketsIn(*)",   H_NetIfInfoFromProc, (char *)IF_INFO_PACKETS_IN,
		DCI_DT_UINT,	DCIDESC_NET_INTERFACE_PACKETSIN },
	{ "Net.Interface.PacketsOut(*)",  H_NetIfInfoFromProc, (char *)IF_INFO_PACKETS_OUT,
		DCI_DT_UINT,	DCIDESC_NET_INTERFACE_PACKETSOUT },
	{ "Net.IP.Forwarding",            H_NetIpForwarding, (char *)4,
		DCI_DT_INT,		DCIDESC_NET_IP_FORWARDING },
	{ "Net.IP6.Forwarding",           H_NetIpForwarding, (char *)6,
		DCI_DT_INT,		DCIDESC_NET_IPV6_FORWARDING },

/*	{ "PhysicalDisk.SmartAttr(*)",    H_PhysicalDiskInfo, "A",
		DCI_DT_STRING,	DCIDESC_PHYSICALDISK_SMARTATTR },
	{ "PhysicalDisk.SmartStatus(*)",  H_PhysicalDiskInfo, "S",
		DCI_DT_INT,		DCIDESC_PHYSICALDISK_SMARTSTATUS },
	{ "PhysicalDisk.Temperature(*)",  H_PhysicalDiskInfo, "T",
		DCI_DT_INT,		DCIDESC_PHYSICALDISK_TEMPERATURE },*/

	{ "Process.Count(*)",             H_ProcessCount,    NULL,
		DCI_DT_UINT,	DCIDESC_PROCESS_COUNT },
	{ "System.ProcessCount",          H_SysProcessCount, NULL,
		DCI_DT_UINT,	DCIDESC_SYSTEM_PROCESSCOUNT },

	{ "System.CPU.LoadAvg",           H_CpuLoad,         NULL,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_LOADAVG },
	{ "System.CPU.LoadAvg5",          H_CpuLoad,         NULL,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_LOADAVG5 },
	{ "System.CPU.LoadAvg15",         H_CpuLoad,         NULL,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_LOADAVG15 },
	{ "System.CPU.Usage",             H_CpuUsage,        (char *)0,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE },
	{ "System.CPU.Usage5",            H_CpuUsage,        (char *)5,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5 },
	{ "System.CPU.Usage15",           H_CpuUsage,        (char *)15,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15 },
	{ "System.Hostname",              H_Hostname,        NULL,
		DCI_DT_STRING,	DCIDESC_SYSTEM_HOSTNAME },
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

	{ "System.ConnectedUsers",        H_ConnectedUsers,  NULL,
		DCI_DT_UINT,	DCIDESC_SYSTEM_CONNECTEDUSERS },
};
static NETXMS_SUBAGENT_ENUM m_enums[] =
{
	{ "Net.ArpCache",                 H_NetArpCache,     NULL },
	{ "Net.IP.RoutingTable",          H_NetRoutingTable, NULL },
	{ "Net.InterfaceList",            H_NetIfList,       NULL },
	{ "System.ProcessList",           H_ProcessList,     NULL },
};

static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	"HP-UX",
	NETXMS_VERSION_STRING,
	SubAgentInit, SubAgentShutdown, NULL,
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

DECLARE_SUBAGENT_ENTRY_POINT(HPUX)
{
	*ppInfo = &m_info;
	return TRUE;
}

//
// Entry points for server
//

extern "C" BOOL __NxSubAgentGetIfList(NETXMS_VALUES_LIST *pValue)
{
	return H_NetIfList("Net.InterfaceList", NULL, pValue) == SYSINFO_RC_SUCCESS;
}

extern "C" BOOL __NxSubAgentGetArpCache(NETXMS_VALUES_LIST *pValue)
{
	return H_NetArpCache("Net.ArpCache", NULL, pValue) == SYSINFO_RC_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.9  2007/06/07 22:07:11  alk
descriptions changed to defines

Revision 1.8  2007/04/25 07:44:09  victor
- Linux and HPUX subagents changed to new model
- ODBCQUERY subagent code cleaned

Revision 1.7  2006/10/26 17:46:22  victor
System.CPU.Usage almost complete

Revision 1.6  2006/10/26 15:19:39  victor
Fixed problems with Process.Count and System.ProcessCount on HP-UX 11.23

Revision 1.5  2006/10/26 06:55:17  victor
Minor changes

Revision 1.4  2006/10/06 14:06:11  victor
AIX -> HPUX

Revision 1.3  2006/10/05 12:41:32  alk
generic: iconv - const detection added
hpux: System.LoggedInCount renamed to System.ConnectedUsers
freebsd: _XOPEN_SOURCE fixed

Revision 1.2  2006/10/05 00:34:24  alk
HPUX: minor cleanup; added System.LoggedInCount (W(1) | wc -l equivalent)

Revision 1.1  2006/10/04 14:59:14  alk
initial version of HPUX subagent


*/
