/* 
** MS SQL Database Driver
** Copyright (C) 2004-2025 Victor Kirhenshtein
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

/**
 * Convert ODBC state to NetXMS database error code and get error text
 */
static DWORD GetSQLErrorInfo(SQLSMALLINT nHandleType, SQLHANDLE hHandle, WCHAR *errorText)
{
   DWORD dwError;

	// Get state information and convert it to NetXMS database error code
   char szState[16];
   SQLSMALLINT nChars;
   SQLRETURN nRet = SQLGetDiagFieldA(nHandleType, hHandle, 1, SQL_DIAG_SQLSTATE, szState, 16, &nChars);
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
static StringBuffer PrepareString(const TCHAR *str, size_t maxSize)
{
   StringBuffer out;
   out.append(_T('\''));
   for (const TCHAR *src = str; (*src != 0) && (maxSize > 0); src++, maxSize--)
   {
      if (*src == _T('\''))
         out.append(_T("''"), 2);
      else
         out.append(*src);
   }
   out.append(_T('\''));
   return out;
}

/**
 * Selected ODBC driver
 */
static char s_driver[SQL_MAX_DSN_LENGTH + 1] = "SQL Native Client";

/**
 * Supported drivers in priority order
 */
static const char *s_supportedDrivers[] =
{
   "SQL Native Client",
   "SQL Server Native Client 10.0",
   "SQL Server Native Client 11.0",
   "ODBC Driver 13 for SQL Server",
   "ODBC Driver 17 for SQL Server",
   "ODBC Driver 18 for SQL Server",
   nullptr
};

/**
 * Trust certificate option
 */
static bool s_trustServerCertificate = true;

/**
 * Initialize driver
 */
static bool Initialize(const char *options)
{
   // Allocate environment
	SQLHENV sqlEnv;
   long rc = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlEnv);
	if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO))
		return false;

	rc = SQLSetEnvAttr(sqlEnv, SQL_ATTR_ODBC_VERSION, (void *)SQL_OV_ODBC3, 0);
	if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO))
		return false;

   s_trustServerCertificate = ExtractNamedOptionValueAsBoolA(options, "trustServerCertificate", true);

	// Find correct driver
   char driver[SQL_MAX_DSN_LENGTH + 1] = "";
   ExtractNamedOptionValueA(options, "driver", driver, sizeof(driver));
   if (driver[0] == 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Enumerating available ODBC drivers:"));
      int selectedDriverIndex = 0;
      char name[SQL_MAX_DSN_LENGTH + 1], attrs[1024];
      SQLSMALLINT l1, l2;
      rc = SQLDriversA(sqlEnv, SQL_FETCH_FIRST, (SQLCHAR *)name, SQL_MAX_DSN_LENGTH + 1, &l1, (SQLCHAR *)attrs, 1024, &l2);
      while ((rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO))
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("   Found ODBC driver \"%hs\""), name);
         for (int i = 0; s_supportedDrivers[i] != nullptr; i++)
         {
            if (!strcmp(name, s_supportedDrivers[i]))
            {
               if (i > selectedDriverIndex)
                  selectedDriverIndex = i;
               break;
            }
         }
         rc = SQLDriversA(sqlEnv, SQL_FETCH_NEXT, (SQLCHAR *)name, SQL_MAX_DSN_LENGTH + 1, &l1, (SQLCHAR *)attrs, 1024, &l2);
      }
      strcpy(s_driver, s_supportedDrivers[selectedDriverIndex]);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("ODBC driver provided in configuration"));
      strcpy(s_driver, driver);
   }
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Selected ODBC driver \"%hs\""), s_driver);

   SQLFreeHandle(SQL_HANDLE_ENV, sqlEnv);
   return true;
}

/**
 * Unload handler
 */
static void Unload()
{
}

/**
 * Connect to database
 */
