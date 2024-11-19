/*
 ** NetXMS - Network Management System
 ** Subagent for Mongo DB monitoring
 ** Copyright (C) 2009-2024 Raden Solutions
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

#define STATUSSIZE 24

/**
 * Read attribute from config string
 */
static String ReadAttribute(const TCHAR *config, const TCHAR *attribute)
{
   TCHAR buffer[256];
   if (!ExtractNamedOptionValue(config, attribute, buffer, 256))
      return String();
   return String(buffer);
}

bool AddMongoDBFromConfig(const TCHAR *config)
{
   AgentWriteDebugLog(2, _T("MONGODB: Trying to parse db."));
   //parse id:server:login:password
   DatabaseInfo info;
   memset(&info, 0, sizeof(info));
   _tcslcpy(info.id, ReadAttribute(config, _T("id")), MAX_STR);
   _tcslcpy(info.server, ReadAttribute(config, _T("server")), MAX_STR);
   _tcslcpy(info.username, ReadAttribute(config, _T("login")), MAX_STR);

   StringBuffer password = ReadAttribute(config, _T("password"));
   if (password.isEmpty())
      password = ReadAttribute(config, _T("encryptedPassword"));

   if (!password.isEmpty())
      DecryptPassword(info.username, password, info.password, MAX_PASSWORD);

   bool sucess = false;
   //add to list of DB connecdtions
   DatabaseInstance *db = new DatabaseInstance(&info);
   if (db->connectionEstablished())
   {
      AgentWriteDebugLog(2, _T("MONGODB: Connection estables and db added to list: %s"), info.id);
      g_instances->add(db);
      sucess = true;
   }
   else
   {
      AgentWriteDebugLog(2, _T("MONGODB: Could not connect to database: %s"), info.id);
      delete db;
   }
   return sucess;
}

static LONG H_GetParameter(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   LONG result = SYSINFO_RC_UNSUPPORTED;
   TCHAR id[MAX_STR];
   if (!AgentGetParameterArg(param, 1, id, MAX_STR))
      return SYSINFO_RC_UNSUPPORTED;

   for(int i = 0; i < g_instances->size(); i++)
   {
      if(!_tcsicmp(g_instances->get(i)->getConnectionName(), id))
      {
         TCHAR *paramName = _tcsdup(param+8);
         TCHAR *tmp = _tcschr(paramName, _T('('));
         *tmp = 0;
         AgentWriteDebugLog(5, _T("MONGODB: Searching for parameter: %s "), paramName);
#ifdef UNICODE
         char *_paramName = UTF8StringFromWideString(paramName);
         result = g_instances->get(i)->getStatusParam(_paramName, value);
         MemFree(_paramName);
#else
         result = g_instances->get(i)->getStatusParam(paramName, value);
#endif // UNICODE
         MemFree(paramName);
      }
   }

   return result;
}

/**
 * Create new database instance object
 */
DatabaseInstance::DatabaseInstance(DatabaseInfo *info) : m_stopCondition(true)
{
   memcpy(&m_info, info, sizeof(DatabaseInfo));
   m_serverStatus = nullptr;
   m_databaseList = nullptr;
   m_data = new StringObjectMap<StringObjectMap<MongoDBCommand> >(Ownership::True);
   connect();
}

/**
 * Connect to database
 */
 bool DatabaseInstance::connect()
 {
   TCHAR connectionString[256];
   _tcscpy(connectionString, _T("mongodb://"));
   if (_tcscmp(m_info.username, _T("")))
   {
      _tcscat(connectionString, m_info.username);
      _tcscat(connectionString, _T(":"));
      _tcscat(connectionString, m_info.password);
      _tcscat(connectionString, _T("@"));
   }
   _tcscat(connectionString, m_info.server);

#ifdef UNICODE
   char *_connectionString = UTF8StringFromWideString(connectionString);
	m_dbConn = mongoc_client_new (_connectionString);
	MemFree(_connectionString);
#else
	m_dbConn = mongoc_client_new(connectionString);
#endif
   return connectionEstablished();
}

/**
 * Destructor
 */
