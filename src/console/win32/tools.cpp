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
   size.cx = rect.right - rect.left + 1;
   size.cy = rect.bottom - rect.top + 1;
   return size;
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
// Create SNMP MIB tree
//

BOOL CreateMIBTree(void)
{
   char szBuffer[MAX_PATH + 32];

   init_mib();
   netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_SAVE_MIB_DESCRS, TRUE);
   strcpy(szBuffer, g_szWorkDir);
   strcat(szBuffer, WORKDIR_MIBCACHE);
   add_mibdir(szBuffer);
   read_all_mibs();
   return TRUE;
}


//
// Destroy previously created MIB tree
//

void DestroyMIBTree(void)
{
   //shutdown_mib();
}


//
// Build full OID for MIB tree leaf
//

void BuildOIDString(struct tree *pNode, char *pszBuffer)
{
   DWORD dwPos, dwSubIdList[MAX_OID_LEN];
   int iBufPos = 0;
   struct tree *pCurr;

   if (!strcmp(pNode->label, "[root]"))
   {
      pszBuffer[0] = 0;
   }
   else
   {
      for(dwPos = 0, pCurr = pNode; pCurr != NULL; pCurr = pCurr->parent)
         dwSubIdList[dwPos++] = pCurr->subid;
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

char *BuildSymbolicOIDString(struct tree *pNode)
{
   DWORD dwPos, dwSize;
   char *pszBuffer, *pszSubIdList[MAX_OID_LEN];
   int iBufPos = 0;
   struct tree *pCurr;

   if (!strcmp(pNode->label, "[root]"))
   {
      pszBuffer = (char *)malloc(4);
      pszBuffer[0] = 0;
   }
   else
   {
      for(dwPos = 0, dwSize = 0, pCurr = pNode; pCurr != NULL; pCurr = pCurr->parent)
      {
         pszSubIdList[dwPos++] = pCurr->label;
         dwSize += strlen(pCurr->label) + 1;
      }
      pszBuffer = (char *)malloc(dwSize + 1);
      while(dwPos > 0)
      {
         sprintf(&pszBuffer[iBufPos], ".%s", pszSubIdList[--dwPos]);
         iBufPos = strlen(pszBuffer);
      }
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
