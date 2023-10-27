/* 
** MySQL Database Driver
** Copyright (C) 2003-2023 Victor Kirhenshtein
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
** File: mysql.cpp
**
**/

#include "mysqldrv.h"

#define DEBUG_TAG _T("db.drv.mysql")

/**
 * Convert wide character string to UTF-8 using internal buffer when possible
 */
inline char *WideStringToUTF8(const WCHAR *str, char *localBuffer, size_t size)
{
#ifdef UNICODE_UCS4
   size_t len = ucs4_utf8len(str, -1);
#else
   size_t len = ucs2_utf8len(str, -1);
#endif
   char *buffer = (len <= size) ? localBuffer : static_cast<char*>(MemAlloc(len));
#ifdef UNICODE_UCS4
   ucs4_to_utf8(str, -1, buffer, len);
#else
   ucs2_to_utf8(str, -1, buffer, len);
#endif
   return buffer;
}

/**
 * Free converted string if local buffer was not used
 */
inline void FreeConvertedString(char *str, char *localBuffer)
{
   if (str != localBuffer)
      MemFree(str);
}

/**
 * Update error message from given source
 */
static void UpdateErrorMessage(const char *source, WCHAR *errorText)
{
	if (errorText != nullptr)
	{
	   utf8_to_wchar(source, -1, errorText, DBDRV_MAX_ERROR_TEXT);
		errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
		RemoveTrailingCRLFW(errorText);
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
      switch(*src)
      {
         case '\'':
            out.append(_T("''"), 2);
            break;
         case '\r':
            out.append(_T("\\r"), 2);
            break;
         case '\n':
            out.append(_T("\\n"), 2);
            break;
         case '\b':
            out.append(_T("\\b"), 2);
            break;
         case '\t':
            out.append(_T("\\t"), 2);
            break;
         case 26:
            out.append(_T("\\Z"), 2);
            break;
         case '\\':
            out.append(_T("\\\\"), 2);
            break;
         default:
            out.append(*src);
            break;
      }
   }
   out.append(_T('\''));
   return out;
}

/**
 * Initialize driver
 */
static bool Initialize(const char *options)
{
	if (mysql_library_init(0, nullptr, nullptr) != 0)
	   return false;
	nxlog_debug_tag(DEBUG_TAG, 4, _T("MariaDB client library version %hs"), mysql_get_client_info());
	return true;
}

/**
 * Unload handler
 */
static void Unload()
{
	mysql_library_end();
}

/**
 * Connect to database
 */
static DBDRV_CONNECTION Connect(const char *host, const char *login, const char *password, const char *database, const char *schema, WCHAR *errorText)
{
	MYSQL *mysql = mysql_init(nullptr);
	if (mysql == nullptr)
	{
		wcscpy(errorText, L"Insufficient memory to allocate connection handle");
		return nullptr;
	}

	uint16_t port = 0;
	char hostBuffer[256];
   const char *hostName = host;
	const char *socketName = strstr(host, "socket:");
	if (socketName != nullptr)
	{
		hostName = nullptr;
		socketName += 7;
	}
	else
	{
	   // Check if port number is provided
	   const char *p = strchr(host, ':');
	   if (p != nullptr)
	   {
	      size_t l = p - host;
	      memcpy(hostBuffer, host, l);
	      hostBuffer[l] = 0;
	      hostName = hostBuffer;
	      port = static_cast<uint16_t>(strtoul(p + 1, nullptr, 10));
	   }
	}

	if (!mysql_real_connect(
         mysql,
         hostName,
         login[0] == 0 ? nullptr : login,
         (password[0] == 0 || login[0] == 0) ? nullptr : password,
         database,
         port,
         socketName,
         0 // flags
		))
	{
		UpdateErrorMessage(mysql_error(mysql), errorText);
		mysql_close(mysql);
		return nullptr;
	}

   // Switch to UTF-8 encoding
   mysql_set_character_set(mysql, "utf8mb4");

   return new MYSQL_CONN(mysql);
}

/**
 * Disconnect from database
 */
static void Disconnect(DBDRV_CONNECTION connection)
{
   mysql_close(static_cast<MYSQL_CONN*>(connection)->mysql);
   delete static_cast<MYSQL_CONN*>(connection);
}

/**
 * Prepare statement
 */
