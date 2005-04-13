/* 
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2004 Victor Kirhenshtein
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
** $module: main.cpp
**
**/

#include "libnxcl.h"


//
// Global variables
//

NXC_DEBUG_CALLBACK g_pDebugCallBack = NULL;


//
// Print debug messages
//

void DebugPrintf(TCHAR *szFormat, ...)
{
   va_list args;
   TCHAR szBuffer[4096];

   if (g_pDebugCallBack == NULL)
      return;

   va_start(args, szFormat);
   _vstprintf(szBuffer, szFormat, args);
   va_end(args);
   g_pDebugCallBack(szBuffer);
}


//
// Initialization function
//

BOOL LIBNXCL_EXPORTABLE NXCInitialize(void)
{
   return TRUE;
}


//
// Shutdown function
//

void LIBNXCL_EXPORTABLE NXCShutdown(void)
{
}


//
// Get library version
//

DWORD LIBNXCL_EXPORTABLE NXCGetVersion(void)
{
   return (NETXMS_VERSION_MAJOR << 24) | (NETXMS_VERSION_MINOR << 16) | NETXMS_VERSION_BUILD;
}


//
// Set event handler
//

void LIBNXCL_EXPORTABLE NXCSetEventHandler(NXC_SESSION hSession, NXC_EVENT_HANDLER pHandler)
{
   ((NXCL_Session *)hSession)->m_pEventHandler = pHandler;
}


//
// Set callback for debug messages
//

void LIBNXCL_EXPORTABLE NXCSetDebugCallback(NXC_DEBUG_CALLBACK pFunc)
{
   g_pDebugCallBack = pFunc;
}


//
// Set command timeout
//

void LIBNXCL_EXPORTABLE NXCSetCommandTimeout(NXC_SESSION hSession, DWORD dwTimeout)
{
   if ((dwTimeout >= 1000) && (dwTimeout <= 60000))
      ((NXCL_Session *)hSession)->m_dwCommandTimeout = dwTimeout;
}


//
// Get server ID
//

void LIBNXCL_EXPORTABLE NXCGetServerID(NXC_SESSION hSession, BYTE *pbsId)
{
   memcpy(pbsId, ((NXCL_Session *)hSession)->m_bsServerId, 8);
}


//
// Get text for error
//

const TCHAR LIBNXCL_EXPORTABLE *NXCGetErrorText(DWORD dwError)
{
   static TCHAR *pszErrorText[] =
   {
      _T("Request completed successfully"),
      _T("Component locked"),
      _T("Access denied"),
      _T("Invalid request"),
      _T("Request timed out"),
      _T("Request is out of state"),
      _T("Database failure"),
      _T("Invalid object ID"),
      _T("Object already exist"),
      _T("Communication failure"),
      _T("System failure"),
      _T("Invalid user ID"),
      _T("Invalid argument"),
      _T("Duplicate DCI"),
      _T("Invalid DCI ID"),
      _T("Out of memory"),
      _T("Input/Output error"),
      _T("Incompatible operation"),
      _T("Object creation failed"),
      _T("Loop in object relationship detected"),
      _T("Invalid object name"),
      _T("Invalid alarm ID"),
      _T("Invalid action ID"),
      _T("Operation in progress"),
      _T("Copy operation failed for one or more DCI(s)"),
      _T("Invalid or unknown event code"),
      _T("No interfaces suitable for sending magic packet"),
      _T("No MAC address on interface"),
      _T("Command not implemented"),
      _T("Invalid trap configuration record ID"),
      _T("Requested data collection item is not supported by agent"),
      _T("Client and server versions mismatch"),
      _T("Error parsing package information file"),
      _T("Package with specified properties already installed on server"),
      _T("Package file already exist on server"),
      _T("Server resource busy"),
      _T("Invalid package ID"),
      _T("Invalid IP address"),
      _T("Action is used in event processing policy"),
      _T("Variable not found"),
      _T("Server uses incompatible version of communication protocol")
   };
   return ((dwError >= 0) && (dwError <= RCC_BAD_PROTOCOL)) ? pszErrorText[dwError] : _T("Unknown error code");
}


//
// DLL entry point
//

#if defined(_WIN32) && !defined(UNDER_CE)

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   return TRUE;
}

#endif   /* _WIN32 */
