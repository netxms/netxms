/* 
** NetXMS - Network Management System
** PocketPC Console
** Copyright (C) 2005 Victor Kirhenshtein
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
** $module: comm.cpp
** Background communication functions
**
**/

#include "stdafx.h"
#include "nxpc.h"


//
// Constants
//

#define UI_THREAD_WAIT_TIME   300


//
// Static data
//

static BOOL m_bClearCache = FALSE;


//
// Set status text in wait window
//

inline void SetInfoText(HWND hWnd, TCHAR *pszText)
{
   SendMessage(hWnd, WM_SET_INFO_TEXT, 0, (LPARAM)pszText);
}


//
// Wrapper for client library event handler
//

static void ClientEventHandler(NXC_SESSION hSession, DWORD dwEvent, DWORD dwCode, void *pArg)
{
   theApp.EventHandler(dwEvent, dwCode, pArg);
}


//
// Login thread
//

static DWORD WINAPI LoginThread(void *pArg)
{
   HWND hWnd = *((HWND *)pArg);    // Handle to status window
   DWORD dwResult;

   dwResult = NXCConnect(g_szServer, g_szLogin, g_szPassword, &g_hSession, 
                         _T("NetXMS Pocket Console/") NETXMS_VERSION_STRING,
								 FALSE, FALSE, NULL);

   // If successful, load container objects' categories
   if (dwResult == RCC_SUCCESS)
   {
      theApp.GetMainWnd()->PostMessage(WM_STATE_CHANGE, TRUE, 0);
      NXCSetEventHandler(g_hSession, ClientEventHandler);

      SetInfoText(hWnd, _T("Loading container categories..."));
      dwResult = NXCLoadCCList(g_hSession, &g_pCCList);
   }

   // Synchronize objects
   if (dwResult == RCC_SUCCESS)
   {
      BYTE bsServerId[8];
      TCHAR szCacheFile[MAX_PATH + 32];

      SetInfoText(hWnd, _T("Synchronizing objects..."));
      NXCGetServerID(g_hSession, bsServerId);
      _tcscpy(szCacheFile, g_szWorkDir);
      _tcscat(szCacheFile, WORKFILE_OBJECTCACHE);
      BinToStr(bsServerId, 8, &szCacheFile[_tcslen(szCacheFile)]);
      _tcscat(szCacheFile, _T("."));
      _tcscat(szCacheFile, g_szLogin);
      if (m_bClearCache)
         DeleteFile(szCacheFile);
      dwResult = NXCSyncObjectsEx(g_hSession, szCacheFile, FALSE);
   }

   // Set subscriptions
   if (dwResult == RCC_SUCCESS)
   {
      SetInfoText(hWnd, _T("Subscribing to notifications..."));
      dwResult = NXCSubscribe(g_hSession, NXC_CHANNEL_OBJECTS | NXC_CHANNEL_ALARMS);
   }

   // Disconnect if some of post-login operations was failed
   if (dwResult != RCC_SUCCESS)
   {
      theApp.GetMainWnd()->PostMessage(WM_STATE_CHANGE, FALSE, 0);
      NXCDisconnect(g_hSession);
      g_hSession = NULL;
   }

   PostMessage(hWnd, WM_REQUEST_COMPLETED, 0, dwResult);
   return dwResult;
}


//
// Perform login
//

DWORD DoLogin(BOOL bClearCache)
{
   HANDLE hThread;
   HWND hWnd = NULL;
   DWORD dwThreadId, dwResult;

   m_bClearCache = bClearCache;
   hThread = CreateThread(NULL, 0, LoginThread, &hWnd, CREATE_SUSPENDED, &dwThreadId);
   if (hThread != NULL)
   {
      CRequestProcessingDlg wndWaitDlg;

      wndWaitDlg.m_phWnd = &hWnd;
      wndWaitDlg.m_hThread = hThread;
      wndWaitDlg.m_strInfoText = L"Connecting to server...";
      dwResult = (DWORD)wndWaitDlg.DoModal();
      CloseHandle(hThread);
   }
   else
   {
      dwResult = RCC_SYSTEM_FAILURE;
   }

   return dwResult;
}


//
// Request processing thread
//

static DWORD WINAPI RequestThread(void *pArg)
{
   RqData *pData = (RqData *)pArg;
   DWORD dwResult;

   switch(pData->dwNumParams)
   {
      case 0:
         dwResult = pData->pFunc();
         break;
      case 1:
         dwResult = pData->pFunc(pData->pArg1);
         break;
      case 2:
         dwResult = pData->pFunc(pData->pArg1, pData->pArg2);
         break;
      case 3:
         dwResult = pData->pFunc(pData->pArg1, pData->pArg2, pData->pArg3);
         break;
      case 4:
         dwResult = pData->pFunc(pData->pArg1, pData->pArg2, pData->pArg3, pData->pArg4);
         break;
      case 5:
         dwResult = pData->pFunc(pData->pArg1, pData->pArg2, pData->pArg3, 
                                 pData->pArg4, pData->pArg5);
         break;
      case 6:
         dwResult = pData->pFunc(pData->pArg1, pData->pArg2, pData->pArg3, 
                                 pData->pArg4, pData->pArg5, pData->pArg6);
         break;
      case 7:
         dwResult = pData->pFunc(pData->pArg1, pData->pArg2, pData->pArg3, 
                                 pData->pArg4, pData->pArg5, pData->pArg6,
                                 pData->pArg7);
         break;
   }
   if (pData->hWnd != NULL)
      PostMessage(pData->hWnd, WM_REQUEST_COMPLETED, 0, dwResult);
   return dwResult;
}


//
// Perform request (common code)
//

