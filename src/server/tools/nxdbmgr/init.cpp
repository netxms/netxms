/* 
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2024 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: init.cpp
**
**/

#include "nxdbmgr.h"

/**
 * Check if query is empty
 */
static bool IsEmptyQuery(const char *query)
{
   for (const char *ptr = query; *ptr != 0; ptr++)
      if ((*ptr != ' ') && (*ptr != '\t') && (*ptr != '\r') && (*ptr != '\n'))
         return false;
   return true;
}

/**
 * Find end of query in batch
 */
static char *FindEndOfQuery(char *start, char *batchEnd)
{
   char *ptr;
   int iState;
   bool proc = false;
   bool procEnd = false;

   for(ptr = start, iState = 0; (ptr < batchEnd) && (iState != -1); ptr++)
   {
      switch(iState)
      {
         case 0:
            if (*ptr == '\'')
            {
               iState = 1;
            }
            else if ((*ptr == ';') && !proc && !procEnd)
            {
               iState = -1;
            }
            else if ((*ptr == '/') && procEnd)
            {
               procEnd = false;
               iState = -1;
            }
            else if ((*ptr == 'C') || (*ptr == 'c'))
            {
               if (!strnicmp(ptr, "CREATE FUNCTION", 15) ||
                   !strnicmp(ptr, "CREATE OR REPLACE FUNCTION", 26) ||
                   !strnicmp(ptr, "CREATE PROCEDURE", 16) ||
                   !strnicmp(ptr, "CREATE OR REPLACE PROCEDURE", 27))
               {
                  proc = true;
               }
            }
            else if (proc && ((*ptr == 'E') || (*ptr == 'e')))
            {
               if (!strnicmp(ptr, "END", 3))
               {
                  proc = false;
                  procEnd = true;
               }
            }
				else if ((*ptr == '\r') || (*ptr == '\n'))
				{
					// CR/LF should be replaced with spaces, otherwise at least
					// Oracle will fail on CREATE FUNCTION / CREATE PROCEDURE
					*ptr = ' ';
				}
            break;
         case 1:
            if (*ptr == '\'')
               iState = 0;
            break;
      }
   }

   *(ptr - 1) = 0;
   return ptr + 1;
}

/**
 * Execute SQL batch file. If file name contains @dbengine@ macro,
 * it will be replaced with current database engine name in lowercase
 */
bool ExecSQLBatch(const char *batchFile, bool showOutput)
{
   size_t size;
   char *batch = reinterpret_cast<char*>(LoadFileA(strcmp(batchFile, "-") ? batchFile : nullptr, &size));
   if (batch == nullptr)
   {
      if (strcmp(batchFile, "-"))
         _tprintf(_T("ERROR: Cannot load SQL command file %hs\n"), batchFile);
      else
         _tprintf(_T("ERROR: Cannot load SQL command file standard input\n"));
      return false;
   }

   bool result = false;
   char *next;
   for(char *query = batch; query < batch + size; query = next)
   {
      next = FindEndOfQuery(query, batch + size);
      if (!IsEmptyQuery((char *)query))
      {
         wchar_t *wcQuery = WideStringFromMBString(query);
         result = SQLQuery(wcQuery, showOutput);
         MemFree(wcQuery);
         if (!result)
            next = batch + size;
      }
   }
   MemFree(batch);
   return result;
}

/**
 * Initialize database
 */
int InitDatabase(const char *initFile)
{
   TCHAR query[256];

   _tprintf(_T("Initializing database...\n"));
   if (!ExecSQLBatch(initFile, false))
      goto init_failed;

   // Generate GUID for user "system"
   _sntprintf(query, 256, _T("UPDATE users SET guid='%s' WHERE id=0"), uuid::generate().toString().cstr());
   if (!SQLQuery(query))
      goto init_failed;

   // Generate GUID for user "admin"
   _sntprintf(query, 256, _T("UPDATE users SET guid='%s' WHERE id=1"), uuid::generate().toString().cstr());
   if (!SQLQuery(query))
      goto init_failed;

   // Generate GUID for user "anonymous"
   _sntprintf(query, 256, _T("UPDATE users SET guid='%s' WHERE id=2"), uuid::generate().toString().cstr());
   if (!SQLQuery(query))
      goto init_failed;

   // Generate GUID for "everyone" group
   _sntprintf(query, 256, _T("UPDATE user_groups SET guid='%s' WHERE id=%d"), uuid::generate().toString().cstr(), GROUP_EVERYONE);
   if (!SQLQuery(query))
      goto init_failed;

   // Generate GUID for "Admins" group
   _sntprintf(query, 256, _T("UPDATE user_groups SET guid='%s' WHERE id=1073741825"), uuid::generate().toString().cstr());
   if (!SQLQuery(query))
      goto init_failed;

   _tprintf(_T("Database initialized successfully\n"));
   return 0;

init_failed:
   _tprintf(_T("Database initialization failed\n"));
   return 10;
}

