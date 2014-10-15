/*
** Windows platform subagent
** Copyright (C) 2003-2014 Victor Kirhenshtein
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

/**
 * Externlals
 */
LONG H_ActiveUserSessions(const TCHAR *cmd, const TCHAR *arg, StringList *value);
LONG H_AgentDesktop(const TCHAR *cmd, const TCHAR *arg, TCHAR *value);
LONG H_AppAddressSpace(const TCHAR *pszCmd, const TCHAR *pArg, TCHAR *pValue);
LONG H_ArpCache(const TCHAR *cmd, const TCHAR *arg, StringList *value);
LONG H_ConnectedUsers(const TCHAR *pszCmd, const TCHAR *pArg, TCHAR *pValue);
LONG H_Desktops(const TCHAR *cmd, const TCHAR *arg, StringList *value);
LONG H_InstalledProducts(const TCHAR *cmd, const TCHAR *arg, Table *value);
LONG H_InterfaceList(const TCHAR *cmd, const TCHAR *arg, StringList *value);
LONG H_IPRoutingTable(const TCHAR *cmd, const TCHAR *arg, StringList *pValue);
LONG H_NetInterface64bitSupport(const TCHAR *cmd, const TCHAR *arg, TCHAR *value);
LONG H_NetInterfaceStats(const TCHAR *cmd, const TCHAR *arg, TCHAR *value);
LONG H_NetIPStats(const TCHAR *cmd, const TCHAR *arg, TCHAR *value);
LONG H_RemoteShareStatus(const TCHAR *pszCmd, const TCHAR *pArg, TCHAR *pValue);
LONG H_ProcessList(const TCHAR *cmd, const TCHAR *arg, StringList *value);
LONG H_ProcessTable(const TCHAR *cmd, const TCHAR *arg, Table *value);
LONG H_ProcCount(const TCHAR *cmd, const TCHAR *arg, TCHAR *value);
LONG H_ProcCountSpecific(const TCHAR *cmd, const TCHAR *arg, TCHAR *value);
LONG H_ProcInfo(const TCHAR *cmd, const TCHAR *arg, TCHAR *value);
LONG H_ServiceList(const TCHAR *pszCmd, const TCHAR *pArg, StringList *value);
LONG H_ServiceState(const TCHAR *cmd, const TCHAR *arg, TCHAR *value);
LONG H_ServiceTable(const TCHAR *pszCmd, const TCHAR *pArg, Table *value);
LONG H_ThreadCount(const TCHAR *cmd, const TCHAR *arg, TCHAR *value);
LONG H_WindowStations(const TCHAR *cmd, const TCHAR *arg, StringList *value);

/**
 * Optional imports
 */
DWORD (__stdcall *imp_GetIfEntry2)(PMIB_IF_ROW2) = NULL;

/**
 * Import symbols
 */
static void ImportSymbols()
{
   HMODULE hModule;

   // IPHLPAPI.DLL
   hModule = LoadLibrary(_T("IPHLPAPI.DLL"));
   if (hModule != NULL)
   {
      imp_GetIfEntry2 = (DWORD (__stdcall *)(PMIB_IF_ROW2))GetProcAddress(hModule, "GetIfEntry2");
   }
   else
   {
		AgentWriteLog(NXLOG_WARNING, _T("Unable to load IPHLPAPI.DLL"));
   }
}

/**
 * Set or clear current privilege
 */
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

/**
 * Shutdown system
 */
static LONG H_ActionShutdown(const TCHAR *pszAction, StringList *pArgList, const TCHAR *pData)
{
	LONG nRet = ERR_INTERNAL_ERROR;

	if (SetCurrentPrivilege(SE_SHUTDOWN_NAME, TRUE))
	{
		if (InitiateSystemShutdown(NULL, NULL, 0, TRUE, (*pData == _T('R')) ? TRUE : FALSE))
			nRet = ERR_SUCCESS;
	}
	return nRet;
}

/**
 * Supported parameters
 */
