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
** $module: globals.h
**
**/

#ifndef _globals_h_
#define _globals_h_


//
// Custom windows messages
//

#define WM_REQUEST_COMPLETED     (WM_USER + 1)
#define WM_SET_INFO_TEXT         (WM_USER + 2)
#define WM_ALARM_UPDATE          (WM_USER + 3)


//
// Application flags
//

#define AF_PLAY_SOUND            0x0001


//
// Functions
//

DWORD DoLogin(void);
DWORD DoRequest(DWORD (* pFunc)(void), char *pszInfoText);
DWORD DoRequestArg1(void *pFunc, void *pArg1, char *pszInfoText);
DWORD DoRequestArg2(void *pFunc, void *pArg1, void *pArg2, char *pszInfoText);
DWORD DoRequestArg3(void *pFunc, void *pArg1, void *pArg2, void *pArg3, char *pszInfoText);
DWORD DoRequestArg4(void *pFunc, void *pArg1, void *pArg2, void *pArg3, void *pArg4, char *pszInfoText);
DWORD DoRequestArg6(void *pFunc, void *pArg1, void *pArg2, void *pArg3, void *pArg4, 
                    void *pArg5, void *pArg6, char *pszInfoText);
BOOL FileFromResource(UINT nResId, TCHAR *pszFileName);


//
// Variables
//

extern TCHAR g_szServer[];
extern TCHAR g_szLogin[];
extern TCHAR g_szPassword[];
extern TCHAR *g_szStatusText[];
extern TCHAR *g_szStatusTextSmall[];
extern TCHAR g_szWorkDir[];
extern COLORREF g_rgbInfoLineButtons;
extern COLORREF g_rgbInfoLineBackground;
extern DWORD g_dwFlags;


#endif