static DBDRV_STATEMENT Prepare(DBDRV_CONNECTION connection, const WCHAR *pwszQuery, bool optimizeForReuse, uint32_t *pdwError, WCHAR *errorText)
{
   MYSQL_STATEMENT *result = nullptr;

   static_cast<MYSQL_CONN*>(connection)->mutexQueryLock.lock();
   MYSQL_STMT *stmt = mysql_stmt_init(static_cast<MYSQL_CONN*>(connection)->mysql);
   if (stmt != nullptr)
   {
      char localBuffer[1024];
		char *pszQueryUTF8 = WideStringToUTF8(pwszQuery, localBuffer, 1024);
		int rc = mysql_stmt_prepare(stmt, pszQueryUTF8, (unsigned long)strlen(pszQueryUTF8));
		if (rc == 0)
		{
			result = MemAllocStruct<MYSQL_STATEMENT>();
			result->connection = static_cast<MYSQL_CONN*>(connection);
			result->statement = stmt;
			result->paramCount = (int)mysql_stmt_param_count(stmt);
			result->bindings = MemAllocArray<MYSQL_BIND>(result->paramCount);
			result->lengthFields = MemAllocArray<unsigned long>(result->paramCount);
			result->buffers = new Array(result->paramCount, 16, Ownership::True);
			*pdwError = DBERR_SUCCESS;
		}
		else
		{
			int nErr = mysql_errno(static_cast<MYSQL_CONN*>(connection)->mysql);
			if (nErr == CR_SERVER_LOST || nErr == CR_CONNECTION_ERROR || nErr == CR_SERVER_GONE_ERROR)
			{
				*pdwError = DBERR_CONNECTION_LOST;
			}
			else
			{
				*pdwError = DBERR_OTHER_ERROR;
			}
			UpdateErrorMessage(mysql_stmt_error(stmt), errorText);
			mysql_stmt_close(stmt);
		}
		FreeConvertedString(pszQueryUTF8, localBuffer);
	}
	else
	{
		*pdwError = DBERR_OTHER_ERROR;
		UpdateErrorMessage("Call to mysql_stmt_init failed", errorText);
	}
   static_cast<MYSQL_CONN*>(connection)->mutexQueryLock.unlock();
	return result;
}

/**
 * Bind parameter to prepared statement
 */
static void Bind(DBDRV_STATEMENT hStmt, int pos, int sqlType, int cType, void *buffer, int allocType)
{
	static size_t bufferSize[] = { 0, sizeof(int32_t), sizeof(uint32_t), sizeof(int64_t), sizeof(uint64_t), sizeof(double), 0 };

   auto statement = static_cast<MYSQL_STATEMENT*>(hStmt);
	if ((pos < 1) || (pos > statement->paramCount))
		return;
	MYSQL_BIND *b = &statement->bindings[pos - 1];

	if (cType == DB_CTYPE_STRING)
	{
		b->buffer = UTF8StringFromWideString((WCHAR *)buffer);
		statement->buffers->add(b->buffer);
		if (allocType == DB_BIND_DYNAMIC)
			MemFree(buffer);
		b->buffer_length = (unsigned long)strlen((char *)b->buffer) + 1;
		statement->lengthFields[pos - 1] = b->buffer_length - 1;
		b->length = &statement->lengthFields[pos - 1];
		b->buffer_type = MYSQL_TYPE_STRING;
	}
	else if (cType == DB_CTYPE_UTF8_STRING)
   {
      b->buffer = (allocType == DB_BIND_DYNAMIC) ? buffer : strdup((char *)buffer);
      statement->buffers->add(b->buffer);
      b->buffer_length = (unsigned long)strlen((char *)b->buffer) + 1;
      statement->lengthFields[pos - 1] = b->buffer_length - 1;
      b->length = &statement->lengthFields[pos - 1];
      b->buffer_type = MYSQL_TYPE_STRING;
   }
	else
	{
		switch(allocType)
		{
			case DB_BIND_STATIC:
				b->buffer = buffer;
				break;
			case DB_BIND_DYNAMIC:
				b->buffer = buffer;
				statement->buffers->add(buffer);
				break;
			case DB_BIND_TRANSIENT:
				b->buffer = MemCopyBlock(buffer, bufferSize[cType]);
				statement->buffers->add(b->buffer);
				break;
			default:
				return;	// Invalid call
		}

		switch(cType)
		{
			case DB_CTYPE_UINT32:
				b->is_unsigned = true;
				/* no break */
			case DB_CTYPE_INT32:
				b->buffer_type = MYSQL_TYPE_LONG;
				break;
			case DB_CTYPE_UINT64:
				b->is_unsigned = true;
            /* no break */
			case DB_CTYPE_INT64:
				b->buffer_type = MYSQL_TYPE_LONGLONG;
				break;
			case DB_CTYPE_DOUBLE:
				b->buffer_type = MYSQL_TYPE_DOUBLE;
				break;
		}
	}
}

/**
 * Execute prepared statement
 */
