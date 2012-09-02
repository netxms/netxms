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
	{ _T("Disk.Avail(*)"),                H_DiskInfo,        (const TCHAR *)DISK_AVAIL,		DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
	{ _T("Disk.AvailPerc(*)"),            H_DiskInfo,        (const TCHAR *)DISK_AVAIL_PERC,	DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
	{ _T("Disk.Free(*)"),                 H_DiskInfo,        (const TCHAR *)DISK_FREE,		DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
	{ _T("Disk.FreePerc(*)"),             H_DiskInfo,        (const TCHAR *)DISK_FREE_PERC,		DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
	{ _T("Disk.Total(*)"),                H_DiskInfo,        (const TCHAR *)DISK_TOTAL,		DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
	{ _T("Disk.Used(*)"),                 H_DiskInfo,        (const TCHAR *)DISK_USED,		DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
	{ _T("Disk.UsedPerc(*)"),             H_DiskInfo,        (const TCHAR *)DISK_USED_PERC,		DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },

	{ _T("FileSystem.Avail(*)"),          H_DiskInfo,        (const TCHAR *)DISK_AVAIL,        DCI_DT_UINT64,	DCIDESC_FS_AVAIL },
	{ _T("FileSystem.AvailPerc(*)"),      H_DiskInfo,        (const TCHAR *)DISK_AVAIL_PERC,   DCI_DT_FLOAT,	DCIDESC_FS_AVAILPERC },
	{ _T("FileSystem.Free(*)"),           H_DiskInfo,        (const TCHAR *)DISK_FREE,         DCI_DT_UINT64,	DCIDESC_FS_FREE },
	{ _T("FileSystem.FreePerc(*)"),       H_DiskInfo,        (const TCHAR *)DISK_FREE_PERC,    DCI_DT_FLOAT,	DCIDESC_FS_FREEPERC },
	{ _T("FileSystem.Total(*)"),          H_DiskInfo,        (const TCHAR *)DISK_TOTAL,        DCI_DT_UINT64,	DCIDESC_FS_TOTAL },
	{ _T("FileSystem.Used(*)"),           H_DiskInfo,        (const TCHAR *)DISK_USED,         DCI_DT_UINT64,	DCIDESC_FS_USED },
	{ _T("FileSystem.UsedPerc(*)"),       H_DiskInfo,        (const TCHAR *)DISK_USED_PERC,    DCI_DT_FLOAT,	DCIDESC_FS_USEDPERC },

	{ _T("Net.Interface.AdminStatus(*)"), H_NetIfAdmStatus,  NULL,				DCI_DT_INT,	DCIDESC_NET_INTERFACE_ADMINSTATUS },
	{ _T("Net.Interface.BytesIn(*)"),     H_NetIfInfoFromKVM, (const TCHAR *)IF_INFO_BYTES_IN,    DCI_DT_UINT64, DCIDESC_NET_INTERFACE_BYTESIN },
	{ _T("Net.Interface.BytesOut(*)"),    H_NetIfInfoFromKVM, (const TCHAR *)IF_INFO_BYTES_OUT,   DCI_DT_UINT64, DCIDESC_NET_INTERFACE_BYTESOUT },
	{ _T("Net.Interface.InErrors(*)"),    H_NetIfInfoFromKVM, (const TCHAR *)IF_INFO_IN_ERRORS,   DCI_DT_UINT64, DCIDESC_NET_INTERFACE_INERRORS },
	{ _T("Net.Interface.Link(*)"),        H_NetIfLink,       NULL,				DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
	{ _T("Net.Interface.OperStatus(*)"),  H_NetIfLink,       NULL,				DCI_DT_INT,	DCIDESC_NET_INTERFACE_OPERSTATUS },
	{ _T("Net.Interface.OutErrors(*)"),   H_NetIfInfoFromKVM, (const TCHAR *)IF_INFO_OUT_ERRORS,  DCI_DT_UINT64, DCIDESC_NET_INTERFACE_OUTERRORS },
	{ _T("Net.Interface.PacketsIn(*)"),   H_NetIfInfoFromKVM, (const TCHAR *)IF_INFO_PACKETS_IN,  DCI_DT_UINT64, DCIDESC_NET_INTERFACE_PACKETSIN },
	{ _T("Net.Interface.PacketsOut(*)"),  H_NetIfInfoFromKVM, (const TCHAR *)IF_INFO_PACKETS_OUT, DCI_DT_UINT64, DCIDESC_NET_INTERFACE_PACKETSOUT },
	{ _T("Net.IP.Forwarding"),            H_NetIpForwarding, (const TCHAR *)4,			DCI_DT_INT,	DCIDESC_NET_IP_FORWARDING },
	{ _T("Net.IP6.Forwarding"),           H_NetIpForwarding, (const TCHAR *)6,			DCI_DT_INT,	DCIDESC_NET_IP6_FORWARDING },

	{ _T("Process.Count(*)"),             H_ProcessCount,    _T("P"),			DCI_DT_UINT,	DCIDESC_PROCESS_COUNT },
	{ _T("Process.CountEx(*)"),           H_ProcessCount,    _T("E"),			DCI_DT_UINT,	DCIDESC_PROCESS_COUNTEX },
	{ _T("Process.CPUTime(*)"),           H_ProcessInfo,     CAST_TO_POINTER(PROCINFO_CPUTIME, const TCHAR *),
		DCI_DT_INT64,	DCIDESC_PROCESS_CPUTIME },
	{ _T("Process.Threads(*)"),           H_ProcessInfo,     CAST_TO_POINTER(PROCINFO_THREADS, const TCHAR *),
		DCI_DT_INT64,	DCIDESC_PROCESS_THREADS },
	{ _T("Process.VMSize(*)"),            H_ProcessInfo,     CAST_TO_POINTER(PROCINFO_VMSIZE, const TCHAR *),
		DCI_DT_INT64,	DCIDESC_PROCESS_VMSIZE },
	{ _T("Process.WkSet(*)"),             H_ProcessInfo,     CAST_TO_POINTER(PROCINFO_WKSET, const TCHAR *),
		DCI_DT_INT64,	DCIDESC_PROCESS_WKSET },

	{ _T("System.CPU.Count"),             H_CpuCount,        NULL,				DCI_DT_INT,	DCIDESC_SYSTEM_CPU_COUNT },

	{ _T("System.CPU.LoadAvg"),           H_CpuLoad,         NULL,				DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_LOADAVG },
	{ _T("System.CPU.LoadAvg5"),          H_CpuLoad,         NULL,				DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_LOADAVG5 },
	{ _T("System.CPU.LoadAvg15"),         H_CpuLoad,         NULL,				DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_LOADAVG15 },
	{ _T("System.Hostname"),              H_Hostname,        NULL,				DCI_DT_FLOAT,	DCIDESC_SYSTEM_HOSTNAME },
	{ _T("System.Memory.Physical.Free"),  H_MemoryInfo,      (const TCHAR *)PHYSICAL_FREE,		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE },
	{ _T("System.Memory.Physical.FreePerc"), H_MemoryInfo,   (const TCHAR *)PHYSICAL_FREE_PCT,	DCI_DT_UINT,	DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE_PCT },
	{ _T("System.Memory.Physical.Total"), H_MemoryInfo,      (const TCHAR *)PHYSICAL_TOTAL,		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_PHYSICAL_TOTAL },
	{ _T("System.Memory.Physical.Used"),  H_MemoryInfo,      (const TCHAR *)PHYSICAL_USED,		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED },
	{ _T("System.Memory.Physical.UsedPerc"), H_MemoryInfo,   (const TCHAR *)PHYSICAL_USED_PCT,	DCI_DT_UINT,	DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED_PCT },
#if HAVE_KVM_SWAPINFO
	{ _T("System.Memory.Swap.Free"),      H_MemoryInfo,      (const TCHAR *)SWAP_FREE,		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_SWAP_FREE },
	{ _T("System.Memory.Swap.FreePerc"),  H_MemoryInfo,      (const TCHAR *)SWAP_FREE_PCT,	DCI_DT_UINT,	DCIDESC_SYSTEM_MEMORY_SWAP_FREE_PCT },
	{ _T("System.Memory.Swap.Total"),     H_MemoryInfo,      (const TCHAR *)SWAP_TOTAL,	DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_SWAP_TOTAL },
	{ _T("System.Memory.Swap.Used"),      H_MemoryInfo,      (const TCHAR *)SWAP_USED,		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_SWAP_USED },
	{ _T("System.Memory.Swap.UsedPerc"),      H_MemoryInfo,  (const TCHAR *)SWAP_USED_PCT,	DCI_DT_UINT,	DCIDESC_SYSTEM_MEMORY_SWAP_USED_PCT },
	{ _T("System.Memory.Virtual.Free"),   H_MemoryInfo,      (const TCHAR *)VIRTUAL_FREE,	DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_VIRTUAL_FREE },
	{ _T("System.Memory.Virtual.FreePerc"), H_MemoryInfo,    (const TCHAR *)VIRTUAL_FREE_PCT,	DCI_DT_UINT,	DCIDESC_SYSTEM_MEMORY_VIRTUAL_FREE_PCT },
	{ _T("System.Memory.Virtual.Total"),  H_MemoryInfo,      (const TCHAR *)VIRTUAL_TOTAL,	DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_VIRTUAL_TOTAL },
	{ _T("System.Memory.Virtual.Used"),   H_MemoryInfo,      (const TCHAR *)VIRTUAL_USED,	DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_VIRTUAL_USED },
	{ _T("System.Memory.Virtual.UsedPerc"), H_MemoryInfo,    (const TCHAR *)VIRTUAL_USED_PCT,	DCI_DT_UINT,	DCIDESC_SYSTEM_MEMORY_VIRTUAL_USED_PCT },
#endif
	{ _T("System.ProcessCount"),          H_ProcessCount,    _T("S"),			DCI_DT_UINT,	DCIDESC_SYSTEM_PROCESSCOUNT },
	{ _T("System.ThreadCount"),           H_ProcessCount,    _T("T"),			DCI_DT_UINT,	DCIDESC_SYSTEM_THREADCOUNT },
	{ _T("System.Uname"),                 H_Uname,           NULL,				DCI_DT_STRING,	DCIDESC_SYSTEM_UNAME },
	{ _T("System.Uptime"),                H_Uptime,          NULL,				DCI_DT_UINT,	DCIDESC_SYSTEM_UPTIME },

	{ _T("Agent.SourcePackageSupport"),   H_SourcePkgSupport,NULL,				DCI_DT_INT,	DCIDESC_AGENT_SOURCEPACKAGESUPPORT },
};

static NETXMS_SUBAGENT_LIST m_enums[] =
{
	{ _T("Net.ArpCache"),                 H_NetArpCache,     NULL },
	{ _T("Net.InterfaceList"),            H_NetIfList,       NULL },
	{ _T("Net.IP.RoutingTable"),          H_NetRoutingTable, NULL },
	{ _T("System.ProcessList"),           H_ProcessList,     NULL },
};

static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("FreeBSD"),
	NETXMS_VERSION_STRING,
	NULL, // init handler
	NULL, // shutdown handler
	NULL, // command handler
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	sizeof(m_enums) / sizeof(NETXMS_SUBAGENT_LIST),
	m_enums,
	0, NULL,	// tables
	0, NULL,	// actions
	0, NULL	// push parameters
};

//
// Entry point for NetXMS agent
//

DECLARE_SUBAGENT_ENTRY_POINT(FREEBSD)
{
	*ppInfo = &m_info;
	return TRUE;
}


//
// Entry points for server
//

extern "C" BOOL __NxSubAgentGetIfList(StringList *pValue)
{
	return H_NetIfList(_T("Net.InterfaceList"), NULL, pValue) == SYSINFO_RC_SUCCESS;
}

extern "C" BOOL __NxSubAgentGetArpCache(StringList *pValue)
{
	return H_NetArpCache(_T("Net.ArpCache"), NULL, pValue) == SYSINFO_RC_SUCCESS;
}
