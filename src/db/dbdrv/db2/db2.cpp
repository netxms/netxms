/* 
** DB2 Database Driver
** Copyright (C) 2010-2025 Raden Solutions
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
 * Check CLI return code for success
 */
static inline bool IsSuccess(SQLRETURN rc)
{
   return (rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO);
}

/**
 * Convert UCS-2 string returned by CLI to driver's wide string (new allocation)
 */
static inline NETXMS_WCHAR *WideStringFromSQLWCHAR(const SQLWCHAR *s)
{
#if defined(_WIN32) || defined(UNICODE_UCS2)
   return MemCopyStringW(reinterpret_cast<const WCHAR*>(s));
#else
   return UCS4StringFromUCS2String(reinterpret_cast<const UCS2CHAR*>(s));
#endif
}

/**
 * Convert driver's wide string to UCS-2 string for CLI. On UCS-2 platforms returns
 * input pointer without allocation, otherwise returns new allocation (caller must free).
 */
static inline SQLWCHAR *SQLWCHARFromWideString(const NETXMS_WCHAR *s, bool *allocated)
{
#if defined(_WIN32) || defined(UNICODE_UCS2)
   *allocated = false;
   return reinterpret_cast<SQLWCHAR*>(const_cast<NETXMS_WCHAR*>(s));
#else
   *allocated = true;
   return reinterpret_cast<SQLWCHAR*>(UCS2StringFromUCS4String(s));
#endif
}

/**
 * Convert DB2 state to NetXMS database error code and get error text
 */
static uint32_t GetSQLErrorInfo(SQLSMALLINT handleType, SQLHANDLE handle, NETXMS_WCHAR *errorText)
{
   uint32_t error = DBERR_OTHER_ERROR;
   SQLSMALLINT chars;
   SQLWCHAR buffer[DBDRV_MAX_ERROR_TEXT];

   // Get state information and convert it to NetXMS database error code
   SQLRETURN rc = SQLGetDiagFieldW(handleType, handle, 1, SQL_DIAG_SQLSTATE, buffer, 16, &chars);
   if (rc == SQL_SUCCESS)
   {
      char state[16];
      ucs2_to_mb(reinterpret_cast<UCS2CHAR*>(buffer), -1, state, 16);
      if (!strcmp(state, "08003") ||   // Connection does not exist
          !strcmp(state, "08S01") ||   // Communication link failure
          !strcmp(state, "HYT00") ||   // Timeout expired
          !strcmp(state, "HYT01") ||   // Connection timeout expired
          !strcmp(state, "08506"))     // SQL30108N: A connection failed but has been re-established.
      {
         error = DBERR_CONNECTION_LOST;
      }
   }

   // Get error message
   if (errorText != nullptr)
   {
      rc = SQLGetDiagFieldW(handleType, handle, 1, SQL_DIAG_MESSAGE_TEXT, buffer, sizeof(buffer), &chars);
      if (rc == SQL_SUCCESS)
      {
         buffer[DBDRV_MAX_ERROR_TEXT - 1] = 0;
#if defined(_WIN32) || defined(UNICODE_UCS2)
         wcslcpy(errorText, reinterpret_cast<WCHAR*>(buffer), DBDRV_MAX_ERROR_TEXT);
#else
         ucs2_to_ucs4(reinterpret_cast<UCS2CHAR*>(buffer), -1, errorText, DBDRV_MAX_ERROR_TEXT);
#endif
         RemoveTrailingCRLFW(errorText);
      }
      else
      {
         wcscpy(errorText, L"Unable to obtain description for this error");
      }
   }

   return error;
}

/**
 * Clear any pending result sets on given statement
 */
static void ClearPendingResults(SQLHSTMT statement)
{
   while(IsSuccess(SQLMoreResults(statement)))
      ;
}

/**
 * Execute query text on statement handle (wraps UCS-2 conversion on UCS-4 platforms)
 */
