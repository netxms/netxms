/*
 ** NetXMS - Network Management System
 ** Subagent for PostgreSQL monitoring
 ** Copyright (C) 2009-2019 Raden Solutions
 ** Copyright (C) 2020 Petr Votava
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

#include "pgsql_subagent.h"
#include <netxms-regex.h>

/**
 * Create new database instance object
 */
DatabaseInstance::DatabaseInstance(DatabaseInfo *info)
{
	memcpy(&m_info, info, sizeof(DatabaseInfo));
	m_pollerThread = INVALID_THREAD_HANDLE;
	m_session = nullptr;
	m_connected = false;
	m_data = nullptr;
	m_dataLock = MutexCreate();
	m_sessionLock = MutexCreate();
	m_stopCondition = ConditionCreate(true);
	m_version = 0;
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
 * Detect PosgreSQL version
 */
int DatabaseInstance::getPgsqlVersion() 
{
	DB_RESULT hResult = DBSelect(m_session, _T("SELECT substring(version() from '(\\\\d+\\.\\\\d+(\\\\.\\\\d+)?)')"));
	if (hResult == NULL)	
	{
		return 0;
	}

	TCHAR versionString[16];
	DBGetField(hResult, 0, 0, versionString, 16);
	int ver1 = 0, ver2 = 0, ver3 = 0;
	_stscanf(versionString, _T("%d.%d.%d"), &ver1, &ver2, &ver3);
	DBFreeResult(hResult);

	return MAKE_PGSQL_VERSION(ver1, ver2, ver3);
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
   nxlog_debug_tag(DEBUG_TAG, 3, _T("PGSQL: poller thread for database server %s started"), m_info.id);
   INT64 connectionTTL = (INT64)m_info.connectionTTL * _LL(1000);
   do
   {
reconnect:
		MutexLock(m_sessionLock);

		TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
		m_session = DBConnect(g_pgsqlDriver, m_info.server, m_info.name, m_info.login, m_info.password, nullptr, errorText);
		if (m_session == nullptr)
		{
			MutexUnlock(m_sessionLock);
			nxlog_debug_tag(DEBUG_TAG, 5, _T("Cannot connect to PostgreSQL database server %s (%s)"), m_info.id, errorText);
			continue;
		}

		m_connected = true;
		DBEnableReconnect(m_session, false);
		m_version = getPgsqlVersion();
		if ((m_version & 0xFF) == 0)
		   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Connection with PostgreSQL database server %s restored (version %d.%d, connection TTL %d)"),
				m_info.id, m_version >> 16, (m_version >> 8) & 0xFF, m_info.connectionTTL);
		else
		   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Connection with PostgreSQL database server %s restored (version %d.%d.%d, connection TTL %d)"),
				m_info.id, m_version >> 16, (m_version >> 8) & 0xFF, m_version & 0xFF, m_info.connectionTTL);

		MutexUnlock(m_sessionLock);

		int64_t pollerLoopStartTime = GetCurrentTimeMs();
		uint32_t sleepTime;
		do
		{
		   int64_t startTime = GetCurrentTimeMs();
			if (!poll())
			{
				nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Connection with PostgreSQL database server %s is lost"), m_info.id);
				break;
			}
			int64_t currTime = GetCurrentTimeMs();
			if (currTime - pollerLoopStartTime > connectionTTL)
			{
				nxlog_debug_tag(DEBUG_TAG, 4, _T("Planned connection reset for database %s"), m_info.id);
				MutexLock(m_sessionLock);
				m_connected = false;
				DBDisconnect(m_session);
				m_session = nullptr;
				MutexUnlock(m_sessionLock);
				goto reconnect;
			}
			int64_t elapsedTime = currTime - startTime;
			sleepTime = (UINT32)((elapsedTime >= 60000) ? 60000 : (60000 - elapsedTime));
		}
		while(!ConditionWait(m_stopCondition, sleepTime));

		MutexLock(m_sessionLock);
		m_connected = false;
		DBDisconnect(m_session);
		m_session = NULL;
		MutexUnlock(m_sessionLock);
	}
	while(!ConditionWait(m_stopCondition, 60000));	// reconnect every 60 seconds
	nxlog_debug_tag(DEBUG_TAG, 3, _T("Poller thread for database server %s stopped"), m_info.id);
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
			continue;	// not supported by this database

		if (g_queries[i].maxVersion != 0 && g_queries[i].maxVersion <= m_version)
			continue;	// not supported by this database

		count++;
		DB_RESULT hResult = DBSelect(m_session, g_queries[i].query);
		if (hResult == NULL)
		{
			failures++;
			continue;
		}

		int rowCount = DBGetNumRows(hResult);
		if (rowCount == 0)
		{
			continue;
		}

		TCHAR tag[256];
		_tcscpy(tag, g_queries[i].name);
		int tagBaseLen = (int)_tcslen(tag);
		tag[tagBaseLen++] = _T('/');

		int numColumns = DBGetColumnCount(hResult);
		if (g_queries[i].instanceColumns > 0)
		{
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
	MutexLock(m_dataLock);
	if (m_data != NULL)
	{
		const char *eptr;
		int eoffset;
		TagListCallbackData data;
		data.list = value;
		data.preg = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(pattern), PCRE_COMMON_FLAGS | PCRE_CASELESS, &eptr, &eoffset, NULL);
		if (data.preg != NULL)
		{
			m_data->forEach(TagListCallback, &data);
			_pcre_free_t(data.preg);
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
				value->setPreallocated(col, DBGetField(hResult, row, col, nullptr, 0));
			}
		}

		DBFreeResult(hResult);
		success = true;
	}

	MutexUnlock(m_sessionLock);
	return success;
}
