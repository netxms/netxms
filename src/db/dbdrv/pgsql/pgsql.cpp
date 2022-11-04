/* 
** PostgreSQL Database Driver
** Copyright (C) 2003-2022 Raden Solutions
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
** File: pgsql.cpp
**
**/

#include "pgsqldrv.h"

#ifndef _WIN32
#include <dlfcn.h>
#endif

#ifdef _WIN32
#pragma warning(disable : 4996)
#endif

typedef Buffer<char, 1024> QueryString;

static bool UnsafeQuery(PG_CONN *pConn, const char *szQuery, WCHAR *errorText);

#ifndef _WIN32
static void *s_libpq = nullptr;
static int (*s_PQsetSingleRowMode)(PGconn *) = nullptr;
#endif

#if !HAVE_DECL_PGRES_SINGLE_TUPLE
#define PGRES_SINGLE_TUPLE    9
#endif

#define DEBUG_TAG _T("db.drv.pgsql")

/**
 * Statement ID
 */
static VolatileCounter s_statementId = 0;

/**
 * Prepare string for using in SQL query - enclose in quotes and escape as needed
 */
static StringBuffer PrepareString(const TCHAR *str, size_t maxSize)
{
	StringBuffer out;
	out.append(_T('\''));
	for(const TCHAR *src = str; (*src != 0) && (maxSize > 0); src++, maxSize--)
	{
		if (*src < 32)
		{
			TCHAR buffer[8];
			_sntprintf(buffer, 8, _T("\\%03o"), static_cast<uint32_t>(*src));
			out.append(buffer);
		}
		else if (*src == _T('\''))
		{
         out.append(_T("''"), 2);
		}
		else if (*src == _T('\\'))
		{
         out.append(_T("\\\\"), 2);
		}
		else
		{
			out.append(*src);
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
#ifndef _WIN32
   s_libpq = dlopen("libpq.so.5", RTLD_NOW);
   if (s_libpq != nullptr)
      s_PQsetSingleRowMode = (int (*)(PGconn *))dlsym(s_libpq, "PQsetSingleRowMode");
   nxlog_debug_tag(DEBUG_TAG, 2, _T("PostgreSQL driver: single row mode %s"), (s_PQsetSingleRowMode != NULL) ? _T("enabled") : _T("disabled"));
#endif
	return true;
}

/**
 * Unload handler
 */
static void Unload()
{
#ifndef _WIN32
   if (s_libpq != nullptr)
      dlclose(s_libpq);
#endif
}

/**
 * Connect to database
 */
static DBDRV_CONNECTION Connect(const char *serverAddress,	const char *login, const char *password, const char *database, const char *schema, WCHAR *errorText)
{
	if ((database != nullptr) && (*database == 0))
	{
		wcscpy(errorText, L"Database name is empty");
		return nullptr;
	}

	if ((serverAddress == nullptr) || (*serverAddress == 0))
	{
		wcscpy(errorText, L"Host name is empty");
		return nullptr;
	}

	const char *host, *port;
	char hostBuffer[1024];
   const char *p = strchr(serverAddress, ':');
   if (p != nullptr)
   {
      strlcpy(hostBuffer, serverAddress, 1024);
      host = hostBuffer;
      hostBuffer[p - serverAddress] = 0;
      port = p + 1;
   }
   else
   {
      host = serverAddress;
      port = nullptr;
   }
	
	PG_CONN *connection = nullptr;

	// should be replaced with PQconnectdb();
	PGconn *handle = PQsetdbLogin(host, port, nullptr, nullptr, (database != nullptr) ? database : "template1", login, password);
   if (PQstatus(handle) != CONNECTION_BAD)
   {
      PGresult *pResult = PQexec(handle, "SET standard_conforming_strings TO off");
      PQclear(pResult);

      pResult = PQexec(handle, "SET escape_string_warning TO off");
      PQclear(pResult);

      PQsetClientEncoding(handle, "UTF8");

      connection = new PG_CONN(handle);

      if ((schema != nullptr) && (schema[0] != 0))
      {
         char query[256];
         snprintf(query, 256, "SET search_path=%s", schema);
         if (!UnsafeQuery(connection, query, errorText))
         {
            PQfinish(handle);
            delete_and_null(connection);
         }
      }
   }
   else
   {
      utf8_to_wchar(PQerrorMessage(handle), -1, errorText, DBDRV_MAX_ERROR_TEXT);
      errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
      RemoveTrailingCRLFW(errorText);
      PQfinish(handle);
   }

   return connection;
}

/**
 * Disconnect from database
 */
static void Disconnect(DBDRV_CONNECTION connection)
{
   PQfinish(static_cast<PG_CONN*>(connection)->handle);
   delete static_cast<PG_CONN*>(connection);
}

/**
 * Convert wide character query string to UTF-8
 */
inline QueryString QueryToUTF8(const WCHAR *query)
{
#ifdef UNICODE_UCS4
   size_t len = ucs4_utf8len(query, -1);
#else
   size_t len = ucs2_utf8len(query, -1);
#endif
   QueryString queryUTF8(len);
#ifdef UNICODE_UCS4
   ucs4_to_utf8(query, -1, queryUTF8, len);
#else
   ucs2_to_utf8(query, -1, queryUTF8, len);
#endif
   return queryUTF8;
}

/**
 * Convert query from NetXMS portable format to native PostgreSQL format
 */
static QueryString ConvertQuery(const WCHAR *query)
{
   int count = NumCharsW(query, '?');
   if (count == 0)
      return QueryToUTF8(query);

   QueryString srcQuery = QueryToUTF8(query);

	size_t dstSize = strlen(srcQuery) + count * 3 + 1;
	QueryString dstQuery(dstSize);
	bool inString = false;
	int pos = 1;
	char *src, *dst;
	for(src = srcQuery.buffer(), dst = dstQuery.buffer(); *src != 0; src++)
	{
		switch(*src)
		{
			case '\'':
				*dst++ = *src;
				inString = !inString;
				break;
			case '\\':
				*dst++ = *src++;
				*dst++ = *src;
				break;
			case '?':
				if (inString)
				{
					*dst++ = '?';
				}
				else
				{
					*dst++ = '$';
					if (pos < 10)
					{
						*dst++ = pos + '0';
					}
					else if (pos < 100)
					{
						*dst++ = pos / 10 + '0';
						*dst++ = pos % 10 + '0';
					}
					else
					{
						*dst++ = pos / 100 + '0';
						*dst++ = (pos % 100) / 10 + '0';
						*dst++ = pos % 10 + '0';
					}
					pos++;
				}
				break;
			default:
				*dst++ = *src;
				break;
		}
	}
	*dst = 0;
	return dstQuery;
}

/**
 * Prepare statement
 */
static DBDRV_STATEMENT Prepare(DBDRV_CONNECTION connection, const WCHAR *query, bool optimizeForReuse, uint32_t *errorCode, WCHAR *errorText)
{
   QueryString queryUTF8 = ConvertQuery(query);
	auto hStmt = new PG_STATEMENT(static_cast<PG_CONN*>(connection));

   if (optimizeForReuse)
   {
      snprintf(hStmt->name, 64, "netxms_stmt_%p_%d", hStmt, (int)InterlockedIncrement(&s_statementId));

      static_cast<PG_CONN*>(connection)->mutexQueryLock.lock();
      PGresult	*pResult = PQprepare(static_cast<PG_CONN*>(connection)->handle, hStmt->name, queryUTF8, 0, nullptr);
      if ((pResult == nullptr) || (PQresultStatus(pResult) != PGRES_COMMAND_OK))
      {
         delete_and_null(hStmt);

         *errorCode = (PQstatus(static_cast<PG_CONN*>(connection)->handle) == CONNECTION_BAD) ? DBERR_CONNECTION_LOST : DBERR_OTHER_ERROR;

         if (errorText != nullptr)
         {
            utf8_to_wchar(PQerrorMessage(static_cast<PG_CONN*>(connection)->handle), -1, errorText, DBDRV_MAX_ERROR_TEXT);
            errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
            RemoveTrailingCRLFW(errorText);
         }
      }
      else
      {
         *errorCode = DBERR_SUCCESS;
      }
      static_cast<PG_CONN*>(connection)->mutexQueryLock.unlock();
      if (pResult != nullptr)
         PQclear(pResult);
   }
   else
   {
      hStmt->name[0] = 0;
      hStmt->query = queryUTF8.takeBuffer();
   }
	return hStmt;
}

/**
 * Bind parameter to prepared statement
 */
static void Bind(DBDRV_STATEMENT hStmt, int pos, int sqlType, int cType, void *buffer, int allocType)
{
	if (pos <= 0)
		return;

	auto stmt = static_cast<PG_STATEMENT*>(hStmt);
	if (stmt->buffers.size() < pos)
	{
	   if (stmt->buffers.capacity() < pos)
	   {
	      stmt->buffers.reserve(std::max(stmt->buffers.capacity() + 16, static_cast<size_t>(pos)));
	   }
	   for(int i = static_cast<int>(stmt->buffers.size()); i < pos; i++)
	      stmt->buffers.emplace_back();
	}

	size_t utf8len;
	switch(cType)
	{
		case DB_CTYPE_STRING:
		   utf8len = wchar_utf8len(static_cast<WCHAR*>(buffer), -1);
			stmt->buffers[pos - 1].realloc(utf8len);
		   wchar_to_utf8(static_cast<WCHAR*>(buffer), -1, stmt->buffers[pos - 1], utf8len);
			break;
      case DB_CTYPE_UTF8_STRING:
         if (allocType == DB_BIND_DYNAMIC)
         {
            stmt->buffers[pos - 1].setPreallocated(static_cast<char*>(buffer), strlen(static_cast<char*>(buffer)) + 1);
            buffer = nullptr; // prevent deallocation
         }
         else
         {
            stmt->buffers[pos - 1].set(static_cast<char*>(buffer), strlen(static_cast<char*>(buffer)) + 1);
         }
         break;
		case DB_CTYPE_INT32:
			stmt->buffers[pos - 1].realloc(16);
			IntegerToString(*static_cast<int32_t*>(buffer), stmt->buffers[pos - 1]);
			break;
		case DB_CTYPE_UINT32:
         stmt->buffers[pos - 1].realloc(16);
         IntegerToString(*static_cast<uint32_t*>(buffer), stmt->buffers[pos - 1]);
			break;
		case DB_CTYPE_INT64:
         stmt->buffers[pos - 1].realloc(32);
         IntegerToString(*static_cast<int64_t*>(buffer), stmt->buffers[pos - 1]);
			break;
		case DB_CTYPE_UINT64:
         stmt->buffers[pos - 1].realloc(32);
         IntegerToString(*static_cast<uint64_t*>(buffer), stmt->buffers[pos - 1]);
			break;
		case DB_CTYPE_DOUBLE:
         stmt->buffers[pos - 1].realloc(32);
			sprintf(stmt->buffers[pos - 1], "%f", *static_cast<double*>(buffer));
			break;
		default:
			stmt->buffers[pos - 1].set("", 1);
			break;
	}

	if (allocType == DB_BIND_DYNAMIC)
		MemFree(buffer);
}

/**
 * Execute prepared statement
 */
static uint32_t Execute(DBDRV_CONNECTION connection, DBDRV_STATEMENT hStmt, WCHAR *errorText)
{
	uint32_t rc;
   auto stmt = static_cast<PG_STATEMENT*>(hStmt);

   char **values = static_cast<char**>(MemAllocLocal(stmt->buffers.size() * sizeof(char*)));
   for(int i = 0; i < stmt->buffers.size(); i++)
      values[i] = stmt->buffers[i].buffer();

	static_cast<PG_CONN*>(connection)->mutexQueryLock.lock();
   bool retry;
   int retryCount = 60;
   do
   {
      retry = false;
	   PGresult	*pResult = (stmt->name[0] != 0) ? 
         PQexecPrepared(static_cast<PG_CONN*>(connection)->handle, stmt->name, static_cast<int>(stmt->buffers.size()), values, nullptr, nullptr, 0) :
         PQexecParams(static_cast<PG_CONN*>(connection)->handle, stmt->query, static_cast<int>(stmt->buffers.size()), nullptr, values, nullptr, nullptr, 0);
      if (PQresultStatus(pResult) == PGRES_COMMAND_OK)
      {
         if (errorText != nullptr)
            *errorText = 0;
         rc = DBERR_SUCCESS;
      }
      else
      {
         const char *sqlState = PQresultErrorField(pResult, PG_DIAG_SQLSTATE);
         if ((PQstatus(static_cast<PG_CONN*>(connection)->handle) != CONNECTION_BAD) &&
             (sqlState != nullptr) && (!strcmp(sqlState, "53000") || !strcmp(sqlState, "53200")) && (retryCount > 0))
         {
            ThreadSleep(500);
            retry = true;
            retryCount--;
         }
         else
         {
            if (errorText != nullptr)
            {
               utf8_to_wchar(CHECK_NULL_EX_A(sqlState), -1, errorText, DBDRV_MAX_ERROR_TEXT);
               int len = (int)wcslen(errorText);
               if (len > 0)
               {
                  errorText[len] = L' ';
                  len++;
               }
               utf8_to_wchar(PQerrorMessage(static_cast<PG_CONN*>(connection)->handle), -1, &errorText[len], DBDRV_MAX_ERROR_TEXT - len);
               errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
               RemoveTrailingCRLFW(errorText);
            }
            rc = (PQstatus(static_cast<PG_CONN*>(connection)->handle) == CONNECTION_BAD) ? DBERR_CONNECTION_LOST : DBERR_OTHER_ERROR;
         }
      }

      PQclear(pResult);
   }
   while(retry);
   static_cast<PG_CONN*>(connection)->mutexQueryLock.unlock();
	return rc;
}

/**
 * Destroy prepared statement
 */
static void FreeStatement(DBDRV_STATEMENT hStmt)
{
   auto stmt = static_cast<PG_STATEMENT*>(hStmt);
   if (stmt->name[0] != 0)
   {
      char query[256];
      snprintf(query, 256, "DEALLOCATE \"%s\"", stmt->name);

      stmt->connection->mutexQueryLock.lock();
      UnsafeQuery(stmt->connection, query, nullptr);
      stmt->connection->mutexQueryLock.unlock();
   }
   delete stmt;
}

/**
 * Perform non-SELECT query - internal implementation
 */
static bool UnsafeQuery(PG_CONN *pConn, const char *szQuery, WCHAR *errorText)
{
   int retryCount = 60;

retry:
   PGresult	*result = PQexec(pConn->handle, szQuery);

   if ((PQresultStatus(result) != PGRES_COMMAND_OK) && (PQresultStatus(result) != PGRES_TUPLES_OK))
	{
      const char *sqlState = PQresultErrorField(result, PG_DIAG_SQLSTATE);
      if ((PQstatus(pConn->handle) != CONNECTION_BAD) &&
          (sqlState != nullptr) && (!strcmp(sqlState, "53000") || !strcmp(sqlState, "53200")) && (retryCount > 0))
      {
         ThreadSleep(500);
         retryCount--;
   		PQclear(result);
         goto retry;
      }
      else
      {
	      if (errorText != nullptr)
	      {
		      utf8_to_wchar(CHECK_NULL_EX_A(sqlState), -1, errorText, DBDRV_MAX_ERROR_TEXT);
            int len = (int)wcslen(errorText);
            if (len > 0)
            {
               errorText[len] = L' ';
               len++;
            }
            utf8_to_wchar(PQerrorMessage(pConn->handle), -1, &errorText[len], DBDRV_MAX_ERROR_TEXT - len);
		      errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
		      RemoveTrailingCRLFW(errorText);
	      }
      }
		PQclear(result);
		return false;
	}

	PQclear(result);
	if (errorText != nullptr)
		*errorText = 0;
   return true;
}

/**
 * Perform non-SELECT query
 */
static uint32_t Query(DBDRV_CONNECTION connection, const WCHAR *query, WCHAR *errorText)
{
	uint32_t rc;

	QueryString queryUTF8 = QueryToUTF8(query);
   static_cast<PG_CONN*>(connection)->mutexQueryLock.lock();
	if (UnsafeQuery(static_cast<PG_CONN*>(connection), queryUTF8, errorText))
   {
      rc = DBERR_SUCCESS;
   }
   else
   {
      rc = (PQstatus(static_cast<PG_CONN*>(connection)->handle) == CONNECTION_BAD) ? DBERR_CONNECTION_LOST : DBERR_OTHER_ERROR;
   }
	static_cast<PG_CONN*>(connection)->mutexQueryLock.unlock();

	return rc;
}

/**
 * Perform SELECT query - internal implementation
 */
static DBDRV_RESULT UnsafeSelect(PG_CONN *conn, const char *query, WCHAR *errorText)
{
   int retryCount = 60;

retry:
	PGresult	*result = PQexec(conn->handle, query);

	if ((PQresultStatus(result) != PGRES_COMMAND_OK) && (PQresultStatus(result) != PGRES_TUPLES_OK))
	{	
      const char *sqlState = PQresultErrorField(result, PG_DIAG_SQLSTATE);
      if ((PQstatus(conn->handle) != CONNECTION_BAD) &&
          (sqlState != nullptr) && (!strcmp(sqlState, "53000") || !strcmp(sqlState, "53200")) && (retryCount > 0))
      {
         ThreadSleep(500);
         retryCount--;
   		PQclear(result);
         goto retry;
      }
      else
      {
	      if (errorText != nullptr)
	      {
	         utf8_to_wchar(CHECK_NULL_EX_A(sqlState), -1, errorText, DBDRV_MAX_ERROR_TEXT);
            int len = (int)wcslen(errorText);
            if (len > 0)
            {
               errorText[len] = L' ';
               len++;
            }
            utf8_to_wchar(PQerrorMessage(conn->handle), -1, &errorText[len], DBDRV_MAX_ERROR_TEXT - len);
		      errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
		      RemoveTrailingCRLFW(errorText);
	      }
      }
		PQclear(result);
		return nullptr;
	}

	if (errorText != nullptr)
		*errorText = 0;
   return (DBDRV_RESULT)result;
}

/**
 * Perform SELECT query
 */
static DBDRV_RESULT Select(DBDRV_CONNECTION connection, const WCHAR *query, uint32_t *errorCode, WCHAR *errorText)
{
   QueryString queryUTF8 = QueryToUTF8(query);
   static_cast<PG_CONN*>(connection)->mutexQueryLock.lock();
	DBDRV_RESULT pResult = UnsafeSelect(static_cast<PG_CONN*>(connection), queryUTF8, errorText);
   if (pResult != nullptr)
   {
      *errorCode = DBERR_SUCCESS;
   }
   else
   {
      *errorCode = (PQstatus(static_cast<PG_CONN*>(connection)->handle) == CONNECTION_BAD) ? DBERR_CONNECTION_LOST : DBERR_OTHER_ERROR;
   }
   static_cast<PG_CONN*>(connection)->mutexQueryLock.unlock();
   return pResult;
}

/**
 * Perform SELECT query using prepared statement
 */
static DBDRV_RESULT SelectPrepared(DBDRV_CONNECTION connection, DBDRV_STATEMENT hStmt, uint32_t *errorCode, WCHAR *errorText)
{
   auto stmt = static_cast<PG_STATEMENT*>(hStmt);
   PGresult	*result;

   char **values = static_cast<char**>(MemAllocLocal(stmt->buffers.size() * sizeof(char*)));
   for(size_t i = 0; i < stmt->buffers.size(); i++)
      values[i] = stmt->buffers[i].buffer();

   static_cast<PG_CONN*>(connection)->mutexQueryLock.lock();
   bool retry;
   int retryCount = 60;
   do
   {
      retry = false;
	   result = (stmt->name[0] != 0) ?
            PQexecPrepared(static_cast<PG_CONN*>(connection)->handle, stmt->name, static_cast<int>(stmt->buffers.size()), values, nullptr, nullptr, 0) :
            PQexecParams(static_cast<PG_CONN*>(connection)->handle, stmt->query, static_cast<int>(stmt->buffers.size()), nullptr, values, nullptr, nullptr, 0);
      if ((PQresultStatus(result) == PGRES_COMMAND_OK) || (PQresultStatus(result) == PGRES_TUPLES_OK))
      {
         if (errorText != nullptr)
            *errorText = 0;
         *errorCode = DBERR_SUCCESS;
      }
      else
      {
         const char *sqlState = PQresultErrorField(result, PG_DIAG_SQLSTATE);
         if ((PQstatus(static_cast<PG_CONN*>(connection)->handle) != CONNECTION_BAD) &&
             (sqlState != nullptr) && (!strcmp(sqlState, "53000") || !strcmp(sqlState, "53200")) && (retryCount > 0))
         {
            ThreadSleep(500);
            retry = true;
            retryCount--;
         }
         else
         {
            if (errorText != nullptr)
            {
               utf8_to_wchar(CHECK_NULL_EX_A(sqlState), -1, errorText, DBDRV_MAX_ERROR_TEXT);
               int len = (int)wcslen(errorText);
               if (len > 0)
               {
                  errorText[len] = L' ';
                  len++;
               }
               utf8_to_wchar(PQerrorMessage(static_cast<PG_CONN*>(connection)->handle), -1, &errorText[len], DBDRV_MAX_ERROR_TEXT - len);
               errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
               RemoveTrailingCRLFW(errorText);
            }
         }
         PQclear(result);
         result = nullptr;
         *errorCode = (PQstatus(static_cast<PG_CONN*>(connection)->handle) == CONNECTION_BAD) ? DBERR_CONNECTION_LOST : DBERR_OTHER_ERROR;
      }
   }
   while(retry);
   static_cast<PG_CONN*>(connection)->mutexQueryLock.unlock();

	return result;
}

/**
 * Get field length from result
 */
static int32_t GetFieldLength(DBDRV_RESULT result, int row, int column)
{
   const char *value = PQgetvalue(static_cast<PGresult*>(result), row, column);
   return (value != nullptr) ? static_cast<int32_t>(strlen(value)) : -1;
}

/**
 * Get field value from result
 */
static WCHAR *GetField(DBDRV_RESULT result, int row, int column, WCHAR *buffer, int size)
{
   if (PQfformat(static_cast<PGresult*>(result), column) != 0)
      return nullptr;

   const char *value = PQgetvalue(static_cast<PGresult*>(result), row, column);
   if (value == nullptr)
      return nullptr;

   utf8_to_wchar(value, -1, buffer, size);
   buffer[size - 1] = 0;
   return buffer;
}

/**
 * Get field value from result as UTF8 string
 */
static char *GetFieldUTF8(DBDRV_RESULT result, int row, int column, char *buffer, int size)
{
	const char *value = PQgetvalue(static_cast<PGresult*>(result), row, column);
	if (value == nullptr)
	   return nullptr;

   strlcpy(buffer, value, size);
	return nullptr;
}

/**
 * Get number of rows in result
 */
static int GetNumRows(DBDRV_RESULT result)
{
	return PQntuples(static_cast<PGresult*>(result));
}

/**
 * Get column count in query result
 */
static int GetColumnCount(DBDRV_RESULT result)
{
	return PQnfields(static_cast<PGresult*>(result));
}

/**
 * Get column name in query result
 */
static const char *GetColumnName(DBDRV_RESULT result, int column)
{
	return PQfname(static_cast<PGresult*>(result), column);
}

/**
 * Free SELECT results
 */
static void FreeResult(DBDRV_RESULT result)
{
   PQclear(static_cast<PGresult*>(result));
}

/**
 * Perform unbuffered SELECT query
 */
static DBDRV_UNBUFFERED_RESULT SelectUnbuffered(DBDRV_CONNECTION connection, const WCHAR *query, uint32_t *errorCode, WCHAR *errorText)
{
	PG_UNBUFFERED_RESULT *result = MemAllocStruct<PG_UNBUFFERED_RESULT>();
	result->conn = static_cast<PG_CONN*>(connection);
   result->fetchBuffer = nullptr;
   result->keepFetchBuffer = true;

   static_cast<PG_CONN*>(connection)->mutexQueryLock.lock();

	bool success = false;
	bool retry;
	int retryCount = 60;
   QueryString queryUTF8 = QueryToUTF8(query);
   do
   {
      retry = false;
      if (PQsendQuery(static_cast<PG_CONN*>(connection)->handle, queryUTF8))
      {
#ifdef _WIN32
         if (PQsetSingleRowMode(static_cast<PG_CONN*>(connection)->handle))
         {
            result->singleRowMode = true;
#else
         if ((s_PQsetSingleRowMode == nullptr) || s_PQsetSingleRowMode(static_cast<PG_CONN*>(connection)->handle))
         {
            result->singleRowMode = (s_PQsetSingleRowMode != nullptr);
#endif
            result->currRow = 0;

            // Fetch first row (to check for errors in Select instead of Fetch call)
            result->fetchBuffer = PQgetResult(static_cast<PG_CONN*>(connection)->handle);
            if ((PQresultStatus(result->fetchBuffer) == PGRES_COMMAND_OK) ||
                (PQresultStatus(result->fetchBuffer) == PGRES_TUPLES_OK) ||
                (PQresultStatus(result->fetchBuffer) == PGRES_SINGLE_TUPLE))
            {
               if (errorText != nullptr)
                  *errorText = 0;
               *errorCode = DBERR_SUCCESS;
               success = true;
            }
            else
            {
               const char *sqlState = PQresultErrorField(result->fetchBuffer, PG_DIAG_SQLSTATE);
               if ((PQstatus(static_cast<PG_CONN*>(connection)->handle) != CONNECTION_BAD) &&
                   (sqlState != nullptr) && (!strcmp(sqlState, "53000") || !strcmp(sqlState, "53200")) && (retryCount > 0))
               {
                  ThreadSleep(500);
                  retry = true;
                  retryCount--;
               }
               else
               {
                  if (errorText != nullptr)
                  {
                     utf8_to_wchar(CHECK_NULL_EX_A(sqlState), -1, errorText, DBDRV_MAX_ERROR_TEXT);
                     int len = (int)wcslen(errorText);
                     if (len > 0)
                     {
                        errorText[len] = L' ';
                        len++;
                     }
                     utf8_to_wchar(PQerrorMessage(static_cast<PG_CONN*>(connection)->handle), -1, &errorText[len], DBDRV_MAX_ERROR_TEXT - len);
                     errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
                     RemoveTrailingCRLFW(errorText);
                  }
               }
               PQclear(result->fetchBuffer);
               result->fetchBuffer = nullptr;
               *errorCode = (PQstatus(static_cast<PG_CONN*>(connection)->handle) == CONNECTION_BAD) ? DBERR_CONNECTION_LOST : DBERR_OTHER_ERROR;
            }
         }
         else
         {
            if (errorText != nullptr)
               wcsncpy(errorText, L"Internal error (call to PQsetSingleRowMode failed)", DBDRV_MAX_ERROR_TEXT);
            *errorCode = (PQstatus(static_cast<PG_CONN*>(connection)->handle) == CONNECTION_BAD) ? DBERR_CONNECTION_LOST : DBERR_OTHER_ERROR;
         }
      }
      else
      {
         if (errorText != nullptr)
            wcsncpy(errorText, L"Internal error (call to PQsendQuery failed)", DBDRV_MAX_ERROR_TEXT);
         *errorCode = (PQstatus(static_cast<PG_CONN*>(connection)->handle) == CONNECTION_BAD) ? DBERR_CONNECTION_LOST : DBERR_OTHER_ERROR;
      }
   }
   while(retry);

   if (!success)
   {
      MemFreeAndNull(result);
      static_cast<PG_CONN*>(connection)->mutexQueryLock.unlock();
   }
   return result;
}

/**
 * Perform unbuffered SELECT query using prepared statement
 */
static DBDRV_UNBUFFERED_RESULT SelectPreparedUnbuffered(DBDRV_CONNECTION connection, DBDRV_STATEMENT hStmt, uint32_t *errorCode, WCHAR *errorText)
{
   PG_UNBUFFERED_RESULT *result = MemAllocStruct<PG_UNBUFFERED_RESULT>();
   result->conn = static_cast<PG_CONN*>(connection);
   result->fetchBuffer = nullptr;
   result->keepFetchBuffer = true;

   auto stmt = static_cast<PG_STATEMENT*>(hStmt);

   char **values = static_cast<char**>(MemAllocLocal(stmt->buffers.size() * sizeof(char*)));
   for(int i = 0; i < stmt->buffers.size(); i++)
      values[i] = stmt->buffers[i].buffer();

   static_cast<PG_CONN*>(connection)->mutexQueryLock.lock();

   bool success = false;
   bool retry;
   int retryCount = 60;
   do
   {
      retry = false;
      int sendSuccess = (stmt->name[0] != 0) ?
            PQsendQueryPrepared(static_cast<PG_CONN*>(connection)->handle, stmt->name, static_cast<int>(stmt->buffers.size()), values, nullptr, nullptr, 0) :
            PQsendQueryParams(static_cast<PG_CONN*>(connection)->handle, stmt->query, static_cast<int>(stmt->buffers.size()), nullptr, values, nullptr, nullptr, 0);
      if (sendSuccess)
      {
#ifdef _WIN32
         if (PQsetSingleRowMode(static_cast<PG_CONN*>(connection)->handle))
         {
            result->singleRowMode = true;
#else
         if ((s_PQsetSingleRowMode == nullptr) || s_PQsetSingleRowMode(static_cast<PG_CONN*>(connection)->handle))
         {
            result->singleRowMode = (s_PQsetSingleRowMode != nullptr);
#endif
            result->currRow = 0;

            // Fetch first row (to check for errors in Select instead of Fetch call)
            result->fetchBuffer = PQgetResult(static_cast<PG_CONN*>(connection)->handle);
            if ((PQresultStatus(result->fetchBuffer) == PGRES_COMMAND_OK) ||
                (PQresultStatus(result->fetchBuffer) == PGRES_TUPLES_OK) ||
                (PQresultStatus(result->fetchBuffer) == PGRES_SINGLE_TUPLE))
            {
               if (errorText != nullptr)
                  *errorText = 0;
               *errorCode = DBERR_SUCCESS;
               success = true;
            }
            else
            {
               const char *sqlState = PQresultErrorField(result->fetchBuffer, PG_DIAG_SQLSTATE);
               if ((PQstatus(static_cast<PG_CONN*>(connection)->handle) != CONNECTION_BAD) &&
                   (sqlState != nullptr) && (!strcmp(sqlState, "53000") || !strcmp(sqlState, "53200")) && (retryCount > 0))
               {
                  ThreadSleep(500);
                  retry = true;
                  retryCount--;
               }
               else
               {
                  if (errorText != nullptr)
                  {
                     utf8_to_wchar(CHECK_NULL_EX_A(sqlState), -1, errorText, DBDRV_MAX_ERROR_TEXT);
                     int len = (int)wcslen(errorText);
                     if (len > 0)
                     {
                        errorText[len] = L' ';
                        len++;
                     }
                     utf8_to_wchar(PQerrorMessage(static_cast<PG_CONN*>(connection)->handle), -1, &errorText[len], DBDRV_MAX_ERROR_TEXT - len);
                     errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
                     RemoveTrailingCRLFW(errorText);
                  }
               }
               PQclear(result->fetchBuffer);
               result->fetchBuffer = nullptr;
               *errorCode = (PQstatus(static_cast<PG_CONN*>(connection)->handle) == CONNECTION_BAD) ? DBERR_CONNECTION_LOST : DBERR_OTHER_ERROR;
            }
         }
         else
         {
            if (errorText != nullptr)
               wcsncpy(errorText, L"Internal error (call to PQsetSingleRowMode failed)", DBDRV_MAX_ERROR_TEXT);
            *errorCode = (PQstatus(static_cast<PG_CONN*>(connection)->handle) == CONNECTION_BAD) ? DBERR_CONNECTION_LOST : DBERR_OTHER_ERROR;
         }
      }
      else
      {
         if (errorText != nullptr)
            wcsncpy(errorText, L"Internal error (call to PQsendQueryPrepared/PQsendQueryParams failed)", DBDRV_MAX_ERROR_TEXT);
         *errorCode = (PQstatus(static_cast<PG_CONN*>(connection)->handle) == CONNECTION_BAD) ? DBERR_CONNECTION_LOST : DBERR_OTHER_ERROR;
      }
   }
   while(retry);

   if (!success)
   {
      MemFreeAndNull(result);
      static_cast<PG_CONN*>(connection)->mutexQueryLock.unlock();
   }
   return result;
}

/**
 * Fetch next result line from asynchronous SELECT results
 */
static bool Fetch(DBDRV_UNBUFFERED_RESULT hResult)
{
   auto result = static_cast<PG_UNBUFFERED_RESULT*>(hResult);

   if (!result->keepFetchBuffer)
   {
      if (result->singleRowMode)
      {
         if (result->fetchBuffer != nullptr)
            PQclear(result->fetchBuffer);
         result->fetchBuffer = PQgetResult(result->conn->handle);
      }
      else
      {
         if (result->fetchBuffer != nullptr)
         {
            result->currRow++;
            if (result->currRow >= PQntuples(result->fetchBuffer))
            {
               PQclear(result->fetchBuffer);
               result->fetchBuffer = PQgetResult(result->conn->handle);
               result->currRow = 0;
            }
         }
         else
         {
            result->currRow = 0;
         }
      }
   }
   else
   {
      result->keepFetchBuffer = false;
   }

   if (result->fetchBuffer == nullptr)
      return false;

   bool success;
   if ((PQresultStatus(result->fetchBuffer) == PGRES_SINGLE_TUPLE) || (PQresultStatus(result->fetchBuffer) == PGRES_TUPLES_OK))
   {
      success = (PQntuples(result->fetchBuffer) > 0);
   }
   else
   {
      PQclear(result->fetchBuffer);
      result->fetchBuffer = nullptr;
      success = false;
   }

   return success;
}

/**
 * Get field length from async quety result
 */
static int32_t GetFieldLengthUnbuffered(DBDRV_UNBUFFERED_RESULT hResult, int column)
{
   auto result = static_cast<PG_UNBUFFERED_RESULT*>(hResult);

	if (result->fetchBuffer == nullptr)
		return 0;

	// validate column index
	if (column >= PQnfields(result->fetchBuffer))
		return 0;

	char *value = PQgetvalue(result->fetchBuffer, result->currRow, column);
	if (value == nullptr)
		return 0;

	return static_cast<int32_t>(strlen(value));
}

/**
 * Get field from current row in async query result
 */
static WCHAR *GetFieldUnbuffered(DBDRV_UNBUFFERED_RESULT hResult, int column, WCHAR *buffer, int bufferSize)
{
   auto result = static_cast<PG_UNBUFFERED_RESULT*>(hResult);

	if (result->fetchBuffer == nullptr)
		return nullptr;

	// validate column index
	if (column >= PQnfields(result->fetchBuffer))
		return nullptr;

	if (PQfformat(result->fetchBuffer, column) != 0)
		return nullptr;

	char *value = PQgetvalue(result->fetchBuffer, result->currRow, column);
	if (value == nullptr)
		return nullptr;

	utf8_to_wchar(value, -1, buffer, bufferSize);
   buffer[bufferSize - 1] = 0;

   return buffer;
}

/**
 * Get field from current row in async query result as UTF-8 string
 */
static char *GetFieldUnbufferedUTF8(DBDRV_UNBUFFERED_RESULT hResult, int column, char *buffer, int bufferSize)
{
   auto result = static_cast<PG_UNBUFFERED_RESULT*>(hResult);

   if (result->fetchBuffer == nullptr)
      return nullptr;

   // validate column index
   if (column >= PQnfields(result->fetchBuffer))
      return nullptr;

   if (PQfformat(result->fetchBuffer, column) != 0)
      return nullptr;

   char *value = PQgetvalue(result->fetchBuffer, result->currRow, column);
   if (value == nullptr)
      return nullptr;

   strlcpy(buffer, value, bufferSize);
   return buffer;
}

/**
 * Get column count in async query result
 */
static int GetColumnCountUnbuffered(DBDRV_UNBUFFERED_RESULT hResult)
{
	return (static_cast<PG_UNBUFFERED_RESULT*>(hResult)->fetchBuffer != nullptr) ? PQnfields(static_cast<PG_UNBUFFERED_RESULT*>(hResult)->fetchBuffer) : 0;
}

/**
 * Get column name in async query result
 */
static const char *GetColumnNameUnbuffered(DBDRV_UNBUFFERED_RESULT hResult, int column)
{
	return (static_cast<PG_UNBUFFERED_RESULT*>(hResult)->fetchBuffer != nullptr)? PQfname(static_cast<PG_UNBUFFERED_RESULT*>(hResult)->fetchBuffer, column) : nullptr;
}

/**
 * Destroy result of async query
 */
static void FreeUnbufferedResult(DBDRV_UNBUFFERED_RESULT hResult)
{
   auto result = static_cast<PG_UNBUFFERED_RESULT*>(hResult);

   if (result->fetchBuffer != nullptr)
      PQclear(result->fetchBuffer);

   // read all outstanding results
   while(true)
   {
      result->fetchBuffer = PQgetResult(result->conn->handle);
      if (result->fetchBuffer == nullptr)
         break;
      PQclear(result->fetchBuffer);
   }

   result->conn->mutexQueryLock.unlock();
   MemFree(result);
}

/**
 * Begin transaction
 */
static uint32_t Begin(DBDRV_CONNECTION connection)
{
   uint32_t dwResult;
   static_cast<PG_CONN*>(connection)->mutexQueryLock.lock();
	if (UnsafeQuery(static_cast<PG_CONN*>(connection), "BEGIN", nullptr))
   {
      dwResult = DBERR_SUCCESS;
   }
   else
   {
      dwResult = (PQstatus(static_cast<PG_CONN*>(connection)->handle) == CONNECTION_BAD) ? DBERR_CONNECTION_LOST : DBERR_OTHER_ERROR;
   }
	static_cast<PG_CONN*>(connection)->mutexQueryLock.unlock();
   return dwResult;
}

/**
 * Commit transaction
 */
static uint32_t Commit(DBDRV_CONNECTION connection)
{
   static_cast<PG_CONN*>(connection)->mutexQueryLock.lock();
	bool bRet = UnsafeQuery(static_cast<PG_CONN*>(connection), "COMMIT", nullptr);
	static_cast<PG_CONN*>(connection)->mutexQueryLock.unlock();
   return bRet ? DBERR_SUCCESS : DBERR_OTHER_ERROR;
}

/**
 * Rollback transaction
 */
static uint32_t Rollback(DBDRV_CONNECTION connection)
{
	static_cast<PG_CONN*>(connection)->mutexQueryLock.lock();
	bool bRet = UnsafeQuery(static_cast<PG_CONN*>(connection), "ROLLBACK", nullptr);
	static_cast<PG_CONN*>(connection)->mutexQueryLock.unlock();
   return bRet ? DBERR_SUCCESS : DBERR_OTHER_ERROR;
}

/**
 * Check if table exist
 */
static int IsTableExist(DBDRV_CONNECTION connection, const WCHAR *name)
{
   char query[256];
   snprintf(query, 256, "SELECT count(*) FROM information_schema.tables WHERE table_catalog=current_database() AND table_schema=current_schema() AND lower(table_name)=lower('%ls')", name);
   WCHAR errorText[DBDRV_MAX_ERROR_TEXT];
   int rc = DBIsTableExist_Failure;
   static_cast<PG_CONN*>(connection)->mutexQueryLock.lock();
   DBDRV_RESULT hResult = UnsafeSelect(static_cast<PG_CONN*>(connection), query, errorText);
   static_cast<PG_CONN*>(connection)->mutexQueryLock.unlock();
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

DB_DRIVER_ENTRY_POINT("PGSQL", s_callTable)

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
