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
#define STATUSSIZE 24

/**
 * Read attribute from config string
 */
static TCHAR *ReadAttribute(const TCHAR *config, const TCHAR *attribute)
{
   TCHAR buffer[256];
   if (!ExtractNamedOptionValue(config, attribute, buffer, 256))
      return NULL;
   return _tcsdup(buffer);
}

bool AddMongoDBFromConfig(const TCHAR *config)
{
   AgentWriteDebugLog(NXLOG_WARNING, _T("MONGODB: Trying to parse db."));
   //parse id:server:login:password
   DatabaseInfo info;
   TCHAR *tmp = ReadAttribute(config, _T("id"));
   _tcsncpy(info.id, CHECK_NULL_EX(tmp), MAX_STR);
   safe_free(tmp);
   tmp = ReadAttribute(config, _T("server"));
   _tcsncpy(info.server, CHECK_NULL_EX(tmp), MAX_STR);
   safe_free(tmp);
   tmp = ReadAttribute(config, _T("login"));
   _tcsncpy(info.username, CHECK_NULL_EX(tmp), MAX_STR);
   safe_free(tmp);

   TCHAR *password = ReadAttribute(config, _T("encryptedPassword"));
   if (password != NULL)
   {
      TCHAR buffer[256];
      DecryptPassword(CHECK_NULL_EX(info.username), password, buffer);
      free(password);
      _tcsncpy(info.password, CHECK_NULL_EX(buffer), MAX_STR);
   }
   else
   {
      tmp = ReadAttribute(config, _T("password"));
      _tcsncpy(info.password, CHECK_NULL_EX(tmp), MAX_STR);
      safe_free(tmp);
   }

   bool sucess = false;
   //add to list of DB connecdtions
   DatabaseInstance *db = new DatabaseInstance(&info);
   if(db->connectionEstablished())
   {
      AgentWriteDebugLog(NXLOG_WARNING, _T("MONGODB: Connsection estables and db added to list: %s"), info.id);
      g_instances->add(db);
      sucess = true;
   }
   else
   {
      AgentWriteDebugLog(NXLOG_WARNING, _T("MONGODB: Could not connect to database: %s"), info.id);
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
         AgentWriteDebugLog(NXLOG_INFO, _T("MONGODB: Searching for parameter: %s "), paramName);
#ifdef UNICODE
         char *_paramName = UTF8StringFromWideString(paramName);
         result = g_instances->get(i)->getStatusParam(_paramName, value);
         safe_free(_paramName);
#else
         result = g_instances->get(i)->getStatusParam(paramName, value);
#endif // UNICODE
         safe_free(paramName);
      }
   }

   return result;
}


/**
 * Create new database instance object
 */
DatabaseInstance::DatabaseInstance(DatabaseInfo *info)
{
   memcpy(&m_info, info, sizeof(DatabaseInfo));
   m_serverStatus = NULL;
   connect();
	m_serverStatusLock = MutexCreate();
   m_stopCondition = ConditionCreate(TRUE);
   m_databaseListLock = MutexCreate();
   m_dataLock = MutexCreate();
   m_databaseList = NULL;
   m_data = new StringObjectMap<StringObjectMap<MongoDBCommand> >(true);
}

/**
 * Connect to database
 */
 bool DatabaseInstance::connect()
 {
   TCHAR connectionString[256];
   _tcscpy(connectionString, _T("mongodb://"));
   if(_tcscmp(m_info.username, _T("")))
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
	safe_free(_connectionString);
#else
	m_dbConn = mongoc_client_new(connectionString);
#endif
   return connectionEstablished();
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
   ConditionSet(m_stopCondition);
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
   AgentWriteDebugLog(NXLOG_INFO, _T("MONGODB: poller thread for database %s started"), m_info.id);
   do
   {
      getServerStatus();
      getDatabases();
   }
   while(!ConditionWait(m_stopCondition, 60000));
   AgentWriteDebugLog(NXLOG_INFO, _T("MONGODB: poller thread for database %s stopped"), m_info.id);
}

/**
 * Get all databases and it's size
 */
void DatabaseInstance::getDatabases()
{
   bson_error_t error;
   MutexLock(m_databaseListLock);
   if(m_databaseList != NULL)
   {
      bson_strfreev(m_databaseList);
      m_databaseList = NULL;
   }
   if (!(m_databaseList = mongoc_client_get_database_names(m_dbConn, &error)))
   {
#ifdef UNICODE
      TCHAR *_error = WideStringFromUTF8String(error.message);
      AgentWriteDebugLog(NXLOG_INFO, _T("MONGODB: Failed to run command: %s\n"), _error);
      safe_free(_error);
#else
      AgentWriteDebugLog(NXLOG_INFO, _T("MONGODB: Failed to run command: %s\n"), error.message);
#endif
      bson_strfreev (m_databaseList);
      m_databaseList = NULL;
   }
   MutexUnlock(m_databaseListLock);
}

