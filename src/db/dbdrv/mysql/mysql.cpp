/* 
** MySQL Database Driver
** Copyright (C) 2003-2021 Victor Kirhenshtein
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

DECLARE_DRIVER_HEADER("MYSQL")

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
 * Update buffer length in DrvPrepareStringW
 */
#define UPDATE_LENGTH \
				len++; \
				if (len >= bufferSize - 1) \
				{ \
					bufferSize += 128; \
					out = (WCHAR *)realloc(out, bufferSize * sizeof(WCHAR)); \
				}

/**
 * Prepare string for using in SQL query - enclose in quotes and escape as needed
 * (wide string version)
 */
extern "C" WCHAR __EXPORT *DrvPrepareStringW(const WCHAR *str)
{
	int len = (int)wcslen(str) + 3;   // + two quotes and \0 at the end
	int bufferSize = len + 128;
	WCHAR *out = (WCHAR *)MemAlloc(bufferSize * sizeof(WCHAR));
	out[0] = _T('\'');

	const WCHAR *src = str;
	int outPos;
	for(outPos = 1; *src != 0; src++)
	{
		switch(*src)
		{
			case L'\'':
				out[outPos++] = L'\'';
				out[outPos++] = L'\'';
				UPDATE_LENGTH;
				break;
			case L'\r':
				out[outPos++] = L'\\';
				out[outPos++] = L'\r';
				UPDATE_LENGTH;
				break;
			case L'\n':
				out[outPos++] = L'\\';
				out[outPos++] = L'\n';
				UPDATE_LENGTH;
				break;
			case L'\b':
				out[outPos++] = L'\\';
				out[outPos++] = L'\b';
				UPDATE_LENGTH;
				break;
			case L'\t':
				out[outPos++] = L'\\';
				out[outPos++] = L'\t';
				UPDATE_LENGTH;
				break;
			case 26:
				out[outPos++] = L'\\';
				out[outPos++] = L'Z';
				break;
			case L'\\':
				out[outPos++] = L'\\';
				out[outPos++] = L'\\';
				UPDATE_LENGTH;
				break;
			default:
				out[outPos++] = *src;
				break;
		}
	}
	out[outPos++] = L'\'';
	out[outPos++] = 0;

	return out;
}

#undef UPDATE_LENGTH

/**
 * Update buffer length in DrvPrepareStringA
 */
#define UPDATE_LENGTH \
				len++; \
				if (len >= bufferSize - 1) \
				{ \
					bufferSize += 128; \
					out = (char *)realloc(out, bufferSize); \
				}

/**
 * Prepare string for using in SQL query - enclose in quotes and escape as needed
 * (multibyte string version)
 */
extern "C" char __EXPORT *DrvPrepareStringA(const char *str)
{
	int len = (int)strlen(str) + 3;   // + two quotes and \0 at the end
	int bufferSize = len + 128;
	char *out = (char *)MemAlloc(bufferSize);
	out[0] = _T('\'');

	const char *src = str;
	int outPos;
	for(outPos = 1; *src != 0; src++)
	{
		switch(*src)
		{
			case '\'':
				out[outPos++] = '\'';
				out[outPos++] = '\'';
				UPDATE_LENGTH;
				break;
			case '\r':
				out[outPos++] = '\\';
				out[outPos++] = '\r';
				UPDATE_LENGTH;
				break;
			case '\n':
				out[outPos++] = '\\';
				out[outPos++] = '\n';
				UPDATE_LENGTH;
				break;
			case '\b':
				out[outPos++] = '\\';
				out[outPos++] = '\b';
				UPDATE_LENGTH;
				break;
			case '\t':
				out[outPos++] = '\\';
				out[outPos++] = '\t';
				UPDATE_LENGTH;
				break;
			case 26:
				out[outPos++] = '\\';
				out[outPos++] = 'Z';
				break;
			case '\\':
				out[outPos++] = '\\';
				out[outPos++] = '\\';
				UPDATE_LENGTH;
				break;
			default:
				out[outPos++] = *src;
				break;
		}
	}
	out[outPos++] = '\'';
	out[outPos++] = 0;

	return out;
}

#undef UPDATE_LENGTH

/**
 * Initialize driver
 */
