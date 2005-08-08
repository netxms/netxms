/*
** Windows 95/98/Me NetXMS subagent
** Copyright (C) 2003, 2004, 2005 Victor Kirhenshtein
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

#include "win9x_subagent.h"


//
// Externlals
//

LONG H_ProcessList(char *pszCmd, char *pArg, NETXMS_VALUES_LIST *value);
LONG H_ProcCount(char *pszCmd, char *pArg, char *pValue);
LONG H_ProcCountSpecific(char *pszCmd, char *pArg, char *pValue);
LONG H_ThreadCount(char *pszCmd, char *pArg, char *pValue);


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
   { "Process.Count(*)", H_ProcCountSpecific, NULL, DCI_DT_INT, "Number of {instance} processes" },
   { "System.ProcessCount", H_ProcCount, NULL, DCI_DT_INT, "Total number of processes" },
   { "System.ThreadCount", H_ThreadCount, NULL, DCI_DT_INT, "Total number of threads" }
};
static NETXMS_SUBAGENT_ENUM m_enums[] =
{
   { "System.ProcessList", H_ProcessList, NULL }
};

static NETXMS_SUBAGENT_INFO m_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
	_T("Win9x"), NETXMS_VERSION_STRING, UnloadHandler, NULL,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	sizeof(m_enums) / sizeof(NETXMS_SUBAGENT_ENUM),
	m_enums
};


//
// Entry point for NetXMS agent
//

extern "C" BOOL __declspec(dllexport) __cdecl
   NxSubAgentInit(NETXMS_SUBAGENT_INFO **ppInfo, TCHAR *pszConfigFile)
{
   *ppInfo = &m_info;
   return TRUE;
}


//
// DLL entry point
//

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   return TRUE;
}
