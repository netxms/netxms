/*
** NetXMS subagent for Informix monitoring
** Copyright (C) 2011-2026 Raden Solutions
**/

#include "informix_subagent.h"
#include <netxms-version.h>

/**
 * Driver handle
 */
DB_DRIVER g_driverHandle = nullptr;

/**
 * Configured database connections
 */
static ObjectArray<DatabaseConnection> *s_connections = nullptr;

/**
 * Parameter group definitions (shared by all connections)
 */
const ParameterGroup g_paramGroups[NUM_PARAM_GROUPS] =
{
   {
      1100, _T("Informix.Session."),
      _T("select ") DB_NULLARG_MAGIC _T(" ValueName, count(*) Count from syssessions")
   },
   {
      1100, _T("Informix.Database."),
      _T("select name ValueName, owner Owner, is_logging Logged, created Created from sysdatabases")
   },
   {
      1100, _T("Informix.Dbspace.Pages."),
      _T("select name ValueName,sum(chksize) PageSize,sum(chksize)-sum(nfree) Used,")
      _T("sum(nfree) Free,round((sum(nfree))/(sum(chksize))*100,2) FreePerc from sysdbspaces d,syschunks c ")
      _T("where d.dbsnum=c.dbsnum	group by name order by name")
   }
};

/**
 * Find connection by ID
 */
static DatabaseConnection *FindConnection(const TCHAR *id)
{
   for(int i = 0; i < s_connections->size(); i++)
   {
      DatabaseConnection *db = s_connections->get(i);
      if (!_tcsicmp(db->getId(), id))
         return db;
   }
   return nullptr;
}

/**
 * Figure out Informix DBMS version
 */
static int GetInformixVersion(DB_HANDLE handle)
{
   return 1100;
}

/**
 * Create new database connection object
 */
DatabaseConnection::DatabaseConnection(const ConnectionInfo *info) : m_dataLock(MutexType::FAST), m_stopCondition(true)
{
   memcpy(&m_info, info, sizeof(ConnectionInfo));
   m_pollerThread = INVALID_THREAD_HANDLE;
   m_session = nullptr;
   m_connected = false;
   m_version = 0;
   for(int i = 0; i < NUM_PARAM_GROUPS; i++)
      m_data[i] = new StringObjectMap<StringMap>(Ownership::True);
}

/**
 * Destructor
 */
DatabaseConnection::~DatabaseConnection()
{
   stop();
   for(int i = 0; i < NUM_PARAM_GROUPS; i++)
      delete m_data[i];
}

/**
 * Run poller thread
 */
void DatabaseConnection::run()
{
   m_pollerThread = ThreadCreateEx(this, &DatabaseConnection::pollerThread);
}

/**
 * Stop poller thread
 */
void DatabaseConnection::stop()
{
   m_stopCondition.set();
   ThreadJoin(m_pollerThread);
   m_pollerThread = INVALID_THREAD_HANDLE;
   if (m_session != nullptr)
   {
      DBDisconnect(m_session);
      m_session = nullptr;
   }
}

/**
 * Poller thread
 */
void DatabaseConnection::pollerThread()
{
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Poller thread for connection %s started"), m_info.id);
   int64_t connectionTTL = static_cast<int64_t>(m_info.connectionTTL) * _LL(1000);
   do
   {
reconnect:
      TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
      m_session = DBConnect(g_driverHandle, m_info.endpoint, m_info.database, m_info.username, m_info.password, nullptr, errorText);
      if (m_session == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("Cannot connect to database %s: %s"), m_info.id, errorText);
         continue;
      }

      m_connected = true;
      m_version = GetInformixVersion(m_session);
      DBEnableReconnect(m_session, false);
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Connection with database %s restored (connection TTL %u)"), m_info.id, m_info.connectionTTL);

      int64_t pollerLoopStartTime = GetCurrentTimeMs();
      uint32_t sleepTime;
      do
      {
         int64_t startTime = GetCurrentTimeMs();
         if (!poll())
         {
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Connection with database %s lost"), m_info.id);
            break;
         }
         int64_t currTime = GetCurrentTimeMs();
         if (currTime - pollerLoopStartTime > connectionTTL)
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("Planned connection reset"));
            m_connected = false;
            DBDisconnect(m_session);
            m_session = nullptr;
            goto reconnect;
         }
         int64_t elapsedTime = currTime - startTime;
         sleepTime = static_cast<uint32_t>((elapsedTime >= 60000) ? 60000 : (60000 - elapsedTime));
      }
      while(!m_stopCondition.wait(sleepTime));

      m_connected = false;
      DBDisconnect(m_session);
      m_session = nullptr;
   }
   while(!m_stopCondition.wait(30000));   // reconnect every 30 seconds
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Poller thread for connection %s stopped"), m_info.id);
}

