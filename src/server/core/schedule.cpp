/*
** NetXMS - Network Management System
** Copyright (C) 2003-2015 Raden Solutions
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
 * Scheduled task execution pool
 */
ThreadPool *g_schedulerThreadPool = NULL;

Schedule::Schedule(int id, const TCHAR *taskId, const TCHAR *schedule, const TCHAR *params, UINT32 owner, UINT32 objectId, UINT32 flags)
{
   m_id = id;
   m_taskId = _tcsdup(CHECK_NULL_EX(taskId));
   m_schedule = _tcsdup(CHECK_NULL_EX(schedule));
   m_params = _tcsdup(CHECK_NULL_EX(params));
   m_executionTime = NEVER;
   m_lastExecution = NEVER;
   m_flags = flags;
   m_owner = owner;
   m_objectId = objectId;
}

Schedule::Schedule(int id, const TCHAR *taskId, time_t executionTime, const TCHAR *params, UINT32 owner, UINT32 objectId, UINT32 flags)
{
   m_id = id;
   m_taskId = _tcsdup(CHECK_NULL_EX(taskId));
   m_schedule = _tcsdup(_T(""));
   m_params = _tcsdup(CHECK_NULL_EX(params));
   m_executionTime = executionTime;
   m_lastExecution = NEVER;
   m_flags = flags;
   m_owner = owner;
   m_objectId = objectId;
}

Schedule::Schedule(DB_RESULT hResult, int row)
{
   m_id = DBGetFieldULong(hResult, row, 0);
   m_taskId = DBGetField(hResult, row, 1, NULL, 0);
   m_schedule = DBGetField(hResult, row, 2, NULL, 0);
   m_params = DBGetField(hResult, row, 3, NULL, 0);
   m_executionTime = DBGetFieldULong(hResult, row, 4);
   m_lastExecution = DBGetFieldULong(hResult, row, 5);
   m_flags = DBGetFieldULong(hResult, row, 6);
   m_owner = DBGetFieldULong(hResult, row, 7);
   m_objectId = DBGetFieldULong(hResult, row, 8);
}

void Schedule::update(const TCHAR *taskId, const TCHAR *schedule, const TCHAR *params, UINT32 owner, UINT32 objectId, UINT32 flags)
{
   safe_free(m_taskId);
   m_taskId = _tcsdup(taskId);
   safe_free(m_schedule);
   m_schedule = _tcsdup(schedule);
   safe_free(m_params);
   m_params = _tcsdup(params);
   m_owner = owner;
   m_objectId = objectId;
   m_flags = flags;
}

void Schedule::update(const TCHAR *taskId, time_t nextExecution, const TCHAR *params, UINT32 owner, UINT32 objectId, UINT32 flags)
{
   safe_free(m_taskId);
   m_taskId = _tcsdup(taskId);
   safe_free(m_params);
   m_params = _tcsdup(params);
   m_executionTime = nextExecution;
   m_owner  = owner;
   m_objectId = objectId;
   m_flags = flags;
}

void Schedule::saveToDatabase(bool newObject)
{
   DB_HANDLE db = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt;

   if (newObject)
   {
		hStmt = DBPrepare(db,
                    _T("INSERT INTO scheduled_tasks (taskId,schedule,params,execution_time,")
                    _T("last_execution_time,flags,owner,object_id,id) VALUES (?,?,?,?,?,?,?,?,?)"));
   }
   else
   {
      hStmt = DBPrepare(db,
                    _T("UPDATE scheduled_tasks SET taskId=?,schedule=?,params=?,")
                    _T("execution_time=?,last_execution_time=?,flags=?,owner=?,object_id=? ")
                    _T("WHERE id=?"));
   }

	DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_taskId, DB_BIND_STATIC);
	DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_schedule, DB_BIND_STATIC);
	DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_params, DB_BIND_STATIC);
	DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (UINT32)m_executionTime);
	DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, (UINT32)m_lastExecution);
	DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, (LONG)m_flags);
	DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_owner);
	DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_objectId);
	DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, (LONG)m_id);

	if (hStmt == NULL)
		return;

   DBExecute(hStmt);
   DBFreeStatement(hStmt);
	DBConnectionPoolReleaseConnection(db);
	NotifyClientSessions(NX_NOTIFY_SCHEDULE_UPDATE,0);
}