static uint32_t Execute(DBDRV_CONNECTION connection, DBDRV_STATEMENT hStmt, WCHAR *errorText)
{
   uint32_t rc;

   static_cast<MYSQL_CONN*>(connection)->mutexQueryLock.lock();
   auto statement = static_cast<MYSQL_STATEMENT*>(hStmt);
	if (mysql_stmt_bind_param(statement->statement, statement->bindings) == 0)
	{
		if (mysql_stmt_execute(statement->statement) == 0)
		{
			rc = DBERR_SUCCESS;
		}
		else
		{
			int nErr = mysql_errno(static_cast<MYSQL_CONN*>(connection)->mysql);
			if (nErr == CR_SERVER_LOST || nErr == CR_CONNECTION_ERROR || nErr == CR_SERVER_GONE_ERROR)
			{
				rc = DBERR_CONNECTION_LOST;
			}
			else
			{
				rc = DBERR_OTHER_ERROR;
			}
			UpdateErrorMessage(mysql_stmt_error(statement->statement), errorText);
		}
	}
	else
	{
		UpdateErrorMessage(mysql_stmt_error(statement->statement), errorText);
		rc = DBERR_OTHER_ERROR;
	}

   static_cast<MYSQL_CONN*>(connection)->mutexQueryLock.unlock();
   return rc;
}

/**
 * Destroy prepared statement
 */
static void FreeStatement(DBDRV_STATEMENT hStmt)
{
   auto statement = static_cast<MYSQL_STATEMENT*>(hStmt);
   statement->connection->mutexQueryLock.lock();
   mysql_stmt_close(statement->statement);
   statement->connection->mutexQueryLock.unlock();
   delete statement->buffers;
   MemFree(statement->bindings);
   MemFree(statement->lengthFields);
   MemFree(statement);
}

/**
 * Perform actual non-SELECT query
 */
static uint32_t QueryInternal(DBDRV_CONNECTION connection, const char *query, WCHAR *errorText)
{
   uint32_t rc = DBERR_INVALID_HANDLE;

   static_cast<MYSQL_CONN*>(connection)->mutexQueryLock.lock();
   if (mysql_query(static_cast<MYSQL_CONN*>(connection)->mysql, query) == 0)
   {
      rc = DBERR_SUCCESS;
      if (errorText != nullptr)
         *errorText = 0;
   }
   else
   {
      int nErr = mysql_errno(static_cast<MYSQL_CONN*>(connection)->mysql);
      if (nErr == CR_SERVER_LOST || nErr == CR_CONNECTION_ERROR || nErr == CR_SERVER_GONE_ERROR) // CR_SERVER_GONE_ERROR - ???
      {
         rc = DBERR_CONNECTION_LOST;
      }
      else
      {
         rc = DBERR_OTHER_ERROR;
      }
      UpdateErrorMessage(mysql_error(static_cast<MYSQL_CONN*>(connection)->mysql), errorText);
   }

   static_cast<MYSQL_CONN*>(connection)->mutexQueryLock.unlock();
   return rc;
}

/**
 * Perform non-SELECT query
 */
static uint32_t Query(DBDRV_CONNECTION connection, const WCHAR *query, WCHAR *errorText)
{
   char localBuffer[1024];
   char *pszQueryUTF8 = WideStringToUTF8(query, localBuffer, 1024);
   uint32_t rc = QueryInternal(connection, pszQueryUTF8, errorText);
   FreeConvertedString(pszQueryUTF8, localBuffer);
   return rc;
}

/**
 * Perform SELECT query
 */
static DBDRV_RESULT Select(DBDRV_CONNECTION connection, const WCHAR *query, uint32_t *errorCode, WCHAR *errorText)
{
   MYSQL_RESULT *result = nullptr;

   char localBuffer[1024];
   char *pszQueryUTF8 = WideStringToUTF8(query, localBuffer, 1024);
   static_cast<MYSQL_CONN*>(connection)->mutexQueryLock.lock();
   if (mysql_query(static_cast<MYSQL_CONN*>(connection)->mysql, pszQueryUTF8) == 0)
   {
      result = MemAllocStruct<MYSQL_RESULT>();
      result->connection = static_cast<MYSQL_CONN*>(connection);
      result->isPreparedStatement = false;
      result->resultSet = mysql_store_result(static_cast<MYSQL_CONN*>(connection)->mysql);
      result->numColumns = (int)mysql_num_fields(result->resultSet);
      result->numRows = (int)mysql_num_rows(result->resultSet);
      result->rows = MemAllocArray<MYSQL_ROW>(result->numRows);
      result->currentRow = -1;
      *errorCode = DBERR_SUCCESS;
      if (errorText != nullptr)
         *errorText = 0;
   }
   else
   {
      int nErr = mysql_errno(static_cast<MYSQL_CONN*>(connection)->mysql);
      if (nErr == CR_SERVER_LOST || nErr == CR_CONNECTION_ERROR || nErr == CR_SERVER_GONE_ERROR) // CR_SERVER_GONE_ERROR - ???
      {
         *errorCode = DBERR_CONNECTION_LOST;
      }
      else
      {
         *errorCode = DBERR_OTHER_ERROR;
      }
      UpdateErrorMessage(mysql_error(static_cast<MYSQL_CONN*>(connection)->mysql), errorText);
   }

   static_cast<MYSQL_CONN*>(connection)->mutexQueryLock.unlock();
   FreeConvertedString(pszQueryUTF8, localBuffer);
   return result;
}

