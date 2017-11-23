/*
** NetXMS - Network Management System
** Copyright (C) 2003-2016 Raden Solutions
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

/**
 * Static fields
 */
static StringObjectMap<SchedulerCallback> s_callbacks(true);
static ObjectArray<ScheduledTask> s_cronSchedules(5, 5, true);
static ObjectArray<ScheduledTask> s_oneTimeSchedules(5, 5, true);
static Condition s_wakeupCondition(false);
static Mutex s_cronScheduleLock;
static Mutex s_oneTimeScheduleLock;

/**
 * Scheduled task execution pool
 */
ThreadPool *g_schedulerThreadPool = NULL;

/**
 * Create recurrent task object
 */
ScheduledTask::ScheduledTask(int id, const TCHAR *taskId, const TCHAR *schedule, const TCHAR *params, const TCHAR *comments, UINT32 owner, UINT32 objectId, UINT32 flags)
{
   m_id = id;
   m_taskHandlerId = _tcsdup(CHECK_NULL_EX(taskId));
   m_schedule = _tcsdup(CHECK_NULL_EX(schedule));
   m_params = _tcsdup(CHECK_NULL_EX(params));
   m_comments = _tcsdup(CHECK_NULL_EX(comments));
   m_executionTime = NEVER;
   m_lastExecution = NEVER;
   m_flags = flags;
   m_owner = owner;
   m_objectId = objectId;
}

/**
 * Create one-time execution task object
 */
ScheduledTask::ScheduledTask(int id, const TCHAR *taskId, time_t executionTime, const TCHAR *params, const TCHAR *comments, UINT32 owner, UINT32 objectId, UINT32 flags)
{
   m_id = id;
   m_taskHandlerId = _tcsdup(CHECK_NULL_EX(taskId));
   m_schedule = _tcsdup(_T(""));
   m_params = _tcsdup(CHECK_NULL_EX(params));
   m_comments = _tcsdup(CHECK_NULL_EX(comments));
   m_executionTime = executionTime;
   m_lastExecution = NEVER;
   m_flags = flags;
   m_owner = owner;
   m_objectId = objectId;
}

/**
 * Create task object from database record
 */
ScheduledTask::ScheduledTask(DB_RESULT hResult, int row)
{
   m_id = DBGetFieldULong(hResult, row, 0);
   m_taskHandlerId = DBGetField(hResult, row, 1, NULL, 0);
   m_schedule = DBGetField(hResult, row, 2, NULL, 0);
   m_params = DBGetField(hResult, row, 3, NULL, 0);
   m_executionTime = DBGetFieldULong(hResult, row, 4);
   m_lastExecution = DBGetFieldULong(hResult, row, 5);
   m_flags = DBGetFieldULong(hResult, row, 6);
   m_owner = DBGetFieldULong(hResult, row, 7);
   m_objectId = DBGetFieldULong(hResult, row, 8);
   m_comments = DBGetField(hResult, row, 9, NULL, 0);
}

/**
 * Destructor
 */
ScheduledTask::~ScheduledTask()
{
   safe_free(m_taskHandlerId);
   safe_free(m_schedule);
   safe_free(m_params);
   safe_free(m_comments);
}

/**
 * Update task
 */
void ScheduledTask::update(const TCHAR *taskHandlerId, const TCHAR *schedule, const TCHAR *params, const TCHAR *comments, UINT32 owner, UINT32 objectId, UINT32 flags)
{
   safe_free(m_taskHandlerId);
   m_taskHandlerId = _tcsdup(CHECK_NULL_EX(taskHandlerId));
   safe_free(m_schedule);
   m_schedule = _tcsdup(CHECK_NULL_EX(schedule));
   safe_free(m_params);
   m_params = _tcsdup(CHECK_NULL_EX(params));
   safe_free(m_comments);
   m_comments = _tcsdup(CHECK_NULL_EX(comments));
   m_owner = owner;
   m_objectId = objectId;
   m_flags = flags;
}

/**
 * Update task
 */
