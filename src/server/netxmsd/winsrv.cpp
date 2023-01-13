/* 
** NetXMS - Network Management System
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
** File: winsrv.cpp
**
**/

#include "netxmsd.h"

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

         if (ctrlCode == SERVICE_CONTROL_SHUTDOWN)
            FastShutdown(ShutdownReason::BY_SERVICE_MANAGER);
         else
            InitiateShutdown(ShutdownReason::BY_SERVICE_MANAGER);

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
static VOID WINAPI CoreServiceMain(DWORD argc, LPTSTR *argv)
{
   SERVICE_STATUS status;

   s_serviceHandle = RegisterServiceCtrlHandler(CORE_SERVICE_NAME, ServiceCtrlHandler);

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
      InitiateProcessShutdown();

      // Remove database lock
      if (g_flags & AF_DB_LOCKED)
         UnlockDatabase();
      ShutdownDatabase();

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

   Main(NULL);
}

/**
 * Initialize service
 */
void InitService()
{
   static SERVICE_TABLE_ENTRY serviceTable[2] = { { CORE_SERVICE_NAME, CoreServiceMain }, { nullptr, nullptr } };
   if (!StartServiceCtrlDispatcher(serviceTable))
   {
      TCHAR errorText[1024];
      wprintf(L"StartServiceCtrlDispatcher() failed: %s\n", GetSystemErrorText(GetLastError(), errorText, 1024));
   }
}

/**
 * Create service
 */
void InstallService(const TCHAR *execName, const TCHAR *dllName, const TCHAR *login, const TCHAR *password, bool manualStart)
{
   SC_HANDLE hMgr = OpenSCManagerW(nullptr, nullptr, GENERIC_WRITE);
   if (hMgr == nullptr)
   {
      TCHAR errorText[1024];
      wprintf(L"ERROR: Cannot connect to Service Manager (%s)\n", GetSystemErrorText(GetLastError(), errorText, 1024));
      return;
   }

   bool update = false;
   TCHAR szCmdLine[MAX_PATH * 2];
   _sntprintf(szCmdLine, MAX_PATH * 2, _T("\"%s\" -d --config \"%s\"%s"), execName, g_szConfigFile, g_checkDatabase ? _T(" --check-db") : _T(""));
   SC_HANDLE hService = CreateService(hMgr, CORE_SERVICE_NAME, _T("NetXMS Core"), GENERIC_READ | SERVICE_CHANGE_CONFIG | SERVICE_START,
         SERVICE_WIN32_OWN_PROCESS, manualStart ? SERVICE_DEMAND_START : SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
         szCmdLine, nullptr, nullptr, nullptr, login, password);
   if (hService == nullptr)
   {
      DWORD code = GetLastError();
      if (code == ERROR_SERVICE_EXISTS)
      {
         _tprintf(_T("WARNING: Service named '") CORE_SERVICE_NAME _T("' already exist\n"));

         // Open service for update in case we are updating older installation and change startup mode
         hService = OpenService(hMgr, CORE_SERVICE_NAME, SERVICE_CHANGE_CONFIG | SERVICE_START);
         if (hService != nullptr)
         {
            if (!ChangeServiceConfig(hService, SERVICE_NO_CHANGE, manualStart ? SERVICE_DEMAND_START : SERVICE_AUTO_START, SERVICE_NO_CHANGE, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr))
            {
               TCHAR errorText[1024];
               wprintf(L"WARNING: cannot set service start type (%s)\n", GetSystemErrorText(GetLastError(), errorText, 1024));
            }
         }
         update = true;
      }
      else
      {
         TCHAR errorText[1024];
         wprintf(L"ERROR: Cannot create service (%s)\n", GetSystemErrorText(code, errorText, 1024));
      }
   }

   if (hService != nullptr)
   {
      bool warnings = false;

      SERVICE_DESCRIPTIONW sd;
      sd.lpDescription = L"This service provides core functionality of NetXMS management server";
      if (!ChangeServiceConfig2(hService, SERVICE_CONFIG_DESCRIPTION, &sd))
      {
         TCHAR errorText[1024];
         wprintf(L"WARNING: cannot set service description (%s)\n", GetSystemErrorText(GetLastError(), errorText, 1024));
         warnings = true;
      }

      SC_ACTION restartServiceAction;
      restartServiceAction.Type = SC_ACTION_RESTART;
      restartServiceAction.Delay = 30000;

      SERVICE_FAILURE_ACTIONSW failureActions;
      memset(&failureActions, 0, sizeof(failureActions));
      failureActions.dwResetPeriod = 3600;
      failureActions.cActions = 1;
      failureActions.lpsaActions = &restartServiceAction;
      if (!ChangeServiceConfig2W(hService, SERVICE_CONFIG_FAILURE_ACTIONS, &failureActions))
      {
         WCHAR errorText[1024];
         wprintf(L"WARNING: cannot set service failure actions (%s)\n", GetSystemErrorText(GetLastError(), errorText, 1024));
         warnings = true;
      }

      wprintf(L"NetXMS Core service %s %s\n", update ? L"updated" : L"created", warnings ? L"with warnings" : L"successfully");
      CloseServiceHandle(hService);
   }

   CloseServiceHandle(hMgr);

   InstallEventSource(dllName);
}