/**
 * Perform SELECT query using prepared statement
 */
static DBDRV_RESULT SelectPrepared(DBDRV_CONNECTION connection, DBDRV_STATEMENT hStmt, uint32_t *errorCode, WCHAR *errorText)
{
   MYSQL_RESULT *result = nullptr;

   static_cast<MYSQL_CONN*>(connection)->mutexQueryLock.lock();
   auto statement = static_cast<MYSQL_STATEMENT*>(hStmt);
	if (mysql_stmt_bind_param(statement->statement, statement->bindings) == 0)
	{
		if (mysql_stmt_execute(statement->statement) == 0)
		{
			result = MemAllocStruct<MYSQL_RESULT>();
         result->connection = static_cast<MYSQL_CONN*>(connection);
			result->isPreparedStatement = true;
			result->statement = statement->statement;
			result->resultSet = mysql_stmt_result_metadata(statement->statement);
			if (result->resultSet != nullptr)
			{
				result->numColumns = mysql_num_fields(result->resultSet);
				result->lengthFields = MemAllocArray<unsigned long>(result->numColumns);

				result->bindings = MemAllocArray<MYSQL_BIND>(result->numColumns);
				for(int i = 0; i < result->numColumns; i++)
				{
					result->bindings[i].buffer_type = MYSQL_TYPE_STRING;
					result->bindings[i].length = &result->lengthFields[i];
				}

				mysql_stmt_bind_result(statement->statement, result->bindings);

				if (mysql_stmt_store_result(statement->statement) == 0)
				{
					result->numRows = (int)mysql_stmt_num_rows(statement->statement);
					result->currentRow = -1;
         		*errorCode = DBERR_SUCCESS;
				}
				else
				{
					UpdateErrorMessage(mysql_stmt_error(statement->statement), errorText);
					*errorCode = DBERR_OTHER_ERROR;
					mysql_free_result(result->resultSet);
					MemFree(result->bindings);
					MemFree(result->lengthFields);
					MemFreeAndNull(result);
				}
			}
			else
			{
				UpdateErrorMessage(mysql_stmt_error(statement->statement), errorText);
				*errorCode = DBERR_OTHER_ERROR;
				MemFreeAndNull(result);
			}
		}
		else
		{
			int nErr = mysql_errno(static_cast<MYSQL_CONN*>(connection)->mysql);
			if (nErr == CR_SERVER_LOST || nErr == CR_CONNECTION_ERROR || nErr == CR_SERVER_GONE_ERROR)
			{
				*errorCode = DBERR_CONNECTION_LOST;
			}
			else
			{
				*errorCode = DBERR_OTHER_ERROR;
			}
			UpdateErrorMessage(mysql_stmt_error(statement->statement), errorText);
		}
	}
	else
	{
		UpdateErrorMessage(mysql_stmt_error(statement->statement), errorText);
		*errorCode = DBERR_OTHER_ERROR;
	}

   static_cast<MYSQL_CONN*>(connection)->mutexQueryLock.unlock();
	return result;
}

/**
 * Get field length from result
 */
static int32_t GetFieldLength(DBDRV_RESULT hResult, int iRow, int iColumn)
{
   auto result = static_cast<MYSQL_RESULT*>(hResult);
   if ((iRow < 0) || (iRow >= result->numRows) ||
       (iColumn < 0) || (iColumn >= result->numColumns))
      return -1;

	if (result->isPreparedStatement)
	{
		if (result->currentRow != iRow)
		{
         result->connection->mutexQueryLock.lock();
         if (iRow != result->currentRow + 1)
            mysql_stmt_data_seek(result->statement, iRow);
			mysql_stmt_fetch(result->statement);
			result->currentRow = iRow;
	      result->connection->mutexQueryLock.unlock();
		}
		return (int32_t)result->lengthFields[iColumn];
	}
	else
	{
      MYSQL_ROW row;
      if (result->currentRow != iRow)
      {
         if (result->rows[iRow] == nullptr)
         {
            if (iRow != result->currentRow + 1)
               mysql_data_seek(result->resultSet, iRow);
            row = mysql_fetch_row(result->resultSet);
            result->rows[iRow] = row;
         }
         else
         {
            row = result->rows[iRow];
         }
         result->currentRow = iRow;
      }
      else
      {
         row = result->rows[iRow];
      }
      return (row == nullptr) ? (LONG)-1 : ((row[iColumn] == nullptr) ? -1 : (int32_t)strlen(row[iColumn]));
	}
}

