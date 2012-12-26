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
** File: tools.cpp
** Various tools and helper functions
**
**/

#include "stdafx.h"
#include "nxcon.h"


//
// Format time stamp
//

TCHAR *FormatTimeStamp(time_t ts, TCHAR *pszBuffer, int iType)
{
   struct tm tmbuf;
   static TCHAR *pFormat[] = { _T("%d-%b-%Y %H:%M:%S"), _T("%H:%M:%S"), _T("%b/%d"), _T("%b") };

   localtime_s(&tmbuf, &ts);
	_tcsftime(pszBuffer, 32, pFormat[iType], &tmbuf);
   return pszBuffer;
}


//
// Get size of a window
//

CSize GetWindowSize(CWnd *pWnd)
{
   RECT rect;
   CSize size;

   pWnd->GetWindowRect(&rect);
   size.cx = rect.right - rect.left;
   size.cy = rect.bottom - rect.top;
   return size;
}


//
// Check if dialog button is checked
//

BOOL IsButtonChecked(CDialog *pWnd, int nCtrl)
{
   return pWnd->SendDlgItemMessage(nCtrl, BM_GETCHECK) == BST_CHECKED;
}


//
// Select given item in list view
// Will remove selection from all currently selected items and set
// LVIS_SELECTED and LVIS_FOCUSED flags to specified item
//

