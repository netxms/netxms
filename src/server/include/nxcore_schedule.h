/*
** NetXMS - Network Management System
** Server Core
** Copyright (C) 2015 Victor Kirhenshtein
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
** File: nxcore_schedule.h
**
**/

#ifndef _nxcore_schedule_h_
#define _nxcore_schedule_h_

#define NEVER 0

#define SCHEDULE_DISABLED           1
#define SCHEDULE_EXECUTED           2
#define SCHEDULE_IN_PROGRES         4
#define SCHEDULE_INTERNAL           5

class Schedule;
class ScheduleCallback;

typedef void (*scheduled_action_executor)(const TCHAR *params);

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
 * Mutex for schedule structures
 */
static MUTEX s_cronScheduleLock = MutexCreate();
static MUTEX s_oneTimeScheduleLock = MutexCreate();

class ScheduleCallback
{
public:
   scheduled_action_executor m_func;
   UINT64 m_accessRight;
   ScheduleCallback(scheduled_action_executor func, UINT64 accessRight) { m_func = func; m_accessRight = accessRight; }
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
   UINT32 m_flags;
   UINT32 m_owner;

public:

   Schedule(int id, const TCHAR *taskId, const TCHAR *schedule, const TCHAR *params, UINT32 owner, UINT32 flags = 0);
   Schedule(int id, const TCHAR *taskId, time_t executionTime, const TCHAR *params, UINT32 owner, UINT32 flags = 0);
   Schedule(DB_RESULT hResult, int row);

   ~Schedule(){ delete m_taskId; delete m_schedule; delete m_params; }

   UINT32 getId() { return m_id; }
   const TCHAR *getTaskId() { return m_taskId; }
   const TCHAR *getSchedule() { return m_schedule; }
   const TCHAR *getParams() { return m_params; }
   time_t getExecutionTime() { return m_executionTime; }
   UINT32 getOwner() { return m_owner; }

   void setLastExecutionTime(time_t time) { m_lastExecution = time; };
   void setExecutionTime(time_t time) { m_executionTime = time; }
   void setFlag(int flag) { m_flags |= flag; }
   void removeFlag(int flag) { m_flags &= ~flag; }

   void update(const TCHAR *taskId, const TCHAR *schedule, const TCHAR *params, UINT32 owner);
   void update(const TCHAR *taskId, time_t nextExecution, const TCHAR *params, UINT32 owner);

   void saveToDatabase(bool newObject);
   void run(ScheduleCallback *callback);
   void fillMessage(NXCPMessage *msg);
   void fillMessage(NXCPMessage *msg, UINT32 base);

   bool checkFlag(UINT32 flag) { return (m_flags & flag) > 0 ? true : false; }
   bool isInProgress() { return (m_flags & SCHEDULE_IN_PROGRES) > 0 ? true : false; }
   bool canAccess(UINT32 user, UINT64 systemAccess);
};

void RegisterSchedulerTaskHandler(const TCHAR *id, scheduled_action_executor exec, UINT64 accessRight);
UINT32 AddSchedule(const TCHAR *task, const TCHAR *schedule, const TCHAR *params, UINT32 owner, UINT64 systemRights, int flags = 0);
UINT32 AddOneTimeSchedule(const TCHAR *task, time_t nextExecutionTime, const TCHAR *params, UINT32 owner, UINT64 systemRights, int flags = 0);
UINT32 UpdateSchedule(int id, const TCHAR *task, const TCHAR *schedule, const TCHAR *params, UINT32 owner, UINT64 systemAccessRights, int flags);
UINT32 UpdateOneTimeAction(int id, const TCHAR *task, time_t nextExecutionTime, const TCHAR *params, UINT32 owner, UINT64 systemAccessRights, int flags);
UINT32 RemoveSchedule(UINT32 id, UINT32 user, UINT64 systemRights);
void GetCallbackIdList(NXCPMessage *msg, UINT64 accessRights);
void GetSheduleList(NXCPMessage *msg, UINT32 user, UINT64 systemRights);
UINT32 UpdateScheduleFromMsg(NXCPMessage *request, UINT32 owner, UINT64 systemAccessRights);
UINT32 CreateScehduleFromMsg(NXCPMessage *request, UINT32 owner, UINT64 systemAccessRights);

#endif
