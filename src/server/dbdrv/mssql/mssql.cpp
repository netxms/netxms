/* 
** Microsoft SQL Server Database Driver
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
** $module: mssql.cpp
**
**/

#include "mssqldrv.h"


//
// Static data
//

static MUTEX m_hQueryLock;


//
// Initialize driver
//

extern "C" BOOL EXPORT DrvInit(char *szCmdLine)
{
   m_hQueryLock = MutexCreate();
   return dbinit() != NULL;
}


//
// Unload handler
//

extern "C" void EXPORT DrvUnload(void)
{
   MutexDestroy(m_hQueryLock);
}


//
// Connect to database
//

extern "C" DB_HANDLE EXPORT DrvConnect(char *szHost, char *szLogin, char *szPassword, char *szDatabase)
{
   LOGINREC *loginrec;

   loginrec = dblogin();
   DBSETLUSER(loginrec, szLogin);
   DBSETLPWD(loginrec, szPassword);
   DBSETLAPP(loginrec, "NetXMS");
   return (DB_HANDLE)dbopen(loginrec, szHost);
}


//
// Disconnect from database
//

extern "C" void EXPORT DrvDisconnect(DB_HANDLE hConn)
{
   dbclose((PDBPROCESS)hConn);
}


//
// Perform non-SELECT query
//

extern "C" BOOL EXPORT DrvQuery(DB_HANDLE hConn, char *szQuery)
{
   BOOL bResult;

   MutexLock(m_hQueryLock, INFINITE);
   dbcmd((PDBPROCESS)hConn, szQuery);
   bResult = (dbsqlexec((PDBPROCESS)hConn) == SUCCEED);
   // Process query results if any
   if (bResult)
      if (dbresults((PDBPROCESS)hConn) == SUCCEED)
         while(dbnextrow((PDBPROCESS)hConn) != NO_MORE_ROWS);
   MutexUnlock(m_hQueryLock);
   return bResult;
}


//
// Perform SELECT query
//

extern "C" DB_RESULT EXPORT DrvSelect(DB_HANDLE hConn, char *szQuery)
{
   MSDB_QUERY_RESULT *pResult = NULL;
   int i, iCurrPos, iLen, *piColTypes;
   void *pData;

   MutexLock(m_hQueryLock, INFINITE);
   dbcmd((PDBPROCESS)hConn, szQuery);
   if (dbsqlexec((PDBPROCESS)hConn) == SUCCEED)
   {
      // Process query results
      if (dbresults((PDBPROCESS)hConn) == SUCCEED)
      {
         pResult = (MSDB_QUERY_RESULT *)malloc(sizeof(MSDB_QUERY_RESULT));
         pResult->iNumRows = 0;
         pResult->iNumCols = dbnumcols((PDBPROCESS)hConn);
         pResult->pValues = NULL;

         // Determine column types
         piColTypes = (int *)malloc(pResult->iNumCols * sizeof(int));
         for(i = 0; i < pResult->iNumCols; i++)
            piColTypes[i] = dbcoltype((PDBPROCESS)hConn, i + 1);

         // Retrieve data
         iCurrPos = 0;
         while(dbnextrow((PDBPROCESS)hConn) != NO_MORE_ROWS)
         {
            pResult->iNumRows++;
            pResult->pValues = (char **)realloc(pResult->pValues, 
                                                sizeof(char *) * pResult->iNumRows * pResult->iNumCols);
            for(i = 1; i <= pResult->iNumCols; i++, iCurrPos++)
            {
               pData = (void *)dbdata((PDBPROCESS)hConn, i);
               if (pData != NULL)
               {
                  switch(piColTypes[i - 1])
                  {
                     case SQLCHAR:
                     case SQLTEXT:
                     case SQLBINARY:
                        iLen = dbdatlen((PDBPROCESS)hConn, i);
                        pResult->pValues[iCurrPos] = (char *)malloc(iLen + 1);
                        if (iLen > 0)
                           memcpy(pResult->pValues[iCurrPos], (char *)pData, iLen);
                        pResult->pValues[iCurrPos][iLen] = 0;
                        break;
                     case SQLINT1:
                        pResult->pValues[iCurrPos] = (char *)malloc(4);
                        if (pData)
                           sprintf(pResult->pValues[iCurrPos], "%d", *((char *)pData));
                        break;
                     case SQLINT2:
                        pResult->pValues[iCurrPos] = (char *)malloc(8);
                        sprintf(pResult->pValues[iCurrPos], "%d", *((short *)pData));
                        break;
                     case SQLINT4:
                        pResult->pValues[iCurrPos] = (char *)malloc(16);
                        sprintf(pResult->pValues[iCurrPos], "%ld", *((long *)pData));
                        break;
                     case SQLFLT4:
                        pResult->pValues[iCurrPos] = (char *)malloc(32);
                        sprintf(pResult->pValues[iCurrPos], "%f", *((float *)pData));
                        break;
                     case SQLFLT8:
                        pResult->pValues[iCurrPos] = (char *)malloc(32);
                        sprintf(pResult->pValues[iCurrPos], "%f", *((double *)pData));
                        break;
                     default:    // Unknown data type
                        pResult->pValues[iCurrPos] = (char *)malloc(2);
                        pResult->pValues[iCurrPos][0] = 0;
                        break;
                  }
               }
               else
               {
                  pResult->pValues[iCurrPos] = (char *)malloc(2);
                  pResult->pValues[iCurrPos][0] = 0;
               }
            }
         }
      }
   }
   MutexUnlock(m_hQueryLock);
   return (DB_RESULT)pResult;
}


//
// Get field value from result
//

extern "C" char EXPORT *DrvGetField(DB_RESULT hResult, int iRow, int iColumn)
{
   if ((iRow < 0) || (iRow >= ((MSDB_QUERY_RESULT *)hResult)->iNumRows) ||
       (iColumn < 0) || (iColumn >= ((MSDB_QUERY_RESULT *)hResult)->iNumCols))
      return NULL;
   return ((MSDB_QUERY_RESULT *)hResult)->pValues[iRow * ((MSDB_QUERY_RESULT *)hResult)->iNumCols + iColumn];
}


//
// Get number of rows in result
//

extern "C" int EXPORT DrvGetNumRows(DB_RESULT hResult)
{
   return (hResult != NULL) ? ((MSDB_QUERY_RESULT *)hResult)->iNumRows : 0;
}


//
// Free SELECT results
//

extern "C" void EXPORT DrvFreeResult(DB_RESULT hResult)
{
   if (hResult != NULL)
   {
      int i, iNumValues;

      iNumValues = ((MSDB_QUERY_RESULT *)hResult)->iNumRows * ((MSDB_QUERY_RESULT *)hResult)->iNumCols;
      for(i = 0; i < iNumValues; i++)
         free(((MSDB_QUERY_RESULT *)hResult)->pValues[i]);
      if (((MSDB_QUERY_RESULT *)hResult)->pValues != NULL)
         free(((MSDB_QUERY_RESULT *)hResult)->pValues);
      free((MSDB_QUERY_RESULT *)hResult);
   }
}


//
// Perform asynchronous SELECT query
//

extern "C" DB_ASYNC_RESULT EXPORT DrvAsyncSelect(DB_HANDLE hConn, char *szQuery)
{
   MSDB_ASYNC_QUERY_RESULT *pResult = NULL;
   int i;

   MutexLock(m_hQueryLock, INFINITE);
   dbcmd((PDBPROCESS)hConn, szQuery);
   if (dbsqlexec((PDBPROCESS)hConn) == SUCCEED)
   {
      // Prepare query results for processing
      if (dbresults((PDBPROCESS)hConn) == SUCCEED)
      {
         // Fill in result information structure
         pResult = (MSDB_ASYNC_QUERY_RESULT *)malloc(sizeof(MSDB_ASYNC_QUERY_RESULT));
         pResult->hProcess = (PDBPROCESS)hConn;
         pResult->bNoMoreRows = FALSE;
         pResult->iNumCols = dbnumcols((PDBPROCESS)hConn);
         pResult->piColTypes = (int *)malloc(sizeof(int) * pResult->iNumCols);

         // Determine column types
         for(i = 0; i < pResult->iNumCols; i++)
            pResult->piColTypes[i] = dbcoltype((PDBPROCESS)hConn, i + 1);
      }
   }
   if (pResult == NULL)
      MutexUnlock(m_hQueryLock);
   return pResult;
}


