/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
** File: aitask.cpp
**
**/

#include "nxcore.h"
#include <iris.h>

#define DEBUG_TAG L"llm.aitask"

char *ProcessRequestToAIAssistantEx(const char *prompt, const char *systemPrompt, NetObj *context, json_t *eventData, GenericClientSession *session, int maxIterations);

/**
 * AI Task management
 */
static SharedHashMap<uint32_t, AITask> s_aiTasks;
static Mutex s_aiTasksLock;
static VolatileCounter s_taskId = 0; // Last used task ID
static VolatileCounter64 s_logRecordId = 0;
static std::string s_systemPrompt;

/**
 * Initialize AI tasks
 */
void InitAITasks()
{
   s_taskId = ConfigReadInt(_T("AITask.LastTaskId"), 0);
   s_logRecordId = ConfigReadInt64(_T("AITask.LastLogRecordId"), 0);

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_RESULT hResult = DBSelect(hdb, _T("SELECT max(record_id) FROM ai_task_execution_log"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_logRecordId = std::max(DBGetFieldInt64(hResult, 0, 0), static_cast<int64_t>(s_logRecordId));
      DBFreeResult(hResult);
   }

   // Load tasks from database
   hResult = DBSelect(hdb, _T("SELECT id,description,prompt,memento,last_execution_time,next_execution_time,iteration,user_id FROM ai_tasks ORDER BY id"));
   if (hResult != nullptr)
   {
      uint32_t maxId = 0;
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         shared_ptr<AITask> task = make_shared<AITask>(hResult, i);
         s_aiTasks.set(task->getId(), task);
         if (task->getId() > maxId)
            maxId = task->getId();
      }
      DBFreeResult(hResult);

      if (maxId > s_taskId)
         s_taskId = maxId;
   }

   DBConnectionPoolReleaseConnection(hdb);

   s_systemPrompt =
      "You are an AI agent integrated with NetXMS, a network management system. "
      "You are capable of performing background tasks that may require multiple executions to complete. "
      "You have access to various functions that allow you to retrieve data from the NetXMS system and perform actions as needed. "
      "You are operating autonomously without user interaction. "
      "Never stop to ask for user input or confirmation. "
      "Perform requested task according to instructions below after <instructions> tag. "
      "Task could be long running and may require multiple executions to complete. "
      "Use available functions to retrieve necessary data from NetXMS system or perform actions. "
      "Provide structured execution report in JSON format, use only clean JSON without any additional text. "
      "Do not use markdown formatting or code blocks. "
      "If task is long or inifinite, respond with completed=false and provide next_execution_time for rescheduling. "
      "This is background task, so do not ask for any user input. "
      "If different instructions are needed for next execution, provide them in the instructions field in response. "
      "If data need to be preserved between executions, use memento field for that purpose. "
      "Memento content will be passed back to you during next execution inside <memento> tag. "
      "Memento can also be used to provide instructions or context for next execution. "
      "<iteration> tag indicates current execution iteration starting from 1. "
      "Answer should have the following fields: completed, next_execution_time, instructions, memento, and explanation. "
      "'completed' attribute should be set to true if no further action is required, or false if task needs to be rescheduled. "
      "'next_execution_time' should be in number if seconds and indicate delay until next task execution if it is not completed. "
      "'instructions' should contain updated instructions for next execution (will replace current instructions). "
      "'memento' should contain any data that needs to be preserved between task executions. "
      "'explanation' should provide a brief reasoning behind your decision. ";
   ENUMERATE_MODULES(pfGetAIAgentInstructions)
   {
      s_systemPrompt.append(CURRENT_MODULE.pfGetAIAgentInstructions());
   }

   nxlog_debug_tag(DEBUG_TAG, 2, L"Last AI task ID=%u, last AI task log record ID=" INT64_FMT, s_taskId, s_logRecordId);
}

/**
 * Register AI task
 */
uint32_t NXCORE_EXPORTABLE RegisterAITask(const wchar_t *description, uint32_t userId, const wchar_t *prompt)
{
   shared_ptr<AITask> task = make_shared<AITask>(description, userId, prompt);
   s_aiTasksLock.lock();
   s_aiTasks.set(task->getId(), task);
   s_aiTasksLock.unlock();
   nxlog_debug_tag(DEBUG_TAG, 4, L"Registered AI task [%u] \"%s\"", task->getId(), description);
   ConfigWriteInt(L"AITask.LastTaskId", s_taskId, true, false, true);
   return task->getId();
}

/**
 * AI task scheduler thread
 */
