/* 
** ODBC Database Driver
** Copyright (C) 2004, 2005, 2006, 2007, 2008 Victor Kirhenshtein
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
** File: odbc.cpp
**
**/

#include "odbcdrv.h"


//
// Constants
//

#define DATA_BUFFER_SIZE      65536


//
// API version
//

extern "C" int EXPORT drvAPIVersion;
int EXPORT drvAPIVersion = DBDRV_API_VERSION;


//
// Static data
//

static BOOL m_useUnicode = TRUE;


//
// Convert ODBC state to NetXMS database error code and get error text
//

static DWORD GetSQLErrorInfo(SQLSMALLINT nHandleType, SQLHANDLE hHandle, TCHAR *errorText)
{
   SQLRETURN nRet;
   SQLSMALLINT nChars;
   DWORD dwError;
   char szState[16];

	// Get state information and convert it to NetXMS database error code
   nRet = SQLGetDiagField(nHandleType, hHandle, 1, SQL_DIAG_SQLSTATE, szState, 16, &nChars);
   if (nRet == SQL_SUCCESS)
   {
      if ((!strcmp(szState, "08003")) ||     // Connection does not exist
          (!strcmp(szState, "08S01")) ||     // Communication link failure
          (!strcmp(szState, "HYT00")) ||     // Timeout expired
          (!strcmp(szState, "HYT01")))       // Connection timeout expired
      {
         dwError = DBERR_CONNECTION_LOST;
      }
      else
      {
         dwError = DBERR_OTHER_ERROR;
      }
   }
   else
   {
      dwError = DBERR_OTHER_ERROR;
   }

	// Get error message
	if (errorText != NULL)
	{
		nRet = SQLGetDiagField(nHandleType, hHandle, 1, SQL_DIAG_MESSAGE_TEXT, errorText, DBDRV_MAX_ERROR_TEXT, &nChars);
		if (nRet == SQL_SUCCESS)
		{
			errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
			RemoveTrailingCRLF(errorText);
		}
		else
		{
			nx_strncpy(errorText, _T("Unable to obtain description for this error"), DBDRV_MAX_ERROR_TEXT);
		}
   }
   
   return dwError;
}


//
// Prepare string for using in SQL query - enclose in quotes and escape as needed
//

extern "C" TCHAR EXPORT *DrvPrepareString(const TCHAR *str)
{
	int len = (int)_tcslen(str) + 3;   // + two quotes and \0 at the end
	int bufferSize = len + 128;
	TCHAR *out = (TCHAR *)malloc(bufferSize * sizeof(TCHAR));
	out[0] = _T('\'');

	const TCHAR *src = str;
	int outPos;
	for(outPos = 1; *src != NULL; src++)
	{
		if (*src == _T('\''))
		{
			len++;
			if (len >= bufferSize)
			{
				bufferSize += 128;
				out = (TCHAR *)realloc(out, bufferSize * sizeof(TCHAR));
			}
			out[outPos++] = _T('\'');
			out[outPos++] = _T('\'');
		}
		else
		{
			out[outPos++] = *src;
		}
	}
	out[outPos++] = _T('\'');
	out[outPos++] = 0;

	return out;
}


//
// Initialize driver
//

