/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2023 Victor Kirhenshtein
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
** File: upgrade.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>

/**
 * Externals
 */
bool MajorSchemaUpgrade_V0();
bool MajorSchemaUpgrade_V21();
bool MajorSchemaUpgrade_V22();
bool MajorSchemaUpgrade_V30();
bool MajorSchemaUpgrade_V31();
bool MajorSchemaUpgrade_V32();
bool MajorSchemaUpgrade_V33();
bool MajorSchemaUpgrade_V34();
bool MajorSchemaUpgrade_V35();
bool MajorSchemaUpgrade_V36();
bool MajorSchemaUpgrade_V37();
bool MajorSchemaUpgrade_V38();
bool MajorSchemaUpgrade_V39();
bool MajorSchemaUpgrade_V40();
bool MajorSchemaUpgrade_V41();
bool MajorSchemaUpgrade_V42();
bool MajorSchemaUpgrade_V43();
bool MajorSchemaUpgrade_V44();

/**
 * Move to next major version of DB schema
 */
bool SetMajorSchemaVersion(int32_t nextMajor, int32_t nextMinor)
{
   int32_t currMajor, currMinor;
   if (!DBGetSchemaVersion(g_dbHandle, &currMajor, &currMinor))
      return false;

   TCHAR var[64], query[256];
   _sntprintf(var, 64, _T("SchemaVersionLevel.%d"), currMajor);
   if (IsDatabaseRecordExist(g_dbHandle, _T("metadata"), _T("var_name"), var))
      _sntprintf(query, 256, _T("UPDATE metadata SET var_value='%d' WHERE var_name='%s'"), currMinor, var);
   else
      _sntprintf(query, 256, _T("INSERT INTO metadata (var_name,var_value) VALUES ('%s','%d')"), var, currMinor);
   if (!SQLQuery(query))
      return false;

   _sntprintf(query, 256, _T("UPDATE metadata SET var_value='%d' WHERE var_name='SchemaVersionMajor'"), nextMajor);
   if (!SQLQuery(query))
      return false;

   _sntprintf(query, 256, _T("UPDATE metadata SET var_value='%d' WHERE var_name='SchemaVersionMinor'"), nextMinor);
   if (!SQLQuery(query))
      return false;

   return true;
}

/**
 * Move to next minor version of DB schema
 */
bool SetMinorSchemaVersion(int32_t nextMinor)
{
   TCHAR query[256];
   _sntprintf(query, 256, _T("UPDATE metadata SET var_value='%d' WHERE var_name='SchemaVersionMinor'"), nextMinor);
   return SQLQuery(query);
}

/**
 * Get last used schema level for given major version
 */
int32_t GetSchemaLevelForMajorVersion(int32_t major)
{
   TCHAR var[64];
   _sntprintf(var, 64, _T("SchemaVersionLevel.%d"), major);
   int32_t minor = DBMgrMetaDataReadInt32(var, -1);
   if (minor != -1)
      return minor;

   // Missing minor version for requested major could mean two different scenarios:
   // 1. Initial installation was on newer version
   // 2. We didn't upgrade up to that major version yet
   // Attempt to deduce most recent major version by scanning all SchemaVersionLevel.* variables
   DB_RESULT hResult = SQLSelect(_T("SELECT var_name FROM metadata WHERE var_name LIKE 'SchemaVersionLevel.%'"));
   if (hResult == nullptr)
      return -1;  // Unexpected SQL error, assume that changes for requested major version was not made

   int32_t lastMajor = 0;
   int count = DBGetNumRows(hResult);
   for(int i = 0; i < count; i++)
   {
      TCHAR name[256];
      DBGetField(hResult, i, 0, name, 256);
      int32_t version = _tcstol(&name[19], nullptr, 10);
      if (version > lastMajor)
         lastMajor = version;
   }
   DBFreeResult(hResult);

   // Presence of SchemaVersionLevel for major version higher than requested indicates that
   // all changes for requested version was already included at database initialization
   return (lastMajor > major) ? INT_MAX : -1;
}

/**
 * Set last used schema level for given major version
 */
bool SetSchemaLevelForMajorVersion(int32_t major, int32_t level)
{
   TCHAR var[64], query[256];
   _sntprintf(var, 64, _T("SchemaVersionLevel.%d"), major);
   if (IsDatabaseRecordExist(g_dbHandle, _T("metadata"), _T("var_name"), var))
      _sntprintf(query, 256, _T("UPDATE metadata SET var_value='%d' WHERE var_name='%s'"), level, var);
   else
      _sntprintf(query, 256, _T("INSERT INTO metadata (var_name,var_value) VALUES ('%s','%d')"), var, level);
   return SQLQuery(query);
}

