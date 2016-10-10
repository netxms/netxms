/*
 ** Vmgr management subagent
 ** Copyright (C) 2016 Raden Solutions
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

#include "vmgr.h"

/**
 * General VM connection list
 */
StringObjectMap<HostConnections> g_connectionList(true);

/**
 * Configuration file template for host list
 */
static TCHAR *s_hostList = NULL;
static NX_CFG_TEMPLATE m_cfgTemplate[] =
{
	{ _T("Host"), CT_STRING_LIST, _T('\n'), 0, 0, 0, &s_hostList },
	{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
};

/**
 * Configuration file template for host detailed configuration
 */
static char s_url[512];
static char s_login[64];
static char s_password[128];
static NX_CFG_TEMPLATE m_aditionalCfgTemplate[] =
{
	{ _T("Url"), CT_MB_STRING, 0, 0, 512, 0, s_url },
	{ _T("Username"), CT_MB_STRING, 0, 0, 64, 0, s_login },
	{ _T("Password"), CT_MB_STRING, 0, 0, 128, 0, s_password },
	{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
};

/**
 * Adds connection object to array if it was possible to connect to it
 */
bool AddConnection(const TCHAR *name)
{
   HostConnections *conn = new HostConnections(name, s_url, s_login, s_password);
   bool success = conn->connect();
   if(!success)
   {
      delete conn;
   }
   else
   {
      g_connectionList.set(name, conn);
   }
   return success;
}

void AddHostConfiguraiton(TCHAR *hostName, Config *config)
{
   TCHAR section[512];
   _sntprintf(section, 512, _T("VMGR:%s"), hostName);
   s_url[0] = 0;
   s_login[0] = 0;
   s_password[0] = 0;
	bool success = config->parseTemplate(section, m_aditionalCfgTemplate);
	if (success)
	{
	   StrStripA(s_url);
	   StrStripA(s_login);
	   StrStripA(s_password);
	}

   if(!success || s_url[0] == 0 || !AddConnection(hostName))
   {
      AgentWriteLog(NXLOG_WARNING, _T("VMGR: Unable to add connection URL \"%hs\" for connection name \"%s\""), s_url, hostName);
   }
   else
   {
      AgentWriteLog(6, _T("VMGR: VM successfully added: URL \"%hs\", name \"%s\""), s_url, hostName);
   }
}


/**
 * Callback for global libvirt errors
 */
static void customGlobalErrorFunc(void *userdata, virErrorPtr err)
{
   int lvl = NXLOG_DEBUG;
   switch(err->level)
   {
   case VIR_ERR_WARNING:
      lvl = NXLOG_WARNING;
      break;
   case VIR_ERR_ERROR:
      lvl = NXLOG_ERROR;
      break;
   default:
      lvl = NXLOG_DEBUG;
      break;
   }
   AgentWriteLog(lvl, _T("VMGR: %hs"), err->message);
}

/**
 * Function that sets all common configurations for libvirt
 */
void InitLibvirt()
{
   virSetErrorFunc(NULL, customGlobalErrorFunc);
}

/**
 * Subagent initialization
 */
static BOOL SubagentInit(Config *config)
{
	// Parse configuration
   if (!config->parseTemplate(_T("VMGR"), m_cfgTemplate))
      return FALSE;

   if (s_hostList != NULL)
	{
      TCHAR *item, *itemEnd;
      for(item = itemEnd = s_hostList; (*item != 0) && (itemEnd != NULL); item = itemEnd + 1)
      {
         itemEnd = _tcschr(item, _T('\n'));
         if (itemEnd != NULL)
            *itemEnd = 0;
         StrStrip(item);
         AddHostConfiguraiton(item, config);
      }
   }

   if (s_hostList == NULL || g_connectionList.size() == 0)
   {
      AgentWriteLog(NXLOG_ERROR, _T("VMGR: No connection URL found. Vmgr subagent will not be started."));
      free(s_hostList);
      return FALSE;
   }

	free(s_hostList);
	InitLibvirt();

   AgentWriteDebugLog(2, _T("VMGR: subagent initialized"));
   return TRUE;
}

/**
 * Called by master agent at unload
 */
static void SubagentShutdown()
{
   //Preparations for shutdown
}

/**
 * Process commands like VM up/down
 */
static BOOL ProcessCommands(UINT32 command, NXCPMessage *request, NXCPMessage *response, AbstractCommSession *session)
{
   switch(command)
   {
      default:
         return FALSE;
   }
}

/**
 * Supported parameters
 */
static NETXMS_SUBAGENT_PARAM s_parameters[] =
{
   { _T("VMGR.Host.CPU.Arch(*)"), H_GetFromCapabilities, _T("/host/cpu/arch"), DCI_DT_STRING, _T("Host CPU architecture") },
   { _T("VMGR.Host.CPU.MaxCount(*)"), H_GetUIntParam, _T("C"), DCI_DT_UINT, _T("Host maximum virtual CPU count") },
   { _T("VMGR.Host.FreeMemory(*)"), H_GetUInt64Param, _T("F"), DCI_DT_UINT64, _T("Host free memory") },
   { _T("VMGR.Host.MemorySize(*)"), H_GetUInt64Param, _T("M"), DCI_DT_UINT64, _T("Host memory size") },
   { _T("VMGR.Host.CPU.Model(*)"), H_GetStringParam, _T("M"), DCI_DT_STRING, _T("Host CPU model name") },
   { _T("VMGR.Host.CPU.Frequency(*)"), H_GetUIntParam, _T("F"), DCI_DT_UINT, _T("Host CPU frequency") },
   { _T("VMGR.Host.ConnectionType(*)"), H_GetStringParam, _T("C"), DCI_DT_STRING, _T("Connection type") },
   { _T("VMGR.Host.LibraryVersion(*)"), H_GetUInt64Param, _T("L"), DCI_DT_UINT64, _T("Library version") },
   { _T("VMGR.Host.ConnectionVersion(*)"), H_GetUInt64Param, _T("V"), DCI_DT_UINT64, _T("Connection version") },
   { _T("VMGR.VM.Memory.Used(*)"), H_GetVMInfoAsParam, _T("U"), DCI_DT_UINT64, _T("Memory currently used by VM") },
   { _T("VMGR.VM.Memory.UsedPrec(*)"), H_GetVMInfoAsParam, _T("P"), DCI_DT_UINT, _T("Percentage of currently memory usage by VM") },
   { _T("VMGR.VM.Memory.Max(*)"), H_GetVMInfoAsParam, _T("M"), DCI_DT_UINT64, _T("Maximum VM available memory") },
   { _T("VMGR.VM.CPU.Time(*)"), H_GetVMInfoAsParam, _T("C"), DCI_DT_UINT64, _T("Maximum VM CPU time") }
};

/**
 * Supported lists(enums)
 */
static NETXMS_SUBAGENT_LIST s_lists[] =
{
	{ _T("VMGR.VMHost"), H_GetHostList, NULL },
	{ _T("VMGR.VMList(*)"), H_GetVMList, NULL }
};

/**
 * Supported tables
 */
static NETXMS_SUBAGENT_TABLE m_tables[] =
{
	{ _T("VMGR.VM(*)"), H_GetVMTable, NULL, _T("NAME"), _T("Connection VM table") },
	{ _T("VMGR.InterfaceList(*)"), H_GetIfaceTable, NULL, _T("NAME"), _T("Connection interface list") },
	{ _T("VMGR.VMDisks(*)"), H_GetVMDiskTable, NULL, _T("DNAME"), _T("VM Disks") },
	{ _T("VMGR.VMController(*)"), H_GetVMControllerTable, NULL, _T("TYPE"), _T("VM Controllers") },
	{ _T("VMGR.VMInterface(*)"), H_GetVMInterfaceTable, NULL, _T("MAC"), _T("VM Interfaces") },
	{ _T("VMGR.VMVideo(*)"), H_GetVMVideoTable, NULL, _T("TYPE"), _T("VM Video adapter settings") },
	{ _T("VMGR.Networks(*)"), H_GetNetworksTable, NULL, _T("Name"), _T("Networks table") },
	{ _T("VMGR.Storages(*)"), H_GetStoragesTable, NULL, _T("Name"), _T("Storages table") }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO m_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("VMGR"), NETXMS_VERSION_STRING,
   SubagentInit, SubagentShutdown, ProcessCommands,
	sizeof(s_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	s_parameters,
	sizeof(s_lists) / sizeof(NETXMS_SUBAGENT_LIST),
	s_lists,
	sizeof(m_tables) / sizeof(NETXMS_SUBAGENT_TABLE),
	m_tables,
   0, NULL, // actions
   0, NULL  // push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(VMGR)
{
   *ppInfo = &m_info;
   return TRUE;
}

#ifdef _WIN32

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif
