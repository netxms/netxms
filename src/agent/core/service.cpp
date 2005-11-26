/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: service.cpp
**
**/

#include "nxagentd.h"


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
         status.dwWaitHint = 4000;
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

static VOID WINAPI AgentServiceMain(DWORD argc, LPTSTR *argv)
{
   SERVICE_STATUS status;

   serviceHandle = RegisterServiceCtrlHandler(AGENT_SERVICE_NAME, ServiceCtrlHandler);

   // Now we start service initialization
   status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
   status.dwCurrentState = SERVICE_START_PENDING;
   status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
   status.dwWin32ExitCode = 0;
   status.dwServiceSpecificExitCode = 0;
   status.dwCheckPoint = 0;
   status.dwWaitHint = 10000 + g_dwStartupDelay * 1000;
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


//
// Initialize service
//

void InitService(void)
{
   static SERVICE_TABLE_ENTRY serviceTable[2]={ { AGENT_SERVICE_NAME, AgentServiceMain }, { NULL, NULL } };
   char szErrorText[256];

   if (!StartServiceCtrlDispatcher(serviceTable))
      printf("StartServiceCtrlDispatcher() failed: %s\n",
             GetSystemErrorText(GetLastError(), szErrorText, 256));
}


//
// Create service
//

void InstallService(char *execName, char *confFile)
{
   SC_HANDLE mgr, service;
   char cmdLine[MAX_PATH*2], szErrorText[256];

   mgr = OpenSCManager(NULL, NULL, GENERIC_WRITE);
   if (mgr == NULL)
   {
      printf("ERROR: Cannot connect to Service Manager (%s)\n",
             GetSystemErrorText(GetLastError(), szErrorText, 256));
      return;
   }

   sprintf(cmdLine,"\"%s\" -d -c \"%s\"", execName, confFile);
   service=CreateService(mgr, AGENT_SERVICE_NAME, "NetXMS Agent", GENERIC_READ,SERVICE_WIN32_OWN_PROCESS,
                         SERVICE_AUTO_START,SERVICE_ERROR_NORMAL,cmdLine,NULL,NULL,NULL,NULL,NULL);
   if (service == NULL)
   {
      DWORD code = GetLastError();

      if (code == ERROR_SERVICE_EXISTS)
         printf("ERROR: Service named '" AGENT_SERVICE_NAME "' already exist\n");
      else
         printf("ERROR: Cannot create service (%s)\n", GetSystemErrorText(code, szErrorText, 256));
   }
   else
   {
      printf("Win32 Agent service created successfully\n");
      CloseServiceHandle(service);
   }

   CloseServiceHandle(mgr);

   InstallEventSource(execName);
}


//
// Remove service
//

void RemoveService(void)
{
   SC_HANDLE mgr, service;
   char szErrorText[256];

   mgr = OpenSCManager(NULL,NULL,GENERIC_WRITE);
   if (mgr==NULL)
   {
      printf("ERROR: Cannot connect to Service Manager (%s)\n",
             GetSystemErrorText(GetLastError(), szErrorText, 256));
      return;
   }

   service=OpenService(mgr, AGENT_SERVICE_NAME, DELETE);
   if (service==NULL)
   {
      printf("ERROR: Cannot open service named '" AGENT_SERVICE_NAME "' (%s)\n",
             GetSystemErrorText(GetLastError(), szErrorText, 256));
   }
   else
   {
      if (DeleteService(service))
         printf("Win32 Agent service deleted successfully\n");
      else
         printf("ERROR: Cannot remove service named '" AGENT_SERVICE_NAME "' (%s)\n",
                GetSystemErrorText(GetLastError(), szErrorText, 256));

      CloseServiceHandle(service);
   }

   CloseServiceHandle(mgr);

   RemoveEventSource();
}


//
// Start service
//

void StartAgentService(void)
{
   SC_HANDLE mgr, service;
   char szErrorText[256];

   mgr = OpenSCManager(NULL, NULL, GENERIC_WRITE);
   if (mgr == NULL)
   {
      printf("ERROR: Cannot connect to Service Manager (%s)\n",
             GetSystemErrorText(GetLastError(), szErrorText, 256));
      return;
   }

   service = OpenService(mgr, AGENT_SERVICE_NAME, SERVICE_START);
   if (service == NULL)
   {
      printf("ERROR: Cannot open service named '" AGENT_SERVICE_NAME "' (%s)\n",
             GetSystemErrorText(GetLastError(), szErrorText, 256));
   }
   else
   {
      if (StartService(service, 0, NULL))
         printf("Win32 Agent service started successfully\n");
      else
         printf("ERROR: Cannot start service named '" AGENT_SERVICE_NAME "' (%s)\n",
                GetSystemErrorText(GetLastError(), szErrorText, 256));

      CloseServiceHandle(service);
   }

   CloseServiceHandle(mgr);
}


//
// Stop service
//

void StopAgentService(void)
{
   SC_HANDLE mgr, service;
   char szErrorText[256];

   mgr = OpenSCManager(NULL, NULL, GENERIC_WRITE);
   if (mgr == NULL)
   {
      printf("ERROR: Cannot connect to Service Manager (%s)\n", 
             GetSystemErrorText(GetLastError(), szErrorText, 256));
      return;
   }

   service = OpenService(mgr, AGENT_SERVICE_NAME, SERVICE_STOP);
   if (service == NULL)
   {
      printf("ERROR: Cannot open service named '" AGENT_SERVICE_NAME "' (%s)\n",
             GetSystemErrorText(GetLastError(), szErrorText, 256));
   }
   else
   {
      SERVICE_STATUS status;

      if (ControlService(service, SERVICE_CONTROL_STOP, &status))
         printf("Win32 Agent service stopped successfully\n");
      else
         printf("ERROR: Cannot stop service named '" AGENT_SERVICE_NAME "' (%s)\n",
                GetSystemErrorText(GetLastError(), szErrorText, 256));

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
   char szErrorText[256];

   if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_LOCAL_MACHINE,
         "System\\CurrentControlSet\\Services\\EventLog\\System\\" AGENT_EVENT_SOURCE,
         0,NULL,REG_OPTION_NON_VOLATILE,KEY_SET_VALUE,NULL,&hKey,NULL))
   {
      printf("Unable to create registry key: %s\n",
             GetSystemErrorText(GetLastError(), szErrorText, 256));
      return;
   }

   RegSetValueEx(hKey, "TypesSupported", 0, REG_DWORD, (BYTE *)&dwTypes, sizeof(DWORD));
   RegSetValueEx(hKey, "EventMessageFile", 0, REG_EXPAND_SZ, (BYTE *)path, strlen(path) + 1);

   RegCloseKey(hKey);
   printf("Event source \"" AGENT_EVENT_SOURCE "\" installed successfully\n");
}


