/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2015 Victor Kirhenshtein
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
** File: localdb.cpp
**
**/

#include "nxagentd.h"

/**
 * Database schema version
 */
#define DB_SCHEMA_VERSION     1

/**
 * Database driver
 */
static DB_DRIVER s_driver = NULL;

/**
 * Database handle
 */
static DB_HANDLE s_db = NULL;

/**
 * Read metadata
 */
TCHAR *ReadMetadata(const TCHAR *attr, TCHAR *buffer)
{
   TCHAR query[256], *value = NULL;
   _sntprintf(query, 256, _T("SELECT value FROM metadata WHERE attribute='%s'"), attr);

   DB_RESULT hResult = DBSelect(s_db, query);
   if (hResult != NULL)
   {
      value = DBGetField(hResult, 0, 0, buffer, MAX_DB_STRING);
      DBFreeResult(hResult);
   }
   return value;
}

/**
 * Read integer from metadata
 */
INT32 ReadMetadataAsInt(const TCHAR *attr)
{
   TCHAR buffer[MAX_DB_STRING];
   if (ReadMetadata(attr, buffer) == NULL)
      return 0;
   return _tcstol(buffer, NULL, 0);
}

/**
 * Set value in metadata
 */
bool WriteMetadata(const TCHAR *name, const TCHAR *value)
{
   String prepValue = DBPrepareString(s_db, value, 255);

   TCHAR query[1024];
   _sntprintf(query, 1024, _T("UPDATE OR IGNORE metadata SET value=%s WHERE attribute='%s'"), (const TCHAR *)prepValue, name);
   if (!DBQuery(s_db, query))
      return false;

   _sntprintf(query, 1024, _T("INSERT OR IGNORE INTO metadata (attribute,value) VALUES ('%s',%s)"), name, (const TCHAR *)prepValue);
   return DBQuery(s_db, query);
}

/**
 * Set value in metadata from int
 */
bool WriteMetadata(const TCHAR *name, INT32 value)
{
   TCHAR buffer[64];
   _sntprintf(buffer, 64, _T("%d"), value);
   return WriteMetadata(name, buffer);
}

/**
 * Check database structure
 */
static void CheckDatabaseStructure()
{
   int version;
   if (!DBIsTableExist(s_db, _T("metadata")))
   {
      DBQuery(s_db, _T("CREATE TABLE metadata (attribute varchar(63), value varchar(255), PRIMARY KEY(attribute))"));

      TCHAR query[256];
      _sntprintf(query, 256, _T("INSERT INTO metadata (attribute, value) VALUES ('SchemaVersion', '%d')"), DB_SCHEMA_VERSION);
      DBQuery(s_db, query);

      version = DB_SCHEMA_VERSION;
   }
   else
   {
      version = ReadMetadataAsInt(_T("SchemaVersion"));
   }

   if ((version <= 0) || (version > DB_SCHEMA_VERSION))
   {
      nxlog_write(MSG_LOCAL_DB_CORRUPTED, NXLOG_ERROR, NULL);
      return;
   }
}

/**
 * Open local agent database
 */
bool OpenLocalDatabase()
{
   s_driver = DBLoadDriver(_T("sqlite.ddr"), _T(""), false, NULL, NULL);
   if (s_driver == NULL)
   {
      return false;
   }

   TCHAR dbFile[MAX_PATH];
	nx_strncpy(dbFile, g_szDataDirectory, MAX_PATH - 12);
	if (dbFile[_tcslen(dbFile) - 1] != FS_PATH_SEPARATOR_CHAR)
		_tcscat(dbFile, FS_PATH_SEPARATOR);
	_tcscat(dbFile, _T("nxagentd.db"));

   TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
   s_db = DBConnect(s_driver, NULL, dbFile, NULL, NULL, NULL, errorText);
   if (s_db == NULL)
   {
      DebugPrintf(INVALID_INDEX, 1, _T("Local database open error: %s"), errorText);
      return false;
   }

   CheckDatabaseStructure();
   return true;
}

/**
 * Close local database
 */
void CloseLocalDatabase()
{
   DBDisconnect(s_db);
   DBUnloadDriver(s_driver);
}

/**
 * Get database handle
 */
DB_HANDLE GetLocalDatabaseHandle()
{
   return s_db;
}
