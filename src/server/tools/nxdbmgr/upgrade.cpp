/* 
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004 Victor Kirhenshtein
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
** $module: upgrade.cpp
**
**/

#include "nxdbmgr.h"


//
// Create table
//

static BOOL CreateTable(TCHAR *pszQuery)
{
   TCHAR *pszBuffer;
   BOOL bResult;

   pszBuffer = (TCHAR *)malloc(_tcslen(pszQuery) * sizeof(TCHAR) + 256);
   _tcscpy(pszBuffer, pszQuery);
   TranslateStr(pszBuffer, _T("$SQL:TEXT"), g_pszSqlType[g_iSyntax][SQL_TYPE_TEXT]);
   TranslateStr(pszBuffer, _T("$SQL:INT64"), g_pszSqlType[g_iSyntax][SQL_TYPE_INT64]);
   if (g_iSyntax == DB_SYNTAX_MYSQL)
      _tcscat(pszBuffer, g_pszTableSuffix);
   bResult = SQLQuery(pszBuffer);
   free(pszBuffer);
   return bResult;
}


//
// Create configuration parameter if it doesn't exist
//

static BOOL CreateConfigParam(TCHAR *pszName, TCHAR *pszValue, int iVisible, int iNeedRestart)
{
   TCHAR szQuery[1024], *pszEscValue;
   DB_RESULT hResult;
   BOOL bVarExist = FALSE, bResult = TRUE;

   // Check for variable existence
   _stprintf(szQuery, _T("SELECT var_value FROM config WHERE var_name='%s'"), pszName);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != 0)
   {
      if (DBGetNumRows(hResult) > 0)
         bVarExist = TRUE;
      DBFreeResult(hResult);
   }

   if (!bVarExist)
   {
      pszEscValue = EncodeSQLString(pszValue);
      _stprintf(szQuery, _T("INSERT INTO config (var_name,var_value,is_visible,"
                            "need_server_restart) VALUES ('%s','%s',%d,%d)"), 
                pszName, pszEscValue, iVisible, iNeedRestart);
      free(pszEscValue);
      bResult = SQLQuery(szQuery);
   }
   return bResult;
}


//
// Upgrade from V53 to V54
//

