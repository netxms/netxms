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
** $module: globals.h
**
**/

#ifndef _globals_h_
#define _globals_h_


//
// Application object declaration
//

class CConsoleApp;
extern CConsoleApp theApp;


//
// Constants
//

#define MAX_SERVER_NAME_LEN   64
#define MAX_LOGIN_NAME_LEN    64
#define MAX_PASSWORD_LEN      64
#define MAX_INTERFACE_TYPE    32


//
// Custom windows messages
//

#define WM_REQUEST_COMPLETED     (WM_USER + 1)
#define WM_CLOSE_STATUS_DLG      (WM_USER + 2)
#define WM_OBJECT_CHANGE         (WM_USER + 3)
#define WM_FIND_OBJECT           (WM_USER + 4)
#define WM_EDITBOX_EVENT         (WM_USER + 5)
#define WM_SET_INFO_TEXT         (WM_USER + 6)
#define WM_USERDB_CHANGE         (WM_USER + 7)


//
// Functions
//

DWORD DoLogin(void);
DWORD DoRequest(DWORD (* pFunc)(void), char *pszInfoText);
DWORD DoRequestArg1(void *pFunc, void *pArg1, char *pszInfoText);
DWORD DoRequestArg2(void *pFunc, void *pArg1, void *pArg2, char *pszInfoText);
DWORD DoRequestArg3(void *pFunc, void *pArg1, void *pArg2, void *pArg3, char *pszInfoText);
DWORD DoRequestArg6(void *pFunc, void *pArg1, void *pArg2, void *pArg3, void *pArg4, 
                    void *pArg5, void *pArg6, char *pszInfoText);


//
// Window and drawing functions
//

void DrawPieChart(CDC &dc, RECT *pRect, int iNumElements, DWORD *pdwValues, COLORREF *pColors);


//
// Utility functions
//

char *FormatTimeStamp(DWORD dwTimeStamp, char *pszBuffer);
CSize GetWindowSize(CWnd *pWnd);


//
// Variables
//

extern char g_szServer[];
extern char g_szLogin[];
extern char g_szPassword[];
extern char *g_szStatusText[];
extern char *g_szStatusTextSmall[];
extern COLORREF g_statusColorTable[];
extern char *g_szObjectClass[];
extern char *g_szInterfaceTypes[];
extern char *g_pszItemOrigin[];
extern char *g_pszItemOriginLong[];
extern char *g_pszItemDataType[];
extern char *g_pszItemStatus[];

#endif
