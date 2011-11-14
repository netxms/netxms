/*
 ** NetXMS subagent for SunOS/Solaris
 ** Copyright (C) 2004-2011 Victor Kirhenshtein
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
 ** File: main.cpp
 **
 **/

#include "sunos_subagent.h"


//
// Hanlder functions
//

LONG H_CPUCount(const char *pszParam, const char *pArg, char *pValue);
LONG H_CPUUsage(const char *pszParam, const char *pArg, char *pValue);
LONG H_DiskInfo(const char *pszParam, const char *pArg, char *pValue);
LONG H_Hostname(const char *pszParam, const char *pArg, char *pValue);
LONG H_IOStats(const char *pszParam, const char *pArg, char *pValue);
LONG H_IOStatsTotal(const char *pszParam, const char *pArg, char *pValue);
LONG H_KStat(const char *pszParam, const char *pArg, char *pValue);
LONG H_LoadAvg(const char *pszParam, const char *pArg, char *pValue);
LONG H_MemoryInfo(const char *pszParam, const char *pArg, char *pValue);
LONG H_NetIfList(const char *pszParam, const char *pArg, StringList *pValue);
LONG H_NetIfAdminStatus(const char *pszParam, const char *pArg, char *pValue);
LONG H_NetInterfaceLink(const char *pszParam, const char *pArg, char *pValue);
LONG H_NetIfDescription(const char *pszParam, const char *pArg, char *pValue);
LONG H_NetInterfaceStats(const char *pszParam, const char *pArg, char *pValue);
LONG H_ProcessCount(const char *pszParam, const char *pArg, char *pValue);
LONG H_ProcessInfo(const char *pszParam, const char *pArg, char *pValue);
LONG H_ProcessList(const char *pszParam, const char *pArg, StringList *pValue);
LONG H_SysProcCount(const char *pszParam, const char *pArg, char *pValue);
LONG H_Uname(const char *pszParam, const char *pArg, char *pValue);
LONG H_Uptime(const char *pszParam, const char *pArg, char *pValue);


//
// Global variables
//

BOOL g_bShutdown = FALSE;


//
// Static data
//

static THREAD s_cpuStatThread = INVALID_THREAD_HANDLE;
static THREAD s_ioStatThread = INVALID_THREAD_HANDLE;
static MUTEX s_kstatLock = INVALID_MUTEX_HANDLE;


//
// Lock access to kstat API
//

void kstat_lock()
{
	MutexLock(s_kstatLock);
}

void kstat_unlock()
{
	MutexUnlock(s_kstatLock);
}


//
// Detect support for source packages
//

static LONG H_SourcePkg(const char *pszParam, const char *pArg, char *pValue)
{
	ret_int(pValue, 1);
	return SYSINFO_RC_SUCCESS;
}


//
// Initalization callback
//

static BOOL SubAgentInit(Config *config)
{
	s_cpuStatThread = ThreadCreateEx(CPUStatCollector, 0, NULL);
	s_ioStatThread = ThreadCreateEx(IOStatCollector, 0, NULL);
	s_kstatLock = MutexCreate();

	return TRUE;
}


//
// Called by master agent at unload
//

static void SubAgentShutdown()
{
	g_bShutdown = TRUE;
	ThreadJoin(s_cpuStatThread);
	ThreadJoin(s_ioStatThread);
	MutexDestroy(s_kstatLock);
}


//
// Subagent information
//

