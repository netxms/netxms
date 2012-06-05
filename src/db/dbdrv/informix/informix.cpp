/* 
** Informix Database Driver
** Copyright (C) 2010-2012 Raden Solutinos
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
**/

#include "informixdrv.h"

DECLARE_DRIVER_HEADER("INFORMIX")


//
// Constants
//

#define DATA_BUFFER_SIZE      65536


//
// Convert INFORMIX state to NetXMS database error code and get error text
//

static DWORD GetSQLErrorInfo(SQLSMALLINT nHandleType, SQLHANDLE hHandle, WCHAR *errorText)
{
	SQLRETURN nRet;
	SQLSMALLINT nChars;
	DWORD dwError;
	SQLWCHAR sqlState[32];

	// Get state information and convert it to NetXMS database error code
	nRet = SQLGetDiagFieldW(nHandleType, hHandle, 1, SQL_DIAG_SQLSTATE, sqlState, 16, &nChars);
	if (nRet == SQL_SUCCESS)
	{
		if (
			(!wcscmp(sqlState, L"08003")) ||	// Connection does not exist
			(!wcscmp(sqlState, L"08S01")) ||	// Communication link failure
			(!wcscmp(sqlState, L"HYT00")) ||	// Timeout expired
			(!wcscmp(sqlState, L"HYT01")) ||	// Connection timeout expired
			(!wcscmp(sqlState, L"08506")))	// SQL30108N: A connection failed but has been re-established.
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

	return dwError;
}


//
// Clear any pending result sets on given statement
//

static void ClearPendingResults(SQLHSTMT statement)
{
	while(1)
	{
		SQLRETURN rc = SQLMoreResults(statement);
		if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO))
		{
			break;
		}
	}
}


//
// Prepare string for using in SQL query - enclose in quotes and escape as needed
//