//
// Remove event source
//

void RemoveEventSource(void)
{
   char szErrorText[256];

   if (ERROR_SUCCESS == RegDeleteKey(HKEY_LOCAL_MACHINE,
         "System\\CurrentControlSet\\Services\\EventLog\\System\\" AGENT_EVENT_SOURCE))
   {
      printf("Event source \"" AGENT_EVENT_SOURCE "\" uninstalled successfully\n");
   }
   else
   {
      printf("Unable to uninstall event source \"" AGENT_EVENT_SOURCE "\": %s\n",
             GetSystemErrorText(GetLastError(), szErrorText, 256));
   }
}


//
// Wait for service
//

BOOL WaitForService(DWORD dwDesiredState)
{
   SC_HANDLE mgr, service;
   char szErrorText[256];
   BOOL bResult = FALSE;

   mgr = OpenSCManager(NULL, NULL, GENERIC_WRITE);
   if (mgr == NULL)
   {
      printf("ERROR: Cannot connect to Service Manager (%s)\n", 
             GetSystemErrorText(GetLastError(), szErrorText, 256));
      return FALSE;
   }

   service = OpenService(mgr, AGENT_SERVICE_NAME, SERVICE_QUERY_STATUS);
   if (service == NULL)
   {
      printf("ERROR: Cannot open service named '" AGENT_SERVICE_NAME "' (%s)\n",
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
               bResult = TRUE;
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