static SQLRETURN ExecDirect(SQLHSTMT statement, const NETXMS_WCHAR *query)
{
   bool allocated;
   SQLWCHAR *text = SQLWCHARFromWideString(query, &allocated);
   SQLRETURN rc = SQLExecDirectW(statement, text, SQL_NTS);
   if (allocated)
      MemFree(text);
   return rc;
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
 * host should be set to DB2 catalog alias, database name is ignored
 */
static DBDRV_CONNECTION Connect(const char *host, const char *login, const char *password, const char *database, const char *schema, NETXMS_WCHAR *errorText)
{
   // Allocate our connection structure
   auto pConn = MemAllocStruct<DB2DRV_CONN>();

   // Allocate environment
   SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &pConn->sqlEnv);
   if (!IsSuccess(rc))
   {
      wcscpy(errorText, L"Cannot allocate environment handle");
      goto connect_failure_0;
   }

   // Set required DB2 version
   rc = SQLSetEnvAttr(pConn->sqlEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
   if (!IsSuccess(rc))
   {
      wcscpy(errorText, L"Call to SQLSetEnvAttr failed");
      goto connect_failure_1;
   }

   // Allocate connection handle, set timeout
   rc = SQLAllocHandle(SQL_HANDLE_DBC, pConn->sqlEnv, &pConn->sqlConn); 
   if (!IsSuccess(rc))
   {
      wcscpy(errorText, L"Cannot allocate connection handle");
      goto connect_failure_1;
   }
   SQLSetConnectAttr(pConn->sqlConn, SQL_ATTR_LOGIN_TIMEOUT, (SQLPOINTER)15, 0);
   SQLSetConnectAttr(pConn->sqlConn, SQL_ATTR_CONNECTION_TIMEOUT, (SQLPOINTER)30, 0);

   // Connect to the datasource 
   rc = SQLConnect(pConn->sqlConn, (SQLCHAR *)host, SQL_NTS, (SQLCHAR *)login, SQL_NTS, (SQLCHAR *)password, SQL_NTS);
   if (!IsSuccess(rc))
   {
      GetSQLErrorInfo(SQL_HANDLE_DBC, pConn->sqlConn, errorText);
      goto connect_failure_2;
   }

   // Explicitly set autocommit mode
   SQLSetConnectAttr(pConn->sqlConn, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);

   // Set current schema
   if ((schema != nullptr) && (schema[0] != 0))
   {
      SQLHSTMT sqlStatement;
      rc = SQLAllocHandle(SQL_HANDLE_STMT, pConn->sqlConn, &sqlStatement);
      if (!IsSuccess(rc))
      {
         GetSQLErrorInfo(SQL_HANDLE_DBC, pConn->sqlConn, errorText);
         goto connect_failure_3;
      }

      // Build "SET CURRENT SCHEMA = 'name'" with quotes in name doubled
      WCHAR *schemaW = WideStringFromUTF8String(schema);
      WCHAR *query = MemAllocStringW(wcslen(schemaW) * 2 + 32);
      wcscpy(query, L"SET CURRENT SCHEMA = '");
      WCHAR *p = query + wcslen(query);
      for(const WCHAR *s = schemaW; *s != 0; s++)
      {
         if (*s == L'\'')
            *p++ = L'\'';
         *p++ = *s;
      }
      *p++ = L'\'';
      *p = 0;
      MemFree(schemaW);
      rc = ExecDirect(sqlStatement, query);
      MemFree(query);
      if (!IsSuccess(rc) && (rc != SQL_NO_DATA))
      {
         GetSQLErrorInfo(SQL_HANDLE_STMT, sqlStatement, errorText);
         SQLFreeHandle(SQL_HANDLE_STMT, sqlStatement);
         goto connect_failure_3;
      }
      SQLFreeHandle(SQL_HANDLE_STMT, sqlStatement);
   }

   // Create mutex
   pConn->mutexQuery = new Mutex(MutexType::NORMAL);

   // Success
   return pConn;

   // Handle failures
connect_failure_3:
   SQLDisconnect(pConn->sqlConn);

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
static DBDRV_STATEMENT Prepare(DBDRV_CONNECTION connection, const NETXMS_WCHAR *query, bool optimizeForReuse, uint32_t *errorCode, NETXMS_WCHAR *errorText)
{
   auto c = static_cast<DB2DRV_CONN*>(connection);
   DB2DRV_STATEMENT *result = nullptr;

   c->mutexQuery->lock();

   // Allocate statement handle
   SQLHSTMT statement;
   SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_STMT, c->sqlConn, &statement);
   if (IsSuccess(rc))
   {
      // Prepare statement
      bool allocated;
      SQLWCHAR *text = SQLWCHARFromWideString(query, &allocated);
      rc = SQLPrepareW(statement, text, SQL_NTS);
      if (allocated)
         MemFree(text);
      if (IsSuccess(rc))
      {
         result = MemAllocStruct<DB2DRV_STATEMENT>();
         result->handle = statement;
         result->buffers = new Array(0, 16, Ownership::True);
         result->connection = c;
         *errorCode = DBERR_SUCCESS;
      }
      else
      {
         *errorCode = GetSQLErrorInfo(SQL_HANDLE_STMT, statement, errorText);
         SQLFreeHandle(SQL_HANDLE_STMT, statement);
      }
   }
   else
   {
      *errorCode = GetSQLErrorInfo(SQL_HANDLE_DBC, c->sqlConn, errorText);
   }

   c->mutexQuery->unlock();
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
   int length = 0;   // string length in characters including terminator (for CLI ColumnSize)
   SQLPOINTER sqlBuffer;
   bool owned;       // true if sqlBuffer should be freed with statement

   if (cType == DB_CTYPE_UTF8_STRING)
   {
      // CLI always needs UCS-2 string, so conversion is unavoidable
      sqlBuffer = UCS2StringFromUTF8String(static_cast<char*>(buffer));
      length = static_cast<int>(ucs2_strlen(static_cast<UCS2CHAR*>(sqlBuffer))) + 1;
      owned = true;
      if (allocType == DB_BIND_DYNAMIC)
         MemFree(buffer);
   }
   else if (cType == DB_CTYPE_STRING)
   {
#if defined(_WIN32) || defined(UNICODE_UCS2)
      length = static_cast<int>(wcslen(static_cast<WCHAR*>(buffer))) + 1;
      switch(allocType)
      {
         case DB_BIND_STATIC:
            sqlBuffer = buffer;
            owned = false;
            break;
         case DB_BIND_DYNAMIC:
            sqlBuffer = buffer;
            owned = true;
            break;
         default:   // DB_BIND_TRANSIENT
            sqlBuffer = MemCopyBlock(buffer, length * sizeof(WCHAR));
            owned = true;
            break;
      }
#else
      sqlBuffer = UCS2StringFromUCS4String(static_cast<WCHAR*>(buffer));
      length = static_cast<int>(ucs2_strlen(static_cast<UCS2CHAR*>(sqlBuffer))) + 1;
      owned = true;
      if (allocType == DB_BIND_DYNAMIC)
         MemFree(buffer);
#endif
   }
   else
   {
      switch(allocType)
      {
         case DB_BIND_STATIC:
            sqlBuffer = buffer;
            owned = false;
            break;
         case DB_BIND_DYNAMIC:
            sqlBuffer = buffer;
            owned = true;
            break;
         default:   // DB_BIND_TRANSIENT
            sqlBuffer = MemCopyBlock(buffer, bufferSize[cType]);
            owned = true;
            break;
      }
   }

   if (owned)
      statement->buffers->add(sqlBuffer);
   SQLBindParameter(statement->handle, pos, SQL_PARAM_INPUT, odbcCType[cType], odbcSqlType[sqlType], length, 0, sqlBuffer, 0, nullptr);
}

/**
 * Execute prepared statement
 */
static uint32_t Execute(DBDRV_CONNECTION connection, DBDRV_STATEMENT hStmt, NETXMS_WCHAR *errorText)
{
   auto c = static_cast<DB2DRV_CONN*>(connection);
   uint32_t result;

   c->mutexQuery->lock();
   SQLHSTMT handle = static_cast<DB2DRV_STATEMENT*>(hStmt)->handle;
   SQLRETURN rc = SQLExecute(handle);
   if (IsSuccess(rc) || (rc == SQL_NO_DATA))
   {
      ClearPendingResults(handle);
      result = DBERR_SUCCESS;
   }
   else
   {
      result = GetSQLErrorInfo(SQL_HANDLE_STMT, handle, errorText);
   }
   c->mutexQuery->unlock();
   return result;
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
static uint32_t Query(DBDRV_CONNECTION connection, const NETXMS_WCHAR *query, NETXMS_WCHAR *errorText)
{
   auto c = static_cast<DB2DRV_CONN*>(connection);
   uint32_t result;

   c->mutexQuery->lock();

   // Allocate statement handle
   SQLHSTMT sqlStatement;
   SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_STMT, c->sqlConn, &sqlStatement);
   if (IsSuccess(rc))
   {
      rc = ExecDirect(sqlStatement, query);
      if (IsSuccess(rc) || (rc == SQL_NO_DATA))
      {
         result = DBERR_SUCCESS;
      }
      else
      {
         result = GetSQLErrorInfo(SQL_HANDLE_STMT, sqlStatement, errorText);
      }
      SQLFreeHandle(SQL_HANDLE_STMT, sqlStatement);
   }
   else
   {
      result = GetSQLErrorInfo(SQL_HANDLE_DBC, c->sqlConn, errorText);
   }

   c->mutexQuery->unlock();
   return result;
}

/**
 * Get complete field data from current row. Never returns nullptr (NULL and errors are returned as empty string).
 */
static NETXMS_WCHAR *GetFieldData(SQLHSTMT sqlStatement, short column)
{
   SQLWCHAR buffer[256];
   SQLLEN dataSize;
   SQLRETURN rc = SQLGetData(sqlStatement, column, SQL_C_WCHAR, buffer, sizeof(buffer), &dataSize);
   if (!IsSuccess(rc) || (dataSize == SQL_NULL_DATA))
      return MemCopyStringW(L"");

   // Complete value fits into buffer (SQL_SUCCESS_WITH_INFO can be a non-truncation warning)
   if ((rc == SQL_SUCCESS) || ((dataSize != SQL_NO_TOTAL) && (dataSize <= static_cast<SQLLEN>(sizeof(buffer) - sizeof(SQLWCHAR)))))
      return WideStringFromSQLWCHAR(buffer);

   // Value truncated: first call returned 255 characters, read the rest in a growing buffer.
   // On truncation CLI reports in dataSize total number of bytes that was available before the call,
   // so required capacity (in characters, with terminator) is used + dataSize / 2 + 1.
   size_t used = sizeof(buffer) / sizeof(SQLWCHAR) - 1;    // characters already received (without terminator)
   size_t capacity = (dataSize != SQL_NO_TOTAL) ? dataSize / sizeof(SQLWCHAR) + 1 : used + 4096;
   SQLWCHAR *temp = MemAllocArrayNoInit<SQLWCHAR>(capacity);
   memcpy(temp, buffer, used * sizeof(SQLWCHAR));
   while(true)
   {
      SQLLEN readSize = static_cast<SQLLEN>((capacity - used) * sizeof(SQLWCHAR));
      rc = SQLGetData(sqlStatement, column, SQL_C_WCHAR, &temp[used], readSize, &dataSize);
      if (rc == SQL_SUCCESS)
         break;   // remaining data with terminator placed at &temp[used]
      if (rc != SQL_SUCCESS_WITH_INFO)
      {
         // SQL_NO_DATA (nothing left after warning on previous call) or error
         temp[used] = 0;
         break;
      }
      size_t received = used;
      used = capacity - 1;
      capacity = (dataSize != SQL_NO_TOTAL) ? received + dataSize / sizeof(SQLWCHAR) + 1 : capacity + 4096;
      temp = MemReallocArray<SQLWCHAR>(temp, capacity);
   }
   NETXMS_WCHAR *result = WideStringFromSQLWCHAR(temp);
   MemFree(temp);
   return result;
}

/**
 * Get column names for result set
 */
static char **GetColumnNames(SQLHSTMT statement, int numColumns)
{
   char **names = MemAllocArray<char*>(numColumns);
   for(int i = 0; i < numColumns; i++)
   {
      SQLWCHAR name[256];
      SQLSMALLINT len;   // in bytes
      SQLRETURN rc = SQLColAttributeW(statement, static_cast<SQLSMALLINT>(i + 1), SQL_DESC_NAME, name, sizeof(name), &len, nullptr); 
      if (IsSuccess(rc))
      {
         size_t chars = static_cast<size_t>(len) / sizeof(SQLWCHAR);   // reported length is in bytes and may exceed buffer
         if (chars > sizeof(name) / sizeof(SQLWCHAR) - 1)
            chars = sizeof(name) / sizeof(SQLWCHAR) - 1;
         name[chars] = 0;
         names[i] = MBStringFromUCS2String(reinterpret_cast<UCS2CHAR*>(name));
      }
      else
      {
         names[i] = MemCopyStringA("");
      }
   }
   return names;
}

/**
 * Process results of SELECT query
 */
static DB2DRV_QUERY_RESULT *ProcessSelectResults(SQLHSTMT statement)
{
   // Allocate result buffer and determine column info
   DB2DRV_QUERY_RESULT *pResult = MemAllocStruct<DB2DRV_QUERY_RESULT>();
   short numColumns;
   SQLNumResultCols(statement, &numColumns);
   pResult->numColumns = numColumns;
   pResult->columnNames = GetColumnNames(statement, numColumns);

   // Fetch all data
   long allocated = 0;
   long currValue = 0;
   while(IsSuccess(SQLFetch(statement)))
   {
      pResult->numRows++;
      if (pResult->numRows * pResult->numColumns > allocated)
      {
         allocated += pResult->numColumns * 256;
         pResult->values = MemReallocArray<NETXMS_WCHAR*>(pResult->values, allocated);
      }
      for(int i = 1; i <= numColumns; i++)
      {
         pResult->values[currValue++] = GetFieldData(statement, static_cast<short>(i));
      }
   }

   return pResult;
}

/**
 * Perform SELECT query
 */
static DBDRV_RESULT Select(DBDRV_CONNECTION connection, const NETXMS_WCHAR *query, uint32_t *errorCode, NETXMS_WCHAR *errorText)
{
   auto c = static_cast<DB2DRV_CONN*>(connection);
   DB2DRV_QUERY_RESULT *pResult = nullptr;

   c->mutexQuery->lock();

   // Allocate statement handle
   SQLHSTMT sqlStatement;
   SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_STMT, c->sqlConn, &sqlStatement);
   if (IsSuccess(rc))
   {
      rc = ExecDirect(sqlStatement, query);
      if (IsSuccess(rc))
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
      *errorCode = GetSQLErrorInfo(SQL_HANDLE_DBC, c->sqlConn, errorText);
   }

   c->mutexQuery->unlock();
   return pResult;
}

