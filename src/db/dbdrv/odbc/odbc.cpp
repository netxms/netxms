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
** File: odbc.cpp
**
**/

#include "odbcdrv.h"

/**
 * Flag for enable/disable UNICODE
 */
static bool m_useUnicode = true;

/**
 * Convert ODBC state to NetXMS database error code and get error text
 */
static uint32_t GetSQLErrorInfo(SQLSMALLINT nHandleType, SQLHANDLE hHandle, NETXMS_WCHAR *errorText)
{
	// Get state information and convert it to NetXMS database error code
   uint32_t errorCode;
   SQLSMALLINT nChars;
   char sqlState[16];
   SQLRETURN nRet = SQLGetDiagFieldA(nHandleType, hHandle, 1, SQL_DIAG_SQLSTATE, sqlState, 16, &nChars);
   if (nRet == SQL_SUCCESS)
   {
      if ((!strcmp(sqlState, "08003")) ||     // Connection does not exist
          (!strcmp(sqlState, "08S01")) ||     // Communication link failure
          (!strcmp(sqlState, "HYT00")) ||     // Timeout expired
          (!strcmp(sqlState, "HYT01")))       // Connection timeout expired
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
#if UNICODE_UCS2
		nRet = SQLGetDiagFieldW(nHandleType, hHandle, 1, SQL_DIAG_MESSAGE_TEXT, errorText, DBDRV_MAX_ERROR_TEXT, &nChars);
#else
		char buffer[DBDRV_MAX_ERROR_TEXT];
		nRet = SQLGetDiagFieldA(nHandleType, hHandle, 1, SQL_DIAG_MESSAGE_TEXT, buffer, DBDRV_MAX_ERROR_TEXT, &nChars);
#endif
		if (nRet == SQL_SUCCESS)
		{
#if UNICODE_UCS4
			buffer[DBDRV_MAX_ERROR_TEXT - 1] = 0;
			mb_to_wchar(buffer, -1, errorText, DBDRV_MAX_ERROR_TEXT);
#endif
			RemoveTrailingCRLFW(errorText);
		}
		else
		{
			wcscpy(errorText, L"Unable to obtain description for this error");
		}
   }

   return errorCode;
}

/**
 * Clear any pending result sets on given statement
 */
