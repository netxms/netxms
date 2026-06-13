/*
 ** NetXMS - Network Management System
 ** Subagent for Microsoft SQL Server monitoring
 ** Copyright (C) 2026 Raden Solutions
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

#ifndef _mssql_subagent_h_
#define _mssql_subagent_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_agent.h>
#include <nxdbapi.h>

#define DEBUG_TAG _T("mssql")

/**
 * Database connection information
 */
struct DatabaseInfo
{
   TCHAR id[MAX_DB_STRING];      // instance ID
   TCHAR server[MAX_DB_STRING];  // server address, optionally host\instance
   TCHAR name[MAX_DB_STRING];    // initial database name
   TCHAR login[MAX_DB_LOGIN];
   TCHAR password[MAX_DB_PASSWORD];
   uint32_t connectionTTL;
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
 * Polling query descriptor
 */
struct DatabaseQuery
{
   const TCHAR *name;
   int instanceColumns;
   const TCHAR *query;
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
   TCHAR m_version[64];
   StringMap *m_data;
   Mutex m_dataLock;
   Mutex m_sessionLock;
   Condition m_stopCondition;

   void pollerThread();
   bool poll();
   void readServerVersion();

public:
   DatabaseInstance(const DatabaseInfo *info);
   ~DatabaseInstance();

   void run();
   void stop();

   const TCHAR *getId() { return m_info.id; }
   bool isConnected() { return m_connected; }
   void getVersion(TCHAR *buffer, size_t size);

   bool getData(const TCHAR *tag, TCHAR *value);
   bool getTagList(const TCHAR *pattern, StringList *value);
   bool queryTable(TableDescriptor *td, Table *value);
};

/**
 * Global variables
 */
extern DB_DRIVER g_mssqlDriver;
extern DatabaseQuery g_queries[];

#endif   /* _mssql_subagent_h_ */
