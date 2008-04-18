/* 
** SQLite Database Driver
** Copyright (C) 2005, 2006, 2007, 2008 Victor Kirhenshtein
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
** File: sqlite.cpp
**
**/

#include "sqlitedrv.h"


//
// API version
//

extern "C" int EXPORT drvAPIVersion;
int EXPORT drvAPIVersion = DBDRV_API_VERSION;


//
// SELECT callback
//

static int SelectCallback(void *pArg, int nCols, char **ppszData, char **ppszNames)
{
   int i, nPos, nMaxCol;

   if (((SQLITE_RESULT *)pArg)->nCols == 0)
   {
      ((SQLITE_RESULT *)pArg)->nCols = nCols;
      nMaxCol = nCols;
   }
   else
   {
      nMaxCol = min(((SQLITE_RESULT *)pArg)->nCols, nCols);
   }

   nPos = ((SQLITE_RESULT *)pArg)->nRows * ((SQLITE_RESULT *)pArg)->nCols;
   ((SQLITE_RESULT *)pArg)->nRows++;
   ((SQLITE_RESULT *)pArg)->ppszData = (char **)realloc(((SQLITE_RESULT *)pArg)->ppszData,
         sizeof(char *) * ((SQLITE_RESULT *)pArg)->nCols * ((SQLITE_RESULT *)pArg)->nRows);

   for(i = 0; i < nMaxCol; i++, nPos++)
      ((SQLITE_RESULT *)pArg)->ppszData[nPos] = strdup(CHECK_NULL_EX(ppszData[i]));
   for(; i < ((SQLITE_RESULT *)pArg)->nCols; i++, nPos++)
      ((SQLITE_RESULT *)pArg)->ppszData[nPos] = strdup("");
   return 0;
}


//
// Worker thread
//

static THREAD_RESULT THREAD_CALL SQLiteWorkerThread(void *pArg)
{
   SQLITE_CONN *pConn = (SQLITE_CONN *)pArg;
   const char *pszTail;
   BOOL bRun = TRUE;

   // Wait for requests
   do
   {
      ConditionWait(pConn->condCommand, INFINITE);
      switch(pConn->nCommand)
      {
         case SQLITE_DRV_OPEN:
            pConn->nResult = sqlite3_open(pConn->pszQuery, &pConn->pdb);
            if (pConn->nResult == SQLITE_OK)
            {
               sqlite3_busy_timeout(pConn->pdb, 30000);  // 30 sec. busy timeout
            }
            break;
         case SQLITE_DRV_CLOSE:
            if (pConn->pdb != NULL)
            {
               pConn->nResult = sqlite3_close(pConn->pdb);
               if (pConn->nResult == SQLITE_OK)
                  pConn->pdb = NULL;
            }
            bRun = FALSE;
            break;
         case SQLITE_DRV_QUERY:
            pConn->nResult = sqlite3_exec(pConn->pdb, pConn->pszQuery, NULL, NULL, NULL);
            break;
         case SQLITE_DRV_SELECT:
            pConn->pResult = (SQLITE_RESULT *)malloc(sizeof(SQLITE_RESULT));
            memset(pConn->pResult, 0, sizeof(SQLITE_RESULT));
            pConn->nResult = sqlite3_exec(pConn->pdb, pConn->pszQuery,
                                          SelectCallback, pConn->pResult, NULL);
            if (pConn->nResult != SQLITE_OK)
            {
               safe_free(pConn->pResult->ppszData);
               free(pConn->pResult);
               pConn->pResult = NULL;
            }
            break;
         case SQLITE_DRV_PREPARE:
            pConn->nResult = sqlite3_prepare(pConn->pdb, pConn->pszQuery, -1,
                                             &pConn->pvm, &pszTail);
            break;
         case SQLITE_DRV_STEP:
            pConn->nResult = sqlite3_step(pConn->pvm);
            if (pConn->nResult == SQLITE_ROW)
               pConn->nNumCols = sqlite3_column_count(pConn->pvm);
            break;
         case SQLITE_DRV_FINALIZE:
            pConn->nResult = sqlite3_finalize(pConn->pvm);
            if (pConn->nResult == SQLITE_OK)
               pConn->pvm = NULL;
            break;
         default:    // Unknown command
            pConn->nResult = SQLITE_ERROR;
            break;
      }
      
      if (pConn->pszErrorText != NULL)
      {
      	if (pConn->nResult == SQLITE_OK)
      	{
      		*pConn->pszErrorText = 0;
      	}
      	else
      	{
#ifdef UNICODE
				MultiByteToWideChar(CP_UTF8, 0, sqlite3_errmsg(pConn->pdb), -1, pConn->pszErrorText, DBDRV_MAX_ERROR_TEXT);
#else      	
				WCHAR *wtemp;
		   	
				wtemp = WideStringFromUTF8String(sqlite3_errmsg(pConn->pdb));
				WideCharToMultiByte(CP_ACP,  WC_COMPOSITECHECK | WC_DEFAULTCHAR, wtemp, -1, pConn->pszErrorText, DBDRV_MAX_ERROR_TEXT, NULL, NULL);
				free(wtemp);
#endif
				pConn->pszErrorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
			}
      }
      	
      ConditionSet(pConn->condResult);
   } while(bRun);
   return THREAD_OK;
}


