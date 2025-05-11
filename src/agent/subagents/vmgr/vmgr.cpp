/*
 ** Vmgr management subagent
 ** Copyright (C) 2016-2020 Raden Solutions
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
#include <netxms-version.h>

/**
 * General VM connection list
 */
StringObjectMap<HostConnections> g_connectionList(Ownership::True);

/**
 * Configuration file template for host list
 */
static TCHAR *s_hostList = nullptr;
static NX_CFG_TEMPLATE m_cfgTemplate[] =
{
	{ _T("Host"), CT_STRING_CONCAT, _T('\n'), 0, 0, 0, &s_hostList },
	{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr }
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
	{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr }
};

/**
 * Adds connection object to array if it was possible to connect to it
 */
static bool ConnectToHost(const TCHAR *name)
{
   HostConnections *conn = new HostConnections(name, s_url, s_login, s_password);
   bool success = conn->connect();
   if (success)
   {
      g_connectionList.set(name, conn);
   }
   else
   {
      delete conn;
   }
   return success;
}

/**
 * Add host connection from configuration file
 */
static void AddHostConnectionFromConfig(TCHAR *hostName, Config *config)
{
   TCHAR section[512];
   _sntprintf(section, 512, _T("VMGR:%s"), hostName);
   s_url[0] = 0;
   s_login[0] = 0;
   s_password[0] = 0;
	bool success = config->parseTemplate(section, m_aditionalCfgTemplate);
	if (success)
	{
	   TrimA(s_url);
	   TrimA(s_login);
	   TrimA(s_password);
	}

   if (!success || s_url[0] == 0 || !ConnectToHost(hostName))
   {
      nxlog_write_tag(NXLOG_WARNING, VMGR_DEBUG_TAG, _T("Unable to add host connection \"%s\" with URL \"%hs\""), hostName, s_url);
   }
   else
   {
      nxlog_debug_tag(VMGR_DEBUG_TAG, 6, _T("VMGR: VM successfully added: URL \"%hs\", name \"%s\""), s_url, hostName);
   }
}

/**
 * Libvirt error logger
 */
static void LibvirtErrorLogger(void *context, virErrorPtr err)
{
   int level;
   switch(err->level)
   {
      case VIR_ERR_WARNING:
         level = NXLOG_WARNING;
         break;
      case VIR_ERR_ERROR:
         level = NXLOG_ERROR;
         break;
      default:
         level = NXLOG_INFO;
         break;
   }
   nxlog_write_tag(level, VMGR_DEBUG_TAG, _T("%hs"), err->message);
}

/**
 * Subagent initialization
 */
static bool SubagentInit(Config *config)
{
	// Parse configuration
   if (!config->parseTemplate(_T("VMGR"), m_cfgTemplate))
      return false;

   virSetErrorFunc(nullptr, LibvirtErrorLogger);

   if (s_hostList != nullptr)
	{
      TCHAR *item, *itemEnd;
      for(item = itemEnd = s_hostList; (*item != 0) && (itemEnd != nullptr); item = itemEnd + 1)
      {
         itemEnd = _tcschr(item, _T('\n'));
         if (itemEnd != nullptr)
            *itemEnd = 0;
         AddHostConnectionFromConfig(Trim(item), config);
      }
   }

   if (s_hostList == nullptr || g_connectionList.size() == 0)
   {
      nxlog_write_tag(NXLOG_WARNING, VMGR_DEBUG_TAG, _T("No connections defined, VMGR subagent will not start"));
      MemFree(s_hostList);
      return false;
   }

   MemFree(s_hostList);

   nxlog_write_tag(NXLOG_INFO, VMGR_DEBUG_TAG, _T("VMGR subagent initialized"));
   return true;
}

/**
 * Called by master agent at unload
 */
static void SubagentShutdown()
{
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
	{ _T("VMGR.VMHost"), H_GetHostList, nullptr },
	{ _T("VMGR.VMList(*)"), H_GetVMList, nullptr }
};

/**
 * Supported tables
 */
static NETXMS_SUBAGENT_TABLE s_tables[] =
{
	{ _T("VMGR.VM(*)"), H_GetVMTable, nullptr, _T("NAME"), _T("Connection VM table") },
	{ _T("VMGR.InterfaceList(*)"), H_GetIfaceTable, nullptr, _T("NAME"), _T("Connection interface list") },
	{ _T("VMGR.VMDisks(*)"), H_GetVMDiskTable, nullptr, _T("DNAME"), _T("VM Disks") },
	{ _T("VMGR.VMController(*)"), H_GetVMControllerTable, nullptr, _T("TYPE"), _T("VM Controllers") },
	{ _T("VMGR.VMInterface(*)"), H_GetVMInterfaceTable, nullptr, _T("MAC"), _T("VM Interfaces") },
	{ _T("VMGR.VMVideo(*)"), H_GetVMVideoTable, nullptr, _T("TYPE"), _T("VM Video adapter settings") },
	{ _T("VMGR.Networks(*)"), H_GetNetworksTable, nullptr, _T("Name"), _T("Networks table") },
	{ _T("VMGR.Storages(*)"), H_GetStoragesTable, nullptr, _T("Name"), _T("Storages table") }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO s_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("VMGR"), NETXMS_VERSION_STRING,
   SubagentInit, SubagentShutdown, nullptr, nullptr, nullptr,
	sizeof(s_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	s_parameters,
	sizeof(s_lists) / sizeof(NETXMS_SUBAGENT_LIST),
	s_lists,
	sizeof(s_tables) / sizeof(NETXMS_SUBAGENT_TABLE),
	s_tables,
   0, nullptr, // actions
   0, nullptr  // push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(VMGR)
{
   *ppInfo = &s_info;
   return true;
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
