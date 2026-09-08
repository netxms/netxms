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
static VolatileCounter s_checkId = 0;       // Last used standing check ID
static VolatileCounter64 s_observationId = 0;  // Last used observation ID
static VolatileCounter64 s_logRecordId = 0;    // Last used execution log record ID
static VolatileCounter64 s_instructionsHistoryId = 0;   // Last used instructions history record ID
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
   m_instructionsLocked = false;
   m_lastExecutionTime = 0;
   m_nextExecutionTime = 0;
   m_iteration = 0;
   m_creationTime = m_modificationTime = time(nullptr);
   m_executing = false;
   m_consecutiveFailures = 0;
   m_interruptPending = false;
}

/**
 * Create AI operator instance from database row. Expected column order:
 * id,name,description,owner_user_id,enabled,scope_filter,model_slot,min_interval,max_interval,daily_token_budget,
 * tokens_used,usage_day,persona_prompt,current_focus,watch_list,memento,observation_retention_days,
 * observation_max_records,instructions,instructions_locked,last_execution_time,next_execution_time,iteration,created,modified
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
   text = DBGetFieldUTF8(hResult, row, 18, nullptr, 0);
   m_instructions = CHECK_NULL_EX_A(text);
   MemFree(text);
   DBGetField(hResult, row, 19, flag, 2);
   m_instructionsLocked = (flag[0] == '1');
   m_lastExecutionTime = DBGetFieldUInt32(hResult, row, 20);
   m_nextExecutionTime = DBGetFieldUInt32(hResult, row, 21);
   m_iteration = DBGetFieldUInt32(hResult, row, 22);
   m_creationTime = DBGetFieldUInt32(hResult, row, 23);
   m_modificationTime = DBGetFieldUInt32(hResult, row, 24);
   m_executing = false;
   m_consecutiveFailures = 0;
   m_interruptPending = false;
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
      L"watch_list", L"memento", L"observation_retention_days", L"observation_max_records", L"instructions",
      L"instructions_locked", L"last_execution_time", L"next_execution_time", L"iteration", L"created", L"modified", nullptr
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
      DBBind(hStmt, 18, DB_SQLTYPE_TEXT, DB_CTYPE_UTF8_STRING, m_instructions.c_str(), DB_BIND_STATIC);
      DBBind(hStmt, 19, DB_SQLTYPE_VARCHAR, m_instructionsLocked ? L"1" : L"0", DB_BIND_STATIC);
      DBBind(hStmt, 20, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_lastExecutionTime));
      DBBind(hStmt, 21, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_nextExecutionTime));
      DBBind(hStmt, 22, DB_SQLTYPE_INTEGER, m_iteration);
      DBBind(hStmt, 23, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_creationTime));
      DBBind(hStmt, 24, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_modificationTime));
      DBBind(hStmt, 25, DB_SQLTYPE_INTEGER, m_id);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }

   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Delete AI operator instance, its observations, standing checks, and instructions history from database
 */
void AIOperatorInstance::deleteFromDatabase()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   ExecuteQueryOnObject(hdb, m_id, L"DELETE FROM ai_operator_instances WHERE id=?");
   ExecuteQueryOnObject(hdb, m_id, L"DELETE FROM ai_operator_observations WHERE instance_id=?");
   ExecuteQueryOnObject(hdb, m_id, L"DELETE FROM ai_operator_checks WHERE instance_id=?");
   ExecuteQueryOnObject(hdb, m_id, L"DELETE FROM ai_operator_instr_history WHERE instance_id=?");
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Log AI operator execution to database. Must be called with instance lock held.
 */