/**
 * Get field value from result - UNICODE and UTF8 implementation
 */
static void *GetFieldInternal(MYSQL_RESULT *hResult, int iRow, int iColumn, void *pBuffer, int nBufSize, bool utf8)
{
   if ((iRow < 0) || (iRow >= hResult->numRows) ||
       (iColumn < 0) || (iColumn >= hResult->numColumns))
      return nullptr;

	void *value = nullptr;
	if (hResult->isPreparedStatement)
	{
      hResult->connection->mutexQueryLock.lock();
		if (hResult->currentRow != iRow)
		{
         if (iRow != hResult->currentRow + 1)
            mysql_stmt_data_seek(hResult->statement, iRow);
			mysql_stmt_fetch(hResult->statement);
			hResult->currentRow = iRow;
		}

		MYSQL_BIND b;
		unsigned long l = 0;
#if HAVE_MY_BOOL
		my_bool isNull;
#else
		bool isNull;
#endif

		memset(&b, 0, sizeof(MYSQL_BIND));
		b.buffer = MemAllocLocal(hResult->lengthFields[iColumn] + 1);
		b.buffer_length = hResult->lengthFields[iColumn] + 1;
		b.buffer_type = MYSQL_TYPE_STRING;
		b.length = &l;
		b.is_null = &isNull;
      int rc = mysql_stmt_fetch_column(hResult->statement, &b, iColumn, 0);
      if (rc == 0)
		{
         if (!isNull)
         {
			   ((char *)b.buffer)[l] = 0;
            if (utf8)
            {
			      strlcpy((char *)pBuffer, (char *)b.buffer, nBufSize);
            }
            else
            {
			      utf8_to_wchar((char *)b.buffer, -1, (WCHAR *)pBuffer, nBufSize);
   			   ((WCHAR *)pBuffer)[nBufSize - 1] = 0;
            }
         }
         else
         {
            if (utf8)
               *((char *)pBuffer) = 0;
            else
               *((WCHAR *)pBuffer) = 0;
         }
			value = pBuffer;
		}
      hResult->connection->mutexQueryLock.unlock();
		MemFreeLocal(b.buffer);
	}
	else
	{
      MYSQL_ROW row;
      if (hResult->currentRow != iRow)
      {
         if (hResult->rows[iRow] == nullptr)
         {
            if (iRow != hResult->currentRow + 1)
               mysql_data_seek(hResult->resultSet, iRow);
            row = mysql_fetch_row(hResult->resultSet);
            hResult->rows[iRow] = row;
         }
         else
         {
            row = hResult->rows[iRow];
         }
         hResult->currentRow = iRow;
      }
      else
      {
         row = hResult->rows[iRow];
      }
      if (row != nullptr)
		{
			if (row[iColumn] != nullptr)
			{
            if (utf8)
            {
   				strlcpy((char *)pBuffer, row[iColumn], nBufSize);
            }
            else
            {
   				utf8_to_wchar(row[iColumn], -1, (WCHAR *)pBuffer, nBufSize);
   			   ((WCHAR *)pBuffer)[nBufSize - 1] = 0;
            }
				value = pBuffer;
			}
		}
	}
	return value;
}

/**
 * Get field value from result
 */
static WCHAR *GetField(DBDRV_RESULT hResult, int iRow, int iColumn, WCHAR *pBuffer, int nBufSize)
{
   return (WCHAR *)GetFieldInternal(static_cast<MYSQL_RESULT*>(hResult), iRow, iColumn, pBuffer, nBufSize, false);
}

/**
 * Get field value from result as UTF8 string
 */
static char *GetFieldUTF8(DBDRV_RESULT hResult, int iRow, int iColumn, char *pBuffer, int nBufSize)
{
   return (char *)GetFieldInternal(static_cast<MYSQL_RESULT*>(hResult), iRow, iColumn, pBuffer, nBufSize, true);
}

/**
 * Get number of rows in result
 */
static int GetNumRows(DBDRV_RESULT hResult)
{
   return static_cast<MYSQL_RESULT*>(hResult)->numRows;
}

/**
 * Get column count in query result
 */
static int GetColumnCount(DBDRV_RESULT hResult)
{
   return static_cast<MYSQL_RESULT*>(hResult)->numColumns;
}

/**
 * Get column name in query result
 */
