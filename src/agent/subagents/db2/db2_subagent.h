/**
 * NetXMS - open source network management system
 * Copyright (C) 2013-2025 Raden Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef DB2_SUBAGENT_H_
#define DB2_SUBAGENT_H_

#include <nms_agent.h>
#include <nxdbapi.h>
#include <nms_util.h>
#include <nms_common.h>

#include "db2dci.h"

#define SUBAGENT_NAME      _T("DB2")
#define DEBUG_TAG_DB2      _T("db2")
#define STR_MAX            256
#define QUERY_MAX          2048
#define DCI_LIST_SIZE      53

/**
 * DB2 constants
 */
#ifdef _WIN32
#define DB2_MAX_USER_NAME 32 + 1
#else
#define DB2_MAX_USER_NAME 8 + 1
#endif

/**
 * Database connection information
 */
struct ConnectionInfo
{
   TCHAR id[STR_MAX];                    // connection ID (subsection name)
   TCHAR endpoint[STR_MAX];              // catalog alias = connect target
   TCHAR database[STR_MAX];              // DB2 database name
   TCHAR username[DB2_MAX_USER_NAME];
   TCHAR password[MAX_PASSWORD];
   uint32_t connectionTTL;
};

/**
 * Query for polling
 */
struct QUERY
{
   Dci dciList[DCI_LIST_SIZE];
   TCHAR query[QUERY_MAX];
};

/**
 * Database connection
 */
class DatabaseConnection
{
private:
   ConnectionInfo m_info;
   THREAD m_pollerThread;
   DB_HANDLE m_session;
   bool m_connected;
   bool m_dataValid;
   TCHAR m_data[NUM_OF_DCI][STR_MAX];
   Mutex m_dataLock;
   Mutex m_sessionLock;
   Condition m_stopCondition;

   void pollerThread();
   bool poll();

public:
   DatabaseConnection(ConnectionInfo *info);
   ~DatabaseConnection();

   void run();
   void stop();

   const TCHAR *getId() { return m_info.id; }
   bool isConnected() { return m_connected; }

   bool getData(Dci dci, TCHAR *value);
};

/**
 * Global variables
 */
extern DB_DRIVER g_db2Driver;
extern QUERY g_queries[];

#endif /* DB2_SUBAGENT_H_ */