void AIOperatorInstance::logExecution(wchar_t status, uint32_t durationMs, int64_t inputTokens, int64_t outputTokens, const wchar_t *explanation)
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
      DBBind(hStmt, 10, DB_SQLTYPE_TEXT, explanation, DB_BIND_STATIC);
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
   // Models may wrap the JSON object into a markdown code fence or prepend prose despite instructions,
   // so parse from the first '{' and ignore trailing text
   const char *jsonStart = strchr(response, '{');
   json_t *json = (jsonStart != nullptr) ? json_loads(jsonStart, JSON_DISABLE_EOF_CHECK, nullptr) : nullptr;
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

   // Standing instructions: omitted = keep, empty string = clear, string = replace
   json_t *instructions = json_object_get(json, "instructions");
   if (json_is_string(instructions))
   {
      if (m_instructionsLocked)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, L"AI operator [%u] \"%s\" returned standing instructions but they are locked, update ignored", m_id, m_name);
         m_lastExplanation.append(L"\n[standing instructions update ignored: locked by administrator]");
      }
      else
      {
         size_t truncatedTo = setInstructions(json_string_value(instructions), now);
         if (truncatedTo > 0)
            m_lastExplanation.appendFormattedString(L"\n[standing instructions truncated to %u characters]", static_cast<uint32_t>(truncatedTo));
      }
   }

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

   // Interrupts raised by standing checks are consumed by this iteration; if the iteration cannot run
   // they are dropped rather than queued indefinitely
   std::vector<AIOperatorInterrupt> interrupts;
   interrupts.swap(m_pendingInterrupts);
   m_interruptPending = false;

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
      if (!interrupts.empty())
         nxlog_debug_tag(DEBUG_TAG, 4, L"AI operator [%u] \"%s\" dropped %d pending check interrupt(s) because daily token budget is exhausted",
            m_id, m_name, static_cast<int>(interrupts.size()));
      logExecution('S', 0, 0, 0, m_lastExplanation);
      saveToDatabase();
      m_mutex.unlock();
      clearExecutingState();
      return;
   }

   if ((GetEffectiveSystemRights(s_operatorUserId) & SYSTEM_ACCESS_USE_AI_ASSISTANT) == 0)
   {
      handleFailure("AI operator account does not have AI assistant access right", now);
      logExecution('F', 0, 0, 0, m_lastExplanation);
      saveToDatabase();
      m_mutex.unlock();
      clearExecutingState();
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
   if (!m_instructions.empty())
   {
      systemPrompt.append("\n\nSTANDING INSTRUCTIONS (written by you on earlier iterations; persona and system rules above take precedence):\n");
      systemPrompt.append(m_instructions);
   }

   std::string prompt("Perform one monitoring iteration now according to your instructions.\n<iteration>");
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
   appendChecksToPrompt(prompt);
   if (!interrupts.empty())
   {
      json_t *list = json_array();
      for(const AIOperatorInterrupt& interrupt : interrupts)
      {
         json_t *element = json_object();
         json_object_set_new(element, "id", json_integer(interrupt.checkId));
         json_object_set_new(element, "name", json_string(interrupt.checkName.c_str()));
         json_object_set_new(element, "fired_at", json_string(FormatISO8601Timestamp(interrupt.timestamp).c_str()));
         json_t *payload = json_loads(interrupt.payload.c_str(), 0, nullptr);
         json_object_set_new(element, "payload", (payload != nullptr) ? payload : json_string(interrupt.payload.c_str()));
         json_array_append_new(list, element);
      }
      char *text = json_dumps(list, JSON_COMPACT);
      prompt.append("\n<triggered_checks>");
      prompt.append(CHECK_NULL_EX_A(text));
      prompt.append("</triggered_checks>");
      MemFree(text);
      json_decref(list);
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

   logExecution(success ? 'C' : 'F', durationMs, tokenUsage.inputTokens, tokenUsage.outputTokens, m_lastExplanation);
   m_mutex.unlock();

   // Do not re-create database record if instance was deleted while executing.
   // Lock order is always instance list lock -> AIOperatorInstance::m_mutex, never the reverse.
   s_instancesLock.lock();
   bool registered = s_instances.contains(m_id);
   s_instancesLock.unlock();

   if (registered)
   {
      LockGuard lockGuard(m_mutex);
      saveToDatabase();
   }

   // Executing flag is cleared only after database update is complete, so that
   // scheduler cannot dispatch this instance again while its state is being saved
   clearExecutingState();
}

/**
 * Clear executing flag. Flag is protected by instance list lock, so that setting it at dispatch time
 * and clearing it here are serialized with scheduler's check. Instance can be deleted after execution
 * checked registration but before database record was written, so registration is checked again here
 * to remove record re-created by such execution.
 */
void AIOperatorInstance::clearExecutingState()
{
   s_instancesLock.lock();
   m_executing = false;
   bool registered = s_instances.contains(m_id);
   s_instancesLock.unlock();

   if (!registered)
      deleteFromDatabase();
}

/**
 * Modify AI operator instance from JSON configuration
 */
uint32_t AIOperatorInstance::modifyFromJSON(json_t *config)
{
   LockGuard lockGuard(m_mutex);

   // Validate and stage every field into locals first, then commit only after all checks pass,
   // so a rejected request never leaves the live instance partially modified.
   bool updateName = false;
   wchar_t name[64];
   json_t *jname = json_object_get(config, "name");
   if (jname != nullptr)
   {
      if (!json_is_string(jname) || (*json_string_value(jname) == 0))
         return RCC_INVALID_ARGUMENT;
      utf8_to_wchar(json_string_value(jname), -1, name, 64);
      name[63] = 0;
      updateName = true;
   }

   json_t *description = json_object_get(config, "description");
   if ((description != nullptr) && !json_is_string(description) && !json_is_null(description))
      return RCC_INVALID_ARGUMENT;

   json_t *scopeFilter = json_object_get(config, "scopeFilter");
   if ((scopeFilter != nullptr) && !json_is_string(scopeFilter) && !json_is_null(scopeFilter))
      return RCC_INVALID_ARGUMENT;

   json_t *personaPrompt = json_object_get(config, "personaPrompt");
   if ((personaPrompt != nullptr) && !json_is_string(personaPrompt) && !json_is_null(personaPrompt))
      return RCC_INVALID_ARGUMENT;

   json_t *instructions = json_object_get(config, "instructions");
   if ((instructions != nullptr) && !json_is_string(instructions) && !json_is_null(instructions))
      return RCC_INVALID_ARGUMENT;

   bool instructionsLocked = m_instructionsLocked;
   if (!json_object_update_boolean(config, "instructionsLocked", &instructionsLocked))
      return RCC_INVALID_ARGUMENT;

   char modelSlot[64];
   memcpy(modelSlot, m_modelSlot, sizeof(modelSlot));
   uint32_t minInterval = m_minInterval;
   uint32_t maxInterval = m_maxInterval;
   uint32_t dailyTokenBudget = m_dailyTokenBudget;
   uint32_t observationRetentionDays = m_observationRetentionDays;
   uint32_t observationMaxRecords = m_observationMaxRecords;
   if (!json_object_update_string_utf8(config, "modelSlot", modelSlot, 64) ||
       !json_object_update_integer(config, "minInterval", &minInterval) ||
       !json_object_update_integer(config, "maxInterval", &maxInterval) ||
       !json_object_update_integer(config, "dailyTokenBudget", &dailyTokenBudget) ||
       !json_object_update_integer(config, "observationRetentionDays", &observationRetentionDays) ||
       !json_object_update_integer(config, "observationMaxRecords", &observationMaxRecords))
      return RCC_INVALID_ARGUMENT;

   if (minInterval < 60)
      minInterval = 60;
   if (maxInterval < minInterval)
      return RCC_INVALID_ARGUMENT;

   bool enabled = m_enabled;
   if (!json_object_update_boolean(config, "enabled", &enabled))
      return RCC_INVALID_ARGUMENT;

   // All checks passed - commit staged values to the live instance
   if (updateName)
      wcscpy(m_name, name);
   if (description != nullptr)
      m_description = String(json_is_string(description) ? json_string_value(description) : "", "utf8");
   if (scopeFilter != nullptr)
      m_scopeFilter = json_is_string(scopeFilter) ? json_string_value(scopeFilter) : "";
   if (personaPrompt != nullptr)
      m_personaPrompt = json_is_string(personaPrompt) ? json_string_value(personaPrompt) : "";
   m_instructionsLocked = instructionsLocked;
   if (instructions != nullptr)
      setInstructions(json_is_string(instructions) ? json_string_value(instructions) : "", time(nullptr));
   memcpy(m_modelSlot, modelSlot, sizeof(m_modelSlot));
   m_minInterval = minInterval;
   m_maxInterval = maxInterval;
   m_dailyTokenBudget = dailyTokenBudget;
   m_observationRetentionDays = observationRetentionDays;
   m_observationMaxRecords = observationMaxRecords;

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
 * Reset accumulated state (memento, focus, watch list, standing instructions, iteration counter).
 * Standing checks are deliberately left intact - they are delegated work, not state.
 */
void AIOperatorInstance::resetMemento()
{
   LockGuard lockGuard(m_mutex);
   setInstructions("", time(nullptr));
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
   json_object_set_new(json, "instructions", json_string(m_instructions.c_str()));
   json_object_set_new(json, "instructionsLocked", json_boolean(m_instructionsLocked));
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
 * Fill NXCP message with AI operator instance data
 */
void AIOperatorInstance::fillMessage(NXCPMessage *msg, uint32_t baseId) const
{
   LockGuard lockGuard(m_mutex);
   msg->setField(baseId, m_id);
   msg->setField(baseId + 1, m_name);
   msg->setField(baseId + 2, m_description);
   msg->setField(baseId + 3, m_ownerUserId);
   msg->setField(baseId + 4, m_enabled);
   msg->setField(baseId + 5, m_executing);
   msg->setFieldFromUtf8String(baseId + 6, m_scopeFilter.c_str());
   msg->setFieldFromUtf8String(baseId + 7, m_modelSlot);
   msg->setField(baseId + 8, m_minInterval);
   msg->setField(baseId + 9, m_maxInterval);
   msg->setField(baseId + 10, m_dailyTokenBudget);
   msg->setField(baseId + 11, m_tokensUsed);
   msg->setFieldFromUtf8String(baseId + 12, m_personaPrompt.c_str());
   msg->setField(baseId + 13, m_currentFocus);
   msg->setFieldFromUtf8String(baseId + 14, m_watchList.c_str());
   msg->setFieldFromUtf8String(baseId + 15, m_memento.c_str());
   msg->setField(baseId + 16, m_observationRetentionDays);
   msg->setField(baseId + 17, m_observationMaxRecords);
   msg->setFieldFromTime(baseId + 18, m_lastExecutionTime);
   msg->setFieldFromTime(baseId + 19, m_nextExecutionTime);
   msg->setField(baseId + 20, m_iteration);
   msg->setField(baseId + 21, static_cast<int32_t>(m_consecutiveFailures));
   msg->setField(baseId + 22, m_lastExplanation);
   msg->setFieldFromTime(baseId + 23, m_creationTime);
   msg->setFieldFromTime(baseId + 24, m_modificationTime);
   msg->setFieldFromUtf8String(baseId + 25, m_instructions.c_str());
   msg->setField(baseId + 26, m_instructionsLocked);
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
 * Get all AI operator instances
 */
unique_ptr<SharedObjectArray<AIOperatorInstance>> NXCORE_EXPORTABLE GetAIOperatorInstances()
{
   auto instances = make_unique<SharedObjectArray<AIOperatorInstance>>();
   s_instancesLock.lock();
   s_instances.forEach(
      [&instances] (const uint32_t& key, const shared_ptr<AIOperatorInstance>& instance) -> EnumerationCallbackResult
      {
         instances->add(instance);
         return _CONTINUE;
      });
   s_instancesLock.unlock();
   return instances;
}

/**
 * Get all AI operator instances as JSON array
 */
json_t NXCORE_EXPORTABLE *GetAIOperatorInstancesAsJson()
{
   // Collect instance references under list lock and release it before serialization,
   // because AIOperatorInstance::toJson acquires instance lock which can be held across database update
   unique_ptr<SharedObjectArray<AIOperatorInstance>> instances = GetAIOperatorInstances();

   json_t *output = json_array();
   for(int i = 0; i < instances->size(); i++)
      json_array_append_new(output, instances->get(i)->toJson());
   return output;
}

/**
 * Fill NXCP message with all AI operator instances
 */
void FillAIOperatorListMessage(NXCPMessage *msg)
{
   // Collect instance references under list lock and release it before reading instance attributes,
   // because AIOperatorInstance::fillMessage acquires instance lock which can be held across database update
   unique_ptr<SharedObjectArray<AIOperatorInstance>> instances = GetAIOperatorInstances();

   uint32_t fieldId = VID_ELEMENT_LIST_BASE;
   for(int i = 0; i < instances->size(); i++)
   {
      instances->get(i)->fillMessage(msg, fieldId);
      fieldId += 30;
   }
   msg->setField(VID_NUM_ELEMENTS, static_cast<uint32_t>(instances->size()));
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
 * Update AI operator observation state (acknowledge/dismiss). Changing an observation's state is a write
 * operation, so it is gated on access to the observation's source object (OBJECT_ACCESS_UPDATE_ALARMS,
 * same right used for acknowledging alarms). Server-level observations (object_id = 0) require the AI
 * operator management right instead.
 */
uint32_t NXCORE_EXPORTABLE UpdateAIOperatorObservationState(int64_t observationId, AIObservationState state, uint32_t userId, uint32_t *sourceObjectId)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   // Resolve the observation's source object for the access control check
   uint32_t objectId = 0;
   uint32_t rcc = RCC_DB_FAILURE;
   DB_STATEMENT hStmt = DBPrepare(hdb, L"SELECT object_id FROM ai_operator_observations WHERE id=?");
   if (hStmt == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return RCC_DB_FAILURE;
   }
   DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, observationId);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult == nullptr)
   {
      DBFreeStatement(hStmt);
      DBConnectionPoolReleaseConnection(hdb);
      return RCC_DB_FAILURE;
   }
   bool found = (DBGetNumRows(hResult) > 0);
   if (found)
      objectId = DBGetFieldUInt32(hResult, 0, 0);
   DBFreeResult(hResult);
   DBFreeStatement(hStmt);

   if (!found)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return RCC_NO_SUCH_RECORD;
   }

   if (sourceObjectId != nullptr)
      *sourceObjectId = objectId;

   bool authorized;
   if (objectId != 0)
   {
      shared_ptr<NetObj> object = FindObjectById(objectId);
      authorized = (object != nullptr) && object->checkAccessRights(userId, OBJECT_ACCESS_UPDATE_ALARMS);
   }
   else
   {
      authorized = (GetEffectiveSystemRights(userId) & SYSTEM_ACCESS_MANAGE_AI_OPERATORS) != 0;
   }
   if (!authorized)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return RCC_ACCESS_DENIED;
   }

   hStmt = DBPrepare(hdb, L"UPDATE ai_operator_observations SET state=? WHERE id=?");
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
 * Set error text if output pointer is provided
 */
static inline void SetErrorText(MutableString *errorText, const wchar_t *text)
{
   if (errorText != nullptr)
      *errorText = text;
}

/**
 * Convert verdict to database/protocol character
 */
static inline wchar_t VerdictToChar(AICheckVerdict verdict)
{
   switch(verdict)
   {
      case AICheckVerdict::QUIET:
         return L'Q';
      case AICheckVerdict::FIRED:
         return L'F';
      case AICheckVerdict::FAILED:
         return L'E';
      default:
         return L'N';
   }
}

/**
 * Convert verdict to symbolic name
 */
static inline const char *VerdictToName(AICheckVerdict verdict)
{
   switch(verdict)
   {
      case AICheckVerdict::QUIET:
         return "quiet";
      case AICheckVerdict::FIRED:
         return "fired";
      case AICheckVerdict::FAILED:
         return "error";
      default:
         return "never";
   }
}

/**
 * Serialize fired check result as JSON payload
 */
static std::string CheckResultToJson(const AICheckResult& result)
{
   json_t *json = json_object();
   if (result.verdict == AICheckVerdict::FAILED)
   {
      json_object_set_new(json, "error", json_string(result.error.c_str()));
   }
   else
   {
      json_object_set_new(json, "title", json_string(result.title.c_str()));
      json_object_set_new(json, "severity", json_integer(result.severity));
      if (!result.details.empty())
         json_object_set_new(json, "details", json_string(result.details.c_str()));
   }
   return JsonToString(json);   // JsonToString releases the object
}

/**
 * Compile standing check source. Returns nullptr and sets error text on failure.
 */
static NXSL_Program *CompileCheckSource(const char *source, MutableString *errorText)
{
   NXSL_ServerEnv env;
   NXSL_CompilationDiagnostic diag;
   NXSL_Program *program = NXSLCompile(source, &env, &diag);
   if ((program == nullptr) && (errorText != nullptr))
      *errorText = diag.errorText;
   return program;
}

/**
 * Token shared between a running check script and its watchdog. The watchdog stops the VM only while
 * the run is still in progress; the runner clears the VM pointer before destroying it.
 */
struct AICheckRunToken
{
   Mutex mutex;
   NXSL_VM *vm;

   AICheckRunToken(NXSL_VM *_vm) : vm(_vm)
   {
   }
};

/**
 * Watchdog: stop check script that exceeded execution time limit
 */
static void AbortCheckRun(const shared_ptr<AICheckRunToken>& token)
{
   LockGuard lockGuard(token->mutex);
   if (token->vm != nullptr)
      token->vm->stop();
}

/**
 * Execute compiled standing check script under the AI operator account's security context.
 * Must not be called with instance lock held.
 */
static AICheckResult ExecuteCheckScript(const shared_ptr<NXSL_Program>& program, uint32_t objectId, const wchar_t *checkName, uint32_t timeLimit)
{
   AICheckResult result;

   shared_ptr<NetObj> object;
   if (objectId != 0)
   {
      object = FindObjectById(objectId);
      if (object == nullptr)
      {
         result.verdict = AICheckVerdict::FAILED;
         char buffer[64];
         result.error = std::string("bound object [").append(IntegerToString(objectId, buffer)).append("] does not exist");
         return result;
      }
   }

   NXSL_VM *vm = new NXSL_VM(new NXSL_ServerEnv());
   if (!vm->load(program.get()))
   {
      result.verdict = AICheckVerdict::FAILED;
      char *error = UTF8StringFromWideString(vm->getErrorText());
      result.error = std::string("cannot load script: ").append(error);
      MemFree(error);
      delete vm;
      return result;
   }

   if (object != nullptr)
      SetupServerScriptVM(vm, object, shared_ptr<DCObjectInfo>());
   else
      vm->setGlobalVariable("$object", vm->createValue());
   vm->setSecurityContext(new NXSL_UserSecurityContext(s_operatorUserId));

   auto token = make_shared<AICheckRunToken>(vm);
   if (timeLimit > 0)
      ThreadPoolScheduleRelative(s_threadPool, timeLimit * 1000, AbortCheckRun, token);

   bool success = vm->run();

   token->mutex.lock();
   token->vm = nullptr;
   token->mutex.unlock();

   if (success)
   {
      result = EvaluateAICheckResult(vm->getResult(), checkName);
   }
   else
   {
      result.verdict = AICheckVerdict::FAILED;
      if (vm->getErrorCode() == NXSL_ERR_EXECUTION_ABORTED)
      {
         result.error = "execution time limit exceeded";
      }
      else
      {
         char *error = UTF8StringFromWideString(vm->getErrorText());
         result.error = error;
         MemFree(error);
      }
   }
   delete vm;
   return result;
}

/**
 * Create new standing check
 */
AIOperatorCheck::AIOperatorCheck(uint32_t instanceId, bool createdByModel)
{
   m_id = InterlockedIncrement(&s_checkId);
   m_instanceId = instanceId;
   m_name[0] = 0;
   m_enabled = true;
   m_locked = false;
   m_createdByModel = createdByModel;
   m_interval = 300;
   m_objectId = 0;
   m_action = AICheckAction::WAKE;
   m_cooldown = 0;
   m_renotifyInterval = 0;
   m_lastRun = 0;
   m_lastVerdict = AICheckVerdict::NONE;
   m_lastFire = 0;
   m_consecutiveErrors = 0;
   m_runCount = 0;
   m_creationTime = m_modificationTime = time(nullptr);
   m_running = false;
}

/**
 * Create standing check from database row. Expected column order:
 * id,instance_id,name,description,enabled,locked,created_by,source,check_interval,object_id,check_action,cooldown,
 * renotify_interval,last_run,last_verdict,last_fire,last_payload,consecutive_errors,run_count,created,modified
 */
AIOperatorCheck::AIOperatorCheck(DB_RESULT hResult, int row)
{
   m_id = DBGetFieldUInt32(hResult, row, 0);
   m_instanceId = DBGetFieldUInt32(hResult, row, 1);
   DBGetField(hResult, row, 2, m_name, 64);
   m_description = DBGetFieldAsString(hResult, row, 3);
   wchar_t flag[2];
   DBGetField(hResult, row, 4, flag, 2);
   m_enabled = (flag[0] == '1');
   DBGetField(hResult, row, 5, flag, 2);
   m_locked = (flag[0] == '1');
   DBGetField(hResult, row, 6, flag, 2);
   m_createdByModel = (flag[0] == 'M');
   char *text = DBGetFieldUTF8(hResult, row, 7, nullptr, 0);
   m_source = CHECK_NULL_EX_A(text);
   MemFree(text);
   m_interval = DBGetFieldUInt32(hResult, row, 8);
   m_objectId = DBGetFieldUInt32(hResult, row, 9);
   DBGetField(hResult, row, 10, flag, 2);
   m_action = (flag[0] == 'O') ? AICheckAction::OBSERVE : AICheckAction::WAKE;
   m_cooldown = DBGetFieldUInt32(hResult, row, 11);
   m_renotifyInterval = DBGetFieldUInt32(hResult, row, 12);
   m_lastRun = DBGetFieldUInt32(hResult, row, 13);
   DBGetField(hResult, row, 14, flag, 2);
   switch(flag[0])
   {
      case 'Q':
         m_lastVerdict = AICheckVerdict::QUIET;
         break;
      case 'F':
         m_lastVerdict = AICheckVerdict::FIRED;
         break;
      case 'E':
         m_lastVerdict = AICheckVerdict::FAILED;
         break;
      default:
         m_lastVerdict = AICheckVerdict::NONE;
         break;
   }
   m_lastFire = DBGetFieldUInt32(hResult, row, 15);
   text = DBGetFieldUTF8(hResult, row, 16, nullptr, 0);
   m_lastPayload = CHECK_NULL_EX_A(text);
   MemFree(text);
   m_consecutiveErrors = DBGetFieldUInt32(hResult, row, 17);
   m_runCount = DBGetFieldUInt32(hResult, row, 18);
   m_creationTime = DBGetFieldUInt32(hResult, row, 19);
   m_modificationTime = DBGetFieldUInt32(hResult, row, 20);
   m_running = false;

   m_program = shared_ptr<NXSL_Program>(CompileCheckSource(m_source.c_str(), &m_compileError));
}

/**
 * Save standing check to database. Must be called with instance lock held.
 */
void AIOperatorCheck::saveToDatabase() const
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   static const wchar_t *mergeColumns[] = {
      L"instance_id", L"name", L"description", L"enabled", L"locked", L"created_by", L"source", L"check_interval",
      L"object_id", L"check_action", L"cooldown", L"renotify_interval", L"last_run", L"last_verdict", L"last_fire",
      L"last_payload", L"consecutive_errors", L"run_count", L"created", L"modified", nullptr
   };
   DB_STATEMENT hStmt = DBPrepareMerge(hdb, L"ai_operator_checks", L"id", m_id, mergeColumns);
   if (hStmt != nullptr)
   {
      wchar_t verdict[2] = { VerdictToChar(m_lastVerdict), 0 };
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_instanceId);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC, 63);
      DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_description, DB_BIND_STATIC, 255);
      DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_enabled ? L"1" : L"0", DB_BIND_STATIC);
      DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, m_locked ? L"1" : L"0", DB_BIND_STATIC);
      DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, m_createdByModel ? L"M" : L"H", DB_BIND_STATIC);
      DBBind(hStmt, 7, DB_SQLTYPE_TEXT, DB_CTYPE_UTF8_STRING, m_source.c_str(), DB_BIND_STATIC);
      DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_interval);
      DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, m_objectId);
      DBBind(hStmt, 10, DB_SQLTYPE_VARCHAR, (m_action == AICheckAction::OBSERVE) ? L"O" : L"W", DB_BIND_STATIC);
      DBBind(hStmt, 11, DB_SQLTYPE_INTEGER, m_cooldown);
      DBBind(hStmt, 12, DB_SQLTYPE_INTEGER, m_renotifyInterval);
      DBBind(hStmt, 13, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_lastRun));
      DBBind(hStmt, 14, DB_SQLTYPE_VARCHAR, verdict, DB_BIND_STATIC);
      DBBind(hStmt, 15, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_lastFire));
      DBBind(hStmt, 16, DB_SQLTYPE_TEXT, DB_CTYPE_UTF8_STRING, m_lastPayload.c_str(), DB_BIND_STATIC);
      DBBind(hStmt, 17, DB_SQLTYPE_INTEGER, m_consecutiveErrors);
      DBBind(hStmt, 18, DB_SQLTYPE_INTEGER, m_runCount);
      DBBind(hStmt, 19, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_creationTime));
      DBBind(hStmt, 20, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_modificationTime));
      DBBind(hStmt, 21, DB_SQLTYPE_INTEGER, m_id);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }

   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Delete standing check from database
 */
void AIOperatorCheck::deleteFromDatabase()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   ExecuteQueryOnObject(hdb, m_id, L"DELETE FROM ai_operator_checks WHERE id=?");
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Record outcome of a run. Returns true if persistent state changed and the check should be saved.
 */