static DBDRV_CONNECTION Connect(const char *host, const char *login, const char *password, const char *database, const char *schema, WCHAR *errorText)
{
   MSSQL_CONN *pConn = MemAllocStruct<MSSQL_CONN>();

   // Allocate environment
   long iResult = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &pConn->sqlEnv);
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
      snprintf(connectString, 1024, "DRIVER={%s};Server=%s;TrustServerCertificate=%s;Trusted_Connection=yes;Database=%s;APP=NetXMS", s_driver, host, s_trustServerCertificate ? "yes" : "no", (database != nullptr) ? database : "master");
	}
	else
	{
		snprintf(connectString, 1024, "DRIVER={%s};Server=%s;TrustServerCertificate=%s;UID=%s;PWD=%s;Database=%s;APP=NetXMS", s_driver, host, s_trustServerCertificate ? "yes" : "no", login, password, (database != nullptr) ? database : "master");
	}
	iResult = SQLDriverConnectA(pConn->sqlConn, nullptr, (SQLCHAR *)connectString, SQL_NTS, nullptr, 0, &outLen, SQL_DRIVER_NOPROMPT);

	if ((iResult != SQL_SUCCESS) && (iResult != SQL_SUCCESS_WITH_INFO))
	{
		GetSQLErrorInfo(SQL_HANDLE_DBC, pConn->sqlConn, errorText);
      goto connect_failure_2;
	}

   // Create mutex
   pConn->mutexQuery = new Mutex(MutexType::NORMAL);

   // Success
   return (DBDRV_CONNECTION)pConn;

   // Handle failures
connect_failure_2:
   SQLFreeHandle(SQL_HANDLE_DBC, pConn->sqlConn);

connect_failure_1:
   SQLFreeHandle(SQL_HANDLE_ENV, pConn->sqlEnv);

connect_failure_0:
   MemFree(pConn);
   return nullptr;
}

/**
 * Disconnect from database
 */
static void Disconnect(DBDRV_CONNECTION connection)
{
   auto c = static_cast<MSSQL_CONN*>(connection);
   c->mutexQuery->lock();
   c->mutexQuery->unlock();
   SQLDisconnect(c->sqlConn);
   SQLFreeHandle(SQL_HANDLE_DBC, c->sqlConn);
   SQLFreeHandle(SQL_HANDLE_ENV, c->sqlEnv);
   delete c->mutexQuery;
   MemFree(c);
}

/**
 * Prepare statement
 */
static DBDRV_STATEMENT Prepare(DBDRV_CONNECTION connection, const WCHAR *query, bool optimizeForReuse, uint32_t *errorCode, WCHAR *errorText)
{
	SQLHSTMT stmt;
	MSSQL_STATEMENT *result;

   static_cast<MSSQL_CONN*>(connection)->mutexQuery->lock();

   // Allocate statement handle
   SQLRETURN iResult = SQLAllocHandle(SQL_HANDLE_STMT, static_cast<MSSQL_CONN*>(connection)->sqlConn, &stmt);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
   {
      // Prepare statement
	   iResult = SQLPrepareW(stmt, (SQLWCHAR *)query, SQL_NTS);
	   if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
		{
         result = MemAllocStruct<MSSQL_STATEMENT>();
			result->handle = stmt;
			result->buffers = new Array(0, 16, Ownership::True);
			result->connection = static_cast<MSSQL_CONN*>(connection);
			*errorCode = DBERR_SUCCESS;
		}
		else
      {
         *errorCode = GetSQLErrorInfo(SQL_HANDLE_STMT, stmt, errorText);
	      SQLFreeHandle(SQL_HANDLE_STMT, stmt);
			result = nullptr;
      }
   }
   else
   {
      *errorCode = GetSQLErrorInfo(SQL_HANDLE_DBC, static_cast<MSSQL_CONN*>(connection)->sqlConn, errorText);
		result = nullptr;
   }

   static_cast<MSSQL_CONN*>(connection)->mutexQuery->unlock();
   return result;
}

/**
 * Bind parameter to statement
 */
