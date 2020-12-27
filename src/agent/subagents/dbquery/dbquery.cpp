/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: dbquery.cpp
**
**/

#include "dbquery.h"
#include <netxms-version.h>

/**
 * Global variables
 */
CONDITION g_condShutdown;

/**
 * Subagent initialization
 */
static bool SubAgentInit(Config *config)
{
	// Create shutdown condition and start poller threads
   g_condShutdown = ConditionCreate(true);
   StartPollingThreads();
   return true;
}

/**
 * Called by master agent at unload
 */
static void SubAgentShutdown();

/**
 * Parameters supported by agent
 */
static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
   { _T("DB.Query(*)"), H_DirectQuery, nullptr, DCI_DT_STRING, _T("Direct database query result") },
   { _T("DB.QueryResult(*)"), H_PollResult, _T("R"), DCI_DT_STRING, _T("Database query result") },
   { _T("DB.QueryStatus(*)"), H_PollResult, _T("S"), DCI_DT_UINT, _T("Database query status") },
   { _T("DB.QueryStatusText(*)"), H_PollResult, _T("T"), DCI_DT_STRING, _T("Database query status as text") }
};

/**
 * Tables supported by agent
 */
static NETXMS_SUBAGENT_TABLE m_tables[] =
{
   { _T("DB.Query(*)"), H_DirectQueryTable, nullptr, _T(""), _T("Direct database query result") },
   { _T("DB.QueryResult(*)"), H_PollResultTable, nullptr, _T(""), _T("Database query result") }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO m_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
	_T("DBQUERY"), NETXMS_VERSION_STRING,
   SubAgentInit, SubAgentShutdown, nullptr, nullptr,
	0,	nullptr, // parameters
	0, nullptr,	// lists
	0,	nullptr, // tables
   0, nullptr,	// actions
	0, nullptr	// push parameters
};

/**
 * Called by master agent at unload
 */
static void SubAgentShutdown()
{
   MemFree(m_info.parameters);
   MemFree(m_info.tables);
   ConditionSet(g_condShutdown);
   StopPollingThreads();
   ShutdownConnections();
   ConditionDestroy(g_condShutdown);
}

static void AddDCIParam(StructArray<NETXMS_SUBAGENT_PARAM> *parameters, Query *query, bool parameterRequired)
{
   NETXMS_SUBAGENT_PARAM *param = new NETXMS_SUBAGENT_PARAM();

   _tcscpy(param->name, query->getName());
   if (parameterRequired)
      _tcscat(param->name, _T("(*)"));

   param->handler = H_DirectQueryConfigurable;
   param->arg = query->getName();
   param->dataType = DCI_DT_STRING;
   if(query->getDescription() != nullptr)
   {
      _tcscpy(param->description, query->getDescription());
   }
   else
   {
      _tcscpy(param->description, _T(""));
   }

   parameters->add(param);

   delete param;
}

static void AddDCIParamTable(StructArray<NETXMS_SUBAGENT_TABLE> *parametersTable, Query *query, bool parameterRequired)
{
   NETXMS_SUBAGENT_TABLE *param = new NETXMS_SUBAGENT_TABLE();

   _tcscpy(param->name, query->getName());
   if(parameterRequired)
      _tcscat(param->name, _T("(*)"));
   param->handler = H_DirectQueryConfigurableTable;
   param->arg = query->getName();
   _tcscpy(param->instanceColumns, _T(""));
   if(query->getDescription() != nullptr)
   {
      _tcscpy(param->description, query->getDescription());
   }
   else
   {
      _tcscpy(param->description, _T(""));
   }

   parametersTable->add(param);

   delete param;
}

static void AddParameters(StructArray<NETXMS_SUBAGENT_PARAM> *parameters, StructArray<NETXMS_SUBAGENT_TABLE> *parametersTable, Config * config)
{
   // Add database connections
	ConfigEntry *databases = config->getEntry(_T("/DBQuery/Database"));
   if (databases != nullptr)
   {
      for(int i = 0; i < databases->getValueCount(); i++)
		{
         if (!AddDatabaseFromConfig(databases->getValue(i)))
			{
            AgentWriteLog(EVENTLOG_WARNING_TYPE,
               _T("Unable to add database connection from configuration file. ")
									 _T("Original configuration record: %s"), databases->getValue(i));
			}
		}
   }

	// Add queries
	ConfigEntry *queries = config->getEntry(_T("/DBQuery/Query"));
	if (queries != nullptr)
	{
		for(int i = 0; i < queries->getValueCount(); i++)
		{
         Query *query;
			if (AddQueryFromConfig(queries->getValue(i), &query))
			{
            AddDCIParam(parameters, query, false);
            AddDCIParamTable(parametersTable, query, false);
			}
			else
			{
            AgentWriteLog(EVENTLOG_WARNING_TYPE,
                            _T("Unable to add query from configuration file. ")
									 _T("Original configuration record: %s"), queries->getValue(i));
			}
		}
	}

   // Add configurable queries
	ConfigEntry *configurableQueries = config->getEntry(_T("/DBQuery/ConfigurableQuery"));
	if (configurableQueries != nullptr)
	{
		for(int i = 0; i < configurableQueries->getValueCount(); i++)
		{
         Query *query;
			if (AddConfigurableQueryFromConfig(configurableQueries->getValue(i), &query))
			{
            AddDCIParam(parameters, query, true);
            AddDCIParamTable(parametersTable, query, true);
			}
			else
			{
            AgentWriteLog(EVENTLOG_WARNING_TYPE,
                            _T("Unable to add query from configuration file. ")
									 _T("Original configuration record: %s"), configurableQueries->getValue(i));
			}
		}
	}
}

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(DBQUERY)
{
   StructArray<NETXMS_SUBAGENT_PARAM> *parameters = nullptr;
   parameters = new StructArray<NETXMS_SUBAGENT_PARAM>(m_parameters, sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM));
   StructArray<NETXMS_SUBAGENT_TABLE> *parametersTable = nullptr;
   parametersTable = new StructArray<NETXMS_SUBAGENT_TABLE>(m_tables, sizeof(m_tables) / sizeof(NETXMS_SUBAGENT_TABLE));

   AddParameters(parameters, parametersTable, config);

   m_info.numParameters = parameters->size();
   m_info.parameters = MemCopyBlock(parameters->getBuffer(),
                     parameters->size() * sizeof(NETXMS_SUBAGENT_PARAM));

   m_info.numTables = parametersTable->size();
   m_info.tables = MemCopyBlock(parametersTable->getBuffer(),
                     parametersTable->size() * sizeof(NETXMS_SUBAGENT_TABLE));

   delete parameters;
   delete parametersTable;

   *ppInfo = &m_info;
   return true;
}

/**
 * DLL entry point
 */
#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif
