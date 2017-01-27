/* 
** ODBC Database Driver
** Copyright (C) 2004-2016 Victor Kirhenshtein
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


DECLARE_DRIVER_HEADER("ODBC")

/**
 * Flag for enable/disable UNICODE
 */
static bool m_useUnicode = true;

/**
 * Convert ODBC state to NetXMS database error code and get error text
 */
static DWORD GetSQLErrorInfo(SQLSMALLINT nHandleType, SQLHANDLE hHandle, NETXMS_WCHAR *errorText)
{
   SQLRETURN nRet;
   SQLSMALLINT nChars;
   DWORD dwError;
   char szState[16];

	// Get state information and convert it to NetXMS database error code
   nRet = SQLGetDiagFieldA(nHandleType, hHandle, 1, SQL_DIAG_SQLSTATE, szState, 16, &nChars);
   if (nRet == SQL_SUCCESS)
   {
      if ((!strcmp(szState, "08003")) ||     // Connection does not exist
          (!strcmp(szState, "08S01")) ||     // Communication link failure
          (!strcmp(szState, "HYT00")) ||     // Timeout expired
          (!strcmp(szState, "HYT01")))       // Connection timeout expired
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
		char buffer[DBDRV_MAX_ERROR_TEXT];
		nRet = SQLGetDiagFieldA(nHandleType, hHandle, 1, SQL_DIAG_MESSAGE_TEXT, buffer, DBDRV_MAX_ERROR_TEXT, &nChars);
#endif
		if (nRet == SQL_SUCCESS)
		{
#if UNICODE_UCS4
			buffer[DBDRV_MAX_ERROR_TEXT - 1] = 0;
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, buffer, -1, errorText, DBDRV_MAX_ERROR_TEXT);
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
static void ClearPendingResults(SQLHSTMT stmt)
{
	while(1)
	{
		SQLRETURN rc = SQLMoreResults(stmt);
		if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO))
			break;
	}
}

/**
 * Prepare string for using in SQL query - enclose in quotes and escape as needed
 */
extern "C" NETXMS_WCHAR EXPORT *DrvPrepareStringW(const NETXMS_WCHAR *str)
{
	int len = (int)wcslen(str) + 3;   // + two quotes and \0 at the end
	int bufferSize = len + 128;
	NETXMS_WCHAR *out = (NETXMS_WCHAR *)malloc(bufferSize * sizeof(NETXMS_WCHAR));
	out[0] = L'\'';

	const NETXMS_WCHAR *src = str;
	int outPos;
	for(outPos = 1; *src != 0; src++)
	{
		if (*src == L'\'')
		{
			len++;
			if (len >= bufferSize)
			{
				bufferSize += 128;
				out = (NETXMS_WCHAR *)realloc(out, bufferSize * sizeof(NETXMS_WCHAR));
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
   m_useUnicode = ExtractNamedOptionValueAsBoolA(cmdLine, "unicode", true);
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
 * pszHost should be set to ODBC source name, and pszDatabase is ignored
 */
extern "C" DBDRV_CONNECTION EXPORT DrvConnect(const char *pszHost, const char *pszLogin, const char *pszPassword, 
                                              const char *pszDatabase, const char *schema, NETXMS_WCHAR *errorText)
{
   long iResult;
   ODBCDRV_CONN *pConn;

   // Allocate our connection structure
   pConn = (ODBCDRV_CONN *)malloc(sizeof(ODBCDRV_CONN));

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
	if (strchr(pszHost, '=') != NULL)
	{
		SQLSMALLINT outLen;
		iResult = SQLDriverConnectA(pConn->sqlConn, NULL, (SQLCHAR*)pszHost, SQL_NTS, NULL, 0, &outLen, SQL_DRIVER_NOPROMPT);
	}
	else
	{
		iResult = SQLConnectA(pConn->sqlConn, (SQLCHAR *)pszHost, SQL_NTS,
									(SQLCHAR *)pszLogin, SQL_NTS, (SQLCHAR *)pszPassword, SQL_NTS);
	}
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

/**
 * Disconnect from database
 */
extern "C" void EXPORT DrvDisconnect(ODBCDRV_CONN *pConn)
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
extern "C" DBDRV_STATEMENT EXPORT DrvPrepare(ODBCDRV_CONN *pConn, NETXMS_WCHAR *pwszQuery, DWORD *pdwError, NETXMS_WCHAR *errorText)
{
   long iResult;
	SQLHSTMT stmt;
	ODBCDRV_STATEMENT *result;

   MutexLock(pConn->mutexQuery);

   // Allocate statement handle
   iResult = SQLAllocHandle(SQL_HANDLE_STMT, pConn->sqlConn, &stmt);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
   {
      // Prepare statement
      if (m_useUnicode)
      {
#if defined(_WIN32) || defined(UNICODE_UCS2)
		   iResult = SQLPrepareW(stmt, (SQLWCHAR *)pwszQuery, SQL_NTS);
#else
			SQLWCHAR *temp = UCS2StringFromUCS4String(pwszQuery);
		   iResult = SQLPrepareW(stmt, temp, SQL_NTS);
		   free(temp);
#endif      
		}
		else
		{
			char *temp = MBStringFromWideString(pwszQuery);
		   iResult = SQLPrepareA(stmt, (SQLCHAR *)temp, SQL_NTS);
		   free(temp);
		}
	   if ((iResult == SQL_SUCCESS) || 
          (iResult == SQL_SUCCESS_WITH_INFO))
		{
			result = (ODBCDRV_STATEMENT *)malloc(sizeof(ODBCDRV_STATEMENT));
			result->handle = stmt;
			result->buffers = new Array(0, 16, true);
			result->connection = pConn;
			*pdwError = DBERR_SUCCESS;
		}
		else
      {
         *pdwError = GetSQLErrorInfo(SQL_HANDLE_STMT, stmt, errorText);
	      SQLFreeHandle(SQL_HANDLE_STMT, stmt);
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
extern "C" void EXPORT DrvBind(ODBCDRV_STATEMENT *stmt, int pos, int sqlType, int cType, void *buffer, int allocType)
{
	static SQLSMALLINT odbcSqlType[] = { SQL_VARCHAR, SQL_INTEGER, SQL_BIGINT, SQL_DOUBLE, SQL_LONGVARCHAR };
	static SQLSMALLINT odbcCTypeW[] = { SQL_C_WCHAR, SQL_C_SLONG, SQL_C_ULONG, SQL_C_SBIGINT, SQL_C_UBIGINT, SQL_C_DOUBLE, SQL_C_WCHAR };
	static SQLSMALLINT odbcCTypeA[] = { SQL_C_CHAR, SQL_C_SLONG, SQL_C_ULONG, SQL_C_SBIGINT, SQL_C_UBIGINT, SQL_C_DOUBLE, SQL_C_CHAR };
	static DWORD bufferSize[] = { 0, sizeof(LONG), sizeof(DWORD), sizeof(INT64), sizeof(QWORD), sizeof(double), 0 };

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
               sqlBuffer = nx_memdup(buffer, (cType == DB_CTYPE_STRING) ? (DWORD)(length * sizeof(WCHAR)) : bufferSize[cType]);
            }
#else
				if (cType == DB_CTYPE_STRING)
				{
					sqlBuffer = UCS2StringFromUCS4String((NETXMS_WCHAR *)buffer);
				}
            if (cType == DB_CTYPE_UTF8_STRING)
            {
               sqlBuffer = UCS2StringFromUTF8String((char *)buffer);
               length = (int)ucs2_strlen((UCS2CHAR *)sqlBuffer) + 1;
            }
				else
				{
					sqlBuffer = nx_memdup(buffer, bufferSize[cType]);
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
					sqlBuffer = nx_memdup(buffer, bufferSize[cType]);
				}
			}
			stmt->buffers->add(sqlBuffer);
			break;
		default:
			return;	// Invalid call
	}
	SQLBindParameter(stmt->handle, pos, SQL_PARAM_INPUT, m_useUnicode ? odbcCTypeW[cType] : odbcCTypeA[cType], odbcSqlType[sqlType],
	                 ((cType == DB_CTYPE_STRING) || (cType == DB_CTYPE_UTF8_STRING)) ? length : 0, 0, sqlBuffer, 0, NULL);
}

/**
 * Execute prepared statement
 */
extern "C" DWORD EXPORT DrvExecute(ODBCDRV_CONN *pConn, ODBCDRV_STATEMENT *stmt, NETXMS_WCHAR *errorText)
{
   DWORD dwResult;

   MutexLock(pConn->mutexQuery);
	long rc = SQLExecute(stmt->handle);
   if ((rc == SQL_SUCCESS) || 
       (rc == SQL_SUCCESS_WITH_INFO) || 
       (rc == SQL_NO_DATA))
   {
		ClearPendingResults(stmt->handle);
      dwResult = DBERR_SUCCESS;
   }
   else
   {
		dwResult = GetSQLErrorInfo(SQL_HANDLE_STMT, stmt->handle, errorText);
   }
   MutexUnlock(pConn->mutexQuery);
	return dwResult;
}

/**
 * Destroy prepared statement
 */
extern "C" void EXPORT DrvFreeStatement(ODBCDRV_STATEMENT *stmt)
{
	if (stmt == NULL)
		return;

	MutexLock(stmt->connection->mutexQuery);
	SQLFreeHandle(SQL_HANDLE_STMT, stmt->handle);
	MutexUnlock(stmt->connection->mutexQuery);
	delete stmt->buffers;
	free(stmt);
}

/**
 * Perform non-SELECT query
 */
extern "C" DWORD EXPORT DrvQuery(ODBCDRV_CONN *pConn, NETXMS_WCHAR *pwszQuery, NETXMS_WCHAR *errorText)
{
   DWORD dwResult;

   MutexLock(pConn->mutexQuery);

   // Allocate statement handle
   SQLHSTMT sqlStatement;
   SQLRETURN iResult = SQLAllocHandle(SQL_HANDLE_STMT, pConn->sqlConn, &sqlStatement);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
   {
      // Execute statement
      if (m_useUnicode)
      {
#if defined(_WIN32) || defined(UNICODE_UCS2)
		   iResult = SQLExecDirectW(sqlStatement, (SQLWCHAR *)pwszQuery, SQL_NTS);
#else
			SQLWCHAR *temp = UCS2StringFromUCS4String(pwszQuery);
		   iResult = SQLExecDirectW(sqlStatement, temp, SQL_NTS);
		   free(temp);
#endif      
		}
		else
		{
			char *temp = MBStringFromWideString(pwszQuery);
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
   SQLLEN dataSize;
   if (m_useUnicode)
   {
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
   return (result != NULL) ? result : wcsdup(L"");
}

/**
 * Process results of SELECT query
 */
static ODBCDRV_QUERY_RESULT *ProcessSelectResults(SQLHSTMT stmt)
{
   // Allocate result buffer and determine column info
   ODBCDRV_QUERY_RESULT *pResult = (ODBCDRV_QUERY_RESULT *)malloc(sizeof(ODBCDRV_QUERY_RESULT));
   short wNumCols;
   SQLNumResultCols(stmt, &wNumCols);
   pResult->numColumns = wNumCols;
   pResult->numRows = 0;
   pResult->pValues = NULL;

	// Get column names
	pResult->columnNames = (char **)malloc(sizeof(char *) * pResult->numColumns);
	for(int i = 0; i < (int)pResult->numColumns; i++)
	{
		char name[256];
		SQLSMALLINT len;

		SQLRETURN iResult = SQLColAttributeA(stmt, (SQLSMALLINT)(i + 1), SQL_DESC_NAME, name, 256, &len, NULL); 
		if ((iResult == SQL_SUCCESS) || 
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
   while(iResult = SQLFetch(stmt), 
         (iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
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
extern "C" DBDRV_RESULT EXPORT DrvSelect(ODBCDRV_CONN *pConn, NETXMS_WCHAR *pwszQuery, DWORD *pdwError, NETXMS_WCHAR *errorText)
{
   ODBCDRV_QUERY_RESULT *pResult = NULL;

   MutexLock(pConn->mutexQuery);

   // Allocate statement handle
   SQLHSTMT sqlStatement;
   SQLRETURN iResult = SQLAllocHandle(SQL_HANDLE_STMT, pConn->sqlConn, &sqlStatement);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
   {
      // Execute statement
      if (m_useUnicode)
      {
#if defined(_WIN32) || defined(UNICODE_UCS2)
	      iResult = SQLExecDirectW(sqlStatement, (SQLWCHAR *)pwszQuery, SQL_NTS);
#else
			SQLWCHAR *temp = UCS2StringFromUCS4String(pwszQuery);
		   iResult = SQLExecDirectW(sqlStatement, temp, SQL_NTS);
		   free(temp);
#endif
		}
		else
		{
			char *temp = MBStringFromWideString(pwszQuery);
		   iResult = SQLExecDirectA(sqlStatement, (SQLCHAR *)temp, SQL_NTS);
		   free(temp);
		}
	   if ((iResult == SQL_SUCCESS) || 
          (iResult == SQL_SUCCESS_WITH_INFO))
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
extern "C" DBDRV_RESULT EXPORT DrvSelectPrepared(ODBCDRV_CONN *pConn, ODBCDRV_STATEMENT *stmt, DWORD *pdwError, NETXMS_WCHAR *errorText)
{
   ODBCDRV_QUERY_RESULT *pResult = NULL;

   MutexLock(pConn->mutexQuery);
	long rc = SQLExecute(stmt->handle);
   if ((rc == SQL_SUCCESS) || 
       (rc == SQL_SUCCESS_WITH_INFO))
   {
		pResult = ProcessSelectResults(stmt->handle);
	   *pdwError = DBERR_SUCCESS;
   }
   else
   {
		*pdwError = GetSQLErrorInfo(SQL_HANDLE_STMT, stmt->handle, errorText);
   }
   MutexUnlock(pConn->mutexQuery);
	return pResult;
}

/**
 * Get field length from result
 */
extern "C" LONG EXPORT DrvGetFieldLength(ODBCDRV_QUERY_RESULT *pResult, int iRow, int iColumn)
{
   LONG nLen = -1;

   if (pResult != NULL)
   {
      if ((iRow < pResult->numRows) && (iRow >= 0) &&
          (iColumn < pResult->numColumns) && (iColumn >= 0))
         nLen = (LONG)wcslen(pResult->pValues[iRow * pResult->numColumns + iColumn]);
   }
   return nLen;
}

/**
 * Get field value from result
 */
extern "C" NETXMS_WCHAR EXPORT *DrvGetField(ODBCDRV_QUERY_RESULT *pResult, int iRow, int iColumn,
                                            NETXMS_WCHAR *pBuffer, int nBufSize)
{
   NETXMS_WCHAR *pValue = NULL;

   if (pResult != NULL)
   {
      if ((iRow < pResult->numRows) && (iRow >= 0) &&
          (iColumn < pResult->numColumns) && (iColumn >= 0))
      {
#ifdef _WIN32
         wcsncpy_s(pBuffer, nBufSize, pResult->pValues[iRow * pResult->numColumns + iColumn], _TRUNCATE);
#else
         wcsncpy(pBuffer, pResult->pValues[iRow * pResult->numColumns + iColumn], nBufSize);
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
extern "C" int EXPORT DrvGetNumRows(ODBCDRV_QUERY_RESULT *pResult)
{
   return (pResult != NULL) ? pResult->numRows : 0;
}

/**
 * Get column count in query result
 */
extern "C" int EXPORT DrvGetColumnCount(ODBCDRV_QUERY_RESULT *pResult)
{
	return (pResult != NULL) ? pResult->numColumns : 0;
}

/**
 * Get column name in query result
 */
extern "C" const char EXPORT *DrvGetColumnName(ODBCDRV_QUERY_RESULT *pResult, int column)
{
	return ((pResult != NULL) && (column >= 0) && (column < pResult->numColumns)) ? pResult->columnNames[column] : NULL;
}

/**
 * Free SELECT results
 */
extern "C" void EXPORT DrvFreeResult(ODBCDRV_QUERY_RESULT *pResult)
{
   if (pResult != NULL)
   {
      int i, iNumValues;

      iNumValues = pResult->numColumns * pResult->numRows;
      for(i = 0; i < iNumValues; i++)
         safe_free(pResult->pValues[i]);
      safe_free(pResult->pValues);

		for(i = 0; i < pResult->numColumns; i++)
			safe_free(pResult->columnNames[i]);
		safe_free(pResult->columnNames);

      free(pResult);
   }
}

/**
 * Perform unbuffered SELECT query
 */
extern "C" DBDRV_UNBUFFERED_RESULT EXPORT DrvSelectUnbuffered(ODBCDRV_CONN *pConn, NETXMS_WCHAR *pwszQuery, DWORD *pdwError, NETXMS_WCHAR *errorText)
{
   ODBCDRV_UNBUFFERED_QUERY_RESULT *pResult = NULL;
   long iResult;
   short wNumCols;
	int i;

   MutexLock(pConn->mutexQuery);

   // Allocate statement handle
   SQLHSTMT sqlStatement;
   iResult = SQLAllocHandle(SQL_HANDLE_STMT, pConn->sqlConn, &sqlStatement);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
   {
      // Execute statement
      if (m_useUnicode)
      {
#if defined(_WIN32) || defined(UNICODE_UCS2)
	      iResult = SQLExecDirectW(sqlStatement, (SQLWCHAR *)pwszQuery, SQL_NTS);
#else
			SQLWCHAR *temp = UCS2StringFromUCS4String(pwszQuery);
		   iResult = SQLExecDirectW(sqlStatement, temp, SQL_NTS);
		   free(temp);
#endif
		}
		else
		{
			char *temp = MBStringFromWideString(pwszQuery);
		   iResult = SQLExecDirectA(sqlStatement, (SQLCHAR *)temp, SQL_NTS);
		   free(temp);
		}
	   if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
      {
         // Allocate result buffer and determine column info
         pResult = (ODBCDRV_UNBUFFERED_QUERY_RESULT *)malloc(sizeof(ODBCDRV_UNBUFFERED_QUERY_RESULT));
         pResult->sqlStatement = sqlStatement;
         pResult->isPrepared = false;
         
         SQLNumResultCols(sqlStatement, &wNumCols);
         pResult->numColumns = wNumCols;
         pResult->pConn = pConn;
         pResult->noMoreRows = false;

			// Get column names
			pResult->columnNames = (char **)malloc(sizeof(char *) * pResult->numColumns);
			for(i = 0; i < pResult->numColumns; i++)
			{
				char name[256];
				SQLSMALLINT len;

				iResult = SQLColAttributeA(sqlStatement, (SQLSMALLINT)(i + 1),
				                           SQL_DESC_NAME, name, 256, &len, NULL); 
				if ((iResult == SQL_SUCCESS) || 
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

			// Column values cache
			pResult->values = (NETXMS_WCHAR **)malloc(sizeof(NETXMS_WCHAR *) * pResult->numColumns);
			memset(pResult->values, 0, sizeof(NETXMS_WCHAR *) * pResult->numColumns);

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
      MutexUnlock(pConn->mutexQuery);
   return pResult;
}

/**
 * Perform unbuffered SELECT query using prepared statement
 */
extern "C" DBDRV_UNBUFFERED_RESULT EXPORT DrvSelectPreparedUnbuffered(ODBCDRV_CONN *pConn, ODBCDRV_STATEMENT *stmt, DWORD *pdwError, NETXMS_WCHAR *errorText)
{
   ODBCDRV_UNBUFFERED_QUERY_RESULT *pResult = NULL;

   MutexLock(pConn->mutexQuery);
	SQLRETURN rc = SQLExecute(stmt->handle);
   if ((rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO))
   {
      // Allocate result buffer and determine column info
      pResult = (ODBCDRV_UNBUFFERED_QUERY_RESULT *)malloc(sizeof(ODBCDRV_UNBUFFERED_QUERY_RESULT));
      pResult->sqlStatement = stmt->handle;
      pResult->isPrepared = true;
      
      short wNumCols;
      SQLNumResultCols(pResult->sqlStatement, &wNumCols);
      pResult->numColumns = wNumCols;
      pResult->pConn = pConn;
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
				pResult->columnNames[i] = strdup(name);
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
extern "C" bool EXPORT DrvFetch(ODBCDRV_UNBUFFERED_QUERY_RESULT *pResult)
{
   bool success = false;

   if (pResult != NULL)
   {
      SQLRETURN rc = SQLFetch(pResult->sqlStatement);
      success = ((rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO));
      if (success)
      {
         for(int i = 0; i < pResult->numColumns; i++)
         {
            free(pResult->values[i]);
            pResult->values[i] = GetFieldData(pResult->sqlStatement, (short)i + 1);
         }
      }
      else
      {
         pResult->noMoreRows = true;
      }
   }
   return success;
}

/**
 * Get field length from async query result
 */
extern "C" LONG EXPORT DrvGetFieldLengthUnbuffered(ODBCDRV_UNBUFFERED_QUERY_RESULT *result, int col)
{
   if (result == NULL)
      return -1;

   if ((col >= result->numColumns) || (col < 0))
      return -1;

   return (result->values[col] != NULL) ? (LONG)wcslen(result->values[col]) : -1;
}

/**
 * Get field from current row in async query result
 */
extern "C" NETXMS_WCHAR EXPORT *DrvGetFieldUnbuffered(ODBCDRV_UNBUFFERED_QUERY_RESULT *result, int col, NETXMS_WCHAR *buffer, int bufferSize)
{
   // Check if we have valid result handle
   if (result == NULL)
      return NULL;

   // Check if there are valid fetched row
   if (result->noMoreRows)
      return NULL;

   if ((col >= 0) && (col < result->numColumns) && (result->values[col] != NULL))
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
extern "C" int EXPORT DrvGetColumnCountUnbuffered(ODBCDRV_UNBUFFERED_QUERY_RESULT *pResult)
{
	return (pResult != NULL) ? pResult->numColumns : 0;
}

/**
 * Get column name in async query result
 */
extern "C" const char EXPORT *DrvGetColumnNameUnbuffered(ODBCDRV_UNBUFFERED_QUERY_RESULT *pResult, int column)
{
	return ((pResult != NULL) && (column >= 0) && (column < pResult->numColumns)) ? pResult->columnNames[column] : NULL;
}

/**
 * Destroy result of async query
 */
extern "C" void EXPORT DrvFreeUnbufferedResult(ODBCDRV_UNBUFFERED_QUERY_RESULT *pResult)
{
   if (pResult == NULL)
      return;

   if (pResult->isPrepared)
      SQLCloseCursor(pResult->sqlStatement);
   else
      SQLFreeHandle(SQL_HANDLE_STMT, pResult->sqlStatement);
   for(int i = 0; i < pResult->numColumns; i++)
   {
      safe_free(pResult->columnNames[i]);
      safe_free(pResult->values[i]);
   }
   free(pResult->columnNames);
   free(pResult->values);
   MutexUnlock(pResult->pConn->mutexQuery);
   free(pResult);
}

/**
 * Begin transaction
 */
extern "C" DWORD EXPORT DrvBegin(ODBCDRV_CONN *pConn)
{
   SQLRETURN nRet;
   DWORD dwResult;

	if (pConn == NULL)
      return DBERR_INVALID_HANDLE;

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
extern "C" DWORD EXPORT DrvCommit(ODBCDRV_CONN *pConn)
{
   SQLRETURN nRet;

	if (pConn == NULL)
      return DBERR_INVALID_HANDLE;

	MutexLock(pConn->mutexQuery);
   nRet = SQLEndTran(SQL_HANDLE_DBC, pConn->sqlConn, SQL_COMMIT);
   SQLSetConnectAttr(pConn->sqlConn, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
	MutexUnlock(pConn->mutexQuery);
   return ((nRet == SQL_SUCCESS) || (nRet == SQL_SUCCESS_WITH_INFO)) ? DBERR_SUCCESS : DBERR_OTHER_ERROR;
}

/**
 * Rollback transaction
 */
extern "C" DWORD EXPORT DrvRollback(ODBCDRV_CONN *pConn)
{
   SQLRETURN nRet;

	if (pConn == NULL)
      return DBERR_INVALID_HANDLE;

	MutexLock(pConn->mutexQuery);
   nRet = SQLEndTran(SQL_HANDLE_DBC, pConn->sqlConn, SQL_ROLLBACK);
   SQLSetConnectAttr(pConn->sqlConn, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
	MutexUnlock(pConn->mutexQuery);
   return ((nRet == SQL_SUCCESS) || (nRet == SQL_SUCCESS_WITH_INFO)) ? DBERR_SUCCESS : DBERR_OTHER_ERROR;
}

/**
 * Check if table exist
 */
extern "C" int EXPORT DrvIsTableExist(ODBCDRV_CONN *pConn, const NETXMS_WCHAR *name)
{
   int rc = DBIsTableExist_Failure;

   MutexLock(pConn->mutexQuery);

   SQLHSTMT sqlStatement;
   SQLRETURN iResult = SQLAllocHandle(SQL_HANDLE_STMT, pConn->sqlConn, &sqlStatement);
	if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
   {
      if (m_useUnicode)
      {
#if defined(_WIN32) || defined(UNICODE_UCS2)
	      iResult = SQLTablesW(sqlStatement, NULL, 0, NULL, 0, (SQLWCHAR *)name, SQL_NTS, NULL, 0);
#else
			SQLWCHAR *temp = UCS2StringFromUCS4String(name);
	      iResult = SQLTablesW(sqlStatement, NULL, 0, NULL, 0, (SQLWCHAR *)temp, SQL_NTS, NULL, 0);
		   free(temp);
#endif
		}
		else
		{
			char *temp = MBStringFromWideString(name);
	      iResult = SQLTablesA(sqlStatement, NULL, 0, NULL, 0, (SQLCHAR *)temp, SQL_NTS, NULL, 0);
		   free(temp);
		}
	   if ((iResult == SQL_SUCCESS) || (iResult == SQL_SUCCESS_WITH_INFO))
      {
			ODBCDRV_QUERY_RESULT *pResult = ProcessSelectResults(sqlStatement);
         rc = (DrvGetNumRows(pResult) > 0) ? DBIsTableExist_Found : DBIsTableExist_NotFound;
         DrvFreeResult(pResult);
      }
      SQLFreeHandle(SQL_HANDLE_STMT, sqlStatement);
   }

   MutexUnlock(pConn->mutexQuery);
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