void AITaskSchedulerThread(ThreadPool *aiTaskThreadPool)
{
   nxlog_debug_tag(DEBUG_TAG, 1, _T("AI task scheduler thread started"));

   while(!SleepAndCheckForShutdown(30))
   {
      time_t now = time(nullptr);
      std::vector<shared_ptr<AITask>> tasksToExecute;

      // Find tasks ready for execution
      s_aiTasksLock.lock();
      for(const shared_ptr<AITask>& task : s_aiTasks)
      {
         if ((task->getStatus() == AITaskStatus::SCHEDULED) && (task->getNextExecutionTime() <= now))
         {
            tasksToExecute.push_back(task);
         }
      }
      s_aiTasksLock.unlock();

      // Execute ready tasks
      for(const shared_ptr<AITask>& task : tasksToExecute)
      {
         if (g_flags & AF_SHUTDOWN)
            break;

         nxlog_debug_tag(DEBUG_TAG, 5, _T("Executing AI task [%u] \"%s\""), task->getId(), task->getDescription());
         ThreadPoolExecute(aiTaskThreadPool, task, &AITask::execute);
      }
   }

   nxlog_debug_tag(DEBUG_TAG, 1, _T("AI task scheduler thread stopped"));
}

/**
 * AITask constructor
 */
AITask::AITask(const wchar_t *descripion, uint32_t userId, const wchar_t *prompt) : m_description(descripion)
{
   m_id = InterlockedIncrement(&s_taskId);
   m_status = AITaskStatus::SCHEDULED;
   m_lastExecutionTime = 0;
   m_nextExecutionTime = time(nullptr);
   m_iteration = 0;
   m_userId = userId;
   char *mp = UTF8StringFromWideString(prompt);
   m_prompt = mp;
   MemFree(mp);
}

/**
 * AITask constructor from database row
 * Expected column order:
 * id,description,prompt,memento,last_execution_time,next_execution_time,iteration,user_id
 */
AITask::AITask(DB_RESULT hResult, int row) : m_description(DBGetFieldAsString(hResult, row, 1))
{
   m_id = DBGetFieldUInt32(hResult, row, 0);
   char *prompt = DBGetFieldUTF8(hResult, row, 2, nullptr, 0);
   m_prompt = prompt;
   MemFree(prompt);
   char *memento = DBGetFieldUTF8(hResult, row, 3, nullptr, 0);
   m_memento = memento;
   MemFree(memento);
   m_lastExecutionTime = DBGetFieldUInt32(hResult, row, 4);
   m_nextExecutionTime = DBGetFieldUInt32(hResult, row, 5);
   m_iteration = DBGetFieldUInt32(hResult, row, 6);
   m_userId = DBGetFieldUInt32(hResult, row, 7);
   m_status = (m_nextExecutionTime == 0) ? AITaskStatus::COMPLETED : AITaskStatus::SCHEDULED;
}

/**
 * Execute AI task
 */
void AITask::execute()
{
   m_status = AITaskStatus::RUNNING;
   std::string prompt(m_prompt);
   prompt.append("</instructions>\n<current_time>");
   prompt.append(FormatISO8601Timestamp(time(nullptr)));
   prompt.append("</current_time>\n<iteration>");
   char buffer[32];
   prompt.append(IntegerToString(++m_iteration, buffer));
   prompt.append("</iteration>");
   if (!m_memento.empty())
   {
      prompt.append("\n<memento>");
      prompt.append(m_memento);
      prompt.append("</memento>\n");
   }
   char *response = ProcessRequestToAIAssistantEx(prompt.c_str(), s_systemPrompt.c_str(), nullptr, nullptr, nullptr, 64);
   if (response != nullptr)
   {
      json_t *json = json_loads(response, 0, nullptr);
      if (json != nullptr)
      {
         json_t *completed = json_object_get(json, "completed");
         if (json_is_boolean(completed))
         {
            m_explanation.clear();
            m_explanation.appendUtf8String(json_object_get_string_utf8(json, "explanation", ""));
            if (json_boolean_value(completed))
            {
               m_nextExecutionTime = 0;
               m_status = AITaskStatus::COMPLETED;
               nxlog_debug_tag(DEBUG_TAG, 5, L"AI task [%u] \"%s\" marked as completed", m_id, m_description.cstr());
            }
            else
            {
               m_nextExecutionTime = time(nullptr) + json_object_get_time(json, "next_execution_time", 0);
               m_status = AITaskStatus::SCHEDULED;
               nxlog_debug_tag(DEBUG_TAG, 5, L"AI task [%u] \"%s\" scheduled for next execution at %s", m_id, m_description.cstr(),
                  FormatTimestamp(m_nextExecutionTime).cstr());

               const char *newInstructions = json_object_get_string_utf8(json, "instructions", nullptr);
               if ((newInstructions != nullptr) && (*newInstructions != 0))
               {
                  m_prompt = newInstructions;
                  nxlog_debug_tag(DEBUG_TAG, 6, L"AI task [%u] new instructions: %hs", m_id, newInstructions);
               }

               m_memento.clear();
               json_t *mementoElement = json_object_get(json, "memento");
               if (json_is_string(mementoElement))
               {
                  const char *memento = json_string_value(mementoElement);
                  if (*memento != 0)
                  {
                     m_memento = memento;
                     nxlog_debug_tag(DEBUG_TAG, 6, L"AI task [%u] memento: %hs", m_id, memento);
                  }
               }
               else if (json_is_object(mementoElement) || json_is_array(mementoElement))
               {
                  char *memento = json_dumps(mementoElement, JSON_COMPACT);
                  if (memento != nullptr)
                  {
                     m_memento = memento;
                     nxlog_debug_tag(DEBUG_TAG, 6, L"AI task [%u] memento: %hs", m_id, memento);
                     MemFree(memento);
                  }
               }
            }
            nxlog_debug_tag(DEBUG_TAG, 6, L"AI task [%u] explanation: %s", m_id, m_explanation.cstr());
         }
         else
         {
            m_status = AITaskStatus::FAILED;
            nxlog_debug_tag(DEBUG_TAG, 5, L"AI task [%u] \"%s\" execution failed (cannot parse JSON response)", m_id, m_description.cstr());
         }
         json_decref(json);
      }
      else
      {
         m_status = AITaskStatus::FAILED;
         nxlog_debug_tag(DEBUG_TAG, 5, L"AI task [%u] \"%s\" execution failed (cannot parse JSON response)", m_id, m_description.cstr());
      }
      MemFree(response);
   }
   else
   {
      m_status = AITaskStatus::FAILED;
      nxlog_debug_tag(DEBUG_TAG, 5, L"AI task [%u] \"%s\" execution failed (no response from assistant)", m_id, m_description.cstr());
   }

   logExecution();

   if (m_status == AITaskStatus::SCHEDULED)
   {
      saveToDatabase();
   }
   else
   {
      deleteFromDatabase();
   }
}

/**
 * Log AI task execution to database
 */
void AITask::logExecution()
{
   DB_HANDLE db = DBConnectionPoolAcquireConnection();

   // Generate unique record ID
   uint64_t recordId = InterlockedIncrement64(&s_logRecordId);

   // Determine status character based on current task status
   wchar_t status[2];
   switch (m_status)
   {
      case AITaskStatus::COMPLETED:
         status[0] = 'C';
         break;
      case AITaskStatus::FAILED:
         status[0] = 'F';
         break;
      case AITaskStatus::SCHEDULED:
         status[0] = 'S';
         break;
      case AITaskStatus::RUNNING:
         status[0] = 'R';
         break;
      default:
         status[0] = 'U'; // Unknown
         break;
   }
   status[1] = 0;

   DB_STATEMENT hStmt = DBPrepare(db, 
      g_dbSyntax == DB_SYNTAX_TSDB ?
         L"INSERT INTO ai_task_execution_log (record_id,execution_timestamp,task_id,task_description,user_id,status,iteration,explanation) VALUES (?,to_timestamp(?),?,?,?,?,?,?)" :
         L"INSERT INTO ai_task_execution_log (record_id,execution_timestamp,task_id,task_description,user_id,status,iteration,explanation) VALUES (?,?,?,?,?,?,?,?)");

   if (hStmt != nullptr)
   {
      time_t now = time(nullptr);
      DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, recordId);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(now));
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_id);
      DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_description, DB_BIND_STATIC);
      DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, m_userId);
      DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, status, DB_BIND_STATIC);
      DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_iteration);
      DBBind(hStmt, 8, DB_SQLTYPE_TEXT, m_explanation, DB_BIND_STATIC);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }

   DBConnectionPoolReleaseConnection(db);
}

/**
 * Save AI task to database
 */
void AITask::saveToDatabase() const
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   
   const wchar_t *mergeColumns[] = { L"user_id", L"description", L"prompt", L"memento", L"last_execution_time", L"next_execution_time", L"iteration", nullptr };
   DB_STATEMENT hStmt = DBPrepareMerge(hdb, L"ai_tasks", L"id", m_id, mergeColumns);
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_userId);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_description, DB_BIND_STATIC, 255);
      DBBind(hStmt, 3, DB_SQLTYPE_TEXT, DB_CTYPE_UTF8_STRING, m_prompt.c_str(), DB_BIND_STATIC);
      DBBind(hStmt, 4, DB_SQLTYPE_TEXT, DB_CTYPE_UTF8_STRING, m_memento.c_str(), DB_BIND_STATIC);
      DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_lastExecutionTime));
      DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_nextExecutionTime));
      DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_iteration);
      DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_id);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
   
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Delete AI task from database
 */
void AITask::deleteFromDatabase()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM ai_tasks WHERE id=?"));
   DBConnectionPoolReleaseConnection(hdb);
}
