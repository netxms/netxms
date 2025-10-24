/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Raden Solutions
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
** File: schedule.cpp
**
**/

#include "nxcore.h"

#define DEBUG_TAG _T("scheduler")

/**
 * Static fields
 */
static StringObjectMap<SchedulerCallback> s_callbacks(Ownership::True);
static ObjectArray<ScheduledTask> s_recurrentTasks(64, 64, Ownership::True);
static ObjectArray<ScheduledTask> s_oneTimeTasks(64, 64, Ownership::True);
static ObjectArray<ScheduledTask> s_completedOneTimeTasks(64, 64, Ownership::True);
static Condition s_wakeupCondition(false);
static Mutex s_recurrentTaskLock;
static Mutex s_oneTimeTaskLock;
static VolatileCounter64 s_taskId = 0; // Last used task ID

/**
 * Scheduled task execution pool
 */
ThreadPool *g_schedulerThreadPool = nullptr;

/**
 * Task handler replacement for missing handlers
 */
static void MissingTaskHandler(const shared_ptr<ScheduledTaskParameters>& parameters)
{
}

/**
 * Delayed delete of scheduled task
 */
static void DelayedTaskDelete(void *taskId)
{
   while(IsScheduledTaskRunning(CAST_FROM_POINTER(taskId, uint32_t)))
      ThreadSleep(10);
   DeleteScheduledTask(CAST_FROM_POINTER(taskId, uint32_t), 0, SYSTEM_ACCESS_FULL);
}

/**
 * Removes scheduled task from database by id
 */
static void DeleteScheduledTaskFromDB(uint64_t id)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   ExecuteQueryOnObject(hdb, id, L"DELETE FROM scheduled_tasks WHERE id=?");
   DBConnectionPoolReleaseConnection(hdb);
   NotifyClientSessions(NX_NOTIFY_SCHEDULE_UPDATE, 0);
}

/**
 * Callback definition for missing task handlers
 */
static SchedulerCallback s_missingTaskHandler(MissingTaskHandler, 0);

/**
 * Constructor for scheduled task transient data
 */
ScheduledTaskTransientData::ScheduledTaskTransientData()
{
}

/**
 * Destructor for scheduled task transient data
 */
ScheduledTaskTransientData::~ScheduledTaskTransientData()
{
}

/**
 * Create recurrent task object
 */
ScheduledTask::ScheduledTask(uint64_t id, const TCHAR *taskHandlerId, const TCHAR *schedule,
         shared_ptr<ScheduledTaskParameters> parameters, bool systemTask) :
         m_taskHandlerId(taskHandlerId), m_schedule(schedule), m_parameters(parameters)
{
   m_id = id;
   m_scheduledExecutionTime = TIMESTAMP_NEVER;
   m_lastExecutionTime = TIMESTAMP_NEVER;
   m_recurrent = true;
   m_flags = systemTask ? SCHEDULED_TASK_SYSTEM : 0;
}

/**
 * Create one-time execution task object
 */
ScheduledTask::ScheduledTask(uint64_t id, const TCHAR *taskHandlerId, time_t executionTime,
         shared_ptr<ScheduledTaskParameters> parameters, bool systemTask) :
         m_taskHandlerId(taskHandlerId), m_parameters(parameters)
{
   m_id = id;
   m_scheduledExecutionTime = executionTime;
   m_lastExecutionTime = TIMESTAMP_NEVER;
   m_recurrent = false;
   m_flags = systemTask ? SCHEDULED_TASK_SYSTEM : 0;
}

/**
 * Create task object from database record
 * Expected field order: id,taskid,schedule,params,execution_time,last_execution_time,flags,owner,object_id,comments,task_key
 */
ScheduledTask::ScheduledTask(DB_RESULT hResult, int row)
{
   m_id = DBGetFieldUInt64(hResult, row, 0);
   m_taskHandlerId = DBGetFieldAsSharedString(hResult, row, 1);
   m_schedule = DBGetFieldAsSharedString(hResult, row, 2);
   m_scheduledExecutionTime = DBGetFieldULong(hResult, row, 4);
   m_lastExecutionTime = DBGetFieldULong(hResult, row, 5);
   m_flags = DBGetFieldULong(hResult, row, 6);
   m_recurrent = !m_schedule.isEmpty();

   wchar_t persistentData[1024];
   DBGetField(hResult, row, 3, persistentData, 1024);
   uint32_t userId = DBGetFieldULong(hResult, row, 7);
   uint32_t objectId = DBGetFieldULong(hResult, row, 8);
   wchar_t key[256], comments[256];
   DBGetField(hResult, row, 9, comments, 256);
   DBGetField(hResult, row, 10, key, 256);
   m_parameters = make_shared<ScheduledTaskParameters>(key, userId, objectId, persistentData, nullptr, comments);
}

/**
 * Update task
 */
void ScheduledTask::update(const TCHAR *taskHandlerId, const TCHAR *schedule,
         shared_ptr<ScheduledTaskParameters> parameters, bool systemTask, bool disabled)
{
   lock();

   m_taskHandlerId = CHECK_NULL_EX(taskHandlerId);
   m_schedule = CHECK_NULL_EX(schedule);
   m_parameters = parameters;
   m_recurrent = true;

   if (systemTask)
      m_flags |= SCHEDULED_TASK_SYSTEM;
   else
      m_flags &= ~SCHEDULED_TASK_SYSTEM;

   if (disabled)
      m_flags |= SCHEDULED_TASK_DISABLED;
   else
      m_flags &= ~SCHEDULED_TASK_DISABLED;

   unlock();
}

/**
 * Update task
 */
void ScheduledTask::update(const TCHAR *taskHandlerId, time_t nextExecution,
         shared_ptr<ScheduledTaskParameters> parameters, bool systemTask, bool disabled)
{
   lock();

   m_taskHandlerId = CHECK_NULL_EX(taskHandlerId);
   m_schedule = L"";
   m_parameters = parameters;
   m_scheduledExecutionTime = nextExecution;
   m_recurrent = false;

   if (systemTask)
      m_flags |= SCHEDULED_TASK_SYSTEM;
   else
      m_flags &= ~SCHEDULED_TASK_SYSTEM;

   if (disabled)
      m_flags |= SCHEDULED_TASK_DISABLED;
   else
      m_flags &= ~SCHEDULED_TASK_DISABLED;

   unlock();
}

