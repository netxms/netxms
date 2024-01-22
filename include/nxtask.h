/*
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
** File: nxtask.h
**
**/

#ifndef _nxtask_h_
#define _nxtask_h_

#include <nms_util.h>
#include <vector>

/**
 * Background task state
 */
enum class BackgroundTaskState
{
   PENDING = 0,
   RUNNING = 1,
   COMPLETED = 2,
   FAILED = 3
};

#ifdef _WIN32
class BackgroundTask;
template class LIBNETXMS_TEMPLATE_EXPORTABLE std::function<bool (BackgroundTask*)>;
#endif

/**
 * Background task
 */
class LIBNETXMS_EXPORTABLE BackgroundTask
{
private:
   uint64_t m_id;
   std::function<bool (BackgroundTask*)> m_body;
   BackgroundTaskState m_state;
   int m_progress;
   time_t m_completionTime;
   MutableString m_failureReason;
   Condition m_completionCondition;
   String m_description;

public:
   BackgroundTask(uint64_t id, const std::function<bool (BackgroundTask*)>& body, const TCHAR *description) : m_body(body), m_completionCondition(true), m_description(description)
   {
      m_id = id;
      m_state = BackgroundTaskState::PENDING;
      m_progress = 0;
      m_completionTime = 0;
   }

   /**
    * Run task. This is internal method that is intended to be called only from thread function.
    */
   void run();

   /**
    * Set task failure reason. Always returns false, so it can be used like
    * <code>
    *    return task->failure(_T("Reason for failure"));
    * </code>
    * in task function.
    */
   bool failure(const TCHAR *format, ...);

   uint64_t getId() const
   {
      return m_id;
   }

   const TCHAR *getDescription() const
   {
      return m_description.cstr();
   }

   BackgroundTaskState getState() const
   {
      return m_state;
   }

   bool isFinished() const
   {
      return m_state >= BackgroundTaskState::COMPLETED;
   }

   time_t getCompletionTime() const
   {
      return m_completionTime;
   }

   bool isFailed() const
   {
      return m_state == BackgroundTaskState::FAILED;
   }

   const String& getFailureReson() const
   {
      return m_failureReason;
   }

   int getProgress() const
   {
      return m_progress;
   }

   void markProgress(int pctCompleted)
   {
      if ((pctCompleted > m_progress) && (pctCompleted <= 100))
         m_progress = pctCompleted;
   }

   /**
    * Wait for task completion
    */
   bool waitForCompletion(uint32_t timeout = INFINITE)
   {
      return m_completionCondition.wait(timeout);
   }
};

/**
 * Create and start new background task
 */
shared_ptr<BackgroundTask> LIBNETXMS_EXPORTABLE CreateBackgroundTask(ThreadPool *p, const std::function<bool (BackgroundTask*)>& f, const TCHAR *description = nullptr);

/**
 * Create and start new serialized background task
 */
shared_ptr<BackgroundTask> LIBNETXMS_EXPORTABLE CreateSerializedBackgroundTask(ThreadPool *p, const TCHAR *key, const std::function<bool (BackgroundTask*)>& f, const TCHAR *description = nullptr);

/**
 * Get background task from registry by ID
 */
shared_ptr<BackgroundTask> LIBNETXMS_EXPORTABLE GetBackgroundTask(uint64_t id);

/**
 * Set retention time for completed background tasks
 */
void LIBNETXMS_EXPORTABLE SetBackgroundTaskRetentionTime(uint32_t seconds);

/**
 * Get all registered background tasks
 */
std::vector<shared_ptr<BackgroundTask>> LIBNETXMS_EXPORTABLE GetBackgroundTasks();

#endif   /* _nxtask_h_ */
