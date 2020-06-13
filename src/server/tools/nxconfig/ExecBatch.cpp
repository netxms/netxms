/* 
** NetXMS - Network Management System
** Server Configurator for Windows
** Copyright (C) 2005-2020 Victor Kirhenshtein
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

/**
 * Check if query is empty
 */
static bool IsEmptyQuery(const char *pszQuery)
{
   for (const char *ptr = pszQuery; *ptr != 0; ptr++)
      if ((*ptr != ' ') && (*ptr != '\t') && (*ptr != '\r') && (*ptr != '\n'))
         return false;
   return true;
}

/**
 * Find end of query in batch
 */
static BYTE *FindEndOfQuery(BYTE *pStart, BYTE *pBatchEnd)
{
   BYTE *ptr;
   int iState;
   bool proc = false;
   bool procEnd = false;

   for (ptr = pStart, iState = 0; (ptr < pBatchEnd) && (iState != -1); ptr++)
   {
      switch (iState)
      {
      case 0:
         if (*ptr == '\'')
         {
            iState = 1;
         }
         else if ((*ptr == ';') && !proc && !procEnd)
         {
            iState = -1;
         }
         else if ((*ptr == '/') && procEnd)
         {
            procEnd = false;
            iState = -1;
         }
         else if ((*ptr == 'C') || (*ptr == 'c'))
         {
            if (!strnicmp((char *)ptr, "CREATE FUNCTION", 15) ||
               !strnicmp((char *)ptr, "CREATE OR REPLACE FUNCTION", 26) ||
               !strnicmp((char *)ptr, "CREATE PROCEDURE", 16) ||
               !strnicmp((char *)ptr, "CREATE OR REPLACE PROCEDURE", 27))
            {
               proc = true;
            }
         }
         else if (proc && ((*ptr == 'E') || (*ptr == 'e')))
         {
            if (!strnicmp((char *)ptr, "END", 3))
            {
               proc = false;
               procEnd = true;
            }
         }
         else if ((*ptr == '\r') || (*ptr == '\n'))
         {
            // CR/LF should be replaced with spaces, otherwise at least
            // Oracle will fail on CREATE FUNCTION / CREATE PROCEDURE
            *ptr = ' ';
         }
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

/**
 * Execute SQL batch file
 */
BOOL ExecSQLBatch(DB_HANDLE hConn, TCHAR *pszFile)
{
   BYTE *pQuery, *pNext;
   BOOL bResult = FALSE;

   size_t size;
   BYTE *pBatch = LoadFile(pszFile, &size);
   if (pBatch != nullptr)
   {
      for(pQuery = pBatch; pQuery < pBatch + size; pQuery = pNext)
      {
         pNext = FindEndOfQuery(pQuery, pBatch + size);
         if (!IsEmptyQuery((char *)pQuery))
         {
            TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
#ifdef UNICODE
				WCHAR *wquery = WideStringFromUTF8String((char *)pQuery);
            bResult = DBQueryEx(hConn, wquery, errorText);
				MemFree(wquery);
#else
            bResult = DBQueryEx(hConn, (char *)pQuery, errorText);
#endif
            if (!bResult)
            {
               _sntprintf(g_szWizardErrorText, MAX_ERROR_TEXT, _T("SQL query failed:\n%hs\n%s"), pQuery, errorText);
               break;
            }
         }
      }
      MemFree(pBatch);
   }
   else
   {
      _sntprintf(g_szWizardErrorText, MAX_ERROR_TEXT, _T("Cannot load SQL command file %s"), pszFile);
   }

   return bResult;
}