/**
 * Save task to database
 */
void ScheduledTask::saveToDatabase(bool newObject) const
{
   DB_HANDLE db = DBConnectionPoolAcquireConnection();

   DB_STATEMENT hStmt;
   if (newObject)
   {
		hStmt = DBPrepare(db,
                    _T("INSERT INTO scheduled_tasks (taskId,schedule,params,execution_time,")
                    _T("last_execution_time,flags,owner,object_id,comments,task_key,id) VALUES (?,?,?,?,?,?,?,?,?,?,?)"));
   }
   else
   {
      hStmt = DBPrepare(db,
                    _T("UPDATE scheduled_tasks SET taskId=?,schedule=?,params=?,")
                    _T("execution_time=?,last_execution_time=?,flags=?,owner=?,object_id=?,")
                    _T("comments=?,task_key=? WHERE id=?"));
   }

   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_taskHandlerId, DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_schedule, DB_BIND_STATIC);
      DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_parameters->m_persistentData, DB_BIND_STATIC, 1023);
      DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, static_cast<int64_t>(m_scheduledExecutionTime));
      DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, static_cast<int64_t>(m_lastExecutionTime));
      DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_flags);
      DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_parameters->m_userId);
      DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_parameters->m_objectId);
      DBBind(hStmt, 9, DB_SQLTYPE_VARCHAR, m_parameters->m_comments, DB_BIND_STATIC, 255);
      DBBind(hStmt, 10, DB_SQLTYPE_VARCHAR, m_parameters->m_taskKey, DB_BIND_STATIC, 255);
      DBBind(hStmt, 11, DB_SQLTYPE_BIGINT, m_id);

      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
	DBConnectionPoolReleaseConnection(db);
	NotifyClientSessions(NX_NOTIFY_SCHEDULE_UPDATE, 0);
}

/**
 * Scheduled task comparator (used for task sorting)
 */
static int ScheduledTaskComparator(const ScheduledTask **e1, const ScheduledTask **e2)
{
   return COMPARE_NUMBERS((*e1)->getScheduledExecutionTime(), (*e2)->getScheduledExecutionTime());
}

/**
 * Start task execution
 */
void ScheduledTask::startExecution(SchedulerCallback *callback)
{
   lock();
   if (!(m_flags & SCHEDULED_TASK_RUNNING) && !((m_flags & SCHEDULED_TASK_COMPLETED) && !m_recurrent))
   {
      m_flags |= SCHEDULED_TASK_RUNNING;
      ThreadPoolExecute(g_schedulerThreadPool, this, &ScheduledTask::run, callback);
   }
   else
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG,
               _T("Internal error - attempt to start execution of already running or completed scheduled task %s [%u]"),
               m_taskHandlerId.cstr(), m_id);
   }
   unlock();
}

/**
 * Run scheduled task
 */
void ScheduledTask::run(SchedulerCallback *callback)
{
   if (!m_recurrent)
   {
      uint32_t diff = static_cast<uint32_t>(time(nullptr) - m_scheduledExecutionTime);
      if (diff > 0)
         nxlog_debug_tag(DEBUG_TAG, 6, _T("Scheduled task [%u] execution delayed by %u seconds"), m_id, diff);
   }

	NotifyClientSessions(NX_NOTIFY_SCHEDULE_UPDATE, 0);

   callback->m_handler(m_parameters);
   nxlog_debug_tag(DEBUG_TAG, 6, _T("Scheduled task [%u] execution completed"), m_id);

   lock();
   m_lastExecutionTime = time(nullptr);
   m_flags &= ~SCHEDULED_TASK_RUNNING;
   m_flags |= SCHEDULED_TASK_COMPLETED;
   saveToDatabase(false);
   bool isSystemTask = ((m_flags & SCHEDULED_TASK_SYSTEM) != 0);
   bool recurrent = m_recurrent;
   uint64_t id = m_id;
   unlock();

   // After this point task can be deleted from outside because SCHEDULED_TASK_RUNNING flag is cleared
   // No access to class members should be made

   if (!recurrent)
   {
      s_oneTimeTaskLock.lock();
      for(int i = 0; i < s_oneTimeTasks.size(); i++)
      {
         ScheduledTask *task = s_oneTimeTasks.get(i);
         if (task->m_id == id)
         {
            if (isSystemTask)
            {
               s_oneTimeTasks.remove(i);
               DeleteScheduledTaskFromDB(id);
            }
            else
            {
               s_oneTimeTasks.unlink(i);
               s_completedOneTimeTasks.add(task);
            }
            break;
         }
      }
      s_oneTimeTaskLock.unlock();
   }
}

/**
 * Fill NXCP message with task data
 */
void ScheduledTask::fillMessage(NXCPMessage *msg) const
{
   lock();
   msg->setField(VID_SCHEDULED_TASK_ID, m_id);
   msg->setField(VID_TASK_HANDLER, m_taskHandlerId);
   msg->setField(VID_SCHEDULE, m_schedule);
   msg->setField(VID_PARAMETER, m_parameters->m_persistentData);
   msg->setFieldFromTime(VID_EXECUTION_TIME, m_scheduledExecutionTime);
   msg->setFieldFromTime(VID_LAST_EXECUTION_TIME, m_lastExecutionTime);
   msg->setField(VID_FLAGS, m_flags);
   msg->setField(VID_OWNER, m_parameters->m_userId);
   msg->setField(VID_OBJECT_ID, m_parameters->m_objectId);
   msg->setField(VID_COMMENTS, m_parameters->m_comments);
   msg->setField(VID_TASK_KEY, m_parameters->m_taskKey);
   unlock();
}

/**
 * Fill NXCP message with task data
 */
void ScheduledTask::fillMessage(NXCPMessage *msg, uint32_t base) const
{
   lock();
   msg->setField(base, m_id);
   msg->setField(base + 1, m_taskHandlerId);
   msg->setField(base + 2, m_schedule);
   msg->setField(base + 3, m_parameters->m_persistentData);
   msg->setFieldFromTime(base + 4, m_scheduledExecutionTime);
   msg->setFieldFromTime(base + 5, m_lastExecutionTime);
   msg->setField(base + 6, m_flags);
   msg->setField(base + 7, m_parameters->m_userId);
   msg->setField(base + 8, m_parameters->m_objectId);
   msg->setField(base + 9, m_parameters->m_comments);
   msg->setField(base + 10, m_parameters->m_taskKey);
   unlock();
}

