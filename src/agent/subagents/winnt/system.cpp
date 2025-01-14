/* 
** Windows 2000+ NetXMS subagent
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
#include <wuapi.h>
#include <netfw.h>
#include <comdef.h>
#include <VersionHelpers.h>

/**
 * Handler for System.ServiceState parameter
 */
LONG H_ServiceState(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR serviceName[MAX_PATH];
   if (!AgentGetParameterArg(cmd, 1, serviceName, MAX_PATH))
      return SYSINFO_RC_UNSUPPORTED;

   SC_HANDLE hManager = OpenSCManager(nullptr, nullptr, GENERIC_READ);
   if (hManager == nullptr)
      return SYSINFO_RC_ERROR;

   LONG rc;
   SC_HANDLE hService = OpenService(hManager, serviceName, SERVICE_QUERY_STATUS);
   if (hService != nullptr)
   {
      SERVICE_STATUS status;
      if (QueryServiceStatus(hService, &status))
      {
         static DWORD dwStates[7] = { SERVICE_RUNNING, SERVICE_PAUSED, SERVICE_START_PENDING,
                                      SERVICE_PAUSE_PENDING, SERVICE_CONTINUE_PENDING,
                                      SERVICE_STOP_PENDING, SERVICE_STOPPED };

         int i;
         for(i = 0; i < 7; i++)
            if (status.dwCurrentState == dwStates[i])
               break;
         ret_uint(value, i);
         rc = SYSINFO_RC_SUCCESS;
      }
      else
      {
         rc = SYSINFO_RC_ERROR;    // Unable to retrieve information
      }
      CloseServiceHandle(hService);
   }
   else
   {
      rc = SYSINFO_RC_UNSUPPORTED;
   }

   CloseServiceHandle(hManager);
   return rc;
}

/**
 * Handler for System.Services list
 */
LONG H_ServiceList(const TCHAR *pszCmd, const TCHAR *pArg, StringList *value, AbstractCommSession *session)
{
   SC_HANDLE hManager = OpenSCManager(nullptr, nullptr, GENERIC_READ);
   if (hManager == nullptr)
      return SYSINFO_RC_ERROR;

   LONG rc = SYSINFO_RC_ERROR;
   DWORD bytes, count;
   EnumServicesStatusEx(hManager, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL, nullptr, 0, &bytes, &count, nullptr, nullptr);
   if (GetLastError() == ERROR_MORE_DATA)
   {
      auto services = static_cast<ENUM_SERVICE_STATUS_PROCESS*>(MemAlloc(bytes));
      if (EnumServicesStatusEx(hManager, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL, (BYTE *)services, bytes, &bytes, &count, nullptr, nullptr))
      {
         for(DWORD i = 0; i < count; i++)
            value->add(services[i].lpServiceName);
         rc = SYSINFO_RC_SUCCESS;
      }
      MemFree(services);
   }

   CloseServiceHandle(hManager);
   return rc;
}

/**
 * Handler for System.Services table
 */
LONG H_ServiceTable(const TCHAR *pszCmd, const TCHAR *pArg, Table *value, AbstractCommSession *session)
{
   SC_HANDLE hManager = OpenSCManager(nullptr, nullptr, GENERIC_READ);
   if (hManager == nullptr)
      return SYSINFO_RC_ERROR;

   LONG rc = SYSINFO_RC_ERROR;
   DWORD bytes, count;
   EnumServicesStatusEx(hManager, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL, nullptr, 0, &bytes, &count, nullptr, nullptr);
   if (GetLastError() == ERROR_MORE_DATA)
   {
      auto services = static_cast<ENUM_SERVICE_STATUS_PROCESS*>(MemAlloc(bytes));
      if (EnumServicesStatusEx(hManager, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL, (BYTE *)services, bytes, &bytes, &count, nullptr, nullptr))
      {
         value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"), true);
         value->addColumn(_T("DISPNAME"), DCI_DT_STRING, _T("Display name"));
         value->addColumn(_T("TYPE"), DCI_DT_STRING, _T("Type"));
         value->addColumn(_T("STATE"), DCI_DT_STRING, _T("State"));
         value->addColumn(_T("STARTUP"), DCI_DT_STRING, _T("Startup"));
         value->addColumn(_T("RUN_AS"), DCI_DT_STRING, _T("Run As"));
         value->addColumn(_T("PID"), DCI_DT_UINT, _T("PID"));
         value->addColumn(_T("BINARY"), DCI_DT_STRING, _T("Binary"));
         value->addColumn(_T("DEPENDENCIES"), DCI_DT_STRING, _T("Dependencies"));

         SERVICE_DELAYED_AUTO_START_INFO delayedStartInfo;
         auto triggerInfo = static_cast<SERVICE_TRIGGER_INFO*>(alloca(8192));

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
               value->set(6, static_cast<uint32_t>(services[i].ServiceStatusProcess.dwProcessId));

            SC_HANDLE hService = OpenService(hManager, services[i].lpServiceName, SERVICE_QUERY_CONFIG);
            if (hService != nullptr)
            {
               BYTE buffer[8192];
               auto cfg = reinterpret_cast<QUERY_SERVICE_CONFIG*>(&buffer);
               if (QueryServiceConfig(hService, cfg, 8192, &bytes))
               {
                  StringBuffer startType;
                  DWORD bytesNeeded;

                  delayedStartInfo.fDelayedAutostart = FALSE;
                  triggerInfo->cTriggers = 0;
                  QueryServiceConfig2(hService, SERVICE_CONFIG_TRIGGER_INFO, reinterpret_cast<BYTE*>(triggerInfo), 8192, &bytesNeeded);

                  switch(cfg->dwStartType)
                  {
                     case SERVICE_AUTO_START:
                        QueryServiceConfig2(hService, SERVICE_CONFIG_DELAYED_AUTO_START_INFO, reinterpret_cast<BYTE*>(&delayedStartInfo), sizeof(delayedStartInfo), &bytesNeeded);
                        startType = _T("Auto");
                        break;
                     case SERVICE_BOOT_START:
                        startType = _T("Boot");
                        break;
                     case SERVICE_DEMAND_START:
                        startType = _T("Manual");
                        break;
                     case SERVICE_DISABLED:
                        startType = _T("Disabled");
                        break;
                     case SERVICE_SYSTEM_START:
                        startType = _T("System");
                        break;
                     default:
                        startType.append(static_cast<uint32_t>(cfg->dwStartType));
                        break;
                  }
                  if (delayedStartInfo.fDelayedAutostart || (triggerInfo->cTriggers > 0))
                  {
                     startType.append(_T(" ("));
                     if (delayedStartInfo.fDelayedAutostart)
                        startType.append(_T("Delayed Start"));
                     if (triggerInfo->cTriggers > 0)
                     {
                        if (delayedStartInfo.fDelayedAutostart)
                           startType.append(_T(", "));
                        startType.append(_T("Trigger Start"));
                     }
                     startType.append(_T(")"));
                  }
                  value->set(4, startType);

                  value->set(5, cfg->lpServiceStartName);
                  value->set(7, cfg->lpBinaryPathName);
                  value->set(8, cfg->lpDependencies);
               }
               CloseServiceHandle(hService);
            }
         }
         rc = SYSINFO_RC_SUCCESS;
      }
      MemFree(services);
   }

   CloseServiceHandle(hManager);
   return rc;
}

/**
 * Change start type for given service
 */
static uint32_t ChangeServiceStartType(SC_HANDLE hService, DWORD startType, const TCHAR *serviceName)
{
   if (ChangeServiceConfig(hService, SERVICE_NO_CHANGE, startType, SERVICE_NO_CHANGE, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr))
   {
      nxlog_debug_tag(DEBUG_TAG, 3, _T("H_ServiceControl: start type for service \"%s\" changed to %u"), serviceName, startType);
      return ERR_SUCCESS;
   }

   TCHAR buffer[1024];
   nxlog_debug_tag(DEBUG_TAG, 3, _T("H_ServiceControl: cannot stop service \"%s\" (%s)"), serviceName, GetSystemErrorText(GetLastError(), buffer, 1024));
   return ERR_INTERNAL_ERROR;
}

/**
 * Handler for service control actions
 */
uint32_t H_ServiceControl(const shared_ptr<ActionExecutionContext>& context)
{
   if (!context->hasArgs())
      return ERR_BAD_ARGUMENTS;

   SC_HANDLE hManager = OpenSCManager(nullptr, nullptr, GENERIC_READ);
   if (hManager == nullptr)
      return ERR_INTERNAL_ERROR;

   TCHAR operation = *context->getData<TCHAR>();
   DWORD accessLevel;
   switch (operation)
   {
      case 's':
         accessLevel = SERVICE_START;
         break;
      case 'S':
         accessLevel = SERVICE_STOP;
         break;
      default:
         accessLevel = SERVICE_CHANGE_CONFIG;
         break;
   }

   uint32_t rc;
   SC_HANDLE hService = OpenService(hManager, context->getArg(0), accessLevel);
   if (hService != nullptr)
   {
      SERVICE_STATUS ss;
      switch (operation)
      {
         case 'A':   // Disable service
            rc = ChangeServiceStartType(hService, SERVICE_AUTO_START, context->getArg(0));
            break;
         case 'D':   // Disable service
            rc = ChangeServiceStartType(hService, SERVICE_DISABLED, context->getArg(0));
            break;
         case 'M':   // Manual start
            rc = ChangeServiceStartType(hService, SERVICE_DEMAND_START, context->getArg(0));
            break;
         case 's':
            if (StartService(hService, 0, nullptr))
            {
               nxlog_debug_tag(DEBUG_TAG, 3, _T("H_ServiceControl: service \"%s\" started"), context->getArg(0));
               rc = ERR_SUCCESS;
            }
            else
            {
               TCHAR buffer[1024];
               nxlog_debug_tag(DEBUG_TAG, 3, _T("H_ServiceControl: cannot start service \"%s\" (%s)"), context->getArg(0), GetSystemErrorText(GetLastError(), buffer, 1024));
               rc = ERR_INTERNAL_ERROR;
            }
            break;
         case 'S':
            if (ControlService(hService, SERVICE_CONTROL_STOP, &ss))
            {
               nxlog_debug_tag(DEBUG_TAG, 3, _T("H_ServiceControl: service \"%s\" stopped"), context->getArg(0));
               rc = ERR_SUCCESS;
            }
            else
            {
               TCHAR buffer[1024];
               nxlog_debug_tag(DEBUG_TAG, 3, _T("H_ServiceControl: cannot stop service \"%s\" (%s)"), context->getArg(0), GetSystemErrorText(GetLastError(), buffer, 1024));
               rc = ERR_INTERNAL_ERROR;
            }
            break;
      }
      CloseServiceHandle(hService);
   }
   else
   {
      TCHAR buffer[1024];
      nxlog_debug_tag(DEBUG_TAG, 3, _T("H_ServiceControl: cannot open service \"%s\" (%s)"), context->getArg(0), GetSystemErrorText(GetLastError(), buffer, 1024));
      rc = ERR_INTERNAL_ERROR;
   }

   CloseServiceHandle(hManager);
   return rc;
}

/**
 * Handler for System.ThreadCount
 */
