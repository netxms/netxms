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

#define SCHEDULED_TASK_DISABLED           1
#define SCHEDULED_TASK_COMPLETED          2
#define SCHEDULED_TASK_RUNNING            4
#define SCHEDULED_TASK_SYSTEM             8

class ScheduledTask;
class SchedulerCallback;
class ScheduledTaskParameters;

typedef void (*scheduled_action_executor)(const ScheduledTaskParameters *params);

/**
 * Scheduler callback
 */
class SchedulerCallback
{
public:
   scheduled_action_executor m_func;
   UINT64 m_accessRight;

   SchedulerCallback(scheduled_action_executor func, UINT64 accessRight) { m_func = func; m_accessRight = accessRight; }
};

/**
 * Parameters for scheduled task
 */
class ScheduledTaskParameters
{
public:
   TCHAR *m_params;
   UINT32 m_userId;
   UINT32 m_objectId;

   ScheduledTaskParameters(const TCHAR *param, UINT32 userId, UINT32 objectId) { m_params = _tcsdup(param); m_userId = userId; m_objectId = objectId; }
   ScheduledTaskParameters() { m_params = NULL; }
   ~ScheduledTaskParameters() { safe_free(m_params); }
};

/**
 * Scheduled task
 */
class ScheduledTask
{
private:
   UINT32 m_id;
   TCHAR *m_taskHandlerId;
   TCHAR *m_schedule;
   TCHAR *m_params;
   TCHAR *m_comments;
   time_t m_executionTime;
   time_t m_lastExecution;
   UINT32 m_flags;
   UINT32 m_owner;
   UINT32 m_objectId;

public:
   ScheduledTask(int id, const TCHAR *taskHandlerId, const TCHAR *schedule, const TCHAR *params, const TCHAR *comments, UINT32 owner, UINT32 objectId, UINT32 flags = 0);
   ScheduledTask(int id, const TCHAR *taskHandlerId, time_t executionTime, const TCHAR *params, const TCHAR *comments, UINT32 owner, UINT32 objectId, UINT32 flags = 0);
   ScheduledTask(DB_RESULT hResult, int row);
   ~ScheduledTask();

   UINT32 getId() const { return m_id; }
   const TCHAR *getTaskHandlerId() const { return m_taskHandlerId; }
   const TCHAR *getSchedule() const { return m_schedule; }
   const TCHAR *getParams() const { return m_params; }
   time_t getExecutionTime() const { return m_executionTime; }
   UINT32 getOwner() const { return m_owner; }
   UINT32 getObjectId() const { return m_objectId; }
   UINT32 getFlags() const { return m_flags; }

   void setLastExecutionTime(time_t time) { m_lastExecution = time; };
   void setExecutionTime(time_t time) { m_executionTime = time; }
   void setFlag(UINT32 flag) { m_flags |= flag; }
   void removeFlag(UINT32 flag) { m_flags &= ~flag; }

   void run(SchedulerCallback *callback);

   void update(const TCHAR *taskHandlerId, const TCHAR *schedule, const TCHAR *params, const TCHAR *comments, UINT32 owner, UINT32 objectId, UINT32 flags);
   void update(const TCHAR *taskHandlerId, time_t nextExecution, const TCHAR *params, const TCHAR *comments, UINT32 owner, UINT32 objectId, UINT32 flags);

   void saveToDatabase(bool newObject) const;
   void fillMessage(NXCPMessage *msg) const;
   void fillMessage(NXCPMessage *msg, UINT32 base) const;

   bool checkFlag(UINT32 flag) const { return (m_flags & flag) != 0; }
   bool isRunning() const { return (m_flags & SCHEDULED_TASK_RUNNING) != 0; }
   bool isCompleted() const { return (m_flags & SCHEDULED_TASK_COMPLETED) != 0; }
   bool isDisabled() const { return (m_flags & SCHEDULED_TASK_DISABLED) != 0; }
   bool canAccess(UINT32 userId, UINT64 systemAccess) const;
};

/**
 * Scheduler public API
 */
void InitializeTaskScheduler();
void CloseTaskScheduler();
void RegisterSchedulerTaskHandler(const TCHAR *id, scheduled_action_executor exec, UINT64 accessRight);
UINT32 AddScheduledTask(const TCHAR *task, const TCHAR *schedule, const TCHAR *params, UINT32 owner, UINT32 objectId, UINT64 systemRights, const TCHAR *comments = _T(""), UINT32 flags = 0);
UINT32 AddOneTimeScheduledTask(const TCHAR *task, time_t nextExecutionTime, const TCHAR *params, UINT32 owner, UINT32 objectId, UINT64 systemRights, const TCHAR *comments = _T(""), UINT32 flags = 0);
UINT32 UpdateScheduledTask(int id, const TCHAR *task, const TCHAR *schedule, const TCHAR *params, const TCHAR *comments, UINT32 owner, UINT32 objectId, UINT64 systemAccessRights, UINT32 flags);
UINT32 UpdateOneTimeScheduledTask(int id, const TCHAR *task, time_t nextExecutionTime, const TCHAR *params, const TCHAR *comments, UINT32 owner, UINT32 objectId, UINT64 systemAccessRights, UINT32 flags);
UINT32 RemoveScheduledTask(UINT32 id, UINT32 user, UINT64 systemRights);
bool RemoveScheduledTaskByHandlerId(const TCHAR *taskHandlerId);
ScheduledTask *FindScheduledTaskByHandlerId(const TCHAR *taskHandlerId);
void GetSchedulerTaskHandlers(NXCPMessage *msg, UINT64 accessRights);
void GetScheduledTasks(NXCPMessage *msg, UINT32 userId, UINT64 systemRights);
UINT32 UpdateScheduledTaskFromMsg(NXCPMessage *request, UINT32 owner, UINT64 systemAccessRights);
UINT32 CreateScehduledTaskFromMsg(NXCPMessage *request, UINT32 owner, UINT64 systemAccessRights);

#endif
