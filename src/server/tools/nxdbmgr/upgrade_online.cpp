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
** File: upgrade_online.cpp
**
**/

#include "nxdbmgr.h"

/**
 * Check if there are pending online upgrades
 */
bool IsOnlineUpgradePending()
{
   TCHAR buffer[MAX_DB_STRING];
   MetaDataReadStr(_T("PendingOnlineUpgrades"), buffer, MAX_DB_STRING, _T(""));
   Trim(buffer);
   return buffer[0] != 0;
}

/**
 * Register online upgrade
 */
void RegisterOnlineUpgrade(int major, int minor)
{
   TCHAR id[16];
   _sntprintf(id, 16, _T("%X"), (major << 16) | minor);

   TCHAR buffer[MAX_DB_STRING];
   MetaDataReadStr(_T("PendingOnlineUpgrades"), buffer, MAX_DB_STRING, _T(""));
   Trim(buffer);

   if (buffer[0] == 0)
   {
      MetaDataWriteStr(_T("PendingOnlineUpgrades"), id);
   }
   else
   {
      _tcslcat(buffer, _T(","), MAX_DB_STRING);
      _tcslcat(buffer, id, MAX_DB_STRING);
      MetaDataWriteStr(_T("PendingOnlineUpgrades"), buffer);
   }
}

/**
 * Unregister online upgrade
 */
void UnregisterOnlineUpgrade(int major, int minor)
{
   TCHAR id[16];
   _sntprintf(id, 16, _T("%X"), (major << 16) | minor);

   TCHAR buffer[MAX_DB_STRING];
   MetaDataReadStr(_T("PendingOnlineUpgrades"), buffer, MAX_DB_STRING, _T(""));
   Trim(buffer);

   bool changed = false;
   StringList *upgradeList = String(buffer).split(_T(","));
   for(int i = 0; i < upgradeList->size(); i++)
   {
      if (!_tcsicmp(upgradeList->get(i), id))
      {
         changed = true;
         upgradeList->remove(i);
         break;
      }
   }

   if (changed)
   {
      TCHAR *list = upgradeList->join(_T(","));
      MetaDataWriteStr(_T("PendingOnlineUpgrades"), (list[0] != 0) ? list : _T(" ")); // Oracle workaround
      free(list);
   }
   delete upgradeList;
}

/**
 * Set zone UIN in log table
 */
static bool SetZoneUIN(const TCHAR *table, const TCHAR *idColumn, const TCHAR *objectColumn)
{
   _tprintf(_T("Updating zone UIN in table %s...\n"), table);

   const TCHAR *queryTemplate;
   switch(g_dbSyntax)
   {
      case DB_SYNTAX_DB2:
         queryTemplate = _T("SELECT %s,%s FROM %s WHERE zone_uin IS NULL FETCH FIRST 1000 ROWS ONLY");
         break;
      case DB_SYNTAX_MSSQL:
         queryTemplate = _T("SELECT TOP 1000 %s,%s FROM %s WHERE zone_uin IS NULL");
         break;
      case DB_SYNTAX_MYSQL:
      case DB_SYNTAX_PGSQL:
      case DB_SYNTAX_SQLITE:
         queryTemplate = _T("SELECT %s,%s FROM %s WHERE zone_uin IS NULL LIMIT 1000");
         break;
      case DB_SYNTAX_ORACLE:
         queryTemplate = _T("SELECT * FROM (SELECT %s,%s FROM %s WHERE zone_uin IS NULL) WHERE ROWNUM<=1000");
         break;
      default:
         _tprintf(_T("Internal error\n"));
         return false;
   }

   TCHAR query[256];
   _sntprintf(query, 256, queryTemplate, idColumn, objectColumn, table);

   TCHAR updateQuery[256];
   _sntprintf(updateQuery, 256, _T("UPDATE %s SET zone_uin=COALESCE((SELECT zone_guid FROM nodes WHERE nodes.id=?),0) WHERE %s=?"), table, idColumn);
   DB_STATEMENT hStmt = DBPrepare(g_hCoreDB, updateQuery);
   if (hStmt == NULL)
      return false;

   while(true)
   {
      DB_RESULT hResult = DBSelect(g_hCoreDB, query);
      if (hResult == NULL)
      {
         DBFreeStatement(hStmt);
         return false;
      }

      int count = DBGetNumRows(hResult);
      if (count == 0)
         break;

      bool success = DBBegin(g_hCoreDB);
      for(int i = 0; (i < count) && success; i++)
      {
         UINT64 id = DBGetFieldUInt64(hResult, i, 0);
         UINT32 objectId = DBGetFieldULong(hResult, i, 1);
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, objectId);
         DBBind(hStmt, 2, DB_SQLTYPE_BIGINT, id);
         success = DBExecute(hStmt);
      }
      if (success)
         DBCommit(g_hCoreDB);
      else
         DBRollback(g_hCoreDB);
      DBFreeResult(hResult);

      if (!success)
      {
         DBFreeStatement(hStmt);
         return false;
      }

      ThreadSleep(1);
   }

   DBFreeStatement(hStmt);
   return true;
}