DatabaseInstance::~DatabaseInstance()
{
   if (m_serverStatus != nullptr)
   {
      bson_destroy (m_serverStatus);
      delete m_serverStatus;
   }
   mongoc_client_destroy (m_dbConn);
   if (m_databaseList != nullptr)
      bson_strfreev(m_databaseList);
   delete m_data;
}

/**
 * Run
 */
void DatabaseInstance::run()
{
   m_pollerThread = ThreadCreateEx(DatabaseInstance::pollerThreadStarter, 0, this);
}

/**
 * Stop
 */
void DatabaseInstance::stop()
{
   m_stopCondition.set();
   ThreadJoin(m_pollerThread);
   m_pollerThread = INVALID_THREAD_HANDLE;
}

/**
 * Poller thread starter
 */
THREAD_RESULT THREAD_CALL DatabaseInstance::pollerThreadStarter(void *arg)
{
   ((DatabaseInstance *)arg)->pollerThread();
   return THREAD_OK;
}

/**
 * Poller thread
 */
void DatabaseInstance::pollerThread()
{
   AgentWriteDebugLog(3, _T("MONGODB: poller thread for database %s started"), m_info.id);
   do
   {
      getServerStatus();
      getDatabases();
   }
   while(!m_stopCondition.wait(60000));
   AgentWriteDebugLog(3, _T("MONGODB: poller thread for database %s stopped"), m_info.id);
}

/**
 * Get all databases and it's size
 */
void DatabaseInstance::getDatabases()
{
   bson_error_t error;
   m_databaseListLock.lock();
   if(m_databaseList != NULL)
   {
      bson_strfreev(m_databaseList);
      m_databaseList = NULL;
   }
   m_connLock.lock();
#if HAVE_MONGOC_CLIENT_GET_DATABASE_NAMES_WITH_OPTS
   if (!(m_databaseList = mongoc_client_get_database_names_with_opts(m_dbConn, nullptr, &error)))
#else
   if (!(m_databaseList = mongoc_client_get_database_names(m_dbConn, &error)))
#endif
   {
#ifdef UNICODE
      TCHAR *_error = WideStringFromUTF8String(error.message);
      AgentWriteDebugLog(3, _T("MONGODB: Failed to run command: %s\n"), _error);
      MemFree(_error);
#else
      AgentWriteDebugLog(3, _T("MONGODB: Failed to run command: %s\n"), error.message);
#endif
      bson_strfreev (m_databaseList);
      m_databaseList = nullptr;
   }
   m_connLock.unlock();
   m_databaseListLock.unlock();
}

/**
 * Get the status of the database
 */
bool DatabaseInstance::getServerStatus()
{
   m_serverStatusLock.lock();

   bson_error_t error;
   bool sucess = true;
   if (m_serverStatus != nullptr)
   {
      bson_destroy(m_serverStatus);
   }
   else
   {
      m_serverStatus = new bson_t;
   }

   bson_t cmd = BSON_INITIALIZER;
   BSON_APPEND_INT32(&cmd, "serverStatus", 1);

   m_connLock.lock();
   if (!mongoc_client_command_simple(m_dbConn, "admin", &cmd, nullptr, m_serverStatus, &error))
   {
#ifdef UNICODE
      TCHAR *_error = WideStringFromUTF8String(error.message);
      AgentWriteDebugLog(3, _T("MONGODB: Failed to run command: %s\n"), _error);
      MemFree(_error);
#else
      AgentWriteDebugLog(3, _T("MONGODB: Failed to run command: %s\n"), error.message);
#endif
      sucess = false;
      bson_destroy(m_serverStatus);
      delete_and_null(m_serverStatus);
   }
   bson_destroy(&cmd);

   m_connLock.unlock();
   m_serverStatusLock.unlock();
   return sucess;
}

/**
 * Returns value of status parameter
 */
LONG DatabaseInstance::getStatusParam(const char *paramName, TCHAR *value)
{
   m_serverStatusLock.lock();
   LONG result = getParam(m_serverStatus, paramName, value);
   m_serverStatusLock.unlock();
   return result;
}

/**
 * Returns value of any parameter
 */
