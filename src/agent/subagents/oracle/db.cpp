/*
 ** NetXMS - Network Management System
 ** Subagent for Oracle monitoring
 ** Copyright (C) 2009-2022 Raden Solutions
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
DatabaseInstance::DatabaseInstance(DatabaseInfo *info) : m_dataLock(MutexType::FAST), m_sessionLock(MutexType::NORMAL), m_stopCondition(true)
{
   memcpy(&m_info, info, sizeof(DatabaseInfo));
	m_pollerThread = INVALID_THREAD_HANDLE;
	m_session = nullptr;
	m_connected = false;
	m_version = 0;
   m_data = nullptr;
}

/**
 * Destructor
 */
DatabaseInstance::~DatabaseInstance()
{
   stop();
   delete m_data;
}

/**
 * Run
 */
void DatabaseInstance::run()
{
   m_pollerThread = ThreadCreateEx(this, &DatabaseInstance::pollerThread);
}

/**
 * Stop
 */
void DatabaseInstance::stop()
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
 * Detect Oracle DBMS version
 */
int DatabaseInstance::getOracleVersion() 
{
	DB_RESULT hResult = DBSelect(m_session, _T("SELECT version FROM v$instance"));
	if (hResult == nullptr)
		return 700;		// assume Oracle 7.0 by default

	TCHAR versionString[32];
	DBGetField(hResult, 0, 0, versionString, 32);
	int major = 0, minor = 0;
	_stscanf(versionString, _T("%d.%d"), &major, &minor);
	DBFreeResult(hResult);

	return MAKE_ORACLE_VERSION(major, minor);
}

/**
 * Poller thread
 */
void DatabaseInstance::pollerThread()
{
   nxlog_debug_tag(DEBUG_TAG_ORACLE, 3, _T("ORACLE: poller thread for database %s started"), m_info.id);
   int64_t connectionTTL = static_cast<int64_t>(m_info.connectionTTL) * _LL(1000);
   do
   {
reconnect:
      m_sessionLock.lock();

      TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
      m_session = DBConnect(g_oracleDriver, m_info.name, nullptr, m_info.username, m_info.password, nullptr, errorText);
      if (m_session == nullptr)
      {
         m_sessionLock.unlock();
         nxlog_debug_tag(DEBUG_TAG_ORACLE, 6, _T("Cannot connect to database %s: %s"), m_info.id, errorText);
         continue;
      }

      m_connected = true;
		DBEnableReconnect(m_session, false);
		DBSetLongRunningThreshold(m_session, 5000);  // Override global long running query threshold
      m_version = getOracleVersion();
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_ORACLE, _T("Connection with database %s restored (version %d.%d, connection TTL %d)"),
         m_info.id, m_version >> 8, m_version &0xFF, m_info.connectionTTL);

      m_sessionLock.unlock();

      int64_t pollerLoopStartTime = GetCurrentTimeMs();
      uint32_t sleepTime;
      do
      {
         int64_t startTime = GetCurrentTimeMs();
         if (!poll())
         {
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_ORACLE, _T("Connection with database %s lost"), m_info.id);
            break;
         }
         int64_t currTime = GetCurrentTimeMs();
         if (currTime - pollerLoopStartTime > connectionTTL)
         {
            nxlog_debug_tag(DEBUG_TAG_ORACLE, 4, _T("Planned connection reset"));
            m_sessionLock.lock();
            m_connected = false;
            DBDisconnect(m_session);
            m_session = NULL;
            m_sessionLock.unlock();
            goto reconnect;
         }
         int64_t elapsedTime = currTime - startTime;
         sleepTime = static_cast<uint32_t>((elapsedTime >= 60000) ? 60000 : (60000 - elapsedTime));
      }
      while(!m_stopCondition.wait(sleepTime));

      m_sessionLock.lock();
      m_connected = false;
      DBDisconnect(m_session);
      m_session = nullptr;
      m_sessionLock.unlock();
   }
   while(!m_stopCondition.wait(60000));   // reconnect every 60 seconds
   nxlog_debug_tag(DEBUG_TAG_ORACLE, 3, _T("Poller thread for database %s stopped"), m_info.id);
}

