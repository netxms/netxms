/* $Id: main.cpp,v 1.4 2006-10-06 14:06:11 victor Exp $ */

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
// Called by master agent at unload
//

static void UnloadHandler(void)
{
	ShutdownCpuUsageCollector();
}


//
// Subagent information
//

static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
	{ "Agent.SourcePackageSupport",	H_SourcePkg,		NULL,
		DCI_DT_INT, "" },

	{ "Disk.Avail(*)",                H_DiskInfo,        (char *)DISK_AVAIL,
		DCI_DT_UINT64,	"Available disk space on {instance}" },
	{ "Disk.AvailPerc(*)",            H_DiskInfo,        (char *)DISK_AVAIL_PERC,
		DCI_DT_FLOAT,	"Percentage of available disk space on {instance}" },
	{ "Disk.Free(*)",                 H_DiskInfo,        (char *)DISK_FREE,
		DCI_DT_UINT64,	"Free disk space on {instance}" },
	{ "Disk.FreePerc(*)",             H_DiskInfo,        (char *)DISK_FREE_PERC,
		DCI_DT_FLOAT,	"Percentage of free disk space on {instance}" },
	{ "Disk.Total(*)",                H_DiskInfo,        (char *)DISK_TOTAL,
		DCI_DT_UINT64,	"Total disk space on {instance}" },
	{ "Disk.Used(*)",                 H_DiskInfo,        (char *)DISK_USED,
		DCI_DT_UINT64,	"Used disk space on {instance}" },
	{ "Disk.UsedPerc(*)",             H_DiskInfo,        (char *)DISK_USED_PERC,
		DCI_DT_FLOAT,	"Percentage of used disk space on {instance}" },

	{ "Net.Interface.AdminStatus(*)", H_NetIfInfoFromIOCTL, (char *)IF_INFO_ADMIN_STATUS,
		DCI_DT_INT,		"Administrative status of interface {instance}" },
	{ "Net.Interface.BytesIn(*)",     H_NetIfInfoFromProc, (char *)IF_INFO_BYTES_IN,
		DCI_DT_UINT,	"Number of input bytes on interface {instance}" },
	{ "Net.Interface.BytesOut(*)",    H_NetIfInfoFromProc, (char *)IF_INFO_BYTES_OUT,
		DCI_DT_UINT,	"Number of output bytes on interface {instance}" },
	{ "Net.Interface.Description(*)", H_NetIfInfoFromIOCTL, (char *)IF_INFO_DESCRIPTION,
		DCI_DT_STRING, "Description of interface {instance}" },
	{ "Net.Interface.InErrors(*)",    H_NetIfInfoFromProc, (char *)IF_INFO_IN_ERRORS,
		DCI_DT_UINT,	"Number of input errors on interface {instance}" },
	{ "Net.Interface.Link(*)",        H_NetIfInfoFromIOCTL, (char *)IF_INFO_OPER_STATUS,
		DCI_DT_INT,		"Operational status of interface {instance}" },
	{ "Net.Interface.OutErrors(*)",   H_NetIfInfoFromProc, (char *)IF_INFO_OUT_ERRORS,
		DCI_DT_UINT,	"Number of output errors on interface {instance}" },
	{ "Net.Interface.PacketsIn(*)",   H_NetIfInfoFromProc, (char *)IF_INFO_PACKETS_IN,
		DCI_DT_UINT,	"Number of input packets on interface {instance}" },
	{ "Net.Interface.PacketsOut(*)",  H_NetIfInfoFromProc, (char *)IF_INFO_PACKETS_OUT,
		DCI_DT_UINT,	"Number of output packets on interface {instance}" },
	{ "Net.IP.Forwarding",            H_NetIpForwarding, (char *)4,
		DCI_DT_INT,		"IP forwarding status" },
	{ "Net.IP6.Forwarding",           H_NetIpForwarding, (char *)6,
		DCI_DT_INT,		"IPv6 forwarding status" },