LONG DatabaseInstance::getParam(bson_t *bsonDoc, const char *paramName, TCHAR *value)
{
   if(bsonDoc == NULL)
      return SYSINFO_RC_ERROR;
   bson_iter_t iter;
   bson_iter_t baz;
   LONG result = SYSINFO_RC_UNSUPPORTED;

   //uncomment for debug purposes
   char *json;
   if ((json = bson_as_json(bsonDoc, NULL))) {
#ifdef UNICODE
      TCHAR *_json = WideStringFromUTF8String(json);
      nxlog_debug(7, _T("MONGODB: trying to get param from %s"), _json);
      MemFree(_json);
#else
      nxlog_debug(7, _T("MONGODB: trying to get param from %s"), json);
#endif // UNICODE
      bson_free (json);
   }

   if (bson_iter_init (&iter, bsonDoc) && bson_iter_find_descendant (&iter, paramName, &baz))
   {
      AgentWriteDebugLog(NXLOG_INFO, _T("MONGODB: Trying to get parameter. Type: %d"), bson_iter_type(&baz));
      char *val = (char *) malloc(sizeof(char) * MAX_RESULT_LENGTH);
      strcpy(val, "Unknown type");
      switch(bson_iter_type(&baz))
      {
      case BSON_TYPE_UTF8:
         strlcpy(val, bson_iter_utf8(&baz, NULL), MAX_RESULT_LENGTH);
         break;
      case BSON_TYPE_INT32:
         IntegerToString(bson_iter_int32(&baz), val);
         break;
      case BSON_TYPE_INT64:
         IntegerToString(bson_iter_int64(&baz), val);
         break;
      case BSON_TYPE_DOUBLE:
         sprintf(val, "%f", bson_iter_double(&baz));
         break;
      case BSON_TYPE_BOOL:
         sprintf(val, "%d", bson_iter_bool(&baz));
         break;
      }
#ifdef UNICODE
      TCHAR *v = WideStringFromUTF8String(val);
      ret_string(value, v);
      MemFree(v);
#else
      ret_string(value, val);
#endif // UNICODE
      MemFree(val);
      result = SYSINFO_RC_SUCCESS;
   }

   return result;
}

/**
 * Returns list of available status parameters
 */
NETXMS_SUBAGENT_PARAM *DatabaseInstance::getParameters(int *paramCount)
{
   //try to get data first time
   getServerStatus();
   if (m_serverStatus == nullptr)
      return nullptr;

   int attrSize = STATUSSIZE;
   NETXMS_SUBAGENT_PARAM *result = MemAllocArray<NETXMS_SUBAGENT_PARAM>(attrSize);
   bson_iter_t iter;
   bson_iter_t child;

   m_serverStatusLock.lock();
   int i = 0;
   if (bson_iter_init (&iter, m_serverStatus))
   {
      while (bson_iter_next (&iter))
      {
         TCHAR name[MAX_PARAM_NAME];
         _tcscpy(name, _T("MongoDB."));
#ifdef UNICODE
         TCHAR *_name = WideStringFromUTF8String(bson_iter_key(&iter));
         _tcslcat(name, _name, MAX_PARAM_NAME);
         MemFree(_name);
#else
         _tcslcat(name, bson_iter_key(&iter), MAX_PARAM_NAME);
#endif // UNICODE
         if (!_tcsicmp(name, _T("MongoDB.ok")))
            continue;

         if (bson_iter_recurse (&iter, &child))
         {
            _tcscat(name, _T(".*"));
         }
         else
         {
            _tcscat(name, _T("(*)"));
         }
         AgentWriteDebugLog(6, _T("MONGODB: Param name added: %s"), name);
         if (i >= attrSize)
         {
            attrSize++;
            result = MemReallocArray(result, attrSize);
         }

         memset(&result[i], 0, sizeof(NETXMS_SUBAGENT_PARAM));
         _tcscpy(result[i].name, name);
         result[i].handler = H_GetParameter;
         result[i].dataType = DCI_DT_STRING;
         _tcscpy(result[i].description, _T(""));
         i++;
      }
   }
   m_serverStatusLock.unlock();
   if (i < attrSize)
   {
      result = MemReallocArray(result, i);
   }
   *paramCount = i;
   return result;
}

