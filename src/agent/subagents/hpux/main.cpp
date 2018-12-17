/*
** NetXMS subagent for HP-UX
** Copyright (C) 2006-2015 Raden Solutions
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

/**
 * Detect support for source packages
 */
static LONG H_SourcePkg(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
	ret_int(pValue, 1); // assume that we have sane build env
	return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for shutdown/restart actions
 */
static LONG H_Shutdown(const TCHAR *pszAction, const StringList *pArgList, const TCHAR *pData, AbstractCommSession *session)
{
   chdir("/");
   char cmd[128];
   snprintf(cmd, 128, "/sbin/shutdown %s -y now", (*pData == _T('R')) ? "-r" : "-h");
   return (system(cmd) >= 0) ? ERR_SUCCESS : ERR_INTERNAL_ERROR;
}

/**
 * Initialization callback
 */
static bool SubAgentInit(Config *config)
{
	StartCpuUsageCollector();
	StartIOStatCollector();
   InitProc();
	return true;
}

/**
 * Called by master agent at unload
 */
static void SubAgentShutdown()
{
   ShutdownProc();
	ShutdownCpuUsageCollector();
	ShutdownIOStatCollector();
}

/**
 * Parameters provided by subagent
 */
static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
	{ _T("Agent.SourcePackageSupport"), H_SourcePkg, NULL, DCI_DT_INT, DCIDESC_AGENT_SOURCEPACKAGESUPPORT },

	{ _T("Disk.Avail(*)"),                H_DiskInfo,        (const TCHAR *)DISK_AVAIL,
		DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
	{ _T("Disk.AvailPerc(*)"),            H_DiskInfo,        (const TCHAR *)DISK_AVAIL_PERC,
		DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
	{ _T("Disk.Free(*)"),                 H_DiskInfo,        (const TCHAR *)DISK_FREE,
		DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
	{ _T("Disk.FreePerc(*)"),             H_DiskInfo,        (const TCHAR *)DISK_FREE_PERC,
		DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
	{ _T("Disk.Total(*)"),                H_DiskInfo,        (const TCHAR *)DISK_TOTAL,
		DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
	{ _T("Disk.Used(*)"),                 H_DiskInfo,        (const TCHAR *)DISK_USED,
		DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
	{ _T("Disk.UsedPerc(*)"),             H_DiskInfo,        (const TCHAR *)DISK_USED_PERC,
		DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },

	{ _T("FileSystem.Avail(*)"), H_DiskInfo, (const TCHAR *)DISK_AVAIL, DCI_DT_UINT64, DCIDESC_FS_AVAIL },
	{ _T("FileSystem.AvailPerc(*)"), H_DiskInfo, (const TCHAR *)DISK_AVAIL_PERC, DCI_DT_FLOAT, DCIDESC_FS_AVAILPERC },
	{ _T("FileSystem.Free(*)"), H_DiskInfo, (const TCHAR *)DISK_FREE, DCI_DT_UINT64, DCIDESC_FS_FREE },
	{ _T("FileSystem.FreePerc(*)"), H_DiskInfo, (const TCHAR *)DISK_FREE_PERC, DCI_DT_FLOAT, DCIDESC_FS_FREEPERC },
	{ _T("FileSystem.Total(*)"), H_DiskInfo, (const TCHAR *)DISK_TOTAL, DCI_DT_UINT64, DCIDESC_FS_TOTAL },
	{ _T("FileSystem.Used(*)"), H_DiskInfo, (const TCHAR *)DISK_USED, DCI_DT_UINT64, DCIDESC_FS_USED },
	{ _T("FileSystem.UsedPerc(*)"), H_DiskInfo, (const TCHAR *)DISK_USED_PERC, DCI_DT_FLOAT, DCIDESC_FS_USEDPERC },

	{ _T("Net.Interface.AdminStatus(*)"), H_NetIfInfo, (const TCHAR *)IF_INFO_ADMIN_STATUS, DCI_DT_INT, DCIDESC_NET_INTERFACE_ADMINSTATUS },
	{ _T("Net.Interface.BytesIn(*)"), H_NetIfInfo, (const TCHAR *)IF_INFO_BYTES_IN, DCI_DT_UINT, DCIDESC_NET_INTERFACE_BYTESIN },
	{ _T("Net.Interface.BytesOut(*)"), H_NetIfInfo, (const TCHAR *)IF_INFO_BYTES_OUT, DCI_DT_UINT, DCIDESC_NET_INTERFACE_BYTESOUT },
	{ _T("Net.Interface.Description(*)"), H_NetIfInfo, (const TCHAR *)IF_INFO_DESCRIPTION, DCI_DT_STRING, DCIDESC_NET_INTERFACE_DESCRIPTION },
	{ _T("Net.Interface.InErrors(*)"), H_NetIfInfo, (const TCHAR *)IF_INFO_IN_ERRORS, DCI_DT_UINT, DCIDESC_NET_INTERFACE_INERRORS },
	{ _T("Net.Interface.Link(*)"), H_NetIfInfo, (const TCHAR *)IF_INFO_OPER_STATUS, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
	{ _T("Net.Interface.MTU(*)"), H_NetIfInfo, (const TCHAR *)IF_INFO_MTU, DCI_DT_INT, DCIDESC_NET_INTERFACE_MTU },
	{ _T("Net.Interface.OperStatus(*)"), H_NetIfInfo, (const TCHAR *)IF_INFO_OPER_STATUS, DCI_DT_INT, DCIDESC_NET_INTERFACE_OPERSTATUS },
	{ _T("Net.Interface.OutErrors(*)"), H_NetIfInfo, (const TCHAR *)IF_INFO_OUT_ERRORS, DCI_DT_UINT, DCIDESC_NET_INTERFACE_OUTERRORS },
	{ _T("Net.Interface.PacketsIn(*)"), H_NetIfInfo, (const TCHAR *)IF_INFO_PACKETS_IN, DCI_DT_UINT, DCIDESC_NET_INTERFACE_PACKETSIN },
	{ _T("Net.Interface.PacketsOut(*)"), H_NetIfInfo, (const TCHAR *)IF_INFO_PACKETS_OUT, DCI_DT_UINT, DCIDESC_NET_INTERFACE_PACKETSOUT },
	{ _T("Net.Interface.Speed(*)"), H_NetIfInfo, (const TCHAR *)IF_INFO_SPEED, DCI_DT_INT, DCIDESC_NET_INTERFACE_SPEED },
	
	{ _T("Net.IP.Forwarding"), H_NetIpForwarding, (const TCHAR *)4, DCI_DT_INT, DCIDESC_NET_IP_FORWARDING },
	{ _T("Net.IP6.Forwarding"), H_NetIpForwarding, (const TCHAR *)6, DCI_DT_INT, DCIDESC_NET_IP6_FORWARDING },

	{ _T("Process.Count(*)"), H_ProcessCount, NULL, DCI_DT_UINT, DCIDESC_PROCESS_COUNT },
	{ _T("Process.CPUTime(*)"), H_ProcessInfo, (const TCHAR *)PROCINFO_CPUTIME, DCI_DT_UINT64, DCIDESC_PROCESS_CPUTIME },
	{ _T("Process.IO.ReadOp(*)"), H_ProcessInfo, (const TCHAR *)PROCINFO_IO_READ_OP, DCI_DT_UINT64, DCIDESC_PROCESS_IO_READOP },
	{ _T("Process.IO.WriteOp(*)"), H_ProcessInfo, (const TCHAR *)PROCINFO_IO_WRITE_OP, DCI_DT_UINT64, DCIDESC_PROCESS_IO_WRITEOP },
	{ _T("Process.KernelTime(*)"), H_ProcessInfo, (const TCHAR *)PROCINFO_KTIME, DCI_DT_UINT64, DCIDESC_PROCESS_KERNELTIME },
	{ _T("Process.PageFaults(*)"), H_ProcessInfo, (const TCHAR *)PROCINFO_PF, DCI_DT_UINT64, DCIDESC_PROCESS_PAGEFAULTS },
	{ _T("Process.Threads(*)"), H_ProcessInfo, (const TCHAR *)PROCINFO_THREADS, DCI_DT_UINT64, DCIDESC_PROCESS_THREADS },
	{ _T("Process.UserTime(*)"), H_ProcessInfo, (const TCHAR *)PROCINFO_UTIME, DCI_DT_UINT64, DCIDESC_PROCESS_USERTIME },
	{ _T("Process.VMSize(*)"), H_ProcessInfo, (const TCHAR *)PROCINFO_VMSIZE, DCI_DT_UINT64, DCIDESC_PROCESS_VMSIZE },
	{ _T("Process.WkSet(*)"), H_ProcessInfo, (const TCHAR *)PROCINFO_WKSET, DCI_DT_UINT64, DCIDESC_PROCESS_WKSET },

	{ _T("System.ConnectedUsers"), H_ConnectedUsers, NULL, DCI_DT_UINT, DCIDESC_SYSTEM_CONNECTEDUSERS },

	{ _T("System.CPU.LoadAvg"), H_CpuLoad, NULL, DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_LOADAVG },
	{ _T("System.CPU.LoadAvg5"), H_CpuLoad, NULL, DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_LOADAVG5 },
	{ _T("System.CPU.LoadAvg15"), H_CpuLoad, NULL, DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_LOADAVG15 },
	{ _T("System.CPU.Usage"), H_CpuUsage, (const TCHAR *)0, DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE },
	{ _T("System.CPU.Usage5"), H_CpuUsage, (const TCHAR *)5, DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5 },
	{ _T("System.CPU.Usage15"), H_CpuUsage, (const TCHAR *)15, DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15 },

	{ _T("System.IO.BytesReadRate"), H_IOStatsTotal, (const TCHAR *)IOSTAT_NUM_RBYTES, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEREADS },
	{ _T("System.IO.BytesReadRate(*)"), H_IOStats, (const TCHAR *)IOSTAT_NUM_RBYTES, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEREADS_EX },
	{ _T("System.IO.BytesWriteRate"), H_IOStatsTotal, (const TCHAR *)IOSTAT_NUM_WBYTES, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEWRITES },
	{ _T("System.IO.BytesWriteRate(*)"), H_IOStats, (const TCHAR *)IOSTAT_NUM_WBYTES, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEWRITES_EX },
	{ _T("System.IO.DiskQueue"), H_IOStatsTotal, (const TCHAR *)IOSTAT_QUEUE, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_DISKQUEUE },
	{ _T("System.IO.DiskQueue(*)"), H_IOStats, (const TCHAR *)IOSTAT_QUEUE, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_DISKQUEUE_EX },
	{ _T("System.IO.OpenFiles"), H_OpenFiles, NULL, DCI_DT_INT, DCIDESC_SYSTEM_IO_OPENFILES },
	{ _T("System.IO.ReadRate"), H_IOStatsTotal, (const TCHAR *)IOSTAT_NUM_READS, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_READS },
	{ _T("System.IO.ReadRate(*)"), H_IOStats, (const TCHAR *)IOSTAT_NUM_READS, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_READS_EX },
	{ _T("System.IO.TransferRate"), H_IOStatsTotal, (const TCHAR *)IOSTAT_NUM_XFERS, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_XFERS },
	{ _T("System.IO.TransferRate(*)"), H_IOStats, (const TCHAR *)IOSTAT_NUM_XFERS, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_XFERS_EX },
	{ _T("System.IO.WaitTime"), H_IOStatsTotal, (const TCHAR *)IOSTAT_WAIT_TIME, DCI_DT_INT, DCIDESC_SYSTEM_IO_WAITTIME },
	{ _T("System.IO.WaitTime(*)"), H_IOStats, (const TCHAR *)IOSTAT_WAIT_TIME, DCI_DT_INT, DCIDESC_SYSTEM_IO_WAITTIME_EX },
	{ _T("System.IO.WriteRate"), H_IOStatsTotal, (const TCHAR *)IOSTAT_NUM_WRITES, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_WRITES },
	{ _T("System.IO.WriteRate(*)"), H_IOStats, (const TCHAR *)IOSTAT_NUM_WRITES, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_WRITES_EX },

	{ _T("System.Memory.Physical.Free"), H_MemoryInfo, (const TCHAR *)PHYSICAL_FREE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE },
	{ _T("System.Memory.Physical.FreePerc"), H_MemoryInfo, (const TCHAR *)PHYSICAL_FREE_PCT, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE_PCT },
	{ _T("System.Memory.Physical.Total"), H_MemoryInfo, (const TCHAR *)PHYSICAL_TOTAL, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_TOTAL },
	{ _T("System.Memory.Physical.Used"),  H_MemoryInfo, (const TCHAR *)PHYSICAL_USED, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED },
	{ _T("System.Memory.Physical.UsedPerc"), H_MemoryInfo, (const TCHAR *)PHYSICAL_USED_PCT, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED_PCT },
	{ _T("System.Memory.Swap.Free"), H_MemoryInfo, (const TCHAR *)SWAP_FREE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_SWAP_FREE },
	{ _T("System.Memory.Swap.FreePerc"), H_MemoryInfo, (const TCHAR *)SWAP_FREE_PCT, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_SWAP_FREE_PCT },
	{ _T("System.Memory.Swap.Total"), H_MemoryInfo, (const TCHAR *)SWAP_TOTAL, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_SWAP_TOTAL },
	{ _T("System.Memory.Swap.Used"), H_MemoryInfo, (const TCHAR *)SWAP_USED, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_SWAP_USED },
	{ _T("System.Memory.Swap.UsedPerc"), H_MemoryInfo, (const TCHAR *)SWAP_USED_PCT, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_SWAP_USED_PCT },
	{ _T("System.Memory.Virtual.Free"), H_MemoryInfo, (const TCHAR *)VIRTUAL_FREE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_VIRTUAL_FREE },
	{ _T("System.Memory.Virtual.FreePerc"), H_MemoryInfo, (const TCHAR *)VIRTUAL_FREE_PCT, DCI_DT_FLOAT,	DCIDESC_SYSTEM_MEMORY_VIRTUAL_FREE_PCT },
   { _T("System.Memory.Virtual.Total"), H_MemoryInfo, (const TCHAR *)VIRTUAL_TOTAL, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_VIRTUAL_TOTAL },
	{ _T("System.Memory.Virtual.Used"), H_MemoryInfo, (const TCHAR *)VIRTUAL_USED, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_VIRTUAL_USED },
	{ _T("System.Memory.Virtual.UsedPerc"), H_MemoryInfo, (const TCHAR *)VIRTUAL_USED_PCT, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_VIRTUAL_USED_PCT },

   { _T("System.MsgQueue.BytesMax(*)"), H_SysMsgQueue, _T("B"), DCI_DT_UINT64, DCIDESC_SYSTEM_MSGQUEUE_BYTES_MAX },
   { _T("System.MsgQueue.ChangeTime(*)"), H_SysMsgQueue, _T("c"), DCI_DT_UINT64, DCIDESC_SYSTEM_MSGQUEUE_CHANGE_TIME },
   { _T("System.MsgQueue.Messages(*)"), H_SysMsgQueue, _T("m"), DCI_DT_UINT, DCIDESC_SYSTEM_MSGQUEUE_MESSAGES },
   { _T("System.MsgQueue.RecvTime(*)"), H_SysMsgQueue, _T("r"), DCI_DT_UINT64, DCIDESC_SYSTEM_MSGQUEUE_RECV_TIME },
   { _T("System.MsgQueue.SendTime(*)"), H_SysMsgQueue, _T("s"), DCI_DT_UINT64, DCIDESC_SYSTEM_MSGQUEUE_SEND_TIME },
		
	{ _T("System.ProcessCount"), H_SysProcessCount, NULL, DCI_DT_UINT, DCIDESC_SYSTEM_PROCESSCOUNT },
	{ _T("System.ThreadCount"), H_SysThreadCount, NULL, DCI_DT_INT, DCIDESC_SYSTEM_THREADCOUNT },
	{ _T("System.Uname"), H_Uname, NULL, DCI_DT_STRING, DCIDESC_SYSTEM_UNAME },
	{ _T("System.Uptime"), H_Uptime, NULL, DCI_DT_UINT, DCIDESC_SYSTEM_UPTIME }
};

/**
 * Subagent's lists
 */
static NETXMS_SUBAGENT_LIST m_lists[] =
{
   { _T("FileSystem.MountPoints"), H_MountPoints, NULL },
   { _T("Net.ArpCache"), H_NetArpCache, NULL },
   { _T("Net.IP.RoutingTable"), H_NetRoutingTable, NULL },
   { _T("Net.InterfaceList"), H_NetIfList, NULL },
   { _T("Net.InterfaceNames"), H_NetIfNames, NULL },
   { _T("System.ProcessList"), H_ProcessList, NULL },
};

/**
 * Subagent's tables
 */
static NETXMS_SUBAGENT_TABLE m_tables[] =
{
   { _T("FileSystem.Volumes"), H_FileSystems, NULL, _T("MOUNTPOINT"), DCTDESC_FILESYSTEM_VOLUMES }
};

/**
 * Subagent's actions
 */
static NETXMS_SUBAGENT_ACTION m_actions[] =
{
   { _T("System.Restart"), H_Shutdown, _T("R"), _T("Restart system") },
   { _T("System.Shutdown"), H_Shutdown, _T("S"), _T("Shutdown system") }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("HP-UX"), NETXMS_VERSION_STRING,
	SubAgentInit, SubAgentShutdown, NULL, NULL,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	sizeof(m_lists) / sizeof(NETXMS_SUBAGENT_LIST),
	m_lists,
   sizeof(m_tables) / sizeof(NETXMS_SUBAGENT_TABLE),
   m_tables,
   sizeof(m_actions) / sizeof(NETXMS_SUBAGENT_ACTION),
   m_actions,
	0, NULL	// push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(HPUX)
{
	*ppInfo = &m_info;
	return true;
}

/**
 * Entry point for server - interface list
 */
extern "C" BOOL __EXPORT __NxSubAgentGetIfList(StringList *pValue)
{
	return H_NetIfList(_T("Net.InterfaceList"), NULL, pValue, NULL) == SYSINFO_RC_SUCCESS;
}

/**
 * Entry point for server - ARP cache
 */
extern "C" BOOL __EXPORT __NxSubAgentGetArpCache(StringList *pValue)
{
	return H_NetArpCache(_T("Net.ArpCache"), NULL, pValue, NULL) == SYSINFO_RC_SUCCESS;
}
