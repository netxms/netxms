/* 
** ODBC Database Driver
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
** $module: odbc.cpp
**
**/

#include "odbcdrv.h"


//
// Constants
//

#define DATA_BUFFER_SIZE      65536


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

extern "C" DB_HANDLE EXPORT DrvConnect(char *szHost, char *szLogin, char *szPassword, char *szDatabase)
{
   long iResult;
   ODBCDRV_CONN *pConn;

   // Allocate our connection structure
   pConn = (ODBCDRV_CONN *)malloc(sizeof(ODBCDRV_CONN));

   // Allocate environment
   iResult = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &pConn->sqlEnv);
	if ((iResult != SQL_SUCCESS) && (iResult != SQL_SUCCESS_WITH_INFO))
      goto connect_failure_0;

   // Set required ODBC version
	iResult = SQLSetEnvAttr(pConn->sqlEnv, SQL_ATTR_ODBC_VERSION, (void *)SQL_OV_ODBC3, 0);
	if ((iResult != SQL_SUCCESS) && (iResult != SQL_SUCCESS_WITH_INFO))
      goto connect_failure_1;

	// Allocate connection handle, set timeout
	iResult = SQLAllocHandle(SQL_HANDLE_DBC, pConn->sqlEnv, &pConn->sqlConn); 
	if ((iResult != SQL_SUCCESS) && (iResult != SQL_SUCCESS_WITH_INFO))
      goto connect_failure_1;
	SQLSetConnectAttr(pConn->sqlConn, SQL_LOGIN_TIMEOUT, (SQLPOINTER *)5, 0);

	// Connect to the datasource "web" 
	iResult = SQLConnect(pConn->sqlConn, (SQLCHAR *)szHost, SQL_NTS, 
                        (SQLCHAR *)szLogin, SQL_NTS, (SQLCHAR *)szPassword, SQL_NTS);
	if ((iResult != SQL_SUCCESS) && (iResult != SQL_SUCCESS_WITH_INFO))
      goto connect_failure_2;

   // Create mutex
   pConn->mutexQuery = MutexCreate();

   // Success
   return (DB_HANDLE)pConn;

   // Handle failures
connect_failure_2:
   SQLFreeHandle(SQL_HANDLE_DBC, pConn->sqlConn);

connect_failure_1:
   SQLFreeHandle(SQL_HANDLE_ENV, pConn->sqlEnv);

connect_failure_0:
   free(pConn);
   return NULL;
}


//
// Disconnect from database
//

extern "C" void EXPORT DrvDisconnect(ODBCDRV_CONN *pConn)
{
   MutexLock(pConn->mutexQuery, INFINITE);
   MutexUnlock(pConn->mutexQuery);
   SQLDisconnect(pConn->sqlConn);
   SQLFreeHandle(SQL_HANDLE_DBC, pConn->sqlConn);
   SQLFreeHandle(SQL_HANDLE_ENV, pConn->sqlEnv);
   MutexDestroy(pConn->mutexQuery);
   free(pConn);
}


//
// Perform non-SELECT query
//

extern "C" BOOL EXPORT DrvQuery(ODBCDRV_CONN *pConn, char *szQuery)
{
   BOOL bSuccess = FALSE;
   long iResult;

   MutexLock(pConn->mutexQuery, INFINITE);

   // Allocate statement handle
   iResult = SQLAllocHandle(SQL_HANDLE_STMT, pConn->sqlConn, &pConn->sqlStatement);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
   {
      // Execute statement
      iResult = SQLExecDirect(pConn->sqlStatement, (SQLCHAR *)szQuery, SQL_NTS);
	   if ((iResult == SQL_SUCCESS) || 
          (iResult == SQL_SUCCESS_WITH_INFO) || 
          (iResult == SQL_NO_DATA))
         bSuccess = TRUE;
      SQLFreeHandle(SQL_HANDLE_STMT, pConn->sqlStatement);
   }

   MutexUnlock(pConn->mutexQuery);
   return bSuccess;
}


//
// Perform SELECT query
//

