/*
** NetXMS - Network Management System
** Server Core
** Copyright (C) 2015-2025 Victor Kirhenshtein
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

#define TIMESTAMP_NEVER                   0

#define SCHEDULED_TASK_DISABLED           0x01
#define SCHEDULED_TASK_COMPLETED          0x02
#define SCHEDULED_TASK_RUNNING            0x04
#define SCHEDULED_TASK_SYSTEM             0x08

class ScheduledTask;
class ScheduledTaskParameters;
struct SchedulerCallback;

typedef void (*ScheduledTaskHandler)(const shared_ptr<ScheduledTaskParameters>& parameters);

/**
 * Scheduler callback
 */
struct SchedulerCallback
{
   ScheduledTaskHandler m_handler;
   uint64_t m_accessRight;

   SchedulerCallback(ScheduledTaskHandler handler, uint64_t accessRight)
   {
      m_handler = handler;
      m_accessRight = accessRight;
   }
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
   wchar_t *m_taskKey;
   uint32_t m_userId;
   uint32_t m_objectId;
   wchar_t *m_persistentData;
   wchar_t *m_comments;
   ScheduledTaskTransientData *m_transientData;

   ScheduledTaskParameters(const wchar_t *taskKey, uint32_t userId, uint32_t objectId, const wchar_t *persistentData = nullptr,
            ScheduledTaskTransientData *transientData = nullptr, const wchar_t *comments = nullptr)
   {
      m_taskKey = MemCopyStringW(taskKey);
      m_userId = userId;
      m_objectId = objectId;
      m_persistentData = MemCopyStringW(persistentData);
      m_transientData = transientData;
      m_comments = MemCopyStringW(comments);
   }

   ScheduledTaskParameters(uint32_t userId, uint32_t objectId, const wchar_t *persistentData = nullptr,
            ScheduledTaskTransientData *transientData = nullptr, const wchar_t *comments = nullptr)
   {
      m_taskKey = nullptr;
      m_userId = userId;
      m_objectId = objectId;
      m_persistentData = MemCopyStringW(persistentData);
      m_transientData = transientData;
      m_comments = MemCopyStringW(comments);
   }

   ScheduledTaskParameters()
   {
      m_taskKey = nullptr;
      m_userId = 0;
      m_objectId = 0;
      m_persistentData = nullptr;
      m_transientData = nullptr;
      m_comments = nullptr;
   }

   ~ScheduledTaskParameters()
   {
      MemFree(m_taskKey);
      MemFree(m_persistentData);
      MemFree(m_comments);
      delete m_transientData;
   }
};

/**
 * Scheduled task
 */
class ScheduledTask
{
private:
   uint64_t m_id;
   uint32_t m_flags;
   SharedString m_taskHandlerId;
   SharedString m_schedule;
   time_t m_scheduledExecutionTime;
   bool m_recurrent;
   time_t m_lastExecutionTime;
   shared_ptr<ScheduledTaskParameters> m_parameters;
   Mutex m_mutex;

   void lock() const { m_mutex.lock(); }
   void unlock() const { m_mutex.unlock(); }

   void run(SchedulerCallback *callback);

public:
   ScheduledTask(uint64_t id, const TCHAR *taskHandlerId, const TCHAR *schedule, shared_ptr<ScheduledTaskParameters> parameters, bool systemTask);
   ScheduledTask(uint64_t id, const TCHAR *taskHandlerId, time_t executionTime, shared_ptr<ScheduledTaskParameters> parameters, bool systemTask);
   ScheduledTask(DB_RESULT hResult, int row);

   uint64_t getId() const { return m_id; }
   SharedString getTaskHandlerId() const { return GetAttributeWithLock(m_taskHandlerId, m_mutex); }
   SharedString getSchedule() const { return GetAttributeWithLock(m_schedule, m_mutex); }
   String getTaskKey() const
   {
      lock();
      String key = m_parameters->m_taskKey;
      unlock();
      return key;
   }
   String getPersistentData() const
   {
      lock();
      String data = m_parameters->m_persistentData;
      unlock();
      return data;
   }
   time_t getScheduledExecutionTime() const { return m_scheduledExecutionTime; }
   time_t getLastExecutionTime() const { return m_lastExecutionTime; }
   uint32_t getOwner() const
   {
      lock();
      uint32_t userId = m_parameters->m_userId;
      unlock();
      return userId;
   }
   uint32_t getObjectId() const
   {
      lock();
      uint32_t objectId = m_parameters->m_objectId;
      unlock();
      return objectId;
   }

   void setSystem()
   {
      lock();
      m_flags |= SCHEDULED_TASK_SYSTEM;
      unlock();
   }

   void disable()
   {
      lock();
      m_flags |= SCHEDULED_TASK_DISABLED;
      unlock();
   }

   void startExecution(SchedulerCallback *callback);