void ScheduledTask::update(const TCHAR *taskHandlerId, time_t nextExecution, const TCHAR *params, const TCHAR *comments, UINT32 owner, UINT32 objectId, UINT32 flags)
{
   free(m_taskHandlerId);
   m_taskHandlerId = _tcsdup(CHECK_NULL_EX(taskHandlerId));
   free(m_schedule);
   m_schedule = _tcsdup(_T(""));
   free(m_params);
   m_params = _tcsdup(CHECK_NULL_EX(params));
   free(m_comments);
   m_comments = _tcsdup(CHECK_NULL_EX(comments));
   m_executionTime = nextExecution;
   m_owner  = owner;
   m_objectId = objectId;
   m_flags = flags;
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
                    _T("last_execution_time,flags,owner,object_id,comments,id) VALUES (?,?,?,?,?,?,?,?,?,?)"));
   }
   else
   {
      hStmt = DBPrepare(db,
                    _T("UPDATE scheduled_tasks SET taskId=?,schedule=?,params=?,")
                    _T("execution_time=?,last_execution_time=?,flags=?,owner=?,object_id=?,comments=? ")
                    _T("WHERE id=?"));
   }

   if (hStmt != NULL)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_taskHandlerId, DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_schedule, DB_BIND_STATIC);
      DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_params, DB_BIND_STATIC);
      DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (UINT32)m_executionTime);
      DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, (UINT32)m_lastExecution);
      DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, (LONG)m_flags);
      DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_owner);
      DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_objectId);
      DBBind(hStmt, 9, DB_SQLTYPE_VARCHAR, m_comments, DB_BIND_STATIC);
      DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, (LONG)m_id);

      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
	DBConnectionPoolReleaseConnection(db);
	NotifyClientSessions(NX_NOTIFY_SCHEDULE_UPDATE,0);
}

/**
 * Scheduled task comparator (used for task sorting)
 */
static int ScheduledTaskComparator(const ScheduledTask **e1, const ScheduledTask **e2)
{
   const ScheduledTask *s1 = *e1;
   const ScheduledTask *s2 = *e2;

   // Executed schedules should go down
   if (s1->checkFlag(SCHEDULED_TASK_COMPLETED) != s2->checkFlag(SCHEDULED_TASK_COMPLETED))
   {
      return s1->checkFlag(SCHEDULED_TASK_COMPLETED) ? 1 : -1;
   }

   // Schedules with execution time 0 should go down, others should be compared
   if (s1->getExecutionTime() == s2->getExecutionTime())
   {
      return 0;
   }

   return (((s1->getExecutionTime() < s2->getExecutionTime()) && (s1->getExecutionTime() != 0)) || (s2->getExecutionTime() == 0)) ? -1 : 1;
}

/**
 * Run scheduled task
 */
void ScheduledTask::run(SchedulerCallback *callback)
{
   bool oneTimeSchedule = !_tcscmp(m_schedule, _T(""));

   setFlag(SCHEDULED_TASK_RUNNING);
	NotifyClientSessions(NX_NOTIFY_SCHEDULE_UPDATE,0);
   ScheduledTaskParameters param(m_params, m_owner, m_objectId);
   callback->m_func(&param);
   setLastExecutionTime(time(NULL));

   if (oneTimeSchedule)
   {
      s_oneTimeScheduleLock.lock();
   }

   removeFlag(SCHEDULED_TASK_RUNNING);
   setFlag(SCHEDULED_TASK_COMPLETED);
   saveToDatabase(false);
   if (oneTimeSchedule)
   {
      s_oneTimeSchedules.sort(ScheduledTaskComparator);
      s_oneTimeScheduleLock.unlock();
   }

   if (oneTimeSchedule && checkFlag(SCHEDULED_TASK_SYSTEM))
      RemoveScheduledTask(m_id, 0, SYSTEM_ACCESS_FULL);
}

/**
 * Fill NXCP message with task data
 */
