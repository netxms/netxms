/* 
** NetXMS subagent for GNU/Linux
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
** File: linux.cpp
**
**/

#include "linux_subagent.h"


//
// Initalization callback
//

static BOOL SubAgentInit(Config *config)
{
	StartCpuUsageCollector();
	StartIoStatCollector();
	InitDrbdCollector();
	return TRUE;
}


//
// Shutdown callback
//

static void SubAgentShutdown(void)
{
	ShutdownCpuUsageCollector();
	ShutdownIoStatCollector();
	StopDrbdCollector();
}


//
// Externals
//

LONG H_DRBDDeviceList(const char *pszParam, const char *pszArg, StringList *pValue);
LONG H_DRBDDeviceInfo(const TCHAR *pszCmd, const TCHAR *pArg, TCHAR *pValue);
LONG H_DRBDVersion(const TCHAR *pszCmd, const TCHAR *pArg, TCHAR *pValue);
LONG H_PhysicalDiskInfo(const char *pszParam, const char *pszArg, char *pValue);


//
// Subagent information
//

static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
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

	{ "FileSystem.Avail(*)",                H_DiskInfo,        (char *)DISK_AVAIL,
		DCI_DT_UINT64,	DCIDESC_FS_AVAIL },
	{ "FileSystem.AvailPerc(*)",            H_DiskInfo,        (char *)DISK_AVAIL_PERC,
		DCI_DT_FLOAT,	DCIDESC_FS_AVAILPERC },
	{ "FileSystem.Free(*)",                 H_DiskInfo,        (char *)DISK_FREE,
		DCI_DT_UINT64,	DCIDESC_FS_FREE },
	{ "FileSystem.FreePerc(*)",             H_DiskInfo,        (char *)DISK_FREE_PERC,
		DCI_DT_FLOAT,	DCIDESC_FS_FREEPERC },
	{ "FileSystem.Total(*)",                H_DiskInfo,        (char *)DISK_TOTAL,
		DCI_DT_UINT64,	DCIDESC_FS_TOTAL },
	{ "FileSystem.Used(*)",                 H_DiskInfo,        (char *)DISK_USED,
		DCI_DT_UINT64,	DCIDESC_FS_USED },
	{ "FileSystem.UsedPerc(*)",             H_DiskInfo,        (char *)DISK_USED_PERC,
		DCI_DT_FLOAT,	DCIDESC_FS_USEDPERC },

	{ "DRBD.ConnState(*)",            H_DRBDDeviceInfo,  "c",
		DCI_DT_STRING, "Connection state of DRBD device {instance}" },
	{ "DRBD.DataState(*)",            H_DRBDDeviceInfo,  "d",
		DCI_DT_STRING, "Data state of DRBD device {instance}" },
	{ "DRBD.DeviceState(*)",          H_DRBDDeviceInfo,  "s",
		DCI_DT_STRING, "State of DRBD device {instance}" },
	{ "DRBD.PeerDataState(*)",        H_DRBDDeviceInfo,  "D",
		DCI_DT_STRING, "Data state of DRBD peer device {instance}" },
	{ "DRBD.PeerDeviceState(*)",      H_DRBDDeviceInfo,  "S",
		DCI_DT_STRING, "State of DRBD peer device {instance}" },
	{ "DRBD.Protocol(*)",             H_DRBDDeviceInfo,  "p",
		DCI_DT_STRING, "Protocol type used by DRBD device {instance}" },
	{ "DRBD.Version.API",             H_DRBDVersion,     "a",
		DCI_DT_STRING, "DRBD API version" },
	{ "DRBD.Version.Driver",          H_DRBDVersion,     "v",
		DCI_DT_STRING, "DRBD driver version" },
	{ "DRBD.Version.Protocol",        H_DRBDVersion,     "p",
		DCI_DT_STRING, "DRBD protocol version" },

	{ "Net.Interface.AdminStatus(*)", H_NetIfInfoFromIOCTL, (char *)IF_INFO_ADMIN_STATUS,
		DCI_DT_INT,		DCIDESC_NET_INTERFACE_ADMINSTATUS },
	{ "Net.Interface.BytesIn(*)",     H_NetIfInfoFromProc, (char *)IF_INFO_BYTES_IN,
		DCI_DT_UINT,	DCIDESC_NET_INTERFACE_BYTESIN },
	{ "Net.Interface.BytesOut(*)",    H_NetIfInfoFromProc, (char *)IF_INFO_BYTES_OUT,
		DCI_DT_UINT,	DCIDESC_NET_INTERFACE_BYTESOUT },
	{ "Net.Interface.Description(*)", H_NetIfInfoFromIOCTL, (char *)IF_INFO_DESCRIPTION,
		DCI_DT_STRING,	DCIDESC_NET_INTERFACE_DESCRIPTION },
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
		DCI_DT_INT,		DCIDESC_NET_IP6_FORWARDING },

	{ "PhysicalDisk.SmartAttr(*)",    H_PhysicalDiskInfo, "A",
		DCI_DT_STRING,	DCIDESC_PHYSICALDISK_SMARTATTR },
	{ "PhysicalDisk.SmartStatus(*)",  H_PhysicalDiskInfo, "S",
		DCI_DT_INT,		DCIDESC_PHYSICALDISK_SMARTSTATUS },
	{ "PhysicalDisk.Temperature(*)",  H_PhysicalDiskInfo, "T",
		DCI_DT_INT,		DCIDESC_PHYSICALDISK_TEMPERATURE },

	{ "Process.Count(*)",             H_ProcessCount,    "S",
		DCI_DT_UINT,	DCIDESC_PROCESS_COUNT },
	{ "Process.CountEx(*)",           H_ProcessCount,    "E",
		DCI_DT_UINT,	DCIDESC_PROCESS_COUNTEX },
	{ "Process.CPUTime(*)",           H_ProcessDetails,  CAST_TO_POINTER(PROCINFO_CPUTIME, const char *),
		DCI_DT_INT64,	DCIDESC_PROCESS_CPUTIME },
	{ "Process.KernelTime(*)",        H_ProcessDetails,  CAST_TO_POINTER(PROCINFO_KTIME, const char *),
		DCI_DT_INT64,	DCIDESC_PROCESS_KERNELTIME },
	{ "Process.PageFaults(*)",        H_ProcessDetails,  CAST_TO_POINTER(PROCINFO_PAGEFAULTS, const char *),
		DCI_DT_INT64,	DCIDESC_PROCESS_PAGEFAULTS },
	{ "Process.Threads(*)",           H_ProcessDetails,  CAST_TO_POINTER(PROCINFO_THREADS, const char *),
		DCI_DT_INT64,	DCIDESC_PROCESS_THREADS },
	{ "Process.UserTime(*)",          H_ProcessDetails,  CAST_TO_POINTER(PROCINFO_UTIME, const char *),
		DCI_DT_INT64,	DCIDESC_PROCESS_USERTIME },
	{ "Process.VMSize(*)",            H_ProcessDetails,  CAST_TO_POINTER(PROCINFO_VMSIZE, const char *),
		DCI_DT_INT64,	DCIDESC_PROCESS_VMSIZE },
	{ "Process.WkSet(*)",             H_ProcessDetails,  CAST_TO_POINTER(PROCINFO_WKSET, const char *),
		DCI_DT_INT64,	DCIDESC_PROCESS_WKSET },
	
	{ "System.ProcessCount",          H_ProcessCount,    "T",
		DCI_DT_UINT,	DCIDESC_SYSTEM_PROCESSCOUNT },
	{ "System.ThreadCount",           H_ThreadCount,     NULL,
		DCI_DT_UINT,	DCIDESC_SYSTEM_THREADCOUNT },

	{ "System.ConnectedUsers",        H_ConnectedUsers,  NULL,
		DCI_DT_INT,    DCIDESC_SYSTEM_CONNECTEDUSERS },

	{ "System.CPU.Count",             H_CpuCount,        NULL,
		DCI_DT_UINT,	DCIDESC_SYSTEM_CPU_COUNT },
	{ "System.CPU.LoadAvg",           H_CpuLoad,         (char *)INTERVAL_1MIN,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_LOADAVG },
	{ "System.CPU.LoadAvg5",          H_CpuLoad,         (char *)INTERVAL_5MIN,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_LOADAVG5 },
	{ "System.CPU.LoadAvg15",         H_CpuLoad,         (char *)INTERVAL_15MIN,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_LOADAVG15 },

	/**************************************************************/
	/* usage */
	{ "System.CPU.Usage",             H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_OVERAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE },
	{ "System.CPU.Usage5",            H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_OVERAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5 },
	{ "System.CPU.Usage15",           H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_OVERAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15 },
	{ "System.CPU.Usage(*)",          H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_OVERAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_EX },
	{ "System.CPU.Usage5(*)",         H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_OVERAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_EX },
	{ "System.CPU.Usage15(*)",        H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_OVERAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_EX },

	/* user */
	{ "System.CPU.Usage.User",             H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_USER),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_USER },
	{ "System.CPU.Usage5.User",            H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_USER),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_USER },
	{ "System.CPU.Usage15.User",           H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_USER),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_USER },
	{ "System.CPU.Usage.User(*)",          H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_USER),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_USER_EX },
	{ "System.CPU.Usage5.User(*)",         H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_USER),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_USER_EX },
	{ "System.CPU.Usage15.User(*)",        H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_USER),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_USER_EX },

	/* nice */
	{ "System.CPU.Usage.Nice",             H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_NICE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_NICE },
	{ "System.CPU.Usage5.Nice",            H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_NICE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_NICE },
	{ "System.CPU.Usage15.Nice",           H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_NICE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_NICE },
	{ "System.CPU.Usage.Nice(*)",          H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_NICE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_NICE_EX },
	{ "System.CPU.Usage5.Nice(*)",         H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_NICE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_NICE_EX },
	{ "System.CPU.Usage15.Nice(*)",        H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_NICE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_NICE_EX },

	/* system */
	{ "System.CPU.Usage.System",             H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_SYSTEM),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_SYSTEM },
	{ "System.CPU.Usage5.System",            H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_SYSTEM),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_SYSTEM },
	{ "System.CPU.Usage15.System",           H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_SYSTEM),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_SYSTEM },
	{ "System.CPU.Usage.System(*)",          H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_SYSTEM),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_SYSTEM_EX },
	{ "System.CPU.Usage5.System(*)",         H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_SYSTEM),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_SYSTEM_EX },
	{ "System.CPU.Usage15.System(*)",        H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_SYSTEM),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_SYSTEM_EX },

	/* idle */
	{ "System.CPU.Usage.Idle",             H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_IDLE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_IDLE },
	{ "System.CPU.Usage5.Idle",            H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_IDLE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_IDLE },
	{ "System.CPU.Usage15.Idle",           H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_IDLE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_IDLE },
	{ "System.CPU.Usage.Idle(*)",          H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_IDLE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_IDLE_EX },
	{ "System.CPU.Usage5.Idle5(*)",         H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_IDLE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_IDLE_EX },
	{ "System.CPU.Usage15.Idle(*)",        H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_IDLE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_IDLE_EX },

	/* iowait */
	{ "System.CPU.Usage.IoWait",             H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_IOWAIT),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_IOWAIT },
	{ "System.CPU.Usage5.IoWait",            H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_IOWAIT),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_IOWAIT },
	{ "System.CPU.Usage15.IoWait",           H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_IOWAIT),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_IOWAIT },
	{ "System.CPU.Usage.IoWait(*)",          H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_IOWAIT),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_IOWAIT_EX },
	{ "System.CPU.Usage5.IoWait(*)",         H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_IOWAIT),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_IOWAIT_EX },
	{ "System.CPU.Usage15.IoWait(*)",        H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_IOWAIT),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_IOWAIT_EX },

	/* irq */
	{ "System.CPU.Usage.Irq",             H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_IRQ),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_IRQ },
	{ "System.CPU.Usage5.Irq",            H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_IRQ),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_IRQ },
	{ "System.CPU.Usage15.Irq",           H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_IRQ),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_IRQ },
	{ "System.CPU.Usage.Irq(*)",          H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_IRQ),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_IRQ_EX },
	{ "System.CPU.Usage5.Irq(*)",         H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_IRQ),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_IRQ_EX },
	{ "System.CPU.Usage15.Irq(*)",        H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_IRQ),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_IRQ_EX },

	/* softirq */
	{ "System.CPU.Usage.SoftIrq",             H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_SOFTIRQ),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_SOFTIRQ },
	{ "System.CPU.Usage5.SoftIrq",            H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_SOFTIRQ),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_SOFTIRQ },
	{ "System.CPU.Usage15.SoftIrq",           H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_SOFTIRQ),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_SOFTIRQ },
	{ "System.CPU.Usage.SoftIrq(*)",          H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_SOFTIRQ),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_SOFTIRQ_EX },
	{ "System.CPU.Usage5.SoftIrq(*)",         H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_SOFTIRQ),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_SOFTIRQ_EX },
	{ "System.CPU.Usage15.SoftIrq(*)",        H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_SOFTIRQ),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_SOFTIRQ_EX },

	/* steal */
	{ "System.CPU.Usage.Steal",             H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_STEAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_STEAL },
	{ "System.CPU.Usage5.Steal",            H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_STEAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_STEAL },
	{ "System.CPU.Usage15.Steal",           H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_STEAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_STEAL },
	{ "System.CPU.Usage.Steal(*)",          H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_STEAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_STEAL_EX },
	{ "System.CPU.Usage5.Steal(*)",         H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_STEAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_STEAL_EX },
	{ "System.CPU.Usage15.Steal(*)",        H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_STEAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_STEAL_EX },

	/* Guest */
	{ "System.CPU.Usage.Guest",             H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_GUEST),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_GUEST },
	{ "System.CPU.Usage5.Guest",            H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_GUEST),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_GUEST },
	{ "System.CPU.Usage15.Guest",           H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_GUEST),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_GUEST },
	{ "System.CPU.Usage.Guest(*)",          H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_GUEST),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_GUEST_EX },
	{ "System.CPU.Usage5.Guest(*)",         H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_GUEST),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_GUEST_EX },
	{ "System.CPU.Usage15.Guest(*)",        H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_GUEST),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_GUEST_EX },

	/**************************************************************/
	{ "System.Hostname",              H_Hostname,        NULL,
		DCI_DT_STRING,	DCIDESC_SYSTEM_HOSTNAME },
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
	{ "System.Memory.Physical.Available",  H_MemoryInfo,      (char *)PHYSICAL_AVAILABLE,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_PHYSICAL_AVAILABLE },
	{ "System.Memory.Physical.AvailablePerc", H_MemoryInfo,   (char *)PHYSICAL_AVAILABLE_PCT,
		DCI_DT_UINT,	DCIDESC_SYSTEM_MEMORY_PHYSICAL_AVAILABLE_PCT },
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
	{ "System.Memory.Virtual.Available",   H_MemoryInfo,      (char *)VIRTUAL_AVAILABLE,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_VIRTUAL_AVAILABLE },
	{ "System.Memory.Virtual.AvailablePerc", H_MemoryInfo,    (char *)VIRTUAL_AVAILABLE_PCT,
		DCI_DT_UINT,	DCIDESC_SYSTEM_MEMORY_VIRTUAL_AVAILABLE_PCT },
	{ "System.Uname",                 H_Uname,           NULL,
		DCI_DT_STRING,	DCIDESC_SYSTEM_UNAME },
	{ "System.Uptime",                H_Uptime,          NULL,
		DCI_DT_UINT,	DCIDESC_SYSTEM_UPTIME },

	{ "Agent.SourcePackageSupport",   H_SourcePkgSupport,NULL,
		DCI_DT_INT,		DCIDESC_AGENT_SOURCEPACKAGESUPPORT },

	/* iostat */
	{ "System.IO.ReadRate",           H_IoStatsTotal, (const char *)IOSTAT_NUM_READS,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_IO_READS },
	{ "System.IO.ReadRate(*)",        H_IoStats, (const char *)IOSTAT_NUM_READS,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_IO_READS_EX },
	{ "System.IO.WriteRate",          H_IoStatsTotal, (const char *)IOSTAT_NUM_WRITES,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_IO_WRITES },
	{ "System.IO.WriteRate(*)",       H_IoStats, (const char *)IOSTAT_NUM_WRITES,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_IO_WRITES_EX },
	{ "System.IO.BytesReadRate",      H_IoStatsTotal, (const char *)IOSTAT_NUM_SREADS,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_IO_BYTEREADS },
	{ "System.IO.BytesReadRate(*)",   H_IoStats, (const char *)IOSTAT_NUM_SREADS,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_IO_BYTEREADS_EX },
	{ "System.IO.BytesWriteRate",     H_IoStatsTotal, (const char *)IOSTAT_NUM_SWRITES,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_IO_BYTEWRITES },
	{ "System.IO.BytesWriteRate(*)",  H_IoStats, (const char *)IOSTAT_NUM_SWRITES,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_IO_BYTEWRITES_EX },
	{ "System.IO.DiskQueue(*)",       H_DiskQueue,       NULL,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_IO_DISKQUEUE_EX },
	{ "System.IO.DiskQueue",          H_DiskQueueTotal,  NULL,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_IO_DISKQUEUE },
	{ "System.IO.DiskTime",           H_IoStatsTotal, (const char *)IOSTAT_IO_TIME,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_IO_DISKTIME },
	{ "System.IO.DiskTime(*)",        H_IoStats, (const char *)IOSTAT_IO_TIME,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_IO_DISKTIME_EX },
};

static NETXMS_SUBAGENT_LIST m_enums[] =
{
	{ "DRBD.DeviceList",              H_DRBDDeviceList,     NULL },
	{ "Net.ArpCache",                 H_NetArpCache,        NULL },
	{ "Net.IP.RoutingTable",          H_NetRoutingTable,    NULL },
	{ "Net.InterfaceList",            H_NetIfList,          NULL },
	{ "System.ActiveUserSessions",    H_ActiveUserSessions, NULL },
	{ "System.ProcessList",           H_ProcessList,        NULL },
};

static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	"Linux",
	NETXMS_VERSION_STRING,
	SubAgentInit,     /* initialization handler */
	SubAgentShutdown, /* unload handler */
	NULL,             /* command handler */
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

DECLARE_SUBAGENT_ENTRY_POINT(LINUX)
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