/**
 * Perform SELECT query using prepared statement
 */
static DBDRV_RESULT SelectPrepared(DBDRV_CONNECTION connection, DBDRV_STATEMENT hStmt, uint32_t *errorCode, NETXMS_WCHAR *errorText)
{
   auto c = static_cast<DB2DRV_CONN*>(connection);
   DB2DRV_QUERY_RESULT *pResult = nullptr;

   c->mutexQuery->lock();
   SQLHSTMT handle = static_cast<DB2DRV_STATEMENT*>(hStmt)->handle;
   SQLRETURN rc = SQLExecute(handle);
   if (IsSuccess(rc))
   {
      pResult = ProcessSelectResults(handle);
      *errorCode = DBERR_SUCCESS;
   }
   else
   {
      *errorCode = GetSQLErrorInfo(SQL_HANDLE_STMT, handle, errorText);
   }
   c->mutexQuery->unlock();
   return pResult;
}

/**
 * Get field length from result
 */
static int32_t GetFieldLength(DBDRV_RESULT hResult, int row, int column)
{
   auto result = static_cast<DB2DRV_QUERY_RESULT*>(hResult);
   if ((row < 0) || (row >= result->numRows) || (column < 0) || (column >= result->numColumns))
      return -1;
   return static_cast<int32_t>(wcslen(result->values[row * result->numColumns + column]));
}

/**
 * Get field value from result
 */
static NETXMS_WCHAR *GetField(DBDRV_RESULT hResult, int row, int column, NETXMS_WCHAR *buffer, int bufferSize)
{
   auto result = static_cast<DB2DRV_QUERY_RESULT*>(hResult);
   if ((row < 0) || (row >= result->numRows) || (column < 0) || (column >= result->numColumns))
      return nullptr;
   wcslcpy(buffer, result->values[row * result->numColumns + column], bufferSize);
   return buffer;
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
   auto result = static_cast<DB2DRV_QUERY_RESULT*>(hResult);
   return ((column >= 0) && (column < result->numColumns)) ? result->columnNames[column] : nullptr;
}

/**
 * Free SELECT results
 */
static void FreeResult(DBDRV_RESULT hResult)
{
   auto result = static_cast<DB2DRV_QUERY_RESULT*>(hResult);

   long count = result->numColumns * result->numRows;
   for(long i = 0; i < count; i++)
      MemFree(result->values[i]);
   MemFree(result->values);

   for(int i = 0; i < result->numColumns; i++)
      MemFree(result->columnNames[i]);
   MemFree(result->columnNames);

   MemFree(result);
}

/**
 * Create unbuffered result object for successfully executed statement
 */
static DB2DRV_UNBUFFERED_QUERY_RESULT *CreateUnbufferedResult(DB2DRV_CONN *connection, SQLHSTMT statement, bool isPrepared)
{
   auto result = MemAllocStruct<DB2DRV_UNBUFFERED_QUERY_RESULT>();
   result->sqlStatement = statement;
   result->isPrepared = isPrepared;
   result->connection = connection;

   short numColumns;
   SQLNumResultCols(statement, &numColumns);
   result->numColumns = numColumns;
   result->columnNames = GetColumnNames(statement, numColumns);
   result->values = MemAllocArray<NETXMS_WCHAR*>(numColumns);
   return result;
}