//
// Execute command by worker thread
//

static int ExecCommand(SQLITE_CONN *pConn, int nCmd, char *pszQuery, TCHAR *errorText)
{
   pConn->pszQuery = pszQuery;
   pConn->nCommand = nCmd;
   pConn->pszErrorText = errorText;
   ConditionSet(pConn->condCommand);
   ConditionWait(pConn->condResult, INFINITE);
   return pConn->nResult;
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
}


//
// Disconnect from database
//

extern "C" void EXPORT DrvDisconnect(SQLITE_CONN *hConn)
{
   if (hConn != NULL)
   {
      ExecCommand(hConn, SQLITE_DRV_CLOSE, NULL, NULL);
      ThreadJoin(hConn->hThread);
      ConditionDestroy(hConn->condCommand);
      ConditionDestroy(hConn->condResult);
      MutexDestroy(hConn->mutexQueryLock);
      free(hConn);
   }
}


//
// Connect to database
//

extern "C" DB_CONNECTION EXPORT DrvConnect(char *pszHost, char *pszLogin,
                                       char *pszPassword, char *pszDatabase)
{
   SQLITE_CONN *pConn;

   // Create new handle
   pConn = (SQLITE_CONN *)malloc(sizeof(SQLITE_CONN));
   memset(pConn, 0, sizeof(SQLITE_CONN));
   pConn->mutexQueryLock = MutexCreate();
   pConn->condCommand = ConditionCreate(FALSE);
   pConn->condResult = ConditionCreate(FALSE);
   pConn->hThread = ThreadCreateEx(SQLiteWorkerThread, 0, pConn);

   if (ExecCommand(pConn, SQLITE_DRV_OPEN, pszDatabase, NULL) != SQLITE_OK)
   {
      DrvDisconnect(pConn);
      pConn = NULL;
   }
   return (DB_CONNECTION)pConn;
}


//
// Internal query
//

static DWORD DrvQueryInternal(SQLITE_CONN *pConn, char *pszQuery, TCHAR *errorText)
{
   BOOL bResult;

   MutexLock(pConn->mutexQueryLock, INFINITE);
   bResult = (ExecCommand(pConn, SQLITE_DRV_QUERY, pszQuery, errorText) == SQLITE_OK);
   MutexUnlock(pConn->mutexQueryLock);
   return bResult ? DBERR_SUCCESS : DBERR_OTHER_ERROR;
}


//
// Perform non-SELECT query
//

extern "C" DWORD EXPORT DrvQuery(SQLITE_CONN *pConn, WCHAR *pwszQuery, TCHAR *errorText)
{
   DWORD dwResult;
   char *pszQueryUTF8;

   pszQueryUTF8 = UTF8StringFromWideString(pwszQuery);
   dwResult = DrvQueryInternal(pConn, pszQueryUTF8, errorText);
   free(pszQueryUTF8);
   return dwResult;
}


//
// Perform SELECT query
//

extern "C" DB_RESULT EXPORT DrvSelect(SQLITE_CONN *hConn, WCHAR *pwszQuery, DWORD *pdwError, TCHAR *errorText)
{
   DB_RESULT pResult = NULL;
   char *pszQueryUTF8;

   pszQueryUTF8 = UTF8StringFromWideString(pwszQuery);
   MutexLock(hConn->mutexQueryLock, INFINITE);
   if (ExecCommand(hConn, SQLITE_DRV_SELECT, pszQueryUTF8, errorText) == SQLITE_OK)
   {
      pResult = hConn->pResult;
   }
   MutexUnlock(hConn->mutexQueryLock);
   free(pszQueryUTF8);
   *pdwError = (pResult != NULL) ? DBERR_SUCCESS : DBERR_OTHER_ERROR;
   return pResult;
}


//
// Get field length from result
//

extern "C" LONG EXPORT DrvGetFieldLength(DB_RESULT hResult, int iRow, int iColumn)
{
   if ((iRow < ((SQLITE_RESULT *)hResult)->nRows) &&
       (iColumn < ((SQLITE_RESULT *)hResult)->nCols) &&
       (iRow >= 0) && (iColumn >= 0))
      return strlen(((SQLITE_RESULT *)hResult)->ppszData[iRow * ((SQLITE_RESULT *)hResult)->nCols + iColumn]);
   return -1;
}