static const char *GetColumnName(DBDRV_RESULT hResult, int column)
{
	MYSQL_FIELD *field = mysql_fetch_field_direct(static_cast<MYSQL_RESULT*>(hResult)->resultSet, column);
	return (field != nullptr) ? field->name : nullptr;
}

/**
 * Free SELECT results
 */
static void FreeResult(DBDRV_RESULT hResult)
{
   auto result = static_cast<MYSQL_RESULT*>(hResult);
   if (result->isPreparedStatement)
   {
      MemFree(result->bindings);
      MemFree(result->lengthFields);
   }

   mysql_free_result(result->resultSet);
   MemFree(result->rows);
   MemFree(result);
}

/**
 * Perform unbuffered SELECT query
 */
static DBDRV_UNBUFFERED_RESULT SelectUnbuffered(DBDRV_CONNECTION connection, const WCHAR *query, uint32_t *errorCode, WCHAR *errorText)
{
   MYSQL_UNBUFFERED_RESULT *pResult = nullptr;

   char localBuffer[1024];
   char *pszQueryUTF8 = WideStringToUTF8(query, localBuffer, 1024);
   static_cast<MYSQL_CONN*>(connection)->mutexQueryLock.lock();
	if (mysql_query(static_cast<MYSQL_CONN*>(connection)->mysql, pszQueryUTF8) == 0)
	{
		pResult = MemAllocStruct<MYSQL_UNBUFFERED_RESULT>();
		pResult->connection = static_cast<MYSQL_CONN*>(connection);
		pResult->isPreparedStatement = false;
		pResult->resultSet = mysql_use_result(static_cast<MYSQL_CONN*>(connection)->mysql);
		if (pResult->resultSet != nullptr)
		{
			pResult->numColumns = mysql_num_fields(pResult->resultSet);
			pResult->lengthFields = MemAllocArray<unsigned long>(pResult->numColumns);
		}
		else
		{
			MemFreeAndNull(pResult);
		}

		*errorCode = DBERR_SUCCESS;
		if (errorText != nullptr)
			*errorText = 0;
	}
	else
	{
		int nErr = mysql_errno(static_cast<MYSQL_CONN*>(connection)->mysql);
		if (nErr == CR_SERVER_LOST || nErr == CR_CONNECTION_ERROR || nErr == CR_SERVER_GONE_ERROR) // CR_SERVER_GONE_ERROR - ???
		{
			*errorCode = DBERR_CONNECTION_LOST;
		}
		else
		{
			*errorCode = DBERR_OTHER_ERROR;
		}
		
		if (errorText != nullptr)
		{
		   utf8_to_wchar(mysql_error(static_cast<MYSQL_CONN*>(connection)->mysql), -1, errorText, DBDRV_MAX_ERROR_TEXT);
			errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
			RemoveTrailingCRLFW(errorText);
		}
	}

	if (pResult == nullptr)
	{
      static_cast<MYSQL_CONN*>(connection)->mutexQueryLock.unlock();
	}
	FreeConvertedString(pszQueryUTF8, localBuffer);

	return pResult;
}

/**
 * Perform unbuffered SELECT query using prepared statement
 */
static DBDRV_RESULT SelectPreparedUnbuffered(DBDRV_CONNECTION connection, DBDRV_STATEMENT hStmt, uint32_t *errorCode, WCHAR *errorText)
{
   MYSQL_UNBUFFERED_RESULT *result = nullptr;

   static_cast<MYSQL_CONN*>(connection)->mutexQueryLock.lock();
   auto statement = static_cast<MYSQL_STATEMENT*>(hStmt);
   if (mysql_stmt_bind_param(statement->statement, statement->bindings) == 0)
   {
      if (mysql_stmt_execute(statement->statement) == 0)
      {
         result = MemAllocStruct<MYSQL_UNBUFFERED_RESULT>();
         result->connection = static_cast<MYSQL_CONN*>(connection);
         result->isPreparedStatement = true;
         result->statement = statement->statement;
         result->resultSet = mysql_stmt_result_metadata(statement->statement);
         if (result->resultSet != nullptr)
         {
            result->numColumns = mysql_num_fields(result->resultSet);
            result->lengthFields = MemAllocArray<unsigned long>(result->numColumns);

            result->bindings = MemAllocArray<MYSQL_BIND>(result->numColumns);
            for(int i = 0; i < result->numColumns; i++)
            {
               result->bindings[i].buffer_type = MYSQL_TYPE_STRING;
               result->bindings[i].length = &result->lengthFields[i];
            }

            mysql_stmt_bind_result(statement->statement, result->bindings);
            *errorCode = DBERR_SUCCESS;
         }
         else
         {
            UpdateErrorMessage(mysql_stmt_error(statement->statement), errorText);
            *errorCode = DBERR_OTHER_ERROR;
            MemFreeAndNull(result);
         }
      }
      else
      {
         int nErr = mysql_errno(static_cast<MYSQL_CONN*>(connection)->mysql);
         if (nErr == CR_SERVER_LOST || nErr == CR_CONNECTION_ERROR || nErr == CR_SERVER_GONE_ERROR)
         {
            *errorCode = DBERR_CONNECTION_LOST;
         }
         else
         {
            *errorCode = DBERR_OTHER_ERROR;
         }
         UpdateErrorMessage(mysql_stmt_error(statement->statement), errorText);
      }
   }
   else
   {
      UpdateErrorMessage(mysql_stmt_error(statement->statement), errorText);
      *errorCode = DBERR_OTHER_ERROR;
   }

   if (result == nullptr)
   {
      static_cast<MYSQL_CONN*>(connection)->mutexQueryLock.unlock();
   }
   return result;
}

