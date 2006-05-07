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
#include "RequestProcessingDlg.h"


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

inline void SetInfoText(HWND hWnd, char *pszText)
{
   SendMessage(hWnd, NXCM_SET_INFO_TEXT, 0, (LPARAM)pszText);
}


//
// Wrapper for client library event handler
//

static void ClientEventHandler(NXC_SESSION hSession, DWORD dwEvent, DWORD dwCode, void *pArg)
{
   theApp.EventHandler(dwEvent, dwCode, pArg);
}


//
// Compare two NXC_OBJECT_TOOL structures
//

static int CompareTools(const void *p1, const void *p2)
{
   TCHAR szName1[MAX_DB_STRING], szName2[MAX_DB_STRING];
   int i, j;

   for(i = 0, j = 0; ((NXC_OBJECT_TOOL *)p1)->szName[i] != 0; i++)
   {
      if (((NXC_OBJECT_TOOL *)p1)->szName[i] == _T('&'))
      {
         i++;
         if (((NXC_OBJECT_TOOL *)p1)->szName[i] == 0)
            break;
      }
      szName1[j++] = ((NXC_OBJECT_TOOL *)p1)->szName[i];
   }
   szName1[j] = 0;

   for(i = 0, j = 0; ((NXC_OBJECT_TOOL *)p2)->szName[i] != 0; i++)
   {
      if (((NXC_OBJECT_TOOL *)p2)->szName[i] == _T('&'))
      {
         i++;
         if (((NXC_OBJECT_TOOL *)p2)->szName[i] == 0)
            break;
      }
      szName2[j++] = ((NXC_OBJECT_TOOL *)p2)->szName[i];
   }
   szName2[j] = 0;

   return _tcsicmp(szName1, szName2);
}


//
// (Re)load object tools
//

DWORD LoadObjectTools(void)
{
   DWORD dwResult, dwTemp;
   CMenu *pMenu;
   HMENU hObjMenu;
   static BOOL bReload = FALSE;

   NXCDestroyObjectToolList(g_dwNumObjectTools, g_pObjectToolList);
   dwResult = NXCGetObjectTools(g_hSession, &g_dwNumObjectTools, &g_pObjectToolList);
   if (dwResult == RCC_SUCCESS)
   {
      // Sort tools in alphabetical order
      qsort(g_pObjectToolList, g_dwNumObjectTools, sizeof(NXC_OBJECT_TOOL), CompareTools);

      // Create tools submenu
      dwTemp = 0;
      pMenu = CreateToolsSubmenu(NULL, _T(""), &dwTemp);
      hObjMenu = GetSubMenu(theApp.m_hObjectBrowserMenu, LAST_APP_MENU - 1);
      if (bReload)
         DeleteMenu(hObjMenu, 18, MF_BYPOSITION);
      InsertMenu(hObjMenu, 18, MF_BYPOSITION | MF_STRING | MF_POPUP,
                 (UINT)pMenu->GetSafeHmenu(), _T("&Tools"));
      pMenu->Detach();
      delete pMenu;
      bReload = TRUE;
   }
   return dwResult;
}


//
// Login thread
//

