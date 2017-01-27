/* 
** DB2 Database Driver
** Copyright (C) 2010-2017 Raden Solutinos
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

#define _CRT_SECURE_NO_WARNINGS

#include "db2drv.h"

DECLARE_DRIVER_HEADER("DB2")

/**
 * Convert DB2 state to NetXMS database error code and get error text
 */
static DWORD GetSQLErrorInfo(SQLSMALLINT nHandleType, SQLHANDLE hHandle, NETXMS_WCHAR *errorText)
{
	SQLRETURN nRet;
	SQLSMALLINT nChars;
	DWORD dwError;
	SQLWCHAR buffer[DBDRV_MAX_ERROR_TEXT];

	// Get state information and convert it to NetXMS database error code
	nRet = SQLGetDiagFieldW(nHandleType, hHandle, 1, SQL_DIAG_SQLSTATE, buffer, 16, &nChars);
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
		{
			break;
		}
	}
}

/**
 * Prepare string for using in SQL query - enclose in quotes and escape as needed
 */
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

/**
 * Initialize driver
 */
extern "C" bool EXPORT DrvInit(const char *cmdLine)
{
	return true;
}

/**
 * Unload handler
 */
extern "C" void EXPORT DrvUnload()
{
}

/**
 * Connect to database
 * pszHost should be set to DB2 source name, and pszDatabase is ignored
 */
extern "C" DBDRV_CONNECTION EXPORT DrvConnect(char *pszHost, char *pszLogin,
                                              char *pszPassword, char *pszDatabase, const char *schema, NETXMS_WCHAR *errorText)
{
	long iResult;
	DB2DRV_CONN *pConn;
   SQLHSTMT sqlStatement;

	// Allocate our connection structure
	pConn = (DB2DRV_CONN *)malloc(sizeof(DB2DRV_CONN));

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
	pConn->mutexQuery = MutexCreate();

	// Success
	return (DBDRV_CONNECTION)pConn;

	// Handle failures
connect_failure_3:
	SQLFreeHandle(SQL_HANDLE_STMT, sqlStatement);

connect_failure_2:
	SQLFreeHandle(SQL_HANDLE_DBC, pConn->sqlConn);

connect_failure_1:
	SQLFreeHandle(SQL_HANDLE_ENV, pConn->sqlEnv);

connect_failure_0:
	free(pConn);
	return NULL;
}

/**
 * Disconnect from database
 */
extern "C" void EXPORT DrvDisconnect(DB2DRV_CONN *pConn)
{
	MutexLock(pConn->mutexQuery);
	MutexUnlock(pConn->mutexQuery);
	SQLDisconnect(pConn->sqlConn);
	SQLFreeHandle(SQL_HANDLE_DBC, pConn->sqlConn);
	SQLFreeHandle(SQL_HANDLE_ENV, pConn->sqlEnv);
	MutexDestroy(pConn->mutexQuery);
	free(pConn);
}

/**
 * Prepare statement
 */
