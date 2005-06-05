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

#include "ping.h"

#ifdef _WIN32
#define PING_EXPORTABLE __declspec(dllexport) __cdecl
#else
#define PING_EXPORTABLE
#endif


//
// Static data
//

static CONDITION m_hCondShutdown = INVALID_CONDITION_HANDLE;
static BOOL m_bShutdown = FALSE;
static DWORD m_dwNumTargets = 0;
static PING_TARGET *m_pTargetList = NULL;
static DWORD m_dwTimeout = 3000;    // Default timeout is 3 seconds


//
// Poller thread
//

static THREAD_RESULT THREAD_CALL PollerThread(void *pArg)
{
   QWORD qwStartTime;
   DWORD i, dwSum, dwElapsedTime;

   while(!m_bShutdown)
   {
      qwStartTime = GetCurrentTimeMs();
      if (IcmpPing(((PING_TARGET *)pArg)->dwIpAddr, 3, m_dwTimeout, &((PING_TARGET *)pArg)->dwLastRTT) != ICMP_SUCCESS)
         ((PING_TARGET *)pArg)->dwLastRTT = 10000;

      ((PING_TARGET *)pArg)->pdwHistory[((PING_TARGET *)pArg)->iBufPos++] = ((PING_TARGET *)pArg)->dwLastRTT;
      if (((PING_TARGET *)pArg)->iBufPos == POLLS_PER_MINUTE)
         ((PING_TARGET *)pArg)->iBufPos = 0;
      for(i = 0, dwSum = 0; i < POLLS_PER_MINUTE; i++)
         dwSum += ((PING_TARGET *)pArg)->pdwHistory[i];
      ((PING_TARGET *)pArg)->dwAvgRTT = dwSum / POLLS_PER_MINUTE;

      dwElapsedTime = (DWORD)(GetCurrentTimeMs() - qwStartTime);

      if (ConditionWait(m_hCondShutdown, 60000 / POLLS_PER_MINUTE - dwElapsedTime))
         break;
   }
   return THREAD_OK;
}


//
// Hanlder for immediate ping request
//

static LONG H_IcmpPing(TCHAR *pszParam, TCHAR *pArg, TCHAR *pValue)
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
// Handler for poller information
//

static LONG H_PollResult(TCHAR *pszParam, TCHAR *pArg, TCHAR *pValue)
{
   TCHAR szTarget[MAX_DB_STRING];
   DWORD i, dwIpAddr;
   BOOL bUseName = FALSE;

   if (!NxGetParameterArg(pszParam, 1, szTarget, MAX_DB_STRING))
      return SYSINFO_RC_UNSUPPORTED;
   StrStrip(szTarget);

   dwIpAddr = _t_inet_addr(szTarget);
   if ((dwIpAddr == INADDR_ANY) || (dwIpAddr == INADDR_NONE))
      bUseName = TRUE;

   for(i = 0; i < m_dwNumTargets; i++)
   {
      if (bUseName)
      {
         if (!_tcsicmp(m_pTargetList[i].szName, szTarget))
            break;
      }
      else
      {
         if (m_pTargetList[i].dwIpAddr == dwIpAddr)
            break;
      }
   }

   if (i == m_dwNumTargets)
      return SYSINFO_RC_UNSUPPORTED;   // No such target

   if (*pArg == _T('A'))
      ret_uint(pValue, m_pTargetList[i].dwAvgRTT);
   else
      ret_uint(pValue, m_pTargetList[i].dwLastRTT);

   return SYSINFO_RC_SUCCESS;
}


//
// Handler for configured target list
//

static LONG H_TargetList(TCHAR *pszParam, TCHAR *pArg, NETXMS_VALUES_LIST *pValue)
{
   DWORD i;
   TCHAR szBuffer[MAX_DB_STRING + 64], szIpAddr[16];

   for(i = 0; i < m_dwNumTargets; i++)
   {
      _stprintf(szBuffer, _T("%s %lu %lu %s"), IpToStr(ntohl(m_pTargetList[i].dwIpAddr), szIpAddr),
                m_pTargetList[i].dwLastRTT, m_pTargetList[i].dwAvgRTT, 
                m_pTargetList[i].szName);
      NxAddResultString(pValue, szBuffer);
   }

   return SYSINFO_RC_SUCCESS;
}


//
// Called by master agent at unload
//

