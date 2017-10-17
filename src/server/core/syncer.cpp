/*
** NetXMS - Network Management System
** Copyright (C) 2003-2016 Victor Kirhenshtein
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

#define DEBUG_TAG_SYNC           _T("sync")
#define DEBUG_TAG_OBJECT_SYNC    _T("obj.sync")

/**
 * Externals
 */
void NetObjDelete(NetObj *pObject);

/**
 * Object transaction lock
 */
static RWLOCK s_objectTxnLock = RWLockCreate();

/**
 * Start object transaction
 */
void NXCORE_EXPORTABLE ObjectTransactionStart()
{
   if (g_flags & AF_ENABLE_OBJECT_TRANSACTIONS)
      RWLockReadLock(s_objectTxnLock, INFINITE);
}

/**
 * End object transaction
 */
void NXCORE_EXPORTABLE ObjectTransactionEnd()
{
   if (g_flags & AF_ENABLE_OBJECT_TRANSACTIONS)
      RWLockUnlock(s_objectTxnLock);
}

/**
 * Save objects to database
 */
void SaveObjects(DB_HANDLE hdb, UINT32 watchdogId, bool saveRuntimeData)
{
   if (g_flags & AF_ENABLE_OBJECT_TRANSACTIONS)
      RWLockWriteLock(s_objectTxnLock, INFINITE);

	ObjectArray<NetObj> *objects = g_idxObjectById.getObjects(false);
   nxlog_debug_tag(DEBUG_TAG_SYNC, 5, _T("%d objects to process"), objects->size());
	for(int i = 0; i < objects->size(); i++)
   {
	   WatchdogNotify(watchdogId);
   	NetObj *object = objects->get(i);
      if (object->isDeleted())
      {
         nxlog_debug_tag(DEBUG_TAG_OBJECT_SYNC, 5, _T("Object %s [%d] marked for deletion"), object->getName(), object->getId());
         if (object->getRefCount() == 0)
         {
   		   DBBegin(hdb);
            if (object->deleteFromDatabase(hdb))
            {
               nxlog_debug_tag(DEBUG_TAG_OBJECT_SYNC, 4, _T("Object %d \"%s\" deleted from database"), object->getId(), object->getName());
               DBCommit(hdb);
               NetObjDelete(object);
            }
            else
            {
               DBRollback(hdb);
               nxlog_debug_tag(DEBUG_TAG_OBJECT_SYNC, 4, _T("Call to deleteFromDatabase() failed for object %s [%d], transaction rollback"), object->getName(), object->getId());
            }
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG_OBJECT_SYNC, 3, _T("Unable to delete object with id %d because it is being referenced %d time(s)"),
                      object->getId(), object->getRefCount());
         }
      }
		else if (object->isModified())
		{
		   nxlog_debug_tag(DEBUG_TAG_OBJECT_SYNC, 5, _T("Object %s [%d] modified"), object->getName(), object->getId());
		   DBBegin(hdb);
			if (object->saveToDatabase(hdb))
			{
				DBCommit(hdb);
			}
			else
			{
				DBRollback(hdb);
			}
		}
		else if (saveRuntimeData)
		{
         object->saveRuntimeData(hdb);
		}
   }

   if (g_flags & AF_ENABLE_OBJECT_TRANSACTIONS)
      RWLockUnlock(s_objectTxnLock);
	delete objects;
	nxlog_debug_tag(DEBUG_TAG_SYNC, 5, _T("Save objects completed"));
}

/**
 * Syncer thread
 */
THREAD_RESULT THREAD_CALL Syncer(void *arg)
{
   ThreadSetName("Syncer");

   int syncInterval = ConfigReadInt(_T("SyncInterval"), 60);
   UINT32 watchdogId = WatchdogAddThread(_T("Syncer Thread"), 30);

   nxlog_debug_tag(DEBUG_TAG_SYNC, 1, _T("Syncer thread started, sync_interval = %d"), syncInterval);

   // Main syncer loop
   WatchdogStartSleep(watchdogId);
   while(!SleepAndCheckForShutdown(syncInterval))
   {
      WatchdogNotify(watchdogId);
      nxlog_debug_tag(DEBUG_TAG_SYNC, 7, _T("Syncer wakeup"));
      if (!(g_flags & AF_DB_CONNECTION_LOST))    // Don't try to save if DB connection is lost
      {
         DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
         SaveObjects(hdb, watchdogId, false);
         nxlog_debug_tag(DEBUG_TAG_SYNC, 5, _T("Saving user database"));
         SaveUsers(hdb, watchdogId);
         nxlog_debug_tag(DEBUG_TAG_SYNC, 5, _T("Saving NXSL persistent storage"));
         UpdatePStorageDatabase(hdb, watchdogId);
         DBConnectionPoolReleaseConnection(hdb);
      }
      WatchdogStartSleep(watchdogId);
      nxlog_debug_tag(DEBUG_TAG_SYNC, 7, _T("Syncer run completed"));
   }

   nxlog_debug_tag(DEBUG_TAG_SYNC, 1, _T("Syncer thread terminated"));
   return THREAD_OK;
}
