/* $Id: linux.cpp,v 1.9 2004-10-22 22:08:34 alk Exp $ */

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
** $module: linux.cpp
**
**/

#include <nms_common.h>
#include <nms_agent.h>

#include "net.h"
#include "system.h"
#include "disk.h"

//
// Subagent information
//

static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
   { "Disk.Free(*)",                 H_DiskInfo,        (char *)DISK_FREE },
   { "Disk.Total(*)",                H_DiskInfo,        (char *)DISK_TOTAL },
   { "Disk.Used(*)",                 H_DiskInfo,        (char *)DISK_USED },

   { "Net.IP.Forwarding",            H_NetIpForwarding, (char *)4 },
   { "Net.IP6.Forwarding",           H_NetIpForwarding, (char *)6 },

   { "Process.Count(*)",             H_ProcessCount,    NULL },

   { "System.CPU.LoadAvg",           H_CpuLoad,         NULL },
   { "System.CPU.LoadAvg5",          H_CpuLoad,         NULL },
   { "System.CPU.LoadAvg15",         H_CpuLoad,         NULL },
   { "System.CPU.Usage",             H_CpuUsage,        NULL },
   { "System.CPU.Usage5",            H_CpuUsage,        NULL },
   { "System.CPU.Usage15",           H_CpuUsage,        NULL },
   { "System.Hostname",              H_Hostname,        NULL },
   { "System.Memory.Physical.Free",  H_MemoryInfo,      (char *)PHYSICAL_FREE },
   { "System.Memory.Physical.Total", H_MemoryInfo,      (char *)PHYSICAL_TOTAL},
   { "System.Memory.Physical.Used",  H_MemoryInfo,      (char *)PHYSICAL_USED },
   { "System.Memory.Swap.Free",      H_MemoryInfo,      (char *)SWAP_FREE },
   { "System.Memory.Swap.Total",     H_MemoryInfo,      (char *)SWAP_TOTAL },
   { "System.Memory.Swap.Used",      H_MemoryInfo,      (char *)SWAP_USED },
   { "System.Memory.Virtual.Free",   H_MemoryInfo,      (char *)VIRTUAL_FREE },
   { "System.Memory.Virtual.Total",  H_MemoryInfo,      (char *)VIRTUAL_TOTAL },
   { "System.Memory.Virtual.Used",   H_MemoryInfo,      (char *)VIRTUAL_USED },
   { "System.Uname",                 H_Uname,           NULL },
   { "System.Uptime",                H_Uptime,          NULL },
};

static NETXMS_SUBAGENT_ENUM m_enums[] =
{
   { "Net.ArpCache",                 H_NetArpCache,     NULL },
   { "Net.InterfaceList",            H_NetIfList,       NULL },
   { "System.ProcessList",           H_ProcessList,     NULL },
};

static NETXMS_SUBAGENT_INFO m_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
	"Linux",
	NETXMS_VERSION_STRING,
	NULL,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	sizeof(m_enums) / sizeof(NETXMS_SUBAGENT_ENUM),
	m_enums
};

//
// Entry point for NetXMS agent
//

extern "C" BOOL NxSubAgentInit(NETXMS_SUBAGENT_INFO **ppInfo)
{
   *ppInfo = &m_info;

   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
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