static void ClearPendingResults(SQLHSTMT stmt)
{
	while(true)
	{
		SQLRETURN rc = SQLMoreResults(stmt);
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
   m_useUnicode = ExtractNamedOptionValueAsBoolA(options, "unicode", true);
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
 * pszHost should be set to ODBC source name, and pszDatabase is ignored
 */
static DBDRV_CONNECTION Connect(const char *pszHost, const char *pszLogin, const char *pszPassword,
         const char *pszDatabase, const char *schema, NETXMS_WCHAR *errorText)
{
   long iResult;

   // Allocate our connection structure
   auto pConn = MemAllocStruct<ODBCDRV_CONN>();

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

	// Connect to the datasource
	// If DSN name contains = character, assume that it's a connection string
	if (strchr(pszHost, '=') != nullptr)
	{
		SQLSMALLINT outLen;
		iResult = SQLDriverConnectA(pConn->sqlConn, nullptr, (SQLCHAR*)pszHost, SQL_NTS, NULL, 0, &outLen, SQL_DRIVER_NOPROMPT);
	}
	else
	{
	   SQLWCHAR *wcHost = UCS2StringFromUTF8String(pszHost);
      SQLWCHAR *wcLogin = UCS2StringFromUTF8String(pszLogin);
      SQLWCHAR *wcPassword = UCS2StringFromUTF8String(pszPassword);
		iResult = SQLConnectW(pConn->sqlConn, wcHost, SQL_NTS, wcLogin, SQL_NTS, wcPassword, SQL_NTS);
		MemFree(wcHost);
      MemFree(wcLogin);
      MemFree(wcPassword);
	}
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
   auto c = static_cast<ODBCDRV_CONN*>(connection);
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
static DBDRV_STATEMENT Prepare(DBDRV_CONNECTION connection, const NETXMS_WCHAR *query, bool optimizeForReuse, uint32_t *errorCode, NETXMS_WCHAR *errorText)
{
   long iResult;
	SQLHSTMT stmt;
	ODBCDRV_STATEMENT *result;

	static_cast<ODBCDRV_CONN*>(connection)->mutexQuery->lock();

   // Allocate statement handle
   iResult = SQLAllocHandle(SQL_HANDLE_STMT, static_cast<ODBCDRV_CONN*>(connection)->sqlConn, &stmt);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
   {
      // Prepare statement
      if (m_useUnicode)
      {
#if defined(_WIN32) || defined(UNICODE_UCS2)
		   iResult = SQLPrepareW(stmt, (SQLWCHAR *)query, SQL_NTS);
#else
			SQLWCHAR *temp = UCS2StringFromUCS4String(query);
		   iResult = SQLPrepareW(stmt, temp, SQL_NTS);
		   MemFree(temp);
#endif      
		}
		else
		{
			char *temp = MBStringFromWideString(query);
		   iResult = SQLPrepareA(stmt, (SQLCHAR *)temp, SQL_NTS);
		   MemFree(temp);
		}
	   if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
		{
			result = MemAllocStruct<ODBCDRV_STATEMENT>();
			result->handle = stmt;
			result->buffers = new Array(0, 16, Ownership::True);
			result->connection = static_cast<ODBCDRV_CONN*>(connection);
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
      *errorCode = GetSQLErrorInfo(SQL_HANDLE_DBC, static_cast<ODBCDRV_CONN*>(connection)->sqlConn, errorText);
		result = nullptr;
   }

	static_cast<ODBCDRV_CONN*>(connection)->mutexQuery->unlock();
   return result;
}

/**
 * Bind parameter to statement
 */
static void Bind(DBDRV_STATEMENT hStmt, int pos, int sqlType, int cType, void *buffer, int allocType)
{
	static SQLSMALLINT odbcSqlType[] = { SQL_VARCHAR, SQL_INTEGER, SQL_BIGINT, SQL_DOUBLE, SQL_LONGVARCHAR };
	static SQLSMALLINT odbcCTypeW[] = { SQL_C_WCHAR, SQL_C_SLONG, SQL_C_ULONG, SQL_C_SBIGINT, SQL_C_UBIGINT, SQL_C_DOUBLE, SQL_C_WCHAR };
	static SQLSMALLINT odbcCTypeA[] = { SQL_C_CHAR, SQL_C_SLONG, SQL_C_ULONG, SQL_C_SBIGINT, SQL_C_UBIGINT, SQL_C_DOUBLE, SQL_C_CHAR };
	static size_t bufferSize[] = { 0, sizeof(int32_t), sizeof(uint32_t), sizeof(int64_t), sizeof(uint64_t), sizeof(double), 0 };

	auto stmt = static_cast<ODBCDRV_STATEMENT*>(hStmt);
	int length = (cType == DB_CTYPE_STRING) ? (int)wcslen((NETXMS_WCHAR *)buffer) + 1 : 0;

	SQLPOINTER sqlBuffer;
	switch(allocType)
	{
		case DB_BIND_STATIC:
			if (m_useUnicode)
			{
#if defined(_WIN32) || defined(UNICODE_UCS2)
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
#else
				if (cType == DB_CTYPE_STRING)
				{
					sqlBuffer = UCS2StringFromUCS4String((NETXMS_WCHAR *)buffer);
					stmt->buffers->add(sqlBuffer);
				}
				else if (cType == DB_CTYPE_UTF8_STRING)
            {
               sqlBuffer = UCS2StringFromUTF8String((char *)buffer);
               stmt->buffers->add(sqlBuffer);
               length = (int)ucs2_strlen((UCS2CHAR *)sqlBuffer) + 1;
            }
				else
				{
					sqlBuffer = buffer;
				}
#endif
			}
			else
			{
				if (cType == DB_CTYPE_STRING)
				{
					sqlBuffer = MBStringFromWideString((NETXMS_WCHAR *)buffer);
					stmt->buffers->add(sqlBuffer);
				}
				else if (cType == DB_CTYPE_UTF8_STRING)
            {
               sqlBuffer = MBStringFromUTF8String((char *)buffer);
               stmt->buffers->add(sqlBuffer);
               length = (int)strlen((char *)sqlBuffer) + 1;
            }
				else
				{
					sqlBuffer = buffer;
				}
			}
			break;
		case DB_BIND_DYNAMIC:
			if (m_useUnicode)
			{
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
					sqlBuffer = UCS2StringFromUCS4String((NETXMS_WCHAR *)buffer);
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
			}
			else
			{
				if (cType == DB_CTYPE_STRING)
				{
					sqlBuffer = MBStringFromWideString((NETXMS_WCHAR *)buffer);
					free(buffer);
				}
            else if (cType == DB_CTYPE_UTF8_STRING)
            {
               sqlBuffer = MBStringFromUTF8String((char *)buffer);
               free(buffer);
               length = (int)strlen((char *)sqlBuffer) + 1;
            }
				else
				{
					sqlBuffer = buffer;
				}
			}
			stmt->buffers->add(sqlBuffer);
			break;
		case DB_BIND_TRANSIENT:
			if (m_useUnicode)
			{
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
					sqlBuffer = UCS2StringFromUCS4String((NETXMS_WCHAR *)buffer);
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
			}
			else
			{
				if (cType == DB_CTYPE_STRING)
				{
					sqlBuffer = MBStringFromWideString((NETXMS_WCHAR *)buffer);
				}
				else if (cType == DB_CTYPE_UTF8_STRING)
            {
               sqlBuffer = MBStringFromUTF8String((char *)buffer);
               length = (int)strlen((char *)sqlBuffer) + 1;
            }
				else
				{
					sqlBuffer = MemCopyBlock(buffer, bufferSize[cType]);
				}
			}
			stmt->buffers->add(sqlBuffer);
			break;
		default:
			return;	// Invalid call
	}
	SQLBindParameter(stmt->handle, pos, SQL_PARAM_INPUT, m_useUnicode ? odbcCTypeW[cType] : odbcCTypeA[cType], odbcSqlType[sqlType],
	                 ((cType == DB_CTYPE_STRING) || (cType == DB_CTYPE_UTF8_STRING)) ? length : 0, 0, sqlBuffer, 0, nullptr);
}

/**
 * Execute prepared statement
 */
static uint32_t Execute(DBDRV_CONNECTION connection, DBDRV_STATEMENT hStmt, NETXMS_WCHAR *errorText)
{
   uint32_t result;

   static_cast<ODBCDRV_CONN*>(connection)->mutexQuery->lock();
   SQLHSTMT handle = static_cast<ODBCDRV_STATEMENT*>(hStmt)->handle;
	long rc = SQLExecute(handle);
   if ((rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO) || (rc == SQL_NO_DATA))
   {
		ClearPendingResults(handle);
      result = DBERR_SUCCESS;
   }
   else
   {
		result = GetSQLErrorInfo(SQL_HANDLE_STMT, handle, errorText);
   }
   static_cast<ODBCDRV_CONN*>(connection)->mutexQuery->unlock();
	return result;
}

/**
 * Destroy prepared statement
 */
static void FreeStatement(DBDRV_STATEMENT hStmt)
{
   auto stmt = static_cast<ODBCDRV_STATEMENT*>(hStmt);
	stmt->connection->mutexQuery->lock();
	SQLFreeHandle(SQL_HANDLE_STMT, stmt->handle);
	stmt->connection->mutexQuery->unlock();
	delete stmt->buffers;
	MemFree(stmt);
}

/**
 * Perform non-SELECT query
 */
static uint32_t Query(DBDRV_CONNECTION connection, const NETXMS_WCHAR *query, NETXMS_WCHAR *errorText)
{
   uint32_t dwResult;

   static_cast<ODBCDRV_CONN*>(connection)->mutexQuery->lock();

   // Allocate statement handle
   SQLHSTMT sqlStatement;
   SQLRETURN iResult = SQLAllocHandle(SQL_HANDLE_STMT, static_cast<ODBCDRV_CONN*>(connection)->sqlConn, &sqlStatement);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
   {
      // Execute statement
      if (m_useUnicode)
      {
#if defined(_WIN32) || defined(UNICODE_UCS2)
		   iResult = SQLExecDirectW(sqlStatement, (SQLWCHAR *)query, SQL_NTS);
#else
			SQLWCHAR *temp = UCS2StringFromUCS4String(query);
		   iResult = SQLExecDirectW(sqlStatement, temp, SQL_NTS);
		   free(temp);
#endif      
		}
		else
		{
			char *temp = MBStringFromWideString(query);
		   iResult = SQLExecDirectA(sqlStatement, (SQLCHAR *)temp, SQL_NTS);
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
         dwResult = GetSQLErrorInfo(SQL_HANDLE_STMT, sqlStatement, errorText);
      }
      SQLFreeHandle(SQL_HANDLE_STMT, sqlStatement);
   }
   else
   {
      dwResult = GetSQLErrorInfo(SQL_HANDLE_DBC, static_cast<ODBCDRV_CONN*>(connection)->sqlConn, errorText);
   }

	static_cast<ODBCDRV_CONN*>(connection)->mutexQuery->unlock();
   return dwResult;
}

/**
 * Get complete field data
 */
static NETXMS_WCHAR *GetFieldData(SQLHSTMT sqlStatement, short column)
{
   NETXMS_WCHAR *result = NULL;
   SQLLEN dataSize;
   if (m_useUnicode)
   {
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
            size_t tempSize = sizeof(buffer) * 4;
            WCHAR *temp = (WCHAR *)malloc(tempSize);
            memcpy(temp, buffer, sizeof(buffer));
            size_t offset = sizeof(buffer) - sizeof(WCHAR);
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
#else
      UCS2CHAR buffer[256];
      SQLRETURN rc = SQLGetData(sqlStatement, column, SQL_C_WCHAR, buffer, sizeof(buffer), &dataSize);
      if (((rc == SQL_SUCCESS) || ((rc == SQL_SUCCESS_WITH_INFO) && (dataSize >= 0) && (dataSize <= (SQLLEN)(sizeof(buffer) - sizeof(UCS2CHAR))))) && (dataSize != SQL_NULL_DATA))
      {
         int len = ucs2_strlen(buffer);
         result = (NETXMS_WCHAR *)MemAlloc((len + 1) * sizeof(NETXMS_WCHAR));
         ucs2_to_ucs4(buffer, -1, result, len + 1);
      }
      else if ((rc == SQL_SUCCESS_WITH_INFO) && (dataSize != SQL_NULL_DATA))
      {
         if (dataSize > (SQLLEN)(sizeof(buffer) - sizeof(UCS2CHAR)))
         {
            UCS2CHAR *temp = (UCS2CHAR *)MemAlloc(dataSize + sizeof(UCS2CHAR));
            memcpy(temp, buffer, sizeof(buffer));
            rc = SQLGetData(sqlStatement, column, SQL_C_WCHAR, &temp[255], dataSize - 254 * sizeof(UCS2CHAR), &dataSize);
            if ((rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO))
            {
               int len = ucs2_strlen(temp);
               result = (NETXMS_WCHAR *)MemAlloc((len + 1) * sizeof(NETXMS_WCHAR));
               ucs2_to_ucs4(temp, -1, result, len + 1);
            }
            MemFree(temp);
         }
         else if (dataSize == SQL_NO_TOTAL)
         {
            size_t tempSize = sizeof(buffer) * 4;
            UCS2CHAR *temp = (UCS2CHAR *)MemAlloc(tempSize);
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
               temp = (UCS2CHAR *)MemRealloc(temp, tempSize);
               offset += readSize - sizeof(UCS2CHAR);
            }
            int len = ucs2_strlen(temp);
            result = (NETXMS_WCHAR *)MemAlloc((len + 1) * sizeof(NETXMS_WCHAR));
            ucs2_to_ucs4(temp, -1, result, len + 1);
            MemFree(temp);
         }
      }
#endif
   }
   else
   {
      char buffer[256];
      SQLRETURN rc = SQLGetData(sqlStatement, column, SQL_C_CHAR, buffer, sizeof(buffer), &dataSize);
      if (((rc == SQL_SUCCESS) || ((rc == SQL_SUCCESS_WITH_INFO) && (dataSize >= 0) && (dataSize <= (SQLLEN)(sizeof(buffer) - 1)))) && (dataSize != SQL_NULL_DATA))
      {
         result = WideStringFromMBString(buffer);
      }
      else if ((rc == SQL_SUCCESS_WITH_INFO) && (dataSize != SQL_NULL_DATA))
      {
         if (dataSize > (SQLLEN)(sizeof(buffer) - 1))
         {
            char *temp = (char *)malloc(dataSize + 1);
            memcpy(temp, buffer, sizeof(buffer));
            rc = SQLGetData(sqlStatement, column, SQL_C_CHAR, &temp[255], dataSize - 254, &dataSize);
            if ((rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO))
            {
               result = WideStringFromMBString(temp);
            }
            free(temp);
         }
         else if (dataSize == SQL_NO_TOTAL)
         {
            size_t tempSize = sizeof(buffer) * 4;
            char *temp = (char *)malloc(tempSize);
            memcpy(temp, buffer, sizeof(buffer));
            size_t offset = sizeof(buffer) - 1;
            while(true)
            {
               SQLLEN readSize = tempSize - offset;
               rc = SQLGetData(sqlStatement, column, SQL_C_CHAR, &temp[offset], readSize, &dataSize);
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
               temp = (char *)realloc(temp, tempSize);
               offset += readSize - 1;
            }
            result = WideStringFromMBString(temp);
            free(temp);
         }
      }
   }
   return (result != nullptr) ? result : MemCopyStringW(L"");
}

/**
 * Process results of SELECT query
 */
static ODBCDRV_QUERY_RESULT *ProcessSelectResults(SQLHSTMT stmt)
{
   // Allocate result buffer and determine column info
   auto pResult = MemAllocStruct<ODBCDRV_QUERY_RESULT>();
   short wNumCols;
   SQLNumResultCols(stmt, &wNumCols);
   pResult->numColumns = wNumCols;

	// Get column names
	pResult->columnNames = MemAllocArray<char*>(pResult->numColumns);
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
   while(iResult = SQLFetch(stmt), (iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
   {
      pResult->numRows++;
      pResult->pValues = (NETXMS_WCHAR **)realloc(pResult->pValues, 
                  sizeof(NETXMS_WCHAR *) * (pResult->numRows * pResult->numColumns));
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
static DBDRV_RESULT Select(DBDRV_CONNECTION connection, const NETXMS_WCHAR *query, uint32_t *errorCode, NETXMS_WCHAR *errorText)
{
   ODBCDRV_QUERY_RESULT *pResult = nullptr;

   static_cast<ODBCDRV_CONN*>(connection)->mutexQuery->lock();

   // Allocate statement handle
   SQLHSTMT sqlStatement;
   SQLRETURN iResult = SQLAllocHandle(SQL_HANDLE_STMT, static_cast<ODBCDRV_CONN*>(connection)->sqlConn, &sqlStatement);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
   {
      // Execute statement
      if (m_useUnicode)
      {
#if defined(_WIN32) || defined(UNICODE_UCS2)
	      iResult = SQLExecDirectW(sqlStatement, (SQLWCHAR *)query, SQL_NTS);
#else
			SQLWCHAR *temp = UCS2StringFromUCS4String(query);
		   iResult = SQLExecDirectW(sqlStatement, temp, SQL_NTS);
		   MemFree(temp);
#endif
		}
		else
		{
			char *temp = MBStringFromWideString(query);
		   iResult = SQLExecDirectA(sqlStatement, (SQLCHAR *)temp, SQL_NTS);
		   MemFree(temp);
		}
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
      *errorCode = GetSQLErrorInfo(SQL_HANDLE_DBC, static_cast<ODBCDRV_CONN*>(connection)->sqlConn, errorText);
   }

	static_cast<ODBCDRV_CONN*>(connection)->mutexQuery->unlock();
   return pResult;
}

/**
 * Perform SELECT query using prepared statement
 */
static DBDRV_RESULT SelectPrepared(DBDRV_CONNECTION connection, DBDRV_STATEMENT hStmt, uint32_t *errorCode, NETXMS_WCHAR *errorText)
{
   ODBCDRV_QUERY_RESULT *result = nullptr;

   static_cast<ODBCDRV_CONN*>(connection)->mutexQuery->lock();
   SQLHSTMT handle = static_cast<ODBCDRV_STATEMENT*>(hStmt)->handle;
	long rc = SQLExecute(handle);
   if ((rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO))
   {
		result = ProcessSelectResults(handle);
	   *errorCode = DBERR_SUCCESS;
   }
   else
   {
		*errorCode = GetSQLErrorInfo(SQL_HANDLE_STMT, handle, errorText);
   }
   static_cast<ODBCDRV_CONN*>(connection)->mutexQuery->unlock();
	return result;
}

/**
 * Get field length from result
 */
static int32_t GetFieldLength(DBDRV_RESULT hResult, int iRow, int iColumn)
{
   auto result = static_cast<ODBCDRV_QUERY_RESULT*>(hResult);
   int32_t nLen = -1;
   if ((iRow < result->numRows) && (iRow >= 0) &&
       (iColumn < result->numColumns) && (iColumn >= 0))
      nLen = static_cast<int32_t>(wcslen(result->pValues[iRow * result->numColumns + iColumn]));
   return nLen;
}

/**
 * Get field value from result
 */
static NETXMS_WCHAR *GetField(DBDRV_RESULT hResult, int iRow, int iColumn, NETXMS_WCHAR *pBuffer, int nBufSize)
{
   auto result = static_cast<ODBCDRV_QUERY_RESULT*>(hResult);
   NETXMS_WCHAR *pValue = nullptr;
   if ((iRow < result->numRows) && (iRow >= 0) && (iColumn < result->numColumns) && (iColumn >= 0))
   {
#ifdef _WIN32
      wcsncpy_s(pBuffer, nBufSize, result->pValues[iRow * result->numColumns + iColumn], _TRUNCATE);
#else
      wcsncpy(pBuffer, result->pValues[iRow * result->numColumns + iColumn], nBufSize);
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
   return static_cast<ODBCDRV_QUERY_RESULT*>(hResult)->numRows;
}

/**
 * Get column count in query result
 */
static int GetColumnCount(DBDRV_RESULT hResult)
{
	return static_cast<ODBCDRV_QUERY_RESULT*>(hResult)->numColumns;
}

/**
 * Get column name in query result
 */
static const char *GetColumnName(DBDRV_RESULT hResult, int column)
{
	return ((column >= 0) && (column < static_cast<ODBCDRV_QUERY_RESULT*>(hResult)->numColumns)) ? static_cast<ODBCDRV_QUERY_RESULT*>(hResult)->columnNames[column] : nullptr;
}

/**
 * Free SELECT results
 */
static void FreeResult(DBDRV_RESULT hResult)
{
   auto result = static_cast<ODBCDRV_QUERY_RESULT*>(hResult);

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
static DBDRV_UNBUFFERED_RESULT SelectUnbuffered(DBDRV_CONNECTION connection, const NETXMS_WCHAR *query, uint32_t *errorCode, NETXMS_WCHAR *errorText)
{
   ODBCDRV_UNBUFFERED_QUERY_RESULT *pResult = NULL;
   long iResult;
   short wNumCols;
	int i;

	static_cast<ODBCDRV_CONN*>(connection)->mutexQuery->lock();

   // Allocate statement handle
   SQLHSTMT sqlStatement;
   iResult = SQLAllocHandle(SQL_HANDLE_STMT, static_cast<ODBCDRV_CONN*>(connection)->sqlConn, &sqlStatement);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
   {
      // Execute statement
      if (m_useUnicode)
      {
#if defined(_WIN32) || defined(UNICODE_UCS2)
	      iResult = SQLExecDirectW(sqlStatement, (SQLWCHAR *)query, SQL_NTS);
#else
			SQLWCHAR *temp = UCS2StringFromUCS4String(query);
		   iResult = SQLExecDirectW(sqlStatement, temp, SQL_NTS);
		   free(temp);
#endif
		}
		else
		{
			char *temp = MBStringFromWideString(query);
		   iResult = SQLExecDirectA(sqlStatement, (SQLCHAR *)temp, SQL_NTS);
		   free(temp);
		}
	   if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
      {
         // Allocate result buffer and determine column info
         pResult = MemAllocStruct<ODBCDRV_UNBUFFERED_QUERY_RESULT>();
         pResult->sqlStatement = sqlStatement;
         pResult->isPrepared = false;
         
         SQLNumResultCols(sqlStatement, &wNumCols);
         pResult->numColumns = wNumCols;
         pResult->pConn = static_cast<ODBCDRV_CONN*>(connection);
         pResult->noMoreRows = false;

			// Get column names
			pResult->columnNames = (char **)malloc(sizeof(char *) * pResult->numColumns);
			for(i = 0; i < pResult->numColumns; i++)
			{
				char name[256];
				SQLSMALLINT len;
				iResult = SQLColAttributeA(sqlStatement, (SQLSMALLINT)(i + 1), SQL_DESC_NAME, name, 256, &len, nullptr);
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

			// Column values cache
			pResult->values = (NETXMS_WCHAR **)malloc(sizeof(NETXMS_WCHAR *) * pResult->numColumns);
			memset(pResult->values, 0, sizeof(NETXMS_WCHAR *) * pResult->numColumns);

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
      *errorCode = GetSQLErrorInfo(SQL_HANDLE_DBC, static_cast<ODBCDRV_CONN*>(connection)->sqlConn, errorText);
   }

   if (pResult == NULL) // Unlock mutex if query has failed
      static_cast<ODBCDRV_CONN*>(connection)->mutexQuery->unlock();
   return pResult;
}

/**
 * Perform unbuffered SELECT query using prepared statement
 */
static DBDRV_UNBUFFERED_RESULT SelectPreparedUnbuffered(DBDRV_CONNECTION connection, DBDRV_STATEMENT hStmt, uint32_t *errorCode, NETXMS_WCHAR *errorText)
{
   ODBCDRV_UNBUFFERED_QUERY_RESULT *pResult = nullptr;

   static_cast<ODBCDRV_CONN*>(connection)->mutexQuery->lock();
   SQLHSTMT handle = static_cast<ODBCDRV_STATEMENT*>(hStmt)->handle;
	SQLRETURN rc = SQLExecute(handle);
   if ((rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO))
   {
      // Allocate result buffer and determine column info
      pResult = MemAllocStruct<ODBCDRV_UNBUFFERED_QUERY_RESULT>();
      pResult->sqlStatement = handle;
      pResult->isPrepared = true;
      
      short wNumCols;
      SQLNumResultCols(pResult->sqlStatement, &wNumCols);
      pResult->numColumns = wNumCols;
      pResult->pConn = static_cast<ODBCDRV_CONN*>(connection);
      pResult->noMoreRows = false;
		pResult->values = (NETXMS_WCHAR **)malloc(sizeof(NETXMS_WCHAR *) * pResult->numColumns);
      memset(pResult->values, 0, sizeof(NETXMS_WCHAR *) * pResult->numColumns);

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
      static_cast<ODBCDRV_CONN*>(connection)->mutexQuery->unlock();
	return pResult;
}

/**
 * Fetch next result line from asynchronous SELECT results
 */
static bool Fetch(DBDRV_UNBUFFERED_RESULT hResult)
{
   auto result = static_cast<ODBCDRV_UNBUFFERED_QUERY_RESULT*>(hResult);
   bool success = false;
   SQLRETURN rc = SQLFetch(result->sqlStatement);
   success = ((rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO));
   if (success)
   {
      for(int i = 0; i < result->numColumns; i++)
      {
         MemFree(result->values[i]);
         result->values[i] = GetFieldData(result->sqlStatement, (short)i + 1);
      }
   }
   else
   {
      result->noMoreRows = true;
   }
   return success;
}

/**
 * Get field length from async query result
 */
static int32_t GetFieldLengthUnbuffered(DBDRV_UNBUFFERED_RESULT hResult, int col)
{
   auto result = static_cast<ODBCDRV_UNBUFFERED_QUERY_RESULT*>(hResult);
   if ((col >= result->numColumns) || (col < 0))
      return -1;

   return (result->values[col] != nullptr) ? static_cast<int32_t>(wcslen(result->values[col])) : -1;
}

/**
 * Get field from current row in async query result
 */
static NETXMS_WCHAR *GetFieldUnbuffered(DBDRV_UNBUFFERED_RESULT hResult, int col, NETXMS_WCHAR *buffer, int bufferSize)
{
   auto result = static_cast<ODBCDRV_UNBUFFERED_QUERY_RESULT*>(hResult);

   // Check if there are valid fetched row
   if (result->noMoreRows)
      return nullptr;

   if ((col >= 0) && (col < result->numColumns) && (result->values[col] != nullptr))
   {
      wcsncpy(buffer, result->values[col], bufferSize - 1);
      buffer[bufferSize - 1] = 0;
   }
   else
   {
      buffer[0] = 0;
   }
   return buffer;
}

/**
 * Get column count in async query result
 */
static int GetColumnCountUnbuffered(DBDRV_UNBUFFERED_RESULT hResult)
{
	return static_cast<ODBCDRV_UNBUFFERED_QUERY_RESULT*>(hResult)->numColumns;
}

/**
 * Get column name in async query result
 */
static const char *GetColumnNameUnbuffered(DBDRV_UNBUFFERED_RESULT hResult, int column)
{
	return ((column >= 0) && (column < static_cast<ODBCDRV_UNBUFFERED_QUERY_RESULT*>(hResult)->numColumns)) ? static_cast<ODBCDRV_UNBUFFERED_QUERY_RESULT*>(hResult)->columnNames[column] : nullptr;
}

/**
 * Destroy result of async query
 */
static void FreeUnbufferedResult(DBDRV_UNBUFFERED_RESULT hResult)
{
   auto result = static_cast<ODBCDRV_UNBUFFERED_QUERY_RESULT*>(hResult);
   if (result->isPrepared)
      SQLCloseCursor(result->sqlStatement);
   else
      SQLFreeHandle(SQL_HANDLE_STMT, result->sqlStatement);
   for(int i = 0; i < result->numColumns; i++)
   {
      MemFree(result->columnNames[i]);
      MemFree(result->values[i]);
   }
   MemFree(result->columnNames);
   MemFree(result->values);
   result->pConn->mutexQuery->unlock();
   MemFree(result);
}

/**
 * Begin transaction
 */
static uint32_t Begin(DBDRV_CONNECTION connection)
{
   uint32_t result;
   static_cast<ODBCDRV_CONN*>(connection)->mutexQuery->lock();
	SQLRETURN nRet = SQLSetConnectAttr(static_cast<ODBCDRV_CONN*>(connection)->sqlConn, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0);
   if ((nRet == SQL_SUCCESS) || (nRet == SQL_SUCCESS_WITH_INFO))
   {
      result = DBERR_SUCCESS;
   }
   else
   {
      result = GetSQLErrorInfo(SQL_HANDLE_DBC, static_cast<ODBCDRV_CONN*>(connection)->sqlConn, nullptr);
   }
   static_cast<ODBCDRV_CONN*>(connection)->mutexQuery->unlock();
   return result;
}

/**
 * Commit transaction
 */
static uint32_t Commit(DBDRV_CONNECTION connection)
{
   static_cast<ODBCDRV_CONN*>(connection)->mutexQuery->lock();
	SQLRETURN nRet = SQLEndTran(SQL_HANDLE_DBC, static_cast<ODBCDRV_CONN*>(connection)->sqlConn, SQL_COMMIT);
   SQLSetConnectAttr(static_cast<ODBCDRV_CONN*>(connection)->sqlConn, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
   static_cast<ODBCDRV_CONN*>(connection)->mutexQuery->unlock();
   return ((nRet == SQL_SUCCESS) || (nRet == SQL_SUCCESS_WITH_INFO)) ? DBERR_SUCCESS : DBERR_OTHER_ERROR;
}

/**
 * Rollback transaction
 */
static uint32_t Rollback(DBDRV_CONNECTION connection)
{
   static_cast<ODBCDRV_CONN*>(connection)->mutexQuery->lock();
	SQLRETURN nRet = SQLEndTran(SQL_HANDLE_DBC, static_cast<ODBCDRV_CONN*>(connection)->sqlConn, SQL_ROLLBACK);
   SQLSetConnectAttr(static_cast<ODBCDRV_CONN*>(connection)->sqlConn, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
   static_cast<ODBCDRV_CONN*>(connection)->mutexQuery->unlock();
   return ((nRet == SQL_SUCCESS) || (nRet == SQL_SUCCESS_WITH_INFO)) ? DBERR_SUCCESS : DBERR_OTHER_ERROR;
}

/**
 * Check if table exist
 */
static int IsTableExist(DBDRV_CONNECTION connection, const NETXMS_WCHAR *name)
{
   int rc = DBIsTableExist_Failure;

   static_cast<ODBCDRV_CONN*>(connection)->mutexQuery->lock();

   SQLHSTMT sqlStatement;
   SQLRETURN iResult = SQLAllocHandle(SQL_HANDLE_STMT, static_cast<ODBCDRV_CONN*>(connection)->sqlConn, &sqlStatement);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
   {
      if (m_useUnicode)
      {
#if defined(_WIN32) || defined(UNICODE_UCS2)
	      iResult = SQLTablesW(sqlStatement, nullptr, 0, nullptr, 0, (SQLWCHAR *)name, SQL_NTS, nullptr, 0);
#else
			SQLWCHAR *temp = UCS2StringFromUCS4String(name);
	      iResult = SQLTablesW(sqlStatement, nullptr, 0, nullptr, 0, (SQLWCHAR *)temp, SQL_NTS, nullptr, 0);
		   MemFree(temp);
#endif
		}
		else
		{
			char *temp = MBStringFromWideString(name);
	      iResult = SQLTablesA(sqlStatement, nullptr, 0, nullptr, 0, (SQLCHAR *)temp, SQL_NTS, nullptr, 0);
		   MemFree(temp);
		}
	   if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
      {
			ODBCDRV_QUERY_RESULT *pResult = ProcessSelectResults(sqlStatement);
         rc = (GetNumRows(pResult) > 0) ? DBIsTableExist_Found : DBIsTableExist_NotFound;
         FreeResult(pResult);
      }
      SQLFreeHandle(SQL_HANDLE_STMT, sqlStatement);
   }

	static_cast<ODBCDRV_CONN*>(connection)->mutexQuery->unlock();
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

DB_DRIVER_ENTRY_POINT("ODBC", s_callTable)

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
