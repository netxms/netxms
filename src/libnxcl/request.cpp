/* 
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2004 Victor Kirhenshtein
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
** $module: request.cpp
**
**/

#include "libnxcl.h"


//
// Create request for processing
//

HREQUEST CreateRequest(DWORD dwCode, void *pArg, BOOL bDynamicArg)
{
   REQUEST *pRequest;

   pRequest = (REQUEST *)MemAlloc(sizeof(REQUEST));
   pRequest->dwCode = dwCode;
   pRequest->pArg = pArg;
   pRequest->bDynamicArg = bDynamicArg;
   pRequest->dwHandle = g_dwRequestId++;
   g_pRequestQueue->Put(pRequest);
   return pRequest->dwHandle;
}


//
// Process async user request
//

HREQUEST LIBNXCL_EXPORTABLE NXCRequest(DWORD dwOperation, ...)
{
   HREQUEST hRequest;
   va_list args;

   if (g_dwState == STATE_DISCONNECTED)
   {
      hRequest = INVALID_REQUEST_HANDLE;
   }
   else
   {
      va_start(args, dwOperation);
      switch(dwOperation)
      {
         case NXC_OP_SYNC_OBJECTS:
            hRequest = CreateRequest(RQ_SYNC_OBJECTS, NULL, FALSE);
            break;
         case NXC_OP_SYNC_EVENTS:
            hRequest = CreateRequest(RQ_SYNC_EVENTS, NULL, FALSE);
            break;
         case NXC_OP_OPEN_EVENT_DB:
            hRequest = CreateRequest(RQ_OPEN_EVENT_DB, NULL, FALSE);
            break;
         case NXC_OP_LOAD_USER_DB:
            hRequest = CreateRequest(RQ_LOAD_USER_DB, NULL, FALSE);
            break;
         case NXC_OP_CLOSE_EVENT_DB:
            hRequest = CreateRequest(RQ_CLOSE_EVENT_DB, (void *)va_arg(args, BOOL), FALSE);
            break;
         case NXC_OP_SET_EVENT_INFO:
            hRequest = CreateRequest(RQ_SET_EVENT_INFO, 
                                     DuplicateEventTemplate(va_arg(args, NXC_EVENT_TEMPLATE *)), TRUE);
            break;
         case NXC_OP_MODIFY_OBJECT:
            hRequest = CreateRequest(RQ_MODIFY_OBJECT, 
                                     DuplicateObjectUpdate(va_arg(args, NXC_OBJECT_UPDATE *)), TRUE);
            break;
         case NXC_OP_CREATE_USER:
            if (va_arg(args, BOOL))
               hRequest = CreateRequest(RQ_CREATE_USER_GROUP, nx_strdup(va_arg(args, char *)), TRUE);
            else
               hRequest = CreateRequest(RQ_CREATE_USER, nx_strdup(va_arg(args, char *)), TRUE);
            break;
         case NXC_OP_DELETE_USER:
            hRequest = CreateRequest(RQ_DELETE_USER, (void *)va_arg(args, DWORD), FALSE);
            break;
         case NXC_OP_LOCK_USER_DB:
            hRequest = CreateRequest(RQ_LOCK_USER_DB, (void *)va_arg(args, BOOL), FALSE);
            break;
         default:
            hRequest = INVALID_REQUEST_HANDLE;
            break;
      }
      va_end(args);
   }
   return hRequest;
}


//
// Client request processor
//

void RequestProcessor(void *pArg)
{
   REQUEST *pRequest;
   DWORD dwRetCode;

   while(1)
   {
      pRequest = (REQUEST *)g_pRequestQueue->GetOrBlock();
      dwRetCode = RCC_SUCCESS;
      switch(pRequest->dwCode)
      {
         case RQ_CONNECT:
            if (Connect())
            {
               ChangeState(STATE_IDLE);
            }
            else
            {
               ChangeState(STATE_DISCONNECTED);
            }
            break;
         case RQ_SYNC_OBJECTS:
            SyncObjects();
            break;
         case RQ_MODIFY_OBJECT:
            dwRetCode = ModifyObject(pRequest->dwHandle, (NXC_OBJECT_UPDATE *)pRequest->pArg,
                                     pRequest->bDynamicArg);
            break;
         case RQ_SYNC_EVENTS:
            SyncEvents();
            break;
         case RQ_OPEN_EVENT_DB:
            dwRetCode = OpenEventDB(pRequest->dwHandle);
            break;
         case RQ_CLOSE_EVENT_DB:
            dwRetCode = CloseEventDB(pRequest->dwHandle, (BOOL)pRequest->pArg);
            break;
         case RQ_SET_EVENT_INFO:
            dwRetCode = SetEventInfo(pRequest->dwHandle, (NXC_EVENT_TEMPLATE *)pRequest->pArg, 
                                     pRequest->bDynamicArg);
            break;
         case RQ_LOAD_USER_DB:
            dwRetCode = LoadUserDB(pRequest->dwHandle);
            break;
         case RQ_CREATE_USER:
            dwRetCode = CreateUser(pRequest->dwHandle, (char *)pRequest->pArg, FALSE);
            break;
         case RQ_CREATE_USER_GROUP:
            dwRetCode = CreateUser(pRequest->dwHandle, (char *)pRequest->pArg, TRUE);
            break;
         case RQ_DELETE_USER:
            dwRetCode = DeleteUser(pRequest->dwHandle, (DWORD)pRequest->pArg);
            break;
         case RQ_LOCK_USER_DB:
            dwRetCode = LockUserDB(pRequest->dwHandle, (BOOL)pRequest->pArg);
            break;
         default:
            CallEventHandler(NXC_EVENT_ERROR, NXC_ERR_INTERNAL, (void *)"Internal error");
            dwRetCode = RCC_INVALID_REQUEST;
            break;
      }
      CallEventHandler(NXC_EVENT_REQUEST_COMPLETED, pRequest->dwHandle, (void *)dwRetCode);
      if (pRequest->bDynamicArg)
         MemFree(pRequest->pArg);
      MemFree(pRequest);
   }
}
