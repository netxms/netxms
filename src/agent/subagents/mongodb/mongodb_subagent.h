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

#ifndef _mongodb_subagent_h_
#define _mongodb_subagent_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_agent.h>
//mongodb
#include <mongoc.h>
#include <bson.h>
#include <bcon.h>

/**
 * Misc defines
 */
#define MAX_STR				(256)

/**
 * Database connection information
 */
struct DatabaseInfo
{
	TCHAR id[MAX_STR];				// instance ID
	TCHAR server[MAX_STR];
	TCHAR username[MAX_STR];
	TCHAR password[MAX_STR];
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
 * As unique request id for selections and commands will be used current time: time(NULL)
 */
class DatabaseInstance
{
private:
   mongoc_client_t *m_dbConn;
	THREAD m_pollerThread;
   DatabaseInfo m_info;
   CONDITION m_stopCondition;
   //server status
	MUTEX m_serverStatusLock;
   bson_t *m_serverStatus;
   //Database list with size
   MUTEX m_databaseListLock;
   char **m_databaseList;

   static THREAD_RESULT THREAD_CALL pollerThreadStarter(void *arg);
   void pollerThread();
   bool getServerStatus();
   bool connect();

public:
   DatabaseInstance(DatabaseInfo *info);
   ~DatabaseInstance();

   void run();
   void stop();
   NETXMS_SUBAGENT_PARAM *getParameters(int *paramCount);
   bool connectionEstablished();
   const TCHAR *getConnectionName() { return m_info.id; }
   LONG getStatusParam(const char *paramName, TCHAR *value);
   LONG setDbNames(StringList *value);
   void getDatabases();
};

/**
 * Database instances
 */
extern ObjectArray<DatabaseInstance> *g_instances;

bool AddMongoDBFromConfig(const TCHAR *config);

#endif   /* _mongodb_subagent_h_ */