void ScheduledTask::fillMessage(NXCPMessage *msg) const
{
   msg->setField(VID_SCHEDULED_TASK_ID, m_id);
   msg->setField(VID_TASK_HANDLER, m_taskHandlerId);
   msg->setField(VID_SCHEDULE, m_schedule);
   msg->setField(VID_PARAMETER, m_params);
   msg->setFieldFromTime(VID_EXECUTION_TIME, m_executionTime);
   msg->setFieldFromTime(VID_LAST_EXECUTION_TIME, m_lastExecution);
   msg->setField(VID_FLAGS, (UINT32)m_flags);
   msg->setField(VID_OWNER, m_owner);
   msg->setField(VID_OBJECT_ID, m_objectId);
   msg->setField(VID_COMMENTS, m_comments);
}

/**
 * Fill NXCP message with task data
 */
void ScheduledTask::fillMessage(NXCPMessage *msg, UINT32 base) const
{
   msg->setField(base, m_id);
   msg->setField(base + 1, m_taskHandlerId);
   msg->setField(base + 2, m_schedule);
   msg->setField(base + 3, m_params);
   msg->setFieldFromTime(base + 4, m_executionTime);
   msg->setFieldFromTime(base + 5, m_lastExecution);
   msg->setField(base + 6, m_flags);
   msg->setField(base + 7, m_owner);
   msg->setField(base + 8, m_objectId);
   msg->setField(base + 9, m_comments);
}

/**
 * Check if user can access this scheduled task
 */
bool ScheduledTask::canAccess(UINT32 userId, UINT64 systemAccess) const
{
   if (systemAccess & SYSTEM_ACCESS_ALL_SCHEDULED_TASKS)
      return true;

   if(systemAccess & SYSTEM_ACCESS_USER_SCHEDULED_TASKS)
      return !checkFlag(SCHEDULED_TASK_SYSTEM);

   if (systemAccess & SYSTEM_ACCESS_OWN_SCHEDULED_TASKS)
      return userId == m_owner;

   return false;
}

/**
 * Function that adds to list task handler function
 */
void RegisterSchedulerTaskHandler(const TCHAR *id, scheduled_action_executor exec, UINT64 accessRight)
{
   s_callbacks.set(id, new SchedulerCallback(exec, accessRight));
   DbgPrintf(6, _T("Registered scheduler task %s"), id);
}

/**
 * Scheduled task creation function
 */
UINT32 AddScheduledTask(const TCHAR *task, const TCHAR *schedule, const TCHAR *params, UINT32 owner, UINT32 objectId, UINT64 systemRights, const TCHAR *comments, UINT32 flags)
{
   if ((systemRights & (SYSTEM_ACCESS_ALL_SCHEDULED_TASKS | SYSTEM_ACCESS_USER_SCHEDULED_TASKS | SYSTEM_ACCESS_OWN_SCHEDULED_TASKS)) == 0)
      return RCC_ACCESS_DENIED;
   DbgPrintf(7, _T("AddSchedule: Add cron schedule %s, %s, %s"), task, schedule, params);
   s_cronScheduleLock.lock();
   ScheduledTask *sh = new ScheduledTask(CreateUniqueId(IDG_SCHEDULED_TASK), task, schedule, params, comments, owner, objectId, flags);
   sh->saveToDatabase(true);
   s_cronSchedules.add(sh);
   s_cronScheduleLock.unlock();
   return RCC_SUCCESS;
}

/**
 * One time schedule creation function
 */
UINT32 AddOneTimeScheduledTask(const TCHAR *task, time_t nextExecutionTime, const TCHAR *params, UINT32 owner, UINT32 objectId, UINT64 systemRights, const TCHAR *comments, UINT32 flags)
{
   if ((systemRights & (SYSTEM_ACCESS_ALL_SCHEDULED_TASKS | SYSTEM_ACCESS_USER_SCHEDULED_TASKS | SYSTEM_ACCESS_OWN_SCHEDULED_TASKS)) == 0)
      return RCC_ACCESS_DENIED;
   DbgPrintf(7, _T("AddOneTimeAction: Add one time schedule %s, %d, %s"), task, nextExecutionTime, params);
   s_oneTimeScheduleLock.lock();
   ScheduledTask *sh = new ScheduledTask(CreateUniqueId(IDG_SCHEDULED_TASK), task, nextExecutionTime, params, comments, owner, objectId, flags);
   sh->saveToDatabase(true);
   s_oneTimeSchedules.add(sh);
   s_oneTimeSchedules.sort(ScheduledTaskComparator);
   s_oneTimeScheduleLock.unlock();
   s_wakeupCondition.set();
   return RCC_SUCCESS;
}