/**
 * Check if user can access this scheduled task
 */
bool ScheduledTask::canAccess(uint32_t userId, uint64_t systemAccess) const
{
   if (userId == 0)
      return true;

   if (systemAccess & SYSTEM_ACCESS_ALL_SCHEDULED_TASKS)
      return true;

   bool result = false;
   if (systemAccess & SYSTEM_ACCESS_USER_SCHEDULED_TASKS)
   {
      result = !isSystem();
   }
   else if (systemAccess & SYSTEM_ACCESS_OWN_SCHEDULED_TASKS)
   {
      lock();
      result = (userId == m_parameters->m_userId);
      unlock();
   }
   return result;
}

/**
 * Function that adds to list task handler function
 */
void NXCORE_EXPORTABLE RegisterSchedulerTaskHandler(const wchar_t *id, ScheduledTaskHandler handler, uint64_t accessRight)
{
   s_callbacks.set(id, new SchedulerCallback(handler, accessRight));
   nxlog_debug_tag(DEBUG_TAG, 6, L"Registered scheduler task %s", id);
}

/**
 * Scheduled task creation function
 */
uint32_t NXCORE_EXPORTABLE AddRecurrentScheduledTask(const TCHAR *taskHandlerId, const TCHAR *schedule, const TCHAR *persistentData,
         ScheduledTaskTransientData *transientData, uint32_t owner, uint32_t objectId, uint64_t systemRights,
         const TCHAR *comments, const TCHAR *key, bool systemTask)
{
   if ((systemRights & (SYSTEM_ACCESS_ALL_SCHEDULED_TASKS | SYSTEM_ACCESS_USER_SCHEDULED_TASKS | SYSTEM_ACCESS_OWN_SCHEDULED_TASKS)) == 0)
      return RCC_ACCESS_DENIED;

   nxlog_debug_tag(DEBUG_TAG, 7, _T("AddSchedule: Add recurrent task %s, %s, %s"), taskHandlerId, schedule, persistentData);
   auto task = new ScheduledTask(InterlockedIncrement64(&s_taskId), taskHandlerId, schedule,
            make_shared<ScheduledTaskParameters>(key, owner, objectId, persistentData, transientData, comments), systemTask);

   s_recurrentTaskLock.lock();
   task->saveToDatabase(true);
   s_recurrentTasks.add(task);
   s_recurrentTaskLock.unlock();

   return RCC_SUCCESS;
}

/**
 * Create scheduled task only if task with same task ID does not exist
 * @param schedule is crontab
 */
uint32_t NXCORE_EXPORTABLE AddUniqueRecurrentScheduledTask(const TCHAR *taskHandlerId, const TCHAR *schedule, const TCHAR *persistentData,
         ScheduledTaskTransientData *transientData, uint32_t owner, uint32_t objectId, uint64_t systemRights,
         const TCHAR *comments, const TCHAR *key, bool systemTask)
{
   ScheduledTask *task = FindScheduledTaskByHandlerId(taskHandlerId);
   if (task != nullptr)
   {
      // Make sure that existing task marked as system if requested
      if (!task->isSystem() && systemTask)
      {
         task->setSystem();
         task->saveToDatabase(false);
      }
      return RCC_SUCCESS;
   }
   return AddRecurrentScheduledTask(taskHandlerId, schedule, persistentData, transientData, owner, objectId, systemRights, comments, key, systemTask);
}

/**
 * Add one time scheduled task to ordered task list
 */
static void AddOneTimeTaskToList(ScheduledTask *task)
{
   if (s_oneTimeTasks.isEmpty() || (task->getScheduledExecutionTime() >= s_oneTimeTasks.get(s_oneTimeTasks.size() - 1)->getScheduledExecutionTime()))
   {
      s_oneTimeTasks.add(task);
      return;
   }

   for(int i = 0; i < s_oneTimeTasks.size(); i++)
   {
      if (task->getScheduledExecutionTime() <= s_oneTimeTasks.get(i)->getScheduledExecutionTime())
      {
         s_oneTimeTasks.insert(i, task);
         break;
      }
   }
}

/**
 * One time schedule creation function
 */
uint32_t NXCORE_EXPORTABLE AddOneTimeScheduledTask(const TCHAR *taskHandlerId, time_t nextExecutionTime, const TCHAR *persistentData,
         ScheduledTaskTransientData *transientData, uint32_t owner, uint32_t objectId, uint64_t systemRights,
         const TCHAR *comments, const TCHAR *key, bool systemTask)
{
   if ((systemRights & (SYSTEM_ACCESS_ALL_SCHEDULED_TASKS | SYSTEM_ACCESS_USER_SCHEDULED_TASKS | SYSTEM_ACCESS_OWN_SCHEDULED_TASKS)) == 0)
      return RCC_ACCESS_DENIED;

   nxlog_debug_tag(DEBUG_TAG, 5, _T("AddOneTimeAction: Add one time schedule %s, %d, %s"), taskHandlerId, nextExecutionTime, persistentData);
   auto task = new ScheduledTask(InterlockedIncrement64(&s_taskId), taskHandlerId, nextExecutionTime,
            make_shared<ScheduledTaskParameters>(key, owner, objectId, persistentData, transientData, comments), systemTask);

   s_oneTimeTaskLock.lock();
   task->saveToDatabase(true);
   AddOneTimeTaskToList(task);
   s_oneTimeTaskLock.unlock();
   s_wakeupCondition.set();

   return RCC_SUCCESS;
}

/**
 * Recurrent scheduled task update
 */
