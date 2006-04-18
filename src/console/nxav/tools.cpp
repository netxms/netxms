/* 
** NetXMS - Network Management System
** Alarm Viewer
** Copyright (C) 2004, 2005 Victor Kirhenshtein
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
** $module: tools.cpp
** Various utility functions
**
**/

#include "stdafx.h"
#include "nxav.h"


//
// Global data
//

MONITORINFOEX *g_pMonitorList = NULL;
DWORD g_dwNumMonitors = 0;


//
// Create file from resource
//

BOOL FileFromResource(UINT nResId, TCHAR *pszFileName)
{
   HRSRC hResource;
   HGLOBAL hMem;
   BYTE *pData;
   DWORD dwSize, dwBytes;
   HANDLE hFile;
   BOOL bResult = FALSE;

   hResource = FindResource(appAlarmViewer.m_hInstance, MAKEINTRESOURCE(nResId), _T("File"));
   if (hResource != NULL)
   {
      dwSize = SizeofResource(appAlarmViewer.m_hInstance, hResource);
      hMem = LoadResource(appAlarmViewer.m_hInstance, hResource);
      if (hMem != NULL)
      {
         pData = (BYTE *)LockResource(hMem);
         if (pData != NULL)
         {
            TCHAR szName[MAX_PATH];

            _sntprintf(szName, MAX_PATH, _T("%s\\%s"), g_szWorkDir, pszFileName);
            hFile = CreateFile(szName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile != INVALID_HANDLE_VALUE)
            {
               WriteFile(hFile, pData, dwSize, &dwBytes, NULL);
               CloseHandle(hFile);
               bResult = (dwSize == dwBytes);
            }
         }
      }
   }

   return bResult;
}


//
// Callback function for EnumDisplayMonitors()
//

static BOOL CALLBACK MonitorEnumCallback(HMONITOR hMonitor, HDC hdcMonitor,
                                         LPRECT lprcMonitor, LPARAM dwData)
{
   g_pMonitorList = (MONITORINFOEX *)realloc(g_pMonitorList, sizeof(MONITORINFOEX) * (g_dwNumMonitors + 1));
   g_pMonitorList[g_dwNumMonitors].cbSize = sizeof(MONITORINFOEX);
   GetMonitorInfo(hMonitor, &g_pMonitorList[g_dwNumMonitors]);
   g_dwNumMonitors++;
   return TRUE;
}


//
// Create list of available monitors
//

void CreateMonitorList(void)
{
   g_dwNumMonitors = 0;
   EnumDisplayMonitors(NULL, NULL, MonitorEnumCallback, 0);
}