/**
 * Scheduled actionUpdate
 */
UINT32 UpdateScheduledTask(int id, const TCHAR *task, const TCHAR *schedule, const TCHAR *params, const TCHAR *comments, UINT32 owner, UINT32 objectId, UINT64 systemAccessRights, UINT32 flags)
{
   DbgPrintf(7, _T("UpdateSchedule: update cron schedule %d, %s, %s, %s"), id, task, schedule, params);
   s_cronScheduleLock.lock();
   UINT32 rcc = RCC_SUCCESS;
   bool found = false;
   for (int i = 0; i < s_cronSchedules.size(); i++)
   {
      if (s_cronSchedules.get(i)->getId() == id)
      {
         if (!s_cronSchedules.get(i)->canAccess(owner, systemAccessRights))
         {
            rcc = RCC_ACCESS_DENIED;
            break;
         }
         s_cronSchedules.get(i)->update(task, schedule, params, comments, owner, objectId, flags);
         s_cronSchedules.get(i)->saveToDatabase(false);
         found = true;
         break;
      }
   }
   s_cronScheduleLock.unlock();

   if (!found)
   {
      //check in different queue and if exists - remove from one and add to another
      ScheduledTask *st = NULL;
      s_oneTimeScheduleLock.lock();
      for(int i = 0; i < s_oneTimeSchedules.size(); i++)
      {
         if (s_oneTimeSchedules.get(i)->getId() == id)
         {
            if (!s_oneTimeSchedules.get(i)->canAccess(owner, systemAccessRights))
            {
               rcc = RCC_ACCESS_DENIED;
               break;
            }
            st = s_oneTimeSchedules.get(i);
            s_oneTimeSchedules.unlink(i);
            s_oneTimeSchedules.sort(ScheduledTaskComparator);
            st->update(task, schedule, params, comments, owner, objectId, flags);
            st->saveToDatabase(false);
            found = true;
            break;
         }
      }
      s_oneTimeScheduleLock.unlock();
      if(found && st != NULL)
      {
         s_cronScheduleLock.lock();
         s_cronSchedules.add(st);
         s_cronScheduleLock.unlock();
      }
   }

   return rcc;
}

/**
 * One time action update
 */
UINT32 UpdateOneTimeScheduledTask(int id, const TCHAR *task, time_t nextExecutionTime, const TCHAR *params, const TCHAR *comments, UINT32 owner, UINT32 objectId, UINT64 systemAccessRights, UINT32 flags)
{
   DbgPrintf(7, _T("UpdateOneTimeAction: update one time schedule %d, %s, %d, %s"), id, task, nextExecutionTime, params);
   bool found = false;
   s_oneTimeScheduleLock.lock();
   UINT32 rcc = RCC_SUCCESS;
   for (int i = 0; i < s_oneTimeSchedules.size(); i++)
   {
      if (s_oneTimeSchedules.get(i)->getId() == id)
      {
         if(!s_oneTimeSchedules.get(i)->canAccess(owner, systemAccessRights))
         {
            rcc = RCC_ACCESS_DENIED;
            break;
         }
         s_oneTimeSchedules.get(i)->update(task, nextExecutionTime, params, comments, owner, objectId, flags);
         s_oneTimeSchedules.get(i)->saveToDatabase(false);
         s_oneTimeSchedules.sort(ScheduledTaskComparator);
         found = true;
         break;
      }
   }
   s_oneTimeScheduleLock.unlock();

   if (!found && (rcc == RCC_SUCCESS))
   {
      //check in different queue and if exists - remove from one and add to another
      ScheduledTask *st = NULL;
      s_cronScheduleLock.lock();
      for (int i = 0; i < s_cronSchedules.size(); i++)
      {
         if (s_cronSchedules.get(i)->getId() == id)
         {
            if (!s_cronSchedules.get(i)->canAccess(owner, systemAccessRights))
            {
               rcc = RCC_ACCESS_DENIED;
               break;
            }
            st = s_cronSchedules.get(i);
            s_cronSchedules.unlink(i);
            st->update(task, nextExecutionTime, params, comments, owner, objectId, flags);
            st->saveToDatabase(false);
            found = true;
            break;
         }
      }
      s_cronScheduleLock.unlock();

      if (found && st != NULL)
      {
         s_oneTimeScheduleLock.lock();
         s_oneTimeSchedules.add(st);
         s_oneTimeSchedules.sort(ScheduledTaskComparator);
         s_oneTimeScheduleLock.unlock();
      }
   }

   if (found)
      s_wakeupCondition.set();
   return rcc;
}