bool AIOperatorCheck::recordRun(time_t now, const AICheckResult& result, AICheckTransition transition)
{
   m_lastRun = now;
   m_runCount++;

   switch(transition)
   {
      case AICheckTransition::FIRE_EDGE:
      case AICheckTransition::FIRE_RENOTIFY:
         m_lastVerdict = AICheckVerdict::FIRED;
         m_lastFire = now;
         m_lastPayload = CheckResultToJson(result);
         m_consecutiveErrors = 0;
         return true;
      case AICheckTransition::CLEAR:
         m_lastVerdict = AICheckVerdict::QUIET;
         m_consecutiveErrors = 0;
         return true;
      case AICheckTransition::FAILURE:
         m_lastVerdict = AICheckVerdict::FAILED;
         m_lastPayload = CheckResultToJson(result);
         m_consecutiveErrors++;
         return true;
      case AICheckTransition::SUPPRESSED:
         // Action deferred until cooldown expires; previous verdict kept so the next run is still an edge
         m_consecutiveErrors = 0;
         return false;
      default:
         {
            bool changed = (m_lastVerdict != result.verdict) || (m_consecutiveErrors != 0);
            m_lastVerdict = result.verdict;
            m_consecutiveErrors = 0;
            return changed;
         }
   }
}

/**
 * Modify standing check from JSON configuration. Compiles the source before committing any change,
 * so a rejected request never leaves the check partially modified.
 */
uint32_t AIOperatorCheck::modifyFromJSON(json_t *config, bool byModel, MutableString *errorText)
{
   if (byModel && m_locked)
   {
      SetErrorText(errorText, L"check is locked");
      return RCC_ACCESS_DENIED;
   }

   bool updateName = false;
   wchar_t name[64];
   json_t *jname = json_object_get(config, "name");
   if (jname != nullptr)
   {
      if (!json_is_string(jname) || (*json_string_value(jname) == 0))
      {
         SetErrorText(errorText, L"name must be a non-empty string");
         return RCC_INVALID_ARGUMENT;
      }
      utf8_to_wchar(json_string_value(jname), -1, name, 64);
      name[63] = 0;
      updateName = true;
   }

   json_t *description = json_object_get(config, "description");
   if ((description != nullptr) && !json_is_string(description) && !json_is_null(description))
   {
      SetErrorText(errorText, L"description must be a string");
      return RCC_INVALID_ARGUMENT;
   }

   const char *source = nullptr;
   json_t *jsource = json_object_get(config, "source");
   if (jsource != nullptr)
   {
      if (!json_is_string(jsource) || (*json_string_value(jsource) == 0))
      {
         SetErrorText(errorText, L"source must be a non-empty string");
         return RCC_INVALID_ARGUMENT;
      }
      if (m_source.compare(json_string_value(jsource)) != 0)
         source = json_string_value(jsource);
   }

   uint32_t interval = m_interval;
   uint32_t objectId = m_objectId;
   uint32_t cooldown = m_cooldown;
   uint32_t renotifyInterval = m_renotifyInterval;
   if (!json_object_update_integer(config, "interval", &interval) ||
       !json_object_update_integer(config, "objectId", &objectId) ||
       !json_object_update_integer(config, "cooldown", &cooldown) ||
       !json_object_update_integer(config, "renotifyInterval", &renotifyInterval))
   {
      SetErrorText(errorText, L"interval, objectId, cooldown, and renotifyInterval must be integers");
      return RCC_INVALID_ARGUMENT;
   }
   if (interval < 30)
      interval = 30;

   AICheckAction action = m_action;
   json_t *jaction = json_object_get(config, "action");
   if (json_is_string(jaction))
   {
      const char *text = json_string_value(jaction);
      if (!stricmp(text, "wake"))
         action = AICheckAction::WAKE;
      else if (!stricmp(text, "observe"))
         action = AICheckAction::OBSERVE;
      else
      {
         SetErrorText(errorText, L"action must be 'wake' or 'observe'");
         return RCC_INVALID_ARGUMENT;
      }
   }
   else if (json_is_integer(jaction))
   {
      json_int_t value = json_integer_value(jaction);
      if ((value < 0) || (value > 1))
      {
         SetErrorText(errorText, L"action must be 'wake' or 'observe'");
         return RCC_INVALID_ARGUMENT;
      }
      action = static_cast<AICheckAction>(value);
   }
   else if (jaction != nullptr)
   {
      SetErrorText(errorText, L"action must be 'wake' or 'observe'");
      return RCC_INVALID_ARGUMENT;
   }

   bool enabled = m_enabled;
   if (!json_object_update_boolean(config, "enabled", &enabled))
   {
      SetErrorText(errorText, L"enabled must be a boolean");
      return RCC_INVALID_ARGUMENT;
   }

   // Lock can only be changed by a human
   bool locked = m_locked;
   if (!byModel && !json_object_update_boolean(config, "locked", &locked))
   {
      SetErrorText(errorText, L"locked must be a boolean");
      return RCC_INVALID_ARGUMENT;
   }

   // Bound object must exist and be readable by the AI operator account, so that the script sees
   // exactly what the operator's tool calls see
   if ((objectId != 0) && (objectId != m_objectId))
   {
      shared_ptr<NetObj> object = FindObjectById(objectId);
      if (object == nullptr)
      {
         SetErrorText(errorText, L"object does not exist");
         return RCC_INVALID_ARGUMENT;
      }
      if ((s_operatorUserId != INVALID_UID) && !object->checkAccessRights(s_operatorUserId, OBJECT_ACCESS_READ))
      {
         SetErrorText(errorText, L"AI operator account has no read access to object");
         return RCC_INVALID_ARGUMENT;
      }
   }

   NXSL_Program *program = nullptr;
   if (source != nullptr)
   {
      program = CompileCheckSource(source, errorText);
      if (program == nullptr)
         return RCC_NXSL_COMPILATION_ERROR;
   }

   // All checks passed - commit staged values
   if (updateName)
      wcscpy(m_name, name);
   if (description != nullptr)
      m_description = String(json_is_string(description) ? json_string_value(description) : "", "utf8");
   if (source != nullptr)
   {
      m_source = source;
      m_program = shared_ptr<NXSL_Program>(program);
      m_compileError = L"";
   }
   m_interval = interval;
   m_objectId = objectId;
   m_action = action;
   m_cooldown = cooldown;
   m_renotifyInterval = renotifyInterval;
   m_enabled = enabled;
   m_locked = locked;
   m_modificationTime = time(nullptr);
   return RCC_SUCCESS;
}

/**
 * Serialize standing check to JSON
 */
json_t *AIOperatorCheck::toJson() const
{
   json_t *json = json_object();
   json_object_set_new(json, "id", json_integer(m_id));
   json_object_set_new(json, "instanceId", json_integer(m_instanceId));
   json_object_set_new(json, "name", json_string_w(m_name));
   json_object_set_new(json, "description", json_string_t(m_description));
   json_object_set_new(json, "enabled", json_boolean(m_enabled));
   json_object_set_new(json, "locked", json_boolean(m_locked));
   json_object_set_new(json, "createdBy", json_string(m_createdByModel ? "model" : "human"));
   json_object_set_new(json, "source", json_string(m_source.c_str()));
   json_object_set_new(json, "interval", json_integer(m_interval));
   json_object_set_new(json, "objectId", json_integer(m_objectId));
   json_object_set_new(json, "action", json_string((m_action == AICheckAction::OBSERVE) ? "observe" : "wake"));
   json_object_set_new(json, "cooldown", json_integer(m_cooldown));
   json_object_set_new(json, "renotifyInterval", json_integer(m_renotifyInterval));
   json_object_set_new(json, "lastRun", json_time_string(m_lastRun));
   json_object_set_new(json, "lastVerdict", json_string(VerdictToName(m_lastVerdict)));
   json_object_set_new(json, "lastFire", json_time_string(m_lastFire));
   json_object_set_new(json, "lastPayload", json_string(m_lastPayload.c_str()));
   json_object_set_new(json, "consecutiveErrors", json_integer(m_consecutiveErrors));
   json_object_set_new(json, "runCount", json_integer(m_runCount));
   json_object_set_new(json, "compileError", json_string_t(m_compileError));
   json_object_set_new(json, "created", json_time_string(m_creationTime));
   json_object_set_new(json, "modified", json_time_string(m_modificationTime));
   return json;
}

/**
 * Serialize standing check as compact JSON for the <checks> prompt block
 */
json_t *AIOperatorCheck::toPromptJson() const
{
   json_t *json = json_object();
   json_object_set_new(json, "id", json_integer(m_id));
   json_object_set_new(json, "name", json_string_w(m_name));
   if (!m_description.isEmpty())
      json_object_set_new(json, "description", json_string_t(m_description));
   json_object_set_new(json, "interval", json_integer(m_interval));
   if (m_objectId != 0)
      json_object_set_new(json, "object_id", json_integer(m_objectId));
   json_object_set_new(json, "action", json_string((m_action == AICheckAction::OBSERVE) ? "observe" : "wake"));
   json_object_set_new(json, "enabled", json_boolean(m_enabled));
   json_object_set_new(json, "locked", json_boolean(m_locked));
   json_object_set_new(json, "last_verdict", json_string(VerdictToName(m_lastVerdict)));
   if (m_lastRun != 0)
      json_object_set_new(json, "last_run", json_string(FormatISO8601Timestamp(m_lastRun).c_str()));
   if (m_lastFire != 0)
      json_object_set_new(json, "last_fire", json_string(FormatISO8601Timestamp(m_lastFire).c_str()));
   if (m_consecutiveErrors != 0)
      json_object_set_new(json, "consecutive_errors", json_integer(m_consecutiveErrors));
   if ((m_lastVerdict == AICheckVerdict::FAILED) && !m_lastPayload.empty())
   {
      json_t *payload = json_loads(m_lastPayload.c_str(), 0, nullptr);
      if (payload != nullptr)
         json_object_set_new(json, "last_error", json_incref(json_object_get(payload, "error")));
      json_decref(payload);
   }
   return json;
}

/**
 * Fill NXCP message with standing check data
 */
