/*
** NetXMS subagent for SunOS/Solaris
** Copyright (C) 2004 Victor Kirhenshtein
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
   { "Disk.Free(*)", H_DiskInfo, (char *)DISK_FREE },
   { "Disk.Total(*)", H_DiskInfo, (char *)DISK_TOTAL },
   { "Disk.Used(*)", H_DiskInfo, (char *)DISK_USED },
   { "Process.Count(*)", H_ProcessCount, NULL },
   { "System.ProcessCount", H_SysProcCount, NULL }
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
