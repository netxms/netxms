/*
** NetXMS PING subagent
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
** $module: ping.cpp
**
**/

#include <nms_common.h>
#include <nms_agent.h>

#ifdef _WIN32
#define PING_EXPORTABLE __declspec(dllexport) __cdecl
#else
#define PING_EXPORTABLE
#endif


//
// Hanlder functions
//

static LONG H_IcmpPing(char *pszParam, char *pArg, char *pValue)
{
   TCHAR szHostName[256], szTimeOut[32];
   DWORD dwAddr, dwTimeOut, dwRTT;

   if (!NxGetParameterArg(pszParam, 1, szHostName, 256))
      return SYSINFO_RC_UNSUPPORTED;
   StrStrip(szHostName);
   if (!NxGetParameterArg(pszParam, 2, szTimeOut, 256))
      return SYSINFO_RC_UNSUPPORTED;
   StrStrip(szTimeOut);

   dwAddr = _t_inet_addr(szHostName);
   if (szTimeOut[0] != 0)
      dwTimeOut = _tcstoul(szTimeOut, NULL, 0);
   if (dwTimeOut < 500)
      dwTimeOut = 500;
   if (dwTimeOut > 5000)
      dwTimeOut = 5000;

   if (IcmpPing(dwAddr, 3, dwTimeOut, &dwRTT) != ICMP_SUCCESS)
      dwRTT = 10000;
   ret_uint(pValue, dwRTT);
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
   { _T("Icmp.Ping(*)"), H_IcmpPing, NULL, DCI_DT_UINT, "ICMP ping responce time for *" }
};
//static NETXMS_SUBAGENT_ENUM m_enums[] =
//{
//   { "Skeleton.Enum", H_Enum, NULL }
//};

static NETXMS_SUBAGENT_INFO m_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
	_T("PING"), _T(NETXMS_VERSION_STRING),
   UnloadHandler, NULL,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	0, //sizeof(m_enums) / sizeof(NETXMS_SUBAGENT_ENUM),
	NULL //m_enums
};


//
// Entry point for NetXMS agent
//

#ifdef _NETWARE
extern "C" BOOL PING_EXPORTABLE NxSubAgentInit_PING(NETXMS_SUBAGENT_INFO **ppInfo, TCHAR *pszConfigFile)
#else
extern "C" BOOL PING_EXPORTABLE NxSubAgentInit(NETXMS_SUBAGENT_INFO **ppInfo, TCHAR *pszConfigFile)
#endif
{
   *ppInfo = &m_info;
   return TRUE;
}


//
// DLL entry point
//

#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   return TRUE;
}

#endif


//
// NetWare library entry point
//

#ifdef _NETWARE

int _init(void)
{
   return 0;
}

int _fini(void)
{
   return 0;
}

#endif
