/* $Id$ */

/*
 ** NetXMS subagent for SunOS/Solaris
 ** Copyright (C) 2004, 2005 Victor Kirhenshtein
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
 ** $module: main.cpp
 **
 **/

#include "sunos_subagent.h"


//
// Hanlder functions
//

LONG H_CPUCount(char *pszParam, char *pArg, char *pValue);
LONG H_CPUUsage(char *pszParam, char *pArg, char *pValue);
LONG H_DiskInfo(char *pszParam, char *pArg, char *pValue);
LONG H_Hostname(char *pszParam, char *pArg, char *pValue);
LONG H_KStat(char *pszParam, char *pArg, char *pValue);
LONG H_LoadAvg(char *pszParam, char *pArg, char *pValue);
LONG H_MemoryInfo(char *pszParam, char *pArg, char *pValue);
LONG H_NetIfList(char *pszParam, char *pArg, NETXMS_VALUES_LIST *pValue);
LONG H_NetIfAdminStatus(char *pszParam, char *pArg, char *pValue);
LONG H_NetInterfaceLink(char *pszParam, char *pArg, char *pValue);
LONG H_NetIfDescription(char *pszParam, char *pArg, char *pValue);
LONG H_NetInterfaceStats(char *pszParam, char *pArg, char *pValue);
LONG H_ProcessCount(char *pszParam, char *pArg, char *pValue);
LONG H_ProcessInfo(char *pszParam, char *pArg, char *pValue);
LONG H_ProcessList(char *pszParam, char *pArg, NETXMS_VALUES_LIST *pValue);
LONG H_SysProcCount(char *pszParam, char *pArg, char *pValue);
LONG H_Uname(char *pszParam, char *pArg, char *pValue);
LONG H_Uptime(char *pszParam, char *pArg, char *pValue);


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

static LONG H_SourcePkg(char *pszParam, char *pArg, char *pValue)
{
	ret_int(pValue, 1);
	return SYSINFO_RC_SUCCESS;
}

//
// Initalization callback
//

static BOOL SubAgentInit(TCHAR *pszConfigFile)
{
	m_hCPUStatThread = ThreadCreateEx(CPUStatCollector, 0, NULL);

	return TRUE;
}

//
// Called by master agent at unload
//

static void UnloadHandler(void)
{
	g_bShutdown = TRUE;
	ThreadJoin(m_hCPUStatThread);
}


//
// Subagent information
//