void Schedule::run(ScheduleCallback *callback)
{
   bool oneTimeSchedule = !_tcscmp(m_schedule, _T(""));

   setFlag(SCHEDULE_IN_PROGRES);
	NotifyClientSessions(NX_NOTIFY_SCHEDULE_UPDATE,0);
   ScheduleParameters param(m_params, m_owner, m_objectId);
   callback->m_func(&param);
   setLastExecutionTime(time(NULL));

   if (oneTimeSchedule)
   {
      MutexLock(s_oneTimeScheduleLock);
   }

   removeFlag(SCHEDULE_IN_PROGRES);
   setFlag(SCHEDULE_EXECUTED);
   saveToDatabase(false);
   if (oneTimeSchedule)
   {
      s_oneTimeSchedules.sort(ScheduleListSortCallback);
      MutexUnlock(s_oneTimeScheduleLock);
   }

   if(checkFlag(SCHEDULE_INTERNAL))
      RemoveSchedule(m_id, 0, SYSTEM_ACCESS_FULL);
}

void Schedule::fillMessage(NXCPMessage *msg)
{
   msg->setField(VID_SCHEDULED_TASK_ID, m_id);
   msg->setField(VID_TASK_HANDLER, m_taskId);
   msg->setField(VID_SCHEDULE, m_schedule);
   msg->setField(VID_PARAMETER, m_params);
   msg->setFieldFromTime(VID_EXECUTION_TIME, m_executionTime);
   msg->setFieldFromTime(VID_LAST_EXECUTION_TIME, m_lastExecution);
   msg->setField(VID_FLAGS, (UINT32)m_flags);
   msg->setField(VID_OWNER, m_owner);
   msg->setField(VID_OBJECT_ID, m_objectId);
}

void Schedule::fillMessage(NXCPMessage *msg, UINT32 base)
{
   msg->setField(base, m_id);
   msg->setField(base + 1, m_taskId);
   msg->setField(base + 2, m_schedule);
   msg->setField(base + 3, m_params);
   msg->setFieldFromTime(base + 4, m_executionTime);
   msg->setFieldFromTime(base + 5, m_lastExecution);
   msg->setField(base + 6, m_flags);
   msg->setField(base + 7, m_owner);
   msg->setField(base + 8, m_objectId);
}

/**
 * Check if user can access this scheduled task
 */
bool Schedule::canAccess(UINT32 user, UINT64 systemAccess)
{
   if (systemAccess & SYSTEM_ACCESS_ALL_SCHEDULED_TASKS)
      return true;

   if(systemAccess & SYSTEM_ACCESS_USERS_SCHEDULED_TASKS)
      return !checkFlag(SCHEDULE_INTERNAL);

   if (systemAccess & SYSTEM_ACCESS_OWN_SCHEDULED_TASKS)
      return user == m_owner;

   return false;
}

/**
 * Callback for sorting reset list
 */
static int ScheduleListSortCallback(const void *e1, const void *e2)
{
   Schedule * s1 = *((Schedule**)e1);
   Schedule * s2 = *((Schedule**)e2);

   //Executed schedules schould go down
   if(s1->checkFlag(SCHEDULE_EXECUTED) != s2->checkFlag(SCHEDULE_EXECUTED))
   {
      if(s1->checkFlag(SCHEDULE_EXECUTED))
      {
         return 1;
      }
      else
      {
         return -1;
      }
   }

   //Schedules with execution time 0 should go down, others should be compared
   if (s1->getExecutionTime() == s2->getExecutionTime())
   {
      return 0;
   }

   if (((s1->getExecutionTime() < s2->getExecutionTime()) && (s1->getExecutionTime() != 0)) || (s2->getExecutionTime() == 0))
   {
      return -1;
   }
   else
   {
      return 1;
   }
}