extern "C" WCHAR EXPORT *DrvPrepareStringW(const WCHAR *str)
{
	int len = (int)wcslen(str) + 3;   // + two quotes and \0 at the end
	int bufferSize = len + 128;
	WCHAR *out = (WCHAR *)malloc(bufferSize * sizeof(WCHAR));
	out[0] = L'\'';

	const WCHAR *src = str;
	int outPos;
	for(outPos = 1; *src != 0; src++)
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
	for(outPos = 1; *src != 0; src++)
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
// database should be set to Informix source name. Host and schema are ignored
//

extern "C" DBDRV_CONNECTION EXPORT DrvConnect(char *host, char *login, char *password, char *database, const char *schema, WCHAR *errorText)
{
	long iResult;
	INFORMIX_CONN *pConn;

	// Allocate our connection structure
	pConn = (INFORMIX_CONN *)malloc(sizeof(INFORMIX_CONN));

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
	SQLSMALLINT outLen;
	char connectString[1024];
	snprintf(connectString, 1024, "DSN=%s;UID=%s;PWD=%s", database, login, password);
	iResult = SQLDriverConnect(pConn->sqlConn, NULL, (SQLCHAR *)connectString, SQL_NTS, NULL, 0, &outLen, SQL_DRIVER_NOPROMPT);
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


//
// Disconnect from database
//

extern "C" void EXPORT DrvDisconnect(INFORMIX_CONN *pConn)
{
	MutexLock(pConn->mutexQuery);
	MutexUnlock(pConn->mutexQuery);
	SQLDisconnect(pConn->sqlConn);
	SQLFreeHandle(SQL_HANDLE_DBC, pConn->sqlConn);
	SQLFreeHandle(SQL_HANDLE_ENV, pConn->sqlEnv);
	MutexDestroy(pConn->mutexQuery);
	free(pConn);
}

//
// Prepare statement
//

extern "C" DBDRV_STATEMENT EXPORT DrvPrepare(INFORMIX_CONN *pConn, WCHAR *pwszQuery, DWORD *pdwError, WCHAR *errorText)
{
	long iResult;
	SQLHSTMT statement;
	INFORMIX_STATEMENT *result;

	MutexLock(pConn->mutexQuery);

	// Allocate statement handle
	iResult = SQLAllocHandle(SQL_HANDLE_STMT, pConn->sqlConn, &statement);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
	{
		// Prepare statement
		iResult = SQLPrepareW(statement, (SQLWCHAR *)pwszQuery, SQL_NTS);
		if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
		{
			result = (INFORMIX_STATEMENT *)malloc(sizeof(INFORMIX_STATEMENT));
			result->handle = statement;
			result->buffers = new Array(0, 16, true);
			result->connection = pConn;
			*pdwError = DBERR_SUCCESS;
		}
		else
		{
			*pdwError = GetSQLErrorInfo(SQL_HANDLE_STMT, statement, errorText);
			SQLFreeHandle(SQL_HANDLE_STMT, statement);
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


//
// Bind parameter to statement
//

extern "C" void EXPORT DrvBind(INFORMIX_STATEMENT *statement, int pos, int sqlType, int cType, void *buffer, int allocType)
{
	static SQLSMALLINT odbcSqlType[] = { SQL_VARCHAR, SQL_INTEGER, SQL_BIGINT, SQL_DOUBLE, SQL_LONGVARCHAR };
	static SQLSMALLINT odbcCType[] = { SQL_C_WCHAR, SQL_C_SLONG, SQL_C_ULONG, SQL_C_SBIGINT, SQL_C_UBIGINT, SQL_C_DOUBLE };
	static DWORD bufferSize[] = { 0, sizeof(LONG), sizeof(DWORD), sizeof(INT64), sizeof(QWORD), sizeof(double) };

	int length = (int)wcslen((WCHAR *)buffer) + 1;

	SQLPOINTER sqlBuffer;
	switch(allocType)
	{
		case DB_BIND_STATIC:
			sqlBuffer = buffer;
			break;
		case DB_BIND_DYNAMIC:
			sqlBuffer = buffer;
			statement->buffers->add(sqlBuffer);
			break;
		case DB_BIND_TRANSIENT:
			sqlBuffer = nx_memdup(buffer, (cType == DB_CTYPE_STRING) ? (DWORD)(length * sizeof(WCHAR)) : bufferSize[cType]);
			statement->buffers->add(sqlBuffer);
			break;
		default:
			return;	// Invalid call
	}
	SQLBindParameter(statement->handle, pos, SQL_PARAM_INPUT, odbcCType[cType], odbcSqlType[sqlType],
	                 (cType == DB_CTYPE_STRING) ? length : 0, 0, sqlBuffer, 0, NULL);
}


//
// Execute prepared statement
//

extern "C" DWORD EXPORT DrvExecute(INFORMIX_CONN *pConn, INFORMIX_STATEMENT *statement, WCHAR *errorText)
{
	DWORD dwResult;

	MutexLock(pConn->mutexQuery);
	long rc = SQLExecute(statement->handle);
	if (
		(rc == SQL_SUCCESS) ||
		(rc == SQL_SUCCESS_WITH_INFO) ||
		(rc == SQL_NO_DATA))
	{
		ClearPendingResults(statement->handle);
		dwResult = DBERR_SUCCESS;
	}
	else
	{
		dwResult = GetSQLErrorInfo(SQL_HANDLE_STMT, statement->handle, errorText);
	}
	MutexUnlock(pConn->mutexQuery);
	return dwResult;
}


//
// Destroy prepared statement
//

extern "C" void EXPORT DrvFreeStatement(INFORMIX_STATEMENT *statement)
{
	if (statement == NULL)
	{
		return;
	}

	MutexLock(statement->connection->mutexQuery);
	SQLFreeHandle(SQL_HANDLE_STMT, statement->handle);
	MutexUnlock(statement->connection->mutexQuery);
	delete statement->buffers;
	free(statement);
}


//
// Perform non-SELECT query
//

extern "C" DWORD EXPORT DrvQuery(INFORMIX_CONN *pConn, WCHAR *pwszQuery, WCHAR *errorText)
{
	long iResult;
	DWORD dwResult;

	MutexLock(pConn->mutexQuery);

	// Allocate statement handle
	iResult = SQLAllocHandle(SQL_HANDLE_STMT, pConn->sqlConn, &pConn->sqlStatement);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
	{
		// Execute statement
		iResult = SQLExecDirectW(pConn->sqlStatement, (SQLWCHAR *)pwszQuery, SQL_NTS);
		if (
			(iResult == SQL_SUCCESS) || 
			(iResult == SQL_SUCCESS_WITH_INFO) || 
			(iResult == SQL_NO_DATA))
		{
			dwResult = DBERR_SUCCESS;
		}
		else
		{
			dwResult = GetSQLErrorInfo(SQL_HANDLE_STMT, pConn->sqlStatement, errorText);
		}
		SQLFreeHandle(SQL_HANDLE_STMT, pConn->sqlStatement);
	}
	else
	{
		dwResult = GetSQLErrorInfo(SQL_HANDLE_DBC, pConn->sqlConn, errorText);
	}

	MutexUnlock(pConn->mutexQuery);
	return dwResult;
}


//
// Process results of SELECT query
//

static INFORMIX_QUERY_RESULT *ProcessSelectResults(SQLHSTMT statement)
{
	// Allocate result buffer and determine column info
	INFORMIX_QUERY_RESULT *pResult = (INFORMIX_QUERY_RESULT *)malloc(sizeof(INFORMIX_QUERY_RESULT));
	short wNumCols;
	SQLNumResultCols(statement, &wNumCols);
	pResult->iNumCols = wNumCols;
	pResult->iNumRows = 0;
	pResult->pValues = NULL;

	BYTE *pDataBuffer = (BYTE *)malloc(DATA_BUFFER_SIZE * sizeof(wchar_t));

	// Get column names
	pResult->columnNames = (char **)malloc(sizeof(char *) * pResult->iNumCols);
	for(int i = 0; i < (int)pResult->iNumCols; i++)
	{
		char name[256];
		SQLSMALLINT len;

		SQLRETURN iResult = SQLColAttribute(statement, (SQLSMALLINT)(i + 1), SQL_DESC_NAME, (SQLPOINTER)name, 256, &len, NULL); 
		if (
			(iResult == SQL_SUCCESS) || 
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
	while(iResult = SQLFetch(statement), (iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
	{
		pResult->iNumRows++;
		pResult->pValues = (WCHAR **)realloc(pResult->pValues, sizeof(WCHAR *) * (pResult->iNumRows * pResult->iNumCols));
		for(int i = 1; i <= (int)pResult->iNumCols; i++)
		{
			SQLLEN iDataSize;

			pDataBuffer[0] = 0;
			iResult = SQLGetData(statement, (short)i, SQL_C_WCHAR, pDataBuffer, DATA_BUFFER_SIZE, &iDataSize);
			if (iDataSize != SQL_NULL_DATA)
			{
				pResult->pValues[iCurrValue++] = wcsdup((const WCHAR *)pDataBuffer);
			}
			else
			{
				pResult->pValues[iCurrValue++] = wcsdup(L"");
			}
		}
	}

	free(pDataBuffer);
	return pResult;
}


//
// Perform SELECT query
//

extern "C" DBDRV_RESULT EXPORT DrvSelect(INFORMIX_CONN *pConn, WCHAR *pwszQuery, DWORD *pdwError, WCHAR *errorText)
{
	INFORMIX_QUERY_RESULT *pResult = NULL;

	MutexLock(pConn->mutexQuery);

	// Allocate statement handle
	long iResult = SQLAllocHandle(SQL_HANDLE_STMT, pConn->sqlConn, &pConn->sqlStatement);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
	{
		// Execute statement
		iResult = SQLExecDirectW(pConn->sqlStatement, (SQLWCHAR *)pwszQuery, SQL_NTS);
		if (
			(iResult == SQL_SUCCESS) || 
			(iResult == SQL_SUCCESS_WITH_INFO))
		{
			pResult = ProcessSelectResults(pConn->sqlStatement);
			*pdwError = DBERR_SUCCESS;
		}
		else
		{
			*pdwError = GetSQLErrorInfo(SQL_HANDLE_STMT, pConn->sqlStatement, errorText);
		}
		SQLFreeHandle(SQL_HANDLE_STMT, pConn->sqlStatement);
	}
	else
	{
		*pdwError = GetSQLErrorInfo(SQL_HANDLE_DBC, pConn->sqlConn, errorText);
	}

	MutexUnlock(pConn->mutexQuery);
	return pResult;
}


//
// Perform SELECT query using prepared statement
//

extern "C" DBDRV_RESULT EXPORT DrvSelectPrepared(INFORMIX_CONN *pConn, INFORMIX_STATEMENT *statement, DWORD *pdwError, WCHAR *errorText)
{
	INFORMIX_QUERY_RESULT *pResult = NULL;

	MutexLock(pConn->mutexQuery);
	long rc = SQLExecute(statement->handle);
	if (
		(rc == SQL_SUCCESS) ||
		(rc == SQL_SUCCESS_WITH_INFO))
	{
		pResult = ProcessSelectResults(statement->handle);
		*pdwError = DBERR_SUCCESS;
	}
	else
	{
		*pdwError = GetSQLErrorInfo(SQL_HANDLE_STMT, statement->handle, errorText);
	}
	MutexUnlock(pConn->mutexQuery);
	return pResult;
}


//
// Get field length from result
//

extern "C" LONG EXPORT DrvGetFieldLength(INFORMIX_QUERY_RESULT *pResult, int iRow, int iColumn)
{
	LONG nLen = -1;

	if (pResult != NULL)
	{
		if ((iRow < pResult->iNumRows) && (iRow >= 0) &&
				(iColumn < pResult->iNumCols) && (iColumn >= 0))
		{
			nLen = (LONG)wcslen(pResult->pValues[iRow * pResult->iNumCols + iColumn]);
		}
	}
	return nLen;
}


//
// Get field value from result
//

extern "C" WCHAR EXPORT *DrvGetField(INFORMIX_QUERY_RESULT *pResult, int iRow, int iColumn,
		WCHAR *pBuffer, int nBufSize)
{
	WCHAR *pValue = NULL;

	if (pResult != NULL)
	{
		if ((iRow < pResult->iNumRows) && (iRow >= 0) &&
				(iColumn < pResult->iNumCols) && (iColumn >= 0))
		{
#ifdef _WIN32
			wcsncpy_s(pBuffer, nBufSize, pResult->pValues[iRow * pResult->iNumCols + iColumn], _TRUNCATE);
#else
			wcsncpy(pBuffer, pResult->pValues[iRow * pResult->iNumCols + iColumn], nBufSize);
			pBuffer[nBufSize - 1] = 0;
#endif
			pValue = pBuffer;
		}
	}
	return pValue;
}


//
// Get number of rows in result
//

extern "C" int EXPORT DrvGetNumRows(INFORMIX_QUERY_RESULT *pResult)
{
	return (pResult != NULL) ? pResult->iNumRows : 0;
}


//
// Get column count in query result
//

extern "C" int EXPORT DrvGetColumnCount(INFORMIX_QUERY_RESULT *pResult)
{
	return (pResult != NULL) ? pResult->iNumCols : 0;
}


//
// Get column name in query result
//

extern "C" const char EXPORT *DrvGetColumnName(INFORMIX_QUERY_RESULT *pResult, int column)
{
	return ((pResult != NULL) && (column >= 0) && (column < pResult->iNumCols)) ? pResult->columnNames[column] : NULL;
}


//
// Free SELECT results
//

extern "C" void EXPORT DrvFreeResult(INFORMIX_QUERY_RESULT *pResult)
{
	if (pResult != NULL)
	{
		int i, iNumValues;

		iNumValues = pResult->iNumCols * pResult->iNumRows;
		for(i = 0; i < iNumValues; i++)
		{
			safe_free(pResult->pValues[i]);
		}
		safe_free(pResult->pValues);

		for(i = 0; i < pResult->iNumCols; i++)
		{
			safe_free(pResult->columnNames[i]);
		}
		safe_free(pResult->columnNames);

		free(pResult);
	}
}


//
// Perform asynchronous SELECT query
//

extern "C" DBDRV_ASYNC_RESULT EXPORT DrvAsyncSelect(INFORMIX_CONN *pConn, WCHAR *pwszQuery,
		DWORD *pdwError, WCHAR *errorText)
{
	INFORMIX_ASYNC_QUERY_RESULT *pResult = NULL;
	long iResult;
	short wNumCols;
	int i;

	MutexLock(pConn->mutexQuery);

	// Allocate statement handle
	iResult = SQLAllocHandle(SQL_HANDLE_STMT, pConn->sqlConn, &pConn->sqlStatement);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
	{
		// Execute statement
		iResult = SQLExecDirectW(pConn->sqlStatement, (SQLWCHAR *)pwszQuery, SQL_NTS);
		if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
		{
			// Allocate result buffer and determine column info
			pResult = (INFORMIX_ASYNC_QUERY_RESULT *)malloc(sizeof(INFORMIX_ASYNC_QUERY_RESULT));
			SQLNumResultCols(pConn->sqlStatement, &wNumCols);
			pResult->iNumCols = wNumCols;
			pResult->pConn = pConn;
			pResult->bNoMoreRows = FALSE;

			// Get column names
			pResult->columnNames = (char **)malloc(sizeof(char *) * pResult->iNumCols);
			for(i = 0; i < pResult->iNumCols; i++)
			{
				SQLWCHAR name[256];
				SQLSMALLINT len;

				iResult = SQLColAttributeW(pConn->sqlStatement, (SQLSMALLINT)(i + 1),
						SQL_DESC_NAME, name, 256, &len, NULL); 
				if (
					(iResult == SQL_SUCCESS) || 
					(iResult == SQL_SUCCESS_WITH_INFO))
				{
					name[len] = 0;
					pResult->columnNames[i] = MBStringFromWideString(name);
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
			*pdwError = GetSQLErrorInfo(SQL_HANDLE_STMT, pConn->sqlStatement, errorText);
			// Free statement handle if query failed
			SQLFreeHandle(SQL_HANDLE_STMT, pConn->sqlStatement);
		}
	}
	else
	{
		*pdwError = GetSQLErrorInfo(SQL_HANDLE_DBC, pConn->sqlConn, errorText);
	}

	if (pResult == NULL) // Unlock mutex if query has failed
	{
		MutexUnlock(pConn->mutexQuery);
	}
	return pResult;
}


//
// Fetch next result line from asynchronous SELECT results
//

extern "C" BOOL EXPORT DrvFetch(INFORMIX_ASYNC_QUERY_RESULT *pResult)
{
	BOOL bResult = FALSE;

	if (pResult != NULL)
	{
		long iResult;

		iResult = SQLFetch(pResult->pConn->sqlStatement);
		bResult = ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO));
		if (!bResult)
		{
			pResult->bNoMoreRows = TRUE;
		}
	}
	return bResult;
}


//
// Get field length from async query result
//

extern "C" LONG EXPORT DrvGetFieldLengthAsync(INFORMIX_ASYNC_QUERY_RESULT *pResult, int iColumn)
{
	LONG nLen = -1;

	if (pResult != NULL)
	{
		if ((iColumn < pResult->iNumCols) && (iColumn >= 0))
		{
			SQLLEN dataSize;
			char temp[1];
			long rc = SQLGetData(pResult->pConn->sqlStatement, (short)iColumn + 1, SQL_C_CHAR, temp, 0, &dataSize);
			if ((rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO))
			{
				nLen = (LONG)dataSize;
			}
		}
	}
	return nLen;
}


//
// Get field from current row in async query result
//

extern "C" WCHAR EXPORT *DrvGetFieldAsync(INFORMIX_ASYNC_QUERY_RESULT *pResult,
		int iColumn, WCHAR *pBuffer, int iBufSize)
{
	SQLLEN iDataSize;
	long iResult;

	// Check if we have valid result handle
	if (pResult == NULL)
	{
		return NULL;
	}

	// Check if there are valid fetched row
	if (pResult->bNoMoreRows)
	{
		return NULL;
	}

	if ((iColumn >= 0) && (iColumn < pResult->iNumCols))
	{
		// At least on HP-UX driver expects length in chars, not bytes
		// otherwise it crashes
		// TODO: check other platforms
		iResult = SQLGetData(pResult->pConn->sqlStatement, (short)iColumn + 1, SQL_C_WCHAR, pBuffer, iBufSize, &iDataSize);
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


//
// Get column count in async query result
//

extern "C" int EXPORT DrvGetColumnCountAsync(INFORMIX_ASYNC_QUERY_RESULT *pResult)
{
	return (pResult != NULL) ? pResult->iNumCols : 0;
}


//
// Get column name in async query result
//

extern "C" const char EXPORT *DrvGetColumnNameAsync(INFORMIX_ASYNC_QUERY_RESULT *pResult, int column)
{
	return ((pResult != NULL) && (column >= 0) && (column < pResult->iNumCols)) ? pResult->columnNames[column] : NULL;
}


//
// Destroy result of async query
//

extern "C" void EXPORT DrvFreeAsyncResult(INFORMIX_ASYNC_QUERY_RESULT *pResult)
{
	if (pResult != NULL)
	{
		SQLFreeHandle(SQL_HANDLE_STMT, pResult->pConn->sqlStatement);
		MutexUnlock(pResult->pConn->mutexQuery);
		free(pResult);
	}
}


//
// Begin transaction
//

extern "C" DWORD EXPORT DrvBegin(INFORMIX_CONN *pConn)
{
	SQLRETURN nRet;
	DWORD dwResult;

	if (pConn == NULL)
	{
		return DBERR_INVALID_HANDLE;
	}

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


//
// Commit transaction
//

extern "C" DWORD EXPORT DrvCommit(INFORMIX_CONN *pConn)
{
	SQLRETURN nRet;

	if (pConn == NULL)
	{
		return DBERR_INVALID_HANDLE;
	}

	MutexLock(pConn->mutexQuery);
	nRet = SQLEndTran(SQL_HANDLE_DBC, pConn->sqlConn, SQL_COMMIT);
	SQLSetConnectAttr(pConn->sqlConn, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
	MutexUnlock(pConn->mutexQuery);
	return ((nRet == SQL_SUCCESS) || (nRet == SQL_SUCCESS_WITH_INFO)) ? DBERR_SUCCESS : DBERR_OTHER_ERROR;
}


//
// Rollback transaction
//

extern "C" DWORD EXPORT DrvRollback(INFORMIX_CONN *pConn)
{
	SQLRETURN nRet;

	if (pConn == NULL)
	{
		return DBERR_INVALID_HANDLE;
	}

	MutexLock(pConn->mutexQuery);
	nRet = SQLEndTran(SQL_HANDLE_DBC, pConn->sqlConn, SQL_ROLLBACK);
	SQLSetConnectAttr(pConn->sqlConn, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
	MutexUnlock(pConn->mutexQuery);
	return ((nRet == SQL_SUCCESS) || (nRet == SQL_SUCCESS_WITH_INFO)) ? DBERR_SUCCESS : DBERR_OTHER_ERROR;
}


//
// DLL Entry point
//

#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hInstance);
	}
	return TRUE;
}

#endif