/**
 * Upgrade map
 */
static struct
{
   int majorVersion;
   bool (* upgradeProc)();
} s_dbUpgradeMap[] =
{
   { 0, MajorSchemaUpgrade_V0 },
   { 21, MajorSchemaUpgrade_V21 },
   { 22, MajorSchemaUpgrade_V22 },
   { 30, MajorSchemaUpgrade_V30 },
   { 31, MajorSchemaUpgrade_V31 },
   { 32, MajorSchemaUpgrade_V32 },
   { 33, MajorSchemaUpgrade_V33 },
   { 34, MajorSchemaUpgrade_V34 },
   { 35, MajorSchemaUpgrade_V35 },
   { 36, MajorSchemaUpgrade_V36 },
   { 37, MajorSchemaUpgrade_V37 },
   { 38, MajorSchemaUpgrade_V38 },
   { 39, MajorSchemaUpgrade_V39 },
   { 40, MajorSchemaUpgrade_V40 },
   { 41, MajorSchemaUpgrade_V41 },
   { 42, MajorSchemaUpgrade_V42 },
   { 43, MajorSchemaUpgrade_V43 },
   { 44, MajorSchemaUpgrade_V44 },
   { 0, nullptr }
};

/**
 * Upgrade database to new version
 */
void UpgradeDatabase()
{
   _tprintf(_T("Upgrading database...\n"));

   // Get database format version
   int32_t major, minor;
	if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
	{
      _tprintf(_T("Unable to determine database schema version\n"));
	   return;
	}

   if ((major == DB_SCHEMA_VERSION_MAJOR) && (minor == DB_SCHEMA_VERSION_MINOR))
   {
      _tprintf(_T("Core database schema is up to date\n"));
      bool modSuccess = UpgradeModuleSchemas();
      _tprintf(_T("Database upgrade %s\n"), modSuccess ? _T("succeeded") : _T("failed"));
      return;
   }

   if ((major > DB_SCHEMA_VERSION_MAJOR) || ((major == DB_SCHEMA_VERSION_MAJOR) && (minor > DB_SCHEMA_VERSION_MINOR)))
   {
       _tprintf(_T("Your database has core schema version %d.%d, this tool is compiled for version %d.%d.\n")
                   _T("You need to upgrade your server before using this database.\n"),
                major, minor, DB_SCHEMA_VERSION_MAJOR, DB_SCHEMA_VERSION_MINOR);
       return;
   }

   // Check if database is locked
   bool locked = false;
   DB_RESULT hResult = DBSelect(g_dbHandle, _T("SELECT var_value FROM config WHERE var_name='DBLockStatus'"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         TCHAR buffer[MAX_DB_STRING];
         locked = _tcscmp(DBGetField(hResult, 0, 0, buffer, MAX_DB_STRING), _T("UNLOCKED"));
      }
      DBFreeResult(hResult);
   }
   if (locked)
   {
      _tprintf(_T("Database is locked\n"));
      return;
   }

   while(major < DB_SCHEMA_VERSION_MAJOR)
   {
      // Find upgrade procedure
      int i;
      for(i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].majorVersion == major)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         _tprintf(_T("Unable to find upgrade procedure for major version %d\n"), major);
         break;
      }
      if (!s_dbUpgradeMap[i].upgradeProc())
         break;
      if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      {
         _tprintf(_T("Internal error\n"));
         break;
      }
   }

   if ((major == DB_SCHEMA_VERSION_MAJOR) && (minor < DB_SCHEMA_VERSION_MINOR))
   {
      // Find upgrade procedure
      int i;
      for(i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].majorVersion == major)
            break;
      if (s_dbUpgradeMap[i].upgradeProc != nullptr)
      {
         s_dbUpgradeMap[i].upgradeProc();
         if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
            _tprintf(_T("Internal error\n"));
      }
      else
      {
         _tprintf(_T("Unable to find upgrade procedure for major version %d\n"), major);
      }
   }

   bool modSuccess = UpgradeModuleSchemas();
   _tprintf(_T("Database upgrade %s\n"), ((major == DB_SCHEMA_VERSION_MAJOR) && (minor == DB_SCHEMA_VERSION_MINOR) && modSuccess) ? _T("succeeded") : _T("failed"));
}
