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

#ifndef _oracle_subagent_h_
#define _oracle_subagent_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_agent.h>
#include <nxdbapi.h>

//
// Misc defines
//

#define MAX_STR				(255)
#define MAX_QUERY			(8192)
#define MYNAMESTR			_T("oracle")
#define DB_NULLARG_MAGIC	_T("8201")

// Oracle-specific
#define MAX_USERNAME	(30+1)

/**
 * Database connection information
 */
struct DatabaseInfo
{
	TCHAR id[MAX_STR];				// instance ID
	TCHAR name[MAX_STR];
	TCHAR server[MAX_STR];
	TCHAR username[MAX_USERNAME];
	TCHAR password[MAX_PASSWORD];
	UINT32 connectionTTL;
};

/**
 * Table query descriptor
 */
struct TableDescriptor
{
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
	MUTEX m_dataLock;
	MUTEX m_sessionLock;
   CONDITION m_stopCondition;

   static THREAD_RESULT THREAD_CALL pollerThreadStarter(void *arg);

   void pollerThread();
   bool poll();
   int getOracleVersion();

public:
   DatabaseInstance(DatabaseInfo *info);
   ~DatabaseInstance();

   void run();
   void stop();

   const TCHAR *getId() { return m_info.id; }
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
   int minVersion;
   int instanceColumns;
   const TCHAR *query;
};

/**
 * Make version number
 */
#define MAKE_ORACLE_VERSION(major, minor) (((major) << 8) | (minor))

/**
 * Global variables
 */
extern DB_DRIVER g_oracleDriver;
extern DatabaseQuery g_queries[];

#endif   /* _oracle_subagent_h_ */
