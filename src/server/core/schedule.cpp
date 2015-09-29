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

#define NEVER 0

#define SCHEDULE_DISABLED           1
#define SCHEDULE_EXECUTED           2
#define SCHEDULE_IN_PROGRES         4
#define SCHEDULE_INTERNAL           5
//#define SCHEDULE_

class Schedule;
class ScheduleCallback;

/**
 * Scheduled task execution pool
 */
static ThreadPool *s_taskSchedullPool = NULL;
/**
 * Static fields
 */
static StringObjectMap<ScheduleCallback> s_callbacks(true);
static ObjectArray<Schedule> s_cronSchedules(5, 5, true);
static ObjectArray<Schedule> s_oneTimeSchedules(5, 5, true);
static CONDITION s_cond = ConditionCreate(false);

/**
 * Static functions
 */
void InitializeTaskScheduler();
void CloseTaskScheduler();
static THREAD_RESULT THREAD_CALL OneTimeEventThread(void *arg);
static THREAD_RESULT THREAD_CALL CronCheckThread(void *arg);
static bool IsItTime(struct tm *currTime, const TCHAR *schedule, time_t currTimestamp);
static int ScheduleListSortCallback(const void *e1, const void *e2);

/**
 * Mutex for shedule structures
 */
static MUTEX s_cronScheduleLock = MutexCreate();
static MUTEX s_oneTimeScheduleLock = MutexCreate();

class ScheduleCallback
{
public:
   scheduled_action_executor m_func;
   ScheduleCallback(scheduled_action_executor func) { m_func = func; }
};

class Schedule
{
private:
   UINT32 m_id;
   TCHAR *m_taskId;
   TCHAR *m_schedule;
   TCHAR *m_params;
   time_t m_executionTime;
   time_t m_lastExecution;
   int m_flags;

public:

   Schedule(int id, const TCHAR *taskId, const TCHAR *schedule, const TCHAR *params, int flags = 0);
   Schedule(int id, const TCHAR *taskId, time_t executionTime, const TCHAR *params, int flags = 0);
   Schedule(DB_RESULT hResult, int row);

   ~Schedule(){ delete m_taskId; delete m_schedule; delete m_params; }

   UINT32 getId() { return m_id; }
   const TCHAR *getTaskId() { return m_taskId; }
   const TCHAR *getSchedule() { return m_schedule; }
   const TCHAR *getParams() { return m_params; }
   time_t getExecutionTime() { return m_executionTime; }

   void setLastExecutionTime(time_t time) { m_lastExecution = time; };
   void setExecutionTime(time_t time) { m_executionTime = time; }
   void setFlag(int flag) { m_flags |= flag; }
   void removeFlag(int flag) { m_flags &= ~flag; }

   void update(const TCHAR *taskId, const TCHAR *schedule, const TCHAR *params);
   void update(const TCHAR *taskId, time_t nextExecution, const TCHAR *params);

   void saveToDatabase(bool newObject);
   void run(ScheduleCallback *callback);
   void fillMessage(NXCPMessage *msg);
   void fillMessage(NXCPMessage *msg, UINT32 base);

   bool checkFlag(int flag) { return (m_flags & flag) > 0 ? true : false; }
   bool isInProgress() { return (m_flags & SCHEDULE_IN_PROGRES) > 0 ? true : false; }
};

Schedule::Schedule(int id, const TCHAR *taskId, const TCHAR *schedule, const TCHAR *params, int flags)
{
   m_id = id;
   m_taskId = _tcsdup(CHECK_NULL_EX(taskId));
   m_schedule = _tcsdup(CHECK_NULL_EX(schedule));
   m_params = _tcsdup(CHECK_NULL_EX(params));
   m_executionTime = NEVER;
   m_lastExecution = NEVER;
   m_flags = flags;
}

Schedule::Schedule(int id, const TCHAR *taskId, time_t executionTime, const TCHAR *params, int flags)
{
   m_id = id;
   m_taskId = _tcsdup(CHECK_NULL_EX(taskId));
   m_schedule = _tcsdup(_T(""));
   m_params = _tcsdup(CHECK_NULL_EX(params));
   m_executionTime = executionTime;
   m_lastExecution = NEVER;
   m_flags = flags;
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
}

void Schedule::update(const TCHAR *taskId, const TCHAR *schedule, const TCHAR *params)
{
   safe_free(m_taskId);
   m_taskId = _tcsdup(taskId);
   safe_free(m_schedule);
   m_schedule = _tcsdup(schedule);
   safe_free(m_params);
   m_params = _tcsdup(params);
}

void Schedule::update(const TCHAR *taskId, time_t nextExecution, const TCHAR *params)
{
   safe_free(m_taskId);
   m_taskId = _tcsdup(taskId);
   safe_free(m_params);
   m_params = _tcsdup(params);
   m_executionTime = nextExecution;
}

void Schedule::saveToDatabase(bool newObject)
{
   DB_HANDLE db = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt;

   if (newObject)
   {
		hStmt = DBPrepare(db,
                    _T("INSERT INTO schedule (taskId,shedule,params,execution_time,")
                    _T("last_execution_time,flags, id) VALUES (?,?,?,?,?,?,?)"));
   }
   else
   {
      hStmt = DBPrepare(db,
                    _T("UPDATE schedule SET taskId=?,shedule=?,params=?,")
                    _T("execution_time=?,last_execution_time=?,flags=? ")
                    _T("WHERE id=?"));
   }

	DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_taskId, DB_BIND_STATIC);
	DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_schedule, DB_BIND_STATIC);
	DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_params, DB_BIND_STATIC);
	DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (UINT32)m_executionTime);
	DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, (UINT32)m_lastExecution);
	DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, (LONG)m_flags);
	DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, (LONG)m_id);

	if (hStmt == NULL)
		return;

   DBExecute(hStmt);
   DBFreeStatement(hStmt);
	DBConnectionPoolReleaseConnection(db);
}

void Schedule::run(ScheduleCallback *callback)
{
   bool oneTimeShedule = !_tcscmp(m_schedule, _T(""));

   setFlag(SCHEDULE_IN_PROGRES);
   callback->m_func(m_params);
   setLastExecutionTime(time(NULL));

   if(oneTimeShedule)
   {
      MutexLock(s_oneTimeScheduleLock);
      setExecutionTime(NEVER);
   }

   removeFlag(SCHEDULE_IN_PROGRES);
   setFlag(SCHEDULE_EXECUTED);
   saveToDatabase(false);
   if(oneTimeShedule)
   {
      s_oneTimeSchedules.sort(ScheduleListSortCallback);
      MutexUnlock(s_oneTimeScheduleLock);
   }
}

void Schedule::fillMessage(NXCPMessage *msg)
{
   msg->setField(VID_SCHEDULE_ID, m_id);
   msg->setField(VID_TASK_ID, m_taskId);
   msg->setField(VID_SCHEDULE, m_schedule);
   msg->setField(VID_PARAMETER, m_params);
   msg->setFieldFromTime(VID_EXECUTION_TIME, m_executionTime);
   msg->setFieldFromTime(VID_LAST_EXECUTION_TIME, m_lastExecution);
   msg->setField(VID_FLAGS, (UINT32)m_flags);
}

void Schedule::fillMessage(NXCPMessage *msg, UINT32 base)
{
   msg->setField(base, m_id);
   msg->setField(base+1, m_taskId);
   msg->setField(base+2, m_schedule);
   msg->setField(base+3, m_params);
   msg->setFieldFromTime(base+4, m_executionTime);
   msg->setFieldFromTime(base+5, m_lastExecution);
   msg->setField(base+6, (UINT32)m_flags);
}

/**
 * Callback for sorting reset list
 */