/**
 * Create database in MySQL or MariaDB
 */
static bool CreateDatabase_MySQL(const TCHAR *dbName, const TCHAR *dbLogin, const TCHAR *dbPassword)
{
   TCHAR query[256];
   _sntprintf(query, 256, _T("CREATE DATABASE %s"), dbName);
   bool success = SQLQuery(query);

   if (success)
   {
      _sntprintf(query, 256, _T("CREATE USER %s IDENTIFIED BY %s"), dbLogin, DBPrepareString(g_dbHandle, dbPassword).cstr());
      success = SQLQuery(query);
   }

   if (success)
   {
      _sntprintf(query, 256, _T("CREATE USER %s@localhost IDENTIFIED BY %s"), dbLogin, DBPrepareString(g_dbHandle, dbPassword).cstr());
      success = SQLQuery(query);
   }

   if (success)
   {
      _sntprintf(query, 256, _T("GRANT ALL ON %s.* TO %s"), dbName, dbLogin);
      success = SQLQuery(query);
   }

   if (success)
   {
      _sntprintf(query, 256, _T("GRANT ALL ON %s.* TO %s@localhost"), dbName, dbLogin);
      success = SQLQuery(query);
   }

   if (success)
      success = SQLQuery(_T("FLUSH PRIVILEGES"));
   return success;
}

/**
 * Create database (actually user) in Oracle
 */
static bool CreateDatabase_Oracle(const TCHAR *dbLogin, const TCHAR *dbPassword)
{
   TCHAR query[256];
   _sntprintf(query, 256, _T("CREATE USER %s IDENTIFIED BY %s"), dbLogin, dbPassword);
   bool success = SQLQuery(query);

   if (success)
   {
      _sntprintf(query, 256, _T("ALTER USER %s QUOTA UNLIMITED ON USERS"), dbLogin);
      success = SQLQuery(query);
   }

   if (success)
   {
      _sntprintf(query, 256, _T("GRANT CREATE SESSION, RESOURCE, CREATE VIEW TO %s"), dbLogin);
      success = SQLQuery(query);
   }

   return success;
}

/**
 * Create database in PostgreSQL
 */
static bool CreateDatabase_PostgreSQL(const TCHAR *dbName, const TCHAR *dbLogin, const TCHAR *dbPassword)
{
   TCHAR query[256];
   _sntprintf(query, 256, _T("CREATE USER \"%s\" WITH PASSWORD %s"), dbLogin, DBPrepareString(g_dbHandle, dbPassword).cstr());
   bool success = SQLQuery(query);

   if (success)
   {
      _sntprintf(query, 256, _T("CREATE DATABASE \"%s\" OWNER \"%s\""), dbName, dbLogin);
      success = SQLQuery(query);
   }

   return success;
}

/**
 * Create database in Microsoft SQL
 */
static bool CreateDatabase_MSSQL(const TCHAR *dbName, const TCHAR *dbLogin, const TCHAR *dbPassword)
{
   TCHAR query[512];

   bool success = SQLQuery(_T("USE master"));
   if (success)
   {
      _sntprintf(query, 512, _T("CREATE DATABASE %s"), dbName);
      success = SQLQuery(query);
   }

   if (success)
   {
      _sntprintf(query, 512, _T("USE %s"), dbName);
      success = SQLQuery(query);
   }

   // TODO: implement grant for Windows authentication
   if (success && _tcscmp(dbLogin, _T("*")))
   {
      _sntprintf(query, 512, _T("sp_addlogin @loginame = '%s', @passwd = '%s', @defdb = '%s'"), dbLogin, dbPassword, dbName);
      success = SQLQuery(query);

      if (success)
      {
         _sntprintf(query, 512, _T("sp_grantdbaccess @loginame = '%s'"), dbLogin);
         success = SQLQuery(query);
      }

      if (success)
      {
         _sntprintf(query, 512, _T("GRANT ALL TO %s"), dbLogin);
         success = SQLQuery(query);
      }
   }

   return success;
}

/**
 * Create database and database user
 */
bool CreateDatabase(const char *driver, const TCHAR *dbName, const TCHAR *dbLogin, const TCHAR *dbPassword)
{
   _tprintf(_T("Creating database and user...\n"));

   if (!stricmp(driver, "mssql"))
      return CreateDatabase_MSSQL(dbName, dbLogin, dbPassword);
   if (!stricmp(driver, "mysql") || !stricmp(driver, "mariadb"))
      return CreateDatabase_MySQL(dbName, dbLogin, dbPassword);
   if (!stricmp(driver, "oracle"))
      return CreateDatabase_Oracle(dbLogin, dbPassword);
   if (!stricmp(driver, "pgsql"))
      return CreateDatabase_PostgreSQL(dbName, dbLogin, dbPassword);

   _tprintf(_T("Database creation is not implemented for selected database type"));
   return false;
}