static void Bind(DBDRV_STATEMENT hStmt, int pos, int sqlType, int cType, void *buffer, int allocType)
{
	static SQLSMALLINT odbcSqlType[] = { SQL_VARCHAR, SQL_INTEGER, SQL_BIGINT, SQL_DOUBLE, SQL_LONGVARCHAR };
	static SQLSMALLINT odbcCType[] = { SQL_C_WCHAR, SQL_C_SLONG, SQL_C_ULONG, SQL_C_SBIGINT, SQL_C_UBIGINT, SQL_C_DOUBLE, SQL_C_WCHAR };
	static size_t bufferSize[] = { 0, sizeof(int32_t), sizeof(uint32_t), sizeof(int64_t), sizeof(uint64_t), sizeof(double), 0 };

   auto stmt = static_cast<MSSQL_STATEMENT*>(hStmt);
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
            MemFree(buffer);
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
            sqlBuffer = MemCopyBlock(buffer, (cType == DB_CTYPE_STRING) ? (DWORD)(length * sizeof(WCHAR)) : bufferSize[cType]);
         }
         stmt->buffers->add(sqlBuffer);
         break;
      default:
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Invalid bind allocation type %d for position %d"), allocType, pos);
         return;	// Invalid call
   }

   SQLRETURN rc = SQLBindParameter(stmt->handle, pos, SQL_PARAM_INPUT, odbcCType[cType], odbcSqlType[sqlType], length, 0, sqlBuffer, 0, nullptr);
   if (rc != SQL_SUCCESS)
   {
      TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
      GetSQLErrorInfo(SQL_HANDLE_STMT, stmt->handle, errorText);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Call to SQLBindParameter failed: %s"), errorText);
   }
}

/**
 * Execute prepared statement
 */
static uint32_t Execute(DBDRV_CONNECTION connection, DBDRV_STATEMENT hStmt, WCHAR *errorText)
{
   uint32_t dwResult;

   static_cast<MSSQL_CONN*>(connection)->mutexQuery->lock();
   SQLHSTMT handle = static_cast<MSSQL_STATEMENT*>(hStmt)->handle;
	long rc = SQLExecute(handle);
   if ((rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO) || (rc == SQL_NO_DATA))
   {
		ClearPendingResults(handle);
      dwResult = DBERR_SUCCESS;
   }
   else
   {
		dwResult = GetSQLErrorInfo(SQL_HANDLE_STMT, handle, errorText);
   }
   static_cast<MSSQL_CONN*>(connection)->mutexQuery->unlock();
	return dwResult;
}

/**
 * Destroy prepared statement
 */
static void FreeStatement(DBDRV_STATEMENT hStmt)
{
   auto stmt = static_cast<MSSQL_STATEMENT*>(hStmt);
	stmt->connection->mutexQuery->lock();
	SQLFreeHandle(SQL_HANDLE_STMT, stmt->handle);
	stmt->connection->mutexQuery->unlock();
	delete stmt->buffers;
	MemFree(stmt);
}

/**
 * Perform non-SELECT query
 */
