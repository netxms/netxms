/* 
** NetXMS subagent for Darwin
** Copyright (C) 2012 Alex Kirhenshtein
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
**/

#include <nms_common.h>
#include <nms_agent.h>

#include "darwin.h"
#include "net.h"
#include "system.h"
#include "disk.h"
#include "cpu.h"


//
// Initalization callback
//

static BOOL SubAgentInit(Config *config)
{
   StartCpuUsageCollector();
   return TRUE;
}


//
// Shutdown callback
//

static void SubAgentShutdown()
{
   ShutdownCpuUsageCollector();
}

//
// Subagent information
//

static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
   { _T("Disk.Avail(*)"),                H_DiskInfo,        (const TCHAR *)DISK_AVAIL, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.AvailPerc(*)"),            H_DiskInfo,        (const TCHAR *)DISK_AVAIL_PERC, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.Free(*)"),                 H_DiskInfo,        (const TCHAR *)DISK_FREE, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.FreePerc(*)"),             H_DiskInfo,        (const TCHAR *)DISK_FREE_PERC, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.Total(*)"),                H_DiskInfo,        (const TCHAR *)DISK_TOTAL, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.Used(*)"),                 H_DiskInfo,        (const TCHAR *)DISK_USED, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Disk.UsedPerc(*)"),             H_DiskInfo,        (const TCHAR *)DISK_USED_PERC, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },

   { _T("FileSystem.Avail(*)"),          H_DiskInfo,        (const TCHAR *)DISK_AVAIL,        DCI_DT_UINT64, DCIDESC_FS_AVAIL },
   { _T("FileSystem.AvailPerc(*)"),      H_DiskInfo,        (const TCHAR *)DISK_AVAIL_PERC,   DCI_DT_FLOAT, DCIDESC_FS_AVAILPERC },
   { _T("FileSystem.Free(*)"),           H_DiskInfo,        (const TCHAR *)DISK_FREE,         DCI_DT_UINT64, DCIDESC_FS_FREE },
   { _T("FileSystem.FreePerc(*)"),       H_DiskInfo,        (const TCHAR *)DISK_FREE_PERC,    DCI_DT_FLOAT, DCIDESC_FS_FREEPERC },
   { _T("FileSystem.Total(*)"),          H_DiskInfo,        (const TCHAR *)DISK_TOTAL,        DCI_DT_UINT64, DCIDESC_FS_TOTAL },
   { _T("FileSystem.Used(*)"),           H_DiskInfo,        (const TCHAR *)DISK_USED,         DCI_DT_UINT64, DCIDESC_FS_USED },
   { _T("FileSystem.UsedPerc(*)"),       H_DiskInfo,        (const TCHAR *)DISK_USED_PERC,    DCI_DT_FLOAT, DCIDESC_FS_USEDPERC },

   { _T("System.Uname"),                 H_Uname,           NULL, DCI_DT_STRING, DCIDESC_SYSTEM_UNAME },
   { _T("System.Uptime"),                H_Uptime,          NULL, DCI_DT_UINT, DCIDESC_SYSTEM_UPTIME },
   { _T("System.Hostname"),              H_Hostname,        NULL, DCI_DT_FLOAT, DCIDESC_SYSTEM_HOSTNAME },

   { _T("System.CPU.Count"),             H_CpuCount,        NULL, DCI_DT_INT, DCIDESC_SYSTEM_CPU_COUNT },
   { _T("System.CPU.LoadAvg"),           H_CpuLoad,         NULL, DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_LOADAVG },
   { _T("System.CPU.LoadAvg5"),          H_CpuLoad,         NULL, DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_LOADAVG5 },
   { _T("System.CPU.LoadAvg15"),         H_CpuLoad,         NULL, DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_LOADAVG15 },

   /**************************************************************/
   /* usage */
   { _T("System.CPU.Usage"),             H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_OVERAL),
      DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE },
   { _T("System.CPU.Usage5"),            H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_OVERAL),
      DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5 },
   { _T("System.CPU.Usage15"),           H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_OVERAL),
      DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15 },
   { _T("System.CPU.Usage(*)"),          H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_OVERAL),
      DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_EX },
   { _T("System.CPU.Usage5(*)"),         H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_OVERAL),
      DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_EX },
   { _T("System.CPU.Usage15(*)"),        H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_OVERAL),
      DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_EX },

   /* user */
   { _T("System.CPU.Usage.User"),             H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_USER),
      DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_USER },
   { _T("System.CPU.Usage5.User"),            H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_USER),
      DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_USER },
   { _T("System.CPU.Usage15.User"),           H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_USER),
      DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_USER },
   { _T("System.CPU.Usage.User(*)"),          H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_USER),
      DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_USER_EX },
   { _T("System.CPU.Usage5.User(*)"),         H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_USER),
      DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_USER_EX },
   { _T("System.CPU.Usage15.User(*)"),        H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_USER),
      DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_USER_EX },

   /* system */
   { _T("System.CPU.Usage.System"),             H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_SYSTEM),
      DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_SYSTEM },
   { _T("System.CPU.Usage5.System"),            H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_SYSTEM),
      DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_SYSTEM },
   { _T("System.CPU.Usage15.System"),           H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_SYSTEM),
      DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_SYSTEM },
   { _T("System.CPU.Usage.System(*)"),          H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_SYSTEM),
      DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_SYSTEM_EX },
   { _T("System.CPU.Usage5.System(*)"),         H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_SYSTEM),
      DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_SYSTEM_EX },
   { _T("System.CPU.Usage15.System(*)"),        H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_SYSTEM),
      DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_SYSTEM_EX },

   /* idle */
   { _T("System.CPU.Usage.Idle"),             H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_IDLE),
      DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_IDLE },
   { _T("System.CPU.Usage5.Idle"),            H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_IDLE),
      DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_IDLE },
   { _T("System.CPU.Usage15.Idle"),           H_CpuUsage,        MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_IDLE),
      DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_IDLE },
   { _T("System.CPU.Usage.Idle(*)"),          H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_1MIN, CPU_USAGE_IDLE),
      DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE_IDLE_EX },
   { _T("System.CPU.Usage5.Idle5(*)"),         H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_5MIN, CPU_USAGE_IDLE),
      DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE5_IDLE_EX },
   { _T("System.CPU.Usage15.Idle(*)"),        H_CpuUsageEx,      MAKE_CPU_USAGE_PARAM(INTERVAL_15MIN, CPU_USAGE_IDLE),
      DCI_DT_FLOAT, DCIDESC_SYSTEM_CPU_USAGE15_IDLE_EX },

   /**************************************************************/
   { _T("Net.IP.Forwarding"),            H_NetIpForwarding, (const TCHAR *)4, DCI_DT_INT, DCIDESC_NET_IP_FORWARDING },
   { _T("Net.IP6.Forwarding"),           H_NetIpForwarding, (const TCHAR *)6, DCI_DT_INT, DCIDESC_NET_IP6_FORWARDING },
   { _T("Net.Interface.Link(*)"),        H_NetIfLink,       NULL, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Net.Interface.OperStatus(*)"),  H_NetIfLink,       NULL, DCI_DT_INT, DCIDESC_NET_INTERFACE_OPERSTATUS },
};

