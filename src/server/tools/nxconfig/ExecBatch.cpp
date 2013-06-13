/* 
** NetXMS - Network Management System
** Server Configurator for Windows
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
** $module: ExecBatch.cpp
** Execute SQL batch file
**
**/

#include "stdafx.h"
#include "nxconfig.h"


//
// Check if query is empty
//

static BOOL IsEmptyQuery(char *pszQuery)
{
   char *ptr;

   for(ptr = pszQuery; *ptr != NULL; ptr++)
      if ((*ptr != ' ') && (*ptr != '\t') && (*ptr != '\r') && (*ptr != '\n'))
         return FALSE;
   return TRUE;
}


//
// Find end of query in batch
//

static BYTE *FindEndOfQuery(BYTE *pStart, BYTE *pBatchEnd)
{
   BYTE *ptr;
   int iState;

   for(ptr = pStart, iState = 0; (ptr < pBatchEnd) && (iState != -1); ptr++)
   {
      switch(iState)
      {
         case 0:
            if (*ptr == '\'')
               iState = 1;
            if (*ptr == ';')
               iState = -1;
            break;
         case 1:
            if (*ptr == '\'')
               iState = 0;
            break;
      }
   }

   *(ptr - 1) = 0;
   return ptr + 1;
}


//
// Execute SQL batch file
//

BOOL ExecSQLBatch(DB_HANDLE hConn, TCHAR *pszFile)
{
   BYTE *pBatch, *pQuery, *pNext;
   UINT32 dwSize;
   BOOL bResult = FALSE;

   pBatch = LoadFile(pszFile, &dwSize);
   if (pBatch != NULL)
   {
      for(pQuery = pBatch; pQuery < pBatch + dwSize; pQuery = pNext)
      {
         pNext = FindEndOfQuery(pQuery, pBatch + dwSize);
         if (!IsEmptyQuery((char *)pQuery))
         {
#ifdef UNICODE
				WCHAR *wquery = WideStringFromMBString((char *)pQuery);
            bResult = DBQuery(hConn, wquery);
				free(wquery);
#else
            bResult = DBQuery(hConn, (char *)pQuery);
#endif
            if (!bResult)
            {
               _sntprintf(g_szWizardErrorText, MAX_ERROR_TEXT, _T("SQL query failed:\n%hs"), pQuery);
               break;
            }
         }
      }
      free(pBatch);
   }
   else
   {
      _sntprintf(g_szWizardErrorText, MAX_ERROR_TEXT, _T("Cannot load SQL command file %s"), pszFile);
   }

   return bResult;
}