extern "C" DB_RESULT EXPORT DrvSelect(ODBCDRV_CONN *pConn, char *szQuery)
{
   long i, iResult, iCurrValue;
   ODBCDRV_QUERY_RESULT *pResult = NULL;
   short wNumCols;
   SQLINTEGER iDataSize;
   BYTE *pDataBuffer;

   pDataBuffer = (BYTE *)malloc(DATA_BUFFER_SIZE);

   MutexLock(pConn->mutexQuery, INFINITE);

   // Allocate statement handle
   iResult = SQLAllocHandle(SQL_HANDLE_STMT, pConn->sqlConn, &pConn->sqlStatement);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
   {
      // Execute statement
      iResult = SQLExecDirect(pConn->sqlStatement, (SQLCHAR *)szQuery, SQL_NTS);
	   if ((iResult == SQL_SUCCESS) || 
          (iResult == SQL_SUCCESS_WITH_INFO))
      {
         // Allocate result buffer and determine column info
         pResult = (ODBCDRV_QUERY_RESULT *)malloc(sizeof(ODBCDRV_QUERY_RESULT));
         SQLNumResultCols(pConn->sqlStatement, &wNumCols);
         pResult->iNumCols = wNumCols;
         pResult->iNumRows = 0;
         pResult->pValues = NULL;
         iCurrValue = 0;

         // Fetch all data
         while(iResult = SQLFetch(pConn->sqlStatement), 
               (iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
         {
            pResult->iNumRows++;
            pResult->pValues = (char **)realloc(pResult->pValues, 
                        sizeof(char *) * (pResult->iNumRows * pResult->iNumCols));
            for(i = 1; i <= pResult->iNumCols; i++)
            {
               pDataBuffer[0] = 0;
               SQLGetData(pConn->sqlStatement, (short)i, SQL_C_CHAR, pDataBuffer, 
                          DATA_BUFFER_SIZE, &iDataSize);
               pResult->pValues[iCurrValue++] = strdup((const char *)pDataBuffer);
            }
         }
      }
      SQLFreeHandle(SQL_HANDLE_STMT, pConn->sqlStatement);
   }

   MutexUnlock(pConn->mutexQuery);
   free(pDataBuffer);
   return pResult;
}


//
// Get field value from result
//

extern "C" char EXPORT *DrvGetField(ODBCDRV_QUERY_RESULT *pResult, int iRow, int iColumn)
{
   char *pValue = NULL;

   if (pResult != NULL)
   {
      if ((iRow < pResult->iNumRows) && (iRow >= 0) &&
          (iColumn < pResult->iNumCols) && (iColumn >= 0))
         pValue = pResult->pValues[iRow * pResult->iNumCols + iColumn];
   }
   return pValue;
}


//
// Get number of rows in result
//

extern "C" int EXPORT DrvGetNumRows(ODBCDRV_QUERY_RESULT *pResult)
{
   return (pResult != NULL) ? pResult->iNumRows : 0;
}


//
// Free SELECT results
//

extern "C" void EXPORT DrvFreeResult(ODBCDRV_QUERY_RESULT *pResult)
{
   if (pResult != NULL)
   {
      int i, iNumValues;

      iNumValues = pResult->iNumCols * pResult->iNumRows;
      for(i = 0; i < iNumValues; i++)
         safe_free(pResult->pValues[i]);
      safe_free(pResult->pValues);
      free(pResult);
   }
}


//
// Perform asynchronous SELECT query
//

extern "C" DB_ASYNC_RESULT EXPORT DrvAsyncSelect(ODBCDRV_CONN *pConn, char *szQuery)
{
   ODBCDRV_ASYNC_QUERY_RESULT *pResult = NULL;
   long iResult;
   short wNumCols;

   MutexLock(pConn->mutexQuery, INFINITE);

   // Allocate statement handle
   iResult = SQLAllocHandle(SQL_HANDLE_STMT, pConn->sqlConn, &pConn->sqlStatement);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
   {
      // Execute statement
      iResult = SQLExecDirect(pConn->sqlStatement, (SQLCHAR *)szQuery, SQL_NTS);
	   if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
      {
         // Allocate result buffer and determine column info
         pResult = (ODBCDRV_ASYNC_QUERY_RESULT *)malloc(sizeof(ODBCDRV_ASYNC_QUERY_RESULT));
         SQLNumResultCols(pConn->sqlStatement, &wNumCols);
         pResult->iNumCols = wNumCols;
         pResult->pConn = pConn;
         pResult->bNoMoreRows = FALSE;
      }
      else
      {
         // Free statement handle if query failed
         SQLFreeHandle(SQL_HANDLE_STMT, pConn->sqlStatement);
      }
   }

   if (pResult == NULL) // Unlock mutex if query has failed
      MutexUnlock(pConn->mutexQuery);
   return pResult;
}


//
// Fetch next result line from asynchronous SELECT results
//

extern "C" BOOL EXPORT DrvFetch(ODBCDRV_ASYNC_QUERY_RESULT *pResult)
{
   BOOL bResult = FALSE;

   if (pResult != NULL)
   {
      long iResult;

      iResult = SQLFetch(pResult->pConn->sqlStatement);
      bResult = ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO));
      if (!bResult)
         pResult->bNoMoreRows = TRUE;
   }
   return bResult;
}


//
// Get field from current row in async query result
//

extern "C" char EXPORT *DrvGetFieldAsync(ODBCDRV_ASYNC_QUERY_RESULT *pResult, int iColumn, char *pBuffer, int iBufSize)
{
   SQLINTEGER iDataSize;
   long iResult;

   // Check if we have valid result handle
   if (pResult == NULL)
      return NULL;

   // Check if there are valid fetched row
   if (pResult->bNoMoreRows)
      return NULL;

   if ((iColumn >= 0) && (iColumn < pResult->iNumCols))
   {
      iResult = SQLGetData(pResult->pConn->sqlStatement, (short)iColumn + 1, SQL_C_CHAR,
                           pBuffer, iBufSize, &iDataSize);
      if ((iResult != SQL_SUCCESS) && (iResult != SQL_SUCCESS_WITH_INFO))
         pBuffer[0] = 0;
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

extern "C" void EXPORT DrvFreeAsyncResult(ODBCDRV_ASYNC_QUERY_RESULT *pResult)
{
   if (pResult != NULL)
   {
      SQLFreeHandle(SQL_HANDLE_STMT, pResult->pConn->sqlStatement);
      MutexUnlock(pResult->pConn->mutexQuery);
      free(pResult);
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
