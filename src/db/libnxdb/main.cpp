/* 
** NetXMS - Network Management System
** Database Abstraction Library
** Copyright (C) 2003-2016 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
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
**
**/

#include "libnxdb.h"

/**
 * Set global long running query threshold (milliseconds)
 */
void LIBNXDB_EXPORTABLE DBSetLongRunningThreshold(uint32_t threshold)
{
	g_sqlQueryExecTimeThreshold = threshold;
   nxlog_debug_tag(_T("db.query"), 3, _T("DB Library: global long running query threshold set to %u"), threshold);
}

/**
 * Set per-session long running query threshold (milliseconds)
 */
void LIBNXDB_EXPORTABLE DBSetLongRunningThreshold(DB_HANDLE conn, uint32_t threshold)
{
   conn->m_sqlQueryExecTimeThreshold = threshold;
   nxlog_debug_tag(_T("db.query"), 3, _T("DB Library: long running query threshold for session %p set to %u"), conn, threshold);
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