/**
 * Remove service
 */
void RemoveService()
{
	WCHAR errorText[1024];

   SC_HANDLE mgr = OpenSCManagerW(nullptr, nullptr, GENERIC_WRITE);
   if (mgr == nullptr)
   {
      wprintf(L"ERROR: Cannot connect to Service Manager (%s)\n", GetSystemErrorText(GetLastError(), errorText, 1024));
      return;
   }

   SC_HANDLE service = OpenService(mgr, CORE_SERVICE_NAME, DELETE);
   if (service != nullptr)
   {
      if (DeleteService(service))
         wprintf(L"NetXMS Core service deleted successfully\n");
      else
         wprintf(L"ERROR: Cannot remove service named '" CORE_SERVICE_NAME L"' (%s)\n", GetSystemErrorText(GetLastError(), errorText, 1024));
      CloseServiceHandle(service);
   }
   else
   {
      wprintf(L"ERROR: Cannot open service named '" CORE_SERVICE_NAME L"' (%s)\n", GetSystemErrorText(GetLastError(), errorText, 1024));
   }

   CloseServiceHandle(mgr);

   RemoveEventSource();
}

/**
 * Check if -d switch is given on command line
 */
static bool IsDSwitchPresent(const WCHAR *cmdLine)
{
	int state;
	const WCHAR *ptr;
	for(ptr = cmdLine, state = 0; *ptr != 0; ptr++)
	{
		switch(state)
		{
			case 0:
			case 4:
				if (*ptr == ' ')
					state = 2;
				else if (*ptr == '"')
					state++;
				break;
			case 2:
				switch(*ptr)
				{
					case ' ':
						break;
					case '"':
						state++;
						break;
					case '-':
						if (*(ptr + 1) == 'd')
							return true;
					default:
						state = 4;
						break;
				}
				break;
			case 1:
			case 3:
			case 5:
				if (*ptr == '"')
					state--;
				break;
		}
	}
	return false;
}

/**
 * Check service configuration
 * Things to check:
 *   1. Versions 0.2.20 and above requires -d switch
 */
void CheckServiceConfig()
{
	WCHAR errorText[1024];

   SC_HANDLE mgr = OpenSCManagerW(nullptr, nullptr, GENERIC_WRITE);
   if (mgr == nullptr)
   {
      wprintf(L"ERROR: Cannot connect to Service Manager (%s)\n", GetSystemErrorText(GetLastError(), errorText, 1024));
      return;
   }

   SC_HANDLE service = OpenServiceW(mgr, CORE_SERVICE_NAME, SERVICE_QUERY_CONFIG | SERVICE_CHANGE_CONFIG);
   if (service != nullptr)
	{
      DWORD bytes;
      QueryServiceConfigW(service, nullptr, 0, &bytes);
		DWORD error = GetLastError();
		if (error == ERROR_INSUFFICIENT_BUFFER)
		{
         QUERY_SERVICE_CONFIG *cfg = (QUERY_SERVICE_CONFIG *)MemAlloc(bytes);
			if (QueryServiceConfigW(service, cfg, bytes, &bytes))
			{
				if (!IsDSwitchPresent(cfg->lpBinaryPathName))
				{
					StringBuffer cmdLine(cfg->lpBinaryPathName);
					cmdLine.append(L" -d");
					if (ChangeServiceConfigW(service, cfg->dwServiceType, cfg->dwStartType, cfg->dwErrorControl, cmdLine, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr))
					{
						wprintf(L"INFO: Service configuration successfully updated\n");
					}
					else
					{
						wprintf(L"ERROR: Cannot update configuration for service '" CORE_SERVICE_NAME L"' (%s)\n", GetSystemErrorText(GetLastError(), errorText, 1024));
					}
				}
				else
				{
					wprintf(L"INFO: Service configuration is valid\n");
				}
			}
			else
			{
				wprintf(L"ERROR: Cannot query configuration for service '" CORE_SERVICE_NAME L"' (%s)\n", GetSystemErrorText(GetLastError(), errorText, 1024));
			}
			MemFree(cfg);
		}
		else
		{
			wprintf(L"ERROR: Cannot query configuration for service '" CORE_SERVICE_NAME L"' (%s)\n", GetSystemErrorText(error, errorText, 1024));
		}
      CloseServiceHandle(service);
	}
	else
   {
      wprintf(L"ERROR: Cannot open service named '" CORE_SERVICE_NAME L"' (%s)\n", GetSystemErrorText(GetLastError(), errorText, 1024));
   }

   CloseServiceHandle(mgr);
}

/**
 * Check if service configured for manual startup 
 */