/**
 * Do actual database polling. Should return false if connection is broken.
 */
bool DatabaseConnection::poll()
{
   StringObjectMap<StringMap> *newData[NUM_PARAM_GROUPS];
   for(int i = 0; i < NUM_PARAM_GROUPS; i++)
      newData[i] = nullptr;

   bool success = true;
   for(int i = 0; i < NUM_PARAM_GROUPS; i++)
   {
      if (g_paramGroups[i].version > m_version)   // group not supported for this DB version
         continue;

      DB_RESULT queryResult = DBSelect(m_session, g_paramGroups[i].query);
      if (queryResult == nullptr)
      {
         success = false;
         break;
      }

      StringObjectMap<StringMap> *groupData = new StringObjectMap<StringMap>(Ownership::True);
      int rows = DBGetNumRows(queryResult);
      for(int r = 0; r < rows; r++)
      {
         TCHAR name[MAX_STR];
         DBGetField(queryResult, r, 0, name, MAX_STR);

         StringMap *attrs = new StringMap();
         TCHAR colName[MAX_STR];
         for(int c = 1; DBGetColumnName(queryResult, c, colName, MAX_STR); c++)
         {
            TCHAR colValue[MAX_STR];
            DBGetField(queryResult, r, c, colValue, MAX_STR);
            attrs->set(colName, colValue);
         }
         groupData->set(name, attrs);
      }
      DBFreeResult(queryResult);
      newData[i] = groupData;
   }

   if (!success)
   {
      for(int i = 0; i < NUM_PARAM_GROUPS; i++)
         delete newData[i];
      return false;
   }

   m_dataLock.lock();
   for(int i = 0; i < NUM_PARAM_GROUPS; i++)
   {
      if (newData[i] != nullptr)
      {
         delete m_data[i];
         m_data[i] = newData[i];
      }
   }
   m_dataLock.unlock();
   return true;
}

/**
 * Get collected data for given parameter group, entity, and attribute key
 */
LONG DatabaseConnection::getData(int groupIndex, const TCHAR *entity, const TCHAR *key, TCHAR *value)
{
   LONG rc = SYSINFO_RC_ERROR;
   m_dataLock.lock();
   StringMap *attrs = m_data[groupIndex]->get(entity);
   if (attrs != nullptr)
   {
      const TCHAR *v = attrs->get(key);
      if (v != nullptr)
      {
         ret_string(value, v);
         rc = SYSINFO_RC_SUCCESS;
      }
   }
   m_dataLock.unlock();
   return rc;
}

/**
 * Handler for Informix.X(connectionId[, object]) parameters
 */
static LONG H_DatabaseParameter(const TCHAR *parameter, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR id[MAX_STR];
   if (!AgentGetParameterArg(parameter, 1, id, MAX_STR))
      return SYSINFO_RC_UNSUPPORTED;

   TCHAR entity[MAX_STR];
   if (!AgentGetParameterArg(parameter, 2, entity, MAX_STR) || (entity[0] == 0))
      _tcslcpy(entity, DB_NULLARG_MAGIC, MAX_STR);

   DatabaseConnection *db = FindConnection(id);
   if (db == nullptr)
      return SYSINFO_RC_UNSUPPORTED;

   // Find parameter group whose prefix matches the requested parameter
   for(int i = 0; i < NUM_PARAM_GROUPS; i++)
   {
      size_t prefixLen = _tcslen(g_paramGroups[i].prefix);
      if (_tcsnicmp(g_paramGroups[i].prefix, parameter, prefixLen))
         continue;

      // Derive attribute key (parameter name without prefix and arguments)
      TCHAR key[MAX_STR];
      _tcslcpy(key, parameter + prefixLen, MAX_STR);
      TCHAR *p = _tcschr(key, _T('('));
      if (p != nullptr)
         *p = 0;

      return db->getData(i, entity, key, value);
   }

   return SYSINFO_RC_UNSUPPORTED;
}

/**
 * Configuration template (shared by simple and named-subsection forms)
 */
