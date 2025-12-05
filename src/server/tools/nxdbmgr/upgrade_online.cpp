/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2025 Victor Kirhenshtein
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

bool ConvertTDataTables(int stage);

/**
 * Check if there are pending online upgrades
 */
bool IsOnlineUpgradePending()
{
   wchar_t buffer[MAX_DB_STRING];
   DBMgrMetaDataReadStr(_T("PendingOnlineUpgrades"), buffer, MAX_DB_STRING, _T(""));
   Trim(buffer);
   return buffer[0] != 0;
}

/**
 * Register online upgrade
 */
void RegisterOnlineUpgrade(int major, int minor)
{
   wchar_t id[16];
   _sntprintf(id, 16, _T("%X"), (major << 16) | minor);

   wchar_t buffer[MAX_DB_STRING];
   DBMgrMetaDataReadStr(L"PendingOnlineUpgrades", buffer, MAX_DB_STRING, L"");
   Trim(buffer);

   if (buffer[0] == 0)
   {
      DBMgrMetaDataWriteStr(L"PendingOnlineUpgrades", id);
   }
   else
   {
      wcslcat(buffer, L",", MAX_DB_STRING);
      wcslcat(buffer, id, MAX_DB_STRING);
      DBMgrMetaDataWriteStr(L"PendingOnlineUpgrades", buffer);
   }
}

/**
 * Unregister online upgrade
 */
void UnregisterOnlineUpgrade(int major, int minor)
{
   wchar_t id[16];
   _sntprintf(id, 16, _T("%X"), (major << 16) | minor);

   wchar_t buffer[MAX_DB_STRING];
   DBMgrMetaDataReadStr(L"PendingOnlineUpgrades", buffer, MAX_DB_STRING, L"");
   Trim(buffer);

   bool changed = false;
   StringList upgradeList = String(buffer).split(_T(","));
   for(int i = 0; i < upgradeList.size(); i++)
   {
      if (!_tcsicmp(upgradeList.get(i), id))
      {
         changed = true;
         upgradeList.remove(i);
         break;
      }
   }

   if (changed)
   {
      TCHAR *list = upgradeList.join(_T(","));
      DBMgrMetaDataWriteStr(_T("PendingOnlineUpgrades"), (list[0] != 0) ? list : _T(" ")); // Oracle workaround
      MemFree(list);
   }
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
         queryTemplate = _T("SELECT %s,%s FROM %s WHERE zone_uin IS NULL FETCH FIRST 2000 ROWS ONLY");
         break;
      case DB_SYNTAX_MSSQL:
         queryTemplate = _T("SELECT TOP 2000 %s,%s FROM %s WHERE zone_uin IS NULL");
         break;
      case DB_SYNTAX_MYSQL:
      case DB_SYNTAX_PGSQL:
      case DB_SYNTAX_SQLITE:
      case DB_SYNTAX_TSDB:
         queryTemplate = _T("SELECT %s,%s FROM %s WHERE zone_uin IS NULL LIMIT 2000");
         break;
      case DB_SYNTAX_ORACLE:
         queryTemplate = _T("SELECT * FROM (SELECT %s,%s FROM %s WHERE zone_uin IS NULL) WHERE ROWNUM<=2000");
         break;
      default:
         _tprintf(_T("Internal error\n"));
         return false;
   }

   TCHAR query[256];
   _sntprintf(query, 256, queryTemplate, idColumn, objectColumn, table);

   TCHAR updateQuery[256];
   _sntprintf(updateQuery, 256, _T("UPDATE %s SET zone_uin=COALESCE((SELECT zone_guid FROM nodes WHERE nodes.id=?),0) WHERE %s=?"), table, idColumn);
   DB_STATEMENT hStmt = DBPrepare(g_dbHandle, updateQuery);
   if (hStmt == NULL)
      return false;

   while(true)
   {
      DB_RESULT hResult = DBSelect(g_dbHandle, query);
      if (hResult == NULL)
      {
         DBFreeStatement(hStmt);
         return false;
      }

      int count = DBGetNumRows(hResult);
      if (count == 0)
         break;

      bool success = DBBegin(g_dbHandle);
      for(int i = 0; (i < count) && success; i++)
      {
         uint64_t id = DBGetFieldUInt64(hResult, i, 0);
         uint32_t objectId = DBGetFieldUInt32(hResult, i, 1);
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, objectId);
         DBBind(hStmt, 2, DB_SQLTYPE_BIGINT, id);
         success = DBExecute(hStmt);
      }
      if (success)
         DBCommit(g_dbHandle);
      else
         DBRollback(g_dbHandle);
      DBFreeResult(hResult);

      if (!success)
      {
         DBFreeStatement(hStmt);
         return false;
      }

      ThreadSleepMs(1000);
   }

   DBFreeStatement(hStmt);
   return true;
}