static DWORD WINAPI LoginThread(void *pArg)
{
   HWND hWnd = *((HWND *)pArg);    // Handle to status window
   DWORD i, dwResult;

   dwResult = NXCConnect(g_szServer, g_szLogin, g_szPassword, &g_hSession,
                         _T("NetXMS Console/") NETXMS_VERSION_STRING,
                         (g_dwOptions & OPT_MATCH_SERVER_VERSION) ? TRUE : FALSE,
                         (g_dwOptions & OPT_ENCRYPT_CONNECTION) ? TRUE : FALSE);

   // If successful, load container objects' categories
   if (dwResult == RCC_SUCCESS)
   {
      theApp.GetMainWnd()->PostMessage(NXCM_STATE_CHANGE, TRUE, 0);
      NXCSetEventHandler(g_hSession, ClientEventHandler);

      if (NXCNeedPasswordChange(g_hSession))
      {
         TCHAR szPassword[MAX_DB_STRING];

         if (SendMessage(hWnd, NXCM_CHANGE_PASSWORD, 0, (LPARAM)szPassword))
         {
            SetInfoText(hWnd, "Changing password...");
            dwResult = NXCSetPassword(g_hSession, NXCGetCurrentUserId(g_hSession), szPassword);
         }
      }
   }

   if (dwResult == RCC_SUCCESS)
   {
      SetInfoText(hWnd, "Loading container categories...");
      dwResult = NXCLoadCCList(g_hSession, &g_pCCList);
   }

   // Synchronize objects
   if (dwResult == RCC_SUCCESS)
   {
      BYTE bsServerId[8];
      TCHAR szCacheFile[MAX_PATH + 32];

      SetInfoText(hWnd, "Synchronizing objects...");
      NXCGetServerID(g_hSession, bsServerId);
      _tcscpy(szCacheFile, g_szWorkDir);
      _tcscat(szCacheFile, WORKFILE_OBJECTCACHE);
      BinToStr(bsServerId, 8, &szCacheFile[_tcslen(szCacheFile)]);
      _tcscat(szCacheFile, _T("."));
      _tcscat(szCacheFile, g_szLogin);
      if (m_bClearCache)
         DeleteFile(szCacheFile);
      dwResult = NXCSubscribe(g_hSession, NXC_CHANNEL_OBJECTS);
      if (dwResult == RCC_SUCCESS)
         dwResult = NXCSyncObjectsEx(g_hSession, szCacheFile);
   }

   if (dwResult == RCC_SUCCESS)
   {
      SetInfoText(hWnd, "Loading user database...");
      dwResult = NXCLoadUserDB(g_hSession);
   }

   if (dwResult == RCC_SUCCESS)
   {
      SetInfoText(hWnd, "Loading action configuration...");
      dwResult = NXCLoadActions(g_hSession, &g_dwNumActions, &g_pActionList);
      if (dwResult == RCC_ACCESS_DENIED)
         dwResult = RCC_SUCCESS;    // User may not have rights to see actions, it's ok here
   }

   if (dwResult == RCC_SUCCESS)
   {
      DWORD dwServerTS, dwLocalTS;
      TCHAR szFileName[MAX_PATH];
      BOOL bNeedDownload;

      SetInfoText(hWnd, "Loading and initializing MIB files...");
      _tcscpy(szFileName, g_szWorkDir);
      _tcscat(szFileName, WORKDIR_MIBCACHE);
      _tcscat(szFileName, _T("\\netxms.mib"));
      if (SNMPGetMIBTreeTimestamp(szFileName, &dwLocalTS) == SNMP_ERR_SUCCESS)
      {
         if (NXCGetMIBFileTimeStamp(g_hSession, &dwServerTS) == RCC_SUCCESS)
         {
            bNeedDownload = (dwServerTS > dwLocalTS);
         }
         else
         {
            bNeedDownload = FALSE;
         }
      }
      else
      {
         bNeedDownload = TRUE;
      }

      if (bNeedDownload)
      {
         dwResult = NXCDownloadMIBFile(g_hSession, szFileName);
         if (dwResult != RCC_SUCCESS)
         {
            theApp.ErrorBox(dwResult, _T("Error downloading MIB file from server: %s"));
            dwResult = RCC_SUCCESS;
         }
      }

      if (dwResult == RCC_SUCCESS)
      {
         if (SNMPLoadMIBTree(szFileName, &g_pMIBRoot) == SNMP_ERR_SUCCESS)
         {
            g_pMIBRoot->SetName(_T("[root]"));
         }
         else
         {
            g_pMIBRoot = new SNMP_MIBObject(0, _T("[root]"));
         }
      }
   }

   if (dwResult == RCC_SUCCESS)
   {
      SetInfoText(hWnd, "Loading event information...");
      dwResult = NXCLoadEventDB(g_hSession);
   }

   // Synchronize images
   if (dwResult == RCC_SUCCESS)
   {
      char szCacheDir[MAX_PATH];

      SetInfoText(hWnd, "Synchronizing images...");
      strcpy(szCacheDir, g_szWorkDir);
      strcat(szCacheDir, WORKDIR_IMAGECACHE);
      dwResult = NXCSyncImages(g_hSession, &g_pSrvImageList, szCacheDir, IMAGE_FORMAT_ICO);
      if (dwResult == RCC_SUCCESS)
         CreateObjectImageList();
   }

   // Load default image list
   if (dwResult == RCC_SUCCESS)
   {
      DWORD *pdwClassId, *pdwImageId;

      SetInfoText(hWnd, "Loading default image list...");
      dwResult = NXCLoadDefaultImageList(g_hSession, &g_dwDefImgListSize, &pdwClassId, &pdwImageId);
      if (dwResult == RCC_SUCCESS)
      {
         g_pDefImgList = (DEF_IMG *)realloc(g_pDefImgList, sizeof(DEF_IMG) * g_dwDefImgListSize);
         for(i = 0; i < g_dwDefImgListSize; i++)
         {
            g_pDefImgList[i].dwObjectClass = pdwClassId[i];
            g_pDefImgList[i].dwImageId = pdwImageId[i];
            g_pDefImgList[i].iImageIndex = ImageIdToIndex(pdwImageId[i]);
         }
         safe_free(pdwClassId);
         safe_free(pdwImageId);
      }
   }

   // Load object tools
   if (dwResult == RCC_SUCCESS)
   {
      SetInfoText(hWnd, "Loading object tools information...");
      dwResult = LoadObjectTools();
   }

   // Disconnect if some of post-login operations was failed
   if (dwResult != RCC_SUCCESS)
   {
      theApp.GetMainWnd()->PostMessage(NXCM_STATE_CHANGE, FALSE, 0);
      NXCDisconnect(g_hSession);
      g_hSession = NULL;
   }

   PostMessage(hWnd, NXCM_REQUEST_COMPLETED, 0, dwResult);
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
      PostMessage(pData->hWnd, NXCM_REQUEST_COMPLETED, 0, dwResult);
   return dwResult;
}