/**
 * Initialize task scheduler - read all schedules form database and start threads for one time and crom schedules
 */
void InitializeTaskScheduler()
{
   g_schedulerThreadPool = ThreadPoolCreate(1, 64, _T("SCHEDULER"));
   //read from DB configuration
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT id,taskId,schedule,params,execution_time,last_execution_time,flags,owner,object_id FROM scheduled_tasks"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         Schedule *sh = new Schedule(hResult, i);
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
   s_oneTimeSchedules.sort(ScheduleListSortCallback);
   //start threads that will start cron and one time tasks threads
   ThreadCreate(OneTimeEventThread, 0, NULL);
   ThreadCreate(CronCheckThread, 0, NULL);
}

/**
 * Stop all scheduler threads and free all memory
 */
void CloseTaskScheduler()
{
   ConditionSet(s_cond);
   s_cond = INVALID_CONDITION_HANDLE;
   ConditionDestroy(s_cond);
   ThreadPoolDestroy(g_schedulerThreadPool);
   MutexDestroy(s_cronScheduleLock);
   MutexDestroy(s_oneTimeScheduleLock);
}

/**
 * Function that adds to list task handler function
 */
void RegisterSchedulerTaskHandler(const TCHAR *id, scheduled_action_executor exec, UINT64 accessRight)
{
   s_callbacks.set(id, new ScheduleCallback(exec, accessRight));
   DbgPrintf(6, _T("Registered scheduler task %s"), id);
}

/**
 * Scheduled task creation function
 */
UINT32 AddSchedule(const TCHAR *task, const TCHAR *schedule, const TCHAR *params, UINT32 owner, UINT32 objectId, UINT64 systemRights, UINT32 flags)
{
   if ((systemRights & (SYSTEM_ACCESS_ALL_SCHEDULED_TASKS | SYSTEM_ACCESS_USERS_SCHEDULED_TASKS | SYSTEM_ACCESS_OWN_SCHEDULED_TASKS)) == 0)
      return RCC_ACCESS_DENIED;
   DbgPrintf(7, _T("AddSchedule: Add cron schedule %s, %s, %s"), task, schedule, params);
   MutexLock(s_cronScheduleLock);
   Schedule *sh = new Schedule(CreateUniqueId(IDG_SCHEDULE), task, schedule, params, owner, objectId, flags);
   sh->saveToDatabase(true);
   s_cronSchedules.add(sh);
   MutexUnlock(s_cronScheduleLock);
   return RCC_SUCCESS;
}

/**
 * One time schedule creation function
 */
UINT32 AddOneTimeSchedule(const TCHAR *task, time_t nextExecutionTime, const TCHAR *params, UINT32 owner, UINT32 objectId, UINT64 systemRights, UINT32 flags)
{
   if ((systemRights & (SYSTEM_ACCESS_ALL_SCHEDULED_TASKS | SYSTEM_ACCESS_USERS_SCHEDULED_TASKS | SYSTEM_ACCESS_OWN_SCHEDULED_TASKS)) == 0)
      return RCC_ACCESS_DENIED;
   DbgPrintf(7, _T("AddOneTimeAction: Add one time schedule %s, %d, %s"), task, nextExecutionTime, params);
   MutexLock(s_oneTimeScheduleLock);
   Schedule *sh = new Schedule(CreateUniqueId(IDG_SCHEDULE), task, nextExecutionTime, params, owner, objectId, flags);
   sh->saveToDatabase(true);
   s_oneTimeSchedules.add(sh);
   s_oneTimeSchedules.sort(ScheduleListSortCallback);
   MutexUnlock(s_oneTimeScheduleLock);
   ConditionSet(s_cond);
   return RCC_SUCCESS;
}

/**
 * Scheduled actionUpdate
 */
