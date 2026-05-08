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

static constexpr int64_t CACHE_MAX_AGE_MS = 300 * 1000;

/**
 * Create new database instance object
 */
DatabaseInstance::DatabaseInstance(DatabaseInfo *info) : m_dataLock(MutexType::FAST), m_sessionLock(MutexType::NORMAL), m_stopCondition(true)
{
	memcpy(&m_info, info, sizeof(DatabaseInfo));
	m_pollerThread = INVALID_THREAD_HANDLE;
	m_session = nullptr;
	m_connected = false;
	m_data = nullptr;
	m_version = 0;
	m_firstPollDone = false;
	m_lastSuccessfulPoll = 0;
	m_staleLogged = false;
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
	if (m_session != nullptr)
	{
		DBDisconnect(m_session);
		m_session = nullptr;
	}
}

/**
 * Detect PostgreSQL version
 */
int DatabaseInstance::getPgsqlVersion()
{
	DB_RESULT hResult = DBSelect(m_session, _T("SELECT current_setting('server_version_num')::int"));
	if (hResult == nullptr)
	{
		nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Failed to detect PostgreSQL version for database server %s - version-gated metrics will be skipped"), m_info.id);
		return 0;
	}

	int32_t v = DBGetFieldLong(hResult, 0, 0);
	DBFreeResult(hResult);

	if (v < 90000)
	{
		nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Detected unsupported PostgreSQL version (%d) for database server %s - version-gated metrics will be skipped"), v, m_info.id);
		return 0;
	}

	int maj, min, patch;
	if (v >= 100000)
	{
		maj = v / 10000;
		min = v % 10000;
		patch = 0;
	}
	else
	{
		maj = v / 10000;
		min = (v / 100) % 100;
		patch = v % 100;
	}
	return MAKE_PGSQL_VERSION(maj, min, patch);
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
   bool plannedReconnect = false;
   do
   {
      plannedReconnect = false;
		m_sessionLock.lock();

		TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
		m_session = DBConnect(g_pgsqlDriver, m_info.server, m_info.name, m_info.login, m_info.password, nullptr, errorText);
		if (m_session == nullptr)
		{
			m_sessionLock.unlock();
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

		m_sessionLock.unlock();

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
				plannedReconnect = true;
				break;
			}
			int64_t elapsedTime = currTime - startTime;
			// re-poll immediately if over budget; clamp negative deltas (clock jumped backwards) to no-wait
			sleepTime = (elapsedTime < 0 || elapsedTime >= 60000) ? 0u : static_cast<uint32_t>(60000 - elapsedTime);
		}
		while(!m_stopCondition.wait(sleepTime));

		m_sessionLock.lock();
		m_connected = false;
		DBDisconnect(m_session);
		m_session = nullptr;
		m_sessionLock.unlock();
	}
	while(!m_stopCondition.wait(plannedReconnect ? 0 : 60000));	// reconnect every 60s, or immediately on planned reconnect — but always honor stop request
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
		if (hResult == nullptr)
		{
			failures++;
			continue;
		}

		int rowCount = DBGetNumRows(hResult);
		if (rowCount == 0)
		{
		   DBFreeResult(hResult);
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

				// Process instance columns. break exits only this composition loop;
				// per-row metric emission below continues with the truncated instance.
				TCHAR instance[128];
				instance[0] = 0;
				const size_t instanceCap = sizeof(instance) / sizeof(TCHAR);
				for(col = 0; (col < g_queries[i].instanceColumns) && (col < numColumns); col++)
				{
					size_t len = _tcslen(instance);
					if (len > 0)
					{
						// need room for '|' + at least one char + NUL; otherwise stop without
						// overwriting the terminator and leaving the buffer non-NUL-terminated
						if (instanceCap - len < 3)
							break;
						instance[len++] = _T('|');
					}
					DBGetField(hResult, row, col, &instance[len], static_cast<int>(instanceCap - len));
				}
				// composition loop may have broken before all instance columns were consumed;
				// skip past them so the metric loop does not treat instance columns as metrics
				if (col < g_queries[i].instanceColumns)
					col = g_queries[i].instanceColumns;

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

	bool success = (failures < count);

	// update cached data
	m_dataLock.lock();
	if (success)
	{
		m_lastSuccessfulPoll = GetMonotonicClockTime();
		m_staleLogged = false;
		delete m_data;
		m_data = data;
	}
	else
	{
		// preserve previously cached data so the TTL window can serve it during reconnect
		delete data;
	}
	m_dataLock.unlock();

	// Only mark first poll as done when a poll actually published data — otherwise the
	// guard in handlers would let `?`-prefixed metrics fall through to the missing-as-zero
	// path while m_data is still null, producing phantom zeros.
	if (success)
		m_firstPollDone = true;

	return success;
}

/**
 * Check whether cached data has exceeded the maximum age. Caller must hold m_dataLock —
 * uses the monotonic clock so it is immune to wall-clock jumps in either direction.
 */
bool DatabaseInstance::isCacheStaleLocked() const
{
	return (m_lastSuccessfulPoll != 0) &&
		(GetMonotonicClockTime() - m_lastSuccessfulPoll > CACHE_MAX_AGE_MS);
}

/**
 * Get collected data. Returns Found/Missing/Stale so the caller can distinguish "no such tag"
 * (where the ?-prefix "missing as zero" rule applies) from "cache exceeded TTL" (always an error).
 */
CacheReadResult DatabaseInstance::getData(const TCHAR *tag, TCHAR *value)
{
	CacheReadResult result = CacheReadResult::Missing;
	bool shouldLog = false;
	int64_t ageSeconds = 0;
	m_dataLock.lock();
	if (isCacheStaleLocked())
	{
		shouldLog = !m_staleLogged;
		m_staleLogged = true;
		ageSeconds = (GetMonotonicClockTime() - m_lastSuccessfulPoll) / 1000;
		result = CacheReadResult::Stale;
	}
	else if (m_data != nullptr)
	{
		const TCHAR *v = m_data->get(tag);
		if (v != nullptr)
		{
			ret_string(value, v);
			result = CacheReadResult::Found;
		}
	}
	m_dataLock.unlock();

	if (shouldLog)
		nxlog_debug_tag(DEBUG_TAG, 4, _T("Cached data for %s is stale (last poll %lld seconds ago)"), m_info.id, static_cast<long long>(ageSeconds));

	return result;
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
	if (_pcre_exec_t(data->preg, nullptr, reinterpret_cast<const PCRE_TCHAR*>(key), static_cast<int>(_tcslen(key)), 0, 0, pmatch, 9) >= 2) // MATCH
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
	bool shouldLog = false;
	int64_t ageSeconds = 0;
	m_dataLock.lock();
	if (isCacheStaleLocked())
	{
		shouldLog = !m_staleLogged;
		m_staleLogged = true;
		ageSeconds = (GetMonotonicClockTime() - m_lastSuccessfulPoll) / 1000;
	}
	else if (m_data != nullptr)
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
	m_dataLock.unlock();

	if (shouldLog)
		nxlog_debug_tag(DEBUG_TAG, 4, _T("Cached data for %s is stale (last poll %lld seconds ago)"), m_info.id, static_cast<long long>(ageSeconds));

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
	if (hResult != NULL)
	{
		int numColumns = DBGetColumnCount(hResult);
		if (numColumns > MAX_TABLE_COLUMNS)
		{
			nxlog_debug_tag(DEBUG_TAG, 3, _T("queryTable: query returned %d columns, truncating to MAX_TABLE_COLUMNS (%d)"), numColumns, MAX_TABLE_COLUMNS);
			numColumns = MAX_TABLE_COLUMNS;
		}
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
