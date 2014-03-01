/* 
** Windows 2000+ NetXMS subagent
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
** File: system.cpp
** WinNT+ specific system information parameters
**/

#include "winnt_subagent.h"

/**
 * Handler for System.ServiceState parameter
 */
LONG H_ServiceState(const TCHAR *cmd, const TCHAR *arg, TCHAR *value)
{
   SC_HANDLE hManager, hService;
   TCHAR szServiceName[MAX_PATH];
   LONG iResult = SYSINFO_RC_SUCCESS;

   if (!AgentGetParameterArg(cmd, 1, szServiceName, MAX_PATH))
      return SYSINFO_RC_UNSUPPORTED;

   hManager = OpenSCManager(NULL, NULL, GENERIC_READ);
   if (hManager == NULL)
   {
      return SYSINFO_RC_ERROR;
   }

   hService = OpenService(hManager, szServiceName, SERVICE_QUERY_STATUS);
   if (hService == NULL)
   {
      iResult = SYSINFO_RC_UNSUPPORTED;
   }
   else
   {
      SERVICE_STATUS status;

      if (QueryServiceStatus(hService, &status))
      {
         int i;
         static DWORD dwStates[7]={ SERVICE_RUNNING, SERVICE_PAUSED, SERVICE_START_PENDING,
                                    SERVICE_PAUSE_PENDING, SERVICE_CONTINUE_PENDING,
                                    SERVICE_STOP_PENDING, SERVICE_STOPPED };

         for(i = 0; i < 7; i++)
            if (status.dwCurrentState == dwStates[i])
               break;
         ret_uint(value, i);
      }
      else
      {
         ret_uint(value, 255);    // Unable to retrieve information
      }
      CloseServiceHandle(hService);
   }

   CloseServiceHandle(hManager);
   return iResult;
}

/**
 * Handler for System.Services list
 */
LONG H_ServiceList(const TCHAR *pszCmd, const TCHAR *pArg, StringList *value)
{
   SC_HANDLE hManager = OpenSCManager(NULL, NULL, GENERIC_READ);
   if (hManager == NULL)
   {
      return SYSINFO_RC_ERROR;
   }

   LONG rc = SYSINFO_RC_ERROR;
   DWORD bytes, count;
   EnumServicesStatusEx(hManager, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL, NULL, 0, &bytes, &count, NULL, NULL);
   if (GetLastError() == ERROR_MORE_DATA)
   {
      ENUM_SERVICE_STATUS_PROCESS *services = (ENUM_SERVICE_STATUS_PROCESS *)malloc(bytes);
      if (EnumServicesStatusEx(hManager, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL, (BYTE *)services, bytes, &bytes, &count, NULL, NULL))
      {
         for(DWORD i = 0; i < count; i++)
            value->add(services[i].lpServiceName);
         rc = SYSINFO_RC_SUCCESS;
      }
      free(services);
   }

   CloseServiceHandle(hManager);
   return rc;
}

/**
 * Handler for System.Services table
 */
LONG H_ServiceTable(const TCHAR *pszCmd, const TCHAR *pArg, Table *value)
{
   SC_HANDLE hManager = OpenSCManager(NULL, NULL, GENERIC_READ);
   if (hManager == NULL)
   {
      return SYSINFO_RC_ERROR;
   }

   LONG rc = SYSINFO_RC_ERROR;
   DWORD bytes, count;
   EnumServicesStatusEx(hManager, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL, NULL, 0, &bytes, &count, NULL, NULL);
   if (GetLastError() == ERROR_MORE_DATA)
   {
      ENUM_SERVICE_STATUS_PROCESS *services = (ENUM_SERVICE_STATUS_PROCESS *)malloc(bytes);
      if (EnumServicesStatusEx(hManager, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL, (BYTE *)services, bytes, &bytes, &count, NULL, NULL))
      {
         value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"), true);
         value->addColumn(_T("DISPNAME"), DCI_DT_STRING, _T("Display name"));
         value->addColumn(_T("TYPE"), DCI_DT_STRING, _T("Type"));
         value->addColumn(_T("STATE"), DCI_DT_STRING, _T("State"));
         value->addColumn(_T("STARTUP"), DCI_DT_STRING, _T("Startup"));
         value->addColumn(_T("PID"), DCI_DT_UINT, _T("PID"));
         value->addColumn(_T("BINARY"), DCI_DT_STRING, _T("Binary"));
         value->addColumn(_T("DEPENDENCIES"), DCI_DT_STRING, _T("Dependencies"));
         for(DWORD i = 0; i < count; i++)
         {
            value->addRow();
            value->set(0, services[i].lpServiceName);
            value->set(1, services[i].lpDisplayName);
            value->set(2, (services[i].ServiceStatusProcess.dwServiceType == SERVICE_WIN32_SHARE_PROCESS) ? _T("Shared") : _T("Own"));
            switch(services[i].ServiceStatusProcess.dwCurrentState)
            {
               case SERVICE_CONTINUE_PENDING:
                  value->set(3, _T("Continue Pending"));
                  break;
               case SERVICE_PAUSE_PENDING:
                  value->set(3, _T("Pausing"));
                  break;
               case SERVICE_PAUSED:
                  value->set(3, _T("Paused"));
                  break;
               case SERVICE_RUNNING:
                  value->set(3, _T("Running"));
                  break;
               case SERVICE_START_PENDING:
                  value->set(3, _T("Starting"));
                  break;
               case SERVICE_STOP_PENDING:
                  value->set(3, _T("Stopping"));
                  break;
               case SERVICE_STOPPED:
                  value->set(3, _T("Stopped"));
                  break;
               default:
                  value->set(3, (UINT32)services[i].ServiceStatusProcess.dwCurrentState);
                  break;
            }
            if (services[i].ServiceStatusProcess.dwProcessId != 0)
               value->set(5, (UINT32)services[i].ServiceStatusProcess.dwProcessId);

            SC_HANDLE hService = OpenService(hManager, services[i].lpServiceName, SERVICE_QUERY_CONFIG);
            if (hService != NULL)
            {
               BYTE buffer[8192];
               QUERY_SERVICE_CONFIG *cfg = (QUERY_SERVICE_CONFIG *)&buffer;
               if (QueryServiceConfig(hService, cfg, 8192, &bytes))
               {
                  switch(cfg->dwStartType)
                  {
                     case SERVICE_AUTO_START:
                        value->set(4, _T("Auto"));
                        break;
                     case SERVICE_BOOT_START:
                        value->set(4, _T("Boot"));
                        break;
                     case SERVICE_DEMAND_START:
                        value->set(4, _T("Manual"));
                        break;
                     case SERVICE_DISABLED:
                        value->set(4, _T("Disabled"));
                        break;
                     case SERVICE_SYSTEM_START:
                        value->set(4, _T("System"));
                        break;
                     default:
                        value->set(4, (UINT32)cfg->dwStartType);
                        break;
                  }
                  value->set(6, cfg->lpBinaryPathName);
                  value->set(7, cfg->lpDependencies);
               }
               CloseServiceHandle(hService);
            }
         }
         rc = SYSINFO_RC_SUCCESS;
      }
      free(services);
   }

   CloseServiceHandle(hManager);
   return rc;
}

