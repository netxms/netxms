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
#include <unordered_map>

#define DEBUG_TAG _T("ai.skills")

json_t *RebuildFunctionDeclarations(const std::unordered_map<std::string, shared_ptr<AssistantFunction>>& functions);

/**
 * System prompt for delegated skill execution
 */
static const char *s_delegationSystemPrompt =
   "You are executing a delegated task as a specialized agent within NetXMS. "
   "Complete the task described below using the available functions. "
   "When done, provide a concise summary of what was accomplished and any relevant results. "
   "Do not ask questions - make reasonable decisions based on the information provided. "
   "Focus only on the specific task given.";

/**
 * Skills
 */
static std::unordered_map<std::string, shared_ptr<AssistantSkill>> s_skills;
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
 * Register AI assistant skill. This function intended to be called only during server core or module initialization.
 */
void NXCORE_EXPORTABLE RegisterAIAssistantSkill(const char *name, const char *description, const char *prompt)
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
      s_skills.emplace(name, make_shared<AssistantSkill>(name, description, fileContent));
      return;
   }

   s_skills.emplace(name, make_shared<AssistantSkill>(name, description, prompt));
}

/**
 * Register AI assistant skill with functions. This function intended to be called only during server core or module initialization.
 */
void NXCORE_EXPORTABLE RegisterAIAssistantSkill(const char *name, const char *description, const char *prompt, const std::vector<AssistantFunction>& functions)
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
      s_skills.emplace(name, make_shared<AssistantSkill>(name, description, fileContent, functions));
      return;
   }

   s_skills.emplace(name, make_shared<AssistantSkill>(name, description, prompt, functions));
}

/**
 * Get list of registered AI assistant skills as string
 */
std::string GetRegisteredSkills()
{
   json_t *output = json_array();

   s_skillsMutex.lock();
   for(const auto& pair : s_skills)
   {
      const AssistantSkill& skill = *pair.second;
      json_t *skillObject = json_object();
      json_object_set_new(skillObject, "name", json_string(skill.name.c_str()));
      json_object_set_new(skillObject, "description", json_string(skill.description.c_str()));
      json_object_set_new(skillObject, "supports_delegation", json_boolean(skill.supportsDelegation));
      json_object_set_new(skillObject, "default_mode", json_string(skill.defaultMode == SkillExecutionMode::DELEGATED ? "delegated" : "loaded"));
      json_array_append_new(output, skillObject);
   }
   s_skillsMutex.unlock();

   return JsonToString(output);
}

/**
 * Load AI assistant skill into chat context
 */
std::string Chat::loadSkill(const char *skillName)
{
   s_skillsMutex.lock();
   auto it = s_skills.find(skillName);
   if (it == s_skills.end())
   {
      s_skillsMutex.unlock();
      return std::string("Error: skill not found");
   }

   const AssistantSkill& skill = *it->second;

   // Register skill functions
   for(const AssistantFunction& function : skill.functions)
   {
      m_functions.emplace(function.name, make_shared<AssistantFunction>(function));
      nxlog_debug_tag(DEBUG_TAG, 6, L"Loaded AI assistant function \"%hs\" from skill \"%hs\"", function.name.c_str(), skill.name.c_str());
   }
   s_skillsMutex.unlock();

   json_decref(m_functionDeclarations);
   m_functionDeclarations = RebuildFunctionDeclarations(m_functions);

   return skill.prompt;
}

/**
 * Delegate task to a skill running in a separate ephemeral chat context
 */
std::string Chat::delegateToSkill(const char *skillName, const char *task)
{
   s_skillsMutex.lock();
   auto it = s_skills.find(skillName);
   if (it == s_skills.end())
   {
      s_skillsMutex.unlock();
      return std::string("Error: skill not found");
   }

   shared_ptr<AssistantSkill> skill = it->second;
   if (!skill->supportsDelegation)
   {
      s_skillsMutex.unlock();
      return std::string("Error: skill does not support delegation");
   }
   s_skillsMutex.unlock();

   nxlog_debug_tag(DEBUG_TAG, 4, L"Delegating task to skill \"%hs\"", skillName);

   // Create temporary non-interactive chat with delegation system prompt
   Chat delegationChat(nullptr, nullptr, m_userId, s_delegationSystemPrompt, false);
   delegationChat.setSlot("delegation");

   // Remove delegate-to-skill from delegation chat to prevent nesting
   delegationChat.m_functions.erase("delegate-to-skill");
   json_decref(delegationChat.m_functionDeclarations);
   delegationChat.m_functionDeclarations = RebuildFunctionDeclarations(delegationChat.m_functions);

   // Load skill into delegation chat
   std::string loadResult = delegationChat.loadSkill(skillName);
   if (loadResult.find("Error:") == 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"Failed to load skill \"%hs\" into delegation chat: %hs", skillName, loadResult.c_str());
      return loadResult;
   }

   // Add skill prompt as system message
   delegationChat.addMessage("system", loadResult.c_str());

   // Execute the task
   char *result = delegationChat.sendRequest(task, 16);
   std::string output;
   if (result != nullptr)
   {
      output = result;
      MemFree(result);
      nxlog_debug_tag(DEBUG_TAG, 4, L"Skill \"%hs\" delegation completed successfully", skillName);
   }
   else
   {
      output = "Error: delegation failed - no response from skill";
      nxlog_debug_tag(DEBUG_TAG, 4, L"Skill \"%hs\" delegation failed - no response", skillName);
   }

   return output;
}

/**
 * Register AI assistant skill with delegation support. This function intended to be called only during server core or module initialization.
 */
void NXCORE_EXPORTABLE RegisterAIAssistantSkill(const char *name, const char *description, const char *prompt,
   bool supportsDelegation, SkillExecutionMode defaultMode)
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
      s_skills.emplace(name, make_shared<AssistantSkill>(name, description, fileContent, supportsDelegation, defaultMode));
      return;
   }

   s_skills.emplace(name, make_shared<AssistantSkill>(name, description, prompt, supportsDelegation, defaultMode));
}

/**
 * Register AI assistant skill with functions and delegation support. This function intended to be called only during server core or module initialization.
 */
void NXCORE_EXPORTABLE RegisterAIAssistantSkill(const char *name, const char *description, const char *prompt,
   const std::vector<AssistantFunction>& functions, bool supportsDelegation, SkillExecutionMode defaultMode)
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
      s_skills.emplace(name, make_shared<AssistantSkill>(name, description, fileContent, functions, supportsDelegation, defaultMode));
      return;
   }

   s_skills.emplace(name, make_shared<AssistantSkill>(name, description, prompt, functions, supportsDelegation, defaultMode));
}