static int ScheduleListSortCallback(const void *e1, const void *e2)
{
   Schedule * s1 = *((Schedule**)e1);
   Schedule * s2 = *((Schedule**)e2);

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
 * Initialize task sheduler - read all shedules form database and start threads for one time and crom shedules
 */
void InitializeTaskScheduler()
{
   s_taskSchedullPool = ThreadPoolCreate(1, 64, _T("TASKSCHEDULL"));
   //read from DB configuration
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT id,taskId,shedule,params,execution_time,last_execution_time,flags FROM schedule"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         Schedule *sh = new Schedule(hResult, i);
         if(!_tcscmp(sh->getSchedule(), _T("")))
         {
            DbgPrintf(7, _T("InitializeTaskScheduler: Add one time shedule %d, %d"), sh->getId(), sh->getExecutionTime());
            s_oneTimeSchedules.add(sh);
         }
         else
         {
            DbgPrintf(7, _T("InitializeTaskScheduler: Add cron shedule %d, %s"), sh->getId(), sh->getSchedule());
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
 * Stop all sheduler threads and free all memory
 */
void CloseTaskScheduler()
{
   ConditionSet(s_cond);
   s_cond = INVALID_CONDITION_HANDLE;
   ConditionDestroy(s_cond);
   ThreadPoolDestroy(s_taskSchedullPool);
   MutexDestroy(s_cronScheduleLock);
   MutexDestroy(s_oneTimeScheduleLock);
}

/**
 * Function that adds to list task handler function
 */
void AddSchedulleTaskHandler(const TCHAR *id, scheduled_action_executor exec)
{
   s_callbacks.set(id, new ScheduleCallback(exec));
   DbgPrintf(6, _T("AddSchedulleTaskHandler: Add shedule callback %s"), id);
}

/**
 * Scheduled task creation function
 */
void AddSchedule(const TCHAR *task, const TCHAR *schedule, const TCHAR *params, int flags)
{
   DbgPrintf(7, _T("AddSchedule: Add cron shedule %s, %s, %s"), task, schedule, params);
   MutexLock(s_cronScheduleLock);
   Schedule *sh = new Schedule(CreateUniqueId(IDG_SCHEDULE), task, schedule, params, flags);
   sh->saveToDatabase(true);
   s_cronSchedules.add(sh);
   MutexUnlock(s_cronScheduleLock);
}

/**
 * One time action creation function
 */
void AddOneTimeAction(const TCHAR *task, time_t nextExecutionTime, const TCHAR *params, int flags)
{
   DbgPrintf(7, _T("AddOneTimeAction: Add one time shedule %s, %d, %s"), task, nextExecutionTime, params);
   MutexLock(s_oneTimeScheduleLock);
   Schedule *sh = new Schedule(CreateUniqueId(IDG_SCHEDULE), task, nextExecutionTime, params, flags);
   sh->saveToDatabase(true);
   s_oneTimeSchedules.add(sh);
   s_oneTimeSchedules.sort(ScheduleListSortCallback);
   MutexUnlock(s_oneTimeScheduleLock);
   ConditionSet(s_cond);
}

/**
 * Scheduled actionUpdate
 */
void UpdateSchedule(int id, const TCHAR *task, const TCHAR *schedule, const TCHAR *params, int flags)
{
   DbgPrintf(7, _T("UpdateSchedule: update cron shedule %d, %s, %s, %s"), id, task, schedule, params);
   MutexLock(s_cronScheduleLock);
   for (int i = 0; i < s_cronSchedules.size(); i++)
   {
      if (s_cronSchedules.get(i)->getId() == id)
      {
         s_cronSchedules.get(i)->update(task, schedule, params);
         s_cronSchedules.get(i)->setFlag(flags);
         s_cronSchedules.get(i)->saveToDatabase(false);
         break;
      }
   }
   MutexUnlock(s_cronScheduleLock);

}

/**
 * One time action update
 */
void UpdateOneTimeAction(int id, const TCHAR *task, time_t nextExecutionTime, const TCHAR *params, int flags)
{
   DbgPrintf(7, _T("UpdateOneTimeAction: update one time shedule %d, %s, %d, %s"), id, task, nextExecutionTime, params);
   bool found = true;
   MutexLock(s_oneTimeScheduleLock);
   for (int i = 0; i < s_oneTimeSchedules.size(); i++)
   {
      if (s_oneTimeSchedules.get(i)->getId() == id)
      {
         s_oneTimeSchedules.get(i)->update(task, nextExecutionTime, params);
         s_cronSchedules.get(i)->setFlag(flags);
         s_oneTimeSchedules.get(i)->saveToDatabase(false);
         s_oneTimeSchedules.sort(ScheduleListSortCallback);
         found = true;
         break;
      }
   }
   MutexUnlock(s_oneTimeScheduleLock);

   if(found)
      ConditionSet(s_cond);
}

/**
 * Removes shedule form Database by id
 */
void DeleteFromDB(UINT32 id)
{
   DB_HANDLE db = DBConnectionPoolAcquireConnection();
   TCHAR query[256];
   _sntprintf(query, 256, _T("DELETE FROM schedule WHERE id = %s"), id);
   DBQuery(db, query);
   DB_STATEMENT hStmt;
	DBConnectionPoolReleaseConnection(db);
}

/**
 * Removes shedule by id
 */
void RemoveSchedule(UINT32 id, bool alreadyLocked)
{
   DbgPrintf(7, _T("RemoveSchedule: shedule(%d) removed"), id);
   bool found = false;

   MutexLock(s_cronScheduleLock);
   for (int i = 0; i < s_cronSchedules.size(); i++)
   {
      if (s_cronSchedules.get(i)->getId() == id)
      {
         s_cronSchedules.remove(i);
         found = true;
         break;
      }
   }
   MutexUnlock(s_cronScheduleLock);

   if(found)
   {
      DeleteFromDB(id);
      return;
   }

   MutexLock(s_oneTimeScheduleLock);
   for (int i = 0; i < s_oneTimeSchedules.size(); i++)
   {
      if (s_oneTimeSchedules.get(i)->getId() == id)
      {
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
      DeleteFromDB(id);
   }
}

/**
 * Fills message with shedule list
 */
void GetSheduleList(NXCPMessage *msg)
{
   int sheduleCount = 0;
   int base = VID_SCHEDULE_LIST_BASE;

   MutexLock(s_oneTimeScheduleLock);
   for(int i = 0; i < s_oneTimeSchedules.size(); i++, base+=10)
   {
      s_oneTimeSchedules.get(i)->fillMessage(msg, base);
   }
   sheduleCount += s_oneTimeSchedules.size();
   MutexUnlock(s_oneTimeScheduleLock);

   MutexLock(s_cronScheduleLock);
   for(int i = 0; i < s_cronSchedules.size(); i++, base+=10)
   {
      s_cronSchedules.get(i)->fillMessage(msg, base);
   }
   sheduleCount += s_cronSchedules.size();
   MutexUnlock(s_cronScheduleLock);

   msg->setField(VID_SCHEDULE_COUNT, sheduleCount);
}

/**
 * Fills message with callback id list
 */
void GetCallbackIdList(NXCPMessage *msg)
{
   msg->setField(VID_CALLBACK_COUNT, s_callbacks.size());
   int base = VID_CALLBACK_BASE;

   StringList *keyList = s_callbacks.keys();
   for(int i = 0; i < keyList->size(); i++, base++)
      msg->setField(base, keyList->get(i));
}

/**
 * Creates schedule from message
 */
void CreateScehduleFromMsg(NXCPMessage *request)
{
   TCHAR *taskId = request->getFieldAsString(VID_TASK_ID);
   TCHAR *schedule = NULL;
   time_t nextExecutionTime = 0;
   TCHAR *params = request->getFieldAsString(VID_PARAMETER);
   int flags = request->getFieldAsInt32(VID_FLAGS);
   if(request->isFieldExist(VID_SCHEDULE))
   {
      schedule = request->getFieldAsString(VID_SCHEDULE);
      AddSchedule(taskId, schedule, params, flags);
   }
   else
   {
      nextExecutionTime = request->getFieldAsTime(VID_EXECUTION_TIME);
      AddOneTimeAction(taskId, nextExecutionTime, params, flags);
   }
   safe_free(taskId);
   safe_free(schedule);
   safe_free(params);
}

/**
 * Updage schedule from message
 */
void UpdateScheduleFromMsg(NXCPMessage *request)
{
   UINT32 id = request->getFieldAsInt32(VID_SCHEDULE_ID);
   TCHAR *taskId = request->getFieldAsString(VID_TASK_ID);
   TCHAR *schedule = NULL;
   time_t nextExecutionTime = 0;
   TCHAR *params = request->getFieldAsString(VID_PARAMETER);
   int flags = request->getFieldAsInt32(VID_FLAGS);
   if(request->isFieldExist(VID_SCHEDULE))
   {
      schedule = request->getFieldAsString(VID_SCHEDULE);
      UpdateSchedule(id, taskId, schedule, params, flags);
   }
   else
   {
      nextExecutionTime = request->getFieldAsTime(VID_EXECUTION_TIME);
      UpdateOneTimeAction(id, taskId, nextExecutionTime, params, flags);
   }
   safe_free(taskId);
   safe_free(schedule);
   safe_free(params);
}

/**
 * Thread that checks one time shedules and executes them
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
            DbgPrintf(7, _T("OneTimeEventThread: run shedule id=\'%d\', execution time =\'%d\'"), sh->getId(), sh->getExecutionTime());
            ThreadPoolExecute(s_taskSchedullPool, sh, &Schedule::run, callback);
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

         sleepTime = sh->getExecutionTime() - now;
         sleepTime = sleepTime < 0 ? 0 : sleepTime * 1000;
         break;
      }

      DbgPrintf(7, _T("OneTimeEventThread: thread will sleep for %d"), sleepTime);
      MutexUnlock(s_oneTimeScheduleLock);
   }
   DbgPrintf(3, _T("OneTimeEventThread: stopped"));
}

/**
 * Wakes up for execution of one time shedule or for recolculation new wake up timestamp
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
         ScheduleCallback *callback = s_callbacks.get(sh->getTaskId());
         if(callback == NULL)
         {
            DbgPrintf(3, _T("CronCheckThread: Cron execution function with taskId=\'%s\' not found"), sh->getTaskId());
            continue;
         }
         if(IsItTime(&currLocal, sh->getSchedule(), now))
         {
            DbgPrintf(7, _T("OneTimeEventThread: run shedule id=\'%d\', shedule=\'%s\'"), sh->getId(), sh->getSchedule());
            ThreadPoolExecute(s_taskSchedullPool, sh, &Schedule::run, callback);
         }
      }
      MutexUnlock(s_cronScheduleLock);
   } while(!SleepAndCheckForShutdown(60)); //sleep 1 minute
   DbgPrintf(3, _T("CronCheckThread: stopped"));
}

/**
 * Checks if it is time to execute cron shedule
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