UINT32 UpdateSchedule(int id, const TCHAR *task, const TCHAR *schedule, const TCHAR *params, UINT32 owner, UINT32 objectId, UINT64 systemAccessRights, UINT32 flags)
{
   DbgPrintf(7, _T("UpdateSchedule: update cron schedule %d, %s, %s, %s"), id, task, schedule, params);
   MutexLock(s_cronScheduleLock);
   UINT32 rcc = RCC_SUCCESS;
   for (int i = 0; i < s_cronSchedules.size(); i++)
   {
      if (s_cronSchedules.get(i)->getId() == id && s_oneTimeSchedules.get(i)->canAccess(owner, systemAccessRights))
      {
         if(!s_oneTimeSchedules.get(i)->canAccess(owner, systemAccessRights))
         {
            rcc = RCC_ACCESS_DENIED;
            break;
         }
         s_cronSchedules.get(i)->update(task, schedule, params, owner, objectId, flags);
         s_cronSchedules.get(i)->saveToDatabase(false);
         break;
      }
   }
   MutexUnlock(s_cronScheduleLock);
   return rcc;
}

/**
 * One time action update
 */
UINT32 UpdateOneTimeAction(int id, const TCHAR *task, time_t nextExecutionTime, const TCHAR *params, UINT32 owner, UINT32 objectId, UINT64 systemAccessRights, UINT32 flags)
{
   DbgPrintf(7, _T("UpdateOneTimeAction: update one time schedule %d, %s, %d, %s"), id, task, nextExecutionTime, params);
   bool found = true;
   MutexLock(s_oneTimeScheduleLock);
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
         s_oneTimeSchedules.get(i)->update(task, nextExecutionTime, params, owner, objectId, flags);
         s_oneTimeSchedules.get(i)->saveToDatabase(false);
         s_oneTimeSchedules.sort(ScheduleListSortCallback);
         found = true;
         break;
      }
   }
   MutexUnlock(s_oneTimeScheduleLock);

   if(found)
      ConditionSet(s_cond);
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
 * Removes schedule by id
 */
UINT32 RemoveSchedule(UINT32 id, UINT32 user, UINT64 systemRights)
{
   DbgPrintf(7, _T("RemoveSchedule: schedule(%d) removed"), id);
   bool found = false;
   UINT32 rcc = RCC_SUCCESS;

   MutexLock(s_cronScheduleLock);
   for (int i = 0; i < s_cronSchedules.size(); i++)
   {
      if (s_cronSchedules.get(i)->getId() == id)
      {
         if(!s_cronSchedules.get(i)->canAccess(user, systemRights))
         {
            rcc = RCC_ACCESS_DENIED;
            break;
         }
         s_cronSchedules.remove(i);
         found = true;
         break;
      }
   }
   MutexUnlock(s_cronScheduleLock);

   if(found)
   {
      DeleteScheduledTaskFromDB(id);
      return rcc;
   }

   MutexLock(s_oneTimeScheduleLock);
   for (int i = 0; i < s_oneTimeSchedules.size(); i++)
   {
      if (s_oneTimeSchedules.get(i)->getId() == id)
      {
         if(!s_cronSchedules.get(i)->canAccess(user, systemRights))
         {
            rcc = RCC_ACCESS_DENIED;
            break;
         }
         s_oneTimeSchedules.remove(i);
         s_oneTimeSchedules.sort(ScheduleListSortCallback);
         found = true;
         break;
      }
   }
   MutexUnlock(s_oneTimeScheduleLock);

   if (found)
   {
      ConditionSet(s_cond);
      DeleteScheduledTaskFromDB(id);
   }
   return rcc;
}

/**
 * Fills message with schedule list
 */
void GetSheduleList(NXCPMessage *msg, UINT32 userRights, UINT64 systemRights)
{
   int scheduleCount = 0;
   int base = VID_SCHEDULE_LIST_BASE;

   MutexLock(s_oneTimeScheduleLock);
   for(int i = 0; i < s_oneTimeSchedules.size(); i++)
   {
      if(s_oneTimeSchedules.get(i)->canAccess(userRights, systemRights))
      {
         s_oneTimeSchedules.get(i)->fillMessage(msg, base);
         scheduleCount++;
         base+=10;
      }
   }
   MutexUnlock(s_oneTimeScheduleLock);

   MutexLock(s_cronScheduleLock);
   for(int i = 0; i < s_cronSchedules.size(); i++)
   {
      if(s_oneTimeSchedules.get(i)->canAccess(userRights, systemRights))
      {
         s_cronSchedules.get(i)->fillMessage(msg, base);
         scheduleCount++;
         base+=10;
      }
   }
   MutexUnlock(s_cronScheduleLock);

   msg->setField(VID_SCHEDULE_COUNT, scheduleCount);
}

