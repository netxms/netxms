/*
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
#include <nxcore_ps.h>
#include <nms_users.h>

#define DEBUG_TAG_SYNC           _T("sync")
#define DEBUG_TAG_OBJECT_SYNC    _T("obj.sync")

/**
 * Thread pool
 */
ThreadPool *g_syncerThreadPool = nullptr;

/**
 * Outstanding save requests
 */
static VolatileCounter s_outstandingSaveRequests = 0;

/**
 * Syncer run time statistic
 */
static ManualGauge64 s_syncerRunTime(_T("Syncer"), 5, 900);
static Mutex s_syncerGaugeLock(MutexType::FAST);
static time_t s_lastRunTime = 0;

/**
 * Get syncer run time
 */
int64_t GetSyncerRunTime(StatisticType statType)
{
   s_syncerGaugeLock.lock();
   int64_t value;
   switch(statType)
   {
      case StatisticType::AVERAGE:
         value = static_cast<int64_t>(s_syncerRunTime.getAverage());
         break;
      case StatisticType::CURRENT:
         value = s_syncerRunTime.getCurrent();
         break;
      case StatisticType::MAX:
         value = s_syncerRunTime.getMax();
         break;
      case StatisticType::MIN:
         value = s_syncerRunTime.getMin();
         break;
   }
   s_syncerGaugeLock.unlock();
   return value;
}

/**
 * Show syncer stats on server debug console
 */
void ShowSyncerStats(ServerConsole *console)
{
   TCHAR runTime[128];
   s_syncerGaugeLock.lock();
   console->printf(
            _T("Last run at .........: %s\n")
            _T("Last run time .......: %d ms\n")
            _T("Average run time ....: %d ms\n")
            _T("Max run time ........: %d ms\n")
            _T("Min run time ........: %d ms\n")
            _T("\n"), FormatTimestamp(s_lastRunTime, runTime),
            s_syncerRunTime.getCurrent(), static_cast<int>(s_syncerRunTime.getAverage()),
            s_syncerRunTime.getMax(), s_syncerRunTime.getMin());
   s_syncerGaugeLock.unlock();
}

/**
 * Save object to database on separate thread
 */
static void SaveObject(NetObj *object)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DBBegin(hdb);
   if (object->saveToDatabase(hdb))
   {
      DBCommit(hdb);
      object->markAsSaved();
   }
   else
   {
      DBRollback(hdb);
   }
   DBConnectionPoolReleaseConnection(hdb);
   InterlockedDecrement(&s_outstandingSaveRequests);
}

/**
 * Save objects to database
 */
void SaveObjects(DB_HANDLE hdb, uint32_t watchdogId, bool saveRuntimeData)
{
   s_outstandingSaveRequests = 0;

	unique_ptr<SharedObjectArray<NetObj>> objects = g_idxObjectById.getObjects();
   nxlog_debug_tag(DEBUG_TAG_SYNC, 5, _T("%d objects to process"), objects->size());
	for(int i = 0; i < objects->size(); i++)
   {
	   WatchdogNotify(watchdogId);
   	NetObj *object = objects->get(i);
   	nxlog_debug_tag(DEBUG_TAG_OBJECT_SYNC, 8, _T("Object %s [%d] at index %d"), object->getName(), object->getId(), i);
      if (object->isDeleted())
      {
         nxlog_debug_tag(DEBUG_TAG_OBJECT_SYNC, 5, _T("Object %s [%d] marked for deletion"), object->getName(), object->getId());
         DBBegin(hdb);
         if (object->deleteFromDatabase(hdb))
         {
            nxlog_debug_tag(DEBUG_TAG_OBJECT_SYNC, 4, _T("Object %d \"%s\" deleted from database"), object->getId(), object->getName());
            DBCommit(hdb);

            // Remove object from global object index by ID
            g_idxObjectById.remove(object->getId());
         }
         else
         {
            DBRollback(hdb);
            nxlog_debug_tag(DEBUG_TAG_OBJECT_SYNC, 4, _T("Call to deleteFromDatabase() failed for object %s [%d], transaction rollback"), object->getName(), object->getId());
         }
      }
		else if (object->isModified())
		{
         if (saveRuntimeData)
         {
            object->markAsModified(MODIFY_COMMON_PROPERTIES); //save runtime data as well
         }
		   nxlog_debug_tag(DEBUG_TAG_OBJECT_SYNC, 5, _T("Object %s [%d] modified with flags %08X"), object->getName(), object->getId(), object->getModifyFlags());
		   if (g_syncerThreadPool != nullptr)
		   {
		      InterlockedIncrement(&s_outstandingSaveRequests);
		      ThreadPoolExecute(g_syncerThreadPool, SaveObject, object);
		   }
		   else
		   {
            DBBegin(hdb);
            if (object->saveToDatabase(hdb))
            {
               DBCommit(hdb);
               object->markAsSaved();
            }
            else
            {
               DBRollback(hdb);
            }
		   }
		}
		else if (saveRuntimeData)
		{
         object->saveRuntimeData(hdb);
		}
   }

	if (g_syncerThreadPool != nullptr)
	{
	   while(s_outstandingSaveRequests > 0)
	   {
	      nxlog_debug_tag(DEBUG_TAG_OBJECT_SYNC, 7, _T("Waiting for outstanding object save requests (%d requests in queue)"), (int)s_outstandingSaveRequests);
	      ThreadSleep(1);
	      WatchdogNotify(watchdogId);
	   }
	}

	nxlog_debug_tag(DEBUG_TAG_SYNC, 5, _T("Save objects completed"));
}

/**
 * Syncer thread
 */
void Syncer()
{
   ThreadSetName("Syncer");

   int syncInterval = ConfigReadInt(_T("Objects.SyncInterval"), 60);
   uint32_t watchdogId = WatchdogAddThread(_T("Syncer Thread"), 30);

   nxlog_debug_tag(DEBUG_TAG_SYNC, 1, _T("Syncer thread started, sync_interval = %d"), syncInterval);

   // Main syncer loop
   WatchdogStartSleep(watchdogId);
   while(!SleepAndCheckForShutdown(syncInterval))
   {
      WatchdogNotify(watchdogId);
      nxlog_debug_tag(DEBUG_TAG_SYNC, 7, _T("Syncer wakeup"));

      // Don't try to save if DB connection is lost or server not fully initialized yet
      if ((g_flags & (AF_DB_CONNECTION_LOST | AF_SERVER_INITIALIZED)) == AF_SERVER_INITIALIZED)
      {
         int64_t startTime = GetCurrentTimeMs();
         DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
         SaveObjects(hdb, watchdogId, false);
         nxlog_debug_tag(DEBUG_TAG_SYNC, 5, _T("Saving user database"));
         SaveUsers(hdb, watchdogId);
         nxlog_debug_tag(DEBUG_TAG_SYNC, 5, _T("Saving NXSL persistent storage"));
         UpdatePStorageDatabase(hdb, watchdogId);
         DBConnectionPoolReleaseConnection(hdb);
         s_syncerGaugeLock.lock();
         s_syncerRunTime.update(GetCurrentTimeMs() - startTime);
         s_lastRunTime = static_cast<time_t>(startTime / 1000);
         s_syncerGaugeLock.unlock();
      }
      WatchdogStartSleep(watchdogId);
      nxlog_debug_tag(DEBUG_TAG_SYNC, 7, _T("Syncer run completed"));
   }

   nxlog_debug_tag(DEBUG_TAG_SYNC, 1, _T("Syncer thread terminated"));
}