/**
 * Handler for System.ThreadCount
 */
LONG H_ThreadCount(const TCHAR *cmd, const TCHAR *arg, TCHAR *value)
{
   PERFORMANCE_INFORMATION pi;
   pi.cb = sizeof(PERFORMANCE_INFORMATION);
   if (!GetPerformanceInfo(&pi, sizeof(PERFORMANCE_INFORMATION)))
      return SYSINFO_RC_ERROR;
   ret_uint(value, pi.ThreadCount);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.ConnectedUsers parameter
 */
LONG H_ConnectedUsers(const TCHAR *pszCmd, const TCHAR *pArg, TCHAR *pValue)
{
   LONG nRet;
   WTS_SESSION_INFO *pSessionList;
   DWORD i, dwNumSessions, dwCount;

   if (WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pSessionList, &dwNumSessions))
   {
      for(i = 0, dwCount = 0; i < dwNumSessions; i++)
         if ((pSessionList[i].State == WTSActive) ||
             (pSessionList[i].State == WTSConnected))
            dwCount++;
      WTSFreeMemory(pSessionList);
      ret_uint(pValue, dwCount);
      nRet = SYSINFO_RC_SUCCESS;
   }
   else
   {
      nRet = SYSINFO_RC_ERROR;
   }
   return nRet;
}

/**
 * Handler for System.ActiveUserSessions enum
 */
LONG H_ActiveUserSessions(const TCHAR *pszCmd, const TCHAR *pArg, StringList *value)
{
   LONG nRet;
   WTS_SESSION_INFO *pSessionList;
   DWORD i, dwNumSessions, dwBytes;
   TCHAR *pszClientName, *pszUserName, szBuffer[1024];

   if (WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pSessionList, &dwNumSessions))
   {
      for(i = 0; i < dwNumSessions; i++)
		{
         if ((pSessionList[i].State == WTSActive) ||
             (pSessionList[i].State == WTSConnected))
         {
            if (!WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, pSessionList[i].SessionId,
                                            WTSClientName, &pszClientName, &dwBytes))
            {
               pszClientName = NULL;
            }
            if (!WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, pSessionList[i].SessionId,
                                            WTSUserName, &pszUserName, &dwBytes))
            {
               pszUserName = NULL;
            }

            _sntprintf(szBuffer, 1024, _T("\"%s\" \"%s\" \"%s\""),
                       pszUserName == NULL ? _T("UNKNOWN") : pszUserName,
                       pSessionList[i].pWinStationName,
                       pszClientName == NULL ? _T("UNKNOWN") : pszClientName);
            value->add(szBuffer);

            if (pszUserName != NULL)
               WTSFreeMemory(pszUserName);
            if (pszClientName != NULL)
               WTSFreeMemory(pszClientName);
         }
		}
      WTSFreeMemory(pSessionList);
      nRet = SYSINFO_RC_SUCCESS;
   }
   else
   {
      nRet = SYSINFO_RC_ERROR;
   }
   return nRet;
}

/**
 * Handler for System.AppAddressSpace
 */
LONG H_AppAddressSpace(const TCHAR *pszCmd, const TCHAR *pArg, TCHAR *pValue)
{
	SYSTEM_INFO si;

	GetSystemInfo(&si);
	DWORD_PTR size = (DWORD_PTR)si.lpMaximumApplicationAddress - (DWORD_PTR)si.lpMinimumApplicationAddress;
	ret_uint(pValue, (DWORD)(size / 1024 / 1024));
	return SYSINFO_RC_SUCCESS;
}
