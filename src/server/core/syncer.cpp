/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2012 Victor Kirhenshtein
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
** File: syncer.cpp
**
**/

#include "nxcore.h"

/**
 * Externals
 */
void NetObjDelete(NetObj *pObject);

/**
 * Save objects to database
 */
void SaveObjects(DB_HANDLE hdb)
{
	ObjectArray<NetObj> *objects = g_idxObjectById.getObjects();

	for(int i = 0; i < objects->size(); i++)
   {
   	NetObj *object = objects->get(i);
      if (object->isDeleted())
      {
         if (object->getRefCount() == 0)
         {
            DbgPrintf(4, _T("* Syncer * Object %d \"%s\" deleted"), object->Id(), object->Name());
            object->DeleteFromDB();
            NetObjDelete(object);
         }
         else
         {
            DbgPrintf(3, _T("* Syncer * Unable to delete object with id %d because it is being referenced %d time(s)"),
                      object->Id(), object->getRefCount());
         }
      }
		else if (object->isModified())
		{
		   DBBegin(hdb);
			if (object->SaveToDB(hdb))
			{
				DBCommit(hdb);
			}
			else
			{
				DBRollback(hdb);
			}
		}
   }

	delete objects;
}


//
// Syncer thread
//

THREAD_RESULT THREAD_CALL Syncer(void *arg)
{
   int iSyncInterval;
   DWORD dwWatchdogId;
   DB_HANDLE hdb;

   // Establish separate connection to database if needed
   if (g_dwFlags & AF_ENABLE_MULTIPLE_DB_CONN)
   {
		TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
      hdb = DBConnect(g_dbDriver, g_szDbServer, g_szDbName, g_szDbLogin, g_szDbPassword, g_szDbSchema, errorText);
      if (hdb == NULL)
      {
         nxlog_write(MSG_DB_CONNFAIL, EVENTLOG_ERROR_TYPE, "s", errorText);
         hdb = g_hCoreDB;   // Switch to main DB connection
      }
   }
   else
   {
      hdb = g_hCoreDB;
   }

   // Read configuration
   iSyncInterval = ConfigReadInt(_T("SyncInterval"), 60);
   DbgPrintf(1, _T("Syncer thread started, sync_interval = %d"), iSyncInterval);
   dwWatchdogId = WatchdogAddThread(_T("Syncer Thread"), iSyncInterval * 2 + 10);

   // Main syncer loop
   while(!IsShutdownInProgress())
   {
      if (SleepAndCheckForShutdown(iSyncInterval))
         break;   // Shutdown time has arrived
      WatchdogNotify(dwWatchdogId);
      if (!(g_dwFlags & AF_DB_CONNECTION_LOST))    // Don't try to save if DB connection is lost
      {
         SaveObjects(hdb);
         SaveUsers(hdb);
      }
   }

   // Disconnect from database if using separate connection
   if (hdb != g_hCoreDB)
   {
      DBDisconnect(hdb);
   }

   DbgPrintf(1, _T("Syncer thread terminated"));
   return THREAD_OK;
}
