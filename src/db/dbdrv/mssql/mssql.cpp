/* 
** MS SQL Database Driver
** Copyright (C) 2004-2017 Victor Kirhenshtein
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

#undef EXPORT
#define EXPORT __declspec(dllexport)

DECLARE_DRIVER_HEADER("MSSQL")

/**
 * Selected ODBC driver
 */
static char s_driver[SQL_MAX_DSN_LENGTH + 1] = "SQL Native Client";

/**
 * Convert ODBC state to NetXMS database error code and get error text
 */
static DWORD GetSQLErrorInfo(SQLSMALLINT nHandleType, SQLHANDLE hHandle, WCHAR *errorText)
{
   SQLRETURN nRet;
   SQLSMALLINT nChars;
   DWORD dwError;
   char szState[16];

	// Get state information and convert it to NetXMS database error code
   nRet = SQLGetDiagFieldA(nHandleType, hHandle, 1, SQL_DIAG_SQLSTATE, szState, 16, &nChars);
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
		nRet = SQLGetDiagFieldW(nHandleType, hHandle, 1, SQL_DIAG_MESSAGE_TEXT, errorText, DBDRV_MAX_ERROR_TEXT, &nChars);
		if (nRet == SQL_SUCCESS)
		{
			RemoveTrailingCRLFW(errorText);
		}
		else
		{
			wcscpy(errorText, L"Unable to obtain description for this error");
		}
   }
   
   return dwError;
}

/**
 * Clear any pending result sets on given statement
 */
static void ClearPendingResults(SQLHSTMT stmt)
{
	while(1)
	{
		SQLRETURN rc = SQLMoreResults(stmt);
		if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO))
			break;
	}
}

/**
 * Prepare string for using in SQL query - enclose in quotes and escape as needed
 */
extern "C" WCHAR EXPORT *DrvPrepareStringW(const WCHAR *str)
{
	int len = (int)wcslen(str) + 3;   // + two quotes and \0 at the end
	int bufferSize = len + 128;
	WCHAR *out = (WCHAR *)malloc(bufferSize * sizeof(WCHAR));
	out[0] = L'\'';

	const WCHAR *src = str;
	int outPos;
	for(outPos = 1; *src != NULL; src++)
	{
		if (*src == L'\'')
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
	int len = (int)strlen(str) + 3;   // + two quotes and \0 at the end
	int bufferSize = len + 128;
	char *out = (char *)malloc(bufferSize);
	out[0] = '\'';

	const char *src = str;
	int outPos;
	for(outPos = 1; *src != NULL; src++)
	{
		if (*src == '\'')
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

/**
 * Initialize driver
 */
extern "C" bool EXPORT DrvInit(const char *cmdLine)
{
   // Allocate environment
	SQLHENV sqlEnv;
   long rc = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlEnv);
	if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO))
		return false;

	rc = SQLSetEnvAttr(sqlEnv, SQL_ATTR_ODBC_VERSION, (void *)SQL_OV_ODBC3, 0);
	if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO))
		return false;

	// Find correct driver
	// Default is "SQL Native Client", but switch to "SQL Server Native Client 10.0" if found
	char name[SQL_MAX_DSN_LENGTH + 1], attrs[1024];
	SQLSMALLINT l1, l2;
	rc = SQLDriversA(sqlEnv, SQL_FETCH_FIRST, (SQLCHAR *)name, SQL_MAX_DSN_LENGTH + 1, &l1, (SQLCHAR *)attrs, 1024, &l2);
	while((rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO))
	{
		if (!strcmp(name, "SQL Server Native Client 10.0") ||
          !strcmp(name, "SQL Server Native Client 11.0"))
		{
			strcpy(s_driver, name);
			break;
		}
		rc = SQLDriversA(sqlEnv, SQL_FETCH_NEXT, (SQLCHAR *)name, SQL_MAX_DSN_LENGTH + 1, &l1, (SQLCHAR *)attrs, 1024, &l2);
	}

   SQLFreeHandle(SQL_HANDLE_ENV, sqlEnv);
   return true;
}

/**
 * Unload handler
 */
extern "C" void EXPORT DrvUnload()
{
}

/**
 * Connect to database
 */
