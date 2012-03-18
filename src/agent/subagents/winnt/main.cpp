/*
** Windows platform subagent
** Copyright (C) 2003-2012 Victor Kirhenshtein
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
** File: main.cpp
**
**/

#include "winnt_subagent.h"


//
// Externlals
//

LONG H_ActiveUserSessions(const char *cmd, const char *arg, StringList *value);
LONG H_AppAddressSpace(const char *pszCmd, const char *pArg, char *pValue);
LONG H_ConnectedUsers(const char *pszCmd, const char *pArg, char *pValue);
LONG H_RemoteShareStatus(const char *pszCmd, const char *pArg, char *pValue);
LONG H_ProcessList(const char *cmd, const char *arg, StringList *value);
LONG H_ProcessTable(const char *cmd, const char *arg, Table *value);
LONG H_ProcCount(const char *cmd, const char *arg, char *value);
LONG H_ProcCountSpecific(const char *cmd, const char *arg, char *value);
LONG H_ProcInfo(const char *cmd, const char *arg, char *value);
LONG H_ServiceState(const char *cmd, const char *arg, char *value);
LONG H_ThreadCount(const char *cmd, const char *arg, char *value);


//
// Global variables
//

BOOL (__stdcall *imp_GetProcessIoCounters)(HANDLE, PIO_COUNTERS) = NULL;
BOOL (__stdcall *imp_GetPerformanceInfo)(PPERFORMANCE_INFORMATION, DWORD) = NULL;
DWORD (__stdcall *imp_GetGuiResources)(HANDLE, DWORD) = NULL;
BOOL (__stdcall *imp_WTSEnumerateSessionsA)(HANDLE, DWORD, DWORD, PWTS_SESSION_INFOA *, DWORD *) = NULL;
BOOL (__stdcall *imp_WTSQuerySessionInformationA)(HANDLE, DWORD, WTS_INFO_CLASS, LPSTR *, DWORD *) = NULL;
void (__stdcall *imp_WTSFreeMemory)(void *) = NULL;


//
// Set or clear current privilege
//

static BOOL SetCurrentPrivilege(LPCTSTR pszPrivilege, BOOL bEnablePrivilege)
{
	HANDLE hToken;
	LUID luid;
	TOKEN_PRIVILEGES tp, tpPrevious;
	DWORD cbPrevious = sizeof(TOKEN_PRIVILEGES);
	BOOL bSuccess = FALSE;

	if (!LookupPrivilegeValue(NULL, pszPrivilege, &luid))
		return FALSE;

	if(!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken))
		return FALSE;

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = 0;

	if (AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES),
				&tpPrevious, &cbPrevious))
	{
		tpPrevious.PrivilegeCount = 1;
		tpPrevious.Privileges[0].Luid = luid;

		if (bEnablePrivilege)
			tpPrevious.Privileges[0].Attributes |= SE_PRIVILEGE_ENABLED;
		else
			tpPrevious.Privileges[0].Attributes &= ~SE_PRIVILEGE_ENABLED;

		bSuccess = AdjustTokenPrivileges(hToken, FALSE, &tpPrevious, cbPrevious, NULL, NULL);
	}

	CloseHandle(hToken);

	return bSuccess;
}



//
// Shutdown system
//

static LONG H_ActionShutdown(const TCHAR *pszAction, StringList *pArgList, const TCHAR *pData)
{
	LONG nRet = ERR_INTERNAL_ERROR;

	if (SetCurrentPrivilege(SE_SHUTDOWN_NAME, TRUE))
	{
		if (InitiateSystemShutdown(NULL, NULL, 0, TRUE,
					(*pData == 'R') ? TRUE : FALSE))
			nRet = ERR_SUCCESS;
	}
	return nRet;
}


//
// Subagent information
//

