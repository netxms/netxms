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
** File: skills.cpp
**
**/

#include "nxcore.h"
#include <nxai.h>
#include <ai_provider.h>
#include <unordered_map>
#include <unordered_set>

#define DEBUG_TAG _T("ai.skills")
#define DEBUG_TAG_WORKERS _T("ai.workers")

json_t *RebuildToolDeclarations(const std::unordered_map<std::string, shared_ptr<AITool>>& tools);

/**
 * System prompt for workers
 */
static const char *s_workerSystemPrompt =
   "You are a worker started by the NetXMS AI assistant to execute one self-contained task. "
   "Complete the task described below using the available tools. "
   "When done, provide a concise summary of what was accomplished and any relevant results. "
   "You cannot interact with the user: make reasonable decisions based on the information provided. "
   "If the task cannot be completed as stated or requires a decision you cannot make, do not guess - "
   "stop and report what is missing or which options exist, so that the requester can resolve it. "
   "Focus only on the specific task given.";

/**
 * Worker thread pool and settings
 */
static ThreadPool *s_workerThreadPool = nullptr;
static int s_maxParallelWorkers = 4;
static uint32_t s_workerTimeout = 600;   // seconds

/**
 * Get name of skill execution mode for external representation
 */
static inline const char *SkillExecutionModeName(SkillExecutionMode mode)
{
   return (mode == SkillExecutionMode::WORKER) ? "worker" : "load";
}

/**
 * Load worker settings from server configuration
 */
void LoadAIWorkerSettings()
{
   s_maxParallelWorkers = std::max(ConfigReadInt(L"AI.Workers.MaxParallel", 4), 1);
   s_workerTimeout = std::max(ConfigReadULong(L"AI.Workers.Timeout", 600), static_cast<uint32_t>(10));
}

/**
 * Initialize workers
 */
void InitAIWorkers()
{
   LoadAIWorkerSettings();
   s_workerThreadPool = ThreadPoolCreate(L"AI-WORKERS",
         ConfigReadInt(L"ThreadPool.AIWorkers.BaseSize", 4),
         ConfigReadInt(L"ThreadPool.AIWorkers.MaxSize", 16));
   nxlog_debug_tag(DEBUG_TAG_WORKERS, 3, L"AI workers initialized (max parallel workers = %d, timeout = %u seconds)", s_maxParallelWorkers, s_workerTimeout);
}

/**
 * Skills
 */
static std::unordered_map<std::string, shared_ptr<AISkill>> s_skills;
static Mutex s_skillsMutex(MutexType::FAST);

/**
 * Get number registered AI assistant skills
 */
size_t GetRegisteredSkillCount()
{
   s_skillsMutex.lock();
   size_t count = s_skills.size();
   s_skillsMutex.unlock();
   return count;
}

/**
 * Load skill file content
 */
static std::string LoadSkillFile(const char *fileName)
{
   wchar_t path[MAX_PATH];
   GetNetXMSDirectory(nxDirShare, path);
   wcslcat(path, FS_PATH_SEPARATOR L"skills" FS_PATH_SEPARATOR, MAX_PATH);
   size_t len = wcslen(path);
   mb_to_wchar(fileName, -1, &path[len], MAX_PATH - len);
   char *content = LoadFileAsUTF8String(path);
   if (content == nullptr)
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"Cannot load AI assistant skill file \"%s\"", path);
      return std::string();
   }
   std::string result(content);
   MemFree(content);
   return result;
}

/**
 * Register AI skill. This function intended to be called only during server core or module initialization.
 */
void NXCORE_EXPORTABLE RegisterAISkill(const char *name, const char *description, const char *prompt)
{
   if (s_skills.find(name) != s_skills.end())
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"AI assistant skill \"%hs\" already registered", name);
      return;
   }

   if (prompt[0] == '@')
   {
      std::string fileContent = LoadSkillFile(&prompt[1]);
      if (fileContent.empty())
         return;
      s_skills.emplace(name, make_shared<AISkill>(name, description, fileContent));
      return;
   }

   s_skills.emplace(name, make_shared<AISkill>(name, description, prompt));
}

/**
 * Register AI skill with tools. This function intended to be called only during server core or module initialization.
 */