/**
 * Check that the connection is established
 */
bool DatabaseInstance::connectionEstablished()
{
   return (m_dbConn != NULL);
}

/**
 * Get the status of the database
 */
bool DatabaseInstance::getServerStatus()
{
   MutexLock(m_serverStatusLock);
   bson_error_t error;
   bool sucess = true;
   if (m_serverStatus != NULL)
   {
      bson_destroy(m_serverStatus);
      delete m_serverStatus;
   }
   m_serverStatus = new bson_t;
   if (!mongoc_client_get_server_status(m_dbConn, NULL, m_serverStatus, &error)) {
#ifdef UNICODE
      TCHAR *_error = WideStringFromUTF8String(error.message);
      AgentWriteDebugLog(NXLOG_INFO, _T("MONGODB: Failed to run command: %s\n"), _error);
      safe_free(_error);
#else
      AgentWriteDebugLog(NXLOG_INFO, _T("MONGODB: Failed to run command: %s\n"), error.message);
#endif
      sucess = false;
      bson_destroy(m_serverStatus);
      delete m_serverStatus;
      m_serverStatus = NULL;
   }
   MutexUnlock(m_serverStatusLock);
   return sucess;
}

/**
 * Returns value of status parameter
 */
LONG DatabaseInstance::getStatusParam(const char *paramName, TCHAR *value)
{
   MutexLock(m_serverStatusLock);
   LONG result = getParam(m_serverStatus, paramName, value);
   MutexUnlock(m_serverStatusLock);
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
      AgentWriteDebugLog(NXLOG_DEBUG, _T("MONGODB: trying to get param from %s \n<<<<<<<"), _json);
      safe_free(_json);
#else
      AgentWriteDebugLog(NXLOG_INFO, _T("MONGODB: trying to get param from %s \n<<<<<<<"), json);
#endif // UNICODE
      bson_free (json);
   }

   if (bson_iter_init (&iter, bsonDoc) && bson_iter_find_descendant (&iter, paramName, &baz))
   {
      AgentWriteDebugLog(NXLOG_INFO, _T("MONGODB: Trying to get parameter. Type: %d"), bson_iter_type(&baz));
      char *val = (char *) malloc(sizeof(char) * MAX_RESULT_LENGTH);
      strncpy(val, "Unknown type", MAX_RESULT_LENGTH);
      switch(bson_iter_type(&baz))
      {
      case BSON_TYPE_UTF8:
         strncpy(val, bson_iter_utf8(&baz, NULL), MAX_RESULT_LENGTH);
         break;
      case BSON_TYPE_INT32:
         sprintf(val, "%d", bson_iter_int32(&baz));
         break;
      case BSON_TYPE_INT64:
         sprintf(val, "%ld", bson_iter_int64(&baz));
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
      safe_free(v);
#else
      ret_string(value, val);
#endif // UNICODE
      safe_free(val);
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
   if(m_serverStatus == NULL)
    return NULL;

   int attrSize = STATUSSIZE;
   NETXMS_SUBAGENT_PARAM *result = (NETXMS_SUBAGENT_PARAM *)malloc(sizeof(NETXMS_SUBAGENT_PARAM)*attrSize);
   bson_iter_t iter;
   bson_iter_t child;

   MutexLock(m_serverStatusLock);
   int i = 0;
   if (bson_iter_init (&iter, m_serverStatus))
   {
      while (bson_iter_next (&iter))
      {
         TCHAR name[MAX_PARAM_NAME];
         _tcscpy(name, _T("MongoDB."));
#ifdef UNICODE
         TCHAR *_name = WideStringFromUTF8String(bson_iter_key (&iter));
         _tcscat(name, _name);
         safe_free(_name);
#else
         _tcscat(name, bson_iter_key (&iter));
#endif // UNICODE
         if(!_tcsicmp(name, _T("MongoDB.ok")))
            continue;

         if (bson_iter_recurse (&iter, &child))
         {
            _tcscat(name, _T(".*"));
         }
         else
         {
            _tcscat(name, _T("(*)"));
         }
         AgentWriteDebugLog(NXLOG_INFO, _T("MONGODB: Param name added: %s"), name);
         if((i+1)>attrSize)
         {
            attrSize++;
            result = (NETXMS_SUBAGENT_PARAM *)realloc(result, sizeof(NETXMS_SUBAGENT_PARAM)*attrSize);
         }

         memset(result+i, 0, sizeof(NETXMS_SUBAGENT_PARAM));
         _tcscpy(result[i].name, name);
         result[i].handler = H_GetParameter;
         result[i].dataType = DCI_DT_STRING;
         //result[i].arg = NULL;
         _tcscmp(result[i].description, _T(""));
         i++;
      }
   }
   MutexUnlock(m_serverStatusLock);
   if(i<attrSize)
   {
      result = (NETXMS_SUBAGENT_PARAM *)realloc(result, sizeof(NETXMS_SUBAGENT_PARAM)*i);
   }
   *paramCount = i;
   return result;
}

LONG DatabaseInstance::setDbNames(StringList *value)
{
   if(m_databaseList == NULL)
      return SYSINFO_RC_ERROR;
   MutexLock(m_databaseListLock);
   for (int i = 0; m_databaseList[i]; i++)
   {
#ifdef UNICODE
      TCHAR *_name = WideStringFromUTF8String(m_databaseList[i]);
      value->add(_name);
      safe_free(_name);
#else
      value->add(m_databaseList[i]);
#endif
   }
   MutexUnlock(m_databaseListLock);
}

/**
 * Get parameter from command
 */
LONG DatabaseInstance::getOtherParam(const TCHAR *param, const TCHAR *arg, const TCHAR *command, TCHAR *value)
{
   TCHAR dbName[MAX_STR];
   if(!AgentGetParameterArg(param, 2, dbName, MAX_STR))
      return SYSINFO_RC_ERROR;
   LONG result = SYSINFO_RC_UNSUPPORTED;
   MutexLock(m_dataLock);
   MongoDBCommand *commandData = NULL;
   StringObjectMap<MongoDBCommand> * list = m_data->get(dbName);
   if(list != NULL)
   {
     commandData = list->get(command);
   }
   if(commandData != NULL)
   {
      if((GetCurrentTimeMs() - commandData->m_lastUpdateTime) > 60000)
      {
         result = commandData->getData(m_dbConn, dbName, command) ?  SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
      }
      else
      {
         result = SYSINFO_RC_SUCCESS;
      }
   }
   else
   {
      commandData = new MongoDBCommand();
      result = commandData->getData(m_dbConn, dbName, command) ?  SYSINFO_RC_SUCCESS : SYSINFO_RC_ERROR;
      list = new StringObjectMap<MongoDBCommand>(true);
      list->set(command,commandData);
      m_data->set(dbName,list);
   }

   if(result == SYSINFO_RC_SUCCESS)
   {
#ifdef UNICODE
      char *_arg = UTF8StringFromWideString(arg);
      result = getParam(&(commandData->m_result), _arg, value);
      safe_free(_arg);
#else
      result = getParam(&(commandData->m_result), arg, value);
#endif
   }

   MutexUnlock(m_dataLock);
   return result;
}


/**
 * Destructor
 */
DatabaseInstance::~DatabaseInstance()
{
   MutexDestroy(m_serverStatusLock);
   if(m_serverStatus != NULL)
   {
      bson_destroy (m_serverStatus);
      delete m_serverStatus;
   }
   mongoc_client_destroy (m_dbConn);
   ConditionDestroy(m_stopCondition);
   if(m_databaseList != NULL)
      bson_strfreev(m_databaseList);
   MutexDestroy(m_databaseListLock);
   MutexDestroy(m_dataLock);
   delete m_data;
}


bool MongoDBCommand::getData(mongoc_client_t *m_dbConn, const TCHAR *dbName, const TCHAR *command)
{
   bool sucess = false;
   m_lastUpdateTime = GetCurrentTimeMs();
   bson_error_t error;
#ifdef UNICODE
   char *_dbName = UTF8StringFromWideString(dbName);
   mongoc_database_t *database = mongoc_client_get_database (m_dbConn, _dbName);
   safe_free(_dbName);
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
      safe_free(_command);
#else
      bson_append_int32(&cmd, command, strlen(command), 1);
#endif // UNICODE
      sucess = mongoc_database_command_simple (database, &cmd, NULL, &m_result, &error);
      bson_destroy (&cmd);
      AgentWriteDebugLog(NXLOG_INFO, _T("MONGODB: Command %s executed with result %d"), command, sucess);
      if(!sucess)
      {
#ifdef UNICODE
         TCHAR *_error = WideStringFromUTF8String(error.message);
         AgentWriteDebugLog(NXLOG_INFO, _T("MONGODB: Failed to run command(%s): %s\n"), command, _error);
         safe_free(_error);
#else
         AgentWriteDebugLog(NXLOG_INFO, _T("MONGODB: Failed to run command(%s): %s\n"), command, error.message);
#endif
      }
      mongoc_database_destroy(database);
   }
   else
   {
      AgentWriteDebugLog(NXLOG_INFO, _T("MONGODB: Database not found: %s\n"), dbName);
      sucess = false;
   }
   return sucess;
}