LONG H_ThreadCount(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   PERFORMANCE_INFORMATION pi;
   pi.cb = sizeof(PERFORMANCE_INFORMATION);
   if (!GetPerformanceInfo(&pi, sizeof(PERFORMANCE_INFORMATION)))
      return SYSINFO_RC_ERROR;
   ret_uint(value, pi.ThreadCount);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.HandleCount
 */
LONG H_HandleCount(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   PERFORMANCE_INFORMATION pi;
   pi.cb = sizeof(PERFORMANCE_INFORMATION);
   if (!GetPerformanceInfo(&pi, sizeof(PERFORMANCE_INFORMATION)))
      return SYSINFO_RC_ERROR;
   ret_uint(value, pi.HandleCount);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.ConnectedUsers parameter
 */
LONG H_ConnectedUsers(const TCHAR *pszCmd, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
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
 * Handler for System.ActiveUserSessions list
 */
LONG H_ActiveUserSessionsList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   LONG nRet;
   WTS_SESSION_INFO *sessions;
   DWORD count;
   if (WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &sessions, &count))
   {
      for(DWORD i = 0; i < count; i++)
		{
         if ((sessions[i].State == WTSActive) || (sessions[i].State == WTSConnected))
         {
            TCHAR *clientName, *userName;
            DWORD bytes;
            if (!WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, sessions[i].SessionId, WTSClientName, &clientName, &bytes))
            {
               clientName = nullptr;
            }
            if (!WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, sessions[i].SessionId, WTSUserName, &userName, &bytes))
            {
               userName = nullptr;
            }

            TCHAR buffer[1024];
            _sntprintf(buffer, 1024, _T("\"%s\" \"%s\" \"%s\""),
                  userName == nullptr ? _T("UNKNOWN") : userName, sessions[i].pWinStationName,
                  clientName == nullptr ? _T("UNKNOWN") : clientName);
            value->add(buffer);

            if (userName != nullptr)
               WTSFreeMemory(userName);
            if (clientName != nullptr)
               WTSFreeMemory(clientName);
         }
		}
      WTSFreeMemory(sessions);
      nRet = SYSINFO_RC_SUCCESS;
   }
   else
   {
      nRet = SYSINFO_RC_ERROR;
   }
   return nRet;
}

/**
 * Handler for System.ActiveUserSessions table
 */
LONG H_ActiveUserSessionsTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   LONG nRet;
   WTS_SESSION_INFO *sessions;
   DWORD count;
   if (WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &sessions, &count))
   {
      value->addColumn(_T("ID"), DCI_DT_UINT, _T("ID"), true);
      value->addColumn(_T("USER_NAME"), DCI_DT_STRING, _T("User name"));
      value->addColumn(_T("TERMINAL"), DCI_DT_STRING, _T("Terminal"));
      value->addColumn(_T("STATE"), DCI_DT_STRING, _T("State"));
      value->addColumn(_T("CLIENT_NAME"), DCI_DT_STRING, _T("Client name"));
      value->addColumn(_T("CLIENT_ADDRESS"), DCI_DT_STRING, _T("Client address"));
      value->addColumn(_T("CLIENT_DISPLAY"), DCI_DT_STRING, _T("Client display"));
      value->addColumn(_T("CONNECT_TIMESTAMP"), DCI_DT_UINT64, _T("Connect time"));
      value->addColumn(_T("LOGON_TIMESTAMP"), DCI_DT_UINT64, _T("Logon time"));
      value->addColumn(_T("IDLE_TIME"), DCI_DT_UINT, _T("Idle for"));

      DWORD consoleSessionId = WTSGetActiveConsoleSessionId();

      for (DWORD i = 0; i < count; i++)
      {
         if ((sessions[i].State == WTSActive) || (sessions[i].State == WTSDisconnected))
         {
            value->addRow();
            value->set(0, static_cast<uint32_t>(sessions[i].SessionId));
            value->set(3, (sessions[i].State == WTSActive) ? _T("Active") : _T("Disconnected"));

            WTSINFO *info;
            DWORD bytes;
            if (WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, sessions[i].SessionId, WTSSessionInfo, reinterpret_cast<LPTSTR*>(&info), &bytes))
            {
               TCHAR userName[256];
               _sntprintf(userName, 256, _T("%s\\%s"), info->Domain, info->UserName);
               value->set(1, userName);
               value->set(2, info->WinStationName);

               if (info->ConnectTime.QuadPart > 0)
                  value->set(7, FileTimeToUnixTime(info->ConnectTime));
               else
                  value->set(7, _T(""));

               if (info->LogonTime.QuadPart > 0)
                  value->set(8, FileTimeToUnixTime(info->LogonTime));
               else
                  value->set(8, _T(""));

               if (info->LastInputTime.QuadPart > 0)
                  value->set(9, FileTimeToUnixTime(info->CurrentTime) - FileTimeToUnixTime(info->LastInputTime));
               else
                  value->set(9, _T(""));

               WTSFreeMemory(info);
            }

            bool addressRetrieved = false;
            WTS_CLIENT_ADDRESS *clientAddress;
            if (WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, sessions[i].SessionId, WTSClientAddress, reinterpret_cast<LPTSTR*>(&clientAddress), &bytes))
            {
               if (clientAddress->AddressFamily == AF_INET)
                  value->set(5, InetAddress(ntohl(*reinterpret_cast<uint32_t*>(&clientAddress->Address[2]))).toString());
               else if (clientAddress->AddressFamily == AF_INET6)
                  value->set(5, InetAddress(clientAddress->Address).toString());
               WTSFreeMemory(clientAddress);
               addressRetrieved = true;
            }

            WTSCLIENT *client;
            if (WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, sessions[i].SessionId, WTSClientInfo, reinterpret_cast<LPTSTR*>(&client), &bytes))
            {
               value->set(4, client->ClientName);

               if (!addressRetrieved)
               {
                  if (client->ClientAddressFamily == AF_INET)
                  {
                     BYTE a[4];
                     for (int n = 0; n < 4; n++)
                        a[n] = static_cast<BYTE>(client->ClientAddress[n] >> 8);
                     value->set(5, InetAddress(ntohl(*reinterpret_cast<uint32_t*>(a))).toString());
                  }
                  else if (client->ClientAddressFamily == AF_INET6)
                  {
                     value->set(5, InetAddress(reinterpret_cast<uint8_t*>(client->ClientAddress)).toString());
                  }
               }

               if ((client->HRes > 0) && (client->VRes > 0) && (client->ColorDepth > 0) && (sessions[i].SessionId != consoleSessionId))
               {
                  // Translate color depth value to bits-per-pizel as per 
                  // https://learn.microsoft.com/en-us/windows/win32/api/wtsapi32/ns-wtsapi32-wts_client_display
                  uint32_t bpp;
                  switch (client->ColorDepth)
                  {
                     case 1:
                        bpp = 4;
                        break;
                     case 2:
                        bpp = 8;
                        break;
                     case 4:
                        bpp = 16;
                        break;
                     case 8:
                        bpp = 24;
                        break;
                     case 16:
                        bpp = 15;
                        break;
                     case 24:
                        bpp = 24;
                        break;
                     case 32:
                        bpp = 32;
                        break;
                     default:
                        bpp = 0;
                        break;
                  }

                  TCHAR clientScreen[256];
                  _sntprintf(clientScreen, 256, _T("%ux%ux%u"), client->HRes, client->VRes, bpp);
                  value->set(6, clientScreen);
               }
               else
               {
                  // Attempt to get screen resolution from session agent
                  uint32_t width, height, bpp;
                  if (AgentGetScreenInfoForUserSession(sessions[i].SessionId, &width, &height, &bpp))
                  {
                     if ((width > 0) && (height > 0) && (bpp > 0))
                     {
                        TCHAR clientScreen[256];
                        _sntprintf(clientScreen, 256, _T("%ux%ux%u"), width, height, bpp);
                        value->set(6, clientScreen);
                     }
                     else
                     {
                        nxlog_debug_tag(DEBUG_TAG, 6, _T("Invalid screen resolution data received from session agent"));
                     }
                  }
               }

               WTSFreeMemory(client);
            }
         }
      }
      WTSFreeMemory(sessions);
      nRet = SYSINFO_RC_SUCCESS;
   }
   else
   {
      nRet = SYSINFO_RC_ERROR;
   }
   return nRet;
}