extern "C" DBDRV_CONNECTION EXPORT DrvConnect(const char *host, const char *login, const char *password, 
															 const char *database, const char *schema, WCHAR *errorText)
{
   long iResult;
   MSSQL_CONN *pConn;

   // Allocate our connection structure
   pConn = (MSSQL_CONN *)malloc(sizeof(MSSQL_CONN));

   // Allocate environment
   iResult = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &pConn->sqlEnv);
	if ((iResult != SQL_SUCCESS) && (iResult != SQL_SUCCESS_WITH_INFO))
	{
		wcscpy(errorText, L"Cannot allocate environment handle");
		goto connect_failure_0;
	}

   // Set required ODBC version
	iResult = SQLSetEnvAttr(pConn->sqlEnv, SQL_ATTR_ODBC_VERSION, (void *)SQL_OV_ODBC3, 0);
	if ((iResult != SQL_SUCCESS) && (iResult != SQL_SUCCESS_WITH_INFO))
	{
		wcscpy(errorText, L"Call to SQLSetEnvAttr failed");
      goto connect_failure_1;
	}

	// Allocate connection handle, set timeout
	iResult = SQLAllocHandle(SQL_HANDLE_DBC, pConn->sqlEnv, &pConn->sqlConn); 
	if ((iResult != SQL_SUCCESS) && (iResult != SQL_SUCCESS_WITH_INFO))
	{
		wcscpy(errorText, L"Cannot allocate connection handle");
      goto connect_failure_1;
	}
	SQLSetConnectAttr(pConn->sqlConn, SQL_ATTR_LOGIN_TIMEOUT, (SQLPOINTER *)15, 0);
	SQLSetConnectAttr(pConn->sqlConn, SQL_ATTR_CONNECTION_TIMEOUT, (SQLPOINTER *)30, 0);

	// Connect to the server
	SQLSMALLINT outLen;
	char connectString[1024];

	if (!strcmp(login, "*"))
	{
		snprintf(connectString, 1024, "DRIVER={%s};Server=%s;Trusted_Connection=yes;Database=%s;APP=NetXMS", s_driver, host, database);
	}
	else
	{
		snprintf(connectString, 1024, "DRIVER={%s};Server=%s;UID=%s;PWD=%s;Database=%s;APP=NetXMS", s_driver, host, login, password, database);
	}
	iResult = SQLDriverConnectA(pConn->sqlConn, NULL, (SQLCHAR *)connectString, SQL_NTS, NULL, 0, &outLen, SQL_DRIVER_NOPROMPT);

	if ((iResult != SQL_SUCCESS) && (iResult != SQL_SUCCESS_WITH_INFO))
	{
		GetSQLErrorInfo(SQL_HANDLE_DBC, pConn->sqlConn, errorText);
      goto connect_failure_2;
	}

   // Create mutex
   pConn->mutexQuery = MutexCreate();

   // Success
   return (DBDRV_CONNECTION)pConn;

   // Handle failures
connect_failure_2:
   SQLFreeHandle(SQL_HANDLE_DBC, pConn->sqlConn);

connect_failure_1:
   SQLFreeHandle(SQL_HANDLE_ENV, pConn->sqlEnv);

connect_failure_0:
   free(pConn);
   return NULL;
}

/**
 * Disconnect from database
 */
extern "C" void EXPORT DrvDisconnect(MSSQL_CONN *pConn)
{
   MutexLock(pConn->mutexQuery);
   MutexUnlock(pConn->mutexQuery);
   SQLDisconnect(pConn->sqlConn);
   SQLFreeHandle(SQL_HANDLE_DBC, pConn->sqlConn);
   SQLFreeHandle(SQL_HANDLE_ENV, pConn->sqlEnv);
   MutexDestroy(pConn->mutexQuery);
   free(pConn);
}

/**
 * Prepare statement
 */