uint32_t NXCORE_EXPORTABLE UpdateRecurrentScheduledTask(uint64_t id, const TCHAR *taskHandlerId, const TCHAR *schedule, const TCHAR *persistentData,
         ScheduledTaskTransientData *transientData, const TCHAR *comments, uint32_t owner, uint32_t objectId,
         uint64_t systemAccessRights, bool disabled)
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("UpdateRecurrentScheduledTask: task [%u]: handler=%s, schedule=%s, data=%s"), id, taskHandlerId, schedule, persistentData);

   uint32_t rcc = RCC_SUCCESS;

   bool found = false;
   s_recurrentTaskLock.lock();
   for (int i = 0; i < s_recurrentTasks.size(); i++)
   {
      ScheduledTask *task = s_recurrentTasks.get(i);
      if (task->getId() == id)
      {
         if (!task->canAccess(owner, systemAccessRights))
         {
            rcc = RCC_ACCESS_DENIED;
            break;
         }
         task->update(taskHandlerId, schedule,
                  make_shared<ScheduledTaskParameters>(task->getTaskKey(), owner, objectId, persistentData, transientData, comments),
                  task->isSystem(), disabled);
         task->saveToDatabase(false);
         found = true;
         break;
      }
   }
   s_recurrentTaskLock.unlock();

   if (!found)
   {
      // check in different queue and if exists - remove from one and add to another
      ScheduledTask *task = nullptr;
      s_oneTimeTaskLock.lock();

      for(int i = 0; i < s_oneTimeTasks.size(); i++)
      {
         if (s_oneTimeTasks.get(i)->getId() == id)
         {
            if (!s_oneTimeTasks.get(i)->canAccess(owner, systemAccessRights))
            {
               rcc = RCC_ACCESS_DENIED;
               break;
            }
            task = s_oneTimeTasks.get(i);
            s_oneTimeTasks.unlink(i);
            task->update(taskHandlerId, schedule,
                     make_shared<ScheduledTaskParameters>(task->getTaskKey(), owner, objectId, persistentData, transientData, comments),
                     task->isSystem(), disabled);
            task->saveToDatabase(false);
            found = true;
            break;
         }
      }

      for(int i = 0; i < s_completedOneTimeTasks.size(); i++)
      {
         if (s_completedOneTimeTasks.get(i)->getId() == id)
         {
            if (!s_completedOneTimeTasks.get(i)->canAccess(owner, systemAccessRights))
            {
               rcc = RCC_ACCESS_DENIED;
               break;
            }
            task = s_completedOneTimeTasks.get(i);
            s_completedOneTimeTasks.unlink(i);
            task->update(taskHandlerId, schedule,
                     make_shared<ScheduledTaskParameters>(task->getTaskKey(), owner, objectId, persistentData, transientData, comments),
                     task->isSystem(), disabled);
            task->saveToDatabase(false);
            found = true;
            break;
         }
      }

      s_oneTimeTaskLock.unlock();

      if (found && (task != nullptr))
      {
         s_recurrentTaskLock.lock();
         s_recurrentTasks.add(task);
         s_recurrentTaskLock.unlock();
      }
   }

   return rcc;
}

/**
 * One time action update
 */
uint32_t NXCORE_EXPORTABLE UpdateOneTimeScheduledTask(uint64_t id, const TCHAR *taskHandlerId, time_t nextExecutionTime, const TCHAR *persistentData,
         ScheduledTaskTransientData *transientData, const TCHAR *comments, uint32_t owner, uint32_t objectId,
         uint64_t systemAccessRights, bool disabled)
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("UpdateOneTimeScheduledTask: task [%u]: handler=%s, time=") INT64_FMT _T(", data=%s"),
            id, taskHandlerId, static_cast<int64_t>(nextExecutionTime), persistentData);

   uint32_t rcc = RCC_SUCCESS;

   bool found = false;
   s_oneTimeTaskLock.lock();

   for (int i = 0; i < s_oneTimeTasks.size(); i++)
   {
      ScheduledTask *task = s_oneTimeTasks.get(i);
      if (task->getId() == id)
      {
         if (!task->canAccess(owner, systemAccessRights))
         {
            rcc = RCC_ACCESS_DENIED;
            break;
         }
         task->update(taskHandlerId, nextExecutionTime,
                  make_shared<ScheduledTaskParameters>(task->getTaskKey(), owner, objectId, persistentData, transientData, comments),
                  task->isSystem(), disabled);
         task->saveToDatabase(false);
         s_oneTimeTasks.sort(ScheduledTaskComparator);
         found = true;
         break;
      }
   }

   for (int i = 0; !found && (i < s_completedOneTimeTasks.size()); i++)
   {
      ScheduledTask *task = s_completedOneTimeTasks.get(i);
      if (task->getId() == id)
      {
         if (!task->canAccess(owner, systemAccessRights))
         {
            rcc = RCC_ACCESS_DENIED;
            break;
         }
         task->update(taskHandlerId, nextExecutionTime,
                  make_shared<ScheduledTaskParameters>(task->getTaskKey(), owner, objectId, persistentData, transientData, comments),
                  task->isSystem(), disabled);
         task->saveToDatabase(false);
         found = true;
      }
   }

   s_oneTimeTaskLock.unlock();

   if (!found && (rcc == RCC_SUCCESS))
   {
      // check in different queue and if exists - remove from one and add to another
      ScheduledTask *task = nullptr;
      s_recurrentTaskLock.lock();
      for (int i = 0; i < s_recurrentTasks.size(); i++)
      {
         if (s_recurrentTasks.get(i)->getId() == id)
         {
            if (!s_recurrentTasks.get(i)->canAccess(owner, systemAccessRights))
            {
               rcc = RCC_ACCESS_DENIED;
               break;
            }
            task = s_recurrentTasks.get(i);
            s_recurrentTasks.unlink(i);
            task->update(taskHandlerId, nextExecutionTime,
                     make_shared<ScheduledTaskParameters>(task->getTaskKey(), owner, objectId, persistentData, transientData, comments),
                     task->isSystem(), disabled);
            task->saveToDatabase(false);
            found = true;
            break;
         }
      }
      s_recurrentTaskLock.unlock();

      if (found && (task != nullptr))
      {
         s_oneTimeTaskLock.lock();
         AddOneTimeTaskToList(task);
         s_oneTimeTaskLock.unlock();
      }
   }

   if (found)
      s_wakeupCondition.set();
   return rcc;
}

/**
 * Removes scheduled task by id
 */
