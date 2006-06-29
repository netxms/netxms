/* 
** SQLite Database Driver
** Copyright (C) 2005 Victor Kirhenshtein
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
** $module: sqlite.cpp
**
**/

#include "sqlitedrv.h"


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
      ConditionSet(pConn->condResult);
   } while(bRun);
   return THREAD_OK;
}


//
// Execute command by worker thread
//

static int ExecCommand(SQLITE_CONN *pConn, int nCmd, char *pszQuery)
{
   pConn->pszQuery = pszQuery;
   pConn->nCommand = nCmd;
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

extern "C" void EXPORT DrvDisconnect(DB_HANDLE hConn)
{
   if (hConn != NULL)
   {
      ExecCommand((SQLITE_CONN *)hConn, SQLITE_DRV_CLOSE, NULL);
      ThreadJoin(((SQLITE_CONN *)hConn)->hThread);
      ConditionDestroy(((SQLITE_CONN *)hConn)->condCommand);
      ConditionDestroy(((SQLITE_CONN *)hConn)->condResult);
      MutexDestroy(((SQLITE_CONN *)hConn)->mutexQueryLock);
      free(hConn);
   }
}


//
// Connect to database
//

extern "C" DB_HANDLE EXPORT DrvConnect(char *pszHost, char *pszLogin,
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

   if (ExecCommand(pConn, SQLITE_DRV_OPEN, pszDatabase) != SQLITE_OK)
   {
      DrvDisconnect(pConn);
      pConn = NULL;
   }
   return (DB_HANDLE)pConn;
}


//
// Perform non-SELECT query
//

extern "C" BOOL EXPORT DrvQuery(DB_HANDLE hConn, char *pszQuery)
{
   BOOL bResult;

   MutexLock(((SQLITE_CONN *)hConn)->mutexQueryLock, INFINITE);
   bResult = (ExecCommand((SQLITE_CONN *)hConn, SQLITE_DRV_QUERY, pszQuery) == SQLITE_OK);
   MutexUnlock(((SQLITE_CONN *)hConn)->mutexQueryLock);
   return bResult;
}


//
// Perform SELECT query
//

extern "C" DB_RESULT EXPORT DrvSelect(DB_HANDLE hConn, char *pszQuery)
{
   DB_RESULT pResult = NULL;

   MutexLock(((SQLITE_CONN *)hConn)->mutexQueryLock, INFINITE);
   if (ExecCommand((SQLITE_CONN *)hConn, SQLITE_DRV_SELECT, pszQuery) == SQLITE_OK)
   {
      pResult = ((SQLITE_CONN *)hConn)->pResult;
   }
   MutexUnlock(((SQLITE_CONN *)hConn)->mutexQueryLock);
   return pResult;
}


//
// Get field value from result
//

extern "C" char EXPORT *DrvGetField(DB_RESULT hResult, int iRow, int iColumn)
{
   if ((iRow < ((SQLITE_RESULT *)hResult)->nRows) &&
       (iColumn < ((SQLITE_RESULT *)hResult)->nCols) &&
       (iRow >= 0) && (iColumn >= 0))
      return ((SQLITE_RESULT *)hResult)->ppszData[iRow * ((SQLITE_RESULT *)hResult)->nCols + iColumn];
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

extern "C" DB_ASYNC_RESULT EXPORT DrvAsyncSelect(DB_HANDLE hConn, char *pszQuery)
{
   DB_ASYNC_RESULT hResult;

   MutexLock(((SQLITE_CONN *)hConn)->mutexQueryLock, INFINITE);
   if (ExecCommand((SQLITE_CONN *)hConn, SQLITE_DRV_PREPARE, pszQuery) == SQLITE_OK)
   {
      hResult = hConn;
   }
   else
   {
      MutexUnlock(((SQLITE_CONN *)hConn)->mutexQueryLock);
      hResult = NULL;
   }
   return hResult;
}


//
// Fetch next result line from asynchronous SELECT results
//

extern "C" BOOL EXPORT DrvFetch(DB_ASYNC_RESULT hResult)
{
   return hResult ? (ExecCommand((SQLITE_CONN *)hResult, SQLITE_DRV_STEP, NULL) == SQLITE_ROW) : FALSE;
}


//
// Get field from current row in async query result
//

extern "C" char EXPORT *DrvGetFieldAsync(DB_ASYNC_RESULT hResult, int iColumn, char *pBuffer, int iBufSize)
{
   char *pszData;

   if ((iColumn >= 0) && (iColumn < ((SQLITE_CONN *)hResult)->nNumCols))
   {
      pszData = (char *)sqlite3_column_text(((SQLITE_CONN *)hResult)->pvm, iColumn);
      if (pszData != NULL)
      {
         strncpy(pBuffer, pszData, iBufSize);
         pBuffer[iBufSize - 1] = 0;
         pszData = pBuffer;
      }
   }
   else
   {
      pszData = NULL;
   }
   return pszData;
}


//
// Destroy result of async query
//

extern "C" void EXPORT DrvFreeAsyncResult(DB_ASYNC_RESULT hResult)
{
   if (hResult != NULL)
   {
      ExecCommand((SQLITE_CONN *)hResult, SQLITE_DRV_FINALIZE, NULL);
      MutexUnlock(((SQLITE_CONN *)hResult)->mutexQueryLock);
   }
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