/*	{ "PhysicalDisk.SmartAttr(*)",    H_PhysicalDiskInfo, "A",
		DCI_DT_STRING,	"" },
	{ "PhysicalDisk.SmartStatus(*)",  H_PhysicalDiskInfo, "S",
		DCI_DT_INT,		"Status of hard disk {instance} reported by SMART" },
	{ "PhysicalDisk.Temperature(*)",  H_PhysicalDiskInfo, "T",
		DCI_DT_INT,		"Temperature of hard disk {instance}" },*/

	{ "Process.Count(*)",             H_ProcessCount,    (char *)0,
		DCI_DT_UINT,	"Number of {instance} processes" },
	{ "System.ProcessCount",          H_ProcessCount,    (char *)1,
		DCI_DT_UINT,	"Total number of processes" },

	{ "System.CPU.LoadAvg",           H_CpuLoad,         NULL,
		DCI_DT_FLOAT,	"Average CPU load for last minute" },
	{ "System.CPU.LoadAvg5",          H_CpuLoad,         NULL,
		DCI_DT_FLOAT,	"Average CPU load for last 5 minutes" },
	{ "System.CPU.LoadAvg15",         H_CpuLoad,         NULL,
		DCI_DT_FLOAT,	"Average CPU load for last 15 minutes" },
	{ "System.CPU.Usage",             H_CpuUsage,        (char *)0,
		DCI_DT_FLOAT,	"" },
	{ "System.CPU.Usage5",            H_CpuUsage,        (char *)5,
		DCI_DT_FLOAT,	"" },
	{ "System.CPU.Usage15",           H_CpuUsage,        (char *)15,
		DCI_DT_FLOAT,	"" },
	{ "System.Hostname",              H_Hostname,        NULL,
		DCI_DT_STRING,	"Host name" },
	{ "System.Memory.Physical.Free",  H_MemoryInfo,      (char *)PHYSICAL_FREE,
		DCI_DT_UINT64,	"Free physical memory" },
	{ "System.Memory.Physical.Total", H_MemoryInfo,      (char *)PHYSICAL_TOTAL,
		DCI_DT_UINT64,	"Total amount of physical memory" },
	{ "System.Memory.Physical.Used",  H_MemoryInfo,      (char *)PHYSICAL_USED,
		DCI_DT_UINT64,	"Used physical memory" },
	{ "System.Memory.Swap.Free",      H_MemoryInfo,      (char *)SWAP_FREE,
		DCI_DT_UINT64,	"Free swap space" },
	{ "System.Memory.Swap.Total",     H_MemoryInfo,      (char *)SWAP_TOTAL,
		DCI_DT_UINT64,	"Total amount of swap space" },
	{ "System.Memory.Swap.Used",      H_MemoryInfo,      (char *)SWAP_USED,
		DCI_DT_UINT64,	"Used swap space" },
	{ "System.Memory.Virtual.Free",   H_MemoryInfo,      (char *)VIRTUAL_FREE,
		DCI_DT_UINT64,	"Free virtual memory" },
	{ "System.Memory.Virtual.Total",  H_MemoryInfo,      (char *)VIRTUAL_TOTAL,
		DCI_DT_UINT64,	"Total amount of virtual memory" },
	{ "System.Memory.Virtual.Used",   H_MemoryInfo,      (char *)VIRTUAL_USED,
		DCI_DT_UINT64,	"Used virtual memory" },
	{ "System.Uname",                 H_Uname,           NULL,
		DCI_DT_STRING,	"System uname" },
	{ "System.Uptime",                H_Uptime,          NULL,
		DCI_DT_UINT,	"System uptime" },

	{ "System.ConnectedUsers",        H_W,               NULL,
		DCI_DT_UINT,	"Number of logged in users" },
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
	UnloadHandler,
	NULL,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	sizeof(m_enums) / sizeof(NETXMS_SUBAGENT_ENUM),
	m_enums
};


//
// Entry point for NetXMS agent
//

DECLARE_SUBAGENT_INIT(HPUX)
{
	StartCpuUsageCollector();

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
Revision 1.3  2006/10/05 12:41:32  alk
generic: iconv - const detection added
hpux: System.LoggedInCount renamed to System.ConnectedUsers
freebsd: _XOPEN_SOURCE fixed

Revision 1.2  2006/10/05 00:34:24  alk
HPUX: minor cleanup; added System.LoggedInCount (W(1) | wc -l equivalent)

Revision 1.1  2006/10/04 14:59:14  alk
initial version of HPUX subagent


*/