static NETXMS_SUBAGENT_LIST m_enums[] =
{
   { _T("Net.ArpCache"),                 H_NetArpCache,     NULL },
   { _T("Net.InterfaceList"),            H_NetIfList,       NULL },
   { _T("Net.IP.RoutingTable"),          H_NetRoutingTable, NULL },
   { _T("FileSystem.MountPoints"),       H_MountPoints,     NULL },
};

static NETXMS_SUBAGENT_TABLE m_tables[] =
{
   { _T("FileSystem.Volumes"), H_FileSystems, NULL, _T("VOLUME"), DCTDESC_FILESYSTEM_VOLUMES },
};

static NETXMS_SUBAGENT_INFO m_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("Darwin"),
   NETXMS_VERSION_STRING,
   SubAgentInit, // init handler
   SubAgentShutdown, // shutdown handler
   NULL, // command handler
   sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
   m_parameters,
   sizeof(m_enums) / sizeof(NETXMS_SUBAGENT_LIST),
   m_enums,
   sizeof(m_tables) / sizeof(NETXMS_SUBAGENT_TABLE), m_tables,
   0, NULL, // actions
   0, NULL // push parameters
};


//
// Entry point for NetXMS agent
//

DECLARE_SUBAGENT_ENTRY_POINT(DARWIN)
{
   *ppInfo = &m_info;
   return TRUE;
}


//
// Entry points for server
//

extern "C" BOOL __NxSubAgentGetIfList(StringList *value)
{
   return H_NetIfList(_T("Net.InterfaceList"), NULL, value) == SYSINFO_RC_SUCCESS;
}

extern "C" BOOL __NxSubAgentGetArpCache(StringList *value)
{
   return H_NetArpCache(_T("Net.ArpCache"), NULL, value) == SYSINFO_RC_SUCCESS;
}