//
// Fetch next result line from asynchronous SELECT results
//

extern "C" BOOL EXPORT DrvFetch(DB_ASYNC_RESULT hResult)
{
   BOOL bResult = TRUE;
   
   if (hResult == NULL)
   {
      bResult = FALSE;
   }
   else
   {
      // Try to fetch next row from server
      if (dbnextrow(((MSDB_ASYNC_QUERY_RESULT *)hResult)->hProcess) == NO_MORE_ROWS)
      {
         ((MSDB_ASYNC_QUERY_RESULT *)hResult)->bNoMoreRows = TRUE;
         bResult = FALSE;
         MutexUnlock(m_hQueryLock);
      }
   }
   return bResult;
}


//
// Get field from current row in async query result
//

extern "C" char EXPORT *DrvGetFieldAsync(DB_ASYNC_RESULT hResult, int iColumn, char *pBuffer, int iBufSize)
{
   void *pData;
   int iLen;

   // Check if we have valid result handle
   if (hResult == NULL)
      return NULL;

   // Check if there are valid fetched row
   if (((MSDB_ASYNC_QUERY_RESULT *)hResult)->bNoMoreRows)
      return NULL;

   // Now get column data
   pData = (void *)dbdata(((MSDB_ASYNC_QUERY_RESULT *)hResult)->hProcess, iColumn + 1);
   if (pData != NULL)
   {
      switch(((MSDB_ASYNC_QUERY_RESULT *)hResult)->piColTypes[iColumn])
      {
         case SQLCHAR:
         case SQLTEXT:
         case SQLBINARY:
            iLen = min(dbdatlen(((MSDB_ASYNC_QUERY_RESULT *)hResult)->hProcess, iColumn + 1), iBufSize - 1);
            if (iLen > 0)
               memcpy(pBuffer, (char *)pData, iLen);
            pBuffer[iLen] = 0;
            break;
         case SQLINT1:
            sprintf(pBuffer, "%d", *((char *)pData));
            break;
         case SQLINT2:
            sprintf(pBuffer, "%d", *((short *)pData));
            break;
         case SQLINT4:
            sprintf(pBuffer, "%ld", *((long *)pData));
            break;
         case SQLFLT4:
            sprintf(pBuffer, "%f", *((float *)pData));
            break;
         case SQLFLT8:
            sprintf(pBuffer, "%f", *((double *)pData));
            break;
         default:    // Unknown data type
            pBuffer[0] = 0;
            break;
      }
   }
   else
   {
      pBuffer[0] = 0;
   }
   
   return pBuffer;
}


//
// Destroy result of async query
//

extern "C" void EXPORT DrvFreeAsyncResult(DB_ASYNC_RESULT hResult)
{
   if (hResult != NULL)
   {
      // Check if all result rows fetchef
      if (!((MSDB_ASYNC_QUERY_RESULT *)hResult)->bNoMoreRows)
      {
         // Fetch remaining rows
         while(dbnextrow(((MSDB_ASYNC_QUERY_RESULT *)hResult)->hProcess) != NO_MORE_ROWS);

         // Now we are ready for next query, so unlock query mutex
         MutexUnlock(m_hQueryLock);
      }

      // Free allocated memory
      if (((MSDB_ASYNC_QUERY_RESULT *)hResult)->piColTypes != NULL)
         free(((MSDB_ASYNC_QUERY_RESULT *)hResult)->piColTypes);
      free(hResult);
   }
}


//
// DLL Entry point
//

#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   return TRUE;
}

#endif