uint32_t NXCORE_EXPORTABLE DeleteScheduledTask(uint64_t id, uint32_t user, uint64_t systemRights)
{
   uint32_t rcc = RCC_INVALID_OBJECT_ID;

   s_recurrentTaskLock.lock();
   for(int i = 0; i < s_recurrentTasks.size(); i++)
   {
      ScheduledTask *task = s_recurrentTasks.get(i);
      if (task->getId() == id)
      {
         if (!task->canAccess(user, systemRights))
         {
            rcc = RCC_ACCESS_DENIED;
            break;
         }
         if (task->isRunning())
         {
            rcc = RCC_RESOURCE_BUSY;
            break;
         }
         s_recurrentTasks.remove(i);
         rcc = RCC_SUCCESS;
         break;
      }
   }
   s_recurrentTaskLock.unlock();

   if (rcc == RCC_INVALID_OBJECT_ID)
   {
      s_oneTimeTaskLock.lock();
      for(int i = 0; i < s_oneTimeTasks.size(); i++)
      {
         ScheduledTask *task = s_oneTimeTasks.get(i);
         if (task->getId() == id)
         {
            if (!task->canAccess(user, systemRights))
            {
               rcc = RCC_ACCESS_DENIED;
               break;
            }
            if (task->isRunning())
            {
               rcc = RCC_RESOURCE_BUSY;
               break;
            }
            s_oneTimeTasks.remove(i);
            s_wakeupCondition.set();
            rcc = RCC_SUCCESS;
            break;
         }
      }

      if (rcc == RCC_INVALID_OBJECT_ID)
      {
         for(int i = 0; i < s_completedOneTimeTasks.size(); i++)
         {
            ScheduledTask *task = s_completedOneTimeTasks.get(i);
            if (task->getId() == id)
            {
               if (!task->canAccess(user, systemRights))
               {
                  rcc = RCC_ACCESS_DENIED;
                  break;
               }
               s_completedOneTimeTasks.remove(i);
               rcc = RCC_SUCCESS;
               break;
            }
         }
      }

      s_oneTimeTaskLock.unlock();
   }

   if (rcc == RCC_SUCCESS)
   {
      DeleteScheduledTaskFromDB(id);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("DeleteScheduledTask: task [%u] removed"), id);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("DeleteScheduledTask: task [%u] cannot be removed (RCC=%u)"), id, rcc);
   }
   return rcc;
}

/**
 * Find scheduled task by task handler id
 */
ScheduledTask NXCORE_EXPORTABLE *FindScheduledTaskByHandlerId(const TCHAR *taskHandlerId)
{
   ScheduledTask *task;
   bool found = false;

   s_recurrentTaskLock.lock();
   for (int i = 0; i < s_recurrentTasks.size(); i++)
   {
      if (_tcscmp(s_recurrentTasks.get(i)->getTaskHandlerId(), taskHandlerId) == 0)
      {
         task = s_recurrentTasks.get(i);
         found = true;
         break;
      }
   }
   s_recurrentTaskLock.unlock();

   if (found)
      return task;

   s_oneTimeTaskLock.lock();
   for (int i = 0; i < s_oneTimeTasks.size(); i++)
   {
      if (_tcscmp(s_oneTimeTasks.get(i)->getTaskHandlerId(), taskHandlerId) == 0)
      {
         task = s_oneTimeTasks.get(i);
         found = true;
         break;
      }
   }
   if (!found)
   {
      for (int i = 0; i < s_completedOneTimeTasks.size(); i++)
      {
         if (_tcscmp(s_completedOneTimeTasks.get(i)->getTaskHandlerId(), taskHandlerId) == 0)
         {
            task = s_completedOneTimeTasks.get(i);
            found = true;
            break;
         }
      }
   }
   s_oneTimeTaskLock.unlock();

   if (found)
      return task;

   return NULL;
}

/**
 * Delete scheduled task(s) by task handler id from specific task category
 */
static void DeleteScheduledTaskByHandlerId(ObjectArray<ScheduledTask> *category, const TCHAR *taskHandlerId, IntegerArray<uint64_t> *deleteList)
{
   for (int i = 0; i < category->size(); i++)
   {
      ScheduledTask *task = category->get(i);
      if (!_tcscmp(task->getTaskHandlerId(), taskHandlerId))
      {
         if (!task->isRunning())
         {
            deleteList->add(task->getId());
            category->remove(i);
            i--;
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("Delete of scheduled task [%u] delayed because task is still running"), task->getId());
            task->disable();  // Prevent re-run
            ThreadPoolExecuteSerialized(g_schedulerThreadPool, _T("DeleteTask"), DelayedTaskDelete, CAST_TO_POINTER(task->getId(), void*));
         }
      }
   }
}

/**
 * Delete scheduled task(s) by task handler id
 */
bool NXCORE_EXPORTABLE DeleteScheduledTaskByHandlerId(const TCHAR *taskHandlerId)
{
   IntegerArray<uint64_t> deleteList;

   s_oneTimeTaskLock.lock();
   DeleteScheduledTaskByHandlerId(&s_oneTimeTasks, taskHandlerId, &deleteList);
   DeleteScheduledTaskByHandlerId(&s_completedOneTimeTasks, taskHandlerId, &deleteList);
   s_oneTimeTaskLock.unlock();

   s_recurrentTaskLock.lock();
   DeleteScheduledTaskByHandlerId(&s_recurrentTasks, taskHandlerId, &deleteList);
   s_recurrentTaskLock.unlock();

   for(int i = 0; i < deleteList.size(); i++)
   {
      DeleteScheduledTaskFromDB(deleteList.get(i));
   }

   return !deleteList.isEmpty();
}

/**
 * Delete scheduled task(s) by task key from given task category
 */
static void DeleteScheduledTasksByKey(ObjectArray<ScheduledTask> *category, const TCHAR *taskKey, IntegerArray<uint64_t> *deleteList)
{
   for (int i = 0; i < category->size(); i++)
   {
      ScheduledTask *task = category->get(i);
      if (!_tcscmp(task->getTaskKey(), taskKey))
      {
         if (!task->isRunning())
         {
            deleteList->add(task->getId());
            category->remove(i);
            i--;
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("Delete of scheduled task [%u] delayed because task is still running"), task->getId());
            task->disable();  // Prevent re-run
            ThreadPoolExecuteSerialized(g_schedulerThreadPool, _T("DeleteTask"), DelayedTaskDelete, CAST_TO_POINTER(task->getId(), void*));
         }
      }
   }
}

/**
 * Delete scheduled task(s) by task key
 */
int NXCORE_EXPORTABLE DeleteScheduledTasksByKey(const TCHAR *taskKey)
{
   IntegerArray<uint64_t> deleteList;

   s_oneTimeTaskLock.lock();
   DeleteScheduledTasksByKey(&s_oneTimeTasks, taskKey, &deleteList);
   DeleteScheduledTasksByKey(&s_completedOneTimeTasks, taskKey, &deleteList);
   s_oneTimeTaskLock.unlock();

   s_recurrentTaskLock.lock();
   DeleteScheduledTasksByKey(&s_recurrentTasks, taskKey, &deleteList);
   s_recurrentTaskLock.unlock();

   for(int i = 0; i < deleteList.size(); i++)
   {
      DeleteScheduledTaskFromDB(deleteList.get(i));
   }

   return deleteList.size();
}

/**
 * Delete scheduled task(s) by associated object id
 */
static void DeleteScheduledTasksByObjectId(ObjectArray<ScheduledTask> *category, uint32_t objectId, bool allTasks, IntegerArray<uint64_t> *deleteList)
{
   for (int i = 0; i < category->size(); i++)
   {
      ScheduledTask *task = category->get(i);
      if (task->getObjectId() == objectId && (allTasks || task->isSystem()))
      {
         if (!task->isRunning())
         {
            deleteList->add(task->getId());
            category->remove(i);
            i--;
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("Delete of scheduled task [%u] delayed because task is still running"), task->getId());
            task->disable();  // Prevent re-run
            ThreadPoolExecuteSerialized(g_schedulerThreadPool, _T("DeleteTask"), DelayedTaskDelete, CAST_TO_POINTER(task->getId(), void*));
         }
      }
   }

}

/**
 * Delete scheduled task(s) by associated object id
 */
int NXCORE_EXPORTABLE DeleteScheduledTasksByObjectId(uint32_t objectId, bool allTasks)
{
   IntegerArray<uint64_t> deleteList;

   s_oneTimeTaskLock.lock();
   DeleteScheduledTasksByObjectId(&s_oneTimeTasks, objectId, allTasks, &deleteList);
   DeleteScheduledTasksByObjectId(&s_completedOneTimeTasks, objectId, allTasks, &deleteList);
   s_oneTimeTaskLock.unlock();

   s_recurrentTaskLock.lock();
   DeleteScheduledTasksByObjectId(&s_recurrentTasks, objectId, allTasks, &deleteList);
   s_recurrentTaskLock.unlock();

   for(int i = 0; i < deleteList.size(); i++)
   {
      DeleteScheduledTaskFromDB(deleteList.get(i));
   }

   return deleteList.size();
}

/**
 * Get number of scheduled tasks with given key
 */
int NXCORE_EXPORTABLE CountScheduledTasksByKey(const TCHAR *taskKey)
{
   int count = 0;

   s_oneTimeTaskLock.lock();
   for (int i = 0; i < s_oneTimeTasks.size(); i++)
   {
      const TCHAR *k = s_oneTimeTasks.get(i)->getTaskKey();
      if ((k != nullptr) && !_tcscmp(k, taskKey))
      {
         count++;
      }
   }
   for (int i = 0; i < s_completedOneTimeTasks.size(); i++)
   {
      const TCHAR *k = s_completedOneTimeTasks.get(i)->getTaskKey();
      if ((k != nullptr) && !_tcscmp(k, taskKey))
      {
         count++;
      }
   }
   s_oneTimeTaskLock.unlock();

   s_recurrentTaskLock.lock();
   for (int i = 0; i < s_recurrentTasks.size(); i++)
   {
      const TCHAR *k = s_recurrentTasks.get(i)->getTaskKey();
      if ((k != nullptr) && !_tcscmp(k, taskKey))
      {
         count++;
      }
   }
   s_recurrentTaskLock.unlock();

   return count;
}

/**
 * Check if scheduled task with given ID is currently running
 */
bool NXCORE_EXPORTABLE IsScheduledTaskRunning(uint64_t taskId)
{
   bool found = false, running = false;

   s_oneTimeTaskLock.lock();
   for (int i = 0; i < s_oneTimeTasks.size(); i++)
   {
      ScheduledTask *task = s_oneTimeTasks.get(i);
      if (task->getId() == taskId)
      {
         found = true;
         running = task->isRunning();
         break;
      }
   }
   s_oneTimeTaskLock.unlock();

   if (found)
      return running;

   s_recurrentTaskLock.lock();
   for (int i = 0; i < s_recurrentTasks.size(); i++)
   {
      ScheduledTask *task = s_recurrentTasks.get(i);
      if (task->getId() == taskId)
      {
         running = task->isRunning();
         break;
      }
   }
   s_recurrentTaskLock.unlock();

   return running;
}

/**
 * Fills message with scheduled tasks list
 */
void GetScheduledTasks(NXCPMessage *msg, uint32_t userId, uint64_t systemRights, bool (*filter)(const ScheduledTask *task, void *context), void *context)
{
   uint32_t taskCount = 0;
   uint32_t fieldId = VID_SCHEDULE_LIST_BASE;

   s_oneTimeTaskLock.lock();
   for(int i = 0; i < s_oneTimeTasks.size(); i++)
   {
      ScheduledTask *task = s_oneTimeTasks.get(i);
      if (task->canAccess(userId, systemRights) && ((filter == nullptr) || filter(task, context)))
      {
         task->fillMessage(msg, fieldId);
         taskCount++;
         fieldId += 100;
      }
   }
   for(int i = 0; i < s_completedOneTimeTasks.size(); i++)
   {
      ScheduledTask *task = s_completedOneTimeTasks.get(i);
      if (task->canAccess(userId, systemRights) && ((filter == nullptr) || filter(task, context)))
      {
         task->fillMessage(msg, fieldId);
         taskCount++;
         fieldId += 100;
      }
   }
   s_oneTimeTaskLock.unlock();

   s_recurrentTaskLock.lock();
   for(int i = 0; i < s_recurrentTasks.size(); i++)
   {
      ScheduledTask *task = s_recurrentTasks.get(i);
      if (task->canAccess(userId, systemRights) && ((filter == nullptr) || filter(task, context)))
      {
         task->fillMessage(msg, fieldId);
         taskCount++;
         fieldId += 100;
      }
   }
   s_recurrentTaskLock.unlock();

   msg->setField(VID_SCHEDULE_COUNT, taskCount);
}

/**
 * Fills message with task handlers list
 */
void GetSchedulerTaskHandlers(NXCPMessage *msg, uint64_t accessRights)
{
   uint32_t fieldId = VID_CALLBACK_BASE;
   uint32_t count = 0;

   StringList keyList = s_callbacks.keys();
   for(int i = 0; i < keyList.size(); i++)
   {
      if (accessRights & s_callbacks.get(keyList.get(i))->m_accessRight)
      {
         msg->setField(fieldId, keyList.get(i));
         count++;
         fieldId++;
      }
   }
   msg->setField(VID_CALLBACK_COUNT, count);
}

/**
 * Creates scheduled task from message
 */
uint32_t CreateScheduledTaskFromMsg(const NXCPMessage& request, uint32_t owner, uint64_t systemAccessRights)
{
   TCHAR *taskHandler = request.getFieldAsString(VID_TASK_HANDLER);
   SchedulerCallback *callback = s_callbacks.get(taskHandler);
   if ((callback != nullptr) && ((callback->m_accessRight == 0) || ((callback->m_accessRight & systemAccessRights) != callback->m_accessRight)))
   {
      // Access rights set to 0 for system task handlers that could not be scheduled by user
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Attempt to create scheduled task with handler %s by user [%u] failed because of insufficient rights"), taskHandler, owner);
      MemFree(taskHandler);
      return RCC_ACCESS_DENIED;
   }

   uint32_t objectId = request.getFieldAsInt32(VID_OBJECT_ID);
   if (!_tcsncmp(taskHandler, _T("Maintenance."), 12))   // Do additional check on maintenance enter/leave
   {
      shared_ptr<NetObj> object = FindObjectById(objectId);
      if (object == nullptr)
      {
         MemFree(taskHandler);
         return RCC_INVALID_OBJECT_ID;
      }
      if (!object->checkAccessRights(owner, OBJECT_ACCESS_MAINTENANCE))
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Attempt to create scheduled task with handler %s by user [%u] failed because of insufficient rights on object"), taskHandler, owner);
         MemFree(taskHandler);
         return RCC_ACCESS_DENIED;
      }
   }

   TCHAR *persistentData = request.getFieldAsString(VID_PARAMETER);
   TCHAR *comments = request.getFieldAsString(VID_COMMENTS);
   TCHAR *key = request.getFieldAsString(VID_TASK_KEY);
   uint32_t rcc;
   if (request.isFieldExist(VID_SCHEDULE))
   {
      TCHAR *schedule = request.getFieldAsString(VID_SCHEDULE);
      rcc = AddRecurrentScheduledTask(taskHandler, schedule, persistentData, nullptr, owner, objectId, systemAccessRights, comments, key);
      MemFree(schedule);
   }
   else
   {
      rcc = AddOneTimeScheduledTask(taskHandler, request.getFieldAsTime(VID_EXECUTION_TIME),
               persistentData, nullptr, owner, objectId, systemAccessRights, comments, key);
   }
   MemFree(taskHandler);
   MemFree(persistentData);
   MemFree(comments);
   MemFree(key);
   return rcc;
}

/**
 * Update scheduled task from message
 */
uint32_t UpdateScheduledTaskFromMsg(const NXCPMessage& request,  uint32_t owner, uint64_t systemAccessRights)
{
   uint64_t taskId = request.getFieldAsUInt64(VID_SCHEDULED_TASK_ID);
   wchar_t *taskHandler = request.getFieldAsString(VID_TASK_HANDLER);
   wchar_t *persistentData = request.getFieldAsString(VID_PARAMETER);
   wchar_t *comments = request.getFieldAsString(VID_COMMENTS);
   uint32_t objectId = request.getFieldAsInt32(VID_OBJECT_ID);
   bool disabled = request.getFieldAsBoolean(VID_TASK_IS_DISABLED);
   uint32_t rcc;
   if (request.isFieldExist(VID_SCHEDULE))
   {
      wchar_t *schedule = request.getFieldAsString(VID_SCHEDULE);
      rcc = UpdateRecurrentScheduledTask(taskId, taskHandler, schedule, persistentData, nullptr,
               comments, owner, objectId, systemAccessRights, disabled);
      MemFree(schedule);
   }
   else
   {
      rcc = UpdateOneTimeScheduledTask(taskId, taskHandler, request.getFieldAsTime(VID_EXECUTION_TIME),
               persistentData, nullptr, comments, owner, objectId, systemAccessRights, disabled);
   }
   MemFree(taskHandler);
   MemFree(persistentData);
   MemFree(comments);
   return rcc;
}

/**
 * Thread that checks one time schedules and executes them
 */
static void AdHocScheduler()
{
   ThreadSetName("Scheduler/A");
   uint32_t sleepTime = 1;
   uint32_t watchdogId = WatchdogAddThread(_T("Ad hoc scheduler"), 5);
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Ad hoc scheduler started"));
   while(true)
   {
      WatchdogStartSleep(watchdogId);
      s_wakeupCondition.wait(sleepTime * 1000);
      WatchdogNotify(watchdogId);

      if (g_flags & AF_SHUTDOWN)
         break;

      sleepTime = 3600;

      s_oneTimeTaskLock.lock();
      time_t now = time(nullptr);
      for(int i = 0; i < s_oneTimeTasks.size(); i++)
      {
         ScheduledTask *task = s_oneTimeTasks.get(i);
         if (task->isDisabled() || task->isRunning() || task->isCompleted())
            continue;

         // execute all tasks that is expected to execute now
         if (now >= task->getScheduledExecutionTime())
         {
            nxlog_debug_tag(DEBUG_TAG, 6, _T("AdHocScheduler: run scheduled task with id = %d, execution time = ") INT64_FMT,
                     task->getId(), static_cast<int64_t>(task->getScheduledExecutionTime()));

            SchedulerCallback *callback = s_callbacks.get(task->getTaskHandlerId());
            if (callback == nullptr)
            {
               nxlog_debug_tag(DEBUG_TAG, 3, _T("AdHocScheduler: task handler \"%s\" not registered"), task->getTaskHandlerId().cstr());
               callback = &s_missingTaskHandler;
            }

            task->startExecution(callback);
         }
         else
         {
            time_t diff = task->getScheduledExecutionTime() - now;
            if (diff < static_cast<time_t>(sleepTime))
               sleepTime = static_cast<uint32_t>(diff);
            break;
         }
      }
      s_oneTimeTaskLock.unlock();
      nxlog_debug_tag(DEBUG_TAG, 6, _T("AdHocScheduler: sleeping for %u seconds"), sleepTime);
   }
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Ad hoc scheduler stopped"));
}