/**
 * Perform unbuffered SELECT query
 */
static DBDRV_UNBUFFERED_RESULT SelectUnbuffered(DBDRV_CONNECTION connection, const NETXMS_WCHAR *query, uint32_t *errorCode, NETXMS_WCHAR *errorText)
{
   auto c = static_cast<DB2DRV_CONN*>(connection);
   DB2DRV_UNBUFFERED_QUERY_RESULT *pResult = nullptr;

   c->mutexQuery->lock();

   // Allocate statement handle
   SQLHSTMT sqlStatement;
   SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_STMT, c->sqlConn, &sqlStatement);
   if (IsSuccess(rc))
   {
      rc = ExecDirect(sqlStatement, query);
      if (IsSuccess(rc))
      {
         pResult = CreateUnbufferedResult(c, sqlStatement, false);
         *errorCode = DBERR_SUCCESS;
      }
      else
      {
         *errorCode = GetSQLErrorInfo(SQL_HANDLE_STMT, sqlStatement, errorText);
         SQLFreeHandle(SQL_HANDLE_STMT, sqlStatement);
      }
   }
   else
   {
      *errorCode = GetSQLErrorInfo(SQL_HANDLE_DBC, c->sqlConn, errorText);
   }

   if (pResult == nullptr) // Unlock mutex if query has failed
      c->mutexQuery->unlock();
   return pResult;
}