/**
 * Terminate user session
 */
uint32_t H_TerminateUserSession(const shared_ptr<ActionExecutionContext>& context)
{
   if (!context->hasArgs())
      return ERR_BAD_ARGUMENTS;

   TCHAR *eptr;
   DWORD sid = _tcstoul(context->getArg(0), &eptr, 0);
   if ((*eptr != 0) || (sid == WTS_CURRENT_SESSION) || (sid == WTS_ANY_SESSION))
      return ERR_BAD_ARGUMENTS;

   if (!WTSLogoffSession(WTS_CURRENT_SERVER_HANDLE, sid, false))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("H_TerminateUserSession: call to WTSLogoffSession for SID %u failed (%s)"), sid, GetSystemErrorText(GetLastError()).cstr());
      return ERR_SYSCALL_FAILED;
   }

   nxlog_debug_tag(DEBUG_TAG, 4, _T("H_TerminateUserSession: call to WTSLogoffSession for SID %u successfull"), sid);
   return ERR_SUCCESS;
}

/**
 * Callback for window stations enumeration
 */
static BOOL CALLBACK WindowStationsEnumCallback(LPTSTR lpszWindowStation, LPARAM lParam)
{
   ((StringList*)lParam)->add(lpszWindowStation);
   return TRUE;
}

/**
 * Handler for System.WindowStations list
 */
LONG H_WindowStations(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   return EnumWindowStations(WindowStationsEnumCallback, (LONG_PTR)value) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
}

/**
 * Callback for desktop enumeration
 */
static BOOL CALLBACK DesktopsEnumCallback(LPTSTR lpszDesktop, LPARAM lParam)
{
   ((StringList*)lParam)->add(lpszDesktop);
   return TRUE;
}

/**
 * Handler for System.Desktops list
 */
LONG H_Desktops(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   TCHAR wsName[256];
   AgentGetParameterArg(cmd, 1, wsName, 256);
   HWINSTA ws = OpenWindowStation(wsName, FALSE, WINSTA_ENUMDESKTOPS);
   if (ws == NULL)
      return SYSINFO_RC_ERROR;

   LONG rc = EnumDesktops(ws, DesktopsEnumCallback, (LONG_PTR)value) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
   CloseWindowStation(ws);
   return rc;
}

/**
 * Handler for Agent.Desktop parameter
 */
LONG H_AgentDesktop(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   HWINSTA ws = GetProcessWindowStation();
   if (ws == NULL)
      return SYSINFO_RC_ERROR;

   HDESK desk = GetThreadDesktop(GetCurrentThreadId());
   if (desk == NULL)
      return SYSINFO_RC_ERROR;

   TCHAR wsName[64], deskName[64];
   DWORD size;
   if (GetUserObjectInformation(ws, UOI_NAME, wsName, 64 * sizeof(TCHAR), &size) &&
       GetUserObjectInformation(desk, UOI_NAME, deskName, 64 * sizeof(TCHAR), &size))
   {
      DWORD sid;
      if (ProcessIdToSessionId(GetCurrentProcessId(), &sid))
      {
         TCHAR *sessionName;
         if (WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, sid, WTSWinStationName, &sessionName, &size))
         {
            _sntprintf(value, MAX_RESULT_LENGTH, _T("/%s/%s/%s"), sessionName, wsName, deskName);
            WTSFreeMemory(sessionName);
         }
         else
         {
            _sntprintf(value, MAX_RESULT_LENGTH, _T("/%u/%s/%s"), sid, wsName, deskName);
         }
      }
      else
      {
         _sntprintf(value, MAX_RESULT_LENGTH, _T("/?/%s/%s"), wsName, deskName);
      }
      return SYSINFO_RC_SUCCESS;
   }
   else
   {
      return SYSINFO_RC_ERROR;
   }
}

/**
 * Handler for System.AppAddressSpace
 */
LONG H_AppAddressSpace(const TCHAR *pszCmd, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	DWORD_PTR size = (DWORD_PTR)si.lpMaximumApplicationAddress - (DWORD_PTR)si.lpMinimumApplicationAddress;
	ret_uint(pValue, static_cast<uint32_t>(size / 1024 / 1024));
	return SYSINFO_RC_SUCCESS;
}