void NXCORE_EXPORTABLE RegisterAISkill(const char *name, const char *description, const char *prompt, const std::vector<AITool>& tools)
{
   if (s_skills.find(name) != s_skills.end())
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"AI assistant skill \"%hs\" already registered", name);
      return;
   }

   if (prompt[0] == '@')
   {
      std::string fileContent = LoadSkillFile(&prompt[1]);
      if (fileContent.empty())
         return;
      s_skills.emplace(name, make_shared<AISkill>(name, description, fileContent, tools));
      return;
   }

   s_skills.emplace(name, make_shared<AISkill>(name, description, prompt, tools));
}

/**
 * Get list of registered AI assistant skills as string
 */
std::string GetRegisteredSkills()
{
   json_t *output = json_array();
   auto disabledSkills = GetAIDisabledSkills();

   s_skillsMutex.lock();
   for(const auto& pair : s_skills)
   {
      if (disabledSkills.count(pair.first))
         continue;

      const AISkill& skill = *pair.second;
      json_t *skillObject = json_object();
      json_object_set_new(skillObject, "name", json_string(skill.name.c_str()));
      json_object_set_new(skillObject, "description", json_string(skill.description.c_str()));
      json_object_set_new(skillObject, "supports_worker", json_boolean(skill.supportsWorker));
      json_object_set_new(skillObject, "default_mode", json_string(SkillExecutionModeName(skill.defaultMode)));
      json_array_append_new(output, skillObject);
   }
   s_skillsMutex.unlock();

   return JsonToString(output);
}

/**
 * Get set of registered skill names
 */
std::unordered_set<std::string> GetRegisteredSkillNames()
{
   std::unordered_set<std::string> names;
   s_skillsMutex.lock();
   for(const auto& pair : s_skills)
      names.insert(pair.first);
   s_skillsMutex.unlock();
   return names;
}

/**
 * Collect tools from all registered skills not in the given disabled skills set
 */
void CollectSkillTools(std::vector<shared_ptr<AITool>>& tools, const std::unordered_set<std::string>& disabledSkills)
{
   LockGuard lockGuard(s_skillsMutex);
   for(const auto& pair : s_skills)
   {
      if (disabledSkills.count(pair.first))
         continue;
      for(const AITool& tool : pair.second->tools)
         tools.push_back(make_shared<AITool>(tool));
   }
}

/**
 * Find tool by name across all registered skills, skipping disabled skills. Returns nullptr if not found.
 */
shared_ptr<AITool> FindSkillTool(const char *name)
{
   auto disabledSkills = GetAIDisabledSkills();

   LockGuard lockGuard(s_skillsMutex);
   for(const auto& pair : s_skills)
   {
      if (disabledSkills.count(pair.first))
         continue;
      for(const AITool& tool : pair.second->tools)
      {
         if (tool.name == name)
            return make_shared<AITool>(tool);
      }
   }
   return shared_ptr<AITool>();
}

/**
 * Get skill prompt list for MCP clients (excluding disabled skills)
 */
json_t NXCORE_EXPORTABLE *GetMCPPromptsAsJson()
{
   auto disabledSkills = GetAIDisabledSkills();

   json_t *prompts = json_array();
   LockGuard lockGuard(s_skillsMutex);
   for(const auto& pair : s_skills)
   {
      if (disabledSkills.count(pair.first))
         continue;
      const AISkill& skill = *pair.second;
      json_t *entry = json_object();
      json_object_set_new(entry, "name", json_string(skill.name.c_str()));
      json_object_set_new(entry, "description", json_string(skill.description.c_str()));
      json_array_append_new(prompts, entry);
   }
   return prompts;
}

/**
 * Get single skill prompt for MCP clients
 */
json_t NXCORE_EXPORTABLE *GetMCPPromptAsJson(const char *name)
{
   if (IsAISkillDisabled(name))
      return nullptr;

   LockGuard lockGuard(s_skillsMutex);
   auto it = s_skills.find(name);
   if (it == s_skills.end())
      return nullptr;

   const AISkill& skill = *it->second;
   json_t *entry = json_object();
   json_object_set_new(entry, "name", json_string(skill.name.c_str()));
   json_object_set_new(entry, "description", json_string(skill.description.c_str()));
   json_object_set_new(entry, "prompt", json_string(skill.prompt.c_str()));
   return entry;
}