/**
 * Online upgrade for version 0.411
 */
static bool Upgrade_0_411()
{
   CHK_EXEC_NO_SP(ConvertTDataTables(2));
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

   CHK_EXEC_NO_SP(DBSetNotNullConstraint(g_dbHandle, _T("alarms"), _T("zone_uin")));
   CHK_EXEC_NO_SP(DBSetNotNullConstraint(g_dbHandle, _T("event_log"), _T("zone_uin")));
   CHK_EXEC_NO_SP(DBSetNotNullConstraint(g_dbHandle, _T("snmp_trap_log"), _T("zone_uin")));
   CHK_EXEC_NO_SP(DBSetNotNullConstraint(g_dbHandle, _T("syslog"), _T("zone_uin")));

   return true;
}

/**
 * Insert into idata table using storage class
 */
static bool InsertUsingStorageClass_IData(const TCHAR *sclass, const StringObjectMap<StringList>& data)
{
   StringList *elements = data.get(sclass);
   if (elements->isEmpty())
      return true;

   StringBuffer query = _T("INSERT INTO idata_sc_");
   query.append(sclass);
   query.append(_T(" (item_id,idata_timestamp,idata_value,raw_value) VALUES"));
   for(int i = 0; i < elements->size(); i++)
   {
      query.append((i == 0) ? _T(" ") : _T(","));
      query.append(elements->get(i));
   }
   query.append(_T(" ON CONFLICT DO NOTHING"));
   return SQLQuery(query);
}

/**
 * Insert into tdata table using storage class
 */
static bool InsertUsingStorageClass_TData(const TCHAR *sclass, const StringObjectMap<StringList>& data)
{
   StringList *elements = data.get(sclass);
   if (elements->isEmpty())
      return true;

   StringBuffer query = _T("INSERT INTO tdata_sc_");
   query.append(sclass);
   query.append(_T(" (item_id,tdata_timestamp,tdata_value) VALUES"));
   for(int i = 0; i < elements->size(); i++)
   {
      query.append((i == 0) ? _T(" ") : _T(","));
      query.append(elements->get(i));
   }
   query.append(_T(" ON CONFLICT DO NOTHING"));
   return SQLQuery(query);
}

/**
 * Online upgrade for version 30.87
 */
static bool Upgrade_30_87()
{
   if (g_dbSyntax != DB_SYNTAX_TSDB)
      return true;   // not needed

   CHK_EXEC_NO_SP(LoadDataCollectionObjects());

   WriteToTerminal(_T("Converting table \x1b[1midata\x1b[0m\n"));

   int totalCount = 0;
   time_t cutoffTime = 0, nextCutoffTime = 0, currTime = 0;
   UINT32 currId = 0;
   while(true)
   {
      TCHAR query[1024];
      _sntprintf(query, 1024,
               _T("SELECT item_id,idata_timestamp,idata_value,raw_value FROM idata_old WHERE (idata_timestamp=%u AND item_id>%u) OR idata_timestamp>%u ORDER BY idata_timestamp,item_id LIMIT 10000"),
               static_cast<UINT32>(currTime), currId, static_cast<UINT32>(currTime));
      DB_UNBUFFERED_RESULT hResult = SQLSelectUnbuffered(query);
      if (hResult == NULL)
      {
         if (g_ignoreErrors)
            continue;
         return false;
      }

      int count = 0;

      StringObjectMap<StringList> data(Ownership::True);
      data.set(_T("default"), new StringList());
      data.set(_T("7"), new StringList());
      data.set(_T("30"), new StringList());
      data.set(_T("90"), new StringList());
      data.set(_T("180"), new StringList());
      data.set(_T("other"), new StringList());

      while(DBFetch(hResult))
      {
         currId = DBGetFieldULong(hResult, 0);
         currTime = DBGetFieldULong(hResult, 1);

         TCHAR query[1024], buffer1[256], buffer2[256];
         _sntprintf(query, 1024, _T("(%u,%u,%s,%s)"),
                  currId, static_cast<UINT32>(currTime),
                  (const TCHAR *)DBPrepareString(g_dbHandle, DBGetField(hResult, 2, buffer1, 256)),
                  (const TCHAR *)DBPrepareString(g_dbHandle, DBGetField(hResult, 3, buffer2, 256)));
         data.get(GetDCObjectStorageClass(currId))->add(query);

         if (currTime != nextCutoffTime)
         {
            cutoffTime = nextCutoffTime;
            nextCutoffTime = currTime;
         }
         count++;
      }
      DBFreeResult(hResult);

      if (count == 0)
         break;   // End of data

      CHK_EXEC_NO_SP(DBBegin(g_dbHandle));

      CHK_EXEC(InsertUsingStorageClass_IData(_T("default"), data));
      CHK_EXEC(InsertUsingStorageClass_IData(_T("7"), data));
      CHK_EXEC(InsertUsingStorageClass_IData(_T("30"), data));
      CHK_EXEC(InsertUsingStorageClass_IData(_T("90"), data));
      CHK_EXEC(InsertUsingStorageClass_IData(_T("180"), data));
      CHK_EXEC(InsertUsingStorageClass_IData(_T("other"), data));

      if (cutoffTime != 0)
      {
         _sntprintf(query, 256, _T("SELECT drop_chunks(%u, 'idata_old')"), (UINT32)cutoffTime);
         CHK_EXEC(SQLQuery(query));
         cutoffTime = 0;
      }

      CHK_EXEC_NO_SP(DBCommit(g_dbHandle));

      totalCount += count;
      WriteToTerminalEx(_T("   %d records processed\n"), totalCount);
      if (count < 10000)
         break;
   }

   CHK_EXEC_NO_SP(SQLQuery(_T("DROP TABLE idata_old")));

   WriteToTerminal(_T("Converting table \x1b[1mtdata\x1b[0m\n"));

   totalCount = 0;
   cutoffTime = 0;
   nextCutoffTime = 0;
   currTime = 0;
   currId = 0;
   while(true)
   {
      TCHAR query[1024];
      _sntprintf(query, 1024,
               _T("SELECT item_id,tdata_timestamp,tdata_value FROM tdata_old WHERE (tdata_timestamp=%u AND item_id>%u) OR tdata_timestamp>%u ORDER BY tdata_timestamp,item_id LIMIT 1000"),
               static_cast<UINT32>(currTime), currId, static_cast<UINT32>(currTime));
      DB_UNBUFFERED_RESULT hResult = SQLSelectUnbuffered(query);
      if (hResult == NULL)
      {
         if (g_ignoreErrors)
            continue;
         return false;
      }

      int count = 0;

      StringObjectMap<StringList> data(Ownership::True);
      data.set(_T("default"), new StringList());
      data.set(_T("7"), new StringList());
      data.set(_T("30"), new StringList());
      data.set(_T("90"), new StringList());
      data.set(_T("180"), new StringList());
      data.set(_T("other"), new StringList());

      while(DBFetch(hResult))
      {
         currId = DBGetFieldULong(hResult, 0);
         currTime = DBGetFieldULong(hResult, 1);

         _sntprintf(query, 1024, _T("(%u,%u,'"), currId, static_cast<UINT32>(currTime));
         StringBuffer dataQuery(query);
         dataQuery.appendPreallocated(DBGetField(hResult, 2, nullptr, 0));
         dataQuery.append(_T("')"));
         data.get(GetDCObjectStorageClass(currId))->add(dataQuery);

         if (currTime != nextCutoffTime)
         {
            cutoffTime = nextCutoffTime;
            nextCutoffTime = currTime;
         }
         count++;
      }
      DBFreeResult(hResult);

      if (count == 0)
         break;   // End of data

      CHK_EXEC_NO_SP(DBBegin(g_dbHandle));

      CHK_EXEC(InsertUsingStorageClass_TData(_T("default"), data));
      CHK_EXEC(InsertUsingStorageClass_TData(_T("7"), data));
      CHK_EXEC(InsertUsingStorageClass_TData(_T("30"), data));
      CHK_EXEC(InsertUsingStorageClass_TData(_T("90"), data));
      CHK_EXEC(InsertUsingStorageClass_TData(_T("180"), data));
      CHK_EXEC(InsertUsingStorageClass_TData(_T("other"), data));

      if (cutoffTime != 0)
      {
         TCHAR query[256];
         _sntprintf(query, 256, _T("SELECT drop_chunks(%u, 'tdata_old')"), (UINT32)cutoffTime);
         CHK_EXEC(SQLQuery(query));
         cutoffTime = 0;
      }

      CHK_EXEC_NO_SP(DBCommit(g_dbHandle));

      totalCount += count;
      WriteToTerminalEx(_T("   %d records processed\n"), totalCount);
      if (count < 10000)
         break;
   }

   CHK_EXEC_NO_SP(SQLQuery(_T("DROP TABLE tdata_old")));

   return true;
}