/**
 * Read update time from registry
 */
static bool ReadSystemUpdateTimeFromRegistry(const TCHAR *type, TCHAR *value)
{
   TCHAR buffer[MAX_PATH];
   _sntprintf(buffer, MAX_PATH, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\Auto Update\\Results\\%s"), type);

   HKEY hKey;
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, buffer, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
      return false;

   DWORD size = MAX_PATH * sizeof(TCHAR);
   if (RegQueryValueEx(hKey, _T("LastSuccessTime"), NULL, NULL, (BYTE *)buffer, &size) != ERROR_SUCCESS)
   {
      RegCloseKey(hKey);
      return false;
   }
   RegCloseKey(hKey);

   // Date stored as YYYY-mm-dd HH:MM:SS in UTC
   if (_tcslen(buffer) != 19)
      return false;

   struct tm t;
   memset(&t, 0, sizeof(struct tm));
   t.tm_isdst = 0;

   buffer[4] = 0;
   t.tm_year = _tcstol(buffer, NULL, 10) - 1900;

   buffer[7] = 0;
   t.tm_mon = _tcstol(&buffer[5], NULL, 10) - 1;

   buffer[10] = 0;
   t.tm_mday = _tcstol(&buffer[8], NULL, 10);

   buffer[13] = 0;
   t.tm_hour = _tcstol(&buffer[11], NULL, 10);

   buffer[16] = 0;
   t.tm_min = _tcstol(&buffer[14], NULL, 10);

   t.tm_sec = _tcstol(&buffer[17], NULL, 10);

   ret_int64(value, timegm(&t));
   return true;
}

/**
* Read update time from COM component
*/
static bool ReadSystemUpdateTimeFromCOM(const TCHAR *type, TCHAR *value)
{
   HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
   if ((hr != S_OK) && (hr != S_FALSE))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Call to CoInitializeEx failed (0x%08x: %s)"), hr, _com_error(hr).ErrorMessage());
      return false;
   }

   bool success = false;

   IAutomaticUpdates2 *updateService;
   if (CoCreateInstance(CLSID_AutomaticUpdates, nullptr, CLSCTX_ALL, IID_IAutomaticUpdates2, (void**)&updateService) == S_OK)
   {
      // Enable call cancellation and attempt to cancel call after timeout
      // as there were reports of this call hanging
      hr = CoEnableCallCancellation(nullptr);
      if (hr == S_OK)
      {
         DWORD threadId = GetCurrentThreadId();
         volatile bool completed = false;
         AgentSetTimer(2500, 
            [threadId, &completed]() -> void
            {
               if (!completed)
               {
                  HRESULT hr = CoCancelCall(threadId, 0);
                  if (hr != S_OK)
                     nxlog_debug_tag(DEBUG_TAG, 5, _T("Call to CoCancelCall failed (0x%08x: %s)"), hr, _com_error(hr).ErrorMessage());
               }
            });

         IAutomaticUpdatesResults *results;
         hr = updateService->get_Results(&results);
         if (hr == S_OK)
         {
            bool detect = (_tcscmp(type, _T("Detect")) == 0);
            VARIANT v;
            HRESULT hr = detect ? results->get_LastSearchSuccessDate(&v) : results->get_LastInstallationSuccessDate(&v);
            completed = true;
            if (hr == S_OK)
            {
               if (v.vt == VT_DATE)
               {
                  SYSTEMTIME st;
                  VariantTimeToSystemTime(v.date, &st);

                  FILETIME ft;
                  SystemTimeToFileTime(&st, &ft);

                  ret_int64(value, FileTimeToUnixTime(ft)); // Convert to seconds
                  success = true;
               }
               else if (v.vt == VT_EMPTY)
               {
                  ret_int64(value, 0);
                  success = true;
               }
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG, 5, _T("Call to IAutomaticUpdatesResults::%s failed (0x%08x: %s)"), detect ? _T("get_LastSearchSuccessDate") : _T("get_LastInstallationSuccessDate"), hr, _com_error(hr).ErrorMessage());
            }
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("Call to IAutomaticUpdates2::get_Results failed (0x%08x: %s)"), hr, _com_error(hr).ErrorMessage());
         }
         CoDisableCallCancellation(nullptr);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Call to CoEnableCallCancellation failed (0x%08x: %s)"), hr, _com_error(hr).ErrorMessage());
      }
      updateService->Release();
   }

   CoUninitialize();
   return success;
}

/**
 * Handler for System.Update.*Time parameters
 */
LONG H_SysUpdateTime(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   if (ReadSystemUpdateTimeFromRegistry(arg, value))
      return SYSINFO_RC_SUCCESS;

   return ReadSystemUpdateTimeFromCOM(arg, value) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
}

/**
 * Handler for System.Uname parameter
 */
