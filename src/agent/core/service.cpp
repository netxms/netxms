/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003-2023 Victor Kirhenshtein
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
** File: service.cpp
**
**/

#include "nxagentd.h"

/**
 * Global variables
 */
TCHAR g_windowsServiceName[MAX_PATH] = DEFAULT_AGENT_SERVICE_NAME;
TCHAR g_windowsServiceDisplayName[MAX_PATH] = _T("NetXMS Agent");
TCHAR g_windowsEventSourceName[MAX_PATH] = DEFAULT_AGENT_EVENT_SOURCE;

/**
 * Service handle
 */
static SERVICE_STATUS_HANDLE serviceHandle;

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
         status.dwWaitHint = 4000;
         SetServiceStatus(serviceHandle, &status);

			DebugPrintf(1, _T("Service control code %d - calling Shutdown()"), ctrlCode);
         Shutdown();

         status.dwCurrentState = SERVICE_STOPPED;
         status.dwWaitHint = 0;
         break;
      default:
         break;
   }

   SetServiceStatus(serviceHandle, &status);
}

/**
 * Service main
 */
static VOID WINAPI AgentServiceMain(DWORD argc, LPTSTR *argv)
{
   SERVICE_STATUS status;

   serviceHandle = RegisterServiceCtrlHandler(g_windowsServiceName, ServiceCtrlHandler);

   // Now we start service initialization
   status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
   status.dwCurrentState = SERVICE_START_PENDING;
   status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
   status.dwWin32ExitCode = 0;
   status.dwServiceSpecificExitCode = 0;
   status.dwCheckPoint = 0;
   status.dwWaitHint = 10000 + g_startupDelay * 1000;
   SetServiceStatus(serviceHandle, &status);

   // Actual initialization
   if (!Initialize())
   {
      Shutdown();

      // Now service is stopped
      status.dwCurrentState = SERVICE_STOPPED;
      status.dwWaitHint = 0;
      SetServiceStatus(serviceHandle, &status);
      return;
   }

   // Now service is running
   status.dwCurrentState = SERVICE_RUNNING;
   status.dwWaitHint = 0;
   SetServiceStatus(serviceHandle, &status);

   Main();
}

/**
 * Initialize service
 */
void InitService()
{
   static SERVICE_TABLE_ENTRY serviceTable[2]={ { g_windowsServiceName, AgentServiceMain }, { nullptr, nullptr } };
   if (!StartServiceCtrlDispatcher(serviceTable))
   {
      TCHAR errorText[256];
      _tprintf(_T("StartServiceCtrlDispatcher() failed: %s\n"), GetSystemErrorText(GetLastError(), errorText, 256));
   }
}

/**
 * Create service
 */