/**
 * Copy data from old format idata_sc_* or tdata_dc_* table to new format
 */
static bool CopyDataTable_V33_6(const TCHAR *table, bool tableData)
{
   WriteToTerminalEx(_T("Converting table \x1b[1m%s\x1b[0m\n"), table);

   TCHAR query[1024];
   if (tableData)
   {
      _sntprintf(query, 1024, _T("INSERT INTO %s (item_id,tdata_timestamp,tdata_value) SELECT item_id,to_timestamp(tdata_timestamp),tdata_value FROM v33_5_%s"), table, table);
   }
   else
   {
      _sntprintf(query, 1024, _T("INSERT INTO %s (item_id,idata_timestamp,idata_value,raw_value) SELECT item_id,to_timestamp(idata_timestamp),idata_value,raw_value FROM v33_5_%s"), table, table);
   }
   CHK_EXEC_NO_SP(SQLQuery(query));

   _sntprintf(query, 1024, _T("DROP TABLE v33_5_%s CASCADE"), table);
   CHK_EXEC_NO_SP(SQLQuery(query));
   return true;
}

/**
 * Online upgrade for version 33.6
 */
static bool Upgrade_33_6()
{
   if (g_dbSyntax != DB_SYNTAX_TSDB)
      return true;   // not needed

   CHK_EXEC_NO_SP(CopyDataTable_V33_6(_T("idata_sc_default"), false));
   CHK_EXEC_NO_SP(CopyDataTable_V33_6(_T("idata_sc_7"), false));
   CHK_EXEC_NO_SP(CopyDataTable_V33_6(_T("idata_sc_30"), false));
   CHK_EXEC_NO_SP(CopyDataTable_V33_6(_T("idata_sc_90"), false));
   CHK_EXEC_NO_SP(CopyDataTable_V33_6(_T("idata_sc_180"), false));
   CHK_EXEC_NO_SP(CopyDataTable_V33_6(_T("idata_sc_other"), false));
   CHK_EXEC_NO_SP(CopyDataTable_V33_6(_T("tdata_sc_default"), true));
   CHK_EXEC_NO_SP(CopyDataTable_V33_6(_T("tdata_sc_7"), true));
   CHK_EXEC_NO_SP(CopyDataTable_V33_6(_T("tdata_sc_30"), true));
   CHK_EXEC_NO_SP(CopyDataTable_V33_6(_T("tdata_sc_90"), true));
   CHK_EXEC_NO_SP(CopyDataTable_V33_6(_T("tdata_sc_180"), true));
   CHK_EXEC_NO_SP(CopyDataTable_V33_6(_T("tdata_sc_other"), true));

   return true;
}