LONG H_SystemUname(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR computerName[MAX_COMPUTERNAME_LENGTH + 1];
   DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
   GetComputerName(computerName, &dwSize);

   OSVERSIONINFO versionInfo;
   versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
#pragma warning(push)
#pragma warning(disable : 4996)
   GetVersionEx(&versionInfo);
#pragma warning(pop)

   TCHAR osVersion[256];
   if (!GetWindowsVersionString(osVersion, 256))
   {
      switch(versionInfo.dwPlatformId)
      {
         case VER_PLATFORM_WIN32_WINDOWS:
            _sntprintf(osVersion, 256, _T("Windows %s-%s"), versionInfo.dwMinorVersion == 0 ? _T("95") :
               (versionInfo.dwMinorVersion == 10 ? _T("98") :
               (versionInfo.dwMinorVersion == 90 ? _T("Me") : _T("Unknown"))), versionInfo.szCSDVersion);
            break;
         case VER_PLATFORM_WIN32_NT:
            _sntprintf(osVersion, 256, _T("Windows NT %d.%d %s"), versionInfo.dwMajorVersion,
               versionInfo.dwMinorVersion, versionInfo.szCSDVersion);
            break;
         default:
            _tcscpy(osVersion, _T("Windows [Unknown Version]"));
            break;
         }
   }

   const TCHAR *cpuType;
   SYSTEM_INFO sysInfo;
   GetSystemInfo(&sysInfo);
   switch (sysInfo.wProcessorArchitecture)
   {
      case PROCESSOR_ARCHITECTURE_INTEL:
         cpuType = _T("Intel IA-32");
         break;
      case PROCESSOR_ARCHITECTURE_MIPS:
         cpuType = _T("MIPS");
         break;
      case PROCESSOR_ARCHITECTURE_ALPHA:
         cpuType = _T("Alpha");
         break;
      case PROCESSOR_ARCHITECTURE_PPC:
         cpuType = _T("PowerPC");
         break;
      case PROCESSOR_ARCHITECTURE_IA64:
         cpuType = _T("Intel IA-64");
         break;
      case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
         cpuType = _T("IA-32 on IA-64");
         break;
      case PROCESSOR_ARCHITECTURE_AMD64:
         cpuType = _T("AMD-64");
         break;
      case PROCESSOR_ARCHITECTURE_IA32_ON_ARM64:
         cpuType = _T("IA-32 on ARM-64");
         break;
      case PROCESSOR_ARCHITECTURE_ARM64:
         cpuType = _T("ARM-64");
         break;
      default:
         cpuType = _T("unknown");
         break;
   }

   _sntprintf(value, MAX_RESULT_LENGTH, _T("Windows %s %d.%d.%d %s %s"), computerName, versionInfo.dwMajorVersion,
      versionInfo.dwMinorVersion, versionInfo.dwBuildNumber, osVersion, cpuType);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.Architecture parameter
 */
LONG H_SystemArchitecture(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   SYSTEM_INFO sysInfo;
   GetSystemInfo(&sysInfo);
   switch (sysInfo.wProcessorArchitecture)
   {
      case PROCESSOR_ARCHITECTURE_INTEL:
         ret_string(value, _T("i686"));
         break;
      case PROCESSOR_ARCHITECTURE_MIPS:
         ret_string(value, _T("mips"));
         break;
      case PROCESSOR_ARCHITECTURE_ALPHA:
         ret_string(value, _T("alpha"));
         break;
      case PROCESSOR_ARCHITECTURE_PPC:
         ret_string(value, _T("ppc"));
         break;
      case PROCESSOR_ARCHITECTURE_IA64:
         ret_string(value, _T("ia64"));
         break;
      case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
      case PROCESSOR_ARCHITECTURE_AMD64:
         ret_string(value, _T("amd64"));
         break;
      case PROCESSOR_ARCHITECTURE_IA32_ON_ARM64:
      case PROCESSOR_ARCHITECTURE_ARM64:
         ret_string(value, _T("aarch64"));
         break;
      default:
         ret_string(value, _T("unknown"));
         break;
   }
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.OS.* parameters
 */
LONG H_SystemVersionInfo(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   OSVERSIONINFOEX versionInfo;
   versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
#pragma warning(push)
#pragma warning(disable : 4996)
   if (!GetVersionEx(reinterpret_cast<OSVERSIONINFO*>(&versionInfo)))
      return SYSINFO_RC_ERROR;
#pragma warning(pop)

   switch(*arg)
   {
      case 'B':
         ret_uint(value, versionInfo.dwBuildNumber);
         break;
      case 'S':
         _sntprintf(value, MAX_RESULT_LENGTH, _T("%u.%u"), versionInfo.wServicePackMajor, versionInfo.wServicePackMinor);
         break;
      case 'T':
         ret_string(value, versionInfo.wProductType == VER_NT_WORKSTATION ? _T("Workstation") : _T("Server"));
         break;
      case 'V':
         _sntprintf(value, MAX_RESULT_LENGTH, _T("%u.%u.%u"), versionInfo.dwMajorVersion, versionInfo.dwMinorVersion, versionInfo.dwBuildNumber);
         break;
   }

   return SYSINFO_RC_SUCCESS;
}

/**
 * Decode product key for Windows 7 and below
 */
static void DecodeProductKeyWin7(const BYTE *digitalProductId, TCHAR *decodedKey)
{
   static const char digits[] = "BCDFGHJKMPQRTVWXY2346789";

   BYTE pid[16];
   memcpy(pid, &digitalProductId[52], 16);
   for(int i = 28; i >= 0; i--)
   {
      if ((i + 1) % 6 == 0) // Every sixth char is a separator.
      {
         decodedKey[i] = '-';
      }
      else
      {
         int digitMapIndex = 0;
         for(int j = 14; j >= 0; j--)
         {
            int byteValue = (digitMapIndex << 8) | pid[j];
            pid[j] = (BYTE)(byteValue / 24);
            digitMapIndex = byteValue % 24;
            decodedKey[i] = digits[digitMapIndex];
         }
      }
   }
   decodedKey[29] = 0;
}

/**
 * Decode product key for Windows 8 and above
 */
static void DecodeProductKeyWin8(const BYTE *digitalProductId, TCHAR *decodedKey)
{
   static const char digits[] = "BCDFGHJKMPQRTVWXY2346789";

   BYTE pid[16];
   memcpy(pid, &digitalProductId[52], 16);

   BYTE isWin8 = (pid[15] / 6) & 1;
   pid[15] = (pid[15] & 0xF7) | ((isWin8 & 2) << 2);

   int digitMapIndex;
   for(int i = 24; i >= 0; i--)
   {
      digitMapIndex = 0;
      for (int j = 14; j >= 0; j--)
      {
         int byteValue = (digitMapIndex << 8) | pid[j];
         pid[j] = (BYTE)(byteValue / 24);
         digitMapIndex = byteValue % 24;
         decodedKey[i] = digits[digitMapIndex];
      }
   }

   TCHAR buffer[32];
   memcpy(buffer, &decodedKey[1], digitMapIndex * sizeof(TCHAR));
   buffer[digitMapIndex] = 'N';
   memcpy(&buffer[digitMapIndex + 1], &decodedKey[digitMapIndex + 1], (25 - digitMapIndex) * sizeof(TCHAR));

   for(int i = 0, j = 0; i < 25; i += 5, j += 6)
   {
      memcpy(&decodedKey[j], &buffer[i], 5 * sizeof(TCHAR));
      decodedKey[j + 5] = '-';
   }
   decodedKey[29] = 0;
}

/**
 * Handler for System.OS.ProductId parameters
 */
LONG H_SystemProductInfo(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   HKEY hKey;
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
   {
      TCHAR buffer[1024];
      nxlog_debug_tag(DEBUG_TAG, 5, _T("H_SystemProductInfo: Cannot open registry key (%s)"), GetSystemErrorText(GetLastError(), buffer, 1024));
      return SYSINFO_RC_ERROR;
   }

   LONG rc;
   BYTE buffer[1024];
   DWORD size = sizeof(buffer);
   if (RegQueryValueEx(hKey, arg, nullptr, nullptr, buffer, &size) == ERROR_SUCCESS)
   {
      if (!_tcscmp(arg, _T("DigitalProductId")))
      {
         if (IsWindows8OrGreater())
            DecodeProductKeyWin8(buffer, value);
         else
            DecodeProductKeyWin7(buffer, value);
      }
      else if (!_tcscmp(arg, _T("ProductName")))
      {
         if (!_tcsncmp(reinterpret_cast<TCHAR*>(buffer), _T("Windows 10 "), 11))
         {
            // Check if product is actually Windows 11 (at least first versions report "Windows 10" as product name)
            OSVERSIONINFOEX osInfo;
            osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
#pragma warning(push)
#pragma warning(disable : 4996)
            if (GetVersionEx(reinterpret_cast<OSVERSIONINFO*>(&osInfo)))
            {
               if (osInfo.dwBuildNumber >= 22000)
               {
                  *(reinterpret_cast<TCHAR*>(buffer) + 9) = _T('1');
               }
            }
#pragma warning(pop)
         }
         ret_string(value, reinterpret_cast<TCHAR*>(buffer));
      }
      else
      {
         ret_string(value, reinterpret_cast<TCHAR*>(buffer));
      }
      rc = SYSINFO_RC_SUCCESS;
   }
   else
   {
      TCHAR buffer[1024];
      nxlog_debug_tag(DEBUG_TAG, 5, _T("H_SystemProductInfo: Cannot read registry key %s (%s)"), arg, GetSystemErrorText(GetLastError(), buffer, 1024));
      rc = SYSINFO_RC_ERROR;
   }
   RegCloseKey(hKey);
   return rc;
}

/**
 * Handler for System.Memory.XXX parameters
 */
LONG H_MemoryInfo(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   MEMORYSTATUSEX mse;
   mse.dwLength = sizeof(MEMORYSTATUSEX);
   if (!GlobalMemoryStatusEx(&mse))
      return SYSINFO_RC_ERROR;

   switch (CAST_FROM_POINTER(arg, int))
   {
      case MEMINFO_PHYSICAL_AVAIL:
         ret_uint64(value, mse.ullAvailPhys);
         break;
      case MEMINFO_PHYSICAL_AVAIL_PCT:
         ret_double(value, ((double)mse.ullAvailPhys * 100.0 / mse.ullTotalPhys), 2);
         break;
      case MEMINFO_PHYSICAL_TOTAL:
         ret_uint64(value, mse.ullTotalPhys);
         break;
      case MEMINFO_PHYSICAL_USED:
         ret_uint64(value, mse.ullTotalPhys - mse.ullAvailPhys);
         break;
      case MEMINFO_PHYSICAL_USED_PCT:
         ret_double(value, (((double)mse.ullTotalPhys - mse.ullAvailPhys) * 100.0 / mse.ullTotalPhys), 2);
         break;
      case MEMINFO_VIRTUAL_FREE:
         ret_uint64(value, mse.ullAvailPageFile);
         break;
      case MEMINFO_VIRTUAL_FREE_PCT:
         ret_double(value, ((double)mse.ullAvailPageFile * 100.0 / mse.ullTotalPageFile), 2);
         break;
      case MEMINFO_VIRTUAL_TOTAL:
         ret_uint64(value, mse.ullTotalPageFile);
         break;
      case MEMINFO_VIRTUAL_USED:
         ret_uint64(value, mse.ullTotalPageFile - mse.ullAvailPageFile);
         break;
      case MEMINFO_VIRTUAL_USED_PCT:
         ret_double(value, (((double)mse.ullTotalPageFile - mse.ullAvailPageFile) * 100.0 / mse.ullTotalPageFile), 2);
         break;
      default:
         return SYSINFO_RC_UNSUPPORTED;
   }

   return SYSINFO_RC_SUCCESS;
}

/**
* Handler for System.Memory.XXX parameters
*/
LONG H_MemoryInfo2(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   PERFORMANCE_INFORMATION pi;
   if (!GetPerformanceInfo(&pi, sizeof(PERFORMANCE_INFORMATION)))
      return SYSINFO_RC_ERROR;

   switch (CAST_FROM_POINTER(arg, int))
   {
      case MEMINFO_PHYSICAL_FREE:
         ret_uint64(value, (pi.PhysicalAvailable - pi.SystemCache) * pi.PageSize);
         break;
      case MEMINFO_PHYSICAL_FREE_PCT:
         ret_double(value, ((double)(pi.PhysicalAvailable - pi.SystemCache) * 100.0 / pi.PhysicalTotal), 2);
         break;
      case MEMINFO_PHYSICAL_CACHE:
         ret_uint64(value, pi.SystemCache * pi.PageSize);
         break;
      case MEMINFO_PHYSICAL_CACHE_PCT:
         ret_double(value, ((double)pi.SystemCache * 100.0 / pi.PhysicalTotal), 2);
         break;
      default:
         return SYSINFO_RC_UNSUPPORTED;
   }

   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for System.Uptime parameter
 */
LONG H_Uptime(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   ret_uint(value, static_cast<uint32_t>(GetTickCount64() / 1000));
   return SYSINFO_RC_SUCCESS;
}

/**
 * Windows firewall profile names and codes
 */
static CodeLookupElement s_firewallProfiles[] =
{
   { NET_FW_PROFILE2_DOMAIN, _T("DOMAIN") },
   { NET_FW_PROFILE2_PRIVATE, _T("PRIVATE") },
   { NET_FW_PROFILE2_PUBLIC, _T("PUBLIC") },
   { 0, nullptr }
};

/**
 * Get state of Windows Firewall for specific profile
 */
LONG H_WindowsFirewallProfileState(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR profileName[64];
   if (!AgentGetParameterArg(cmd, 1, profileName, 64))
      return SYSINFO_RC_UNSUPPORTED;

   _tcsupr(profileName);
   int32_t profile = CodeFromText(profileName, s_firewallProfiles, -1);
   if (profile == -1)
      return SYSINFO_RC_UNSUPPORTED;

   HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
   if ((hr != S_OK) && (hr != S_FALSE))
      return SYSINFO_RC_ERROR;

   INetFwPolicy2 *fwPolicy;
   hr = CoCreateInstance(__uuidof(NetFwPolicy2), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwPolicy2), (void**)&fwPolicy);
   if (hr != S_OK)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("H_WindowsFirewallProfileState: call to CoCreateInstance(NetFwPolicy2) failed (0x%08x: %s)"), hr, _com_error(hr).ErrorMessage());
      CoUninitialize();
      return SYSINFO_RC_ERROR;
   }

   bool success = false;
   VARIANT_BOOL v = FALSE;
   hr = fwPolicy->get_FirewallEnabled((NET_FW_PROFILE_TYPE2)profile, &v);
   if (hr == S_OK)
   {
      success = true;
      ret_boolean(value, v ? true : false);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("H_WindowsFirewallProfileState: call to INetFwPolicy2::get_FirewallEnabled failed (0x%08x: %s)"), hr, _com_error(hr).ErrorMessage());
   }

   fwPolicy->Release();
   CoUninitialize();
   return success ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
}