extern "C" DBDRV_STATEMENT EXPORT DrvPrepare(MSSQL_CONN *pConn, WCHAR *pwszQuery, DWORD *pdwError, WCHAR *errorText)
{
   long iResult;
	SQLHSTMT stmt;
	MSSQL_STATEMENT *result;

   MutexLock(pConn->mutexQuery);

   // Allocate statement handle
   iResult = SQLAllocHandle(SQL_HANDLE_STMT, pConn->sqlConn, &stmt);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
   {
      // Prepare statement
	   iResult = SQLPrepareW(stmt, (SQLWCHAR *)pwszQuery, SQL_NTS);
	   if ((iResult == SQL_SUCCESS) || 
          (iResult == SQL_SUCCESS_WITH_INFO))
		{
			result = (MSSQL_STATEMENT *)malloc(sizeof(MSSQL_STATEMENT));
			result->handle = stmt;
			result->buffers = new Array(0, 16, true);
			result->connection = pConn;
			*pdwError = DBERR_SUCCESS;
		}
		else
      {
         *pdwError = GetSQLErrorInfo(SQL_HANDLE_STMT, stmt, errorText);
	      SQLFreeHandle(SQL_HANDLE_STMT, stmt);
			result = NULL;
      }
   }
   else
   {
      *pdwError = GetSQLErrorInfo(SQL_HANDLE_DBC, pConn->sqlConn, errorText);
		result = NULL;
   }

   MutexUnlock(pConn->mutexQuery);
   return result;
}

/**
 * Bind parameter to statement
 */
extern "C" void EXPORT DrvBind(MSSQL_STATEMENT *stmt, int pos, int sqlType, int cType, void *buffer, int allocType)
{
	static SQLSMALLINT odbcSqlType[] = { SQL_VARCHAR, SQL_INTEGER, SQL_BIGINT, SQL_DOUBLE, SQL_LONGVARCHAR };
	static SQLSMALLINT odbcCType[] = { SQL_C_WCHAR, SQL_C_SLONG, SQL_C_ULONG, SQL_C_SBIGINT, SQL_C_UBIGINT, SQL_C_DOUBLE, SQL_C_WCHAR };
	static DWORD bufferSize[] = { 0, sizeof(LONG), sizeof(DWORD), sizeof(INT64), sizeof(QWORD), sizeof(double), 0 };

	int length = (cType == DB_CTYPE_STRING) ? ((int)wcslen((WCHAR *)buffer) + 1) : 0;

	SQLPOINTER sqlBuffer;
	switch(allocType)
	{
		case DB_BIND_STATIC:
         if (cType == DB_CTYPE_UTF8_STRING)
         {
            sqlBuffer = WideStringFromUTF8String((char *)buffer);
            stmt->buffers->add(sqlBuffer);
            length = (int)wcslen((WCHAR *)sqlBuffer) + 1;
         }
         else
         {
            sqlBuffer = buffer;
         }
			break;
		case DB_BIND_DYNAMIC:
         if (cType == DB_CTYPE_UTF8_STRING)
         {
            sqlBuffer = WideStringFromUTF8String((char *)buffer);
            free(buffer);
            length = (int)wcslen((WCHAR *)sqlBuffer) + 1;
         }
         else
         {
            sqlBuffer = buffer;
         }
			stmt->buffers->add(sqlBuffer);
			break;
		case DB_BIND_TRANSIENT:
         if (cType == DB_CTYPE_UTF8_STRING)
         {
            sqlBuffer = WideStringFromUTF8String((char *)buffer);
            length = (int)wcslen((WCHAR *)sqlBuffer) + 1;
         }
         else
         {
            sqlBuffer = nx_memdup(buffer, (cType == DB_CTYPE_STRING) ? (DWORD)(length * sizeof(WCHAR)) : bufferSize[cType]);
         }
			stmt->buffers->add(sqlBuffer);
			break;
		default:
			return;	// Invalid call
	}
	SQLBindParameter(stmt->handle, pos, SQL_PARAM_INPUT, odbcCType[cType], odbcSqlType[sqlType], length, 0, sqlBuffer, 0, NULL);
}

/**
 * Execute prepared statement
 */
extern "C" DWORD EXPORT DrvExecute(MSSQL_CONN *pConn, MSSQL_STATEMENT *stmt, WCHAR *errorText)
{
   DWORD dwResult;

   MutexLock(pConn->mutexQuery);
	long rc = SQLExecute(stmt->handle);
   if ((rc == SQL_SUCCESS) || 
       (rc == SQL_SUCCESS_WITH_INFO) || 
       (rc == SQL_NO_DATA))
   {
		ClearPendingResults(stmt->handle);
      dwResult = DBERR_SUCCESS;
   }
   else
   {
		dwResult = GetSQLErrorInfo(SQL_HANDLE_STMT, stmt->handle, errorText);
   }
   MutexUnlock(pConn->mutexQuery);
	return dwResult;
}