//
// Get field value from result
//

extern "C" WCHAR EXPORT *DrvGetField(DB_RESULT hResult, int iRow, int iColumn,
                                     WCHAR *pwszBuffer, int nBufLen)
{
   if ((iRow < ((SQLITE_RESULT *)hResult)->nRows) &&
       (iColumn < ((SQLITE_RESULT *)hResult)->nCols) &&
       (iRow >= 0) && (iColumn >= 0))
   {
      MultiByteToWideChar(CP_UTF8, 0, ((SQLITE_RESULT *)hResult)->ppszData[iRow * ((SQLITE_RESULT *)hResult)->nCols + iColumn],
                          -1, pwszBuffer, nBufLen);
      pwszBuffer[nBufLen - 1] = 0;
      return pwszBuffer;
   }
   return NULL;
}


//
// Get number of rows in result
//

extern "C" int EXPORT DrvGetNumRows(DB_RESULT hResult)
{
   return ((SQLITE_RESULT *)hResult)->nRows;
}


//
// Free SELECT results
//

extern "C" void EXPORT DrvFreeResult(DB_RESULT hResult)
{
   int i, nCount;

   if (hResult != NULL)
   {
      if (((SQLITE_RESULT *)hResult)->ppszData != NULL)
      {
         nCount = ((SQLITE_RESULT *)hResult)->nRows * ((SQLITE_RESULT *)hResult)->nCols;
         for(i = 0; i < nCount; i++)
            safe_free(((SQLITE_RESULT *)hResult)->ppszData[i]);
         free(((SQLITE_RESULT *)hResult)->ppszData);
      }
      free(hResult);
   }
}


//
// Perform asynchronous SELECT query
//

extern "C" DB_ASYNC_RESULT EXPORT DrvAsyncSelect(SQLITE_CONN *hConn, WCHAR *pwszQuery,
                                                 DWORD *pdwError, TCHAR *errorText)
{
   DB_ASYNC_RESULT hResult;
   char *pszQueryUTF8;

   pszQueryUTF8 = UTF8StringFromWideString(pwszQuery);
   MutexLock(hConn->mutexQueryLock, INFINITE);
   if (ExecCommand(hConn, SQLITE_DRV_PREPARE, pszQueryUTF8, errorText) == SQLITE_OK)
   {
      hResult = hConn;
   }
   else
   {
      MutexUnlock(hConn->mutexQueryLock);
      hResult = NULL;
   }
   free(pszQueryUTF8);
   *pdwError = (hResult != NULL) ? DBERR_SUCCESS : DBERR_OTHER_ERROR;
   return hResult;
}


//
// Fetch next result line from asynchronous SELECT results
//

extern "C" BOOL EXPORT DrvFetch(DB_ASYNC_RESULT hResult)
{
   return hResult ? (ExecCommand((SQLITE_CONN *)hResult, SQLITE_DRV_STEP, NULL, NULL) == SQLITE_ROW) : FALSE;
}


//
// Get field from current row in async query result
//

extern "C" WCHAR EXPORT *DrvGetFieldAsync(DB_ASYNC_RESULT hResult, int iColumn,
                                          WCHAR *pBuffer, int iBufSize)
{
   char *pszData;
   WCHAR *pwszRet = NULL;

   if ((iColumn >= 0) && (iColumn < ((SQLITE_CONN *)hResult)->nNumCols))
   {
      pszData = (char *)sqlite3_column_text(((SQLITE_CONN *)hResult)->pvm, iColumn);
      if (pszData != NULL)
      {
         MultiByteToWideChar(CP_UTF8, 0, pszData, -1, pBuffer, iBufSize);
         pBuffer[iBufSize - 1] = 0;
         pwszRet = pBuffer;
      }
   }
   return pwszRet;
}


//
// Destroy result of async query
//

extern "C" void EXPORT DrvFreeAsyncResult(DB_ASYNC_RESULT hResult)
{
   if (hResult != NULL)
   {
      ExecCommand((SQLITE_CONN *)hResult, SQLITE_DRV_FINALIZE, NULL, NULL);
      MutexUnlock(((SQLITE_CONN *)hResult)->mutexQueryLock);
   }
}


//
// Begin transaction
//

extern "C" DWORD EXPORT DrvBegin(SQLITE_CONN *pConn)
{
   return DrvQueryInternal(pConn, "BEGIN", NULL);
}


//
// Commit transaction
//

extern "C" DWORD EXPORT DrvCommit(SQLITE_CONN *pConn)
{
   return DrvQueryInternal(pConn, "COMMIT", NULL);
}


//
// Rollback transaction
//

extern "C" DWORD EXPORT DrvRollback(SQLITE_CONN *pConn)
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