/**
 * Get state of Windows Firewall for current profile(s)
 */
LONG H_WindowsFirewallState(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
   if ((hr != S_OK) && (hr != S_FALSE))
      return SYSINFO_RC_ERROR;

   INetFwPolicy2 *fwPolicy;
   hr = CoCreateInstance(__uuidof(NetFwPolicy2), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwPolicy2), (void**)&fwPolicy);
   if (hr != S_OK)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("H_WindowsFirewallState: call to CoCreateInstance(NetFwPolicy2) failed (0x%08x: %s)"), hr, _com_error(hr).ErrorMessage());
      CoUninitialize();
      return SYSINFO_RC_ERROR;
   }

   bool success;
   long profiles;
   hr = fwPolicy->get_CurrentProfileTypes(&profiles);
   if (hr == S_OK)
   {
      bool selected = false;
      bool enabled = true;
      for (int i = 1; i < 8; i = i << 1)
      {
         if ((profiles & i) == 0)
            continue;

         selected = true;  // At least one profile set as current

         VARIANT_BOOL v = FALSE;
         fwPolicy->get_FirewallEnabled((NET_FW_PROFILE_TYPE2)i, &v);
         if (!v)
         {
            enabled = false;
            break;
         }
      }

      ret_boolean(value, selected && enabled);  // TRUE if all selected profiles are enabled
      success = true;
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("H_WindowsFirewallState: call to INetFwPolicy2::get_CurrentProfileTypes failed (0x%08x: %s)"), hr, _com_error(hr).ErrorMessage());
      success = false;
   }

   fwPolicy->Release();
   CoUninitialize();
   return success ? SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
}