LONG DatabaseInstance::setDbNames(StringList *value)
{
   if (m_databaseList == nullptr)
      return SYSINFO_RC_ERROR;

   m_databaseListLock.lock();
   for (int i = 0; m_databaseList[i]; i++)
   {
#ifdef UNICODE
      TCHAR *_name = WideStringFromUTF8String(m_databaseList[i]);
      value->add(_name);
      MemFree(_name);
#else
      value->add(m_databaseList[i]);
#endif
   }
   m_databaseListLock.unlock();
   return SYSINFO_RC_SUCCESS;
}

/**
 * Get parameter from command
 */
LONG DatabaseInstance::getOtherParam(const TCHAR *param, const TCHAR *arg, const TCHAR *command, TCHAR *value)
{
   TCHAR dbName[MAX_STR];
   if (!AgentGetParameterArg(param, 2, dbName, MAX_STR))
      return SYSINFO_RC_ERROR;
   LONG result = SYSINFO_RC_UNSUPPORTED;
   m_dataLock.lock();
   MongoDBCommand *commandData = NULL;
   StringObjectMap<MongoDBCommand> * list = m_data->get(dbName);
   if (list != NULL)
   {
     commandData = list->get(command);
   }
   if (commandData != NULL)
   {
      if((GetCurrentTimeMs() - commandData->m_lastUpdateTime) > 60000)
      {
         m_connLock.lock();
         result = commandData->getData(m_dbConn, dbName, command) ?  SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
         m_connLock.unlock();
      }
      else
      {
         result = SYSINFO_RC_SUCCESS;
      }
   }
   else
   {
      commandData = new MongoDBCommand();
      m_connLock.lock();
      result = commandData->getData(m_dbConn, dbName, command) ?  SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
      m_connLock.unlock();
      list = new StringObjectMap<MongoDBCommand>(Ownership::True);
      list->set(command,commandData);
      m_data->set(dbName,list);
   }

   if(result == SYSINFO_RC_SUCCESS)
   {
#ifdef UNICODE
      char *_arg = UTF8StringFromWideString(arg);
      result = getParam(&(commandData->m_result), _arg, value);
      MemFree(_arg);
#else
      result = getParam(&(commandData->m_result), arg, value);
#endif
   }

   m_dataLock.unlock();
   return result;
}

bool MongoDBCommand::getData(mongoc_client_t *m_dbConn, const TCHAR *dbName, const TCHAR *command)
{
   bool sucess = false;
   m_lastUpdateTime = GetCurrentTimeMs();
   bson_error_t error;
#ifdef UNICODE
   char *_dbName = UTF8StringFromWideString(dbName);
   mongoc_database_t *database = mongoc_client_get_database (m_dbConn, _dbName);
   MemFree(_dbName);
#else
   mongoc_database_t *database = mongoc_client_get_database(m_dbConn, dbName);
#endif // UNICODE
   if(database != NULL)
   {
      bson_destroy(&m_result);
      bson_t cmd;
      bson_init(&cmd);
#ifdef UNICODE
      char *_command = UTF8StringFromWideString(command);
      bson_append_int32(&cmd, _command, strlen(_command), 1);
      MemFree(_command);
#else
      bson_append_int32(&cmd, command, strlen(command), 1);
#endif // UNICODE
      sucess = mongoc_database_command_simple (database, &cmd, NULL, &m_result, &error);
      bson_destroy (&cmd);
      AgentWriteDebugLog(5, _T("MONGODB: Command %s executed with result %d"), command, sucess);
      if(!sucess)
      {
#ifdef UNICODE
         TCHAR *_error = WideStringFromUTF8String(error.message);
         AgentWriteDebugLog(3, _T("MONGODB: Failed to run command(%s): %s\n"), command, _error);
         MemFree(_error);
#else
         AgentWriteDebugLog(3, _T("MONGODB: Failed to run command(%s): %s\n"), command, error.message);
#endif
      }
      mongoc_database_destroy(database);
   }
   else
   {
      AgentWriteDebugLog(5, _T("MONGODB: Database not found: %s\n"), dbName);
      sucess = false;
   }
   return sucess;
}