extern "C" BOOL EXPORT DrvInit(char *cmdLine)
{
	m_useUnicode = ExtractNamedOptionValueAsBool(cmdLine, _T("unicode"), TRUE);
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
// pszHost should be set to ODBC source name, and pszDatabase is ignored
//

extern "C" DB_CONNECTION EXPORT DrvConnect(char *pszHost, char *pszLogin,
                                           char *pszPassword, char *pszDatabase)
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
	SQLSetConnectAttr(pConn->sqlConn, SQL_ATTR_LOGIN_TIMEOUT, (SQLPOINTER *)15, 0);
	SQLSetConnectAttr(pConn->sqlConn, SQL_ATTR_CONNECTION_TIMEOUT, (SQLPOINTER *)30, 0);

	// Connect to the datasource 
	iResult = SQLConnect(pConn->sqlConn, (SQLCHAR *)pszHost, SQL_NTS,
                        (SQLCHAR *)pszLogin, SQL_NTS, (SQLCHAR *)pszPassword, SQL_NTS);
	if ((iResult != SQL_SUCCESS) && (iResult != SQL_SUCCESS_WITH_INFO))
      goto connect_failure_2;

   // Create mutex
   pConn->mutexQuery = MutexCreate();

   // Success
   return (DB_CONNECTION)pConn;

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

extern "C" DWORD EXPORT DrvQuery(ODBCDRV_CONN *pConn, NETXMS_WCHAR *pwszQuery, TCHAR *errorText)
{
   long iResult;
   DWORD dwResult;

   MutexLock(pConn->mutexQuery, INFINITE);

   // Allocate statement handle
   iResult = SQLAllocHandle(SQL_HANDLE_STMT, pConn->sqlConn, &pConn->sqlStatement);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
   {
      // Execute statement
      if (m_useUnicode)
      {
#if defined(_WIN32) || defined(UNICODE_UCS2)
		   iResult = SQLExecDirectW(pConn->sqlStatement, (SQLWCHAR *)pwszQuery, SQL_NTS);
#else
			SQLWCHAR *temp = UCS2StringFromUCS4String(pwszQuery);
		   iResult = SQLExecDirectW(pConn->sqlStatement, temp, SQL_NTS);
		   free(temp);
#endif      
		}
		else
		{
			char *temp = MBStringFromWideString(pwszQuery);
		   iResult = SQLExecDirect(pConn->sqlStatement, (SQLCHAR *)temp, SQL_NTS);
		   free(temp);
		}
	   if ((iResult == SQL_SUCCESS) || 
          (iResult == SQL_SUCCESS_WITH_INFO) || 
          (iResult == SQL_NO_DATA))
      {
         dwResult = DBERR_SUCCESS;
      }
      else
      {
         dwResult = GetSQLErrorInfo(SQL_HANDLE_STMT, pConn->sqlStatement, errorText);
      }
      SQLFreeHandle(SQL_HANDLE_STMT, pConn->sqlStatement);
   }
   else
   {
      dwResult = GetSQLErrorInfo(SQL_HANDLE_DBC, pConn->sqlConn, errorText);
   }

   MutexUnlock(pConn->mutexQuery);
   return dwResult;
}


//
// Perform SELECT query
//

extern "C" DB_RESULT EXPORT DrvSelect(ODBCDRV_CONN *pConn, NETXMS_WCHAR *pwszQuery, DWORD *pdwError, TCHAR *errorText)
{
   long i, iResult, iCurrValue;
   ODBCDRV_QUERY_RESULT *pResult = NULL;
   short wNumCols;
   SQLLEN iDataSize;
   BYTE *pDataBuffer;

   pDataBuffer = (BYTE *)malloc(DATA_BUFFER_SIZE);

   MutexLock(pConn->mutexQuery, INFINITE);

   // Allocate statement handle
   iResult = SQLAllocHandle(SQL_HANDLE_STMT, pConn->sqlConn, &pConn->sqlStatement);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
   {
      // Execute statement
      if (m_useUnicode)
      {
#if defined(_WIN32) || defined(UNICODE_UCS2)
	      iResult = SQLExecDirectW(pConn->sqlStatement, (SQLWCHAR *)pwszQuery, SQL_NTS);
#else
			SQLWCHAR *temp = UCS2StringFromUCS4String(pwszQuery);
		   iResult = SQLExecDirectW(pConn->sqlStatement, temp, SQL_NTS);
		   free(temp);
#endif
		}
		else
		{
			char *temp = MBStringFromWideString(pwszQuery);
		   iResult = SQLExecDirect(pConn->sqlStatement, (SQLCHAR *)temp, SQL_NTS);
		   free(temp);
		}
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

			// Get column names
			pResult->columnNames = (char **)malloc(sizeof(char *) * pResult->iNumCols);
			for(i = 0; i < pResult->iNumCols; i++)
			{
				char name[256];
				SQLSMALLINT len;

				iResult = SQLColAttributeA(pConn->sqlStatement, (SQLSMALLINT)(i + 1),
				                           SQL_DESC_NAME, name, 256, &len, NULL); 
				if ((iResult == SQL_SUCCESS) || 
					 (iResult == SQL_SUCCESS_WITH_INFO))
				{
					name[len] = 0;
					pResult->columnNames[i] = strdup(name);
				}
				else
				{
					pResult->columnNames[i] = strdup("");
				}
			}

         // Fetch all data
         while(iResult = SQLFetch(pConn->sqlStatement), 
               (iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
         {
            pResult->iNumRows++;
            pResult->pValues = (NETXMS_WCHAR **)realloc(pResult->pValues, 
                        sizeof(NETXMS_WCHAR *) * (pResult->iNumRows * pResult->iNumCols));
            for(i = 1; i <= pResult->iNumCols; i++)
            {
               pDataBuffer[0] = 0;
               iResult = SQLGetData(pConn->sqlStatement, (short)i,
                                    m_useUnicode ? SQL_C_WCHAR : SQL_C_CHAR,
                                    pDataBuffer, DATA_BUFFER_SIZE, &iDataSize);
					if (iDataSize != SQL_NULL_DATA)
					{
						if (m_useUnicode)
						{
#if defined(_WIN32) || defined(UNICODE_UCS2)
               		pResult->pValues[iCurrValue++] = wcsdup((const WCHAR *)pDataBuffer);
#else
	               	pResult->pValues[iCurrValue++] = UCS4StringFromUCS2String((const UCS2CHAR *)pDataBuffer);
#endif
						}
						else
						{
               		pResult->pValues[iCurrValue++] = WideStringFromMBString((const char *)pDataBuffer);
						}
					}
					else
					{
						pResult->pValues[iCurrValue++] = wcsdup(L"");
					}
            }
         }
         *pdwError = DBERR_SUCCESS;
      }
      else
      {
         *pdwError = GetSQLErrorInfo(SQL_HANDLE_STMT, pConn->sqlStatement, errorText);
      }
      SQLFreeHandle(SQL_HANDLE_STMT, pConn->sqlStatement);
   }
   else
   {
      *pdwError = GetSQLErrorInfo(SQL_HANDLE_DBC, pConn->sqlConn, errorText);
   }

   MutexUnlock(pConn->mutexQuery);
   free(pDataBuffer);
   return pResult;
}


//
// Get field length from result
//

extern "C" LONG EXPORT DrvGetFieldLength(ODBCDRV_QUERY_RESULT *pResult, int iRow, int iColumn)
{
   LONG nLen = -1;

   if (pResult != NULL)
   {
      if ((iRow < pResult->iNumRows) && (iRow >= 0) &&
          (iColumn < pResult->iNumCols) && (iColumn >= 0))
         nLen = (LONG)wcslen(pResult->pValues[iRow * pResult->iNumCols + iColumn]);
   }
   return nLen;
}


//
// Get field value from result
//

extern "C" NETXMS_WCHAR EXPORT *DrvGetField(ODBCDRV_QUERY_RESULT *pResult, int iRow, int iColumn,
                                            NETXMS_WCHAR *pBuffer, int nBufSize)
{
   NETXMS_WCHAR *pValue = NULL;

   if (pResult != NULL)
   {
      if ((iRow < pResult->iNumRows) && (iRow >= 0) &&
          (iColumn < pResult->iNumCols) && (iColumn >= 0))
      {
#ifdef _WIN32
         wcsncpy_s(pBuffer, nBufSize, pResult->pValues[iRow * pResult->iNumCols + iColumn], _TRUNCATE);
#else
         wcsncpy(pBuffer, pResult->pValues[iRow * pResult->iNumCols + iColumn], nBufSize);
         pBuffer[nBufSize - 1] = 0;
#endif
         pValue = pBuffer;
      }
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
// Get column count in query result
//

extern "C" int EXPORT DrvGetColumnCount(ODBCDRV_QUERY_RESULT *pResult)
{
	return (pResult != NULL) ? pResult->iNumCols : 0;
}


//
// Get column name in query result
//

extern "C" const char EXPORT *DrvGetColumnName(ODBCDRV_QUERY_RESULT *pResult, int column)
{
	return ((pResult != NULL) && (column >= 0) && (column < pResult->iNumCols)) ? pResult->columnNames[column] : NULL;
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

		for(i = 0; i < pResult->iNumCols; i++)
			safe_free(pResult->columnNames[i]);
		safe_free(pResult->columnNames);

      free(pResult);
   }
}


//
// Perform asynchronous SELECT query
//

extern "C" DB_ASYNC_RESULT EXPORT DrvAsyncSelect(ODBCDRV_CONN *pConn, NETXMS_WCHAR *pwszQuery,
                                                 DWORD *pdwError, TCHAR *errorText)
{
   ODBCDRV_ASYNC_QUERY_RESULT *pResult = NULL;
   long iResult;
   short wNumCols;
	int i;

   MutexLock(pConn->mutexQuery, INFINITE);

   // Allocate statement handle
   iResult = SQLAllocHandle(SQL_HANDLE_STMT, pConn->sqlConn, &pConn->sqlStatement);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
   {
      // Execute statement
      if (m_useUnicode)
      {
#if defined(_WIN32) || defined(UNICODE_UCS2)
	      iResult = SQLExecDirectW(pConn->sqlStatement, (SQLWCHAR *)pwszQuery, SQL_NTS);
#else
			SQLWCHAR *temp = UCS2StringFromUCS4String(pwszQuery);
		   iResult = SQLExecDirectW(pConn->sqlStatement, temp, SQL_NTS);
		   free(temp);
#endif
		}
		else
		{
			char *temp = MBStringFromWideString(pwszQuery);
		   iResult = SQLExecDirect(pConn->sqlStatement, (SQLCHAR *)temp, SQL_NTS);
		   free(temp);
		}
	   if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
      {
         // Allocate result buffer and determine column info
         pResult = (ODBCDRV_ASYNC_QUERY_RESULT *)malloc(sizeof(ODBCDRV_ASYNC_QUERY_RESULT));
         SQLNumResultCols(pConn->sqlStatement, &wNumCols);
         pResult->iNumCols = wNumCols;
         pResult->pConn = pConn;
         pResult->bNoMoreRows = FALSE;

			// Get column names
			pResult->columnNames = (char **)malloc(sizeof(char *) * pResult->iNumCols);
			for(i = 0; i < pResult->iNumCols; i++)
			{
				char name[256];
				SQLSMALLINT len;

				iResult = SQLColAttributeA(pConn->sqlStatement, (SQLSMALLINT)(i + 1),
				                           SQL_DESC_NAME, name, 256, &len, NULL); 
				if ((iResult == SQL_SUCCESS) || 
					 (iResult == SQL_SUCCESS_WITH_INFO))
				{
					name[len] = 0;
					pResult->columnNames[i] = strdup(name);
				}
				else
				{
					pResult->columnNames[i] = strdup("");
				}
			}

			*pdwError = DBERR_SUCCESS;
      }
      else
      {
         *pdwError = GetSQLErrorInfo(SQL_HANDLE_STMT, pConn->sqlStatement, errorText);
         // Free statement handle if query failed
         SQLFreeHandle(SQL_HANDLE_STMT, pConn->sqlStatement);
      }
   }
   else
   {
      *pdwError = GetSQLErrorInfo(SQL_HANDLE_DBC, pConn->sqlConn, errorText);
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
// Get field length from async query result
//

extern "C" LONG EXPORT DrvGetFieldLengthAsync(ODBCDRV_ASYNC_QUERY_RESULT *pResult, int iColumn)
{
   LONG nLen = -1;

   if (pResult != NULL)
   {
      if ((iColumn < pResult->iNumCols) && (iColumn >= 0))
		{
			SQLLEN dataSize;
			char temp[1];
		   long rc = SQLGetData(pResult->pConn->sqlStatement, (short)iColumn + 1, SQL_C_CHAR,
		                        temp, 0, &dataSize);
			if ((rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO))
			{
				nLen = dataSize;
			}
		}
   }
   return nLen;
}


//
// Get field from current row in async query result
//

extern "C" NETXMS_WCHAR EXPORT *DrvGetFieldAsync(ODBCDRV_ASYNC_QUERY_RESULT *pResult,
                                                 int iColumn, NETXMS_WCHAR *pBuffer, int iBufSize)
{
   SQLLEN iDataSize;
   long iResult;

   // Check if we have valid result handle
   if (pResult == NULL)
      return NULL;

   // Check if there are valid fetched row
   if (pResult->bNoMoreRows)
      return NULL;

   if ((iColumn >= 0) && (iColumn < pResult->iNumCols))
   {
   	if (m_useUnicode)
   	{
#if defined(_WIN32) || defined(UNICODE_UCS2)
		   iResult = SQLGetData(pResult->pConn->sqlStatement, (short)iColumn + 1, SQL_C_WCHAR,
		                        pBuffer, iBufSize * sizeof(WCHAR), &iDataSize);
#else
			SQLWCHAR *tempBuff = (SQLWCHAR *)malloc(iBufSize * sizeof(SQLWCHAR));
		   iResult = SQLGetData(pResult->pConn->sqlStatement, (short)iColumn + 1, SQL_C_WCHAR,
		                        tempBuff, iBufSize * sizeof(SQLWCHAR), &iDataSize);
		   ucs2_to_ucs4(tempBuff, -1, pBuffer, iBufSize);
		   pBuffer[iBufSize - 1] = 0;
		   free(tempBuff);
#endif
		}
		else
		{
			SQLCHAR *tempBuff = (SQLCHAR *)malloc(iBufSize * sizeof(SQLCHAR));
		   iResult = SQLGetData(pResult->pConn->sqlStatement, (short)iColumn + 1, SQL_C_CHAR,
		                        tempBuff, iBufSize * sizeof(SQLCHAR), &iDataSize);
		   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (char *)tempBuff, -1, pBuffer, iBufSize);
		   pBuffer[iBufSize - 1] = 0;
		   free(tempBuff);
		}
      if (((iResult != SQL_SUCCESS) && (iResult != SQL_SUCCESS_WITH_INFO)) || (iDataSize == SQL_NULL_DATA))
         pBuffer[0] = 0;
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

extern "C" int EXPORT DrvGetColumnCountAsync(ODBCDRV_ASYNC_QUERY_RESULT *pResult)
{
	return (pResult != NULL) ? pResult->iNumCols : 0;
}


//
// Get column name in async query result
//

extern "C" const char EXPORT *DrvGetColumnNameAsync(ODBCDRV_ASYNC_QUERY_RESULT *pResult, int column)
{
	return ((pResult != NULL) && (column >= 0) && (column < pResult->iNumCols)) ? pResult->columnNames[column] : NULL;
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
// Begin transaction
//

extern "C" DWORD EXPORT DrvBegin(ODBCDRV_CONN *pConn)
{
   SQLRETURN nRet;
   DWORD dwResult;

	if (pConn == NULL)
      return DBERR_INVALID_HANDLE;

	MutexLock(pConn->mutexQuery, INFINITE);
   nRet = SQLSetConnectAttr(pConn->sqlConn, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0);
   if ((nRet == SQL_SUCCESS) || (nRet == SQL_SUCCESS_WITH_INFO))
   {
      dwResult = DBERR_SUCCESS;
   }
   else
   {
      dwResult = GetSQLErrorInfo(SQL_HANDLE_DBC, pConn->sqlConn, NULL);
   }
	MutexUnlock(pConn->mutexQuery);
   return dwResult;
}


//
// Commit transaction
//

extern "C" DWORD EXPORT DrvCommit(ODBCDRV_CONN *pConn)
{
   SQLRETURN nRet;

	if (pConn == NULL)
      return DBERR_INVALID_HANDLE;

	MutexLock(pConn->mutexQuery, INFINITE);
   nRet = SQLEndTran(SQL_HANDLE_DBC, pConn->sqlConn, SQL_COMMIT);
   SQLSetConnectAttr(pConn->sqlConn, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
	MutexUnlock(pConn->mutexQuery);
   return ((nRet == SQL_SUCCESS) || (nRet == SQL_SUCCESS_WITH_INFO)) ? DBERR_SUCCESS : DBERR_OTHER_ERROR;
}


//
// Rollback transaction
//

extern "C" DWORD EXPORT DrvRollback(ODBCDRV_CONN *pConn)
{
   SQLRETURN nRet;

	if (pConn == NULL)
      return DBERR_INVALID_HANDLE;

	MutexLock(pConn->mutexQuery, INFINITE);
   nRet = SQLEndTran(SQL_HANDLE_DBC, pConn->sqlConn, SQL_ROLLBACK);
   SQLSetConnectAttr(pConn->sqlConn, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
	MutexUnlock(pConn->mutexQuery);
   return ((nRet == SQL_SUCCESS) || (nRet == SQL_SUCCESS_WITH_INFO)) ? DBERR_SUCCESS : DBERR_OTHER_ERROR;
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