/**
 * Perform unbuffered SELECT query using prepared statement
 */
static DBDRV_UNBUFFERED_RESULT SelectPreparedUnbuffered(DBDRV_CONNECTION connection, DBDRV_STATEMENT hStmt, uint32_t *errorCode, NETXMS_WCHAR *errorText)
{
   auto c = static_cast<DB2DRV_CONN*>(connection);
   DB2DRV_UNBUFFERED_QUERY_RESULT *pResult = nullptr;

   c->mutexQuery->lock();
   SQLHSTMT handle = static_cast<DB2DRV_STATEMENT*>(hStmt)->handle;
   SQLRETURN rc = SQLExecute(handle);
   if (IsSuccess(rc))
   {
      pResult = CreateUnbufferedResult(c, handle, true);
      *errorCode = DBERR_SUCCESS;
   }
   else
   {
      *errorCode = GetSQLErrorInfo(SQL_HANDLE_STMT, handle, errorText);
   }

   if (pResult == nullptr) // Unlock mutex if query has failed
      c->mutexQuery->unlock();
   return pResult;
}

/**
 * Fetch next result line from unbuffered SELECT results. All column values are read
 * immediately because CLI allows SQLGetData for each column only once per row.
 */
static bool Fetch(DBDRV_UNBUFFERED_RESULT hResult)
{
   auto result = static_cast<DB2DRV_UNBUFFERED_QUERY_RESULT*>(hResult);
   for(int i = 0; i < result->numColumns; i++)
   {
      MemFree(result->values[i]);
      result->values[i] = nullptr;
   }

   if (!IsSuccess(SQLFetch(result->sqlStatement)))
      return false;

   for(int i = 0; i < result->numColumns; i++)
      result->values[i] = GetFieldData(result->sqlStatement, static_cast<short>(i + 1));
   return true;
}

