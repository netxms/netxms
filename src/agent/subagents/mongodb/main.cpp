/*
 ** NetXMS - Network Management System
 ** Subagent for MongoDB monitoring
 ** Copyright (C) 2009-2020 Raden Solutions
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
#include <netxms-version.h>

/**
 * Database connections
 */
ObjectArray<DatabaseConnection> *g_connections;
static NETXMS_SUBAGENT_PARAM *s_parameters;

static bool SubAgentInit(Config *config);
static void SubAgentShutdown();
static LONG H_Databases(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session);
static LONG H_AllDatabases(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session);
static LONG H_ConnectionsList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session);
static LONG H_GetOtherParam(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);

/**
 * Supported lists
 */
static NETXMS_SUBAGENT_LIST s_lists[] =
{
   { _T("MongoDB.AllDatabases"), H_AllDatabases, nullptr, _T("MongoDB: all discovered databases across connections") },
   { _T("MongoDB.Connections"), H_ConnectionsList, nullptr, _T("MongoDB: configured database connections") },
   { _T("MongoDB.Databases(*)"), H_Databases, nullptr, _T("MongoDB: databases on connection") },
   { _T("MongoDB.ListDatabases(*)"), H_Databases, nullptr, _T("MongoDB: databases on connection (deprecated, use MongoDB.Databases)") }
};

/**
 * Supported parameters
 */
static NETXMS_SUBAGENT_PARAM s_parametersPredef[] =
{
	{ _T("MongoDB.collectionsNum(*)"), H_GetOtherParam, _T("collections"), DCI_DT_STRING, _T("Contains a count of the number of collections in that database.") },
	{ _T("MongoDB.objectsNum(*)"), H_GetOtherParam, _T("objects"), DCI_DT_STRING, _T("Contains a count of the number of objects (i.e. documents) in the database across all collections.") },
   { _T("MongoDB.avgObjSize(*)"), H_GetOtherParam, _T("avgObjSize"), DCI_DT_STRING, _T("The average size of each document in bytes.") },
   { _T("MongoDB.dataSize(*)"), H_GetOtherParam, _T("dataSize"), DCI_DT_STRING, _T("The total size in bytes of the data held in this database including the padding factor.") },
   { _T("MongoDB.storageSize(*)"), H_GetOtherParam, _T("storageSize"), DCI_DT_STRING, _T("The total amount of space in bytes allocated to collections in this database for document storage.") },
   { _T("MongoDB.numExtents(*)"), H_GetOtherParam, _T("numExtents"), DCI_DT_STRING, _T("Contains a count of the number of extents in the database across all collections.") },
   { _T("MongoDB.indexesNum(*)"), H_GetOtherParam, _T("indexes"), DCI_DT_STRING, _T("Contains a count of the total number of indexes across all collections in the database.") },
   { _T("MongoDB.indexSize(*)"), H_GetOtherParam, _T("indexSize"), DCI_DT_STRING, _T("The total size in bytes of all indexes created on this database.") },
   { _T("MongoDB.fileSize(*)"), H_GetOtherParam, _T("fileSize"), DCI_DT_STRING, _T("The total size in bytes of the data files that hold the database.") },
   { _T("MongoDB.nsSizeMB(*)"), H_GetOtherParam, _T("nsSizeMB"), DCI_DT_STRING, _T("The total size of the namespace files (i.e. that end with .ns) for this database.") }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO s_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("MONGODB"), NETXMS_VERSION_STRING,
	SubAgentInit, SubAgentShutdown, nullptr, nullptr, nullptr,
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
         severity  = NXLOG_INFO; //NXLOG_DEBUG
         break;
   }
#ifdef UNICODE
   nxlog_write_tag(severity, DEBUG_TAG, _T("driver message (domain %hs): %hs"), log_domain, message);
#else
   nxlog_write_tag(severity, DEBUG_TAG, _T("driver message (domain %s): %s"), log_domain, message);
#endif // UNICODE
}

/**
 * Subagent initialization
 */
static bool SubAgentInit(Config *config)
{
	return true;
}

/**
 * Shutdown handler
 */
static void SubAgentShutdown()
{
	nxlog_debug_tag(DEBUG_TAG, 1, _T("stopping pollers"));
   for(int i = 0; i < g_connections->size(); i++)
      g_connections->get(i)->stop();
	mongoc_cleanup();
	//remove names form instances
   delete g_connections;
	nxlog_debug_tag(DEBUG_TAG, 1, _T("stopped"));
}

/**
 * Connection configuration template
 */
static ConnectionInfo s_connInfo;
static NX_CFG_TEMPLATE s_configTemplate[] =
{
   { _T("ConnectionTTL"),     CT_LONG,   0, 0, 0,            0, &s_connInfo.connectionTTL },
   { _T("EncryptedPassword"), CT_STRING, 0, 0, MAX_PASSWORD, 0, s_connInfo.password },
   { _T("Endpoint"),          CT_STRING, 0, 0, MAX_STR,      0, s_connInfo.endpoint },
   { _T("Id"),                CT_STRING, 0, 0, MAX_STR,      0, s_connInfo.id },
   { _T("Login"),             CT_STRING, 0, 0, MAX_STR,      0, s_connInfo.username },
   { _T("Password"),          CT_STRING, 0, 0, MAX_PASSWORD, 0, s_connInfo.password },
   { _T("Server"),            CT_STRING, 0, 0, MAX_STR,      0, s_connInfo.endpoint },   // alias for Endpoint (MongoDB endpoint is a URI / host[:port])
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr }
};

/**
 * Add connection from parsed template into the connection list
 */
static void AddConnectionFromTemplate()
{
   if (s_connInfo.id[0] == 0)
      return;

   DecryptPassword(s_connInfo.username, s_connInfo.password, s_connInfo.password, MAX_PASSWORD);

   DatabaseConnection *db = new DatabaseConnection(&s_connInfo);
   if (db->connectionEstablished())
   {
      nxlog_debug_tag(DEBUG_TAG, 2, _T("connection established, added to list: %s"), s_connInfo.id);
      g_connections->add(db);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 2, _T("could not connect to database: %s"), s_connInfo.id);
      delete db;
   }
}

/**
 * Reset connection template buffer to defaults
 */
static void ResetConnectionInfoDefaults()
{
   memset(&s_connInfo, 0, sizeof(s_connInfo));
   s_connInfo.connectionTTL = 3600;
}

/**
 * Add database connection from legacy colon-delimited record
 * (id:server:login:password parsed via named options)
 */
static void AddLegacyConnection(const TCHAR *record)
{
   nxlog_debug_tag(DEBUG_TAG, 3, _T("configuration entry [MongoDB/Database] is deprecated; use [mongodb/connections/<id>] instead"));

   ResetConnectionInfoDefaults();
   ExtractNamedOptionValue(record, _T("id"), s_connInfo.id, MAX_STR);
   ExtractNamedOptionValue(record, _T("server"), s_connInfo.endpoint, MAX_STR);
   ExtractNamedOptionValue(record, _T("login"), s_connInfo.username, MAX_STR);
   if (!ExtractNamedOptionValue(record, _T("password"), s_connInfo.password, MAX_PASSWORD))
      ExtractNamedOptionValue(record, _T("encryptedPassword"), s_connInfo.password, MAX_PASSWORD);

   AddConnectionFromTemplate();
}

/**
 * Load configuration, start connection
 */
static BOOL LoadConfiguration(Config *config)
{
   g_connections = new ObjectArray<DatabaseConnection>(8, 8, Ownership::True);
   mongoc_init();
   mongoc_log_set_handler(MongoLogging, nullptr);

   // Simple single-connection configuration in [mongodb] section
   if (config->getEntry(_T("/mongodb/id")) != nullptr || config->getEntry(_T("/mongodb/endpoint")) != nullptr ||
       config->getEntry(_T("/mongodb/server")) != nullptr || config->getEntry(_T("/mongodb/login")) != nullptr)
   {
      ResetConnectionInfoDefaults();
      _tcscpy(s_connInfo.id, _T("localdb"));
      _tcscpy(s_connInfo.endpoint, _T("127.0.0.1"));
      if (config->parseTemplate(_T("MONGODB"), s_configTemplate))
         AddConnectionFromTemplate();
   }

   // Named connection subsections (mongodb/connections/<id>)
   ConfigEntry *connectionsRoot = config->getEntry(_T("/mongodb/connections"));
   if (connectionsRoot != nullptr)
   {
      unique_ptr<ObjectArray<ConfigEntry>> connections = connectionsRoot->getSubEntries(_T("*"));
      for(int i = 0; i < connections->size(); i++)
      {
         ConfigEntry *e = connections->get(i);
         ResetConnectionInfoDefaults();
         _tcslcpy(s_connInfo.id, e->getName(), MAX_STR);   // Id defaults to subsection name

         TCHAR section[MAX_STR];
         _sntprintf(section, MAX_STR, _T("mongodb/connections/%s"), e->getName());
         if (!config->parseTemplate(section, s_configTemplate))
         {
            nxlog_debug_tag(DEBUG_TAG, NXLOG_WARNING, _T("error parsing configuration for connection %s"), e->getName());
            continue;
         }
         AddConnectionFromTemplate();
      }
   }

   // Legacy colon-delimited connection records ([MongoDB] Database=id:server:login:password)
	ConfigEntry *databases = config->getEntry(_T("/MongoDB/Database"));
   if (databases != nullptr)
   {
      for(int i = 0; i < databases->getValueCount(); i++)
         AddLegacyConnection(databases->getValue(i));
   }

	// Exit if no usable configuration found
   if (g_connections->size() == 0)
	{
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("no databases to monitor, exiting"));
      delete g_connections;
      mongoc_cleanup();
      return FALSE;
	}

   //create list
   int paramCount = 0;
   for(int i = 0; i < g_connections->size(); i++)
   {
      s_parameters = g_connections->get(i)->getParameters(&paramCount);
      if (s_parameters != nullptr)
         break;
   }

   if (s_parameters == nullptr)
   {
      delete g_connections;
      mongoc_cleanup();
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("no parameters will be added, not possible to get any status"));
      return FALSE;
   }

   int predefParamCount = sizeof(s_parametersPredef) / sizeof(NETXMS_SUBAGENT_PARAM);
   if (predefParamCount > 0)
   {
      s_parameters = MemRealloc(s_parameters, (paramCount + predefParamCount) * sizeof(NETXMS_SUBAGENT_PARAM));
      memcpy(s_parameters + paramCount, s_parametersPredef, predefParamCount * sizeof(NETXMS_SUBAGENT_PARAM));
      paramCount += predefParamCount;
   }

   for(int i = 0; i < g_connections->size(); i++)
      g_connections->get(i)->run();

   s_info.numParameters = paramCount;
   s_info.parameters = s_parameters;
   return TRUE;
}

/**
 * Get list of databases on a connection
 */
static LONG H_Databases(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   LONG result = SYSINFO_RC_UNSUPPORTED;
   TCHAR id[MAX_STR];
   if (!AgentGetParameterArg(param, 1, id, MAX_STR))
      return SYSINFO_RC_ERROR;

   for(int i = 0; i < g_connections->size(); i++)
   {
      if(!_tcsicmp(g_connections->get(i)->getId(), id))
      {
         result = g_connections->get(i)->getDatabaseList(value);
      }
   }
   return result;
}

/**
 * Get list of all discovered databases across all connections (connectionId,databaseName)
 */
static LONG H_AllDatabases(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   for(int i = 0; i < g_connections->size(); i++)
      g_connections->get(i)->appendAllDatabases(value);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Get list of configured connections
 */
static LONG H_ConnectionsList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   for(int i = 0; i < g_connections->size(); i++)
      value->add(g_connections->get(i)->getId());
   return SYSINFO_RC_SUCCESS;
}

static LONG H_GetOtherParam(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   LONG result = SYSINFO_RC_UNSUPPORTED;
   TCHAR id[MAX_STR];
   if (!AgentGetParameterArg(param, 1, id, MAX_STR) || arg == NULL)
      return SYSINFO_RC_ERROR;

   for(int i = 0; i < g_connections->size(); i++)
   {
      if(!_tcsicmp(g_connections->get(i)->getId(), id))
      {
         result = g_connections->get(i)->getOtherParam(param, arg, _T("dbStats"), value);
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
      return false;  // Most likely another instance of MONGODB subagent already loaded

   *ppInfo = &s_info;
   return LoadConfiguration(config);
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
