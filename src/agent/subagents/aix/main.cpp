/*
** NetXMS subagent for AIX
** Copyright (C) 2005-2014 Victor Kirhenshtein
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

LONG H_CpuCount(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_CpuUsage(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_CpuUsageEx(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_DiskInfo(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_FileSystems(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *);
LONG H_Hostname(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_IOStats(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_IOStatsTotal(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_LoadAvg(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_MemoryInfo(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_MountPoints(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_NetInterfaceStatus(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_NetInterfaceInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_NetInterfaceList(const TCHAR *pszParam, const TCHAR *pArg, StringList *pValue, AbstractCommSession *session);
LONG H_ProcessCount(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_ProcessInfo(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_ProcessList(const TCHAR *pszParam, const TCHAR *pArg, StringList *pValue, AbstractCommSession *session);
LONG H_SysProcessCount(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_SysThreadCount(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_Uname(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_Uptime(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_VirtualMemoryInfo(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);


//
// Global variables
//

BOOL g_bShutdown = FALSE;


//
// Static data
//

static THREAD m_hCPUStatThread = INVALID_THREAD_HANDLE;

/**
 * Detect support for source packages
 */
static LONG H_SourcePkg(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
	ret_int(pValue, 1);
	return SYSINFO_RC_SUCCESS;
}

/**
 * Initalization callback
 */
static BOOL SubAgentInit(Config *config)
{
	StartCpuUsageCollector();
	StartIOStatCollector();
	return TRUE;
}

/**
 * Called by master agent at unload
 */
static void SubAgentShutdown()
{
	g_bShutdown = TRUE;
	ShutdownCpuUsageCollector();
	ShutdownIOStatCollector();
}

/**
 * Subagent's parameters
 */
