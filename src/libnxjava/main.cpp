/*
** NetXMS - Network Management System
** Copyright (C) 2003-2017 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: main.cpp
**/

#include "libnxjava.h"

/**
 * Error messages
 */
static const TCHAR *s_errorMessages[] =
{
   _T("Success"),
   _T("JVM load failed"),
   _T("JVM entry point not found"),
   _T("Cannot create JVM"),
   _T("Java application class not found"),
   _T("Java application entry point not found")
};

/**
 * Get error message from error code
 */
const TCHAR LIBNXJAVA_EXPORTABLE *GetJavaBridgeErrorMessage(JavaBridgeError error)
{
   if ((int)error < 0 || (int)error > NXJAVA_APP_ENTRY_POINT_NOT_FOUND)
      return _T("Unknown Java bridge error");
   return s_errorMessages[(int)error];
}

#ifdef _WIN32

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif   /* _WIN32 */
