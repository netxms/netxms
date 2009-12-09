/* $Id$ */
/* 
** Oracle Database Driver
** Copyright (C) 2007, 2008, 2009 Victor Kirhenshtein
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


//
// API version
//

extern "C" int EXPORT drvAPIVersion = DBDRV_API_VERSION;
extern "C" const char EXPORT *drvName = "ORACLE";


//
// Prepare string for using in SQL query - enclose in quotes and escape as needed
//

extern "C" TCHAR EXPORT *DrvPrepareString(const TCHAR *str)
{
	int len = (int)_tcslen(str) + 3;   // + two quotes and \0 at the end
	int bufferSize = len + 128;
	TCHAR *out = (TCHAR *)malloc(bufferSize * sizeof(TCHAR));
	out[0] = _T('\'');

	const TCHAR *src = str;
	int outPos;
	for(outPos = 1; *src != 0; src++)
	{
		if (*src == _T('\''))
		{
			len++;
			if (len >= bufferSize)
			{
				bufferSize += 128;
				out = (TCHAR *)realloc(out, bufferSize * sizeof(TCHAR));
			}
			out[outPos++] = _T('\'');
			out[outPos++] = _T('\'');
		}
		else
		{
			out[outPos++] = *src;
		}
	}
	out[outPos++] = _T('\'');
	out[outPos++] = 0;

	return out;
}


//
// Initialize driver
//

extern "C" BOOL EXPORT DrvInit(char *szCmdLine)
{
	return TRUE;
}

//
// Unload handler
//

extern "C" void EXPORT DrvUnload(void)
{
	OCITerminate(OCI_DEFAULT);
}


//
// Set last error text
//

static void SetLastErrorText(ORACLE_CONN *pConn)
{
	sb4 nCode;

#ifdef UNICODE
	OCIErrorGet(pConn->handleError, 1, NULL, &nCode, (text *)pConn->szLastError,
	            DBDRV_MAX_ERROR_TEXT, OCI_HTYPE_ERROR);
	pConn->szLastError[DBDRV_MAX_ERROR_TEXT - 1] = 0;
#else
	UCS2CHAR buffer[DBDRV_MAX_ERROR_TEXT];

	OCIErrorGet(pConn->handleError, 1, NULL, &nCode, (text *)buffer,
	            DBDRV_MAX_ERROR_TEXT, OCI_HTYPE_ERROR);
	buffer[DBDRV_MAX_ERROR_TEXT - 1] = 0;
#if UNICODE_UCS4
	ucs2_to_mb(buffer, ucs2_strlen(buffer) + 1, pConn->szLastError, DBDRV_MAX_ERROR_TEXT);
	pConn->szLastError[DBDRV_MAX_ERROR_TEXT - 1] = 0;
#else
	WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, buffer, -1,
	                    pConn->szLastError, DBDRV_MAX_ERROR_TEXT, NULL, NULL);
#endif
#endif
	RemoveTrailingCRLF(pConn->szLastError);
}


//
// Check if last error was caused by lost connection to server
//

static DWORD IsConnectionError(ORACLE_CONN *pConn)
{
	ub4 nStatus = 0;

	OCIAttrGet(pConn->handleServer, OCI_HTYPE_SERVER, &nStatus,
	           NULL, OCI_ATTR_SERVER_STATUS, pConn->handleError);
	return (nStatus == OCI_SERVER_NOT_CONNECTED) ? DBERR_CONNECTION_LOST : DBERR_OTHER_ERROR;
}


//
// Destroy query result
//

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


//
// Connect to database
//

extern "C" DBDRV_CONNECTION EXPORT DrvConnect(char *pszHost, char *pszLogin,
                                           char *pszPassword, char *pszDatabase)
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
			pwszStr = UCS2StringFromMBString(pszHost);
			if (OCIServerAttach(pConn->handleServer, pConn->handleError,
			                    (text *)pwszStr, (sb4)ucs2_strlen(pwszStr) * sizeof(UCS2CHAR), OCI_DEFAULT) == OCI_SUCCESS)
			{
				free(pwszStr);

				// Initialize service handle
				OCIHandleAlloc(pConn->handleEnv, (void **)&pConn->handleService, OCI_HTYPE_SVCCTX, 0, NULL);
				OCIAttrSet(pConn->handleService, OCI_HTYPE_SVCCTX, pConn->handleServer, 0, OCI_ATTR_SERVER, pConn->handleError);
				
				// Initialize session handle
				OCIHandleAlloc(pConn->handleEnv, (void **)&pConn->handleSession, OCI_HTYPE_SESSION, 0, NULL);
				pwszStr = UCS2StringFromMBString(pszLogin);
				OCIAttrSet(pConn->handleSession, OCI_HTYPE_SESSION, pwszStr,
				           (ub4)ucs2_strlen(pwszStr) * sizeof(UCS2CHAR), OCI_ATTR_USERNAME, pConn->handleError);
				free(pwszStr);
				pwszStr = UCS2StringFromMBString(pszPassword);
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
				}
				else
				{
/*
	text error[1024];
	sb4 nCode;
	memset(error, 0, sizeof(error));
	OCIErrorGet(pConn->handleError, 1, NULL, &nCode, (text *)error, DBDRV_MAX_ERROR_TEXT, OCI_HTYPE_ERROR);
	printf("%s\n", MBStringFromUCS2String((UCS2CHAR *)error));
*/
					OCIHandleFree(pConn->handleEnv, OCI_HTYPE_ENV);
					free(pConn);
					pConn = NULL;
				}
			}
			else
			{
/*
	text error[1024];
	sb4 nCode;
	memset(error, 0, sizeof(error));
	OCIErrorGet(pConn->handleError, 1, NULL, &nCode, (text *)error, DBDRV_MAX_ERROR_TEXT, OCI_HTYPE_ERROR);
	printf("%s\n", MBStringFromUCS2String((UCS2CHAR *)error));
*/
				OCIHandleFree(pConn->handleEnv, OCI_HTYPE_ENV);
				free(pConn);
				pConn = NULL;
			}
			free(pwszStr);
		}
		else
		{
			free(pConn);
			pConn = NULL;
		}
	}

   return (DBDRV_CONNECTION)pConn;
}