/**
 * Online upgrade for version 22.21
 */
static bool Upgrade_22_21()
{
   CHK_EXEC_NO_SP(SetZoneUIN(_T("alarms"), _T("alarm_id"), _T("source_object_id")));
   CHK_EXEC_NO_SP(SetZoneUIN(_T("event_log"), _T("event_id"), _T("event_source")));
   CHK_EXEC_NO_SP(SetZoneUIN(_T("snmp_trap_log"), _T("trap_id"), _T("object_id")));
   CHK_EXEC_NO_SP(SetZoneUIN(_T("syslog"), _T("msg_id"), _T("source_object_id")));

   CHK_EXEC_NO_SP(DBSetNotNullConstraint(g_hCoreDB, _T("alarms"), _T("zone_uin")));
   CHK_EXEC_NO_SP(DBSetNotNullConstraint(g_hCoreDB, _T("event_log"), _T("zone_uin")));
   CHK_EXEC_NO_SP(DBSetNotNullConstraint(g_hCoreDB, _T("snmp_trap_log"), _T("zone_uin")));
   CHK_EXEC_NO_SP(DBSetNotNullConstraint(g_hCoreDB, _T("syslog"), _T("zone_uin")));

   return true;
}

/**
 * Online upgrade registry
 */
struct
{
   int major;
   int minor;
   bool (*handler)();
} s_handlers[] =
{
   { 22, 21, Upgrade_22_21 },
   { 0, 0, NULL }
};

/**
 * Run pending online upgrades
 */
void RunPendingOnlineUpgrades()
{
   TCHAR buffer[MAX_DB_STRING];
   MetaDataReadStr(_T("PendingOnlineUpgrades"), buffer, MAX_DB_STRING, _T(""));
   Trim(buffer);
   if (buffer[0] == 0)
   {
      _tprintf(_T("No pending online upgrades\n"));
      return;
   }

   StringList *upgradeList = String(buffer).split(_T(","));
   for(int i = 0; i < upgradeList->size(); i++)
   {
      UINT32 id = _tcstol(upgradeList->get(i), NULL, 16);
      int major = id >> 16;
      int minor = id & 0xFFFF;
      if ((major != 0) && (minor != 0))
      {
         bool (*handler)() = NULL;
         for(int j = 0; s_handlers[j].major != 0; j++)
         {
            if ((s_handlers[j].major == major) && (s_handlers[j].minor == minor))
            {
               handler = s_handlers[j].handler;
               break;
            }
         }
         if (handler != NULL)
         {
            _tprintf(_T("Running online upgrade procedure for version %d.%d\n"), major, minor);
            if (!handler())
            {
               _tprintf(_T("Online upgrade procedure for version %d.%d failed\n"), major, minor);
               break;
            }
            _tprintf(_T("Online upgrade procedure for version %d.%d completed\n"), major, minor);
            UnregisterOnlineUpgrade(major, minor);
         }
         else
         {
            _tprintf(_T("Cannot find online upgrade procedure for version %d.%d\n"), major, minor);
         }
      }
   }
   delete upgradeList;
}
