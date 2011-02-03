/* 
** Microsoft SQL Server Database Driver
** Copyright (C) 2004-2009 Victor Kirhenshtein
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
extern "C" const char EXPORT *drvName = "MSSQL";


//
// Prepare string for using in SQL query - enclose in quotes and escape as needed
//

extern "C" WCHAR EXPORT *DrvPrepareStringW(const WCHAR *str)
{
	int len = (int)wcslen(str) + 4;   // + two quotes, N prefix, and \0 at the end
	int bufferSize = len + 128;
	WCHAR *out = (WCHAR *)malloc(bufferSize * sizeof(WCHAR));
	wcscpy(out, L"N'");

	const WCHAR *src = str;
	int outPos;
	for(outPos = 2; *src != NULL; src++)
	{
		long chval = *src;
		if (chval < 32)
		{
			WCHAR buffer[32];

			_snwprintf(buffer, 32, L"'+nchar(%ld)+N'", chval);
			int l = (int)wcslen(buffer);

			len += l;
			if (len >= bufferSize)
			{
				bufferSize += 128;
				out = (WCHAR *)realloc(out, bufferSize * sizeof(WCHAR));
			}
			memcpy(&out[outPos], buffer, l * sizeof(WCHAR));
			outPos += l;
		}
		else if (*src == L'\'')
		{
			len++;
			if (len >= bufferSize)
			{
				bufferSize += 128;
				out = (WCHAR *)realloc(out, bufferSize * sizeof(WCHAR));
			}
			out[outPos++] = L'\'';
			out[outPos++] = L'\'';
		}
		else
		{
			out[outPos++] = *src;
		}
	}
	out[outPos++] = L'\'';
	out[outPos++] = 0;

	return out;
}

extern "C" char EXPORT *DrvPrepareStringA(const char *str)
{
	int len = (int)strlen(str) + 4;   // + two quotes, N prefix, and \0 at the end
	int bufferSize = len + 128;
	char *out = (char *)malloc(bufferSize);
	strcpy(out, "N'");

	const char *src = str;
	int outPos;
	for(outPos = 2; *src != NULL; src++)
	{
		long chval = (long)(*((unsigned char *)src));
		if (chval < 32)
		{
			char buffer[32];

			snprintf(buffer, 32, "'+nchar(%ld)+N'", chval);
			int l = (int)strlen(buffer);

			len += l;
			if (len >= bufferSize)
			{
				bufferSize += 128;
				out = (char *)realloc(out, bufferSize);
			}
			memcpy(&out[outPos], buffer, l);
			outPos += l;
		}
		else if (*src == '\'')
		{
			len++;
			if (len >= bufferSize)
			{
				bufferSize += 128;
				out = (char *)realloc(out, bufferSize);
			}
			out[outPos++] = '\'';
			out[outPos++] = '\'';
		}
		else
		{
			out[outPos++] = *src;
		}
	}
	out[outPos++] = '\'';
	out[outPos++] = 0;

	return out;
}


//
// Initialize driver
//

extern "C" BOOL EXPORT DrvInit(const char *cmdLine)
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

extern "C" void EXPORT DrvUnload()
{
}


//
// Connect to database
//

extern "C" DBDRV_CONNECTION EXPORT DrvConnect(const char *host, const char *login,
                                           const char *password, const char *database)
{
   LOGINREC *loginrec;
   MSDB_CONN *pConn = NULL;
   PDBPROCESS hProcess;

   loginrec = dblogin();
   if (!strcmp(login, "*"))
   {
      DBSETLSECURE(loginrec);
   }
   else
   {
      DBSETLUSER(loginrec, login);
      DBSETLPWD(loginrec, password);
   }
   DBSETLAPP(loginrec, "NetXMS");
   DBSETLTIME(loginrec, CONNECT_TIMEOUT);
   hProcess = dbopen(loginrec, host);

   if (hProcess != NULL)
   {
      dbsetuserdata(hProcess, NULL);

      // Change to specified database
      if (database != NULL)
      {
         if (dbuse(hProcess, database) != SUCCEED)
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
         nx_strncpy(pConn->szHost, host, MAX_CONN_STRING);
         nx_strncpy(pConn->szLogin, login, MAX_CONN_STRING);
         nx_strncpy(pConn->szPassword, password, MAX_CONN_STRING);
         nx_strncpy(pConn->szDatabase, CHECK_NULL_EX(database), MAX_CONN_STRING);
			pConn->szErrorText[0] = 0;

         dbsetuserdata(hProcess, pConn);
      }
   }

   return (DBDRV_CONNECTION)pConn;
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

extern "C" DBDRV_RESULT EXPORT DrvSelect(MSDB_CONN *pConn, WCHAR *pwszQuery, DWORD *pdwError, TCHAR *errorText)
{
   MSDB_QUERY_RESULT *pResult = NULL;
   int i, iCurrPos, iLen, *piColTypes;
   void *pData;
   char *pszQueryUTF8;
	LPCSTR name;

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
			pResult->columnNames = (char **)malloc(sizeof(char *) * pResult->iNumCols);

         // Determine column names and types
         piColTypes = (int *)malloc(pResult->iNumCols * sizeof(int));
         for(i = 0; i < pResult->iNumCols; i++)
			{
            piColTypes[i] = dbcoltype(pConn->hProcess, i + 1);
				name = dbcolname(pConn->hProcess, i + 1);
				pResult->columnNames[i] = strdup(CHECK_NULL_A(name));
			}

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
   return (DBDRV_RESULT)pResult;
}


//
// Get field length from result
//

extern "C" LONG EXPORT DrvGetFieldLength(MSDB_QUERY_RESULT *pResult, int iRow, int iColumn)
{
   if ((iRow < 0) || (iRow >= pResult->iNumRows) ||
       (iColumn < 0) || (iColumn >= pResult->iNumCols))
      return -1;
   return strlen(pResult->pValues[iRow * pResult->iNumCols + iColumn]);
}


//
// Get field value from result
//

extern "C" WCHAR EXPORT *DrvGetField(MSDB_QUERY_RESULT *pResult, int iRow, int iColumn,
                                     WCHAR *pBuffer, int nBufSize)
{
   if ((iRow < 0) || (iRow >= pResult->iNumRows) ||
       (iColumn < 0) || (iColumn >= pResult->iNumCols))
      return NULL;
   MultiByteToWideChar(CP_ACP, 0, pResult->pValues[iRow * pResult->iNumCols + iColumn],
                       -1, pBuffer, nBufSize);
   pBuffer[nBufSize - 1] = 0;
   return pBuffer;
}


//
// Get number of rows in result
//

extern "C" int EXPORT DrvGetNumRows(MSDB_QUERY_RESULT *pResult)
{
   return (pResult != NULL) ? pResult->iNumRows : 0;
}


//
// Get column count in query result
//

extern "C" int EXPORT DrvGetColumnCount(MSDB_QUERY_RESULT *pResult)
{
	return (pResult != NULL) ? pResult->iNumCols : 0;
}


//
// Get column name in query result
//

extern "C" const char EXPORT *DrvGetColumnName(MSDB_QUERY_RESULT *pResult, int column)
{
	return ((pResult != NULL) && (column >= 0) && (column < pResult->iNumCols)) ? pResult->columnNames[column] : NULL;
}


//
// Free SELECT results
//

extern "C" void EXPORT DrvFreeResult(MSDB_QUERY_RESULT *pResult)
{
   if (pResult != NULL)
   {
      int i, iNumValues;

      iNumValues = pResult->iNumRows * pResult->iNumCols;
      for(i = 0; i < iNumValues; i++)
         free(pResult->pValues[i]);
      safe_free(pResult->pValues);
		for(i = 0; i < pResult->iNumCols; i++)
			safe_free(pResult->columnNames[i]);
		safe_free(pResult->columnNames);
      free(pResult);
   }
}


//
// Perform asynchronous SELECT query
//

extern "C" DBDRV_ASYNC_RESULT EXPORT DrvAsyncSelect(MSDB_CONN *pConn, WCHAR *pwszQuery,
                                                 DWORD *pdwError, TCHAR *errorText)
{
   MSDB_ASYNC_QUERY_RESULT *pResult = NULL;
   char *pszQueryUTF8;
	LPCSTR name;
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
			pResult->columnNames = (char **)malloc(sizeof(char *) * pResult->iNumCols);

         // Determine column names and types
         for(i = 0; i < pResult->iNumCols; i++)
			{
            pResult->piColTypes[i] = dbcoltype(pConn->hProcess, i + 1);
				name = dbcolname(pConn->hProcess, i + 1);
				pResult->columnNames[i] = strdup(CHECK_NULL_A(name));
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
      MutexUnlock(pConn->mutexQueryLock);
   }
   free(pszQueryUTF8);
   return pResult;
}


//
// Fetch next result line from asynchronous SELECT results
//

extern "C" BOOL EXPORT DrvFetch(MSDB_ASYNC_QUERY_RESULT *pResult)
{
   BOOL success = TRUE;
   
   if (pResult == NULL)
   {
      success = FALSE;
   }
   else
   {
      // Try to fetch next row from server
      if (dbnextrow(pResult->pConnection->hProcess) == NO_MORE_ROWS)
      {
         pResult->bNoMoreRows = TRUE;
         success = FALSE;
         MutexUnlock(pResult->pConnection->mutexQueryLock);
      }
   }
   return success;
}


//
// Get field length from async query result
//

extern "C" LONG EXPORT DrvGetFieldLengthAsync(MSDB_ASYNC_QUERY_RESULT *result, int column)
{
	if ((result == NULL) || (column < 0) || (column >= result->iNumCols))
      return -1;

   switch(result->piColTypes[column])
   {
      case SQLCHAR:
      case SQLTEXT:
      case SQLBINARY:
         return dbdatlen(result->pConnection->hProcess, column + 1);
      case SQLINT1:
         return 4;
      case SQLINT2:
         return 6;
      case SQLINT4:
         return 12;
      case SQLFLT4:
         return 30;
      case SQLFLT8:
			return 20;
	}
	return 0;
}


//
// Get field from current row in async query result
//

extern "C" WCHAR EXPORT *DrvGetFieldAsync(MSDB_ASYNC_QUERY_RESULT *pResult, int iColumn,
                                          WCHAR *pBuffer, int iBufSize)
{
   void *pData;
   int nLen;

   // Check if we have valid result handle
   if (pResult == NULL)
      return NULL;

   // Check if there are valid fetched row
   if (pResult->bNoMoreRows)
      return NULL;

   // Now get column data
   pData = (void *)dbdata(pResult->pConnection->hProcess, iColumn + 1);
   if (pData != NULL)
   {
      switch(pResult->piColTypes[iColumn])
      {
         case SQLCHAR:
         case SQLTEXT:
         case SQLBINARY:
            nLen = MultiByteToWideChar(CP_ACP, 0, (char *)pData,
                                       dbdatlen(pResult->pConnection->hProcess, iColumn + 1),
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
// Get column count in async query result
//

extern "C" int EXPORT DrvGetColumnCountAsync(MSDB_ASYNC_QUERY_RESULT *pResult)
{
	return (pResult != NULL) ? pResult->iNumCols : 0;
}


//
// Get column name in async query result
//

extern "C" const char EXPORT *DrvGetColumnNameAsync(MSDB_ASYNC_QUERY_RESULT *pResult, int column)
{
	return ((pResult != NULL) && (column >= 0) && (column < pResult->iNumCols)) ? pResult->columnNames[column] : NULL;
}


//
// Destroy result of async query
//

extern "C" void EXPORT DrvFreeAsyncResult(MSDB_ASYNC_QUERY_RESULT *pResult)
{
   if (pResult != NULL)
   {
      // Check if all result rows fetchef
      if (!pResult->bNoMoreRows)
      {
         // Fetch remaining rows
         while(dbnextrow(pResult->pConnection->hProcess) != NO_MORE_ROWS);

         // Now we are ready for next query, so unlock query mutex
         MutexUnlock(pResult->pConnection->mutexQueryLock);
      }

      // Free allocated memory
      if (pResult->piColTypes != NULL)
         free(pResult->piColTypes);
		for(int i = 0; i < pResult->iNumCols; i++)
			safe_free(pResult->columnNames[i]);
		safe_free(pResult->columnNames);
      free(pResult);
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