/**
 * Wakes up for execution of one time schedule or for recalculation new wake up timestamp
 */
static void RecurrentScheduler()
{
   ThreadSetName("Scheduler/R");
   uint32_t watchdogId = WatchdogAddThread(_T("Recurrent scheduler"), 5);
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Recurrent scheduler started"));
   do
   {
      WatchdogNotify(watchdogId);
      time_t now = time(nullptr);
      struct tm currLocal;
#if HAVE_LOCALTIME_R
      localtime_r(&now, &currLocal);
#else
      memcpy(&currLocal, localtime(&now), sizeof(struct tm));
#endif

      s_recurrentTaskLock.lock();
      for(int i = 0; i < s_recurrentTasks.size(); i++)
      {
         ScheduledTask *task = s_recurrentTasks.get(i);
         if (task->isDisabled() || task->isRunning())
            continue;

         if (MatchSchedule(task->getSchedule(), nullptr, &currLocal, now))
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("RecurrentScheduler: starting scheduled task [%u] with handler \"%s\" (schedule \"%s\")"),
                     task->getId(), task->getTaskHandlerId().cstr(), task->getSchedule().cstr());

            SchedulerCallback *callback = s_callbacks.get(task->getTaskHandlerId());
            if (callback == nullptr)
            {
               nxlog_debug_tag(DEBUG_TAG, 3, _T("RecurrentScheduler: task handler \"%s\" not registered"), task->getTaskHandlerId().cstr());
               callback = &s_missingTaskHandler;
            }

            task->startExecution(callback);
         }
      }
      s_recurrentTaskLock.unlock();
      WatchdogStartSleep(watchdogId);
   } while(!SleepAndCheckForShutdown(60)); //sleep 1 minute
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Recurrent scheduler stopped"));
}

/**
 * Delete expired tasks
 */
static void DeleteExpiredTasks()
{
   time_t taskRetentionTime = ConfigReadULong(_T("Scheduler.TaskRetentionTime"), 86400);
   s_oneTimeTaskLock.lock();
   time_t now = time(nullptr);
   for(int i = 0; i < s_completedOneTimeTasks.size(); i++)
   {
      ScheduledTask *task = s_completedOneTimeTasks.get(i);
      if (task->isCompleted())
      {
         time_t deleteTime = std::max(now, task->getLastExecutionTime() + taskRetentionTime);
         if (deleteTime < now + 3600)
         {
            nxlog_debug_tag(DEBUG_TAG, 6, _T("DeleteExpiredTasks: scheduling delete for task [%u]"), task->getId());
            ThreadPoolScheduleAbsolute(g_schedulerThreadPool, deleteTime, DelayedTaskDelete, CAST_TO_POINTER(task->getId(), void*));
         }
      }
   }
   s_oneTimeTaskLock.unlock();

   ThreadPoolScheduleRelative(g_schedulerThreadPool, 3600000, DeleteExpiredTasks); // Run every hour
}

/**
 * Scheduler thread handles
 */
static THREAD s_oneTimeEventThread = INVALID_THREAD_HANDLE;
static THREAD s_cronSchedulerThread = INVALID_THREAD_HANDLE;

/**
 * Initialize task scheduler - read all schedules form database and start threads for one time and cron schedules
 */
void InitializeTaskScheduler()
{
   g_schedulerThreadPool = ThreadPoolCreate(_T("SCHEDULER"),
            ConfigReadInt(_T("ThreadPool.Scheduler.BaseSize"), 1),
            ConfigReadInt(_T("ThreadPool.Scheduler.MaxSize"), 64));

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   // Get first available scheduled_tasks id
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT max(id) FROM scheduled_tasks"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_taskId = DBGetFieldInt64(hResult, 0, 0);
      DBFreeResult(hResult);
   }

   hResult = DBSelect(hdb, _T("SELECT id,taskId,schedule,params,execution_time,last_execution_time,flags,owner,object_id,comments,task_key FROM scheduled_tasks"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         ScheduledTask *task = new ScheduledTask(hResult, i);
         if (task->getSchedule().isEmpty())
         {
            nxlog_debug_tag(DEBUG_TAG, 7, _T("InitializeTaskScheduler: added one time task [%u] at ") INT64_FMT,
                     task->getId(), static_cast<int64_t>(task->getScheduledExecutionTime()));
            if (task->isCompleted())
               s_completedOneTimeTasks.add(task);
            else
               s_oneTimeTasks.add(task);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 7, _T("InitializeTaskScheduler: added recurrent task %u at %s"),
                     task->getId(), task->getSchedule().cstr());
            s_recurrentTasks.add(task);
         }
      }
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);
   s_oneTimeTasks.sort(ScheduledTaskComparator);

   s_oneTimeEventThread = ThreadCreateEx(AdHocScheduler);
   s_cronSchedulerThread = ThreadCreateEx(RecurrentScheduler);

   ThreadPoolExecute(g_schedulerThreadPool, DeleteExpiredTasks);
}

/**
 * Stop all scheduler threads and free all memory
 */
void ShutdownTaskScheduler()
{
   if (g_schedulerThreadPool == nullptr)
      return;

   s_wakeupCondition.set();
   ThreadJoin(s_oneTimeEventThread);
   ThreadJoin(s_cronSchedulerThread);
   ThreadPoolDestroy(g_schedulerThreadPool);
}