/**
 * Get current profile of Windows Firewall
 */
LONG H_WindowsFirewallCurrentProfile(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
   if ((hr != S_OK) && (hr != S_FALSE))
      return -1;

   INetFwPolicy2 *fwPolicy;
   hr = CoCreateInstance(__uuidof(NetFwPolicy2), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwPolicy2), (void**)&fwPolicy);
   if (hr != S_OK)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("H_WindowsFirewallCurrentProfile: call to CoCreateInstance(NetFwPolicy2) failed"));
      CoUninitialize();
      return SYSINFO_RC_ERROR;
   }

   long profiles;
   fwPolicy->get_CurrentProfileTypes(&profiles);

   StringBuffer sb;
   for (int i = 1; i < 8; i = i << 1)
   {
      if ((profiles & i) == 0)
         continue;

      const TCHAR *pn = CodeToText(i, s_firewallProfiles, _T(""));
      if (*pn != 0)
      {
         if (!sb.isEmpty())
            sb.append(_T(','));
         sb.append(pn);
      }
   }

   fwPolicy->Release();
   CoUninitialize();
   ret_string(value, sb);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Check if system restart is pending
 */
LONG H_SystemIsRestartPending(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   HKEY hKey;
   if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Session Manager", 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
   {
      TCHAR buffer[1024];
      nxlog_debug_tag(DEBUG_TAG, 5, L"H_SystemIsRestartPending: Cannot open registry key (%s)", GetSystemErrorText(GetLastError(), buffer, 1024));
      return SYSINFO_RC_ERROR;
   }

   BYTE buffer[1024];
   DWORD size = sizeof(buffer);
   LSTATUS status = RegQueryValueExW(hKey, L"PendingFileRenameOperations", nullptr, nullptr, buffer, &size);
   if ((status == ERROR_SUCCESS) || (status == ERROR_MORE_DATA))
   {
      nxlog_debug_tag(L"restart", 1, L"status=%d size=%d", (int)status, (int)size);
      ret_boolean(value, size > 2);
   }
   else
   {
      ret_boolean(value, false);
   }

   RegCloseKey(hKey);
   return SYSINFO_RC_SUCCESS;
}
