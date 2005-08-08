/*
** Windows NT/2000/XP/2003 NetXMS subagent
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

#include "winnt_subagent.h"


//
// Externlals
//

LONG H_ProcessList(char *cmd, char *arg, NETXMS_VALUES_LIST *value);
LONG H_ProcCount(char *cmd, char *arg, char *value);
LONG H_ProcCountSpecific(char *cmd, char *arg, char *value);
LONG H_ProcInfo(char *cmd, char *arg, char *value);
LONG H_ServiceState(char *cmd, char *arg, char *value);
LONG H_ThreadCount(char *cmd, char *arg, char *value);


//
// Global variables
//

BOOL (__stdcall *imp_GetProcessIoCounters)(HANDLE, PIO_COUNTERS) = NULL;
BOOL (__stdcall *imp_GetPerformanceInfo)(PPERFORMANCE_INFORMATION, DWORD) = NULL;
DWORD (__stdcall *imp_GetGuiResources)(HANDLE, DWORD) = NULL;


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
   { "Process.GdiObj(*)", H_ProcInfo, (char *)PROCINFO_GDI_OBJ, DCI_DT_UINT64, "" },
   { "Process.IO.OtherB(*)", H_ProcInfo, (char *)PROCINFO_IO_OTHER_B, DCI_DT_UINT64, "" },
   { "Process.IO.OtherOp(*)", H_ProcInfo, (char *)PROCINFO_IO_OTHER_OP, DCI_DT_UINT64, "" },
   { "Process.IO.ReadB(*)", H_ProcInfo, (char *)PROCINFO_IO_READ_B, DCI_DT_UINT64, "" },
   { "Process.IO.ReadOp(*)", H_ProcInfo, (char *)PROCINFO_IO_READ_OP, DCI_DT_UINT64, "" },
   { "Process.IO.WriteB(*)", H_ProcInfo, (char *)PROCINFO_IO_WRITE_B, DCI_DT_UINT64, "" },
   { "Process.IO.WriteOp(*)", H_ProcInfo, (char *)PROCINFO_IO_WRITE_OP, DCI_DT_UINT64, "" },
   { "Process.KernelTime(*)", H_ProcInfo, (char *)PROCINFO_KTIME, DCI_DT_UINT64, "" },
   { "Process.PageFaults(*)", H_ProcInfo, (char *)PROCINFO_PF, DCI_DT_UINT64, "" },
   { "Process.UserObj(*)", H_ProcInfo, (char *)PROCINFO_USER_OBJ, DCI_DT_UINT64, "" },
   { "Process.UserTime(*)", H_ProcInfo, (char *)PROCINFO_UTIME, DCI_DT_UINT64, "" },
   { "Process.VMSize(*)", H_ProcInfo, (char *)PROCINFO_VMSIZE, DCI_DT_UINT64, "" },
   { "Process.WkSet(*)", H_ProcInfo, (char *)PROCINFO_WKSET, DCI_DT_UINT64, "" },
   { "System.ProcessCount", H_ProcCount, NULL, DCI_DT_INT, "Total number of processes" },
   { "System.ServiceState(*)", H_ServiceState, NULL, DCI_DT_INT, "State of {instance} service" },
   { "System.ThreadCount", H_ThreadCount, NULL, DCI_DT_INT, "Total number of threads" }
};
static NETXMS_SUBAGENT_ENUM m_enums[] =
{
   { "System.ProcessList", H_ProcessList, NULL }
};

static NETXMS_SUBAGENT_INFO m_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
	_T("WinNT"), NETXMS_VERSION_STRING, UnloadHandler, NULL,
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
   HMODULE hModule;

   // Import functions which may not be presented in all Windows versions
   hModule = GetModuleHandle("USER32.DLL");
   if (hModule != NULL)
   {
      imp_GetGuiResources = (DWORD (__stdcall *)(HANDLE, DWORD))GetProcAddress(hModule, "GetGuiResources");
   }

   hModule = GetModuleHandle("KERNEL32.DLL");
   if (hModule != NULL)
   {
      imp_GetProcessIoCounters = (BOOL (__stdcall *)(HANDLE, PIO_COUNTERS))GetProcAddress(hModule, "GetProcessIoCounters");
   }

   hModule = GetModuleHandle("PSAPI.DLL");
   if (hModule != NULL)
   {
      imp_GetPerformanceInfo = (BOOL (__stdcall *)(PPERFORMANCE_INFORMATION, DWORD))GetProcAddress(hModule, "GetPerformanceInfo");
   }

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