/**
 * Do actual database polling. Should return false if connection is broken.
 */
bool DatabaseInstance::poll()
{
   StringMap *data = new StringMap();

   int count = 0;
   int failures = 0;

   for(int i = 0; g_queries[i].name != nullptr; i++)
   {
      if (g_queries[i].minVersion > m_version)
         continue;   // not supported by this database

      count++;
      DB_RESULT hResult = DBSelect(m_session, g_queries[i].query);
      if (hResult == nullptr)
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
               size_t len = _tcslen(instance);
               if (len > 0)
                  instance[len++] = _T('|');
               DBGetField(hResult, row, col, &instance[len], 128 - len);
            }

            for(; col < numColumns; col++)
            {
               DBGetColumnName(hResult, col, &tag[tagBaseLen], 256 - tagBaseLen);
               size_t tagLen = _tcslen(tag);
               tag[tagLen++] = _T('@');
               _tcslcpy(&tag[tagLen], instance, 256 - tagLen);
               data->setPreallocated(MemCopyString(tag), DBGetField(hResult, row, col, nullptr, 0));
            }
         }
      }
      else
      {
         for(int col = 0; col < numColumns; col++)
         {
            DBGetColumnName(hResult, col, &tag[tagBaseLen], 256 - tagBaseLen);
            data->setPreallocated(MemCopyString(tag), DBGetField(hResult, 0, col, nullptr, 0));
         }
      }

      DBFreeResult(hResult);
   }

   // update cached data
   m_dataLock.lock();
   delete m_data;
   m_data = data;
   m_dataLock.unlock();

   return failures < count;
}

/**
 * Get collected data
 */
bool DatabaseInstance::getData(const TCHAR *tag, TCHAR *value)
{
   bool success = false;
   m_dataLock.lock();
   if (m_data != nullptr)
   {
      const TCHAR *v = m_data->get(tag);
      if (v != nullptr)
      {
         ret_string(value, v);
         success = true;
      }
   }
   m_dataLock.unlock();
   return success;
}

/**
 * Tag list callback data
 */
struct TagListCallbackData
{
   PCRE *preg;
   StringList *list;
};

/**
 * Tag list callback
 */
static EnumerationCallbackResult TagListCallback(const TCHAR *key, const TCHAR *value, TagListCallbackData *data)
{
   int pmatch[9];
   if (_pcre_exec_t(data->preg, NULL, reinterpret_cast<const PCRE_TCHAR*>(key), static_cast<int>(_tcslen(key)), 0, 0, pmatch, 9) >= 2) // MATCH
   {
      size_t slen = pmatch[3] - pmatch[2];
      TCHAR *s = MemAllocString(slen + 1);
      memcpy(s, &key[pmatch[2]], slen * sizeof(TCHAR));
      s[slen] = 0;
      data->list->addPreallocated(s);
   }
   return _CONTINUE;
}

/**
 * Get list of tags matching given pattern from collected data
 */
bool DatabaseInstance::getTagList(const TCHAR *pattern, StringList *value)
{
   bool success = false;
   m_dataLock.lock();
   if (m_data != nullptr)
   {
      const char *eptr;
      int eoffset;
      TagListCallbackData data;
      data.list = value;
      data.preg = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(pattern), PCRE_COMMON_FLAGS | PCRE_CASELESS, &eptr, &eoffset, nullptr);
	   if (data.preg != nullptr)
	   {
         m_data->forEach(TagListCallback, &data);
         _pcre_free_t(data.preg);
         success = true;
	   }
   }
   m_dataLock.unlock();
   return success;
}

/**
 * Query table
 */
bool DatabaseInstance::queryTable(TableDescriptor *td, Table *value)
{
   m_sessionLock.lock();
   
   if (!m_connected || (m_session == nullptr))
   {
      m_sessionLock.unlock();
      return false;
   }

   bool success = false;

   DB_RESULT hResult = DBSelect(m_session, td->query);
   if (hResult != nullptr)
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
            value->setPreallocated(col, DBGetField(hResult, row, col, nullptr, 0));
         }
      }

      DBFreeResult(hResult);
      success = true;
   }

   m_sessionLock.unlock();
   return success;
}