static uint32_t Query(DBDRV_CONNECTION connection, const WCHAR *pwszQuery, WCHAR *errorText)
{
   uint32_t dwResult;

   static_cast<MSSQL_CONN*>(connection)->mutexQuery->lock();

   // Allocate statement handle
   SQLHSTMT sqlStatement;
   SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_STMT, static_cast<MSSQL_CONN*>(connection)->sqlConn, &sqlStatement);
	if ((rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO))
   {
      // Execute statement
      rc = SQLExecDirectW(sqlStatement, (SQLWCHAR *)pwszQuery, SQL_NTS);
	   if ((rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO) || (rc == SQL_NO_DATA))
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
      dwResult = GetSQLErrorInfo(SQL_HANDLE_DBC, static_cast<MSSQL_CONN*>(connection)->sqlConn, errorText);
   }

   static_cast<MSSQL_CONN*>(connection)->mutexQuery->unlock();
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
      result = MemCopyStringW(buffer);
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
   return (result != NULL) ? result : MemCopyStringW(L"");
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
			pResult->columnNames[i] = MemCopyStringA(name);
		}
		else
		{
			pResult->columnNames[i] = MemCopyStringA("");
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
static DBDRV_RESULT Select(DBDRV_CONNECTION connection, const WCHAR *query, uint32_t *errorCode, WCHAR *errorText)
{
   MSSQL_QUERY_RESULT *pResult = NULL;

   static_cast<MSSQL_CONN*>(connection)->mutexQuery->lock();

   // Allocate statement handle
   SQLHSTMT sqlStatement;
   SQLRETURN iResult = SQLAllocHandle(SQL_HANDLE_STMT, static_cast<MSSQL_CONN*>(connection)->sqlConn, &sqlStatement);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
   {
      // Execute statement
      iResult = SQLExecDirectW(sqlStatement, (SQLWCHAR *)query, SQL_NTS);
	   if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
      {
			pResult = ProcessSelectResults(sqlStatement);
		   *errorCode = DBERR_SUCCESS;
      }
      else
      {
         *errorCode = GetSQLErrorInfo(SQL_HANDLE_STMT, sqlStatement, errorText);
      }
      SQLFreeHandle(SQL_HANDLE_STMT, sqlStatement);
   }
   else
   {
      *errorCode = GetSQLErrorInfo(SQL_HANDLE_DBC, static_cast<MSSQL_CONN*>(connection)->sqlConn, errorText);
   }

   static_cast<MSSQL_CONN*>(connection)->mutexQuery->unlock();
   return pResult;
}

/**
 * Perform SELECT query using prepared statement
 */
static DBDRV_RESULT SelectPrepared(DBDRV_CONNECTION connection, DBDRV_STATEMENT hStmt, uint32_t *errorCode, WCHAR *errorText)
{
   MSSQL_QUERY_RESULT *pResult = NULL;

   static_cast<MSSQL_CONN*>(connection)->mutexQuery->lock();
   SQLHSTMT handle = static_cast<MSSQL_STATEMENT*>(hStmt)->handle;
	long rc = SQLExecute(handle);
   if ((rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO))
   {
		pResult = ProcessSelectResults(handle);
	   *errorCode = DBERR_SUCCESS;
   }
   else
   {
		*errorCode = GetSQLErrorInfo(SQL_HANDLE_STMT, handle, errorText);
   }
   static_cast<MSSQL_CONN*>(connection)->mutexQuery->unlock();
	return pResult;
}

/**
 * Get field length from result
 */
static int32_t GetFieldLength(DBDRV_RESULT hResult, int iRow, int iColumn)
{
   auto result = static_cast<MSSQL_QUERY_RESULT*>(hResult);
   int32_t nLen = -1;
   if ((iRow < result->numRows) && (iRow >= 0) &&
         (iColumn < result->numColumns) && (iColumn >= 0))
      nLen = (int32_t)wcslen(result->pValues[iRow * result->numColumns + iColumn]);
   return nLen;
}

/**
 * Get field value from result
 */
static WCHAR *GetField(DBDRV_RESULT hResult, int iRow, int iColumn, WCHAR *pBuffer, int nBufSize)
{
   auto result = static_cast<MSSQL_QUERY_RESULT*>(hResult);
   WCHAR *pValue = NULL;
   if ((iRow < result->numRows) && (iRow >= 0) &&
         (iColumn < result->numColumns) && (iColumn >= 0))
   {
      wcsncpy_s(pBuffer, nBufSize, result->pValues[iRow * result->numColumns + iColumn], _TRUNCATE);
      pValue = pBuffer;
   }
   return pValue;
}

/**
 * Get number of rows in result
 */
static int GetNumRows(DBDRV_RESULT hResult)
{
   return static_cast<MSSQL_QUERY_RESULT*>(hResult)->numRows;
}

/**
 * Get column count in query result
 */
static int GetColumnCount(DBDRV_RESULT hResult)
{
	return static_cast<MSSQL_QUERY_RESULT*>(hResult)->numColumns;
}

/**
 * Get column name in query result
 */
static const char *GetColumnName(DBDRV_RESULT hResult, int column)
{
	return ((column >= 0) && (column < static_cast<MSSQL_QUERY_RESULT*>(hResult)->numColumns)) ? static_cast<MSSQL_QUERY_RESULT*>(hResult)->columnNames[column] : nullptr;
}

/**
 * Free SELECT results
 */
static void FreeResult(DBDRV_RESULT hResult)
{
   auto result = static_cast<MSSQL_QUERY_RESULT*>(hResult);
   int count = result->numColumns * result->numRows;
   for(int i = 0; i < count; i++)
      MemFree(result->pValues[i]);
   MemFree(result->pValues);

	for(int i = 0; i < result->numColumns; i++)
		MemFree(result->columnNames[i]);
	MemFree(result->columnNames);

   MemFree(result);
}

/**
 * Perform unbuffered SELECT query
 */
static DBDRV_UNBUFFERED_RESULT SelectUnbuffered(DBDRV_CONNECTION connection, const WCHAR *pwszQuery, uint32_t *pdwError, WCHAR *errorText)
{
   MSSQL_UNBUFFERED_QUERY_RESULT *pResult = NULL;

   static_cast<MSSQL_CONN*>(connection)->mutexQuery->lock();

   // Allocate statement handle
   SQLHSTMT sqlStatement;
   SQLRETURN iResult = SQLAllocHandle(SQL_HANDLE_STMT, static_cast<MSSQL_CONN*>(connection)->sqlConn, &sqlStatement);
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
         pResult->pConn = static_cast<MSSQL_CONN*>(connection);
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
					pResult->columnNames[i] = MemCopyStringA(name);
				}
				else
				{
					pResult->columnNames[i] = MemCopyStringA("");
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
      *pdwError = GetSQLErrorInfo(SQL_HANDLE_DBC, static_cast<MSSQL_CONN*>(connection)->sqlConn, errorText);
   }

   if (pResult == nullptr) // Unlock mutex if query has failed
      static_cast<MSSQL_CONN*>(connection)->mutexQuery->unlock();
   return pResult;
}