/**
 * Online upgrade for version 35.2
 */
static bool Upgrade_35_2()
{
   if (g_dbSyntax != DB_SYNTAX_TSDB)
      return true;   // not needed

   WriteToTerminalEx(_T("Converting table \x1b[1mevent_log\x1b[0m\n"));
   CHK_EXEC_NO_SP(SQLQuery(_T("INSERT INTO event_log (event_id,event_code,event_timestamp,origin,origin_timestamp,event_source,zone_uin,dci_id,event_severity,event_message,event_tags,root_event_id,raw_data) SELECT event_id,event_code,to_timestamp(event_timestamp),origin,origin_timestamp,event_source,zone_uin,dci_id,event_severity,event_message,event_tags,root_event_id,raw_data FROM event_log_v35_2")));
   CHK_EXEC_NO_SP(SQLQuery(_T("DROP TABLE event_log_v35_2 CASCADE")));

   WriteToTerminalEx(_T("Converting table \x1b[1msyslog\x1b[0m\n"));
   CHK_EXEC_NO_SP(SQLQuery(_T("INSERT INTO syslog (msg_id,msg_timestamp,facility,severity,source_object_id,zone_uin,hostname,msg_tag,msg_text) SELECT msg_id,to_timestamp(msg_timestamp),facility,severity,source_object_id,zone_uin,hostname,msg_tag,msg_text FROM syslog_v35_2")));
   CHK_EXEC_NO_SP(SQLQuery(_T("DROP TABLE syslog_v35_2 CASCADE")));

   WriteToTerminalEx(_T("Converting table \x1b[1msnmp_trap_log\x1b[0m\n"));
   CHK_EXEC_NO_SP(SQLQuery(_T("INSERT INTO snmp_trap_log (trap_id,trap_timestamp,ip_addr,object_id,zone_uin,trap_oid,trap_varlist) SELECT trap_id,to_timestamp(trap_timestamp),ip_addr,object_id,zone_uin,trap_oid,trap_varlist FROM snmp_trap_log_v35_2")));
   CHK_EXEC_NO_SP(SQLQuery(_T("DROP TABLE snmp_trap_log_v35_2 CASCADE")));

   return true;
}

/**
 * Online upgrade for version 43.9
 */
static bool Upgrade_43_9()
{
   if (g_dbSyntax != DB_SYNTAX_TSDB)
      return true;   // not needed

   WriteToTerminalEx(_T("Converting table \x1b[1mmaintenance_journal\x1b[0m\n"));
   CHK_EXEC_NO_SP(SQLQuery(_T("INSERT INTO maintenance_journal (record_id,object_id,author,last_edited_by,description,creation_time,modification_time) SELECT record_id,object_id,author,last_edited_by,description,to_timestamp(creation_time),to_timestamp(modification_time) FROM maintenance_journal_v43_9")));
   CHK_EXEC_NO_SP(SQLQuery(_T("DROP TABLE maintenance_journal_v43_9 CASCADE")));

   WriteToTerminalEx(_T("Converting table \x1b[1mcertificate_action_log\x1b[0m\n"));
   CHK_EXEC_NO_SP(SQLQuery(_T("INSERT INTO certificate_action_log (record_id,operation_timestamp,operation,user_id,node_id,node_guid,cert_type,subject,serial) SELECT record_id,to_timestamp(timestamp),operation,user_id,node_id,node_guid,cert_type,subject,serial FROM certificate_action_log_v43_9")));
   CHK_EXEC_NO_SP(SQLQuery(_T("DROP TABLE certificate_action_log_v43_9 CASCADE")));

   return true;
}

/**
 * Delete duplicate records from data table
 */
