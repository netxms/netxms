/* 
** DB2 Database Driver
** Copyright (C) 2010-2022 Raden Solutinos
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
** File: db2.cpp
**
**/

#define _CRT_SECURE_NO_WARNINGS

#include "db2drv.h"

/**
 * Convert DB2 state to NetXMS database error code and get error text
 */
static uint32_t GetSQLErrorInfo(SQLSMALLINT nHandleType, SQLHANDLE hHandle, NETXMS_WCHAR *errorText)
{
   uint32_t dwError;
   SQLSMALLINT nChars;
	SQLWCHAR buffer[DBDRV_MAX_ERROR_TEXT];

	// Get state information and convert it to NetXMS database error code
   SQLRETURN nRet = SQLGetDiagFieldW(nHandleType, hHandle, 1, SQL_DIAG_SQLSTATE, buffer, 16, &nChars);
	if (nRet == SQL_SUCCESS)
	{
#if UNICODE_UCS2
		if (
			(!wcscmp((WCHAR *)buffer, L"08003")) ||	// Connection does not exist
			(!wcscmp((WCHAR *)buffer, L"08S01")) ||	// Communication link failure
			(!wcscmp((WCHAR *)buffer, L"HYT00")) ||	// Timeout expired
			(!wcscmp((WCHAR *)buffer, L"HYT01")) ||	// Connection timeout expired
			(!wcscmp((WCHAR *)buffer, L"08506")))	// SQL30108N: A connection failed but has been re-established.
#else
		char state[16];
		ucs2_to_mb(buffer, -1, state, 16);
		if (
			(!strcmp(state, "08003")) ||	// Connection does not exist
			(!strcmp(state, "08S01")) ||	// Communication link failure
			(!strcmp(state, "HYT00")) ||	// Timeout expired
			(!strcmp(state, "HYT01")) ||	// Connection timeout expired
			(!strcmp(state, "08506")))	// SQL30108N: A connection failed but has been re-established.
#endif
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
#if UNICODE_UCS2
		nRet = SQLGetDiagFieldW(nHandleType, hHandle, 1, SQL_DIAG_MESSAGE_TEXT, errorText, DBDRV_MAX_ERROR_TEXT, &nChars);
#else
		nRet = SQLGetDiagFieldW(nHandleType, hHandle, 1, SQL_DIAG_MESSAGE_TEXT, buffer, DBDRV_MAX_ERROR_TEXT, &nChars);
#endif
		if (nRet == SQL_SUCCESS)
		{
#if UNICODE_UCS4
			buffer[DBDRV_MAX_ERROR_TEXT - 1] = 0;
			ucs2_to_ucs4(buffer, -1, errorText, DBDRV_MAX_ERROR_TEXT);
#endif
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
static StringBuffer PrepareString(const TCHAR *str, size_t maxSize)
{
   StringBuffer out;
   out.append(_T('\''));
   for(const TCHAR *src = str; (*src != 0) && (maxSize > 0); src++, maxSize--)
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
 * pszHost should be set to DB2 source name, and pszDatabase is ignored
 */
static DBDRV_CONNECTION Connect(const char *pszHost, const char *pszLogin, const char *pszPassword, const char *pszDatabase, const char *schema, NETXMS_WCHAR *errorText)
{
	long iResult;
   SQLHSTMT sqlStatement;

	// Allocate our connection structure
   auto pConn = MemAllocStruct<DB2DRV_CONN>();

   // Allocate environment
   iResult = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &pConn->sqlEnv);
	if ((iResult != SQL_SUCCESS) && (iResult != SQL_SUCCESS_WITH_INFO))
	{
		wcscpy(errorText, L"Cannot allocate environment handle");
		goto connect_failure_0;
	}

	// Set required DB2 version
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

	// Connect to the datasource 
	iResult = SQLConnect(pConn->sqlConn, (SQLCHAR *)pszHost, SQL_NTS, (SQLCHAR *)pszLogin, SQL_NTS, (SQLCHAR *)pszPassword, SQL_NTS);
	if ((iResult != SQL_SUCCESS) && (iResult != SQL_SUCCESS_WITH_INFO))
	{
		GetSQLErrorInfo(SQL_HANDLE_DBC, pConn->sqlConn, errorText);
		goto connect_failure_2;
	}

   // Explicitly set autocommit mode
   SQLSetConnectAttr(pConn->sqlConn, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);

	// Set current schema
	if ((schema != NULL) && (schema[0] != 0))
	{
		char query[256];
		snprintf(query, 256, "SET CURRENT SCHEMA = '%s'", schema);

		iResult = SQLAllocHandle(SQL_HANDLE_STMT, pConn->sqlConn, &sqlStatement);
		if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
		{
			// Execute statement
			SQLWCHAR *temp = (SQLWCHAR *)UCS2StringFromMBString(query);
			iResult = SQLExecDirectW(sqlStatement, temp, SQL_NTS);
			free(temp);
			if ((iResult != SQL_SUCCESS) && 
				 (iResult != SQL_SUCCESS_WITH_INFO) && 
				 (iResult != SQL_NO_DATA))
			{
				GetSQLErrorInfo(SQL_HANDLE_STMT, sqlStatement, errorText);
				goto connect_failure_3;
			}
			SQLFreeHandle(SQL_HANDLE_STMT, sqlStatement);
		}
		else
		{
			GetSQLErrorInfo(SQL_HANDLE_DBC, pConn->sqlConn, errorText);
			goto connect_failure_2;
		}
	}

	// Create mutex
	pConn->mutexQuery = new Mutex(MutexType::NORMAL);

	// Success
	return pConn;

	// Handle failures
connect_failure_3:
	SQLFreeHandle(SQL_HANDLE_STMT, sqlStatement);

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
   auto c = static_cast<DB2DRV_CONN*>(connection);
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
static DBDRV_STATEMENT Prepare(DBDRV_CONNECTION connection, const NETXMS_WCHAR *pwszQuery, bool optimizeForReuse, uint32_t *pdwError, NETXMS_WCHAR *errorText)
{
	long iResult;
	SQLHSTMT statement;
	DB2DRV_STATEMENT *result;

   static_cast<DB2DRV_CONN*>(connection)->mutexQuery->lock();

	// Allocate statement handle
	iResult = SQLAllocHandle(SQL_HANDLE_STMT, static_cast<DB2DRV_CONN*>(connection)->sqlConn, &statement);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
	{
		// Prepare statement
#if defined(_WIN32) || defined(UNICODE_UCS2)
		iResult = SQLPrepareW(statement, (SQLWCHAR *)pwszQuery, SQL_NTS);
#else
		SQLWCHAR *temp = UCS2StringFromUCS4String(pwszQuery);
		iResult = SQLPrepareW(statement, temp, SQL_NTS);
		MemFree(temp);
#endif
		if ((iResult == SQL_SUCCESS) ||
				(iResult == SQL_SUCCESS_WITH_INFO))
		{
         result = MemAllocStruct<DB2DRV_STATEMENT>();
			result->handle = statement;
			result->buffers = new Array(0, 16, Ownership::True);
			result->connection = static_cast<DB2DRV_CONN*>(connection);
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
		*pdwError = GetSQLErrorInfo(SQL_HANDLE_DBC, static_cast<DB2DRV_CONN*>(connection)->sqlConn, errorText);
		result = nullptr;
	}

   static_cast<DB2DRV_CONN*>(connection)->mutexQuery->unlock();
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

   auto statement = static_cast<DB2DRV_STATEMENT*>(hStmt);
   int length = (cType == DB_CTYPE_STRING) ? (int)wcslen((WCHAR *)buffer) + 1 : 0;

	SQLPOINTER sqlBuffer;
	switch(allocType)
	{
		case DB_BIND_STATIC:
#if defined(_WIN32) || defined(UNICODE_UCS2)
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
#else
			if (cType == DB_CTYPE_STRING)
			{
				sqlBuffer = UCS2StringFromUCS4String((WCHAR *)buffer);
				statement->buffers->add(sqlBuffer);
			}
			else if (cType == DB_CTYPE_UTF8_STRING)
         {
            sqlBuffer = UCS2StringFromUTF8String((char *)buffer);
            statement->buffers->add(sqlBuffer);
            length = (int)ucs2_strlen((UCS2CHAR *)sqlBuffer) + 1;
         }
			else
			{
				sqlBuffer = buffer;
			}
#endif
			break;
		case DB_BIND_DYNAMIC:
#if defined(_WIN32) || defined(UNICODE_UCS2)
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
#else
			if (cType == DB_CTYPE_STRING)
			{
				sqlBuffer = UCS2StringFromUCS4String((WCHAR *)buffer);
				free(buffer);
			}
			else if (cType == DB_CTYPE_UTF8_STRING)
         {
            sqlBuffer = UCS2StringFromUTF8String((char *)buffer);
            free(buffer);
            length = (int)ucs2_strlen((UCS2CHAR *)sqlBuffer) + 1;
         }
			else
			{
				sqlBuffer = buffer;
			}
#endif
			statement->buffers->add(sqlBuffer);
			break;
		case DB_BIND_TRANSIENT:
#if defined(_WIN32) || defined(UNICODE_UCS2)
         if (cType == DB_CTYPE_UTF8_STRING)
         {
            sqlBuffer = WideStringFromUTF8String((char *)buffer);
            length = (int)wcslen((WCHAR *)sqlBuffer) + 1;
         }
         else
         {
            sqlBuffer = MemCopyBlock(buffer, (cType == DB_CTYPE_STRING) ? (DWORD)(length * sizeof(WCHAR)) : bufferSize[cType]);
         }
#else
			if (cType == DB_CTYPE_STRING)
			{
				sqlBuffer = UCS2StringFromUCS4String((WCHAR *)buffer);
			}
			else if (cType == DB_CTYPE_UTF8_STRING)
         {
            sqlBuffer = UCS2StringFromUTF8String((char *)buffer);
            length = (int)ucs2_strlen((UCS2CHAR *)sqlBuffer) + 1;
         }
			else
			{
				sqlBuffer = MemCopyBlock(buffer, bufferSize[cType]);
			}
#endif
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
static uint32_t Execute(DBDRV_CONNECTION connection, DBDRV_STATEMENT hStmt, NETXMS_WCHAR *errorText)
{
	uint32_t dwResult;

   static_cast<DB2DRV_CONN*>(connection)->mutexQuery->lock();
   SQLHSTMT handle = static_cast<DB2DRV_STATEMENT*>(hStmt)->handle;
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
   static_cast<DB2DRV_CONN*>(connection)->mutexQuery->unlock();
	return dwResult;
}

/**
 * Destroy prepared statement
 */
static void FreeStatement(DBDRV_STATEMENT hStmt)
{
   auto statement = static_cast<DB2DRV_STATEMENT*>(hStmt);
   statement->connection->mutexQuery->lock();
	SQLFreeHandle(SQL_HANDLE_STMT, statement->handle);
	statement->connection->mutexQuery->unlock();
	delete statement->buffers;
	MemFree(statement);
}

/**
 * Perform non-SELECT query
 */
static uint32_t Query(DBDRV_CONNECTION connection, const NETXMS_WCHAR *pwszQuery, NETXMS_WCHAR *errorText)
{
	long iResult;
	uint32_t dwResult;

   static_cast<DB2DRV_CONN*>(connection)->mutexQuery->lock();

	// Allocate statement handle
   SQLHSTMT sqlStatement;
	iResult = SQLAllocHandle(SQL_HANDLE_STMT, static_cast<DB2DRV_CONN*>(connection)->sqlConn, &sqlStatement);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
	{
		// Execute statement
#if defined(_WIN32) || defined(UNICODE_UCS2)
		iResult = SQLExecDirectW(sqlStatement, (SQLWCHAR *)pwszQuery, SQL_NTS);
#else
		SQLWCHAR *temp = UCS2StringFromUCS4String(pwszQuery);
		iResult = SQLExecDirectW(sqlStatement, temp, SQL_NTS);
		MemFree(temp);
#endif      
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
		dwResult = GetSQLErrorInfo(SQL_HANDLE_DBC, static_cast<DB2DRV_CONN*>(connection)->sqlConn, errorText);
	}

   static_cast<DB2DRV_CONN*>(connection)->mutexQuery->unlock();
	return dwResult;
}

/**
 * Get complete field data
 */
static NETXMS_WCHAR *GetFieldData(SQLHSTMT sqlStatement, short column)
{
   NETXMS_WCHAR *result = NULL;
   SQLINTEGER dataSize;
#if defined(_WIN32) || defined(UNICODE_UCS2)
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
         size_t tempSize = sizeof(buffer) * 4;   // temporary buffer size in bytes
         WCHAR *temp = (WCHAR *)malloc(tempSize);
         memcpy(temp, buffer, sizeof(buffer));
         size_t offset = sizeof(buffer) - sizeof(WCHAR); // offset in buffer in bytes
         while(true)
         {
            SQLINTEGER readSize = (SQLINTEGER)(tempSize - offset);
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
#else
   UCS2CHAR buffer[256];
   SQLRETURN rc = SQLGetData(sqlStatement, column, SQL_C_WCHAR, buffer, sizeof(buffer), &dataSize);
   if (((rc == SQL_SUCCESS) || ((rc == SQL_SUCCESS_WITH_INFO) && (dataSize >= 0) && (dataSize <= (SQLLEN)(sizeof(buffer) - sizeof(UCS2CHAR))))) && (dataSize != SQL_NULL_DATA))
   {
      int len = ucs2_strlen(buffer);
      result = (NETXMS_WCHAR *)malloc((len + 1) * sizeof(NETXMS_WCHAR));
      ucs2_to_ucs4(buffer, -1, result, len + 1);
   }
   else if ((rc == SQL_SUCCESS_WITH_INFO) && (dataSize != SQL_NULL_DATA))
   {
      if (dataSize > (SQLLEN)(sizeof(buffer) - sizeof(UCS2CHAR)))
      {
         UCS2CHAR *temp = (UCS2CHAR *)malloc(dataSize + sizeof(UCS2CHAR));
         memcpy(temp, buffer, sizeof(buffer));
         rc = SQLGetData(sqlStatement, column, SQL_C_WCHAR, &temp[255], dataSize - 254 * sizeof(UCS2CHAR), &dataSize);
         if ((rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO))
         {
            int len = ucs2_strlen(temp);
            result = (NETXMS_WCHAR *)malloc((len + 1) * sizeof(NETXMS_WCHAR));
            ucs2_to_ucs4(temp, -1, result, len + 1);
         }
         free(temp);
      }
      else if (dataSize == SQL_NO_TOTAL)
      {
         size_t tempSize = sizeof(buffer) * 4;
         UCS2CHAR *temp = (UCS2CHAR *)malloc(tempSize);
         memcpy(temp, buffer, sizeof(buffer));
         size_t offset = sizeof(buffer) - sizeof(UCS2CHAR);
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
            temp = (UCS2CHAR *)realloc(temp, tempSize);
            offset += readSize - sizeof(UCS2CHAR);
         }
         int len = ucs2_strlen(temp);
         result = (NETXMS_WCHAR *)malloc((len + 1) * sizeof(NETXMS_WCHAR));
         ucs2_to_ucs4(temp, -1, result, len + 1);
         free(temp);
      }
   }
#endif
   return (result != nullptr) ? result : MemCopyStringW(L"");
}

/**
 * Process results of SELECT query
 */
static DB2DRV_QUERY_RESULT *ProcessSelectResults(SQLHSTMT statement)
{
	// Allocate result buffer and determine column info
   DB2DRV_QUERY_RESULT *pResult = MemAllocStruct<DB2DRV_QUERY_RESULT>();
	short wNumCols;
	SQLNumResultCols(statement, &wNumCols);
	pResult->numColumns = wNumCols;

	// Get column names
	pResult->columnNames = MemAllocArray<char*>(pResult->numColumns);
	for(int i = 0; i < (int)pResult->numColumns; i++)
	{
		UCS2CHAR name[256];
		SQLSMALLINT len;

		SQLRETURN iResult = SQLColAttributeW(statement, (SQLSMALLINT)(i + 1), SQL_DESC_NAME, (SQLPOINTER)name, 256, &len, NULL); 
		if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
		{
			name[len] = 0;
			pResult->columnNames[i] = MBStringFromUCS2String(name);
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
		pResult->values = (NETXMS_WCHAR **)MemRealloc(pResult->values, sizeof(NETXMS_WCHAR *) * (pResult->numRows * pResult->numColumns));
		for(int i = 1; i <= (int)pResult->numColumns; i++)
		{
			pResult->values[iCurrValue++] = GetFieldData(statement, (short)i);
		}
	}

	return pResult;
}

/**
 * Perform SELECT query
 */
static DBDRV_RESULT Select(DBDRV_CONNECTION connection, const NETXMS_WCHAR *pwszQuery, uint32_t *errorCode, NETXMS_WCHAR *errorText)
{
	DB2DRV_QUERY_RESULT *pResult = nullptr;

   static_cast<DB2DRV_CONN*>(connection)->mutexQuery->lock();

	// Allocate statement handle
   SQLHSTMT sqlStatement;
	long iResult = SQLAllocHandle(SQL_HANDLE_STMT, static_cast<DB2DRV_CONN*>(connection)->sqlConn, &sqlStatement);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
	{
		// Execute statement
#if defined(_WIN32) || defined(UNICODE_UCS2)
		iResult = SQLExecDirectW(sqlStatement, (SQLWCHAR *)pwszQuery, SQL_NTS);
#else
		SQLWCHAR *temp = UCS2StringFromUCS4String(pwszQuery);
		iResult = SQLExecDirectW(sqlStatement, temp, SQL_NTS);
		MemFree(temp);
#endif
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
		*errorCode = GetSQLErrorInfo(SQL_HANDLE_DBC, static_cast<DB2DRV_CONN*>(connection)->sqlConn, errorText);
	}

   static_cast<DB2DRV_CONN*>(connection)->mutexQuery->unlock();
	return pResult;
}

/**
 * Perform SELECT query using prepared statement
 */
static DBDRV_RESULT SelectPrepared(DBDRV_CONNECTION connection, DBDRV_STATEMENT hStmt, uint32_t *errorCode, NETXMS_WCHAR *errorText)
{
	DB2DRV_QUERY_RESULT *pResult = nullptr;
   static_cast<DB2DRV_CONN*>(connection)->mutexQuery->lock();
   SQLHSTMT handle = static_cast<DB2DRV_STATEMENT*>(hStmt)->handle;
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
   static_cast<DB2DRV_CONN*>(connection)->mutexQuery->unlock();
	return pResult;
}

/**
 * Get field length from result
 */
static int32_t GetFieldLength(DBDRV_RESULT hResult, int iRow, int iColumn)
{
   auto result = static_cast<DB2DRV_QUERY_RESULT*>(hResult);
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
static NETXMS_WCHAR *GetField(DBDRV_RESULT hResult, int iRow, int iColumn, NETXMS_WCHAR *pBuffer, int nBufSize)
{
   auto result = static_cast<DB2DRV_QUERY_RESULT*>(hResult);
	NETXMS_WCHAR *pValue = nullptr;
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
	return static_cast<DB2DRV_QUERY_RESULT*>(hResult)->numRows;
}

/**
 * Get column count in query result
 */
static int GetColumnCount(DBDRV_RESULT hResult)
{
	return static_cast<DB2DRV_QUERY_RESULT*>(hResult)->numColumns;
}

/**
 * Get column name in query result
 */
static const char *GetColumnName(DBDRV_RESULT hResult, int column)
{
	return ((column >= 0) && (column < static_cast<DB2DRV_QUERY_RESULT*>(hResult)->numColumns)) ? static_cast<DB2DRV_QUERY_RESULT*>(hResult)->columnNames[column] : nullptr;
}

/**
 * Free SELECT results
 */
static void FreeResult(DBDRV_RESULT hResult)
{
   auto result = static_cast<DB2DRV_QUERY_RESULT*>(hResult);

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
static DBDRV_UNBUFFERED_RESULT SelectUnbuffered(DBDRV_CONNECTION connection, const NETXMS_WCHAR *pwszQuery, uint32_t *pdwError, NETXMS_WCHAR *errorText)
{
	DB2DRV_UNBUFFERED_QUERY_RESULT *pResult = nullptr;

   static_cast<DB2DRV_CONN*>(connection)->mutexQuery->lock();

	// Allocate statement handle
   SQLHSTMT sqlStatement;
	SQLRETURN iResult = SQLAllocHandle(SQL_HANDLE_STMT, static_cast<DB2DRV_CONN*>(connection)->sqlConn, &sqlStatement);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
	{
		// Execute statement
#if defined(_WIN32) || defined(UNICODE_UCS2)
		iResult = SQLExecDirectW(sqlStatement, (SQLWCHAR *)pwszQuery, SQL_NTS);
#else
		SQLWCHAR *temp = UCS2StringFromUCS4String(pwszQuery);
		iResult = SQLExecDirectW(sqlStatement, temp, SQL_NTS);
		MemFree(temp);
#endif
		if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
		{
			// Allocate result buffer and determine column info
			pResult = MemAllocStruct<DB2DRV_UNBUFFERED_QUERY_RESULT>();
         pResult->sqlStatement = sqlStatement;
         pResult->isPrepared = false;
      	
         short wNumCols;
			SQLNumResultCols(sqlStatement, &wNumCols);
			pResult->numColumns = wNumCols;
			pResult->pConn = static_cast<DB2DRV_CONN*>(connection);
			pResult->noMoreRows = false;

			// Get column names
			pResult->columnNames = MemAllocArray<char*>(pResult->numColumns);
			for(int i = 0; i < pResult->numColumns; i++)
			{
				SQLWCHAR name[256];
				SQLSMALLINT len;

				iResult = SQLColAttributeW(sqlStatement, (SQLSMALLINT)(i + 1), SQL_DESC_NAME, name, 256, &len, NULL); 
				if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
				{
					name[len] = 0;
					pResult->columnNames[i] = MBStringFromUCS2String((UCS2CHAR *)name);
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
		*pdwError = GetSQLErrorInfo(SQL_HANDLE_DBC, static_cast<DB2DRV_CONN*>(connection)->sqlConn, errorText);
	}

	if (pResult == nullptr) // Unlock mutex if query has failed
	{
      static_cast<DB2DRV_CONN*>(connection)->mutexQuery->unlock();
	}
	return pResult;
}

/**
 * Perform unbuffered SELECT query using prepared statement
 */
static DBDRV_UNBUFFERED_RESULT SelectPreparedUnbuffered(DBDRV_CONNECTION connection, DBDRV_STATEMENT hStmt, uint32_t *errorCode, WCHAR *errorText)
{
   DB2DRV_UNBUFFERED_QUERY_RESULT *pResult = NULL;

   static_cast<DB2DRV_CONN*>(connection)->mutexQuery->lock();
   SQLHSTMT handle = static_cast<DB2DRV_STATEMENT*>(hStmt)->handle;
   SQLRETURN rc = SQLExecute(handle);
   if ((rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO))
   {
      // Allocate result buffer and determine column info
      pResult = MemAllocStruct<DB2DRV_UNBUFFERED_QUERY_RESULT>();
      pResult->sqlStatement = handle;
      pResult->isPrepared = true;
      
      short wNumCols;
      SQLNumResultCols(pResult->sqlStatement, &wNumCols);
      pResult->numColumns = wNumCols;
      pResult->pConn = static_cast<DB2DRV_CONN*>(connection);
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
      static_cast<DB2DRV_CONN*>(connection)->mutexQuery->unlock();
	return pResult;
}

/**
 * Fetch next result line from unbuffered SELECT results
 */
static bool Fetch(DBDRV_UNBUFFERED_RESULT hResult)
{
	bool bResult = false;
	long iResult = SQLFetch(static_cast<DB2DRV_UNBUFFERED_QUERY_RESULT*>(hResult)->sqlStatement);
	bResult = ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO));
	if (!bResult)
	{
      static_cast<DB2DRV_UNBUFFERED_QUERY_RESULT*>(hResult)->noMoreRows = true;
	}
	return bResult;
}

/**
 * Get field length from unbuffered query result
 */
static int32_t GetFieldLengthUnbuffered(DBDRV_UNBUFFERED_RESULT hResult, int column)
{
	int32_t nLen = -1;
	if ((column < static_cast<DB2DRV_UNBUFFERED_QUERY_RESULT*>(hResult)->numColumns) && (column >= 0))
	{
		SQLLEN dataSize;
		char temp[1];
		long rc = SQLGetData(static_cast<DB2DRV_UNBUFFERED_QUERY_RESULT*>(hResult)->sqlStatement, (short)column + 1, SQL_C_CHAR, temp, 0, &dataSize);
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
static NETXMS_WCHAR *GetFieldUnbuffered(DBDRV_UNBUFFERED_RESULT hResult, int iColumn, NETXMS_WCHAR *pBuffer, int iBufSize)
{
   auto result = static_cast<DB2DRV_UNBUFFERED_QUERY_RESULT*>(hResult);

   // Check if there are valid fetched row
   if (result->noMoreRows)
      return nullptr;

   SQLLEN iDataSize;
	long iResult;

	if ((iColumn >= 0) && (iColumn < result->numColumns))
	{
#if defined(_WIN32) || defined(UNICODE_UCS2)
		iResult = SQLGetData(result->sqlStatement, (short)iColumn + 1, SQL_C_WCHAR,
				pBuffer, iBufSize * sizeof(WCHAR), &iDataSize);
#else
		SQLWCHAR *tempBuff = (SQLWCHAR *)malloc(iBufSize * sizeof(SQLWCHAR));
		iResult = SQLGetData(result->sqlStatement, (short)iColumn + 1, SQL_C_WCHAR,
				tempBuff, iBufSize * sizeof(SQLWCHAR), &iDataSize);
		ucs2_to_ucs4(tempBuff, -1, pBuffer, iBufSize);
		pBuffer[iBufSize - 1] = 0;
		free(tempBuff);
#endif
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
	return static_cast<DB2DRV_UNBUFFERED_QUERY_RESULT*>(hResult)->numColumns;
}

/**
 * Get column name in unbuffered query result
 */
static const char *GetColumnNameUnbuffered(DBDRV_UNBUFFERED_RESULT hResult, int column)
{
	return ((column >= 0) && (column < static_cast<DB2DRV_UNBUFFERED_QUERY_RESULT*>(hResult)->numColumns)) ? static_cast<DB2DRV_UNBUFFERED_QUERY_RESULT*>(hResult)->columnNames[column] : nullptr;
}

/**
 * Destroy result of unbuffered query
 */
static void FreeUnbufferedResult(DBDRV_UNBUFFERED_RESULT hResult)
{
   auto result = static_cast<DB2DRV_UNBUFFERED_QUERY_RESULT*>(hResult);

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
   static_cast<DB2DRV_CONN*>(connection)->mutexQuery->lock();
   SQLRETURN nRet = SQLSetConnectAttr(static_cast<DB2DRV_CONN*>(connection)->sqlConn, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0);
	if ((nRet == SQL_SUCCESS) || (nRet == SQL_SUCCESS_WITH_INFO))
	{
		dwResult = DBERR_SUCCESS;
	}
	else
	{
		dwResult = GetSQLErrorInfo(SQL_HANDLE_DBC, static_cast<DB2DRV_CONN*>(connection)->sqlConn, nullptr);
	}
   static_cast<DB2DRV_CONN*>(connection)->mutexQuery->unlock();
	return dwResult;
}

/**
 * Commit transaction
 */
static uint32_t Commit(DBDRV_CONNECTION connection)
{
   static_cast<DB2DRV_CONN*>(connection)->mutexQuery->lock();
   SQLRETURN nRet = SQLEndTran(SQL_HANDLE_DBC, static_cast<DB2DRV_CONN*>(connection)->sqlConn, SQL_COMMIT);
	SQLSetConnectAttr(static_cast<DB2DRV_CONN*>(connection)->sqlConn, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
   static_cast<DB2DRV_CONN*>(connection)->mutexQuery->unlock();
	return ((nRet == SQL_SUCCESS) || (nRet == SQL_SUCCESS_WITH_INFO)) ? DBERR_SUCCESS : DBERR_OTHER_ERROR;
}

/**
 * Rollback transaction
 */
static uint32_t Rollback(DBDRV_CONNECTION connection)
{
	static_cast<DB2DRV_CONN*>(connection)->mutexQuery->lock();
   SQLRETURN nRet = SQLEndTran(SQL_HANDLE_DBC, static_cast<DB2DRV_CONN*>(connection)->sqlConn, SQL_ROLLBACK);
	SQLSetConnectAttr(static_cast<DB2DRV_CONN*>(connection)->sqlConn, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
   static_cast<DB2DRV_CONN*>(connection)->mutexQuery->unlock();
	return ((nRet == SQL_SUCCESS) || (nRet == SQL_SUCCESS_WITH_INFO)) ? DBERR_SUCCESS : DBERR_OTHER_ERROR;
}

/**
 * Check if table exist
 */
static int IsTableExist(DBDRV_CONNECTION connection, const NETXMS_WCHAR *name)
{
   WCHAR query[256];
   swprintf(query, 256, L"SELECT count(*) FROM sysibm.systables WHERE type='T' AND upper(name)=upper('%ls')", name);
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

DB_DRIVER_ENTRY_POINT("DB2", s_callTable)

#ifdef _WIN32

/**
 * DLL Entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hInstance);
	}
	return TRUE;
}

#endif
