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
** File: skills.cpp
**
**/

#include "nxcore.h"
#include <iris.h>
#include <unordered_map>

#define DEBUG_TAG _T("ai.skills")

json_t *RebuildFunctionDeclarations(const std::unordered_map<std::string, shared_ptr<AssistantFunction>>& functions);

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
   wcslcat(path, L"/skills/", MAX_PATH);
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
 * Get list of registered AI assistant skills
 */
std::string F_GetRegisteredSkills(json_t *arguments, uint32_t userId)
{
   json_t *output = json_array();

   s_skillsMutex.lock();
   for(const auto& pair : s_skills)
   {
      const AssistantSkill& skill = *pair.second;
      json_t *skillObject = json_object();
      json_object_set_new(skillObject, "name", json_string(skill.name.c_str()));
      json_object_set_new(skillObject, "description", json_string(skill.description.c_str()));
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
      nxlog_debug_tag(DEBUG_TAG, 6, L"Loaded AI assistant function \"%s\" from skill \"%s\"", function.name.c_str(), skill.name.c_str());
   }
   s_skillsMutex.unlock();

   json_decref(m_functionDeclarations);
   m_functionDeclarations = RebuildFunctionDeclarations(m_functions);

   return skill.prompt;
}
