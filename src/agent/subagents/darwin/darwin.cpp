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

#include "net.h"
#include "system.h"

//
// Subagent information
//

static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
  { _T("System.Uname"),                 H_Uname,           NULL,				DCI_DT_STRING,	DCIDESC_SYSTEM_UNAME },
  { _T("System.Uptime"),                H_Uptime,          NULL,				DCI_DT_UINT,	DCIDESC_SYSTEM_UPTIME },
	{ _T("System.Hostname"),              H_Hostname,        NULL,				DCI_DT_FLOAT,	DCIDESC_SYSTEM_HOSTNAME },
	{ _T("System.CPU.Count"),             H_CpuCount,        NULL,				DCI_DT_INT,	DCIDESC_SYSTEM_CPU_COUNT },
	{ _T("System.CPU.LoadAvg"),           H_CpuLoad,         NULL,				DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_LOADAVG },
	{ _T("System.CPU.LoadAvg5"),          H_CpuLoad,         NULL,				DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_LOADAVG5 },
	{ _T("System.CPU.LoadAvg15"),         H_CpuLoad,         NULL,				DCI_DT_FLOAT,	DCIDESC_SYSTEM_CPU_LOADAVG15 },

	{ _T("Net.IP.Forwarding"),            H_NetIpForwarding, (const TCHAR *)4,			DCI_DT_INT,	DCIDESC_NET_IP_FORWARDING },
	{ _T("Net.IP6.Forwarding"),           H_NetIpForwarding, (const TCHAR *)6,			DCI_DT_INT,	DCIDESC_NET_IP6_FORWARDING },
	{ _T("Net.Interface.Link(*)"),        H_NetIfLink,       NULL,				DCI_DT_DEPRECATED,	DCIDESC_DEPRECATED },
	{ _T("Net.Interface.OperStatus(*)"),  H_NetIfLink,       NULL,				DCI_DT_INT,	DCIDESC_NET_INTERFACE_OPERSTATUS },
};

static NETXMS_SUBAGENT_LIST m_enums[] =
{
  { _T("Net.ArpCache"),                 H_NetArpCache,     NULL },
  { _T("Net.InterfaceList"),            H_NetIfList,       NULL },
  { _T("Net.IP.RoutingTable"),          H_NetRoutingTable, NULL },
};

static NETXMS_SUBAGENT_INFO m_info =
{
  NETXMS_SUBAGENT_INFO_MAGIC,
  _T("Darwin"),
  NETXMS_VERSION_STRING,
  NULL, // init handler
  NULL, // shutdown handler
  NULL, // command handler
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