static DWORD ExecuteRequest(RqData *pData, TCHAR *pszInfoText)
{
   HANDLE hThread;
   DWORD dwThreadId, dwResult;

   hThread = CreateThread(NULL, 0, RequestThread, pData, 0, &dwThreadId);
   if (hThread != NULL)
   {
      CRequestProcessingDlg wndWaitDlg;

      // Wait for request completion
      if (WaitForSingleObject(hThread, UI_THREAD_WAIT_TIME) == WAIT_TIMEOUT)
      {
         // Thread still not finished, open status window
         SuspendThread(hThread);
         wndWaitDlg.m_phWnd = &pData->hWnd;
         wndWaitDlg.m_hThread = hThread;
         wndWaitDlg.m_strInfoText = pszInfoText;
         dwResult = (DWORD)wndWaitDlg.DoModal();
      }
      else
      {
         // Thread is finished, get it's exit code
         if (!GetExitCodeThread(hThread, &dwResult))
            dwResult = RCC_SYSTEM_FAILURE;
      }
      CloseHandle(hThread);
   }
   else
   {
      dwResult = RCC_SYSTEM_FAILURE;
   }

   return dwResult;
}


//
// Perform generic request without parameters
//

DWORD DoRequest(DWORD (* pFunc)(void), TCHAR *pszInfoText)
{
   RqData rqData;

   rqData.hWnd = NULL;
   rqData.dwNumParams = 0;
   rqData.pFunc = (DWORD (*)(...))pFunc;
   return ExecuteRequest(&rqData, pszInfoText);
}


//
// Perform request with 1 parameter
//

DWORD DoRequestArg1(void *pFunc, void *pArg1, TCHAR *pszInfoText)
{
   RqData rqData;

   rqData.hWnd = NULL;
   rqData.dwNumParams = 1;
   rqData.pArg1 = pArg1;
   rqData.pFunc = (DWORD (*)(...))pFunc;
   return ExecuteRequest(&rqData, pszInfoText);
}


//
// Perform request with 2 parameters
//

DWORD DoRequestArg2(void *pFunc, void *pArg1, void *pArg2, TCHAR *pszInfoText)
{
   RqData rqData;

   rqData.hWnd = NULL;
   rqData.dwNumParams = 2;
   rqData.pArg1 = pArg1;
   rqData.pArg2 = pArg2;
   rqData.pFunc = (DWORD (*)(...))pFunc;
   return ExecuteRequest(&rqData, pszInfoText);
}


//
// Perform request with 3 parameter
//

DWORD DoRequestArg3(void *pFunc, void *pArg1, void *pArg2, void *pArg3, TCHAR *pszInfoText)
{
   RqData rqData;

   rqData.hWnd = NULL;
   rqData.dwNumParams = 3;
   rqData.pArg1 = pArg1;
   rqData.pArg2 = pArg2;
   rqData.pArg3 = pArg3;
   rqData.pFunc = (DWORD (*)(...))pFunc;
   return ExecuteRequest(&rqData, pszInfoText);
}


//
// Perform request with 4 parameter
//

DWORD DoRequestArg4(void *pFunc, void *pArg1, void *pArg2, void *pArg3, 
                    void *pArg4, TCHAR *pszInfoText)
{
   RqData rqData;

   rqData.hWnd = NULL;
   rqData.dwNumParams = 4;
   rqData.pArg1 = pArg1;
   rqData.pArg2 = pArg2;
   rqData.pArg3 = pArg3;
   rqData.pArg4 = pArg4;
   rqData.pFunc = (DWORD (*)(...))pFunc;
   return ExecuteRequest(&rqData, pszInfoText);
}


//
// Perform request with 5 parameter
//

DWORD DoRequestArg5(void *pFunc, void *pArg1, void *pArg2, void *pArg3, void *pArg4, 
                    void *pArg5, TCHAR *pszInfoText)
{
   RqData rqData;

   rqData.hWnd = NULL;
   rqData.dwNumParams = 5;
   rqData.pArg1 = pArg1;
   rqData.pArg2 = pArg2;
   rqData.pArg3 = pArg3;
   rqData.pArg4 = pArg4;
   rqData.pArg5 = pArg5;
   rqData.pFunc = (DWORD (*)(...))pFunc;
   return ExecuteRequest(&rqData, pszInfoText);
}


//
// Perform request with 6 parameter
//

DWORD DoRequestArg6(void *pFunc, void *pArg1, void *pArg2, void *pArg3, void *pArg4, 
                    void *pArg5, void *pArg6, TCHAR *pszInfoText)
{
   RqData rqData;

   rqData.hWnd = NULL;
   rqData.dwNumParams = 6;
   rqData.pArg1 = pArg1;
   rqData.pArg2 = pArg2;
   rqData.pArg3 = pArg3;
   rqData.pArg4 = pArg4;
   rqData.pArg5 = pArg5;
   rqData.pArg6 = pArg6;
   rqData.pFunc = (DWORD (*)(...))pFunc;
   return ExecuteRequest(&rqData, pszInfoText);
}


//
// Perform request with 7 parameter
//

DWORD DoRequestArg7(void *pFunc, void *pArg1, void *pArg2, void *pArg3, void *pArg4, 
                    void *pArg5, void *pArg6, void *pArg7, TCHAR *pszInfoText)
{
   RqData rqData;

   rqData.hWnd = NULL;
   rqData.dwNumParams = 7;
   rqData.pArg1 = pArg1;
   rqData.pArg2 = pArg2;
   rqData.pArg3 = pArg3;
   rqData.pArg4 = pArg4;
   rqData.pArg5 = pArg5;
   rqData.pArg6 = pArg6;
   rqData.pArg7 = pArg7;
   rqData.pFunc = (DWORD (*)(...))pFunc;
   return ExecuteRequest(&rqData, pszInfoText);
}
