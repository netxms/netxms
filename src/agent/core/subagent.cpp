/*
** NetXMS multiplatform core agent
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
** File: subagent.cpp
**
**/

#include "nxagentd.h"

#define DEBUG_TAG _T("subagents")

/**
 * Subagent list
 */
static StructArray<SUBAGENT> s_subAgents(8, 8);

/**
 * Subagent aliases
 */
static struct
{
   const TCHAR *alias;
   const TCHAR *realName;
   const TCHAR *reason;
} s_aliases[] =
{
   { _T("ecs.nsm"), _T("netsvc.nsm"), _T("ECS subagent is superseded by NETSVC subagent") },
   { _T("portcheck.nsm"), _T("netsvc.nsm"), _T("PORTCHECK subagent is superseded by NETSVC subagent") },
   { nullptr, nullptr, nullptr }
};

/**
 * Check if given subagent name is an alias and has to be replaced
 */
static void CheckAlias(TCHAR *name)
{
   TCHAR *s = _tcsrchr(name, FS_PATH_SEPARATOR_CHAR);
   if (s != nullptr)
      s++;
   else
      s = name;

   for(int i = 0; s_aliases[i].alias != nullptr; i++)
   {
      if (!_tcsicmp(s, s_aliases[i].alias))
      {
         TCHAR key[128], message[1024];

         _tcscpy(key, _T("subagent-subst-"));
         _tcscat(key, s_aliases[i].alias);

         _sntprintf(message, 1024, _T("Subagent module %s was substituted with %s (%s)"), s_aliases[i].alias, s_aliases[i].realName, s_aliases[i].reason);

         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("%s"), message);
         RegisterProblem(SEVERITY_WARNING, key, message);

         _tcscpy(s, s_aliases[i].realName);
         break;
      }
   }
}

/**
 * Initialize subagent
 */
bool InitSubAgent(HMODULE hModule, const TCHAR *moduleName, bool (*SubAgentRegister)(NETXMS_SUBAGENT_INFO**, Config*))
{
   bool success = false;

   NETXMS_SUBAGENT_INFO *info;
   shared_ptr<Config> config = g_config;
   if (SubAgentRegister(&info, config.get()))
   {
      // Check if information structure is valid
      if (info->magic == NETXMS_SUBAGENT_INFO_MAGIC)
      {
         // Check if subagent with given name already loaded
         SUBAGENT *sa = nullptr;
         for(int i = 0; i < s_subAgents.size(); i++)
            if (!_tcsicmp(s_subAgents.get(i)->info->name, info->name))
            {
               sa = s_subAgents.get(i);
               break;
            }
         if (sa == nullptr)
         {
				// Initialize subagent
				bool initOK = (info->init != nullptr) ? info->init(config.get()) : true;
				if (initOK)
				{
					// Add subagent to subagent's list
				   SUBAGENT s;
					s.moduleHandle = hModule;
					_tcslcpy(s.moduleName, moduleName, MAX_PATH);
					s.info = info;
					s_subAgents.add(&s);

					// Add parameters provided by this subagent to common list
					for(size_t i = 0; i < info->numParameters; i++)
					   AddMetric(info->parameters[i].name, info->parameters[i].handler, info->parameters[i].arg, info->parameters[i].dataType, info->parameters[i].description, info->mericFilter);

					// Add push parameters provided by this subagent to common list
					for(size_t i = 0; i < info->numPushParameters; i++)
						AddPushMetric(info->pushParameters[i].name, info->pushParameters[i].dataType, info->pushParameters[i].description);

					// Add lists provided by this subagent to common list
					for(size_t i = 0; i < info->numLists; i++)
						AddList(info->lists[i].name, info->lists[i].handler, info->lists[i].arg, info->mericFilter);

					// Add tables provided by this subagent to common list
					for(size_t i = 0; i < info->numTables; i++)
						AddTable(info->tables[i].name,
								   info->tables[i].handler,
								   info->tables[i].arg,
									info->tables[i].instanceColumns,
									info->tables[i].description,
                           info->tables[i].numColumns,
                           info->tables[i].columns,
                           info->mericFilter);

					// Add actions provided by this subagent to common list
					for(size_t i = 0; i < info->numActions; i++)
						AddAction(info->actions[i].name, false, info->actions[i].arg, info->actions[i].handler, info->name, info->actions[i].description);

					nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Subagent \"%s\" (%s) loaded successfully (version %s)"), info->name, moduleName, info->version);
					success = true;
				}
				else
				{
					nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Initialization of subagent \"%s\" (%s) failed"), info->name, moduleName);
					DLClose(hModule);
				}
         }
         else
         {
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Subagent \"%s\" already loaded from module \"%s\""), info->name, sa->moduleName);
            DLClose(hModule);
         }
      }
      else
      {
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Subagent \"%s\" has invalid magic number - probably it was compiled for different agent version"), moduleName);
         DLClose(hModule);
      }
   }
   else
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Registration of subagent \"%s\" failed"), moduleName);
      DLClose(hModule);
   }

   return success;
}

/**
 * Load subagent
 */
