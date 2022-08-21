/* 
** ODBC Database Driver
** Copyright (C) 2004-2022 Victor Kirhenshtein
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
** File: informix.cpp
**
**/

#include "informixdrv.h"

/**
 * Data buffer size
 */
#define DATA_BUFFER_SIZE      65536

/**
 * Convert INFORMIX state to NetXMS database error code and get error text
 */
static uint32_t GetSQLErrorInfo(SQLSMALLINT nHandleType, SQLHANDLE hHandle, WCHAR *errorText)
{
	// Get state information and convert it to NetXMS database error code
   uint32_t errorCode;
   SQLSMALLINT nChars;
   SQLWCHAR sqlState[32];

   SQLRETURN nRet = SQLGetDiagFieldW(nHandleType, hHandle, 1, SQL_DIAG_SQLSTATE, sqlState, 16, &nChars);
   if (nRet == SQL_SUCCESS)
   {
		if (
			(!wcscmp(sqlState, L"08003")) ||	// Connection does not exist
			(!wcscmp(sqlState, L"08S01")) ||	// Communication link failure
			(!wcscmp(sqlState, L"HYT00")) ||	// Timeout expired
			(!wcscmp(sqlState, L"HYT01")) ||	// Connection timeout expired
			(!wcscmp(sqlState, L"08506")))	// SQL30108N: A connection failed but has been re-established.
      {
         errorCode = DBERR_CONNECTION_LOST;
      }
      else
      {
         errorCode = DBERR_OTHER_ERROR;
      }
   }
   else
   {
      errorCode = DBERR_OTHER_ERROR;
   }

	// Get error message
	if (errorText != nullptr)
	{
		errorText[0] = L'[';
		wcscpy(&errorText[1], sqlState);
		int pos = (int)wcslen(errorText);
		errorText[pos++] = L']';
		errorText[pos++] = L' ';
		nRet = SQLGetDiagFieldW(nHandleType, hHandle, 1, SQL_DIAG_MESSAGE_TEXT, &errorText[pos], DBDRV_MAX_ERROR_TEXT - pos, &nChars);
		if (nRet == SQL_SUCCESS)
		{
			RemoveTrailingCRLFW(errorText);
		}
		else
		{
			wcscpy(&errorText[pos], L"Unable to obtain description for this error");
		}
   }

   return errorCode;
}

/**
 * Clear any pending result sets on given statement
 */
static void ClearPendingResults(SQLHSTMT statement)
{
	while(1)
	{
		SQLRETURN rc = SQLMoreResults(statement);
		if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO))
			break;
	}
}

/**
 * Prepare string for using in SQL query - enclose in quotes and escape as needed
 */
