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

static void SubAgentShutdown()
{
	ShutdownCpuUsageCollector();
	ShutdownIoStatCollector();
	StopDrbdCollector();
}


//
// Externals
//

LONG H_DRBDDeviceList(const TCHAR *pszParam, const TCHAR *pszArg, StringList *pValue);
LONG H_DRBDDeviceInfo(const TCHAR *pszCmd, const TCHAR *pArg, TCHAR *pValue);
LONG H_DRBDVersion(const TCHAR *pszCmd, const TCHAR *pArg, TCHAR *pValue);
LONG H_PhysicalDiskInfo(const TCHAR *pszParam, const TCHAR *pszArg, TCHAR *pValue);


//
// Subagent information
//

static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
	{ _T("Disk.Avail(*)"),                H_DiskInfo,        (TCHAR *)DISK_AVAIL,
		DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
	{ _T("Disk.AvailPerc(*)"),            H_DiskInfo,        (TCHAR *)DISK_AVAIL_PERC,
		DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
	{ _T("Disk.Free(*)"),                 H_DiskInfo,        (TCHAR *)DISK_FREE,
		DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
	{ _T("Disk.FreePerc(*)"),             H_DiskInfo,        (TCHAR *)DISK_FREE_PERC,
		DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
	{ _T("Disk.Total(*)"),                H_DiskInfo,        (TCHAR *)DISK_TOTAL,
		DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
	{ _T("Disk.Used(*)"),                 H_DiskInfo,        (TCHAR *)DISK_USED,
		DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
	{ _T("Disk.UsedPerc(*)"),             H_DiskInfo,        (TCHAR *)DISK_USED_PERC,
		DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },

	{ _T("FileSystem.Avail(*)"),                H_DiskInfo,        (TCHAR *)DISK_AVAIL,
		DCI_DT_UINT64,	DCIDESC_FS_AVAIL },
	{ _T("FileSystem.AvailPerc(*)"),            H_DiskInfo,        (TCHAR *)DISK_AVAIL_PERC,
		DCI_DT_FLOAT,	DCIDESC_FS_AVAILPERC },
	{ _T("FileSystem.Free(*)"),                 H_DiskInfo,        (TCHAR *)DISK_FREE,
		DCI_DT_UINT64,	DCIDESC_FS_FREE },
	{ _T("FileSystem.FreePerc(*)"),             H_DiskInfo,        (TCHAR *)DISK_FREE_PERC,
		DCI_DT_FLOAT,	DCIDESC_FS_FREEPERC },
	{ _T("FileSystem.Total(*)"),                H_DiskInfo,        (TCHAR *)DISK_TOTAL,
		DCI_DT_UINT64,	DCIDESC_FS_TOTAL },
	{ _T("FileSystem.Used(*)"),                 H_DiskInfo,        (TCHAR *)DISK_USED,
		DCI_DT_UINT64,	DCIDESC_FS_USED },
	{ _T("FileSystem.UsedPerc(*)"),             H_DiskInfo,        (TCHAR *)DISK_USED_PERC,
		DCI_DT_FLOAT,	DCIDESC_FS_USEDPERC },

	{ _T("DRBD.ConnState(*)"),            H_DRBDDeviceInfo,  _T("c"),
		DCI_DT_STRING, "Connection state of DRBD device {instance}" },
	{ _T("DRBD.DataState(*)"),            H_DRBDDeviceInfo,  _T("d"),
		DCI_DT_STRING, "Data state of DRBD device {instance}" },
	{ _T("DRBD.DeviceState(*)"),          H_DRBDDeviceInfo,  _T("s"),
		DCI_DT_STRING, "State of DRBD device {instance}" },
	{ _T("DRBD.PeerDataState(*)"),        H_DRBDDeviceInfo,  _T("D"),
		DCI_DT_STRING, "Data state of DRBD peer device {instance}" },
	{ _T("DRBD.PeerDeviceState(*)"),      H_DRBDDeviceInfo,  _T("S"),
		DCI_DT_STRING, "State of DRBD peer device {instance}" },
	{ _T("DRBD.Protocol(*)"),             H_DRBDDeviceInfo,  _T("p"),
		DCI_DT_STRING, "Protocol type used by DRBD device {instance}" },
	{ _T("DRBD.Version.API"),             H_DRBDVersion,     _T("a"),
		DCI_DT_STRING, "DRBD API version" },
	{ _T("DRBD.Version.Driver"),          H_DRBDVersion,     _T("v"),
		DCI_DT_STRING, "DRBD driver version" },
	{ _T("DRBD.Version.Protocol"),        H_DRBDVersion,     _T("p"),
		DCI_DT_STRING, "DRBD protocol version" },

	{ _T("Net.Interface.AdminStatus(*)"), H_NetIfInfoFromIOCTL, (TCHAR *)IF_INFO_ADMIN_STATUS,
		DCI_DT_INT,		DCIDESC_NET_INTERFACE_ADMINSTATUS },
	{ _T("Net.Interface.BytesIn(*)"),     H_NetIfInfoFromProc, (TCHAR *)IF_INFO_BYTES_IN,
		DCI_DT_UINT,	DCIDESC_NET_INTERFACE_BYTESIN },
	{ _T("Net.Interface.BytesOut(*)"),    H_NetIfInfoFromProc, (TCHAR *)IF_INFO_BYTES_OUT,
		DCI_DT_UINT,	DCIDESC_NET_INTERFACE_BYTESOUT },
	{ _T("Net.Interface.Description(*)"), H_NetIfInfoFromIOCTL, (TCHAR *)IF_INFO_DESCRIPTION,
		DCI_DT_STRING,	DCIDESC_NET_INTERFACE_DESCRIPTION },
	{ _T("Net.Interface.InErrors(*)"),    H_NetIfInfoFromProc, (TCHAR *)IF_INFO_IN_ERRORS,
		DCI_DT_UINT,	DCIDESC_NET_INTERFACE_INERRORS },
	{ _T("Net.Interface.Link(*)"),        H_NetIfInfoFromIOCTL, (TCHAR *)IF_INFO_OPER_STATUS,
		DCI_DT_INT,		DCIDESC_NET_INTERFACE_LINK },
	{ _T("Net.Interface.OutErrors(*)"),   H_NetIfInfoFromProc, (TCHAR *)IF_INFO_OUT_ERRORS,
		DCI_DT_UINT,	DCIDESC_NET_INTERFACE_OUTERRORS },
	{ _T("Net.Interface.PacketsIn(*)"),   H_NetIfInfoFromProc, (TCHAR *)IF_INFO_PACKETS_IN,
		DCI_DT_UINT,	DCIDESC_NET_INTERFACE_PACKETSIN },
	{ _T("Net.Interface.PacketsOut(*)"),  H_NetIfInfoFromProc, (TCHAR *)IF_INFO_PACKETS_OUT,
		DCI_DT_UINT,	DCIDESC_NET_INTERFACE_PACKETSOUT },
	{ _T("Net.IP.Forwarding"),            H_NetIpForwarding, (TCHAR *)4,
		DCI_DT_INT,		DCIDESC_NET_IP_FORWARDING },
	{ _T("Net.IP6.Forwarding"),           H_NetIpForwarding, (TCHAR *)6,
		DCI_DT_INT,		DCIDESC_NET_IP6_FORWARDING },

	{ _T("PhysicalDisk.SmartAttr(*)"),    H_PhysicalDiskInfo, _T("A"),
		DCI_DT_STRING,	DCIDESC_PHYSICALDISK_SMARTATTR },
	{ _T("PhysicalDisk.SmartStatus(*)"),  H_PhysicalDiskInfo, _T("S"),
		DCI_DT_INT,		DCIDESC_PHYSICALDISK_SMARTSTATUS },
	{ _T("PhysicalDisk.Temperature(*)"),  H_PhysicalDiskInfo, _T("T"),
		DCI_DT_INT,		DCIDESC_PHYSICALDISK_TEMPERATURE },

	{ _T("Process.Count(*)"),             H_ProcessCount,    _T("S"),
		DCI_DT_UINT,	DCIDESC_PROCESS_COUNT },
	{ _T("Process.CountEx(*)"),           H_ProcessCount,    _T("E"),
		DCI_DT_UINT,	DCIDESC_PROCESS_COUNTEX },
	{ _T("Process.CPUTime(*)"),           H_ProcessDetails,  CAST_TO_POINTER(PROCINFO_CPUTIME, const TCHAR *),
		DCI_DT_INT64,	DCIDESC_PROCESS_CPUTIME },
	{ _T("Process.KernelTime(*)"),        H_ProcessDetails,  CAST_TO_POINTER(PROCINFO_KTIME, const TCHAR *),
		DCI_DT_INT64,	DCIDESC_PROCESS_KERNELTIME },
	{ _T("Process.PageFaults(*)"),        H_ProcessDetails,  CAST_TO_POINTER(PROCINFO_PAGEFAULTS, const TCHAR *),
		DCI_DT_INT64,	DCIDESC_PROCESS_PAGEFAULTS },
	{ _T("Process.Threads(*)"),           H_ProcessDetails,  CAST_TO_POINTER(PROCINFO_THREADS, const TCHAR *),
		DCI_DT_INT64,	DCIDESC_PROCESS_THREADS },
	{ _T("Process.UserTime(*)"),          H_ProcessDetails,  CAST_TO_POINTER(PROCINFO_UTIME, const TCHAR *),
		DCI_DT_INT64,	DCIDESC_PROCESS_USERTIME },
	{ _T("Process.VMSize(*)"),            H_ProcessDetails,  CAST_TO_POINTER(PROCINFO_VMSIZE, const TCHAR *),
		DCI_DT_INT64,	DCIDESC_PROCESS_VMSIZE },
	{ _T("Process.WkSet(*)"),             H_ProcessDetails,  CAST_TO_POINTER(PROCINFO_WKSET, const TCHAR *),
		DCI_DT_INT64,	DCIDESC_PROCESS_WKSET },
	
	{ _T("System.ProcessCount"),          H_ProcessCount,    _T("T"),
		DCI_DT_UINT,	DCIDESC_SYSTEM_PROCESSCOUNT },
	{ _T("System.ThreadCount"),           H_ThreadCount,     NULL,
		DCI_DT_UINT,	DCIDESC_SYSTEM_THREADCOUNT },

	{ _T("System.ConnectedUsers"),        H_ConnectedUsers,  NULL,
		DCI_DT_INT,    DCIDESC_SYSTEM_CONNECTEDUSERS },

	{ _T("System.CPU.Count"),             H_CpuCount,        NULL,
		DCI_DT_UINT,	DCIDESC_SYSTEM_CPU_COUNT },
	{ _T("System.CPU.LoadAvg"),           H_CpuLoad,         (TCHAR *)INTERVAL_1MIN,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_LOADAVG },
	{ _T("System.CPU.LoadAvg5"),          H_CpuLoad,         (TCHAR *)INTERVAL_5MIN,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_LOADAVG5 },
	{ _T("System.CPU.LoadAvg15"),         H_CpuLoad,         (TCHAR *)INTERVAL_15MIN,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_LOADAVG15 },

	/**************************************************************/
	/* usage */
	{ _T("System.CPU.Usage"),             H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_OVERAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE },
	{ _T("System.CPU.Usage5"),            H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_OVERAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5 },
	{ _T("System.CPU.Usage15"),           H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_OVERAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15 },
	{ _T("System.CPU.Usage(*)"),          H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_OVERAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_EX },
	{ _T("System.CPU.Usage5(*)"),         H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_OVERAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_EX },
	{ _T("System.CPU.Usage15(*)"),        H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_OVERAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_EX },

	/* user */
	{ _T("System.CPU.Usage.User"),             H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_USER),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_USER },
	{ _T("System.CPU.Usage5.User"),            H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_USER),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_USER },
	{ _T("System.CPU.Usage15.User"),           H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_USER),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_USER },
	{ _T("System.CPU.Usage.User(*)"),          H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_USER),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_USER_EX },
	{ _T("System.CPU.Usage5.User(*)"),         H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_USER),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_USER_EX },
	{ _T("System.CPU.Usage15.User(*)"),        H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_USER),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_USER_EX },

	/* nice */
	{ _T("System.CPU.Usage.Nice"),             H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_NICE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_NICE },
	{ _T("System.CPU.Usage5.Nice"),            H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_NICE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_NICE },
	{ _T("System.CPU.Usage15.Nice"),           H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_NICE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_NICE },
	{ _T("System.CPU.Usage.Nice(*)"),          H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_NICE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_NICE_EX },
	{ _T("System.CPU.Usage5.Nice(*)"),         H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_NICE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_NICE_EX },
	{ _T("System.CPU.Usage15.Nice(*)"),        H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_NICE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_NICE_EX },

	/* system */
	{ _T("System.CPU.Usage.System"),             H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_SYSTEM),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_SYSTEM },
	{ _T("System.CPU.Usage5.System"),            H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_SYSTEM),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_SYSTEM },
	{ _T("System.CPU.Usage15.System"),           H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_SYSTEM),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_SYSTEM },
	{ _T("System.CPU.Usage.System(*)"),          H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_SYSTEM),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_SYSTEM_EX },
	{ _T("System.CPU.Usage5.System(*)"),         H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_SYSTEM),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_SYSTEM_EX },
	{ _T("System.CPU.Usage15.System(*)"),        H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_SYSTEM),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_SYSTEM_EX },

	/* idle */
	{ _T("System.CPU.Usage.Idle"),             H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_IDLE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_IDLE },
	{ _T("System.CPU.Usage5.Idle"),            H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_IDLE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_IDLE },
	{ _T("System.CPU.Usage15.Idle"),           H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_IDLE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_IDLE },
	{ _T("System.CPU.Usage.Idle(*)"),          H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_IDLE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_IDLE_EX },
	{ _T("System.CPU.Usage5.Idle5(*)"),         H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_IDLE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_IDLE_EX },
	{ _T("System.CPU.Usage15.Idle(*)"),        H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_IDLE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_IDLE_EX },

	/* iowait */
	{ _T("System.CPU.Usage.IoWait"),             H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_IOWAIT),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_IOWAIT },
	{ _T("System.CPU.Usage5.IoWait"),            H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_IOWAIT),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_IOWAIT },
	{ _T("System.CPU.Usage15.IoWait"),           H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_IOWAIT),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_IOWAIT },
	{ _T("System.CPU.Usage.IoWait(*)"),          H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_IOWAIT),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_IOWAIT_EX },
	{ _T("System.CPU.Usage5.IoWait(*)"),         H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_IOWAIT),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_IOWAIT_EX },
	{ _T("System.CPU.Usage15.IoWait(*)"),        H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_IOWAIT),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_IOWAIT_EX },

	/* irq */
	{ _T("System.CPU.Usage.Irq"),             H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_IRQ),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_IRQ },
	{ _T("System.CPU.Usage5.Irq"),            H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_IRQ),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_IRQ },
	{ _T("System.CPU.Usage15.Irq"),           H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_IRQ),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_IRQ },
	{ _T("System.CPU.Usage.Irq(*)"),          H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_IRQ),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_IRQ_EX },
	{ _T("System.CPU.Usage5.Irq(*)"),         H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_IRQ),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_IRQ_EX },
	{ _T("System.CPU.Usage15.Irq(*)"),        H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_IRQ),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_IRQ_EX },

	/* softirq */
	{ _T("System.CPU.Usage.SoftIrq"),             H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_SOFTIRQ),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_SOFTIRQ },
	{ _T("System.CPU.Usage5.SoftIrq"),            H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_SOFTIRQ),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_SOFTIRQ },
	{ _T("System.CPU.Usage15.SoftIrq"),           H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_SOFTIRQ),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_SOFTIRQ },
	{ _T("System.CPU.Usage.SoftIrq(*)"),          H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_SOFTIRQ),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_SOFTIRQ_EX },
	{ _T("System.CPU.Usage5.SoftIrq(*)"),         H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_SOFTIRQ),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_SOFTIRQ_EX },
	{ _T("System.CPU.Usage15.SoftIrq(*)"),        H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_SOFTIRQ),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_SOFTIRQ_EX },

	/* steal */
	{ _T("System.CPU.Usage.Steal"),             H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_STEAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_STEAL },
	{ _T("System.CPU.Usage5.Steal"),            H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_STEAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_STEAL },
	{ _T("System.CPU.Usage15.Steal"),           H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_STEAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_STEAL },
	{ _T("System.CPU.Usage.Steal(*)"),          H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_STEAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_STEAL_EX },
	{ _T("System.CPU.Usage5.Steal(*)"),         H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_STEAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_STEAL_EX },
	{ _T("System.CPU.Usage15.Steal(*)"),        H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_STEAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_STEAL_EX },

	/* Guest */
	{ _T("System.CPU.Usage.Guest"),             H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_GUEST),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_GUEST },
	{ _T("System.CPU.Usage5.Guest"),            H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_GUEST),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_GUEST },
	{ _T("System.CPU.Usage15.Guest"),           H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_GUEST),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_GUEST },
	{ _T("System.CPU.Usage.Guest(*)"),          H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_GUEST),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_GUEST_EX },
	{ _T("System.CPU.Usage5.Guest(*)"),         H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_GUEST),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_GUEST_EX },
	{ _T("System.CPU.Usage15.Guest(*)"),        H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_GUEST),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_GUEST_EX },

	/**************************************************************/
	{ _T("System.Hostname"),              H_Hostname,        NULL,
		DCI_DT_STRING,	DCIDESC_SYSTEM_HOSTNAME },
	{ _T("System.Memory.Physical.Free"),  H_MemoryInfo,      (TCHAR *)PHYSICAL_FREE,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE },
	{ _T("System.Memory.Physical.FreePerc"), H_MemoryInfo,   (TCHAR *)PHYSICAL_FREE_PCT,
		DCI_DT_UINT,	DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE_PCT },
	{ _T("System.Memory.Physical.Total"), H_MemoryInfo,      (TCHAR *)PHYSICAL_TOTAL,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_PHYSICAL_TOTAL },
	{ _T("System.Memory.Physical.Used"),  H_MemoryInfo,      (TCHAR *)PHYSICAL_USED,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED },
	{ _T("System.Memory.Physical.UsedPerc"), H_MemoryInfo,   (TCHAR *)PHYSICAL_USED_PCT,
		DCI_DT_UINT,	DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED_PCT },
	{ _T("System.Memory.Physical.Available"),  H_MemoryInfo,      (TCHAR *)PHYSICAL_AVAILABLE,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_PHYSICAL_AVAILABLE },
	{ _T("System.Memory.Physical.AvailablePerc"), H_MemoryInfo,   (TCHAR *)PHYSICAL_AVAILABLE_PCT,
		DCI_DT_UINT,	DCIDESC_SYSTEM_MEMORY_PHYSICAL_AVAILABLE_PCT },
	{ _T("System.Memory.Swap.Free"),      H_MemoryInfo,      (TCHAR *)SWAP_FREE,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_SWAP_FREE },
	{ _T("System.Memory.Swap.FreePerc"),  H_MemoryInfo,      (TCHAR *)SWAP_FREE_PCT,
		DCI_DT_UINT,	DCIDESC_SYSTEM_MEMORY_SWAP_FREE_PCT },
	{ _T("System.Memory.Swap.Total"),     H_MemoryInfo,      (TCHAR *)SWAP_TOTAL,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_SWAP_TOTAL },
	{ _T("System.Memory.Swap.Used"),      H_MemoryInfo,      (TCHAR *)SWAP_USED,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_SWAP_USED },
	{ _T("System.Memory.Swap.UsedPerc"),      H_MemoryInfo,  (TCHAR *)SWAP_USED_PCT,
		DCI_DT_UINT,	DCIDESC_SYSTEM_MEMORY_SWAP_USED_PCT },
	{ _T("System.Memory.Virtual.Free"),   H_MemoryInfo,      (TCHAR *)VIRTUAL_FREE,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_VIRTUAL_FREE },
	{ _T("System.Memory.Virtual.FreePerc"), H_MemoryInfo,    (TCHAR *)VIRTUAL_FREE_PCT,
		DCI_DT_UINT,	DCIDESC_SYSTEM_MEMORY_VIRTUAL_FREE_PCT },
	{ _T("System.Memory.Virtual.Total"),  H_MemoryInfo,      (TCHAR *)VIRTUAL_TOTAL,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_VIRTUAL_TOTAL },
	{ _T("System.Memory.Virtual.Used"),   H_MemoryInfo,      (TCHAR *)VIRTUAL_USED,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_VIRTUAL_USED },
	{ _T("System.Memory.Virtual.UsedPerc"), H_MemoryInfo,    (TCHAR *)VIRTUAL_USED_PCT,
		DCI_DT_UINT,	DCIDESC_SYSTEM_MEMORY_VIRTUAL_USED_PCT },
	{ _T("System.Memory.Virtual.Available"),   H_MemoryInfo,      (TCHAR *)VIRTUAL_AVAILABLE,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_MEMORY_VIRTUAL_AVAILABLE },
	{ _T("System.Memory.Virtual.AvailablePerc"), H_MemoryInfo,    (TCHAR *)VIRTUAL_AVAILABLE_PCT,
		DCI_DT_UINT,	DCIDESC_SYSTEM_MEMORY_VIRTUAL_AVAILABLE_PCT },
	{ _T("System.Uname"),                 H_Uname,           NULL,
		DCI_DT_STRING,	DCIDESC_SYSTEM_UNAME },
	{ _T("System.Uptime"),                H_Uptime,          NULL,
		DCI_DT_UINT,	DCIDESC_SYSTEM_UPTIME },

	{ _T("Agent.SourcePackageSupport"),   H_SourcePkgSupport,NULL,
		DCI_DT_INT,		DCIDESC_AGENT_SOURCEPACKAGESUPPORT },

	/* iostat */
	{ _T("System.IO.ReadRate"),           H_IoStatsTotal, (const TCHAR *)IOSTAT_NUM_READS,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_IO_READS },
	{ _T("System.IO.ReadRate(*)"),        H_IoStats, (const TCHAR *)IOSTAT_NUM_READS,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_IO_READS_EX },
	{ _T("System.IO.WriteRate"),          H_IoStatsTotal, (const TCHAR *)IOSTAT_NUM_WRITES,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_IO_WRITES },
	{ _T("System.IO.WriteRate(*)"),       H_IoStats, (const TCHAR *)IOSTAT_NUM_WRITES,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_IO_WRITES_EX },
	{ _T("System.IO.BytesReadRate"),      H_IoStatsTotal, (const TCHAR *)IOSTAT_NUM_SREADS,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_IO_BYTEREADS },
	{ _T("System.IO.BytesReadRate(*)"),   H_IoStats, (const TCHAR *)IOSTAT_NUM_SREADS,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_IO_BYTEREADS_EX },
	{ _T("System.IO.BytesWriteRate"),     H_IoStatsTotal, (const TCHAR *)IOSTAT_NUM_SWRITES,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_IO_BYTEWRITES },
	{ _T("System.IO.BytesWriteRate(*)"),  H_IoStats, (const TCHAR *)IOSTAT_NUM_SWRITES,
		DCI_DT_UINT64,	DCIDESC_SYSTEM_IO_BYTEWRITES_EX },
	{ _T("System.IO.DiskQueue(*)"),       H_DiskQueue,       NULL,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_IO_DISKQUEUE_EX },
	{ _T("System.IO.DiskQueue"),          H_DiskQueueTotal,  NULL,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_IO_DISKQUEUE },
	{ _T("System.IO.DiskTime"),           H_IoStatsTotal, (const TCHAR *)IOSTAT_IO_TIME,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_IO_DISKTIME },
	{ _T("System.IO.DiskTime(*)"),        H_IoStats, (const TCHAR *)IOSTAT_IO_TIME,
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_IO_DISKTIME_EX },
};

static NETXMS_SUBAGENT_LIST m_enums[] =
{
	{ _T("DRBD.DeviceList"),              H_DRBDDeviceList,     NULL },
	{ _T("Net.ArpCache"),                 H_NetArpCache,        NULL },
	{ _T("Net.IP.RoutingTable"),          H_NetRoutingTable,    NULL },
	{ _T("Net.InterfaceList"),            H_NetIfList,          NULL },
	{ _T("System.ActiveUserSessions"),    H_ActiveUserSessions, NULL },
	{ _T("System.ProcessList"),           H_ProcessList,        NULL },
};

static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("Linux"),
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
	return H_NetIfList(_T("Net.InterfaceList"), NULL, pValue) == SYSINFO_RC_SUCCESS;
}

extern "C" BOOL __NxSubAgentGetArpCache(StringList *pValue)
{
	return H_NetArpCache(_T("Net.ArpCache"), NULL, pValue) == SYSINFO_RC_SUCCESS;
}