static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
	{ "Net.RemoteShareStatus(*)", H_RemoteShareStatus, "C", DCI_DT_INT, _T("Status of remote shared resource") },
	{ "Net.RemoteShareStatusText(*)", H_RemoteShareStatus, "T", DCI_DT_STRING, _T("Status of remote shared resource as text") },
	{ "Process.Count(*)", H_ProcCountSpecific, "N", DCI_DT_INT, DCIDESC_PROCESS_COUNT },
	{ "Process.CountEx(*)", H_ProcCountSpecific, "E", DCI_DT_INT, DCIDESC_PROCESS_COUNTEX },
	{ "Process.CPUTime(*)", H_ProcInfo, (char *)PROCINFO_CPUTIME, DCI_DT_UINT64, DCIDESC_PROCESS_CPUTIME },
	{ "Process.GDIObjects(*)", H_ProcInfo, (char *)PROCINFO_GDI_OBJ, DCI_DT_UINT64, DCIDESC_PROCESS_GDIOBJ },
	{ "Process.IO.OtherB(*)", H_ProcInfo, (char *)PROCINFO_IO_OTHER_B, DCI_DT_UINT64, DCIDESC_PROCESS_IO_OTHERB },
	{ "Process.IO.OtherOp(*)", H_ProcInfo, (char *)PROCINFO_IO_OTHER_OP, DCI_DT_UINT64, DCIDESC_PROCESS_IO_OTHEROP },
	{ "Process.IO.ReadB(*)", H_ProcInfo, (char *)PROCINFO_IO_READ_B, DCI_DT_UINT64, DCIDESC_PROCESS_IO_READB },
	{ "Process.IO.ReadOp(*)", H_ProcInfo, (char *)PROCINFO_IO_READ_OP, DCI_DT_UINT64, DCIDESC_PROCESS_IO_READOP },
	{ "Process.IO.WriteB(*)", H_ProcInfo, (char *)PROCINFO_IO_WRITE_B, DCI_DT_UINT64, DCIDESC_PROCESS_IO_WRITEB },
	{ "Process.IO.WriteOp(*)", H_ProcInfo, (char *)PROCINFO_IO_WRITE_OP, DCI_DT_UINT64, DCIDESC_PROCESS_IO_WRITEOP },
	{ "Process.KernelTime(*)", H_ProcInfo, (char *)PROCINFO_KTIME, DCI_DT_UINT64, DCIDESC_PROCESS_KERNELTIME },
	{ "Process.PageFaults(*)", H_ProcInfo, (char *)PROCINFO_PF, DCI_DT_UINT64, DCIDESC_PROCESS_PAGEFAULTS },
	{ "Process.UserObjects(*)", H_ProcInfo, (char *)PROCINFO_USER_OBJ, DCI_DT_UINT64, DCIDESC_PROCESS_USEROBJ },
	{ "Process.UserTime(*)", H_ProcInfo, (char *)PROCINFO_UTIME, DCI_DT_UINT64, DCIDESC_PROCESS_USERTIME },
	{ "Process.VMSize(*)", H_ProcInfo, (char *)PROCINFO_VMSIZE, DCI_DT_UINT64, DCIDESC_PROCESS_VMSIZE },
	{ "Process.WkSet(*)", H_ProcInfo, (char *)PROCINFO_WKSET, DCI_DT_UINT64, DCIDESC_PROCESS_WKSET },
	{ "System.AppAddressSpace", H_AppAddressSpace, NULL, DCI_DT_UINT, DCIDESC_SYSTEM_APPADDRESSSPACE },
	{ "System.ConnectedUsers", H_ConnectedUsers, NULL, DCI_DT_INT, DCIDESC_SYSTEM_CONNECTEDUSERS },
	{ "System.ProcessCount", H_ProcCount, NULL, DCI_DT_UINT, DCIDESC_SYSTEM_PROCESSCOUNT },
	{ "System.ServiceState(*)", H_ServiceState, NULL, DCI_DT_INT, DCIDESC_SYSTEM_SERVICESTATE },
	{ "System.ThreadCount", H_ThreadCount, NULL, DCI_DT_UINT, DCIDESC_SYSTEM_THREADCOUNT }
};
static NETXMS_SUBAGENT_LIST m_enums[] =
{
	{ "System.ActiveUserSessions", H_ActiveUserSessions, NULL },
	{ "System.ProcessList", H_ProcessList, NULL }
};
static NETXMS_SUBAGENT_TABLE m_tables[] =
{
	{ "System.Processes", H_ProcessTable, NULL, "PID", DCTDESC_SYSTEM_PROCESSES }
};
static NETXMS_SUBAGENT_ACTION m_actions[] =
{
	{ "System.Restart", H_ActionShutdown, "R", "Restart system" },
	{ "System.Shutdown", H_ActionShutdown, "S", "Shutdown system" }
};

static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("WinNT"), NETXMS_VERSION_STRING,
	NULL, NULL, NULL,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	sizeof(m_enums) / sizeof(NETXMS_SUBAGENT_LIST),
	m_enums,
	sizeof(m_tables) / sizeof(NETXMS_SUBAGENT_TABLE),
	m_tables,
	sizeof(m_actions) / sizeof(NETXMS_SUBAGENT_ACTION),
	m_actions,
	0, NULL	// push parameters
};


//
// Entry point for NetXMS agent
//

extern "C" BOOL __declspec(dllexport) __cdecl
NxSubAgentRegister(NETXMS_SUBAGENT_INFO **ppInfo, Config *config)
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

	hModule = LoadLibrary("WTSAPI32.DLL");
	if (hModule != NULL)
	{
		imp_WTSEnumerateSessionsA = (BOOL (__stdcall *)(HANDLE, DWORD, DWORD, PWTS_SESSION_INFOA *, DWORD *))GetProcAddress(hModule, "WTSEnumerateSessionsA");
		imp_WTSQuerySessionInformationA = (BOOL (__stdcall *)(HANDLE, DWORD, WTS_INFO_CLASS, LPSTR *, DWORD *))GetProcAddress(hModule, "WTSQuerySessionInformationA");
		imp_WTSFreeMemory = (void (__stdcall *)(void *))GetProcAddress(hModule, "WTSFreeMemory");
	}
	*ppInfo = &m_info;
	return TRUE;
}


//
// DLL entry point
//

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}
