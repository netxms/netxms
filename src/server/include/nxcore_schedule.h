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
 * Scheduled task transient data - can be used to pass runtime data to scheduled
 * task as an alternative to reading necessary data from database.
 */
class ScheduledTaskTransientData
{
public:
   ScheduledTaskTransientData();
   virtual ~ScheduledTaskTransientData();
};

/**
 * Parameters for scheduled task
 */
class ScheduledTaskParameters
{
public:
   TCHAR *m_taskKey;
   UINT32 m_userId;
   UINT32 m_objectId;
   TCHAR *m_persistentData;
   ScheduledTaskTransientData *m_transientData;

   ScheduledTaskParameters(const TCHAR *taskKey, UINT32 userId, UINT32 objectId, const TCHAR *persistentData = NULL, ScheduledTaskTransientData *transientData = NULL)
   {
      m_taskKey = _tcsdup_ex(taskKey);
      m_userId = userId;
      m_objectId = objectId;
      m_persistentData = _tcsdup_ex(persistentData);
      m_transientData = transientData;
   }

   ScheduledTaskParameters(UINT32 userId, UINT32 objectId, const TCHAR *persistentData = NULL, ScheduledTaskTransientData *transientData = NULL)
   {
      m_taskKey = NULL;
      m_userId = userId;
      m_objectId = objectId;
      m_persistentData = _tcsdup_ex(persistentData);
      m_transientData = transientData;
   }

   ScheduledTaskParameters()
   {
      m_taskKey = NULL;
      m_userId = 0;
      m_objectId = 0;
      m_persistentData = NULL;
      m_transientData = NULL;
   }

   ~ScheduledTaskParameters()
   {
      MemFree(m_taskKey);
      MemFree(m_persistentData);
      delete m_transientData;
   }
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
   ScheduledTaskParameters *m_parameters;
   TCHAR *m_comments;
   time_t m_executionTime;
   time_t m_lastExecution;
   UINT32 m_flags;

public:
   ScheduledTask(int id, const TCHAR *taskHandlerId, const TCHAR *schedule,
            ScheduledTaskParameters *params, const TCHAR *comments, UINT32 flags = 0);
   ScheduledTask(int id, const TCHAR *taskHandlerId, time_t executionTime,
            ScheduledTaskParameters *params, const TCHAR *comments, UINT32 flags = 0);
   ScheduledTask(DB_RESULT hResult, int row);
   ~ScheduledTask();

   UINT32 getId() const { return m_id; }
   const TCHAR *getTaskHandlerId() const { return m_taskHandlerId; }
   const TCHAR *getTaskKey() const { return m_parameters->m_taskKey; }
   const TCHAR *getSchedule() const { return m_schedule; }
   const TCHAR *getPersistentData() const { return m_parameters->m_persistentData; }
   const ScheduledTaskTransientData *getTransientData() const { return m_parameters->m_transientData; }
   time_t getExecutionTime() const { return m_executionTime; }
   UINT32 getOwner() const { return m_parameters->m_userId; }
   UINT32 getObjectId() const { return m_parameters->m_objectId; }
   UINT32 getFlags() const { return m_flags; }

   void setLastExecutionTime(time_t time) { m_lastExecution = time; };
   void setExecutionTime(time_t time) { m_executionTime = time; }
   void setFlag(UINT32 flag) { m_flags |= flag; }
   void removeFlag(UINT32 flag) { m_flags &= ~flag; }

   void run(SchedulerCallback *callback);

   void update(const TCHAR *taskHandlerId, const TCHAR *schedule, ScheduledTaskParameters *params, const TCHAR *comments, UINT32 flags);
   void update(const TCHAR *taskHandlerId, time_t nextExecution, ScheduledTaskParameters *params, const TCHAR *comments, UINT32 flags);

   void saveToDatabase(bool newObject) const;
   void fillMessage(NXCPMessage *msg) const;
   void fillMessage(NXCPMessage *msg, UINT32 base) const;

   bool checkFlag(UINT32 flag) const { return (m_flags & flag) != 0; }
   bool isRunning() const { return (m_flags & SCHEDULED_TASK_RUNNING) != 0; }
   bool isCompleted() const { return (m_flags & SCHEDULED_TASK_COMPLETED) != 0; }
   bool isDisabled() const { return (m_flags & SCHEDULED_TASK_DISABLED) != 0; }
   bool isSystem() const { return (m_flags & SCHEDULED_TASK_SYSTEM) != 0; }
   bool canAccess(UINT32 userId, UINT64 systemAccess) const;
};

/**
 * Scheduler public API
 */
void InitializeTaskScheduler();
void ShutdownTaskScheduler();
void RegisterSchedulerTaskHandler(const TCHAR *id, scheduled_action_executor exec, UINT64 accessRight);
UINT32 NXCORE_EXPORTABLE AddRecurrentScheduledTask(const TCHAR *task, const TCHAR *schedule, const TCHAR *persistentData,
         ScheduledTaskTransientData *transientData, UINT32 owner, UINT32 objectId, UINT64 systemRights,
         const TCHAR *comments = NULL, UINT32 flags = 0, const TCHAR *key = NULL);
UINT32 NXCORE_EXPORTABLE AddOneTimeScheduledTask(const TCHAR *task, time_t nextExecutionTime, const TCHAR *persistentData,
         ScheduledTaskTransientData *transientData, UINT32 owner, UINT32 objectId, UINT64 systemRights,
         const TCHAR *comments = NULL, UINT32 flags = 0, const TCHAR *key = NULL);
UINT32 NXCORE_EXPORTABLE UpdateRecurrentScheduledTask(int id, const TCHAR *task, const TCHAR *schedule, const TCHAR *persistentData,
         ScheduledTaskTransientData *transientData, const TCHAR *comments, UINT32 owner, UINT32 objectId,
         UINT64 systemAccessRights, UINT32 flags, const TCHAR *key = NULL);
UINT32 NXCORE_EXPORTABLE UpdateOneTimeScheduledTask(int id, const TCHAR *task, time_t nextExecutionTime, const TCHAR *persistentData,
         ScheduledTaskTransientData *transientData, const TCHAR *comments, UINT32 owner, UINT32 objectId,
         UINT64 systemAccessRights, UINT32 flags, const TCHAR *key = NULL);
UINT32 NXCORE_EXPORTABLE DeleteScheduledTask(UINT32 id, UINT32 user, UINT64 systemRights);
bool NXCORE_EXPORTABLE DeleteScheduledTaskByHandlerId(const TCHAR *taskHandlerId);
bool NXCORE_EXPORTABLE DeleteScheduledTaskByKey(const TCHAR *taskKey);
ScheduledTask *FindScheduledTaskByHandlerId(const TCHAR *taskHandlerId);
void GetSchedulerTaskHandlers(NXCPMessage *msg, UINT64 accessRights);
void GetScheduledTasks(NXCPMessage *msg, UINT32 userId, UINT64 systemRights);
UINT32 UpdateScheduledTaskFromMsg(NXCPMessage *request, UINT32 owner, UINT64 systemAccessRights);
UINT32 CreateScehduledTaskFromMsg(NXCPMessage *request, UINT32 owner, UINT64 systemAccessRights);

#endif
