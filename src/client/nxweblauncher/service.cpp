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
** File: service.cpp
**
**/

#include "nxweblauncher.h"

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

   switch (ctrlCode)
   {
      case SERVICE_CONTROL_STOP:
      case SERVICE_CONTROL_SHUTDOWN:
         status.dwCurrentState = SERVICE_STOP_PENDING;
         status.dwWaitHint = 60000;
         SetServiceStatus(s_serviceHandle, &status);

         StopJetty();

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
static VOID WINAPI WebUIServiceMain(DWORD argc, LPWSTR *argv)
{
   SERVICE_STATUS status;

   s_serviceHandle = RegisterServiceCtrlHandler(WEBUI_SERVICE_NAME, ServiceCtrlHandler);

   // Now we start service initialization
   status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
   status.dwCurrentState = SERVICE_RUNNING;
   status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
   status.dwWin32ExitCode = 0;
   status.dwServiceSpecificExitCode = 0;
   status.dwCheckPoint = 0;
   status.dwWaitHint = 0;
   SetServiceStatus(s_serviceHandle, &status);

   SetProcessShutdownParameters(0x3FF, 0);

   RunServer();

   // Now service is stopped
   status.dwCurrentState = SERVICE_STOPPED;
   status.dwWaitHint = 0;
   SetServiceStatus(s_serviceHandle, &status);
}

/**
 * Initialize service
 */
void InitService()
{
   static SERVICE_TABLE_ENTRYW serviceTable[2] = { { (WCHAR*)WEBUI_SERVICE_NAME, WebUIServiceMain }, { nullptr, nullptr } };
   if (!StartServiceCtrlDispatcherW(serviceTable))
   {
      TCHAR errorText[1024];
      wprintf(L"StartServiceCtrlDispatcher() failed: %s\n", GetSystemErrorText(GetLastError(), errorText, 1024));
   }
}

/**
 * Create service
 */
void InstallService()
{
   SC_HANDLE hMgr = OpenSCManagerW(nullptr, nullptr, GENERIC_WRITE);
   if (hMgr == nullptr)
   {
      TCHAR errorText[1024];
      wprintf(L"ERROR: Cannot connect to Service Manager (%s)\n", GetSystemErrorText(GetLastError(), errorText, 1024));
      return;
   }

   TCHAR execName[MAX_PATH];
   GetModuleFileNameW(nullptr, execName, MAX_PATH);

   bool update = false;
   TCHAR szCmdLine[MAX_PATH + 64];
   _sntprintf(szCmdLine, MAX_PATH + 64, _T("\"%s\" service"), execName);
   SC_HANDLE hService = CreateService(hMgr, WEBUI_SERVICE_NAME, _T("NetXMS Web UI (Jetty)"), GENERIC_READ | SERVICE_CHANGE_CONFIG | SERVICE_START,
      SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
      szCmdLine, nullptr, nullptr, nullptr, nullptr, nullptr);
   if (hService == nullptr)
   {
      DWORD code = GetLastError();
      if (code == ERROR_SERVICE_EXISTS)
      {
         _tprintf(_T("WARNING: Service named '") WEBUI_SERVICE_NAME _T("' already exist\n"));

         // Open service for update in case we are updating older installation and change startup mode
         hService = OpenService(hMgr, WEBUI_SERVICE_NAME, SERVICE_CHANGE_CONFIG | SERVICE_START);
         if (hService != nullptr)
         {
            if (!ChangeServiceConfig(hService, SERVICE_NO_CHANGE, SERVICE_AUTO_START, SERVICE_NO_CHANGE, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr))
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
      sd.lpDescription = (WCHAR*)L"This service provides web UI and API functionality for NetXMS management server";
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

      wprintf(L"NetXMS Web UI service %s %s\n", update ? L"updated" : L"created", warnings ? L"with warnings" : L"successfully");
      CloseServiceHandle(hService);
   }

   CloseServiceHandle(hMgr);
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

   SC_HANDLE service = OpenService(mgr, WEBUI_SERVICE_NAME, DELETE);
   if (service != nullptr)
   {
      if (DeleteService(service))
         wprintf(L"NetXMS Web UI service deleted successfully\n");
      else
         wprintf(L"ERROR: Cannot remove service named '" WEBUI_SERVICE_NAME L"' (%s)\n", GetSystemErrorText(GetLastError(), errorText, 1024));
      CloseServiceHandle(service);
   }
   else
   {
      wprintf(L"ERROR: Cannot open service named '" WEBUI_SERVICE_NAME L"' (%s)\n", GetSystemErrorText(GetLastError(), errorText, 1024));
   }

   CloseServiceHandle(mgr);
}

/**
 * Start service
 */
void StartWebUIService()
{
   SC_HANDLE mgr = OpenSCManagerW(nullptr, nullptr, GENERIC_WRITE);
   if (mgr == nullptr)
   {
      TCHAR errorText[1024];
      _tprintf(_T("ERROR: Cannot connect to Service Manager (%s)\n"), GetSystemErrorText(GetLastError(), errorText, 1024));
      return;
   }

   SC_HANDLE service = OpenServiceW(mgr, WEBUI_SERVICE_NAME, SERVICE_START);
   if (service != nullptr)
   {
      if (StartService(service, 0, nullptr))
      {
         _tprintf(_T("NetXMS Web UI service started successfully\n"));
      }
      else
      {
         TCHAR errorText[1024];
         _tprintf(_T("ERROR: Cannot start service named '") WEBUI_SERVICE_NAME _T("' (%s)\n"), GetSystemErrorText(GetLastError(), errorText, 1024));
      }
      CloseServiceHandle(service);
   }
   else
   {
      TCHAR errorText[1024];
      _tprintf(_T("ERROR: Cannot open service named '") WEBUI_SERVICE_NAME _T("' (%s)\n"), GetSystemErrorText(GetLastError(), errorText, 1024));
   }
   CloseServiceHandle(mgr);
}

/**
 * Stop service
 */
void StopWebUIService()
{
   TCHAR errorText[1024];

   SC_HANDLE mgr = OpenSCManager(NULL, NULL, GENERIC_WRITE);
   if (mgr == nullptr)
   {
      _tprintf(_T("ERROR: Cannot connect to Service Manager (%s)\n"), GetSystemErrorText(GetLastError(), errorText, 1024));
      return;
   }

   SC_HANDLE service = OpenService(mgr, WEBUI_SERVICE_NAME, SERVICE_STOP);
   if (service != nullptr)
   {
      SERVICE_STATUS status;

      if (ControlService(service, SERVICE_CONTROL_STOP, &status))
         _tprintf(_T("NetXMS Web UI service stopped successfully\n"));
      else
         _tprintf(_T("ERROR: Cannot stop service named '") WEBUI_SERVICE_NAME _T("' (%s)\n"), GetSystemErrorText(GetLastError(), errorText, 1024));

      CloseServiceHandle(service);
   }
   else
   {
      _tprintf(_T("ERROR: Cannot open service named '") WEBUI_SERVICE_NAME _T("' (%s)\n"), GetSystemErrorText(GetLastError(), errorText, 1024));
   }

   CloseServiceHandle(mgr);
}