/**
 * Fill NXCP message with registered skills list (including disabled status)
 */
void FillAISkillListMessage(NXCPMessage *msg)
{
   auto disabledSkills = GetAIDisabledSkills();

   s_skillsMutex.lock();
   msg->setField(VID_NUM_SKILLS, static_cast<uint32_t>(s_skills.size()));

   uint32_t baseFieldId = VID_SKILL_LIST_BASE;
   for(const auto& pair : s_skills)
   {
      const AISkill& skill = *pair.second;
      uint32_t fieldId = baseFieldId;
      msg->setFieldFromUtf8String(fieldId++, skill.name.c_str());         // +0
      msg->setFieldFromUtf8String(fieldId++, skill.description.c_str());  // +1
      msg->setField(fieldId++, static_cast<uint16_t>(disabledSkills.count(skill.name) ? 1 : 0)); // +2 disabled flag
      msg->setField(fieldId++, static_cast<uint16_t>(skill.supportsWorker ? 1 : 0)); // +3
      msg->setFieldFromUtf8String(fieldId++, SkillExecutionModeName(skill.defaultMode)); // +4
      baseFieldId += 0x100;
   }
   s_skillsMutex.unlock();
}

/**
 * Get registered skills as JSON array
 */
json_t NXCORE_EXPORTABLE *GetAISkillsAsJson()
{
   auto disabledSkills = GetAIDisabledSkills();

   json_t *result = json_array();
   LockGuard lockGuard(s_skillsMutex);
   for (const auto& pair : s_skills)
   {
      const AISkill& skill = *pair.second;
      json_t *entry = json_object();
      json_object_set_new(entry, "name", json_string(skill.name.c_str()));
      json_object_set_new(entry, "description", json_string(skill.description.c_str()));
      json_object_set_new(entry, "disabled", json_boolean(disabledSkills.count(skill.name) > 0));
      json_object_set_new(entry, "supportsWorker", json_boolean(skill.supportsWorker));
      json_object_set_new(entry, "defaultMode", json_string(SkillExecutionModeName(skill.defaultMode)));
      json_array_append_new(result, entry);
   }
   return result;
}

/**
 * Load AI assistant skill into chat context
 */
std::string Chat::loadSkill(const char *skillName)
{
   if (IsAISkillDisabled(skillName))
      return std::string("Error: skill is disabled by administrator");

   s_skillsMutex.lock();
   auto it = s_skills.find(skillName);
   if (it == s_skills.end())
   {
      s_skillsMutex.unlock();
      return std::string("Error: skill not found");
   }

   const AISkill& skill = *it->second;
   auto disabledTools = GetAIDisabledTools();

   // Register skill tools (skip disabled ones)
   for(const AITool& tool : skill.tools)
   {
      if (disabledTools.count(tool.name))
         continue;
      m_tools.emplace(tool.name, make_shared<AITool>(tool));
      nxlog_debug_tag(DEBUG_TAG, 6, L"Loaded AI tool \"%hs\" from skill \"%hs\"", tool.name.c_str(), skill.name.c_str());
   }
   s_skillsMutex.unlock();

   json_decref(m_toolDeclarations);
   m_toolDeclarations = RebuildToolDeclarations(m_tools);

   return skill.prompt;
}

/**
 * State of single worker. Fields below "chat" are protected by the batch mutex once the worker is started.
 */
struct AIWorker
{
   std::string skill;
   std::string task;
   shared_ptr<Chat> chat;
   bool finished;
   bool success;
   std::string output;   // result on success, error message on failure
   LLMTokenUsage tokenUsage;

   AIWorker(const char *skill, const char *task) : skill(skill), task(task)
   {
      finished = false;
      success = false;
   }
};

/**
 * Batch of workers started by single run-workers call. Shared with worker threads, so it outlives the call on timeout.
 */
struct AIWorkerBatch
{
   Mutex mutex;
   Condition completed;
   std::vector<shared_ptr<AIWorker>> workers;
   size_t pending;