/**
 * Removes scheduled task from database by id
 */
static void DeleteScheduledTaskFromDB(UINT32 id)
{
   DB_HANDLE db = DBConnectionPoolAcquireConnection();
   TCHAR query[256];
   _sntprintf(query, 256, _T("DELETE FROM scheduled_tasks WHERE id = %d"), id);
   DBQuery(db, query);
	DBConnectionPoolReleaseConnection(db);
	NotifyClientSessions(NX_NOTIFY_SCHEDULE_UPDATE,0);
}

/**
 * Removes scheduled task by id
 */
UINT32 RemoveScheduledTask(UINT32 id, UINT32 user, UINT64 systemRights)
{
   DbgPrintf(7, _T("RemoveSchedule: schedule(%d) removed"), id);
   UINT32 rcc = RCC_INVALID_OBJECT_ID;

   s_cronScheduleLock.lock();
   for(int i = 0; i < s_cronSchedules.size(); i++)
   {
      if (s_cronSchedules.get(i)->getId() == id)
      {
         if (!s_cronSchedules.get(i)->canAccess(user, systemRights))
         {
            rcc = RCC_ACCESS_DENIED;
            break;
         }
         s_cronSchedules.remove(i);
         rcc = RCC_SUCCESS;
         break;
      }
   }
   s_cronScheduleLock.unlock();

   s_oneTimeScheduleLock.lock();
   for(int i = 0; i < s_oneTimeSchedules.size() && rcc == RCC_INVALID_OBJECT_ID; i++)
   {
      if (s_oneTimeSchedules.get(i)->getId() == id)
      {
         if (!s_cronSchedules.get(i)->canAccess(user, systemRights))
         {
            rcc = RCC_ACCESS_DENIED;
            break;
         }
         s_oneTimeSchedules.remove(i);
         s_oneTimeSchedules.sort(ScheduledTaskComparator);
         s_wakeupCondition.set();
         rcc = RCC_SUCCESS;
         break;
      }
   }
   s_oneTimeScheduleLock.unlock();

   if (rcc == RCC_SUCCESS)
   {
      DeleteScheduledTaskFromDB(id);
   }

   return rcc;
}

/**
 * Find scheduled task by task handler id
 */
ScheduledTask *FindScheduledTaskByHandlerId(const TCHAR *taskHandlerId)
{
   ScheduledTask *task;
   bool found = false;

   s_cronScheduleLock.lock();
   for (int i = 0; i < s_cronSchedules.size(); i++)
   {
      if (_tcscmp(s_cronSchedules.get(i)->getTaskHandlerId(), taskHandlerId) == 0)
      {
         task = s_cronSchedules.get(i);
         found = true;
         break;
      }
   }
   s_cronScheduleLock.unlock();

   if (found)
      return task;

   s_oneTimeScheduleLock.lock();
   for (int i = 0; i < s_oneTimeSchedules.size(); i++)
   {
      if (_tcscmp(s_oneTimeSchedules.get(i)->getTaskHandlerId(), taskHandlerId) == 0)
      {
         task = s_oneTimeSchedules.get(i);
         found = true;
         break;
      }
   }
   s_oneTimeScheduleLock.unlock();

   if (found)
      return task;

   return NULL;
}