/**
 * Get field length from unbuffered query result
 */
static int32_t GetFieldLengthUnbuffered(DBDRV_UNBUFFERED_RESULT hResult, int column)
{
   auto result = static_cast<DB2DRV_UNBUFFERED_QUERY_RESULT*>(hResult);
   if ((column < 0) || (column >= result->numColumns) || (result->values[column] == nullptr))
      return -1;
   return static_cast<int32_t>(wcslen(result->values[column]));
}

/**
 * Get field from current row in unbuffered query result
 */
static NETXMS_WCHAR *GetFieldUnbuffered(DBDRV_UNBUFFERED_RESULT hResult, int column, NETXMS_WCHAR *buffer, int bufferSize)
{
   auto result = static_cast<DB2DRV_UNBUFFERED_QUERY_RESULT*>(hResult);
   if ((column < 0) || (column >= result->numColumns) || (result->values[column] == nullptr))
      return nullptr;
   wcslcpy(buffer, result->values[column], bufferSize);
   return buffer;
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
   auto result = static_cast<DB2DRV_UNBUFFERED_QUERY_RESULT*>(hResult);
   return ((column >= 0) && (column < result->numColumns)) ? result->columnNames[column] : nullptr;
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
   result->connection->mutexQuery->unlock();

   for(int i = 0; i < result->numColumns; i++)
   {
      MemFree(result->columnNames[i]);
      MemFree(result->values[i]);
   }
   MemFree(result->columnNames);
   MemFree(result->values);

   MemFree(result);
}

/**
 * Begin transaction
 */
static uint32_t Begin(DBDRV_CONNECTION connection)
{
   auto c = static_cast<DB2DRV_CONN*>(connection);
   uint32_t result;
   c->mutexQuery->lock();
   SQLRETURN rc = SQLSetConnectAttr(c->sqlConn, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0);
   if (IsSuccess(rc))
   {
      result = DBERR_SUCCESS;
   }
   else
   {
      result = GetSQLErrorInfo(SQL_HANDLE_DBC, c->sqlConn, nullptr);
   }
   c->mutexQuery->unlock();
   return result;
}

/**
 * Commit transaction
 */
static uint32_t Commit(DBDRV_CONNECTION connection)
{
   auto c = static_cast<DB2DRV_CONN*>(connection);
   c->mutexQuery->lock();
   SQLRETURN rc = SQLEndTran(SQL_HANDLE_DBC, c->sqlConn, SQL_COMMIT);
   SQLSetConnectAttr(c->sqlConn, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
   c->mutexQuery->unlock();
   return IsSuccess(rc) ? DBERR_SUCCESS : DBERR_OTHER_ERROR;
}

/**
 * Rollback transaction
 */
static uint32_t Rollback(DBDRV_CONNECTION connection)
{
   auto c = static_cast<DB2DRV_CONN*>(connection);
   c->mutexQuery->lock();
   SQLRETURN rc = SQLEndTran(SQL_HANDLE_DBC, c->sqlConn, SQL_ROLLBACK);
   SQLSetConnectAttr(c->sqlConn, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
   c->mutexQuery->unlock();
   return IsSuccess(rc) ? DBERR_SUCCESS : DBERR_OTHER_ERROR;
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
   nullptr, // AddBatchRow
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
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif
