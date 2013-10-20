/* 
** Oracle Database Driver
** Copyright (C) 2007-2013 Victor Kirhenshtein
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
** File: oracle.cpp
**
**/

#include "oracledrv.h"

DECLARE_DRIVER_HEADER("ORACLE")

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

/**
 * Prepare string for using in SQL query - enclose in quotes and escape as needed
 */
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
extern "C" BOOL EXPORT DrvInit(const char *cmdLine)
{
	return TRUE;
}

/**
 * Unload handler
 */
extern "C" void EXPORT DrvUnload()
{
	OCITerminate(OCI_DEFAULT);
}

/**
 * Get error text from error handle
 */
static void GetErrorTextFromHandle(OCIError *handle, WCHAR *errorText)
{
	sb4 nCode;

#if UNICODE_UCS2
	OCIErrorGet(handle, 1, NULL, &nCode, (text *)errorText, DBDRV_MAX_ERROR_TEXT, OCI_HTYPE_ERROR);
	errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
#else
	UCS2CHAR buffer[DBDRV_MAX_ERROR_TEXT];

	OCIErrorGet(handle, 1, NULL, &nCode, (text *)buffer, DBDRV_MAX_ERROR_TEXT, OCI_HTYPE_ERROR);
	buffer[DBDRV_MAX_ERROR_TEXT - 1] = 0;
	ucs2_to_ucs4(buffer, ucs2_strlen(buffer) + 1, errorText, DBDRV_MAX_ERROR_TEXT);
	errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
#endif
	RemoveTrailingCRLFW(errorText);
}

/**
 * Set last error text
 */
static void SetLastErrorText(ORACLE_CONN *pConn)
{
	GetErrorTextFromHandle(pConn->handleError, pConn->szLastError);
}

/**
 * Check if last error was caused by lost connection to server
 */
static DWORD IsConnectionError(ORACLE_CONN *pConn)
{
	ub4 nStatus = 0;

	OCIAttrGet(pConn->handleServer, OCI_HTYPE_SERVER, &nStatus,
	           NULL, OCI_ATTR_SERVER_STATUS, pConn->handleError);
	return (nStatus == OCI_SERVER_NOT_CONNECTED) ? DBERR_CONNECTION_LOST : DBERR_OTHER_ERROR;
}

/**
 * Destroy query result
 */
static void DestroyQueryResult(ORACLE_RESULT *pResult)
{
	int i, nCount;

	nCount = pResult->nCols * pResult->nRows;
	for(i = 0; i < nCount; i++)
		safe_free(pResult->pData[i]);
	safe_free(pResult->pData);

	for(i = 0; i < pResult->nCols; i++)
		safe_free(pResult->columnNames[i]);
	safe_free(pResult->columnNames);

	free(pResult);
}

/**
 * Connect to database
 */
extern "C" DBDRV_CONNECTION EXPORT DrvConnect(const char *host, const char *login, const char *password, 
                                              const char *database, const char *schema, WCHAR *errorText)
{
	ORACLE_CONN *pConn;
	UCS2CHAR *pwszStr;

	pConn = (ORACLE_CONN *)malloc(sizeof(ORACLE_CONN));
	if (pConn != NULL)
	{
		memset(pConn, 0, sizeof(ORACLE_CONN));

		if (OCIEnvNlsCreate(&pConn->handleEnv, OCI_THREADED | OCI_NCHAR_LITERAL_REPLACE_OFF,
		                    NULL, NULL, NULL, NULL, 0, NULL, OCI_UTF16ID, OCI_UTF16ID) == OCI_SUCCESS)
		{
			OCIHandleAlloc(pConn->handleEnv, (void **)&pConn->handleError, OCI_HTYPE_ERROR, 0, NULL);
			OCIHandleAlloc(pConn->handleEnv, (void **)&pConn->handleServer, OCI_HTYPE_SERVER, 0, NULL);
			pwszStr = UCS2StringFromMBString(host);
			if (OCIServerAttach(pConn->handleServer, pConn->handleError,
			                    (text *)pwszStr, (sb4)ucs2_strlen(pwszStr) * sizeof(UCS2CHAR), OCI_DEFAULT) == OCI_SUCCESS)
			{
				free(pwszStr);

				// Initialize service handle
				OCIHandleAlloc(pConn->handleEnv, (void **)&pConn->handleService, OCI_HTYPE_SVCCTX, 0, NULL);
				OCIAttrSet(pConn->handleService, OCI_HTYPE_SVCCTX, pConn->handleServer, 0, OCI_ATTR_SERVER, pConn->handleError);
				
				// Initialize session handle
				OCIHandleAlloc(pConn->handleEnv, (void **)&pConn->handleSession, OCI_HTYPE_SESSION, 0, NULL);
				pwszStr = UCS2StringFromMBString(login);
				OCIAttrSet(pConn->handleSession, OCI_HTYPE_SESSION, pwszStr,
				           (ub4)ucs2_strlen(pwszStr) * sizeof(UCS2CHAR), OCI_ATTR_USERNAME, pConn->handleError);
				free(pwszStr);
				pwszStr = UCS2StringFromMBString(password);
				OCIAttrSet(pConn->handleSession, OCI_HTYPE_SESSION, pwszStr,
				           (ub4)ucs2_strlen(pwszStr) * sizeof(UCS2CHAR), OCI_ATTR_PASSWORD, pConn->handleError);

				// Authenticate
				if (OCISessionBegin(pConn->handleService, pConn->handleError,
				                    pConn->handleSession, OCI_CRED_RDBMS, OCI_DEFAULT) == OCI_SUCCESS)
				{
					OCIAttrSet(pConn->handleService, OCI_HTYPE_SVCCTX, pConn->handleSession, 0,
					           OCI_ATTR_SESSION, pConn->handleError);
					pConn->mutexQueryLock = MutexCreate();
					pConn->nTransLevel = 0;
					pConn->szLastError[0] = 0;

					if ((schema != NULL) && (schema[0] != 0))
					{
						free(pwszStr);
						pwszStr = UCS2StringFromMBString(schema);
						OCIAttrSet(pConn->handleSession, OCI_HTYPE_SESSION, pwszStr,
									  (ub4)ucs2_strlen(pwszStr) * sizeof(UCS2CHAR), OCI_ATTR_CURRENT_SCHEMA, pConn->handleError);
					}
				}
				else
				{
					GetErrorTextFromHandle(pConn->handleError, errorText);
					OCIHandleFree(pConn->handleEnv, OCI_HTYPE_ENV);
					free(pConn);
					pConn = NULL;
				}
			}
			else
			{
				GetErrorTextFromHandle(pConn->handleError, errorText);
				OCIHandleFree(pConn->handleEnv, OCI_HTYPE_ENV);
				free(pConn);
				pConn = NULL;
			}
			free(pwszStr);
		}
		else
		{
			wcscpy(errorText, L"Cannot allocate environment handle");
			free(pConn);
			pConn = NULL;
		}
	}
	else
	{
		wcscpy(errorText, L"Memory allocation error");
	}

   return (DBDRV_CONNECTION)pConn;
}

