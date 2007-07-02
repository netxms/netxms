/* 
** NetXMS - Network Management System
** PocketPC Console
** Copyright (C) 2005, 2006 Victor Kirhenshtein
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
#include "nxpc.h"


//
// Format time stamp
//

TCHAR *FormatTimeStamp(DWORD dwTimeStamp, TCHAR *pszBuffer, int iType)
{
   struct tm *ptm;

   ptm = WCE_FCTN(localtime)((const time_t *)&dwTimeStamp);
   switch(iType)
   {
      case TS_LONG_DATE_TIME:
         _stprintf(pszBuffer, _T("%02d-%02d-%04d %02d:%02d:%02d"), ptm->tm_mday,
                   ptm->tm_mon + 1, ptm->tm_year + 1900, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
         break;
      case TS_LONG_TIME:
         _stprintf(pszBuffer, _T("%02d:%02d:%02d"), ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
         break;
      case TS_DAY_AND_MONTH:
         _stprintf(pszBuffer, _T("%s/%d"), g_szMonthAbbr[ptm->tm_mon], ptm->tm_mday);
         break;
      case TS_MONTH:
         wcscpy(pszBuffer, g_szMonthAbbr[ptm->tm_mon]);
         break;
      default:
         *pszBuffer = 0;
         break;
   }
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
   pImageList->Add(theApp.LoadIcon(IDI_OUTSTANDING));    // For alarms
   pImageList->Add(theApp.LoadIcon(IDI_ACK));
   return pImageList;
}


//
// MulDiv() implementation
//

int MulDiv(int nNumber, int nNumerator, int nDenominator)
{
   INT64 qnResult;
   int nResult;

   qnResult = (INT64)nNumber * (INT64)nNumerator;
   nResult = (int)(qnResult / (INT64)nDenominator);
   if ((qnResult % nDenominator) > (nDenominator / 2))
      nResult++;
   return nResult;
}


//
// Get screen size
//

CSize GetScreenSize(void)
{
	CSize size;

	CDC *pDC = AfxGetMainWnd()->GetDC();
	size.cx = pDC->GetDeviceCaps(HORZRES);
	size.cy = pDC->GetDeviceCaps(VERTRES);
	AfxGetMainWnd()->ReleaseDC(pDC);

	return size;
}