void InstallService(TCHAR *execName, TCHAR *confFile, int debugLevel)
{
   SC_HANDLE mgr, service;
   TCHAR cmdLine[8192];

   mgr = OpenSCManagerW(nullptr, nullptr, GENERIC_WRITE);
   if (mgr == nullptr)
   {
      WCHAR errorText[256];
      wprintf(L"ERROR: Cannot connect to Service Manager (%s)\n", GetSystemErrorText(GetLastError(), errorText, 256));
      return;
   }

   snwprintf(cmdLine, 8192, L"\"%s\" -d -c \"%s\" -n \"%s\" -e \"%s\"", execName, confFile, g_windowsServiceName, g_windowsEventSourceName);
   if (debugLevel != NXCONFIG_UNINITIALIZED_VALUE)
   {
      size_t len = _tcslen(cmdLine);
      snwprintf(&cmdLine[len], 8192 - len, L" -D %d", debugLevel);
   }
	
	if (g_szPlatformSuffix[0] != 0)
	{
		wcscat(cmdLine, L" -P \"");
		wcscat(cmdLine, g_szPlatformSuffix);
		wcscat(cmdLine, L"\"");
	}
	if (g_dwFlags & AF_CENTRAL_CONFIG)
	{
		wcscat(cmdLine, L" -M \"");
		wcscat(cmdLine, g_szConfigServer);
		wcscat(cmdLine, L"\"");
	}
   
   bool update = false;
	service = CreateServiceW(mgr, g_windowsServiceName, g_windowsServiceDisplayName, GENERIC_READ | SERVICE_CHANGE_CONFIG | SERVICE_START,
         SERVICE_WIN32_OWN_PROCESS | ((g_dwFlags & AF_INTERACTIVE_SERVICE) ? SERVICE_INTERACTIVE_PROCESS : 0),
         SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, cmdLine, nullptr, nullptr, nullptr, nullptr, nullptr);
   if (service == nullptr)
   {
      DWORD code = GetLastError();
      if (code == ERROR_SERVICE_EXISTS)
      {
         wprintf(L"WARNING: Service named '%s' already exist\n", g_windowsServiceName);

         // Open service for update in case we are updating older installation
         service = OpenServiceW(mgr, g_windowsServiceName, SERVICE_CHANGE_CONFIG | SERVICE_START);
         update = true;
      }
      else
      {
         TCHAR errorText[256];
         _tprintf(_T("ERROR: Cannot create service (%s)\n"), GetSystemErrorText(code, errorText, 256));
      }
   }

   // Set or update service description and recovery mode
   if (service != nullptr)
   {
      bool warnings = false;

      SERVICE_DESCRIPTIONW sd;
      sd.lpDescription = L"This service collects system health and performance information and forwards it to remote NetXMS server";
      if (!ChangeServiceConfig2W(service, SERVICE_CONFIG_DESCRIPTION, &sd))
      {
         WCHAR errorText[1024];
         wprintf(L"WARNING: cannot set service description (%s)\n", GetSystemErrorText(GetLastError(), errorText, 1024));
         warnings = true;
      }

      SC_ACTION restartServiceAction;
      restartServiceAction.Type = SC_ACTION_RESTART;
      restartServiceAction.Delay = 5000;

      SERVICE_FAILURE_ACTIONSW failureActions;
      memset(&failureActions, 0, sizeof(failureActions));
      failureActions.dwResetPeriod = 3600;
      failureActions.cActions = 1;
      failureActions.lpsaActions = &restartServiceAction;
      if (!ChangeServiceConfig2W(service, SERVICE_CONFIG_FAILURE_ACTIONS, &failureActions))
      {
         WCHAR errorText[1024];
         wprintf(L"WARNING: cannot set service failure actions (%s)\n", GetSystemErrorText(GetLastError(), errorText, 1024));
         warnings = true;
      }

      wprintf(L"Service \"%s\" %s %s\n", g_windowsServiceName, update ? L"updated" : L"created", warnings ? L"with warnings" : L"successfully");
      CloseServiceHandle(service);

		InstallEventSource(execName);
   }

   CloseServiceHandle(mgr);
}

/**
 * Remove service
 */
void RemoveService()
{
   TCHAR szErrorText[256];

   SC_HANDLE mgr = OpenSCManagerW(nullptr, nullptr, GENERIC_WRITE);
   if (mgr == nullptr)
   {
      _tprintf(_T("ERROR: Cannot connect to Service Manager (%s)\n"), GetSystemErrorText(GetLastError(), szErrorText, 256));
      return;
   }

   SC_HANDLE service = OpenServiceW(mgr, g_windowsServiceName, DELETE);
   if (service != nullptr)
   {
      if (DeleteService(service))
         _tprintf(_T("NetXMS Agent service deleted successfully\n"));
      else
         _tprintf(_T("ERROR: Cannot remove service named '%s' (%s)\n"), g_windowsServiceName, GetSystemErrorText(GetLastError(), szErrorText, 256));
      CloseServiceHandle(service);
   }
   else
   {
      _tprintf(_T("ERROR: Cannot open service named '%s' (%s)\n"), g_windowsServiceName, GetSystemErrorText(GetLastError(), szErrorText, 256));
   }

   CloseServiceHandle(mgr);

   RemoveEventSource();
}

/**
 * Start service
 */
void StartAgentService()
{
   SC_HANDLE mgr, service;
   TCHAR szErrorText[256];

   mgr = OpenSCManager(NULL, NULL, GENERIC_WRITE);
   if (mgr == NULL)
   {
      _tprintf(_T("ERROR: Cannot connect to Service Manager (%s)\n"),
             GetSystemErrorText(GetLastError(), szErrorText, 256));
      return;
   }

   service = OpenService(mgr, g_windowsServiceName, SERVICE_START);
   if (service == NULL)
   {
      _tprintf(_T("ERROR: Cannot open service named '%s' (%s)\n"), g_windowsServiceName,
             GetSystemErrorText(GetLastError(), szErrorText, 256));
   }
   else
   {
      if (StartService(service, 0, NULL))
         _tprintf(_T("Win32 Agent service started successfully\n"));
      else
         _tprintf(_T("ERROR: Cannot start service named '%s' (%s)\n"), g_windowsServiceName,
                GetSystemErrorText(GetLastError(), szErrorText, 256));

      CloseServiceHandle(service);
   }

   CloseServiceHandle(mgr);
}