//
// Perform request (common code)
//

static DWORD ExecuteRequest(RqData *pData, char *pszInfoText)
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

DWORD DoRequest(DWORD (* pFunc)(void), char *pszInfoText)
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

DWORD DoRequestArg1(void *pFunc, void *pArg1, char *pszInfoText)
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

DWORD DoRequestArg2(void *pFunc, void *pArg1, void *pArg2, char *pszInfoText)
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

DWORD DoRequestArg3(void *pFunc, void *pArg1, void *pArg2, void *pArg3, char *pszInfoText)
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
                    void *pArg4, char *pszInfoText)
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
                    void *pArg5, char *pszInfoText)
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
                    void *pArg5, void *pArg6, char *pszInfoText)
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
                    void *pArg5, void *pArg6, void *pArg7, char *pszInfoText)
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


//
// Callback function for node poller
//

static void PollerCallback(TCHAR *pszMsg, void *pArg)
{
   if (((RqData *)pArg)->hWnd != NULL)
      PostMessage(((RqData *)pArg)->hWnd, NXCM_POLLER_MESSAGE, 0, (LPARAM)_tcsdup(pszMsg));
}


//
// Poller thread
//

DWORD WINAPI PollerThread(void *pArg)
{
   RqData *pData = (RqData *)pArg;
   DWORD dwResult;

   dwResult = NXCPollNode(g_hSession, (DWORD)pData->pArg1, (int)pData->pArg2, 
                          PollerCallback, pArg);
   if (pData->hWnd != NULL)
      PostMessage(pData->hWnd, NXCM_REQUEST_COMPLETED, 0, dwResult);
   return dwResult;
}