static StringBuffer PrepareString(const NETXMS_TCHAR *str, size_t maxSize)
{
   StringBuffer out;
   out.append(_T('\''));
   for(const NETXMS_TCHAR *src = str; (*src != 0) && (maxSize > 0); src++, maxSize--)
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
 * Initialize driver
 */
static bool Initialize(const char *options)
{
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
 * database should be set to Informix source name. Host and schema are ignored
 */
static DBDRV_CONNECTION Connect(const char *host, const char *login, const char *password, const char *database, const char *schema, WCHAR *errorText)
{
	long iResult;

	// Allocate our connection structure
	auto pConn = MemAllocStruct<INFORMIX_CONN>();

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

	// Connect to the database 
	// Server name could be in form host[:port][@proto][/service] or * to indicate ODBC DSN
	if ((host != NULL) && (*host != 0) && strcmp(host, "*"))
	{
      char hostName[256];
      strlcpy(hostName, host, 256);

      char *serverName = strchr(hostName, '/');
      if (serverName != NULL)
      {
         *serverName = 0;
         serverName++;
      }
      else
      {
         serverName = hostName;
      }

      char *proto = strchr(hostName, '@');
      if (proto != NULL)
      {
         *proto = 0;
         proto++;
      }

      char *port = strchr(hostName, ':');
      if (port != NULL)
      {
         *port = 0;
         port++;
      }

      SQLSMALLINT outLen;
#ifdef _WIN32
      WCHAR connectString[1024];
      snwprintf(connectString, 1024, L"Host=%hs;Server=%hs;Protocol=%hs;Service=%hs;DB=%hs;UID=%hs;PWD=%hs", hostName, serverName,
               (proto != NULL) ? proto : "onsoctcp", (port != NULL) ? port : "9088", database, login, password);
      iResult = SQLDriverConnectW(pConn->sqlConn, NULL, (SQLWCHAR *)connectString, SQL_NTS, NULL, 0, &outLen, SQL_DRIVER_NOPROMPT);
#else
      char connectString[1024];
      snprintf(connectString, 1024, "Host=%s;Server=%s;Protocol=%s;Service=%s;DB=%s;UID=%s;PWD=%s", hostName, serverName,
               (proto != NULL) ? proto : "onsoctcp", (port != NULL) ? port : "9088", database, login, password);
      iResult = SQLDriverConnect(pConn->sqlConn, NULL, (SQLCHAR *)connectString, SQL_NTS, NULL, 0, &outLen, SQL_DRIVER_NOPROMPT);
#endif
	}
	else
	{
      SQLSMALLINT outLen;
#ifdef _WIN32
      WCHAR connectString[1024];
      snwprintf(connectString, 1024, L"DSN=%hs;UID=%hs;PWD=%hs", database, login, password);
      iResult = SQLDriverConnectW(pConn->sqlConn, NULL, (SQLWCHAR *)connectString, SQL_NTS, NULL, 0, &outLen, SQL_DRIVER_NOPROMPT);
#else
      char connectString[1024];
      snprintf(connectString, 1024, "DSN=%s;UID=%s;PWD=%s", database, login, password);
      iResult = SQLDriverConnect(pConn->sqlConn, NULL, (SQLCHAR *)connectString, SQL_NTS, NULL, 0, &outLen, SQL_DRIVER_NOPROMPT);
#endif
	}
	if ((iResult != SQL_SUCCESS) && (iResult != SQL_SUCCESS_WITH_INFO))
	{
		GetSQLErrorInfo(SQL_HANDLE_DBC, pConn->sqlConn, errorText);
      goto connect_failure_2;
	}

   // Create mutex
   pConn->mutexQuery = new Mutex(MutexType::NORMAL);

   // Success
   return pConn;

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
   auto c = static_cast<INFORMIX_CONN*>(connection);
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
static DBDRV_STATEMENT Prepare(DBDRV_CONNECTION connection, const WCHAR *query, bool optimizeForReuse, uint32_t *pdwError, WCHAR *errorText)
{
	INFORMIX_STATEMENT *result;

   static_cast<INFORMIX_CONN*>(connection)->mutexQuery->lock();

	// Allocate statement handle
   SQLHSTMT statement;
   SQLRETURN iResult = SQLAllocHandle(SQL_HANDLE_STMT, static_cast<INFORMIX_CONN*>(connection)->sqlConn, &statement);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
	{
		// Prepare statement
		iResult = SQLPrepareW(statement, (SQLWCHAR *)query, SQL_NTS);
		if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
		{
         result = MemAllocStruct<INFORMIX_STATEMENT>();
			result->handle = statement;
			result->buffers = new Array(0, 16, Ownership::True);
			result->connection = static_cast<INFORMIX_CONN*>(connection);
			*pdwError = DBERR_SUCCESS;
		}
		else
		{
			*pdwError = GetSQLErrorInfo(SQL_HANDLE_STMT, statement, errorText);
			SQLFreeHandle(SQL_HANDLE_STMT, statement);
			result = nullptr;
		}
	}
	else
	{
		*pdwError = GetSQLErrorInfo(SQL_HANDLE_DBC, static_cast<INFORMIX_CONN*>(connection)->sqlConn, errorText);
		result = nullptr;
	}

   static_cast<INFORMIX_CONN*>(connection)->mutexQuery->unlock();
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

   auto statement = static_cast<INFORMIX_STATEMENT*>(hStmt);
   int length = (cType == DB_CTYPE_STRING) ? (int)wcslen((WCHAR *)buffer) + 1 : 0;

	SQLPOINTER sqlBuffer;
	switch(allocType)
	{
		case DB_BIND_STATIC:
         if (cType == DB_CTYPE_UTF8_STRING)
         {
            sqlBuffer = WideStringFromUTF8String((char *)buffer);
            statement->buffers->add(sqlBuffer);
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
			statement->buffers->add(sqlBuffer);
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
			statement->buffers->add(sqlBuffer);
			break;
		default:
			return;	// Invalid call
	}
	SQLBindParameter(statement->handle, pos, SQL_PARAM_INPUT, odbcCType[cType], odbcSqlType[sqlType],
	                 ((cType == DB_CTYPE_STRING) || (cType == DB_CTYPE_UTF8_STRING)) ? length : 0, 0, sqlBuffer, 0, nullptr);
}

/**
 * Execute prepared statement
 */
static uint32_t Execute(DBDRV_CONNECTION connection, DBDRV_STATEMENT hStmt, WCHAR *errorText)
{
	uint32_t dwResult;

   static_cast<INFORMIX_CONN*>(connection)->mutexQuery->lock();
   SQLHSTMT handle = static_cast<INFORMIX_STATEMENT*>(hStmt)->handle;
   long rc = SQLExecute(handle);
	if (
		(rc == SQL_SUCCESS) ||
		(rc == SQL_SUCCESS_WITH_INFO) ||
		(rc == SQL_NO_DATA))
	{
		ClearPendingResults(handle);
		dwResult = DBERR_SUCCESS;
	}
	else
	{
		dwResult = GetSQLErrorInfo(SQL_HANDLE_STMT, handle, errorText);
	}
   static_cast<INFORMIX_CONN*>(connection)->mutexQuery->unlock();
	return dwResult;
}

/**
 * Destroy prepared statement
 */
static void FreeStatement(DBDRV_STATEMENT hStmt)
{
   auto statement = static_cast<INFORMIX_STATEMENT*>(hStmt);
   statement->connection->mutexQuery->lock();
	SQLFreeHandle(SQL_HANDLE_STMT, statement->handle);
	statement->connection->mutexQuery->unlock();
	delete statement->buffers;
	MemFree(statement);
}

/**
 * Perform non-SELECT query
 */
static uint32_t Query(DBDRV_CONNECTION connection, const WCHAR *query, WCHAR *errorText)
{
	uint32_t dwResult;

   static_cast<INFORMIX_CONN*>(connection)->mutexQuery->lock();

	// Allocate statement handle
   SQLHSTMT sqlStatement;
	SQLRETURN iResult = SQLAllocHandle(SQL_HANDLE_STMT, static_cast<INFORMIX_CONN*>(connection)->sqlConn, &sqlStatement);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
	{
		// Execute statement
		iResult = SQLExecDirectW(sqlStatement, (SQLWCHAR *)query, SQL_NTS);
		if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO) || (iResult == SQL_NO_DATA))
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
		dwResult = GetSQLErrorInfo(SQL_HANDLE_DBC, static_cast<INFORMIX_CONN*>(connection)->sqlConn, errorText);
	}

   static_cast<INFORMIX_CONN*>(connection)->mutexQuery->unlock();
	return dwResult;
}

/**
 * Process results of SELECT query
 */
static INFORMIX_QUERY_RESULT *ProcessSelectResults(SQLHSTMT statement)
{
	// Allocate result buffer and determine column info
	INFORMIX_QUERY_RESULT *pResult = MemAllocStruct<INFORMIX_QUERY_RESULT>();
	short wNumCols;
	SQLNumResultCols(statement, &wNumCols);
	pResult->numColumns = wNumCols;

	BYTE *pDataBuffer = (BYTE *)MemAlloc(DATA_BUFFER_SIZE * sizeof(wchar_t));

	// Get column names
	pResult->columnNames = MemAllocArray<char*>(pResult->numColumns);
	for(int i = 0; i < (int)pResult->numColumns; i++)
	{
		char name[256];
		SQLSMALLINT len;

		SQLRETURN iResult = SQLColAttribute(statement, (SQLSMALLINT)(i + 1), SQL_DESC_NAME, (SQLPOINTER)name, 256, &len, nullptr); 
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

	// Fetch all data
	long iCurrValue = 0;
	SQLRETURN iResult;
	while(iResult = SQLFetch(statement), (iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
	{
		pResult->numRows++;
		pResult->values = MemRealloc(pResult->values, sizeof(WCHAR *) * (pResult->numRows * pResult->numColumns));
		for(int i = 1; i <= (int)pResult->numColumns; i++)
		{
			SQLLEN iDataSize;

			pDataBuffer[0] = 0;
			iResult = SQLGetData(statement, (short)i, SQL_C_WCHAR, pDataBuffer, DATA_BUFFER_SIZE, &iDataSize);
			if (iDataSize != SQL_NULL_DATA)
			{
				pResult->values[iCurrValue++] = MemCopyStringW((const WCHAR *)pDataBuffer);
			}
			else
			{
				pResult->values[iCurrValue++] = MemCopyStringW(L"");
			}
		}
	}

	MemFree(pDataBuffer);
	return pResult;
}

/**
 * Perform SELECT query
 */
static DBDRV_RESULT Select(DBDRV_CONNECTION connection, const WCHAR *query, uint32_t *errorCode, WCHAR *errorText)
{
	INFORMIX_QUERY_RESULT *pResult = NULL;

   static_cast<INFORMIX_CONN*>(connection)->mutexQuery->lock();

	// Allocate statement handle
   SQLHSTMT sqlStatement;
	SQLRETURN iResult = SQLAllocHandle(SQL_HANDLE_STMT, static_cast<INFORMIX_CONN*>(connection)->sqlConn, &sqlStatement);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
	{
		// Execute statement
		iResult = SQLExecDirectW(sqlStatement, (SQLWCHAR *)query, SQL_NTS);
		if (
			(iResult == SQL_SUCCESS) || 
			(iResult == SQL_SUCCESS_WITH_INFO))
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
		*errorCode = GetSQLErrorInfo(SQL_HANDLE_DBC, static_cast<INFORMIX_CONN*>(connection)->sqlConn, errorText);
	}

   static_cast<INFORMIX_CONN*>(connection)->mutexQuery->unlock();
	return pResult;
}

/**
 * Perform SELECT query using prepared statement
 */
static DBDRV_RESULT SelectPrepared(DBDRV_CONNECTION connection, DBDRV_STATEMENT hStmt, uint32_t *errorCode, WCHAR *errorText)
{
	INFORMIX_QUERY_RESULT *pResult = nullptr;
   static_cast<INFORMIX_CONN*>(connection)->mutexQuery->lock();
   SQLHSTMT handle = static_cast<INFORMIX_STATEMENT*>(hStmt)->handle;
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
   static_cast<INFORMIX_CONN*>(connection)->mutexQuery->unlock();
	return pResult;
}

/**
 * Get field length from result
 */
static int32_t GetFieldLength(DBDRV_RESULT hResult, int iRow, int iColumn)
{
   auto result = static_cast<INFORMIX_QUERY_RESULT*>(hResult);
	int32_t nLen = -1;
	if ((iRow < result->numRows) && (iRow >= 0) &&
			(iColumn < result->numColumns) && (iColumn >= 0))
	{
		nLen =static_cast<int32_t>(wcslen(result->values[iRow * result->numColumns + iColumn]));
	}
	return nLen;
}

/**
 * Get field value from result
 */
static WCHAR *GetField(DBDRV_RESULT hResult, int iRow, int iColumn, WCHAR *pBuffer, int nBufSize)
{
   auto result = static_cast<INFORMIX_QUERY_RESULT*>(hResult);
	WCHAR *pValue = nullptr;
	if ((iRow < result->numRows) && (iRow >= 0) &&
			(iColumn < result->numColumns) && (iColumn >= 0))
	{
#ifdef _WIN32
		wcsncpy_s(pBuffer, nBufSize, result->values[iRow * result->numColumns + iColumn], _TRUNCATE);
#else
		wcsncpy(pBuffer, result->values[iRow * result->numColumns + iColumn], nBufSize);
		pBuffer[nBufSize - 1] = 0;
#endif
		pValue = pBuffer;
	}
	return pValue;
}

/**
 * Get number of rows in result
 */
static int GetNumRows(DBDRV_RESULT hResult)
{
	return static_cast<INFORMIX_QUERY_RESULT*>(hResult)->numRows;
}

/**
 * Get column count in query result
 */
static int GetColumnCount(DBDRV_RESULT hResult)
{
	return static_cast<INFORMIX_QUERY_RESULT*>(hResult)->numColumns;
}

/**
 * Get column name in query result
 */
static const char *GetColumnName(DBDRV_RESULT hResult, int column)
{
	return ((column >= 0) && (column < static_cast<INFORMIX_QUERY_RESULT*>(hResult)->numColumns)) ? static_cast<INFORMIX_QUERY_RESULT*>(hResult)->columnNames[column] : nullptr;
}

/**
 * Free SELECT results
 */
static void FreeResult(DBDRV_RESULT hResult)
{
   auto result = static_cast<INFORMIX_QUERY_RESULT*>(hResult);

	int count = result->numColumns * result->numRows;
	for(int i = 0; i < count; i++)
	{
		MemFree(result->values[i]);
	}
	MemFree(result->values);

	for(int i = 0; i < result->numColumns; i++)
	{
		MemFree(result->columnNames[i]);
	}
	MemFree(result->columnNames);
	MemFree(result);
}

/**
 * Perform unbuffered SELECT query
 */
static DBDRV_UNBUFFERED_RESULT SelectUnbuffered(DBDRV_CONNECTION connection, const WCHAR *query, uint32_t *errorCode, WCHAR *errorText)
{
	INFORMIX_UNBUFFERED_QUERY_RESULT *pResult = nullptr;

   static_cast<INFORMIX_CONN*>(connection)->mutexQuery->lock();

	// Allocate statement handle
   SQLHSTMT sqlStatement;
	SQLRETURN iResult = SQLAllocHandle(SQL_HANDLE_STMT, static_cast<INFORMIX_CONN*>(connection)->sqlConn, &sqlStatement);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
	{
		// Execute statement
		iResult = SQLExecDirectW(sqlStatement, (SQLWCHAR *)query, SQL_NTS);
		if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
		{
			// Allocate result buffer and determine column info
         pResult = MemAllocStruct<INFORMIX_UNBUFFERED_QUERY_RESULT>();
         pResult->sqlStatement = sqlStatement;
         pResult->isPrepared = false;
         
         short wNumCols;
         SQLNumResultCols(sqlStatement, &wNumCols);
			pResult->numColumns = wNumCols;
			pResult->pConn = static_cast<INFORMIX_CONN*>(connection);
			pResult->noMoreRows = false;

			// Get column names
			pResult->columnNames = (char **)malloc(sizeof(char *) * pResult->numColumns);
			for(int i = 0; i < pResult->numColumns; i++)
			{
				SQLWCHAR name[256];
				SQLSMALLINT len;

				iResult = SQLColAttributeW(sqlStatement, (SQLSMALLINT)(i + 1), SQL_DESC_NAME, name, 256, &len, NULL); 
				if (
					(iResult == SQL_SUCCESS) || 
					(iResult == SQL_SUCCESS_WITH_INFO))
				{
					name[len] = 0;
					pResult->columnNames[i] = MBStringFromWideString(name);
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
			*errorCode = GetSQLErrorInfo(SQL_HANDLE_STMT, sqlStatement, errorText);
			// Free statement handle if query failed
			SQLFreeHandle(SQL_HANDLE_STMT, sqlStatement);
		}
	}
	else
	{
		*errorCode = GetSQLErrorInfo(SQL_HANDLE_DBC, static_cast<INFORMIX_CONN*>(connection)->sqlConn, errorText);
	}

	if (pResult == nullptr) // Unlock mutex if query has failed
	{
      static_cast<INFORMIX_CONN*>(connection)->mutexQuery->unlock();
	}
	return pResult;
}

/**
 * Perform unbuffered SELECT query using prepared statement
 */
static DBDRV_UNBUFFERED_RESULT SelectPreparedUnbuffered(DBDRV_CONNECTION connection, DBDRV_STATEMENT hStmt, uint32_t *errorCode, WCHAR *errorText)
{
   INFORMIX_UNBUFFERED_QUERY_RESULT *pResult = NULL;

   static_cast<INFORMIX_CONN*>(connection)->mutexQuery->lock();
   SQLHSTMT handle = static_cast<INFORMIX_STATEMENT*>(hStmt)->handle;
   SQLRETURN rc = SQLExecute(handle);
   if ((rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO))
   {
      // Allocate result buffer and determine column info
      pResult = (INFORMIX_UNBUFFERED_QUERY_RESULT *)malloc(sizeof(INFORMIX_UNBUFFERED_QUERY_RESULT));
      pResult->sqlStatement = handle;
      pResult->isPrepared = true;
      
      short wNumCols;
      SQLNumResultCols(pResult->sqlStatement, &wNumCols);
      pResult->numColumns = wNumCols;
      pResult->pConn = static_cast<INFORMIX_CONN*>(connection);
      pResult->noMoreRows = false;

		// Get column names
		pResult->columnNames = MemAllocArray<char*>(pResult->numColumns);
		for(int i = 0; i < pResult->numColumns; i++)
		{
			SQLWCHAR name[256];
			SQLSMALLINT len;

			rc = SQLColAttributeW(pResult->sqlStatement, (SQLSMALLINT)(i + 1), SQL_DESC_NAME, name, 256, &len, NULL); 
			if ((rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO))
			{
				name[len] = 0;
				pResult->columnNames[i] = MBStringFromUCS2String((UCS2CHAR *)name);
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
      static_cast<INFORMIX_CONN*>(connection)->mutexQuery->unlock();
	return pResult;
}

/**
 * Fetch next result line from unbuffered SELECT results
 */
static bool Fetch(DBDRV_UNBUFFERED_RESULT hResult)
{
	bool bResult = false;
	long iResult = SQLFetch(static_cast<INFORMIX_UNBUFFERED_QUERY_RESULT*>(hResult)->sqlStatement);
	bResult = ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO));
	if (!bResult)
	{
      static_cast<INFORMIX_UNBUFFERED_QUERY_RESULT*>(hResult)->noMoreRows = true;
	}
	return bResult;
}

/**
 * Get field length from unbuffered query result
 */
static int32_t GetFieldLengthUnbuffered(DBDRV_UNBUFFERED_RESULT hResult, int column)
{
	int32_t nLen = -1;
	if ((column < static_cast<INFORMIX_UNBUFFERED_QUERY_RESULT*>(hResult)->numColumns) && (column >= 0))
	{
		SQLLEN dataSize;
		char temp[1];
		long rc = SQLGetData(static_cast<INFORMIX_UNBUFFERED_QUERY_RESULT*>(hResult)->sqlStatement, (short)column + 1, SQL_C_CHAR, temp, 0, &dataSize);
		if ((rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO))
		{
			nLen = static_cast<int32_t>(dataSize);
		}
	}
	return nLen;
}

/**
 * Get field from current row in unbuffered query result
 */
static WCHAR *GetFieldUnbuffered(DBDRV_UNBUFFERED_RESULT hResult, int iColumn, WCHAR *pBuffer, int iBufSize)
{
   auto result = static_cast<INFORMIX_UNBUFFERED_QUERY_RESULT*>(hResult);

   // Check if there are valid fetched row
   if (result->noMoreRows)
      return nullptr;

   SQLLEN iDataSize;
	long iResult;

	if ((iColumn >= 0) && (iColumn < result->numColumns))
	{
		// At least on HP-UX driver expects length in chars, not bytes
		// otherwise it crashes
		// TODO: check other platforms
		iResult = SQLGetData(result->sqlStatement, (short)iColumn + 1, SQL_C_WCHAR, pBuffer, iBufSize, &iDataSize);
		if (((iResult != SQL_SUCCESS) && (iResult != SQL_SUCCESS_WITH_INFO)) || (iDataSize == SQL_NULL_DATA))
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
	return static_cast<INFORMIX_UNBUFFERED_QUERY_RESULT*>(hResult)->numColumns;
}

/**
 * Get column name in unbuffered query result
 */
static const char *GetColumnNameUnbuffered(DBDRV_UNBUFFERED_RESULT hResult, int column)
{
	return ((column >= 0) && (column < static_cast<INFORMIX_UNBUFFERED_QUERY_RESULT*>(hResult)->numColumns)) ? static_cast<INFORMIX_UNBUFFERED_QUERY_RESULT*>(hResult)->columnNames[column] : nullptr;
}

/**
 * Destroy result of unbuffered query
 */
static void FreeUnbufferedResult(DBDRV_UNBUFFERED_RESULT hResult)
{
   auto result = static_cast<INFORMIX_UNBUFFERED_QUERY_RESULT*>(hResult);

   if (result->isPrepared)
      SQLCloseCursor(result->sqlStatement);
   else
      SQLFreeHandle(SQL_HANDLE_STMT, result->sqlStatement);
   result->pConn->mutexQuery->unlock();

   for(int i = 0; i < result->numColumns; i++)
	{
		MemFree(result->columnNames[i]);
	}
	MemFree(result->columnNames);

   MemFree(result);
}

/**
 * Begin transaction
 */
static uint32_t Begin(DBDRV_CONNECTION connection)
{
	uint32_t dwResult;
   static_cast<INFORMIX_CONN*>(connection)->mutexQuery->lock();
   SQLRETURN nRet = SQLSetConnectAttr(static_cast<INFORMIX_CONN*>(connection)->sqlConn, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0);
	if ((nRet == SQL_SUCCESS) || (nRet == SQL_SUCCESS_WITH_INFO))
	{
		dwResult = DBERR_SUCCESS;
	}
	else
	{
		dwResult = GetSQLErrorInfo(SQL_HANDLE_DBC, static_cast<INFORMIX_CONN*>(connection)->sqlConn, nullptr);
	}
   static_cast<INFORMIX_CONN*>(connection)->mutexQuery->unlock();
	return dwResult;
}

/**
 * Commit transaction
 */
static uint32_t Commit(DBDRV_CONNECTION connection)
{
   static_cast<INFORMIX_CONN*>(connection)->mutexQuery->lock();
   SQLRETURN nRet = SQLEndTran(SQL_HANDLE_DBC, static_cast<INFORMIX_CONN*>(connection)->sqlConn, SQL_COMMIT);
	SQLSetConnectAttr(static_cast<INFORMIX_CONN*>(connection)->sqlConn, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
   static_cast<INFORMIX_CONN*>(connection)->mutexQuery->unlock();
	return ((nRet == SQL_SUCCESS) || (nRet == SQL_SUCCESS_WITH_INFO)) ? DBERR_SUCCESS : DBERR_OTHER_ERROR;
}

/**
 * Rollback transaction
 */
static uint32_t Rollback(DBDRV_CONNECTION connection)
{
   static_cast<INFORMIX_CONN*>(connection)->mutexQuery->lock();
   SQLRETURN nRet = SQLEndTran(SQL_HANDLE_DBC, static_cast<INFORMIX_CONN*>(connection)->sqlConn, SQL_ROLLBACK);
	SQLSetConnectAttr(static_cast<INFORMIX_CONN*>(connection)->sqlConn, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
   static_cast<INFORMIX_CONN*>(connection)->mutexQuery->unlock();
	return ((nRet == SQL_SUCCESS) || (nRet == SQL_SUCCESS_WITH_INFO)) ? DBERR_SUCCESS : DBERR_OTHER_ERROR;
}

/**
 * Check if table exist
 */
static int IsTableExist(DBDRV_CONNECTION connection, const WCHAR *name)
{
   WCHAR query[256];
   swprintf(query, 256, L"SELECT count(*) FROM informix.systables WHERE tabtype='T' AND upper(tabname)=upper('%ls')", name);
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
   nullptr, //GetFieldUTF8
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

DB_DRIVER_ENTRY_POINT("INFORMIX", s_callTable)

#ifdef _WIN32

/**
 * DLL Entry point
 */
bool WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return true;
}

#endif
