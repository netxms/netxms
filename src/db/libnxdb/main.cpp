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
 * Write log
 */
void __DBWriteLog(WORD level, const TCHAR *format, ...)
{
	TCHAR buffer[4096];
	va_list args;
	
	va_start(args, format);
	_vsntprintf(buffer, 4096, format, args);
	va_end(args);
	nxlog_write(g_logMsgCode, level, "s", buffer);
}

/**
 * Set long running query threshold (milliseconds)
 */
void LIBNXDB_EXPORTABLE DBSetLongRunningThreshold(UINT32 threshold)
{
	g_sqlQueryExecTimeThreshold = threshold;
   nxlog_debug_tag(_T("db.query"), 3, _T("DB Library: long running query threshold set to %u"), threshold);
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
