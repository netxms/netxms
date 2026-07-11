/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
** File: ai_operator.cpp
**
**/

#include "nxcore.h"
#include <nxai.h>
#include <ai_provider.h>
#include <nms_users.h>

#define DEBUG_TAG L"ai.operator"

/**
 * Login name of out-of-band account used for AI operator executions
 */
#define AI_OPERATOR_ACCOUNT_NAME L"ai-operator"

/**
 * AI operator instance registry
 */
static SharedHashMap<uint32_t, AIOperatorInstance> s_instances;
static Mutex s_instancesLock;
static VolatileCounter s_instanceId = 0;    // Last used instance ID
static VolatileCounter64 s_observationId = 0;  // Last used observation ID
static VolatileCounter64 s_logRecordId = 0;    // Last used execution log record ID
static uint32_t s_operatorUserId = INVALID_UID;
static ThreadPool *s_threadPool = nullptr;
static std::string s_systemPrompt;

/**
 * Instance being executed on this thread (set for the duration of AIOperatorInstance::execute()
 * so that AI functions like record-observation can attribute their effects)
 */
static thread_local AIOperatorInstance *s_currentInstance = nullptr;

/**
 * Create new AI operator instance
 */
AIOperatorInstance::AIOperatorInstance(const wchar_t *name, uint32_t ownerUserId)
{
   m_id = InterlockedIncrement(&s_instanceId);
   wcslcpy(m_name, name, 64);
   m_ownerUserId = ownerUserId;
   m_enabled = false;
   m_modelSlot[0] = 0;
   m_minInterval = 300;
   m_maxInterval = 3600;
   m_dailyTokenBudget = 0;
   m_tokensUsed = 0;
   m_usageDay = 0;
   m_currentFocus[0] = 0;
   m_observationRetentionDays = 0;
   m_observationMaxRecords = 0;
   m_lastExecutionTime = 0;
   m_nextExecutionTime = 0;
   m_iteration = 0;
   m_creationTime = m_modificationTime = time(nullptr);
   m_executing = false;
   m_consecutiveFailures = 0;
}

/**
 * Create AI operator instance from database row. Expected column order:
 * id,name,description,owner_user_id,enabled,scope_filter,model_slot,min_interval,max_interval,daily_token_budget,
 * tokens_used,usage_day,persona_prompt,current_focus,watch_list,memento,observation_retention_days,
 * observation_max_records,last_execution_time,next_execution_time,iteration,created,modified
 */
AIOperatorInstance::AIOperatorInstance(DB_RESULT hResult, int row)
{
   m_id = DBGetFieldUInt32(hResult, row, 0);
   DBGetField(hResult, row, 1, m_name, 64);
   m_description = DBGetFieldAsString(hResult, row, 2);
   m_ownerUserId = DBGetFieldUInt32(hResult, row, 3);
   wchar_t flag[2];
   DBGetField(hResult, row, 4, flag, 2);
   m_enabled = (flag[0] == '1');
   char *text = DBGetFieldUTF8(hResult, row, 5, nullptr, 0);
   m_scopeFilter = CHECK_NULL_EX_A(text);
   MemFree(text);
   DBGetFieldUTF8(hResult, row, 6, m_modelSlot, 64);
   m_minInterval = DBGetFieldUInt32(hResult, row, 7);
   m_maxInterval = DBGetFieldUInt32(hResult, row, 8);
   m_dailyTokenBudget = DBGetFieldUInt32(hResult, row, 9);
   m_tokensUsed = DBGetFieldInt64(hResult, row, 10);
   m_usageDay = DBGetFieldUInt32(hResult, row, 11);
   text = DBGetFieldUTF8(hResult, row, 12, nullptr, 0);
   m_personaPrompt = CHECK_NULL_EX_A(text);
   MemFree(text);
   DBGetField(hResult, row, 13, m_currentFocus, 256);
   text = DBGetFieldUTF8(hResult, row, 14, nullptr, 0);
   m_watchList = CHECK_NULL_EX_A(text);
   MemFree(text);
   text = DBGetFieldUTF8(hResult, row, 15, nullptr, 0);
   m_memento = CHECK_NULL_EX_A(text);
   MemFree(text);
   m_observationRetentionDays = DBGetFieldUInt32(hResult, row, 16);
   m_observationMaxRecords = DBGetFieldUInt32(hResult, row, 17);
   m_lastExecutionTime = DBGetFieldUInt32(hResult, row, 18);
   m_nextExecutionTime = DBGetFieldUInt32(hResult, row, 19);
   m_iteration = DBGetFieldUInt32(hResult, row, 20);
   m_creationTime = DBGetFieldUInt32(hResult, row, 21);
   m_modificationTime = DBGetFieldUInt32(hResult, row, 22);
   m_executing = false;
   m_consecutiveFailures = 0;
}

/**
 * Save AI operator instance to database. Must be called with instance lock held.
 */
void AIOperatorInstance::saveToDatabase() const
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   static const wchar_t *mergeColumns[] = {
      L"name", L"description", L"owner_user_id", L"enabled", L"scope_filter", L"model_slot", L"min_interval",
      L"max_interval", L"daily_token_budget", L"tokens_used", L"usage_day", L"persona_prompt", L"current_focus",
      L"watch_list", L"memento", L"observation_retention_days", L"observation_max_records", L"last_execution_time",
      L"next_execution_time", L"iteration", L"created", L"modified", nullptr
   };
   DB_STATEMENT hStmt = DBPrepareMerge(hdb, L"ai_operator_instances", L"id", m_id, mergeColumns);
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC, 63);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_description, DB_BIND_STATIC, 255);
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_ownerUserId);
      DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_enabled ? L"1" : L"0", DB_BIND_STATIC);
      DBBind(hStmt, 5, DB_SQLTYPE_TEXT, DB_CTYPE_UTF8_STRING, m_scopeFilter.c_str(), DB_BIND_STATIC);
      DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, DB_CTYPE_UTF8_STRING, m_modelSlot, DB_BIND_STATIC);
      DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_minInterval);
      DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_maxInterval);
      DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, m_dailyTokenBudget);
      DBBind(hStmt, 10, DB_SQLTYPE_BIGINT, m_tokensUsed);
      DBBind(hStmt, 11, DB_SQLTYPE_INTEGER, m_usageDay);
      DBBind(hStmt, 12, DB_SQLTYPE_TEXT, DB_CTYPE_UTF8_STRING, m_personaPrompt.c_str(), DB_BIND_STATIC);
      DBBind(hStmt, 13, DB_SQLTYPE_VARCHAR, m_currentFocus, DB_BIND_STATIC, 255);
      DBBind(hStmt, 14, DB_SQLTYPE_TEXT, DB_CTYPE_UTF8_STRING, m_watchList.c_str(), DB_BIND_STATIC);
      DBBind(hStmt, 15, DB_SQLTYPE_TEXT, DB_CTYPE_UTF8_STRING, m_memento.c_str(), DB_BIND_STATIC);
      DBBind(hStmt, 16, DB_SQLTYPE_INTEGER, m_observationRetentionDays);
      DBBind(hStmt, 17, DB_SQLTYPE_INTEGER, m_observationMaxRecords);
      DBBind(hStmt, 18, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_lastExecutionTime));
      DBBind(hStmt, 19, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_nextExecutionTime));
      DBBind(hStmt, 20, DB_SQLTYPE_INTEGER, m_iteration);
      DBBind(hStmt, 21, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_creationTime));
      DBBind(hStmt, 22, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_modificationTime));
      DBBind(hStmt, 23, DB_SQLTYPE_INTEGER, m_id);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }

   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Delete AI operator instance and its observations from database
 */
void AIOperatorInstance::deleteFromDatabase()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   ExecuteQueryOnObject(hdb, m_id, L"DELETE FROM ai_operator_instances WHERE id=?");
   ExecuteQueryOnObject(hdb, m_id, L"DELETE FROM ai_operator_observations WHERE instance_id=?");
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Log AI operator execution to database. Must be called with instance lock held.
 */
void AIOperatorInstance::logExecution(wchar_t status, uint32_t durationMs, int64_t inputTokens, int64_t outputTokens)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT hStmt = DBPrepare(hdb,
      g_dbSyntax == DB_SYNTAX_TSDB ?
         L"INSERT INTO ai_operator_execution_log (record_id,execution_timestamp,instance_id,instance_name,status,iteration,duration_ms,input_tokens,output_tokens,explanation) VALUES (?,to_timestamp(?),?,?,?,?,?,?,?,?)" :
         L"INSERT INTO ai_operator_execution_log (record_id,execution_timestamp,instance_id,instance_name,status,iteration,duration_ms,input_tokens,output_tokens,explanation) VALUES (?,?,?,?,?,?,?,?,?,?)");
   if (hStmt != nullptr)
   {
      wchar_t statusText[2] = { status, 0 };
      DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, InterlockedIncrement64(&s_logRecordId));
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(time(nullptr)));
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_id);
      DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC, 63);
      DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, statusText, DB_BIND_STATIC);
      DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_iteration);
      DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, durationMs);
      DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, static_cast<int32_t>(inputTokens));
      DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, static_cast<int32_t>(outputTokens));
      DBBind(hStmt, 10, DB_SQLTYPE_TEXT, m_lastExplanation, DB_BIND_STATIC);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }

   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Process LLM response for one monitoring iteration. Must be called with instance lock held.
 * Returns false if response does not follow the expected contract.
 */
bool AIOperatorInstance::processResponse(const char *response, time_t now)
{
   json_t *json = json_loads(response, 0, nullptr);
   if (!json_is_object(json))
   {
      if (json != nullptr)
         json_decref(json);
      return false;
   }

   time_t delay = json_object_get_time(json, "next_execution_time", 0);
   if (delay < m_minInterval)
      delay = m_minInterval;
   else if (delay > m_maxInterval)
      delay = m_maxInterval;
   m_nextExecutionTime = now + delay;

   const char *currentFocus = json_object_get_string_utf8(json, "current_focus", nullptr);
   if (currentFocus != nullptr)
   {
      utf8_to_wchar(currentFocus, -1, m_currentFocus, 256);
      m_currentFocus[255] = 0;
   }

   json_t *watchList = json_object_get(json, "watch_list");
   if (json_is_string(watchList))
   {
      m_watchList = json_string_value(watchList);
   }
   else if (json_is_array(watchList) || json_is_object(watchList))
   {
      char *text = json_dumps(watchList, JSON_COMPACT);
      if (text != nullptr)
      {
         m_watchList = text;
         MemFree(text);
      }
   }

   // Keep previous memento when the field is omitted from an otherwise valid response
   json_t *memento = json_object_get(json, "memento");
   if (json_is_string(memento))
   {
      m_memento = json_string_value(memento);
   }
   else if (json_is_array(memento) || json_is_object(memento))
   {
      char *text = json_dumps(memento, JSON_COMPACT);
      if (text != nullptr)
      {
         m_memento = text;
         MemFree(text);
      }
   }

   m_lastExplanation.clear();
   m_lastExplanation.appendUtf8String(json_object_get_string_utf8(json, "explanation", ""));

   json_decref(json);

   nxlog_debug_tag(DEBUG_TAG, 5, L"AI operator [%u] \"%s\" iteration %u completed, next execution at %s",
      m_id, m_name, m_iteration, FormatTimestamp(m_nextExecutionTime).cstr());
   nxlog_debug_tag(DEBUG_TAG, 6, L"AI operator [%u] explanation: %s", m_id, m_lastExplanation.cstr());
   return true;
}

/**
 * Handle execution failure. Must be called with instance lock held.
 */
void AIOperatorInstance::handleFailure(const char *error, time_t now)
{
   m_consecutiveFailures++;
   m_lastExplanation.clear();
   m_lastExplanation.appendUtf8String(error);

   int maxFailures = ConfigReadInt(L"AIOperator.MaxConsecutiveFailures", 3);
   if (m_consecutiveFailures >= maxFailures)
   {
      m_enabled = false;
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"AI operator [%u] \"%s\" disabled after %d consecutive failures (last error: %hs)",
         m_id, m_name, m_consecutiveFailures, error);
      EventBuilder(EVENT_AI_OPERATOR_FAILURE, GetServerEventSourceId())
         .param(L"instanceId", m_id)
         .param(L"instanceName", m_name)
         .param(L"failureCount", static_cast<uint32_t>(m_consecutiveFailures))
         .paramUtf8String(L"lastError", error)
         .post();
   }
   else
   {
      // Retry with previous memento after minimal interval
      m_nextExecutionTime = now + m_minInterval;
      nxlog_debug_tag(DEBUG_TAG, 5, L"AI operator [%u] \"%s\" iteration %u failed (%hs), retry at %s",
         m_id, m_name, m_iteration, error, FormatTimestamp(m_nextExecutionTime).cstr());
   }
}

/**
 * Execute one monitoring iteration
 */
void AIOperatorInstance::execute()
{
   int64_t startTime = GetCurrentTimeMs();
   time_t now = time(nullptr);

   m_mutex.lock();

   // Reset token usage counter on day boundary (UTC)
   uint32_t today = static_cast<uint32_t>(now / 86400);
   if (m_usageDay != today)
   {
      m_usageDay = today;
      m_tokensUsed = 0;
   }

   if ((m_dailyTokenBudget > 0) && (m_tokensUsed >= m_dailyTokenBudget))
   {
      // Budget exhausted - skip until next day
      m_nextExecutionTime = static_cast<time_t>(today + 1) * 86400;
      m_lastExplanation = L"Daily token budget exhausted";
      nxlog_debug_tag(DEBUG_TAG, 5, L"AI operator [%u] \"%s\" skipped (daily token budget exhausted), next execution at %s",
         m_id, m_name, FormatTimestamp(m_nextExecutionTime).cstr());
      logExecution('S', 0, 0, 0);
      saveToDatabase();
      m_executing = false;
      m_mutex.unlock();
      return;
   }

   if ((GetEffectiveSystemRights(s_operatorUserId) & SYSTEM_ACCESS_USE_AI_ASSISTANT) == 0)
   {
      handleFailure("AI operator account does not have AI assistant access right", now);
      logExecution('F', 0, 0, 0);
      saveToDatabase();
      m_executing = false;
      m_mutex.unlock();
      return;
   }

   m_iteration++;
   m_lastExecutionTime = now;

   std::string systemPrompt(s_systemPrompt);
   if (!m_personaPrompt.empty())
   {
      systemPrompt.append("\n\nOPERATOR PERSONA:\n");
      systemPrompt.append(m_personaPrompt);
   }

   std::string prompt("Perform one monitoring iteration now according to your instructions.\n<current_time>");
   prompt.append(FormatISO8601Timestamp(now));
   prompt.append("</current_time>\n<iteration>");
   char buffer[32];
   prompt.append(IntegerToString(m_iteration, buffer));
   prompt.append("</iteration>\n<interval_bounds>min=");
   prompt.append(IntegerToString(m_minInterval, buffer));
   prompt.append(" max=");
   prompt.append(IntegerToString(m_maxInterval, buffer));
   prompt.append("</interval_bounds>");
   if (!m_scopeFilter.empty())
   {
      prompt.append("\n<scope>");
      prompt.append(m_scopeFilter);
      prompt.append("</scope>");
   }
   if (m_currentFocus[0] != 0)
   {
      char *currentFocus = UTF8StringFromWideString(m_currentFocus);
      prompt.append("\n<current_focus>");
      prompt.append(currentFocus);
      prompt.append("</current_focus>");
      MemFree(currentFocus);
   }
   if (!m_watchList.empty())
   {
      prompt.append("\n<watch_list>");
      prompt.append(m_watchList);
      prompt.append("</watch_list>");
   }
   if (!m_memento.empty())
   {
      prompt.append("\n<memento>");
      prompt.append(m_memento);
      prompt.append("</memento>");
   }

   char slot[64];
   strlcpy(slot, m_modelSlot, 64);

   m_mutex.unlock();

   Chat chat(nullptr, nullptr, s_operatorUserId, systemPrompt.c_str(), false);
   if (slot[0] != 0)
      chat.setSlot(slot);

   s_currentInstance = this;
   LLMTokenUsage tokenUsage;
   LLMTokenUsage *previousCollector = SetLLMTokenUsageCollector(&tokenUsage);
   char *response = chat.sendRequest(prompt.c_str());
   SetLLMTokenUsageCollector(previousCollector);
   s_currentInstance = nullptr;

   uint32_t durationMs = static_cast<uint32_t>(GetCurrentTimeMs() - startTime);

   m_mutex.lock();
   m_tokensUsed += tokenUsage.inputTokens + tokenUsage.outputTokens;

   bool success = false;
   if (response != nullptr)
   {
      success = processResponse(response, time(nullptr));
      if (!success)
         handleFailure("Cannot parse assistant response", time(nullptr));
      MemFree(response);
   }
   else
   {
      handleFailure("No response from assistant", time(nullptr));
   }

   if (success)
      m_consecutiveFailures = 0;

   logExecution(success ? 'C' : 'F', durationMs, tokenUsage.inputTokens, tokenUsage.outputTokens);

   // Do not re-create database record if instance was deleted while executing
   s_instancesLock.lock();
   bool registered = s_instances.contains(m_id);
   s_instancesLock.unlock();
   if (registered)
      saveToDatabase();

   m_executing = false;
   m_mutex.unlock();
}

