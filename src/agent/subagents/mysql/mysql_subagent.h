/*
 ** NetXMS - Network Management System
 ** Subagent for Oracle monitoring
 ** Copyright (C) 2016 Raden Solutions
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

#ifndef _mysql_subagent_h_
#define _mysql_subagent_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_agent.h>
#include <nxdbapi.h>

/**
 * Database connection information
 */
struct DatabaseInfo
{
	TCHAR id[MAX_DB_STRING];				// instance ID
	TCHAR name[MAX_DB_STRING];
	TCHAR server[MAX_DB_STRING];
	TCHAR login[MAX_DB_LOGIN];
	TCHAR password[MAX_DB_PASSWORD];
	UINT32 connectionTTL;
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
   StringMap *m_data;
	MUTEX m_dataLock;
	MUTEX m_sessionLock;
   CONDITION m_stopCondition;

   static THREAD_RESULT THREAD_CALL pollerThreadStarter(void *arg);

   void pollerThread();
   bool poll();

public:
   DatabaseInstance(DatabaseInfo *info);
   ~DatabaseInstance();

   void run();
   void stop();

   const TCHAR *getId() { return m_info.id; }
   bool isConnected() { return m_connected; }

   bool getData(const TCHAR *tag, TCHAR *value);
};

/**
 * Global variables
 */
extern DB_DRIVER g_mysqlDriver;

#endif   /* _oracle_subagent_h_ */
