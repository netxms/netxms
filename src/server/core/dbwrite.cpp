/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: hk.cpp
**
**/

#include "nms_core.h"


//
// Global variables
//

Queue *g_pLazyRequestQueue = NULL;


//
// Put SQL request into queue for later execution
//

void QueueSQLRequest(char *szQuery)
{
   g_pLazyRequestQueue->Put(strdup(szQuery));
}


//
// Database "lazy" write thread
//

void DBWriteThread(void *pArg)
{
   char *pQuery;

   while(!ShutdownInProgress())
   {
      pQuery = (char *)g_pLazyRequestQueue->GetOrBlock();
      if (pQuery != NULL)
      {
         DBQuery(g_hCoreDB, pQuery);
         free(pQuery);
      }
   }
}
