/* 
** MySQL Database Driver
** Copyright (C) 2003-2009 Victor Kirhenshtein
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


//
// API version
//

extern "C" int EXPORT drvAPIVersion;
int EXPORT drvAPIVersion = DBDRV_API_VERSION;


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
}


//
// Connect to database
//

extern "C" DB_CONNECTION EXPORT DrvConnect(char *szHost, char *szLogin, char *szPassword,
										   char *szDatabase)
{
	MYSQL *pMySQL;
	MYSQL_CONN *pConn;
	char *pHost = szHost;
	char *pSocket = NULL;
	
	pMySQL = mysql_init(NULL);
	if (pMySQL == NULL)
	{
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
		mysql_close(pMySQL);
		return NULL;
	}
	
	pConn = (MYSQL_CONN *)malloc(sizeof(MYSQL_CONN));
	pConn->pMySQL = pMySQL;
	pConn->mutexQueryLock = MutexCreate();

   // Switch to UTF-8 encoding
   mysql_query(pMySQL, "SET NAMES 'utf8'");
	
	return (DB_CONNECTION)pConn;
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
// Perform actual non-SELECT query
//

static DWORD DrvQueryInternal(MYSQL_CONN *pConn, const char *pszQuery, TCHAR *errorText)
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
		
		if (errorText != NULL)
		{
			nx_strncpy(errorText, mysql_error(pConn->pMySQL), DBDRV_MAX_ERROR_TEXT);
			RemoveTrailingCRLF(errorText);
		}
	}

	MutexUnlock(pConn->mutexQueryLock);
	return dwRet;
}


//
// Perform non-SELECT query
//

extern "C" DWORD EXPORT DrvQuery(MYSQL_CONN *pConn, WCHAR *pwszQuery, TCHAR *errorText)
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

extern "C" DB_RESULT EXPORT DrvSelect(MYSQL_CONN *pConn, WCHAR *pwszQuery, DWORD *pdwError, TCHAR *errorText)
{
	DB_RESULT pResult = NULL;
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
		pResult = (DB_RESULT)mysql_store_result(pConn->pMySQL);
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
			nx_strncpy(errorText, mysql_error(pConn->pMySQL), DBDRV_MAX_ERROR_TEXT);
			RemoveTrailingCRLF(errorText);
		}
	}

	MutexUnlock(pConn->mutexQueryLock);
	free(pszQueryUTF8);
	
	return pResult;
}


//
// Get field length from result
//

extern "C" LONG EXPORT DrvGetFieldLength(DB_RESULT hResult, int iRow, int iColumn)
{
	MYSQL_ROW row;
	
	mysql_data_seek((MYSQL_RES *)hResult, iRow);
	row = mysql_fetch_row((MYSQL_RES *)hResult);
	return (row == NULL) ? (LONG)-1 : (LONG)strlen(row[iColumn]);
}


//
// Get field value from result
//

extern "C" WCHAR EXPORT *DrvGetField(DB_RESULT hResult, int iRow, int iColumn,
                                     WCHAR *pBuffer, int nBufSize)
{
	MYSQL_ROW row;
	WCHAR *pRet = NULL;
	
	mysql_data_seek((MYSQL_RES *)hResult, iRow);
	row = mysql_fetch_row((MYSQL_RES *)hResult);
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

extern "C" int EXPORT DrvGetNumRows(DB_RESULT hResult)
{
	return (hResult != NULL) ? (int)mysql_num_rows((MYSQL_RES *)hResult) : 0;
}


//
// Get column count in query result
//

extern "C" int EXPORT DrvGetColumnCount(DB_RESULT hResult)
{
	return (hResult != NULL) ? (int)mysql_num_fields((MYSQL_RES *)hResult) : 0;
}


//
// Get column name in query result
//

extern "C" const char EXPORT *DrvGetColumnName(DB_RESULT hResult, int column)
{
	MYSQL_FIELD *field;

	if (hResult == NULL)
		return NULL;

	field = mysql_fetch_field_direct((MYSQL_RES *)hResult, column);
	return (field != NULL) ? field->name : NULL;
}


//
// Free SELECT results
//

extern "C" void EXPORT DrvFreeResult(DB_RESULT hResult)
{
	mysql_free_result((MYSQL_RES *)hResult);
}


//
// Perform asynchronous SELECT query
//

extern "C" DB_ASYNC_RESULT EXPORT DrvAsyncSelect(MYSQL_CONN *pConn, WCHAR *pwszQuery,
                                                 DWORD *pdwError, TCHAR *errorText)
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
			nx_strncpy(errorText, mysql_error(pConn->pMySQL), DBDRV_MAX_ERROR_TEXT);
			RemoveTrailingCRLF(errorText);
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

extern "C" BOOL EXPORT DrvFetch(DB_ASYNC_RESULT hResult)
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

extern "C" LONG EXPORT DrvGetFieldLengthAsync(DB_ASYNC_RESULT hResult, int iColumn)
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

extern "C" WCHAR EXPORT *DrvGetFieldAsync(DB_ASYNC_RESULT hResult, int iColumn,
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

extern "C" int EXPORT DrvGetColumnCountAsync(DB_ASYNC_RESULT hResult)
{
	return ((hResult != NULL) && (((MYSQL_ASYNC_RESULT *)hResult)->pHandle != NULL))? (int)mysql_num_fields(((MYSQL_ASYNC_RESULT *)hResult)->pHandle) : 0;
}


//
// Get column name in async query result
//

extern "C" const char EXPORT *DrvGetColumnNameAsync(DB_ASYNC_RESULT hResult, int column)
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

extern "C" void EXPORT DrvFreeAsyncResult(DB_ASYNC_RESULT hResult)
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