bool LoadSubAgent(const TCHAR *moduleName)
{
   bool success = false;

   TCHAR errorText[256], fullName[MAX_PATH];
#ifdef _WIN32
   _tcslcpy(fullName, moduleName, MAX_PATH);
   size_t len = _tcslen(fullName);
   if ((len < 4) || (_tcsicmp(&fullName[len - 4], _T(".nsm")) && _tcsicmp(&fullName[len - 4], _T(".dll"))))
      _tcslcat(fullName, _T(".nsm"), MAX_PATH);
   CheckAlias(fullName);
   HMODULE hModule = DLOpen(fullName, errorText);
#else
   if (_tcschr(moduleName, _T('/')) == nullptr)
   {
      // Assume that subagent name without path given
      // Try to load it from pkglibdir
      TCHAR libDir[MAX_PATH];
      GetNetXMSDirectory(nxDirLib, libDir);
      _sntprintf(fullName, MAX_PATH, _T("%s/%s"), libDir, moduleName);
   }
   else
   {
      _tcslcpy(fullName, moduleName, MAX_PATH);
   }

   size_t len = _tcslen(fullName);
   if ((len < 4) || (_tcsicmp(&fullName[len - 4], _T(".nsm")) && _tcsicmp(&fullName[len - _tcslen(SHLIB_SUFFIX)], SHLIB_SUFFIX)))
      _tcslcat(fullName, _T(".nsm"), MAX_PATH);
   CheckAlias(fullName);

   // Workaround for Python sub-agent: dlopen it with RTLD_GLOBAL
   // so that Python C modules can access symbols from libpython
   HMODULE hModule = DLOpenEx(fullName, _tcsstr(fullName, _T("python.nsm")) != nullptr, errorText);
#endif

   if (hModule != nullptr)
   {
      bool (*SubAgentRegister)(NETXMS_SUBAGENT_INFO**, Config*);
      SubAgentRegister = (bool (*)(NETXMS_SUBAGENT_INFO**, Config*))DLGetSymbolAddr(hModule, "NxSubAgentRegister", errorText);
      if (SubAgentRegister != nullptr)
      {
         success = InitSubAgent(hModule, fullName, SubAgentRegister);
      }
      else
      {
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Unable to find entry point in subagent module \"%s\""), moduleName);
         DLClose(hModule);
      }
   }
   else
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Error loading subagent module \"%s\" (%s)"), moduleName, errorText);
   }

   if (!success)
   {
      TCHAR problemKey[256];
      _sntprintf(problemKey, 256, _T("subagent-load-%s"), moduleName);

      TCHAR message[256];
      _sntprintf(message, 256, _T("Cannot load subagent module \"%s\""), moduleName);

      RegisterProblem(SEVERITY_MAJOR, problemKey, message);
   }

   return success;
}

/**
 * Unload all subagents.
 * This function should be called on shutdown, so we don't care
 * about deregistering parameters and so on.
 */
void UnloadAllSubAgents()
{
   for(int i = 0; i < s_subAgents.size(); i++)
   {
      SUBAGENT *s = s_subAgents.get(i);
      if (s->info->shutdown != nullptr)
         s->info->shutdown();
      DLClose(s->moduleHandle);
   }
}

/**
 * Enumerate loaded subagents
 */
LONG H_SubAgentList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   TCHAR buffer[MAX_PATH + 32];
   for(int i = 0; i < s_subAgents.size(); i++)
   {
      SUBAGENT *s = s_subAgents.get(i);
#ifdef __64BIT__
      _sntprintf(buffer, MAX_PATH + 32, _T("%s %s 0x") UINT64X_FMT(_T("016")) _T(" %s"),
                 s->info->name, s->info->version, CAST_FROM_POINTER(s->moduleHandle, uint64_t), s->moduleName);
#else
      _sntprintf(buffer, MAX_PATH + 32, _T("%s %s 0x%08X %s"),
                 s->info->name, s->info->version, CAST_FROM_POINTER(s->moduleHandle, uint32_t), s->moduleName);
#endif
      value->add(buffer);
   }
   return SYSINFO_RC_SUCCESS;
}

/**
 * Enumerate loaded subagents as a table
 */
LONG H_SubAgentTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"), true);
   value->addColumn(_T("VERSION"), DCI_DT_STRING, _T("Version"));
   value->addColumn(_T("FILE"), DCI_DT_STRING, _T("File"));

   for(int i = 0; i < s_subAgents.size(); i++)
   {
      value->addRow();
      SUBAGENT *s = s_subAgents.get(i);
      value->set(0, s->info->name);
      value->set(1, s->info->version);
      value->set(2, s->moduleName);
   }
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Agent.IsSubagentLoaded
 */
LONG H_IsSubagentLoaded(const TCHAR *pszCmd, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
	TCHAR name[256];
	AgentGetParameterArg(pszCmd, 1, name, 256);
	int rc = 0;
   for(int i = 0; i < s_subAgents.size(); i++)
   {
		if (!_tcsicmp(name, s_subAgents.get(i)->info->name))
		{
			rc = 1;
			break;
		}
	}
	ret_int(pValue, rc);
	return SYSINFO_RC_SUCCESS;
}

/**
 * Process unknown command by subagents
 */
bool ProcessCommandBySubAgent(uint32_t command, NXCPMessage *request, NXCPMessage *response, AbstractCommSession *session)
{
   bool processed = false;
   for(int i = 0; (i < s_subAgents.size()) && !processed; i++)
   {
      NETXMS_SUBAGENT_INFO *s = s_subAgents.get(i)->info;
      if (s->commandHandler != nullptr)
      {
         processed = s->commandHandler(command, request, response, session);
         session->debugPrintf(7, _T("Command %sprocessed by sub-agent %s"), 
            processed ? _T("") : _T("not "), s->name);
      }
   }
   return processed;
}

/**
 * Notify all sub-agents
 */
void NotifySubAgents(uint32_t code, void *data)
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("NotifySubAgents(): processing notification with code %u"), code);
   for(int i = 0; i < s_subAgents.size(); i++)
   {
      NETXMS_SUBAGENT_INFO *s = s_subAgents.get(i)->info;
      if (s->notify != nullptr)
      {
         s->notify(code, data);
      }
   }
}
