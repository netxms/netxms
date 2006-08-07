/* 
** MySQL Database Driver
** Copyright (C) 2003, 2004, 2005 Victor Kirhenshtein
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
** $module: mysql.cpp
**
**/

#include "mysqldrv.h"


//
// Initialize driver
//

extern "C" BOOL EXPORT DrvInit(char *szCmdLine)
{
   return TRUE;
}


//
// Unload handler
//

extern "C" void EXPORT DrvUnload(void)
{
}


//
// Connect to database
//

extern "C" DB_CONNECTION EXPORT DrvConnect(char *szHost, char *szLogin, char *szPassword, char *szDatabase)
{
   MYSQL *pMySQL;
   MYSQL_CONN *pConn;

   pMySQL = mysql_init(NULL);
   if (pMySQL == NULL)
      return NULL;

   if (!mysql_real_connect(
				pMySQL, // MYSQL *
				szHost, // host
				szLogin[0] == 0 ? NULL : szLogin, // user
				(szPassword[0] == 0 || szLogin[0] == 0) ? NULL : szPassword, // pass
				szDatabase, // DB Name
				0, // use default port
				NULL, // char * - unix socket
				0 // flags
			))
   {
      mysql_close(pMySQL);
      return NULL;
   }

   pConn = (MYSQL_CONN *)malloc(sizeof(MYSQL_CONN));
   pConn->pMySQL = pMySQL;
   pConn->mutexQueryLock = MutexCreate();

   return (DB_CONNECTION)pConn;
}


//
// Disconnect from database
//

extern "C" void EXPORT DrvDisconnect(MYSQL_CONN *pConn)
{
   if (pConn != NULL)
   {
      mysql_close(pConn->pMySQL);
      MutexDestroy(pConn->mutexQueryLock);
		free(pConn);
   }
}


//
// Perform non-SELECT query
//

extern "C" BOOL EXPORT DrvQuery(MYSQL_CONN *pConn, char *szQuery)
{
   BOOL bResult;

   MutexLock(pConn->mutexQueryLock, INFINITE);
   bResult = (mysql_query(pConn->pMySQL, szQuery) == 0);
   MutexUnlock(pConn->mutexQueryLock);
   return bResult;
}


//
// Perform SELECT query
//

extern "C" DB_RESULT EXPORT DrvSelect(MYSQL_CONN *pConn, char *szQuery)
{
   DB_RESULT pResult = NULL;

   MutexLock(pConn->mutexQueryLock, INFINITE);
   if (mysql_query(pConn->pMySQL, szQuery) == 0)
      pResult = (DB_RESULT)mysql_store_result(pConn->pMySQL);
   MutexUnlock(pConn->mutexQueryLock);
   return pResult;
}


//
// Get field value from result
//

extern "C" char EXPORT *DrvGetField(DB_RESULT hResult, int iRow, int iColumn)
{
   MYSQL_ROW row;

   mysql_data_seek((MYSQL_RES *)hResult, iRow);
   row = mysql_fetch_row((MYSQL_RES *)hResult);
   return (row == NULL) ? NULL : row[iColumn];
}


//
// Get number of rows in result
//

extern "C" int EXPORT DrvGetNumRows(DB_RESULT hResult)
{
   if (hResult == 0)
      return 0;
   return (int)mysql_num_rows((MYSQL_RES *)hResult);
}


//
// Free SELECT results
//

extern "C" void EXPORT DrvFreeResult(DB_RESULT hResult)
{
   mysql_free_result((MYSQL_RES *)hResult);
}


//
// Perform asynchronous SELECT query
//

extern "C" DB_ASYNC_RESULT EXPORT DrvAsyncSelect(MYSQL_CONN *pConn, char *szQuery)
{
   MYSQL_ASYNC_RESULT *pResult = NULL;

   MutexLock(pConn->mutexQueryLock, INFINITE);
   if (mysql_query(pConn->pMySQL, szQuery) == 0)
   {
      pResult = (MYSQL_ASYNC_RESULT *)malloc(sizeof(MYSQL_ASYNC_RESULT));
      pResult->pConn = pConn;
      pResult->pHandle = mysql_use_result(pConn->pMySQL);
      if (pResult->pHandle != NULL)
      {
         pResult->bNoMoreRows = FALSE;
         pResult->iNumCols = mysql_num_fields(pResult->pHandle);
         pResult->pCurrRow = NULL;
         pResult->pulColLengths = (unsigned long *)malloc(sizeof(unsigned long) * pResult->iNumCols);
      }
      else
      {
         free(pResult);
         pResult = NULL;
      }
   }
   if (pResult == NULL)
      MutexUnlock(pConn->mutexQueryLock);
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
      ((MYSQL_ASYNC_RESULT *)hResult)->pCurrRow = mysql_fetch_row(((MYSQL_ASYNC_RESULT *)hResult)->pHandle);
      if (((MYSQL_ASYNC_RESULT *)hResult)->pCurrRow == NULL)
      {
         ((MYSQL_ASYNC_RESULT *)hResult)->bNoMoreRows = TRUE;
         bResult = FALSE;
         MutexUnlock(((MYSQL_ASYNC_RESULT *)hResult)->pConn->mutexQueryLock);
      }
      else
      {
         unsigned long *pLen;

         // Get column lengths for current row
         pLen = mysql_fetch_lengths(((MYSQL_ASYNC_RESULT *)hResult)->pHandle);
         if (pLen != NULL)
         {
            memcpy(((MYSQL_ASYNC_RESULT *)hResult)->pulColLengths, pLen, 
                   sizeof(unsigned long) * ((MYSQL_ASYNC_RESULT *)hResult)->iNumCols);
         }
         else
         {
            memset(((MYSQL_ASYNC_RESULT *)hResult)->pulColLengths, 0, 
                   sizeof(unsigned long) * ((MYSQL_ASYNC_RESULT *)hResult)->iNumCols);
         }
      }
   }
   return bResult;
}


//
// Get field from current row in async query result
//

extern "C" char EXPORT *DrvGetFieldAsync(DB_ASYNC_RESULT hResult, int iColumn,
                                         char *pBuffer, int iBufSize)
{
   int iLen;

   // Check if we have valid result handle
   if (hResult == NULL)
      return NULL;

   // Check if there are valid fetched row
   if ((((MYSQL_ASYNC_RESULT *)hResult)->bNoMoreRows) ||
       (((MYSQL_ASYNC_RESULT *)hResult)->pCurrRow == NULL))
      return NULL;

   // Check if column number is valid
   if ((iColumn < 0) || (iColumn >= ((MYSQL_ASYNC_RESULT *)hResult)->iNumCols))
      return NULL;

   // Now get column data
   iLen = min((int)((MYSQL_ASYNC_RESULT *)hResult)->pulColLengths[iColumn], iBufSize - 1);
   if (iLen > 0)
      memcpy(pBuffer, ((MYSQL_ASYNC_RESULT *)hResult)->pCurrRow[iColumn], iLen);
   pBuffer[iLen] = 0;
   
   return pBuffer;
}


//
// Destroy result of async query
//

extern "C" void EXPORT DrvFreeAsyncResult(DB_ASYNC_RESULT hResult)
{
   if (hResult != NULL)
   {
      // Check if all result rows fetched
      if (!((MYSQL_ASYNC_RESULT *)hResult)->bNoMoreRows)
      {
         // Fetch remaining rows
         while(mysql_fetch_row(((MYSQL_ASYNC_RESULT *)hResult)->pHandle) != NULL);

         // Now we are ready for next query, so unlock query mutex
         MutexUnlock(((MYSQL_ASYNC_RESULT *)hResult)->pConn->mutexQueryLock);
      }

      // Free allocated memory
      mysql_free_result(((MYSQL_ASYNC_RESULT *)hResult)->pHandle);
      safe_free(((MYSQL_ASYNC_RESULT *)hResult)->pulColLengths);
      free(hResult);
   }
}


//
// Begin transaction
//

extern "C" BOOL EXPORT DrvBegin(MYSQL_CONN *pConn)
{
   return DrvQuery(pConn, "BEGIN");
}


//
// Commit transaction
//

extern "C" BOOL EXPORT DrvCommit(MYSQL_CONN *pConn)
{
   return DrvQuery(pConn, "COMMIT");
}


//
// Rollback transaction
//

extern "C" BOOL EXPORT DrvRollback(MYSQL_CONN *pConn)
{
   return DrvQuery(pConn, "ROLLBACK");
}


//
// DLL Entry point
//

#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif
