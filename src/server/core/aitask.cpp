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

/**
 * AI Task management
 */
static SharedHashMap<uint32_t, AITask> s_aiTasks;
static Mutex s_aiTasksLock;
static VolatileCounter s_taskId = 0; // Last used task ID

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
   m_userId = userId;
   m_prompt =
      "Follow the instructions below to perform requested task. "
      "Task could be long running and may require multiple executions to complete. "
      "Use available functions to retrieve necessary data from NetXMS system or perform actions. "
      "Provide structured execution report in JSON format, use only clean JSON without any additional text. "
      "Do not use markdown formatting or code blocks. "
      "If task is long or inifinite, respond with completed=false and provide next_execution_time for rescheduling. "
      "If data need to be preserved between executions, use memento field for that purpose. "
      "Memento content will be passed back to you during next execution. "
      "Memento can also be used to provide instructions or context for next execution. "
      "Answer should have the following fields: completed, next_execution_time, memento, and explanation. "
      "'completed' attribute should be set to true if no further action is required, or false if task needs to be rescheduled. "
      "'next_execution_time' should be in number if seconds and indicate delay until next task execution if it is not completed. "
      "'memento' should contain any data that needs to be preserved between task executions. "
      "'explanation' should provide a brief reasoning behind your decision. ";
   ENUMERATE_MODULES(pfGetAIAgentInstructions)
   {
      m_prompt.append(CURRENT_MODULE.pfGetAIAgentInstructions());
   }
   m_prompt.append("\n<instructions>");
   char *mp = UTF8StringFromWideString(prompt);
   m_prompt.append(mp);
   MemFree(mp);
   m_prompt.append("</instructions>");
   if (!m_memento.empty())
   {
      m_prompt.append("\n<memento>");
      m_prompt.append(m_memento);
      m_prompt.append("</memento>\n");
   }
}

/**
 * Execute AI task
 */
void AITask::execute()
{
   m_status = AITaskStatus::RUNNING;
   char *response = ProcessRequestToAIAssistant(m_prompt.c_str(), nullptr, nullptr, 16);
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

               m_memento.clear();
               const char *memento = json_object_get_string_utf8(json, "memento", "");
               if (*memento != 0)
               {
                  m_memento = memento;
                  nxlog_debug_tag(DEBUG_TAG, 6, L"AI task [%u] memento: %hs", m_id, memento);
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
}