static ConnectionInfo s_connInfo;
static NX_CFG_TEMPLATE s_configTemplate[] =
{
   { _T("ConnectionTTL"),       CT_LONG,        0, 0, 0,            0, &s_connInfo.connectionTTL },
   { _T("Database"),            CT_STRING,      0, 0, MAX_STR,      0, s_connInfo.database },
   { _T("DBName"),              CT_STRING,      0, 0, MAX_STR,      0, s_connInfo.database },    // deprecated alias for Database
   { _T("Endpoint"),            CT_STRING,      0, 0, MAX_STR,      0, s_connInfo.endpoint },
   { _T("Server"),              CT_STRING,      0, 0, MAX_STR,      0, s_connInfo.endpoint },    // alias for Endpoint (server instance name)
   { _T("DBServer"),            CT_STRING,      0, 0, MAX_STR,      0, s_connInfo.endpoint },    // deprecated alias for Endpoint
   { _T("Id"),                  CT_STRING,      0, 0, MAX_STR,      0, s_connInfo.id },
   { _T("Login"),               CT_STRING,      0, 0, MAX_USERNAME, 0, s_connInfo.username },
   { _T("DBLogin"),             CT_STRING,      0, 0, MAX_USERNAME, 0, s_connInfo.username },    // deprecated alias for Login
   { _T("Password"),            CT_STRING,      0, 0, MAX_PASSWORD, 0, s_connInfo.password },
   { _T("DBPassword"),          CT_STRING,      0, 0, MAX_PASSWORD, 0, s_connInfo.password },    // deprecated alias for Password
   { _T("EncryptedPassword"),   CT_STRING,      0, 0, MAX_PASSWORD, 0, s_connInfo.password },
   { _T("DBPasswordEncrypted"), CT_STRING,      0, 0, MAX_PASSWORD, 0, s_connInfo.password },    // deprecated alias for EncryptedPassword
   { _T(""),                    CT_END_OF_LIST, 0, 0, 0,            0, nullptr }
};

/**
 * Log debug-level deprecation warnings for legacy configuration keys in a section
 */
static void CheckDeprecatedKeys(Config *config, const TCHAR *section)
{
   static const TCHAR *deprecatedKeys[] = { _T("DBServer"), _T("DBName"), _T("DBLogin"), _T("DBPassword"), _T("DBPasswordEncrypted"), nullptr };
   for(int i = 0; deprecatedKeys[i] != nullptr; i++)
   {
      TCHAR path[MAX_STR];
      _sntprintf(path, MAX_STR, _T("/%s/%s"), section, deprecatedKeys[i]);
      if (config->getEntry(path) != nullptr)
         nxlog_debug_tag(DEBUG_TAG, 3, _T("Configuration key \"%s\" in section [%s] is deprecated"), deprecatedKeys[i], section);
   }
}

/**
 * Create connection object from currently parsed configuration template
 */
static void AddConnection(Config *config, const TCHAR *section)
{
   if (s_connInfo.id[0] == 0)
      _tcslcpy(s_connInfo.id, s_connInfo.database, MAX_STR);   // Id defaults to the DSN/database

   if (FindConnection(s_connInfo.id) != nullptr)
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Duplicate connection ID \"%s\" in section [%s] ignored"), s_connInfo.id, section);
      return;
   }

   CheckDeprecatedKeys(config, section);
   DecryptPassword(s_connInfo.username, s_connInfo.password, s_connInfo.password, MAX_PASSWORD);
   s_connections->add(new DatabaseConnection(&s_connInfo));
}

/**
 * Subagent initialization
 */
