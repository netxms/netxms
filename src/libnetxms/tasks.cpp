/*
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2023 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: tp.cpp
**
**/

#include "libnetxms.h"
#include <nxtask.h>

/**
 * Run task. This is internal method that is intended to be called only from thread function.
 */
void BackgroundTask::run()
{
   m_state = BackgroundTaskState::RUNNING;
   m_state = m_body(this) ? BackgroundTaskState::COMPLETED : BackgroundTaskState::FAILED;
   m_completionTime = time(nullptr);
   m_completionCondition.set();
}

/**
 * Set task failure reason. Always returns false, so it can be used like
 * <code>
 *    return task->failure(_T("Reason for failure"));
 * </code>
 * in task function.
 */
bool BackgroundTask::failure(const TCHAR *format, ...)
{
   va_list args;
   TCHAR buffer[4096];
   va_start(args, format);
   _vsntprintf(buffer, 4096, format, args);
   va_end(args);
   m_failureReason = buffer;
   return false;
}

/**
 * Unique background task ID
 */
static VolatileCounter64 s_backgroundTaskId = 0;

/**
 * Task registry
 */
static SynchronizedSharedHashMap<uint64_t, BackgroundTask> s_tasks;

/**
 * Completed task retention time
 */
static uint32_t s_taskRetentionTime = 600;

/**
 * Remove old completed tasks from registry
 */
static void TaskRegistryCleanup()
{
   IntegerArray<uint64_t> deleteList;
   while(!SleepAndCheckForShutdown(5))
   {
      time_t now = time(nullptr);
      s_tasks.forEach(
         [now, &deleteList] (const uint64_t& id, const shared_ptr<BackgroundTask>& task) -> EnumerationCallbackResult
         {
            if (task->isFinished() && (task->getCompletionTime() < now - s_taskRetentionTime))
               deleteList.add(id);
            return _CONTINUE;
         });
      for(int i = 0; i < deleteList.size(); i++)
         s_tasks.remove(deleteList.get(i));
      deleteList.clear();
   }
}

/**
 * Create and start new background task
 */
shared_ptr<BackgroundTask> LIBNETXMS_EXPORTABLE CreateBackgroundTask(ThreadPool *p, const std::function<bool (BackgroundTask*)>& f, const TCHAR *description)
{
   auto task = make_shared<BackgroundTask>(InterlockedIncrement64(&s_backgroundTaskId), f, description);
   s_tasks.set(task->getId(), task);
   ThreadPoolExecute(p, task, &BackgroundTask::run);
   if (task->getId() == 1)
   {
      // First execution, start background cleanup thread
      ThreadCreate(TaskRegistryCleanup);
   }
   return task;
}

/**
 * Create and start new serialized background task
 */
shared_ptr<BackgroundTask> LIBNETXMS_EXPORTABLE CreateSerializedBackgroundTask(ThreadPool *p, const TCHAR *key, const std::function<bool (BackgroundTask*)>& f, const TCHAR *description)
{
   auto task = make_shared<BackgroundTask>(InterlockedIncrement64(&s_backgroundTaskId), f, description);
   s_tasks.set(task->getId(), task);
   ThreadPoolExecuteSerialized(p, key, task, &BackgroundTask::run);
   if (task->getId() == 1)
   {
      // First execution, start background cleanup thread
      ThreadCreate(TaskRegistryCleanup);
   }
   return task;
}

/**
 * Get background task from registry by ID
 */
shared_ptr<BackgroundTask> LIBNETXMS_EXPORTABLE GetBackgroundTask(uint64_t id)
{
   return s_tasks.getShared(id);
}

/**
 * Get all registered background tasks
 */
std::vector<shared_ptr<BackgroundTask>> LIBNETXMS_EXPORTABLE GetBackgroundTasks()
{
   std::vector<shared_ptr<BackgroundTask>> tasks;
   s_tasks.forEach(
      [&tasks] (const uint64_t& id, const shared_ptr<BackgroundTask>& task) -> EnumerationCallbackResult
      {
         tasks.push_back(task);
         return _CONTINUE;
      });
   return tasks;
}

/**
 * Set retention time for completed background tasks
 */
void LIBNETXMS_EXPORTABLE SetBackgroundTaskRetentionTime(uint32_t seconds)
{
   s_taskRetentionTime = seconds;
}
