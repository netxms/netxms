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

LONG H_DiskInfo(char *pszParam, char *pArg, char *pValue);
LONG H_NetIfList(char *pszParam, char *pArg, NETXMS_VALUES_LIST *pValue);
LONG H_ProcessCount(char *pszParam, char *pArg, char *pValue);
LONG H_ProcessInfo(char *pszParam, char *pArg, char *pValue);
LONG H_ProcessList(char *pszParam, char *pArg, NETXMS_VALUES_LIST *pValue);
LONG H_SysProcCount(char *pszParam, char *pArg, char *pValue);
LONG H_Uname(char *pszParam, char *pArg, char *pValue);
LONG H_Uptime(char *pszParam, char *pArg, char *pValue);


//
// Detect support for source packages
//

LONG H_SourcePkg(char *pszParam, char *pArg, char *pValue)
{
	ret_int(pValue, 1);
	return SYSINFO_RC_SUCCESS;
}


//
// Called by master agent at unload
//

static void UnloadHandler(void)
{
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
   { "Process.Count(*)", H_ProcessCount, NULL, DCI_DT_UINT, "" },
   { "Process.KernelTime(*)", H_ProcessInfo, (char *)PROCINFO_KTIME, DCI_DT_UINT64, "" },
   { "Process.PageFaults(*)", H_ProcessInfo, (char *)PROCINFO_PF, DCI_DT_UINT64, "" },
   { "Process.UserTime(*)", H_ProcessInfo, (char *)PROCINFO_UTIME, DCI_DT_UINT64, "" },
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
   *ppInfo = &m_info;
   return TRUE;
}