void AIOperatorCheck::fillMessage(NXCPMessage *msg, uint32_t baseId) const
{
   msg->setField(baseId, m_id);
   msg->setField(baseId + 1, m_instanceId);
   msg->setField(baseId + 2, m_name);
   msg->setField(baseId + 3, m_description);
   msg->setField(baseId + 4, m_enabled);
   msg->setField(baseId + 5, m_locked);
   msg->setField(baseId + 6, m_createdByModel);
   msg->setFieldFromUtf8String(baseId + 7, m_source.c_str());
   msg->setField(baseId + 8, m_interval);
   msg->setField(baseId + 9, m_objectId);
   msg->setField(baseId + 10, static_cast<int16_t>(m_action));
   msg->setField(baseId + 11, m_cooldown);
   msg->setField(baseId + 12, m_renotifyInterval);
   msg->setFieldFromTime(baseId + 13, m_lastRun);
   msg->setField(baseId + 14, static_cast<int16_t>(m_lastVerdict));
   msg->setFieldFromTime(baseId + 15, m_lastFire);
   msg->setFieldFromUtf8String(baseId + 16, m_lastPayload.c_str());
   msg->setField(baseId + 17, m_consecutiveErrors);
   msg->setField(baseId + 18, m_runCount);
   msg->setFieldFromTime(baseId + 19, m_creationTime);
   msg->setFieldFromTime(baseId + 20, m_modificationTime);
   msg->setField(baseId + 21, m_compileError);
}

/**
 * Update standing instructions. Previous text is written to history, new text is truncated to configured
 * maximum size. Returns number of characters kept if the text was truncated, 0 otherwise.
 * Must be called with instance lock held.
 */
size_t AIOperatorInstance::setInstructions(const char *text, time_t now)
{
   std::string value(CHECK_NULL_EX_A(text));
   size_t truncatedTo = 0;
   size_t maxSize = ConfigReadULong(L"AIOperator.Instructions.MaxSize", 8192);
   if ((maxSize > 0) && (value.length() > maxSize))
   {
      // Do not split a multi-byte UTF-8 sequence
      size_t cut = maxSize;
      while((cut > 0) && ((static_cast<unsigned char>(value[cut]) & 0xC0) == 0x80))
         cut--;
      value.resize(cut);
      truncatedTo = cut;
   }

   if (value == m_instructions)
      return truncatedTo;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, L"INSERT INTO ai_operator_instr_history (record_id,instance_id,iteration,change_timestamp,previous_text) VALUES (?,?,?,?,?)");
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, InterlockedIncrement64(&s_instructionsHistoryId));
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_id);
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_iteration);
      DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(now));
      DBBind(hStmt, 5, DB_SQLTYPE_TEXT, DB_CTYPE_UTF8_STRING, m_instructions.c_str(), DB_BIND_STATIC);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }

   uint32_t depth = ConfigReadULong(L"AIOperator.Instructions.HistoryDepth", 20);
   if (depth > 0)
   {
      wchar_t query[256];
      nx_swprintf(query, 256, L"SELECT record_id FROM ai_operator_instr_history WHERE instance_id=%u ORDER BY record_id DESC", m_id);
      DB_RESULT hResult = DBSelect(hdb, query);
      if (hResult != nullptr)
      {
         if (DBGetNumRows(hResult) > static_cast<int>(depth))
         {
            int64_t cutoffId = DBGetFieldInt64(hResult, depth, 0);
            nx_swprintf(query, 256, L"DELETE FROM ai_operator_instr_history WHERE instance_id=%u AND record_id<=" INT64_FMT, m_id, cutoffId);
            DBQuery(hdb, query);
         }
         DBFreeResult(hResult);
      }
   }
   DBConnectionPoolReleaseConnection(hdb);

   m_instructions = value;
   m_modificationTime = now;
   nxlog_debug_tag(DEBUG_TAG, 5, L"AI operator [%u] \"%s\" standing instructions %s (%d characters)", m_id, m_name,
      value.empty() ? L"cleared" : L"updated", static_cast<int>(value.length()));
   return truncatedTo;
}

/**
 * Find index of standing check by ID. Must be called with instance lock held.
 */
int AIOperatorInstance::findCheckIndex(uint32_t checkId) const
{
   for(int i = 0; i < m_checks.size(); i++)
      if (m_checks.get(i)->getId() == checkId)
         return i;
   return -1;
}

/**
 * Append <checks> block to iteration prompt. Must be called with instance lock held.
 */
void AIOperatorInstance::appendChecksToPrompt(std::string& prompt) const
{
   if (m_checks.size() == 0)
      return;

   json_t *list = json_array();
   for(int i = 0; i < m_checks.size(); i++)
      json_array_append_new(list, m_checks.get(i)->toPromptJson());
   char *text = json_dumps(list, JSON_COMPACT);
   prompt.append("\n<checks>");
   prompt.append(CHECK_NULL_EX_A(text));
   prompt.append("</checks>");
   MemFree(text);
   json_decref(list);
}

/**
 * Create standing check
 */
uint32_t AIOperatorInstance::createCheck(json_t *config, bool byModel, uint32_t *checkId, MutableString *errorText)
{
   const char *name = json_object_get_string_utf8(config, "name", nullptr);
   if ((name == nullptr) || (*name == 0))
   {
      SetErrorText(errorText, L"name is required");
      return RCC_INVALID_ARGUMENT;
   }
   const char *source = json_object_get_string_utf8(config, "source", nullptr);
   if ((source == nullptr) || (*source == 0))
   {
      SetErrorText(errorText, L"source is required");
      return RCC_INVALID_ARGUMENT;
   }

   LockGuard lockGuard(m_mutex);

   int limit = ConfigReadInt(L"AIOperator.Checks.MaxPerInstance", 32);
   if ((limit > 0) && (m_checks.size() >= limit))
   {
      SetErrorText(errorText, L"check limit for this instance is reached; delete a check that is no longer needed first");
      return RCC_RESOURCE_NOT_AVAILABLE;
   }

   shared_ptr<AIOperatorCheck> check = make_shared<AIOperatorCheck>(m_id, byModel);
   uint32_t rcc = check->modifyFromJSON(config, byModel, errorText);
   if (rcc != RCC_SUCCESS)
      return rcc;

   m_checks.add(check);
   check->saveToDatabase();
   ConfigWriteInt(L"AIOperator.LastCheckId", s_checkId, true, false, true);

   nxlog_debug_tag(DEBUG_TAG, 4, L"AI operator [%u] \"%s\": standing check [%u] \"%s\" created by %s", m_id, m_name,
      check->getId(), check->getName(), byModel ? L"model" : L"user");
   if (checkId != nullptr)
      *checkId = check->getId();
   return RCC_SUCCESS;
}

/**
 * Modify standing check
 */
uint32_t AIOperatorInstance::modifyCheck(uint32_t checkId, json_t *config, bool byModel, MutableString *errorText)
{
   LockGuard lockGuard(m_mutex);
   int index = findCheckIndex(checkId);
   if (index == -1)
      return RCC_NO_SUCH_RECORD;

   AIOperatorCheck *check = m_checks.get(index);
   uint32_t rcc = check->modifyFromJSON(config, byModel, errorText);
   if (rcc != RCC_SUCCESS)
      return rcc;

   check->saveToDatabase();
   nxlog_debug_tag(DEBUG_TAG, 4, L"AI operator [%u] \"%s\": standing check [%u] \"%s\" modified by %s", m_id, m_name,
      check->getId(), check->getName(), byModel ? L"model" : L"user");
   return RCC_SUCCESS;
}

/**
 * Delete standing check
 */
uint32_t AIOperatorInstance::deleteCheck(uint32_t checkId, bool byModel)
{
   m_mutex.lock();
   int index = findCheckIndex(checkId);
   if (index == -1)
   {
      m_mutex.unlock();
      return RCC_NO_SUCH_RECORD;
   }

   shared_ptr<AIOperatorCheck> check = m_checks.getShared(index);
   if (byModel && check->isLocked())
   {
      m_mutex.unlock();
      return RCC_ACCESS_DENIED;
   }
   m_checks.remove(index);
   m_mutex.unlock();

   check->deleteFromDatabase();
   nxlog_debug_tag(DEBUG_TAG, 4, L"AI operator [%u] \"%s\": standing check [%u] \"%s\" deleted by %s", m_id, m_name,
      check->getId(), check->getName(), byModel ? L"model" : L"user");
   return RCC_SUCCESS;
}

/**
 * Get standing check by ID
 */
shared_ptr<AIOperatorCheck> AIOperatorInstance::getCheck(uint32_t checkId) const
{
   LockGuard lockGuard(m_mutex);
   int index = findCheckIndex(checkId);
   return (index != -1) ? m_checks.getShared(index) : shared_ptr<AIOperatorCheck>();
}

/**
 * Get all standing checks
 */
void AIOperatorInstance::getChecks(SharedObjectArray<AIOperatorCheck> *checks) const
{
   LockGuard lockGuard(m_mutex);
   checks->addAll(m_checks);
}

/**
 * Get number of standing checks (total and enabled)
 */
int AIOperatorInstance::getCheckCount(int *enabledCount) const
{
   LockGuard lockGuard(m_mutex);
   int enabled = 0;
   for(int i = 0; i < m_checks.size(); i++)
      if (m_checks.get(i)->isEnabled())
         enabled++;
   if (enabledCount != nullptr)
      *enabledCount = enabled;
   return m_checks.size();
}

/**
 * Collect standing checks that are due and mark them as running
 */
void AIOperatorInstance::collectDueChecks(time_t now, std::vector<shared_ptr<AIOperatorCheck>> *checks)
{
   LockGuard lockGuard(m_mutex);
   for(int i = 0; i < m_checks.size(); i++)
   {
      const shared_ptr<AIOperatorCheck>& check = m_checks.getShared(i);
      if (check->isDue(now))
      {
         check->setRunning(true);
         checks->push_back(check);
      }
   }
}

/**
 * Drop pending check interrupts (instance is disabled)
 */
