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
** $module: rqproc.cpp
**
**/

#include "libnxcl.h"


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
         default:
            CallEventHandler(NXC_EVENT_ERROR, NXC_ERR_INTERNAL, "Internal error");
            break;
      }
   }
}