void SelectListViewItem(CListCtrl *pListCtrl, int iItem)
{
   int i;

   i = pListCtrl->GetNextItem(-1, LVIS_SELECTED);
   while(i != -1)
   {
      pListCtrl->SetItemState(i, 0, LVIS_SELECTED | LVIS_FOCUSED);
      i = pListCtrl->GetNextItem(i, LVIS_SELECTED);
   }
   if (iItem != -1)
   {
      pListCtrl->SetItemState(iItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
      pListCtrl->SetSelectionMark(iItem);
   }
}


//
// Build full OID for MIB tree leaf
//

void BuildOIDString(SNMP_MIBObject *pNode, TCHAR *pszBuffer)
{
   DWORD dwPos, dwSubIdList[MAX_OID_LEN];
   int iBufPos = 0;
   SNMP_MIBObject *pCurr;

   if (pNode->getParent() == NULL)
   {
      pszBuffer[0] = 0;
   }
   else
   {
      for(dwPos = 0, pCurr = pNode; pCurr->getParent() != NULL; pCurr = pCurr->getParent())
         dwSubIdList[dwPos++] = pCurr->getObjectId();
      while(dwPos > 0)
      {
         _stprintf(&pszBuffer[iBufPos], _T(".%u"), dwSubIdList[--dwPos]);
         iBufPos = (int)_tcslen(pszBuffer);
      }
   }
}


//
// Build full symbolic OID for MIB tree leaf
//

TCHAR *BuildSymbolicOIDString(SNMP_MIBObject *pNode, DWORD dwInstance)
{
   DWORD dwPos, dwSize;
   TCHAR *pszBuffer;
   const TCHAR *pszSubIdList[MAX_OID_LEN];
   int iBufPos = 0;
   SNMP_MIBObject *pCurr;

   if (pNode->getParent() == NULL)
   {
      pszBuffer = (TCHAR *)malloc(8);
      pszBuffer[0] = 0;
   }
   else
   {
      for(dwPos = 0, dwSize = 0, pCurr = pNode; pCurr->getParent() != NULL; pCurr = pCurr->getParent())
      {
         pszSubIdList[dwPos] = CHECK_NULL(pCurr->getName());
         dwSize += (DWORD)_tcslen(pszSubIdList[dwPos]) + 1;
         dwPos++;
      }
      pszBuffer = (TCHAR *)malloc((dwSize + 16) * sizeof(TCHAR));
      for(iBufPos = 0; dwPos > 0;)
      {
         iBufPos += _stprintf(&pszBuffer[iBufPos], _T(".%s"), pszSubIdList[--dwPos]);
      }
      _stprintf(&pszBuffer[iBufPos], _T(".%lu"), dwInstance);
   }
   return pszBuffer;
}


//
// Translate UNIX text to MSDOS (i.e. convert LF to CR/LF)
//

TCHAR *TranslateUNIXText(const TCHAR *pszText)
{
   int n;
   const TCHAR *ptr;
   TCHAR *dptr, *pDst;

   // Count newline characters
   for(ptr = pszText, n = 0; *ptr != 0; ptr++)
      if (*ptr == _T('\n'))
         n++;

   pDst = (TCHAR *)malloc((_tcslen(pszText) + n + 1) * sizeof(TCHAR));
   for(ptr = pszText, dptr = pDst; *ptr != 0; ptr++)
      if (*ptr == _T('\n'))
      {
         *dptr++ = _T('\r');
         *dptr++ = _T('\n');
      }
      else
      {
         *dptr++ = *ptr;
      }
   *dptr = 0;
   return pDst;
}


//
// Load bitmap into image list
//

void LoadBitmapIntoList(CImageList *pImageList, UINT nIDResource, COLORREF rgbMaskColor)
{
   CBitmap bmp;

   bmp.LoadBitmap(nIDResource);
   pImageList->Add(&bmp, rgbMaskColor);
}


//
// Create image list with object images
//

void CreateObjectImageList(void)
{
   // Create small (16x16) image list
   if (g_pObjectSmallImageList != NULL)
      delete g_pObjectSmallImageList;
   g_pObjectSmallImageList = new CImageList;
   g_pObjectSmallImageList->Create(16, 16, ILC_COLOR24 | ILC_MASK, 16, 8);

	g_pObjectSmallImageList->Add(theApp.LoadIcon(IDI_OBJECT_NETWORK));
	g_pObjectSmallImageList->Add(theApp.LoadIcon(IDI_OBJECT_SUBNET));
	g_pObjectSmallImageList->Add(theApp.LoadIcon(IDI_OBJECT_NODE));
	g_pObjectSmallImageList->Add(theApp.LoadIcon(IDI_OBJECT_INTERFACE));
	g_pObjectSmallImageList->Add(theApp.LoadIcon(IDI_OBJECT_NETWORK));
	g_pObjectSmallImageList->Add(theApp.LoadIcon(IDI_OBJECT_CONTAINER));
	g_pObjectSmallImageList->Add(theApp.LoadIcon(IDI_OBJECT_ZONE));
	g_pObjectSmallImageList->Add(theApp.LoadIcon(IDI_OBJECT_NETWORK));
	g_pObjectSmallImageList->Add(theApp.LoadIcon(IDI_OBJECT_TEMPLATE));
	g_pObjectSmallImageList->Add(theApp.LoadIcon(IDI_OBJECT_TEMPLATEGROUP));
	g_pObjectSmallImageList->Add(theApp.LoadIcon(IDI_OBJECT_TEMPLATEROOT));
	g_pObjectSmallImageList->Add(theApp.LoadIcon(IDI_OBJECT_NETWORKSERVICE));
	g_pObjectSmallImageList->Add(theApp.LoadIcon(IDI_OBJECT_VPNC));
	g_pObjectSmallImageList->Add(theApp.LoadIcon(IDI_OBJECT_CONDITION));
	g_pObjectSmallImageList->Add(theApp.LoadIcon(IDI_OBJECT_CLUSTER));
	g_pObjectSmallImageList->Add(theApp.LoadIcon(IDI_OBJECT_CONTAINER));  // show unsupported classes as containers
	g_pObjectSmallImageList->Add(theApp.LoadIcon(IDI_OBJECT_CONTAINER));
	g_pObjectSmallImageList->Add(theApp.LoadIcon(IDI_OBJECT_CONTAINER));
	g_pObjectSmallImageList->Add(theApp.LoadIcon(IDI_OBJECT_CONTAINER));
	g_pObjectSmallImageList->Add(theApp.LoadIcon(IDI_OBJECT_CONTAINER));
	g_pObjectSmallImageList->Add(theApp.LoadIcon(IDI_OBJECT_CONTAINER));
	g_pObjectSmallImageList->Add(theApp.LoadIcon(IDI_OBJECT_CONTAINER));
	g_pObjectSmallImageList->Add(theApp.LoadIcon(IDI_OBJECT_CONTAINER));
	g_pObjectSmallImageList->Add(theApp.LoadIcon(IDI_OBJECT_CONTAINER));
	g_pObjectSmallImageList->Add(theApp.LoadIcon(IDI_OBJECT_CONTAINER));

   // Create normal (32x32) image list
   if (g_pObjectNormalImageList != NULL)
      delete g_pObjectNormalImageList;
   g_pObjectNormalImageList = new CImageList;
   g_pObjectNormalImageList->Create(32, 32, ILC_COLOR24 | ILC_MASK, 16, 8);

	g_pObjectNormalImageList->Add(theApp.LoadIcon(IDI_OBJECT_NETWORK));
	g_pObjectNormalImageList->Add(theApp.LoadIcon(IDI_OBJECT_SUBNET));
	g_pObjectNormalImageList->Add(theApp.LoadIcon(IDI_OBJECT_NODE));
	g_pObjectNormalImageList->Add(theApp.LoadIcon(IDI_OBJECT_INTERFACE));
	g_pObjectNormalImageList->Add(theApp.LoadIcon(IDI_OBJECT_NETWORK));
	g_pObjectNormalImageList->Add(theApp.LoadIcon(IDI_OBJECT_CONTAINER));
	g_pObjectNormalImageList->Add(theApp.LoadIcon(IDI_OBJECT_ZONE));
	g_pObjectNormalImageList->Add(theApp.LoadIcon(IDI_OBJECT_NETWORK));
	g_pObjectNormalImageList->Add(theApp.LoadIcon(IDI_OBJECT_TEMPLATE));
	g_pObjectNormalImageList->Add(theApp.LoadIcon(IDI_OBJECT_TEMPLATEGROUP));
	g_pObjectNormalImageList->Add(theApp.LoadIcon(IDI_OBJECT_TEMPLATEROOT));
	g_pObjectNormalImageList->Add(theApp.LoadIcon(IDI_OBJECT_NETWORKSERVICE));
	g_pObjectNormalImageList->Add(theApp.LoadIcon(IDI_OBJECT_VPNC));
	g_pObjectNormalImageList->Add(theApp.LoadIcon(IDI_OBJECT_CONDITION));
	g_pObjectNormalImageList->Add(theApp.LoadIcon(IDI_OBJECT_CLUSTER));
	g_pObjectNormalImageList->Add(theApp.LoadIcon(IDI_OBJECT_CONTAINER));  // show unsupported classes as containers
	g_pObjectNormalImageList->Add(theApp.LoadIcon(IDI_OBJECT_CONTAINER));
	g_pObjectNormalImageList->Add(theApp.LoadIcon(IDI_OBJECT_CONTAINER));
	g_pObjectNormalImageList->Add(theApp.LoadIcon(IDI_OBJECT_CONTAINER));
	g_pObjectNormalImageList->Add(theApp.LoadIcon(IDI_OBJECT_CONTAINER));
	g_pObjectNormalImageList->Add(theApp.LoadIcon(IDI_OBJECT_CONTAINER));
	g_pObjectNormalImageList->Add(theApp.LoadIcon(IDI_OBJECT_CONTAINER));
	g_pObjectNormalImageList->Add(theApp.LoadIcon(IDI_OBJECT_CONTAINER));
	g_pObjectNormalImageList->Add(theApp.LoadIcon(IDI_OBJECT_CONTAINER));
	g_pObjectNormalImageList->Add(theApp.LoadIcon(IDI_OBJECT_CONTAINER));
}


//
// Create image list with event severity icons
//

CImageList *CreateEventImageList(void)
{
   CImageList *pImageList;

   pImageList = new CImageList;
   pImageList->Create(16, 16, ILC_COLOR24 | ILC_MASK, 8, 8);
   pImageList->Add(theApp.LoadIcon(IDI_SEVERITY_NORMAL));
   pImageList->Add(theApp.LoadIcon(IDI_SEVERITY_WARNING));
   pImageList->Add(theApp.LoadIcon(IDI_SEVERITY_MINOR));
   pImageList->Add(theApp.LoadIcon(IDI_SEVERITY_MAJOR));
   pImageList->Add(theApp.LoadIcon(IDI_SEVERITY_CRITICAL));
   pImageList->Add(theApp.LoadIcon(IDI_UNKNOWN));    // For alarms
   pImageList->Add(theApp.LoadIcon(IDI_ACK));
   return pImageList;
}


//
// Find action in list by ID
//

NXC_ACTION *FindActionById(DWORD dwActionId)
{
   DWORD i;

   for(i = 0; i < g_dwNumActions; i++)
      if (g_pActionList[i].dwId == dwActionId)
         return &g_pActionList[i];
   return NULL;
}


//
// Update action list with information received from server
//

void UpdateActions(DWORD dwCode, NXC_ACTION *pAction)
{
   DWORD i;
   NXC_ACTION *pCurrAction;

   LockActions();

   // Find action with given ID
   pCurrAction = FindActionById(pAction->dwId);

   // Update action list depending on notification code
   switch(dwCode)
   {
      case NX_NOTIFY_ACTION_CREATED:
      case NX_NOTIFY_ACTION_MODIFIED:
         if (pCurrAction == NULL)
         {
            i = g_dwNumActions;
            g_dwNumActions++;
            g_pActionList = (NXC_ACTION *)realloc(g_pActionList, sizeof(NXC_ACTION) * g_dwNumActions);
            memcpy(&g_pActionList[i], pAction, sizeof(NXC_ACTION));
            g_pActionList[i].pszData = _tcsdup(pAction->pszData);
         }
         else
         {
            // Action with given id already exist, just update it
            safe_free(pCurrAction->pszData);
            memcpy(pCurrAction, pAction, sizeof(NXC_ACTION));
            pCurrAction->pszData = _tcsdup(pAction->pszData);
         }
         break;
      case NX_NOTIFY_ACTION_DELETED:
         if (pCurrAction != NULL)
         {
            for(i = 0; i < g_dwNumActions; i++)
               if (g_pActionList[i].dwId == pAction->dwId)
               {
                  g_dwNumActions--;
                  safe_free(g_pActionList[i].pszData);
                  memmove(&g_pActionList[i], &g_pActionList[i + 1], 
                          sizeof(NXC_ACTION) * (g_dwNumActions - i));
                  break;
               }
         }
         break;
   }

   UnlockActions();
}


//
// Update graph list
//

static THREAD_RESULT THREAD_CALL GraphUpdater(void *)
{
	LockGraphs();
	if (g_pGraphList != NULL)
		NXCDestroyGraphList(g_dwNumGraphs, g_pGraphList);
	NXCGetGraphList(g_hSession, &g_dwNumGraphs, &g_pGraphList);
	UnlockGraphs();
	theApp.PostThreadMessage(NXCM_GRAPH_LIST_UPDATED, 0, 0);
	return THREAD_OK;
}

void UpdateGraphList(void)
{
	ThreadCreate(GraphUpdater, 0, NULL);
}


//
// Restore placement of MDI child window
//

void RestoreMDIChildPlacement(CMDIChildWnd *pWnd, WINDOWPLACEMENT *pwp)
{
   BOOL bMaximized;

   bMaximized = (pwp->showCmd == SW_SHOWMAXIMIZED);
   pwp->showCmd = SW_SHOWNORMAL;
   pWnd->SetWindowPlacement(pwp);
   if (bMaximized)
      ::PostMessage(pWnd->GetParent()->m_hWnd, WM_MDIMAXIMIZE, (WPARAM)pWnd->m_hWnd, 0);
}


//
// Extract parameter from window parameters string
//

BOOL ExtractWindowParam(const TCHAR *pszStr, TCHAR *pszParam, TCHAR *pszBuffer, int iSize)
{
   const TCHAR *pCurr;
	TCHAR szName[32];
   int iState = 0, iPos;
   BOOL bResult = FALSE;

   for(pCurr = pszStr, iPos = 0; (*pCurr != 0) && (iState != -1); pCurr++)
   {
      switch(iState)
      {
         case 0:     // Read parameter name
            if (*pCurr == _T(':'))
            {
               szName[iPos] = 0;
               iPos = 0;
               if (!_tcscmp(szName, pszParam))
               {
                  bResult = TRUE;
                  iState = 1;
               }
               else
               {
                  iState = 2;
               }
            }
            else
            {
               if (iPos < 31)
                  szName[iPos++] = *pCurr;
            }
            break;
         case 1:     // Read value
            if (*pCurr == _T('\x7F'))
            {
               iState = -1;    // Finish
            }
            else
            {
               if (iPos < iSize - 1)
                  pszBuffer[iPos++] = *pCurr;
            }
            break;
         case 2:     // Skip value
            if (*pCurr == _T('\x7F'))
               iState = 0;
            break;
      }
   }
   pszBuffer[iPos] = 0;
   return bResult;
}


//
// Extract window parameter as DWORD
//

DWORD ExtractWindowParamULong(const TCHAR *pszStr, TCHAR *pszParam, DWORD dwDefault)
{
   TCHAR szBuffer[32], *eptr;
   DWORD dwResult;

   if (ExtractWindowParam(pszStr, pszParam, szBuffer, 32))
   {
      dwResult = _tcstoul(szBuffer, &eptr, 0);
      if (*eptr != 0)
         dwResult = dwDefault;
   }
   else
   {
      dwResult = dwDefault;
   }
   return dwResult;
}


//
// Extract window parameter as long
//

long ExtractWindowParamLong(const TCHAR *pszStr, TCHAR *pszParam, long nDefault)
{
   TCHAR szBuffer[32], *eptr;
   long nResult;

   if (ExtractWindowParam(pszStr, pszParam, szBuffer, 32))
   {
      nResult = _tcstol(szBuffer, &eptr, 0);
      if (*eptr != 0)
         nResult = nDefault;
   }
   else
   {
      nResult = nDefault;
   }
   return nResult;
}


//
// Constructors for DCIInfo class
//

DCIInfo::DCIInfo()
{
   m_dwNodeId = 0;
   m_dwItemId = 0;
   m_iSource = 0;
   m_iDataType = 0;
   m_pszParameter = NULL;
   m_pszDescription = NULL;
}

DCIInfo::DCIInfo(DCIInfo *pSrc)
{
   m_dwNodeId = pSrc->m_dwNodeId;
   m_dwItemId = pSrc->m_dwItemId;
   m_iSource = pSrc->m_iSource;
   m_iDataType = pSrc->m_iDataType;
   m_pszParameter = _tcsdup(pSrc->m_pszParameter);
   m_pszDescription = _tcsdup(pSrc->m_pszDescription);
}

DCIInfo::DCIInfo(DWORD dwNodeId, NXC_DCI *pItem)
{
   m_dwNodeId = dwNodeId;
   m_dwItemId = pItem->dwId;
   m_iSource = pItem->iSource;
   m_iDataType = pItem->iDataType;
   m_pszParameter = _tcsdup(pItem->szName);
   m_pszDescription = _tcsdup(pItem->szDescription);
}


//
// Destructor for class DCIInfo
//

DCIInfo::~DCIInfo()
{
   safe_free(m_pszParameter);
   safe_free(m_pszDescription);
}


//
// Copy menu items from one menu to another
//

void CopyMenuItems(CMenu *pDst, CMenu *pSrc)
{
   DWORD i;
   TCHAR szBuffer[256];
   MENUITEMINFO info;

   info.cbSize = sizeof(MENUITEMINFO);
   for(i = 0; i < pSrc->GetMenuItemCount(); i++)
   {
      info.fMask = MIIM_ID | MIIM_TYPE;
      info.dwTypeData = szBuffer;
      info.cch = 256;
      pSrc->GetMenuItemInfo(i, &info, TRUE);
      pDst->AppendMenu(MF_ENABLED | ((info.fType == MFT_SEPARATOR) ? MF_SEPARATOR : MF_STRING),
                       info.wID, info.dwTypeData);
   }
}


//
// Create object tools pop-up menu
//

CMenu *CreateToolsSubmenu(NXC_OBJECT *pObject, TCHAR *pszCurrPath, DWORD *pdwStart, UINT nBaseID)
{
   CMenu *pMenu;
   TCHAR szName[MAX_DB_STRING], szPath[MAX_DB_STRING];
   UINT nId;
   DWORD i;
   int j;

   pMenu = new CMenu;
   pMenu->CreatePopupMenu();
   for(i = *pdwStart, nId = nBaseID + *pdwStart; i < g_dwNumObjectTools; i++, nId++)
   {
      if (NXCIsAppropriateTool(&g_pObjectToolList[i], pObject))
      {
         // Separate item name and path
         memset(szPath, 0, sizeof(TCHAR) * MAX_DB_STRING);
         if (_tcsstr(g_pObjectToolList[i].szName, _T("->")) != NULL)
         {
            for(j = (int)_tcslen(g_pObjectToolList[i].szName) - 1; j > 0; j--)
               if ((g_pObjectToolList[i].szName[j] == _T('>')) &&
                   (g_pObjectToolList[i].szName[j - 1] == _T('-')))
               {
                  _tcscpy(szName, &g_pObjectToolList[i].szName[j + 1]);
                  j--;
                  break;
               }
            memcpy(szPath, g_pObjectToolList[i].szName, j * sizeof(TCHAR));
         }
         else
         {
            _tcscpy(szName, g_pObjectToolList[i].szName);
         }

			// Add menu item if we are on right level, otherwise create submenu or go one level up
         if (!_tcsicmp(szPath, pszCurrPath))
         {
            StrStrip(szName);
            pMenu->AppendMenu(MF_ENABLED | MF_STRING, nId, szName);
         }
         else
         {
            for(j = (int)_tcslen(pszCurrPath); (szPath[j] == _T(' ')) || (szPath[j] == _T('\t')); j++);
            if ((*pszCurrPath == 0) ||
                ((!_memicmp(szPath, pszCurrPath, _tcslen(pszCurrPath) * sizeof(TCHAR))) &&
                 (szPath[j] == _T('-')) && (szPath[j + 1] == _T('>'))))
            {
               CMenu *pSubMenu;
               TCHAR *pszName;

               // Submenu of current menu
               if (*pszCurrPath != 0)
                  j += 2;
               pszName = &szPath[j];
               for(; (szPath[j] != 0) && (memcmp(&szPath[j], _T("->"), sizeof(TCHAR) * 2)); j++);
               szPath[j] = 0;
               pSubMenu = CreateToolsSubmenu(pObject, szPath, &i, nBaseID);
               nId = nBaseID + i;
               StrStrip(pszName);
               pMenu->AppendMenu(MF_ENABLED | MF_STRING | MF_POPUP,
                                 (UINT)pSubMenu->Detach(), pszName);
               delete pSubMenu;
            }
            else
            {
               i--;
               break;
            }
         }
      }
   }
   *pdwStart = i;
   return pMenu;
}


//
// Copy list of dynamic strings
//

TCHAR **CopyStringList(TCHAR **ppList, DWORD dwSize)
{
   DWORD i;
   TCHAR **ppDst;

   ppDst = (TCHAR **)malloc(sizeof(TCHAR *) * dwSize);
   for(i = 0; i < dwSize; i++)
      ppDst[i] = _tcsdup(ppList[i]);
   return ppDst;
}


//
// Destroy list of dynamic strings
//

void DestroyStringList(TCHAR **ppList, DWORD dwSize)
{
   DWORD i;

   for(i = 0; i < dwSize; i++)
      free(ppList[i]);
   safe_free(ppList);
}


//
// Find item by text in tree control's subtree
//

HTREEITEM FindTreeCtrlItem(CTreeCtrl &ctrl, HTREEITEM hRoot, TCHAR *pszText)
{
   HTREEITEM hItem;

   hItem = ctrl.GetChildItem(hRoot);
   while(hItem != NULL)
   {
      if (!_tcsicmp(ctrl.GetItemText(hItem), pszText))
         return hItem;
      hItem = ctrl.GetNextItem(hItem, TVGN_NEXT);
   }
   return NULL;
}


//
// Find item by LPARAM in tree control's subtree (all levels)
//

HTREEITEM FindTreeCtrlItemEx(CTreeCtrl &ctrl, HTREEITEM hRoot, DWORD dwData)
{
   HTREEITEM hItem, hResult;

   if (hRoot != TVI_ROOT)
   {
      if (ctrl.GetItemData(hRoot) == dwData)
         return hRoot;
   }

   hItem = ctrl.GetChildItem(hRoot);
   while(hItem != NULL)
   {
      if (ctrl.ItemHasChildren(hItem))
      {
         hResult = FindTreeCtrlItemEx(ctrl, hItem, dwData);
         if (hResult != NULL)
            return hResult;
      }
      else
      {
         if (ctrl.GetItemData(hItem) == dwData)
            return hItem;
      }
      hItem = ctrl.GetNextItem(hItem, TVGN_NEXT);
   }
   return NULL;
}


//
// Load graphic file using IPicture interface
// Will return NULL on error or valid bitmap handle on success
//

#define HIMETRIC_INCH   2540

HBITMAP LoadPicture(TCHAR *pszFile, int nScaleFactor)
{
	IPicture *pPicture = NULL;
	HBITMAP hBitmap = NULL;
	HRESULT hRes;
 
#ifdef UNICODE
   hRes = OleLoadPicturePath(pszFile, NULL, 0,
                             -1, IID_IPicture, (LPVOID *)&pPicture);
#else
   WCHAR wszFile[MAX_PATH];

   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pszFile, -1, wszFile, MAX_PATH);
   hRes = OleLoadPicturePath(wszFile, NULL, 0,
                             -1, IID_IPicture, (LPVOID *)&pPicture);
#endif
			
	if (pPicture != NULL)
	{
   	CBitmap bmMem, *pOldBM;
	   CDC dcMem, *pDC;
      
      pDC = theApp.m_pMainWnd->GetDC();
	   if (dcMem.CreateCompatibleDC(pDC))
	   {
		   long hmWidth;
		   long hmHeight;

		   pPicture->get_Width(&hmWidth);
		   pPicture->get_Height(&hmHeight);
		   
		   int nWidth = MulDiv(hmWidth, pDC->GetDeviceCaps(LOGPIXELSX), HIMETRIC_INCH);
		   int nHeight = MulDiv(hmHeight, pDC->GetDeviceCaps(LOGPIXELSY), HIMETRIC_INCH);
         if (nScaleFactor < 0)
         {
            nWidth /= -nScaleFactor;
            nHeight /= -nScaleFactor;
         }
         else if (nScaleFactor > 0)
         {
            nWidth *= nScaleFactor;
            nHeight *= nScaleFactor;
         }

		   if (bmMem.CreateCompatibleBitmap(pDC, nWidth, nHeight))
		   {
            // Looks like an overkill, but I've seen that without following
            // line bitmap of zero size was created
            bmMem.SetBitmapDimension(nWidth, nHeight);
            pOldBM = dcMem.SelectObject(&bmMem);
			   hRes = pPicture->Render(dcMem.m_hDC, 0, 0, nWidth, nHeight, 0,
                                    hmHeight, hmWidth, -hmHeight, NULL);
			   dcMem.SelectObject(pOldBM);
		   }
			dcMem.DeleteDC();
	   }

	   theApp.m_pMainWnd->ReleaseDC(pDC);
	   hBitmap = (HBITMAP)bmMem.Detach();
      pPicture->Release();
	}

	return hBitmap;
}


//
// Save dimensions of all list control columns into registry
//

void SaveListCtrlColumns(CListCtrl &wndListCtrl, const TCHAR *pszSection, const TCHAR *pszPrefix)
{
   int i;
   LVCOLUMN lvc;
   TCHAR szParam[256];

   lvc.mask = LVCF_WIDTH;
   for(i = 0; wndListCtrl.GetColumn(i, &lvc); i++)
   {
      _sntprintf_s(szParam, 256, _TRUNCATE, _T("%s_%d"), pszPrefix, i);
      theApp.WriteProfileInt(pszSection, szParam, lvc.cx);
   }

   _sntprintf_s(szParam, 256, _TRUNCATE, _T("%s_CNT"), pszPrefix);
   theApp.WriteProfileInt(pszSection, szParam, i);
}


//
// Load and set dimensions of all list control columns
//

void LoadListCtrlColumns(CListCtrl &wndListCtrl, const TCHAR *pszSection, const TCHAR *pszPrefix)
{
   int i, nCount;
   LVCOLUMN lvc;
   TCHAR szParam[256];

   _sntprintf_s(szParam, 256, _TRUNCATE, _T("%s_CNT"), pszPrefix);
   nCount = theApp.GetProfileInt(pszSection, szParam, 0);

   lvc.mask = LVCF_WIDTH;
   for(i = 0; i < nCount; i++)
   {
      _sntprintf_s(szParam, 256, _TRUNCATE, _T("%s_%d"), pszPrefix, i);
      lvc.cx = theApp.GetProfileInt(pszSection, szParam, 50);
      wndListCtrl.SetColumn(i, &lvc);
   }
}


