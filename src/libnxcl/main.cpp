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
DWORD g_dwState = STATE_IDLE;


//
// Initialization function
//

BOOL EXPORTABLE NXCInitialize(void)
{
   ThreadCreate(RequestProcessor, 0, NULL);
   return TRUE;
}


//
// Get library version
//

DWORD EXPORTABLE NXCGetVersion(void)
{
   return (NETXMS_VERSION_MAJOR << 24) | (NETXMS_VERSION_MINOR << 16) | NETXMS_VERSION_RELEASE;
}


//
// Set event handler
//

void EXPORTABLE NXCSetEventHandler(NXC_EVENT_HANDLER pHandler)
{
   g_pEventHandler = pHandler;
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
