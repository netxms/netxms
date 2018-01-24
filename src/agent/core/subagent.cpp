/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2018 Victor Kirhenshtein
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

/**
 * Subagent list
 */
static StructArray<SUBAGENT> s_subAgents(8, 8);

/**
 * Initialize subagent
 */
static bool InitSubAgent(HMODULE hModule, const TCHAR *moduleName, bool (* SubAgentRegister)(NETXMS_SUBAGENT_INFO **, Config *))
{
   bool success = false;

   NETXMS_SUBAGENT_INFO *pInfo;
   if (SubAgentRegister(&pInfo, g_config))
   {
      // Check if information structure is valid
      if (pInfo->magic == NETXMS_SUBAGENT_INFO_MAGIC)
      {
         // Check if subagent with given name already loaded
         int i;
         for(i = 0; i < s_subAgents.size(); i++)
            if (!_tcsicmp(s_subAgents.get(i)->pInfo->name, pInfo->name))
               break;
         if (i == s_subAgents.size())
         {
				// Initialize subagent
				bool initOK = (pInfo->init != NULL) ? pInfo->init(g_config) : true;
				if (initOK)
				{
					// Add subagent to subagent's list
				   SUBAGENT s;
					s.hModule = hModule;
					_tcslcpy(s.szName, moduleName, MAX_PATH);
					s.pInfo = pInfo;
					s_subAgents.add(&s);

					// Add parameters provided by this subagent to common list
					for(i = 0; i < (int)pInfo->numParameters; i++)
						AddParameter(pInfo->parameters[i].name,
										 pInfo->parameters[i].handler,
										 pInfo->parameters[i].arg,
										 pInfo->parameters[i].dataType,
										 pInfo->parameters[i].description);

					// Add push parameters provided by this subagent to common list
					for(i = 0; i < (int)pInfo->numPushParameters; i++)
						AddPushParameter(pInfo->pushParameters[i].name,
						                 pInfo->pushParameters[i].dataType,
					                    pInfo->pushParameters[i].description);

					// Add lists provided by this subagent to common list
					for(i = 0; i < (int)pInfo->numLists; i++)
						AddList(pInfo->lists[i].name,
								  pInfo->lists[i].handler,
								  pInfo->lists[i].arg);

					// Add tables provided by this subagent to common list
					for(i = 0; i < (int)pInfo->numTables; i++)
						AddTable(pInfo->tables[i].name,
								   pInfo->tables[i].handler,
								   pInfo->tables[i].arg,
									pInfo->tables[i].instanceColumns,
									pInfo->tables[i].description,
                           pInfo->tables[i].numColumns,
                           pInfo->tables[i].columns);

					// Add actions provided by this subagent to common list
					for(i = 0; i < (int)pInfo->numActions; i++)
						AddAction(pInfo->actions[i].name,
									 AGENT_ACTION_SUBAGENT,
									 pInfo->actions[i].arg,
									 pInfo->actions[i].handler,
									 pInfo->name,
									 pInfo->actions[i].description);

					nxlog_write(MSG_SUBAGENT_LOADED, NXLOG_INFO, "sss", pInfo->name, moduleName, pInfo->version);
					success = true;
				}
				else
				{
					nxlog_write(MSG_SUBAGENT_INIT_FAILED, NXLOG_ERROR, "ss", pInfo->name, moduleName);
					DLClose(hModule);
				}
         }
         else
         {
            nxlog_write(MSG_SUBAGENT_ALREADY_LOADED, EVENTLOG_WARNING_TYPE,
                        "ss", pInfo->name, s_subAgents.get(i)->szName);
            DLClose(hModule);
         }
      }
      else
      {
         nxlog_write(MSG_SUBAGENT_BAD_MAGIC, NXLOG_ERROR, "s", moduleName);
         DLClose(hModule);
      }
   }
   else
   {
      nxlog_write(MSG_SUBAGENT_REGISTRATION_FAILED, NXLOG_ERROR, "s", moduleName);
      DLClose(hModule);
   }

   return success;
}

/**
 * Load subagent
 */
bool LoadSubAgent(const TCHAR *moduleName)
{
   bool success = FALSE;

   TCHAR errorText[256];
#ifdef _WIN32
   HMODULE hModule = DLOpen(moduleName, errorText);
#else
   TCHAR fullName[MAX_PATH];
   if (_tcschr(moduleName, _T('/')) == NULL)
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
   HMODULE hModule = DLOpen(fullName, errorText);
#endif

   if (hModule != NULL)
   {
      bool (* SubAgentRegister)(NETXMS_SUBAGENT_INFO **, Config *);
      SubAgentRegister = (bool (*)(NETXMS_SUBAGENT_INFO **, Config *))DLGetSymbolAddr(hModule, "NxSubAgentRegister", errorText);
      if (SubAgentRegister != NULL)
      {
         success = InitSubAgent(hModule, moduleName, SubAgentRegister);
      }
      else
      {
         nxlog_write(MSG_NO_SUBAGENT_ENTRY_POINT, EVENTLOG_ERROR_TYPE, "s", moduleName);
         DLClose(hModule);
      }
   }
   else
   {
      nxlog_write(MSG_SUBAGENT_LOAD_FAILED, EVENTLOG_ERROR_TYPE, "ss", moduleName, errorText);
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
      if (s->pInfo->shutdown != NULL)
         s->pInfo->shutdown();
      DLClose(s->hModule);
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
                 s->pInfo->name, s->pInfo->version, CAST_FROM_POINTER(s->hModule, QWORD), s->szName);
#else
      _sntprintf(buffer, MAX_PATH + 32, _T("%s %s 0x%08X %s"),
                 s->pInfo->name, s->pInfo->version, CAST_FROM_POINTER(s->hModule, UINT32), s->szName);
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
   value->addColumn(_T("NAME"));
   value->addColumn(_T("VERSION"));
   value->addColumn(_T("FILE"));

   for(int i = 0; i < s_subAgents.size(); i++)
   {
      value->addRow();
      value->set(0, s_subAgents.get(i)->pInfo->name);
      value->set(1, s_subAgents.get(i)->pInfo->version);
      value->set(2, s_subAgents.get(i)->szName);
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
		if (!_tcsicmp(name, s_subAgents.get(i)->pInfo->name))
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
bool ProcessCommandBySubAgent(UINT32 command, NXCPMessage *request, NXCPMessage *response, AbstractCommSession *session)
{
   bool processed = false;
   for(int i = 0; (i < s_subAgents.size()) && (!processed); i++)
   {
      NETXMS_SUBAGENT_INFO *s = s_subAgents.get(i)->pInfo;
      if (s->commandHandler != NULL)
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
void NotifySubAgents(UINT32 code, void *data)
{
   for(int i = 0; i < s_subAgents.size(); i++)
   {
      NETXMS_SUBAGENT_INFO *s = s_subAgents.get(i)->pInfo;
      if (s->notify != NULL)
      {
         s->notify(code, data);
      }
   }
}
