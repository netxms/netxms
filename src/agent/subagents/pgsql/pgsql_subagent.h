/*
 ** NetXMS - Network Management System
 ** Subagent for PostgreSQL monitoring
 ** Copyright (C) 2009-2014 Raden Solutions
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

#ifndef _pgsql_subagent_h_
#define _pgsql_subagent_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_agent.h>
#include <nxdbapi.h>

#define DEBUG_TAG _T("sa.pgsql")

/**
 * Database connection information
 */
struct DatabaseInfo
{
	TCHAR id[MAX_DB_STRING];				// instance ID (connection name)
	TCHAR name[MAX_DB_STRING];				// maintenance DB name
	TCHAR server[MAX_DB_STRING];
	TCHAR login[MAX_DB_LOGIN];
	TCHAR password[MAX_DB_PASSWORD];
	uint32_t connectionTTL;
};

/**
 * Table query descriptor
 */
struct TableDescriptor
{
	int minVersion;
	const TCHAR *query;
	struct
	{
		int dataType;
		const TCHAR *displayName;
	} columns[32];
};

/**
 * Database instance
 */
class DatabaseInstance
{
private:
	DatabaseInfo m_info;
	THREAD m_pollerThread;
	DB_HANDLE m_session;
	bool m_connected;
	int m_version;
	StringMap *m_data;
	Mutex m_dataLock;
	Mutex m_sessionLock;
	Condition m_stopCondition;

	static THREAD_RESULT THREAD_CALL pollerThreadStarter(void *arg);

	void pollerThread();
	bool poll();
	int getPgsqlVersion();

public:
	DatabaseInstance(DatabaseInfo *info);
	~DatabaseInstance();

	void run();
	void stop();

	const TCHAR *getId() { return m_info.id; }
	const TCHAR *getName() { return m_info.name; }
	bool isConnected() { return m_connected; }
	int getVersion() { return m_version; }

	bool getData(const TCHAR *tag, TCHAR *value);
	bool getTagList(const TCHAR *pattern, StringList *value);
	bool queryTable(TableDescriptor *td, Table *value);
};

/**
 * Query for polling
 */
struct DatabaseQuery
{
	const TCHAR *name;
	int minVersion; // >= 
	int maxVersion; // <
	int instanceColumns;
	const TCHAR *query;
};

/**
 * Global variables
 */
extern DB_DRIVER g_pgsqlDriver;
extern DatabaseQuery g_queries[];

/**
 * Make version number
 */
#define MAKE_PGSQL_VERSION(ver1, ver2, ver3) ((((ver1) << 16) | (ver2) << 8) | (ver3))

#endif	/* _pgsql_subagent_h_ */