/**
 * Remove scheduled task by task handler id
 */
bool RemoveScheduledTaskByHandlerId(const TCHAR *taskHandlerId)
{
   bool removed = false;

   s_cronScheduleLock.lock();
   for (int i = 0; i < s_cronSchedules.size(); i++)
   {
      if (_tcscmp(s_cronSchedules.get(i)->getTaskHandlerId(), taskHandlerId) == 0)
      {
         s_cronSchedules.remove(i);
         removed = true;
         break;
      }
   }
   s_cronScheduleLock.unlock();

   if (removed)
      return true;

   s_oneTimeScheduleLock.lock();
   for (int i = 0; i < s_oneTimeSchedules.size(); i++)
   {
      if (_tcscmp(s_oneTimeSchedules.get(i)->getTaskHandlerId(), taskHandlerId) == 0)
      {
         s_oneTimeSchedules.remove(i);
         s_oneTimeSchedules.sort(ScheduledTaskComparator);
         removed = true;
         break;
      }
   }
   s_oneTimeScheduleLock.unlock();

   if (removed)
      return true;

   return false;

}

/**
 * Fills message with scheduled tasks list
 */
void GetScheduledTasks(NXCPMessage *msg, UINT32 userId, UINT64 systemRights)
{
   int scheduleCount = 0;
   int base = VID_SCHEDULE_LIST_BASE;

   s_oneTimeScheduleLock.lock();
   for(int i = 0; i < s_oneTimeSchedules.size(); i++)
   {
      if (s_oneTimeSchedules.get(i)->canAccess(userId, systemRights))
      {
         s_oneTimeSchedules.get(i)->fillMessage(msg, base);
         scheduleCount++;
         base += 10;
      }
   }
   s_oneTimeScheduleLock.unlock();

   s_cronScheduleLock.lock();
   for(int i = 0; i < s_cronSchedules.size(); i++)
   {
      if (s_cronSchedules.get(i)->canAccess(userId, systemRights))
      {
         s_cronSchedules.get(i)->fillMessage(msg, base);
         scheduleCount++;
         base += 10;
      }
   }
   s_cronScheduleLock.unlock();

   msg->setField(VID_SCHEDULE_COUNT, scheduleCount);
}

/**
 * Fills message with task handlers list
 */
void GetSchedulerTaskHandlers(NXCPMessage *msg, UINT64 accessRights)
{
   UINT32 base = VID_CALLBACK_BASE;
   int count = 0;

   StringList *keyList = s_callbacks.keys();
   for(int i = 0; i < keyList->size(); i++)
   {
      if(accessRights & s_callbacks.get(keyList->get(i))->m_accessRight)
      {
         msg->setField(base, keyList->get(i));
         count++;
         base++;
      }
   }
   delete keyList;
   msg->setField(VID_CALLBACK_COUNT, (UINT32)count);
}

/**
 * Creates scheduled task from message
 */
UINT32 CreateScehduledTaskFromMsg(NXCPMessage *request, UINT32 owner, UINT64 systemAccessRights)
{
   TCHAR *taskId = request->getFieldAsString(VID_TASK_HANDLER);
   TCHAR *schedule = NULL;
   time_t nextExecutionTime = 0;
   TCHAR *params = request->getFieldAsString(VID_PARAMETER);
   TCHAR *comments = request->getFieldAsString(VID_COMMENTS);
   int flags = request->getFieldAsInt32(VID_FLAGS);
   int objectId = request->getFieldAsInt32(VID_OBJECT_ID);
   UINT32 result;
   if (request->isFieldExist(VID_SCHEDULE))
   {
      schedule = request->getFieldAsString(VID_SCHEDULE);
      result = AddScheduledTask(taskId, schedule, params, owner, objectId, systemAccessRights, comments, flags);
   }
   else
   {
      nextExecutionTime = request->getFieldAsTime(VID_EXECUTION_TIME);
      result = AddOneTimeScheduledTask(taskId, nextExecutionTime, params, owner, objectId, systemAccessRights, comments, flags);
   }
   free(taskId);
   free(schedule);
   free(params);
   return result;
}

/**
 * Update scheduled task from message
 */
UINT32 UpdateScheduledTaskFromMsg(NXCPMessage *request,  UINT32 owner, UINT64 systemAccessRights)
{
   UINT32 id = request->getFieldAsInt32(VID_SCHEDULED_TASK_ID);
   TCHAR *taskId = request->getFieldAsString(VID_TASK_HANDLER);
   TCHAR *schedule = NULL;
   time_t nextExecutionTime = 0;
   TCHAR *params = request->getFieldAsString(VID_PARAMETER);
   TCHAR *comments = request->getFieldAsString(VID_COMMENTS);
   UINT32 flags = request->getFieldAsInt32(VID_FLAGS);
   UINT32 objectId = request->getFieldAsInt32(VID_OBJECT_ID);
   UINT32 rcc;
   if(request->isFieldExist(VID_SCHEDULE))
   {
      schedule = request->getFieldAsString(VID_SCHEDULE);
      rcc = UpdateScheduledTask(id, taskId, schedule, params, comments, owner, objectId, systemAccessRights, flags);
   }
   else
   {
      nextExecutionTime = request->getFieldAsTime(VID_EXECUTION_TIME);
      rcc = UpdateOneTimeScheduledTask(id, taskId, nextExecutionTime, params, comments, owner, objectId, systemAccessRights, flags);
   }
   safe_free(taskId);
   safe_free(schedule);
   safe_free(params);
   return rcc;
}

/**
 * Thread that checks one time schedules and executes them
 */
static THREAD_RESULT THREAD_CALL AdHocScheduler(void *arg)
{
   ThreadSetName("Scheduler/A");
   UINT32 sleepTime = 1;
   UINT32 watchdogId = WatchdogAddThread(_T("Ad hoc scheduler"), 5);
   nxlog_debug(3, _T("Ad hoc scheduler started"));
   while(true)
   {
      WatchdogStartSleep(watchdogId);
      s_wakeupCondition.wait(sleepTime * 1000);
      WatchdogNotify(watchdogId);

      if (g_flags & AF_SHUTDOWN)
         break;

      s_oneTimeScheduleLock.lock();
      time_t now = time(NULL);
      for(int i = 0; i < s_oneTimeSchedules.size(); i++)
      {
         ScheduledTask *sh = s_oneTimeSchedules.get(i);
         if (sh->isDisabled() || sh->isRunning() || sh->isCompleted())
            continue;

         SchedulerCallback *callback = s_callbacks.get(sh->getTaskHandlerId());
         if (callback == NULL)
         {
            DbgPrintf(3, _T("AdHocScheduler: One time execution function with taskId=\'%s\' not found"), sh->getTaskHandlerId());
            continue;
         }

         // execute all tasks that is expected to execute now
         if ((sh->getExecutionTime() != 0) && (now >= sh->getExecutionTime()))
         {
            nxlog_debug(6, _T("AdHocScheduler: run scheduled task with id = %d, execution time = %d"), sh->getId(), sh->getExecutionTime());
            ThreadPoolExecute(g_schedulerThreadPool, sh, &ScheduledTask::run, callback);
         }
         else
         {
            break;
         }
      }

      sleepTime = 3600;

      for(int i = 0; i < s_oneTimeSchedules.size(); i++)
      {
         ScheduledTask *sh = s_oneTimeSchedules.get(i);
         if (sh->getExecutionTime() == NEVER)
            break;

         if (now < sh->getExecutionTime())
         {
            time_t diff = sh->getExecutionTime() - now;
            if (diff < (time_t)3600)
               sleepTime = (UINT32)diff;
            break;
         }
      }

      s_oneTimeScheduleLock.unlock();
      nxlog_debug(6, _T("AdHocScheduler: sleeping for %d seconds"), sleepTime);
   }
   nxlog_debug(3, _T("Ad hoc scheduler stopped"));
   return THREAD_OK;
}

