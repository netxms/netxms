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
// Process async user request
//

HREQUEST EXPORTABLE NXCRequest(DWORD dwOperation, ...)
{
   HREQUEST hRequest;

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
      default:
         hRequest = INVALID_REQUEST_HANDLE;
         break;
   }

   return hRequest;
}


//
// Client request processor
//

void RequestProcessor(void *pArg)
{
   REQUEST *pRequest;

   while(1)
   {
      pRequest = (REQUEST *)g_pRequestQueue->GetOrBlock();
      switch(pRequest->dwCode)
      {
         case RQ_CONNECT:
            if (Connect())
               ChangeState(STATE_IDLE);
            else
               ChangeState(STATE_DISCONNECTED);
            break;
         case RQ_SYNC_OBJECTS:
            SyncObjects();
            break;
         case RQ_SYNC_EVENTS:
            SyncEvents();
            break;
         case RQ_OPEN_EVENT_DB:
            OpenEventDB();
            break;
         default:
            CallEventHandler(NXC_EVENT_ERROR, NXC_ERR_INTERNAL, "Internal error");
            break;
      }
      CallEventHandler(NXC_EVENT_REQUEST_COMPLETED, pRequest->dwHandle, NULL);
      MemFree(pRequest);
   }
}