/**
 * Fetch next result line from asynchronous SELECT results
 */
static bool Fetch(DBDRV_UNBUFFERED_RESULT hResult)
{
   auto result = static_cast<MYSQL_UNBUFFERED_RESULT*>(hResult);

   if (result->noMoreRows)
      return false;

	bool success = true;
	
	if (result->isPreparedStatement)
	{
	   int rc = mysql_stmt_fetch(result->statement);
      if ((rc != 0) && (rc != MYSQL_DATA_TRUNCATED))
      {
         result->noMoreRows = true;
         success = false;
         result->connection->mutexQueryLock.unlock();
      }
	}
	else
	{
      // Try to fetch next row from server
      result->pCurrRow = mysql_fetch_row(result->resultSet);
      if (result->pCurrRow == NULL)
      {
         result->noMoreRows = true;
         success = false;
         result->connection->mutexQueryLock.unlock();
      }
      else
      {
         // Get column lengths for current row
         unsigned long *pLen = mysql_fetch_lengths(result->resultSet);
         if (pLen != nullptr)
         {
            memcpy(result->lengthFields, pLen, sizeof(unsigned long) * result->numColumns);
         }
         else
         {
            memset(result->lengthFields, 0, sizeof(unsigned long) * result->numColumns);
         }
      }
	}
	return success;
}

/**
 * Get field length from async query result result
 */
static int32_t GetFieldLengthUnbuffered(DBDRV_UNBUFFERED_RESULT hResult, int column)
{
   auto result = static_cast<MYSQL_UNBUFFERED_RESULT*>(hResult);

   // Check if there are valid fetched row
	if (result->noMoreRows || ((result->pCurrRow == nullptr) && !result->isPreparedStatement))
		return 0;
	
	// Check if column number is valid
	if ((column < 0) || (column >= result->numColumns))
		return 0;

	return result->lengthFields[column];
}

/**
 * Get field from current row in async query result - common implementation for wide char and UTF-8
 */
static void *GetFieldUnbufferedInternal(MYSQL_UNBUFFERED_RESULT *hResult, int iColumn, void *pBuffer, int iBufSize, bool utf8)
{
   // Check if there are valid fetched row
   if ((hResult->noMoreRows) || ((hResult->pCurrRow == nullptr) && !hResult->isPreparedStatement))
      return nullptr;

   // Check if column number is valid
   if ((iColumn < 0) || (iColumn >= hResult->numColumns))
      return nullptr;

   // Now get column data
   void *value = nullptr;
   if (hResult->isPreparedStatement)
   {
      MYSQL_BIND b;
      unsigned long l = 0;
#if HAVE_MY_BOOL
      my_bool isNull;
#else
      bool isNull;
#endif

      memset(&b, 0, sizeof(MYSQL_BIND));
      b.buffer = MemAllocLocal(hResult->lengthFields[iColumn] + 1);
      b.buffer_length = hResult->lengthFields[iColumn] + 1;
      b.buffer_type = MYSQL_TYPE_STRING;
      b.length = &l;
      b.is_null = &isNull;
      int rc = mysql_stmt_fetch_column(hResult->statement, &b, iColumn, 0);
      if (rc == 0)
      {
         if (!isNull)
         {
            ((char *)b.buffer)[l] = 0;
            if (utf8)
            {
               strlcpy((char *)pBuffer, (char *)b.buffer, iBufSize);
            }
            else
            {
               utf8_to_wchar((char *)b.buffer, -1, (WCHAR *)pBuffer, iBufSize);
               ((WCHAR *)pBuffer)[iBufSize - 1] = 0;
            }
         }
         else
         {
            if (utf8)
               *((char *)pBuffer) = 0;
            else
               *((WCHAR *)pBuffer) = 0;
         }
         value = pBuffer;
      }
      MemFreeLocal(b.buffer);
   }
   else
   {
      if (hResult->lengthFields[iColumn] > 0)
      {
         if (utf8)
         {
            int l = std::min((int)hResult->lengthFields[iColumn], iBufSize - 1);
            memcpy(pBuffer, hResult->pCurrRow[iColumn], l);
            ((char *)pBuffer)[l] = 0;
         }
         else
         {
            size_t l = utf8_to_wchar(hResult->pCurrRow[iColumn], hResult->lengthFields[iColumn], (WCHAR *)pBuffer, iBufSize);
            ((WCHAR *)pBuffer)[l] = 0;
         }
      }
      else
      {
         if (utf8)
            *((char *)pBuffer) = 0;
         else
            *((WCHAR *)pBuffer) = 0;
      }
      value = pBuffer;
   }
   return value;
}