/**
 * Wakes up for execution of one time schedule or for recalculation new wake up timestamp
 */
static THREAD_RESULT THREAD_CALL RecurrentScheduler(void *arg)
{
   ThreadSetName("Scheduler/R");
   UINT32 watchdogId = WatchdogAddThread(_T("Recurrent scheduler"), 5);
   nxlog_debug(3, _T("Recurrent scheduler started"));
   do
   {
      WatchdogNotify(watchdogId);
      time_t now = time(NULL);
      struct tm currLocal;
#if HAVE_LOCALTIME_R
      localtime_r(&now, &currLocal);
#else
      memcpy(&currLocal, localtime(&now), sizeof(struct tm));
#endif

      s_cronScheduleLock.lock();
      for(int i = 0; i < s_cronSchedules.size(); i++)
      {
         ScheduledTask *sh = s_cronSchedules.get(i);
         if (sh->checkFlag(SCHEDULED_TASK_DISABLED))
            continue;
         SchedulerCallback *callback = s_callbacks.get(sh->getTaskHandlerId());
         if (callback == NULL)
         {
            DbgPrintf(3, _T("RecurrentScheduler: Cron execution function with taskId=\'%s\' not found"), sh->getTaskHandlerId());
            continue;
         }
         if (MatchSchedule(sh->getSchedule(), &currLocal, now))
         {
            DbgPrintf(7, _T("RecurrentScheduler: run schedule id=\'%d\', schedule=\'%s\'"), sh->getId(), sh->getSchedule());
            ThreadPoolExecute(g_schedulerThreadPool, sh, &ScheduledTask::run, callback);
         }
      }
      s_cronScheduleLock.unlock();
      WatchdogStartSleep(watchdogId);
   } while(!SleepAndCheckForShutdown(60)); //sleep 1 minute
   nxlog_debug(3, _T("Recurrent scheduler stopped"));
   return THREAD_OK;
}

/**
 * Scheduler thread handles
 */
static THREAD s_oneTimeEventThread = INVALID_THREAD_HANDLE;
static THREAD s_cronSchedulerThread = INVALID_THREAD_HANDLE;

/**
 * Initialize task scheduler - read all schedules form database and start threads for one time and crom schedules
 */
void InitializeTaskScheduler()
{
   g_schedulerThreadPool = ThreadPoolCreate(1, 64, _T("SCHEDULER"));
   //read from DB configuration
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT id,taskId,schedule,params,execution_time,last_execution_time,flags,owner,object_id,comments FROM scheduled_tasks"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         ScheduledTask *sh = new ScheduledTask(hResult, i);
         if(!_tcscmp(sh->getSchedule(), _T("")))
         {
            DbgPrintf(7, _T("InitializeTaskScheduler: Add one time schedule %d, %d"), sh->getId(), sh->getExecutionTime());
            s_oneTimeSchedules.add(sh);
         }
         else
         {
            DbgPrintf(7, _T("InitializeTaskScheduler: Add cron schedule %d, %s"), sh->getId(), sh->getSchedule());
            s_cronSchedules.add(sh);
         }
      }
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);
   s_oneTimeSchedules.sort(ScheduledTaskComparator);

   s_oneTimeEventThread = ThreadCreateEx(AdHocScheduler, 0, NULL);
   s_cronSchedulerThread = ThreadCreateEx(RecurrentScheduler, 0, NULL);
}

/**
 * Stop all scheduler threads and free all memory
 */
void CloseTaskScheduler()
{
   s_wakeupCondition.set();
   ThreadJoin(s_oneTimeEventThread);
   ThreadJoin(s_cronSchedulerThread);
   ThreadPoolDestroy(g_schedulerThreadPool);
}
