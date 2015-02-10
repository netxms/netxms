/*
 ** NetXMS - Network Management System
 ** Subagent for Oracle monitoring
 ** Copyright (C) 2009-2014 Raden Solutions
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
 **/

#include "mongodb_subagent.h"

/**
 * Database instances
 */
ObjectArray<DatabaseInstance> *g_instances;
static NETXMS_SUBAGENT_PARAM *s_parameters;

static BOOL SubAgentInit(Config *config);
static void SubAgentShutdown();
static LONG H_Databases(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session);

/**
 * Supported lists
 */
static NETXMS_SUBAGENT_LIST s_lists[] =
{
   { _T("MongoDB.ListDatabases(*)"), H_Databases, NULL }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO s_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("MONGODB"), NETXMS_VERSION_STRING,
	SubAgentInit, SubAgentShutdown, NULL,
	0,	NULL, //parameters
	sizeof(s_lists) / sizeof(NETXMS_SUBAGENT_LIST), s_lists,
	0,	NULL, //tables
	0,	NULL,    // actions
	0,	NULL     // push parameters
};

void MongoLogging(mongoc_log_level_t  log_level, const char *log_domain, const char *message, void *user_data)
{
   int severity = NXLOG_DEBUG;
   switch(log_level)
   {
      case MONGOC_LOG_LEVEL_ERROR:
      case MONGOC_LOG_LEVEL_CRITICAL:
         severity = NXLOG_ERROR;
         break;
      case MONGOC_LOG_LEVEL_WARNING:
         severity = NXLOG_WARNING;
         break;
      case MONGOC_LOG_LEVEL_MESSAGE:
      case MONGOC_LOG_LEVEL_INFO:
         severity = NXLOG_INFO;
         break;
      case MONGOC_LOG_LEVEL_DEBUG:
      case MONGOC_LOG_LEVEL_TRACE:
         severity  = NXLOG_DEBUG;
         break;
   }
#ifdef UNICODE
   TCHAR *_message = WideStringFromUTF8String(message);
   TCHAR *_log_domain = WideStringFromUTF8String(log_domain);
   AgentWriteLog(severity, _T("MONGODB: Driver message: Domain: %s. Message: %s"), _log_domain, _message);
   safe_free(_message);
   safe_free(_log_domain);
#else
   AgentWriteLog(severity, _T("MONGODB: Internal error. Domain: %s. Message: %s"),log_domain, message);
#endif // UNICODE
}

/**
 * Subagent initialization
 */
static BOOL SubAgentInit(Config *config)
{
	return TRUE;
}

/**
 * Shutdown handler
 */
static void SubAgentShutdown()
{
	AgentWriteDebugLog(NXLOG_INFO, _T("MONGODB: stopping pollers"));
   for(int i = 0; i < g_instances->size(); i++)
      g_instances->get(i)->stop();
	mongoc_cleanup();
	//remove names form instances
   delete g_instances;
	AgentWriteDebugLog(NXLOG_INFO, _T("MONGODB: stopped"));
}

/**
 * Load configuration, start connection
 */
static BOOL LoadConfiguration(Config *config)
{
   g_instances = new ObjectArray<DatabaseInstance>(8, 8, true);
   mongoc_init();
   mongoc_log_set_handler(MongoLogging, NULL);

   // Add database connections
	ConfigEntry *databases = config->getEntry(_T("/MongoDB/Database"));
   if (databases != NULL)
   {
      for(int i = 0; i < databases->getValueCount(); i++)
		{
         if (!AddMongoDBFromConfig(databases->getValue(i)))
			{
            AgentWriteLog(NXLOG_WARNING,
               _T("MONGODB: Unable to add database connection from configuration file. ")
									 _T("Original configuration record: %s"), databases->getValue(i));
			}
		}
   }

	// Exit if no usable configuration found
   if (g_instances->size() == 0)
	{
      AgentWriteLog(NXLOG_WARNING, _T("MONGODB: no databases to monitor, exiting"));
      delete g_instances;
      mongoc_cleanup();
      return FALSE;
	}

   //create list
   int paramCount;
   for(int i = 0; i < g_instances->size(); i++)
   {
      s_parameters = g_instances->get(i)->getParameters(&paramCount);
      if(s_parameters != NULL)
         break;
   }

   if(s_parameters == NULL)
   {
      delete g_instances;
      mongoc_cleanup();
      AgentWriteLog(NXLOG_WARNING, _T("MONGODB: No parameters will be added. Not possible to get any status."));
      return FALSE;
   }

	for(int i = 0; i < g_instances->size(); i++)
      g_instances->get(i)->run();

   s_info.numParameters = paramCount;
   s_info.parameters = s_parameters;
   return TRUE;
}

/**
 * Get list of databases
 */
static LONG H_Databases(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   LONG result = SYSINFO_RC_UNSUPPORTED;
   TCHAR id[MAX_STR];
   if (!AgentGetParameterArg(param, 1, id, MAX_STR))
      return SYSINFO_RC_UNSUPPORTED;

   for(int i = 0; i < g_instances->size(); i++)
   {
      if(!_tcsicmp(g_instances->get(i)->getConnectionName(), id))
      {
         result = g_instances->get(i)->setDbNames(value);
      }
   }
   return result;
}

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(MONGODB)
{
   if (s_info.parameters != NULL)
      return FALSE;  // Most likely another instance of MONGODB subagent already loaded

   BOOL result = LoadConfiguration(config);

	*ppInfo = &s_info;
	return result;
}

#ifdef _WIN32

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif

