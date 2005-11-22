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
** $module: tools.cpp
** Various tools and helper functions
**
**/

#include "stdafx.h"
#include "nxcon.h"


//
// Format time stamp
//

char *FormatTimeStamp(DWORD dwTimeStamp, char *pszBuffer, int iType)
{
   struct tm *pTime;
   static char *pFormat[] = { "%d-%b-%Y %H:%M:%S", "%H:%M:%S" };

   pTime = localtime((const time_t *)&dwTimeStamp);
   strftime(pszBuffer, 32, pFormat[iType], pTime);
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
// Enable or disable dialog item
//

void EnableDlgItem(CDialog *pWnd, int nCtrl, BOOL bEnable)
{
   CWnd *pCtrl;

   pCtrl = pWnd->GetDlgItem(nCtrl);
   if (pCtrl != NULL)
      pCtrl->EnableWindow(bEnable);
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
   pListCtrl->SetItemState(iItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
   pListCtrl->SetSelectionMark(iItem);
}


//
// Build full OID for MIB tree leaf
//

void BuildOIDString(SNMP_MIBObject *pNode, TCHAR *pszBuffer)
{
   DWORD dwPos, dwSubIdList[MAX_OID_LEN];
   int iBufPos = 0;
   SNMP_MIBObject *pCurr;

   if (pNode->Parent() == NULL)
   {
      pszBuffer[0] = 0;
   }
   else
   {
      for(dwPos = 0, pCurr = pNode; pCurr->Parent() != NULL; pCurr = pCurr->Parent())
         dwSubIdList[dwPos++] = pCurr->OID();
      while(dwPos > 0)
      {
         sprintf(&pszBuffer[iBufPos], ".%u", dwSubIdList[--dwPos]);
         iBufPos = strlen(pszBuffer);
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

   if (pNode->Parent() == NULL)
   {
      pszBuffer = (char *)malloc(4);
      pszBuffer[0] = 0;
   }
   else
   {
      for(dwPos = 0, dwSize = 0, pCurr = pNode; pCurr->Parent() != NULL; pCurr = pCurr->Parent())
      {
         pszSubIdList[dwPos] = CHECK_NULL(pCurr->Name());
         dwSize += strlen(pszSubIdList[dwPos]) + 1;
         dwPos++;
      }
      pszBuffer = (char *)malloc(dwSize + 16);
      for(iBufPos = 0; dwPos > 0;)
      {
         iBufPos += sprintf(&pszBuffer[iBufPos], ".%s", pszSubIdList[--dwPos]);
      }
      sprintf(&pszBuffer[iBufPos], ".%lu", dwInstance);
   }
   return pszBuffer;
}


//
// Translate given code to text
//

const char *CodeToText(int iCode, CODE_TO_TEXT *pTranslator, const char *pszDefaultText)
{
   int i;

   for(i = 0; pTranslator[i].pszText != NULL; i++)
      if (pTranslator[i].iCode == iCode)
         return pTranslator[i].pszText;
   return pszDefaultText;
}


//
// Translate UNIX text to MSDOS (i.e. convert LF to CR/LF)
//

char *TranslateUNIXText(const char *pszText)
{
   int n;
   const char *ptr;
   char *dptr, *pDst;

   // Count newline characters
   for(ptr = pszText, n = 0; *ptr != 0; ptr++)
      if (*ptr == '\n')
         n++;

   pDst = (char *)malloc(strlen(pszText) + n + 1);
   for(ptr = pszText, dptr = pDst; *ptr != 0; ptr++)
      if (*ptr == '\n')
      {
         *dptr++ = '\r';
         *dptr++ = '\n';
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
// Find image's index in list by image id
//

int ImageIdToIndex(DWORD dwImageId)
{
   DWORD i;

   for(i = 0; i < g_pSrvImageList->dwNumImages; i++)
      if (g_pSrvImageList->pImageList[i].dwId == dwImageId)
         return i;
   return -1;
}


//
// Create image list with object images
//

void CreateObjectImageList(void)
{
   HICON hIcon;
   DWORD i, dwPos;
   char szFileName[MAX_PATH];

   // Create small (16x16) image list
   if (g_pObjectSmallImageList != NULL)
      delete g_pObjectSmallImageList;
   g_pObjectSmallImageList = new CImageList;
   g_pObjectSmallImageList->Create(16, 16, ILC_COLOR24 | ILC_MASK, 16, 8);

   // Create normal (32x32) image list
   if (g_pObjectNormalImageList != NULL)
      delete g_pObjectNormalImageList;
   g_pObjectNormalImageList = new CImageList;
   g_pObjectNormalImageList->Create(32, 32, ILC_COLOR24 | ILC_MASK, 16, 8);

   strcpy(szFileName, g_szWorkDir);
   strcat(szFileName, WORKDIR_IMAGECACHE);
   strcat(szFileName, "\\");
   dwPos = strlen(szFileName);

   for(i = 0; i < g_pSrvImageList->dwNumImages; i++)
   {
      sprintf(&szFileName[dwPos], "%08x.ico", g_pSrvImageList->pImageList[i].dwId);
      
      // Load and add 16x16 image
      hIcon = (HICON)LoadImage(NULL, szFileName, IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
      g_pObjectSmallImageList->Add(hIcon);
      DestroyIcon(hIcon);

      // Load and add 32x32 image
      hIcon = (HICON)LoadImage(NULL, szFileName, IMAGE_ICON, 32, 32, LR_LOADFROMFILE);
      g_pObjectNormalImageList->Add(hIcon);
      DestroyIcon(hIcon);
   }
}


//
// Get index of class default image
//

int GetClassDefaultImageIndex(int iClass)
{
   DWORD i;

   for(i = 0; i < g_dwDefImgListSize; i++)
      if (g_pDefImgList[i].dwObjectClass == (DWORD)iClass)
         return g_pDefImgList[i].iImageIndex;
   return -1;
}


//
// Get image index for given object
//

int GetObjectImageIndex(NXC_OBJECT *pObject)
{
   // Check if object has custom image
   return (pObject->dwImage != IMG_DEFAULT) ?
      ImageIdToIndex(pObject->dwImage) :
      GetClassDefaultImageIndex(pObject->iClass);
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
            g_pActionList[i].pszData = strdup(pAction->pszData);
         }
         else
         {
            // Action with given id already exist, just update it
            safe_free(pCurrAction->pszData);
            memcpy(pCurrAction, pAction, sizeof(NXC_ACTION));
            pCurrAction->pszData = strdup(pAction->pszData);
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

BOOL ExtractWindowParam(TCHAR *pszStr, TCHAR *pszParam, TCHAR *pszBuffer, int iSize)
{
   TCHAR *pCurr, szName[32];
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

DWORD ExtractWindowParamULong(TCHAR *pszStr, TCHAR *pszParam, DWORD dwDefault)
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

long ExtractWindowParamLong(TCHAR *pszStr, TCHAR *pszParam, long nDefault)
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

CMenu *CreateToolsSubmenu(NXC_OBJECT *pObject, TCHAR *pszCurrPath, DWORD *pdwStart)
{
   CMenu *pMenu;
   TCHAR szName[MAX_DB_STRING], szPath[MAX_DB_STRING];
   UINT nId;
   DWORD i;
   int j;

   pMenu = new CMenu;
   pMenu->CreatePopupMenu();
   for(i = *pdwStart, nId = OBJTOOL_MENU_FIRST_ID + *pdwStart; i < g_dwNumObjectTools; i++, nId++)
   {
      if (NXCIsAppropriateTool(&g_pObjectToolList[i], pObject))
      {
         // Separate item name and path
         memset(szPath, 0, sizeof(TCHAR) * MAX_DB_STRING);
         if (_tcsstr(g_pObjectToolList[i].szName, _T("->")) != NULL)
         {
            for(j = _tcslen(g_pObjectToolList[i].szName) - 1; j > 0; j--)
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

         if (!_tcsicmp(szPath, pszCurrPath))
         {
            StrStrip(szName);
            pMenu->AppendMenu(MF_ENABLED | MF_STRING, nId, szName);
         }
         else
         {
            for(j = _tcslen(pszCurrPath); (szPath[j] == _T(' ')) || (szPath[j] == _T('\t')); j++);
            if ((*pszCurrPath == 0) ||
                ((!memicmp(szPath, pszCurrPath, _tcslen(pszCurrPath) * sizeof(TCHAR))) &&
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
               pSubMenu = CreateToolsSubmenu(pObject, szPath, &i);
               nId = OBJTOOL_MENU_FIRST_ID + i;
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