/**
 * Disconnect from database
 */
extern "C" void EXPORT DrvDisconnect(ORACLE_CONN *pConn)
{
	if (pConn != NULL)
	{
		OCISessionEnd(pConn->handleService, pConn->handleError, NULL, OCI_DEFAULT);
		OCIServerDetach(pConn->handleServer, pConn->handleError, OCI_DEFAULT);
		OCIHandleFree(pConn->handleEnv, OCI_HTYPE_ENV);
     	MutexDestroy(pConn->mutexQueryLock);
      free(pConn);
	}
}

/**
 * Convert query from NetXMS portable format to native Oracle format
 */
static UCS2CHAR *ConvertQuery(WCHAR *query)
{
#if UNICODE_UCS4
	UCS2CHAR *srcQuery = UCS2StringFromUCS4String(query);
#else
	UCS2CHAR *srcQuery = query;
#endif
	int count = NumCharsW(query, L'?');
	if (count == 0)
	{
#if UNICODE_UCS4
		return srcQuery;
#else
		return wcsdup(query);
#endif
	}

	UCS2CHAR *dstQuery = (UCS2CHAR *)malloc((ucs2_strlen(srcQuery) + count * 2 + 1) * sizeof(UCS2CHAR));
	bool inString = false;
	int pos = 1;
	UCS2CHAR *src, *dst;
	for(src = srcQuery, dst = dstQuery; *src != 0; src++)
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
					*dst++ = ':';
					if (pos < 10)
					{
						*dst++ = pos + '0';
					}
					else
					{
						*dst++ = pos / 10 + '0';
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
#if UNICODE_UCS4
	free(srcQuery);
#endif
	return dstQuery;
}

/**
 * Prepare statement
 */
extern "C" ORACLE_STATEMENT EXPORT *DrvPrepare(ORACLE_CONN *pConn, WCHAR *pwszQuery, DWORD *pdwError, WCHAR *errorText)
{
	ORACLE_STATEMENT *stmt = NULL;
	OCIStmt *handleStmt;

	UCS2CHAR *ucs2Query = ConvertQuery(pwszQuery);

	MutexLock(pConn->mutexQueryLock);
	if (OCIStmtPrepare2(pConn->handleService, &handleStmt, pConn->handleError, (text *)ucs2Query,
	                    (ub4)ucs2_strlen(ucs2Query) * sizeof(UCS2CHAR), NULL, 0, OCI_NTV_SYNTAX, OCI_DEFAULT) == OCI_SUCCESS)
	{
		stmt = (ORACLE_STATEMENT *)malloc(sizeof(ORACLE_STATEMENT));
		stmt->connection = pConn;
		stmt->handleStmt = handleStmt;
		stmt->bindings = new Array(8, 8, false);
		stmt->buffers = new Array(8, 8, true);
		OCIHandleAlloc(pConn->handleEnv, (void **)&stmt->handleError, OCI_HTYPE_ERROR, 0, NULL);
		*pdwError = DBERR_SUCCESS;
	}
	else
	{
		SetLastErrorText(pConn);
		*pdwError = IsConnectionError(pConn);
	}

	if (errorText != NULL)
	{
		wcsncpy(errorText, pConn->szLastError, DBDRV_MAX_ERROR_TEXT);
		errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
	}
	MutexUnlock(pConn->mutexQueryLock);

	free(ucs2Query);
	return stmt;
}

/**
 * Bind parameter to statement
 */
extern "C" void EXPORT DrvBind(ORACLE_STATEMENT *stmt, int pos, int sqlType, int cType, void *buffer, int allocType)
{
	static DWORD bufferSize[] = { 0, sizeof(LONG), sizeof(DWORD), sizeof(INT64), sizeof(QWORD), sizeof(double) };
	static ub2 oracleType[] = { SQLT_STR, SQLT_INT, SQLT_UIN, SQLT_INT, SQLT_UIN, SQLT_FLT };

	OCIBind *handleBind = (OCIBind *)stmt->bindings->get(pos - 1);
	void *sqlBuffer;
	switch(cType)
	{
		case DB_CTYPE_STRING:
#if UNICODE_UCS4
			sqlBuffer = UCS2StringFromUCS4String((WCHAR *)buffer);
			stmt->buffers->set(pos - 1, sqlBuffer);
			if (allocType == DB_BIND_DYNAMIC)
				free(buffer);
#else
			if (allocType == DB_BIND_TRANSIENT)
			{
				sqlBuffer = wcsdup((WCHAR *)buffer);
				stmt->buffers->set(pos - 1, sqlBuffer);
			}
			else
			{
				sqlBuffer = buffer;
				if (allocType == DB_BIND_DYNAMIC)
					stmt->buffers->set(pos - 1, sqlBuffer);
			}
#endif
			OCIBindByPos(stmt->handleStmt, &handleBind, stmt->handleError, pos, sqlBuffer,
							 ((sb4)ucs2_strlen((UCS2CHAR *)sqlBuffer) + 1) * sizeof(UCS2CHAR), 
							 (sqlType == DB_SQLTYPE_TEXT) ? SQLT_LNG : SQLT_STR,
							 NULL, NULL, NULL, 0, NULL, OCI_DEFAULT);
			break;
		case DB_CTYPE_INT64:	// OCI prior to 11.2 cannot bind 64 bit integers
#ifdef UNICODE_UCS2
			sqlBuffer = malloc(64 * sizeof(WCHAR));
			swprintf((WCHAR *)sqlBuffer, 64, INT64_FMTW, *((INT64 *)buffer));
#else
			{
				char temp[64];
				snprintf(temp, 64, INT64_FMTA, *((INT64 *)buffer));
				sqlBuffer = UCS2StringFromMBString(temp);
			}
#endif
			stmt->buffers->set(pos - 1, sqlBuffer);
			OCIBindByPos(stmt->handleStmt, &handleBind, stmt->handleError, pos, sqlBuffer,
							 ((sb4)ucs2_strlen((UCS2CHAR *)sqlBuffer) + 1) * sizeof(UCS2CHAR), 
							 SQLT_STR, NULL, NULL, NULL, 0, NULL, OCI_DEFAULT);
			if (allocType == DB_BIND_DYNAMIC)
				free(buffer);
			break;
		case DB_CTYPE_UINT64:	// OCI prior to 11.2 cannot bind 64 bit integers
#ifdef UNICODE_UCS2
			sqlBuffer = malloc(64 * sizeof(WCHAR));
			swprintf((WCHAR *)sqlBuffer, 64, UINT64_FMTW, *((QWORD *)buffer));
#else
			{
				char temp[64];
				snprintf(temp, 64, UINT64_FMTA, *((QWORD *)buffer));
				sqlBuffer = UCS2StringFromMBString(temp);
			}
#endif
			stmt->buffers->set(pos - 1, sqlBuffer);
			OCIBindByPos(stmt->handleStmt, &handleBind, stmt->handleError, pos, sqlBuffer,
							 ((sb4)ucs2_strlen((UCS2CHAR *)sqlBuffer) + 1) * sizeof(UCS2CHAR), 
							 SQLT_STR, NULL, NULL, NULL, 0, NULL, OCI_DEFAULT);
			if (allocType == DB_BIND_DYNAMIC)
				free(buffer);
			break;
		default:
			switch(allocType)
			{
				case DB_BIND_STATIC:
					sqlBuffer = buffer;
					break;
				case DB_BIND_DYNAMIC:
					sqlBuffer = buffer;
					stmt->buffers->set(pos - 1, buffer);
					break;
				case DB_BIND_TRANSIENT:
					sqlBuffer = nx_memdup(buffer, bufferSize[cType]);
					stmt->buffers->set(pos - 1, sqlBuffer);
					break;
				default:
					return;	// Invalid call
			}
			OCIBindByPos(stmt->handleStmt, &handleBind, stmt->handleError, pos, sqlBuffer, bufferSize[cType],
							 oracleType[cType], NULL, NULL, NULL, 0, NULL, OCI_DEFAULT);
			break;
	}
	stmt->bindings->set(pos - 1, handleBind);
}


//
// Execute prepared non-select statement
//

extern "C" DWORD EXPORT DrvExecute(ORACLE_CONN *pConn, ORACLE_STATEMENT *stmt, WCHAR *errorText)
{
	DWORD dwResult;

	MutexLock(pConn->mutexQueryLock);
	if (OCIStmtExecute(pConn->handleService, stmt->handleStmt, pConn->handleError, 1, 0, NULL, NULL,
	                   (pConn->nTransLevel == 0) ? OCI_COMMIT_ON_SUCCESS : OCI_DEFAULT) == OCI_SUCCESS)
	{
		dwResult = DBERR_SUCCESS;
	}
	else
	{
		SetLastErrorText(pConn);
		dwResult = IsConnectionError(pConn);
	}

	if (errorText != NULL)
	{
		wcsncpy(errorText, pConn->szLastError, DBDRV_MAX_ERROR_TEXT);
		errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
	}
	MutexUnlock(pConn->mutexQueryLock);
	return dwResult;
}

/**
 * Destroy prepared statement
 */
extern "C" void EXPORT DrvFreeStatement(ORACLE_STATEMENT *stmt)
{
	if (stmt == NULL)
		return;

	MutexLock(stmt->connection->mutexQueryLock);
	OCIStmtRelease(stmt->handleStmt, stmt->handleError, NULL, 0, OCI_DEFAULT);
	OCIHandleFree(stmt->handleError, OCI_HTYPE_ERROR);
	MutexUnlock(stmt->connection->mutexQueryLock);
	delete stmt->bindings;
	delete stmt->buffers;
	free(stmt);
}

/**
 * Perform non-SELECT query
 */
extern "C" DWORD EXPORT DrvQuery(ORACLE_CONN *pConn, WCHAR *pwszQuery, WCHAR *errorText)
{
	OCIStmt *handleStmt;
	DWORD dwResult;

#if UNICODE_UCS4
	UCS2CHAR *ucs2Query = UCS2StringFromUCS4String(pwszQuery);
#else
	UCS2CHAR *ucs2Query = pwszQuery;
#endif

	MutexLock(pConn->mutexQueryLock);
	if (OCIStmtPrepare2(pConn->handleService, &handleStmt, pConn->handleError, (text *)ucs2Query,
	                    (ub4)ucs2_strlen(ucs2Query) * sizeof(UCS2CHAR), NULL, 0, OCI_NTV_SYNTAX, OCI_DEFAULT) == OCI_SUCCESS)
	{
		if (OCIStmtExecute(pConn->handleService, handleStmt, pConn->handleError, 1, 0, NULL, NULL,
		                   (pConn->nTransLevel == 0) ? OCI_COMMIT_ON_SUCCESS : OCI_DEFAULT) == OCI_SUCCESS)
		{
			dwResult = DBERR_SUCCESS;
		}
		else
		{
			SetLastErrorText(pConn);
			dwResult = IsConnectionError(pConn);
		}
		OCIStmtRelease(handleStmt, pConn->handleError, NULL, 0, OCI_DEFAULT);
	}
	else
	{
		SetLastErrorText(pConn);
		dwResult = IsConnectionError(pConn);
	}
	if (errorText != NULL)
	{
		wcsncpy(errorText, pConn->szLastError, DBDRV_MAX_ERROR_TEXT);
		errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
	}
	MutexUnlock(pConn->mutexQueryLock);

#if UNICODE_UCS4
	free(ucs2Query);
#endif
	return dwResult;
}

/**
 * Process SELECT results
 */
static ORACLE_RESULT *ProcessQueryResults(ORACLE_CONN *pConn, OCIStmt *handleStmt, DWORD *pdwError)
{
	OCIParam *handleParam;
	OCIDefine *handleDefine;
	ub4 nCount;
	ub2 nWidth;
	sword nStatus;
	ORACLE_FETCH_BUFFER *pBuffers;
	text *colName;

	ORACLE_RESULT *pResult = (ORACLE_RESULT *)malloc(sizeof(ORACLE_RESULT));
	pResult->nRows = 0;
	pResult->pData = NULL;
	pResult->columnNames = NULL;
	OCIAttrGet(handleStmt, OCI_HTYPE_STMT, &nCount, NULL, OCI_ATTR_PARAM_COUNT, pConn->handleError);
	pResult->nCols = nCount;
	if (pResult->nCols > 0)
	{
		// Prepare receive buffers and fetch column names
		pResult->columnNames = (char **)malloc(sizeof(char *) * pResult->nCols);
		pBuffers = (ORACLE_FETCH_BUFFER *)malloc(sizeof(ORACLE_FETCH_BUFFER) * pResult->nCols);
		memset(pBuffers, 0, sizeof(ORACLE_FETCH_BUFFER) * pResult->nCols);
		for(int i = 0; i < pResult->nCols; i++)
		{
			if ((nStatus = OCIParamGet(handleStmt, OCI_HTYPE_STMT, pConn->handleError,
			                           (void **)&handleParam, (ub4)(i + 1))) == OCI_SUCCESS)
			{
				// Column name
				if (OCIAttrGet(handleParam, OCI_DTYPE_PARAM, &colName, &nCount, OCI_ATTR_NAME, pConn->handleError) == OCI_SUCCESS)
				{
					// We are in UTF-16 mode, so OCIAttrGet will return UTF-16 strings
					pResult->columnNames[i] = MBStringFromUCS2String((UCS2CHAR *)colName);
				}
				else
				{
					pResult->columnNames[i] = strdup("");
				}

				// Data size
				OCIAttrGet(handleParam, OCI_DTYPE_PARAM, &nWidth, NULL, OCI_ATTR_DATA_SIZE, pConn->handleError);
				/* FIXME: find a way to determine real CLOB size */
				if (nWidth == 4000)
					nWidth = 32737;  // CLOB up to 32K
				pBuffers[i].pData = (UCS2CHAR *)malloc((nWidth + 31) * sizeof(UCS2CHAR));
				handleDefine = NULL;
				if ((nStatus = OCIDefineByPos(handleStmt, &handleDefine, pConn->handleError, i + 1,
				                              pBuffers[i].pData, (nWidth + 31) * sizeof(UCS2CHAR),
				                              SQLT_CHR, &pBuffers[i].isNull, &pBuffers[i].nLength,
				                              &pBuffers[i].nCode, OCI_DEFAULT)) != OCI_SUCCESS)
				{
					SetLastErrorText(pConn);
					*pdwError = IsConnectionError(pConn);
				}
				OCIDescriptorFree(handleParam, OCI_DTYPE_PARAM);
			}
			else
			{
				SetLastErrorText(pConn);
				*pdwError = IsConnectionError(pConn);
			}
		}

		// Fetch data
		if (nStatus == OCI_SUCCESS)
		{
			int nPos = 0;
			while(1)
			{
				nStatus = OCIStmtFetch2(handleStmt, pConn->handleError, 1, OCI_FETCH_NEXT, 0, OCI_DEFAULT);
				if (nStatus == OCI_NO_DATA)
				{
					*pdwError = DBERR_SUCCESS;	// EOF
					break;
				}
				if ((nStatus != OCI_SUCCESS) && (nStatus != OCI_SUCCESS_WITH_INFO))
				{
					SetLastErrorText(pConn);
					*pdwError = IsConnectionError(pConn);
					break;
				}

				// New row
				pResult->nRows++;
				pResult->pData = (WCHAR **)realloc(pResult->pData, sizeof(WCHAR *) * pResult->nCols * pResult->nRows);
				for(int i = 0; i < pResult->nCols; i++)
				{
					if (pBuffers[i].isNull)
					{
						pResult->pData[nPos] = (WCHAR *)nx_memdup("\0\0\0", sizeof(WCHAR));
					}
					else
					{
						int length = pBuffers[i].nLength / sizeof(UCS2CHAR);
						pResult->pData[nPos] = (WCHAR *)malloc((length + 1) * sizeof(WCHAR));
#if UNICODE_UCS4
						ucs2_to_ucs4(pBuffers[i].pData, length, pResult->pData[nPos], length + 1);
#else
						memcpy(pResult->pData[nPos], pBuffers[i].pData, pBuffers[i].nLength);
#endif
						pResult->pData[nPos][length] = 0;
					}
					nPos++;
				}
			}
		}

		// Cleanup
		for(int i = 0; i < pResult->nCols; i++)
			safe_free(pBuffers[i].pData);
		free(pBuffers);

		// Destroy results in case of error
		if (*pdwError != DBERR_SUCCESS)
		{
			DestroyQueryResult(pResult);
			pResult = NULL;
		}
	}

	return pResult;
}

/**
 * Perform SELECT query
 */
extern "C" DBDRV_RESULT EXPORT DrvSelect(ORACLE_CONN *pConn, WCHAR *pwszQuery, DWORD *pdwError, WCHAR *errorText)
{
	ORACLE_RESULT *pResult = NULL;
	OCIStmt *handleStmt;

#if UNICODE_UCS4
	UCS2CHAR *ucs2Query = UCS2StringFromUCS4String(pwszQuery);
#else
	UCS2CHAR *ucs2Query = pwszQuery;
#endif

	MutexLock(pConn->mutexQueryLock);
	if (OCIStmtPrepare2(pConn->handleService, &handleStmt, pConn->handleError, (text *)ucs2Query,
	                    (ub4)ucs2_strlen(ucs2Query) * sizeof(UCS2CHAR), NULL, 0, OCI_NTV_SYNTAX, OCI_DEFAULT) == OCI_SUCCESS)
	{
		if (OCIStmtExecute(pConn->handleService, handleStmt, pConn->handleError,
		                   0, 0, NULL, NULL, (pConn->nTransLevel == 0) ? OCI_COMMIT_ON_SUCCESS : OCI_DEFAULT) == OCI_SUCCESS)
		{
			pResult = ProcessQueryResults(pConn, handleStmt, pdwError);
		}
		else
		{
			SetLastErrorText(pConn);
			*pdwError = IsConnectionError(pConn);
		}
		OCIStmtRelease(handleStmt, pConn->handleError, NULL, 0, OCI_DEFAULT);
	}
	else
	{
		SetLastErrorText(pConn);
		*pdwError = IsConnectionError(pConn);
	}
	if (errorText != NULL)
	{
		wcsncpy(errorText, pConn->szLastError, DBDRV_MAX_ERROR_TEXT);
		errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
	}
	MutexUnlock(pConn->mutexQueryLock);

#if UNICODE_UCS4
	free(ucs2Query);
#endif
	return pResult;
}

/**
 * Perform SELECT query using prepared statement
 */
extern "C" DBDRV_RESULT EXPORT DrvSelectPrepared(ORACLE_CONN *pConn, ORACLE_STATEMENT *stmt, DWORD *pdwError, WCHAR *errorText)
{
	ORACLE_RESULT *pResult = NULL;

	MutexLock(pConn->mutexQueryLock);
	if (OCIStmtExecute(pConn->handleService, stmt->handleStmt, pConn->handleError,
	                   0, 0, NULL, NULL, (pConn->nTransLevel == 0) ? OCI_COMMIT_ON_SUCCESS : OCI_DEFAULT) == OCI_SUCCESS)
	{
		pResult = ProcessQueryResults(pConn, stmt->handleStmt, pdwError);
	}
	else
	{
		SetLastErrorText(pConn);
		*pdwError = IsConnectionError(pConn);
	}

	if (errorText != NULL)
	{
		wcsncpy(errorText, pConn->szLastError, DBDRV_MAX_ERROR_TEXT);
		errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
	}
	MutexUnlock(pConn->mutexQueryLock);

	return pResult;
}

/**
 * Get field length from result
 */
extern "C" LONG EXPORT DrvGetFieldLength(ORACLE_RESULT *pResult, int nRow, int nColumn)
{
	if (pResult == NULL)
		return 0;

	if ((nRow >= 0) && (nRow < pResult->nRows) &&
		 (nColumn >= 0) && (nColumn < pResult->nCols))
		return (LONG)wcslen(pResult->pData[pResult->nCols * nRow + nColumn]);
	
	return 0;
}

/**
 * Get field value from result
 */
extern "C" WCHAR EXPORT *DrvGetField(ORACLE_RESULT *pResult, int nRow, int nColumn,
                                     WCHAR *pBuffer, int nBufLen)
{
   WCHAR *pValue = NULL;

   if (pResult != NULL)
   {
      if ((nRow < pResult->nRows) && (nRow >= 0) &&
          (nColumn < pResult->nCols) && (nColumn >= 0))
      {
#ifdef _WIN32
         wcsncpy_s(pBuffer, nBufLen, pResult->pData[nRow * pResult->nCols + nColumn], _TRUNCATE);
#else
         wcsncpy(pBuffer, pResult->pData[nRow * pResult->nCols + nColumn], nBufLen);
         pBuffer[nBufLen - 1] = 0;
#endif
         pValue = pBuffer;
      }
   }
   return pValue;
}


//
// Get number of rows in result
//

extern "C" int EXPORT DrvGetNumRows(ORACLE_RESULT *pResult)
{
	return (pResult != NULL) ? pResult->nRows : 0;
}


//
// Get column count in query result
//

extern "C" int EXPORT DrvGetColumnCount(ORACLE_RESULT *pResult)
{
	return (pResult != NULL) ? pResult->nCols : 0;
}


//
// Get column name in query result
//

extern "C" const char EXPORT *DrvGetColumnName(ORACLE_RESULT *pResult, int column)
{
	return ((pResult != NULL) && (column >= 0) && (column < pResult->nCols)) ? pResult->columnNames[column] : NULL;
}

/**
 * Free SELECT results
 */
extern "C" void EXPORT DrvFreeResult(ORACLE_RESULT *pResult)
{
	if (pResult != NULL)
		DestroyQueryResult(pResult);
}

/**
 * Perform asynchronous SELECT query
 */
extern "C" DBDRV_ASYNC_RESULT EXPORT DrvAsyncSelect(ORACLE_CONN *pConn, WCHAR *pwszQuery,
                                                 DWORD *pdwError, WCHAR *errorText)
{
	OCIParam *handleParam;
	OCIDefine *handleDefine;
	ub4 nCount;
	ub2 nWidth;
	text *colName;
	int i;

#if UNICODE_UCS4
	UCS2CHAR *ucs2Query = UCS2StringFromUCS4String(pwszQuery);
#else
	UCS2CHAR *ucs2Query = pwszQuery;
#endif

	MutexLock(pConn->mutexQueryLock);

	pConn->pBuffers = NULL;
	pConn->nCols = 0;

	if (OCIStmtPrepare2(pConn->handleService, &pConn->handleStmt, pConn->handleError, (text *)ucs2Query,
	                    (ub4)ucs2_strlen(ucs2Query) * sizeof(UCS2CHAR), NULL, 0, OCI_NTV_SYNTAX, OCI_DEFAULT) == OCI_SUCCESS)
	{
		if (OCIStmtExecute(pConn->handleService, pConn->handleStmt, pConn->handleError,
		                   0, 0, NULL, NULL, (pConn->nTransLevel == 0) ? OCI_COMMIT_ON_SUCCESS : OCI_DEFAULT) == OCI_SUCCESS)
		{
			OCIAttrGet(pConn->handleStmt, OCI_HTYPE_STMT, &nCount, NULL, OCI_ATTR_PARAM_COUNT, pConn->handleError);
			pConn->nCols = nCount;
			if (pConn->nCols > 0)
			{
				// Prepare receive buffers and fetch column names
				pConn->columnNames = (char **)malloc(sizeof(char *) * pConn->nCols);
				pConn->pBuffers = (ORACLE_FETCH_BUFFER *)malloc(sizeof(ORACLE_FETCH_BUFFER) * pConn->nCols);
				memset(pConn->pBuffers, 0, sizeof(ORACLE_FETCH_BUFFER) * pConn->nCols);
				for(i = 0; i < pConn->nCols; i++)
				{
					pConn->pBuffers[i].isNull = 1;	// Mark all columns as NULL initially
					if (OCIParamGet(pConn->handleStmt, OCI_HTYPE_STMT, pConn->handleError,
					                (void **)&handleParam, (ub4)(i + 1)) == OCI_SUCCESS)
					{
						// Column name
						if (OCIAttrGet(handleParam, OCI_DTYPE_PARAM, &colName, &nCount, OCI_ATTR_NAME, pConn->handleError) == OCI_SUCCESS)
						{
							// We are in UTF-16 mode, so OCIAttrGet will return UTF-16 strings
							pConn->columnNames[i] = MBStringFromUCS2String((UCS2CHAR *)colName);
						}
						else
						{
							pConn->columnNames[i] = strdup("");
						}

						// Data size
						OCIAttrGet(handleParam, OCI_DTYPE_PARAM, &nWidth, NULL, OCI_ATTR_DATA_SIZE, pConn->handleError);
						pConn->pBuffers[i].pData = (UCS2CHAR *)malloc((nWidth + 31) * sizeof(UCS2CHAR));
						handleDefine = NULL;
						if (OCIDefineByPos(pConn->handleStmt, &handleDefine, pConn->handleError, i + 1,
						                   pConn->pBuffers[i].pData, (nWidth + 31) * sizeof(UCS2CHAR),
						                   SQLT_CHR, &pConn->pBuffers[i].isNull, &pConn->pBuffers[i].nLength,
						                   &pConn->pBuffers[i].nCode, OCI_DEFAULT) == OCI_SUCCESS)
						{
							*pdwError = DBERR_SUCCESS;
						}
						else
						{
							SetLastErrorText(pConn);
							*pdwError = IsConnectionError(pConn);
						}

						OCIDescriptorFree(handleParam, OCI_DTYPE_PARAM);
					}
					else
					{
						SetLastErrorText(pConn);
						*pdwError = IsConnectionError(pConn);
					}
				}
			}
		}
		else
		{
			SetLastErrorText(pConn);
			*pdwError = IsConnectionError(pConn);
		}
	}
	else
	{
		SetLastErrorText(pConn);
		*pdwError = IsConnectionError(pConn);
	}

#if UNICODE_UCS4
	free(ucs2Query);
#endif

	if (*pdwError == DBERR_SUCCESS)
		return pConn;

	// On failure, unlock query mutex and do cleanup
	OCIStmtRelease(pConn->handleStmt, pConn->handleError, NULL, 0, OCI_DEFAULT);
	for(i = 0; i < pConn->nCols; i++)
		safe_free(pConn->pBuffers[i].pData);
	safe_free(pConn->pBuffers);
	if (errorText != NULL)
	{
		wcsncpy(errorText, pConn->szLastError, DBDRV_MAX_ERROR_TEXT);
		errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
	}
	MutexUnlock(pConn->mutexQueryLock);
	return NULL;
}


//
// Fetch next result line from asynchronous SELECT results
//

extern "C" BOOL EXPORT DrvFetch(ORACLE_CONN *pConn)
{
	BOOL success;

	if (pConn == NULL)
		return FALSE;

	sword rc = OCIStmtFetch2(pConn->handleStmt, pConn->handleError, 1, OCI_FETCH_NEXT, 0, OCI_DEFAULT);
	if ((rc == OCI_SUCCESS) || (rc == OCI_SUCCESS_WITH_INFO))
	{
		success = TRUE;
	}
	else
	{
		SetLastErrorText(pConn);
		success = FALSE;
	}
	return success;
}


//
// Get field length from current row in async query result
//

extern "C" LONG EXPORT DrvGetFieldLengthAsync(ORACLE_CONN *pConn, int nColumn)
{
	if (pConn == NULL)
		return 0;

	if ((nColumn < 0) || (nColumn >= pConn->nCols))
		return 0;

	if (pConn->pBuffers[nColumn].isNull)
		return 0;

	return (LONG)(pConn->pBuffers[nColumn].nLength / sizeof(UCS2CHAR));
}


//
// Get field from current row in async query result
//

extern "C" WCHAR EXPORT *DrvGetFieldAsync(ORACLE_CONN *pConn, int nColumn,
                                          WCHAR *pBuffer, int nBufSize)
{
	int nLen;

	if (pConn == NULL)
		return NULL;

	if ((nColumn < 0) || (nColumn >= pConn->nCols))
		return NULL;

	if (pConn->pBuffers[nColumn].isNull)
	{
		*pBuffer = 0;
	}
	else
	{
		nLen = min(nBufSize - 1, ((int)(pConn->pBuffers[nColumn].nLength / sizeof(UCS2CHAR))));
#if UNICODE_UCS4
		ucs2_to_ucs4(pConn->pBuffers[nColumn].pData, nLen, pBuffer, nLen + 1);
#else
		memcpy(pBuffer, pConn->pBuffers[nColumn].pData, nLen * sizeof(WCHAR));
#endif
		pBuffer[nLen] = 0;
	}

	return pBuffer;
}


//
// Get column count in async query result
//

extern "C" int EXPORT DrvGetColumnCountAsync(ORACLE_CONN *pConn)
{
	return (pConn != NULL) ? pConn->nCols : 0;
}


//
// Get column name in async query result
//

extern "C" const char EXPORT *DrvGetColumnNameAsync(ORACLE_CONN *pConn, int column)
{
	return ((pConn != NULL) && (column >= 0) && (column < pConn->nCols)) ? pConn->columnNames[column] : NULL;
}


//
// Destroy result of async query
//

extern "C" void EXPORT DrvFreeAsyncResult(ORACLE_CONN *pConn)
{
	int i;

	if (pConn == NULL)
		return;

	OCIStmtRelease(pConn->handleStmt, pConn->handleError, NULL, 0, OCI_DEFAULT);

	for(i = 0; i < pConn->nCols; i++)
		safe_free(pConn->pBuffers[i].pData);
	safe_free_and_null(pConn->pBuffers);

	for(i = 0; i < pConn->nCols; i++)
		safe_free(pConn->columnNames[i]);
	safe_free_and_null(pConn->columnNames);

	pConn->nCols = 0;
	MutexUnlock(pConn->mutexQueryLock);
}


//
// Begin transaction
//

extern "C" DWORD EXPORT DrvBegin(ORACLE_CONN *pConn)
{
	if (pConn == NULL)
		return DBERR_INVALID_HANDLE;

	MutexLock(pConn->mutexQueryLock);
	pConn->nTransLevel++;
	MutexUnlock(pConn->mutexQueryLock);
	return DBERR_SUCCESS;
}


//
// Commit transaction
//

extern "C" DWORD EXPORT DrvCommit(ORACLE_CONN *pConn)
{
	DWORD dwResult;

	if (pConn == NULL)
		return DBERR_INVALID_HANDLE;

	MutexLock(pConn->mutexQueryLock);
	if (pConn->nTransLevel > 0)
	{
		if (OCITransCommit(pConn->handleService, pConn->handleError, OCI_DEFAULT) == OCI_SUCCESS)
		{
			dwResult = DBERR_SUCCESS;
			pConn->nTransLevel = 0;
		}
		else
		{
			SetLastErrorText(pConn);
			dwResult = IsConnectionError(pConn);
		}
	}
	else
	{
		dwResult = DBERR_SUCCESS;
	}
	MutexUnlock(pConn->mutexQueryLock);
	return dwResult;
}


//
// Rollback transaction
//

extern "C" DWORD EXPORT DrvRollback(ORACLE_CONN *pConn)
{
	DWORD dwResult;

	if (pConn == NULL)
		return DBERR_INVALID_HANDLE;

	MutexLock(pConn->mutexQueryLock);
	if (pConn->nTransLevel > 0)
	{
		if (OCITransRollback(pConn->handleService, pConn->handleError, OCI_DEFAULT) == OCI_SUCCESS)
		{
			dwResult = DBERR_SUCCESS;
			pConn->nTransLevel = 0;
		}
		else
		{
			SetLastErrorText(pConn);
			dwResult = IsConnectionError(pConn);
		}
	}
	else
	{
		dwResult = DBERR_SUCCESS;
	}
	MutexUnlock(pConn->mutexQueryLock);
	return dwResult;
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
