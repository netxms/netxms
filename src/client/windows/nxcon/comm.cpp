/* 
** NetXMS - Network Management System
** Windows Console
** Copyright (C) 2004-2010 Victor Kirhenshtein
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
** File: comm.cpp
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
#define MAX_ERROR_TEXT			1024


//
// Global variables
//

TCHAR g_szUpgradeURL[1024] = _T("");


//
// Static data
//

static BOOL m_bClearCache = FALSE;
static const CERT_CONTEXT *m_pCert = NULL;
static TCHAR m_szErrorText[MAX_ERROR_TEXT];
static HANDLE m_hLocalFile = INVALID_HANDLE_VALUE;


//
// Set status text in wait window
//

inline void SetInfoText(HWND hWnd, TCHAR *pszText)
{
   SendMessage(hWnd, NXCM_SET_INFO_TEXT, 0, (LPARAM)pszText);
}


//
// Wrapper for client library event handler
//

static void ClientEventHandler(NXC_SESSION hSession, UINT32 dwEvent, UINT32 dwCode, void *pArg)
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

DWORD LoadObjectTools()
{
   UINT32 dwResult;
   DWORD dwTemp;
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
      pMenu = CreateToolsSubmenu(NULL, _T(""), &dwTemp, OBJTOOL_OB_MENU_FIRST_ID);
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
// Callback for signing server's challenge
//

static BOOL SignChallenge(BYTE *pChallenge, UINT32 dwChLen, BYTE *pSinature, UINT32 *pdwSigLen, void *pArg)
{
	return SignMessageWithCAPI(pChallenge, dwChLen, m_pCert, pSinature, *pdwSigLen, pdwSigLen);
}


//
// Login thread
//

static DWORD WINAPI LoginThread(void *pArg)
{
   HWND hWnd = *((HWND *)pArg);    // Handle to status window
   DWORD dwResult, dwFlags;
	TCHAR *pszUpgradeURL = NULL;

	dwFlags = 0;
	if (g_dwOptions & OPT_MATCH_SERVER_VERSION)
		dwFlags |= NXCF_EXACT_VERSION_MATCH;
	if (g_dwOptions & OPT_ENCRYPT_CONNECTION)
		dwFlags |= NXCF_ENCRYPT;
	if (g_nAuthType == NETXMS_AUTH_TYPE_CERTIFICATE)		// Use certificate authentication
	{
		dwFlags |= NXCF_USE_CERTIFICATE;
	}
	else
	{
		m_pCert = NULL;
	}

	dwResult = NXCConnect(dwFlags, g_szServer, g_szLogin,
								 (m_pCert != NULL) ? (TCHAR *)m_pCert->pbCertEncoded : g_szPassword,
								 (m_pCert != NULL) ? m_pCert->cbCertEncoded : 0,
								 SignChallenge, NULL, &g_hSession,
								 _T("NetXMS Console/") NETXMS_VERSION_STRING, &pszUpgradeURL);

   if (dwResult == RCC_SUCCESS)
   {
      theApp.GetMainWnd()->PostMessage(NXCM_STATE_CHANGE, TRUE, 0);
      NXCSetEventHandler(g_hSession, ClientEventHandler);

      if (NXCNeedPasswordChange(g_hSession))
      {
         TCHAR szPassword[MAX_DB_STRING];

         if (SendMessage(hWnd, NXCM_CHANGE_PASSWORD, 0, (LPARAM)szPassword))
         {
            SetInfoText(hWnd, _T("Changing password..."));
            dwResult = NXCSetPassword(g_hSession, NXCGetCurrentUserId(g_hSession), szPassword, g_szPassword);
         }
      }
   }
	else if ((dwResult == RCC_VERSION_MISMATCH) || (dwResult == RCC_BAD_PROTOCOL))
	{
		nx_strncpy(g_szUpgradeURL, CHECK_NULL_EX(pszUpgradeURL), 1024);
	}
	else
	{
		g_szUpgradeURL[0] = 0;
	}
	safe_free(pszUpgradeURL);

   // Setup connection watchdog
   if (dwResult == RCC_SUCCESS)
      NXCStartWatchdog(g_hSession);

   // If successful, load container objects' categories
   if (dwResult == RCC_SUCCESS)
   {
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
      nx_strncpy(szCacheFile, g_szWorkDir, MAX_PATH + 32);
      _tcscat_s(szCacheFile, MAX_PATH + 32, WORKFILE_OBJECTCACHE);
      BinToStr(bsServerId, 8, &szCacheFile[_tcslen(szCacheFile)]);
      _tcscat_s(szCacheFile, MAX_PATH + 32, _T("."));
      _tcscat_s(szCacheFile, MAX_PATH + 32, g_szLogin);
      if (m_bClearCache)
         DeleteFile(szCacheFile);
      dwResult = NXCSubscribe(g_hSession, NXC_CHANNEL_OBJECTS);
      if (dwResult == RCC_SUCCESS)
         dwResult = NXCSyncObjectsEx(g_hSession, szCacheFile, TRUE);
   }

   if (dwResult == RCC_SUCCESS)
   {
      SetInfoText(hWnd, _T("Loading user database..."));
      dwResult = NXCLoadUserDB(g_hSession);
   }

   if (dwResult == RCC_SUCCESS)
   {
      SetInfoText(hWnd, _T("Loading action configuration..."));
      dwResult = NXCLoadActions(g_hSession, &g_dwNumActions, &g_pActionList);
      if (dwResult == RCC_ACCESS_DENIED)
         dwResult = RCC_SUCCESS;    // User may not have rights to see actions, it's ok here
   }

   if (dwResult == RCC_SUCCESS)
   {
      UINT32 dwServerTS, dwLocalTS;
      TCHAR szFileName[MAX_PATH];
      BOOL bNeedDownload;

      SetInfoText(hWnd, _T("Loading and initializing MIB files..."));
      _tcscpy_s(szFileName, MAX_PATH, g_szWorkDir);
      _tcscat_s(szFileName, MAX_PATH, WORKDIR_MIBCACHE);
      _tcscat_s(szFileName, MAX_PATH, _T("\\netxms.mib"));
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
            g_pMIBRoot->setName(_T("[root]"));
         }
         else
         {
            g_pMIBRoot = new SNMP_MIBObject(0, _T("[root]"));
         }
      }
   }

   if (dwResult == RCC_SUCCESS)
   {
      SetInfoText(hWnd, _T("Loading event information..."));
      dwResult = NXCLoadEventDB(g_hSession);
		if (dwResult == RCC_ACCESS_DENIED)
			dwResult = RCC_SUCCESS;    // User may not have rights to view event configuration, it's ok here
   }

   // Load object tools
   if (dwResult == RCC_SUCCESS)
   {
      SetInfoText(hWnd, _T("Loading object tools information..."));
      dwResult = LoadObjectTools();
   }

   // Load predefined graphs
   if (dwResult == RCC_SUCCESS)
   {
      SetInfoText(hWnd, _T("Loading predefined graphs..."));
		NXCDestroyGraphList(g_dwNumGraphs, g_pGraphList);
      dwResult = NXCGetGraphList(g_hSession, &g_dwNumGraphs, &g_pGraphList);
   }

   // Synchronizing alarms
   if (dwResult == RCC_SUCCESS)
   {
      SetInfoText(hWnd, _T("Synchronizing alarms..."));
      dwResult = theApp.LoadAlarms();
      if (dwResult == RCC_SUCCESS)
         dwResult = NXCSubscribe(g_hSession, NXC_CHANNEL_ALARMS);
   }

   // Synchronizing situations
   if (dwResult == RCC_SUCCESS)
   {
      SetInfoText(hWnd, _T("Synchronizing situations..."));
      dwResult = theApp.LoadSituations();
      if (dwResult == RCC_SUCCESS)
		{
         dwResult = NXCSubscribe(g_hSession, NXC_CHANNEL_SITUATIONS);
		}
		else
		{
			if (dwResult == RCC_ACCESS_DENIED)
				dwResult = RCC_SUCCESS;    // User may not have rights to see situations, it's ok here
		}
   }

   // Disconnect if some of post-login operations was failed
   if (dwResult != RCC_SUCCESS)
   {
      theApp.GetMainWnd()->PostMessage(NXCM_STATE_CHANGE, FALSE, 0);
      NXCDisconnect(g_hSession);
      g_hSession = NULL;
   }

   PostMessage(hWnd, NXCM_REQUEST_COMPLETED, 0, dwResult);
	if (dwResult == RCC_SUCCESS)
		theApp.PostThreadMessage(NXCM_GRAPH_LIST_UPDATED, 0, 0);
   return dwResult;
}


//
// Starter for login thread
//

static DWORD WINAPI LoginThreadStarter(void *pArg)
{
	DWORD dwResult;

	__try
	{
		dwResult = LoginThread(pArg);
	}
	__except(___ExceptionHandler((EXCEPTION_POINTERS *)_exception_info()))
	{
		ExitProcess(99);
	}
	return dwResult;
}


//
// Perform login
//

DWORD DoLogin(BOOL bClearCache, const CERT_CONTEXT *pCert)
{
   HANDLE hThread;
   HWND hWnd = NULL;
   DWORD dwThreadId, dwResult;

   m_bClearCache = bClearCache;
	m_pCert = pCert;
   hThread = CreateThread(NULL, 0, LoginThreadStarter, &hWnd, CREATE_SUSPENDED, &dwThreadId);
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

	__try
	{
		if (pData->hWnd != NULL)
	      PostMessage(pData->hWnd, NXCM_PROCESSING_REQUEST, pData->wParam, 0);
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
			case 8:
				dwResult = pData->pFunc(pData->pArg1, pData->pArg2, pData->pArg3, 
												pData->pArg4, pData->pArg5, pData->pArg6,
												pData->pArg7, pData->pArg8);
				break;
			case 9:
				dwResult = pData->pFunc(pData->pArg1, pData->pArg2, pData->pArg3, 
												pData->pArg4, pData->pArg5, pData->pArg6,
												pData->pArg7, pData->pArg8, pData->pArg9);
				break;
		}
		if (pData->hWnd != NULL)
			PostMessage(pData->hWnd, NXCM_REQUEST_COMPLETED, pData->wParam, dwResult);
		if (pData->bDynamic)
			free(pData);
	}
	__except(___ExceptionHandler((EXCEPTION_POINTERS *)_exception_info()))
	{
		ExitProcess(99);
	}
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
	rqData.bDynamic = FALSE;
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
	rqData.bDynamic = FALSE;
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
	rqData.bDynamic = FALSE;
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
	rqData.bDynamic = FALSE;
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
	rqData.bDynamic = FALSE;
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
	rqData.bDynamic = FALSE;
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
	rqData.bDynamic = FALSE;
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
	rqData.bDynamic = FALSE;
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
// Perform request with 8 parameter
//

DWORD DoRequestArg8(void *pFunc, void *pArg1, void *pArg2, void *pArg3, void *pArg4, 
                    void *pArg5, void *pArg6, void *pArg7, void *pArg8, TCHAR *pszInfoText)
{
   RqData rqData;

   rqData.hWnd = NULL;
	rqData.bDynamic = FALSE;
   rqData.dwNumParams = 8;
   rqData.pArg1 = pArg1;
   rqData.pArg2 = pArg2;
   rqData.pArg3 = pArg3;
   rqData.pArg4 = pArg4;
   rqData.pArg5 = pArg5;
   rqData.pArg6 = pArg6;
   rqData.pArg7 = pArg7;
   rqData.pArg8 = pArg8;
   rqData.pFunc = (DWORD (*)(...))pFunc;
   return ExecuteRequest(&rqData, pszInfoText);
}


//
// Perform request with 9 parameter
//

DWORD DoRequestArg9(void *pFunc, void *pArg1, void *pArg2, void *pArg3, void *pArg4, 
                    void *pArg5, void *pArg6, void *pArg7, void *pArg8, void *pArg9,
                    TCHAR *pszInfoText)
{
   RqData rqData;

   rqData.hWnd = NULL;
	rqData.bDynamic = FALSE;
   rqData.dwNumParams = 9;
   rqData.pArg1 = pArg1;
   rqData.pArg2 = pArg2;
   rqData.pArg3 = pArg3;
   rqData.pArg4 = pArg4;
   rqData.pArg5 = pArg5;
   rqData.pArg6 = pArg6;
   rqData.pArg7 = pArg7;
   rqData.pArg8 = pArg8;
   rqData.pArg9 = pArg9;
   rqData.pFunc = (DWORD (*)(...))pFunc;
   return ExecuteRequest(&rqData, pszInfoText);
}


//
// Perform async request (common code)
//

static void ExecuteAsyncRequest(RqData *pData)
{
   HANDLE hThread;
   DWORD dwThreadId;
	HWND hWnd;
	WPARAM wParam;

	hWnd = pData->hWnd;
	wParam = pData->wParam;
   hThread = CreateThread(NULL, 0, RequestThread, pData, 0, &dwThreadId);
   if (hThread != NULL)
   {
      CloseHandle(hThread);
   }
	else
	{
      PostMessage(hWnd, NXCM_REQUEST_COMPLETED, wParam, RCC_SYSTEM_FAILURE);
		free(pData);
	}
}


//
// Perform async request with 2 parameter
//

void DoAsyncRequestArg2(HWND hWnd, WPARAM wParam, void *pFunc, void *pArg1, void *pArg2)
{
   RqData *pData;

	pData = (RqData *)malloc(sizeof(RqData));
   pData->hWnd = hWnd;
	pData->bDynamic = TRUE;
	pData->wParam = wParam;
   pData->dwNumParams = 2;
   pData->pArg1 = pArg1;
   pData->pArg2 = pArg2;
   pData->pFunc = (DWORD (*)(...))pFunc;
   ExecuteAsyncRequest(pData);
}


//
// Perform async request with 7 parameter
//

void DoAsyncRequestArg7(HWND hWnd, WPARAM wParam, void *pFunc, void *pArg1, void *pArg2,
                        void *pArg3, void *pArg4, void *pArg5, void *pArg6, void *pArg7)
{
   RqData *pData;

	pData = (RqData *)malloc(sizeof(RqData));
   pData->hWnd = hWnd;
	pData->bDynamic = TRUE;
	pData->wParam = wParam;
   pData->dwNumParams = 7;
   pData->pArg1 = pArg1;
   pData->pArg2 = pArg2;
   pData->pArg3 = pArg3;
   pData->pArg4 = pArg4;
   pData->pArg5 = pArg5;
   pData->pArg6 = pArg6;
   pData->pArg7 = pArg7;
   pData->pFunc = (DWORD (*)(...))pFunc;
   ExecuteAsyncRequest(pData);
}


//
// Perform async request with 9 parameter
//

void DoAsyncRequestArg9(HWND hWnd, WPARAM wParam, void *pFunc, void *pArg1, void *pArg2,
                        void *pArg3, void *pArg4, void *pArg5, void *pArg6, void *pArg7,
								void *pArg8, void *pArg9)
{
   RqData *pData;

	pData = (RqData *)malloc(sizeof(RqData));
   pData->hWnd = hWnd;
	pData->bDynamic = TRUE;
	pData->wParam = wParam;
   pData->dwNumParams = 9;
   pData->pArg1 = pArg1;
   pData->pArg2 = pArg2;
   pData->pArg3 = pArg3;
   pData->pArg4 = pArg4;
   pData->pArg5 = pArg5;
   pData->pArg6 = pArg6;
   pData->pArg7 = pArg7;
   pData->pArg8 = pArg8;
   pData->pArg9 = pArg9;
   pData->pFunc = (DWORD (*)(...))pFunc;
   ExecuteAsyncRequest(pData);
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

THREAD_RESULT THREAD_CALL PollerThread(void *pArg)
{
   RqData *pData = (RqData *)pArg;
   DWORD dwResult;

   dwResult = NXCPollNode(g_hSession, (DWORD)pData->pArg1, (int)pData->pArg2, 
                          PollerCallback, pArg);
   if (pData->hWnd != NULL)
      PostMessage(pData->hWnd, NXCM_REQUEST_COMPLETED, 0, dwResult);
   return THREAD_OK;
}


//
// Download thread
//

static DWORD WINAPI DownloadThread(void *pArg)
{
   HWND hWnd = *((HWND *)pArg);    // Handle to status window
	HINTERNET hSession, hConn, hRequest;
	DWORD dwResult = 1, dwBytes, dwTotal, dwIndex, dwStatus;
	TCHAR szBuffer[256], szHostName[256], szPath[1024];
	BYTE data[8192];
	URL_COMPONENTS url;
	const TCHAR *pszMediaTypes[] = { _T("application/octet-stream"), NULL };

	memset(&url, 0, sizeof(URL_COMPONENTS));
	url.dwStructSize = sizeof(URL_COMPONENTS);
	url.lpszHostName = szHostName;
	url.dwHostNameLength = 256;
	url.lpszUrlPath = szPath;
	url.dwUrlPathLength = 1024;
	InternetCrackUrl(g_szUpgradeURL, 0, 0, &url);

	hSession = InternetOpen(_T("NetXMS Console/") NETXMS_VERSION_STRING,
	                        INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (hSession != NULL)
	{
		SetInfoText(hWnd, _T("Connecting..."));
		hConn = InternetConnect(hSession, szHostName, url.nPort, NULL, NULL,
		                        INTERNET_SERVICE_HTTP, 0, 0);
		if (hConn != NULL)
		{
			hRequest = HttpOpenRequest(hConn, NULL, szPath, NULL, NULL,
			                           pszMediaTypes, INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD, 0);
			if (hRequest != NULL)
			{
				if (HttpSendRequest(hRequest, NULL, 0, NULL, 0))
				{
					dwIndex = 0;
					dwBytes = sizeof(DWORD);
					HttpQueryInfo(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
					               &dwStatus, &dwBytes, &dwIndex);
					if (dwStatus == HTTP_STATUS_OK)
					{
						dwTotal = 0;
						while(1)
						{
							if (!InternetReadFile(hRequest, data, 8192, &dwBytes))
							{
								AfxLoadString(IDS_DATA_TRANSFER_ERROR, m_szErrorText, MAX_ERROR_TEXT);
								break;
							}
							WriteFile(m_hLocalFile, data, dwBytes, &dwStatus, NULL);
							dwTotal += dwBytes;
							_sntprintf_s(szBuffer, 256, _TRUNCATE, _T("Downloading: %dK bytes received"), dwTotal / 1024);
							SetInfoText(hWnd, szBuffer);
							if (dwBytes == 0)
							{
								dwResult = 0;
								break;
							}
						}
					}
					else
					{
						TCHAR szStatus[512];

						dwBytes = 512 * sizeof(TCHAR);
						dwIndex = 0;
						HttpQueryInfo(hRequest, HTTP_QUERY_STATUS_CODE,	szStatus, &dwBytes, &dwIndex);
						AfxLoadString(IDS_HTTP_ERROR, szBuffer, 256);
						_sntprintf_s(m_szErrorText, MAX_ERROR_TEXT, _TRUNCATE, szBuffer, szStatus);
					}
				}
				else
				{
					AfxLoadString(IDS_CANNOT_SEND_HTTP_REQUEST, m_szErrorText, MAX_ERROR_TEXT);
				}
				InternetCloseHandle(hRequest);
			}
			else
			{
				AfxLoadString(IDS_CANNOT_OPEN_HTTP_REQUEST, m_szErrorText, MAX_ERROR_TEXT);
			}
			InternetCloseHandle(hConn);
		}
		else
		{
			AfxLoadString(IDS_WININET_CONNECT_FAILED, m_szErrorText, MAX_ERROR_TEXT);
		}
		InternetCloseHandle(hSession);
	}
	else
	{
		AfxLoadString(IDS_WININET_INIT_FAILED, m_szErrorText, MAX_ERROR_TEXT);
	}

   PostMessage(hWnd, NXCM_REQUEST_COMPLETED, 0, dwResult);
	return dwResult;
}


//
// Download thread
//

static DWORD WINAPI DownloadThreadStarter(void *pArg)
{
	DWORD dwResult;

	__try
	{
		dwResult = DownloadThread(pArg);
	}
	__except(___ExceptionHandler((EXCEPTION_POINTERS *)_exception_info()))
	{
		ExitProcess(99);
	}
	return dwResult;
}


//
// Download upgrade file
//

BOOL DownloadUpgradeFile(HANDLE hFile)
{
   HANDLE hThread;
   HWND hWnd = NULL;
   DWORD dwThreadId;
	BOOL bRet;

	AfxLoadString(IDS_INTERNAL_ERROR, m_szErrorText, MAX_ERROR_TEXT);
	m_hLocalFile = hFile;

   hThread = CreateThread(NULL, 0, DownloadThreadStarter, &hWnd, CREATE_SUSPENDED, &dwThreadId);
   if (hThread != NULL)
   {
      CRequestProcessingDlg wndWaitDlg;

      wndWaitDlg.m_phWnd = &hWnd;
      wndWaitDlg.m_hThread = hThread;
      wndWaitDlg.m_strInfoText = _T("Initializing...");
      bRet = ((DWORD)wndWaitDlg.DoModal() == 0);
      CloseHandle(hThread);
   }
   else
   {
      bRet = FALSE;
   }

	if (!bRet)
	{
		TCHAR szError[2048], szFormat[256], szCaption[256];
	
		AfxLoadString(IDS_DOWNLOAD_ERROR, szFormat, 256);
		_sntprintf_s(szError, 2048, _TRUNCATE, szFormat, m_szErrorText);
		AfxLoadString(IDS_CAPTION_ERROR, szCaption, 256);
		theApp.m_pMainWnd->MessageBox(szError, szCaption, MB_OK | MB_ICONSTOP);
	}

   return bRet;
}