void DeleteDuplicateDataRecords(const wchar_t *tableType, uint32_t objectId)
{
   wchar_t query[1024];
   nx_swprintf(query, 1024,
      L"DELETE FROM %s_%u a "
      L"USING ("
      L"    SELECT item_id, %s_timestamp, MIN(ctid) as min_ctid"
      L"    FROM %s_%u"
      L"    GROUP BY item_id, %s_timestamp"
      L"    HAVING COUNT(*) > 1"
      L") b "
      L"WHERE a.item_id = b.item_id"
      L"  AND a.%s_timestamp = b.%s_timestamp"
      L"  AND a.ctid > b.min_ctid", tableType, objectId, tableType, tableType, objectId, tableType, tableType, tableType);
   SQLQuery(query);
}

/**
 * Add primary key for data tables for objects selected by given query
 */
static bool AddPKForDataTables(const wchar_t *query)
{
   bool success = false;
   DB_RESULT hResult = SQLSelect(query);
   if (hResult != nullptr)
   {
      success = true;
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         uint32_t id = DBGetFieldULong(hResult, i, 0);

         if (IsDataTableExist(L"idata_%u", id))
         {
            DeleteDuplicateDataRecords(L"idata", id);

            wchar_t table[64], index[64];
            _sntprintf(table, 64, L"idata_%u", id);
            _sntprintf(index, 64, L"idx_idata_%u_id_timestamp", id);
            DBDropIndex(g_dbHandle, table, index);
            DBAddPrimaryKey(g_dbHandle,table, _T("item_id,idata_timestamp"));
         }

         if (IsDataTableExist(L"tdata_%u", id))
         {
            DeleteDuplicateDataRecords(L"tdata", id);

            wchar_t table[64], index[64];
            _sntprintf(table, 64, L"tdata_%u", id);
            _sntprintf(index, 64, L"idx_tdata_%u", id);
            DBDropIndex(g_dbHandle, table, index);
            DBAddPrimaryKey(g_dbHandle,table, L"item_id,tdata_timestamp");
         }
      }
      DBFreeResult(hResult);
   }
   return success;
}

/**
 * Add primary key to all data tables for given object class
 */
static bool AddPKForClassDataTables(const wchar_t *className)
{
   wchar_t query[1024];
   _sntprintf(query, 256, L"SELECT id FROM %s", className);
   return AddPKForDataTables(query);
}

/**
 * Online upgrade for version 52.21
 */
static bool Upgrade_52_21()
{
   CHK_EXEC_NO_SP(AddPKForClassDataTables(L"nodes"));
   CHK_EXEC_NO_SP(AddPKForClassDataTables(L"clusters"));
   CHK_EXEC_NO_SP(AddPKForClassDataTables(L"mobile_devices"));
   CHK_EXEC_NO_SP(AddPKForClassDataTables(L"access_points"));
   CHK_EXEC_NO_SP(AddPKForClassDataTables(L"chassis"));
   CHK_EXEC_NO_SP(AddPKForClassDataTables(L"sensors"));
   CHK_EXEC_NO_SP(AddPKForDataTables(L"SELECT id FROM object_containers WHERE object_class=29 OR object_class=30"));
   return true;
}

/**
 * Remove primary key from data tables for objects selected by given query
 */
static bool RemovePKFromDataTables(const wchar_t *query)
{
   bool success = false;
   DB_RESULT hResult = SQLSelect(query);
   if (hResult != nullptr)
   {
      success = true;
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         uint32_t id = DBGetFieldULong(hResult, i, 0);

         if (IsDataTableExist(L"idata_%u", id))
         {
            wchar_t table[64], query[256];
            _sntprintf(table, 64, L"idata_%u", id);
            DBDropPrimaryKey(g_dbHandle, table);

            _sntprintf(query, 256, L"CREATE INDEX idx_idata_%u_id_timestamp ON idata_%u(item_id,idata_timestamp DESC)", id, id);
            SQLQuery(query);
         }

         if (IsDataTableExist(L"tdata_%u", id))
         {
            wchar_t table[64], query[256];
            _sntprintf(table, 64, L"tdata_%u", id);
            DBDropPrimaryKey(g_dbHandle, table);

            _sntprintf(query, 256, L"CREATE INDEX idx_tdata_%u ON tdata_%u(item_id,tdata_timestamp)", id, id);
            SQLQuery(query);
         }
      }
      DBFreeResult(hResult);
   }
   return success;
}

