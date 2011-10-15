/*
** NetXMS subagent for AIX
** Copyright (C) 2005-2011 Victor Kirhenshtein
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

#include "aix_subagent.h"


//
// Hanlder functions
//

LONG H_CpuCount(const char *pszParam, const char *pArg, char *pValue);
LONG H_CpuUsage(const char *pszParam, const char *pArg, char *pValue);
LONG H_CpuUsageEx(const char *pszParam, const char *pArg, char *pValue);
LONG H_DiskInfo(const char *pszParam, const char *pArg, char *pValue);
LONG H_Hostname(const char *pszParam, const char *pArg, char *pValue);
LONG H_LoadAvg(const char *pszParam, const char *pArg, char *pValue);
LONG H_MemoryInfo(const char *pszParam, const char *pArg, char *pValue);
LONG H_VirtualMemoryInfo(const char *pszParam, const char *pArg, char *pValue);
LONG H_NetIfList(const char *pszParam, const char *pArg, StringList *pValue);
LONG H_NetIfAdminStatus(const char *pszParam, const char *pArg, char *pValue);
LONG H_NetIfDescription(const char *pszParam, const char *pArg, char *pValue);
LONG H_NetInterfaceStats(const char *pszParam, const char *pArg, char *pValue);
LONG H_ProcessCount(const char *pszParam, const char *pArg, char *pValue);
LONG H_ProcessInfo(const char *pszParam, const char *pArg, char *pValue);
LONG H_ProcessList(const char *pszParam, const char *pArg, StringList *pValue);
LONG H_SysProcessCount(const char *pszParam, const char *pArg, char *pValue);
LONG H_SysThreadCount(const char *pszParam, const char *pArg, char *pValue);
LONG H_Uname(const char *pszParam, const char *pArg, char *pValue);
LONG H_Uptime(const char *pszParam, const char *pArg, char *pValue);


//
// Global variables
//

BOOL g_bShutdown = FALSE;


//
// Static data
//

static THREAD m_hCPUStatThread = INVALID_THREAD_HANDLE;


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
	StartCpuUsageCollector();
	return TRUE;
}


//
// Called by master agent at unload
//

static void SubAgentShutdown()
{
	g_bShutdown = TRUE;
	ShutdownCpuUsageCollector();
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

/*
   { "Net.Interface.AdminStatus(*)", H_NetIfAdminStatus, NULL, DCI_DT_INT, "Administrative status of interface {instance}" },
   { "Net.Interface.BytesIn(*)", H_NetInterfaceStats, "rbytes", DCI_DT_UINT, "Number of input bytes on interface {instance}" },
   { "Net.Interface.BytesOut(*)", H_NetInterfaceStats, "obytes", DCI_DT_UINT, "Number of output bytes on interface {instance}" },
   { "Net.Interface.Description(*)", H_NetIfDescription, NULL, DCI_DT_STRING, "" },
   { "Net.Interface.InErrors(*)", H_NetInterfaceStats, "ierrors", DCI_DT_UINT, "Number of input errors on interface {instance}" },
   { "Net.Interface.Link(*)", H_NetInterfaceStats, "link_up", DCI_DT_INT, "Link status for interface {instance}" },
   { "Net.Interface.OutErrors(*)", H_NetInterfaceStats, "oerrors", DCI_DT_UINT, "Number of output errors on interface {instance}" },
   { "Net.Interface.PacketsIn(*)", H_NetInterfaceStats, "ipackets", DCI_DT_UINT, "Number of input packets on interface {instance}" },
   { "Net.Interface.PacketsOut(*)", H_NetInterfaceStats, "opackets", DCI_DT_UINT, "Number of output packets on interface {instance}" },
   { "Net.Interface.Speed(*)", H_NetInterfaceStats, "ifspeed", DCI_DT_UINT, "Speed of interface {instance}" },
*/

   { "Process.Count(*)", H_ProcessCount, NULL, DCI_DT_UINT, DCIDESC_PROCESS_COUNT },
   { "Process.CPUTime(*)", H_ProcessInfo, (char *)PROCINFO_CPUTIME, DCI_DT_UINT64, DCIDESC_PROCESS_CPUTIME },
   { "Process.IO.ReadOp(*)", H_ProcessInfo, (char *)PROCINFO_IO_READ_OP, DCI_DT_UINT64, DCIDESC_PROCESS_IO_READOP },
   { "Process.IO.WriteOp(*)", H_ProcessInfo, (char *)PROCINFO_IO_WRITE_OP, DCI_DT_UINT64, DCIDESC_PROCESS_IO_WRITEOP },
   { "Process.KernelTime(*)", H_ProcessInfo, (char *)PROCINFO_KTIME, DCI_DT_UINT64, DCIDESC_PROCESS_KERNELTIME },
   { "Process.PageFaults(*)", H_ProcessInfo, (char *)PROCINFO_PF, DCI_DT_UINT64, DCIDESC_PROCESS_PAGEFAULTS },
   { "Process.Threads(*)", H_ProcessInfo, (char *)PROCINFO_THREADS, DCI_DT_UINT64, DCIDESC_PROCESS_THREADS },
   { "Process.UserTime(*)", H_ProcessInfo, (char *)PROCINFO_UTIME, DCI_DT_UINT64, DCIDESC_PROCESS_USERTIME },
   { "Process.VMSize(*)", H_ProcessInfo, (char *)PROCINFO_VMSIZE, DCI_DT_UINT64, DCIDESC_PROCESS_VMSIZE },
   { "System.CPU.Count", H_CpuCount, NULL, DCI_DT_UINT, DCIDESC_SYSTEM_CPU_COUNT },
   { "System.CPU.LoadAvg", H_LoadAvg, "0", DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_LOADAVG },
   { "System.CPU.LoadAvg5", H_LoadAvg, "1", DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_LOADAVG5 },
   { "System.CPU.LoadAvg15", H_LoadAvg, "2", DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_LOADAVG15 },
   
	{ "System.CPU.Usage", H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_OVERAL), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE },
	{ "System.CPU.Usage5", H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_OVERAL), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5 },
	{ "System.CPU.Usage15", H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_OVERAL), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15 },
	{ "System.CPU.Usage(*)", H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_OVERAL), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_EX },
	{ "System.CPU.Usage5(*)", H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_OVERAL), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_EX },
	{ "System.CPU.Usage15(*)", H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_OVERAL), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_EX },

	{ "System.CPU.Usage.Idle", H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_IDLE), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_IDLE },
	{ "System.CPU.Usage5.Idle", H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_IDLE), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_IDLE },
	{ "System.CPU.Usage15.Idle", H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_IDLE), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_IDLE },
	{ "System.CPU.Usage.Idle(*)", H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_IDLE), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_IDLE_EX },
	{ "System.CPU.Usage5.Idle5(*)", H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_IDLE), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_IDLE_EX },
	{ "System.CPU.Usage15.Idle(*)", H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_IDLE), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_IDLE_EX },

	{ "System.CPU.Usage.IoWait", H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_IOWAIT), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_IOWAIT },
	{ "System.CPU.Usage5.IoWait", H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_IOWAIT), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_IOWAIT },
	{ "System.CPU.Usage15.IoWait", H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_IOWAIT), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_IOWAIT },
	{ "System.CPU.Usage.IoWait(*)", H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_IOWAIT), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_IOWAIT_EX },
	{ "System.CPU.Usage5.IoWait(*)", H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_IOWAIT), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_IOWAIT_EX },
	{ "System.CPU.Usage15.IoWait(*)", H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_IOWAIT), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_IOWAIT_EX },

	{ "System.CPU.Usage.System", H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_SYSTEM), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_SYSTEM },
	{ "System.CPU.Usage5.System", H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_SYSTEM), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_SYSTEM },
	{ "System.CPU.Usage15.System", H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_SYSTEM), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_SYSTEM },
	{ "System.CPU.Usage.System(*)", H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_SYSTEM), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_SYSTEM_EX },
	{ "System.CPU.Usage5.System(*)", H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_SYSTEM), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_SYSTEM_EX },
	{ "System.CPU.Usage15.System(*)", H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_SYSTEM), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_SYSTEM_EX },

	{ "System.CPU.Usage.User", H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_USER), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_USER },
	{ "System.CPU.Usage5.User", H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_USER), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_USER },
	{ "System.CPU.Usage15.User", H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_USER), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_USER },
	{ "System.CPU.Usage.User(*)", H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_USER), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_USER_EX },
	{ "System.CPU.Usage5.User(*)", H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_USER), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_USER_EX },
	{ "System.CPU.Usage15.User(*)", H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_USER), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_USER_EX },

   { "System.Hostname", H_Hostname, NULL, DCI_DT_STRING, DCIDESC_SYSTEM_HOSTNAME },

   { "System.Memory.Physical.Free", H_MemoryInfo, (char *)MEMINFO_PHYSICAL_FREE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE },
   { "System.Memory.Physical.FreePerc", H_MemoryInfo, (char *)MEMINFO_PHYSICAL_FREE_PERC, DCI_DT_UINT, DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE_PCT },
   { "System.Memory.Physical.Total", H_MemoryInfo, (char *)MEMINFO_PHYSICAL_TOTAL, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_TOTAL },
   { "System.Memory.Physical.Used", H_MemoryInfo, (char *)MEMINFO_PHYSICAL_USED, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED },
   { "System.Memory.Physical.UsedPerc", H_MemoryInfo, (char *)MEMINFO_PHYSICAL_USED_PERC, DCI_DT_UINT, DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED_PCT },
   { "System.Memory.Swap.Free", H_VirtualMemoryInfo, (char *)MEMINFO_SWAP_FREE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_SWAP_FREE },
   { "System.Memory.Swap.FreePerc", H_VirtualMemoryInfo, (char *)MEMINFO_SWAP_FREE_PERC, DCI_DT_UINT, DCIDESC_SYSTEM_MEMORY_SWAP_FREE_PCT },
   { "System.Memory.Swap.Total", H_VirtualMemoryInfo, (char *)MEMINFO_SWAP_TOTAL, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_SWAP_TOTAL },
   { "System.Memory.Swap.Used", H_VirtualMemoryInfo, (char *)MEMINFO_SWAP_USED, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_SWAP_USED },
   { "System.Memory.Swap.UsedPerc", H_VirtualMemoryInfo, (char *)MEMINFO_SWAP_USED_PERC, DCI_DT_UINT, DCIDESC_SYSTEM_MEMORY_SWAP_USED_PCT },
   { "System.Memory.Virtual.Active", H_VirtualMemoryInfo, (char *)MEMINFO_VIRTUAL_ACTIVE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_VIRTUAL_ACTIVE },
   { "System.Memory.Virtual.ActivePerc", H_VirtualMemoryInfo, (char *)MEMINFO_VIRTUAL_ACTIVE_PERC, DCI_DT_UINT, DCIDESC_SYSTEM_MEMORY_VIRTUAL_ACTIVE_PCT },
   { "System.Memory.Virtual.Free", H_VirtualMemoryInfo, (char *)MEMINFO_VIRTUAL_FREE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_VIRTUAL_FREE },
   { "System.Memory.Virtual.FreePerc", H_VirtualMemoryInfo, (char *)MEMINFO_VIRTUAL_FREE_PERC, DCI_DT_UINT, DCIDESC_SYSTEM_MEMORY_VIRTUAL_FREE_PCT },
   { "System.Memory.Virtual.Total", H_VirtualMemoryInfo, (char *)MEMINFO_VIRTUAL_TOTAL, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_VIRTUAL_TOTAL },
   { "System.Memory.Virtual.Used", H_VirtualMemoryInfo, (char *)MEMINFO_VIRTUAL_USED, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_VIRTUAL_USED },
   { "System.Memory.Virtual.UsedPerc", H_VirtualMemoryInfo, (char *)MEMINFO_VIRTUAL_USED_PERC, DCI_DT_UINT, DCIDESC_SYSTEM_MEMORY_VIRTUAL_USED_PCT },

   { "System.ProcessCount", H_SysProcessCount, NULL, DCI_DT_INT, DCIDESC_SYSTEM_PROCESSCOUNT },
   { "System.ThreadCount", H_SysThreadCount, NULL, DCI_DT_INT, DCIDESC_SYSTEM_THREADCOUNT },
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
   _T("AIX"), NETXMS_VERSION_STRING,
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

DECLARE_SUBAGENT_ENTRY_POINT(AIX)
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

/*
extern "C" BOOL __NxSubAgentGetArpCache(StringList *pValue)
{
   return H_NetArpCache("Net.ArpCache", NULL, pValue) == SYSINFO_RC_SUCCESS;
}
*/

