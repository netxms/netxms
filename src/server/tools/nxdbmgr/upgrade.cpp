/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2018 Victor Kirhenshtein
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

/**
 * Move to next major version of DB schema
 */
bool SetMajorSchemaVersion(INT32 nextMajor, INT32 nextMinor)
{
   INT32 currMajor, currMinor;
   if (!DBGetSchemaVersion(g_dbHandle, &currMajor, &currMinor))
      return false;

   TCHAR query[256];
   _sntprintf(query, 256, _T("INSERT INTO metadata (var_name,var_value) VALUES ('SchemaVersionLevel.%d','%d')"), currMajor, currMinor);
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
bool SetMinorSchemaVersion(INT32 nextMinor)
{
   TCHAR query[256];
   _sntprintf(query, 256, _T("UPDATE metadata SET var_value='%d' WHERE var_name='SchemaVersionMinor'"), nextMinor);
   return SQLQuery(query);
}

/**
 * Get last used schema level for given major version
 */
INT32 GetSchemaLevelForMajorVersion(INT32 major)
{
   TCHAR var[64];
   _sntprintf(var, 64, _T("SchemaVersionLevel.%d"), major);
   return DBMgrMetaDataReadInt32(var, -1);
}

/**
 * Set last used schema level for given major version
 */
bool SetSchemaLevelForMajorVersion(INT32 major, INT32 level)
{
   TCHAR query[256];
   _sntprintf(query, 256, _T("UPDATE metadata SET var_value='%d' WHERE var_name='SchemaVersionLevel.%d'"), level, major);
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
   { 0, NULL }
};

/**
 * Upgrade database to new version
 */
void UpgradeDatabase()
{
   _tprintf(_T("Upgrading database...\n"));

   // Get database format version
   INT32 major, minor;
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
   if (hResult != NULL)
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
      for(i = 0; s_dbUpgradeMap[i].upgradeProc != NULL; i++)
         if (s_dbUpgradeMap[i].majorVersion == major)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == NULL)
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
      for(i = 0; s_dbUpgradeMap[i].upgradeProc != NULL; i++)
         if (s_dbUpgradeMap[i].majorVersion == major)
            break;
      if (s_dbUpgradeMap[i].upgradeProc != NULL)
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
