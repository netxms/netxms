/*
** NetXMS native device configuration backup module
** Copyright (C) 2025-2026 Raden Solutions
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
** File: scheduler.cpp
**/

#include "devbackup.h"

/**
 * Scheduler thread handle
 */
static THREAD s_schedulerThread = INVALID_THREAD_HANDLE;

/**
 * Scheduler wakeup condition
 */
static Condition s_wakeupCondition(true);

/**
 * Shutdown flag
 */
static bool s_shutdownFlag = false;

/**
 * Scheduler thread
 */
static void SchedulerThread()
{
   nxlog_debug_tag(DEBUG_TAG, 2, L"Backup scheduler thread started");

   // Initial delay to allow server to fully start
   s_wakeupCondition.wait(30000);

   while (!s_shutdownFlag)
   {
      time_t now = time(nullptr);
      time_t nextWakeup = now + 60;  // Default: check again in 60 seconds

      unique_ptr<SharedObjectArray<NetObj>> nodes = g_idxNodeById.getObjects();
      for (int i = 0; i < nodes->size(); i++)
      {
         if (s_shutdownFlag)
            break;

         Node *node = static_cast<Node*>(nodes->get(i));
         if (!(node->getCapabilities() & NC_REGISTERED_FOR_BACKUP))
            continue;

         if (!node->getCustomAttributeAsBoolean(L"$devbackup.registered", false))
            continue;

         // Get per-node or global interval
         int32_t interval = node->getCustomAttributeAsInt32(L"SysConfig:DeviceConfigBackup.Interval", g_backupInterval);
         if (interval <= 0)
            interval = g_backupInterval;

         // Get last backup time
         int64_t lastBackupTime = node->getCustomAttributeAsInt64(L"$devbackup.lastBackupTime", 0);

         time_t dueTime = static_cast<time_t>(lastBackupTime) + interval;
         if (now >= dueTime)
         {
            // Node is due for backup
            if (g_activeBackupJobs < g_maxConcurrentJobs)
            {
               shared_ptr<Node> nodePtr = static_pointer_cast<Node>(FindObjectById(node->getId(), OBJECT_NODE));
               if (nodePtr != nullptr)
               {
                  nxlog_debug_tag(DEBUG_TAG, 5, L"Scheduler: submitting backup job for %s [%u]", node->getName(), node->getId());
                  ThreadPoolExecute(g_mainThreadPool, ExecuteBackupJob, nodePtr);
               }
            }
            else
            {
               // At max concurrent jobs, try again soon
               if (now + 10 < nextWakeup)
                  nextWakeup = now + 10;
            }
         }
         else
         {
            // Node not yet due - track earliest due time
            if (dueTime < nextWakeup)
               nextWakeup = dueTime;
         }
      }

      uint32_t sleepTime = static_cast<uint32_t>((nextWakeup > now) ? (nextWakeup - now) * 1000 : 1000);
      if (sleepTime > 60000)
         sleepTime = 60000;

      nxlog_debug_tag(DEBUG_TAG, 7, L"Scheduler: sleeping for %u ms (active jobs: %d)", sleepTime, static_cast<int>(g_activeBackupJobs));
      s_wakeupCondition.wait(sleepTime);
      s_wakeupCondition.reset();
   }

   nxlog_debug_tag(DEBUG_TAG, 2, L"Backup scheduler thread stopped");
}

/**
 * Start backup scheduler
 */
void StartBackupScheduler()
{
   s_shutdownFlag = false;
   s_schedulerThread = ThreadCreateEx(SchedulerThread);
}

/**
 * Stop backup scheduler
 */
void StopBackupScheduler()
{
   s_shutdownFlag = true;
   s_wakeupCondition.set();
   ThreadJoin(s_schedulerThread);
   s_schedulerThread = INVALID_THREAD_HANDLE;
}

/**
 * Signal scheduler to re-evaluate
 */
void SignalScheduler()
{
   s_wakeupCondition.set();
}
