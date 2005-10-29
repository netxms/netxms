/* 
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004, 2005 Victor Kirhenshtein
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
** $module: init.cpp
**
**/

#include "nxdbmgr.h"


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

static BOOL ExecSQLBatch(TCHAR *pszFile)
{
   BYTE *pBatch, *pQuery, *pNext;
   DWORD dwSize;
   BOOL bResult = FALSE;

   pBatch = LoadFile(pszFile, &dwSize);
   if (pBatch != NULL)
   {
      for(pQuery = pBatch; pQuery < pBatch + dwSize; pQuery = pNext)
      {
         pNext = FindEndOfQuery(pQuery, pBatch + dwSize);
         if (!IsEmptyQuery((char *)pQuery))
         {
            bResult = SQLQuery((char *)pQuery);
            if (!bResult)
               pNext = pBatch + dwSize;
         }
      }
      free(pBatch);
   }
   else
   {
      _tprintf(_T("ERROR: Cannot load SQL command file %s\n"), pszFile);
   }
   return bResult;
}


//
// Initialize database
//

void InitDatabase(TCHAR *pszInitFile)
{
   _tprintf(_T("Initializing database...\n"));
   if (ExecSQLBatch(pszInitFile))
      _tprintf(_T("Database initialized successfully\n"));
   else
      _tprintf(_T("Database initialization failed\n"));
}