/**
 * Destroy prepared statement
 */
extern "C" void EXPORT DrvFreeStatement(MSSQL_STATEMENT *stmt)
{
	if (stmt == NULL)
		return;

	MutexLock(stmt->connection->mutexQuery);
	SQLFreeHandle(SQL_HANDLE_STMT, stmt->handle);
	MutexUnlock(stmt->connection->mutexQuery);
	delete stmt->buffers;
	free(stmt);
}

/**
 * Perform non-SELECT query
 */
extern "C" DWORD EXPORT DrvQuery(MSSQL_CONN *pConn, WCHAR *pwszQuery, WCHAR *errorText)
{
   long iResult;
   DWORD dwResult;

   MutexLock(pConn->mutexQuery);

   // Allocate statement handle
   SQLHSTMT sqlStatement;
   iResult = SQLAllocHandle(SQL_HANDLE_STMT, pConn->sqlConn, &sqlStatement);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
   {
      // Execute statement
	   iResult = SQLExecDirectW(sqlStatement, (SQLWCHAR *)pwszQuery, SQL_NTS);
	   if ((iResult == SQL_SUCCESS) || 
          (iResult == SQL_SUCCESS_WITH_INFO) || 
          (iResult == SQL_NO_DATA))
      {
         dwResult = DBERR_SUCCESS;
      }
      else
      {
         dwResult = GetSQLErrorInfo(SQL_HANDLE_STMT, sqlStatement, errorText);
      }
      SQLFreeHandle(SQL_HANDLE_STMT, sqlStatement);
   }
   else
   {
      dwResult = GetSQLErrorInfo(SQL_HANDLE_DBC, pConn->sqlConn, errorText);
   }

   MutexUnlock(pConn->mutexQuery);
   return dwResult;
}

/**
 * Get complete field data
 */
static WCHAR *GetFieldData(SQLHSTMT sqlStatement, short column)
{
   WCHAR *result = NULL;
   SQLLEN dataSize;
   WCHAR buffer[256];
   SQLRETURN rc = SQLGetData(sqlStatement, column, SQL_C_WCHAR, buffer, sizeof(buffer), &dataSize);
   if (((rc == SQL_SUCCESS) || ((rc == SQL_SUCCESS_WITH_INFO) && (dataSize >= 0) && (dataSize <= (SQLLEN)(sizeof(buffer) - sizeof(WCHAR))))) && (dataSize != SQL_NULL_DATA))
   {
      result = wcsdup(buffer);
   }
   else if ((rc == SQL_SUCCESS_WITH_INFO) && (dataSize != SQL_NULL_DATA))
   {
      if (dataSize > (SQLLEN)(sizeof(buffer) - sizeof(WCHAR)))
      {
         WCHAR *temp = (WCHAR *)malloc(dataSize + sizeof(WCHAR));
         memcpy(temp, buffer, sizeof(buffer));
         rc = SQLGetData(sqlStatement, column, SQL_C_WCHAR, &temp[255], dataSize - 254 * sizeof(WCHAR), &dataSize);
         if ((rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO))
         {
            result = temp;
         }
         else
         {
            free(temp);
         }
      }
      else if (dataSize == SQL_NO_TOTAL)
      {
         size_t tempSize = sizeof(buffer) * 4;  // temporary buffer size in bytes
         WCHAR *temp = (WCHAR *)malloc(tempSize);
         memcpy(temp, buffer, sizeof(buffer));
         size_t offset = sizeof(buffer) - sizeof(WCHAR); // offset in buffer in bytes
         while(true)
         {
            SQLLEN readSize = tempSize - offset;
            rc = SQLGetData(sqlStatement, column, SQL_C_WCHAR, (char *)temp + offset, readSize, &dataSize);
            if ((rc == SQL_SUCCESS) || (rc == SQL_NO_DATA))
               break;
            if (dataSize == SQL_NO_TOTAL)
            {
               tempSize += sizeof(buffer) * 4;
            }
            else
            {
               tempSize += dataSize - readSize;
            }
            temp = (WCHAR *)realloc(temp, tempSize);
            offset += readSize - sizeof(WCHAR);
         }
         result = temp;
      }
   }
   return (result != NULL) ? result : wcsdup(L"");
}

