/* 
** NetXMS - Network Management System
** Alarm Viewer
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
** $module: globals.cpp
**
**/

#include "StdAfx.h"
#include "nxav.h"


//
// Session handle
//

NXC_SESSION g_hSession = NULL;


//
// Global settings
//

BOOL g_bRepeatSound = FALSE;


//
// Login parameters
//

TCHAR g_szServer[MAX_PATH] = _T("localhost");
TCHAR g_szLogin[MAX_USER_NAME] = _T("guest");
TCHAR g_szPassword[MAX_SECRET_LENGTH] = _T("");
DWORD g_dwOptions = 0;


//
// Global text constants
//

TCHAR *g_szStatusText[] = { _T("NORMAL"), _T("WARNING"), _T("MINOR"), _T("MAJOR"), _T("CRITICAL"), _T("UNKNOWN"), _T("UNMANAGED"), _T("DISABLED"), _T("TESTING") };
TCHAR *g_szStatusTextSmall[] = { _T("Normal"), _T("Warning"), _T("Minor"), _T("Major"), _T("Critical"), _T("Unknown"), _T("Unmanaged"), _T("Disabled"), _T("Testing") };
TCHAR *g_szAlarmState[] = { _T("Outstanding"), _T("Acknowledged"), _T("Resolved"), _T("Terminated") };


//
// Working directory
//

TCHAR g_szWorkDir[MAX_PATH];


//
// Colors
//

COLORREF g_rgbInfoLineBackground = RGB(255, 255, 255);
COLORREF g_rgbInfoLineTimer = RGB(0, 0, 0);