/**
 * Remove primary key from all data tables for given object class
 */
static bool RemovePKFromClassDataTables(const wchar_t *className)
{
   wchar_t query[1024];
   _sntprintf(query, 256, L"SELECT id FROM %s", className);
   return RemovePKFromDataTables(query);
}

/**
 * Online upgrade for version 52.24
 */
static bool Upgrade_52_24()
{
   CHK_EXEC_NO_SP(RemovePKFromClassDataTables(L"nodes"));
   CHK_EXEC_NO_SP(RemovePKFromClassDataTables(L"clusters"));
   CHK_EXEC_NO_SP(RemovePKFromClassDataTables(L"mobile_devices"));
   CHK_EXEC_NO_SP(RemovePKFromClassDataTables(L"access_points"));
   CHK_EXEC_NO_SP(RemovePKFromClassDataTables(L"chassis"));
   CHK_EXEC_NO_SP(RemovePKFromClassDataTables(L"sensors"));
   CHK_EXEC_NO_SP(RemovePKFromDataTables(L"SELECT id FROM object_containers WHERE object_class=29 OR object_class=30"));
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
   { 52, 24, Upgrade_52_24 },
   { 52, 21, Upgrade_52_21 },
   { 43, 9,  Upgrade_43_9  },
   { 35, 2,  Upgrade_35_2  },
   { 33, 6,  Upgrade_33_6  },
   { 30, 87, Upgrade_30_87 },
   { 22, 21, Upgrade_22_21 },
   { 0, 411, Upgrade_0_411 },
   { 0, 0, nullptr }
};

/**
 * Run pending online upgrades
 */
void RunPendingOnlineUpgrades()
{
   wchar_t buffer[MAX_DB_STRING];
   DBMgrMetaDataReadStr(_T("PendingOnlineUpgrades"), buffer, MAX_DB_STRING, _T(""));
   Trim(buffer);
   if (buffer[0] == 0)
   {
      _tprintf(_T("No pending background upgrades\n"));
      return;
   }

   // Check if database is locked
   bool locked = false;
   wchar_t lockStatus[MAX_DB_STRING];
   if (DBMgrMetaDataReadStr(L"DBBackgroundUpgradeLock", lockStatus, MAX_DB_STRING, L""))
      locked = wcscmp(lockStatus, L"LOCKED") == 0;

   if (locked && !GetYesNo(_T("Background upgrade is locked by another process\nAre you sure you want to start background upgrade?")))
      return;

   StringList upgradeList = String(buffer).split(_T(","));
   for(int i = 0; i < upgradeList.size(); i++)
   {
      uint32_t id = _tcstol(upgradeList.get(i), nullptr, 16);
      int major = id >> 16;
      int minor = id & 0xFFFF;
      if ((major != 0) || (minor != 0))
      {
         bool (*handler)() = nullptr;
         for(int j = 0; (s_handlers[j].major != 0) || (s_handlers[j].minor != 0); j++)
         {
            if ((s_handlers[j].major == major) && (s_handlers[j].minor == minor))
            {
               handler = s_handlers[j].handler;
               break;
            }
         }
         if (handler != nullptr)
         {
            DBMgrMetaDataWriteStr(_T("DBBackgroundUpgradeLock"), _T("LOCKED"));

            _tprintf(_T("Running background upgrade procedure for version %d.%d\n"), major, minor);
            if (!handler())
            {
               _tprintf(_T("Background upgrade procedure for version %d.%d failed\n"), major, minor);

               DBMgrMetaDataWriteStr(_T("DBOnlineUpgradeLockStatus"), _T("UNLOCKED"));
               break;
            }
            _tprintf(_T("Background upgrade procedure for version %d.%d completed\n"), major, minor);
            UnregisterOnlineUpgrade(major, minor);

            DBMgrMetaDataWriteStr(_T("DBOnlineUpgradeLockStatus"), _T("UNLOCKED"));
         }
         else
         {
            _tprintf(_T("Cannot find background upgrade procedure for version %d.%d\n"), major, minor);
         }
      }
   }
}