static bool SubAgentInit(Config *config)
{
   g_driverHandle = DBLoadDriver(_T("informix.ddr"), nullptr, nullptr, nullptr);
   if (g_driverHandle == nullptr)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Cannot load Informix database driver"));
      return false;
   }

   s_connections = new ObjectArray<DatabaseConnection>(8, 8, Ownership::True);

   // Load configuration from "informix" section to allow simple configuration of one connection
   memset(&s_connInfo, 0, sizeof(s_connInfo));
   s_connInfo.connectionTTL = 3600;
   if (config->parseTemplate(_T("INFORMIX"), s_configTemplate))
   {
      if (s_connInfo.database[0] != 0)
         AddConnection(config, _T("INFORMIX"));
   }

   // Load named connection subsections (informix/connections/<id>)
   ConfigEntry *connectionsRoot = config->getEntry(_T("/informix/connections"));
   if (connectionsRoot != nullptr)
   {
      unique_ptr<ObjectArray<ConfigEntry>> connections = connectionsRoot->getSubEntries(_T("*"));
      for(int i = 0; i < connections->size(); i++)
      {
         ConfigEntry *e = connections->get(i);
         memset(&s_connInfo, 0, sizeof(s_connInfo));
         s_connInfo.connectionTTL = 3600;
         _tcslcpy(s_connInfo.id, e->getName(), MAX_STR);   // Id defaults to subsection name

         TCHAR section[MAX_STR];
         _sntprintf(section, MAX_STR, _T("informix/connections/%s"), e->getName());
         if (!config->parseTemplate(section, s_configTemplate))
         {
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Error parsing Informix subagent configuration for connection %s"), e->getName());
            continue;
         }
         AddConnection(config, section);
      }
   }

   // Load legacy numbered subsections (informix/databases/database#N), deprecated
   ConfigEntry *databasesRoot = config->getEntry(_T("/informix/databases"));
   if (databasesRoot != nullptr)
   {
      unique_ptr<ObjectArray<ConfigEntry>> databases = databasesRoot->getSubEntries(_T("database#*"));
      for(int i = 0; i < databases->size(); i++)
      {
         ConfigEntry *e = databases->get(i);
         memset(&s_connInfo, 0, sizeof(s_connInfo));
         s_connInfo.connectionTTL = 3600;

         TCHAR section[MAX_STR];
         _sntprintf(section, MAX_STR, _T("informix/databases/%s"), e->getName());
         if (!config->parseTemplate(section, s_configTemplate))
         {
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Error parsing Informix subagent configuration for section %s"), e->getName());
            continue;
         }
         nxlog_debug_tag(DEBUG_TAG, 3, _T("Configuration section [%s] is deprecated, use [informix/connections/<id>] instead"), section);
         if (s_connInfo.database[0] != 0)
            AddConnection(config, section);
      }
   }

   // Exit if no usable configuration found
   if (s_connections->isEmpty())
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("No Informix databases to monitor"));
      delete s_connections;
      s_connections = nullptr;
      DBUnloadDriver(g_driverHandle);
      g_driverHandle = nullptr;
      return false;
   }

   // Run poller thread for each configured connection
   for(int i = 0; i < s_connections->size(); i++)
      s_connections->get(i)->run();

   nxlog_debug_tag(DEBUG_TAG, 3, _T("Informix subagent started with %d connection(s)"), s_connections->size());
   return true;
}

/**
 * Shutdown handler
 */
static void SubAgentShutdown()
{
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Stopping Informix database pollers"));
   for(int i = 0; i < s_connections->size(); i++)
      s_connections->get(i)->stop();
   delete s_connections;
   s_connections = nullptr;
   DBUnloadDriver(g_driverHandle);
   g_driverHandle = nullptr;
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Informix subagent stopped"));
}

/**
 * Handler for Informix.Connections list
 */
static LONG H_ConnectionsList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   for(int i = 0; i < s_connections->size(); i++)
      value->add(s_connections->get(i)->getId());
   return SYSINFO_RC_SUCCESS;
}

/**
 * Parameters
 */
static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
   { _T("Informix.Session.Count(*)"), H_DatabaseParameter, nullptr, DCI_DT_INT, _T("Informix/Sessions: Number of sessions opened") },
   { _T("Informix.Database.Owner(*)"), H_DatabaseParameter, nullptr, DCI_DT_STRING, _T("Informix/Databases: Database owner") },
   { _T("Informix.Database.Created(*)"), H_DatabaseParameter, nullptr, DCI_DT_STRING, _T("Informix/Databases: Creation date") },
   { _T("Informix.Database.Logged(*)"), H_DatabaseParameter, nullptr, DCI_DT_INT, _T("Informix/Databases: Is logged") },
   { _T("Informix.Dbspace.Pages.PageSize(*)"), H_DatabaseParameter, nullptr, DCI_DT_INT, _T("Informix/Dbspaces: Page size") },
   { _T("Informix.Dbspace.Pages.Used(*)"), H_DatabaseParameter, nullptr, DCI_DT_INT, _T("Informix/Dbspaces: Number of pages used in dbspace") },
   { _T("Informix.Dbspace.Pages.Free(*)"), H_DatabaseParameter, nullptr, DCI_DT_INT, _T("Informix/Dbspaces: Number of pages free in dbspace") },
   { _T("Informix.Dbspace.Pages.FreePerc(*)"), H_DatabaseParameter, nullptr, DCI_DT_INT, _T("Informix/Dbspaces: Percentage of free space in dbspace") }
};

/**
 * Lists
 */
static NETXMS_SUBAGENT_LIST m_lists[] =
{
   { _T("Informix.Connections"), H_ConnectionsList, nullptr, _T("Informix: configured database connections") }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO m_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("INFORMIX"), NETXMS_VERSION_STRING,
   SubAgentInit, SubAgentShutdown, nullptr, nullptr, nullptr,
   sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM), m_parameters,
   sizeof(m_lists) / sizeof(NETXMS_SUBAGENT_LIST), m_lists,
   0, nullptr,
   0, nullptr,
   0, nullptr
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(INFORMIX)
{
   *ppInfo = &m_info;
   return true;
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