extern "C" DBDRV_STATEMENT EXPORT DrvPrepare(DB2DRV_CONN *pConn, NETXMS_WCHAR *pwszQuery, DWORD *pdwError, NETXMS_WCHAR *errorText)
{
	long iResult;
	SQLHSTMT statement;
	DB2DRV_STATEMENT *result;

	MutexLock(pConn->mutexQuery);

	// Allocate statement handle
	iResult = SQLAllocHandle(SQL_HANDLE_STMT, pConn->sqlConn, &statement);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
	{
		// Prepare statement
#if defined(_WIN32) || defined(UNICODE_UCS2)
		iResult = SQLPrepareW(statement, (SQLWCHAR *)pwszQuery, SQL_NTS);
#else
		SQLWCHAR *temp = UCS2StringFromUCS4String(pwszQuery);
		iResult = SQLPrepareW(statement, temp, SQL_NTS);
		free(temp);
#endif
		if ((iResult == SQL_SUCCESS) ||
				(iResult == SQL_SUCCESS_WITH_INFO))
		{
			result = (DB2DRV_STATEMENT *)malloc(sizeof(DB2DRV_STATEMENT));
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

/**
 * Bind parameter to statement
 */
extern "C" void EXPORT DrvBind(DB2DRV_STATEMENT *statement, int pos, int sqlType, int cType, void *buffer, int allocType)
{
	static SQLSMALLINT odbcSqlType[] = { SQL_VARCHAR, SQL_INTEGER, SQL_BIGINT, SQL_DOUBLE, SQL_LONGVARCHAR };
	static SQLSMALLINT odbcCType[] = { SQL_C_WCHAR, SQL_C_SLONG, SQL_C_ULONG, SQL_C_SBIGINT, SQL_C_UBIGINT, SQL_C_DOUBLE, SQL_C_WCHAR };
	static DWORD bufferSize[] = { 0, sizeof(LONG), sizeof(DWORD), sizeof(INT64), sizeof(QWORD), sizeof(double), 0 };

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
            sqlBuffer = nx_memdup(buffer, (cType == DB_CTYPE_STRING) ? (DWORD)(length * sizeof(WCHAR)) : bufferSize[cType]);
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
				sqlBuffer = nx_memdup(buffer, bufferSize[cType]);
			}
#endif
			statement->buffers->add(sqlBuffer);
			break;
		default:
			return;	// Invalid call
	}
	SQLBindParameter(statement->handle, pos, SQL_PARAM_INPUT, odbcCType[cType], odbcSqlType[sqlType],
	                 ((cType == DB_CTYPE_STRING) || (cType == DB_CTYPE_UTF8_STRING)) ? length : 0, 0, sqlBuffer, 0, NULL);
}

/**
 * Execute prepared statement
 */
extern "C" DWORD EXPORT DrvExecute(DB2DRV_CONN *pConn, DB2DRV_STATEMENT *statement, NETXMS_WCHAR *errorText)
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

extern "C" void EXPORT DrvFreeStatement(DB2DRV_STATEMENT *statement)
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

/**
 * Perform non-SELECT query
 */
extern "C" DWORD EXPORT DrvQuery(DB2DRV_CONN *pConn, NETXMS_WCHAR *pwszQuery, NETXMS_WCHAR *errorText)
{
	long iResult;
	DWORD dwResult;

	MutexLock(pConn->mutexQuery);

	// Allocate statement handle
   SQLHSTMT sqlStatement;
	iResult = SQLAllocHandle(SQL_HANDLE_STMT, pConn->sqlConn, &sqlStatement);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
	{
		// Execute statement
#if defined(_WIN32) || defined(UNICODE_UCS2)
		iResult = SQLExecDirectW(sqlStatement, (SQLWCHAR *)pwszQuery, SQL_NTS);
#else
		SQLWCHAR *temp = UCS2StringFromUCS4String(pwszQuery);
		iResult = SQLExecDirectW(sqlStatement, temp, SQL_NTS);
		free(temp);
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
		dwResult = GetSQLErrorInfo(SQL_HANDLE_DBC, pConn->sqlConn, errorText);
	}

	MutexUnlock(pConn->mutexQuery);
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
      result = wcsdup(buffer);
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
   return (result != NULL) ? result : wcsdup(L"");
}

/**
 * Process results of SELECT query
 */
static DB2DRV_QUERY_RESULT *ProcessSelectResults(SQLHSTMT statement)
{
	// Allocate result buffer and determine column info
	DB2DRV_QUERY_RESULT *pResult = (DB2DRV_QUERY_RESULT *)malloc(sizeof(DB2DRV_QUERY_RESULT));
	short wNumCols;
	SQLNumResultCols(statement, &wNumCols);
	pResult->numColumns = wNumCols;
	pResult->numRows = 0;
	pResult->values = NULL;

	// Get column names
	pResult->columnNames = (char **)malloc(sizeof(char *) * pResult->numColumns);
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
			pResult->columnNames[i] = strdup("");
		}
	}

	// Fetch all data
	long iCurrValue = 0;
	SQLRETURN iResult;
	while(iResult = SQLFetch(statement), (iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
	{
		pResult->numRows++;
		pResult->values = (NETXMS_WCHAR **)realloc(pResult->values, sizeof(NETXMS_WCHAR *) * (pResult->numRows * pResult->numColumns));
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
extern "C" DBDRV_RESULT EXPORT DrvSelect(DB2DRV_CONN *pConn, NETXMS_WCHAR *pwszQuery, DWORD *pdwError, NETXMS_WCHAR *errorText)
{
	DB2DRV_QUERY_RESULT *pResult = NULL;

	MutexLock(pConn->mutexQuery);

	// Allocate statement handle
   SQLHSTMT sqlStatement;
	long iResult = SQLAllocHandle(SQL_HANDLE_STMT, pConn->sqlConn, &sqlStatement);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
	{
		// Execute statement
#if defined(_WIN32) || defined(UNICODE_UCS2)
		iResult = SQLExecDirectW(sqlStatement, (SQLWCHAR *)pwszQuery, SQL_NTS);
#else
		SQLWCHAR *temp = UCS2StringFromUCS4String(pwszQuery);
		iResult = SQLExecDirectW(sqlStatement, temp, SQL_NTS);
		free(temp);
#endif
		if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
		{
			pResult = ProcessSelectResults(sqlStatement);
			*pdwError = DBERR_SUCCESS;
		}
		else
		{
			*pdwError = GetSQLErrorInfo(SQL_HANDLE_STMT, sqlStatement, errorText);
		}
		SQLFreeHandle(SQL_HANDLE_STMT, sqlStatement);
	}
	else
	{
		*pdwError = GetSQLErrorInfo(SQL_HANDLE_DBC, pConn->sqlConn, errorText);
	}

	MutexUnlock(pConn->mutexQuery);
	return pResult;
}

/**
 * Perform SELECT query using prepared statement
 */
extern "C" DBDRV_RESULT EXPORT DrvSelectPrepared(DB2DRV_CONN *pConn, DB2DRV_STATEMENT *statement, DWORD *pdwError, NETXMS_WCHAR *errorText)
{
	DB2DRV_QUERY_RESULT *pResult = NULL;

	MutexLock(pConn->mutexQuery);
	long rc = SQLExecute(statement->handle);
	if ((rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO))
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

/**
 * Get field length from result
 */
extern "C" LONG EXPORT DrvGetFieldLength(DB2DRV_QUERY_RESULT *pResult, int iRow, int iColumn)
{
	LONG nLen = -1;

	if (pResult != NULL)
	{
		if ((iRow < pResult->numRows) && (iRow >= 0) &&
				(iColumn < pResult->numColumns) && (iColumn >= 0))
		{
			nLen = (LONG)wcslen(pResult->values[iRow * pResult->numColumns + iColumn]);
		}
	}
	return nLen;
}

/**
 * Get field value from result
 */
extern "C" NETXMS_WCHAR EXPORT *DrvGetField(DB2DRV_QUERY_RESULT *pResult, int iRow, int iColumn, NETXMS_WCHAR *pBuffer, int nBufSize)
{
	NETXMS_WCHAR *pValue = NULL;

	if (pResult != NULL)
	{
		if ((iRow < pResult->numRows) && (iRow >= 0) &&
				(iColumn < pResult->numColumns) && (iColumn >= 0))
		{
#ifdef _WIN32
			wcsncpy_s(pBuffer, nBufSize, pResult->values[iRow * pResult->numColumns + iColumn], _TRUNCATE);
#else
			wcsncpy(pBuffer, pResult->values[iRow * pResult->numColumns + iColumn], nBufSize);
			pBuffer[nBufSize - 1] = 0;
#endif
			pValue = pBuffer;
		}
	}
	return pValue;
}

/**
 * Get number of rows in result
 */
extern "C" int EXPORT DrvGetNumRows(DB2DRV_QUERY_RESULT *pResult)
{
	return (pResult != NULL) ? pResult->numRows : 0;
}

/**
 * Get column count in query result
 */
extern "C" int EXPORT DrvGetColumnCount(DB2DRV_QUERY_RESULT *pResult)
{
	return (pResult != NULL) ? pResult->numColumns : 0;
}

/**
 * Get column name in query result
 */
extern "C" const char EXPORT *DrvGetColumnName(DB2DRV_QUERY_RESULT *pResult, int column)
{
	return ((pResult != NULL) && (column >= 0) && (column < pResult->numColumns)) ? pResult->columnNames[column] : NULL;
}

/**
 * Free SELECT results
 */
extern "C" void EXPORT DrvFreeResult(DB2DRV_QUERY_RESULT *pResult)
{
	if (pResult == NULL)
      return;

	int i, iNumValues;

	iNumValues = pResult->numColumns * pResult->numRows;
	for(i = 0; i < iNumValues; i++)
	{
		free(pResult->values[i]);
	}
	safe_free(pResult->values);

	for(i = 0; i < pResult->numColumns; i++)
	{
		free(pResult->columnNames[i]);
	}
	free(pResult->columnNames);
	free(pResult);
}

/**
 * Perform unbuffered SELECT query
 */
extern "C" DBDRV_UNBUFFERED_RESULT EXPORT DrvSelectUnbuffered(DB2DRV_CONN *pConn, NETXMS_WCHAR *pwszQuery, DWORD *pdwError, NETXMS_WCHAR *errorText)
{
	DB2DRV_UNBUFFERED_QUERY_RESULT *pResult = NULL;

	MutexLock(pConn->mutexQuery);

	// Allocate statement handle
   SQLHSTMT sqlStatement;
	SQLRETURN iResult = SQLAllocHandle(SQL_HANDLE_STMT, pConn->sqlConn, &sqlStatement);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
	{
		// Execute statement
#if defined(_WIN32) || defined(UNICODE_UCS2)
		iResult = SQLExecDirectW(sqlStatement, (SQLWCHAR *)pwszQuery, SQL_NTS);
#else
		SQLWCHAR *temp = UCS2StringFromUCS4String(pwszQuery);
		iResult = SQLExecDirectW(sqlStatement, temp, SQL_NTS);
		free(temp);
#endif
		if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
		{
			// Allocate result buffer and determine column info
			pResult = (DB2DRV_UNBUFFERED_QUERY_RESULT *)malloc(sizeof(DB2DRV_UNBUFFERED_QUERY_RESULT));
         pResult->sqlStatement = sqlStatement;
         pResult->isPrepared = false;
      	
         short wNumCols;
			SQLNumResultCols(sqlStatement, &wNumCols);
			pResult->numColumns = wNumCols;
			pResult->pConn = pConn;
			pResult->noMoreRows = false;

			// Get column names
			pResult->columnNames = (char **)malloc(sizeof(char *) * pResult->numColumns);
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
					pResult->columnNames[i] = strdup("");
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
		*pdwError = GetSQLErrorInfo(SQL_HANDLE_DBC, pConn->sqlConn, errorText);
	}

	if (pResult == NULL) // Unlock mutex if query has failed
	{
		MutexUnlock(pConn->mutexQuery);
	}
	return pResult;
}

/**
 * Perform unbuffered SELECT query using prepared statement
 */
extern "C" DBDRV_UNBUFFERED_RESULT EXPORT DrvSelectPreparedUnbuffered(DB2DRV_CONN *pConn, DB2DRV_STATEMENT *stmt, DWORD *pdwError, WCHAR *errorText)
{
   DB2DRV_UNBUFFERED_QUERY_RESULT *pResult = NULL;

   MutexLock(pConn->mutexQuery);
	SQLRETURN rc = SQLExecute(stmt->handle);
   if ((rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO))
   {
      // Allocate result buffer and determine column info
      pResult = (DB2DRV_UNBUFFERED_QUERY_RESULT *)malloc(sizeof(DB2DRV_UNBUFFERED_QUERY_RESULT));
      pResult->sqlStatement = stmt->handle;
      pResult->isPrepared = true;
      
      short wNumCols;
      SQLNumResultCols(pResult->sqlStatement, &wNumCols);
      pResult->numColumns = wNumCols;
      pResult->pConn = pConn;
      pResult->noMoreRows = false;

		// Get column names
		pResult->columnNames = (char **)malloc(sizeof(char *) * pResult->numColumns);
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
				pResult->columnNames[i] = strdup("");
			}
		}

      *pdwError = DBERR_SUCCESS;
   }
   else
   {
		*pdwError = GetSQLErrorInfo(SQL_HANDLE_STMT, stmt->handle, errorText);
   }

   if (pResult == NULL) // Unlock mutex if query has failed
      MutexUnlock(pConn->mutexQuery);
	return pResult;
}

/**
 * Fetch next result line from asynchronous SELECT results
 */
extern "C" bool EXPORT DrvFetch(DB2DRV_UNBUFFERED_QUERY_RESULT *pResult)
{
	bool bResult = false;

	if (pResult != NULL)
	{
		long iResult;

		iResult = SQLFetch(pResult->sqlStatement);
		bResult = ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO));
		if (!bResult)
		{
			pResult->noMoreRows = true;
		}
	}
	return bResult;
}

/**
 * Get field length from unbuffered query result
 */
extern "C" LONG EXPORT DrvGetFieldLengthUnbuffered(DB2DRV_UNBUFFERED_QUERY_RESULT *pResult, int iColumn)
{
	LONG nLen = -1;

	if (pResult != NULL)
	{
		if ((iColumn < pResult->numColumns) && (iColumn >= 0))
		{
			SQLLEN dataSize;
			char temp[1];
			long rc = SQLGetData(pResult->sqlStatement, (short)iColumn + 1, SQL_C_CHAR, temp, 0, &dataSize);
			if ((rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO))
			{
				nLen = (LONG)dataSize;
			}
		}
	}
	return nLen;
}

/**
 * Get field from current row in unbuffered query result
 */
extern "C" NETXMS_WCHAR EXPORT *DrvGetFieldUnbuffered(DB2DRV_UNBUFFERED_QUERY_RESULT *pResult, int iColumn, NETXMS_WCHAR *pBuffer, int iBufSize)
{
	SQLLEN iDataSize;
	long iResult;

	// Check if we have valid result handle
	if (pResult == NULL)
	{
		return NULL;
	}

	// Check if there are valid fetched row
	if (pResult->noMoreRows)
	{
		return NULL;
	}

	if ((iColumn >= 0) && (iColumn < pResult->numColumns))
	{
#if defined(_WIN32) || defined(UNICODE_UCS2)
		iResult = SQLGetData(pResult->sqlStatement, (short)iColumn + 1, SQL_C_WCHAR,
				pBuffer, iBufSize * sizeof(WCHAR), &iDataSize);
#else
		SQLWCHAR *tempBuff = (SQLWCHAR *)malloc(iBufSize * sizeof(SQLWCHAR));
		iResult = SQLGetData(pResult->sqlStatement, (short)iColumn + 1, SQL_C_WCHAR,
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
 * Get column count in async query result
 */
extern "C" int EXPORT DrvGetColumnCountUnbuffered(DB2DRV_UNBUFFERED_QUERY_RESULT *pResult)
{
	return (pResult != NULL) ? pResult->numColumns : 0;
}

/**
 * Get column name in async query result
 */
extern "C" const char EXPORT *DrvGetColumnNameUnbuffered(DB2DRV_UNBUFFERED_QUERY_RESULT *pResult, int column)
{
	return ((pResult != NULL) && (column >= 0) && (column < pResult->numColumns)) ? pResult->columnNames[column] : NULL;
}

/**
 * Destroy result of unbuffered query
 */
extern "C" void EXPORT DrvFreeUnbufferedResult(DB2DRV_UNBUFFERED_QUERY_RESULT *pResult)
{
	if (pResult == NULL)
      return;

   if (pResult->isPrepared)
   	SQLCloseCursor(pResult->sqlStatement);
   else
	   SQLFreeHandle(SQL_HANDLE_STMT, pResult->sqlStatement);
	MutexUnlock(pResult->pConn->mutexQuery);

   for(int i = 0; i < pResult->numColumns; i++)
	{
		free(pResult->columnNames[i]);
	}
	free(pResult->columnNames);

   free(pResult);
}

/**
 * Begin transaction
 */
extern "C" DWORD EXPORT DrvBegin(DB2DRV_CONN *pConn)
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

/**
 * Commit transaction
 */
extern "C" DWORD EXPORT DrvCommit(DB2DRV_CONN *pConn)
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

/**
 * Rollback transaction
 */
extern "C" DWORD EXPORT DrvRollback(DB2DRV_CONN *pConn)
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


/**
 * Check if table exist
 */
extern "C" int EXPORT DrvIsTableExist(DB2DRV_CONN *pConn, const NETXMS_WCHAR *name)
{
   WCHAR query[256];
   swprintf(query, 256, L"SELECT count(*) FROM sysibm.systables WHERE type='T' AND upper(name)=upper('%ls')", name);
   DWORD error;
   WCHAR errorText[DBDRV_MAX_ERROR_TEXT];
   int rc = DBIsTableExist_Failure;
   DB2DRV_QUERY_RESULT *hResult = (DB2DRV_QUERY_RESULT *)DrvSelect(pConn, query, &error, errorText);
   if (hResult != NULL)
   {
      WCHAR buffer[64] = L"";
      DrvGetField(hResult, 0, 0, buffer, 64);
      rc = (wcstol(buffer, NULL, 10) > 0) ? DBIsTableExist_Found : DBIsTableExist_NotFound;
      DrvFreeResult(hResult);
   }
   return rc;
}

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
