/*
** nxflowd - NetXMS Flow Collector Daemon
** Copyright (c) 2009-2012 Raden Solutions
*/

#include "nxflowd.h"


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
         status.dwWaitHint = 3000;
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

static VOID WINAPI FlowCollectorServiceMain(DWORD argc, LPTSTR *argv)
{
   SERVICE_STATUS status;

   serviceHandle = RegisterServiceCtrlHandler(NXFLOWD_SERVICE_NAME, ServiceCtrlHandler);

   // Now we start service initialization
   status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
   status.dwCurrentState = SERVICE_START_PENDING;
   status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
   status.dwWin32ExitCode = 0;
   status.dwServiceSpecificExitCode = 0;
   status.dwCheckPoint = 0;
   status.dwWaitHint = 3000;
   SetServiceStatus(serviceHandle, &status);

   // Actual initialization
   if (!Initialize())
   {
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


//
// Initialize service
//

void InitService()
{
   static SERVICE_TABLE_ENTRY serviceTable[2] = { { NXFLOWD_SERVICE_NAME, FlowCollectorServiceMain }, { NULL, NULL } };
   TCHAR szErrorText[256];

   if (!StartServiceCtrlDispatcher(serviceTable))
      _tprintf(_T("StartServiceCtrlDispatcher() failed: %s\n"),
             GetSystemErrorText(GetLastError(), szErrorText, 256));
}


//
// Create service
//

void InstallFlowCollectorService(const TCHAR *pszExecName)
{
   SC_HANDLE mgr, service;
   TCHAR szCmdLine[MAX_PATH * 2], szErrorText[256];

   mgr = OpenSCManager(NULL, NULL, GENERIC_WRITE);
   if (mgr == NULL)
   {
      _tprintf(_T("ERROR: Cannot connect to Service Manager (%s)\n"),
             GetSystemErrorText(GetLastError(), szErrorText, 256));
      return;
   }

   _sntprintf(szCmdLine, MAX_PATH * 2, _T("\"%s\" -d -c \"%s\""), pszExecName, g_configFile);
   service = CreateService(mgr, NXFLOWD_SERVICE_NAME, _T("NetXMS NetFlow/IPFIX Collector"),
                           GENERIC_READ, SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START,
                           SERVICE_ERROR_NORMAL, szCmdLine, NULL, NULL,
                           NULL, NULL, NULL);
   if (service == NULL)
   {
      DWORD code = GetLastError();

      if (code == ERROR_SERVICE_EXISTS)
         _tprintf("ERROR: Service named '" NXFLOWD_SERVICE_NAME "' already exist\n");
      else
         _tprintf("ERROR: Cannot create service (%s)\n", GetSystemErrorText(code, szErrorText, 256));
   }
   else
   {
      _tprintf("NetXMS NetFlow/IPFIX Collector service created successfully\n");
      CloseServiceHandle(service);
   }

   CloseServiceHandle(mgr);

   InstallEventSource(pszExecName);
}


//
// Remove service
//

void RemoveFlowCollectorService()
{
   SC_HANDLE mgr, service;
   TCHAR szErrorText[256];

   mgr = OpenSCManager(NULL, NULL, GENERIC_WRITE);
   if (mgr == NULL)
   {
      _tprintf("ERROR: Cannot connect to Service Manager (%s)\n",
             GetSystemErrorText(GetLastError(), szErrorText, 256));
      return;
   }

   service = OpenService(mgr, NXFLOWD_SERVICE_NAME, DELETE);
   if (service == NULL)
   {
      _tprintf("ERROR: Cannot open service named '" NXFLOWD_SERVICE_NAME "' (%s)\n",
             GetSystemErrorText(GetLastError(), szErrorText, 256));
   }
   else
   {
      if (DeleteService(service))
         _tprintf("NetXMS NetFlow/IPFIX Collector service deleted successfully\n");
      else
         _tprintf("ERROR: Cannot remove service named '" NXFLOWD_SERVICE_NAME "' (%s)\n",
                GetSystemErrorText(GetLastError(), szErrorText, 256));

      CloseServiceHandle(service);
   }

   CloseServiceHandle(mgr);

   RemoveEventSource();
}


//
// Start service
//

void StartFlowCollectorService()
{
   SC_HANDLE mgr, service;
   TCHAR szErrorText[256];

   mgr = OpenSCManager(NULL, NULL, GENERIC_WRITE);
   if (mgr == NULL)
   {
      _tprintf("ERROR: Cannot connect to Service Manager (%s)\n",
             GetSystemErrorText(GetLastError(), szErrorText, 256));
      return;
   }

   service = OpenService(mgr, NXFLOWD_SERVICE_NAME, SERVICE_START);
   if (service == NULL)
   {
      _tprintf("ERROR: Cannot open service named '" NXFLOWD_SERVICE_NAME "' (%s)\n",
             GetSystemErrorText(GetLastError(), szErrorText, 256));
   }
   else
   {
      if (StartService(service, 0, NULL))
         _tprintf("NetXMS NetFlow/IPFIX Collector service started successfully\n");
      else
         _tprintf("ERROR: Cannot start service named '" NXFLOWD_SERVICE_NAME "' (%s)\n",
                GetSystemErrorText(GetLastError(), szErrorText, 256));

      CloseServiceHandle(service);
   }

   CloseServiceHandle(mgr);
}


//
// Stop service
//

void StopFlowCollectorService()
{
   SC_HANDLE mgr, service;
   TCHAR szErrorText[256];

   mgr = OpenSCManager(NULL, NULL, GENERIC_WRITE);
   if (mgr == NULL)
   {
      _tprintf("ERROR: Cannot connect to Service Manager (%s)\n", 
             GetSystemErrorText(GetLastError(), szErrorText, 256));
      return;
   }

   service = OpenService(mgr, NXFLOWD_SERVICE_NAME, SERVICE_STOP);
   if (service == NULL)
   {
      _tprintf("ERROR: Cannot open service named '" NXFLOWD_SERVICE_NAME "' (%s)\n",
             GetSystemErrorText(GetLastError(), szErrorText, 256));
   }
   else
   {
      SERVICE_STATUS status;

      if (ControlService(service, SERVICE_CONTROL_STOP, &status))
         _tprintf(_T("NetXMS NetFlow/IPFIX Collector service stopped successfully\n"));
      else
         _tprintf(_T("ERROR: Cannot stop service named '"_ NXFLOWD_SERVICE_NAME _T("' (%s)\n"),
                GetSystemErrorText(GetLastError(), szErrorText, 256));

      CloseServiceHandle(service);
   }

   CloseServiceHandle(mgr);
}


//
// Install event source
//

void InstallEventSource(const TCHAR *pszPath)
{
   HKEY hKey;
   DWORD dwTypes = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE;
   TCHAR szErrorText[256];

   if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_LOCAL_MACHINE,
         _T("System\\CurrentControlSet\\Services\\EventLog\\System\\") NXFLOWD_EVENT_SOURCE,
         0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, NULL))
   {
      _tprintf(_T("Unable to create registry key: %s\n"),
               GetSystemErrorText(GetLastError(), szErrorText, 256));
      return;
   }

   RegSetValueEx(hKey, _T("TypesSupported"), 0, REG_DWORD, (BYTE *)&dwTypes, sizeof(DWORD));
   RegSetValueEx(hKey, _T("EventMessageFile"), 0, REG_EXPAND_SZ, (BYTE *)pszPath, (DWORD)(_tcslen(pszPath) + 1) * sizeof(TCHAR));

   RegCloseKey(hKey);
   _tprintf(_T("Event source \"") NXFLOWD_EVENT_SOURCE _T("\" installed successfully\n"));
}


//
// Remove event source
//

void RemoveEventSource()
{
   TCHAR szErrorText[256];

   if (ERROR_SUCCESS == RegDeleteKey(HKEY_LOCAL_MACHINE,
         _T("System\\CurrentControlSet\\Services\\EventLog\\System\\") NXFLOWD_EVENT_SOURCE))
   {
      _tprintf(_T("Event source \"") NXFLOWD_EVENT_SOURCE _T("\" uninstalled successfully\n"));
   }
   else
   {
      _tprintf(_T("Unable to uninstall event source \"") NXFLOWD_EVENT_SOURCE _T("\": %s\n"),
               GetSystemErrorText(GetLastError(), szErrorText, 256));
   }
}
