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

LONG H_CPUCount(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_CPUUsage(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_DiskInfo(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_FileSystems(const TCHAR *cmd, const TCHAR *arg, Table *table, AbstractCommSession *session);
LONG H_Hostname(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_IOStats(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_IOStatsTotal(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_KStat(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_LoadAvg(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_MemoryInfo(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_MountPoints(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_NetIfList(const TCHAR *pszParam, const TCHAR *pArg, StringList *pValue, AbstractCommSession *session);
LONG H_NetIfAdminStatus(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_NetInterfaceLink(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_NetIfDescription(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_NetInterfaceStats(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_ProcessCount(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_ProcessInfo(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_ProcessList(const TCHAR *pszParam, const TCHAR *pArg, StringList *pValue, AbstractCommSession *session);
LONG H_SysProcCount(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_Uname(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);
LONG H_Uptime(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);


//
// Global variables
//

BOOL g_bShutdown = FALSE;
DWORD g_flags = 0;


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

static LONG H_SourcePkg(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   ret_int(pValue, 1);
   return SYSINFO_RC_SUCCESS;
}


//
// Configuration file template
//

static NX_CFG_TEMPLATE s_cfgTemplate[] =
{
   { _T("InterfacesFromAllZones"), CT_BOOLEAN, 0, 0, SF_IF_ALL_ZONES, 0, &g_flags },
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
};

/**
 * Initalization callback
 */
static BOOL SubAgentInit(Config *config)
{
   if (!config->parseTemplate(_T("SunOS"), s_cfgTemplate))
      return FALSE;

   // try to determine if we are running in global zone
   if (access("/dev/dump", F_OK) == 0)
   {
      g_flags |= SF_GLOBAL_ZONE;
      AgentWriteDebugLog(2, _T("SunOS: running in global zone"));
   }
   else
   {
      g_flags &= ~SF_GLOBAL_ZONE;
      AgentWriteDebugLog(2, _T("SunOS: running in zone"));
   }

   s_cpuStatThread = ThreadCreateEx(CPUStatCollector, 0, NULL);
   s_ioStatThread = ThreadCreateEx(IOStatCollector, 0, NULL);
   s_kstatLock = MutexCreate();

   return TRUE;
}

/**
 * Called by master agent at unload
 */
static void SubAgentShutdown()
{
   g_bShutdown = TRUE;
   ThreadJoin(s_cpuStatThread);
   ThreadJoin(s_ioStatThread);
   MutexDestroy(s_kstatLock);
}

/**
 * Parameters provided by subagent
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
   { _T("FileSystem.Used(*)"), H_DiskInfo, (TCHAR *)DISK_USED, DCI_DT_UINT64, DCIDESC_FS_USED },
   { _T("FileSystem.UsedPerc(*)"), H_DiskInfo, (TCHAR *)DISK_USED_PERC, DCI_DT_FLOAT, DCIDESC_FS_USEDPERC },

   { _T("Net.Interface.AdminStatus(*)"), H_NetIfAdminStatus, NULL, DCI_DT_INT, DCIDESC_NET_INTERFACE_ADMINSTATUS },
   { _T("Net.Interface.BytesIn(*)"), H_NetInterfaceStats, (const TCHAR *)"rbytes", DCI_DT_UINT, DCIDESC_NET_INTERFACE_BYTESIN },
   { _T("Net.Interface.BytesOut(*)"), H_NetInterfaceStats, (const TCHAR *)"obytes", DCI_DT_UINT, DCIDESC_NET_INTERFACE_BYTESOUT },
   { _T("Net.Interface.Description(*)"), H_NetIfDescription, NULL, DCI_DT_STRING, DCIDESC_NET_INTERFACE_DESCRIPTION },
   { _T("Net.Interface.InErrors(*)"), H_NetInterfaceStats, (const TCHAR *)"ierrors", DCI_DT_UINT, DCIDESC_NET_INTERFACE_INERRORS },
   { _T("Net.Interface.Link(*)"), H_NetInterfaceLink, NULL, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Net.Interface.OperStatus(*)"), H_NetInterfaceLink, NULL, DCI_DT_INT, DCIDESC_NET_INTERFACE_OPERSTATUS },
   { _T("Net.Interface.OutErrors(*)"), H_NetInterfaceStats, (const TCHAR *)"oerrors", DCI_DT_UINT, DCIDESC_NET_INTERFACE_OUTERRORS },
   { _T("Net.Interface.PacketsIn(*)"), H_NetInterfaceStats, (const TCHAR *)"ipackets", DCI_DT_UINT, DCIDESC_NET_INTERFACE_PACKETSIN },
   { _T("Net.Interface.PacketsOut(*)"), H_NetInterfaceStats, (const TCHAR *)"opackets", DCI_DT_UINT, DCIDESC_NET_INTERFACE_PACKETSOUT },
   { _T("Net.Interface.Speed(*)"), H_NetInterfaceStats, (const TCHAR *)"ifspeed", DCI_DT_UINT, DCIDESC_NET_INTERFACE_SPEED },

   { _T("Process.Count(*)"), H_ProcessCount, _T("S"), DCI_DT_UINT, DCIDESC_PROCESS_COUNT },
   { _T("Process.CountEx(*)"), H_ProcessCount, _T("E"), DCI_DT_UINT, DCIDESC_PROCESS_COUNTEX },
   { _T("Process.ZombieCount"), H_ProcessCount, _T("Z"), DCI_DT_UINT, DCIDESC_PROCESS_ZOMBIE_COUNT },
   { _T("Process.CPUTime(*)"), H_ProcessInfo, CAST_TO_POINTER(PROCINFO_CPUTIME, const TCHAR *), DCI_DT_UINT64, DCIDESC_PROCESS_CPUTIME },
   { _T("Process.KernelTime(*)"), H_ProcessInfo, CAST_TO_POINTER(PROCINFO_KTIME, const TCHAR *), DCI_DT_UINT64, DCIDESC_PROCESS_KERNELTIME },
   { _T("Process.PageFaults(*)"), H_ProcessInfo, CAST_TO_POINTER(PROCINFO_PF, const TCHAR *), DCI_DT_UINT64, DCIDESC_PROCESS_PAGEFAULTS },
   { _T("Process.Syscalls(*)"), H_ProcessInfo, CAST_TO_POINTER(PROCINFO_SYSCALLS, const TCHAR *), DCI_DT_UINT64, DCIDESC_PROCESS_SYSCALLS },
   { _T("Process.Threads(*)"), H_ProcessInfo, CAST_TO_POINTER(PROCINFO_THREADS, const TCHAR *), DCI_DT_UINT, DCIDESC_PROCESS_THREADS },
   { _T("Process.UserTime(*)"), H_ProcessInfo,  CAST_TO_POINTER(PROCINFO_UTIME, const TCHAR *), DCI_DT_UINT64, DCIDESC_PROCESS_USERTIME },
   { _T("Process.VMSize(*)"), H_ProcessInfo, CAST_TO_POINTER(PROCINFO_VMSIZE, const TCHAR *), DCI_DT_UINT64, DCIDESC_PROCESS_VMSIZE },
   { _T("Process.WkSet(*)"), H_ProcessInfo, CAST_TO_POINTER(PROCINFO_WKSET, const TCHAR *), DCI_DT_UINT64, DCIDESC_PROCESS_WKSET },

   { _T("System.CPU.Count"), H_CPUCount, NULL, DCI_DT_UINT, DCIDESC_SYSTEM_CPU_COUNT },
   { _T("System.CPU.LoadAvg"), H_LoadAvg, (TCHAR *)0, DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_LOADAVG },
   { _T("System.CPU.LoadAvg5"), H_LoadAvg, (TCHAR *)1, DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_LOADAVG5 },
   { _T("System.CPU.LoadAvg15"), H_LoadAvg, (TCHAR *)2, DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_LOADAVG15 },
   { _T("System.CPU.Usage"), H_CPUUsage, _T("T0"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE },
   { _T("System.CPU.Usage5"), H_CPUUsage, _T("T1"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5 },
   { _T("System.CPU.Usage15"), H_CPUUsage, _T("T2"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15 },
   { _T("System.CPU.Usage(*)"), H_CPUUsage, _T("C0"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_EX },
   { _T("System.CPU.Usage5(*)"), H_CPUUsage, _T("C1"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_EX },
   { _T("System.CPU.Usage15(*)"), H_CPUUsage, _T("C2"), DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_EX },
   { _T("System.Hostname"), H_Hostname, NULL, DCI_DT_STRING, DCIDESC_SYSTEM_HOSTNAME },
   { _T("System.KStat(*)"), H_KStat, NULL, DCI_DT_STRING, _T("") },
   { _T("System.IO.ReadRate"), H_IOStatsTotal, (const TCHAR *)IOSTAT_NUM_READS, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_READS },
   { _T("System.IO.ReadRate.Min"), H_IOStatsTotal, (const TCHAR *)IOSTAT_NUM_READS_MIN, DCI_DT_UINT, DCIDESC_SYSTEM_IO_READS_MIN },
   { _T("System.IO.ReadRate.Max"), H_IOStatsTotal, (const TCHAR *)IOSTAT_NUM_READS_MAX, DCI_DT_UINT, DCIDESC_SYSTEM_IO_READS_MAX },
   { _T("System.IO.ReadRate(*)"), H_IOStats, (const TCHAR *)IOSTAT_NUM_READS, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_READS_EX },
   { _T("System.IO.ReadRate.Min(*)"), H_IOStats, (const TCHAR *)IOSTAT_NUM_READS_MIN, DCI_DT_UINT, DCIDESC_SYSTEM_IO_READS_EX_MIN },
   { _T("System.IO.ReadRate.Max(*)"), H_IOStats, (const TCHAR *)IOSTAT_NUM_READS_MAX, DCI_DT_UINT, DCIDESC_SYSTEM_IO_READS_EX_MAX },
   { _T("System.IO.WriteRate"), H_IOStatsTotal, (const TCHAR *)IOSTAT_NUM_WRITES, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_WRITES },
   { _T("System.IO.WriteRate.Min"), H_IOStatsTotal, (const TCHAR *)IOSTAT_NUM_WRITES_MIN, DCI_DT_UINT, DCIDESC_SYSTEM_IO_WRITES_MIN },
   { _T("System.IO.WriteRate.Max"), H_IOStatsTotal, (const TCHAR *)IOSTAT_NUM_WRITES_MAX, DCI_DT_UINT, DCIDESC_SYSTEM_IO_WRITES_MAX },
   { _T("System.IO.WriteRate(*)"), H_IOStats, (const TCHAR *)IOSTAT_NUM_WRITES, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_WRITES_EX },
   { _T("System.IO.WriteRate.Min(*)"), H_IOStats, (const TCHAR *)IOSTAT_NUM_WRITES_MIN, DCI_DT_UINT, DCIDESC_SYSTEM_IO_WRITES_EX_MIN },
   { _T("System.IO.WriteRate.Max(*)"), H_IOStats, (const TCHAR *)IOSTAT_NUM_WRITES_MAX, DCI_DT_UINT, DCIDESC_SYSTEM_IO_WRITES_EX_MAX },
   { _T("System.IO.BytesReadRate"), H_IOStatsTotal, (const TCHAR *)IOSTAT_NUM_RBYTES, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEREADS },
   { _T("System.IO.BytesReadRate.Min"), H_IOStatsTotal, (const TCHAR *)IOSTAT_NUM_RBYTES_MIN, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEREADS_MIN },
   { _T("System.IO.BytesReadRate.Max"), H_IOStatsTotal, (const TCHAR *)IOSTAT_NUM_RBYTES_MAX, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEREADS_MAX },
   { _T("System.IO.BytesReadRate(*)"), H_IOStats, (const TCHAR *)IOSTAT_NUM_RBYTES, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEREADS_EX },
   { _T("System.IO.BytesReadRate.Min(*)"), H_IOStats, (const TCHAR *)IOSTAT_NUM_RBYTES_MIN, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEREADS_EX_MIN },
   { _T("System.IO.BytesReadRate.Max(*)"), H_IOStats, (const TCHAR *)IOSTAT_NUM_RBYTES_MAX, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEREADS_EX_MAX },
   { _T("System.IO.BytesWriteRate"), H_IOStatsTotal, (const TCHAR *)IOSTAT_NUM_WBYTES, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEWRITES },
   { _T("System.IO.BytesWriteRate.Min"), H_IOStatsTotal, (const TCHAR *)IOSTAT_NUM_WBYTES_MIN, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEWRITES_MIN },
   { _T("System.IO.BytesWriteRate.Max"), H_IOStatsTotal, (const TCHAR *)IOSTAT_NUM_WBYTES_MAX, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEWRITES_MAX },
   { _T("System.IO.BytesWriteRate(*)"), H_IOStats, (const TCHAR *)IOSTAT_NUM_WBYTES, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEWRITES_EX },
   { _T("System.IO.BytesWriteRate.Min(*)"), H_IOStats, (const TCHAR *)IOSTAT_NUM_WBYTES_MIN, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEWRITES_EX_MIN },
   { _T("System.IO.BytesWriteRate.Max(*)"), H_IOStats, (const TCHAR *)IOSTAT_NUM_WBYTES_MAX, DCI_DT_UINT64, DCIDESC_SYSTEM_IO_BYTEWRITES_EX_MAX },
   { _T("System.IO.DiskQueue"), H_IOStatsTotal, (const TCHAR *)IOSTAT_QUEUE, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_DISKQUEUE },
   { _T("System.IO.DiskQueue.Min"), H_IOStatsTotal, (const TCHAR *)IOSTAT_QUEUE_MIN, DCI_DT_UINT, DCIDESC_SYSTEM_IO_DISKQUEUE_MIN },
   { _T("System.IO.DiskQueue.Max"), H_IOStatsTotal, (const TCHAR *)IOSTAT_QUEUE_MAX, DCI_DT_UINT, DCIDESC_SYSTEM_IO_DISKQUEUE_MAX },
   { _T("System.IO.DiskQueue(*)"), H_IOStats, (const TCHAR *)IOSTAT_QUEUE, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_DISKQUEUE_EX },
   { _T("System.IO.DiskQueue.Min(*)"), H_IOStats, (const TCHAR *)IOSTAT_QUEUE_MIN, DCI_DT_UINT, DCIDESC_SYSTEM_IO_DISKQUEUE_EX_MIN },
   { _T("System.IO.DiskQueue.Max(*)"), H_IOStats, (const TCHAR *)IOSTAT_QUEUE_MAX, DCI_DT_UINT, DCIDESC_SYSTEM_IO_DISKQUEUE_EX_MAX },
   //        { _T("System.IO.DiskTime"), H_IoStatsTotal, (const TCHAR *)IOSTAT_IO_TIME, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_DISKTIME },
   //        { _T("System.IO.DiskTime(*)"), H_IoStats, (const TCHAR *)IOSTAT_IO_TIME, DCI_DT_FLOAT, DCIDESC_SYSTEM_IO_DISKTIME_EX },
   { _T("System.Memory.Physical.Free"), H_MemoryInfo, (const TCHAR *)MEMINFO_PHYSICAL_FREE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE },
   { _T("System.Memory.Physical.FreePerc"), H_MemoryInfo, (const TCHAR *)MEMINFO_PHYSICAL_FREEPCT, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE_PCT },
   { _T("System.Memory.Physical.Total"), H_MemoryInfo, (const TCHAR *)MEMINFO_PHYSICAL_TOTAL, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_TOTAL },
   { _T("System.Memory.Physical.Used"), H_MemoryInfo, (const TCHAR *)MEMINFO_PHYSICAL_USED, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED },
   { _T("System.Memory.Physical.UsedPerc"), H_MemoryInfo, (const TCHAR *)MEMINFO_PHYSICAL_USEDPCT, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED_PCT },
   { _T("System.Memory.Swap.Free"), H_MemoryInfo, (const TCHAR *)MEMINFO_SWAP_FREE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_SWAP_FREE },
   { _T("System.Memory.Swap.FreePerc"), H_MemoryInfo, (const TCHAR *)MEMINFO_SWAP_FREEPCT, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_SWAP_FREE_PCT },
   { _T("System.Memory.Swap.Total"), H_MemoryInfo, (const TCHAR *)MEMINFO_SWAP_TOTAL, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_SWAP_TOTAL },
   { _T("System.Memory.Swap.Used"), H_MemoryInfo, (const TCHAR *)MEMINFO_SWAP_USED, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_SWAP_USED },
   { _T("System.Memory.Swap.UsedPerc"), H_MemoryInfo, (const TCHAR *)MEMINFO_SWAP_USEDPCT, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_SWAP_USED_PCT },
   { _T("System.Memory.Virtual.Free"), H_MemoryInfo, (const TCHAR *)MEMINFO_VIRTUAL_FREE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_VIRTUAL_FREE },
   { _T("System.Memory.Virtual.FreePerc"), H_MemoryInfo, (const TCHAR *)MEMINFO_VIRTUAL_FREEPCT, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_VIRTUAL_FREE_PCT },
   { _T("System.Memory.Virtual.Total"), H_MemoryInfo, (const TCHAR *)MEMINFO_VIRTUAL_TOTAL, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_VIRTUAL_TOTAL },
   { _T("System.Memory.Virtual.Used"), H_MemoryInfo, (const TCHAR *)MEMINFO_VIRTUAL_USED, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_VIRTUAL_USED },
   { _T("System.Memory.Virtual.UsedPerc"), H_MemoryInfo, (const TCHAR *)MEMINFO_VIRTUAL_USEDPCT, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_VIRTUAL_USED_PCT },
   { _T("System.ProcessCount"), H_SysProcCount, NULL, DCI_DT_INT, DCIDESC_SYSTEM_PROCESSCOUNT },
   { _T("System.Uname"), H_Uname, NULL, DCI_DT_STRING, DCIDESC_SYSTEM_UNAME },
   { _T("System.Uptime"), H_Uptime, NULL, DCI_DT_UINT, DCIDESC_SYSTEM_UPTIME }
};

/**
 * Subagent's lists
 */
static NETXMS_SUBAGENT_LIST m_lists[] =
{
   { _T("FileSystem.MountPoints"), H_MountPoints, NULL },
   { _T("Net.InterfaceList"), H_NetIfList, NULL },
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
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO m_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("SUNOS"), NETXMS_VERSION_STRING,
   SubAgentInit, // init handler
   SubAgentShutdown, // unload handler
   NULL, // command handler
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
DECLARE_SUBAGENT_ENTRY_POINT(SUNOS)
{
   *ppInfo = &m_info;
   return TRUE;
}

/**
 * Entry points for server
 */
extern "C" BOOL __NxSubAgentGetIfList(StringList *pValue)
{
   return H_NetIfList(_T("Net.InterfaceList"), NULL, pValue) == SYSINFO_RC_SUCCESS;
}
