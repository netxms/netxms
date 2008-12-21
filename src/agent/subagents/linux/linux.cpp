/* $Id$ */

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

#include <nms_common.h>
#include <nms_agent.h>

#include "net.h"
#include "system.h"
#include "disk.h"
#include "iostat.h"

//
// Initalization callback
//

static BOOL SubAgentInit(TCHAR *pszConfigFile)
{
	StartCpuUsageCollector();
	return TRUE;
}


//
// Shutdown callback
//

static void SubAgentShutdown(void)
{
	ShutdownCpuUsageCollector();
}


//
// Externals
//

LONG H_DRBDDeviceList(const char *pszParam, const char *pszArg, NETXMS_VALUES_LIST *pValue);
LONG H_DRBDDeviceInfo(const TCHAR *pszCmd, const TCHAR *pArg, TCHAR *pValue);
LONG H_PhysicalDiskInfo(const char *pszParam, const char *pszArg, char *pValue);


//
// Subagent information
//

static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
	{ "Disk.Avail(*)",                H_DiskInfo,        (char *)DISK_AVAIL,
		DCI_DT_UINT64,	DCIDESC_DISK_AVAIL },
	{ "Disk.AvailPerc(*)",            H_DiskInfo,        (char *)DISK_AVAIL_PERC,
		DCI_DT_FLOAT,	DCIDESC_DISK_AVAILPERC },
	{ "Disk.Free(*)",                 H_DiskInfo,        (char *)DISK_FREE,
		DCI_DT_UINT64,	DCIDESC_DISK_FREE },
	{ "Disk.FreePerc(*)",             H_DiskInfo,        (char *)DISK_FREE_PERC,
		DCI_DT_FLOAT,	DCIDESC_DISK_FREEPERC },
	{ "Disk.Total(*)",                H_DiskInfo,        (char *)DISK_TOTAL,
		DCI_DT_UINT64,	DCIDESC_DISK_TOTAL },
	{ "Disk.Used(*)",                 H_DiskInfo,        (char *)DISK_USED,
		DCI_DT_UINT64,	DCIDESC_DISK_USED },
	{ "Disk.UsedPerc(*)",             H_DiskInfo,        (char *)DISK_USED_PERC,
		DCI_DT_FLOAT,	DCIDESC_DISK_USEDPERC },

	{ "DRBD.ConnState(*)",            H_DRBDDeviceInfo,  "c",
		DCI_DT_INT,    "Connection state of DRBD device {instance}" },
	{ "DRBD.ConnStateText(*)",        H_DRBDDeviceInfo,  "C",
		DCI_DT_STRING, "Connection state of DRBD device {instance} as text" },
	{ "DRBD.LowerDevice(*)",          H_DRBDDeviceInfo,  "L",
		DCI_DT_STRING, "Underlaying device of DRBD device {instance}" },
	{ "DRBD.PeerState(*)",            H_DRBDDeviceInfo,  "p",
		DCI_DT_INT,    "State of DRBD peer device {instance}" },
	{ "DRBD.PeerStateText(*)",        H_DRBDDeviceInfo,  "P",
		DCI_DT_STRING, "State of DRBD peer device {instance} as text" },
	{ "DRBD.State(*)",                H_DRBDDeviceInfo,  "s",
		DCI_DT_INT,    "State of DRBD device {instance}" },
	{ "DRBD.StateText(*)",            H_DRBDDeviceInfo,  "S",
		DCI_DT_STRING, "State of DRBD device {instance} as text" },

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
	{ "System.ProcessCount",          H_ProcessCount,    "T",
		DCI_DT_UINT,	DCIDESC_SYSTEM_PROCESSCOUNT },

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
#define P(q, w) (char *)&((struct CpuUsageParam){q, w})
	/* usage */
	{ "System.CPU.Usage",             H_CpuUsage,        P(INTERVAL_1MIN, CPU_USAGE_OVERAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE },
	{ "System.CPU.Usage5",            H_CpuUsage,        P(INTERVAL_5MIN, CPU_USAGE_OVERAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5 },
	{ "System.CPU.Usage15",           H_CpuUsage,        P(INTERVAL_15MIN, CPU_USAGE_OVERAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15 },
	{ "System.CPU.Usage(*)",          H_CpuUsageEx,      P(INTERVAL_1MIN, CPU_USAGE_OVERAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_EX },
	{ "System.CPU.Usage5(*)",         H_CpuUsageEx,      P(INTERVAL_5MIN, CPU_USAGE_OVERAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_EX },
	{ "System.CPU.Usage15(*)",        H_CpuUsageEx,      P(INTERVAL_15MIN, CPU_USAGE_OVERAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_EX },

	/* user */
	{ "System.CPU.Usage.User",             H_CpuUsage,        P(INTERVAL_1MIN, CPU_USAGE_USER),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_USER },
	{ "System.CPU.Usage5.User",            H_CpuUsage,        P(INTERVAL_5MIN, CPU_USAGE_USER),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_USER },
	{ "System.CPU.Usage15.User",           H_CpuUsage,        P(INTERVAL_15MIN, CPU_USAGE_USER),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_USER },
	{ "System.CPU.Usage.User(*)",          H_CpuUsageEx,      P(INTERVAL_1MIN, CPU_USAGE_USER),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_USER_EX },
	{ "System.CPU.Usage5.User(*)",         H_CpuUsageEx,      P(INTERVAL_5MIN, CPU_USAGE_USER),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_USER_EX },
	{ "System.CPU.Usage15.User(*)",        H_CpuUsageEx,      P(INTERVAL_15MIN, CPU_USAGE_USER),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_USER_EX },

	/* nice */
	{ "System.CPU.Usage.Nice",             H_CpuUsage,        P(INTERVAL_1MIN, CPU_USAGE_NICE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_NICE },
	{ "System.CPU.Usage5.Nice",            H_CpuUsage,        P(INTERVAL_5MIN, CPU_USAGE_NICE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_NICE },
	{ "System.CPU.Usage15.Nice",           H_CpuUsage,        P(INTERVAL_15MIN, CPU_USAGE_NICE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_NICE },
	{ "System.CPU.Usage.Nice(*)",          H_CpuUsageEx,      P(INTERVAL_1MIN, CPU_USAGE_NICE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_NICE_EX },
	{ "System.CPU.Usage5.Nice(*)",         H_CpuUsageEx,      P(INTERVAL_5MIN, CPU_USAGE_NICE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_NICE_EX },
	{ "System.CPU.Usage15.Nice(*)",        H_CpuUsageEx,      P(INTERVAL_15MIN, CPU_USAGE_NICE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_NICE_EX },

	/* system */
	{ "System.CPU.Usage.System",             H_CpuUsage,        P(INTERVAL_1MIN, CPU_USAGE_SYSTEM),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_SYSTEM },
	{ "System.CPU.Usage5.System",            H_CpuUsage,        P(INTERVAL_5MIN, CPU_USAGE_SYSTEM),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_SYSTEM },
	{ "System.CPU.Usage15.System",           H_CpuUsage,        P(INTERVAL_15MIN, CPU_USAGE_SYSTEM),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_SYSTEM },
	{ "System.CPU.Usage.System(*)",          H_CpuUsageEx,      P(INTERVAL_1MIN, CPU_USAGE_SYSTEM),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_SYSTEM_EX },
	{ "System.CPU.Usage5.System(*)",         H_CpuUsageEx,      P(INTERVAL_5MIN, CPU_USAGE_SYSTEM),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_SYSTEM_EX },
	{ "System.CPU.Usage15.System(*)",        H_CpuUsageEx,      P(INTERVAL_15MIN, CPU_USAGE_SYSTEM),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_SYSTEM_EX },

	/* idle */
	{ "System.CPU.Usage.Idle",             H_CpuUsage,        P(INTERVAL_1MIN, CPU_USAGE_IDLE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_IDLE },
	{ "System.CPU.Usage5.Idle",            H_CpuUsage,        P(INTERVAL_5MIN, CPU_USAGE_IDLE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_IDLE },
	{ "System.CPU.Usage15.Idle",           H_CpuUsage,        P(INTERVAL_15MIN, CPU_USAGE_IDLE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_IDLE },
	{ "System.CPU.Usage.Idle(*)",          H_CpuUsageEx,      P(INTERVAL_1MIN, CPU_USAGE_IDLE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_IDLE_EX },
	{ "System.CPU.Usage5.Idle5(*)",         H_CpuUsageEx,      P(INTERVAL_5MIN, CPU_USAGE_IDLE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_IDLE_EX },
	{ "System.CPU.Usage15.Idle(*)",        H_CpuUsageEx,      P(INTERVAL_15MIN, CPU_USAGE_IDLE),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_IDLE_EX },

	/* iowait */
	{ "System.CPU.Usage.IoWait",             H_CpuUsage,        P(INTERVAL_1MIN, CPU_USAGE_IOWAIT),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_IOWAIT },
	{ "System.CPU.Usage5.IoWait",            H_CpuUsage,        P(INTERVAL_5MIN, CPU_USAGE_IOWAIT),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_IOWAIT },
	{ "System.CPU.Usage15.IoWait",           H_CpuUsage,        P(INTERVAL_15MIN, CPU_USAGE_IOWAIT),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_IOWAIT },
	{ "System.CPU.Usage.IoWait(*)",          H_CpuUsageEx,      P(INTERVAL_1MIN, CPU_USAGE_IOWAIT),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_IOWAIT_EX },
	{ "System.CPU.Usage5.IoWait(*)",         H_CpuUsageEx,      P(INTERVAL_5MIN, CPU_USAGE_IOWAIT),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_IOWAIT_EX },
	{ "System.CPU.Usage15.IoWait(*)",        H_CpuUsageEx,      P(INTERVAL_15MIN, CPU_USAGE_IOWAIT),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_IOWAIT_EX },

	/* irq */
	{ "System.CPU.Usage.Irq",             H_CpuUsage,        P(INTERVAL_1MIN, CPU_USAGE_IRQ),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_IRQ },
	{ "System.CPU.Usage5.Irq",            H_CpuUsage,        P(INTERVAL_5MIN, CPU_USAGE_IRQ),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_IRQ },
	{ "System.CPU.Usage15.Irq",           H_CpuUsage,        P(INTERVAL_15MIN, CPU_USAGE_IRQ),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_IRQ },
	{ "System.CPU.Usage.Irq(*)",          H_CpuUsageEx,      P(INTERVAL_1MIN, CPU_USAGE_IRQ),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_IRQ_EX },
	{ "System.CPU.Usage5.Irq(*)",         H_CpuUsageEx,      P(INTERVAL_5MIN, CPU_USAGE_IRQ),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_IRQ_EX },
	{ "System.CPU.Usage15.Irq(*)",        H_CpuUsageEx,      P(INTERVAL_15MIN, CPU_USAGE_IRQ),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_IRQ_EX },

	/* softirq */
	{ "System.CPU.Usage.SoftIrq",             H_CpuUsage,        P(INTERVAL_1MIN, CPU_USAGE_SOFTIRQ),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_SOFTIRQ },
	{ "System.CPU.Usage5.SoftIrq",            H_CpuUsage,        P(INTERVAL_5MIN, CPU_USAGE_SOFTIRQ),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_SOFTIRQ },
	{ "System.CPU.Usage15.SoftIrq",           H_CpuUsage,        P(INTERVAL_15MIN, CPU_USAGE_SOFTIRQ),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_SOFTIRQ },
	{ "System.CPU.Usage.SoftIrq(*)",          H_CpuUsageEx,      P(INTERVAL_1MIN, CPU_USAGE_SOFTIRQ),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_SOFTIRQ_EX },
	{ "System.CPU.Usage5.SoftIrq(*)",         H_CpuUsageEx,      P(INTERVAL_5MIN, CPU_USAGE_SOFTIRQ),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_SOFTIRQ_EX },
	{ "System.CPU.Usage15.SoftIrq(*)",        H_CpuUsageEx,      P(INTERVAL_15MIN, CPU_USAGE_SOFTIRQ),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_SOFTIRQ_EX },

	/* steal */
	{ "System.CPU.Usage.Steal",             H_CpuUsage,        P(INTERVAL_1MIN, CPU_USAGE_STEAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_STEAL },
	{ "System.CPU.Usage5.Steal",            H_CpuUsage,        P(INTERVAL_5MIN, CPU_USAGE_STEAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_STEAL },
	{ "System.CPU.Usage15.Steal",           H_CpuUsage,        P(INTERVAL_15MIN, CPU_USAGE_STEAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_STEAL },
	{ "System.CPU.Usage.Steal(*)",          H_CpuUsageEx,      P(INTERVAL_1MIN, CPU_USAGE_STEAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE_STEAL_EX },
	{ "System.CPU.Usage5.Steal(*)",         H_CpuUsageEx,      P(INTERVAL_5MIN, CPU_USAGE_STEAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE5_STEAL_EX },
	{ "System.CPU.Usage15.Steal(*)",        H_CpuUsageEx,      P(INTERVAL_15MIN, CPU_USAGE_STEAL),
		DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_USAGE15_STEAL_EX },

#undef P

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
	{ "System.Uname",                 H_Uname,           NULL,
		DCI_DT_STRING,	DCIDESC_SYSTEM_UNAME },
	{ "System.Uptime",                H_Uptime,          NULL,
		DCI_DT_UINT,	DCIDESC_SYSTEM_UPTIME },

	{ "Agent.SourcePackageSupport",   H_SourcePkgSupport,NULL,
		DCI_DT_INT,		DCIDESC_AGENT_SOURCEPACKAGESUPPORT },

	/* iostat */
	{ "System.IO.TransferRate(*)",    H_TransferRate,    NULL,
		DCI_DT_UINT64,	"" },
	{ "System.IO.BlockReadRate(*)",   H_BlockReadRate, NULL,
		DCI_DT_UINT64,	"" },
	{ "System.IO.BlockWriteRate(*)",  H_BlockWriteRate,  NULL,
		DCI_DT_UINT64,	"" },
	{ "System.IO.BytesReadRate(*)",   H_BytesReadRate,   NULL,
		DCI_DT_UINT64,	"" },
	{ "System.IO.BytesWriteRate(*)",  H_BytesWriteRate,  NULL,
		DCI_DT_UINT64,	"" },
	{ "System.IO.DiskQueue(*)",       H_DiskQueue,       NULL,
		DCI_DT_UINT64,	"" },
	{ "System.IO.DiskQueue",          H_DiskInfo,        NULL,
		DCI_DT_UINT64,	"" },
	{ "System.IO.DiskTime",           H_DiskTime,        NULL,
		DCI_DT_UINT,	"" },
};

static NETXMS_SUBAGENT_ENUM m_enums[] =
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
	sizeof(m_enums) / sizeof(NETXMS_SUBAGENT_ENUM),
	m_enums,
	0,
	NULL
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
Revision 1.36  2007/10/31 08:14:33  victor
Added per-CPU usage statistics and System.CPU.Count parameter

Revision 1.35  2007/06/08 00:02:36  alk
DECLARE_SUBAGENT_INIT replaced with DECLARE_SUBAGENT_ENTRY_POINT

NETXMS_SUBAGENT_INFO initialization fixed (actions)

Revision 1.34  2007/06/07 22:07:11  alk
descriptions changed to defines

Revision 1.33  2007/04/25 07:44:09  victor
- Linux and HPUX subagents changed to new model
- ODBCQUERY subagent code cleaned

Revision 1.32  2007/04/24 12:04:10  alk
code reformat

Revision 1.31  2007/02/05 12:57:12  alk
*** empty log message ***

Revision 1.30  2007/01/15 00:16:06  victor
Implemented Process.CountEx for Linux

Revision 1.29  2006/10/30 17:25:10  victor
Implemented System.ConnectedUsers and System.ActiveUserSessions

Revision 1.28  2006/09/21 07:45:53  victor
Fixed problems with platform subagent loading by server

Revision 1.27  2006/09/21 07:24:08  victor
Server now can load platform subagents to obtain local ARP cache and interface list

Revision 1.26  2006/06/09 07:08:40  victor
Some minor fixes

Revision 1.25  2006/06/08 15:21:29  victor
Initial support for DRBD device monitoring

Revision 1.24  2006/03/02 21:08:20  alk
implemented:
	System.CPU.Usage5
	System.CPU.Usage15

Revision 1.23  2006/03/01 22:13:09  alk
added System.CPU.Usage [broken]

Revision 1.22  2005/09/15 21:47:02  victor
Added macro DECLARE_SUBAGENT_INIT to simplify initialization function declaration

Revision 1.21  2005/09/15 21:24:54  victor
Minor changes

Revision 1.20  2005/09/15 21:22:58  victor
Added possibility to build statically linked agents (with platform subagent linked in)
For now, agent has to be linked manually. I'll fix it later.

Revision 1.19  2005/08/22 00:11:46  alk
Net.IP.RoutingTable added

Revision 1.18  2005/08/19 15:23:50  victor
Added new parameters

Revision 1.17  2005/06/11 16:28:24  victor
Implemented all Net.Interface.* parameters except Net.Interface.Speed

Revision 1.16  2005/06/09 12:15:43  victor
Added support for Net.Interface.AdminStatus and Net.Interface.Link parameters

Revision 1.15  2005/02/24 17:38:49  victor
Added HDD monitring via SMART on Linux

Revision 1.14  2005/02/21 20:16:05  victor
Fixes in parameter data types and descriptions

Revision 1.13  2005/01/24 19:46:50  alk
SourcePackageSupport; return type/comment addded

Revision 1.12  2005/01/24 19:40:31  alk
return type/comments added for command list

System.ProcessCount/Process.Count(*) misunderstanding resolved

Revision 1.11  2005/01/17 23:31:01  alk
Agent.SourcePackageSupport added

Revision 1.10  2004/12/29 19:42:44  victor
Linux compatibility fixes

Revision 1.9  2004/10/22 22:08:34  alk
source restructured;
implemented:
	Net.IP.Forwarding
	Net.IP6.Forwarding
	Process.Count(*)
	Net.ArpCache
	Net.InterfaceList (if-type not implemented yet)
	System.ProcessList

Revision 1.8  2004/10/16 06:32:04  victor
Parameter name System.CPU.Procload changed to System.CPU.LoadAvg

Revision 1.7  2004/10/06 13:23:32  victor
Necessary changes to build everything on Linux

Revision 1.6  2004/08/26 23:51:26  alk
cosmetic changes

Revision 1.5  2004/08/18 00:12:56  alk
+ System.CPU.Procload* added, SINGLE processor only.

Revision 1.4  2004/08/17 23:19:20  alk
+ Disk.* implemented

Revision 1.3  2004/08/17 15:17:32  alk
+ linux agent: system.uptime, system.uname, system.hostname
! skeleton: amount of _PARM & _ENUM filled with sizeof()

Revision 1.1  2004/08/17 10:24:18  alk
+ subagent selection based on "uname -s"


*/