//
// Disconnect from database
//

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


//
// Perform non-SELECT query
//

extern "C" DWORD EXPORT DrvQuery(ORACLE_CONN *pConn, WCHAR *pwszQuery, TCHAR *errorText)
{
	OCIStmt *handleStmt;
	DWORD dwResult;

#if UNICODE_UCS4
	UCS2CHAR *ucs2Query = UCS2StringFromUCS4String(pwszQuery);
#else
	UCS2CHAR *ucs2Query = pwszQuery;
#endif

	MutexLock(pConn->mutexQueryLock, INFINITE);
	OCIHandleAlloc(pConn->handleEnv, (void **)&handleStmt, OCI_HTYPE_STMT, 0, NULL);
	if (OCIStmtPrepare(handleStmt, pConn->handleError, (text *)ucs2Query,
	                   (ub4)ucs2_strlen(ucs2Query) * sizeof(UCS2CHAR), OCI_NTV_SYNTAX, OCI_DEFAULT) == OCI_SUCCESS)
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
	}
	else
	{
		dwResult = IsConnectionError(pConn);
	}
	OCIHandleFree(handleStmt, OCI_HTYPE_STMT);
	if (errorText != NULL)
		nx_strncpy(errorText, pConn->szLastError, DBDRV_MAX_ERROR_TEXT);
	MutexUnlock(pConn->mutexQueryLock);

#if UNICODE_UCS4
	free(ucs2Query);
#endif
	return dwResult;
}


//
// Perform SELECT query
//