void AIOperatorInstance::dropPendingInterrupts()
{
   LockGuard lockGuard(m_mutex);
   if (!m_pendingInterrupts.empty())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"AI operator [%u] \"%s\" dropped %d pending check interrupt(s) because instance is disabled",
         m_id, m_name, static_cast<int>(m_pendingInterrupts.size()));
      m_pendingInterrupts.clear();
   }
   m_interruptPending = false;
}

/**
 * Run standing check: execute script, evaluate transition, apply action
 */
void AIOperatorInstance::runCheck(shared_ptr<AIOperatorCheck> check)
{
   int64_t startTime = GetCurrentTimeMs();
   uint32_t timeLimit = ConfigReadULong(L"AIOperator.Checks.ExecutionTimeLimit", 10);

   m_mutex.lock();
   shared_ptr<NXSL_Program> program = check->getProgram();
   String compileError = check->getCompileError();
   uint32_t objectId = check->getObjectId();
   wchar_t checkName[64];
   wcscpy(checkName, check->getName());
   m_mutex.unlock();

   AICheckResult result;
   if (program != nullptr)
   {
      result = ExecuteCheckScript(program, objectId, checkName, timeLimit);
   }
   else
   {
      result.verdict = AICheckVerdict::FAILED;
      char *error = UTF8StringFromWideString(compileError);
      result.error = std::string("compilation error: ").append(error);
      MemFree(error);
   }

   time_t now = time(nullptr);
   uint32_t durationMs = static_cast<uint32_t>(GetCurrentTimeMs() - startTime);

   m_mutex.lock();
   AICheckTransition transition = EvaluateAICheckTransition(check->getLastVerdict(), check->getLastFire(), now,
      check->getCooldown(), check->getRenotifyInterval(), result.verdict);
   bool persist = check->recordRun(now, result, transition);

   bool checkDisabled = false;
   if (transition == AICheckTransition::FAILURE)
   {
      uint32_t maxErrors = ConfigReadULong(L"AIOperator.Checks.MaxConsecutiveErrors", 5);
      if ((maxErrors > 0) && (check->getConsecutiveErrors() >= maxErrors))
      {
         check->setEnabled(false);
         checkDisabled = true;
      }
   }
   AICheckAction action = check->getAction();
   bool instanceEnabled = m_enabled;
   uint32_t consecutiveErrors = check->getConsecutiveErrors();

   StringBuffer text;
   switch(transition)
   {
      case AICheckTransition::FIRE_EDGE:
      case AICheckTransition::FIRE_RENOTIFY:
         text.appendFormattedString(L"Check [%u] \"%s\" %s: %hs", check->getId(), checkName,
            (transition == AICheckTransition::FIRE_EDGE) ? L"fired" : L"re-notified", result.title.c_str());
         logExecution('K', durationMs, 0, 0, text);
         break;
      case AICheckTransition::FAILURE:
         text.appendFormattedString(L"Check [%u] \"%s\" failed: %hs", check->getId(), checkName, result.error.c_str());
         if (checkDisabled)
            text.appendFormattedString(L" (check disabled after %u consecutive errors)", consecutiveErrors);
         logExecution('E', durationMs, 0, 0, text);
         break;
      default:
         break;
   }
   m_mutex.unlock();

   nxlog_debug_tag(DEBUG_TAG, (transition == AICheckTransition::NONE) ? 7 : 5,
      L"AI operator [%u] \"%s\": standing check [%u] \"%s\" run completed in %u ms (verdict=%hs, transition=%d)",
      m_id, m_name, check->getId(), checkName, durationMs, VerdictToName(result.verdict), static_cast<int>(transition));

   // Side effects are applied without holding the instance lock
   if ((transition == AICheckTransition::FIRE_EDGE) || (transition == AICheckTransition::FIRE_RENOTIFY))
   {
      if (action == AICheckAction::OBSERVE)
      {
         char buffer[64];
         char *name = UTF8StringFromWideString(checkName);
         json_t *refs = json_array();
         json_array_append_new(refs, json_string(std::string("check:").append(IntegerToString(check->getId(), buffer)).append(":").append(name).c_str()));
         std::string refsText = JsonToString(refs);   // JsonToString releases the array
         MemFree(name);
         RecordObservation(this, result.severity, result.title.c_str(), result.details.empty() ? nullptr : result.details.c_str(), objectId, refsText.c_str());
      }
      else if (instanceEnabled)
      {
         AIOperatorInterrupt interrupt;
         interrupt.checkId = check->getId();
         char *name = UTF8StringFromWideString(checkName);
         interrupt.checkName = name;
         MemFree(name);
         interrupt.payload = CheckResultToJson(result);
         interrupt.timestamp = now;

         m_mutex.lock();
         m_pendingInterrupts.push_back(interrupt);
         m_interruptPending = true;
         m_mutex.unlock();
         nxlog_debug_tag(DEBUG_TAG, 5, L"AI operator [%u] \"%s\": interrupt queued by standing check [%u] \"%s\"", m_id, m_name, check->getId(), checkName);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 4, L"AI operator [%u] \"%s\": interrupt from standing check [%u] \"%s\" dropped because instance is disabled",
            m_id, m_name, check->getId(), checkName);
      }
   }
   else if (checkDisabled)
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"Standing check [%u] \"%s\" of AI operator instance [%u] \"%s\" disabled after %u consecutive errors (last error: %hs)",
         check->getId(), checkName, m_id, m_name, consecutiveErrors, result.error.c_str());

      char buffer[64];
      char *name = UTF8StringFromWideString(checkName);
      std::string title = std::string("Standing check \"").append(name).append("\" disabled after ").append(IntegerToString(consecutiveErrors, buffer)).append(" consecutive errors");
      json_t *refs = json_array();
      json_array_append_new(refs, json_string(std::string("check:").append(IntegerToString(check->getId(), buffer)).append(":").append(name).c_str()));
      std::string refsText = JsonToString(refs);   // JsonToString releases the array
      MemFree(name);
      RecordObservation(this, SEVERITY_WARNING, title.c_str(), result.error.c_str(), objectId, refsText.c_str());
   }

   // Persist unless instance or check was deleted while the script was running.
   // Lock order is always instance list lock -> AIOperatorInstance::m_mutex, never the reverse.
   s_instancesLock.lock();
   bool registered = s_instances.contains(m_id);
   s_instancesLock.unlock();

   m_mutex.lock();
   if (persist && registered && (findCheckIndex(check->getId()) != -1))
      check->saveToDatabase();
   check->setRunning(false);
   m_mutex.unlock();
}

/**
 * Create standing check for AI operator instance
 */
uint32_t NXCORE_EXPORTABLE CreateAIOperatorCheck(uint32_t instanceId, json_t *config, bool byModel, uint32_t *checkId, MutableString *errorText)
{
   shared_ptr<AIOperatorInstance> instance = GetAIOperatorInstance(instanceId);
   if (instance == nullptr)
      return RCC_INVALID_TASK_ID;
   return instance->createCheck(config, byModel, checkId, errorText);
}

/**
 * Modify standing check of AI operator instance
 */
uint32_t NXCORE_EXPORTABLE ModifyAIOperatorCheck(uint32_t instanceId, uint32_t checkId, json_t *config, bool byModel, MutableString *errorText)
{
   shared_ptr<AIOperatorInstance> instance = GetAIOperatorInstance(instanceId);
   if (instance == nullptr)
      return RCC_INVALID_TASK_ID;
   return instance->modifyCheck(checkId, config, byModel, errorText);
}

/**
 * Delete standing check of AI operator instance
 */
uint32_t NXCORE_EXPORTABLE DeleteAIOperatorCheck(uint32_t instanceId, uint32_t checkId, bool byModel)
{
   shared_ptr<AIOperatorInstance> instance = GetAIOperatorInstance(instanceId);
   if (instance == nullptr)
      return RCC_INVALID_TASK_ID;
   return instance->deleteCheck(checkId, byModel);
}

/**
 * Get standing check of AI operator instance
 */
shared_ptr<AIOperatorCheck> NXCORE_EXPORTABLE GetAIOperatorCheck(uint32_t instanceId, uint32_t checkId)
{
   shared_ptr<AIOperatorInstance> instance = GetAIOperatorInstance(instanceId);
   return (instance != nullptr) ? instance->getCheck(checkId) : shared_ptr<AIOperatorCheck>();
}

/**
 * Get standing checks of AI operator instance as JSON array
 */
json_t NXCORE_EXPORTABLE *GetAIOperatorChecksAsJson(uint32_t instanceId)
{
   shared_ptr<AIOperatorInstance> instance = GetAIOperatorInstance(instanceId);
   if (instance == nullptr)
      return nullptr;

   SharedObjectArray<AIOperatorCheck> checks;
   instance->getChecks(&checks);

   json_t *output = json_array();
   for(int i = 0; i < checks.size(); i++)
      json_array_append_new(output, checks.get(i)->toJson());
   return output;
}

/**
 * Fill NXCP message with standing checks of AI operator instance
 */
uint32_t FillAIOperatorCheckListMessage(uint32_t instanceId, NXCPMessage *msg)
{
   shared_ptr<AIOperatorInstance> instance = GetAIOperatorInstance(instanceId);
   if (instance == nullptr)
      return RCC_INVALID_TASK_ID;

   SharedObjectArray<AIOperatorCheck> checks;
   instance->getChecks(&checks);

   uint32_t fieldId = VID_ELEMENT_LIST_BASE;
   for(int i = 0; i < checks.size(); i++)
   {
      checks.get(i)->fillMessage(msg, fieldId);
      fieldId += 30;
   }
   msg->setField(VID_NUM_ELEMENTS, static_cast<uint32_t>(checks.size()));
   return RCC_SUCCESS;
}

/**
 * Read standing instructions history of AI operator instance. Callback receives record ID, iteration,
 * change timestamp, and previous text (UTF-8).
 */
static void ReadInstructionsHistory(uint32_t instanceId, std::function<void (int64_t, uint32_t, time_t, const char*)> callback)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, L"SELECT record_id,iteration,change_timestamp,previous_text FROM ai_operator_instr_history WHERE instance_id=? ORDER BY record_id DESC");
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, instanceId);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         for(int i = 0; i < count; i++)
         {
            char *text = DBGetFieldUTF8(hResult, i, 3, nullptr, 0);
            callback(DBGetFieldInt64(hResult, i, 0), DBGetFieldUInt32(hResult, i, 1), static_cast<time_t>(DBGetFieldInt64(hResult, i, 2)), CHECK_NULL_EX_A(text));
            MemFree(text);
         }
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Get standing instructions history of AI operator instance as JSON array
 */
json_t NXCORE_EXPORTABLE *GetAIOperatorInstructionsHistoryAsJson(uint32_t instanceId)
{
   if (GetAIOperatorInstance(instanceId) == nullptr)
      return nullptr;

   json_t *output = json_array();
   ReadInstructionsHistory(instanceId,
      [output] (int64_t recordId, uint32_t iteration, time_t timestamp, const char *text)
      {
         json_t *record = json_object();
         json_object_set_new(record, "id", json_integer(recordId));
         json_object_set_new(record, "iteration", json_integer(iteration));
         json_object_set_new(record, "timestamp", json_time_string(timestamp));
         json_object_set_new(record, "previousText", json_string(text));
         json_array_append_new(output, record);
      });
   return output;
}

/**
 * Fill NXCP message with standing instructions history of AI operator instance
 */
uint32_t FillAIOperatorInstructionsHistoryMessage(uint32_t instanceId, NXCPMessage *msg)
{
   if (GetAIOperatorInstance(instanceId) == nullptr)
      return RCC_INVALID_TASK_ID;

   uint32_t fieldId = VID_ELEMENT_LIST_BASE;
   uint32_t count = 0;
   ReadInstructionsHistory(instanceId,
      [msg, &fieldId, &count] (int64_t recordId, uint32_t iteration, time_t timestamp, const char *text)
      {
         msg->setField(fieldId, recordId);
         msg->setField(fieldId + 1, iteration);
         msg->setFieldFromTime(fieldId + 2, timestamp);
         msg->setFieldFromUtf8String(fieldId + 3, text);
         fieldId += 10;
         count++;
      });
   msg->setField(VID_NUM_ELEMENTS, count);
   return RCC_SUCCESS;
}

/**
 * Build standing check configuration from AI function arguments. Returns false and sets error message on failure.
 */
static bool BuildCheckConfigFromArguments(json_t *arguments, json_t *config, uint32_t userId, std::string *error)
{
   static const char *stringFields[] = { "name", "description", "source", "action", nullptr };
   for(int i = 0; stringFields[i] != nullptr; i++)
   {
      json_t *value = json_object_get(arguments, stringFields[i]);
      if (value != nullptr)
         json_object_set(config, stringFields[i], value);
   }

   static const struct { const char *argument; const char *tag; } integerFields[] =
   {
      { "interval", "interval" },
      { "cooldown", "cooldown" },
      { "renotify_interval", "renotifyInterval" },
      { nullptr, nullptr }
   };
   for(int i = 0; integerFields[i].argument != nullptr; i++)
   {
      json_t *value = json_object_get(arguments, integerFields[i].argument);
      if (value != nullptr)
         json_object_set_new(config, integerFields[i].tag, json_integer(json_object_get_int32(arguments, integerFields[i].argument, 0)));
   }

   json_t *enabled = json_object_get(arguments, "enabled");
   if (enabled != nullptr)
      json_object_set_new(config, "enabled", json_boolean(json_object_get_boolean(arguments, "enabled", true)));

   json_t *objectElement = json_object_get(arguments, "object");
   if (objectElement != nullptr)
   {
      const char *objectName = json_object_get_string_utf8(arguments, "object", "");
      if ((*objectName == 0) || !stricmp(objectName, "none"))
      {
         json_object_set_new(config, "objectId", json_integer(0));
      }
      else
      {
         shared_ptr<NetObj> object = FindObjectByNameOrId(arguments, "object");
         if (object == nullptr)
         {
            *error = "Error: object not found";
            return false;
         }
         if (!object->checkAccessRights(userId, OBJECT_ACCESS_READ))
         {
            *error = "Error: access to object denied";
            return false;
         }
         json_object_set_new(config, "objectId", json_integer(object->getId()));
      }
   }
   return true;
}

/**
 * Convert RCC from standing check management to AI function result message
 */
static std::string CheckRccToMessage(uint32_t rcc, const String& errorText)
{
   std::string message;
   switch(rcc)
   {
      case RCC_SUCCESS:
         return std::string("OK");
      case RCC_NO_SUCH_RECORD:
         return std::string("Error: check with given ID does not exist in this instance");
      case RCC_ACCESS_DENIED:
         return std::string("Error: check is locked by administrator and cannot be modified or deleted");
      case RCC_RESOURCE_NOT_AVAILABLE:
         message = "Error: ";
         break;
      case RCC_NXSL_COMPILATION_ERROR:
         message = "Error: script compilation failed: ";
         break;
      default:
         message = "Error: invalid check configuration: ";
         break;
   }
   char *text = UTF8StringFromWideString(errorText);
   message.append(text);
   MemFree(text);
   return message;
}

/**
 * AI assistant function: create-check
 */
static std::string F_CreateCheck(json_t *arguments, uint32_t userId)
{
   AIOperatorInstance *instance = s_currentInstance;
   if (instance == nullptr)
      return std::string("Error: this function can only be used during AI operator execution");

   json_t *config = json_object();
   std::string error;
   if (!BuildCheckConfigFromArguments(arguments, config, userId, &error))
   {
      json_decref(config);
      return error;
   }

   uint32_t checkId;
   MutableString errorText;
   uint32_t rcc = instance->createCheck(config, true, &checkId, &errorText);
   json_decref(config);
   if (rcc != RCC_SUCCESS)
      return CheckRccToMessage(rcc, errorText);

   char buffer[64];
   return std::string("Check created with ID ").append(IntegerToString(checkId, buffer));
}

/**
 * AI assistant function: update-check
 */
static std::string F_UpdateCheck(json_t *arguments, uint32_t userId)
{
   AIOperatorInstance *instance = s_currentInstance;
   if (instance == nullptr)
      return std::string("Error: this function can only be used during AI operator execution");

   uint32_t checkId = json_object_get_uint32(arguments, "id", 0);
   if (checkId == 0)
      return std::string("Error: id parameter is required");

   json_t *config = json_object();
   std::string error;
   if (!BuildCheckConfigFromArguments(arguments, config, userId, &error))
   {
      json_decref(config);
      return error;
   }

   MutableString errorText;
   uint32_t rcc = instance->modifyCheck(checkId, config, true, &errorText);
   json_decref(config);
   if (rcc != RCC_SUCCESS)
      return CheckRccToMessage(rcc, errorText);

   char buffer[64];
   return std::string("Check ").append(IntegerToString(checkId, buffer)).append(" updated");
}

/**
 * AI assistant function: delete-check
 */
static std::string F_DeleteCheck(json_t *arguments, uint32_t userId)
{
   AIOperatorInstance *instance = s_currentInstance;
   if (instance == nullptr)
      return std::string("Error: this function can only be used during AI operator execution");

   uint32_t checkId = json_object_get_uint32(arguments, "id", 0);
   if (checkId == 0)
      return std::string("Error: id parameter is required");

   uint32_t rcc = instance->deleteCheck(checkId, true);
   if (rcc != RCC_SUCCESS)
      return CheckRccToMessage(rcc, MutableString());

   char buffer[64];
   return std::string("Check ").append(IntegerToString(checkId, buffer)).append(" deleted");
}

/**
 * AI assistant function: test-check
 */
static std::string F_TestCheck(json_t *arguments, uint32_t userId)
{
   AIOperatorInstance *instance = s_currentInstance;
   if (instance == nullptr)
      return std::string("Error: this function can only be used during AI operator execution");

   const char *source = json_object_get_string_utf8(arguments, "source", nullptr);
   if ((source == nullptr) || (*source == 0))
      return std::string("Error: source parameter is required");

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

   MutableString errorText;
   NXSL_Program *program = CompileCheckSource(source, &errorText);
   if (program == nullptr)
      return CheckRccToMessage(RCC_NXSL_COMPILATION_ERROR, errorText);

   AICheckResult result = ExecuteCheckScript(shared_ptr<NXSL_Program>(program), objectId, L"test", ConfigReadULong(L"AIOperator.Checks.ExecutionTimeLimit", 10));

   std::string output("verdict=");
   output.append(VerdictToName(result.verdict));
   if (result.verdict == AICheckVerdict::FIRED)
   {
      output.append("\ntitle=").append(result.title);
      char buffer[32];
      output.append("\nseverity=").append(IntegerToString(result.severity, buffer));
      if (!result.details.empty())
         output.append("\ndetails=").append(result.details);
   }
   else if (result.verdict == AICheckVerdict::FAILED)
   {
      output.append("\nerror=").append(result.error);
   }
   return output;
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
      std::vector<std::pair<shared_ptr<AIOperatorInstance>, shared_ptr<AIOperatorCheck>>> checksToRun;

      // Mark selected instances and checks as executing at enqueue time to prevent double dispatch.
      // An instance with pending check interrupts is due regardless of its next execution time.
      s_instancesLock.lock();
      for(const shared_ptr<AIOperatorInstance>& instance : s_instances)
      {
         if (!instance->isEnabled())
         {
            if (instance->hasPendingInterrupts())
               instance->dropPendingInterrupts();
            continue;
         }

         if (!instance->isExecuting() && ((instance->getNextExecutionTime() <= now) || instance->hasPendingInterrupts()))
         {
            instance->setExecuting();
            instancesToExecute.push_back(instance);
         }

         std::vector<shared_ptr<AIOperatorCheck>> dueChecks;
         instance->collectDueChecks(now, &dueChecks);
         for(const shared_ptr<AIOperatorCheck>& check : dueChecks)
            checksToRun.push_back({ instance, check });
      }
      s_instancesLock.unlock();

      for(const auto& entry : checksToRun)
      {
         if (g_flags & AF_SHUTDOWN)
            break;

         nxlog_debug_tag(DEBUG_TAG, 7, L"Running standing check [%u] \"%s\" of AI operator instance [%u]",
            entry.second->getId(), entry.second->getName(), entry.first->getId());
         ThreadPoolExecute(s_threadPool, entry.first, &AIOperatorInstance::runCheck, entry.second);
      }

      for(const shared_ptr<AIOperatorInstance>& instance : instancesToExecute)
      {
         if (g_flags & AF_SHUTDOWN)
            break;

         nxlog_debug_tag(DEBUG_TAG, 5, L"Executing AI operator instance [%u] \"%s\"%s", instance->getId(), instance->getName(),
            instance->hasPendingInterrupts() ? L" (triggered by standing check)" : L"");
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
   ConsolePrintf(console, L" %-6s | %-24s | %-8s | %-9s | %-20s | %-20s | %9s | %12s | %-7s\n",
      L"ID", L"Name", L"Enabled", L"State", L"Last execution", L"Next execution", L"Iteration", L"Tokens today", L"Checks");
   ConsolePrintf(console, L"--------+--------------------------+----------+-----------+----------------------+----------------------+-----------+--------------+--------\n");

   s_instancesLock.lock();
   s_instances.forEach(
      [console] (const uint32_t& key, const shared_ptr<AIOperatorInstance>& instance) -> EnumerationCallbackResult
      {
         int enabledChecks;
         int totalChecks = instance->getCheckCount(&enabledChecks);
         ConsolePrintf(console, L" %-6u | %-24s | %-8s | %-9s | %-20s | %-20s | %9u | %12u | %3d/%-3d\n",
            instance->getId(), instance->getName(),
            instance->isEnabled() ? L"yes" : L"no",
            instance->isExecuting() ? L"running" : L"idle",
            (instance->getLastExecutionTime() > 0) ? FormatTimestamp(instance->getLastExecutionTime()).cstr() : L"never",
            instance->isEnabled() ? FormatTimestamp(instance->getNextExecutionTime()).cstr() : L"never",
            instance->getIteration(),
            static_cast<uint32_t>(instance->getTokensUsedToday()),
            enabledChecks, totalChecks);
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
   s_observationId += HAGetRecordIdGap();

   hResult = DBSelect(hdb, L"SELECT max(record_id) FROM ai_operator_execution_log");
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_logRecordId = DBGetFieldInt64(hResult, 0, 0);
      DBFreeResult(hResult);
   }
   s_logRecordId += HAGetRecordIdGap();

   hResult = DBSelect(hdb, L"SELECT max(record_id) FROM ai_operator_instr_history");
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_instructionsHistoryId = DBGetFieldInt64(hResult, 0, 0);
      DBFreeResult(hResult);
   }
   s_instructionsHistoryId += HAGetRecordIdGap();

   hResult = DBSelect(hdb,
      L"SELECT id,name,description,owner_user_id,enabled,scope_filter,model_slot,min_interval,max_interval,daily_token_budget,"
      L"tokens_used,usage_day,persona_prompt,current_focus,watch_list,memento,observation_retention_days,"
      L"observation_max_records,instructions,instructions_locked,last_execution_time,next_execution_time,iteration,created,modified "
      L"FROM ai_operator_instances ORDER BY id");
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

   s_checkId = ConfigReadInt(L"AIOperator.LastCheckId", 0);
   hResult = DBSelect(hdb,
      L"SELECT id,instance_id,name,description,enabled,locked,created_by,source,check_interval,object_id,check_action,cooldown,"
      L"renotify_interval,last_run,last_verdict,last_fire,last_payload,consecutive_errors,run_count,created,modified "
      L"FROM ai_operator_checks ORDER BY id");
   if (hResult != nullptr)
   {
      uint32_t maxId = 0;
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         shared_ptr<AIOperatorCheck> check = make_shared<AIOperatorCheck>(hResult, i);
         shared_ptr<AIOperatorInstance> instance = s_instances.getShared(check->getInstanceId());
         if (instance != nullptr)
         {
            instance->loadCheck(check);
            if (!check->getCompileError().isEmpty())
               nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"Standing check [%u] \"%s\" of AI operator instance [%u] failed to compile: %s",
                  check->getId(), check->getName(), instance->getId(), check->getCompileError().cstr());
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 3, L"Standing check [%u] \"%s\" refers to non-existing AI operator instance [%u] and will be ignored",
               check->getId(), check->getName(), check->getInstanceId());
         }
         if (check->getId() > maxId)
            maxId = check->getId();
      }
      DBFreeResult(hResult);

      if (maxId > static_cast<uint32_t>(s_checkId))
         s_checkId = maxId;
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
      "- <interval_bounds> tag contains allowed range for next execution delay in seconds\n"
      "- <checks> tag (if present) lists your standing checks with their state\n"
      "- <triggered_checks> tag (if present) lists standing checks with 'wake' action that fired since your last iteration, "
      "with their payloads; this iteration was started because of them, so investigate them first\n"
      "- Use available functions to assess current operational status, alarms, events, and metrics\n"
      "- Load skills using load-skill function when you need specialized capabilities\n\n"
      "STANDING CHECKS:\n"
      "- A standing check is an NXSL script the server runs for you on its own schedule, without LLM cost and with consistent verdicts\n"
      "- Prefer a standing check for anything you would otherwise re-poll on every iteration\n"
      "- Manage checks with create-check, update-check, delete-check, and test-check functions; validate a script with test-check "
      "before creating it\n"
      "- Script contract: $object is bound to the check's object (or null); return null or false when everything is fine; "
      "return a string (used as title) or a hash with 'title', 'severity' (normal, warning, minor, major, critical) and 'details' "
      "to fire; any other return value is a check error\n"
      "- Use 'observe' action when a fired check is an unambiguous finding that can be recorded as an observation directly; "
      "use 'wake' when a fired check needs your investigation\n"
      "- Keep the check list small and relevant: review <checks> on every iteration, retire checks that are no longer useful, "
      "and do not re-poll conditions already covered by a check\n"
      "- Checks failing repeatedly are disabled automatically; fix or delete them\n\n"
      "STANDING INSTRUCTIONS:\n"
      "- You may keep standing instructions for yourself: guidance injected into your system prompt on every iteration "
      "after the operator persona\n"
      "- Return them in the 'instructions' response field: omit the field to keep the current text, return an empty string "
      "to clear it, or return the complete new text to replace it\n"
      "- Keep them short and actionable; persona and system rules always take precedence\n\n"
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
      "- 'instructions': updated standing instructions (optional, string; omit to keep, empty string to clear)\n"
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
         { "references", "Optional: array of references supporting the observation (alarm IDs, event IDs, DCI IDs, etc.)", "array", "string" }
      },
      F_RecordObservation);

   RegisterAIAssistantFunction(
      "create-check",
      "Create a standing check for this AI operator instance (available only during AI operator execution). "
      "A standing check is an NXSL script run by the server on schedule without LLM involvement. "
      "Script contract: $object is bound to the check's object (or null); return null or false when everything is fine; "
      "return a string (title) or a hash with 'title', 'severity', and 'details' to fire; any other value is an error. "
      "Source is compiled first; compilation errors are returned.",
      {
         { "name", "Short check name (max 63 chars)" },
         { "description", "Optional: what the check watches and why" },
         { "source", "NXSL source code of the check" },
         { "interval", "Run interval in seconds (minimum 30, default 300)", "integer" },
         { "object", "Optional: name or ID of the object bound as $object" },
         { "action", "Action when the check fires: 'wake' (start an iteration with the payload) or 'observe' (record observation directly); default 'wake'" },
         { "cooldown", "Optional: minimum seconds between two action applications (default 0)", "integer" },
         { "renotify_interval", "Optional: seconds after which the action is applied again while the check stays fired (default 0 = only on the quiet->fired edge)", "integer" }
      },
      F_CreateCheck);

   RegisterAIAssistantFunction(
      "update-check",
      "Update a standing check of this AI operator instance (available only during AI operator execution). "
      "Only provided fields are changed; changed source is compiled first. Locked checks cannot be updated.",
      {
         { "id", "Check ID", "integer" },
         { "name", "Optional: new name" },
         { "description", "Optional: new description" },
         { "source", "Optional: new NXSL source code" },
         { "interval", "Optional: new run interval in seconds (minimum 30)", "integer" },
         { "object", "Optional: name or ID of the object bound as $object ('none' to unbind)" },
         { "action", "Optional: 'wake' or 'observe'" },
         { "cooldown", "Optional: new cooldown in seconds", "integer" },
         { "renotify_interval", "Optional: new renotify interval in seconds", "integer" },
         { "enabled", "Optional: enable or disable the check", "boolean" }
      },
      F_UpdateCheck);

   RegisterAIAssistantFunction(
      "delete-check",
      "Delete a standing check of this AI operator instance (available only during AI operator execution). Locked checks cannot be deleted.",
      {
         { "id", "Check ID", "integer" }
      },
      F_DeleteCheck);

   RegisterAIAssistantFunction(
      "test-check",
      "Run standing check source once with the real security context and return its verdict without creating a check or recording anything "
      "(available only during AI operator execution). Use it to validate a script before create-check or update-check.",
      {
         { "source", "NXSL source code to test" },
         { "object", "Optional: name or ID of the object bound as $object" }
      },
      F_TestCheck);

   s_threadPool = ThreadPoolCreate(AI_OPERATOR_COMPONENT,
      ConfigReadInt(L"ThreadPool.AIOperator.BaseSize", 4),
      ConfigReadInt(L"ThreadPool.AIOperator.MaxSize", 16));
   ThreadCreate(AIOperatorSchedulerThread);

   nxlog_debug_tag(DEBUG_TAG, 2, L"AI operator subsystem initialized (%d instances)", s_instances.size());
}