static void UnloadHandler(void)
{
   m_bShutdown = TRUE;
   if (m_hCondShutdown != INVALID_CONDITION_HANDLE)
      SetEvent(m_hCondShutdown);
   Sleep(500);
}


//
// Add target from configuration file parameter
// Parameter value should be <ip_address>:<name>
// Name part is optional and can be missing
//

static BOOL AddTargetFromConfig(TCHAR *pszCfg)
{
   TCHAR *ptr, *pszLine;
   DWORD dwIpAddr;
   BOOL bResult = FALSE;

   pszLine = _tcsdup(pszCfg);
   ptr = _tcschr(pszLine, _T(':'));
   if (ptr != NULL)
   {
      *ptr = 0;
      ptr++;
      StrStrip(ptr);
   }
   StrStrip(pszLine);

   dwIpAddr = _t_inet_addr(pszLine);
   if ((dwIpAddr != INADDR_ANY) && (dwIpAddr != INADDR_NONE))
   {
      m_pTargetList = (PING_TARGET *)realloc(m_pTargetList, sizeof(PING_TARGET) * (m_dwNumTargets + 1));
      memset(&m_pTargetList[m_dwNumTargets], 0, sizeof(PING_TARGET));
      m_pTargetList[m_dwNumTargets].dwIpAddr = dwIpAddr;
      if (ptr != NULL)
         _tcsncpy(m_pTargetList[m_dwNumTargets].szName, ptr, MAX_DB_STRING);
      else
         IpToStr(ntohl(dwIpAddr), m_pTargetList[m_dwNumTargets].szName);
      m_dwNumTargets++;
      bResult = TRUE;
   }

   free(pszLine);
   return bResult;
}


//
// Subagent information
//

static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
   { _T("Icmp.AvgPingTime(*)"), H_PollResult, _T("A"), DCI_DT_UINT, _T("Average responce time of ICMP ping to {instance} for last minute") },
   { _T("Icmp.LastPingTime(*)"), H_PollResult, _T("L"), DCI_DT_UINT, _T("Responce time of last ICMP ping to {instance}") },
   { _T("Icmp.Ping(*)"), H_IcmpPing, NULL, DCI_DT_UINT, _T("ICMP ping responce time for {instance}") }
};
static NETXMS_SUBAGENT_ENUM m_enums[] =
{
   { _T("Icmp.TargetList"), H_TargetList, NULL }
};

static NETXMS_SUBAGENT_INFO m_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
	_T("PING"), _T(NETXMS_VERSION_STRING),
   UnloadHandler, NULL,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	sizeof(m_enums) / sizeof(NETXMS_SUBAGENT_ENUM),
	m_enums
};


//
// Configuration file template
//

static TCHAR *m_pszTargetList = NULL;
static NX_CFG_TEMPLATE cfgTemplate[] =
{
   { _T("Target"), CT_STRING_LIST, _T('\n'), 0, 0, 0, &m_pszTargetList },
   { _T("Timeout"), CT_LONG, 0, 0, 0, 0, &m_dwTimeout },
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
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
   DWORD i, dwResult;

   // Load configuration
   dwResult = NxLoadConfig(pszConfigFile, _T("Ping"), cfgTemplate, FALSE);
   if (dwResult == NXCFG_ERR_OK)
   {
      TCHAR *pItem, *pEnd;

      // Parse target list
      if (m_pszTargetList != NULL)
      {
         for(pItem = m_pszTargetList; *pItem != 0; pItem = pEnd + 1)
         {
            pEnd = _tcschr(pItem, _T('\n'));
            if (pEnd != NULL)
               *pEnd = 0;
            StrStrip(pItem);
            if (!AddTargetFromConfig(pItem))
               NxWriteAgentLog(EVENTLOG_WARNING_TYPE,
                               _T("Unable to add ICMP ping target from configuration file. "
                                  "Original configuration record: %s",), pItem);
         }
         free(m_pszTargetList);
      }

      // Create shutdown condition and start poller threads
      m_hCondShutdown = ConditionCreate(TRUE);
      for(i = 0; i < m_dwNumTargets; i++)
         ThreadCreate(PollerThread, 0, &m_pTargetList[i]);
   }
   else
   {
      safe_free(m_pszTargetList);
   }

   *ppInfo = &m_info;
   return dwResult == NXCFG_ERR_OK;
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
