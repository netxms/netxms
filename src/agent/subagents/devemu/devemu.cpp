/*
 ** Device Emulation subagent
 ** Copyright (C) 2014 Raden Solutions
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

#ifdef _WIN32
#define DEVEMU_EXPORTABLE __declspec(dllexport) __cdecl
#else
#define DEVEMU_EXPORTABLE
#endif

/**
 * Configuration data
 */
static TCHAR s_ipAddress[32] = _T("10.0.0.1");
static TCHAR s_ipNetMask[32] = _T("255.0.0.0");
static TCHAR s_ifName[64] = _T("eth0");
static TCHAR s_macAddress[16] = _T("000000000000");
static TCHAR s_paramConfigFile[MAX_PATH] = _T("");

/**
 * Interface list
 */
static LONG H_InterfaceList(const TCHAR *pszParam, const TCHAR *pArg, StringList *value)
{
   TCHAR buffer[256];
   _sntprintf(buffer, 256, _T("1 %s/%d 6 %s %s"), s_ipAddress, 
              BitsInMask(ntohl(_t_inet_addr(s_ipNetMask))), s_macAddress, s_ifName);
   value->add(buffer);
   value->add(_T("255 127.0.0.1/8 24 000000000000 lo0"));
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for constant values
 */
static LONG H_Constant(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue)
{
   ret_string(pValue, pArg);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Subagent initialization
 */
static BOOL SubagentInit(Config *config) 
{
   return TRUE;
}

/**
 * Called by master agent at unload
 */
static void SubagentShutdown()
{
}

/**
 * Provided parameters (default set)
 */
static NETXMS_SUBAGENT_PARAM s_parameters[] =
{
   { _T("Net.Interface.AdminStatus(*)"), H_Constant, _T("1"), DCI_DT_INT, DCIDESC_NET_INTERFACE_ADMINSTATUS },
   { _T("Net.Interface.Link(*)"), H_Constant, _T("1"), DCI_DT_DEPRECATED, DCIDESC_DEPRECATED },
   { _T("Net.Interface.MTU(*)"), H_Constant, _T("1500"), DCI_DT_UINT, DCIDESC_NET_INTERFACE_MTU },
   { _T("Net.Interface.OperStatus(*)"), H_Constant, _T("1"), DCI_DT_INT, DCIDESC_NET_INTERFACE_OPERSTATUS },
   { _T("Net.IP.Forwarding"), H_Constant, _T("0"), DCI_DT_INT, DCIDESC_NET_IP_FORWARDING }
};

/**
 * Supported lists
 */
static NETXMS_SUBAGENT_LIST s_lists[] =
{
   { _T("Net.InterfaceList"), H_InterfaceList, NULL }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO m_info = 
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("DEVEMU"), NETXMS_VERSION_STRING,
   SubagentInit, SubagentShutdown, NULL,
   0, NULL, // parameters
	sizeof(s_lists) / sizeof(NETXMS_SUBAGENT_LIST),
	s_lists,
   0, NULL,	// tables
   0, NULL,	// actions
   0, NULL	// push parameters
};

/**
 * Configuration file template
 */
static NX_CFG_TEMPLATE m_cfgTemplate[] =
{
   { _T("InterfaceName"), CT_STRING, 0, 0, 64, 0, s_ifName },
   { _T("IpAddress"), CT_STRING, 0, 0, 32, 0, s_ipAddress },
   { _T("IpNetMask"), CT_STRING, 0, 0, 32, 0, s_ipNetMask },
   { _T("MacAddress"), CT_STRING, 0, 0, 16, 0, s_macAddress },
   { _T("ParametersConfig"), CT_STRING, 0, 0, MAX_PATH, 0, s_paramConfigFile },
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(DEVEMU)
{
	if (m_info.parameters != NULL)
		return FALSE;	// Most likely another instance of DEVEMU subagent already loaded

   if (!config->parseTemplate(_T("DEVEMU"), m_cfgTemplate))
      return FALSE;

   StructArray<NETXMS_SUBAGENT_PARAM> *parameters = new StructArray<NETXMS_SUBAGENT_PARAM>(s_parameters, sizeof(s_parameters) / sizeof(NETXMS_SUBAGENT_PARAM));

   // Add parameters from configuration

   m_info.numParameters = parameters->size();
   m_info.parameters = (NETXMS_SUBAGENT_PARAM *)nx_memdup(parameters->getBuffer(), parameters->size() * sizeof(NETXMS_SUBAGENT_PARAM));
   *ppInfo = &m_info;
   delete parameters;
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
