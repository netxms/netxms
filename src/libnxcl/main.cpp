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
// Externals
//

void RequestProcessor(void *pArg);


//
// Global variables
//

NXC_EVENT_HANDLER g_pEventHandler = NULL;
NXC_DEBUG_CALLBACK g_pDebugCallBack = NULL;
DWORD g_dwState = STATE_DISCONNECTED;
Queue *g_pRequestQueue = NULL;
DWORD g_dwRequestId;


//
// Initialization function
//

BOOL LIBNXCL_EXPORTABLE NXCInitialize(void)
{
   ObjectsInit();
   g_dwRequestId = 1;
   g_pRequestQueue = new Queue;
   if (g_pRequestQueue == NULL)
      return FALSE;
   ThreadCreate(RequestProcessor, 0, NULL);
   return TRUE;
}


//
// Get library version
//

DWORD LIBNXCL_EXPORTABLE NXCGetVersion(void)
{
   return (NETXMS_VERSION_MAJOR << 24) | (NETXMS_VERSION_MINOR << 16) | LIBNXCL_VERSION;
}


//
// Set event handler
//

void LIBNXCL_EXPORTABLE NXCSetEventHandler(NXC_EVENT_HANDLER pHandler)
{
   g_pEventHandler = pHandler;
}


//
// Set callback for debug messages
//

void LIBNXCL_EXPORTABLE NXCSetDebugCallback(NXC_DEBUG_CALLBACK pFunc)
{
   g_pDebugCallBack = pFunc;
}


//
// Ge text for error
//

const char LIBNXCL_EXPORTABLE *NXCGetErrorText(DWORD dwError)
{
   static char *pszErrorText[] =
   {
      "Request completed successfully",
      "Component locked",
      "Access denied",
      "Invalid request",
      "Request timed out",
      "Request is out of state",
      "Database failure",
      "Invalid object ID"
   };
   return ((dwError >= 0) && (dwError <= 7)) ? pszErrorText[dwError] : "Unknown error code";
}


//
// DLL entry point
//

#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   return TRUE;
}

#endif   /* _WIN32 */
