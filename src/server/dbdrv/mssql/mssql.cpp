/* 
** Microsoft SQL Server Database Driver
** Copyright (C) 2004, 2005, 2006 Victor Kirhenshtein
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
** File: mssql.cpp
**
**/

#include "mssqldrv.h"


//
// Constants
//

#define CONNECT_TIMEOUT    30


//
// DB library error handler
//

static int ErrorHandler(PDBPROCESS hProcess, int severity, int dberr,
                        int oserr, const char *dberrstr, const char *oserrstr)
{
   MSDB_CONN *pConn;

   if (hProcess != NULL)
   {
      pConn = (MSDB_CONN *)dbgetuserdata(hProcess);
      if (pConn != NULL)
      {
			nx_strncpy(pConn->szErrorText, dberrstr, DBDRV_MAX_ERROR_TEXT);
			RemoveTrailingCRLF(pConn->szErrorText);
         if (dbdead(hProcess))
            pConn->bProcessDead = TRUE;
      }
   }
   return INT_CANCEL;
}


//
// Re-establish connection to server
//

static BOOL Reconnect(MSDB_CONN *pConn)
{
   LOGINREC *loginrec;
   PDBPROCESS hProcess;
   BOOL bResult = FALSE;

   loginrec = dblogin();
   if (!strcmp(pConn->szLogin, "*"))
   {
      DBSETLSECURE(loginrec);
   }
   else
   {
      DBSETLUSER(loginrec, pConn->szLogin);
      DBSETLPWD(loginrec, pConn->szPassword);
   }
   DBSETLAPP(loginrec, "NetXMS");
   DBSETLTIME(loginrec, CONNECT_TIMEOUT);
   hProcess = dbopen(loginrec, pConn->szHost);

   if ((hProcess != NULL) && (pConn->szDatabase[0] != 0))
   {
      dbsetuserdata(hProcess, NULL);
      if (dbuse(hProcess, pConn->szDatabase) != SUCCEED)
      {
         dbclose(hProcess);
         hProcess = NULL;
      }
   }

   if (hProcess != NULL)
   {
      dbclose(pConn->hProcess);
      pConn->hProcess = hProcess;
      pConn->bProcessDead = FALSE;
      dbsetuserdata(hProcess, pConn);
      bResult = TRUE;
   }

   return bResult;
}


//
// API version
//

extern "C" int EXPORT drvAPIVersion = DBDRV_API_VERSION;


//
// Initialize driver
//