   AIWorkerBatch() : mutex(MutexType::FAST), completed(true)
   {
      pending = 0;
   }
};

/**
 * Execute worker (called on worker thread pool)
 */
static void ExecuteAIWorker(const shared_ptr<AIWorkerBatch>& batch, const shared_ptr<AIWorker>& worker)
{
   nxlog_debug_tag(DEBUG_TAG_WORKERS, 5, L"Worker started (skill=\"%hs\")", worker->skill.c_str());

   LLMTokenUsage tokenUsage;
   LLMTokenUsage *previousCollector = SetLLMTokenUsageCollector(&tokenUsage);
   char *result = worker->chat->sendRequest(worker->task.c_str());
   SetLLMTokenUsageCollector(previousCollector);

   batch->mutex.lock();
   worker->finished = true;
   worker->tokenUsage = tokenUsage;
   if (result != nullptr)
   {
      worker->success = true;
      worker->output = result;
      MemFree(result);
   }
   else
   {
      worker->output = "no response from worker";
   }
   bool allCompleted = (--batch->pending == 0);
   batch->mutex.unlock();

   nxlog_debug_tag(DEBUG_TAG_WORKERS, 5, L"Worker %s (skill=\"%hs\", tokens=" INT64_FMT L"/" INT64_FMT L")",
      worker->success ? L"completed" : L"failed", worker->skill.c_str(), tokenUsage.inputTokens, tokenUsage.outputTokens);
   if (allCompleted)
      batch->completed.set();
}

/**
 * Run workers - separate ephemeral chat contexts executing self-contained tasks in parallel. Workers run as the same user
 * and inherit origin of this chat, cannot ask user questions, and cannot start other workers. Token usage of workers is
 * added to the token usage collector of the calling thread.
 */