//
// Draw text underlined by gradient-filled line
//

void DrawHeading(CDC &dc, TCHAR *pszText, CFont *pFont, RECT *pRect,
                 COLORREF rgbColor1, COLORREF rgbColor2)
{
   TRIVERTEX vtx[2];
   GRADIENT_RECT gr;
   RECT rect;
   CFont *pOldFont;

   memcpy(&rect, pRect, sizeof(RECT));
   rect.bottom -= 5;
   rect.left += 2;
   rect.right -= 2;
   pOldFont = dc.SelectObject(pFont);
   dc.DrawText(pszText, (int)_tcslen(pszText), &rect, DT_LEFT | DT_END_ELLIPSIS | DT_NOPREFIX | DT_SINGLELINE);
   dc.SelectObject(pOldFont);

   vtx[0].x = pRect->left;
   vtx[0].y = pRect->bottom - 3;
   vtx[0].Red = (COLOR16)GetRValue(rgbColor1) << 8;
   vtx[0].Green = (COLOR16)GetGValue(rgbColor1) << 8;
   vtx[0].Blue = (COLOR16)GetBValue(rgbColor1) << 8;
   vtx[0].Alpha = 0;

   vtx[1].x = pRect->right;
   vtx[1].y = pRect->bottom;
   vtx[1].Red = (COLOR16)GetRValue(rgbColor2) << 8;
   vtx[1].Green = (COLOR16)GetGValue(rgbColor2) << 8;
   vtx[1].Blue = (COLOR16)GetBValue(rgbColor2) << 8;
   vtx[1].Alpha = 0;

   gr.UpperLeft = 0;
   gr.LowerRight = 1;

   GradientFill(dc.m_hDC, vtx, 2, &gr, 1, GRADIENT_FILL_RECT_H);
}