/**
 * Get field from current row in async query result
 */
static WCHAR *GetFieldUnbuffered(DBDRV_UNBUFFERED_RESULT hResult, int iColumn, WCHAR *pBuffer, int iBufSize)
{
	return (WCHAR *)GetFieldUnbufferedInternal(static_cast<MYSQL_UNBUFFERED_RESULT*>(hResult), iColumn, pBuffer, iBufSize, false);
}

/**
 * Get field from current row in unbuffered query result
 */
static char *GetFieldUnbufferedUTF8(DBDRV_UNBUFFERED_RESULT hResult, int iColumn, char *pBuffer, int iBufSize)
{
   return (char *)GetFieldUnbufferedInternal(static_cast<MYSQL_UNBUFFERED_RESULT*>(hResult), iColumn, pBuffer, iBufSize, true);
}

/**
 * Get column count in unbuffered query result
 */
static int GetColumnCountUnbuffered(DBDRV_UNBUFFERED_RESULT hResult)
{
	return static_cast<MYSQL_UNBUFFERED_RESULT*>(hResult)->numColumns;
}

/**
 * Get column name in unbuffered query result
 */
static const char *GetColumnNameUnbuffered(DBDRV_UNBUFFERED_RESULT hResult, int column)
{
   auto result = static_cast<MYSQL_UNBUFFERED_RESULT*>(hResult);
   if (result->resultSet == nullptr)
      return nullptr;

   MYSQL_FIELD *field = mysql_fetch_field_direct(result->resultSet, column);
   return (field != nullptr) ? field->name : nullptr;
}

/**
 * Destroy result of unbuffered query
 */
static void FreeUnbufferedResult(DBDRV_UNBUFFERED_RESULT hResult)
{
   auto result = static_cast<MYSQL_UNBUFFERED_RESULT*>(hResult);

   // Check if all result rows fetched
   if (!result->noMoreRows)
   {
      // Fetch remaining rows
      if (!result->isPreparedStatement)
      {
         while(mysql_fetch_row(result->resultSet) != nullptr);
      }

      // Now we are ready for next query, so unlock query mutex
      result->connection->mutexQueryLock.unlock();
   }

   // Free allocated memory
   mysql_free_result(result->resultSet);
   MemFree(result->lengthFields);
   MemFree(result->bindings);
   MemFree(result);
}

/**
 * Begin transaction
 */
static uint32_t Begin(DBDRV_CONNECTION connection)
{
   return QueryInternal(connection, "BEGIN", nullptr);
}

/**
 * Commit transaction
 */
static uint32_t Commit(DBDRV_CONNECTION connection)
{
   return QueryInternal(connection, "COMMIT", nullptr);
}

/**
 * Rollback transaction
 */
static uint32_t Rollback(DBDRV_CONNECTION connection)
{
   return QueryInternal(connection, "ROLLBACK", nullptr);
}

/**
 * Check if table exist
 */
static int IsTableExist(DBDRV_CONNECTION connection, const WCHAR *name)
{
   if (connection == nullptr)
      return DBIsTableExist_Failure;

   WCHAR query[256], lname[256];
   wcsncpy(lname, name, 256);
   wcslwr(lname);
   swprintf(query, 256, L"SHOW TABLES LIKE '%ls'", lname);
   uint32_t error;
   WCHAR errorText[DBDRV_MAX_ERROR_TEXT];
   int rc = DBIsTableExist_Failure;
   DBDRV_RESULT hResult = Select(connection, query, &error, errorText);
   if (hResult != nullptr)
   {
      rc = (GetNumRows(hResult) > 0) ? DBIsTableExist_Found : DBIsTableExist_NotFound;
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
   GetFieldUTF8,
   GetFieldUnbuffered,
   GetFieldUnbufferedUTF8,
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

DB_DRIVER_ENTRY_POINT("MYSQL", s_callTable)

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
