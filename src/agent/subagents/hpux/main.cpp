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

static LONG H_SourcePkg(const char *pszParam, const char *pArg, char *pValue)
{
	ret_int(pValue, 1); // assume that we have sane build env
	return SYSINFO_RC_SUCCESS;
}


//
// Initialization callback
//

static BOOL SubAgentInit(Config *config)
{
	StartCpuUsageCollector();
	StartIOStatCollector();
	return TRUE;
}


//
// Called by master agent at unload
//

static void SubAgentShutdown()
{
	ShutdownCpuUsageCollector();
	ShutdownIOStatCollector();
}


//
// Subagent information
//

static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
	{ "Agent.SourcePackageSupport", H_SourcePkg, NULL, DCI_DT_INT, DCIDESC_AGENT_SOURCEPACKAGESUPPORT },

	{ "Disk.Avail(*)",                H_DiskInfo,        (char *)DISK_AVAIL,
		DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
	{ "Disk.AvailPerc(*)",            H_DiskInfo,        (char *)DISK_AVAIL_PERC,
		DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
	{ "Disk.Free(*)",                 H_DiskInfo,        (char *)DISK_FREE,
		DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
	{ "Disk.FreePerc(*)",             H_DiskInfo,        (char *)DISK_FREE_PERC,
		DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
	{ "Disk.Total(*)",                H_DiskInfo,        (char *)DISK_TOTAL,
		DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
	{ "Disk.Used(*)",                 H_DiskInfo,        (char *)DISK_USED,
		DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
	{ "Disk.UsedPerc(*)",             H_DiskInfo,        (char *)DISK_USED_PERC,
		DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },

	{ "FileSystem.Avail(*)",          H_DiskInfo,        (char *)DISK_AVAIL,
		DCI_DT_UINT64,	DCIDESC_FS_AVAIL },
	{ "FileSystem.AvailPerc(*)",      H_DiskInfo,        (char *)DISK_AVAIL_PERC,
		DCI_DT_FLOAT,	DCIDESC_FS_AVAILPERC },
	{ "FileSystem.Free(*)",           H_DiskInfo,        (char *)DISK_FREE,
		DCI_DT_UINT64,	DCIDESC_FS_FREE },
	{ "FileSystem.FreePerc(*)",       H_DiskInfo,        (char *)DISK_FREE_PERC,
		DCI_DT_FLOAT,	DCIDESC_FS_FREEPERC },
	{ "FileSystem.Total(*)",          H_DiskInfo,        (char *)DISK_TOTAL,
		DCI_DT_UINT64,	DCIDESC_FS_TOTAL },
	{ "FileSystem.Used(*)",           H_DiskInfo,        (char *)DISK_USED,
		DCI_DT_UINT64,	DCIDESC_FS_USED },
	{ "FileSystem.UsedPerc(*)",       H_DiskInfo,        (char *)DISK_USED_PERC,
		DCI_DT_FLOAT,	DCIDESC_FS_USEDPERC },

	{ "Net.Interface.AdminStatus(*)", H_NetIfInfo, (char *)IF_INFO_ADMIN_STATUS,
		DCI_DT_INT,		DCIDESC_NET_INTERFACE_ADMINSTATUS },
	{ "Net.Interface.BytesIn(*)",     H_NetIfInfo, (char *)IF_INFO_BYTES_IN,
		DCI_DT_UINT,	DCIDESC_NET_INTERFACE_BYTESIN },
	{ "Net.Interface.BytesOut(*)",    H_NetIfInfo, (char *)IF_INFO_BYTES_OUT,
		DCI_DT_UINT,	DCIDESC_NET_INTERFACE_BYTESOUT },
	{ "Net.Interface.Description(*)", H_NetIfInfo, (char *)IF_INFO_DESCRIPTION,
		DCI_DT_STRING, DCIDESC_NET_INTERFACE_DESCRIPTION },
	{ "Net.Interface.InErrors(*)",    H_NetIfInfo, (char *)IF_INFO_IN_ERRORS,
		DCI_DT_UINT,	DCIDESC_NET_INTERFACE_INERRORS },
	{ "Net.Interface.Link(*)",        H_NetIfInfo, (char *)IF_INFO_OPER_STATUS,
		DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
	{ "Net.Interface.MTU(*)",         H_NetIfInfo, (char *)IF_INFO_MTU,
		DCI_DT_INT,		DCIDESC_NET_INTERFACE_MTU },
	{ "Net.Interface.OperStatus(*)",  H_NetIfInfo, (char *)IF_INFO_OPER_STATUS,
		DCI_DT_INT,		DCIDESC_NET_INTERFACE_OPERSTATUS },
	{ "Net.Interface.OutErrors(*)",   H_NetIfInfo, (char *)IF_INFO_OUT_ERRORS,
		DCI_DT_UINT,	DCIDESC_NET_INTERFACE_OUTERRORS },
	{ "Net.Interface.PacketsIn(*)",   H_NetIfInfo, (char *)IF_INFO_PACKETS_IN,
		DCI_DT_UINT,	DCIDESC_NET_INTERFACE_PACKETSIN },
	{ "Net.Interface.PacketsOut(*)",  H_NetIfInfo, (char *)IF_INFO_PACKETS_OUT,
		DCI_DT_UINT,	DCIDESC_NET_INTERFACE_PACKETSOUT },
	{ "Net.Interface.Speed(*)",       H_NetIfInfo, (char *)IF_INFO_SPEED,
		DCI_DT_INT,		DCIDESC_NET_INTERFACE_SPEED },
	{ "Net.IP.Forwarding",            H_NetIpForwarding, (char *)4,
		DCI_DT_INT,		DCIDESC_NET_IP_FORWARDING },
	{ "Net.IP6.Forwarding",           H_NetIpForwarding, (char *)6,
		DCI_DT_INT,		DCIDESC_NET_IP6_FORWARDING },

	{ "Process.Count(*)", H_ProcessCount, NULL, DCI_DT_UINT, DCIDESC_PROCESS_COUNT },
   { "Process.IO.ReadOp(*)", H_ProcessInfo, (char *)PROCINFO_IO_READ_OP, DCI_DT_UINT64, DCIDESC_PROCESS_IO_READOP },
   { "Process.IO.WriteOp(*)", H_ProcessInfo, (char *)PROCINFO_IO_WRITE_OP, DCI_DT_UINT64, DCIDESC_PROCESS_IO_WRITEOP },
   { "Process.KernelTime(*)", H_ProcessInfo, (char *)PROCINFO_KTIME, DCI_DT_UINT64, DCIDESC_PROCESS_KERNELTIME },
   { "Process.PageFaults(*)", H_ProcessInfo, (char *)PROCINFO_PF, DCI_DT_UINT64, DCIDESC_PROCESS_PAGEFAULTS },
   { "Process.Threads(*)", H_ProcessInfo, (char *)PROCINFO_THREADS, DCI_DT_UINT64, DCIDESC_PROCESS_THREADS },
   { "Process.UserTime(*)", H_ProcessInfo, (char *)PROCINFO_UTIME, DCI_DT_UINT64, DCIDESC_PROCESS_USERTIME },
   { "Process.VMSize(*)", H_ProcessInfo, (char *)PROCINFO_VMSIZE, DCI_DT_UINT64, DCIDESC_PROCESS_VMSIZE },
   { "Process.WkSet(*)", H_ProcessInfo, (char *)PROCINFO_WKSET, DCI_DT_UINT64, DCIDESC_PROCESS_WKSET },

	{ "System.ConnectedUsers", H_ConnectedUsers, NULL, DCI_DT_UINT, DCIDESC_SYSTEM_CONNECTEDUSERS },
	{ "System.CPU.LoadAvg", H_CpuLoad, NULL, DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_LOADAVG },
	{ "System.CPU.LoadAvg5", H_CpuLoad, NULL, DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_LOADAVG5 },
	{ "System.CPU.LoadAvg15", H_CpuLoad, NULL, DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_LOADAVG15 },
	{ "System.CPU.Usage", H_CpuUsage, (char *)0, DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE },
	{ "System.CPU.Usage5", H_CpuUsage, (char *)5, DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5 },
	{ "System.CPU.Usage15", H_CpuUsage, (char *)15, DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15 },
	{ "System.Hostname", H_Hostname, NULL, DCI_DT_STRING, DCIDESC_SYSTEM_HOSTNAME },
	{ "System.IO.BytesReadRate", H_IOStatsTotal, (const char *)IOSTAT_NUM_RBYTES, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEREADS },
	{ "System.IO.BytesReadRate(*)", H_IOStats, (const char *)IOSTAT_NUM_RBYTES, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEREADS_EX },
	{ "System.IO.BytesWriteRate", H_IOStatsTotal, (const char *)IOSTAT_NUM_WBYTES, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEWRITES },
	{ "System.IO.BytesWriteRate(*)", H_IOStats, (const char *)IOSTAT_NUM_WBYTES, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEWRITES_EX },
	{ "System.IO.DiskQueue", H_IOStatsTotal, (const char *)IOSTAT_QUEUE, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_DISKQUEUE },
	{ "System.IO.DiskQueue(*)", H_IOStats, (const char *)IOSTAT_QUEUE, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_DISKQUEUE_EX },
	{ "System.IO.OpenFiles", H_OpenFiles, NULL, DCI_DT_INT, DCIDESC_SYSTEM_IO_OPENFILES },
	{ "System.IO.ReadRate", H_IOStatsTotal, (const char *)IOSTAT_NUM_READS, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_READS },
	{ "System.IO.ReadRate(*)", H_IOStats, (const char *)IOSTAT_NUM_READS, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_READS_EX },
	{ "System.IO.TransferRate", H_IOStatsTotal, (const char *)IOSTAT_NUM_XFERS, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_XFERS },
	{ "System.IO.TransferRate(*)", H_IOStats, (const char *)IOSTAT_NUM_XFERS, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_XFERS_EX },
	{ "System.IO.WaitTime", H_IOStatsTotal, (const char *)IOSTAT_WAIT_TIME, DCI_DT_INT, DCIDESC_SYSTEM_IO_WAITTIME },
	{ "System.IO.WaitTime(*)", H_IOStats, (const char *)IOSTAT_WAIT_TIME, DCI_DT_INT, DCIDESC_SYSTEM_IO_WAITTIME_EX },
	{ "System.IO.WriteRate", H_IOStatsTotal, (const char *)IOSTAT_NUM_WRITES, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_WRITES },
	{ "System.IO.WriteRate(*)", H_IOStats, (const char *)IOSTAT_NUM_WRITES, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_WRITES_EX },

	{ "System.Memory.Physical.Free",  H_MemoryInfo,      (char *)PHYSICAL_FREE,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE },
	{ "System.Memory.Physical.FreePerc", H_MemoryInfo,   (char *)PHYSICAL_FREE_PCT,
		DCI_DT_UINT,	DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE_PCT },
	{ "System.Memory.Physical.Total", H_MemoryInfo,      (char *)PHYSICAL_TOTAL,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_PHYSICAL_TOTAL },
	{ "System.Memory.Physical.Used",  H_MemoryInfo,      (char *)PHYSICAL_USED,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED },
	{ "System.Memory.Physical.UsedPerc", H_MemoryInfo,   (char *)PHYSICAL_USED_PCT,
		DCI_DT_UINT,	DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED_PCT },
	{ "System.Memory.Swap.Free",      H_MemoryInfo,      (char *)SWAP_FREE,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_SWAP_FREE },
	{ "System.Memory.Swap.FreePerc",  H_MemoryInfo,      (char *)SWAP_FREE_PCT,
		DCI_DT_UINT,	DCIDESC_SYSTEM_MEMORY_SWAP_FREE_PCT },
	{ "System.Memory.Swap.Total",     H_MemoryInfo,      (char *)SWAP_TOTAL,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_SWAP_TOTAL },
	{ "System.Memory.Swap.Used",      H_MemoryInfo,      (char *)SWAP_USED,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_SWAP_USED },
	{ "System.Memory.Swap.UsedPerc",      H_MemoryInfo,  (char *)SWAP_USED_PCT,
		DCI_DT_UINT,	DCIDESC_SYSTEM_MEMORY_SWAP_USED_PCT },
	{ "System.Memory.Virtual.Free",   H_MemoryInfo,      (char *)VIRTUAL_FREE,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_VIRTUAL_FREE },
	{ "System.Memory.Virtual.FreePerc", H_MemoryInfo,    (char *)VIRTUAL_FREE_PCT,
		DCI_DT_UINT,	DCIDESC_SYSTEM_MEMORY_VIRTUAL_FREE_PCT },
	{ "System.Memory.Virtual.Total",  H_MemoryInfo,      (char *)VIRTUAL_TOTAL,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_VIRTUAL_TOTAL },
	{ "System.Memory.Virtual.Used",   H_MemoryInfo,      (char *)VIRTUAL_USED,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_VIRTUAL_USED },
	{ "System.Memory.Virtual.UsedPerc", H_MemoryInfo,    (char *)VIRTUAL_USED_PCT,
		DCI_DT_UINT,	DCIDESC_SYSTEM_MEMORY_VIRTUAL_USED_PCT },
		
	{ "System.ProcessCount", H_SysProcessCount, NULL, DCI_DT_UINT, DCIDESC_SYSTEM_PROCESSCOUNT },
	{ "System.Uname", H_Uname, NULL, DCI_DT_STRING, DCIDESC_SYSTEM_UNAME },
	{ "System.Uptime", H_Uptime, NULL, DCI_DT_UINT, DCIDESC_SYSTEM_UPTIME }
};
static NETXMS_SUBAGENT_LIST m_enums[] =
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
	sizeof(m_enums) / sizeof(NETXMS_SUBAGENT_LIST),
	m_enums,
	0, NULL,	// tables
   0, NULL,	// actions
	0, NULL	// push parameters
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

extern "C" BOOL __NxSubAgentGetIfList(StringList *pValue)
{
	return H_NetIfList("Net.InterfaceList", NULL, pValue) == SYSINFO_RC_SUCCESS;
}

extern "C" BOOL __NxSubAgentGetArpCache(StringList *pValue)
{
	return H_NetArpCache("Net.ArpCache", NULL, pValue) == SYSINFO_RC_SUCCESS;
}
