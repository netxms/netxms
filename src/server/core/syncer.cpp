/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
void SaveObjects(DB_HANDLE hdb)
{
   if (g_flags & AF_ENABLE_OBJECT_TRANSACTIONS)
      RWLockWriteLock(s_objectTxnLock, INFINITE);

	ObjectArray<NetObj> *objects = g_idxObjectById.getObjects(false);
   DbgPrintf(5, _T("Syncer: %d objects to process"), objects->size());
	for(int i = 0; i < objects->size(); i++)
   {
   	NetObj *object = objects->get(i);
      if (object->isDeleted())
      {
         DbgPrintf(5, _T("Syncer: object %s [%d] marked for deletion"), object->getName(), object->getId());
         if (object->getRefCount() == 0)
         {
   		   DBBegin(hdb);
            if (object->deleteFromDatabase(hdb))
            {
               DbgPrintf(4, _T("Syncer: Object %d \"%s\" deleted from database"), object->getId(), object->getName());
               DBCommit(hdb);
               NetObjDelete(object);
            }
            else
            {
               DBRollback(hdb);
               DbgPrintf(4, _T("Syncer: Call to deleteFromDatabase() failed for object %s [%d], transaction rollback"), object->getName(), object->getId());
            }
         }
         else
         {
            DbgPrintf(3, _T("Syncer: Unable to delete object with id %d because it is being referenced %d time(s)"),
                      object->getId(), object->getRefCount());
         }
      }
		else if (object->isModified())
		{
         DbgPrintf(5, _T("Syncer: object %s [%d] modified"), object->getName(), object->getId());
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
   }

   if (g_flags & AF_ENABLE_OBJECT_TRANSACTIONS)
      RWLockUnlock(s_objectTxnLock);
	delete objects;
   DbgPrintf(5, _T("Syncer: save objects completed"));
}

/**
 * Syncer thread
 */
THREAD_RESULT THREAD_CALL Syncer(void *arg)
{
   int iSyncInterval;
   UINT32 dwWatchdogId;

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
      if (!(g_flags & AF_DB_CONNECTION_LOST))    // Don't try to save if DB connection is lost
      {
         DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
         SaveObjects(hdb);
         SaveUsers(hdb);
         DBConnectionPoolReleaseConnection(hdb);
      }
   }

   DbgPrintf(1, _T("Syncer thread terminated"));
   return THREAD_OK;
}