static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
   { _T("Agent.SourcePackageSupport"), H_SourcePkg, NULL, DCI_DT_INT, DCIDESC_AGENT_SOURCEPACKAGESUPPORT },

   { _T("Disk.Avail(*)"), H_DiskInfo, (TCHAR *)DISK_AVAIL, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.AvailPerc(*)"), H_DiskInfo, (TCHAR *)DISK_AVAIL_PERC, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.Free(*)"), H_DiskInfo, (TCHAR *)DISK_FREE, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.FreePerc(*)"), H_DiskInfo, (TCHAR *)DISK_FREE_PERC, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.Total(*)"), H_DiskInfo, (TCHAR *)DISK_TOTAL, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.Used(*)"), H_DiskInfo, (TCHAR *)DISK_USED, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.UsedPerc(*)"), H_DiskInfo, (TCHAR *)DISK_USED_PERC, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },

   { _T("FileSystem.Avail(*)"), H_DiskInfo, (TCHAR *)DISK_AVAIL, DCI_DT_UINT64, DCIDESC_FS_AVAIL },
   { _T("FileSystem.AvailPerc(*)"), H_DiskInfo, (TCHAR *)DISK_AVAIL_PERC, DCI_DT_FLOAT, DCIDESC_FS_AVAILPERC },
   { _T("FileSystem.Free(*)"), H_DiskInfo, (TCHAR *)DISK_FREE, DCI_DT_UINT64, DCIDESC_FS_FREE },
   { _T("FileSystem.FreePerc(*)"), H_DiskInfo, (TCHAR *)DISK_FREE_PERC, DCI_DT_FLOAT, DCIDESC_FS_FREEPERC },
   { _T("FileSystem.Total(*)"), H_DiskInfo, (TCHAR *)DISK_TOTAL, DCI_DT_UINT64, DCIDESC_FS_TOTAL },
   { _T("FileSystem.Type(*)"), H_DiskInfo, (TCHAR *)DISK_FSTYPE, DCI_DT_STRING, DCIDESC_FS_TYPE },
   { _T("FileSystem.Used(*)"), H_DiskInfo, (TCHAR *)DISK_USED, DCI_DT_UINT64, DCIDESC_FS_USED },
   { _T("FileSystem.UsedPerc(*)"), H_DiskInfo, (TCHAR *)DISK_USED_PERC, DCI_DT_FLOAT, DCIDESC_FS_USEDPERC },

	{ _T("Net.Interface.AdminStatus(*)"), H_NetInterfaceStatus, (TCHAR *)IF_INFO_ADMIN_STATUS, DCI_DT_INT, DCIDESC_NET_INTERFACE_ADMINSTATUS },
	{ _T("Net.Interface.BytesIn(*)"), H_NetInterfaceInfo, (TCHAR *)IF_INFO_BYTES_IN, DCI_DT_UINT, DCIDESC_NET_INTERFACE_BYTESIN },
	{ _T("Net.Interface.BytesOut(*)"), H_NetInterfaceInfo, (TCHAR *)IF_INFO_BYTES_OUT, DCI_DT_UINT, DCIDESC_NET_INTERFACE_BYTESOUT },
	{ _T("Net.Interface.Description(*)"), H_NetInterfaceInfo, (TCHAR *)IF_INFO_DESCRIPTION, DCI_DT_STRING, DCIDESC_NET_INTERFACE_DESCRIPTION },
	{ _T("Net.Interface.InErrors(*)"), H_NetInterfaceInfo, (TCHAR *)IF_INFO_IN_ERRORS, DCI_DT_UINT, DCIDESC_NET_INTERFACE_INERRORS },
	{ _T("Net.Interface.Link(*)"), H_NetInterfaceStatus, (TCHAR *)IF_INFO_OPER_STATUS, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
	{ _T("Net.Interface.OperStatus(*)"), H_NetInterfaceStatus, (TCHAR *)IF_INFO_OPER_STATUS, DCI_DT_INT, DCIDESC_NET_INTERFACE_OPERSTATUS },
	{ _T("Net.Interface.MTU(*)"), H_NetInterfaceInfo, (TCHAR *)IF_INFO_MTU, DCI_DT_INT, DCIDESC_NET_INTERFACE_MTU },
	{ _T("Net.Interface.OutErrors(*)"), H_NetInterfaceInfo, (TCHAR *)IF_INFO_OUT_ERRORS, DCI_DT_UINT, DCIDESC_NET_INTERFACE_OUTERRORS },
	{ _T("Net.Interface.PacketsIn(*)"), H_NetInterfaceInfo, (TCHAR *)IF_INFO_PACKETS_IN, DCI_DT_UINT, DCIDESC_NET_INTERFACE_PACKETSIN },
	{ _T("Net.Interface.PacketsOut(*)"), H_NetInterfaceInfo, (TCHAR *)IF_INFO_PACKETS_OUT, DCI_DT_UINT, DCIDESC_NET_INTERFACE_PACKETSOUT },
	{ _T("Net.Interface.Speed(*)"), H_NetInterfaceInfo, (TCHAR *)IF_INFO_SPEED, DCI_DT_INT, DCIDESC_NET_INTERFACE_SPEED },

   { _T("Process.Count(*)"), H_ProcessCount, NULL, DCI_DT_UINT, DCIDESC_PROCESS_COUNT },
   { _T("Process.CPUTime(*)"), H_ProcessInfo, (TCHAR *)PROCINFO_CPUTIME, DCI_DT_UINT64, DCIDESC_PROCESS_CPUTIME },
   { _T("Process.IO.ReadOp(*)"), H_ProcessInfo, (TCHAR *)PROCINFO_IO_READ_OP, DCI_DT_UINT64, DCIDESC_PROCESS_IO_READOP },
   { _T("Process.IO.WriteOp(*)"), H_ProcessInfo, (TCHAR *)PROCINFO_IO_WRITE_OP, DCI_DT_UINT64, DCIDESC_PROCESS_IO_WRITEOP },
   { _T("Process.KernelTime(*)"), H_ProcessInfo, (TCHAR *)PROCINFO_KTIME, DCI_DT_UINT64, DCIDESC_PROCESS_KERNELTIME },
   { _T("Process.PageFaults(*)"), H_ProcessInfo, (TCHAR *)PROCINFO_PF, DCI_DT_UINT64, DCIDESC_PROCESS_PAGEFAULTS },
   { _T("Process.Threads(*)"), H_ProcessInfo, (TCHAR *)PROCINFO_THREADS, DCI_DT_UINT64, DCIDESC_PROCESS_THREADS },
   { _T("Process.UserTime(*)"), H_ProcessInfo, (TCHAR *)PROCINFO_UTIME, DCI_DT_UINT64, DCIDESC_PROCESS_USERTIME },
   { _T("Process.VMSize(*)"), H_ProcessInfo, (TCHAR *)PROCINFO_VMSIZE, DCI_DT_UINT64, DCIDESC_PROCESS_VMSIZE },
   { _T("System.CPU.Count"), H_CpuCount, NULL, DCI_DT_UINT, DCIDESC_SYSTEM_CPU_COUNT },
   { _T("System.CPU.LoadAvg"), H_LoadAvg, _T("0"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_LOADAVG },
   { _T("System.CPU.LoadAvg5"), H_LoadAvg, _T("1"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_LOADAVG5 },
   { _T("System.CPU.LoadAvg15"), H_LoadAvg, _T("2"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_LOADAVG15 },
   
	{ _T("System.CPU.Usage"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_OVERALL), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE },
	{ _T("System.CPU.Usage5"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_OVERALL), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5 },
	{ _T("System.CPU.Usage15"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_OVERALL), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15 },
	{ _T("System.CPU.Usage(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_OVERALL), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_EX },
	{ _T("System.CPU.Usage5(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_OVERALL), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_EX },
	{ _T("System.CPU.Usage15(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_OVERALL), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_EX },

	{ _T("System.CPU.Usage.Idle"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_IDLE), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_IDLE },
	{ _T("System.CPU.Usage5.Idle"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_IDLE), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_IDLE },
	{ _T("System.CPU.Usage15.Idle"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_IDLE), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_IDLE },
	{ _T("System.CPU.Usage.Idle(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_IDLE), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_IDLE_EX },
	{ _T("System.CPU.Usage5.Idle5(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_IDLE), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_IDLE_EX },
	{ _T("System.CPU.Usage15.Idle(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_IDLE), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_IDLE_EX },

	{ _T("System.CPU.Usage.IoWait"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_IOWAIT), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_IOWAIT },
	{ _T("System.CPU.Usage5.IoWait"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_IOWAIT), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_IOWAIT },
	{ _T("System.CPU.Usage15.IoWait"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_IOWAIT), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_IOWAIT },
	{ _T("System.CPU.Usage.IoWait(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_IOWAIT), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_IOWAIT_EX },
	{ _T("System.CPU.Usage5.IoWait(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_IOWAIT), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_IOWAIT_EX },
	{ _T("System.CPU.Usage15.IoWait(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_IOWAIT), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_IOWAIT_EX },

	{ _T("System.CPU.Usage.System"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_SYSTEM), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_SYSTEM },
	{ _T("System.CPU.Usage5.System"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_SYSTEM), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_SYSTEM },
	{ _T("System.CPU.Usage15.System"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_SYSTEM), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_SYSTEM },
	{ _T("System.CPU.Usage.System(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_SYSTEM), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_SYSTEM_EX },
	{ _T("System.CPU.Usage5.System(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_SYSTEM), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_SYSTEM_EX },
	{ _T("System.CPU.Usage15.System(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_SYSTEM), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_SYSTEM_EX },

	{ _T("System.CPU.Usage.User"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_USER), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_USER },
	{ _T("System.CPU.Usage5.User"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_USER), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_USER },
	{ _T("System.CPU.Usage15.User"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_USER), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_USER },
	{ _T("System.CPU.Usage.User(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_USER), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_USER_EX },
	{ _T("System.CPU.Usage5.User(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_USER), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_USER_EX },
	{ _T("System.CPU.Usage15.User(*)"), H_CpuUsageEx, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_USER), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_USER_EX },

   { _T("System.CPU.PhysicalAverage"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_PA_OVERALL), DCI_DT_FLOAT,
         _T("Average Physical CPU utilization for last minute") },
   { _T("System.CPU.PhysicalAverage5"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_PA_OVERALL), DCI_DT_FLOAT,
         _T("Average Physical CPU utilization for last 5 minutes") },
   { _T("System.CPU.PhysicalAverage15"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_PA_OVERALL), DCI_DT_FLOAT,
         _T("Average Physical CPU utilization for last 15 minutes") },
   { _T("System.CPU.PhysicalAverage.User"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_PA_USER), DCI_DT_FLOAT,
         _T("Average Physical CPU utilization (user) for last minute") },
   { _T("System.CPU.PhysicalAverage5.User"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_PA_USER), DCI_DT_FLOAT,
         _T("Average Physical CPU utilization (user) for last 5 minutes") },
   { _T("System.CPU.PhysicalAverage15.User"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_PA_USER), DCI_DT_FLOAT,
         _T("Average Physical CPU utilization (user) for last 15 minutes") },
   { _T("System.CPU.PhysicalAverage.System"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_PA_SYSTEM), DCI_DT_FLOAT,
         _T("Average Physical CPU utilization (system) for last minute") },
   { _T("System.CPU.PhysicalAverage5.System"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_PA_SYSTEM), DCI_DT_FLOAT,
         _T("Average Physical CPU utilization (system) for last 5 minutes") },
   { _T("System.CPU.PhysicalAverage15.System"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_PA_SYSTEM), DCI_DT_FLOAT,
         _T("Average Physical CPU utilization (syste) for last 15 minutes") },
   { _T("System.CPU.PhysicalAverage.Idle"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_PA_IDLE), DCI_DT_FLOAT,
         _T("Average Physical CPU utilization (idle) for last minute") },
   { _T("System.CPU.PhysicalAverage5.Idle"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_PA_IDLE), DCI_DT_FLOAT,
         _T("Average Physical CPU utilization (idle) for last 5 minutes") },
   { _T("System.CPU.PhysicalAverage15.Idle"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_PA_IDLE), DCI_DT_FLOAT,
         _T("Average Physical CPU utilization (idle) for last 15 minutes") },
   { _T("System.CPU.PhysicalAverage.IoWait"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_PA_IOWAIT), DCI_DT_FLOAT,
         _T("Average Physical CPU utilization (iowait) for last minute") },
   { _T("System.CPU.PhysicalAverage5.IoWait"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_PA_IOWAIT), DCI_DT_FLOAT,
         _T("Average Physical CPU utilization (iowait) for last 5 minutes") },
   { _T("System.CPU.PhysicalAverage15.IoWait"), H_CpuUsage, MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_PA_IOWAIT), DCI_DT_FLOAT,
         _T("Average Physical CPU utilization (iowait) for last 15 minutes") },

   { _T("System.Hostname"), H_Hostname, NULL, DCI_DT_STRING, DCIDESC_SYSTEM_HOSTNAME },

	{ _T("System.IO.BytesReadRate"), H_IOStatsTotal, (const TCHAR *)IOSTAT_NUM_RBYTES, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEREADS },
	{ _T("System.IO.BytesReadRate(*)"), H_IOStats, (const TCHAR *)IOSTAT_NUM_RBYTES, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEREADS_EX },
	{ _T("System.IO.BytesWriteRate"), H_IOStatsTotal, (const TCHAR *)IOSTAT_NUM_WBYTES, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEWRITES },
	{ _T("System.IO.BytesWriteRate(*)"), H_IOStats, (const TCHAR *)IOSTAT_NUM_WBYTES, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEWRITES_EX },
	{ _T("System.IO.DiskQueue"), H_IOStatsTotal, (const TCHAR *)IOSTAT_QUEUE, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_DISKQUEUE },
	{ _T("System.IO.DiskQueue(*)"), H_IOStats, (const TCHAR *)IOSTAT_QUEUE, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_DISKQUEUE_EX },
	{ _T("System.IO.ReadRate"), H_IOStatsTotal, (const TCHAR *)IOSTAT_NUM_READS, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_READS },
	{ _T("System.IO.ReadRate(*)"), H_IOStats, (const TCHAR *)IOSTAT_NUM_READS, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_READS_EX },
	{ _T("System.IO.TransferRate"), H_IOStatsTotal, (const TCHAR *)IOSTAT_NUM_XFERS, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_XFERS },
	{ _T("System.IO.TransferRate(*)"), H_IOStats, (const TCHAR *)IOSTAT_NUM_XFERS, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_XFERS_EX },
	{ _T("System.IO.WaitTime"), H_IOStatsTotal, (const TCHAR *)IOSTAT_WAIT_TIME, DCI_DT_INT, DCIDESC_SYSTEM_IO_WAITTIME },
	{ _T("System.IO.WaitTime(*)"), H_IOStats, (const TCHAR *)IOSTAT_WAIT_TIME, DCI_DT_INT, DCIDESC_SYSTEM_IO_WAITTIME_EX },
	{ _T("System.IO.WriteRate"), H_IOStatsTotal, (const TCHAR *)IOSTAT_NUM_WRITES, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_WRITES },
	{ _T("System.IO.WriteRate(*)"), H_IOStats, (const TCHAR *)IOSTAT_NUM_WRITES, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_WRITES_EX },

   { _T("System.Memory.Physical.Free"), H_MemoryInfo, (TCHAR *)MEMINFO_PHYSICAL_FREE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE },
   { _T("System.Memory.Physical.FreePerc"), H_MemoryInfo, (TCHAR *)MEMINFO_PHYSICAL_FREE_PERC, DCI_DT_UINT, DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE_PCT },
   { _T("System.Memory.Physical.Total"), H_MemoryInfo, (TCHAR *)MEMINFO_PHYSICAL_TOTAL, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_TOTAL },
   { _T("System.Memory.Physical.Used"), H_MemoryInfo, (TCHAR *)MEMINFO_PHYSICAL_USED, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED },
   { _T("System.Memory.Physical.UsedPerc"), H_MemoryInfo, (TCHAR *)MEMINFO_PHYSICAL_USED_PERC, DCI_DT_UINT, DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED_PCT },
   { _T("System.Memory.Swap.Free"), H_VirtualMemoryInfo, (TCHAR *)MEMINFO_SWAP_FREE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_SWAP_FREE },
   { _T("System.Memory.Swap.FreePerc"), H_VirtualMemoryInfo, (TCHAR *)MEMINFO_SWAP_FREE_PERC, DCI_DT_UINT, DCIDESC_SYSTEM_MEMORY_SWAP_FREE_PCT },
   { _T("System.Memory.Swap.Total"), H_VirtualMemoryInfo, (TCHAR *)MEMINFO_SWAP_TOTAL, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_SWAP_TOTAL },
   { _T("System.Memory.Swap.Used"), H_VirtualMemoryInfo, (TCHAR *)MEMINFO_SWAP_USED, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_SWAP_USED },
   { _T("System.Memory.Swap.UsedPerc"), H_VirtualMemoryInfo, (TCHAR *)MEMINFO_SWAP_USED_PERC, DCI_DT_UINT, DCIDESC_SYSTEM_MEMORY_SWAP_USED_PCT },
   { _T("System.Memory.Virtual.Active"), H_VirtualMemoryInfo, (TCHAR *)MEMINFO_VIRTUAL_ACTIVE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_VIRTUAL_ACTIVE },
   { _T("System.Memory.Virtual.ActivePerc"), H_VirtualMemoryInfo, (TCHAR *)MEMINFO_VIRTUAL_ACTIVE_PERC, DCI_DT_UINT, DCIDESC_SYSTEM_MEMORY_VIRTUAL_ACTIVE_PCT },
   { _T("System.Memory.Virtual.Free"), H_VirtualMemoryInfo, (TCHAR *)MEMINFO_VIRTUAL_FREE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_VIRTUAL_FREE },
   { _T("System.Memory.Virtual.FreePerc"), H_VirtualMemoryInfo, (TCHAR *)MEMINFO_VIRTUAL_FREE_PERC, DCI_DT_UINT, DCIDESC_SYSTEM_MEMORY_VIRTUAL_FREE_PCT },
   { _T("System.Memory.Virtual.Total"), H_VirtualMemoryInfo, (TCHAR *)MEMINFO_VIRTUAL_TOTAL, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_VIRTUAL_TOTAL },
   { _T("System.Memory.Virtual.Used"), H_VirtualMemoryInfo, (TCHAR *)MEMINFO_VIRTUAL_USED, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_VIRTUAL_USED },
   { _T("System.Memory.Virtual.UsedPerc"), H_VirtualMemoryInfo, (TCHAR *)MEMINFO_VIRTUAL_USED_PERC, DCI_DT_UINT, DCIDESC_SYSTEM_MEMORY_VIRTUAL_USED_PCT },

   { _T("System.ProcessCount"), H_SysProcessCount, NULL, DCI_DT_INT, DCIDESC_SYSTEM_PROCESSCOUNT },
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
   { _T("Net.InterfaceList"), H_NetInterfaceList, NULL },
   { _T("System.ProcessList"), H_ProcessList, NULL }
};

/**
 * Subagent's tables
 */
static NETXMS_SUBAGENT_TABLE m_tables[] =
{
   { _T("FileSystem.Volumes"), H_FileSystems, NULL, _T("MOUNTPOINT"), DCTDESC_FILESYSTEM_VOLUMES }
};

/**
 * Subagent info
 */
static NETXMS_SUBAGENT_INFO m_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("AIX"), NETXMS_VERSION_STRING,
   SubAgentInit, SubAgentShutdown, NULL,
   sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
   m_parameters,
   sizeof(m_lists) / sizeof(NETXMS_SUBAGENT_LIST),
   m_lists,
   sizeof(m_tables) / sizeof(NETXMS_SUBAGENT_TABLE),
   m_tables,
   0, NULL,	// actions
	0, NULL	// push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(AIX)
{
   *ppInfo = &m_info;
   return TRUE;
}

/**
 * Entry point for server: get interface list
 */
extern "C" BOOL __NxSubAgentGetIfList(StringList *pValue)
{
   return H_NetInterfaceList(_T("Net.InterfaceList"), NULL, pValue, NULL) == SYSINFO_RC_SUCCESS;
}  

/*
extern "C" BOOL __NxSubAgentGetArpCache(StringList *pValue)
{
   return H_NetArpCache(_T("Net.ArpCache"), NULL, pValue) == SYSINFO_RC_SUCCESS;
}
*/