static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
   { _T("Agent.Desktop"), H_AgentDesktop, NULL, DCI_DT_STRING, _T("Desktop associated with agent process") },
   { _T("Net.Interface.64BitCounters"), H_NetInterface64bitSupport, NULL, DCI_DT_INT, DCIDESC_NET_INTERFACE_64BITCOUNTERS },
   { _T("Net.Interface.AdminStatus(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_ADMIN_STATUS, DCI_DT_INT, DCIDESC_NET_INTERFACE_ADMINSTATUS },
   { _T("Net.Interface.BytesIn(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_BYTES_IN, DCI_DT_UINT, DCIDESC_NET_INTERFACE_BYTESIN },
   { _T("Net.Interface.BytesIn64(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_BYTES_IN_64, DCI_DT_UINT64, DCIDESC_NET_INTERFACE_BYTESIN },
   { _T("Net.Interface.BytesOut(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_BYTES_OUT, DCI_DT_UINT, DCIDESC_NET_INTERFACE_BYTESOUT },
   { _T("Net.Interface.BytesOut64(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_BYTES_OUT_64, DCI_DT_UINT64, DCIDESC_NET_INTERFACE_BYTESOUT },
   { _T("Net.Interface.Description(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_DESCR, DCI_DT_STRING, DCIDESC_NET_INTERFACE_DESCRIPTION },
   { _T("Net.Interface.InErrors(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_IN_ERRORS, DCI_DT_UINT, DCIDESC_NET_INTERFACE_INERRORS },
   { _T("Net.Interface.Link(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_OPER_STATUS, DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Net.Interface.MTU(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_MTU, DCI_DT_UINT, DCIDESC_NET_INTERFACE_MTU },
   { _T("Net.Interface.OperStatus(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_OPER_STATUS, DCI_DT_INT, DCIDESC_NET_INTERFACE_OPERSTATUS },
   { _T("Net.Interface.OutErrors(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_OUT_ERRORS, DCI_DT_UINT, DCIDESC_NET_INTERFACE_OUTERRORS },
   { _T("Net.Interface.PacketsIn(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_PACKETS_IN, DCI_DT_UINT, DCIDESC_NET_INTERFACE_PACKETSIN },
   { _T("Net.Interface.PacketsIn64(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_PACKETS_IN_64, DCI_DT_UINT64, DCIDESC_NET_INTERFACE_PACKETSIN },
   { _T("Net.Interface.PacketsOut(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_PACKETS_OUT, DCI_DT_UINT, DCIDESC_NET_INTERFACE_PACKETSOUT },
   { _T("Net.Interface.PacketsOut64(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_PACKETS_OUT_64, DCI_DT_UINT64, DCIDESC_NET_INTERFACE_PACKETSOUT },
   { _T("Net.Interface.Speed(*)"), H_NetInterfaceStats, (TCHAR *)NETINFO_IF_SPEED, DCI_DT_UINT64, DCIDESC_NET_INTERFACE_SPEED },
   { _T("Net.IP.Forwarding"), H_NetIPStats, (TCHAR *)NETINFO_IP_FORWARDING, DCI_DT_INT, DCIDESC_NET_IP_FORWARDING },
	{ _T("Net.RemoteShareStatus(*)"), H_RemoteShareStatus, _T("C"), DCI_DT_INT, _T("Status of remote shared resource") },
	{ _T("Net.RemoteShareStatusText(*)"), H_RemoteShareStatus, _T("T"), DCI_DT_STRING, _T("Status of remote shared resource as text") },
	{ _T("Process.Count(*)"), H_ProcCountSpecific, _T("N"), DCI_DT_INT, DCIDESC_PROCESS_COUNT },
	{ _T("Process.CountEx(*)"), H_ProcCountSpecific, _T("E"), DCI_DT_INT, DCIDESC_PROCESS_COUNTEX },
	{ _T("Process.CPUTime(*)"), H_ProcInfo, (TCHAR *)PROCINFO_CPUTIME, DCI_DT_UINT64, DCIDESC_PROCESS_CPUTIME },
	{ _T("Process.GDIObjects(*)"), H_ProcInfo, (TCHAR *)PROCINFO_GDI_OBJ, DCI_DT_UINT64, DCIDESC_PROCESS_GDIOBJ },
	{ _T("Process.IO.OtherB(*)"), H_ProcInfo, (TCHAR *)PROCINFO_IO_OTHER_B, DCI_DT_UINT64, DCIDESC_PROCESS_IO_OTHERB },
	{ _T("Process.IO.OtherOp(*)"), H_ProcInfo, (TCHAR *)PROCINFO_IO_OTHER_OP, DCI_DT_UINT64, DCIDESC_PROCESS_IO_OTHEROP },
	{ _T("Process.IO.ReadB(*)"), H_ProcInfo, (TCHAR *)PROCINFO_IO_READ_B, DCI_DT_UINT64, DCIDESC_PROCESS_IO_READB },
	{ _T("Process.IO.ReadOp(*)"), H_ProcInfo, (TCHAR *)PROCINFO_IO_READ_OP, DCI_DT_UINT64, DCIDESC_PROCESS_IO_READOP },
	{ _T("Process.IO.WriteB(*)"), H_ProcInfo, (TCHAR *)PROCINFO_IO_WRITE_B, DCI_DT_UINT64, DCIDESC_PROCESS_IO_WRITEB },
	{ _T("Process.IO.WriteOp(*)"), H_ProcInfo, (TCHAR *)PROCINFO_IO_WRITE_OP, DCI_DT_UINT64, DCIDESC_PROCESS_IO_WRITEOP },
	{ _T("Process.KernelTime(*)"), H_ProcInfo, (TCHAR *)PROCINFO_KTIME, DCI_DT_UINT64, DCIDESC_PROCESS_KERNELTIME },
	{ _T("Process.PageFaults(*)"), H_ProcInfo, (TCHAR *)PROCINFO_PF, DCI_DT_UINT64, DCIDESC_PROCESS_PAGEFAULTS },
	{ _T("Process.UserObjects(*)"), H_ProcInfo, (TCHAR *)PROCINFO_USER_OBJ, DCI_DT_UINT64, DCIDESC_PROCESS_USEROBJ },
	{ _T("Process.UserTime(*)"), H_ProcInfo, (TCHAR *)PROCINFO_UTIME, DCI_DT_UINT64, DCIDESC_PROCESS_USERTIME },
	{ _T("Process.VMSize(*)"), H_ProcInfo, (TCHAR *)PROCINFO_VMSIZE, DCI_DT_UINT64, DCIDESC_PROCESS_VMSIZE },
	{ _T("Process.WkSet(*)"), H_ProcInfo, (TCHAR *)PROCINFO_WKSET, DCI_DT_UINT64, DCIDESC_PROCESS_WKSET },
	{ _T("System.AppAddressSpace"), H_AppAddressSpace, NULL, DCI_DT_UINT, DCIDESC_SYSTEM_APPADDRESSSPACE },
	{ _T("System.ConnectedUsers"), H_ConnectedUsers, NULL, DCI_DT_INT, DCIDESC_SYSTEM_CONNECTEDUSERS },
	{ _T("System.ProcessCount"), H_ProcCount, NULL, DCI_DT_UINT, DCIDESC_SYSTEM_PROCESSCOUNT },
	{ _T("System.ServiceState(*)"), H_ServiceState, NULL, DCI_DT_INT, DCIDESC_SYSTEM_SERVICESTATE },
	{ _T("System.ThreadCount"), H_ThreadCount, NULL, DCI_DT_UINT, DCIDESC_SYSTEM_THREADCOUNT }
};

/**
 * Supported lists
 */
static NETXMS_SUBAGENT_LIST m_lists[] =
{
   { _T("Net.ArpCache"), H_ArpCache, NULL },
   { _T("Net.InterfaceList"), H_InterfaceList, NULL },
   { _T("Net.IP.RoutingTable"), H_IPRoutingTable, NULL },
	{ _T("System.ActiveUserSessions"), H_ActiveUserSessions, NULL },
	{ _T("System.Desktops(*)"), H_Desktops, NULL },
	{ _T("System.ProcessList"), H_ProcessList, NULL },
	{ _T("System.Services"), H_ServiceList, NULL },
	{ _T("System.WindowStations"), H_WindowStations, NULL }
};

/**
 * Supported tables
 */
static NETXMS_SUBAGENT_TABLE m_tables[] =
{
	{ _T("System.InstalledProducts"), H_InstalledProducts, NULL, _T("NAME"), DCTDESC_SYSTEM_INSTALLED_PRODUCTS },
	{ _T("System.Processes"), H_ProcessTable, NULL, _T("PID"), DCTDESC_SYSTEM_PROCESSES },
	{ _T("System.Services"), H_ServiceTable, NULL, _T("Name"), _T("Services") }
};

/**
 * Supported actions
 */
static NETXMS_SUBAGENT_ACTION m_actions[] =
{
	{ _T("System.Restart"), H_ActionShutdown, _T("R"), _T("Restart system") },
	{ _T("System.Shutdown"), H_ActionShutdown, _T("S"), _T("Shutdown system") }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("WinNT"), NETXMS_VERSION_STRING,
	NULL, NULL, NULL,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	sizeof(m_lists) / sizeof(NETXMS_SUBAGENT_LIST),
	m_lists,
	sizeof(m_tables) / sizeof(NETXMS_SUBAGENT_TABLE),
	m_tables,
	sizeof(m_actions) / sizeof(NETXMS_SUBAGENT_ACTION),
	m_actions,
	0, NULL	// push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(WINNT)
{
	*ppInfo = &m_info;
	ImportSymbols();
	return TRUE;
}

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}
