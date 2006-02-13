/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005 Victor Kirhenshtein
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
** $module: winsrv.cpp
**
**/

#include "netxmsd.h"

#if defined(_WIN32)

//
// Static data
//

static SERVICE_STATUS_HANDLE serviceHandle;


//
// Service control handler
//

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
         SetServiceStatus(serviceHandle, &status);

         Shutdown();

         status.dwCurrentState = SERVICE_STOPPED;
         status.dwWaitHint = 0;
         break;
      default:
         break;
   }

   SetServiceStatus(serviceHandle, &status);
}


//
// Service main
//

static VOID WINAPI CoreServiceMain(DWORD argc, LPTSTR *argv)
{
   SERVICE_STATUS status;

   serviceHandle = RegisterServiceCtrlHandler(CORE_SERVICE_NAME, ServiceCtrlHandler);

   // Now we start service initialization
   status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
   status.dwCurrentState = SERVICE_START_PENDING;
   status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
   status.dwWin32ExitCode = 0;
   status.dwServiceSpecificExitCode = 0;
   status.dwCheckPoint = 0;
   status.dwWaitHint = 300000;   // Assume that startup can take up to 5 minutes
   SetServiceStatus(serviceHandle, &status);

   // Actual initialization
   if (!Initialize())
   {
      // Remove database lock
      if (g_dwFlags & AF_DB_LOCKED)
      {
         UnlockDB();
         ShutdownDB();
      }

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

   Main(NULL);
}


//
// Initialize service
//

void InitService(void)
{
   static SERVICE_TABLE_ENTRY serviceTable[2] = { { CORE_SERVICE_NAME, CoreServiceMain }, { NULL, NULL } };

   if (!StartServiceCtrlDispatcher(serviceTable))
      printf("StartServiceCtrlDispatcher() failed: %s\n", GetSystemErrorText(GetLastError()));
}


//
// Create service
//

void InstallService(TCHAR *pszExecName, TCHAR *pszDllName,
                    TCHAR *pszLogin, TCHAR *pszPassword)
{
   SC_HANDLE hMgr, hService;
   TCHAR szCmdLine[MAX_PATH * 2];

   hMgr = OpenSCManager(NULL, NULL, GENERIC_WRITE);
   if (hMgr == NULL)
   {
      printf("ERROR: Cannot connect to Service Manager (%s)\n",
             GetSystemErrorText(GetLastError()));
      return;
   }

   sprintf(szCmdLine, "\"%s\" --config \"%s\"%s", pszExecName, g_szConfigFile,
           g_bCheckDB ? " --check-db" : "");
   hService = CreateService(hMgr, CORE_SERVICE_NAME, "NetXMS Core",
                            GENERIC_READ, SERVICE_WIN32_OWN_PROCESS,
                            SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
                            szCmdLine, NULL, NULL, NULL, pszLogin, pszPassword);
   if (hService == NULL)
   {
      DWORD code = GetLastError();

      if (code == ERROR_SERVICE_EXISTS)
         printf("ERROR: Service named '" CORE_SERVICE_NAME "' already exist\n");
      else
         printf("ERROR: Cannot create service (%s)\n", GetSystemErrorText(code));
   }
   else
   {
      printf("NetXMS Core service created successfully\n");
      CloseServiceHandle(hService);
   }

   CloseServiceHandle(hMgr);

   InstallEventSource(pszDllName);
}


//
// Remove service
//

void RemoveService(void)
{
   SC_HANDLE mgr, service;

   mgr=OpenSCManager(NULL, NULL, GENERIC_WRITE);
   if (mgr == NULL)
   {
      printf("ERROR: Cannot connect to Service Manager (%s)\n", GetSystemErrorText(GetLastError()));
      return;
   }

   service = OpenService(mgr, CORE_SERVICE_NAME, DELETE);
   if (service == NULL)
   {
      printf("ERROR: Cannot open service named '" CORE_SERVICE_NAME "' (%s)\n",
             GetSystemErrorText(GetLastError()));
   }
   else
   {
      if (DeleteService(service))
         printf("NetXMS Core service deleted successfully\n");
      else
         printf("ERROR: Cannot remove service named '" CORE_SERVICE_NAME "' (%s)\n",
                GetSystemErrorText(GetLastError()));

      CloseServiceHandle(service);
   }

   CloseServiceHandle(mgr);

   RemoveEventSource();
}


//
// Start service
//

void StartCoreService(void)
{
   SC_HANDLE mgr,service;

   mgr = OpenSCManager(NULL,NULL,GENERIC_WRITE);
   if (mgr == NULL)
   {
      printf("ERROR: Cannot connect to Service Manager (%s)\n", GetSystemErrorText(GetLastError()));
      return;
   }

   service = OpenService(mgr, CORE_SERVICE_NAME, SERVICE_START);
   if (service == NULL)
   {
      printf("ERROR: Cannot open service named '" CORE_SERVICE_NAME "' (%s)\n",
             GetSystemErrorText(GetLastError()));
   }
   else
   {
      if (StartService(service, 0, NULL))
         printf("NetXMS Core service started successfully\n");
      else
         printf("ERROR: Cannot start service named '" CORE_SERVICE_NAME "' (%s)\n",
                GetSystemErrorText(GetLastError()));

      CloseServiceHandle(service);
   }

   CloseServiceHandle(mgr);
}


//
// Stop service
//

void StopCoreService(void)
{
   SC_HANDLE mgr,service;

   mgr = OpenSCManager(NULL, NULL, GENERIC_WRITE);
   if (mgr == NULL)
   {
      printf("ERROR: Cannot connect to Service Manager (%s)\n", GetSystemErrorText(GetLastError()));
      return;
   }

   service = OpenService(mgr, CORE_SERVICE_NAME, SERVICE_STOP);
   if (service == NULL)
   {
      printf("ERROR: Cannot open service named '" CORE_SERVICE_NAME "' (%s)\n",
             GetSystemErrorText(GetLastError()));
   }
   else
   {
      SERVICE_STATUS status;

      if (ControlService(service, SERVICE_CONTROL_STOP, &status))
         printf("NetXMS Core service stopped successfully\n");
      else
         printf("ERROR: Cannot stop service named '" CORE_SERVICE_NAME "' (%s)\n",
                GetSystemErrorText(GetLastError()));

      CloseServiceHandle(service);
   }

   CloseServiceHandle(mgr);
}


//
// Install event source
//

void InstallEventSource(char *path)
{
   HKEY hKey;
   DWORD dwTypes = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE;

   if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_LOCAL_MACHINE,
         "System\\CurrentControlSet\\Services\\EventLog\\System\\" CORE_EVENT_SOURCE,
         0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, NULL))
   {
      printf("Unable to create registry key: %s\n", GetSystemErrorText(GetLastError()));
      return;
   }

   RegSetValueEx(hKey, "TypesSupported", 0, REG_DWORD,(BYTE *)&dwTypes, sizeof(DWORD));
   RegSetValueEx(hKey, "EventMessageFile", 0, REG_EXPAND_SZ,(BYTE *)path, strlen(path) + 1);

   RegCloseKey(hKey);
   printf("Event source \"" CORE_EVENT_SOURCE "\" installed successfully\n");
}


//
// Remove event source
//

void RemoveEventSource(void)
{
   if (ERROR_SUCCESS == RegDeleteKey(HKEY_LOCAL_MACHINE,
         "System\\CurrentControlSet\\Services\\EventLog\\System\\" CORE_EVENT_SOURCE))
   {
      printf("Event source \"" CORE_EVENT_SOURCE "\" uninstalled successfully\n");
   }
   else
   {
      printf("Unable to uninstall event source \"" CORE_EVENT_SOURCE "\": %s\n",
             GetSystemErrorText(GetLastError()));
   }
}

#endif   /* _WIN32 */
