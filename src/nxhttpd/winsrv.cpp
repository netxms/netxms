/* 
** NetXMS - Network Management System
** HTTP Server
** Copyright (C) 2005, 2006 Victor Kirhenshtein
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

#include "NxWeb.h"


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

static VOID WINAPI NXHTTPDServiceMain(DWORD argc, LPTSTR *argv)
{
   SERVICE_STATUS status;

   serviceHandle = RegisterServiceCtrlHandler(NXHTTPD_SERVICE_NAME, ServiceCtrlHandler);

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

   Main(NULL);
}


//
// Initialize service
//

void InitService(void)
{
   static SERVICE_TABLE_ENTRY serviceTable[2] = { { NXHTTPD_SERVICE_NAME, NXHTTPDServiceMain }, { NULL, NULL } };
   char szErrorText[256];

   if (!StartServiceCtrlDispatcher(serviceTable))
      printf("StartServiceCtrlDispatcher() failed: %s\n",
             GetSystemErrorText(GetLastError(), szErrorText, 256));
}


//
// Create service
//

void NXHTTPDInstallService(char *pszExecName)
{
   SC_HANDLE mgr, service;
   char szCmdLine[MAX_PATH*2], szErrorText[256];

   mgr = OpenSCManager(NULL, NULL, GENERIC_WRITE);
   if (mgr == NULL)
   {
      printf("ERROR: Cannot connect to Service Manager (%s)\n",
             GetSystemErrorText(GetLastError(), szErrorText, 256));
      return;
   }

   sprintf(szCmdLine, "\"%s\" -d -c \"%s\"", pszExecName, g_szConfigFile);
   service = CreateService(mgr, NXHTTPD_SERVICE_NAME, "NetXMS Web Server",
                           GENERIC_READ, SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START,
                           SERVICE_ERROR_NORMAL, szCmdLine, NULL, NULL,
                           NULL, NULL, NULL);
   if (service == NULL)
   {
      DWORD code = GetLastError();

      if (code == ERROR_SERVICE_EXISTS)
         printf("ERROR: Service named '" NXHTTPD_SERVICE_NAME "' already exist\n");
      else
         printf("ERROR: Cannot create service (%s)\n", GetSystemErrorText(code, szErrorText, 256));
   }
   else
   {
      printf("NetXMS Web Server service created successfully\n");
      CloseServiceHandle(service);
   }

   CloseServiceHandle(mgr);
}


//
// Remove service
//

void NXHTTPDRemoveService(void)
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

   service=OpenService(mgr, NXHTTPD_SERVICE_NAME, DELETE);
   if (service==NULL)
   {
      printf("ERROR: Cannot open service named '" NXHTTPD_SERVICE_NAME "' (%s)\n",
             GetSystemErrorText(GetLastError(), szErrorText, 256));
   }
   else
   {
      if (DeleteService(service))
         printf("NetXMS Web Server service deleted successfully\n");
      else
         printf("ERROR: Cannot remove service named '" NXHTTPD_SERVICE_NAME "' (%s)\n",
                GetSystemErrorText(GetLastError(), szErrorText, 256));

      CloseServiceHandle(service);
   }

   CloseServiceHandle(mgr);
}


//
// Start service
//

void NXHTTPDStartService(void)
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

   service = OpenService(mgr, NXHTTPD_SERVICE_NAME, SERVICE_START);
   if (service == NULL)
   {
      printf("ERROR: Cannot open service named '" NXHTTPD_SERVICE_NAME "' (%s)\n",
             GetSystemErrorText(GetLastError(), szErrorText, 256));
   }
   else
   {
      if (StartService(service, 0, NULL))
         printf("NetXMS Web Server service started successfully\n");
      else
         printf("ERROR: Cannot start service named '" NXHTTPD_SERVICE_NAME "' (%s)\n",
                GetSystemErrorText(GetLastError(), szErrorText, 256));

      CloseServiceHandle(service);
   }

   CloseServiceHandle(mgr);
}


//
// Stop service
//

void NXHTTPDStopService(void)
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

   service = OpenService(mgr, NXHTTPD_SERVICE_NAME, SERVICE_STOP);
   if (service == NULL)
   {
      printf("ERROR: Cannot open service named '" NXHTTPD_SERVICE_NAME "' (%s)\n",
             GetSystemErrorText(GetLastError(), szErrorText, 256));
   }
   else
   {
      SERVICE_STATUS status;

      if (ControlService(service, SERVICE_CONTROL_STOP, &status))
         printf("NetXMS Web Server service stopped successfully\n");
      else
         printf("ERROR: Cannot stop service named '" NXHTTPD_SERVICE_NAME "' (%s)\n",
                GetSystemErrorText(GetLastError(), szErrorText, 256));

      CloseServiceHandle(service);
   }

   CloseServiceHandle(mgr);
}