extern "C" bool __EXPORT DrvInit(const char *cmdLine)
{
	if (mysql_library_init(0, nullptr, nullptr) != 0)
	   return false;
	nxlog_debug_tag(DEBUG_TAG, 4, _T("MariaDB client library version %hs"), mysql_get_client_info());
	return true;
}

/**
 * Unload handler
 */
extern "C" void __EXPORT DrvUnload()
{
	mysql_library_end();
}

/**
 * Connect to database
 */
extern "C" DBDRV_CONNECTION __EXPORT DrvConnect(const char *host, const char *login, const char *password,
      const char *database, const char *schema, WCHAR *errorText)
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
	      strncpy(hostBuffer, host, l);
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
   mysql_set_character_set(mysql, "utf8");

	return new MYSQL_CONN(mysql);
}

/**
 * Disconnect from database
 */
extern "C" void __EXPORT DrvDisconnect(MYSQL_CONN *connection)
{
   mysql_close(connection->mysql);
   delete connection;
}

/**
 * Prepare statement
 */
extern "C" DBDRV_STATEMENT __EXPORT DrvPrepare(MYSQL_CONN *connection, WCHAR *pwszQuery, bool optimizeForReuse, DWORD *pdwError, WCHAR *errorText)
{
	MYSQL_STATEMENT *result = NULL;

	connection->mutexQueryLock.lock();
	MYSQL_STMT *stmt = mysql_stmt_init(connection->mysql);
	if (stmt != NULL)
	{
	   char localBuffer[1024];
		char *pszQueryUTF8 = WideStringToUTF8(pwszQuery, localBuffer, 1024);
		int rc = mysql_stmt_prepare(stmt, pszQueryUTF8, (unsigned long)strlen(pszQueryUTF8));
		if (rc == 0)
		{
			result = MemAllocStruct<MYSQL_STATEMENT>();
			result->connection = connection;
			result->statement = stmt;
			result->paramCount = (int)mysql_stmt_param_count(stmt);
			result->bindings = MemAllocArray<MYSQL_BIND>(result->paramCount);
			result->lengthFields = MemAllocArray<unsigned long>(result->paramCount);
			result->buffers = new Array(result->paramCount, 16, Ownership::True);
			*pdwError = DBERR_SUCCESS;
		}
		else
		{
			int nErr = mysql_errno(connection->mysql);
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
	connection->mutexQueryLock.unlock();
	return result;
}

/**
 * Bind parameter to prepared statement
 */
extern "C" void __EXPORT DrvBind(MYSQL_STATEMENT *hStmt, int pos, int sqlType, int cType, void *buffer, int allocType)
{
	static size_t bufferSize[] = { 0, sizeof(INT32), sizeof(UINT32), sizeof(INT64), sizeof(UINT64), sizeof(double), 0 };

	if ((pos < 1) || (pos > hStmt->paramCount))
		return;
	MYSQL_BIND *b = &hStmt->bindings[pos - 1];

	if (cType == DB_CTYPE_STRING)
	{
		b->buffer = UTF8StringFromWideString((WCHAR *)buffer);
		hStmt->buffers->add(b->buffer);
		if (allocType == DB_BIND_DYNAMIC)
			MemFree(buffer);
		b->buffer_length = (unsigned long)strlen((char *)b->buffer) + 1;
		hStmt->lengthFields[pos - 1] = b->buffer_length - 1;
		b->length = &hStmt->lengthFields[pos - 1];
		b->buffer_type = MYSQL_TYPE_STRING;
	}
	else if (cType == DB_CTYPE_UTF8_STRING)
   {
      b->buffer = (allocType == DB_BIND_DYNAMIC) ? buffer : strdup((char *)buffer);
      hStmt->buffers->add(b->buffer);
      b->buffer_length = (unsigned long)strlen((char *)b->buffer) + 1;
      hStmt->lengthFields[pos - 1] = b->buffer_length - 1;
      b->length = &hStmt->lengthFields[pos - 1];
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
				hStmt->buffers->add(buffer);
				break;
			case DB_BIND_TRANSIENT:
				b->buffer = MemCopyBlock(buffer, bufferSize[cType]);
				hStmt->buffers->add(b->buffer);
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
extern "C" DWORD __EXPORT DrvExecute(MYSQL_CONN *connection, MYSQL_STATEMENT *hStmt, WCHAR *errorText)
{
	DWORD dwResult;

	connection->mutexQueryLock.lock();

	if (mysql_stmt_bind_param(hStmt->statement, hStmt->bindings) == 0)
	{
		if (mysql_stmt_execute(hStmt->statement) == 0)
		{
			dwResult = DBERR_SUCCESS;
		}
		else
		{
			int nErr = mysql_errno(connection->mysql);
			if (nErr == CR_SERVER_LOST || nErr == CR_CONNECTION_ERROR || nErr == CR_SERVER_GONE_ERROR)
			{
				dwResult = DBERR_CONNECTION_LOST;
			}
			else
			{
				dwResult = DBERR_OTHER_ERROR;
			}
			UpdateErrorMessage(mysql_stmt_error(hStmt->statement), errorText);
		}
	}
	else
	{
		UpdateErrorMessage(mysql_stmt_error(hStmt->statement), errorText);
		dwResult = DBERR_OTHER_ERROR;
	}

	connection->mutexQueryLock.unlock();
	return dwResult;
}

/**
 * Destroy prepared statement
 */
extern "C" void __EXPORT DrvFreeStatement(MYSQL_STATEMENT *hStmt)
{
	if (hStmt == NULL)
		return;

	hStmt->connection->mutexQueryLock.lock();
	mysql_stmt_close(hStmt->statement);
	hStmt->connection->mutexQueryLock.unlock();
	delete hStmt->buffers;
	MemFree(hStmt->bindings);
	MemFree(hStmt->lengthFields);
	MemFree(hStmt);
}

/**
 * Perform actual non-SELECT query
 */
static DWORD DrvQueryInternal(MYSQL_CONN *connection, const char *pszQuery, WCHAR *errorText)
{
	DWORD dwRet = DBERR_INVALID_HANDLE;

	connection->mutexQueryLock.lock();
	if (mysql_query(connection->mysql, pszQuery) == 0)
	{
		dwRet = DBERR_SUCCESS;
		if (errorText != NULL)
			*errorText = 0;
	}
	else
	{
		int nErr = mysql_errno(connection->mysql);
		if (nErr == CR_SERVER_LOST || nErr == CR_CONNECTION_ERROR || nErr == CR_SERVER_GONE_ERROR) // CR_SERVER_GONE_ERROR - ???
		{
			dwRet = DBERR_CONNECTION_LOST;
		}
		else
		{
			dwRet = DBERR_OTHER_ERROR;
		}
		UpdateErrorMessage(mysql_error(connection->mysql), errorText);
	}

	connection->mutexQueryLock.unlock();
	return dwRet;
}

/**
 * Perform non-SELECT query
 */
extern "C" DWORD __EXPORT DrvQuery(MYSQL_CONN *connection, WCHAR *pwszQuery, WCHAR *errorText)
{
   char localBuffer[1024];
   char *pszQueryUTF8 = WideStringToUTF8(pwszQuery, localBuffer, 1024);
   DWORD rc = DrvQueryInternal(connection, pszQueryUTF8, errorText);
   FreeConvertedString(pszQueryUTF8, localBuffer);
	return rc;
}

/**
 * Perform SELECT query - actual implementation
 */
static MYSQL_RESULT *DrvSelectInternal(MYSQL_CONN *connection, WCHAR *pwszQuery, DWORD *pdwError, WCHAR *errorText)
{
   MYSQL_RESULT *result = nullptr;

   char localBuffer[1024];
   char *pszQueryUTF8 = WideStringToUTF8(pwszQuery, localBuffer, 1024);
	connection->mutexQueryLock.lock();
	if (mysql_query(connection->mysql, pszQueryUTF8) == 0)
	{
		result = MemAllocStruct<MYSQL_RESULT>();
      result->connection = connection;
		result->isPreparedStatement = false;
		result->resultSet = mysql_store_result(connection->mysql);
		result->numColumns = (int)mysql_num_fields(result->resultSet);
      result->numRows = (int)mysql_num_rows(result->resultSet);
      result->rows = MemAllocArray<MYSQL_ROW>(result->numRows);
      result->currentRow = -1;
		*pdwError = DBERR_SUCCESS;
		if (errorText != nullptr)
			*errorText = 0;
	}
	else
	{
		int nErr = mysql_errno(connection->mysql);
		if (nErr == CR_SERVER_LOST || nErr == CR_CONNECTION_ERROR || nErr == CR_SERVER_GONE_ERROR) // CR_SERVER_GONE_ERROR - ???
		{
			*pdwError = DBERR_CONNECTION_LOST;
		}
		else
		{
			*pdwError = DBERR_OTHER_ERROR;
		}
		UpdateErrorMessage(mysql_error(connection->mysql), errorText);
	}

	connection->mutexQueryLock.unlock();
	FreeConvertedString(pszQueryUTF8, localBuffer);
	return result;
}

/**
 * Perform SELECT query - public entry point
 */
extern "C" DBDRV_RESULT __EXPORT DrvSelect(MYSQL_CONN *connection, WCHAR *query, DWORD *errorCode, WCHAR *errorText)
{
   if (connection == nullptr)
   {
      *errorCode = DBERR_INVALID_HANDLE;
      return nullptr;
   }
   return DrvSelectInternal(connection, query, errorCode, errorText);
}

/**
 * Perform SELECT query using prepared statement
 */
extern "C" DBDRV_RESULT __EXPORT DrvSelectPrepared(MYSQL_CONN *connection, MYSQL_STATEMENT *hStmt, DWORD *pdwError, WCHAR *errorText)
{
	if (connection == nullptr)
	{
		*pdwError = DBERR_INVALID_HANDLE;
		return nullptr;
	}

	MYSQL_RESULT *result = nullptr;

	connection->mutexQueryLock.lock();

	if (mysql_stmt_bind_param(hStmt->statement, hStmt->bindings) == 0)
	{
		if (mysql_stmt_execute(hStmt->statement) == 0)
		{
			result = MemAllocStruct<MYSQL_RESULT>();
         result->connection = connection;
			result->isPreparedStatement = true;
			result->statement = hStmt->statement;
			result->resultSet = mysql_stmt_result_metadata(hStmt->statement);
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

				mysql_stmt_bind_result(hStmt->statement, result->bindings);

				if (mysql_stmt_store_result(hStmt->statement) == 0)
				{
					result->numRows = (int)mysql_stmt_num_rows(hStmt->statement);
					result->currentRow = -1;
         		*pdwError = DBERR_SUCCESS;
				}
				else
				{
					UpdateErrorMessage(mysql_stmt_error(hStmt->statement), errorText);
					*pdwError = DBERR_OTHER_ERROR;
					mysql_free_result(result->resultSet);
					MemFree(result->bindings);
					MemFree(result->lengthFields);
					MemFreeAndNull(result);
				}
			}
			else
			{
				UpdateErrorMessage(mysql_stmt_error(hStmt->statement), errorText);
				*pdwError = DBERR_OTHER_ERROR;
				MemFreeAndNull(result);
			}
		}
		else
		{
			int nErr = mysql_errno(connection->mysql);
			if (nErr == CR_SERVER_LOST || nErr == CR_CONNECTION_ERROR || nErr == CR_SERVER_GONE_ERROR)
			{
				*pdwError = DBERR_CONNECTION_LOST;
			}
			else
			{
				*pdwError = DBERR_OTHER_ERROR;
			}
			UpdateErrorMessage(mysql_stmt_error(hStmt->statement), errorText);
		}
	}
	else
	{
		UpdateErrorMessage(mysql_stmt_error(hStmt->statement), errorText);
		*pdwError = DBERR_OTHER_ERROR;
	}

	connection->mutexQueryLock.unlock();
	return result;
}

/**
 * Get field length from result
 */
extern "C" LONG __EXPORT DrvGetFieldLength(MYSQL_RESULT *hResult, int iRow, int iColumn)
{
   if ((iRow < 0) || (iRow >= hResult->numRows) ||
       (iColumn < 0) || (iColumn >= hResult->numColumns))
      return -1;

	if (hResult->isPreparedStatement)
	{
		if (hResult->currentRow != iRow)
		{
         hResult->connection->mutexQueryLock.lock();
         if (iRow != hResult->currentRow + 1)
            mysql_stmt_data_seek(hResult->statement, iRow);
			mysql_stmt_fetch(hResult->statement);
			hResult->currentRow = iRow;
	      hResult->connection->mutexQueryLock.unlock();
		}
		return (LONG)hResult->lengthFields[iColumn];
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
      return (row == nullptr) ? (LONG)-1 : ((row[iColumn] == nullptr) ? -1 : (LONG)strlen(row[iColumn]));
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
extern "C" WCHAR __EXPORT *DrvGetField(MYSQL_RESULT *hResult, int iRow, int iColumn, WCHAR *pBuffer, int nBufSize)
{
   return (WCHAR *)GetFieldInternal(hResult, iRow, iColumn, pBuffer, nBufSize, false);
}

/**
 * Get field value from result as UTF8 string
 */
extern "C" char __EXPORT *DrvGetFieldUTF8(MYSQL_RESULT *hResult, int iRow, int iColumn, char *pBuffer, int nBufSize)
{
   return (char *)GetFieldInternal(hResult, iRow, iColumn, pBuffer, nBufSize, true);
}

/**
 * Get number of rows in result
 */
extern "C" int __EXPORT DrvGetNumRows(MYSQL_RESULT *hResult)
{
   return (hResult != nullptr) ? hResult->numRows : 0;
}

/**
 * Get column count in query result
 */
extern "C" int __EXPORT DrvGetColumnCount(MYSQL_RESULT *hResult)
{
	return (hResult != nullptr) ? hResult->numColumns : 0;
}

/**
 * Get column name in query result
 */
extern "C" const char __EXPORT *DrvGetColumnName(MYSQL_RESULT *hResult, int column)
{
	if (hResult == nullptr)
		return nullptr;

	MYSQL_FIELD *field = mysql_fetch_field_direct(hResult->resultSet, column);
	return (field != nullptr) ? field->name : nullptr;
}

/**
 * Free SELECT results
 */
static void DrvFreeResultInternal(MYSQL_RESULT *hResult)
{
	if (hResult->isPreparedStatement)
	{
		MemFree(hResult->bindings);
		MemFree(hResult->lengthFields);
	}

	mysql_free_result(hResult->resultSet);
	MemFree(hResult->rows);
	MemFree(hResult);
}

/**
 * Free SELECT results - public entry point
 */
extern "C" void __EXPORT DrvFreeResult(MYSQL_RESULT *hResult)
{
   if (hResult != nullptr)
      DrvFreeResultInternal(hResult);
}

/**
 * Perform unbuffered SELECT query
 */
extern "C" DBDRV_UNBUFFERED_RESULT __EXPORT DrvSelectUnbuffered(MYSQL_CONN *connection, WCHAR *pwszQuery, DWORD *pdwError, WCHAR *errorText)
{
	if (connection == nullptr)
	{
		*pdwError = DBERR_INVALID_HANDLE;
		return nullptr;
	}

   MYSQL_UNBUFFERED_RESULT *pResult = nullptr;

   char localBuffer[1024];
	char *pszQueryUTF8 = WideStringToUTF8(pwszQuery, localBuffer, 1024);
	connection->mutexQueryLock.lock();
	if (mysql_query(connection->mysql, pszQueryUTF8) == 0)
	{
		pResult = MemAllocStruct<MYSQL_UNBUFFERED_RESULT>();
		pResult->connection = connection;
		pResult->isPreparedStatement = false;
		pResult->resultSet = mysql_use_result(connection->mysql);
		if (pResult->resultSet != NULL)
		{
			pResult->noMoreRows = false;
			pResult->numColumns = mysql_num_fields(pResult->resultSet);
			pResult->pCurrRow = NULL;
			pResult->lengthFields = MemAllocArray<unsigned long>(pResult->numColumns);
			pResult->bindings = NULL;
		}
		else
		{
			MemFreeAndNull(pResult);
		}

		*pdwError = DBERR_SUCCESS;
		if (errorText != nullptr)
			*errorText = 0;
	}
	else
	{
		int nErr = mysql_errno(connection->mysql);
		if (nErr == CR_SERVER_LOST || nErr == CR_CONNECTION_ERROR || nErr == CR_SERVER_GONE_ERROR) // CR_SERVER_GONE_ERROR - ???
		{
			*pdwError = DBERR_CONNECTION_LOST;
		}
		else
		{
			*pdwError = DBERR_OTHER_ERROR;
		}
		
		if (errorText != nullptr)
		{
		   utf8_to_wchar(mysql_error(connection->mysql), -1, errorText, DBDRV_MAX_ERROR_TEXT);
			errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
			RemoveTrailingCRLFW(errorText);
		}
	}

	if (pResult == nullptr)
	{
		connection->mutexQueryLock.unlock();
	}
	FreeConvertedString(pszQueryUTF8, localBuffer);

	return pResult;
}

/**
 * Perform unbuffered SELECT query using prepared statement
 */
extern "C" DBDRV_RESULT __EXPORT DrvSelectPreparedUnbuffered(MYSQL_CONN *connection, MYSQL_STATEMENT *hStmt, DWORD *pdwError, WCHAR *errorText)
{
   MYSQL_UNBUFFERED_RESULT *result = nullptr;

   connection->mutexQueryLock.lock();

   if (mysql_stmt_bind_param(hStmt->statement, hStmt->bindings) == 0)
   {
      if (mysql_stmt_execute(hStmt->statement) == 0)
      {
         result = MemAllocStruct<MYSQL_UNBUFFERED_RESULT>();
         result->connection = connection;
         result->isPreparedStatement = true;
         result->statement = hStmt->statement;
         result->resultSet = mysql_stmt_result_metadata(hStmt->statement);
         if (result->resultSet != nullptr)
         {
            result->noMoreRows = false;
            result->numColumns = mysql_num_fields(result->resultSet);
            result->pCurrRow = nullptr;
            result->lengthFields = MemAllocArray<unsigned long>(result->numColumns);

            result->bindings = MemAllocArray<MYSQL_BIND>(result->numColumns);
            for(int i = 0; i < result->numColumns; i++)
            {
               result->bindings[i].buffer_type = MYSQL_TYPE_STRING;
               result->bindings[i].length = &result->lengthFields[i];
            }

            mysql_stmt_bind_result(hStmt->statement, result->bindings);
            *pdwError = DBERR_SUCCESS;
         }
         else
         {
            UpdateErrorMessage(mysql_stmt_error(hStmt->statement), errorText);
            *pdwError = DBERR_OTHER_ERROR;
            MemFreeAndNull(result);
         }
      }
      else
      {
         int nErr = mysql_errno(connection->mysql);
         if (nErr == CR_SERVER_LOST || nErr == CR_CONNECTION_ERROR || nErr == CR_SERVER_GONE_ERROR)
         {
            *pdwError = DBERR_CONNECTION_LOST;
         }
         else
         {
            *pdwError = DBERR_OTHER_ERROR;
         }
         UpdateErrorMessage(mysql_stmt_error(hStmt->statement), errorText);
      }
   }
   else
   {
      UpdateErrorMessage(mysql_stmt_error(hStmt->statement), errorText);
      *pdwError = DBERR_OTHER_ERROR;
   }

   if (result == NULL)
   {
      connection->mutexQueryLock.unlock();
   }
   return result;
}

/**
 * Fetch next result line from asynchronous SELECT results
 */
extern "C" bool __EXPORT DrvFetch(MYSQL_UNBUFFERED_RESULT *result)
{
   if ((result == NULL) || (result->noMoreRows))
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
         unsigned long *pLen;

         // Get column lengths for current row
         pLen = mysql_fetch_lengths(result->resultSet);
         if (pLen != NULL)
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
extern "C" LONG __EXPORT DrvGetFieldLengthUnbuffered(MYSQL_UNBUFFERED_RESULT *hResult, int iColumn)
{
	// Check if we have valid result handle
	if (hResult == nullptr)
		return 0;
	
	// Check if there are valid fetched row
	if (hResult->noMoreRows || ((hResult->pCurrRow == nullptr) && !hResult->isPreparedStatement))
		return 0;
	
	// Check if column number is valid
	if ((iColumn < 0) || (iColumn >= hResult->numColumns))
		return 0;

	return hResult->lengthFields[iColumn];
}

/**
 * Get field from current row in async query result - common implementation for wide char and UTF-8
 */
static void *GetFieldUnbufferedInternal(MYSQL_UNBUFFERED_RESULT *hResult, int iColumn, void *pBuffer, int iBufSize, bool utf8)
{
   // Check if we have valid result handle
   if (hResult == NULL)
      return NULL;

   // Check if there are valid fetched row
   if ((hResult->noMoreRows) || ((hResult->pCurrRow == NULL) && !hResult->isPreparedStatement))
      return NULL;

   // Check if column number is valid
   if ((iColumn < 0) || (iColumn >= hResult->numColumns))
      return NULL;

   // Now get column data
   void *value = NULL;
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
      int iLen = std::min((int)hResult->lengthFields[iColumn], iBufSize - 1);
      if (iLen > 0)
      {
         if (utf8)
         {
            memcpy(pBuffer, hResult->pCurrRow[iColumn], iLen);
            ((char *)pBuffer)[iLen] = 0;
         }
         else
         {
            int cch = MultiByteToWideChar(CP_UTF8, 0, hResult->pCurrRow[iColumn], iLen, (WCHAR *)pBuffer, iBufSize);
            ((WCHAR *)pBuffer)[cch] = 0;
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
extern "C" WCHAR __EXPORT *DrvGetFieldUnbuffered(MYSQL_UNBUFFERED_RESULT *hResult, int iColumn, WCHAR *pBuffer, int iBufSize)
{
	return (WCHAR *)GetFieldUnbufferedInternal(hResult, iColumn, pBuffer, iBufSize, false);
}

/**
 * Get field from current row in unbuffered query result
 */
extern "C" char __EXPORT *DrvGetFieldUnbufferedUTF8(MYSQL_UNBUFFERED_RESULT *hResult, int iColumn, char *pBuffer, int iBufSize)
{
   return (char *)GetFieldUnbufferedInternal(hResult, iColumn, pBuffer, iBufSize, true);
}

/**
 * Get column count in unbuffered query result
 */
extern "C" int __EXPORT DrvGetColumnCountUnbuffered(MYSQL_UNBUFFERED_RESULT *hResult)
{
	return (hResult != NULL) ? hResult->numColumns : 0;
}

/**
 * Get column name in unbuffered query result
 */
extern "C" const char __EXPORT *DrvGetColumnNameUnbuffered(MYSQL_UNBUFFERED_RESULT *hResult, int column)
{
	if ((hResult == NULL) || (hResult->resultSet == NULL))
		return NULL;

	MYSQL_FIELD *field = mysql_fetch_field_direct(hResult->resultSet, column);
	return (field != NULL) ? field->name : NULL;
}

/**
 * Destroy result of unbuffered query
 */
extern "C" void __EXPORT DrvFreeUnbufferedResult(MYSQL_UNBUFFERED_RESULT *hResult)
{
	if (hResult == NULL)
	   return;

   // Check if all result rows fetched
   if (!hResult->noMoreRows)
   {
      // Fetch remaining rows
      if (!hResult->isPreparedStatement)
      {
         while(mysql_fetch_row(hResult->resultSet) != nullptr);
      }

      // Now we are ready for next query, so unlock query mutex
      hResult->connection->mutexQueryLock.unlock();
   }

   // Free allocated memory
   mysql_free_result(hResult->resultSet);
   MemFree(hResult->lengthFields);
   MemFree(hResult->bindings);
   MemFree(hResult);
}

/**
 * Begin transaction
 */
extern "C" DWORD __EXPORT DrvBegin(MYSQL_CONN *connection)
{
	return DrvQueryInternal(connection, "BEGIN", nullptr);
}

/**
 * Commit transaction
 */
extern "C" DWORD __EXPORT DrvCommit(MYSQL_CONN *connection)
{
	return DrvQueryInternal(connection, "COMMIT", nullptr);
}

/**
 * Rollback transaction
 */
extern "C" DWORD __EXPORT DrvRollback(MYSQL_CONN *connection)
{
	return DrvQueryInternal(connection, "ROLLBACK", nullptr);
}

/**
 * Check if table exist
 */
extern "C" int __EXPORT DrvIsTableExist(MYSQL_CONN *connection, const WCHAR *name)
{
   if (connection == nullptr)
      return DBIsTableExist_Failure;

   WCHAR query[256], lname[256];
   wcsncpy(lname, name, 256);
   wcslwr(lname);
   swprintf(query, 256, L"SHOW TABLES LIKE '%ls'", lname);
   DWORD error;
   WCHAR errorText[DBDRV_MAX_ERROR_TEXT];
   int rc = DBIsTableExist_Failure;
   MYSQL_RESULT *hResult = DrvSelectInternal(connection, query, &error, errorText);
   if (hResult != nullptr)
   {
      rc = (hResult->numRows > 0) ? DBIsTableExist_Found : DBIsTableExist_NotFound;
      DrvFreeResultInternal(hResult);
   }
   return rc;
}

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