static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
	{ "Agent.SourcePackageSupport", H_SourcePkg, NULL, DCI_DT_INT, DCIDESC_AGENT_SOURCEPACKAGESUPPORT },
	
	{ "Disk.Avail(*)", H_DiskInfo, (char *)DISK_AVAIL, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
	{ "Disk.AvailPerc(*)", H_DiskInfo, (char *)DISK_AVAIL_PERC, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
	{ "Disk.Free(*)", H_DiskInfo, (char *)DISK_FREE, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
	{ "Disk.FreePerc(*)", H_DiskInfo, (char *)DISK_FREE_PERC, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
	{ "Disk.Total(*)", H_DiskInfo, (char *)DISK_TOTAL, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
	{ "Disk.Used(*)", H_DiskInfo, (char *)DISK_USED, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
	{ "Disk.UsedPerc(*)", H_DiskInfo, (char *)DISK_USED_PERC, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
	
	{ "FileSystem.Avail(*)", H_DiskInfo, (char *)DISK_AVAIL, DCI_DT_UINT64, DCIDESC_FS_AVAIL },
	{ "FileSystem.AvailPerc(*)", H_DiskInfo, (char *)DISK_AVAIL_PERC, DCI_DT_FLOAT, DCIDESC_FS_AVAILPERC },
	{ "FileSystem.Free(*)", H_DiskInfo, (char *)DISK_FREE, DCI_DT_UINT64, DCIDESC_FS_FREE },
	{ "FileSystem.FreePerc(*)", H_DiskInfo, (char *)DISK_FREE_PERC, DCI_DT_FLOAT, DCIDESC_FS_FREEPERC },
	{ "FileSystem.Total(*)", H_DiskInfo, (char *)DISK_TOTAL, DCI_DT_UINT64, DCIDESC_FS_TOTAL },
	{ "FileSystem.Used(*)", H_DiskInfo, (char *)DISK_USED, DCI_DT_UINT64, DCIDESC_FS_USED },
	{ "FileSystem.UsedPerc(*)", H_DiskInfo, (char *)DISK_USED_PERC, DCI_DT_FLOAT, DCIDESC_FS_USEDPERC },
	
	{ "Net.Interface.AdminStatus(*)", H_NetIfAdminStatus, NULL, DCI_DT_INT, DCIDESC_NET_INTERFACE_ADMINSTATUS },
	{ "Net.Interface.BytesIn(*)", H_NetInterfaceStats, "rbytes", DCI_DT_UINT, DCIDESC_NET_INTERFACE_BYTESIN },
	{ "Net.Interface.BytesOut(*)", H_NetInterfaceStats, "obytes", DCI_DT_UINT, DCIDESC_NET_INTERFACE_BYTESOUT },
	{ "Net.Interface.Description(*)", H_NetIfDescription, NULL, DCI_DT_STRING, DCIDESC_NET_INTERFACE_DESCRIPTION },
	{ "Net.Interface.InErrors(*)", H_NetInterfaceStats, "ierrors", DCI_DT_UINT, DCIDESC_NET_INTERFACE_INERRORS },
	{ "Net.Interface.Link(*)", H_NetInterfaceLink, NULL, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
	{ "Net.Interface.OperStatus(*)", H_NetInterfaceLink, NULL, DCI_DT_INT, DCIDESC_NET_INTERFACE_OPERSTATUS },
	{ "Net.Interface.OutErrors(*)", H_NetInterfaceStats, "oerrors", DCI_DT_UINT, DCIDESC_NET_INTERFACE_OUTERRORS },
	{ "Net.Interface.PacketsIn(*)", H_NetInterfaceStats, "ipackets", DCI_DT_UINT, DCIDESC_NET_INTERFACE_PACKETSIN },
	{ "Net.Interface.PacketsOut(*)", H_NetInterfaceStats, "opackets", DCI_DT_UINT, DCIDESC_NET_INTERFACE_PACKETSOUT },
	{ "Net.Interface.Speed(*)", H_NetInterfaceStats, "ifspeed", DCI_DT_UINT, DCIDESC_NET_INTERFACE_SPEED },

	{ "Process.Count(*)", H_ProcessCount, "S", DCI_DT_UINT, DCIDESC_PROCESS_COUNT },
	{ "Process.CountEx(*)", H_ProcessCount, "E", DCI_DT_UINT, DCIDESC_PROCESS_COUNTEX },
	{ "Process.CPUTime(*)", H_ProcessInfo, CAST_TO_POINTER(PROCINFO_CPUTIME, const char *), DCI_DT_UINT64, DCIDESC_PROCESS_CPUTIME },
	{ "Process.KernelTime(*)", H_ProcessInfo, CAST_TO_POINTER(PROCINFO_KTIME, const char *), DCI_DT_UINT64, DCIDESC_PROCESS_KERNELTIME },
	{ "Process.PageFaults(*)", H_ProcessInfo, CAST_TO_POINTER(PROCINFO_PF, const char *), DCI_DT_UINT64, DCIDESC_PROCESS_PAGEFAULTS },
	{ "Process.Syscalls(*)", H_ProcessInfo, CAST_TO_POINTER(PROCINFO_SYSCALLS, const char *), DCI_DT_UINT64, DCIDESC_PROCESS_SYSCALLS },
	{ "Process.Threads(*)", H_ProcessInfo, CAST_TO_POINTER(PROCINFO_THREADS, const char *), DCI_DT_UINT, DCIDESC_PROCESS_THREADS },
	{ "Process.UserTime(*)", H_ProcessInfo,  CAST_TO_POINTER(PROCINFO_UTIME, const char *), DCI_DT_UINT64, DCIDESC_PROCESS_USERTIME },
	{ "Process.VMSize(*)", H_ProcessInfo, CAST_TO_POINTER(PROCINFO_VMSIZE, const char *), DCI_DT_UINT64, DCIDESC_PROCESS_VMSIZE },
	{ "Process.WkSet(*)", H_ProcessInfo, CAST_TO_POINTER(PROCINFO_WKSET, const char *), DCI_DT_UINT64, DCIDESC_PROCESS_WKSET },

	{ "System.CPU.Count", H_CPUCount, NULL, DCI_DT_UINT, DCIDESC_SYSTEM_CPU_COUNT },
	{ "System.CPU.LoadAvg", H_LoadAvg, (char *)0, DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_LOADAVG },
	{ "System.CPU.LoadAvg5", H_LoadAvg, (char *)1, DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_LOADAVG5 },
	{ "System.CPU.LoadAvg15", H_LoadAvg, (char *)2, DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_LOADAVG15 },
	{ "System.CPU.Usage", H_CPUUsage, "T0", DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE },
	{ "System.CPU.Usage5", H_CPUUsage, "T1", DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5 },
	{ "System.CPU.Usage15", H_CPUUsage, "T2", DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15 },
	{ "System.CPU.Usage(*)", H_CPUUsage, "C0", DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_EX },
	{ "System.CPU.Usage5(*)", H_CPUUsage, "C1", DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_EX },
	{ "System.CPU.Usage15(*)", H_CPUUsage, "C2", DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_EX },
	{ "System.Hostname", H_Hostname, NULL, DCI_DT_STRING, DCIDESC_SYSTEM_HOSTNAME },
	{ "System.KStat(*)", H_KStat, NULL, DCI_DT_STRING, "" },
	{ "System.IO.ReadRate", H_IOStatsTotal, (const char *)IOSTAT_NUM_READS, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_READS },
	{ "System.IO.ReadRate(*)", H_IOStats, (const char *)IOSTAT_NUM_READS, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_READS_EX },
	{ "System.IO.WriteRate", H_IOStatsTotal, (const char *)IOSTAT_NUM_WRITES, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_WRITES },
	{ "System.IO.WriteRate(*)", H_IOStats, (const char *)IOSTAT_NUM_WRITES, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_WRITES_EX },
	{ "System.IO.BytesReadRate", H_IOStatsTotal, (const char *)IOSTAT_NUM_RBYTES, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEREADS },
	{ "System.IO.BytesReadRate(*)", H_IOStats, (const char *)IOSTAT_NUM_RBYTES, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEREADS_EX },
	{ "System.IO.BytesWriteRate", H_IOStatsTotal, (const char *)IOSTAT_NUM_WBYTES, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEWRITES },
	{ "System.IO.BytesWriteRate(*)", H_IOStats, (const char *)IOSTAT_NUM_WBYTES, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEWRITES_EX },
	{ "System.IO.DiskQueue", H_IOStatsTotal, (const char *)IOSTAT_QUEUE, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_DISKQUEUE },
	{ "System.IO.DiskQueue(*)", H_IOStats, (const char *)IOSTAT_QUEUE, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_DISKQUEUE_EX },
//        { "System.IO.DiskTime", H_IoStatsTotal, (const char *)IOSTAT_IO_TIME, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_DISKTIME },
//        { "System.IO.DiskTime(*)", H_IoStats, (const char *)IOSTAT_IO_TIME, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_DISKTIME_EX },
	{ "System.Memory.Physical.Free", H_MemoryInfo, (const char *)MEMINFO_PHYSICAL_FREE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE },
	{ "System.Memory.Physical.FreePerc", H_MemoryInfo, (const char *)MEMINFO_PHYSICAL_FREEPCT, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE_PCT },
	{ "System.Memory.Physical.Total", H_MemoryInfo, (const char *)MEMINFO_PHYSICAL_TOTAL, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_TOTAL },
	{ "System.Memory.Physical.Used", H_MemoryInfo, (const char *)MEMINFO_PHYSICAL_USED, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED },
	{ "System.Memory.Physical.UsedPerc", H_MemoryInfo, (const char *)MEMINFO_PHYSICAL_USEDPCT, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED_PCT },
	{ "System.Memory.Swap.Free", H_MemoryInfo, (const char *)MEMINFO_SWAP_FREE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_SWAP_FREE },
	{ "System.Memory.Swap.Total", H_MemoryInfo, (const char *)MEMINFO_SWAP_TOTAL, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_SWAP_TOTAL },
	{ "System.Memory.Swap.Used", H_MemoryInfo, (const char *)MEMINFO_SWAP_USED, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_SWAP_USED },
	{ "System.ProcessCount", H_SysProcCount, NULL, DCI_DT_INT, DCIDESC_SYSTEM_PROCESSCOUNT },
	{ "System.Uname", H_Uname, NULL, DCI_DT_STRING, DCIDESC_SYSTEM_UNAME },
	{ "System.Uptime", H_Uptime, NULL, DCI_DT_UINT, DCIDESC_SYSTEM_UPTIME }
};

static NETXMS_SUBAGENT_LIST m_enums[] =
{
	{ "Net.InterfaceList", H_NetIfList, NULL },
	{ "System.ProcessList", H_ProcessList, NULL }
};

static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("SUNOS"), NETXMS_VERSION_STRING,
	SubAgentInit, // init handler
	SubAgentShutdown, // unload handler
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

DECLARE_SUBAGENT_ENTRY_POINT(SUNOS)
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