static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
	{ "Agent.SourcePackageSupport", H_SourcePkg, NULL, DCI_DT_INT, DCIDESC_AGENT_SOURCEPACKAGESUPPORT },
	{ "Disk.Avail(*)", H_DiskInfo, (char *)DISK_AVAIL, DCI_DT_UINT64, DCIDESC_DISK_AVAIL },
	{ "Disk.AvailPerc(*)", H_DiskInfo, (char *)DISK_AVAIL_PERC, DCI_DT_FLOAT, DCIDESC_DISK_AVAILPERC },
	{ "Disk.Free(*)", H_DiskInfo, (char *)DISK_FREE, DCI_DT_UINT64, DCIDESC_DISK_FREE },
	{ "Disk.FreePerc(*)", H_DiskInfo, (char *)DISK_FREE_PERC, DCI_DT_FLOAT, DCIDESC_DISK_FREEPERC },
	{ "Disk.Total(*)", H_DiskInfo, (char *)DISK_TOTAL, DCI_DT_UINT64, DCIDESC_DISK_TOTAL },
	{ "Disk.Used(*)", H_DiskInfo, (char *)DISK_USED, DCI_DT_UINT64, DCIDESC_DISK_USED },
	{ "Disk.UsedPerc(*)", H_DiskInfo, (char *)DISK_USED_PERC, DCI_DT_FLOAT, DCIDESC_DISK_USEDPERC },
	{ "Net.Interface.AdminStatus(*)", H_NetIfAdminStatus, NULL, DCI_DT_INT, DCIDESC_NET_INTERFACE_ADMINSTATUS },
	{ "Net.Interface.BytesIn(*)", H_NetInterfaceStats, "rbytes", DCI_DT_UINT, DCIDESC_NET_INTERFACE_BYTESIN },
	{ "Net.Interface.BytesOut(*)", H_NetInterfaceStats, "obytes", DCI_DT_UINT, DCIDESC_NET_INTERFACE_BYTESOUT },
	{ "Net.Interface.Description(*)", H_NetIfDescription, NULL, DCI_DT_STRING, DCIDESC_NET_INTERFACE_DESCRIPTION },
	{ "Net.Interface.InErrors(*)", H_NetInterfaceStats, "ierrors", DCI_DT_UINT, DCIDESC_NET_INTERFACE_INERRORS },
	{ "Net.Interface.Link(*)", H_NetInterfaceLink, NULL, DCI_DT_INT, DCIDESC_NET_INTERFACE_LINK },
	{ "Net.Interface.OutErrors(*)", H_NetInterfaceStats, "oerrors", DCI_DT_UINT, DCIDESC_NET_INTERFACE_OUTERRORS },
	{ "Net.Interface.PacketsIn(*)", H_NetInterfaceStats, "ipackets", DCI_DT_UINT, DCIDESC_NET_INTERFACE_PACKETSIN },
	{ "Net.Interface.PacketsOut(*)", H_NetInterfaceStats, "opackets", DCI_DT_UINT, DCIDESC_NET_INTERFACE_PACKETSOUT },
	{ "Net.Interface.Speed(*)", H_NetInterfaceStats, "ifspeed", DCI_DT_UINT, DCIDESC_NET_INTERFACE_SPEED },
	{ "Process.Count(*)", H_ProcessCount, NULL, DCI_DT_UINT, DCIDESC_PROCESS_COUNT },
	{ "Process.KernelTime(*)", H_ProcessInfo, (char *)PROCINFO_KTIME, DCI_DT_UINT64, DCIDESC_PROCESS_KERNELTIME },
	{ "Process.PageFaults(*)", H_ProcessInfo, (char *)PROCINFO_PF, DCI_DT_UINT64, DCIDESC_PROCESS_PAGEFAULTS },
	{ "Process.UserTime(*)", H_ProcessInfo, (char *)PROCINFO_UTIME, DCI_DT_UINT64, DCIDESC_PROCESS_USERTIME },
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
	{ "System.Memory.Physical.Free", H_MemoryInfo, (char *)MEMINFO_PHYSICAL_FREE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE },
	{ "System.Memory.Physical.Total", H_MemoryInfo, (char *)MEMINFO_PHYSICAL_TOTAL, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_TOTAL },
	{ "System.Memory.Physical.Used", H_MemoryInfo, (char *)MEMINFO_PHYSICAL_USED, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED },
	{ "System.Memory.Swap.Free", H_MemoryInfo, (char *)MEMINFO_SWAP_FREE, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_SWAP_FREE },
	{ "System.Memory.Swap.Total", H_MemoryInfo, (char *)MEMINFO_SWAP_TOTAL, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_SWAP_TOTAL },
	{ "System.Memory.Swap.Used", H_MemoryInfo, (char *)MEMINFO_SWAP_USED, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_SWAP_USED },
	{ "System.ProcessCount", H_SysProcCount, NULL, DCI_DT_INT, DCIDESC_SYSTEM_PROCESSCOUNT },
	{ "System.Uname", H_Uname, NULL, DCI_DT_STRING, DCIDESC_SYSTEM_UNAME },
	{ "System.Uptime", H_Uptime, NULL, DCI_DT_UINT, DCIDESC_SYSTEM_UPTIME }
};

static NETXMS_SUBAGENT_ENUM m_enums[] =
{
	{ "Net.InterfaceList", H_NetIfList, NULL },
	{ "System.ProcessList", H_ProcessList, NULL }
};

static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("SUNOS"), NETXMS_VERSION_STRING,
	NULL, // init handler
	UnloadHandler, // unload handler
	NULL, // command handler
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

DECLARE_SUBAGENT_ENTRY_POINT(SUNOS)
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

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.22  2007/06/08 01:18:59  alk
invalid startup/shudown hooks

Revision 1.21  2007/06/08 00:02:36  alk
DECLARE_SUBAGENT_INIT replaced with DECLARE_SUBAGENT_ENTRY_POINT

NETXMS_SUBAGENT_INFO initialization fixed (actions)

Revision 1.20  2007/06/07 22:07:11  alk
descriptions changed to defines

Revision 1.19  2007/01/15 00:27:46  alk
__NxSubAgentGetIfList() added

Revision 1.18  2006/05/15 22:11:22  alk
+ Net.Interface.Link() workaround; trying kstat() first, then
IFF_RUNNING it kstat's link_up failed.
- code reformated


*/