/**
 * Stop service
 */
void StopAgentService()
{
   SC_HANDLE mgr, service;
   TCHAR szErrorText[256];

   mgr = OpenSCManager(NULL, NULL, GENERIC_WRITE);
   if (mgr == NULL)
   {
      _tprintf(_T("ERROR: Cannot connect to Service Manager (%s)\n"), 
             GetSystemErrorText(GetLastError(), szErrorText, 256));
      return;
   }

   service = OpenService(mgr, g_windowsServiceName, SERVICE_STOP);
   if (service == NULL)
   {
      _tprintf(_T("ERROR: Cannot open service named '%s' (%s)\n"), g_windowsServiceName,
             GetSystemErrorText(GetLastError(), szErrorText, 256));
   }
   else
   {
      SERVICE_STATUS status;

      if (ControlService(service, SERVICE_CONTROL_STOP, &status))
         _tprintf(_T("Win32 Agent service stopped successfully\n"));
      else
         _tprintf(_T("ERROR: Cannot stop service named '%s' (%s)\n"), g_windowsServiceName,
                GetSystemErrorText(GetLastError(), szErrorText, 256));

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
   TCHAR szErrorText[256], key[MAX_PATH];

	_sntprintf(key, MAX_PATH, _T("System\\CurrentControlSet\\Services\\EventLog\\System\\%s"), g_windowsEventSourceName);
   if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_LOCAL_MACHINE, key,
         0,NULL,REG_OPTION_NON_VOLATILE,KEY_SET_VALUE,NULL,&hKey,NULL))
   {
      _tprintf(_T("Unable to create registry key: %s\n"),
             GetSystemErrorText(GetLastError(), szErrorText, 256));
      return;
   }

   RegSetValueEx(hKey, _T("TypesSupported"), 0, REG_DWORD, (BYTE *)&dwTypes, sizeof(DWORD));
   RegSetValueEx(hKey, _T("EventMessageFile"), 0, REG_EXPAND_SZ, (BYTE *)path, (DWORD)((_tcslen(path) + 1) * sizeof(TCHAR)));

   RegCloseKey(hKey);
   _tprintf(_T("Event source \"%s\" installed successfully\n"), g_windowsEventSourceName);
}

/**
 * Remove event source
 */
void RemoveEventSource()
{
   TCHAR szErrorText[256], key[MAX_PATH];

	_sntprintf(key, MAX_PATH, _T("System\\CurrentControlSet\\Services\\EventLog\\System\\%s"), g_windowsEventSourceName);
   if (ERROR_SUCCESS == RegDeleteKey(HKEY_LOCAL_MACHINE, key))
   {
      _tprintf(_T("Event source \"%s\" uninstalled successfully\n"), g_windowsEventSourceName);
   }
   else
   {
      _tprintf(_T("Unable to uninstall event source \"%s\": %s\n"), g_windowsEventSourceName,
               GetSystemErrorText(GetLastError(), szErrorText, 256));
   }
}

/**
 * Wait for service
 */
bool WaitForService(DWORD dwDesiredState)
{
   SC_HANDLE mgr, service;
   TCHAR szErrorText[256];
   bool bResult = false;

   mgr = OpenSCManager(NULL, NULL, GENERIC_WRITE);
   if (mgr == NULL)
   {
      _tprintf(_T("ERROR: Cannot connect to Service Manager (%s)\n"), 
             GetSystemErrorText(GetLastError(), szErrorText, 256));
      return false;
   }

   service = OpenService(mgr, g_windowsServiceName, SERVICE_QUERY_STATUS);
   if (service == NULL)
   {
      _tprintf(_T("ERROR: Cannot open service named '%s' (%s)\n"), g_windowsServiceName,
             GetSystemErrorText(GetLastError(), szErrorText, 256));
   }
   else
   {
      SERVICE_STATUS status;

      while(1)
      {
         if (QueryServiceStatus(service, &status))
         {
            if (status.dwCurrentState == dwDesiredState)
            {
               bResult = true;
               break;
            }
         }
         else
         {
            break;   // Error
         }
         Sleep(200);
      }

      CloseServiceHandle(service);
   }

   CloseServiceHandle(mgr);
   return bResult;
}
