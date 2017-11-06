/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
** File: winsrv.cpp
**
**/

#include "nxcproxy.h"

/**
 * Service handle
 */
static SERVICE_STATUS_HANDLE s_serviceHandle;

/**
 * Service control handler
 */
static VOID WINAPI ServiceCtrlHandler(DWORD ctrlCode)
{
   SERVICE_STATUS status;

   status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
   status.dwCurrentState = SERVICE_RUNNING;
   status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
   status.dwWin32ExitCode = 0;
   status.dwServiceSpecificExitCode = 0;
   status.dwCheckPoint = 0;
   status.dwWaitHint = 0;

   switch(ctrlCode)
   {
      case SERVICE_CONTROL_STOP:
      case SERVICE_CONTROL_SHUTDOWN:
         status.dwCurrentState = SERVICE_STOP_PENDING;
         status.dwWaitHint = 60000;
         SetServiceStatus(s_serviceHandle, &status);

         Shutdown();

         status.dwCurrentState = SERVICE_STOPPED;
         status.dwWaitHint = 0;
         break;
      default:
         break;
   }

   SetServiceStatus(s_serviceHandle, &status);
}

/**
 * Service main
 */
static VOID WINAPI ProxyServiceMain(DWORD argc, LPTSTR *argv)
{
   SERVICE_STATUS status;

   s_serviceHandle = RegisterServiceCtrlHandler(NXCPROXY_SERVICE_NAME, ServiceCtrlHandler);

   // Now we start service initialization
   status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
   status.dwCurrentState = SERVICE_START_PENDING;
   status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
   status.dwWin32ExitCode = 0;
   status.dwServiceSpecificExitCode = 0;
   status.dwCheckPoint = 0;
   status.dwWaitHint = 300000;   // Assume that startup can take up to 5 minutes
   SetServiceStatus(s_serviceHandle, &status);

   // Actual initialization
   if (!Initialize())
   {
      // Now service is stopped
      status.dwCurrentState = SERVICE_STOPPED;
      status.dwWaitHint = 0;
      SetServiceStatus(s_serviceHandle, &status);
      return;
   }

   // Now service is running
   status.dwCurrentState = SERVICE_RUNNING;
   status.dwWaitHint = 0;
   SetServiceStatus(s_serviceHandle, &status);

   SetProcessShutdownParameters(0x3FF, 0);

   Main();
}

/**
 * Initialize service
 */
void InitService()
{
   static SERVICE_TABLE_ENTRY serviceTable[2] = { { NXCPROXY_SERVICE_NAME, ProxyServiceMain }, { NULL, NULL } };
	TCHAR errorText[1024];

   if (!StartServiceCtrlDispatcher(serviceTable))
      _tprintf(_T("StartServiceCtrlDispatcher() failed: %s\n"), GetSystemErrorText(GetLastError(), errorText, 1024));
}

/**
 * Create service
 */
void InstallService(const TCHAR *pszExecName, const TCHAR *pszLogin, const TCHAR *pszPassword)
{
   SC_HANDLE hMgr, hService;
   TCHAR szCmdLine[MAX_PATH * 2], errorText[1024];

   hMgr = OpenSCManager(NULL, NULL, GENERIC_WRITE);
   if (hMgr == NULL)
   {
      _tprintf(_T("ERROR: Cannot connect to Service Manager (%s)\n"),
             GetSystemErrorText(GetLastError(), errorText, 1024));
      return;
   }

   _sntprintf(szCmdLine, MAX_PATH * 2, _T("\"%s\" -d --config \"%s\""), pszExecName, g_configFile);
   hService = CreateService(hMgr, NXCPROXY_SERVICE_NAME, _T("NetXMS Client Proxy"),
                            GENERIC_READ, SERVICE_WIN32_OWN_PROCESS,
                            SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
                            szCmdLine, NULL, NULL, NULL, pszLogin, pszPassword);
   if (hService == NULL)
   {
      DWORD code = GetLastError();

      if (code == ERROR_SERVICE_EXISTS)
         _tprintf(_T("ERROR: Service named '") NXCPROXY_SERVICE_NAME _T("' already exist\n"));
      else
         _tprintf(_T("ERROR: Cannot create service (%s)\n"), GetSystemErrorText(code, errorText, 1024));
   }
   else
   {
      _tprintf(_T("NetXMS Client Proxy service created successfully\n"));
      CloseServiceHandle(hService);
   }

   CloseServiceHandle(hMgr);

   InstallEventSource(pszExecName);
}

/**
 * Remove service
 */