std::string Chat::runWorkers(json_t *workers)
{
   if (!json_is_array(workers) || (json_array_size(workers) == 0))
      return std::string("Error: list of workers must be provided");
   if (json_array_size(workers) > static_cast<size_t>(s_maxParallelWorkers))
      return std::string("Error: too many workers requested (maximum is ") + std::to_string(s_maxParallelWorkers) + ")";
   if (s_workerThreadPool == nullptr)
      return std::string("Error: workers are not available");

   auto batch = make_shared<AIWorkerBatch>();

   // Validate all requests before starting any worker
   size_t i;
   json_t *element;
   json_array_foreach(workers, i, element)
   {
      const char *task, *skillName;
      if (json_is_string(element))
      {
         task = json_string_value(element);
         skillName = "";
      }
      else if (json_is_object(element))
      {
         task = json_object_get_string_utf8(element, "task", "");
         skillName = json_object_get_string_utf8(element, "skill", "");
      }
      else
      {
         return std::string("Error: worker #") + std::to_string(i + 1) + " is not an object";
      }
      if (task[0] == 0)
         return std::string("Error: task is missing for worker #") + std::to_string(i + 1);

      if (skillName[0] != 0)
      {
         if (IsAISkillDisabled(skillName))
            return std::string("Error: skill \"") + skillName + "\" is disabled by administrator";

         LockGuard lockGuard(s_skillsMutex);
         auto it = s_skills.find(skillName);
         if (it == s_skills.end())
            return std::string("Error: skill \"") + skillName + "\" not found";
         if (!it->second->supportsWorker)
            return std::string("Error: skill \"") + skillName + "\" cannot be run in a worker, load it with load-skill instead";
      }

      batch->workers.push_back(make_shared<AIWorker>(skillName, task));
   }

   // Create worker chats
   for(const shared_ptr<AIWorker>& worker : batch->workers)
   {
      worker->chat = make_shared<Chat>(nullptr, nullptr, m_userId, s_workerSystemPrompt, false);
      Chat *chat = worker->chat.get();
      chat->setSlot("worker");
      chat->setOrigin(m_origin, m_originId);

      // Workers cannot start other workers
      chat->m_tools.erase("run-workers");
      json_decref(chat->m_toolDeclarations);
      chat->m_toolDeclarations = RebuildToolDeclarations(chat->m_tools);

      if (!worker->skill.empty())
      {
         std::string skillPrompt = chat->loadSkill(worker->skill.c_str());
         if (skillPrompt.find("Error:") == 0)
            return skillPrompt;
         chat->addMessage("system", skillPrompt.c_str());
      }
   }

   nxlog_debug_tag(DEBUG_TAG_WORKERS, 4, L"Chat [%u]: starting %d workers", m_id, static_cast<int>(batch->workers.size()));
   batch->pending = batch->workers.size();
   for(const shared_ptr<AIWorker>& worker : batch->workers)
   {
      ThreadPoolExecute(s_workerThreadPool,
         [batch, worker] () -> void
         {
            ExecuteAIWorker(batch, worker);
         });
   }

   bool allCompleted = batch->completed.wait(s_workerTimeout * 1000);

   LLMTokenUsage totalUsage;
   json_t *output = json_array();
   batch->mutex.lock();
   for(size_t n = 0; n < batch->workers.size(); n++)
   {
      AIWorker *worker = batch->workers[n].get();
      json_t *entry = json_object();
      json_object_set_new(entry, "worker", json_integer(n + 1));
      if (!worker->skill.empty())
         json_object_set_new(entry, "skill", json_string(worker->skill.c_str()));
      if (worker->finished)
      {
         json_object_set_new(entry, "status", json_string(worker->success ? "completed" : "failed"));
         json_object_set_new(entry, worker->success ? "result" : "error", json_string(worker->output.c_str()));
         totalUsage.inputTokens += worker->tokenUsage.inputTokens;
         totalUsage.outputTokens += worker->tokenUsage.outputTokens;
      }
      else
      {
         // Worker keeps running on its thread until next step; its result is discarded
         worker->chat->cancel();
         json_object_set_new(entry, "status", json_string("timeout"));
         json_object_set_new(entry, "error", json_string("worker did not complete within time limit"));
      }
      json_array_append_new(output, entry);
   }
   batch->mutex.unlock();

   if (!allCompleted)
      nxlog_debug_tag(DEBUG_TAG_WORKERS, 4, L"Chat [%u]: some workers did not complete within %u seconds", m_id, s_workerTimeout);

   // Account worker token usage to the requester (for example, AI operator daily budget)
   LLMTokenUsage *collector = GetLLMTokenUsageCollector();
   if (collector != nullptr)
   {
      collector->inputTokens += totalUsage.inputTokens;
      collector->outputTokens += totalUsage.outputTokens;
   }

   return JsonToString(output);
}

/**
 * Register AI skill with execution mode settings. This function intended to be called only during server core or module initialization.
 */
void NXCORE_EXPORTABLE RegisterAISkill(const char *name, const char *description, const char *prompt,
   bool supportsWorker, SkillExecutionMode defaultMode)
{
   if (s_skills.find(name) != s_skills.end())
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"AI assistant skill \"%hs\" already registered", name);
      return;
   }

   if (prompt[0] == '@')
   {
      std::string fileContent = LoadSkillFile(&prompt[1]);
      if (fileContent.empty())
         return;
      s_skills.emplace(name, make_shared<AISkill>(name, description, fileContent, supportsWorker, defaultMode));
      return;
   }

   s_skills.emplace(name, make_shared<AISkill>(name, description, prompt, supportsWorker, defaultMode));
}

/**
 * Register AI skill with tools and execution mode settings. This function intended to be called only during server core or module initialization.
 */
void NXCORE_EXPORTABLE RegisterAISkill(const char *name, const char *description, const char *prompt,
   const std::vector<AITool>& tools, bool supportsWorker, SkillExecutionMode defaultMode)
{
   if (s_skills.find(name) != s_skills.end())
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"AI assistant skill \"%hs\" already registered", name);
      return;
   }

   if (prompt[0] == '@')
   {
      std::string fileContent = LoadSkillFile(&prompt[1]);
      if (fileContent.empty())
         return;
      s_skills.emplace(name, make_shared<AISkill>(name, description, fileContent, tools, supportsWorker, defaultMode));
      return;
   }

   s_skills.emplace(name, make_shared<AISkill>(name, description, prompt, tools, supportsWorker, defaultMode));
}