//
// Print bitmap
//

void PrintBitmap(CDC &dc, CBitmap &bitmap, RECT *pRect)
{
   LPBITMAPINFO info;
   BITMAP bm;
   int i, nColors  = 0;
   RGBQUAD rgb[256];

   // Obtain information about 'hbit' and store it in 'bm'
   GetObject(bitmap.GetSafeHandle(), sizeof(BITMAP), (LPVOID)&bm);

   nColors = (1 << bm.bmBitsPixel);
   if(nColors > 256)
      nColors=0;           // This is when DIB is 24 bit.
                           // In this case there is not any color table information

   info = (LPBITMAPINFO) malloc(sizeof(BITMAPINFO) + (nColors * sizeof(RGBQUAD)));

   // Before 'StretchDIBits()' we have to fill some "info" fields.
   // This information was stored in 'bm'.
   info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
   info->bmiHeader.biWidth = bm.bmWidth;
   info->bmiHeader.biHeight = bm.bmHeight;
   info->bmiHeader.biPlanes = 1;
   info->bmiHeader.biBitCount = bm.bmBitsPixel * bm.bmPlanes;
   info->bmiHeader.biCompression = BI_RGB;
   info->bmiHeader.biSizeImage = bm.bmWidthBytes * bm.bmHeight;
   info->bmiHeader.biXPelsPerMeter = 0;
   info->bmiHeader.biYPelsPerMeter = 0;
   info->bmiHeader.biClrUsed = 0;
   info->bmiHeader.biClrImportant = 0;

   // Now for 256 or less color DIB we have to fill the "info" color table parameter
   if(nColors <= 256)
   {
      HBITMAP hOldBitmap;
      HDC hMemDC = CreateCompatibleDC(NULL);

      hOldBitmap = (HBITMAP)SelectObject(hMemDC, bitmap.GetSafeHandle());
      GetDIBColorTable(hMemDC, 0, nColors, rgb);

      // Now we pass this color information to "info"
      for(i = 0; i < nColors; i++)
      {
         info->bmiColors[i].rgbRed = rgb[i].rgbRed;
         info->bmiColors[i].rgbGreen = rgb[i].rgbGreen;
         info->bmiColors[i].rgbBlue = rgb[i].rgbBlue;
      }

      SelectObject(hMemDC, hOldBitmap);
      DeleteDC(hMemDC);
   }

   bm.bmBits = malloc((info->bmiHeader.biBitCount / 8 + ((info->bmiHeader.biBitCount % 8) > 0 ? 1 : 0)) * bm.bmWidth * bm.bmHeight);
   GetDIBits(dc.GetSafeHdc(), (HBITMAP)bitmap.GetSafeHandle(), 0, bm.bmHeight,
             bm.bmBits, info, DIB_RGB_COLORS);

   // Stretching all the bitmap on a destination rectangle of size (size_x, size_y)
   // and upper left corner at (initial_pos_x, initial_pos_y)
   StretchDIBits(dc.GetSafeHdc(), pRect->left, pRect->top,
                 pRect->right - pRect->left, pRect->bottom - pRect->top,
                 0, 0, bm.bmWidth, bm.bmHeight, bm.bmBits, info,
                 DIB_RGB_COLORS, SRCCOPY);
   free(bm.bmBits);
   free(info);
}


//
// Create a copy of global memory block
//

HGLOBAL CopyGlobalMem(HGLOBAL hSrc)
{
   DWORD dwLen;
   HANDLE hDst;
   void *pSrc, *pDst;

   if (hSrc == NULL)
      return NULL;

   dwLen = (DWORD)GlobalSize(hSrc);
   hDst = GlobalAlloc(GMEM_MOVEABLE, dwLen);
   if(hDst != NULL)
   {
      pSrc = GlobalLock(hSrc);
      pDst = GlobalLock(hDst);
      CopyMemory(pDst, pSrc, dwLen);
      GlobalUnlock(hSrc);
      GlobalUnlock(hDst);
   }
   return hDst;
}