static bool IsManualStartup(SC_HANDLE service)
{
   bool result = false;

   DWORD bytes;
   QueryServiceConfigW(service, nullptr, 0, &bytes);
   DWORD error = GetLastError();
   if (error == ERROR_INSUFFICIENT_BUFFER)
   {
      QUERY_SERVICE_CONFIG *cfg = (QUERY_SERVICE_CONFIG *)MemAlloc(bytes);
      if (QueryServiceConfigW(service, cfg, bytes, &bytes))
      {
         result = (cfg->dwStartType == SERVICE_DEMAND_START);
      }
      else
      {
         TCHAR errorText[1024];
         wprintf(L"ERROR: Cannot query configuration for service '" CORE_SERVICE_NAME L"' (%s)\n", GetSystemErrorText(GetLastError(), errorText, 1024));
      }
      MemFree(cfg);
   }
   else
   {
      TCHAR errorText[1024];
      wprintf(L"ERROR: Cannot query configuration for service '" CORE_SERVICE_NAME L"' (%s)\n", GetSystemErrorText(error, errorText, 1024));
   }
   return result;
}

/**
 * Start service
 */
void StartCoreService(bool ignoreManualStartService)
{
   SC_HANDLE mgr = OpenSCManagerW(nullptr, nullptr, GENERIC_WRITE);
   if (mgr == nullptr)
   {
      TCHAR errorText[1024];
      _tprintf(_T("ERROR: Cannot connect to Service Manager (%s)\n"), GetSystemErrorText(GetLastError(), errorText, 1024));
      return;
   }

   SC_HANDLE service = OpenServiceW(mgr, CORE_SERVICE_NAME, SERVICE_START | SERVICE_QUERY_CONFIG);
   if (service != nullptr)
   {
      if (!ignoreManualStartService || !IsManualStartup(service))
      {
         if (StartService(service, 0, NULL))
         {
            _tprintf(_T("NetXMS Core service started successfully\n"));
         }
         else
         {
            TCHAR errorText[1024];
            _tprintf(_T("ERROR: Cannot start service named '") CORE_SERVICE_NAME _T("' (%s)\n"),
               GetSystemErrorText(GetLastError(), errorText, 1024));
         }
      }
      else
      {
         _tprintf(_T("NetXMS Core service configured for manual startup - not starting\n"));
      }
      CloseServiceHandle(service);
   }
   else
   {
      TCHAR errorText[1024];
      _tprintf(_T("ERROR: Cannot open service named '") CORE_SERVICE_NAME _T("' (%s)\n"),
         GetSystemErrorText(GetLastError(), errorText, 1024));
   }
   CloseServiceHandle(mgr);
}

/**
 * Stop service
 */
void StopCoreService()
{
   SC_HANDLE mgr,service;
	TCHAR errorText[1024];

   mgr = OpenSCManager(NULL, NULL, GENERIC_WRITE);
   if (mgr == NULL)
   {
      _tprintf(_T("ERROR: Cannot connect to Service Manager (%s)\n"), GetSystemErrorText(GetLastError(), errorText, 1024));
      return;
   }

   service = OpenService(mgr, CORE_SERVICE_NAME, SERVICE_STOP);
   if (service == NULL)
   {
      _tprintf(_T("ERROR: Cannot open service named '") CORE_SERVICE_NAME _T("' (%s)\n"),
             GetSystemErrorText(GetLastError(), errorText, 1024));
   }
   else
   {
      SERVICE_STATUS status;

      if (ControlService(service, SERVICE_CONTROL_STOP, &status))
         _tprintf(_T("NetXMS Core service stopped successfully\n"));
      else
         _tprintf(_T("ERROR: Cannot stop service named '") CORE_SERVICE_NAME _T("' (%s)\n"),
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
         _T("System\\CurrentControlSet\\Services\\EventLog\\System\\") CORE_EVENT_SOURCE,
         0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, NULL))
   {
      _tprintf(_T("Unable to create registry key: %s\n"), GetSystemErrorText(GetLastError(), errorText, 1024));
      return;
   }

   RegSetValueEx(hKey, _T("TypesSupported"), 0, REG_DWORD,(BYTE *)&dwTypes, sizeof(DWORD));
   RegSetValueEx(hKey, _T("EventMessageFile"), 0, REG_EXPAND_SZ,(BYTE *)path, (DWORD)((_tcslen(path) + 1) * sizeof(TCHAR)));

   RegCloseKey(hKey);
   _tprintf(_T("Event source \"") CORE_EVENT_SOURCE _T("\" installed successfully\n"));
}

/**
 * Remove event source
 */
void RemoveEventSource()
{
	TCHAR errorText[1024];

   if (ERROR_SUCCESS == RegDeleteKey(HKEY_LOCAL_MACHINE,
         _T("System\\CurrentControlSet\\Services\\EventLog\\System\\") CORE_EVENT_SOURCE))
   {
      _tprintf(_T("Event source \"") CORE_EVENT_SOURCE _T("\" uninstalled successfully\n"));
   }
   else
   {
      _tprintf(_T("Unable to uninstall event source \"") CORE_EVENT_SOURCE _T("\": %s\n"),
             GetSystemErrorText(GetLastError(), errorText, 1024));
   }
}