/**
 * Perform unbuffered SELECT query using prepared statement
 */
static DBDRV_UNBUFFERED_RESULT SelectPreparedUnbuffered(DBDRV_CONNECTION connection, DBDRV_STATEMENT hStmt, uint32_t *errorCode, WCHAR *errorText)
{
   MSSQL_UNBUFFERED_QUERY_RESULT *pResult = nullptr;

   static_cast<MSSQL_CONN*>(connection)->mutexQuery->lock();
   SQLHSTMT handle = static_cast<MSSQL_STATEMENT*>(hStmt)->handle;
	SQLRETURN rc = SQLExecute(handle);
   if ((rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO))
   {
      // Allocate result buffer and determine column info
      pResult = MemAllocStruct<MSSQL_UNBUFFERED_QUERY_RESULT>();
      pResult->sqlStatement = handle;
      pResult->isPrepared = true;
      
      short wNumCols;
      SQLNumResultCols(pResult->sqlStatement, &wNumCols);
      pResult->numColumns = wNumCols;
      pResult->pConn = static_cast<MSSQL_CONN*>(connection);
      pResult->noMoreRows = false;
		pResult->data = MemAllocArray<WCHAR*>(pResult->numColumns);

		// Get column names
		pResult->columnNames = MemAllocArray<char*>(pResult->numColumns);
		for(int i = 0; i < pResult->numColumns; i++)
		{
			char name[256];
			SQLSMALLINT len;

			rc = SQLColAttributeA(pResult->sqlStatement, (SQLSMALLINT)(i + 1), SQL_DESC_NAME, name, 256, &len, nullptr); 
			if ((rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO))
			{
				name[len] = 0;
				pResult->columnNames[i] = MemCopyStringA(name);
			}
			else
			{
				pResult->columnNames[i] = MemCopyStringA("");
			}
		}
	   *errorCode = DBERR_SUCCESS;
   }
   else
   {
		*errorCode = GetSQLErrorInfo(SQL_HANDLE_STMT, handle, errorText);
   }

   if (pResult == nullptr) // Unlock mutex if query has failed
      static_cast<MSSQL_CONN*>(connection)->mutexQuery->unlock();
	return pResult;
}

/**
 * Fetch next result line from unbuffered SELECT results
 */
static bool Fetch(DBDRV_UNBUFFERED_RESULT hResult)
{
   auto result = static_cast<MSSQL_UNBUFFERED_QUERY_RESULT*>(hResult);
   bool success = false;
   SQLRETURN iResult = SQLFetch(result->sqlStatement);
   success = ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO));
   if (success)
   {
      for(int i = 0; i < result->numColumns; i++)
      {
         MemFree(result->data[i]);
         result->data[i] = GetFieldData(result->sqlStatement, (short)i + 1);
      }
   }
   else
   {
      result->noMoreRows = true;
   }
   return success;
}

/**
 * Get field length from unbuffered query result
 */
static int32_t GetFieldLengthUnbuffered(DBDRV_UNBUFFERED_RESULT hResult, int iColumn)
{
   auto result = static_cast<MSSQL_UNBUFFERED_QUERY_RESULT*>(hResult);
   int32_t nLen = -1;
   if ((iColumn < result->numColumns) && (iColumn >= 0))
      nLen = (result->data[iColumn] != NULL) ? (int32_t)wcslen(result->data[iColumn]) : 0;
   return nLen;
}

/**
 * Get field from current row in unbuffered query result
 */
static WCHAR *GetFieldUnbuffered(DBDRV_UNBUFFERED_RESULT hResult, int iColumn, WCHAR *pBuffer, int iBufSize)
{
   auto result = static_cast<MSSQL_UNBUFFERED_QUERY_RESULT*>(hResult);

   // Check if there are valid fetched row
   if (result->noMoreRows)
      return NULL;

   if ((iColumn >= 0) && (iColumn < result->numColumns))
   {
      if (result->data[iColumn] != NULL)
      {
         wcsncpy(pBuffer, result->data[iColumn], iBufSize);
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
static int GetColumnCountUnbuffered(DBDRV_UNBUFFERED_RESULT hResult)
{
	return static_cast<MSSQL_UNBUFFERED_QUERY_RESULT*>(hResult)->numColumns;
}

/**
 * Get column name in unbuffered query result
 */
static const char *GetColumnNameUnbuffered(DBDRV_UNBUFFERED_RESULT hResult, int column)
{
	return ((column >= 0) && (column < static_cast<MSSQL_UNBUFFERED_QUERY_RESULT*>(hResult)->numColumns)) ? static_cast<MSSQL_UNBUFFERED_QUERY_RESULT*>(hResult)->columnNames[column] : nullptr;
}

/**
 * Destroy result of unbuffered query
 */
static void FreeUnbufferedResult(DBDRV_UNBUFFERED_RESULT hResult)
{
   auto result = static_cast<MSSQL_UNBUFFERED_QUERY_RESULT*>(hResult);

   if (result->isPrepared)
      SQLCloseCursor(result->sqlStatement);
   else
      SQLFreeHandle(SQL_HANDLE_STMT, result->sqlStatement);
   result->pConn->mutexQuery->unlock();
   for(int i = 0; i < result->numColumns; i++)
   {
      MemFree(result->data[i]);
      MemFree(result->columnNames[i]);
   }
   MemFree(result->data);
   MemFree(result->columnNames);
   MemFree(result);
}

/**
 * Begin transaction
 */
static uint32_t Begin(DBDRV_CONNECTION connection)
{
   uint32_t dwResult;
   static_cast<MSSQL_CONN*>(connection)->mutexQuery->lock();
   SQLRETURN nRet = SQLSetConnectAttr(static_cast<MSSQL_CONN*>(connection)->sqlConn, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0);
   if ((nRet == SQL_SUCCESS) || (nRet == SQL_SUCCESS_WITH_INFO))
   {
      dwResult = DBERR_SUCCESS;
   }
   else
   {
      dwResult = GetSQLErrorInfo(SQL_HANDLE_DBC, static_cast<MSSQL_CONN*>(connection)->sqlConn, NULL);
   }
   static_cast<MSSQL_CONN*>(connection)->mutexQuery->unlock();
   return dwResult;
}

/**
 * Commit transaction
 */
static uint32_t Commit(DBDRV_CONNECTION connection)
{
   static_cast<MSSQL_CONN*>(connection)->mutexQuery->lock();
   SQLRETURN nRet = SQLEndTran(SQL_HANDLE_DBC, static_cast<MSSQL_CONN*>(connection)->sqlConn, SQL_COMMIT);
   SQLSetConnectAttr(static_cast<MSSQL_CONN*>(connection)->sqlConn, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
   static_cast<MSSQL_CONN*>(connection)->mutexQuery->unlock();
   return ((nRet == SQL_SUCCESS) || (nRet == SQL_SUCCESS_WITH_INFO)) ? DBERR_SUCCESS : DBERR_OTHER_ERROR;
}

/**
 * Rollback transaction
 */
static uint32_t Rollback(DBDRV_CONNECTION connection)
{
   static_cast<MSSQL_CONN*>(connection)->mutexQuery->lock();
   SQLRETURN nRet = SQLEndTran(SQL_HANDLE_DBC, static_cast<MSSQL_CONN*>(connection)->sqlConn, SQL_ROLLBACK);
   SQLSetConnectAttr(static_cast<MSSQL_CONN*>(connection)->sqlConn, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
   static_cast<MSSQL_CONN*>(connection)->mutexQuery->unlock();
   return ((nRet == SQL_SUCCESS) || (nRet == SQL_SUCCESS_WITH_INFO)) ? DBERR_SUCCESS : DBERR_OTHER_ERROR;
}

/**
 * Check if table exist
 */
static int IsTableExist(DBDRV_CONNECTION connection, const WCHAR *name)
{
   WCHAR query[256];
   swprintf(query, 256, L"SELECT count(*) FROM sysobjects WHERE xtype='U' AND upper(name)=upper('%ls')", name);
   uint32_t error;
   WCHAR errorText[DBDRV_MAX_ERROR_TEXT];
   int rc = DBIsTableExist_Failure;
   DBDRV_RESULT hResult = Select(connection, query, &error, errorText);
   if (hResult != nullptr)
   {
      WCHAR buffer[64] = L"";
      GetField(hResult, 0, 0, buffer, 64);
      rc = (wcstol(buffer, nullptr, 10) > 0) ? DBIsTableExist_Found : DBIsTableExist_NotFound;
      FreeResult(hResult);
   }
   return rc;
}

/**
 * Driver call table
 */
static DBDriverCallTable s_callTable =
{
   Initialize,
   Connect,
   Disconnect,
   nullptr, // SetPrefetchLimit
   Prepare,
   FreeStatement,
   nullptr, // OpenBatch
   nullptr, // NextBatchRow
   Bind,
   Execute,
   Query,
   Select,
   SelectUnbuffered,
   SelectPrepared,
   SelectPreparedUnbuffered,
   Fetch,
   GetFieldLength,
   GetFieldLengthUnbuffered,
   GetField,
   nullptr, // GetFieldUTF8
   GetFieldUnbuffered,
   nullptr, // GetFieldUnbufferedUTF8
   GetNumRows,
   FreeResult,
   FreeUnbufferedResult,
   Begin,
   Commit,
   Rollback,
   Unload,
   GetColumnCount,
   GetColumnName,
   GetColumnCountUnbuffered,
   GetColumnNameUnbuffered,
   PrepareString,
   IsTableExist
};

DB_DRIVER_ENTRY_POINT("MSSQL", s_callTable)

/**
 * DLL Entry point
 */
bool WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return true;
}
