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
   { "Agent.SourcePackageSupport", H_SourcePkg, NULL, DCI_DT_INT, "" },
   { "Disk.Free(*)", H_DiskInfo, (char *)DISK_FREE, DCI_DT_UINT64, "Free disk space on *" },
   { "Disk.Total(*)", H_DiskInfo, (char *)DISK_TOTAL, DCI_DT_UINT64, "Total disk space on *" },
   { "Disk.Used(*)", H_DiskInfo, (char *)DISK_USED, DCI_DT_UINT64, "Used disk space on *" },
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
   { "Process.Count(*)", H_ProcessCount, NULL, DCI_DT_UINT, "Number of proceses {instance}" },
   { "Process.KernelTime(*)", H_ProcessInfo, (char *)PROCINFO_KTIME, DCI_DT_UINT64, "" },
   { "Process.PageFaults(*)", H_ProcessInfo, (char *)PROCINFO_PF, DCI_DT_UINT64, "" },
   { "Process.UserTime(*)", H_ProcessInfo, (char *)PROCINFO_UTIME, DCI_DT_UINT64, "" },
	{ "System.CPU.Count", H_CPUCount, NULL, DCI_DT_UINT, "Number of CPU in the system" },
   { "System.CPU.LoadAvg", H_LoadAvg, (char *)0, DCI_DT_FLOAT, "Average CPU load for last minute" },
   { "System.CPU.LoadAvg5", H_LoadAvg, (char *)1, DCI_DT_FLOAT, "Average CPU load for last 5 minutes" },
   { "System.CPU.LoadAvg15", H_LoadAvg, (char *)2, DCI_DT_FLOAT, "Average CPU load for last 15 minutes" },
   { "System.CPU.Usage", H_CPUUsage, "T0", DCI_DT_FLOAT, "Average CPU(s) utilization for last minute" },
   { "System.CPU.Usage5", H_CPUUsage, "T1", DCI_DT_FLOAT, "Average CPU(s) utilization for last 5 minutes" },
   { "System.CPU.Usage15", H_CPUUsage, "T2", DCI_DT_FLOAT, "Average CPU(s) utilization for last 15 minutes" },
   { "System.CPU.Usage(*)", H_CPUUsage, "C0", DCI_DT_FLOAT, "Average CPU {instance} utilization for last minute" },
   { "System.CPU.Usage5(*)", H_CPUUsage, "C1", DCI_DT_FLOAT, "Average CPU {instance} utilization for last 5 minutes" },
   { "System.CPU.Usage15(*)", H_CPUUsage, "C2", DCI_DT_FLOAT, "Average CPU {instance} utilization for last 15 minutes" },
   { "System.Hostname", H_Hostname, NULL, DCI_DT_STRING, "Host name" },
   { "System.KStat(*)", H_KStat, NULL, DCI_DT_STRING, "" },
	{ "System.Memory.Physical.Free", H_MemoryInfo, (char *)MEMINFO_PHYSICAL_FREE, DCI_DT_UINT64, "Available physical memory" },
	{ "System.Memory.Physical.Total", H_MemoryInfo, (char *)MEMINFO_PHYSICAL_TOTAL, DCI_DT_UINT64, "Total amount of physical memory" },
	{ "System.Memory.Physical.Used", H_MemoryInfo, (char *)MEMINFO_PHYSICAL_USED, DCI_DT_UINT64, "Used physical memory" },
   { "System.Memory.Swap.Free", H_MemoryInfo, (char *)MEMINFO_SWAP_FREE, DCI_DT_UINT64, "Free swap space" },
   { "System.Memory.Swap.Total", H_MemoryInfo, (char *)MEMINFO_SWAP_TOTAL, DCI_DT_UINT64, "Total amount of swap space" },
   { "System.Memory.Swap.Used", H_MemoryInfo, (char *)MEMINFO_SWAP_USED, DCI_DT_UINT64, "Used swap space" },
   { "System.ProcessCount", H_SysProcCount, NULL, DCI_DT_INT, "Total number of processes" },
   { "System.Uname", H_Uname, NULL, DCI_DT_STRING, "System uname" },
   { "System.Uptime", H_Uptime, NULL, DCI_DT_UINT, "System uptime" }
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
	UnloadHandler, NULL,
   sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
   m_parameters,
   sizeof(m_enums) / sizeof(NETXMS_SUBAGENT_ENUM),
   m_enums
};


//
// Entry point for NetXMS agent
//

extern "C" BOOL NxSubAgentInit(NETXMS_SUBAGENT_INFO **ppInfo, TCHAR *pszConfigFile)
{
	m_hCPUStatThread = ThreadCreateEx(CPUStatCollector, 0, NULL);

   *ppInfo = &m_info;
   return TRUE;
}
