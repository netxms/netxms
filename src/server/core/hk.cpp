/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2011 Victor Kirhenshtein
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
** File: hk.cpp
**
**/

#include "nxcore.h"


//
// Remove deleted objects which are no longer referenced
//

static void CleanDeletedObjects(DB_HANDLE hdb)
{
   DB_RESULT hResult;

   hResult = DBSelect(hdb, _T("SELECT object_id FROM deleted_objects"));
   if (hResult != NULL)
   {
      DB_ASYNC_RESULT hAsyncResult;
      TCHAR szQuery[256];
      int i, iNumRows;
      DWORD dwObjectId;

      iNumRows = DBGetNumRows(hResult);
      for(i = 0; i < iNumRows; i++)
      {
         dwObjectId = DBGetFieldULong(hResult, i, 0);

         // Check if there are references to this object in event log
         _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT event_source FROM event_log WHERE event_source=%d"), dwObjectId);
         hAsyncResult = DBAsyncSelect(hdb, szQuery);
         if (hAsyncResult != NULL)
         {
            if (!DBFetch(hAsyncResult))
            {
               // No records with that source ID, so we can purge this object
               _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM deleted_objects WHERE object_id=%d"), dwObjectId);
               QueueSQLRequest(szQuery);
               DbgPrintf(4, _T("*HK* Deleted object with id %d was purged"), dwObjectId);
            }
            DBFreeAsyncResult(hdb, hAsyncResult);
         }
      }
      DBFreeResult(hResult);
   }
}


//
// Delete empty subnets
//

static void DeleteEmptySubnets()
{
   DWORD i;

   RWLockReadLock(g_rwlockIdIndex, INFINITE);

   // Walk through subnets and delete empty ones
   for(i = 0; i < g_dwIdIndexSize; i++)
      if (((NetObj *)g_pIndexById[i].pObject)->Type() == OBJECT_SUBNET)
      {
         if (((NetObj *)g_pIndexById[i].pObject)->IsEmpty())
         {
            PostEvent(EVENT_SUBNET_DELETED, ((NetObj *)g_pIndexById[i].pObject)->Id(), NULL);
            ((NetObj *)g_pIndexById[i].pObject)->Delete(TRUE);
         }
      }

   RWLockUnlock(g_rwlockIdIndex);
}


//
// Maintenance tasks specific to PostgreSQL
//

static void PGSQLMaintenance(DB_HANDLE hdb)
{
   if (!ConfigReadInt(_T("DisableVacuum"), 0))
      DBQuery(hdb, _T("VACUUM ANALYZE"));
}


//
// Housekeeper thread
//

THREAD_RESULT THREAD_CALL HouseKeeper(void *pArg)
{
   time_t currTime;
   TCHAR szQuery[256];
   DWORD i, dwRetentionTime, dwInterval;

   // Load configuration
   dwInterval = ConfigReadULong(_T("HouseKeepingInterval"), 3600);

   // Housekeeping loop
   while(!ShutdownInProgress())
   {
      currTime = time(NULL);
      if (SleepAndCheckForShutdown(dwInterval - (DWORD)(currTime % dwInterval)))
         break;      // Shutdown has arrived

		DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

      // Remove outdated event log records
      dwRetentionTime = ConfigReadULong(_T("EventLogRetentionTime"), 90);
      if (dwRetentionTime > 0)
      {
			dwRetentionTime *= 86400;	// Convert days to seconds
         _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM event_log WHERE event_timestamp<%ld"), currTime - dwRetentionTime);
         DBQuery(hdb, szQuery);
      }

      // Remove outdated syslog records
      dwRetentionTime = ConfigReadULong(_T("SyslogRetentionTime"), 90);
      if (dwRetentionTime > 0)
      {
			dwRetentionTime *= 86400;	// Convert days to seconds
         _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM syslog WHERE msg_timestamp<%ld"), currTime - dwRetentionTime);
         DBQuery(hdb, szQuery);
      }

      // Remove outdated audit log records
      dwRetentionTime = ConfigReadULong(_T("AuditLogRetentionTime"), 90);
      if (dwRetentionTime > 0)
      {
			dwRetentionTime *= 86400;	// Convert days to seconds
         _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM audit_log WHERE timestamp<%ld"), currTime - dwRetentionTime);
         DBQuery(hdb, szQuery);
      }

      // Delete empty subnets if needed
      if (g_dwFlags & AF_DELETE_EMPTY_SUBNETS)
         DeleteEmptySubnets();

      // Remove deleted objects which are no longer referenced
      CleanDeletedObjects(hdb);

      // Remove expired DCI data
		RWLockReadLock(g_rwlockIdIndex, INFINITE);
      for(i = 0; i < g_dwIdIndexSize; i++)
			if (((NetObj *)g_pIndexById[i].pObject)->Type() == OBJECT_NODE)
				((Node *)g_pIndexById[i].pObject)->cleanDCIData();
      RWLockUnlock(g_rwlockIdIndex);

      // Run DB-specific maintenance tasks
      if (g_nDBSyntax == DB_SYNTAX_PGSQL)
         PGSQLMaintenance(hdb);

		DBConnectionPoolReleaseConnection(hdb);
   }

   DbgPrintf(1, _T("Housekeeper thread terminated"));
   return THREAD_OK;
}
