/* 
** NetXMS - Network Management System
** Windows Console
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
** $module: comm.cpp
** Background communication functions
**
**/

#include "stdafx.h"
#include "nxcon.h"


//
// Request parameters structure
//

struct RqData
{
   HWND hWnd;
   DWORD (* pFunc)(...);
   DWORD dwNumParams;
   void *pArg1;
   void *pArg2;
};


//
// Set status text in wait window
//

inline void SetInfoText(HWND hWnd, char *pszText)
{
   SendMessage(hWnd, WM_SET_INFO_TEXT, 0, (LPARAM)pszText);
}


//
// Login thread
//

static DWORD WINAPI LoginThread(void *pArg)
{
   HWND hWnd = *((HWND *)pArg);    // Handle to status window
   DWORD dwResult;

   dwResult = NXCConnect(g_szServer, g_szLogin, g_szPassword);
   if (dwResult == RCC_SUCCESS)
   {
      // Now we are connected, request data sync
      SetInfoText(hWnd, "Synchronizing objects...");
      dwResult = NXCSyncObjects();

      if (dwResult == RCC_SUCCESS)
      {
         SetInfoText(hWnd, "Loading user database...");
         dwResult = NXCLoadUserDB();
      }

      // Disconnect if some of post-login operations was failed
      if (dwResult != RCC_SUCCESS)
         NXCDisconnect();
   }

   PostMessage(hWnd, WM_REQUEST_COMPLETED, 0, dwResult);
   return dwResult;
}


//
// Perform login
//

DWORD DoLogin(void)
{
   HANDLE hThread;
   HWND hWnd = NULL;
   DWORD dwThreadId, dwResult;

   hThread = CreateThread(NULL, 0, LoginThread, &hWnd, CREATE_SUSPENDED, &dwThreadId);
   if (hThread != NULL)
   {
      CRequestProcessingDlg wndWaitDlg;

      wndWaitDlg.m_phWnd = &hWnd;
      wndWaitDlg.m_hThread = hThread;
      wndWaitDlg.m_strInfoText = "Connecting to server...";
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
// Login thread
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
   }
   if (pData->hWnd != NULL)
      PostMessage(pData->hWnd, WM_REQUEST_COMPLETED, 0, dwResult);
   return dwResult;
}


//
// Perform generic request without parameters
//

DWORD DoRequest(DWORD (* pFunc)(void), char *pszInfoText)
{
   HANDLE hThread;
   RqData rqData;
   DWORD dwThreadId, dwResult;

   rqData.hWnd = NULL;
   rqData.dwNumParams = 0;
   rqData.pFunc = (DWORD (*)(...))pFunc;
   hThread = CreateThread(NULL, 0, RequestThread, &rqData, 0, &dwThreadId);
   if (hThread != NULL)
   {
      CRequestProcessingDlg wndWaitDlg;

      // Wait half a second for request completion
      if (WaitForSingleObject(hThread, 500) == WAIT_TIMEOUT)
      {
         // Thread still not finished, open status window
         SuspendThread(hThread);
         wndWaitDlg.m_phWnd = &rqData.hWnd;
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
// Perform request with 1 parameter
//

DWORD DoRequestArg1(void *pFunc, void *pArg1, char *pszInfoText)
{
   HANDLE hThread;
   RqData rqData;
   DWORD dwThreadId, dwResult;

   rqData.hWnd = NULL;
   rqData.dwNumParams = 1;
   rqData.pArg1 = pArg1;
   rqData.pFunc = (DWORD (*)(...))pFunc;
   hThread = CreateThread(NULL, 0, RequestThread, &rqData, 0, &dwThreadId);
   if (hThread != NULL)
   {
      CRequestProcessingDlg wndWaitDlg;

      // Wait half a second for request completion
      if (WaitForSingleObject(hThread, 500) == WAIT_TIMEOUT)
      {
         // Thread still not finished, open status window
         SuspendThread(hThread);
         wndWaitDlg.m_phWnd = &rqData.hWnd;
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