static BOOL H_UpgradeFromV53(void)
{
   static TCHAR m_szBatch[] =
      _T("CREATE INDEX idx_address_lists_list_type ON address_lists(list_type)\n")
      _T("DELETE FROM config WHERE var_name='EnableAccessControl'\n")
      _T("DELETE FROM config WHERE var_name='EnableEventAccessControl'\n")
      _T("<END>");

   if (!CreateTable(_T("CREATE TABLE address_lists (")
                    _T("list_type integer not null,")
	                 _T("addr_type integer not null,")
	                 _T("addr1 varchar(15) not null,")
	                 _T("addr2 varchar(15) not null)")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("ActiveNetworkDiscovery"), _T("0"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("DiscoveryFilterFlags"), _T("0"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='54' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V52 to V53
//

static BOOL H_UpgradeFromV52(void)
{
   DB_RESULT hResult;
   int i, nCount;
   DWORD dwId;
   TCHAR szQuery[1024];
   static TCHAR *pszNewIdx[] =
   {
      _T("CREATE INDEX idx_idata_%d_id_timestamp ON idata_%d(item_id,idata_timestamp)"),  // MySQL
      _T("CREATE INDEX idx_idata_%d_timestamp_id ON idata_%d(idata_timestamp,item_id)"),  // POstgreSQL
      _T("CREATE CLUSTERED INDEX idx_idata_%d_id_timestamp ON idata_%d(item_id,idata_timestamp)"), // MS SQL
      _T("CREATE INDEX idx_idata_%d_timestamp_id ON idata_%d(idata_timestamp,item_id)"),  // Oracle
      _T("CREATE INDEX idx_idata_%d_timestamp_id ON idata_%d(idata_timestamp,item_id)")   // SQLite
   };

   hResult = SQLSelect(_T("SELECT id FROM nodes"));
   if (hResult == NULL)
      return FALSE;

   _tprintf(_T("Reindexing database:\n"));
   nCount = DBGetNumRows(hResult);
   for(i = 0; i < nCount; i++)
   {
      dwId = DBGetFieldULong(hResult, i, 0);
      _tprintf("   * idata_%d\n", dwId);

      // Drop old indexes
      _stprintf(szQuery, _T("DROP INDEX idx_idata_%d_timestamp"), dwId);
      DBQuery(g_hCoreDB, szQuery);

      // Create new index
      _stprintf(szQuery, pszNewIdx[g_iSyntax], dwId, dwId);
      SQLQuery(szQuery);
   }

   DBFreeResult(hResult);

   // Update index creation command
   DBQuery(g_hCoreDB, _T("DELETE FROM config WHERE var_name='IDataIndexCreationCommand_1'"));
   if (!CreateConfigParam(_T("IDataIndexCreationCommand_1"), pszNewIdx[g_iSyntax], 0, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='53' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V51 to V52
//

static BOOL H_UpgradeFromV51(void)
{
   static TCHAR m_szBatch[] =
	   "UPDATE object_tools SET tool_data='Configured ICMP targets#7FICMP.TargetList#7F^(.*) (.*) (.*) (.*) (.*)' WHERE tool_id=12\n"
	   "UPDATE object_tools_table_columns SET col_number=4 WHERE col_number=3 AND tool_id=12\n"
	   "UPDATE object_tools_table_columns SET col_number=3 WHERE col_number=2 AND tool_id=12\n"
   	"UPDATE object_tools_table_columns SET col_substr=5 WHERE col_number=1 AND tool_id=12\n"
	   "INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) "
		   "VALUES (12,2,'Packet size','',0,4)\n"
      "INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,matching_oid,description,confirmation_text) "
	      "VALUES (16,'&Info->Active &user sessions',3,"
            "'Active User Sessions#7FSystem.ActiveUserSessions#7F^\"(.*)\" \"(.*)\" \"(.*)\"',"
            "2,'','Show list of active user sessions','#00')\n"
      "INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) "
	      "VALUES (16,0,'User','',0,1)\n"
      "INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) "
	      "VALUES (16,1,'Terminal','',0,2)\n"
      "INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) "
	      "VALUES (16,2,'From','',0,3)\n"
      "INSERT INTO object_tools_acl (tool_id,user_id) VALUES (16,-2147483648)\n"
      "<END>";

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("MailEncoding"), _T("iso-8859-1"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='52' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V50 to V51
//

static BOOL H_UpgradeFromV50(void)
{
   static TCHAR m_szBatch[] =
      "ALTER TABLE event_groups ADD range_start integer\n"
      "ALTER TABLE event_groups ADD range_END integer\n"
      "UPDATE event_groups SET range_start=0,range_end=0\n"
      "<END>";

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='51' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V49 to V50
//

static BOOL H_UpgradeFromV49(void)
{
   static TCHAR m_szBatch[] =
      "ALTER TABLE object_tools ADD confirmation_text varchar(255)\n"
      "UPDATE object_tools SET confirmation_text='#00'\n"
      "UPDATE object_tools SET flags=10 WHERE tool_id=1 OR tool_id=2 OR tool_id=4\n"
      "UPDATE object_tools SET confirmation_text='Host #25OBJECT_NAME#25 (#25OBJECT_IP_ADDR#25) will be shut down. Are you sure?' WHERE tool_id=1\n"
      "UPDATE object_tools SET confirmation_text='Host #25OBJECT_NAME#25 (#25OBJECT_IP_ADDR#25) will be restarted. Are you sure?' WHERE tool_id=2\n"
      "UPDATE object_tools SET confirmation_text='NetXMS agent on host #25OBJECT_NAME#25 (#25OBJECT_IP_ADDR#25) will be restarted. Are you sure?' WHERE tool_id=4\n"
      "<END>";

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='50' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V48 to V49
//

static BOOL H_UpgradeFromV48(void)
{
   static TCHAR m_szBatch[] =
      "ALTER TABLE items ADD all_thresholds integer\n"
      "UPDATE items SET all_thresholds=0\n"
      "ALTER TABLE thresholds ADD rearm_event_code integer\n"
      "UPDATE thresholds SET rearm_event_code=18\n"
      "<END>";

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='49' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V47 to V48
//

static BOOL H_UpgradeFromV47(void)
{
   static TCHAR m_szBatch[] =
      "ALTER TABLE event_policy ADD script $SQL:TEXT\n"
      "UPDATE event_policy SET script='#00'\n"
      "<END>";

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE policy_time_range_list (")
		              _T("rule_id integer not null,")
		              _T("time_range_id integer not null,")
		              _T("PRIMARY KEY(rule_id,time_range_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE time_ranges (")
		              _T("time_range_id integer not null,")
		              _T("wday_mask integer not null,")
		              _T("mday_mask integer not null,")
		              _T("month_mask integer not null,")
		              _T("time_range varchar(255) not null,")
		              _T("PRIMARY KEY(time_range_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='48' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V46 to V47
//

static BOOL H_UpgradeFromV46(void)
{
   static TCHAR m_szBatch[] =
      "ALTER TABLE object_properties ADD comments $SQL:TEXT\n"
      "UPDATE object_properties SET comments='#00'\n"
      "ALTER TABLE nodes DROP COLUMN discovery_flags\n"
      "DROP TABLE alarm_notes\n"
      "ALTER TABLE alarms ADD alarm_state integer\n"
      "ALTER TABLE alarms ADD hd_state integer\n"
      "ALTER TABLE alarms ADD hd_ref varchar(63)\n"
      "ALTER TABLE alarms ADD creation_time integer\n"
      "ALTER TABLE alarms ADD last_change_time integer\n"
      "ALTER TABLE alarms ADD original_severity integer\n"
      "ALTER TABLE alarms ADD current_severity integer\n"
      "ALTER TABLE alarms ADD repeat_count integer\n"
      "ALTER TABLE alarms ADD term_by integer\n"
      "UPDATE alarms SET hd_state=0,hd_ref='#00',creation_time=alarm_timestamp,"
         "last_change_time=alarm_timestamp,original_severity=severity,"
         "current_severity=severity,repeat_count=1,term_by=ack_by\n"
      "UPDATE alarms SET alarm_state=0 WHERE is_ack=0\n"
      "UPDATE alarms SET alarm_state=2 WHERE is_ack<>0\n"
      "ALTER TABLE alarms DROP COLUMN severity\n"
      "ALTER TABLE alarms DROP COLUMN alarm_timestamp\n"
      "ALTER TABLE alarms DROP COLUMN is_ack\n"
      "ALTER TABLE thresholds ADD current_state integer\n"
      "UPDATE thresholds SET current_state=0\n"
      "<END>";
   static TCHAR m_szBatch2[] =
      "CREATE INDEX idx_alarm_notes_alarm_id ON alarm_notes(alarm_id)\n"
      "CREATE INDEX idx_alarm_change_log_alarm_id ON alarm_change_log(alarm_id)\n"
      "<END>";

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE alarm_notes ("
		                 "note_id integer not null,"
		                 "alarm_id integer not null,"
		                 "change_time integer not null,"
		                 "user_id integer not null,"
		                 "note_text $SQL:TEXT not null,"
		                 "PRIMARY KEY(note_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE alarm_change_log	("
                  	  "change_id $SQL:INT64 not null,"
		                 "change_time integer not null,"
		                 "alarm_id integer not null,"
		                 "opcode integer not null,"
		                 "user_id integer not null,"
		                 "info_text $SQL:TEXT not null,"
		                 "PRIMARY KEY(change_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE alarm_grops ("
		                 "alarm_group_id integer not null,"
		                 "group_name varchar(255) not null,"
		                 "PRIMARY KEY(alarm_group_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE alarm_group_map ("
		                 "alarm_group_id integer not null,"
                       "alarm_id integer not null,"
		                 "PRIMARY KEY(alarm_group_id,alarm_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLBatch(m_szBatch2))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='47' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V45 to V46
//

static BOOL H_UpgradeFromV45(void)
{
   static TCHAR m_szBatch[] =
      "UPDATE object_tools_table_columns SET col_format=5 WHERE tool_id=5 AND col_number=1\n"
	   "INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) "
	      "VALUES (2,'.1.3.6.1.4.1.45.3.26.*',3,0)\n"
	   "INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) "
	      "VALUES (3,'.1.3.6.1.4.1.45.3.30.*',3,0)\n"
	   "INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) "
	      "VALUES (4,'.1.3.6.1.4.1.45.3.31.*',3,0)\n"
	   "INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) "
	      "VALUES (5,'.1.3.6.1.4.1.45.3.32.*',3,0)\n"
	   "INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) "
	      "VALUES (6,'.1.3.6.1.4.1.45.3.33.*',3,0)\n"
	   "INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) "
	      "VALUES (7,'.1.3.6.1.4.1.45.3.34.*',3,0)\n"
	   "INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) "
	      "VALUES (8,'.1.3.6.1.4.1.45.3.35.*',3,0)\n"
	   "INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) "
	      "VALUES (9,'.1.3.6.1.4.1.45.3.36.*',3,0)\n"
	   "INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) "
	      "VALUES (10,'.1.3.6.1.4.1.45.3.40.*',3,0)\n"
	   "INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) "
	      "VALUES (11,'.1.3.6.1.4.1.45.3.61.*',3,0)\n"
	   "INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,matching_oid,description) "
		   "VALUES (14,'&Info->Topology table (Nortel)',2 ,'Topology table',1,'','Show topology table (Nortel protocol)')\n"
	   "INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) "
		   "VALUES (14,0,'Peer IP','.1.3.6.1.4.1.45.1.6.13.2.1.1.3',3 ,0)\n"
	   "INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) "
		   "VALUES (14,1,'Peer MAC','.1.3.6.1.4.1.45.1.6.13.2.1.1.5',4 ,0)\n"
	   "INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) "
		   "VALUES (14,2,'Port','.1.3.6.1.4.1.45.1.6.13.2.1.1.2',5 ,0)\n"
	   "INSERT INTO object_tools_acl (tool_id,user_id) VALUES (14,-2147483648)\n"
	   "INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,matching_oid,description) "
		   "VALUES (15,'&Info->Topology table (CDP)',2 ,'Topology table',1,'','Show topology table (CDP)')\n"
	   "INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) "
		   "VALUES (15,0,'Device ID','.1.3.6.1.4.1.9.9.23.1.2.1.1.6',0 ,0)\n"
	   "INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) "
		   "VALUES (15,1,'IP Address','.1.3.6.1.4.1.9.9.23.1.2.1.1.4',3 ,0)\n"
	   "INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) "
		   "VALUES (15,2,'Platform','.1.3.6.1.4.1.9.9.23.1.2.1.1.8',0 ,0)\n"
	   "INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) "
		   "VALUES (15,3,'Version','.1.3.6.1.4.1.9.9.23.1.2.1.1.5',0 ,0)\n"
	   "INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) "
		   "VALUES (15,4,'Port','.1.3.6.1.4.1.9.9.23.1.2.1.1.7',0 ,0)\n"
	   "INSERT INTO object_tools_acl (tool_id,user_id) VALUES (15,-2147483648)\n"
      "<END>";

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='46' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V44 to V45
//

static BOOL H_UpgradeFromV44(void)
{
   static TCHAR m_szBatch[] =
	   "INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) "
         "VALUES (36,'SYS_DB_CONN_LOST',4,1,"
			"'Lost connection with backend database engine',"
			"'Generated if connection with backend database engine is lost.#0D#0A"
			"Parameters:#0D#0A   No message-specific parameters')\n"
	   "INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) "
         "VALUES (37,'SYS_DB_CONN_RESTORED',0,1,"
			"'Connection with backend database engine restored',"
			"'Generated when connection with backend database engine restored.#0D#0A"
			"Parameters:#0D#0A   No message-specific parameters')\n"
      "<END>";

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='45' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V43 to V44
//

static BOOL H_UpgradeFromV43(void)
{
   static TCHAR m_szBatch[] =
      "DELETE FROM object_tools WHERE tool_id=8000\n"
      "DELETE FROM object_tools_table_columns WHERE tool_id=8000\n"
      "DELETE FROM object_tools_acl WHERE tool_id=8000\n"
      "INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,"
         "matching_oid,description) VALUES (13,'&Info->&Process list',3,"
         "'Process List#7FSystem.ProcessList#7F^([0-9]+) (.*)',2,'',"
         "'Show list of currently running processes')\n"
	   "INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,"
         "col_oid,col_format,col_substr) VALUES (13,0,'PID','',0,1)\n"
	   "INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,"
         "col_oid,col_format,col_substr) VALUES (13,1,'Name','',0,2)\n"
	   "INSERT INTO object_tools_acl (tool_id,user_id) VALUES (13,-2147483648)\n"
      "<END>";

   if (!CreateTable(_T("CREATE TABLE agent_configs ("
		                 "config_id integer not null,"
		                 "config_name varchar(255) not null,"
		                 "config_file $SQL:TEXT not null,"
		                 "config_filter $SQL:TEXT not null,"
		                 "sequence_number integer not null,"
		                 "PRIMARY KEY(config_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("DBLockPID"), _T("0"), 0, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='44' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V42 to V43
//

static BOOL H_UpgradeFromV42(void)
{
   if (!CreateConfigParam(_T("RADIUSPort"), _T("1645"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='43' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V41 to V42
//

static BOOL H_UpgradeFromV41(void)
{
   static TCHAR m_szBatch[] =
	   "INSERT INTO images (image_id,name,file_name_png,file_hash_png,"
         "file_name_ico,file_hash_ico) VALUES (15,'Obj.Condition',"
         "'condition.png','<invalid_hash>','condition.ico','<invalid_hash>')\n"
	   "INSERT INTO default_images (object_class,image_id) VALUES (13, 15)\n"
	   "INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) "
	      "VALUES (1,'.1.3.6.1.4.1.3224.1.*',2,0)\n"
	   "INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) "
         "VALUES (34,'SYS_CONDITION_ACTIVATED',2,1,'Condition \"%2\" activated',"
			"'Default event for condition activation.#0D#0AParameters:#0D#0A"
			"   1) Condition object ID#0D#0A   2) Condition object name#0D#0A"
			"   3) Previous condition status#0D#0A   4) Current condition status')\n"
	   "INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) "
         "VALUES (35,'SYS_CONDITION_DEACTIVATED',0,1,'Condition \"%2\" deactivated',"
			"'Default event for condition deactivation.#0D#0AParameters:#0D#0A"
			"   1) Condition object ID#0D#0A   2) Condition object name#0D#0A"
			"   3) Previous condition status#0D#0A   4) Current condition status')\n"
      "<END>";

   if (!CreateTable(_T("CREATE TABLE conditions	("
		                 "id integer not null,"
		                 "activation_event integer not null,"
		                 "deactivation_event integer not null,"
		                 "source_object integer not null,"
		                 "active_status integer not null,"
		                 "inactive_status integer not null,"
		                 "script $SQL:TEXT not null,"
		                 "PRIMARY KEY(id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE cond_dci_map ("
		                 "condition_id integer not null,"
		                 "dci_id integer not null,"
		                 "node_id integer not null,"
		                 "dci_func integer not null,"
		                 "num_polls integer not null,"
		                 "PRIMARY KEY(condition_id,dci_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("NumberOfConditionPollers"), _T("10"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("ConditionPollingInterval"), _T("60"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='42' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V40 to V41
//

static BOOL H_UpgradeFromV40(void)
{
   static TCHAR m_szBatch[] =
      "ALTER TABLE users ADD guid varchar(36)\n"
      "ALTER TABLE users ADD auth_method integer\n"
      "ALTER TABLE user_groups ADD guid varchar(36)\n"
      "UPDATE users SET auth_method=0\n"
      "<END>";
   DB_RESULT hResult;
   int i, nCount;
   DWORD dwId;
   uuid_t guid;
   TCHAR szQuery[256], szGUID[64];

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   // Generate GUIDs for users and groups
   printf("Generating GUIDs...\n");
   
   hResult = SQLSelect(_T("SELECT id FROM users"));
   if (hResult != NULL)
   {
      nCount = DBGetNumRows(hResult);
      for(i = 0; i < nCount; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         uuid_generate(guid);
         _sntprintf(szQuery, 256, _T("UPDATE users SET guid='%s' WHERE id=%d"),
                    uuid_to_string(guid, szGUID), dwId);
         if (!SQLQuery(szQuery))
            if (!g_bIgnoreErrors)
            {
               DBFreeResult(hResult);
               return FALSE;
            }
      }
      DBFreeResult(hResult);
   }

   hResult = SQLSelect(_T("SELECT id FROM user_groups"));
   if (hResult != NULL)
   {
      nCount = DBGetNumRows(hResult);
      for(i = 0; i < nCount; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         uuid_generate(guid);
         _sntprintf(szQuery, 256, _T("UPDATE user_groups SET guid='%s' WHERE id=%d"),
                    uuid_to_string(guid, szGUID), dwId);
         if (!SQLQuery(szQuery))
            if (!g_bIgnoreErrors)
            {
               DBFreeResult(hResult);
               return FALSE;
            }
      }
      DBFreeResult(hResult);
   }

   if (!CreateConfigParam(_T("RADIUSServer"), _T("localhost"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("RADIUSSecret"), _T("netxms"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("RADIUSNumRetries"), _T("5"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("RADIUSTimeout"), _T("3"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='41' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V39 to V40
//

static BOOL H_UpgradeFromV39(void)
{
   static TCHAR m_szBatch[] =
      "ALTER TABLE users ADD grace_logins integer\n"
      "UPDATE users SET grace_logins=5\n"
      "<END>";

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='40' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V38 to V39
//

static BOOL H_UpgradeFromV38(void)
{
   static TCHAR m_szBatch[] =
	   "INSERT INTO maps (map_id,map_name,description,root_object_id) "
		   "VALUES (1,'Default','Default network map',1)\n"
	   "INSERT INTO map_access_lists (map_id,user_id,access_rights) VALUES (1,-2147483648,1)\n"
      "INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) "
         "VALUES (33,'SYS_SCRIPT_ERROR',2,1,"
		   "'Script (%1) execution error: %2',"
		   "'Generated when server encounters NXSL script execution error.#0D#0A"
		   "Parameters:#0D#0A"
		   "   1) Script name#0D#0A"
		   "   2) Error text#0D#0A"
		   "   3) DCI ID if script is DCI transformation script, or 0 otherwise')\n"
      "<END>";

   if (!CreateTable(_T("CREATE TABLE maps	("
		                 "map_id integer not null,"
		                 "map_name varchar(255) not null,"
		                 "description $SQL:TEXT not null,"
		                 "root_object_id integer not null,"
		                 "PRIMARY KEY(map_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE map_access_lists ("
		                 "map_id integer not null,"
		                 "user_id integer not null,"
		                 "access_rights integer not null,"
		                 "PRIMARY KEY(map_id,user_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE submaps ("
		                 "map_id integer not null,"
		                 "submap_id integer not null,"
		                 "attributes integer not null,"	
		                 "PRIMARY KEY(map_id,submap_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE submap_object_positions ("
		                 "map_id integer not null,"
		                 "submap_id integer not null,"
		                 "object_id integer not null,"
		                 "x integer not null,"
		                 "y integer not null,"
		                 "PRIMARY KEY(map_id,submap_id,object_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE submap_links ("
		                 "map_id integer not null,"
		                 "submap_id integer not null,"
		                 "object_id1 integer not null,"
		                 "object_id2 integer not null,"
		                 "link_type integer not null,"
		                 "PRIMARY KEY(map_id,submap_id,object_id1,object_id2))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("LockTimeout"), _T("60000"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("DisableVacuum"), _T("0"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='39' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V37 to V38
//

static BOOL H_UpgradeFromV37(void)
{
   static TCHAR m_szBatch[] =
	   "CREATE INDEX idx_event_log_event_timestamp ON event_log(event_timestamp)\n"
	   "CREATE INDEX idx_syslog_msg_timestamp ON syslog(msg_timestamp)\n"
      "CREATE INDEX idx_snmp_trap_log_trap_timestamp ON snmp_trap_log(trap_timestamp)\n"
      "<END>";

   if (!CreateTable(_T("CREATE TABLE snmp_trap_log ("
		                 "trap_id $SQL:INT64 not null,"
		                 "trap_timestamp integer not null,"
		                 "ip_addr varchar(15) not null,"
		                 "object_id integer not null,"
		                 "trap_oid varchar(255) not null,"
		                 "trap_varlist $SQL:TEXT not null,"
		                 "PRIMARY KEY(trap_id))")))

      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("LogAllSNMPTraps"), _T("0"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='38' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V36 to V37
//

static BOOL H_UpgradeFromV36(void)
{
   static TCHAR m_szBatch[] =
      "DROP TABLE new_nodes\n"
      "DELETE FROM config WHERE var_name='NewNodePollingInterval'\n"
      "INSERT INTO script_library (script_id,script_name,script_code) "
	      "VALUES (1,'Filter::SNMP','sub main()#0D#0A{#0D#0A   return $1->isSNMP;#0D#0A}#0D#0A')\n"
      "INSERT INTO script_library (script_id,script_name,script_code) "
	      "VALUES (2,'Filter::Agent','sub main()#0D#0A{#0D#0A   return $1->isAgent;#0D#0A}#0D#0A')\n"
      "INSERT INTO script_library (script_id,script_name,script_code) "
	      "VALUES (3,'Filter::AgentOrSNMP','sub main()#0D#0A{#0D#0A   return $1->isAgent || $1->isSNMP;#0D#0A}#0D#0A')\n"
      "INSERT INTO script_library (script_id,script_name,script_code) "
	      "VALUES (4,'DCI::SampleTransform','sub dci_transform()#0D#0A{#0D#0A   return $1 + 1;#0D#0A}#0D#0A')\n"
      "<END>";

   if (!CreateTable(_T("CREATE TABLE script_library ("
		                 "script_id integer not null,"
		                 "script_name varchar(63) not null,"
		                 "script_code $SQL:TEXT not null,"
		                 "PRIMARY KEY(script_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("DefaultCommunityString"), _T("public"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("DiscoveryFilter"), _T("none"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='37' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V35 to V36
//

static BOOL H_UpgradeFromV35(void)
{
   static TCHAR m_szBatch[] =
      "ALTER TABLE nodes ADD proxy_node integer\n"
      "UPDATE nodes SET proxy_node=0\n"
      "ALTER TABLE object_tools ADD matching_oid varchar(255)\n"
      "UPDATE object_tools SET matching_oid='#00'\n"
      "<END>";

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("CapabilityExpirationTime"), _T("604800"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='36' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V34 to V35
//

static BOOL H_UpgradeFromV34(void)
{
   static TCHAR m_szBatch[] =
      "ALTER TABLE object_properties DROP COLUMN status_alg\n"
      "ALTER TABLE object_properties ADD status_calc_alg integer\n"
	   "ALTER TABLE object_properties ADD status_prop_alg integer\n"
	   "ALTER TABLE object_properties ADD status_fixed_val integer\n"
	   "ALTER TABLE object_properties ADD status_shift integer\n"
	   "ALTER TABLE object_properties ADD status_translation varchar(8)\n"
	   "ALTER TABLE object_properties ADD status_single_threshold integer\n"
	   "ALTER TABLE object_properties ADD status_thresholds varchar(8)\n"
      "UPDATE object_properties SET status_calc_alg=0,status_prop_alg=0,"
         "status_fixed_val=0,status_shift=0,status_translation='01020304',"
         "status_single_threshold=75,status_thresholds='503C2814'\n"
      "DELETE FROM config WHERE var_name='StatusCalculationAlgorithm'\n"
      "<END>";

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("StatusCalculationAlgorithm"), _T("1"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("StatusPropagationAlgorithm"), _T("1"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("FixedStatusValue"), _T("0"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("StatusShift"), _T("0"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("StatusTranslation"), _T("01020304"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("StatusSingleThreshold"), _T("75"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("StatusThresholds"), _T("503C2814"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='35' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V33 to V34
//

static BOOL H_UpgradeFromV33(void)
{
   static TCHAR m_szBatch[] =
      "ALTER TABLE items ADD adv_schedule integer\n"
      "UPDATE items SET adv_schedule=0\n"
      "<END>";
   
   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE dci_schedules ("
		                 "item_id integer not null,"
                       "schedule varchar(255) not null)")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE syslog ("
                       "msg_id $SQL:INT64 not null,"
		                 "msg_timestamp integer not null,"
		                 "facility integer not null,"
		                 "severity integer not null,"
		                 "source_object_id integer not null,"
		                 "hostname varchar(127) not null,"
		                 "msg_tag varchar(32) not null,"
		                 "msg_text $SQL:TEXT not null,"
		                 "PRIMARY KEY(msg_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("IcmpPingSize"), _T("46"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("SMSDrvConfig"), _T(""), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("EnableSyslogDaemon"), _T("0"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("SyslogListenPort"), _T("514"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("SyslogRetentionTime"), _T("5184000"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='34' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V32 to V33
//

static BOOL H_UpgradeFromV32(void)
{
   static TCHAR m_szBatch[] =
      "INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,description) "
	      "VALUES (5,'&Info->&Switch forwarding database (FDB)',2 ,'Forwarding database',0,'Show switch forwarding database')\n"
      "INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) "
	      "VALUES (5,0,'MAC Address','.1.3.6.1.2.1.17.4.3.1.1',4 ,0)\n"
      "INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) "
	      "VALUES (5,1,'Port','.1.3.6.1.2.1.17.4.3.1.2',1 ,0)\n"
      "INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,description) "
	      "VALUES (6,'&Connect->Open &web browser',4 ,'http://%OBJECT_IP_ADDR%',0,'Open embedded web browser to node')\n"
      "INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,description) "
	      "VALUES (7,'&Connect->Open &web browser (HTTPS)',4 ,'https://%OBJECT_IP_ADDR%',0,'Open embedded web browser to node using HTTPS')\n"
      "INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,description) "
	      "VALUES (8,'&Info->&Agent->&Subagent list',3 ,'Subagent List#7FAgent.SubAgentList#7F^(.*) (.*) (.*) (.*)',0,'Show list of loaded subagents')\n"
      "INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) "
	      "VALUES (8,0,'Name','',0 ,1)\n"
      "INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) "
	      "VALUES (8,1,'Version','',0 ,2)\n"
      "INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) "
	      "VALUES (8,2,'File','',0 ,4)\n"
      "INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) "
	      "VALUES (8,3,'Module handle','',0 ,3)\n"
      "INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,description) "
	      "VALUES (9,'&Info->&Agent->Supported &parameters',3 ,'Supported parameters#7FAgent.SupportedParameters#7F^(.*)',0,'Show list of parameters supported by agent')\n"
      "INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) "
	      "VALUES (9,0,'Parameter','',0 ,1)\n"
      "INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,description) "
	      "VALUES (10,'&Info->&Agent->Supported &enums',3 ,'Supported enums#7FAgent.SupportedEnums#7F^(.*)',0,'Show list of enums supported by agent')\n"
      "INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) "
	      "VALUES (10,0,'Parameter','',0 ,1)\n"
      "INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,description) "
	      "VALUES (11,'&Info->&Agent->Supported &actions',3 ,'Supported actions#7FAgent.ActionList#7F^(.*) (.*) #22(.*)#22.*',0,'Show list of actions supported by agent')\n"
      "INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) "
	      "VALUES (11,0,'Name','',0 ,1)\n"
      "INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) "
	      "VALUES (11,1,'Type','',0 ,2)\n"
      "INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) "
	      "VALUES (11,2,'Data','',0 ,3)\n"
      "INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,description) "
	      "VALUES (12,'&Info->&Agent->Configured &ICMP targets',3 ,'Configured ICMP targets#7FICMP.TargetList#7F^(.*) (.*) (.*) (.*)',0,'Show list of actions supported by agent')\n"
      "INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) "
	      "VALUES (12,0,'IP Address','',0 ,1)\n"
      "INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) "
	      "VALUES (12,1,'Name','',0 ,4)\n"
      "INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) "
	      "VALUES (12,2,'Last RTT','',0 ,2)\n"
      "INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) "
	      "VALUES (12,4,'Average RTT','',0 ,3)\n"
      "INSERT INTO object_tools_acl (tool_id,user_id) VALUES (5,-2147483648)\n"
      "INSERT INTO object_tools_acl (tool_id,user_id) VALUES (6,-2147483648)\n"
      "INSERT INTO object_tools_acl (tool_id,user_id) VALUES (7,-2147483648)\n"
      "INSERT INTO object_tools_acl (tool_id,user_id) VALUES (8,-2147483648)\n"
      "INSERT INTO object_tools_acl (tool_id,user_id) VALUES (9,-2147483648)\n"
      "INSERT INTO object_tools_acl (tool_id,user_id) VALUES (10,-2147483648)\n"
      "INSERT INTO object_tools_acl (tool_id,user_id) VALUES (11,-2147483648)\n"
      "INSERT INTO object_tools_acl (tool_id,user_id) VALUES (12,-2147483648)\n"
      "<END>";

   if (!CreateTable(_T("CREATE TABLE object_tools_table_columns ("
                  	  "tool_id integer not null,"
		                 "col_number integer not null,"
		                 "col_name varchar(255),"
		                 "col_oid varchar(255),"
		                 "col_format integer,"
		                 "col_substr integer,"
		                 "PRIMARY KEY(tool_id,col_number))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='33' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V31 to V32
//

static BOOL H_UpgradeFromV31(void)
{
   static TCHAR m_szBatch[] =
      "INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,description) "
	      "VALUES (1,'&Shutdown system',1 ,'System.Shutdown',0,'Shutdown target node via NetXMS agent')\n"
      "INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,description) "
	      "VALUES (2,'&Restart system',1 ,'System.Restart',0,'Restart target node via NetXMS agent')\n"
      "INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,description) "
	      "VALUES (3,'&Wakeup node',0 ,'wakeup',0,'Wakeup node using Wake-On-LAN magic packet')\n"
      "INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,description) "
	      "VALUES (4,'Restart &agent',1 ,'Agent.Restart',0,'Restart NetXMS agent on target node')\n"
      "INSERT INTO object_tools_acl (tool_id,user_id) VALUES (1,-2147483648)\n"
      "INSERT INTO object_tools_acl (tool_id,user_id) VALUES (2,-2147483648)\n"
      "INSERT INTO object_tools_acl (tool_id,user_id) VALUES (3,-2147483648)\n"
      "INSERT INTO object_tools_acl (tool_id,user_id) VALUES (4,-2147483648)\n"
      "<END>";

   if (!CreateTable(_T("CREATE TABLE object_tools ("
	                    "tool_id integer not null,"
	                    "tool_name varchar(255) not null,"
	                    "tool_type integer not null,"
	                    "tool_data $SQL:TEXT,"
	                    "description varchar(255),"
	                    "flags integer not null,"
	                    "PRIMARY KEY(tool_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE object_tools_acl ("
	                    "tool_id integer not null,"
	                    "user_id integer not null,"
	                    "PRIMARY KEY(tool_id,user_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='32' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V30 to V31
//

static BOOL H_UpgradeFromV30(void)
{
   static TCHAR m_szBatch[] =
	   "INSERT INTO default_images (object_class,image_id) "
		   "VALUES (12, 14)\n"
	   "INSERT INTO images (image_id,name,file_name_png,file_hash_png,"
         "file_name_ico,file_hash_ico) VALUES (14,'Obj.VPNConnector',"
         "'vpnc.png','<invalid_hash>','vpnc.ico','<invalid_hash>')\n"
      "<END>";

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE vpn_connectors ("
		                 "id integer not null,"
		                 "node_id integer not null,"
		                 "peer_gateway integer not null,"
		                 "PRIMARY KEY(id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE vpn_connector_networks ("
		                 "vpn_id integer not null,"
		                 "network_type integer not null,"
		                 "ip_addr varchar(15) not null,"
		                 "ip_netmask varchar(15) not null,"
		                 "PRIMARY KEY(vpn_id,ip_addr))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("NumberOfRoutingTablePollers"), _T("5"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("RoutingTableUpdateInterval"), _T("300"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='31' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V29 to V30
//

static BOOL H_UpgradeFromV29(void)
{
   static TCHAR m_szBatch[] =
      "ALTER TABLE object_properties ADD status_alg integer\n"
      "UPDATE object_properties SET status_alg=-1\n"
      "<END>";

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("StatusCalculationAlgorithm"), _T("0"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("EnableMultipleDBConnections"), _T("1"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("NumberOfDatabaseWriters"), _T("1"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("DefaultEncryptionPolicy"), _T("1"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("AllowedCiphers"), _T("15"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("KeepAliveInterval"), _T("60"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='30' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V28 to V29
//

static BOOL H_UpgradeFromV28(void)
{
   static TCHAR m_szBatch[] =
      "ALTER TABLE nodes ADD zone_guid integer\n"
      "ALTER TABLE subnets ADD zone_guid integer\n"
      "UPDATE nodes SET zone_guid=0\n"
      "UPDATE subnets SET zone_guid=0\n"
      "INSERT INTO default_images (object_class,image_id) VALUES (6,13)\n"
      "INSERT INTO images (image_id,name,file_name_png,file_hash_png,"
         "file_name_ico,file_hash_ico) VALUES (13,'Obj.Zone','zone.png',"
         "'<invalid_hash>','zone.ico','<invalid_hash>')\n"
      "<END>";

   if (!CreateTable(_T("CREATE TABLE zones ("
	                    "id integer not null,"
	                    "zone_guid integer not null,"
	                    "zone_type integer not null,"
	                    "controller_ip varchar(15) not null,"
	                    "description $SQL:TEXT,"
	                    "PRIMARY KEY(id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE zone_ip_addr_list ("
	                    "zone_id integer not null,"
	                    "ip_addr varchar(15) not null,"
	                    "PRIMARY KEY(zone_id,ip_addr))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("EnableZoning"), _T("0"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='29' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V27 to V28
//

static BOOL H_UpgradeFromV27(void)
{
   static TCHAR m_szBatch[] =
      "ALTER TABLE users ADD system_access integer\n"
      "UPDATE users SET system_access=access\n"
      "ALTER TABLE users DROP COLUMN access\n"
      "ALTER TABLE user_groups ADD system_access integer\n"
      "UPDATE user_groups SET system_access=access\n"
      "ALTER TABLE user_groups DROP COLUMN access\n"
      "<END>";

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='28' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Move object data from class-specific tables to object_prioperties table
//

static BOOL MoveObjectData(DWORD dwId, BOOL bInheritRights)
{
   DB_RESULT hResult;
   TCHAR szQuery[1024] ,szName[MAX_OBJECT_NAME];
   BOOL bRead = FALSE, bIsDeleted, bIsTemplate;
   DWORD i, dwStatus, dwImageId;
   static TCHAR *m_pszTableNames[] = { _T("nodes"), _T("interfaces"), _T("subnets"),
                                       _T("templates"), _T("network_services"),
                                       _T("containers"), NULL };

   // Try to read information from nodes table
   for(i = 0; (!bRead) && (m_pszTableNames[i] != NULL); i++)
   {
      bIsTemplate = !_tcscmp(m_pszTableNames[i], _T("templates"));
      _sntprintf(szQuery, 1024, _T("SELECT name,is_deleted,image_id%s FROM %s WHERE id=%d"),
                 bIsTemplate ? _T("") : _T(",status"),
                 m_pszTableNames[i], dwId);
      hResult = SQLSelect(szQuery);
      if (hResult != NULL)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            DBGetField(hResult, 0, 0, szName, MAX_OBJECT_NAME);
            bIsDeleted = DBGetFieldLong(hResult, 0, 1) ? TRUE : FALSE;
            dwImageId = DBGetFieldULong(hResult, 0, 2);
            dwStatus = bIsTemplate ? STATUS_UNKNOWN : DBGetFieldULong(hResult, 0, 3);
            bRead = TRUE;
         }
         DBFreeResult(hResult);
      }
      else
      {
         if (!g_bIgnoreErrors)
            return FALSE;
      }
   }

   if (bRead)
   {
      _sntprintf(szQuery, 1024, _T("INSERT INTO object_properties (object_id,name,"
                                   "status,is_deleted,image_id,inherit_access_rights,"
                                   "last_modified) VALUES (%d,'%s',%d,%d,%d,%d,%ld)"),
                 dwId, szName, dwStatus, bIsDeleted, dwImageId, bInheritRights, time(NULL));

      if (!SQLQuery(szQuery))
         if (!g_bIgnoreErrors)
            return FALSE;
   }
   else
   {
      _tprintf(_T("WARNING: object with ID %d presented in access control tables but cannot be found in data tables\n"), dwId);
   }

   return TRUE;
}


//
// Upgrade from V26 to V27
//

static BOOL H_UpgradeFromV26(void)
{
   DB_RESULT hResult;
   DWORD i, dwNumObjects, dwId;
   static TCHAR m_szBatch[] =
      "ALTER TABLE nodes DROP COLUMN name\n"
      "ALTER TABLE nodes DROP COLUMN status\n"
      "ALTER TABLE nodes DROP COLUMN is_deleted\n"
      "ALTER TABLE nodes DROP COLUMN image_id\n"
      "ALTER TABLE interfaces DROP COLUMN name\n"
      "ALTER TABLE interfaces DROP COLUMN status\n"
      "ALTER TABLE interfaces DROP COLUMN is_deleted\n"
      "ALTER TABLE interfaces DROP COLUMN image_id\n"
      "ALTER TABLE subnets DROP COLUMN name\n"
      "ALTER TABLE subnets DROP COLUMN status\n"
      "ALTER TABLE subnets DROP COLUMN is_deleted\n"
      "ALTER TABLE subnets DROP COLUMN image_id\n"
      "ALTER TABLE network_services DROP COLUMN name\n"
      "ALTER TABLE network_services DROP COLUMN status\n"
      "ALTER TABLE network_services DROP COLUMN is_deleted\n"
      "ALTER TABLE network_services DROP COLUMN image_id\n"
      "ALTER TABLE containers DROP COLUMN name\n"
      "ALTER TABLE containers DROP COLUMN status\n"
      "ALTER TABLE containers DROP COLUMN is_deleted\n"
      "ALTER TABLE containers DROP COLUMN image_id\n"
      "ALTER TABLE templates DROP COLUMN name\n"
      "ALTER TABLE templates DROP COLUMN is_deleted\n"
      "ALTER TABLE templates DROP COLUMN image_id\n"
      "DROP TABLE access_options\n"
      "DELETE FROM config WHERE var_name='TopologyRootObjectName'\n"
      "DELETE FROM config WHERE var_name='TopologyRootImageId'\n"
      "DELETE FROM config WHERE var_name='ServiceRootObjectName'\n"
      "DELETE FROM config WHERE var_name='ServiceRootImageId'\n"
      "DELETE FROM config WHERE var_name='TemplateRootObjectName'\n"
      "DELETE FROM config WHERE var_name='TemplateRootImageId'\n"
      "INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) "
         "VALUES (31,'SYS_SNMP_OK',0,1,'Connectivity with SNMP agent restored',"
		   "'Generated when connectivity with node#27s SNMP agent restored.#0D#0A"
		   "Parameters:#0D#0A   No message-specific parameters')\n"
      "INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) "
		   "VALUES (32,'SYS_AGENT_OK',0,1,'Connectivity with native agent restored',"
		   "'Generated when connectivity with node#27s native agent restored.#0D#0A"
		   "Parameters:#0D#0A   No message-specific parameters')\n"
      "<END>";

   if (!CreateTable(_T("CREATE TABLE object_properties ("
	                    "object_id integer not null,"
	                    "name varchar(63) not null,"
	                    "status integer not null,"
	                    "is_deleted integer not null,"
	                    "image_id integer,"
	                    "last_modified integer not null,"
	                    "inherit_access_rights integer not null,"
	                    "PRIMARY KEY(object_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE user_profiles ("
	                    "user_id integer not null,"
	                    "var_name varchar(255) not null,"
	                    "var_value $SQL:TEXT,"
	                    "PRIMARY KEY(user_id,var_name))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   // Move data from access_options and class-specific tables to object_properties
   hResult = SQLSelect(_T("SELECT object_id,inherit_rights FROM access_options"));
   if (hResult != NULL)
   {
      dwNumObjects = DBGetNumRows(hResult);
      for(i = 0; i < dwNumObjects; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         if (dwId >= 10)   // Id below 10 reserved for built-in objects
         {
            if (!MoveObjectData(dwId, DBGetFieldLong(hResult, i, 1) ? TRUE : FALSE))
            {
               DBFreeResult(hResult);
               return FALSE;
            }
         }
         else
         {
            TCHAR szName[MAX_OBJECT_NAME], szQuery[1024];
            DWORD dwImageId;
            BOOL bValidObject = TRUE;

            switch(dwId)
            {
               case 1:     // Topology Root
                  ConfigReadStr(_T("TopologyRootObjectName"), szName, 
                                MAX_OBJECT_NAME, _T("Entire Network"));
                  dwImageId = ConfigReadULong(_T("TopologyRootImageId"), 0);
                  break;
               case 2:     // Service Root
                  ConfigReadStr(_T("ServiceRootObjectName"), szName, 
                                MAX_OBJECT_NAME, _T("All Services"));
                  dwImageId = ConfigReadULong(_T("ServiceRootImageId"), 0);
                  break;
               case 3:     // Template Root
                  ConfigReadStr(_T("TemplateRootObjectName"), szName, 
                                MAX_OBJECT_NAME, _T("All Services"));
                  dwImageId = ConfigReadULong(_T("TemplateRootImageId"), 0);
                  break;
               default:
                  bValidObject = FALSE;
                  break;
            }

            if (bValidObject)
            {
               _sntprintf(szQuery, 1024, _T("INSERT INTO object_properties (object_id,name,"
                                            "status,is_deleted,image_id,inherit_access_rights,"
                                            "last_modified) VALUES (%d,'%s',5,0,%d,%d,%ld)"),
                          dwId, szName, dwImageId,
                          DBGetFieldLong(hResult, i, 1) ? TRUE : FALSE,
                          time(NULL));

               if (!SQLQuery(szQuery))
                  if (!g_bIgnoreErrors)
                     return FALSE;
            }
            else
            {
               _tprintf(_T("WARNING: Invalid built-in object ID %d\n"), dwId);
            }
         }
      }
      DBFreeResult(hResult);
   }
   else
   {
      if (!g_bIgnoreErrors)
         return FALSE;
   }

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='27' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V25 to V26
//

static BOOL H_UpgradeFromV25(void)
{
   DB_RESULT hResult;
   TCHAR szTemp[512];

   hResult = SQLSelect(_T("SELECT var_value FROM config WHERE var_name='IDataIndexCreationCommand'"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         if (!CreateConfigParam(_T("IDataIndexCreationCommand_0"),
                                DBGetField(hResult, 0, 0, szTemp, 512), 0, 1))
         {
            if (!g_bIgnoreErrors)
            {
               DBFreeResult(hResult);
               return FALSE;
            }
         }
      }
      DBFreeResult(hResult);

      if (!SQLQuery(_T("DELETE FROM config WHERE var_name='IDataIndexCreationCommand'")))
         if (!g_bIgnoreErrors)
            return FALSE;
   }

   if (!CreateConfigParam(_T("IDataIndexCreationCommand_1"), 
                          _T("CREATE INDEX idx_timestamp ON idata_%d(idata_timestamp)"), 0, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='26' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V24 to V25
//

static BOOL H_UpgradeFromV24(void)
{
   DB_RESULT hResult;
   int i, iNumRows;
   DWORD dwNodeId;
   TCHAR szQuery[256];

   _tprintf("Create indexes on existing IDATA tables? (Y/N) ");
   if (GetYesNo())
   {
      hResult = SQLSelect(_T("SELECT id FROM nodes WHERE is_deleted=0"));
      if (hResult != NULL)
      {
         iNumRows = DBGetNumRows(hResult);
         for(i = 0; i < iNumRows; i++)
         {
            dwNodeId = DBGetFieldULong(hResult, i, 0);
            _tprintf(_T("Creating indexes for table \"idata_%d\"...\n"), dwNodeId);
            _sntprintf(szQuery, 256, _T("CREATE INDEX idx_timestamp ON idata_%d(idata_timestamp)"), dwNodeId);
            if (!SQLQuery(szQuery))
               if (!g_bIgnoreErrors)
               {
                  DBFreeResult(hResult);
                  return FALSE;
               }
         }
         DBFreeResult(hResult);
      }
      else
      {
         if (!g_bIgnoreErrors)
            return FALSE;
      }
   }

   if (!SQLQuery(_T("UPDATE config SET var_value='25' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V23 to V24
//

static BOOL H_UpgradeFromV23(void)
{
   DB_RESULT hResult;
   TCHAR szQuery[256];
   int i, iNumRows;

   if (!CreateTable(_T("CREATE TABLE raw_dci_values ("
                       "	item_id integer not null,"
	                    "   raw_value varchar(255),"
	                    "   last_poll_time integer,"
	                    "   PRIMARY KEY(item_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("CREATE INDEX idx_item_id ON raw_dci_values(item_id)")))
      if (!g_bIgnoreErrors)
         return FALSE;


   // Create empty records in raw_dci_values for all existing DCIs
   hResult = SQLSelect(_T("SELECT item_id FROM items"));
   if (hResult != NULL)
   {
      iNumRows = DBGetNumRows(hResult);
      for(i = 0; i < iNumRows; i++)
      {
         _stprintf(szQuery, _T("INSERT INTO raw_dci_values (item_id,"
                               "raw_value,last_poll_time) VALUES (%d,'#00',1)"),
                   DBGetFieldULong(hResult, i, 0));
         if (!SQLQuery(szQuery))
            if (!g_bIgnoreErrors)
            {
               DBFreeResult(hResult);
               return FALSE;
            }
      }
      DBFreeResult(hResult);
   }
   else
   {
      if (!g_bIgnoreErrors)
         return FALSE;
   }

   if (!SQLQuery(_T("UPDATE config SET var_value='24' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V22 to V23
//

static BOOL H_UpgradeFromV22(void)
{
   static TCHAR m_szBatch[] =
      "ALTER TABLE items ADD template_item_id integer\n"
      "UPDATE items SET template_item_id=0\n"
      "CREATE INDEX idx_sequence ON thresholds(sequence_number)\n"
      "<END>";

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='23' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V21 to V22
//

static BOOL H_UpgradeFromV21(void)
{
   static TCHAR m_szBatch[] =
      "INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) "
         "VALUES (4009,'DC_HDD_TEMP_WARNING',1,1,"
		   "'Temperature of hard disk %6 is above warning level of %3 (current: %4)',"
		   "'Custom data collection threshold event.#0D#0AParameters:#0D#0A"
		   "   1) Parameter name#0D#0A   2) Item description#0D#0A"
         "   3) Threshold value#0D#0A   4) Actual value#0D#0A"
         "   5) Data collection item ID#0D#0A   6) Instance')\n"
      "INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) "
      	"VALUES (4010,'DC_HDD_TEMP_MAJOR',3,1,"
		   "'Temperature of hard disk %6 is above %3 (current: %4)',"
		   "'Custom data collection threshold event.#0D#0AParameters:#0D#0A"
		   "   1) Parameter name#0D#0A   2) Item description#0D#0A"
         "   3) Threshold value#0D#0A   4) Actual value#0D#0A"
         "   5) Data collection item ID#0D#0A   6) Instance')\n"
      "INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) "
	      "VALUES (4011,'DC_HDD_TEMP_CRITICAL',4,1,"
		   "'Temperature of hard disk %6 is above critical level of %3 (current: %4)',"
		   "'Custom data collection threshold event.#0D#0AParameters:#0D#0A"
		   "   1) Parameter name#0D#0A   2) Item description#0D#0A"
         "   3) Threshold value#0D#0A   4) Actual value#0D#0A"
         "   5) Data collection item ID#0D#0A   6) Instance')\n"
      "<END>";

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("SMSDriver"), _T("<none>"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("AgentUpgradeWaitTime"), _T("600"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("NumberOfUpgradeThreads"), _T("10"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='22' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V20 to V21
//

static BOOL H_UpgradeFromV20(void)
{
   static TCHAR m_szBatch[] =
      "INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description)" 
         " VALUES (30,'SYS_SMS_FAILURE',1,1,'Unable to send SMS to phone %1',"
		   "'Generated when server is unable to send SMS.#0D#0A"
		   "   Parameters:#0D#0A   1) Phone number')\n"
      "ALTER TABLE nodes ADD node_flags integer\n"
      "<END>";
   static TCHAR m_szBatch2[] =
      "ALTER TABLE nodes DROP COLUMN is_snmp\n"
      "ALTER TABLE nodes DROP COLUMN is_agent\n"
      "ALTER TABLE nodes DROP COLUMN is_bridge\n"
      "ALTER TABLE nodes DROP COLUMN is_router\n"
      "ALTER TABLE nodes DROP COLUMN is_local_mgmt\n"
      "ALTER TABLE nodes DROP COLUMN is_ospf\n"
      "CREATE INDEX idx_item_id ON thresholds(item_id)\n"
      "<END>";
   static DWORD m_dwFlag[] = { NF_IS_SNMP, NF_IS_NATIVE_AGENT, NF_IS_BRIDGE,
                               NF_IS_ROUTER, NF_IS_LOCAL_MGMT, NF_IS_OSPF };
   DB_RESULT hResult;
   int i, j, iNumRows;
   DWORD dwFlags, dwNodeId;
   TCHAR szQuery[256];

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   // Convert "is_xxx" fields into one "node_flags" field
   hResult = SQLSelect(_T("SELECT id,is_snmp,is_agent,is_bridge,is_router,"
                          "is_local_mgmt,is_ospf FROM nodes"));
   if (hResult != NULL)
   {
      iNumRows = DBGetNumRows(hResult);
      for(i = 0; i < iNumRows; i++)
      {
         dwFlags = 0;
         for(j = 1; j <= 6; j++)
            if (DBGetFieldLong(hResult, i, j))
               dwFlags |= m_dwFlag[j - 1];
         _sntprintf(szQuery, 256, _T("UPDATE nodes SET node_flags=%d WHERE id=%d"),
                    dwFlags, DBGetFieldULong(hResult, i, 0));
         if (!SQLQuery(szQuery))
            if (!g_bIgnoreErrors)
            {
               DBFreeResult(hResult);
               return FALSE;
            }
      }
      DBFreeResult(hResult);
   }
   else
   {
      if (!g_bIgnoreErrors)
         return FALSE;
   }

   if (!SQLBatch(m_szBatch2))
      if (!g_bIgnoreErrors)
         return FALSE;

   _tprintf("Create indexes on existing IDATA tables? (Y/N) ");
   if (GetYesNo())
   {
      hResult = SQLSelect(_T("SELECT id FROM nodes WHERE is_deleted=0"));
      if (hResult != NULL)
      {
         iNumRows = DBGetNumRows(hResult);
         for(i = 0; i < iNumRows; i++)
         {
            dwNodeId = DBGetFieldULong(hResult, i, 0);
            _tprintf(_T("Creating indexes for table \"idata_%d\"...\n"), dwNodeId);
            _sntprintf(szQuery, 256, _T("CREATE INDEX idx_item_id ON idata_%d(item_id)"), dwNodeId);
            if (!SQLQuery(szQuery))
               if (!g_bIgnoreErrors)
               {
                  DBFreeResult(hResult);
                  return FALSE;
               }
         }
         DBFreeResult(hResult);
      }
      else
      {
         if (!g_bIgnoreErrors)
            return FALSE;
      }
   }

   if (!CreateConfigParam(_T("NumberOfStatusPollers"), _T("10"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;
   if (!CreateConfigParam(_T("NumberOfConfigurationPollers"), _T("4"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;
   if (!CreateConfigParam(_T("IDataIndexCreationCommand"), _T("CREATE INDEX idx_item_id ON idata_%d(item_id)"), 0, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='21' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V19 to V20
//

static BOOL H_UpgradeFromV19(void)
{
   static TCHAR m_szBatch[] =
      "ALTER TABLE nodes ADD poller_node_id integer\n"
      "ALTER TABLE nodes ADD is_ospf integer\n"
      "UPDATE nodes SET poller_node_id=0,is_ospf=0\n"
      "INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description)"
         " VALUES (28,'SYS_NODE_DOWN',4,1,'Node down',"
		   "'Generated when node is not responding to management server.#0D#0A"
		   "Parameters:#0D#0A   No event-specific parameters')\n"
      "INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description)"
         " VALUES (29,'SYS_NODE_UP',0,1,'Node up',"
		   "'Generated when communication with the node re-established.#0D#0A"
		   "Parameters:#0D#0A   No event-specific parameters')\n"
      "INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description)"
         " VALUES (25,'SYS_SERVICE_DOWN',3,1,'Network service #22%1#22 is not responding',"
		   "'Generated when network service is not responding to management server as "
         "expected.#0D#0AParameters:#0D#0A   1) Service name#0D0A"
		   "   2) Service object ID#0D0A   3) Service type')\n"
      "INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description)"
         " VALUES (26,'SYS_SERVICE_UP',0,1,"
         "'Network service #22%1#22 returned to operational state',"
		   "'Generated when network service responds as expected after failure.#0D#0A"  
         "Parameters:#0D#0A   1) Service name#0D0A"
		   "   2) Service object ID#0D0A   3) Service type')\n"
      "INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description)"
         " VALUES (27,'SYS_SERVICE_UNKNOWN',1,1,"
		   "'Status of network service #22%1#22 is unknown',"
		   "'Generated when management server is unable to determine state of the network "
         "service due to agent or server-to-agent communication failure.#0D#0A"
         "Parameters:#0D#0A   1) Service name#0D0A"
		   "   2) Service object ID#0D0A   3) Service type')\n"
      "INSERT INTO images (image_id,name,file_name_png,file_hash_png,file_name_ico,"
         "file_hash_ico) VALUES (12,'Obj.NetworkService','network_service.png',"
         "'<invalid_hash>','network_service.ico','<invalid_hash>')\n"
      "INSERT INTO default_images (object_class,image_id) VALUES (11,12)\n"
      "<END>";
   
   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE network_services ("
	                    "id integer not null,"
	                    "name varchar(63),"
	                    "status integer,"
                       "is_deleted integer,"
	                    "node_id integer not null,"
	                    "service_type integer,"
	                    "ip_bind_addr varchar(15),"
	                    "ip_proto integer,"
	                    "ip_port integer,"
	                    "check_request $SQL:TEXT,"
	                    "check_responce $SQL:TEXT,"
	                    "poller_node_id integer not null,"
	                    "image_id integer not null,"
	                    "PRIMARY KEY(id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='20' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V18 to V19
//

static BOOL H_UpgradeFromV18(void)
{
   static TCHAR m_szBatch[] =
      "ALTER TABLE nodes ADD platform_name varchar(63)\n"
      "UPDATE nodes SET platform_name=''\n"
      "<END>";

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE agent_pkg ("
	                       "pkg_id integer not null,"
	                       "pkg_name varchar(63),"
	                       "version varchar(31),"
	                       "platform varchar(63),"
	                       "pkg_file varchar(255),"
	                       "description varchar(255),"
	                       "PRIMARY KEY(pkg_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='19' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V17 to V18
//

static BOOL H_UpgradeFromV17(void)
{
   static TCHAR m_szBatch[] =
      "ALTER TABLE nodes DROP COLUMN inherit_access_rights\n"
      "ALTER TABLE nodes ADD agent_version varchar(63)\n"
      "UPDATE nodes SET agent_version='' WHERE is_agent=0\n"
      "UPDATE nodes SET agent_version='<unknown>' WHERE is_agent=1\n"
      "INSERT INTO event_cfg (event_code,event_name,severity,flags,"
         "message,description) SELECT event_id,name,severity,flags,"
         "message,description FROM events\n"
      "DROP TABLE events\n"
      "DROP TABLE event_group_members\n"
      "CREATE TABLE event_group_members (group_id integer not null,"
	      "event_code integer not null,	PRIMARY KEY(group_id,event_code))\n"
      "ALTER TABLE alarms ADD source_event_code integer\n"
      "UPDATE alarms SET source_event_code=source_event_id\n"
      "ALTER TABLE alarms DROP COLUMN source_event_id\n"
      "ALTER TABLE alarms ADD source_event_id bigint\n"
      "UPDATE alarms SET source_event_id=0\n"
      "ALTER TABLE snmp_trap_cfg ADD event_code integer not null default 0\n"
      "UPDATE snmp_trap_cfg SET event_code=event_id\n"
      "ALTER TABLE snmp_trap_cfg DROP COLUMN event_id\n"
      "DROP TABLE event_log\n"
      "CREATE TABLE event_log (event_id bigint not null,event_code integer,"
	      "event_timestamp integer,event_source integer,event_severity integer,"
	      "event_message varchar(255),root_event_id bigint default 0,"
	      "PRIMARY KEY(event_id))\n"
      "<END>";
   TCHAR szQuery[4096];
   DB_RESULT hResult;

   hResult = SQLSelect(_T("SELECT rule_id,event_id FROM policy_event_list"));
   if (hResult != NULL)
   {
      DWORD i, dwNumRows;

      if (!SQLQuery(_T("DROP TABLE policy_event_list")))
      {
         if (!g_bIgnoreErrors)
         {
            DBFreeResult(hResult);
            return FALSE;
         }
      }

      if (!SQLQuery(_T("CREATE TABLE policy_event_list ("
                       "rule_id integer not null,"
                       "event_code integer not null,"
                       "PRIMARY KEY(rule_id,event_code))")))
      {
         if (!g_bIgnoreErrors)
         {
            DBFreeResult(hResult);
            return FALSE;
         }
      }

      dwNumRows = DBGetNumRows(hResult);
      for(i = 0; i < dwNumRows; i++)
      {
         _sntprintf(szQuery, 4096, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"),
                    DBGetFieldULong(hResult, i, 0), DBGetFieldULong(hResult, i, 1));
         if (!SQLQuery(szQuery))
            if (!g_bIgnoreErrors)
            {
               DBFreeResult(hResult);
               return FALSE;
            }
      }

      DBFreeResult(hResult);
   }
   else
   {
      if (!g_bIgnoreErrors)
         return FALSE;
   }

   _sntprintf(szQuery, 4096, 
      _T("CREATE TABLE event_cfg (event_code integer not null,"
	      "event_name varchar(63) not null,severity integer,flags integer,"
	      "message varchar(255),description %s,PRIMARY KEY(event_code))"),
              g_pszSqlType[g_iSyntax][SQL_TYPE_TEXT]);
   if (!SQLQuery(szQuery))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   _sntprintf(szQuery, 4096,
      _T("CREATE TABLE modules (module_id integer not null,"
	      "module_name varchar(63),exec_name varchar(255),"
	      "module_flags integer not null default 0,description %s,"
	      "license_key varchar(255),PRIMARY KEY(module_id))"),
              g_pszSqlType[g_iSyntax][SQL_TYPE_TEXT]);
   if (!SQLQuery(szQuery))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='18' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V16 to V17
//

static BOOL H_UpgradeFromV16(void)
{
   static TCHAR m_szBatch[] =
      "DROP TABLE locks\n"
      "CREATE TABLE snmp_trap_cfg (trap_id integer not null,snmp_oid varchar(255) not null,"
	      "event_id integer not null,description varchar(255),PRIMARY KEY(trap_id))\n"
      "CREATE TABLE snmp_trap_pmap (trap_id integer not null,parameter integer not null,"
         "snmp_oid varchar(255),description varchar(255),PRIMARY KEY(trap_id,parameter))\n"
      "INSERT INTO events (event_id,name,severity,flags,message,description) VALUES "
	      "(500, 'SNMP_UNMATCHED_TRAP',	0, 1,	'SNMP trap received: %1 (Parameters: %2)',"
		   "'Generated when system receives an SNMP trap without match in trap "
         "configuration table#0D#0AParameters:#0D#0A   1) SNMP trap OID#0D#0A"
		   "   2) Trap parameters')\n"
      "INSERT INTO events (event_id,name,severity,flags,message,description) VALUES "
	      "(501, 'SNMP_COLD_START', 0, 1, 'System was cold-started',"
		   "'Generated when system receives a coldStart SNMP trap#0D#0AParameters:#0D#0A"
		   "   1) SNMP trap OID')\n"
      "INSERT INTO events (event_id,name,severity,flags,message,description) VALUES "
	      "(502, 'SNMP_WARM_START', 0, 1, 'System was warm-started',"
		   "'Generated when system receives a warmStart SNMP trap#0D#0A"
		   "Parameters:#0D#0A   1) SNMP trap OID')\n"
      "INSERT INTO events (event_id,name,severity,flags,message,description) VALUES "
	      "(503, 'SNMP_LINK_DOWN', 3, 1, 'Link is down',"
		   "'Generated when system receives a linkDown SNMP trap#0D#0A"
		   "Parameters:#0D#0A   1) SNMP trap OID#0D#0A   2) Interface index')\n"
      "INSERT INTO events (event_id,name,severity,flags,message,description) VALUES "
	      "(504, 'SNMP_LINK_UP', 0, 1, 'Link is up',"
		   "'Generated when system receives a linkUp SNMP trap#0D#0AParameters:#0D#0A"
		   "   1) SNMP trap OID#0D#0A   2) Interface index')\n"
      "INSERT INTO events (event_id,name,severity,flags,message,description) VALUES "
	      "(505, 'SNMP_AUTH_FAILURE', 1, 1, 'SNMP authentication failure',"
		   "'Generated when system receives an authenticationFailure SNMP trap#0D#0A"
		   "Parameters:#0D#0A   1) SNMP trap OID')\n"
      "INSERT INTO events (event_id,name,severity,flags,message,description) VALUES "
	      "(506, 'SNMP_EGP_NEIGHBOR_LOSS',	1, 1,	'EGP neighbor loss',"
		   "'Generated when system receives an egpNeighborLoss SNMP trap#0D#0A"
		   "Parameters:#0D#0A   1) SNMP trap OID')\n"
      "INSERT INTO snmp_trap_cfg (trap_id,snmp_oid,event_id,description) "
         "VALUES (1,'.1.3.6.1.6.3.1.1.5.1',501,'Generic coldStart trap')\n"
      "INSERT INTO snmp_trap_cfg (trap_id,snmp_oid,event_id,description) "
         "VALUES (2,'.1.3.6.1.6.3.1.1.5.2',502,'Generic warmStart trap')\n"
      "INSERT INTO snmp_trap_cfg (trap_id,snmp_oid,event_id,description) "
         "VALUES (3,'.1.3.6.1.6.3.1.1.5.3',503,'Generic linkDown trap')\n"
      "INSERT INTO snmp_trap_cfg (trap_id,snmp_oid,event_id,description) "
         "VALUES (4,'.1.3.6.1.6.3.1.1.5.4',504,'Generic linkUp trap')\n"
      "INSERT INTO snmp_trap_cfg (trap_id,snmp_oid,event_id,description) "
         "VALUES (5,'.1.3.6.1.6.3.1.1.5.5',505,'Generic authenticationFailure trap')\n"
      "INSERT INTO snmp_trap_cfg (trap_id,snmp_oid,event_id,description) "
         "VALUES (6,'.1.3.6.1.6.3.1.1.5.6',506,'Generic egpNeighborLoss trap')\n"
      "INSERT INTO snmp_trap_pmap (trap_id,parameter,snmp_oid,description) "
         "VALUES (3,1,'.1.3.6.1.2.1.2.2.1.1','Interface index')\n"
      "INSERT INTO snmp_trap_pmap (trap_id,parameter,snmp_oid,description) "
         "VALUES (4,1,'.1.3.6.1.2.1.2.2.1.1','Interface index')\n"
      "<END>";

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("DBLockStatus"), _T("UNLOCKED"), 0, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("DBLockInfo"), _T(""), 0, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("EnableSNMPTraps"), _T("1"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("SMSDriver"), _T("<none>"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("SMTPServer"), _T("localhost"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("SMTPFromAddr"), _T("netxms@localhost"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='17' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V15 to V16
//

static BOOL H_UpgradeFromV15(void)
{
   static TCHAR m_szBatch[] =
      "INSERT INTO events (event_id,name,severity,flags,message,description) VALUES "
	      "(4005, 'DC_MAILBOX_TOO_LARGE', 1, 1,"
		   "'Mailbox #22%6#22 exceeds size limit (allowed size: %3; actual size: %4)',"
		   "'Custom data collection threshold event.#0D#0AParameters:#0D#0A"
		   "   1) Parameter name#0D#0A   2) Item description#0D#0A"
         "   3) Threshold value#0D#0A   4) Actual value#0D#0A"
         "   5) Data collection item ID#0D#0A   6) Instance')\n"
      "INSERT INTO events (event_id,name,severity,flags,message,description) VALUES "
	      "(4006, 'DC_AGENT_VERSION_CHANGE',	0, 1,"
         "'NetXMS agent version was changed from %3 to %4',"
         "'Custom data collection threshold event.#0D#0AParameters:#0D#0A"
		   "   1) Parameter name#0D#0A   2) Item description#0D#0A"
         "   3) Threshold value#0D#0A   4) Actual value#0D#0A"
         "   5) Data collection item ID#0D#0A   6) Instance')\n"
      "INSERT INTO events (event_id,name,severity,flags,message,description) VALUES "
	      "(4007, 'DC_HOSTNAME_CHANGE',	1, 1,"
         "'Host name was changed from %3 to %4',"
         "'Custom data collection threshold event.#0D#0AParameters:#0D#0A"
		   "   1) Parameter name#0D#0A   2) Item description#0D#0A"
         "   3) Threshold value#0D#0A   4) Actual value#0D#0A"
         "   5) Data collection item ID#0D#0A   6) Instance')\n"
      "INSERT INTO events (event_id,name,severity,flags,message,description) VALUES "
	      "(4008, 'DC_FILE_CHANGE',	1, 1,"
         "'File #22%6#22 was changed',"
         "'Custom data collection threshold event.#0D#0AParameters:#0D#0A"
		   "   1) Parameter name#0D#0A   2) Item description#0D#0A"
         "   3) Threshold value#0D#0A   4) Actual value#0D#0A"
         "   5) Data collection item ID#0D#0A   6) Instance')\n"
      "<END>";

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;
   if (!SQLQuery(_T("UPDATE config SET var_value='16' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;
   return TRUE;
}


//
// Upgrade from V14 to V15
//

static BOOL H_UpgradeFromV14(void)
{
   static TCHAR m_szBatch[] =
      "ALTER TABLE items ADD instance varchar(255)\n"
      "UPDATE items SET instance=''\n"
      "INSERT INTO config (var_name,var_value,is_visible,need_server_restart) VALUES "
         "('SMTPServer','localhost',1,0)\n"
      "INSERT INTO config (var_name,var_value,is_visible,need_server_restart) VALUES "
         "('SMTPFromAddr','netxms@localhost',1,0)\n"
      "INSERT INTO events (event_id,name,severity,flags,message,description) VALUES "
	      "(4003, 'DC_AGENT_RESTARTED',	0, 1,"
         "'NetXMS agent was restarted within last 5 minutes',"
         "'Custom data collection threshold event.#0D#0AParameters:#0D#0A"
		   "   1) Parameter name#0D#0A   2) Item description#0D#0A"
         "   3) Threshold value#0D#0A   4) Actual value#0D#0A"
         "   5) Data collection item ID#0D#0A   6) Instance')\n"
      "INSERT INTO events (event_id,name,severity,flags,message,description) VALUES "
	      "(4004, 'DC_SERVICE_NOT_RUNNING', 3, 1,"
		   "'Service #22%6#22 is not running',"
		   "'Custom data collection threshold event.#0D#0AParameters:#0D#0A"
		   "   1) Parameter name#0D#0A   2) Item description#0D#0A"
         "   3) Threshold value#0D#0A   4) Actual value#0D#0A"
         "   5) Data collection item ID#0D#0A   6) Instance')\n"
      "UPDATE events SET "
         "description='Generated when threshold value reached "
         "for specific data collection item.#0D#0A"
         "   1) Parameter name#0D#0A   2) Item description#0D#0A"
         "   3) Threshold value#0D#0A   4) Actual value#0D#0A"
         "   5) Data collection item ID#0D#0A   6) Instance' WHERE "
         "event_id=17 OR (event_id>=4000 AND event_id<5000)\n"
      "UPDATE events SET "
         "description='Generated when threshold check is rearmed "
         "for specific data collection item.#0D#0A"
		   "Parameters:#0D#0A   1) Parameter name#0D#0A"
		   "   2) Item description#0D#0A   3) Data collection item ID' "
         "WHERE event_id=18\n"
      "<END>";

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;
   if (!SQLQuery(_T("UPDATE config SET var_value='15' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;
   return TRUE;
}


//
// Upgrade map
//

static struct
{
   int iVersion;
   BOOL (* fpProc)(void);
} m_dbUpgradeMap[] =
{
   { 14, H_UpgradeFromV14 },
   { 15, H_UpgradeFromV15 },
   { 16, H_UpgradeFromV16 },
   { 17, H_UpgradeFromV17 },
   { 18, H_UpgradeFromV18 },
   { 19, H_UpgradeFromV19 },
   { 20, H_UpgradeFromV20 },
   { 21, H_UpgradeFromV21 },
   { 22, H_UpgradeFromV22 },
   { 23, H_UpgradeFromV23 },
   { 24, H_UpgradeFromV24 },
   { 25, H_UpgradeFromV25 },
   { 26, H_UpgradeFromV26 },
   { 27, H_UpgradeFromV27 },
   { 28, H_UpgradeFromV28 },
   { 29, H_UpgradeFromV29 },
   { 30, H_UpgradeFromV30 },
   { 31, H_UpgradeFromV31 },
   { 32, H_UpgradeFromV32 },
   { 33, H_UpgradeFromV33 },
   { 34, H_UpgradeFromV34 },
   { 35, H_UpgradeFromV35 },
   { 36, H_UpgradeFromV36 },
   { 37, H_UpgradeFromV37 },
   { 38, H_UpgradeFromV38 },
   { 39, H_UpgradeFromV39 },
   { 40, H_UpgradeFromV40 },
   { 41, H_UpgradeFromV41 },
   { 42, H_UpgradeFromV42 },
   { 43, H_UpgradeFromV43 },
   { 44, H_UpgradeFromV44 },
   { 45, H_UpgradeFromV45 },
   { 46, H_UpgradeFromV46 },
   { 47, H_UpgradeFromV47 },
   { 48, H_UpgradeFromV48 },
   { 49, H_UpgradeFromV49 },
   { 50, H_UpgradeFromV50 },
   { 51, H_UpgradeFromV51 },
   { 52, H_UpgradeFromV52 },
   { 53, H_UpgradeFromV53 },
   { 0, NULL }
};


//
// Upgrade database to new version
//

void UpgradeDatabase(void)
{
   DB_RESULT hResult;
   LONG i, iVersion = 0;
   BOOL bLocked = FALSE;
   TCHAR szTemp[MAX_DB_STRING];

   _tprintf(_T("Upgrading database...\n"));

   // Get database format version
   hResult = DBSelect(g_hCoreDB, _T("SELECT var_value FROM config WHERE var_name='DBFormatVersion'"));
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         iVersion = DBGetFieldLong(hResult, 0, 0);
      DBFreeResult(hResult);
   }
   if (iVersion == DB_FORMAT_VERSION)
   {
      _tprintf(_T("Your database format is up to date\n"));
   }
   else if (iVersion > DB_FORMAT_VERSION)
   {
       _tprintf(_T("Your database has format version %d, this tool is compiled for version %d.\n"
                   "You need to upgrade your server before using this database.\n"),
                iVersion, DB_FORMAT_VERSION);
   }
   else
   {
      // Check if database is locked
      hResult = DBSelect(g_hCoreDB, _T("SELECT var_value FROM config WHERE var_name='DBLockStatus'"));
      if (hResult != NULL)
      {
         if (DBGetNumRows(hResult) > 0)
            bLocked = _tcscmp(DBGetField(hResult, 0, 0, szTemp, MAX_DB_STRING), _T("UNLOCKED"));
         DBFreeResult(hResult);
      }
      if (!bLocked)
      {
         // Upgrade database
         while(iVersion < DB_FORMAT_VERSION)
         {
            // Find upgrade procedure
            for(i = 0; m_dbUpgradeMap[i].fpProc != NULL; i++)
               if (m_dbUpgradeMap[i].iVersion == iVersion)
                  break;
            if (m_dbUpgradeMap[i].fpProc == NULL)
            {
               _tprintf(_T("Unable to find upgrade procedure for version %d\n"), iVersion);
               break;
            }
            printf("Upgrading from version %d to %d\n", iVersion, iVersion + 1);
            DBBegin(g_hCoreDB);
            if (m_dbUpgradeMap[i].fpProc())
            {
               DBCommit(g_hCoreDB);
               iVersion++;
            }
            else
            {
               printf("Rolling back last stage due to upgrade errors...\n");
               DBRollback(g_hCoreDB);
               break;
            }
         }

         _tprintf(_T("Database upgrade %s\n"), (iVersion == DB_FORMAT_VERSION) ? _T("succeeded") : _T("failed"));
      }
      else
      {
         _tprintf(_T("Database is locked\n"));
      }
   }
}