/**
 * Modify AI operator instance from JSON configuration
 */
uint32_t AIOperatorInstance::modifyFromJSON(json_t *config)
{
   LockGuard lockGuard(m_mutex);

   json_t *name = json_object_get(config, "name");
   if (name != nullptr)
   {
      if (!json_is_string(name) || (*json_string_value(name) == 0))
         return RCC_INVALID_ARGUMENT;
      utf8_to_wchar(json_string_value(name), -1, m_name, 64);
      m_name[63] = 0;
   }

   json_t *description = json_object_get(config, "description");
   if (description != nullptr)
   {
      if (!json_is_string(description) && !json_is_null(description))
         return RCC_INVALID_ARGUMENT;
      m_description = String(json_is_string(description) ? json_string_value(description) : "", "utf8");
   }

   json_t *scopeFilter = json_object_get(config, "scopeFilter");
   if (scopeFilter != nullptr)
   {
      if (!json_is_string(scopeFilter) && !json_is_null(scopeFilter))
         return RCC_INVALID_ARGUMENT;
      m_scopeFilter = json_is_string(scopeFilter) ? json_string_value(scopeFilter) : "";
   }

   json_t *personaPrompt = json_object_get(config, "personaPrompt");
   if (personaPrompt != nullptr)
   {
      if (!json_is_string(personaPrompt) && !json_is_null(personaPrompt))
         return RCC_INVALID_ARGUMENT;
      m_personaPrompt = json_is_string(personaPrompt) ? json_string_value(personaPrompt) : "";
   }

   if (!json_object_update_string_utf8(config, "modelSlot", m_modelSlot, 64) ||
       !json_object_update_integer(config, "minInterval", &m_minInterval) ||
       !json_object_update_integer(config, "maxInterval", &m_maxInterval) ||
       !json_object_update_integer(config, "dailyTokenBudget", &m_dailyTokenBudget) ||
       !json_object_update_integer(config, "observationRetentionDays", &m_observationRetentionDays) ||
       !json_object_update_integer(config, "observationMaxRecords", &m_observationMaxRecords))
      return RCC_INVALID_ARGUMENT;

   if (m_minInterval < 60)
      m_minInterval = 60;
   if (m_maxInterval < m_minInterval)
      return RCC_INVALID_ARGUMENT;

   bool enabled = m_enabled;
   if (!json_object_update_boolean(config, "enabled", &enabled))
      return RCC_INVALID_ARGUMENT;
   if (enabled != m_enabled)
   {
      m_enabled = enabled;
      if (enabled)
      {
         m_consecutiveFailures = 0;
         m_nextExecutionTime = time(nullptr);
      }
   }

   m_modificationTime = time(nullptr);
   saveToDatabase();
   return RCC_SUCCESS;
}

/**
 * Enable or disable AI operator instance
 */
void AIOperatorInstance::setEnabled(bool enabled)
{
   LockGuard lockGuard(m_mutex);
   if (m_enabled == enabled)
      return;

   m_enabled = enabled;
   if (enabled)
   {
      m_consecutiveFailures = 0;
      m_nextExecutionTime = time(nullptr);
   }
   m_modificationTime = time(nullptr);
   saveToDatabase();
   nxlog_debug_tag(DEBUG_TAG, 4, L"AI operator [%u] \"%s\" %s", m_id, m_name, enabled ? L"enabled" : L"disabled");
}

/**
 * Reset accumulated state (memento, focus, watch list, iteration counter)
 */
void AIOperatorInstance::resetMemento()
{
   LockGuard lockGuard(m_mutex);
   m_memento.clear();
   m_watchList.clear();
   m_currentFocus[0] = 0;
   m_iteration = 0;
   m_modificationTime = time(nullptr);
   saveToDatabase();
   nxlog_debug_tag(DEBUG_TAG, 4, L"AI operator [%u] \"%s\" accumulated state reset", m_id, m_name);
}

/**
 * Serialize AI operator instance to JSON
 */
json_t *AIOperatorInstance::toJson() const
{
   LockGuard lockGuard(m_mutex);
   json_t *json = json_object();
   json_object_set_new(json, "id", json_integer(m_id));
   json_object_set_new(json, "name", json_string_w(m_name));
   json_object_set_new(json, "description", json_string_t(m_description));
   json_object_set_new(json, "ownerUserId", json_integer(m_ownerUserId));
   json_object_set_new(json, "enabled", json_boolean(m_enabled));
   json_object_set_new(json, "executing", json_boolean(m_executing));
   json_object_set_new(json, "scopeFilter", json_string(m_scopeFilter.c_str()));
   json_object_set_new(json, "modelSlot", json_string(m_modelSlot));
   json_object_set_new(json, "minInterval", json_integer(m_minInterval));
   json_object_set_new(json, "maxInterval", json_integer(m_maxInterval));
   json_object_set_new(json, "dailyTokenBudget", json_integer(m_dailyTokenBudget));
   json_object_set_new(json, "tokensUsedToday", json_integer(m_tokensUsed));
   json_object_set_new(json, "personaPrompt", json_string(m_personaPrompt.c_str()));
   json_object_set_new(json, "currentFocus", json_string_w(m_currentFocus));
   json_object_set_new(json, "watchList", json_string(m_watchList.c_str()));
   json_object_set_new(json, "memento", json_string(m_memento.c_str()));
   json_object_set_new(json, "observationRetentionDays", json_integer(m_observationRetentionDays));
   json_object_set_new(json, "observationMaxRecords", json_integer(m_observationMaxRecords));
   json_object_set_new(json, "lastExecutionTime", json_time_string(m_lastExecutionTime));
   json_object_set_new(json, "nextExecutionTime", json_time_string(m_nextExecutionTime));
   json_object_set_new(json, "iteration", json_integer(m_iteration));
   json_object_set_new(json, "consecutiveFailures", json_integer(m_consecutiveFailures));
   json_object_set_new(json, "lastExplanation", json_string_t(m_lastExplanation));
   json_object_set_new(json, "created", json_time_string(m_creationTime));
   json_object_set_new(json, "modified", json_time_string(m_modificationTime));
   return json;
}

/**
 * Create AI operator instance from JSON configuration
 */
uint32_t NXCORE_EXPORTABLE CreateAIOperatorInstance(json_t *config, uint32_t ownerUserId, uint32_t *instanceId)
{
   const char *name = json_object_get_string_utf8(config, "name", nullptr);
   if ((name == nullptr) || (*name == 0))
      return RCC_INVALID_ARGUMENT;

   wchar_t wname[64];
   utf8_to_wchar(name, -1, wname, 64);
   wname[63] = 0;

   shared_ptr<AIOperatorInstance> instance = make_shared<AIOperatorInstance>(wname, ownerUserId);
   uint32_t rcc = instance->modifyFromJSON(config);
   if (rcc != RCC_SUCCESS)
      return rcc;

   // New instances are enabled by default unless explicitly created as disabled
   if (json_object_get(config, "enabled") == nullptr)
      instance->setEnabled(true);

   s_instancesLock.lock();
   s_instances.set(instance->getId(), instance);
   s_instancesLock.unlock();
   ConfigWriteInt(L"AIOperator.LastInstanceId", s_instanceId, true, false, true);

   nxlog_debug_tag(DEBUG_TAG, 4, L"Created AI operator instance [%u] \"%s\"", instance->getId(), instance->getName());
   if (instanceId != nullptr)
      *instanceId = instance->getId();
   return RCC_SUCCESS;
}

/**
 * Modify AI operator instance from JSON configuration
 */
uint32_t NXCORE_EXPORTABLE ModifyAIOperatorInstance(uint32_t instanceId, json_t *config)
{
   s_instancesLock.lock();
   shared_ptr<AIOperatorInstance> instance = s_instances.getShared(instanceId);
   s_instancesLock.unlock();
   if (instance == nullptr)
      return RCC_INVALID_TASK_ID;
   return instance->modifyFromJSON(config);
}

/**
 * Delete AI operator instance
 */
uint32_t NXCORE_EXPORTABLE DeleteAIOperatorInstance(uint32_t instanceId)
{
   s_instancesLock.lock();
   shared_ptr<AIOperatorInstance> instance = s_instances.getShared(instanceId);
   if (instance != nullptr)
      s_instances.remove(instanceId);
   s_instancesLock.unlock();
   if (instance == nullptr)
      return RCC_INVALID_TASK_ID;

   instance->deleteFromDatabase();
   nxlog_debug_tag(DEBUG_TAG, 4, L"Deleted AI operator instance [%u] \"%s\"", instance->getId(), instance->getName());
   return RCC_SUCCESS;
}

/**
 * Enable or disable AI operator instance
 */
uint32_t NXCORE_EXPORTABLE SetAIOperatorInstanceEnabled(uint32_t instanceId, bool enabled)
{
   s_instancesLock.lock();
   shared_ptr<AIOperatorInstance> instance = s_instances.getShared(instanceId);
   s_instancesLock.unlock();
   if (instance == nullptr)
      return RCC_INVALID_TASK_ID;
   instance->setEnabled(enabled);
   return RCC_SUCCESS;
}

/**
 * Reset AI operator instance accumulated state
 */
uint32_t NXCORE_EXPORTABLE ResetAIOperatorInstanceMemento(uint32_t instanceId)
{
   s_instancesLock.lock();
   shared_ptr<AIOperatorInstance> instance = s_instances.getShared(instanceId);
   s_instancesLock.unlock();
   if (instance == nullptr)
      return RCC_INVALID_TASK_ID;
   instance->resetMemento();
   return RCC_SUCCESS;
}

/**
 * Get AI operator instance by ID
 */
shared_ptr<AIOperatorInstance> NXCORE_EXPORTABLE GetAIOperatorInstance(uint32_t instanceId)
{
   s_instancesLock.lock();
   shared_ptr<AIOperatorInstance> instance = s_instances.getShared(instanceId);
   s_instancesLock.unlock();
   return instance;
}

/**
 * Get all AI operator instances as JSON array
 */
json_t NXCORE_EXPORTABLE *GetAIOperatorInstancesAsJson()
{
   json_t *output = json_array();
   s_instancesLock.lock();
   s_instances.forEach(
      [output] (const uint32_t& key, const shared_ptr<AIOperatorInstance>& instance) -> EnumerationCallbackResult
      {
         json_array_append_new(output, instance->toJson());
         return _CONTINUE;
      });
   s_instancesLock.unlock();
   return output;
}

/**
 * Record observation made by AI operator instance
 */
static int64_t RecordObservation(AIOperatorInstance *instance, int severity, const char *title, const char *body, uint32_t objectId, const char *refs)
{
   int64_t observationId = InterlockedIncrement64(&s_observationId);

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb,
      g_dbSyntax == DB_SYNTAX_TSDB ?
         L"INSERT INTO ai_operator_observations (id,observation_timestamp,instance_id,severity,title,body,object_id,refs,state) VALUES (?,to_timestamp(?),?,?,?,?,?,?,'0')" :
         L"INSERT INTO ai_operator_observations (id,observation_timestamp,instance_id,severity,title,body,object_id,refs,state) VALUES (?,?,?,?,?,?,?,?,'0')");
   bool success = false;
   if (hStmt != nullptr)
   {
      wchar_t wtitle[256];
      utf8_to_wchar(title, -1, wtitle, 256);
      wtitle[255] = 0;
      DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, observationId);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(time(nullptr)));
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, instance->getId());
      DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, severity);
      DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, wtitle, DB_BIND_STATIC);
      DBBind(hStmt, 6, DB_SQLTYPE_TEXT, DB_CTYPE_UTF8_STRING, CHECK_NULL_EX_A(body), DB_BIND_STATIC);
      DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, objectId);
      DBBind(hStmt, 8, DB_SQLTYPE_TEXT, DB_CTYPE_UTF8_STRING, CHECK_NULL_EX_A(refs), DB_BIND_STATIC);
      success = DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);

   if (!success)
      return -1;

   EventBuilder(EVENT_AI_OPERATOR_OBSERVATION, (objectId != 0) ? objectId : GetServerEventSourceId())
      .param(L"instanceId", instance->getId())
      .param(L"instanceName", instance->getName())
      .param(L"observationId", observationId)
      .param(L"severity", static_cast<uint32_t>(severity))
      .paramUtf8String(L"title", title)
      .post();

   nxlog_debug_tag(DEBUG_TAG, 5, L"AI operator [%u] \"%s\" recorded observation " INT64_FMT L" (severity=%d): %hs",
      instance->getId(), instance->getName(), observationId, severity, title);
   return observationId;
}

/**
 * Update AI operator observation state (acknowledge/dismiss)
 */
uint32_t NXCORE_EXPORTABLE UpdateAIOperatorObservationState(int64_t observationId, AIObservationState state)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   uint32_t rcc = RCC_DB_FAILURE;
   DB_STATEMENT hStmt = DBPrepare(hdb, L"UPDATE ai_operator_observations SET state=? WHERE id=?");
   if (hStmt != nullptr)
   {
      wchar_t stateText[2] = { static_cast<wchar_t>('0' + static_cast<int>(state)), 0 };
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, stateText, DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_BIGINT, observationId);
      if (DBExecute(hStmt))
         rcc = RCC_SUCCESS;
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);
   return rcc;
}

/**
 * Housekeeping for AI operator observations: enforce retention time and per-instance record cap,
 * using per-instance overrides when set (0 = use server-wide defaults)
 */
void CleanAIOperatorObservations(DB_HANDLE hdb, time_t cycleStartTime)
{
   uint32_t defaultRetentionDays = ConfigReadULong(L"AIOperatorObservations.RetentionTime", 90);
   uint32_t defaultMaxRecords = ConfigReadULong(L"AIOperatorObservations.MaxRecordsPerInstance", 1000);

   std::vector<std::pair<uint32_t, std::pair<uint32_t, uint32_t>>> instances;
   s_instancesLock.lock();
   for(const shared_ptr<AIOperatorInstance>& instance : s_instances)
      instances.push_back({ instance->getId(), { instance->getObservationRetentionDays(), instance->getObservationMaxRecords() } });
   s_instancesLock.unlock();

   wchar_t query[256];
   for(const auto& entry : instances)
   {
      uint32_t instanceId = entry.first;

      uint32_t retentionDays = (entry.second.first > 0) ? entry.second.first : defaultRetentionDays;
      if (retentionDays > 0)
      {
         int64_t cutoff = static_cast<int64_t>(cycleStartTime) - static_cast<int64_t>(retentionDays) * 86400;
         if (g_dbSyntax == DB_SYNTAX_TSDB)
            nx_swprintf(query, 256, L"DELETE FROM ai_operator_observations WHERE instance_id=%u AND observation_timestamp<to_timestamp(" INT64_FMT L")", instanceId, cutoff);
         else
            nx_swprintf(query, 256, L"DELETE FROM ai_operator_observations WHERE instance_id=%u AND observation_timestamp<" INT64_FMT, instanceId, cutoff);
         DBQuery(hdb, query);
      }

      uint32_t maxRecords = (entry.second.second > 0) ? entry.second.second : defaultMaxRecords;
      if (maxRecords > 0)
      {
         nx_swprintf(query, 256, L"SELECT id FROM ai_operator_observations WHERE instance_id=%u ORDER BY id DESC", instanceId);
         DB_RESULT hResult = DBSelect(hdb, query);
         if (hResult != nullptr)
         {
            int count = DBGetNumRows(hResult);
            if (count > static_cast<int>(maxRecords))
            {
               int64_t cutoffId = DBGetFieldInt64(hResult, maxRecords, 0);
               nx_swprintf(query, 256, L"DELETE FROM ai_operator_observations WHERE instance_id=%u AND id<=" INT64_FMT, instanceId, cutoffId);
               DBQuery(hdb, query);
            }
            DBFreeResult(hResult);
         }
      }
   }
}

/**
 * AI assistant function: record-observation
 */
static std::string F_RecordObservation(json_t *arguments, uint32_t userId)
{
   AIOperatorInstance *instance = s_currentInstance;
   if (instance == nullptr)
      return std::string("Error: this function can only be used during AI operator execution");

   const char *title = json_object_get_string_utf8(arguments, "title", nullptr);
   if ((title == nullptr) || (*title == 0))
      return std::string("Error: title parameter is required");

   int severity;
   json_t *severityElement = json_object_get(arguments, "severity");
   if (json_is_string(severityElement))
   {
      const char *text = json_string_value(severityElement);
      if (!stricmp(text, "normal"))
         severity = SEVERITY_NORMAL;
      else if (!stricmp(text, "warning"))
         severity = SEVERITY_WARNING;
      else if (!stricmp(text, "minor"))
         severity = SEVERITY_MINOR;
      else if (!stricmp(text, "major"))
         severity = SEVERITY_MAJOR;
      else if (!stricmp(text, "critical"))
         severity = SEVERITY_CRITICAL;
      else
         return std::string("Error: invalid severity value");
   }
   else
   {
      severity = json_object_get_int32(arguments, "severity", SEVERITY_NORMAL);
      if ((severity < SEVERITY_NORMAL) || (severity > SEVERITY_CRITICAL))
         return std::string("Error: invalid severity value");
   }

   uint32_t objectId = 0;
   if (json_object_get(arguments, "object") != nullptr)
   {
      shared_ptr<NetObj> object = FindObjectByNameOrId(arguments, "object");
      if (object == nullptr)
         return std::string("Error: object not found");
      if (!object->checkAccessRights(userId, OBJECT_ACCESS_READ))
         return std::string("Error: access to object denied");
      objectId = object->getId();
   }

   const char *body = json_object_get_string_utf8(arguments, "body", nullptr);

   std::string refs;
   json_t *refsElement = json_object_get_array_ex(arguments, "references");
   if (refsElement != nullptr)
   {
      refs = JsonToString(refsElement);
   }

   int64_t observationId = RecordObservation(instance, severity, title, body, objectId, refs.empty() ? nullptr : refs.c_str());
   if (observationId < 0)
      return std::string("Error: cannot record observation");

   char buffer[64];
   return std::string("Observation recorded with ID ").append(IntegerToString(observationId, buffer));
}

/**
 * AI operator scheduler thread
 */
static void AIOperatorSchedulerThread()
{
   nxlog_debug_tag(DEBUG_TAG, 1, L"AI operator scheduler thread started");

   while(!SleepAndCheckForShutdown(30))
   {
      if (!ConfigReadBoolean(L"AIOperator.Enabled", true))
         continue;

      if (s_operatorUserId == INVALID_UID)
      {
         // Account could be created after server startup
         s_operatorUserId = ResolveUserName(AI_OPERATOR_ACCOUNT_NAME);
         if (s_operatorUserId == INVALID_UID)
            continue;
         nxlog_debug_tag(DEBUG_TAG, 2, L"AI operator account \"%s\" resolved to user ID %u", AI_OPERATOR_ACCOUNT_NAME, s_operatorUserId);
      }

      time_t now = time(nullptr);
      std::vector<shared_ptr<AIOperatorInstance>> instancesToExecute;

      // Mark selected instances as executing at enqueue time to prevent double dispatch
      s_instancesLock.lock();
      for(const shared_ptr<AIOperatorInstance>& instance : s_instances)
      {
         if (instance->isEnabled() && !instance->isExecuting() && (instance->getNextExecutionTime() <= now))
         {
            instance->setExecuting();
            instancesToExecute.push_back(instance);
         }
      }
      s_instancesLock.unlock();

      for(const shared_ptr<AIOperatorInstance>& instance : instancesToExecute)
      {
         if (g_flags & AF_SHUTDOWN)
            break;

         nxlog_debug_tag(DEBUG_TAG, 5, L"Executing AI operator instance [%u] \"%s\"", instance->getId(), instance->getName());
         ThreadPoolExecute(s_threadPool, instance, &AIOperatorInstance::execute);
      }
   }

   nxlog_debug_tag(DEBUG_TAG, 1, L"AI operator scheduler thread stopped");
}

/**
 * Print AI operator instances to server console
 */
void ShowAIOperators(ServerConsole *console)
{
   ConsolePrintf(console, L" %-6s | %-24s | %-8s | %-9s | %-20s | %-20s | %9s | %12s\n",
      L"ID", L"Name", L"Enabled", L"State", L"Last execution", L"Next execution", L"Iteration", L"Tokens today");
   ConsolePrintf(console, L"--------+--------------------------+----------+-----------+----------------------+----------------------+-----------+-------------\n");

   s_instancesLock.lock();
   s_instances.forEach(
      [console] (const uint32_t& key, const shared_ptr<AIOperatorInstance>& instance) -> EnumerationCallbackResult
      {
         ConsolePrintf(console, L" %-6u | %-24s | %-8s | %-9s | %-20s | %-20s | %9u | %12u\n",
            instance->getId(), instance->getName(),
            instance->isEnabled() ? L"yes" : L"no",
            instance->isExecuting() ? L"running" : L"idle",
            (instance->getLastExecutionTime() > 0) ? FormatTimestamp(instance->getLastExecutionTime()).cstr() : L"never",
            instance->isEnabled() ? FormatTimestamp(instance->getNextExecutionTime()).cstr() : L"never",
            instance->getIteration(),
            static_cast<uint32_t>(instance->getTokensUsedToday()));
         return _CONTINUE;
      });
   s_instancesLock.unlock();
   ConsolePrintf(console, L"\n");
}

/**
 * Initialize AI operator subsystem
 */
void InitAIOperators()
{
   s_instanceId = ConfigReadInt(L"AIOperator.LastInstanceId", 0);
   s_operatorUserId = ResolveUserName(AI_OPERATOR_ACCOUNT_NAME);
   if (s_operatorUserId == INVALID_UID)
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"AI operator account \"%s\" not found, AI operator instances will not be executed", AI_OPERATOR_ACCOUNT_NAME);

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_RESULT hResult = DBSelect(hdb, L"SELECT max(id) FROM ai_operator_observations");
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_observationId = DBGetFieldInt64(hResult, 0, 0);
      DBFreeResult(hResult);
   }

   hResult = DBSelect(hdb, L"SELECT max(record_id) FROM ai_operator_execution_log");
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_logRecordId = DBGetFieldInt64(hResult, 0, 0);
      DBFreeResult(hResult);
   }

   hResult = DBSelect(hdb,
      L"SELECT id,name,description,owner_user_id,enabled,scope_filter,model_slot,min_interval,max_interval,daily_token_budget,"
      L"tokens_used,usage_day,persona_prompt,current_focus,watch_list,memento,observation_retention_days,"
      L"observation_max_records,last_execution_time,next_execution_time,iteration,created,modified FROM ai_operator_instances ORDER BY id");
   if (hResult != nullptr)
   {
      uint32_t maxId = 0;
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         shared_ptr<AIOperatorInstance> instance = make_shared<AIOperatorInstance>(hResult, i);
         s_instances.set(instance->getId(), instance);
         if (instance->getId() > maxId)
            maxId = instance->getId();
      }
      DBFreeResult(hResult);

      if (maxId > static_cast<uint32_t>(s_instanceId))
         s_instanceId = maxId;
   }

   DBConnectionPoolReleaseConnection(hdb);

   s_systemPrompt =
      "You are an AI operator integrated with NetXMS, a network management system. "
      "You run as a perpetual adaptive monitoring loop: you are executed periodically, examine the monitored environment, "
      "record notable findings, and decide when to run next. You are never done - each execution is one iteration of an endless loop.\n\n"
      "AUTONOMOUS OPERATION:\n"
      "- Make decisions without user input or confirmation\n"
      "- Take appropriate actions based on available information\n"
      "- Never stop to ask for user input or confirmation\n\n"
      "MONITORING MODEL:\n"
      "- <iteration> tag indicates current iteration number starting from 1\n"
      "- <scope> tag (if present) describes the part of the infrastructure you are responsible for; stay within it\n"
      "- <current_focus> tag contains the focus you set on the previous iteration\n"
      "- <watch_list> tag contains the list of items you decided to watch\n"
      "- <memento> tag contains data preserved from previous iteration (if any)\n"
      "- <current_time> tag contains current server time in ISO 8601 format\n"
      "- <interval_bounds> tag contains allowed range for next execution delay in seconds\n"
      "- Use available functions to assess current operational status, alarms, events, and metrics\n"
      "- Load skills using load-skill function when you need specialized capabilities\n\n"
      "RECORDING FINDINGS:\n"
      "- Use record-observation to record any notable finding (anomaly, degradation, recovery, notable trend)\n"
      "- Observations form a persistent stream reviewed by human operators - record only meaningful findings\n"
      "- Do not re-record an observation you already recorded; track processed items in memento\n"
      "- For urgent or important situations additionally use create-ai-message with type 'alert' to notify users\n\n"
      "ADAPTIVE SCHEDULING:\n"
      "- Choose the next execution delay based on the situation: check more frequently when something is developing, "
      "back off when everything is stable\n"
      "- The delay is clamped to the range given in <interval_bounds>\n\n"
      "TIMESTAMP FORMATS:\n"
      "All functions accepting timestamp arguments support the following formats:\n"
      "- Relative: [+|-]<number>[s|m|h|d] where s=seconds, m=minutes, h=hours, d=days\n"
      "- UNIX timestamp: numeric value representing seconds since epoch\n"
      "- ISO 8601: YYYY-MM-DDTHH:MM:SSZ in UTC\n\n"
      "RESPONSE FORMAT:\n"
      "Provide structured iteration report in JSON format. Use only clean JSON without any additional text or markdown.\n"
      "Fields:\n"
      "- 'next_execution_time': delay in seconds until next execution (required)\n"
      "- 'current_focus': short summary of what you will focus on next (optional, replaces current)\n"
      "- 'watch_list': updated list of items you are watching (optional, string or array, replaces current)\n"
      "- 'memento': data to preserve until next iteration (optional, string or object; omit to keep previous value)\n"
      "- 'explanation': brief summary of this iteration and reasoning behind your decisions (required)\n";
   ENUMERATE_MODULES(pfGetAIAgentInstructions)
   {
      s_systemPrompt.append(CURRENT_MODULE.pfGetAIAgentInstructions());
   }

   RegisterAIAssistantFunction(
      "record-observation",
      "Record a monitoring observation (available only during AI operator execution). "
      "Use this to record notable findings: anomalies, degradations, recoveries, notable trends. "
      "Observations are stored persistently and reviewed by human operators.",
      {
         { "title", "Short title of the observation (max 255 chars)" },
         { "severity", "Severity: 'normal', 'warning', 'minor', 'major', or 'critical'" },
         { "body", "Optional: detailed description of the observation" },
         { "object", "Optional: name or ID of the NetXMS object this observation relates to" },
         { "references", "Optional: array of references supporting the observation (alarm IDs, event IDs, DCI IDs, etc.)" }
      },
      F_RecordObservation);

   s_threadPool = ThreadPoolCreate(AI_OPERATOR_COMPONENT,
      ConfigReadInt(L"ThreadPool.AIOperator.BaseSize", 4),
      ConfigReadInt(L"ThreadPool.AIOperator.MaxSize", 16));
   ThreadCreate(AIOperatorSchedulerThread);

   nxlog_debug_tag(DEBUG_TAG, 2, L"AI operator subsystem initialized (%d instances)", s_instances.size());
}