void RemoveService()
{
   SC_HANDLE mgr, service;
	TCHAR errorText[1024];

   mgr = OpenSCManager(NULL, NULL, GENERIC_WRITE);
   if (mgr == NULL)
   {
      _tprintf(_T("ERROR: Cannot connect to Service Manager (%s)\n"), GetSystemErrorText(GetLastError(), errorText, 1024));
      return;
   }

   service = OpenService(mgr, NXCPROXY_SERVICE_NAME, DELETE);
   if (service == NULL)
   {
      _tprintf(_T("ERROR: Cannot open service named '") NXCPROXY_SERVICE_NAME _T("' (%s)\n"),
             GetSystemErrorText(GetLastError(), errorText, 1024));
   }
   else
   {
      if (DeleteService(service))
         _tprintf(_T("NetXMS Client Proxy service deleted successfully\n"));
      else
         _tprintf(_T("ERROR: Cannot remove service named '") NXCPROXY_SERVICE_NAME _T("' (%s)\n"),
                GetSystemErrorText(GetLastError(), errorText, 1024));

      CloseServiceHandle(service);
   }

   CloseServiceHandle(mgr);

   RemoveEventSource();
}

/**
 * Start service
 */
void StartProxyService()
{
   SC_HANDLE mgr, service;
	TCHAR errorText[1024];

   mgr = OpenSCManager(NULL, NULL, GENERIC_WRITE);
   if (mgr == NULL)
   {
      _tprintf(_T("ERROR: Cannot connect to Service Manager (%s)\n"), GetSystemErrorText(GetLastError(), errorText, 1024));
      return;
   }

   service = OpenService(mgr, NXCPROXY_SERVICE_NAME, SERVICE_START);
   if (service == NULL)
   {
      _tprintf(_T("ERROR: Cannot open service named '") NXCPROXY_SERVICE_NAME _T("' (%s)\n"),
             GetSystemErrorText(GetLastError(), errorText, 1024));
   }
   else
   {
      if (StartService(service, 0, NULL))
         _tprintf(_T("NetXMS Client Proxy service started successfully\n"));
      else
         _tprintf(_T("ERROR: Cannot start service named '") NXCPROXY_SERVICE_NAME _T("' (%s)\n"),
                GetSystemErrorText(GetLastError(), errorText, 1024));

      CloseServiceHandle(service);
   }

   CloseServiceHandle(mgr);
}

/**
 * Stop service
 */
void StopProxyService()
{
   SC_HANDLE mgr,service;
	TCHAR errorText[1024];

   mgr = OpenSCManager(NULL, NULL, GENERIC_WRITE);
   if (mgr == NULL)
   {
      _tprintf(_T("ERROR: Cannot connect to Service Manager (%s)\n"), GetSystemErrorText(GetLastError(), errorText, 1024));
      return;
   }

   service = OpenService(mgr, NXCPROXY_SERVICE_NAME, SERVICE_STOP);
   if (service == NULL)
   {
      _tprintf(_T("ERROR: Cannot open service named '") NXCPROXY_SERVICE_NAME _T("' (%s)\n"),
             GetSystemErrorText(GetLastError(), errorText, 1024));
   }
   else
   {
      SERVICE_STATUS status;

      if (ControlService(service, SERVICE_CONTROL_STOP, &status))
         _tprintf(_T("NetXMS Client Proxy service stopped successfully\n"));
      else
         _tprintf(_T("ERROR: Cannot stop service named '") NXCPROXY_SERVICE_NAME _T("' (%s)\n"),
                GetSystemErrorText(GetLastError(), errorText, 1024));

      CloseServiceHandle(service);
   }

   CloseServiceHandle(mgr);
}

/**
 * Install event source
 */
void InstallEventSource(const TCHAR *path)
{
   HKEY hKey;
   DWORD dwTypes = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE;
	TCHAR errorText[1024];

   if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_LOCAL_MACHINE,
         _T("System\\CurrentControlSet\\Services\\EventLog\\System\\") NXCPROXY_EVENT_SOURCE,
         0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, NULL))
   {
      _tprintf(_T("Unable to create registry key: %s\n"), GetSystemErrorText(GetLastError(), errorText, 1024));
      return;
   }

   RegSetValueEx(hKey, _T("TypesSupported"), 0, REG_DWORD,(BYTE *)&dwTypes, sizeof(DWORD));
   RegSetValueEx(hKey, _T("EventMessageFile"), 0, REG_EXPAND_SZ,(BYTE *)path, (DWORD)((_tcslen(path) + 1) * sizeof(TCHAR)));

   RegCloseKey(hKey);
   _tprintf(_T("Event source \"") NXCPROXY_EVENT_SOURCE _T("\" installed successfully\n"));
}

/**
 * Remove event source
 */
void RemoveEventSource()
{
	TCHAR errorText[1024];

   if (ERROR_SUCCESS == RegDeleteKey(HKEY_LOCAL_MACHINE,
         _T("System\\CurrentControlSet\\Services\\EventLog\\System\\") NXCPROXY_EVENT_SOURCE))
   {
      _tprintf(_T("Event source \"") NXCPROXY_EVENT_SOURCE _T("\" uninstalled successfully\n"));
   }
   else
   {
      _tprintf(_T("Unable to uninstall event source \"") NXCPROXY_EVENT_SOURCE _T("\": %s\n"),
             GetSystemErrorText(GetLastError(), errorText, 1024));
   }
}
