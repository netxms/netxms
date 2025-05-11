/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Raden Solutions
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
 * Shutdown condition
 */
Condition g_condShutdown(true);

/**
 * Flag for allowing empty result sets
 */
bool g_allowEmptyResultSet = true;

/**
 * Subagent initialization
 */
static bool SubAgentInit(Config *config)
{
   g_allowEmptyResultSet = config->getValueAsBoolean(_T("/DBQuery/AllowEmptyResultSet"), true);
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
   { _T("DB.QueryExecutionTime(*)"), H_PollResult, _T("E"), DCI_DT_UINT, _T("Database query execution time") },
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
static NETXMS_SUBAGENT_INFO s_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
	_T("DBQUERY"), NETXMS_VERSION_STRING,
   SubAgentInit, SubAgentShutdown, nullptr, nullptr, nullptr,
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
   MemFree(s_info.parameters);
   MemFree(s_info.tables);
   g_condShutdown.set();
   StopPollingThreads();
   ShutdownConnections();
}

/**
 * Add metric
 */
static void AddMetric(StructArray<NETXMS_SUBAGENT_PARAM> *metrics, Query *query, bool withArguments)
{
   NETXMS_SUBAGENT_PARAM metric;
   memset(&metric, 0, sizeof(metric));

   _tcscpy(metric.name, query->getName());
   if (withArguments)
      _tcscat(metric.name, _T("(*)"));

   metric.handler = H_DirectQueryConfigurable;
   metric.arg = query->getName();
   metric.dataType = DCI_DT_STRING;
   if (query->getDescription() != nullptr)
   {
      _tcslcpy(metric.description, query->getDescription(), MAX_DB_STRING);
   }

   metrics->add(metric);
}

/**
 * Add table
 */
static void AddTable(StructArray<NETXMS_SUBAGENT_TABLE> *tables, Query *query, bool withArguments)
{
   NETXMS_SUBAGENT_TABLE table;
   memset(&table, 0, sizeof(table));

   _tcscpy(table.name, query->getName());
   if (withArguments)
      _tcscat(table.name, _T("(*)"));
   table.handler = H_DirectQueryConfigurableTable;
   table.arg = query->getName();
   _tcscpy(table.instanceColumns, _T(""));
   if (query->getDescription() != nullptr)
   {
      _tcscpy(table.description, query->getDescription());
   }

   tables->add(table);
}

/**
 * Add metrics
 */
static void AddMetrics(StructArray<NETXMS_SUBAGENT_PARAM> *metrics, StructArray<NETXMS_SUBAGENT_TABLE> *tables, Config *config)
{
   // Add database connections from old ini-style configs
	ConfigEntry *dbIniConfigs = config->getEntry(_T("/DBQuery/Database"));
   if (dbIniConfigs != nullptr)
   {
      for(int i = 0; i < dbIniConfigs->getValueCount(); i++)
		{
         const TCHAR *dbConfig = dbIniConfigs->getValue(i);
         if (*dbConfig != 0)
         {
            if (!AddDatabaseFromConfig(dbConfig))
            {
               nxlog_write_tag(NXLOG_WARNING, DBQUERY_DEBUG_TAG, _T("Unable to add database connection from configuration file. Original configuration record: %s"), dbConfig);
            }
         }
		}
   }

   // Add database connections from new hierarchical configs
   unique_ptr<ObjectArray<ConfigEntry>> dbConfigs = config->getSubEntries(_T("/DBQuery/Databases"));
   if (dbConfigs != nullptr)
   {
      for(int i = 0; i < dbConfigs->size(); i++)
      {
         ConfigEntry *ce = dbConfigs->get(i);
         if (!AddDatabaseFromConfig(*ce))
         {
            nxlog_write_tag(NXLOG_WARNING, DBQUERY_DEBUG_TAG, _T("Unable to add database connection from configuration file entry \"/DBQuery/Databases/%s\""), ce->getName());
         }
      }
   }

	// Add queries
	ConfigEntry *queries = config->getEntry(_T("/DBQuery/Query"));
	if (queries != nullptr)
	{
		for(int i = 0; i < queries->getValueCount(); i++)
		{
		   const TCHAR *queryConfig = queries->getValue(i);
         Query *query;
			if (AddQueryFromConfig(queryConfig, &query))
			{
            AddMetric(metrics, query, false);
            AddTable(tables, query, false);
			}
			else
			{
			   nxlog_write_tag(NXLOG_WARNING, DBQUERY_DEBUG_TAG, _T("Unable to add query from configuration file. Original configuration record: %s"), queryConfig);
			}
		}
	}

   // Add configurable queries
	ConfigEntry *configurableQueries = config->getEntry(_T("/DBQuery/ConfigurableQuery"));
	if (configurableQueries != nullptr)
	{
		for(int i = 0; i < configurableQueries->getValueCount(); i++)
		{
         const TCHAR *queryConfig = configurableQueries->getValue(i);
         Query *query;
			if (AddConfigurableQueryFromConfig(queryConfig, &query))
			{
            AddMetric(metrics, query, true);
            AddTable(tables, query, true);
			}
			else
			{
			   nxlog_write_tag(NXLOG_WARNING, DBQUERY_DEBUG_TAG, _T("Unable to add query from configuration file. Original configuration record: %s"), queryConfig);
			}
		}
	}
}

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(DBQUERY)
{
   StructArray<NETXMS_SUBAGENT_PARAM> metrics(m_parameters, sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM));
   StructArray<NETXMS_SUBAGENT_TABLE> tables(m_tables, sizeof(m_tables) / sizeof(NETXMS_SUBAGENT_TABLE));

   AddMetrics(&metrics, &tables, config);

   s_info.numParameters = metrics.size();
   s_info.parameters = MemCopyBlock(metrics.getBuffer(), metrics.size() * sizeof(NETXMS_SUBAGENT_PARAM));

   s_info.numTables = tables.size();
   s_info.tables = MemCopyBlock(tables.getBuffer(), tables.size() * sizeof(NETXMS_SUBAGENT_TABLE));

   *ppInfo = &s_info;
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