/**
 * Fills message with callback id list
 */
void GetCallbackIdList(NXCPMessage *msg, UINT64 accessRights)
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
 * Creates schedule from message
 */
UINT32 CreateScehduleFromMsg(NXCPMessage *request, UINT32 owner, UINT64 systemAccessRights)
{
   TCHAR *taskId = request->getFieldAsString(VID_TASK_HANDLER);
   TCHAR *schedule = NULL;
   time_t nextExecutionTime = 0;
   TCHAR *params = request->getFieldAsString(VID_PARAMETER);
   int flags = request->getFieldAsInt32(VID_FLAGS);
   int objectId = request->getFieldAsInt32(VID_OBJECT_ID);
   UINT32 result;
   if(request->isFieldExist(VID_SCHEDULE))
   {
      schedule = request->getFieldAsString(VID_SCHEDULE);
      result = AddSchedule(taskId, schedule, params, owner, objectId, systemAccessRights, flags);
   }
   else
   {
      nextExecutionTime = request->getFieldAsTime(VID_EXECUTION_TIME);
      result = AddOneTimeSchedule(taskId, nextExecutionTime, params, owner, objectId, systemAccessRights, flags);
   }
   safe_free(taskId);
   safe_free(schedule);
   safe_free(params);
   return result;
}

/**
 * Updage schedule from message
 */
UINT32 UpdateScheduleFromMsg(NXCPMessage *request,  UINT32 owner, UINT64 systemAccessRights)
{
   UINT32 id = request->getFieldAsInt32(VID_SCHEDULED_TASK_ID);
   TCHAR *taskId = request->getFieldAsString(VID_TASK_HANDLER);
   TCHAR *schedule = NULL;
   time_t nextExecutionTime = 0;
   TCHAR *params = request->getFieldAsString(VID_PARAMETER);
   UINT32 flags = request->getFieldAsInt32(VID_FLAGS);
   UINT32 objectId = request->getFieldAsInt32(VID_OBJECT_ID);
   UINT32 rcc;
   if(request->isFieldExist(VID_SCHEDULE))
   {
      schedule = request->getFieldAsString(VID_SCHEDULE);
      rcc = UpdateSchedule(id, taskId, schedule, params, owner, objectId, systemAccessRights, flags);
   }
   else
   {
      nextExecutionTime = request->getFieldAsTime(VID_EXECUTION_TIME);
      rcc = UpdateOneTimeAction(id, taskId, nextExecutionTime, params, owner, objectId, systemAccessRights, flags);
   }
   safe_free(taskId);
   safe_free(schedule);
   safe_free(params);
   return rcc;
}

/**
 * Thread that checks one time schedules and executes them
 */
static THREAD_RESULT THREAD_CALL OneTimeEventThread(void *arg)
{
   int sleepTime = 1;
   DbgPrintf(7, _T("OneTimeEventThread: started"));
   while(true)
   {
      if(!ConditionWait(s_cond, sleepTime) && (g_flags & AF_SHUTDOWN))
         break;

      //ConditionReset(s_cond);
      time_t now = time(NULL);
      struct tm currLocal;
      memcpy(&currLocal, localtime(&now), sizeof(struct tm));

      MutexLock(s_oneTimeScheduleLock);
      for(int i = 0; i < s_oneTimeSchedules.size(); i++)
      {
         Schedule *sh = s_oneTimeSchedules.get(i);
         if(sh->checkFlag(SCHEDULE_DISABLED))
            continue;

         if(sh->checkFlag(SCHEDULE_EXECUTED))
            break;

         ScheduleCallback *callback = s_callbacks.get(sh->getTaskId());
         if(callback == NULL)
         {
            DbgPrintf(3, _T("OneTimeEventThread: One time execution function with taskId=\'%s\' not found"), sh->getTaskId());
            continue;
         }

         if(sh->isInProgress())
            continue;

         //execute all timmers that is expected to execute now
         if(sh->getExecutionTime() != 0 && now >= sh->getExecutionTime())
         {
            DbgPrintf(7, _T("OneTimeEventThread: run schedule id=\'%d\', execution time =\'%d\'"), sh->getId(), sh->getExecutionTime());
            ThreadPoolExecute(g_schedulerThreadPool, sh, &Schedule::run, callback);
         }
         else
         {
            break;
         }
      }

      sleepTime = INFINITE;

      for(int i = 0; i < s_oneTimeSchedules.size(); i++)
      {
         Schedule *sh = s_oneTimeSchedules.get(i);
         if(sh->getExecutionTime() == NEVER)
            break;

         if(now >= sh->getExecutionTime())
            continue;

         sleepTime = (int)(sh->getExecutionTime() - now);
         sleepTime = sleepTime < 0 ? 0 : sleepTime * 1000;
         break;
      }

      DbgPrintf(7, _T("OneTimeEventThread: thread will sleep for %d"), sleepTime);
      MutexUnlock(s_oneTimeScheduleLock);
   }
   DbgPrintf(3, _T("OneTimeEventThread: stopped"));
   return THREAD_OK;
}

/**
 * Wakes up for execution of one time schedule or for recolculation new wake up timestamp
 */
static THREAD_RESULT THREAD_CALL CronCheckThread(void *arg)
{
   DbgPrintf(3, _T("CronCheckThread: started"));
   do {
      time_t now = time(NULL);
      struct tm currLocal;
      memcpy(&currLocal, localtime(&now), sizeof(struct tm));

      MutexLock(s_cronScheduleLock);
      for(int i = 0; i < s_cronSchedules.size(); i++)
      {
         Schedule *sh = s_cronSchedules.get(i);
         if(sh->checkFlag(SCHEDULE_DISABLED))
            continue;
         ScheduleCallback *callback = s_callbacks.get(sh->getTaskId());
         if(callback == NULL)
         {
            DbgPrintf(3, _T("CronCheckThread: Cron execution function with taskId=\'%s\' not found"), sh->getTaskId());
            continue;
         }
         if(IsItTime(&currLocal, sh->getSchedule(), now))
         {
            DbgPrintf(7, _T("OneTimeEventThread: run schedule id=\'%d\', schedule=\'%s\'"), sh->getId(), sh->getSchedule());
            ThreadPoolExecute(g_schedulerThreadPool, sh, &Schedule::run, callback);
         }
      }
      MutexUnlock(s_cronScheduleLock);
   } while(!SleepAndCheckForShutdown(60)); //sleep 1 minute
   DbgPrintf(3, _T("CronCheckThread: stopped"));
   return THREAD_OK;
}

/**
 * Checks if it is time to execute cron schedule
 */
static bool IsItTime(struct tm *currTime, const TCHAR *schedule, time_t currTimestamp)
{
   TCHAR value[256];

   // Minute
   const TCHAR *curr = ExtractWord(schedule, value);
   if (!MatchScheduleElement(value, currTime->tm_min, 59, NULL))
      return false;

   // Hour
   curr = ExtractWord(curr, value);
   if (!MatchScheduleElement(value, currTime->tm_hour, 23, NULL))
      return false;

   // Day of month
   curr = ExtractWord(curr, value);
   if (!MatchScheduleElement(value, currTime->tm_mday, GetLastMonthDay(currTime), NULL))
      return false;

   // Month
   curr = ExtractWord(curr, value);
   if (!MatchScheduleElement(value, currTime->tm_mon + 1, 12, NULL))
      return false;

   // Day of week
   curr = ExtractWord(curr, value);
   for(int i = 0; value[i] != 0; i++)
      if (value[i] == _T('7'))
         value[i] = _T('0');
   if (!MatchScheduleElement(value, currTime->tm_wday, 7, currTime))
      return false;

   return true;
}
