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

#include "oracle_subagent.h"
#include <netxms-regex.h>

/**
 * Create new database instance object
 */
DatabaseInstance::DatabaseInstance(DatabaseInfo *info)
{
   memcpy(&m_info, info, sizeof(DatabaseInfo));
	m_pollerThread = INVALID_THREAD_HANDLE;
	m_session = NULL;
	m_connected = false;
	m_version = 0;
   m_data = NULL;
	m_dataLock = MutexCreate();
	m_sessionLock = MutexCreate();
   m_stopCondition = ConditionCreate(TRUE);
}

/**
 * Destructor
 */
DatabaseInstance::~DatabaseInstance()
{
   stop();
   MutexDestroy(m_dataLock);
   MutexDestroy(m_sessionLock);
   ConditionDestroy(m_stopCondition);
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
   ConditionSet(m_stopCondition);
   ThreadJoin(m_pollerThread);
   m_pollerThread = INVALID_THREAD_HANDLE;
   if (m_session != NULL)
   {
      DBDisconnect(m_session);
      m_session = NULL;
   }
}

/**
 * Detect Oracle DBMS version
 */
int DatabaseInstance::getOracleVersion() 
{
	DB_RESULT hResult = DBSelect(m_session, _T("SELECT version FROM v$instance"));
	if (hResult == NULL)	
	{
		return 700;		// assume Oracle 7.0 by default
	}

	TCHAR versionString[32];
	DBGetField(hResult, 0, 0, versionString, 32);
	int major = 0, minor = 0;
	_stscanf(versionString, _T("%d.%d"), &major, &minor);
	DBFreeResult(hResult);

	return MAKE_ORACLE_VERSION(major, minor);
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
   AgentWriteDebugLog(3, _T("ORACLE: poller thread for database %s started"), m_info.id);
   do
   {
      MutexLock(m_sessionLock);

      TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
      m_session = DBConnect(g_driverHandle, m_info.name, NULL, m_info.username, m_info.password, NULL, errorText);
      if (m_session == NULL)
      {
         MutexUnlock(m_sessionLock);
         AgentWriteDebugLog(6, _T("ORACLE: cannot connect to database %s: %s"), m_info.id, errorText);
         continue;
      }

      m_connected = true;
		DBEnableReconnect(m_session, false);
      m_version = getOracleVersion();
      AgentWriteLog(NXLOG_INFO, _T("ORACLE: connection with database %s restored (version %d.%d)"), 
         m_info.id, m_version >> 8, m_version &0xFF);

      MutexUnlock(m_sessionLock);

      UINT32 sleepTime;
      do
      {
         INT64 startTime = GetCurrentTimeMs();
         if (!poll())
         {
            AgentWriteLog(NXLOG_WARNING, _T("ORACLE: connection with database %s lost"), m_info.id);
            break;
         }
         INT64 elapsedTime = GetCurrentTimeMs() - startTime;
         sleepTime = (UINT32)((elapsedTime >= 60000) ? 60000 : (60000 - elapsedTime));
      }
      while(!ConditionWait(m_stopCondition, sleepTime));

      MutexLock(m_sessionLock);
      m_connected = false;
      DBDisconnect(m_session);
      m_session = NULL;
      MutexUnlock(m_sessionLock);
   }
   while(!ConditionWait(m_stopCondition, 60000));   // reconnect every 60 seconds
   AgentWriteDebugLog(3, _T("ORACLE: poller thread for database %s stopped"), m_info.id);
}

/**
 * Do actual database polling. Should return false if connection is broken.
 */
bool DatabaseInstance::poll()
{
   StringMap *data = new StringMap();

   int count = 0;
   int failures = 0;

   for(int i = 0; g_queries[i].name != NULL; i++)
   {
      if (g_queries[i].minVersion > m_version)
         continue;   // not supported by this database

      count++;
      DB_RESULT hResult = DBSelect(m_session, g_queries[i].query);
      if (hResult == NULL)
      {
         failures++;
         continue;
      }

      TCHAR tag[256];
      _tcscpy(tag, g_queries[i].name);
      int tagBaseLen = (int)_tcslen(tag);
      tag[tagBaseLen++] = _T('/');

      int numColumns = DBGetColumnCount(hResult);
      if (g_queries[i].instanceColumns > 0)
      {
         int rowCount = DBGetNumRows(hResult);
         for(int row = 0; row < rowCount; row++)
         {
            int col;

            // Process instance columns
            TCHAR instance[128];
            instance[0] = 0;
            for(col = 0; (col < g_queries[i].instanceColumns) && (col < numColumns); col++)
            {
               int len = (int)_tcslen(instance);
               if (len > 0)
                  instance[len++] = _T('|');
               DBGetField(hResult, row, col, &instance[len], 128 - len);
            }

            for(; col < numColumns; col++)
            {
               DBGetColumnName(hResult, col, &tag[tagBaseLen], 256 - tagBaseLen);
               size_t tagLen = _tcslen(tag);
               tag[tagLen++] = _T('@');
               nx_strncpy(&tag[tagLen], instance, 256 - tagLen);
               data->setPreallocated(_tcsdup(tag), DBGetField(hResult, row, col, NULL, 0));
            }
         }
      }
      else
      {
         for(int col = 0; col < numColumns; col++)
         {
            DBGetColumnName(hResult, col, &tag[tagBaseLen], 256 - tagBaseLen);
            data->setPreallocated(_tcsdup(tag), DBGetField(hResult, 0, col, NULL, 0));
         }
      }

      DBFreeResult(hResult);
   }

   // update cached data
   MutexLock(m_dataLock);
   delete m_data;
   m_data = data;
   MutexUnlock(m_dataLock);

   return failures < count;
}

/**
 * Get collected data
 */
bool DatabaseInstance::getData(const TCHAR *tag, TCHAR *value)
{
   bool success = false;
   MutexLock(m_dataLock);
   if (m_data != NULL)
   {
      const TCHAR *v = m_data->get(tag);
      if (v != NULL)
      {
         ret_string(value, v);
         success = true;
      }
   }
   MutexUnlock(m_dataLock);
   return success;
}

/**
 * Tag list callback data
 */
struct TagListCallbackData
{
   regex_t preg;
   StringList *list;
};

/**
 * Tag list callback
 */
static EnumerationCallbackResult TagListCallback(const TCHAR *key, const void *value, void *data)
{
   regmatch_t pmatch[16];
   if (_tregexec(&(((TagListCallbackData *)data)->preg), key, 16, pmatch, 0) == 0) // MATCH
   {
      if (pmatch[1].rm_so != -1)
      {
         size_t slen = pmatch[1].rm_eo - pmatch[1].rm_so;
         TCHAR *s = (TCHAR *)malloc((slen + 1) * sizeof(TCHAR));
         memcpy(s, &key[pmatch[1].rm_so], slen * sizeof(TCHAR));
         s[slen] = 0;
         ((TagListCallbackData *)data)->list->addPreallocated(s);
      }
   }
   return _CONTINUE;
}

/**
 * Get list of tags matching given pattern from collected data
 */
bool DatabaseInstance::getTagList(const TCHAR *pattern, StringList *value)
{
   bool success = false;

   MutexLock(m_dataLock);
   if (m_data != NULL)
   {
      TagListCallbackData data;
      data.list = value;
	   if (_tregcomp(&data.preg, pattern, REG_EXTENDED | REG_ICASE) == 0)
	   {
         m_data->forEach(TagListCallback, &data);
         regfree(&data.preg);
         success = true;
	   }
   }
   MutexUnlock(m_dataLock);
   return success;
}

/**
 * Query table
 */
bool DatabaseInstance::queryTable(TableDescriptor *td, Table *value)
{
   MutexLock(m_sessionLock);
   
   if (!m_connected || (m_session == NULL))
   {
      MutexUnlock(m_sessionLock);
      return false;
   }

   bool success = false;

   DB_RESULT hResult = DBSelect(m_session, td->query);
   if (hResult != NULL)
   {
      int numColumns = DBGetColumnCount(hResult);
      for(int col = 0; col < numColumns; col++)
      {
         TCHAR name[64];
         DBGetColumnName(hResult, col, name, 64);
         value->addColumn(name, td->columns[col].dataType, td->columns[col].displayName, col == 0);
      }

      int numRows = DBGetNumRows(hResult);
      for(int row = 0; row < numRows; row++)
      {
         value->addRow();
         for(int col = 0; col < numColumns; col++)
         {
            value->setPreallocated(col, DBGetField(hResult, row, col, NULL, 0));
         }
      }

      DBFreeResult(hResult);
      success = true;
   }

   MutexUnlock(m_sessionLock);
   return success;
}