/**
 * Process results of SELECT query
 */
static MSSQL_QUERY_RESULT *ProcessSelectResults(SQLHSTMT stmt)
{
   // Allocate result buffer and determine column info
   MSSQL_QUERY_RESULT *pResult = (MSSQL_QUERY_RESULT *)malloc(sizeof(MSSQL_QUERY_RESULT));
   short wNumCols;
   SQLNumResultCols(stmt, &wNumCols);
   pResult->numColumns = wNumCols;
   pResult->numRows = 0;
   pResult->pValues = NULL;

	// Get column names
	pResult->columnNames = (char **)malloc(sizeof(char *) * pResult->numColumns);
	for(int i = 0; i < (int)pResult->numColumns; i++)
	{
		char name[256];
		SQLSMALLINT len;

		SQLRETURN iResult = SQLColAttributeA(stmt, (SQLSMALLINT)(i + 1), SQL_DESC_NAME, name, 256, &len, NULL); 
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
   long iCurrValue = 0;
	SQLRETURN iResult;
   while(iResult = SQLFetch(stmt), 
         (iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
   {
      pResult->numRows++;
      pResult->pValues = (WCHAR **)realloc(pResult->pValues, 
                  sizeof(WCHAR *) * (pResult->numRows * pResult->numColumns));
      for(int i = 1; i <= (int)pResult->numColumns; i++)
      {
         pResult->pValues[iCurrValue++] = GetFieldData(stmt, (short)i);
      }
   }

	return pResult;
}

/**
 * Perform SELECT query
 */
extern "C" DBDRV_RESULT EXPORT DrvSelect(MSSQL_CONN *pConn, WCHAR *pwszQuery, DWORD *pdwError, WCHAR *errorText)
{
   MSSQL_QUERY_RESULT *pResult = NULL;

   MutexLock(pConn->mutexQuery);

   // Allocate statement handle
   SQLHSTMT sqlStatement;
   SQLRETURN iResult = SQLAllocHandle(SQL_HANDLE_STMT, pConn->sqlConn, &sqlStatement);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
   {
      // Execute statement
      iResult = SQLExecDirectW(sqlStatement, (SQLWCHAR *)pwszQuery, SQL_NTS);
	   if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
      {
			pResult = ProcessSelectResults(sqlStatement);
		   *pdwError = DBERR_SUCCESS;
      }
      else
      {
         *pdwError = GetSQLErrorInfo(SQL_HANDLE_STMT, sqlStatement, errorText);
      }
      SQLFreeHandle(SQL_HANDLE_STMT, sqlStatement);
   }
   else
   {
      *pdwError = GetSQLErrorInfo(SQL_HANDLE_DBC, pConn->sqlConn, errorText);
   }

   MutexUnlock(pConn->mutexQuery);
   return pResult;
}

/**
 * Perform SELECT query using prepared statement
 */
extern "C" DBDRV_RESULT EXPORT DrvSelectPrepared(MSSQL_CONN *pConn, MSSQL_STATEMENT *stmt, DWORD *pdwError, WCHAR *errorText)
{
   MSSQL_QUERY_RESULT *pResult = NULL;

   MutexLock(pConn->mutexQuery);
	long rc = SQLExecute(stmt->handle);
   if ((rc == SQL_SUCCESS) || 
       (rc == SQL_SUCCESS_WITH_INFO))
   {
		pResult = ProcessSelectResults(stmt->handle);
	   *pdwError = DBERR_SUCCESS;
   }
   else
   {
		*pdwError = GetSQLErrorInfo(SQL_HANDLE_STMT, stmt->handle, errorText);
   }
   MutexUnlock(pConn->mutexQuery);
	return pResult;
}

/**
 * Get field length from result
 */
extern "C" LONG EXPORT DrvGetFieldLength(MSSQL_QUERY_RESULT *pResult, int iRow, int iColumn)
{
   LONG nLen = -1;

   if (pResult != NULL)
   {
      if ((iRow < pResult->numRows) && (iRow >= 0) &&
          (iColumn < pResult->numColumns) && (iColumn >= 0))
         nLen = (LONG)wcslen(pResult->pValues[iRow * pResult->numColumns + iColumn]);
   }
   return nLen;
}

/**
 * Get field value from result
 */
extern "C" WCHAR EXPORT *DrvGetField(MSSQL_QUERY_RESULT *pResult, int iRow, int iColumn, WCHAR *pBuffer, int nBufSize)
{
   WCHAR *pValue = NULL;

   if (pResult != NULL)
   {
      if ((iRow < pResult->numRows) && (iRow >= 0) &&
          (iColumn < pResult->numColumns) && (iColumn >= 0))
      {
         wcsncpy_s(pBuffer, nBufSize, pResult->pValues[iRow * pResult->numColumns + iColumn], _TRUNCATE);
         pValue = pBuffer;
      }
   }
   return pValue;
}

/**
 * Get number of rows in result
 */
extern "C" int EXPORT DrvGetNumRows(MSSQL_QUERY_RESULT *pResult)
{
   return (pResult != NULL) ? pResult->numRows : 0;
}

/**
 * Get column count in query result
 */
extern "C" int EXPORT DrvGetColumnCount(MSSQL_QUERY_RESULT *pResult)
{
	return (pResult != NULL) ? pResult->numColumns : 0;
}

/**
 * Get column name in query result
 */
extern "C" const char EXPORT *DrvGetColumnName(MSSQL_QUERY_RESULT *pResult, int column)
{
	return ((pResult != NULL) && (column >= 0) && (column < pResult->numColumns)) ? pResult->columnNames[column] : NULL;
}

/**
 * Free SELECT results
 */
extern "C" void EXPORT DrvFreeResult(MSSQL_QUERY_RESULT *pResult)
{
   if (pResult != NULL)
   {
      int i, iNumValues;

      iNumValues = pResult->numColumns * pResult->numRows;
      for(i = 0; i < iNumValues; i++)
         safe_free(pResult->pValues[i]);
      safe_free(pResult->pValues);

		for(i = 0; i < pResult->numColumns; i++)
			safe_free(pResult->columnNames[i]);
		safe_free(pResult->columnNames);

      free(pResult);
   }
}

/**
 * Perform unbuffered SELECT query
 */
extern "C" DBDRV_UNBUFFERED_RESULT EXPORT DrvSelectUnbuffered(MSSQL_CONN *pConn, WCHAR *pwszQuery, DWORD *pdwError, WCHAR *errorText)
{
   MSSQL_UNBUFFERED_QUERY_RESULT *pResult = NULL;

   MutexLock(pConn->mutexQuery);

   // Allocate statement handle
   SQLHSTMT sqlStatement;
   SQLRETURN iResult = SQLAllocHandle(SQL_HANDLE_STMT, pConn->sqlConn, &sqlStatement);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
   {
      // Execute statement
      iResult = SQLExecDirectW(sqlStatement, (SQLWCHAR *)pwszQuery, SQL_NTS);
	   if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
      {
         // Allocate result buffer and determine column info
         pResult = (MSSQL_UNBUFFERED_QUERY_RESULT *)malloc(sizeof(MSSQL_UNBUFFERED_QUERY_RESULT));
         pResult->sqlStatement = sqlStatement;
         pResult->isPrepared = false;
         
         short wNumCols;
         SQLNumResultCols(sqlStatement, &wNumCols);
         pResult->numColumns = wNumCols;
         pResult->pConn = pConn;
         pResult->noMoreRows = false;
			pResult->data = (WCHAR **)malloc(sizeof(WCHAR *) * pResult->numColumns);
         memset(pResult->data, 0, sizeof(WCHAR *) * pResult->numColumns);

			// Get column names
			pResult->columnNames = (char **)malloc(sizeof(char *) * pResult->numColumns);
			for(int i = 0; i < pResult->numColumns; i++)
			{
				char name[256];
				SQLSMALLINT len;

				iResult = SQLColAttributeA(sqlStatement, (SQLSMALLINT)(i + 1), SQL_DESC_NAME, name, 256, &len, NULL); 
				if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
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
         *pdwError = GetSQLErrorInfo(SQL_HANDLE_STMT, sqlStatement, errorText);
         // Free statement handle if query failed
         SQLFreeHandle(SQL_HANDLE_STMT, sqlStatement);
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

/**
 * Perform unbuffered SELECT query using prepared statement
 */
extern "C" DBDRV_UNBUFFERED_RESULT EXPORT DrvSelectPreparedUnbuffered(MSSQL_CONN *pConn, MSSQL_STATEMENT *stmt, DWORD *pdwError, WCHAR *errorText)
{
   MSSQL_UNBUFFERED_QUERY_RESULT *pResult = NULL;

   MutexLock(pConn->mutexQuery);
	SQLRETURN rc = SQLExecute(stmt->handle);
   if ((rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO))
   {
      // Allocate result buffer and determine column info
      pResult = (MSSQL_UNBUFFERED_QUERY_RESULT *)malloc(sizeof(MSSQL_UNBUFFERED_QUERY_RESULT));
      pResult->sqlStatement = stmt->handle;
      pResult->isPrepared = true;
      
      short wNumCols;
      SQLNumResultCols(pResult->sqlStatement, &wNumCols);
      pResult->numColumns = wNumCols;
      pResult->pConn = pConn;
      pResult->noMoreRows = false;
		pResult->data = (WCHAR **)malloc(sizeof(WCHAR *) * pResult->numColumns);
      memset(pResult->data, 0, sizeof(WCHAR *) * pResult->numColumns);

		// Get column names
		pResult->columnNames = (char **)malloc(sizeof(char *) * pResult->numColumns);
		for(int i = 0; i < pResult->numColumns; i++)
		{
			char name[256];
			SQLSMALLINT len;

			rc = SQLColAttributeA(pResult->sqlStatement, (SQLSMALLINT)(i + 1), SQL_DESC_NAME, name, 256, &len, NULL); 
			if ((rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO))
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
		*pdwError = GetSQLErrorInfo(SQL_HANDLE_STMT, stmt->handle, errorText);
   }

   if (pResult == NULL) // Unlock mutex if query has failed
      MutexUnlock(pConn->mutexQuery);
	return pResult;
}

/**
 * Fetch next result line from unbuffered SELECT results
 */
extern "C" bool EXPORT DrvFetch(MSSQL_UNBUFFERED_QUERY_RESULT *pResult)
{
   bool bResult = false;

   if (pResult != NULL)
   {
      long iResult;

      iResult = SQLFetch(pResult->sqlStatement);
      bResult = ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO));
      if (bResult)
      {
         for(int i = 0; i < pResult->numColumns; i++)
         {
            free(pResult->data[i]);
            pResult->data[i] = GetFieldData(pResult->sqlStatement, (short)i + 1);
         }
      }
      else
      {
         pResult->noMoreRows = true;
      }
   }
   return bResult;
}

/**
 * Get field length from unbuffered query result
 */
extern "C" LONG EXPORT DrvGetFieldLengthUnbuffered(MSSQL_UNBUFFERED_QUERY_RESULT *pResult, int iColumn)
{
   LONG nLen = -1;
   if (pResult != NULL)
   {
      if ((iColumn < pResult->numColumns) && (iColumn >= 0))
         nLen = (pResult->data[iColumn] != NULL) ? (LONG)wcslen(pResult->data[iColumn]) : 0;
   }
   return nLen;
}

/**
 * Get field from current row in unbuffered query result
 */
extern "C" WCHAR EXPORT *DrvGetFieldUnbuffered(MSSQL_UNBUFFERED_QUERY_RESULT *pResult, int iColumn, WCHAR *pBuffer, int iBufSize)
{
   // Check if we have valid result handle
   if (pResult == NULL)
      return NULL;

   // Check if there are valid fetched row
   if (pResult->noMoreRows)
      return NULL;

   if ((iColumn >= 0) && (iColumn < pResult->numColumns))
   {
      if (pResult->data[iColumn] != NULL)
      {
         wcsncpy(pBuffer, pResult->data[iColumn], iBufSize);
         pBuffer[iBufSize - 1] = 0;
      }
      else
      {
         pBuffer[0] = 0;
      }
   }
   else
   {
      pBuffer[0] = 0;
   }
   return pBuffer;
}

/**
 * Get column count in unbuffered query result
 */
extern "C" int EXPORT DrvGetColumnCountUnbuffered(MSSQL_UNBUFFERED_QUERY_RESULT *pResult)
{
	return (pResult != NULL) ? pResult->numColumns : 0;
}

/**
 * Get column name in unbuffered query result
 */
extern "C" const char EXPORT *DrvGetColumnNameUnbuffered(MSSQL_UNBUFFERED_QUERY_RESULT *pResult, int column)
{
	return ((pResult != NULL) && (column >= 0) && (column < pResult->numColumns)) ? pResult->columnNames[column] : NULL;
}

/**
 * Destroy result of unbuffered query
 */
extern "C" void EXPORT DrvFreeUnbufferedResult(MSSQL_UNBUFFERED_QUERY_RESULT *pResult)
{
   if (pResult == NULL)
      return;

   if (pResult->isPrepared)
      SQLCloseCursor(pResult->sqlStatement);
   else
      SQLFreeHandle(SQL_HANDLE_STMT, pResult->sqlStatement);
   MutexUnlock(pResult->pConn->mutexQuery);
   for(int i = 0; i < pResult->numColumns; i++)
   {
      free(pResult->data[i]);
      free(pResult->columnNames[i]);
   }
   free(pResult->data);
   free(pResult->columnNames);
   free(pResult);
}

/**
 * Begin transaction
 */
extern "C" DWORD EXPORT DrvBegin(MSSQL_CONN *pConn)
{
   SQLRETURN nRet;
   DWORD dwResult;

	if (pConn == NULL)
      return DBERR_INVALID_HANDLE;

	MutexLock(pConn->mutexQuery);
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

/**
 * Commit transaction
 */
extern "C" DWORD EXPORT DrvCommit(MSSQL_CONN *pConn)
{
   SQLRETURN nRet;

	if (pConn == NULL)
      return DBERR_INVALID_HANDLE;

	MutexLock(pConn->mutexQuery);
   nRet = SQLEndTran(SQL_HANDLE_DBC, pConn->sqlConn, SQL_COMMIT);
   SQLSetConnectAttr(pConn->sqlConn, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
	MutexUnlock(pConn->mutexQuery);
   return ((nRet == SQL_SUCCESS) || (nRet == SQL_SUCCESS_WITH_INFO)) ? DBERR_SUCCESS : DBERR_OTHER_ERROR;
}

/**
 * Rollback transaction
 */
extern "C" DWORD EXPORT DrvRollback(MSSQL_CONN *pConn)
{
   SQLRETURN nRet;

	if (pConn == NULL)
      return DBERR_INVALID_HANDLE;

	MutexLock(pConn->mutexQuery);
   nRet = SQLEndTran(SQL_HANDLE_DBC, pConn->sqlConn, SQL_ROLLBACK);
   SQLSetConnectAttr(pConn->sqlConn, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
	MutexUnlock(pConn->mutexQuery);
   return ((nRet == SQL_SUCCESS) || (nRet == SQL_SUCCESS_WITH_INFO)) ? DBERR_SUCCESS : DBERR_OTHER_ERROR;
}

/**
 * Check if table exist
 */
extern "C" int EXPORT DrvIsTableExist(MSSQL_CONN *pConn, const WCHAR *name)
{
   WCHAR query[256];
   swprintf(query, 256, L"SELECT count(*) FROM sysobjects WHERE xtype='U' AND upper(name)=upper('%ls')", name);
   DWORD error;
   WCHAR errorText[DBDRV_MAX_ERROR_TEXT];
   int rc = DBIsTableExist_Failure;
   MSSQL_QUERY_RESULT *hResult = (MSSQL_QUERY_RESULT *)DrvSelect(pConn, query, &error, errorText);
   if (hResult != NULL)
   {
      WCHAR buffer[64] = L"";
      DrvGetField(hResult, 0, 0, buffer, 64);
      rc = (wcstol(buffer, NULL, 10) > 0) ? DBIsTableExist_Found : DBIsTableExist_NotFound;
      DrvFreeResult(hResult);
   }
   return rc;
}

/**
 * DLL Entry point
 */
bool WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return true;
}
