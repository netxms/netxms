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
** $module: tools.cpp
** Various tools and helper functions
**
**/

#include "stdafx.h"
#include "nxpc.h"


//
// Format time stamp
//

TCHAR *FormatTimeStamp(DWORD dwTimeStamp, TCHAR *pszBuffer, int iType)
{
/*   struct tm *pTime;
   static TCHAR *pFormat[] = { L"%d-%b-%Y %H:%M:%S", L"%H:%M:%S" };

   pTime = localtime((const time_t *)&dwTimeStamp);
   _tcsftime(pszBuffer, 32, pFormat[iType], pTime);*/
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
// Translate given code to text
//

const TCHAR *CodeToText(int iCode, CODE_TO_TEXT *pTranslator, const TCHAR *pszDefaultText)
{
   int i;

   for(i = 0; pTranslator[i].pszText != NULL; i++)
      if (pTranslator[i].iCode == iCode)
         return pTranslator[i].pszText;
   return pszDefaultText;
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
/*   HICON hIcon;
   DWORD i, dwPos;
   TCHAR szFileName[MAX_PATH];

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

   wcscpy(szFileName, g_szWorkDir);
   wcscat(szFileName, WORKDIR_IMAGECACHE);
   wcscat(szFileName, L"\\");
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
   }*/
}


//
// Get image index for given object
//

int GetObjectImageIndex(NXC_OBJECT *pObject)
{
   DWORD i;

   // Check if object has custom image
   if (pObject->dwImage != IMG_DEFAULT)
      return ImageIdToIndex(pObject->dwImage);

   // Find default image for class
   for(i = 0; i < g_dwDefImgListSize; i++)
      if (g_pDefImgList[i].dwObjectClass == (DWORD)pObject->iClass)
         return g_pDefImgList[i].iImageIndex;
   
   return -1;
}


//
// Create image list with event severity icons
//

CImageList *CreateEventImageList(void)
{
   CImageList *pImageList;

   pImageList = new CImageList;
   pImageList->Create(16, 16, ILC_MASK, 8, 8);
   pImageList->Add(theApp.LoadIcon(IDI_SEVERITY_NORMAL));
   pImageList->Add(theApp.LoadIcon(IDI_SEVERITY_WARNING));
   pImageList->Add(theApp.LoadIcon(IDI_SEVERITY_MINOR));
   pImageList->Add(theApp.LoadIcon(IDI_SEVERITY_MAJOR));
   pImageList->Add(theApp.LoadIcon(IDI_SEVERITY_CRITICAL));
   pImageList->Add(theApp.LoadIcon(IDI_UNKNOWN));    // For alarms
   pImageList->Add(theApp.LoadIcon(IDI_ACK));
   return pImageList;
}