extern "C" DBDRV_RESULT EXPORT DrvSelect(ORACLE_CONN *pConn, WCHAR *pwszQuery, DWORD *pdwError, TCHAR *errorText)
{
	ORACLE_RESULT *pResult = NULL;
	OCIStmt *handleStmt;
	OCIParam *handleParam;
	OCIDefine *handleDefine;
	ub4 nCount;
	ub2 nWidth;
	sword nStatus;
	ORACLE_FETCH_BUFFER *pBuffers;
	text *colName;
	int i, nPos;

#if UNICODE_UCS4
	UCS2CHAR *ucs2Query = UCS2StringFromUCS4String(pwszQuery);
#else
	UCS2CHAR *ucs2Query = pwszQuery;
#endif

	MutexLock(pConn->mutexQueryLock, INFINITE);
	OCIHandleAlloc(pConn->handleEnv, (void **)&handleStmt, OCI_HTYPE_STMT, 0, NULL);
	if (OCIStmtPrepare(handleStmt, pConn->handleError, (text *)ucs2Query,
	                   (ub4)ucs2_strlen(ucs2Query) * sizeof(UCS2CHAR), OCI_NTV_SYNTAX, OCI_DEFAULT) == OCI_SUCCESS)
	{
		if (OCIStmtExecute(pConn->handleService, handleStmt, pConn->handleError,
		                   0, 0, NULL, NULL, (pConn->nTransLevel == 0) ? OCI_COMMIT_ON_SUCCESS : OCI_DEFAULT) == OCI_SUCCESS)
		{
			pResult = (ORACLE_RESULT *)malloc(sizeof(ORACLE_RESULT));
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
				for(i = 0; i < pResult->nCols; i++)
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
					nPos = 0;
					while(1)
					{
						nStatus = OCIStmtFetch(handleStmt, pConn->handleError, 1, OCI_FETCH_NEXT, OCI_DEFAULT);
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
						for(i = 0; i < pResult->nCols; i++)
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
								ucs2_to_ucs4(pBuffers[i].pData, length, pResult->pData[nPos], length);
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
				for(i = 0; i < pResult->nCols; i++)
					safe_free(pBuffers[i].pData);
				free(pBuffers);

				// Destroy results in case of error
				if (*pdwError != DBERR_SUCCESS)
				{
					DestroyQueryResult(pResult);
					pResult = NULL;
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
	OCIHandleFree(handleStmt, OCI_HTYPE_STMT);
	if (errorText != NULL)
		nx_strncpy(errorText, pConn->szLastError, DBDRV_MAX_ERROR_TEXT);
	MutexUnlock(pConn->mutexQueryLock);

#if UNICODE_UCS4
	free(ucs2Query);
#endif
	return pResult;
}


//
// Get field length from result
//

extern "C" LONG EXPORT DrvGetFieldLength(ORACLE_RESULT *pResult, int nRow, int nColumn)
{
	if (pResult == NULL)
		return 0;

	if ((nRow >= 0) && (nRow < pResult->nRows) &&
		 (nColumn >= 0) && (nColumn < pResult->nCols))
		return (LONG)wcslen(pResult->pData[pResult->nCols * nRow + nColumn]);
	
	return 0;
}


//
// Get field value from result
//

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


//
// Free SELECT results
//

extern "C" void EXPORT DrvFreeResult(ORACLE_RESULT *pResult)
{
	if (pResult != NULL)
		DestroyQueryResult(pResult);
}


//
// Perform asynchronous SELECT query
//

extern "C" DBDRV_ASYNC_RESULT EXPORT DrvAsyncSelect(ORACLE_CONN *pConn, WCHAR *pwszQuery,
                                                 DWORD *pdwError, TCHAR *errorText)
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

	MutexLock(pConn->mutexQueryLock, INFINITE);

	pConn->pBuffers = NULL;
	pConn->nCols = 0;

	OCIHandleAlloc(pConn->handleEnv, (void **)&pConn->handleStmt, OCI_HTYPE_STMT, 0, NULL);
	if (OCIStmtPrepare(pConn->handleStmt, pConn->handleError, (text *)ucs2Query,
	                   (ub4)ucs2_strlen(ucs2Query) * sizeof(UCS2CHAR), OCI_NTV_SYNTAX, OCI_DEFAULT) == OCI_SUCCESS)
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
	for(i = 0; i < pConn->nCols; i++)
		safe_free(pConn->pBuffers[i].pData);
	safe_free(pConn->pBuffers);
	if (errorText != NULL)
		nx_strncpy(errorText, pConn->szLastError, DBDRV_MAX_ERROR_TEXT);
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

	sword rc = OCIStmtFetch(pConn->handleStmt, pConn->handleError, 1, OCI_FETCH_NEXT, OCI_DEFAULT);
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
		nLen = min(nBufSize - 1, (int)(pConn->pBuffers[nColumn].nLength / sizeof(UCS2CHAR)));
#if UNICODE_UCS4
		ucs2_to_ucs4(pConn->pBuffers[nColumn].pData, nLen, pBuffer, nLen);
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

	MutexLock(pConn->mutexQueryLock, INFINITE);
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

	MutexLock(pConn->mutexQueryLock, INFINITE);
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

	MutexLock(pConn->mutexQueryLock, INFINITE);
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
