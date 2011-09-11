/* 
** MySQL Database Driver
** Copyright (C) 2003-2011 Victor Kirhenshtein
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


//
// Update error message from given source
//

static void UpdateErrorMessage(const char *source, WCHAR *errorText)
{
	if (errorText != NULL)
	{
		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, source, -1, errorText, DBDRV_MAX_ERROR_TEXT);
		errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
		RemoveTrailingCRLFW(errorText);
	}
}


//
// Prepare string for using in SQL query - enclose in quotes and escape as needed
//

#define UPDATE_LENGTH \
				len++; \
				if (len >= bufferSize - 1) \
				{ \
					bufferSize += 128; \
					out = (WCHAR *)realloc(out, bufferSize * sizeof(WCHAR)); \
				}

extern "C" WCHAR EXPORT *DrvPrepareStringW(const WCHAR *str)
{
	int len = (int)wcslen(str) + 3;   // + two quotes and \0 at the end
	int bufferSize = len + 128;
	WCHAR *out = (WCHAR *)malloc(bufferSize * sizeof(WCHAR));
	out[0] = _T('\'');

	const WCHAR *src = str;
	int outPos;
	for(outPos = 1; *src != NULL; src++)
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
#define UPDATE_LENGTH \
				len++; \
				if (len >= bufferSize - 1) \
				{ \
					bufferSize += 128; \
					out = (char *)realloc(out, bufferSize); \
				}

extern "C" char EXPORT *DrvPrepareStringA(const char *str)
{
	int len = (int)strlen(str) + 3;   // + two quotes and \0 at the end
	int bufferSize = len + 128;
	char *out = (char *)malloc(bufferSize);
	out[0] = _T('\'');

	const char *src = str;
	int outPos;
	for(outPos = 1; *src != NULL; src++)
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


//
// Initialize driver
//

extern "C" BOOL EXPORT DrvInit(const char *cmdLine)
{
	return TRUE;
}


//
// Unload handler
//

extern "C" void EXPORT DrvUnload()
{
}


//
// Connect to database
//

extern "C" DBDRV_CONNECTION EXPORT DrvConnect(const char *szHost, const char *szLogin, const char *szPassword,
															 const char *szDatabase, const char *schema, WCHAR *errorText)
{
	MYSQL *pMySQL;
	MYSQL_CONN *pConn;
	const char *pHost = szHost;
	const char *pSocket = NULL;
	
	pMySQL = mysql_init(NULL);
	if (pMySQL == NULL)
	{
		wcscpy(errorText, L"Insufficient memory to allocate connection handle");
		return NULL;
	}

	pSocket = strstr(szHost, "socket:");
	if (pSocket != NULL)
	{
		pHost = NULL;
		pSocket += 7;
	}

	if (!mysql_real_connect(
		pMySQL, // MYSQL *
		pHost, // host
		szLogin[0] == 0 ? NULL : szLogin, // user
		(szPassword[0] == 0 || szLogin[0] == 0) ? NULL : szPassword, // pass
		szDatabase, // DB Name
		0, // use default port
		pSocket, // char * - unix socket
		0 // flags
		))
	{
		UpdateErrorMessage(mysql_error(pMySQL), errorText);
		mysql_close(pMySQL);
		return NULL;
	}
	
	pConn = (MYSQL_CONN *)malloc(sizeof(MYSQL_CONN));
	pConn->pMySQL = pMySQL;
	pConn->mutexQueryLock = MutexCreate();

   // Switch to UTF-8 encoding
   mysql_set_character_set(pMySQL, "utf8");
	
	return (DBDRV_CONNECTION)pConn;
}


//
// Disconnect from database
//

extern "C" void EXPORT DrvDisconnect(MYSQL_CONN *pConn)
{
	if (pConn != NULL)
	{
		mysql_close(pConn->pMySQL);
		MutexDestroy(pConn->mutexQueryLock);
		free(pConn);
	}
}


//
// Prepare statement
//

extern "C" DBDRV_STATEMENT EXPORT DrvPrepare(MYSQL_CONN *pConn, WCHAR *pwszQuery, WCHAR *errorText)
{
	MYSQL_STATEMENT *result = NULL;

	MutexLock(pConn->mutexQueryLock, INFINITE);
	MYSQL_STMT *stmt = mysql_stmt_init(pConn->pMySQL);
	if (stmt != NULL)
	{
		char *pszQueryUTF8 = UTF8StringFromWideString(pwszQuery);
		int rc = mysql_stmt_prepare(stmt, pszQueryUTF8, strlen(pszQueryUTF8));
		if (rc == 0)
		{
			result = (MYSQL_STATEMENT *)malloc(sizeof(MYSQL_STATEMENT));
			result->statement = stmt;
			result->paramCount = (int)mysql_stmt_param_count(stmt);
			result->bindings = (MYSQL_BIND *)malloc(sizeof(MYSQL_BIND) * result->paramCount);
			memset(result->bindings, 0, sizeof(MYSQL_BIND) * result->paramCount);
			result->lengthFields = (unsigned long *)malloc(sizeof(unsigned long) * result->paramCount);
			memset(result->lengthFields, 0, sizeof(unsigned long) * result->paramCount);
			result->buffers = new Array(result->paramCount, 16, true);
		}
		else
		{
			UpdateErrorMessage(mysql_stmt_error(stmt), errorText);
			mysql_stmt_close(stmt);
		}
	}
	else
	{
		UpdateErrorMessage("Call to mysql_stmt_init failed", errorText);
	}
	MutexUnlock(pConn->mutexQueryLock);
	return result;
}


//
// Bind parameter to prepared statement
//

extern "C" void EXPORT DrvBind(MYSQL_STATEMENT *hStmt, int pos, int sqlType, int cType, void *buffer, int allocType)
{
	static DWORD bufferSize[] = { 0, sizeof(LONG), sizeof(DWORD), sizeof(INT64), sizeof(QWORD), sizeof(double) };

	if (pos >= hStmt->paramCount)
		return;
	MYSQL_BIND *b = &hStmt->bindings[pos - 1];

	if (cType == DB_CTYPE_STRING)
	{
		b->buffer = UTF8StringFromWideString((WCHAR *)buffer);
		hStmt->buffers->add(b->buffer);
		if (allocType == DB_BIND_DYNAMIC)
			free(buffer);
		b->buffer_length = strlen((char *)b->buffer) + 1;
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
				b->buffer = nx_memdup(buffer, bufferSize[cType]);
				hStmt->buffers->add(b->buffer);
				break;
			default:
				return;	// Invalid call
		}

		switch(cType)
		{
			case DB_CTYPE_UINT32:
				b->is_unsigned = TRUE;
			case DB_CTYPE_INT32:
				b->buffer_type = MYSQL_TYPE_LONG;
				break;
			case DB_CTYPE_UINT64:
				b->is_unsigned = TRUE;
			case DB_CTYPE_INT64:
				b->buffer_type = MYSQL_TYPE_LONGLONG;
				break;
			case DB_CTYPE_DOUBLE:
				b->buffer_type = MYSQL_TYPE_DOUBLE;
				break;
		}
	}
}


//
// Execute prepared statement
//

extern "C" DWORD EXPORT DrvExecute(MYSQL_CONN *pConn, MYSQL_STATEMENT *hStmt, WCHAR *errorText)
{
	DWORD dwResult;

	MutexLock(pConn->mutexQueryLock, INFINITE);

	if (mysql_stmt_bind_param(hStmt->statement, hStmt->bindings) == 0)
	{
		if (mysql_stmt_execute(hStmt->statement) == 0)
		{
			dwResult = DBERR_SUCCESS;
		}
		else
		{
			int nErr = mysql_errno(pConn->pMySQL);
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

	MutexUnlock(pConn->mutexQueryLock);
	return dwResult;
}


//
// Destroy prepared statement
//

extern "C" void EXPORT DrvFreeStatement(MYSQL_STATEMENT *hStmt)
{
	if (hStmt == NULL)
		return;

	mysql_stmt_close(hStmt->statement);
	delete hStmt->buffers;
	safe_free(hStmt->bindings);
	safe_free(hStmt->lengthFields);
	free(hStmt);
}


//
// Perform actual non-SELECT query
//

static DWORD DrvQueryInternal(MYSQL_CONN *pConn, const char *pszQuery, WCHAR *errorText)
{
	DWORD dwRet = DBERR_INVALID_HANDLE;

	MutexLock(pConn->mutexQueryLock, INFINITE);
	if (mysql_query(pConn->pMySQL, pszQuery) == 0)
	{
		dwRet = DBERR_SUCCESS;
		if (errorText != NULL)
			*errorText = 0;
	}
	else
	{
		int nErr = mysql_errno(pConn->pMySQL);
		if (nErr == CR_SERVER_LOST || nErr == CR_CONNECTION_ERROR || nErr == CR_SERVER_GONE_ERROR) // CR_SERVER_GONE_ERROR - ???
		{
			dwRet = DBERR_CONNECTION_LOST;
		}
		else
		{
			dwRet = DBERR_OTHER_ERROR;
		}
		UpdateErrorMessage(mysql_error(pConn->pMySQL), errorText);
	}

	MutexUnlock(pConn->mutexQueryLock);
	return dwRet;
}


//
// Perform non-SELECT query
//

extern "C" DWORD EXPORT DrvQuery(MYSQL_CONN *pConn, WCHAR *pwszQuery, WCHAR *errorText)
{
	DWORD dwRet;
   char *pszQueryUTF8;

   pszQueryUTF8 = UTF8StringFromWideString(pwszQuery);
   dwRet = DrvQueryInternal(pConn, pszQueryUTF8, errorText);
   free(pszQueryUTF8);
	return dwRet;
}


//
// Perform SELECT query
//

extern "C" DBDRV_RESULT EXPORT DrvSelect(MYSQL_CONN *pConn, WCHAR *pwszQuery, DWORD *pdwError, WCHAR *errorText)
{
	MYSQL_RESULT *result = NULL;
	char *pszQueryUTF8;

	if (pConn == NULL)
	{
		*pdwError = DBERR_INVALID_HANDLE;
		return NULL;
	}

	pszQueryUTF8 = UTF8StringFromWideString(pwszQuery);
	MutexLock(pConn->mutexQueryLock, INFINITE);
	if (mysql_query(pConn->pMySQL, pszQueryUTF8) == 0)
	{
		result = (MYSQL_RESULT *)malloc(sizeof(MYSQL_RESULT));
		result->isPreparedStatement = false;
		result->resultSet = mysql_store_result(pConn->pMySQL);
		*pdwError = DBERR_SUCCESS;
		if (errorText != NULL)
			*errorText = 0;
	}
	else
	{
		int nErr = mysql_errno(pConn->pMySQL);
		if (nErr == CR_SERVER_LOST || nErr == CR_CONNECTION_ERROR || nErr == CR_SERVER_GONE_ERROR) // CR_SERVER_GONE_ERROR - ???
		{
			*pdwError = DBERR_CONNECTION_LOST;
		}
		else
		{
			*pdwError = DBERR_OTHER_ERROR;
		}
		UpdateErrorMessage(mysql_error(pConn->pMySQL), errorText);
	}

	MutexUnlock(pConn->mutexQueryLock);
	free(pszQueryUTF8);
	return result;
}


//
// Perform SELECT query using prepared statement
//

extern "C" DBDRV_RESULT EXPORT DrvSelectPrepared(MYSQL_CONN *pConn, MYSQL_STATEMENT *hStmt, DWORD *pdwError, WCHAR *errorText)
{
	MYSQL_RESULT *result = NULL;

	if (pConn == NULL)
	{
		*pdwError = DBERR_INVALID_HANDLE;
		return NULL;
	}

	MutexLock(pConn->mutexQueryLock, INFINITE);

	if (mysql_stmt_bind_param(hStmt->statement, hStmt->bindings) == 0)
	{
		if (mysql_stmt_execute(hStmt->statement) == 0)
		{
			result = (MYSQL_RESULT *)malloc(sizeof(MYSQL_RESULT));
			result->isPreparedStatement = true;
			result->resultSet = mysql_stmt_param_metadata(hStmt->statement);
			if (result->resultSet != NULL)
			{
			}
			else
			{
				UpdateErrorMessage(mysql_stmt_error(hStmt->statement), errorText);
				*pdwError = DBERR_OTHER_ERROR;
				free(result);
				result = NULL;
			}
		}
		else
		{
			int nErr = mysql_errno(pConn->pMySQL);
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

	MutexUnlock(pConn->mutexQueryLock);
	return result;
}


//
// Get field length from result
//

extern "C" LONG EXPORT DrvGetFieldLength(MYSQL_RESULT *hResult, int iRow, int iColumn)
{
	MYSQL_ROW row;
	
	mysql_data_seek(hResult->resultSet, iRow);
	row = mysql_fetch_row(hResult->resultSet);
	return (row == NULL) ? (LONG)-1 : ((row[iColumn] == NULL) ? -1 : (LONG)strlen(row[iColumn]));
}


//
// Get field value from result
//

extern "C" WCHAR EXPORT *DrvGetField(MYSQL_RESULT *hResult, int iRow, int iColumn, WCHAR *pBuffer, int nBufSize)
{
	MYSQL_ROW row;
	WCHAR *pRet = NULL;
	
	mysql_data_seek(hResult->resultSet, iRow);
	row = mysql_fetch_row(hResult->resultSet);
	if (row != NULL)
	{
		if (row[iColumn] != NULL)
		{
			MultiByteToWideChar(CP_UTF8, 0, row[iColumn], -1, pBuffer, nBufSize);
			pBuffer[nBufSize - 1] = 0;
			pRet = pBuffer;
		}
	}
	return pRet;
}


//
// Get number of rows in result
//

extern "C" int EXPORT DrvGetNumRows(MYSQL_RESULT *hResult)
{
	return (hResult != NULL) ? (int)mysql_num_rows(hResult->resultSet) : 0;
}


//
// Get column count in query result
//

extern "C" int EXPORT DrvGetColumnCount(MYSQL_RESULT *hResult)
{
	return (hResult != NULL) ? (int)mysql_num_fields(hResult->resultSet) : 0;
}


//
// Get column name in query result
//

extern "C" const char EXPORT *DrvGetColumnName(MYSQL_RESULT *hResult, int column)
{
	MYSQL_FIELD *field;

	if (hResult == NULL)
		return NULL;

	field = mysql_fetch_field_direct(hResult->resultSet, column);
	return (field != NULL) ? field->name : NULL;
}


//
// Free SELECT results
//

extern "C" void EXPORT DrvFreeResult(MYSQL_RESULT *hResult)
{
	if (hResult == NULL)
		return;

	mysql_free_result(hResult->resultSet);
	free(hResult);
}


//
// Perform asynchronous SELECT query
//

extern "C" DBDRV_ASYNC_RESULT EXPORT DrvAsyncSelect(MYSQL_CONN *pConn, WCHAR *pwszQuery,
                                                 DWORD *pdwError, WCHAR *errorText)
{
	MYSQL_ASYNC_RESULT *pResult = NULL;
	char *pszQueryUTF8;
	
	if (pConn == NULL)
	{
		*pdwError = DBERR_INVALID_HANDLE;
		return NULL;
	}

	pszQueryUTF8 = UTF8StringFromWideString(pwszQuery);
	MutexLock(pConn->mutexQueryLock, INFINITE);
	if (mysql_query(pConn->pMySQL, pszQueryUTF8) == 0)
	{
		pResult = (MYSQL_ASYNC_RESULT *)malloc(sizeof(MYSQL_ASYNC_RESULT));
		pResult->pConn = pConn;
		pResult->pHandle = mysql_use_result(pConn->pMySQL);
		if (pResult->pHandle != NULL)
		{
			pResult->bNoMoreRows = FALSE;
			pResult->iNumCols = mysql_num_fields(pResult->pHandle);
			pResult->pCurrRow = NULL;
			pResult->pulColLengths = (unsigned long *)malloc(sizeof(unsigned long) * pResult->iNumCols);
		}
		else
		{
			free(pResult);
			pResult = NULL;
		}

		*pdwError = DBERR_SUCCESS;
		if (errorText != NULL)
			*errorText = 0;
	}
	else
	{
		int nErr = mysql_errno(pConn->pMySQL);
		if (nErr == CR_SERVER_LOST || nErr == CR_CONNECTION_ERROR || nErr == CR_SERVER_GONE_ERROR) // CR_SERVER_GONE_ERROR - ???
		{
			*pdwError = DBERR_CONNECTION_LOST;
		}
		else
		{
			*pdwError = DBERR_OTHER_ERROR;
		}
		
		if (errorText != NULL)
		{
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, mysql_error(pConn->pMySQL), -1, errorText, DBDRV_MAX_ERROR_TEXT);
			errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
			RemoveTrailingCRLFW(errorText);
		}
	}

	if (pResult == NULL)
	{
		MutexUnlock(pConn->mutexQueryLock);
	}
	free(pszQueryUTF8);

	return pResult;
}


//
// Fetch next result line from asynchronous SELECT results
//

extern "C" BOOL EXPORT DrvFetch(DBDRV_ASYNC_RESULT hResult)
{
	BOOL bResult = TRUE;
	
	if (hResult == NULL)
	{
		bResult = FALSE;
	}
	else
	{
		// Try to fetch next row from server
		((MYSQL_ASYNC_RESULT *)hResult)->pCurrRow = mysql_fetch_row(((MYSQL_ASYNC_RESULT *)hResult)->pHandle);
		if (((MYSQL_ASYNC_RESULT *)hResult)->pCurrRow == NULL)
		{
			((MYSQL_ASYNC_RESULT *)hResult)->bNoMoreRows = TRUE;
			bResult = FALSE;
			MutexUnlock(((MYSQL_ASYNC_RESULT *)hResult)->pConn->mutexQueryLock);
		}
		else
		{
			unsigned long *pLen;
			
			// Get column lengths for current row
			pLen = mysql_fetch_lengths(((MYSQL_ASYNC_RESULT *)hResult)->pHandle);
			if (pLen != NULL)
			{
				memcpy(((MYSQL_ASYNC_RESULT *)hResult)->pulColLengths, pLen, 
					sizeof(unsigned long) * ((MYSQL_ASYNC_RESULT *)hResult)->iNumCols);
			}
			else
			{
				memset(((MYSQL_ASYNC_RESULT *)hResult)->pulColLengths, 0, 
					sizeof(unsigned long) * ((MYSQL_ASYNC_RESULT *)hResult)->iNumCols);
			}
		}
	}
	return bResult;
}


//
// Get field length from async query result result
//

extern "C" LONG EXPORT DrvGetFieldLengthAsync(DBDRV_ASYNC_RESULT hResult, int iColumn)
{
	// Check if we have valid result handle
	if (hResult == NULL)
		return 0;
	
	// Check if there are valid fetched row
	if ((((MYSQL_ASYNC_RESULT *)hResult)->bNoMoreRows) ||
		(((MYSQL_ASYNC_RESULT *)hResult)->pCurrRow == NULL))
		return 0;
	
	// Check if column number is valid
	if ((iColumn < 0) || (iColumn >= ((MYSQL_ASYNC_RESULT *)hResult)->iNumCols))
		return 0;

	return ((MYSQL_ASYNC_RESULT *)hResult)->pulColLengths[iColumn];
}


//
// Get field from current row in async query result
//

extern "C" WCHAR EXPORT *DrvGetFieldAsync(DBDRV_ASYNC_RESULT hResult, int iColumn,
                                          WCHAR *pBuffer, int iBufSize)
{
	int iLen;
	
	// Check if we have valid result handle
	if (hResult == NULL)
		return NULL;
	
	// Check if there are valid fetched row
	if ((((MYSQL_ASYNC_RESULT *)hResult)->bNoMoreRows) ||
		(((MYSQL_ASYNC_RESULT *)hResult)->pCurrRow == NULL))
		return NULL;
	
	// Check if column number is valid
	if ((iColumn < 0) || (iColumn >= ((MYSQL_ASYNC_RESULT *)hResult)->iNumCols))
		return NULL;
	
	// Now get column data
	iLen = min((int)((MYSQL_ASYNC_RESULT *)hResult)->pulColLengths[iColumn], iBufSize - 1);
	if (iLen > 0)
	{
		MultiByteToWideChar(CP_UTF8, 0, ((MYSQL_ASYNC_RESULT *)hResult)->pCurrRow[iColumn],
		                    iLen, pBuffer, iBufSize);
	}
	pBuffer[iLen] = 0;
	
	return pBuffer;
}


//
// Get column count in async query result
//

extern "C" int EXPORT DrvGetColumnCountAsync(DBDRV_ASYNC_RESULT hResult)
{
	return ((hResult != NULL) && (((MYSQL_ASYNC_RESULT *)hResult)->pHandle != NULL))? (int)mysql_num_fields(((MYSQL_ASYNC_RESULT *)hResult)->pHandle) : 0;
}


//
// Get column name in async query result
//

extern "C" const char EXPORT *DrvGetColumnNameAsync(DBDRV_ASYNC_RESULT hResult, int column)
{
	MYSQL_FIELD *field;

	if ((hResult == NULL) || (((MYSQL_ASYNC_RESULT *)hResult)->pHandle == NULL))
		return NULL;

	field = mysql_fetch_field_direct(((MYSQL_ASYNC_RESULT *)hResult)->pHandle, column);
	return (field != NULL) ? field->name : NULL;
}


//
// Destroy result of async query
//

extern "C" void EXPORT DrvFreeAsyncResult(DBDRV_ASYNC_RESULT hResult)
{
	if (hResult != NULL)
	{
		// Check if all result rows fetched
		if (!((MYSQL_ASYNC_RESULT *)hResult)->bNoMoreRows)
		{
			// Fetch remaining rows
			while(mysql_fetch_row(((MYSQL_ASYNC_RESULT *)hResult)->pHandle) != NULL);
			
			// Now we are ready for next query, so unlock query mutex
			MutexUnlock(((MYSQL_ASYNC_RESULT *)hResult)->pConn->mutexQueryLock);
		}
		
		// Free allocated memory
		mysql_free_result(((MYSQL_ASYNC_RESULT *)hResult)->pHandle);
		safe_free(((MYSQL_ASYNC_RESULT *)hResult)->pulColLengths);
		free(hResult);
	}
}


//
// Begin transaction
//

extern "C" DWORD EXPORT DrvBegin(MYSQL_CONN *pConn)
{
	return DrvQueryInternal(pConn, "BEGIN", NULL);
}


//
// Commit transaction
//

extern "C" DWORD EXPORT DrvCommit(MYSQL_CONN *pConn)
{
	return DrvQueryInternal(pConn, "COMMIT", NULL);
}


//
// Rollback transaction
//

extern "C" DWORD EXPORT DrvRollback(MYSQL_CONN *pConn)
{
	return DrvQueryInternal(pConn, "ROLLBACK", NULL);
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