   void update(const TCHAR *taskHandlerId, const TCHAR *schedule, shared_ptr<ScheduledTaskParameters> parameters, bool systemTask, bool disabled);
   void update(const TCHAR *taskHandlerId, time_t executionTime, shared_ptr<ScheduledTaskParameters> parameters, bool systemTask, bool disabled);

   void saveToDatabase(bool newObject) const;
   void fillMessage(NXCPMessage *msg) const;
   void fillMessage(NXCPMessage *msg, uint32_t base) const;

   bool canAccess(uint32_t userId, uint64_t systemAccess) const;

   bool checkFlag(uint32_t flag) const
   {
      lock();
      bool result = ((m_flags & flag) != 0);
      unlock();
      return result;
   }
   bool isRunning() const { return checkFlag(SCHEDULED_TASK_RUNNING); }
   bool isCompleted() const { return checkFlag(SCHEDULED_TASK_COMPLETED); }
   bool isDisabled() const { return checkFlag(SCHEDULED_TASK_DISABLED); }
   bool isSystem() const { return checkFlag(SCHEDULED_TASK_SYSTEM); }
};

/**
 * Scheduler public API
 */
void InitializeTaskScheduler();
void ShutdownTaskScheduler();
void NXCORE_EXPORTABLE RegisterSchedulerTaskHandler(const wchar_t *id, ScheduledTaskHandler executor, uint64_t accessRight);
uint32_t NXCORE_EXPORTABLE AddRecurrentScheduledTask(const TCHAR *taskHandlerId, const TCHAR *schedule, const TCHAR *persistentData,
         ScheduledTaskTransientData *transientData, uint32_t owner, uint32_t objectId, uint64_t systemRights,
         const TCHAR *comments = nullptr, const TCHAR *key = nullptr, bool systemTask = false);
uint32_t NXCORE_EXPORTABLE AddUniqueRecurrentScheduledTask(const TCHAR *taskHandlerId, const TCHAR *schedule, const TCHAR *persistentData,
         ScheduledTaskTransientData *transientData, uint32_t owner, uint32_t objectId, uint64_t systemRights,
         const TCHAR *comments = nullptr, const TCHAR *key = nullptr, bool systemTask = false);
uint32_t NXCORE_EXPORTABLE AddOneTimeScheduledTask(const TCHAR *taskHandlerId, time_t nextExecutionTime, const TCHAR *persistentData,
         ScheduledTaskTransientData *transientData, uint32_t owner, uint32_t objectId, uint64_t systemRights,
         const TCHAR *comments = nullptr, const TCHAR *key = nullptr, bool systemTask = false);
uint32_t NXCORE_EXPORTABLE UpdateRecurrentScheduledTask(uint64_t id, const TCHAR *taskHandlerId, const TCHAR *schedule, const TCHAR *persistentData,
         ScheduledTaskTransientData *transientData, const TCHAR *comments, uint32_t owner, uint32_t objectId,
         uint64_t systemAccessRights, bool disabled = false);
uint32_t NXCORE_EXPORTABLE UpdateOneTimeScheduledTask(uint64_t id, const TCHAR *taskHandlerId, time_t nextExecutionTime, const TCHAR *persistentData,
         ScheduledTaskTransientData *transientData, const TCHAR *comments, uint32_t owner, uint32_t objectId,
         uint64_t systemAccessRights, bool disabled = false);
uint32_t NXCORE_EXPORTABLE DeleteScheduledTask(uint64_t taskId, uint32_t userId, uint64_t systemRights);
bool NXCORE_EXPORTABLE DeleteScheduledTaskByHandlerId(const TCHAR *taskHandlerId);
int NXCORE_EXPORTABLE DeleteScheduledTasksByObjectId(uint32_t objectId, bool allTasks = false);
int NXCORE_EXPORTABLE DeleteScheduledTasksByKey(const TCHAR *taskKey);
int NXCORE_EXPORTABLE CountScheduledTasksByKey(const TCHAR *taskKey);
bool NXCORE_EXPORTABLE IsScheduledTaskRunning(uint64_t taskId);
ScheduledTask NXCORE_EXPORTABLE *FindScheduledTaskByHandlerId(const TCHAR *taskHandlerId);
void GetSchedulerTaskHandlers(NXCPMessage *msg, uint64_t accessRights);
void GetScheduledTasks(NXCPMessage *msg, uint32_t userId, uint64_t systemRights, bool (*filter)(const ScheduledTask *task, void *context) = nullptr, void *context = nullptr);
uint32_t UpdateScheduledTaskFromMsg(const NXCPMessage& request, uint32_t owner, uint64_t systemAccessRights);
uint32_t CreateScheduledTaskFromMsg(const NXCPMessage& request, uint32_t owner, uint64_t systemAccessRights);

#endif
