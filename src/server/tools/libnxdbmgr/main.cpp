/*
** NetXMS database manager library
** Copyright (C) 2004-2018 Victor Kirhenshtein
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
** File: main.cpp
**
**/

#include "libnxdbmgr.h"

/**
 * Database syntax
 */
int LIBNXDBMGR_EXPORTABLE g_dbSyntax = -1;

/**
 * Global DB handle
 */
DB_HANDLE LIBNXDBMGR_EXPORTABLE g_dbHandle = NULL;

/**
 * "Ignore errors" flag
 */
bool LIBNXDBMGR_EXPORTABLE g_ignoreErrors = false;

/**
 * Query trace flag
 */
bool g_queryTrace = false;

/**
 * Table suffix
 */
const TCHAR *g_tableSuffix = _T("");

/**
 * Set query trace mode
 */
void LIBNXDBMGR_EXPORTABLE SetQueryTraceMode(bool enabled)
{
   g_queryTrace = enabled;
}

/**
 * Check if query trace is enabled
 */
bool LIBNXDBMGR_EXPORTABLE IsQueryTraceEnabled()
{
   return g_queryTrace;
}

/**
 * Set table suffix
 */
void LIBNXDBMGR_EXPORTABLE SetTableSuffix(const TCHAR *s)
{
   g_tableSuffix = s;
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
