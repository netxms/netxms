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

#include "nxcore.h"


//
// Remove deleted objects which are no longer referenced
//

static void CleanDeletedObjects(void)
{
   DB_RESULT hResult;

   hResult = DBSelect(g_hCoreDB, "SELECT object_id FROM deleted_objects");
   if (hResult != NULL)
   {
      DB_ASYNC_RESULT hAsyncResult;
      char szQuery[256];
      int i, iNumRows;
      DWORD dwObjectId;

      iNumRows = DBGetNumRows(hResult);
      for(i = 0; i < iNumRows; i++)
      {
         dwObjectId = DBGetFieldULong(hResult, i, 0);

         // Check if there are references to this object in event log
         sprintf(szQuery, "SELECT event_source FROM event_log WHERE event_source=%ld", dwObjectId);
         hAsyncResult = DBAsyncSelect(g_hCoreDB, szQuery);
         if (hAsyncResult != NULL)
         {
            if (!DBFetch(hAsyncResult))
            {
               // No records with that source ID, so we can purge this object
               sprintf(szQuery, "DELETE FROM deleted_objects WHERE object_id=%ld", dwObjectId);
               QueueSQLRequest(szQuery);
               DbgPrintf(AF_DEBUG_HOUSEKEEPER, "*HK* Deleted object with id %ld was purged", dwObjectId);
            }
            DBFreeAsyncResult(hAsyncResult);
         }
      }
      DBFreeResult(hResult);
   }
}


//
// Delete empty subnets
//

static void DeleteEmptySubnets(void)
{
   DWORD i;

   RWLockReadLock(g_rwlockIdIndex, INFINITE);

   // Walk through subnets and delete empty ones
   for(i = 0; i < g_dwIdIndexSize; i++)
      if (g_pIndexById[i].pObject->Type() == OBJECT_SUBNET)
      {
         if (g_pIndexById[i].pObject->IsEmpty())
         {
            PostEvent(EVENT_SUBNET_DELETED, g_pIndexById[i].pObject->Id(), NULL);
            g_pIndexById[i].pObject->Delete(TRUE);
         }
      }

   RWLockUnlock(g_rwlockIdIndex);
}


//
// Housekeeper thread
//

THREAD_RESULT THREAD_CALL HouseKeeper(void *pArg)
{
   time_t currTime;
   char szQuery[256];
   DWORD i, dwEventLogRetentionTime, dwInterval;

   // Load configuration
   dwInterval = ConfigReadULong("HouseKeepingInterval", 3600);
   dwEventLogRetentionTime = ConfigReadULong("EventLogRetentionTime", 5184000);

   // Housekeeping loop
   while(!ShutdownInProgress())
   {
      currTime = time(NULL);
      if (SleepAndCheckForShutdown(dwInterval - (currTime % dwInterval)))
         break;      // Shutdown has arrived

      // Remove outdated event log records
      if (dwEventLogRetentionTime > 0)
      {
         sprintf(szQuery, "DELETE FROM event_log WHERE event_timestamp<%ld", currTime - dwEventLogRetentionTime);
         DBQuery(g_hCoreDB, szQuery);
      }

      // Delete empty subnets if needed
      if (g_dwFlags & AF_DELETE_EMPTY_SUBNETS)
         DeleteEmptySubnets();

      // Remove deleted objects which are no longer referenced
      CleanDeletedObjects();

      // Remove expired DCI data
      RWLockReadLock(g_rwlockNodeIndex, INFINITE);
      for(i = 0; i < g_dwNodeAddrIndexSize; i++)
         ((Node *)g_pNodeIndexByAddr[i].pObject)->CleanDCIData();
      RWLockUnlock(g_rwlockNodeIndex);
   }
   return THREAD_OK;
}
