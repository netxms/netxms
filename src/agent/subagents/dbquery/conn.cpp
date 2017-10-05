/*
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
**
** File: direct.cpp
**
**/

#include "dbquery.h"

/**
 * Database connections
 */
static ObjectArray<DBConnection> s_dbConnections(8, 8, true);
static MUTEX s_dbConnectionsLock = MutexCreate();

/**
 * DB connection constructor
 */
DBConnection::DBConnection()
{
   m_id = NULL;
	m_driver = NULL;
	m_server = NULL;
	m_dbName = NULL;
	m_login = NULL;
	m_password = NULL;
	m_hDriver = NULL;
	m_hdb = NULL;
}

/**
 * DB connection destructor
 */
DBConnection::~DBConnection()
{
   free(m_id);
   free(m_driver);
   free(m_server);
   free(m_dbName);
   free(m_login);
   free(m_password);
   if (m_hdb != NULL)
      DBDisconnect(m_hdb);
   if (m_hDriver != NULL)
      DBUnloadDriver(m_hDriver);
}

/**
 * Read attribute from config string
 */
static TCHAR *ReadAttribute(const TCHAR *config, const TCHAR *attribute)
{
   TCHAR buffer[256];
   if (!ExtractNamedOptionValue(config, attribute, buffer, 256))
      return NULL;
   return _tcsdup(buffer);
}

/**
 * Create DB connection from config
 */
DBConnection *DBConnection::createFromConfig(const TCHAR *config)
{
   DBConnection *conn = new DBConnection();
   conn->m_id = ReadAttribute(config, _T("id"));
   conn->m_driver = ReadAttribute(config, _T("driver"));
   conn->m_server = ReadAttribute(config, _T("server"));
   conn->m_dbName = ReadAttribute(config, _T("dbname"));
   conn->m_login = ReadAttribute(config, _T("login"));
   conn->m_password = ReadAttribute(config, _T("password"));

   if(conn->m_password == NULL)
      conn->m_password = ReadAttribute(config, _T("encryptedPassword"));

   if (conn->m_password != NULL)
      DecryptPassword(CHECK_NULL_EX(conn->m_login), conn->m_password, conn->m_password, _tcslen(conn->m_password));

   if ((conn->m_id == NULL) || (conn->m_driver == NULL))
   {
      delete conn;
      return NULL;
   }

   conn->m_hDriver = DBLoadDriver(conn->m_driver, _T(""), false, NULL, NULL);
   if (conn->m_hDriver == NULL)
   {
      delete conn;
      return NULL;
   }

   conn->connect();
   return conn;
}

/**
 * Connect to database
 */
bool DBConnection::connect()
{
   if (m_hdb != NULL)
      DBDisconnect(m_hdb);

   TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
   m_hdb = DBConnect(m_hDriver, m_server, m_dbName, m_login, m_password, NULL, errorText);
   if (m_hdb != NULL)
   {
      AgentWriteLog(NXLOG_INFO, _T("DBQUERY: connected to database %s"), m_id);
   }
   else
   {
      AgentWriteLog(NXLOG_WARNING, _T("DBQUERY: cannot connect to database %s (%s)"), m_id, errorText);
   }
   return m_hdb != NULL;
}

/**
 * Add database connection to the list from config
 */
bool AddDatabaseFromConfig(const TCHAR *db)
{
   DBConnection *conn = DBConnection::createFromConfig(db);
   if (conn == NULL)
      return false;

   MutexLock(s_dbConnectionsLock);
   s_dbConnections.add(conn);
   MutexUnlock(s_dbConnectionsLock);
   return true;
}

/**
 * Shutdown all DB connections
 */
void ShutdownConnections()
{
   MutexLock(s_dbConnectionsLock);
   s_dbConnections.clear();
   MutexUnlock(s_dbConnectionsLock);
}

/**
 * Get connection handle for given database
 */
DB_HANDLE GetConnectionHandle(const TCHAR *dbid)
{
   DB_HANDLE hdb = NULL;
   MutexLock(s_dbConnectionsLock);
   for(int i = 0; i < s_dbConnections.size(); i++)
      if (!_tcsicmp(dbid, s_dbConnections.get(i)->getId()))
      {
         hdb = s_dbConnections.get(i)->getHandle();
         if (hdb == NULL)
         {
            // Try to (re)connect to database
            s_dbConnections.get(i)->connect();
            hdb = s_dbConnections.get(i)->getHandle();
         }
         break;
      }
   MutexUnlock(s_dbConnectionsLock);
   return hdb;
}