extern "C" BOOL EXPORT DrvInit(char *szCmdLine)
{
   BOOL bResult = FALSE;

   if (dbinit() != NULL)
   {
      dberrhandle(ErrorHandler);
      bResult = TRUE;
   }
   return bResult;
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

extern "C" DB_CONNECTION EXPORT DrvConnect(char *szHost, char *szLogin,
                                           char *szPassword, char *szDatabase)
{
   LOGINREC *loginrec;
   MSDB_CONN *pConn = NULL;
   PDBPROCESS hProcess;

   loginrec = dblogin();
   if (!strcmp(szLogin, "*"))
   {
      DBSETLSECURE(loginrec);
   }
   else
   {
      DBSETLUSER(loginrec, szLogin);
      DBSETLPWD(loginrec, szPassword);
   }
   DBSETLAPP(loginrec, "NetXMS");
   DBSETLTIME(loginrec, CONNECT_TIMEOUT);
   hProcess = dbopen(loginrec, szHost);

   if (hProcess != NULL)
   {
      dbsetuserdata(hProcess, NULL);

      // Change to specified database
      if (szDatabase != NULL)
      {
         if (dbuse(hProcess, szDatabase) != SUCCEED)
         {
            dbclose(hProcess);
            hProcess = NULL;
         }
      }
      
      if (hProcess != NULL)
      {
         pConn = (MSDB_CONN *)malloc(sizeof(MSDB_CONN));
         pConn->hProcess = hProcess;
         pConn->mutexQueryLock = MutexCreate();
         pConn->bProcessDead = FALSE;
         nx_strncpy(pConn->szHost, szHost, MAX_CONN_STRING);
         nx_strncpy(pConn->szLogin, szLogin, MAX_CONN_STRING);
         nx_strncpy(pConn->szPassword, szPassword, MAX_CONN_STRING);
         nx_strncpy(pConn->szDatabase, CHECK_NULL_EX(szDatabase), MAX_CONN_STRING);
			pConn->szErrorText[0] = 0;

         dbsetuserdata(hProcess, pConn);
      }
   }

   return (DB_CONNECTION)pConn;
}


//
// Disconnect from database
//

extern "C" void EXPORT DrvDisconnect(MSDB_CONN *pConn)
{
   if (pConn != NULL)
   {
      dbclose(pConn->hProcess);
      MutexDestroy(pConn->mutexQueryLock);
      free(pConn);
   }
}


//
// Execute query
//

static BOOL ExecuteQuery(MSDB_CONN *pConn, char *pszQuery, TCHAR *errorText)
{
   BOOL bResult;

   dbcmd(pConn->hProcess, pszQuery);
   if (dbsqlexec(pConn->hProcess) == SUCCEED)
   {
      bResult = TRUE;
   }
   else
   {
      if (pConn->bProcessDead)
      {
         if (Reconnect(pConn))
         {
            bResult = (dbsqlexec(pConn->hProcess) == SUCCEED);
         }
         else
         {
            bResult = FALSE;
         }
      }
      else
      {
         bResult = FALSE;
      }
   }

	if (errorText != NULL)
	{
		if (bResult)
		{
			*errorText = 0;
		}
		else
		{
			nx_strncpy(errorText, pConn->szErrorText, DBDRV_MAX_ERROR_TEXT);
		}
	}

   return bResult;
}


//
// Perform non-SELECT query
//

extern "C" DWORD EXPORT DrvQuery(MSDB_CONN *pConn, WCHAR *pwszQuery, TCHAR *errorText)
{
   DWORD dwError;
   char *pszQueryUTF8;

   pszQueryUTF8 = UTF8StringFromWideString(pwszQuery);
   MutexLock(pConn->mutexQueryLock, INFINITE);
   
   if (ExecuteQuery(pConn, pszQueryUTF8, errorText))
   {
      if (dbresults(pConn->hProcess) == SUCCEED)
         while(dbnextrow(pConn->hProcess) != NO_MORE_ROWS);
      dwError = DBERR_SUCCESS;
   }
   else
   {
      dwError = pConn->bProcessDead ? DBERR_CONNECTION_LOST : DBERR_OTHER_ERROR;
   }
   MutexUnlock(pConn->mutexQueryLock);
   free(pszQueryUTF8);
   return dwError;
}


//
// Perform SELECT query
//

extern "C" DB_RESULT EXPORT DrvSelect(MSDB_CONN *pConn, WCHAR *pwszQuery, DWORD *pdwError, TCHAR *errorText)
{
   MSDB_QUERY_RESULT *pResult = NULL;
   int i, iCurrPos, iLen, *piColTypes;
   void *pData;
   char *pszQueryUTF8;

   pszQueryUTF8 = UTF8StringFromWideString(pwszQuery);
   MutexLock(pConn->mutexQueryLock, INFINITE);

   if (ExecuteQuery(pConn, pszQueryUTF8, errorText))
   {
      // Process query results
      if (dbresults(pConn->hProcess) == SUCCEED)
      {
         pResult = (MSDB_QUERY_RESULT *)malloc(sizeof(MSDB_QUERY_RESULT));
         pResult->iNumRows = 0;
         pResult->iNumCols = dbnumcols(pConn->hProcess);
         pResult->pValues = NULL;

         // Determine column types
         piColTypes = (int *)malloc(pResult->iNumCols * sizeof(int));
         for(i = 0; i < pResult->iNumCols; i++)
            piColTypes[i] = dbcoltype(pConn->hProcess, i + 1);

         // Retrieve data
         iCurrPos = 0;
         while(dbnextrow(pConn->hProcess) != NO_MORE_ROWS)
         {
            pResult->iNumRows++;
            pResult->pValues = (char **)realloc(pResult->pValues, 
                                                sizeof(char *) * pResult->iNumRows * pResult->iNumCols);
            for(i = 1; i <= pResult->iNumCols; i++, iCurrPos++)
            {
               pData = (void *)dbdata(pConn->hProcess, i);
               if (pData != NULL)
               {
                  switch(piColTypes[i - 1])
                  {
                     case SQLCHAR:
                     case SQLTEXT:
                     case SQLBINARY:
                        iLen = dbdatlen(pConn->hProcess, i);
                        pResult->pValues[iCurrPos] = (char *)malloc(iLen + 1);
                        if (iLen > 0)
                           memcpy(pResult->pValues[iCurrPos], (char *)pData, iLen);
                        pResult->pValues[iCurrPos][iLen] = 0;
                        break;
                     case SQLINT1:
                        pResult->pValues[iCurrPos] = (char *)malloc(4);
                        if (pData)
                           _snprintf_s(pResult->pValues[iCurrPos], 4, _TRUNCATE, "%d", *((char *)pData));
                        break;
                     case SQLINT2:
                        pResult->pValues[iCurrPos] = (char *)malloc(8);
                        _snprintf_s(pResult->pValues[iCurrPos], 8, _TRUNCATE, "%d", *((short *)pData));
                        break;
                     case SQLINT4:
                        pResult->pValues[iCurrPos] = (char *)malloc(16);
                        _snprintf_s(pResult->pValues[iCurrPos], 16, _TRUNCATE, "%d", *((LONG *)pData));
                        break;
                     case SQLFLT4:
                        pResult->pValues[iCurrPos] = (char *)malloc(32);
                        _snprintf_s(pResult->pValues[iCurrPos], 32, _TRUNCATE, "%f", *((float *)pData));
                        break;
                     case SQLFLT8:
                        pResult->pValues[iCurrPos] = (char *)malloc(32);
                        _snprintf_s(pResult->pValues[iCurrPos], 32, _TRUNCATE, "%f", *((double *)pData));
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

   if (pResult != NULL)
   {
      *pdwError = DBERR_SUCCESS;
   }
   else
   {
      *pdwError = pConn->bProcessDead ? DBERR_CONNECTION_LOST : DBERR_OTHER_ERROR;
   }

   MutexUnlock(pConn->mutexQueryLock);
   free(pszQueryUTF8);
   return (DB_RESULT)pResult;
}


//
// Get field length from result
//

extern "C" LONG EXPORT DrvGetFieldLength(DB_RESULT hResult, int iRow, int iColumn)
{
   if ((iRow < 0) || (iRow >= ((MSDB_QUERY_RESULT *)hResult)->iNumRows) ||
       (iColumn < 0) || (iColumn >= ((MSDB_QUERY_RESULT *)hResult)->iNumCols))
      return -1;
   return strlen(((MSDB_QUERY_RESULT *)hResult)->pValues[iRow * ((MSDB_QUERY_RESULT *)hResult)->iNumCols + iColumn]);
}


//
// Get field value from result
//

extern "C" WCHAR EXPORT *DrvGetField(DB_RESULT hResult, int iRow, int iColumn,
                                     WCHAR *pBuffer, int nBufSize)
{
   if ((iRow < 0) || (iRow >= ((MSDB_QUERY_RESULT *)hResult)->iNumRows) ||
       (iColumn < 0) || (iColumn >= ((MSDB_QUERY_RESULT *)hResult)->iNumCols))
      return NULL;
   MultiByteToWideChar(CP_ACP, 0, ((MSDB_QUERY_RESULT *)hResult)->pValues[iRow * ((MSDB_QUERY_RESULT *)hResult)->iNumCols + iColumn],
                       -1, pBuffer, nBufSize);
   pBuffer[nBufSize - 1] = 0;
   return pBuffer;
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

extern "C" DB_ASYNC_RESULT EXPORT DrvAsyncSelect(MSDB_CONN *pConn, WCHAR *pwszQuery,
                                                 DWORD *pdwError, TCHAR *errorText)
{
   MSDB_ASYNC_QUERY_RESULT *pResult = NULL;
   char *pszQueryUTF8;
   int i;

   pszQueryUTF8 = UTF8StringFromWideString(pwszQuery);
   MutexLock(pConn->mutexQueryLock, INFINITE);
   
   if (ExecuteQuery(pConn, pszQueryUTF8, errorText))
   {
      // Prepare query results for processing
      if (dbresults(pConn->hProcess) == SUCCEED)
      {
         // Fill in result information structure
         pResult = (MSDB_ASYNC_QUERY_RESULT *)malloc(sizeof(MSDB_ASYNC_QUERY_RESULT));
         pResult->pConnection = pConn;
         pResult->bNoMoreRows = FALSE;
         pResult->iNumCols = dbnumcols(pConn->hProcess);
         pResult->piColTypes = (int *)malloc(sizeof(int) * pResult->iNumCols);

         // Determine column types
         for(i = 0; i < pResult->iNumCols; i++)
            pResult->piColTypes[i] = dbcoltype(pConn->hProcess, i + 1);
      }
   }

   if (pResult != NULL)
   {
      *pdwError = DBERR_SUCCESS;
   }
   else
   {
      *pdwError = pConn->bProcessDead ? DBERR_CONNECTION_LOST : DBERR_OTHER_ERROR;
      MutexUnlock(pConn->mutexQueryLock);
   }
   free(pszQueryUTF8);
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
      if (dbnextrow(((MSDB_ASYNC_QUERY_RESULT *)hResult)->pConnection->hProcess) == NO_MORE_ROWS)
      {
         ((MSDB_ASYNC_QUERY_RESULT *)hResult)->bNoMoreRows = TRUE;
         bResult = FALSE;
         MutexUnlock(((MSDB_ASYNC_QUERY_RESULT *)hResult)->pConnection->mutexQueryLock);
      }
   }
   return bResult;
}


//
// Get field from current row in async query result
//

extern "C" WCHAR EXPORT *DrvGetFieldAsync(DB_ASYNC_RESULT hResult, int iColumn,
                                          WCHAR *pBuffer, int iBufSize)
{
   void *pData;
   int nLen;

   // Check if we have valid result handle
   if (hResult == NULL)
      return NULL;

   // Check if there are valid fetched row
   if (((MSDB_ASYNC_QUERY_RESULT *)hResult)->bNoMoreRows)
      return NULL;

   // Now get column data
   pData = (void *)dbdata(((MSDB_ASYNC_QUERY_RESULT *)hResult)->pConnection->hProcess, iColumn + 1);
   if (pData != NULL)
   {
      switch(((MSDB_ASYNC_QUERY_RESULT *)hResult)->piColTypes[iColumn])
      {
         case SQLCHAR:
         case SQLTEXT:
         case SQLBINARY:
            nLen = MultiByteToWideChar(CP_ACP, 0, (char *)pData,
                                       dbdatlen(((MSDB_ASYNC_QUERY_RESULT *)hResult)->pConnection->hProcess, iColumn + 1),
                                       pBuffer, iBufSize);
            pBuffer[nLen] = 0;
            break;
         case SQLINT1:
            _snwprintf_s(pBuffer, iBufSize, _TRUNCATE, L"%d", *((char *)pData));
            break;
         case SQLINT2:
            _snwprintf_s(pBuffer, iBufSize, _TRUNCATE, L"%d", *((short *)pData));
            break;
         case SQLINT4:
            _snwprintf_s(pBuffer, iBufSize, _TRUNCATE, L"%d", *((LONG *)pData));
            break;
         case SQLFLT4:
            _snwprintf_s(pBuffer, iBufSize, _TRUNCATE, L"%f", *((float *)pData));
            break;
         case SQLFLT8:
            _snwprintf_s(pBuffer, iBufSize, _TRUNCATE, L"%f", *((double *)pData));
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
         while(dbnextrow(((MSDB_ASYNC_QUERY_RESULT *)hResult)->pConnection->hProcess) != NO_MORE_ROWS);

         // Now we are ready for next query, so unlock query mutex
         MutexUnlock(((MSDB_ASYNC_QUERY_RESULT *)hResult)->pConnection->mutexQueryLock);
      }

      // Free allocated memory
      if (((MSDB_ASYNC_QUERY_RESULT *)hResult)->piColTypes != NULL)
         free(((MSDB_ASYNC_QUERY_RESULT *)hResult)->piColTypes);
      free(hResult);
   }
}


//
// Begin transaction
//

extern "C" DWORD EXPORT DrvBegin(MSDB_CONN *pConn)
{
   return DrvQuery(pConn, L"BEGIN TRANSACTION", NULL);
}


//
// Commit transaction
//

extern "C" DWORD EXPORT DrvCommit(MSDB_CONN *pConn)
{
   return DrvQuery(pConn, L"COMMIT", NULL);
}


//
// Rollback transaction
//

extern "C" DWORD EXPORT DrvRollback(MSDB_CONN *pConn)
{
   return DrvQuery(pConn, L"ROLLBACK", NULL);
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
