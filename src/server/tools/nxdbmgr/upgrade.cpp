/* 
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2013 Victor Kirhenshtein
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

/**
 * Externals
 */
BOOL MigrateMaps();

/**
 * Create table
 */
static BOOL CreateTable(const TCHAR *pszQuery)
{
   BOOL bResult;
	String query(pszQuery);

   query.replace(_T("$SQL:TEXT"), g_pszSqlType[g_iSyntax][SQL_TYPE_TEXT]);
   query.replace(_T("$SQL:TXT4K"), g_pszSqlType[g_iSyntax][SQL_TYPE_TEXT4K]);
   query.replace(_T("$SQL:INT64"), g_pszSqlType[g_iSyntax][SQL_TYPE_INT64]);
   if (g_iSyntax == DB_SYNTAX_MYSQL)
      query += g_pszTableSuffix;
   bResult = SQLQuery(query);
   return bResult;
}

/**
 * Create configuration parameter if it doesn't exist (unless bForceUpdate set to true)
 */
BOOL CreateConfigParam(const TCHAR *pszName, const TCHAR *pszValue,
                       int iVisible, int iNeedRestart, BOOL bForceUpdate)
{
   TCHAR szQuery[1024];
   DB_RESULT hResult;
   BOOL bVarExist = FALSE, bResult = TRUE;

   // Check for variable existence
   _sntprintf(szQuery, 1024, _T("SELECT var_value FROM config WHERE var_name='%s'"), pszName);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != 0)
   {
      if (DBGetNumRows(hResult) > 0)
         bVarExist = TRUE;
      DBFreeResult(hResult);
   }

   if (!bVarExist)
   {
      _sntprintf(szQuery, 1024, _T("INSERT INTO config (var_name,var_value,is_visible,")
                                _T("need_server_restart) VALUES (%s,%s,%d,%d)"), 
                 (const TCHAR *)DBPrepareString(g_hCoreDB, pszName, 63), 
					  (const TCHAR *)DBPrepareString(g_hCoreDB, pszValue, 255), iVisible, iNeedRestart);
      bResult = SQLQuery(szQuery);
   }
	else if (bForceUpdate)
	{
      _sntprintf(szQuery, 1024, _T("UPDATE config SET var_value=%s WHERE var_name=%s"),
                 (const TCHAR *)DBPrepareString(g_hCoreDB, pszValue, 255), (const TCHAR *)DBPrepareString(g_hCoreDB, pszName, 63));
      bResult = SQLQuery(szQuery);
	}
   return bResult;
}

/**
 * Set primary key constraint
 */
static BOOL SetPrimaryKey(const TCHAR *table, const TCHAR *key)
{
	TCHAR query[4096];

	if (g_iSyntax == DB_SYNTAX_SQLITE)
		return TRUE;	// SQLite does not support adding constraints
			
	_sntprintf(query, 4096, _T("ALTER TABLE %s ADD PRIMARY KEY (%s)"), table, key);
	return SQLQuery(query);
}

/**
 * Drop primary key from table
 */
static BOOL DropPrimaryKey(const TCHAR *table)
{
	TCHAR query[1024];
	DB_RESULT hResult;
	BOOL success;

	switch(g_iSyntax)
	{
		case DB_SYNTAX_ORACLE:
		case DB_SYNTAX_MYSQL:
			_sntprintf(query, 1024, _T("ALTER TABLE %s DROP PRIMARY KEY"), table);
			success = SQLQuery(query);
			break;
		case DB_SYNTAX_PGSQL:
			_sntprintf(query, 1024, _T("ALTER TABLE %s DROP CONSTRAINT %s_pkey"), table, table);
			success = SQLQuery(query);
			break;
		case DB_SYNTAX_MSSQL:
			success = FALSE;
			_sntprintf(query, 1024, _T("SELECT name FROM sysobjects WHERE xtype='PK' AND parent_obj=OBJECT_ID('%s')"), table);
			hResult = SQLSelect(query);
			if (hResult != NULL)
			{
				if (DBGetNumRows(hResult) > 0)
				{
					TCHAR objName[512];

					DBGetField(hResult, 0, 0, objName, 512);
					_sntprintf(query, 1024, _T("ALTER TABLE %s DROP CONSTRAINT %s"), table, objName);
					success = SQLQuery(query);
				}
				DBFreeResult(hResult);
			}
			break;
		default:		// Unsupported DB engine
			success = FALSE;
			break;
	}
	return success;
}

/**
 * Convert strings from # encoded form to normal form
 */
static BOOL ConvertStrings(const TCHAR *table, const TCHAR *idColumn, const TCHAR *idColumn2, const TCHAR *column, bool isStringId)
{
	DB_RESULT hResult;
	TCHAR *query;
	int queryLen = 512;
	BOOL success = FALSE;

	query = (TCHAR *)malloc(queryLen * sizeof(TCHAR));

	switch(g_iSyntax)
	{
		case DB_SYNTAX_MSSQL:
			_sntprintf(query, queryLen, _T("UPDATE %s SET %s='' WHERE CAST(%s AS nvarchar(4000))=N'#00'"), table, column, column);
			break;
		case DB_SYNTAX_ORACLE:
			_sntprintf(query, queryLen, _T("UPDATE %s SET %s='' WHERE to_char(%s)='#00'"), table, column, column);
			break;
		default:
			_sntprintf(query, queryLen, _T("UPDATE %s SET %s='' WHERE %s='#00'"), table, column, column);
			break;
	}
	if (!SQLQuery(query))
	{
		free(query);
		return FALSE;
	}

	_sntprintf(query, queryLen, _T("SELECT %s,%s%s%s FROM %s WHERE %s LIKE '%%#%%'"), 
	           idColumn, column, (idColumn2 != NULL) ? _T(",") : _T(""), (idColumn2 != NULL) ? idColumn2 : _T(""), table, column);
	hResult = SQLSelect(query);
	if (hResult == NULL)
	{
		free(query);
		return FALSE;
	}

	int count = DBGetNumRows(hResult);
	for(int i = 0; i < count; i++)
	{
		TCHAR *value = DBGetField(hResult, i, 1, NULL, 0);
		if (_tcschr(value, _T('#')) != NULL)
		{
			DecodeSQLString(value);
			String newValue = DBPrepareString(g_hCoreDB, value);
			if ((int)newValue.getSize() + 256 > queryLen)
			{
				queryLen = newValue.getSize() + 256;
				query = (TCHAR *)realloc(query, queryLen * sizeof(TCHAR));
			}
			if (isStringId)
			{
				TCHAR *id = DBGetField(hResult, i, 0, NULL, 0);
				if (idColumn2 != NULL)
				{
					TCHAR *id2 = DBGetField(hResult, i, 2, NULL, 0);
					_sntprintf(query, queryLen, _T("UPDATE %s SET %s=%s WHERE %s=%s AND %s=%s"),
								  table, column, (const TCHAR *)newValue,
								  idColumn, (const TCHAR *)DBPrepareString(g_hCoreDB, id), 
								  idColumn2, (const TCHAR *)DBPrepareString(g_hCoreDB, id2));
				}
				else
				{
					_sntprintf(query, queryLen, _T("UPDATE %s SET %s=%s WHERE %s=%s"), table, column,
								  (const TCHAR *)newValue, idColumn, (const TCHAR *)DBPrepareString(g_hCoreDB, id));
				}
				free(id);
			}
			else
			{
				INT64 id = DBGetFieldInt64(hResult, i, 0);
				if (idColumn2 != NULL)
				{
					INT64 id2 = DBGetFieldInt64(hResult, i, 2);
					_sntprintf(query, queryLen, _T("UPDATE %s SET %s=%s WHERE %s=") INT64_FMT _T(" AND %s=") INT64_FMT, 
								  table, column, (const TCHAR *)newValue, idColumn, id, idColumn2, id2);
				}
				else
				{
					_sntprintf(query, queryLen, _T("UPDATE %s SET %s=%s WHERE %s=") INT64_FMT, table, column,
								  (const TCHAR *)newValue, idColumn, id);
				}
			}
			if (!SQLQuery(query))
				goto cleanup;
		}
	}
	success = TRUE;

cleanup:
	DBFreeResult(hResult);
	free(query);
	return success;
}

static BOOL ConvertStrings(const TCHAR *table, const TCHAR *idColumn, const TCHAR *column)
{
	return ConvertStrings(table, idColumn, NULL, column, false);
}

/**
 * Set column nullable (currently only Oracle and PostgreSQL)
 */
static BOOL SetColumnNullable(const TCHAR *table, const TCHAR *column)
{
	TCHAR query[1024] = _T("");

	switch(g_iSyntax)
	{
		case DB_SYNTAX_ORACLE:
			_sntprintf(query, 1024, _T("DECLARE already_null EXCEPTION; ")
			                        _T("PRAGMA EXCEPTION_INIT(already_null, -1451); ")
											_T("BEGIN EXECUTE IMMEDIATE 'ALTER TABLE %s MODIFY %s null'; ")
											_T("EXCEPTION WHEN already_null THEN null; END;"), table, column);
			break;
		case DB_SYNTAX_PGSQL:
			_sntprintf(query, 1024, _T("ALTER TABLE %s ALTER COLUMN %s DROP NOT NULL"), table, column);
			break;
		default:
			break;
	}

	return (query[0] != 0) ? SQLQuery(query) : TRUE;
}

/**
 * Resize varchar column
 */
static BOOL ResizeColumn(const TCHAR *table, const TCHAR *column, int newSize)
{
	TCHAR query[1024];

	switch(g_iSyntax)
	{
		case DB_SYNTAX_DB2:
			_sntprintf(query, 1024, _T("ALTER TABLE %s ALTER COLUMN %s SET DATA TYPE varchar(%d)"), table, column, newSize);
			break;
		case DB_SYNTAX_MSSQL:
			_sntprintf(query, 1024, _T("ALTER TABLE %s ALTER COLUMN %s varchar(%d)"), table, column, newSize);
			break;
		case DB_SYNTAX_PGSQL:
			_sntprintf(query, 1024, _T("ALTER TABLE %s ALTER COLUMN %s TYPE varchar(%d)"), table, column, newSize);
			break;
		case DB_SYNTAX_SQLITE:
			/* TODO: add SQLite support */
			query[0] = 0;
			break;
		default:
			_sntprintf(query, 1024, _T("ALTER TABLE %s MODIFY %s varchar(%d)"), table, column, newSize);
			break;
	}

	return (query[0] != 0) ? SQLQuery(query) : TRUE;
}

/**
 * Create new event template
 */
static BOOL CreateEventTemplate(int code, const TCHAR *name, int severity, int flags, const TCHAR *message, const TCHAR *description)
{
	TCHAR query[4096];

	_sntprintf(query, 4096, _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) VALUES (%d,'%s',%d,%d,%s,%s)"),
	           code, name, severity, flags, (const TCHAR *)DBPrepareString(g_hCoreDB, message),
				  (const TCHAR *)DBPrepareString(g_hCoreDB, description));
	return SQLQuery(query);
}

/**
 * Upgrade from V274 to V275
 */
static BOOL H_UpgradeFromV274(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("ALTER TABLE nodes ADD rack_image varchar(36)\n")
      _T("ALTER TABLE nodes ADD rack_position integer\n")
      _T("ALTER TABLE nodes ADD rack_id integer\n")
      _T("UPDATE nodes SET rack_image='00000000-0000-0000-0000-000000000000',rack_position=0,rack_id=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(CreateTable(_T("CREATE TABLE access_points (")
	                     _T("id integer not null,")
								_T("node_id integer not null,")
	                     _T("mac_address varchar(12) null,")
	                     _T("vendor varchar(64) null,")
	                     _T("model varchar(128) null,")
	                     _T("serial_number varchar(64) null,")
	                     _T("PRIMARY KEY(id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE racks (")
	                     _T("id integer not null,")
								_T("height integer not null,")
	                     _T("PRIMARY KEY(id))")));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='275' WHERE var_name='SchemaVersion'")));
   return TRUE;
}

/**
 * Upgrade from V273 to V274
 */
static BOOL H_UpgradeFromV273(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("ALTER TABLE items ADD samples integer\n")
      _T("UPDATE items SET samples=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='274' WHERE var_name='SchemaVersion'")));
   return TRUE;
}

/**
 * Upgrade from V272 to V273
 */
static BOOL H_UpgradeFromV272(int currVersion, int newVersion)
{
	CHK_EXEC(CreateConfigParam(_T("DefaultDCIRetentionTime"), _T("30"), 1, 0));
	CHK_EXEC(CreateConfigParam(_T("DefaultDCIPollingInterval"), _T("60"), 1, 0));
   CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='273' WHERE var_name='SchemaVersion'")));
   return TRUE;
}

/**
 * Upgrade from V271 to V272
 */
static BOOL H_UpgradeFromV271(int currVersion, int newVersion)
{
	CHK_EXEC(CreateConfigParam(_T("SNMPTrapLogRetentionTime"), _T("90"), 1, 0));
   CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD driver_name varchar(32)\n")));
   CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='272' WHERE var_name='SchemaVersion'")));
   return TRUE;
}

/**
 * Upgrade from V270 to V271
 */
static BOOL H_UpgradeFromV270(int currVersion, int newVersion)
{
   static TCHAR batch[] =
      _T("ALTER TABLE object_properties ADD location_accuracy integer\n")
      _T("ALTER TABLE object_properties ADD location_timestamp integer\n")
      _T("UPDATE object_properties SET location_accuracy=0,location_timestamp=0\n")
      _T("<END>");
   CHK_EXEC(SQLBatch(batch));

   CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='271' WHERE var_name='SchemaVersion'")));
   return TRUE;
}

/**
 * Upgrade from V269 to V270
 */
static BOOL H_UpgradeFromV269(int currVersion, int newVersion)
{
	static TCHAR batch[] = 
		_T("ALTER TABLE items ADD instd_method integer\n")
		_T("ALTER TABLE items ADD instd_data varchar(255)\n")
		_T("ALTER TABLE items ADD instd_filter $SQL:TEXT\n")
		_T("UPDATE items SET instd_method=0\n")
		_T("<END>");
	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='270' WHERE var_name='SchemaVersion'")));
	return TRUE;
}

/**
 * Upgrade from V268 to V269
 */
static BOOL H_UpgradeFromV268(int currVersion, int newVersion)
{
	CHK_EXEC(ResizeColumn(_T("alarms"), _T("message"), 2000));
	CHK_EXEC(ResizeColumn(_T("alarm_events"), _T("message"), 2000));
	CHK_EXEC(ResizeColumn(_T("event_log"), _T("event_message"), 2000));
	CHK_EXEC(ResizeColumn(_T("event_cfg"), _T("message"), 2000));
	CHK_EXEC(ResizeColumn(_T("event_policy"), _T("alarm_message"), 2000));
	CHK_EXEC(ResizeColumn(_T("items"), _T("name"), 1024));
	CHK_EXEC(ResizeColumn(_T("dc_tables"), _T("name"), 1024));

	CHK_EXEC(SetColumnNullable(_T("event_policy"), _T("alarm_key")));
	CHK_EXEC(SetColumnNullable(_T("event_policy"), _T("alarm_message")));
	CHK_EXEC(SetColumnNullable(_T("event_policy"), _T("comments")));
	CHK_EXEC(SetColumnNullable(_T("event_policy"), _T("situation_instance")));
	CHK_EXEC(SetColumnNullable(_T("event_policy"), _T("script")));
	CHK_EXEC(ConvertStrings(_T("event_policy"), _T("rule_id"), _T("alarm_key")));
	CHK_EXEC(ConvertStrings(_T("event_policy"), _T("rule_id"), _T("alarm_message")));
	CHK_EXEC(ConvertStrings(_T("event_policy"), _T("rule_id"), _T("comments")));
	CHK_EXEC(ConvertStrings(_T("event_policy"), _T("rule_id"), _T("situation_instance")));
	CHK_EXEC(ConvertStrings(_T("event_policy"), _T("rule_id"), _T("script")));

	CHK_EXEC(SetColumnNullable(_T("policy_situation_attr_list"), _T("attr_value")));
	// convert strings in policy_situation_attr_list
	DB_RESULT hResult = SQLSelect(_T("SELECT rule_id,situation_id,attr_name,attr_value FROM policy_situation_attr_list"));
	if (hResult != NULL)
	{
		if (SQLQuery(_T("DELETE FROM policy_situation_attr_list")))
		{
			TCHAR name[MAX_DB_STRING], value[MAX_DB_STRING], query[1024];
			int count = DBGetNumRows(hResult);
			for(int i = 0; i < count; i++)
			{
				LONG ruleId = DBGetFieldLong(hResult, i, 0);
				LONG situationId = DBGetFieldLong(hResult, i, 1);
				DBGetField(hResult, i, 2, name, MAX_DB_STRING);
				DBGetField(hResult, i, 3, value, MAX_DB_STRING);

				DecodeSQLString(name);
				DecodeSQLString(value);

				if (name[0] == 0)
					_tcscpy(name, _T("noname"));

				_sntprintf(query, 1024, _T("INSERT INTO policy_situation_attr_list (rule_id,situation_id,attr_name,attr_value) VALUES (%d,%d,%s,%s)"),
				           ruleId, situationId, (const TCHAR *)DBPrepareString(g_hCoreDB, name), (const TCHAR *)DBPrepareString(g_hCoreDB, value));
				if (!SQLQuery(query))
				{
					if (!g_bIgnoreErrors)
					{
						DBFreeResult(hResult);
						return FALSE;
					}
				}
			}
		}
		else
		{
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

	CHK_EXEC(SetColumnNullable(_T("event_cfg"), _T("description")));
	CHK_EXEC(SetColumnNullable(_T("event_cfg"), _T("message")));
	CHK_EXEC(ConvertStrings(_T("event_cfg"), _T("event_code"), _T("description")));
	CHK_EXEC(ConvertStrings(_T("event_cfg"), _T("event_code"), _T("message")));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='269' WHERE var_name='SchemaVersion'")));
	return TRUE;
}

/**
 * Upgrade from V267 to V268
 */
static BOOL H_UpgradeFromV267(int currVersion, int newVersion)
{
	CHK_EXEC(SetColumnNullable(_T("network_services"), _T("check_request")));
	CHK_EXEC(SetColumnNullable(_T("network_services"), _T("check_responce")));
	CHK_EXEC(ConvertStrings(_T("network_services"), _T("id"), _T("check_request")));
	CHK_EXEC(ConvertStrings(_T("network_services"), _T("id"), _T("check_responce")));

	CHK_EXEC(SetColumnNullable(_T("config"), _T("var_value")));
	CHK_EXEC(ConvertStrings(_T("config"), _T("var_name"), NULL, _T("var_value"), true));

	CHK_EXEC(SetColumnNullable(_T("config"), _T("var_value")));
	CHK_EXEC(ConvertStrings(_T("config"), _T("var_name"), NULL, _T("var_value"), true));

	CHK_EXEC(SetColumnNullable(_T("dci_schedules"), _T("schedule")));
	CHK_EXEC(ConvertStrings(_T("dci_schedules"), _T("schedule_id"), _T("item_id"), _T("schedule"), false));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='268' WHERE var_name='SchemaVersion'")));
	return TRUE;
}

/**
 * Upgrade from V266 to V267
 */
static BOOL H_UpgradeFromV266(int currVersion, int newVersion)
{
	CHK_EXEC(CreateEventTemplate(EVENT_NODE_UNREACHABLE, _T("SYS_NODE_UNREACHABLE"), EVENT_SEVERITY_CRITICAL,
	                             EF_LOG, _T("Node unreachable because of network failure"),
										  _T("Generated when node is unreachable by management server because of network failure.\r\nParameters:\r\n   No event-specific parameters")));
	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='267' WHERE var_name='SchemaVersion'")));
	return TRUE;
}

/**
 * Upgrade from V265 to V266
 */
static BOOL H_UpgradeFromV265(int currVersion, int newVersion)
{
	// create index on root event ID in event log
	switch(g_iSyntax)
	{
		case DB_SYNTAX_MSSQL:
		case DB_SYNTAX_PGSQL:
			CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_event_log_root_id ON event_log(root_event_id) WHERE root_event_id > 0")));
			break;
		case DB_SYNTAX_ORACLE:
			CHK_EXEC(SQLQuery(_T("CREATE OR REPLACE FUNCTION zero_to_null(id NUMBER) ")
			                  _T("RETURN NUMBER ")
			                  _T("DETERMINISTIC ")
			                  _T("AS BEGIN")
			                  _T("   IF id > 0 THEN")
			                  _T("      RETURN id;")
			                  _T("   ELSE")
			                  _T("      RETURN NULL;")
			                  _T("   END IF;")
			                  _T("END;")));
			CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_event_log_root_id ON event_log(zero_to_null(root_event_id))")));
			break;
		default:
			CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_event_log_root_id ON event_log(root_event_id)")));
			break;
	}

	CHK_EXEC(CreateTable(_T("CREATE TABLE mapping_tables (")
	                     _T("id integer not null,")
	                     _T("name varchar(63) not null,")
	                     _T("flags integer not null,")
								_T("description $SQL:TXT4K null,")
	                     _T("PRIMARY KEY(id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE mapping_data (")
	                     _T("table_id integer not null,")
	                     _T("md_key varchar(63) not null,")
	                     _T("md_value varchar(255) null,")
	                     _T("description $SQL:TXT4K null,")
	                     _T("PRIMARY KEY(table_id,md_key))")));

	CHK_EXEC(SQLQuery(_T("DROP TABLE deleted_objects")));

	CHK_EXEC(CreateConfigParam(_T("FirstFreeObjectId"), _T("100"), 0, 1, FALSE));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='266' WHERE var_name='SchemaVersion'")));
	return TRUE;
}

/**
 * Upgrade from V264 to V265
 */
static BOOL H_UpgradeFromV264(int currVersion, int newVersion)
{
	CHK_EXEC(CreateTable(_T("CREATE TABLE alarm_events (")
	                     _T("alarm_id integer not null,")
								_T("event_id $SQL:INT64 not null,")
	                     _T("event_code integer not null,")
	                     _T("event_name varchar(63) null,")
	                     _T("severity integer not null,")
	                     _T("source_object_id integer not null,")
	                     _T("event_timestamp integer not null,")
	                     _T("message varchar(255) null,")
	                     _T("PRIMARY KEY(alarm_id,event_id))")));
	CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_alarm_events_alarm_id ON alarm_events(alarm_id)")));
	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='265' WHERE var_name='SchemaVersion'")));
	return TRUE;
}

/**
 * Upgrade from V263 to V264
 */
static BOOL H_UpgradeFromV263(int currVersion, int newVersion)
{
	CHK_EXEC(CreateTable(_T("CREATE TABLE mobile_devices (")
	                     _T("id integer not null,")
								_T("device_id varchar(64) not null,")
								_T("vendor varchar(64) null,")
								_T("model varchar(128) null,")
								_T("serial_number varchar(64) null,")
								_T("os_name varchar(32) null,")
								_T("os_version varchar(64) null,")
								_T("user_id varchar(64) null,")
								_T("battery_level integer not null,")
	                     _T("PRIMARY KEY(id))")));
	CHK_EXEC(CreateConfigParam(_T("MobileDeviceListenerPort"), _T("4747"), 1, 1));
	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='264' WHERE var_name='SchemaVersion'")));
	return TRUE;
}

/**
 * Upgrade from V262 to V263
 */
static BOOL H_UpgradeFromV262(int currVersion, int newVersion)
{
	CHK_EXEC(SQLQuery(_T("ALTER TABLE network_maps ADD radius integer")));
	CHK_EXEC(SQLQuery(_T("UPDATE network_maps SET radius=-1")));
	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='263' WHERE var_name='SchemaVersion'")));
	return TRUE;
}

/**
 * Upgrade from V261 to V262
 */
static BOOL H_UpgradeFromV261(int currVersion, int newVersion)
{
	CHK_EXEC(CreateConfigParam(_T("ApplyDCIFromTemplateToDisabledDCI"), _T("0"), 1, 1));
	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='262' WHERE var_name='SchemaVersion'")));
	return TRUE;
}

/**
 * Upgrade from V260 to V261
 */
static BOOL H_UpgradeFromV260(int currVersion, int newVersion)
{
	CHK_EXEC(CreateConfigParam(_T("NumberOfBusinessServicePollers"), _T("10"), 1, 1));
	CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='NumberOfEventProcessors'")));
	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='261' WHERE var_name='SchemaVersion'")));
	return TRUE;
}

/**
 * Upgrade from V259 to V260
 */
static BOOL H_UpgradeFromV259(int currVersion, int newVersion)
{
	CHK_EXEC(CreateConfigParam(_T("UseFQDNForNodeNames"), _T("1"), 1, 1));
	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='260' WHERE var_name='SchemaVersion'")));
	return TRUE;
}

/**
 * Upgrade from V258 to V259
 */
static BOOL H_UpgradeFromV258(int currVersion, int newVersion)
{
	// have to made these columns nullable again because
	// because they was forgotten as NOT NULL in schema.in
	// and so some databases can still have them as NOT NULL
	CHK_EXEC(SetColumnNullable(_T("templates"), _T("apply_filter")));
	CHK_EXEC(SetColumnNullable(_T("containers"), _T("auto_bind_filter")));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='259' WHERE var_name='SchemaVersion'")));
	return TRUE;
}

/**
 * Upgrade from V257 to V258
 */
static BOOL H_UpgradeFromV257(int currVersion, int newVersion)
{
	static TCHAR batch[] = 
		_T("ALTER TABLE nodes ADD down_since integer\n")
		_T("UPDATE nodes SET down_since=0\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(CreateConfigParam(_T("DeleteUnreachableNodesPeriod"), _T("0"), 1, 1));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='258' WHERE var_name='SchemaVersion'")));
	return TRUE;
}

/**
 * Upgrade from V256 to V257
 */
static BOOL H_UpgradeFromV256(int currVersion, int newVersion)
{
	static TCHAR batch[] = 
		_T("ALTER TABLE network_maps ADD bg_color integer\n")
		_T("ALTER TABLE network_maps ADD link_routing integer\n")
		_T("UPDATE network_maps SET bg_color=16777215,link_routing=1\n")
		_T("ALTER TABLE network_map_links ADD routing integer\n")
		_T("ALTER TABLE network_map_links ADD bend_points $SQL:TXT4K\n")
		_T("UPDATE network_map_links SET routing=0\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));
	
	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='257' WHERE var_name='SchemaVersion'")));
   return TRUE;
}

/**
 * Upgrade from V255 to V256
 */
static BOOL H_UpgradeFromV255(int currVersion, int newVersion)
{
	CHK_EXEC(CreateConfigParam(_T("DefaultConsoleDateFormat"), _T("dd.MM.yyyy"), 1, 0));
	CHK_EXEC(CreateConfigParam(_T("DefaultConsoleTimeFormat"), _T("HH:mm:ss"), 1, 0));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='256' WHERE var_name='SchemaVersion'")));
   return TRUE;
}

/**
 * Upgrade from V254 to V255
 */
static BOOL H_UpgradeFromV254(int currVersion, int newVersion)
{
	static TCHAR batch[] = 
		_T("ALTER TABLE alarms ADD resolved_by integer\n")
		_T("UPDATE alarms SET resolved_by=0\n")
		_T("UPDATE alarms SET alarm_state=3 WHERE alarm_state=2\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='255' WHERE var_name='SchemaVersion'")));
   return TRUE;
}


//
// Upgrade from V253 to V254
//

static BOOL H_UpgradeFromV253(int currVersion, int newVersion)
{
	static TCHAR batch[] = 
		_T("ALTER TABLE network_maps ADD flags integer\n")
		_T("ALTER TABLE network_maps ADD link_color integer\n")
		_T("UPDATE network_maps SET flags=1,link_color=-1\n")
		_T("ALTER TABLE network_map_links ADD color integer\n")
		_T("ALTER TABLE network_map_links ADD status_object integer\n")
		_T("UPDATE network_map_links SET color=-1,status_object=0\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='254' WHERE var_name='SchemaVersion'")));
   return TRUE;
}

/**
 * Upgrade from V252 to V253
 */
static BOOL H_UpgradeFromV252(int currVersion, int newVersion)
{
	CHK_EXEC(SetColumnNullable(_T("templates"), _T("apply_filter")));
	CHK_EXEC(ConvertStrings(_T("templates"), _T("id"), _T("apply_filter")));

	CHK_EXEC(SetColumnNullable(_T("containers"), _T("auto_bind_filter")));
	CHK_EXEC(ConvertStrings(_T("containers"), _T("id"), _T("auto_bind_filter")));

	static TCHAR batch[] = 
		_T("ALTER TABLE templates ADD flags integer\n")
		_T("UPDATE templates SET flags=0 WHERE enable_auto_apply=0\n")
		_T("UPDATE templates SET flags=3 WHERE enable_auto_apply<>0\n")
		_T("ALTER TABLE templates DROP COLUMN enable_auto_apply\n")
		_T("ALTER TABLE containers ADD flags integer\n")
		_T("UPDATE containers SET flags=0 WHERE enable_auto_bind=0\n")
		_T("UPDATE containers SET flags=3 WHERE enable_auto_bind<>0\n")
		_T("ALTER TABLE containers DROP COLUMN enable_auto_bind\n")
		_T("<END>");
	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(CreateEventTemplate(EVENT_CONTAINER_AUTOBIND, _T("SYS_CONTAINER_AUTOBIND"), EVENT_SEVERITY_NORMAL, 1,
	                             _T("Node %2 automatically bound to container %4"), 
										  _T("Generated when node bound to container object by autobind rule.\r\n")
	                             _T("Parameters:#\r\n")
										  _T("   1) Node ID\r\n")
										  _T("   2) Node name\r\n")
										  _T("   3) Container ID\r\n")
										  _T("   4) Container name")
										  ));

	CHK_EXEC(CreateEventTemplate(EVENT_CONTAINER_AUTOUNBIND, _T("SYS_CONTAINER_AUTOUNBIND"), EVENT_SEVERITY_NORMAL, 1,
	                             _T("Node %2 automatically unbound from container %4"), 
										  _T("Generated when node unbound from container object by autobind rule.\r\n")
	                             _T("Parameters:#\r\n")
										  _T("   1) Node ID\r\n")
										  _T("   2) Node name\r\n")
										  _T("   3) Container ID\r\n")
										  _T("   4) Container name")
										  ));

	CHK_EXEC(CreateEventTemplate(EVENT_TEMPLATE_AUTOAPPLY, _T("SYS_TEMPLATE_AUTOAPPLY"), EVENT_SEVERITY_NORMAL, 1,
	                             _T("Template %4 automatically applied to node %2"), 
										  _T("Generated when template applied to node by autoapply rule.\r\n")
	                             _T("Parameters:#\r\n")
										  _T("   1) Node ID\r\n")
										  _T("   2) Node name\r\n")
										  _T("   3) Template ID\r\n")
										  _T("   4) Template name")
										  ));

	CHK_EXEC(CreateEventTemplate(EVENT_TEMPLATE_AUTOREMOVE, _T("SYS_TEMPLATE_AUTOREMOVE"), EVENT_SEVERITY_NORMAL, 1,
	                             _T("Template %4 automatically removed from node %2"), 
										  _T("Generated when template removed from node by autoapply rule.\r\n")
	                             _T("Parameters:#\r\n")
										  _T("   1) Node ID\r\n")
										  _T("   2) Node name\r\n")
										  _T("   3) Template ID\r\n")
										  _T("   4) Template name")
										  ));

	TCHAR buffer[64];
	_sntprintf(buffer, 64, _T("%d"), ConfigReadInt(_T("AllowedCiphers"), 15) + 16);
	CreateConfigParam(_T("AllowedCiphers"), buffer, 1, 1, TRUE);

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='253' WHERE var_name='SchemaVersion'")));
   return TRUE;
}


//
// Upgrade from V251 to V252
//

static BOOL H_UpgradeFromV251(int currVersion, int newVersion)
{
	static TCHAR batch[] = 
		_T("ALTER TABLE interfaces ADD admin_state integer\n")
		_T("ALTER TABLE interfaces ADD oper_state integer\n")
		_T("UPDATE interfaces SET admin_state=0,oper_state=0\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(CreateEventTemplate(EVENT_INTERFACE_UNEXPECTED_UP, _T("SYS_IF_UNEXPECTED_UP"), EVENT_SEVERITY_MAJOR, 1,
	                             _T("Interface \"%2\" unexpectedly changed state to UP (IP Addr: %3/%4, IfIndex: %5)"), 
										  _T("Generated when interface goes up but it's expected state set to DOWN.\r\n")
										  _T("Please note that source of event is node, not an interface itself.\r\n")
	                             _T("Parameters:#\r\n")
										  _T("   1) Interface object ID\r\n")
										  _T("   2) Interface name\r\n")
										  _T("   3) Interface IP address\r\n")
										  _T("   4) Interface netmask\r\n")
										  _T("   5) Interface index")
										  ));

	CHK_EXEC(CreateEventTemplate(EVENT_INTERFACE_EXPECTED_DOWN, _T("SYS_IF_EXPECTED_DOWN"), EVENT_SEVERITY_NORMAL, 1,
	                             _T("Interface \"%2\" with expected state DOWN changed state to DOWN (IP Addr: %3/%4, IfIndex: %5)"), 
										  _T("Generated when interface goes down and it's expected state is DOWN.\r\n")
										  _T("Please note that source of event is node, not an interface itself.\r\n")
	                             _T("Parameters:#\r\n")
										  _T("   1) Interface object ID\r\n")
										  _T("   2) Interface name\r\n")
										  _T("   3) Interface IP address\r\n")
										  _T("   4) Interface netmask\r\n")
										  _T("   5) Interface index")
										  ));

	// Create rule pair in event processing policy
	int ruleId = 0;
	DB_RESULT hResult = SQLSelect(_T("SELECT max(rule_id) FROM event_policy"));
	if (hResult != NULL)
	{
		ruleId = DBGetFieldLong(hResult, 0, 0) + 1;
		DBFreeResult(hResult);
	}

	TCHAR query[1024];
	_sntprintf(query, 1024, 
		_T("INSERT INTO event_policy (rule_id,flags,comments,alarm_message,alarm_severity,alarm_key,")
		_T("script,alarm_timeout,alarm_timeout_event,situation_id,situation_instance)	VALUES ")
		_T("(%d,7944,'Show alarm when interface is unexpectedly up','%%m',5,'IF_UNEXP_UP_%%i_%%1',")
		_T("'#00',0,%d,0,'#00')"), ruleId, EVENT_ALARM_TIMEOUT);
	CHK_EXEC(SQLQuery(query));
	_sntprintf(query, 1024, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_INTERFACE_UNEXPECTED_UP);
	CHK_EXEC(SQLQuery(query));
	ruleId++;

	_sntprintf(query, 1024, 
		_T("INSERT INTO event_policy (rule_id,flags,comments,alarm_message,alarm_severity,alarm_key,")
		_T("script,alarm_timeout,alarm_timeout_event,situation_id,situation_instance)	VALUES ")
		_T("(%d,7944,'Acknowlege interface unexpectedly up alarms when interface goes down','%%m',")
		_T("6,'IF_UNEXP_UP_%%i_%%1','#00',0,%d,0,'#00')"), ruleId, EVENT_ALARM_TIMEOUT);
	CHK_EXEC(SQLQuery(query));
	_sntprintf(query, 1024, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_INTERFACE_EXPECTED_DOWN);
	CHK_EXEC(SQLQuery(query));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='252' WHERE var_name='SchemaVersion'")));
   return TRUE;
}

/**
 * Upgrade from V250 to V251
 */
static BOOL H_UpgradeFromV250(int currVersion, int newVersion)
{
	static TCHAR batch[] = 
		_T("ALTER TABLE thresholds ADD current_severity integer\n")
		_T("ALTER TABLE thresholds ADD last_event_timestamp integer\n")
		_T("UPDATE thresholds SET current_severity=0,last_event_timestamp=0\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SetColumnNullable(_T("thresholds"), _T("fire_value")));
	CHK_EXEC(SetColumnNullable(_T("thresholds"), _T("rearm_value")));
	CHK_EXEC(ConvertStrings(_T("thresholds"), _T("threshold_id"), _T("fire_value")));
	CHK_EXEC(ConvertStrings(_T("thresholds"), _T("threshold_id"), _T("rearm_value")));

	CHK_EXEC(CreateConfigParam(_T("EnableNXSLContainerFunctions"), _T("1"), 1, 1));
	CHK_EXEC(CreateConfigParam(_T("UseDNSNameForDiscoveredNodes"), _T("0"), 1, 0));
	CHK_EXEC(CreateConfigParam(_T("AllowTrapVarbindsConversion"), _T("1"), 1, 1));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='251' WHERE var_name='SchemaVersion'")));
   return TRUE;
}

/**
 * Upgrade from V249 to V250
 */
static BOOL H_UpgradeFromV249(int currVersion, int newVersion)
{
	CHK_EXEC(CreateTable(_T("CREATE TABLE licenses (")
	                     _T("id integer not null,")
								_T("content $SQL:TEXT null,")
	                     _T("PRIMARY KEY(id))")));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='250' WHERE var_name='SchemaVersion'")));
   return TRUE;
}

/**
 * Upgrade from V248 to V249
 */
#define TDATA_CREATE_QUERY  _T("CREATE TABLE tdata_%d (item_id integer not null,tdata_timestamp integer not null,tdata_row integer not null,tdata_column integer not null,tdata_value varchar(255) null)")
#define TDATA_INDEX_MSSQL   _T("CREATE CLUSTERED INDEX idx_tdata_%d_id_timestamp ON tdata_%d(item_id,tdata_timestamp)")
#define TDATA_INDEX_PGSQL   _T("CREATE INDEX idx_tdata_%d_timestamp_id ON tdata_%d(tdata_timestamp,item_id)")
#define TDATA_INDEX_DEFAULT _T("CREATE INDEX idx_tdata_%d_id_timestamp ON tdata_%d(item_id,tdata_timestamp)")

static BOOL CreateTData(DWORD nodeId)
{
	TCHAR query[256];

	_sntprintf(query, 256, TDATA_CREATE_QUERY, (int)nodeId);
	CHK_EXEC(SQLQuery(query));

	switch(g_iSyntax)
	{
		case DB_SYNTAX_MSSQL:
			_sntprintf(query, 256, TDATA_INDEX_MSSQL, (int)nodeId, (int)nodeId);
			break;
		case DB_SYNTAX_PGSQL:
			_sntprintf(query, 256, TDATA_INDEX_PGSQL, (int)nodeId, (int)nodeId);
			break;
		default:
			_sntprintf(query, 256, TDATA_INDEX_DEFAULT, (int)nodeId, (int)nodeId);
			break;
	}
	CHK_EXEC(SQLQuery(query));

	return TRUE;
}

static BOOL H_UpgradeFromV248(int currVersion, int newVersion)
{
	CHK_EXEC(SQLQuery(_T("INSERT INTO metadata (var_name,var_value) VALUES ('TDataTableCreationCommand','") TDATA_CREATE_QUERY _T("')")));

	switch(g_iSyntax)
	{
		case DB_SYNTAX_MSSQL:
			CHK_EXEC(SQLQuery(_T("INSERT INTO metadata (var_name,var_value) VALUES ('TDataIndexCreationCommand_0','") TDATA_INDEX_MSSQL _T("')")));
			break;
		case DB_SYNTAX_PGSQL:
			CHK_EXEC(SQLQuery(_T("INSERT INTO metadata (var_name,var_value) VALUES ('TDataIndexCreationCommand_0','") TDATA_INDEX_PGSQL _T("')")));
			break;
		default:
			CHK_EXEC(SQLQuery(_T("INSERT INTO metadata (var_name,var_value) VALUES ('TDataIndexCreationCommand_0','") TDATA_INDEX_DEFAULT _T("')")));
			break;
	}

	CHK_EXEC(CreateTable(_T("CREATE TABLE dct_column_names (")
								_T("column_id integer not null,")
								_T("column_name varchar(63) not null,")
	                     _T("PRIMARY KEY(column_id))")));

	DB_RESULT hResult = SQLSelect(_T("SELECT id FROM nodes"));
	if (hResult != NULL)
	{
		int count = DBGetNumRows(hResult);
		for(int i = 0 ; i < count; i++)
		{
			if (!CreateTData(DBGetFieldULong(hResult, i, 0)))
			{
				if (!g_bIgnoreErrors)
				{
					DBFreeResult(hResult);
					return FALSE;
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

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='249' WHERE var_name='SchemaVersion'")));
   return TRUE;
}

/**
 * Upgrade from V247 to V248
 */
static BOOL H_UpgradeFromV247(int currVersion, int newVersion)
{
	CHK_EXEC(CreateTable(_T("CREATE TABLE dc_tables (")
	                     _T("item_id integer not null,")
								_T("node_id integer not null,")
								_T("template_id integer not null,")
								_T("template_item_id integer not null,")
								_T("name varchar(255) null,")
								_T("instance_column varchar(63) null,")
								_T("description varchar(255) null,")
								_T("flags integer not null,")
								_T("source integer not null,")
								_T("snmp_port integer not null,")
								_T("polling_interval integer not null,")
								_T("retention_time integer not null,")
								_T("status integer not null,")
								_T("system_tag varchar(255) null,")
								_T("resource_id integer not null,")
								_T("proxy_node integer not null,")
								_T("perftab_settings $SQL:TEXT null,")
	                     _T("PRIMARY KEY(item_id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE dc_table_columns (")
	                     _T("table_id integer not null,")
	                     _T("column_name varchar(63) not null,")
								_T("snmp_oid varchar(1023) null,")
								_T("data_type integer not null,")
								_T("transformation_script $SQL:TEXT null,")
	                     _T("PRIMARY KEY(table_id,column_name))")));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='248' WHERE var_name='SchemaVersion'")));
   return TRUE;
}

/**
 * Upgrade from V246 to V247
 */
static BOOL H_UpgradeFromV246(int currVersion, int newVersion)
{
	static TCHAR insertQuery[] = _T("INSERT INTO object_custom_attributes (object_id,attr_name,attr_value) VALUES (?,?,?)");

	CHK_EXEC(SetColumnNullable(_T("object_custom_attributes"), _T("attr_value")));

	// Convert strings in object_custom_attributes table
	DB_RESULT hResult = SQLSelect(_T("SELECT object_id,attr_name,attr_value FROM object_custom_attributes"));
	if (hResult != NULL)
	{
		if (SQLQuery(_T("DELETE FROM object_custom_attributes")))
		{
			TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
			DB_STATEMENT hStmt = DBPrepareEx(g_hCoreDB, insertQuery, errorText);
			if (hStmt != NULL)
			{
				TCHAR name[128], *value;
				int count = DBGetNumRows(hResult);
				for(int i = 0; i < count; i++)
				{
					DWORD id = DBGetFieldULong(hResult, i, 0);
					DBGetField(hResult, i, 1, name, 128);
					DecodeSQLString(name);
					value = DBGetField(hResult, i, 2, NULL, 0);
					DecodeSQLString(value);

					DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
					DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, name, DB_BIND_STATIC);
					DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, value, DB_BIND_DYNAMIC);
					if (g_bTrace)
						ShowQuery(insertQuery);
					if (!DBExecuteEx(hStmt, errorText))
					{
						WriteToTerminalEx(_T("SQL query failed (%s):\n\x1b[33;1m%s\x1b[0m\n"), errorText, insertQuery);
						if (!g_bIgnoreErrors)
						{
							DBFreeStatement(hStmt);
							DBFreeResult(hResult);
							return FALSE;
						}
					}
				}
				DBFreeStatement(hStmt);
			}
			else
			{
				WriteToTerminalEx(_T("SQL query failed (%s):\n\x1b[33;1m%s\x1b[0m\n"), errorText, insertQuery);
				if (!g_bIgnoreErrors)
				{
					DBFreeResult(hResult);
					return FALSE;
				}
			}
		}
		else
		{
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

	CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_ocattr_oid ON object_custom_attributes(object_id)")));
	CHK_EXEC(CreateConfigParam(_T("AlarmHistoryRetentionTime"), _T("180"), 1, 0));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='247' WHERE var_name='SchemaVersion'")));
   return TRUE;
}

/**
 * Upgrade from V245 to V246
 */
static BOOL H_UpgradeFromV245(int currVersion, int newVersion)
{
	static TCHAR batch[] = 
		_T("ALTER TABLE snmp_trap_pmap ADD flags integer\n")
		_T("UPDATE snmp_trap_pmap SET flags=0\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SetColumnNullable(_T("snmp_trap_pmap"), _T("description")));
	CHK_EXEC(ConvertStrings(_T("snmp_trap_pmap"), _T("trap_id"), _T("parameter"), _T("description"), false));
	
	CHK_EXEC(SetColumnNullable(_T("cluster_resources"), _T("resource_name")));
	CHK_EXEC(ConvertStrings(_T("cluster_resources"), _T("cluster_id"), _T("resource_id"), _T("resource_name"), false));
	
	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='246' WHERE var_name='SchemaVersion'")));
   return TRUE;
}

/**
 * Upgrade from V244 to V245
 */
static BOOL H_UpgradeFromV244(int currVersion, int newVersion)
{
	static TCHAR batch[] = 
		_T("ALTER TABLE nodes ADD runtime_flags integer\n")
		_T("UPDATE nodes SET runtime_flags=0\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SetColumnNullable(_T("actions"), _T("rcpt_addr")));
	CHK_EXEC(SetColumnNullable(_T("actions"), _T("email_subject")));
	CHK_EXEC(SetColumnNullable(_T("actions"), _T("action_data")));

	CHK_EXEC(ConvertStrings(_T("actions"), _T("action_id"), _T("rcpt_addr")));
	CHK_EXEC(ConvertStrings(_T("actions"), _T("action_id"), _T("email_subject")));
	CHK_EXEC(ConvertStrings(_T("actions"), _T("action_id"), _T("action_data")));
	
	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='245' WHERE var_name='SchemaVersion'")));
   return TRUE;
}


//
// Upgrade from V243 to V244
//

static BOOL H_UpgradeFromV243(int currVersion, int newVersion)
{
	static TCHAR batch[] = 
		_T("ALTER TABLE interfaces ADD dot1x_pae_state integer\n")
		_T("ALTER TABLE interfaces ADD dot1x_backend_state integer\n")
		_T("UPDATE interfaces SET dot1x_pae_state=0,dot1x_backend_state=0\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(CreateEventTemplate(EVENT_8021X_PAE_STATE_CHANGED, _T("SYS_8021X_PAE_STATE_CHANGED"),
		EVENT_SEVERITY_NORMAL, 1, _T("Port %6 PAE state changed from %4 to %2"), 
		_T("Generated when switch port PAE state changed.\r\nParameters:\r\n")
		_T("   1) New PAE state code\r\n")
		_T("   2) New PAE state as text\r\n")
		_T("   3) Old PAE state code\r\n")
		_T("   4) Old PAE state as text\r\n")
		_T("   5) Interface index\r\n")
		_T("   6) Interface name")));

	CHK_EXEC(CreateEventTemplate(EVENT_8021X_BACKEND_STATE_CHANGED, _T("SYS_8021X_BACKEND_STATE_CHANGED"),
		EVENT_SEVERITY_NORMAL, 1, _T("Port %6 backend authentication state changed from %4 to %2"), 
		_T("Generated when switch port backend authentication state changed.\r\nParameters:\r\n")
		_T("   1) New backend state code\r\n")
		_T("   2) New backend state as text\r\n")
		_T("   3) Old backend state code\r\n")
		_T("   4) Old backend state as text\r\n")
		_T("   5) Interface index\r\n")
		_T("   6) Interface name")));

	CHK_EXEC(CreateEventTemplate(EVENT_8021X_PAE_FORCE_UNAUTH, _T("SYS_8021X_PAE_FORCE_UNAUTH"),
		EVENT_SEVERITY_MAJOR, 1, _T("Port %2 switched to force unauthorize state"), 
		_T("Generated when switch port PAE state changed to FORCE UNAUTHORIZE.\r\nParameters:\r\n")
		_T("   1) Interface index\r\n")
		_T("   2) Interface name")));

	CHK_EXEC(CreateEventTemplate(EVENT_8021X_AUTH_FAILED, _T("SYS_8021X_AUTH_FAILED"),
		EVENT_SEVERITY_MAJOR, 1, _T("802.1x authentication failed on port %2"), 
		_T("Generated when switch port backend authentication state changed to FAIL.\r\nParameters:\r\n")
		_T("   1) Interface index\r\n")
		_T("   2) Interface name")));

	CHK_EXEC(CreateEventTemplate(EVENT_8021X_AUTH_TIMEOUT, _T("SYS_8021X_AUTH_TIMEOUT"),
		EVENT_SEVERITY_MAJOR, 1, _T("802.1x authentication time out on port %2"), 
		_T("Generated when switch port backend authentication state changed to TIMEOUT.\r\nParameters:\r\n")
		_T("   1) Interface index\r\n")
		_T("   2) Interface name")));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='244' WHERE var_name='SchemaVersion'")));
   return TRUE;
}


//
// Upgrade from V242 to V243
//

static BOOL H_UpgradeFromV242(int currVersion, int newVersion)
{
	static TCHAR batch[] = 
		_T("ALTER TABLE items ADD snmp_raw_value_type integer\n")
		_T("UPDATE items SET snmp_raw_value_type=0\n")
		_T("ALTER TABLE items ADD flags integer\n")
		_T("UPDATE items SET flags=adv_schedule+(all_thresholds*2)\n")
		_T("ALTER TABLE items DROP COLUMN adv_schedule\n")
		_T("ALTER TABLE items DROP COLUMN all_thresholds\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));
	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='243' WHERE var_name='SchemaVersion'")));
   return TRUE;
}


//
// Upgrade from V241 to V242
//

static BOOL H_UpgradeFromV241(int currVersion, int newVersion)
{
	static TCHAR batch[] = 
		_T("DROP TABLE business_service_templates\n")
		_T("ALTER TABLE dashboards ADD options integer\n")
		_T("UPDATE dashboards SET options=0\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));
	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='242' WHERE var_name='SchemaVersion'")));
   return TRUE;
}


//
// Upgrade from V240 to V241
//

static BOOL H_UpgradeFromV240(int currVersion, int newVersion)
{
	static TCHAR batch[] = 
		_T("ALTER TABLE slm_checks ADD template_id integer\n")
		_T("ALTER TABLE slm_checks ADD current_ticket integer\n")
		_T("UPDATE slm_checks SET template_id=0,current_ticket=0\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));
	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='241' WHERE var_name='SchemaVersion'")));
   return TRUE;
}


//
// Upgrade from V239 to V240
//

static BOOL H_UpgradeFromV239(int currVersion, int newVersion)
{
	CHK_EXEC(SQLQuery(_T("ALTER TABLE raw_dci_values ADD transformed_value varchar(255)")));
	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='240' WHERE var_name='SchemaVersion'")));
   return TRUE;
}


//
// Upgrade from V238 to V239
//

static BOOL H_UpgradeFromV238(int currVersion, int newVersion)
{
	CHK_EXEC(SQLQuery(
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) VALUES ")
		_T("(56,'SYS_IP_ADDRESS_CHANGED',1,1,'Primary IP address changed from %2 to %1',")
		_T("'Generated when primary IP address changed (usually because of primary name change or DNS change).#0D#0A")
		_T("Parameters:#0D#0A   1) New IP address#0D#0A   2) Old IP address#0D#0A   3) Primary host name')")));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='239' WHERE var_name='SchemaVersion'")));
   return TRUE;
}


//
// Upgrade from V232 to V238
//

static BOOL H_UpgradeFromV232toV238(int currVersion, int newVersion)
{
	CHK_EXEC(CreateTable(_T("CREATE TABLE slm_checks (")
	                     _T("id integer not null,")
	                     _T("type integer not null,")
								_T("content $SQL:TEXT null,")
	                     _T("threshold_id integer not null,")
	                     _T("reason varchar(255) null,")
	                     _T("is_template integer not null,")
	                     _T("PRIMARY KEY(id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE slm_tickets (")
	                     _T("ticket_id integer not null,")
	                     _T("service_id integer not null,")
	                     _T("check_id integer not null,")
	                     _T("create_timestamp integer not null,")
	                     _T("close_timestamp integer not null,")
	                     _T("reason varchar(255) null,")
	                     _T("PRIMARY KEY(ticket_id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE slm_service_history (")
	                     _T("record_id integer not null,")
	                     _T("service_id integer not null,")
	                     _T("change_timestamp integer not null,")
	                     _T("new_status integer not null,")
	                     _T("PRIMARY KEY(record_id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE report_results (")
	                     _T("report_id integer not null,")
	                     _T("generated integer not null,")
	                     _T("job_id integer not null,")
	                     _T("PRIMARY KEY(report_id,job_id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE reports (")
	                     _T("id integer not null,")
								_T("definition $SQL:TEXT null,")
	                     _T("PRIMARY KEY(id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE job_history (")
	                     _T("id integer not null,")
	                     _T("time_created integer not null,")
	                     _T("time_started integer not null,")
	                     _T("time_finished integer not null,")
	                     _T("job_type varchar(127) null,")
	                     _T("description varchar(255) null,")
								_T("additional_info varchar(255) null,")
	                     _T("node_id integer not null,")
	                     _T("user_id integer not null,")
	                     _T("status integer not null,")
	                     _T("failure_message varchar(255) null,")
	                     _T("PRIMARY KEY(id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE business_services (")
	                     _T("service_id integer not null,")
	                     _T("PRIMARY KEY(service_id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE business_service_templates (")
	                     _T("service_id integer not null,")
	                     _T("template_id integer not null,")
	                     _T("PRIMARY KEY(service_id,template_id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE node_links (")
	                     _T("nodelink_id integer not null,")
	                     _T("node_id integer not null,")
	                     _T("PRIMARY KEY(nodelink_id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE slm_agreements (")
	                     _T("agreement_id integer not null,")
	                     _T("service_id integer not null,")
	                     _T("org_id integer not null,")
	                     _T("uptime varchar(63) not null,")
	                     _T("period integer not null,")
	                     _T("start_date integer not null,")
	                     _T("notes varchar(255),")
	                     _T("PRIMARY KEY(agreement_id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE organizations (")
	                     _T("id integer not null,")
	                     _T("parent_id integer not null,")
	                     _T("org_type integer not null,")
	                     _T("name varchar(63) not null,")
	                     _T("description varchar(255),")
	                     _T("manager integer not null,")
	                     _T("PRIMARY KEY(id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE persons (")
	                     _T("id integer not null,")
	                     _T("org_id integer not null,")
	                     _T("first_name varchar(63),")
	                     _T("last_name varchar(63),")
	                     _T("title varchar(255),")
	                     _T("status integer not null,")
	                     _T("PRIMARY KEY(id))")));

	CHK_EXEC(CreateConfigParam(_T("JobHistoryRetentionTime"), _T("90"), 1, 0));

	CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET description=")
	                  _T("'Generated when threshold check is rearmed for specific data collection item.#0D#0A")
	                  _T("Parameters:#0D#0A")
	                  _T("   1) Parameter name#0D#0A")
	                  _T("   2) Item description#0D#0A")
	                  _T("   3) Data collection item ID#0D#0A")
	                  _T("   4) Instance#0D#0A")
	                  _T("   5) Threshold value#0D#0A")
	                  _T("   6) Actual value' WHERE event_code=18")));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='238' WHERE var_name='SchemaVersion'")));
   return TRUE;
}


//
// Upgrade from V237 to V238
//

static BOOL H_UpgradeFromV237(int currVersion, int newVersion)
{
	static TCHAR batch[] = 
		_T("DROP TABLE slm_check_templates\n")
		_T("DROP TABLE node_link_checks\n")
		_T("DROP TABLE slm_checks\n")
		_T("DROP TABLE slm_tickets\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(CreateTable(_T("CREATE TABLE slm_checks (")
	                     _T("id integer not null,")
	                     _T("type integer not null,")
								_T("content $SQL:TEXT null,")
	                     _T("threshold_id integer not null,")
	                     _T("reason varchar(255) null,")
	                     _T("is_template integer not null,")
	                     _T("PRIMARY KEY(id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE slm_tickets (")
	                     _T("ticket_id integer not null,")
	                     _T("service_id integer not null,")
	                     _T("check_id integer not null,")
	                     _T("create_timestamp integer not null,")
	                     _T("close_timestamp integer not null,")
	                     _T("reason varchar(255) null,")
	                     _T("PRIMARY KEY(ticket_id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE slm_service_history (")
	                     _T("record_id integer not null,")
	                     _T("service_id integer not null,")
	                     _T("change_timestamp integer not null,")
	                     _T("new_status integer not null,")
	                     _T("PRIMARY KEY(record_id))")));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='238' WHERE var_name='SchemaVersion'")));
   return TRUE;
}


//
// Upgrade from V236 to V237
//

static BOOL H_UpgradeFromV236(int currVersion, int newVersion)
{
	static TCHAR batch[] = 
		_T("ALTER TABLE business_services DROP COLUMN name\n")
		_T("ALTER TABLE business_services DROP COLUMN parent_id\n")
		_T("ALTER TABLE business_services DROP COLUMN status\n")
		_T("ALTER TABLE slm_checks DROP COLUMN name\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='237' WHERE var_name='SchemaVersion'")));
   return TRUE;
}


//
// Upgrade from V235 to V236
//

static BOOL H_UpgradeFromV235(int currVersion, int newVersion)
{
	CHK_EXEC(CreateTable(_T("CREATE TABLE report_results (")
	                     _T("report_id integer not null,")
	                     _T("generated integer not null,")
	                     _T("job_id integer not null,")
	                     _T("PRIMARY KEY(report_id,job_id))")));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='236' WHERE var_name='SchemaVersion'")));
   return TRUE;
}


//
// Upgrade from V234 to V235
//

static BOOL H_UpgradeFromV234(int currVersion, int newVersion)
{
	CHK_EXEC(CreateTable(_T("CREATE TABLE reports (")
	                     _T("id integer not null,")
								_T("definition $SQL:TEXT null,")
	                     _T("PRIMARY KEY(id))")));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='235' WHERE var_name='SchemaVersion'")));
   return TRUE;
}


//
// Upgrade from V233 to V234
//

static BOOL H_UpgradeFromV233(int currVersion, int newVersion)
{
	CHK_EXEC(CreateTable(_T("CREATE TABLE job_history (")
	                     _T("id integer not null,")
	                     _T("time_created integer not null,")
	                     _T("time_started integer not null,")
	                     _T("time_finished integer not null,")
	                     _T("job_type varchar(127) null,")
	                     _T("description varchar(255) null,")
								_T("additional_info varchar(255) null,")
	                     _T("node_id integer not null,")
	                     _T("user_id integer not null,")
	                     _T("status integer not null,")
	                     _T("failure_message varchar(255) null,")
	                     _T("PRIMARY KEY(id))")));

	CHK_EXEC(CreateConfigParam(_T("JobHistoryRetentionTime"), _T("90"), 1, 0));

	CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET description=")
	                  _T("'Generated when threshold check is rearmed for specific data collection item.#0D#0A")
	                  _T("Parameters:#0D#0A")
	                  _T("   1) Parameter name#0D#0A")
	                  _T("   2) Item description#0D#0A")
	                  _T("   3) Data collection item ID#0D#0A")
	                  _T("   4) Instance#0D#0A")
	                  _T("   5) Threshold value#0D#0A")
	                  _T("   6) Actual value' WHERE event_code=18")));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='234' WHERE var_name='SchemaVersion'")));
   return TRUE;
}


//
// Upgrade from V232 to V233
//

static BOOL H_UpgradeFromV232(int currVersion, int newVersion)
{
	CHK_EXEC(CreateTable(_T("CREATE TABLE business_services (")
	                     _T("service_id integer not null,")
	                     _T("name varchar(63) not null,")
	                     _T("parent_id integer not null,")
	                     _T("status integer not null,")
	                     _T("PRIMARY KEY(service_id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE business_service_templates (")
	                     _T("service_id integer not null,")
	                     _T("template_id integer not null,")
	                     _T("PRIMARY KEY(service_id,template_id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE slm_checks (")
	                     _T("check_id integer not null,")
	                     _T("name varchar(63) not null,")
	                     _T("type integer not null,")
								_T("content $SQL:TEXT,")
	                     _T("threshold_id integer not null,")
	                     _T("state integer not null,")
	                     _T("reason varchar(255) not null,")
	                     _T("PRIMARY KEY(check_id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE slm_check_templates (")
	                     _T("id integer not null,")
	                     _T("name varchar(63) not null,")
	                     _T("type integer not null,")
								_T("content $SQL:TEXT,")
	                     _T("threshold_id integer not null,")
	                     _T("PRIMARY KEY(id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE node_links (")
	                     _T("nodelink_id integer not null,")
	                     _T("node_id integer not null,")
	                     _T("PRIMARY KEY(nodelink_id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE node_link_checks (")
	                     _T("nodelink_id integer not null,")
	                     _T("check_id integer not null,")
	                     _T("PRIMARY KEY(nodelink_id,check_id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE slm_agreements (")
	                     _T("agreement_id integer not null,")
	                     _T("service_id integer not null,")
	                     _T("org_id integer not null,")
	                     _T("uptime varchar(63) not null,")
	                     _T("period integer not null,")
	                     _T("start_date integer not null,")
	                     _T("notes varchar(255),")
	                     _T("PRIMARY KEY(agreement_id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE slm_tickets (")
	                     _T("ticket_id integer not null,")
	                     _T("service_id integer not null,")
	                     _T("create_timestamp integer not null,")
	                     _T("close_timestamp integer not null,")
	                     _T("reason varchar(255) not null,")
	                     _T("PRIMARY KEY(ticket_id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE organizations (")
	                     _T("id integer not null,")
	                     _T("parent_id integer not null,")
	                     _T("org_type integer not null,")
	                     _T("name varchar(63) not null,")
	                     _T("description varchar(255),")
	                     _T("manager integer not null,")
	                     _T("PRIMARY KEY(id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE persons (")
	                     _T("id integer not null,")
	                     _T("org_id integer not null,")
	                     _T("first_name varchar(63),")
	                     _T("last_name varchar(63),")
	                     _T("title varchar(255),")
	                     _T("status integer not null,")
	                     _T("PRIMARY KEY(id))")));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='233' WHERE var_name='SchemaVersion'")));
   return TRUE;
}


//
// Upgrade from V231 to V232
//

static BOOL H_UpgradeFromV231(int currVersion, int newVersion)
{
	static TCHAR batch[] = 
		_T("ALTER TABLE object_properties ADD submap_id integer\n")
		_T("UPDATE object_properties SET submap_id=0\n")
		_T("DROP TABLE maps\n")
		_T("DROP TABLE map_access_lists\n")
		_T("DROP TABLE submaps\n")
		_T("DROP TABLE submap_object_positions\n")
		_T("DROP TABLE submap_links\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='232' WHERE var_name='SchemaVersion'")));
   return TRUE;
}


//
// Upgrade from V230 to V231
//

static BOOL H_UpgradeFromV230(int currVersion, int newVersion)
{
	static TCHAR batch[] = 
		_T("ALTER TABLE nodes ADD bridge_base_addr varchar(15)\n")
		_T("UPDATE nodes SET bridge_base_addr='000000000000'\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='231' WHERE var_name='SchemaVersion'")));
   return TRUE;
}


//
// Upgrade from V229 to V230
//

static BOOL H_UpgradeFromV229(int currVersion, int newVersion)
{
	static TCHAR batch1[] = 
		_T("ALTER TABLE network_maps ADD bg_latitude varchar(20)\n")
		_T("ALTER TABLE network_maps ADD bg_longitude varchar(20)\n")
		_T("ALTER TABLE network_maps ADD bg_zoom integer\n")
		_T("ALTER TABLE dashboard_elements ADD layout_data $SQL:TEXT\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch1));

	DB_RESULT hResult = SQLSelect(_T("SELECT dashboard_id,element_id,horizontal_span,vertical_span,horizontal_alignment,vertical_alignment FROM dashboard_elements"));
	if (hResult != NULL)
	{
		TCHAR query[1024], xml[1024];

		int count = DBGetNumRows(hResult);
		for(int i = 0; i < count; i++)
		{
			_sntprintf(xml, 1024, _T("<layout><horizontalSpan>%d</horizontalSpan><verticalSpan>%d</verticalSpan><horizontalAlignment>%d</horizontalAlignment><verticalAlignment>%d</verticalAlignment></layout>"),
			           (int)DBGetFieldLong(hResult, i, 2), (int)DBGetFieldLong(hResult, i, 3),
						  (int)DBGetFieldLong(hResult, i, 4), (int)DBGetFieldLong(hResult, i, 5));
			_sntprintf(query, 1024, _T("UPDATE dashboard_elements SET layout_data=%s WHERE dashboard_id=%d AND element_id=%d"),
			           (const TCHAR *)DBPrepareString(g_hCoreDB, xml), (int)DBGetFieldLong(hResult, i, 0), (int)DBGetFieldLong(hResult, i, 1));
			CHK_EXEC(SQLQuery(query));
		}
		DBFreeResult(hResult);
	}
	else
	{
		if (!g_bIgnoreErrors)
			return FALSE;
	}

	static TCHAR batch2[] = 
		_T("ALTER TABLE dashboard_elements DROP COLUMN horizontal_span\n")
		_T("ALTER TABLE dashboard_elements DROP COLUMN vertical_span\n")
		_T("ALTER TABLE dashboard_elements DROP COLUMN horizontal_alignment\n")
		_T("ALTER TABLE dashboard_elements DROP COLUMN vertical_alignment\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch2));

	CreateConfigParam(_T("TileServerURL"), _T("http://tile.openstreetmap.org/"), 1, 0);

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='230' WHERE var_name='SchemaVersion'")));
   return TRUE;
}


//
// Upgrade from V228 to V229
//

static BOOL H_UpgradeFromV228(int currVersion, int newVersion)
{
	CHK_EXEC(CreateTable(_T("CREATE TABLE dashboards (")
				_T("   id integer not null,")
				_T("   num_columns integer not null,")
				_T("   PRIMARY KEY(id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE dashboard_elements (")
				_T("   dashboard_id integer not null,")
				_T("   element_id integer not null,")
				_T("   element_type integer not null,")
				_T("   element_data $SQL:TEXT null,")
				_T("   horizontal_span integer not null,")
				_T("   vertical_span integer not null,")
				_T("   horizontal_alignment integer not null,")
				_T("   vertical_alignment integer not null,")
				_T("   PRIMARY KEY(dashboard_id,element_id))")));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='229' WHERE var_name='SchemaVersion'")));
   return TRUE;
}


//
// Upgrade from V227 to V228
//

static BOOL H_UpgradeFromV227(int currVersion, int newVersion)
{
	CHK_EXEC(SQLQuery(_T("DROP TABLE web_maps")));
	CHK_EXEC(MigrateMaps());
	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='228' WHERE var_name='SchemaVersion'")));
   return TRUE;
}


//
// Upgrade from V226 to V227
//

static BOOL H_UpgradeFromV226(int currVersion, int newVersion)
{
	static TCHAR batch[] = 
		_T("ALTER TABLE clusters ADD zone_guid integer\n")
		_T("UPDATE clusters SET zone_guid=0\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='227' WHERE var_name='SchemaVersion'")));
   return TRUE;
}


//
// Upgrade from V225 to V226
//

static BOOL H_UpgradeFromV225(int currVersion, int newVersion)
{
	static TCHAR batch[] = 
		_T("ALTER TABLE interfaces ADD flags integer\n")
		_T("UPDATE interfaces SET flags=0\n")
		_T("UPDATE interfaces SET flags=1 WHERE synthetic_mask<>0\n")
		_T("ALTER TABLE interfaces DROP COLUMN synthetic_mask\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='226' WHERE var_name='SchemaVersion'")));
   return TRUE;
}


//
// Upgrade from V224 to V225
//

static BOOL H_UpgradeFromV224(int currVersion, int newVersion)
{
	static TCHAR batch[] = 
		_T("ALTER TABLE interfaces ADD description varchar(255)\n")
		_T("UPDATE interfaces SET description=''\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='225' WHERE var_name='SchemaVersion'")));
   return TRUE;
}


//
// Upgrade from V223 to V224
//

static BOOL H_UpgradeFromV223(int currVersion, int newVersion)
{
	static TCHAR batch[] = 
		_T("DROP TABLE zone_ip_addr_list\n")
		_T("ALTER TABLE zones DROP COLUMN zone_type\n")
		_T("ALTER TABLE zones DROP COLUMN controller_ip\n")
		_T("ALTER TABLE zones ADD agent_proxy integer\n")
		_T("ALTER TABLE zones ADD snmp_proxy integer\n")
		_T("ALTER TABLE zones ADD icmp_proxy integer\n")
		_T("UPDATE zones SET agent_proxy=0,snmp_proxy=0,icmp_proxy=0\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='224' WHERE var_name='SchemaVersion'")));
   return TRUE;
}


//
// Upgrade from V222 to V223
//

static BOOL H_UpgradeFromV222(int currVersion, int newVersion)
{
	static TCHAR batch[] = 
		_T("DROP TABLE oid_to_type\n")
		_T("ALTER TABLE nodes DROP COLUMN node_type\n")
		_T("ALTER TABLE nodes ADD primary_name varchar(255)\n")
		_T("UPDATE nodes SET primary_name=primary_ip\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='223' WHERE var_name='SchemaVersion'")));
   return TRUE;
}


//
// Upgrade from V221 to V222
//

static BOOL H_UpgradeFromV221(int currVersion, int newVersion)
{
	static TCHAR batch[] = 
		_T("ALTER TABLE object_properties ADD image varchar(36)\n")
		_T("UPDATE object_properties SET image='00000000-0000-0000-0000-000000000000'\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='222' WHERE var_name='SchemaVersion'")));
   return TRUE;
}


//
// Upgrade from V220 to V221
//

static BOOL H_UpgradeFromV220(int currVersion, int newVersion)
{
	static TCHAR batch[] = 
		_T("ALTER TABLE network_maps DROP COLUMN background\n")
		_T("ALTER TABLE network_maps ADD background varchar(36)\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='221' WHERE var_name='SchemaVersion'")));
   return TRUE;
}


//
// Upgrade from V219 to V220
//

static BOOL H_UpgradeFromV219(int currVersion, int newVersion)
{
	static TCHAR batch[] = 
		_T("ALTER TABLE interfaces ADD bridge_port integer\n")
		_T("ALTER TABLE interfaces ADD phy_slot integer\n")
		_T("ALTER TABLE interfaces ADD phy_port integer\n")
		_T("ALTER TABLE interfaces ADD peer_node_id integer\n")
		_T("ALTER TABLE interfaces ADD peer_if_id integer\n")
		_T("UPDATE interfaces SET bridge_port=0,phy_slot=0,phy_port=0,peer_node_id=0,peer_if_id=0\n")
		_T("ALTER TABLE nodes ADD snmp_sys_name varchar(127)\n")
		_T("UPDATE nodes SET snmp_sys_name=''\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='220' WHERE var_name='SchemaVersion'")));
   return TRUE;
}


//
// Upgrade from V218 to V219
//

static BOOL H_UpgradeFromV218(int currVersion, int newVersion)
{
	CHK_EXEC(CreateTable(_T("CREATE TABLE images (")
				_T("   guid varchar(36) not null,")
				_T("   mimetype varchar(64) not null,")
				_T("   name varchar(255) not null,")
				_T("   category varchar(255) not null,")
				_T("   protected integer default 0,")
				_T("   ")
				_T("   PRIMARY KEY(guid),")
				_T("   UNIQUE(name, category))")));

   static TCHAR batch[] =
      _T("INSERT INTO images (guid, mimetype, name, category, protected) VALUES ")
         _T("('1ddb76a3-a05f-4a42-acda-22021768feaf', 'image/png', 'ATM', 'Network Objects', 1)\n")
      _T("INSERT INTO images (guid, mimetype, name, category, protected) VALUES ")
         _T("('b314cf44-b2aa-478e-b23a-73bc5bb9a624', 'image/png', 'HSM', 'Network Objects', 1)\n")
      _T("INSERT INTO images (guid, mimetype, name, category, protected) VALUES ")
         _T("('904e7291-ee3f-41b7-8132-2bd29288ecc8', 'image/png', 'Node', 'Network Objects', 1)\n")
      _T("INSERT INTO images (guid, mimetype, name, category, protected) VALUES ")
         _T("('f5214d16-1ab1-4577-bb21-063cfd45d7af', 'image/png', 'Printer', 'Network Objects', 1)\n")
      _T("INSERT INTO images (guid, mimetype, name, category, protected) VALUES ")
         _T("('bacde727-b183-4e6c-8dca-ab024c88b999', 'image/png', 'Router', 'Network Objects', 1)\n")
      _T("INSERT INTO images (guid, mimetype, name, category, protected) VALUES ")
         _T("('ba6ab507-f62d-4b8f-824c-ca9d46f22375', 'image/png', 'Server', 'Network Objects', 1)\n")
      _T("INSERT INTO images (guid, mimetype, name, category, protected) VALUES ")
         _T("('092e4b35-4e7c-42df-b9b7-d5805bfac64e', 'image/png', 'Service', 'Network Objects', 1)\n")
      _T("INSERT INTO images (guid, mimetype, name, category, protected) VALUES ")
         _T("('f9105c54-8dcf-483a-b387-b4587dfd3cba', 'image/png', 'Switch', 'Network Objects', 1)\n")
      _T("INSERT INTO images (guid, mimetype, name, category, protected) VALUES ")
         _T("('7cd999e9-fbe0-45c3-a695-f84523b3a50c', 'image/png', 'Unknown', 'Network Objects', 1)\n")
      _T("<END>");

   CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='219' WHERE var_name='SchemaVersion'")));
   return TRUE;
}

/**
 * Upgrade from V217 to V218
 */
static BOOL H_UpgradeFromV217(int currVersion, int newVersion)
{
	CHK_EXEC(SetColumnNullable(_T("snmp_communities"), _T("community")));
	CHK_EXEC(ConvertStrings(_T("snmp_communities"), _T("id"), _T("community")));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='218' WHERE var_name='SchemaVersion'")));
   return TRUE;
}

/**
 * Upgrade from V216 to V217
 */
static BOOL H_UpgradeFromV216(int currVersion, int newVersion)
{
	static TCHAR batch[] = 
		_T("ALTER TABLE nodes ADD snmp_port integer\n")
		_T("UPDATE nodes SET snmp_port=161\n")
		_T("ALTER TABLE items ADD snmp_port integer\n")
		_T("UPDATE items SET snmp_port=0\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));

	CHK_EXEC(SetColumnNullable(_T("nodes"), _T("community")));
	CHK_EXEC(ConvertStrings(_T("nodes"), _T("id"), _T("community")));

	CHK_EXEC(SetColumnNullable(_T("nodes"), _T("usm_auth_password")));
	CHK_EXEC(ConvertStrings(_T("nodes"), _T("id"), _T("usm_auth_password")));

	CHK_EXEC(SetColumnNullable(_T("nodes"), _T("usm_priv_password")));
	CHK_EXEC(ConvertStrings(_T("nodes"), _T("id"), _T("usm_priv_password")));

	CHK_EXEC(SetColumnNullable(_T("nodes"), _T("snmp_oid")));
	CHK_EXEC(ConvertStrings(_T("nodes"), _T("id"), _T("snmp_oid")));

	CHK_EXEC(SetColumnNullable(_T("nodes"), _T("secret")));
	CHK_EXEC(ConvertStrings(_T("nodes"), _T("id"), _T("secret")));

	CHK_EXEC(SetColumnNullable(_T("nodes"), _T("agent_version")));
	CHK_EXEC(ConvertStrings(_T("nodes"), _T("id"), _T("agent_version")));

	CHK_EXEC(SetColumnNullable(_T("nodes"), _T("platform_name")));
	CHK_EXEC(ConvertStrings(_T("nodes"), _T("id"), _T("platform_name")));

	CHK_EXEC(SetColumnNullable(_T("nodes"), _T("uname")));
	CHK_EXEC(ConvertStrings(_T("nodes"), _T("id"), _T("uname")));

	CHK_EXEC(SetColumnNullable(_T("items"), _T("name")));
	CHK_EXEC(ConvertStrings(_T("items"), _T("item_id"), _T("name")));

	CHK_EXEC(SetColumnNullable(_T("items"), _T("description")));
	CHK_EXEC(ConvertStrings(_T("items"), _T("item_id"), _T("description")));

	CHK_EXEC(SetColumnNullable(_T("items"), _T("transformation")));
	CHK_EXEC(ConvertStrings(_T("items"), _T("item_id"), _T("transformation")));

	CHK_EXEC(SetColumnNullable(_T("items"), _T("instance")));
	CHK_EXEC(ConvertStrings(_T("items"), _T("item_id"), _T("instance")));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='217' WHERE var_name='SchemaVersion'")));
   return TRUE;
}

/**
 * Upgrade from V215 to V216
 */
static BOOL H_UpgradeFromV215(int currVersion, int newVersion)
{
	CHK_EXEC(SetColumnNullable(_T("ap_common"), _T("description")));
	CHK_EXEC(ConvertStrings(_T("ap_common"), _T("id"), _T("description")));

	if (g_iSyntax != DB_SYNTAX_SQLITE)
		CHK_EXEC(SQLQuery(_T("ALTER TABLE ap_config_files DROP COLUMN file_name")));

	CHK_EXEC(SetColumnNullable(_T("ap_config_files"), _T("file_content")));
	CHK_EXEC(ConvertStrings(_T("ap_config_files"), _T("policy_id"), _T("file_content")));

	CHK_EXEC(SQLQuery(_T("ALTER TABLE object_properties ADD guid varchar(36)")));

	// Generate GUIDs for all objects
	DB_RESULT hResult = SQLSelect(_T("SELECT object_id FROM object_properties"));
	if (hResult != NULL)
	{
		int count = DBGetNumRows(hResult);
		for(int i = 0; i < count; i++)
		{
			uuid_t guid;
			TCHAR query[256], buffer[64];

			uuid_generate(guid);
			_sntprintf(query, 256, _T("UPDATE object_properties SET guid='%s' WHERE object_id=%d"),
			           uuid_to_string(guid, buffer), DBGetFieldULong(hResult, i, 0));
			CHK_EXEC(SQLQuery(query));
		}
		DBFreeResult(hResult);
	}
	
	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='216' WHERE var_name='SchemaVersion'")));
   return TRUE;
}


//
// Upgrade from V214 to V215
//

static BOOL H_UpgradeFromV214(int currVersion, int newVersion)
{
	CHK_EXEC(CreateTable(_T("CREATE TABLE network_maps (")
		                  _T("id integer not null,")
						      _T("map_type integer not null,")
						      _T("layout integer not null,")
						      _T("seed integer not null,")
						      _T("background integer not null,")
	                     _T("PRIMARY KEY(id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE network_map_elements (")
		                  _T("map_id integer not null,")
		                  _T("element_id integer not null,")
						      _T("element_type integer not null,")
								_T("element_data $SQL:TEXT not null,")
	                     _T("PRIMARY KEY(map_id,element_id))")));

	CHK_EXEC(CreateTable(_T("CREATE TABLE network_map_links (")
		                  _T("map_id integer not null,")
		                  _T("element1 integer not null,")
		                  _T("element2 integer not null,")
						      _T("link_type integer not null,")
								_T("link_name varchar(255) null,")
								_T("connector_name1 varchar(255) null,")
								_T("connector_name2 varchar(255) null,")
	                     _T("PRIMARY KEY(map_id,element1,element2))")));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='215' WHERE var_name='SchemaVersion'")));
   return TRUE;
}

/**
 * Upgrade from V213 to V214
 */
static BOOL H_UpgradeFromV213(int currVersion, int newVersion)
{
	CHK_EXEC(SetColumnNullable(_T("script_library"), _T("script_code")));
	CHK_EXEC(ConvertStrings(_T("script_library"), _T("script_id"), _T("script_code")));

	CHK_EXEC(SetColumnNullable(_T("raw_dci_values"), _T("raw_value")));
	CHK_EXEC(ConvertStrings(_T("raw_dci_values"), _T("item_id"), _T("raw_value")));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='CREATE TABLE idata_%d (item_id integer not null,idata_timestamp integer not null,idata_value varchar(255) null)' WHERE var_name='IDataTableCreationCommand'")));

	DB_RESULT hResult = SQLSelect(_T("SELECT id FROM nodes"));
	if (hResult != NULL)
	{
		int count = DBGetNumRows(hResult);
		for(int i = 0; i < count; i++)
		{
			TCHAR table[32];

			DWORD nodeId = DBGetFieldULong(hResult, i, 0);
			_sntprintf(table, 32, _T("idata_%d"), nodeId);
			CHK_EXEC(SetColumnNullable(table, _T("idata_value")));
		}
		DBFreeResult(hResult);
	}

	// Convert values for string DCIs from # encoded to normal form
	hResult = SQLSelect(_T("SELECT node_id,item_id FROM items WHERE datatype=4"));
	if (hResult != NULL)
	{
		int count = DBGetNumRows(hResult);
		for(int i = 0; i < count; i++)
		{
			TCHAR query[512];

			DWORD nodeId = DBGetFieldULong(hResult, i, 0);
			DWORD dciId = DBGetFieldULong(hResult, i, 1);

			if (IsNodeExist(nodeId))
			{
				_sntprintf(query, 512, _T("SELECT idata_timestamp,idata_value FROM idata_%d WHERE item_id=%d AND idata_value LIKE '%%#%%'"), nodeId, dciId);
				DB_RESULT hData = SQLSelect(query);
				if (hData != NULL)
				{
					int valueCount = DBGetNumRows(hData);
					for(int j = 0; j < valueCount; j++)
					{
						TCHAR buffer[MAX_DB_STRING];

						LONG ts = DBGetFieldLong(hData, j, 0);
						DBGetField(hData, j, 1, buffer, MAX_DB_STRING);
						DecodeSQLString(buffer);

						_sntprintf(query, 512, _T("UPDATE idata_%d SET idata_value=%s WHERE item_id=%d AND idata_timestamp=%ld"),
									  nodeId, (const TCHAR *)DBPrepareString(g_hCoreDB, buffer), dciId, (long)ts);
						CHK_EXEC(SQLQuery(query));
					}
					DBFreeResult(hData);
				}
			}
		}
		DBFreeResult(hResult);
	}

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='214' WHERE var_name='SchemaVersion'")));
   return TRUE;
}

/**
 * Upgrade from V212 to V213
 */
static BOOL H_UpgradeFromV212(int currVersion, int newVersion)
{
	CHK_EXEC(SetColumnNullable(_T("items"), _T("custom_units_name")));
	CHK_EXEC(SetColumnNullable(_T("items"), _T("perftab_settings")));

	CHK_EXEC(ConvertStrings(_T("items"), _T("item_id"), _T("custom_units_name")));
	CHK_EXEC(ConvertStrings(_T("items"), _T("item_id"), _T("perftab_settings")));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='213' WHERE var_name='SchemaVersion'")));

   return TRUE;
}

/**
 * Upgrade from V211 to V212
 */
static BOOL H_UpgradeFromV211(int currVersion, int newVersion)
{
	CHK_EXEC(SetColumnNullable(_T("snmp_trap_cfg"), _T("snmp_oid")));
	CHK_EXEC(SetColumnNullable(_T("snmp_trap_cfg"), _T("user_tag")));
	CHK_EXEC(SetColumnNullable(_T("snmp_trap_cfg"), _T("description")));

	CHK_EXEC(ConvertStrings(_T("snmp_trap_cfg"), _T("trap_id"), _T("user_tag")));
	CHK_EXEC(ConvertStrings(_T("snmp_trap_cfg"), _T("trap_id"), _T("description")));

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='212' WHERE var_name='SchemaVersion'")));

   return TRUE;
}

/**
 * Upgrade from V210 to V211
 */
static BOOL H_UpgradeFromV210(int currVersion, int newVersion)
{
	static TCHAR batch[] = 
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) VALUES")
			_T(" (53,'SYS_DCI_UNSUPPORTED',2,1,'Status of DCI %1 (%5: %2) changed to UNSUPPORTED',")
			_T("'Generated when DCI status changed to UNSUPPORTED.#0D#0AParameters:#0D#0A")
			_T("   1) DCI ID#0D#0A   2) DCI Name#0D#0A   3) DCI Description#0D#0A   4) DCI Origin code#0D#0A   5) DCI Origin name')\n")
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) VALUES")
			_T(" (54,'SYS_DCI_DISABLED',1,1,'Status of DCI %1 (%5: %2) changed to DISABLED',")
			_T("'Generated when DCI status changed to DISABLED.#0D#0AParameters:#0D#0A")
			_T("   1) DCI ID#0D#0A   2) DCI Name#0D#0A   3) DCI Description#0D#0A   4) DCI Origin code#0D#0A   5) DCI Origin name')\n")
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) VALUES")
			_T(" (55,'SYS_DCI_ACTIVE',0,1,'Status of DCI %1 (%5: %2) changed to ACTIVE',")
			_T("'Generated when DCI status changed to ACTIVE.#0D#0AParameters:#0D#0A")
			_T("   1) DCI ID#0D#0A   2) DCI Name#0D#0A   3) DCI Description#0D#0A   4) DCI Origin code#0D#0A   5) DCI Origin name')\n")
		_T("<END>");

	CHK_EXEC(SQLBatch(batch));
	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='211' WHERE var_name='SchemaVersion'")));

   return TRUE;
}


//
// Upgrade from V209 to V210
//

static BOOL H_UpgradeFromV209(int currVersion, int newVersion)
{
	if (!SQLQuery(_T("DELETE FROM metadata WHERE var_name like 'IDataIndexCreationCommand_%'")))
		if (!g_bIgnoreErrors)
			return FALSE;

	const TCHAR *query;
	switch(g_iSyntax)
	{
		case DB_SYNTAX_PGSQL:
			query = _T("INSERT INTO metadata (var_name,var_value)	VALUES ('IDataIndexCreationCommand_0','CREATE INDEX idx_idata_%d_timestamp_id ON idata_%d(idata_timestamp,item_id)')");
			break;
		case DB_SYNTAX_MSSQL:
			query = _T("INSERT INTO metadata (var_name,var_value)	VALUES ('IDataIndexCreationCommand_0','CREATE CLUSTERED INDEX idx_idata_%d_id_timestamp ON idata_%d(item_id,idata_timestamp)')");
			break;
		default:
			query = _T("INSERT INTO metadata (var_name,var_value)	VALUES ('IDataIndexCreationCommand_0','CREATE INDEX idx_idata_%d_id_timestamp ON idata_%d(item_id,idata_timestamp)')");
			break;
	}

	if (!SQLQuery(query))
		if (!g_bIgnoreErrors)
			return FALSE;

	ReindexIData();

	if (!SQLQuery(_T("UPDATE metadata SET var_value='210' WHERE var_name='SchemaVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V208 to V209
//

static BOOL H_UpgradeFromV208(int currVersion, int newVersion)
{
	static TCHAR batch[] = 
		_T("ALTER TABLE users ADD auth_failures integer\n")
		_T("ALTER TABLE users ADD last_passwd_change integer\n")
		_T("ALTER TABLE users ADD min_passwd_length integer\n")
		_T("ALTER TABLE users ADD disabled_until integer\n")
		_T("ALTER TABLE users ADD last_login integer\n")
		_T("ALTER TABLE users ADD password_history $SQL:TEXT\n")
		_T("UPDATE users SET auth_failures=0,last_passwd_change=0,min_passwd_length=-1,disabled_until=0,last_login=0\n")
		_T("<END>");

	if (!SQLBatch(batch))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("PasswordHistoryLength"), _T("0"), 1, 0))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("IntruderLockoutThreshold"), _T("0"), 1, 0))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("IntruderLockoutTime"), _T("30"), 1, 0))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("MinPasswordLength"), _T("0"), 1, 0))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("PasswordComplexity"), _T("0"), 1, 0))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("PasswordExpiration"), _T("0"), 1, 0))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("BlockInactiveUserAccounts"), _T("0"), 1, 0))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE metadata SET var_value='209' WHERE var_name='SchemaVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V207 to V208
//

static BOOL H_UpgradeFromV207(int currVersion, int newVersion)
{
	if (!SQLQuery(_T("ALTER TABLE items ADD system_tag varchar(255)")))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!SQLQuery(_T("UPDATE metadata SET var_value='208' WHERE var_name='SchemaVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V206 to V207
//

static BOOL H_UpgradeFromV206(int currVersion, int newVersion)
{
	if (!CreateConfigParam(_T("RADIUSSecondaryServer"), _T("none"), 1, 0))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("RADIUSSecondarySecret"), _T("netxms"), 1, 0))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("RADIUSSecondaryPort"), _T("1645"), 1, 0))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("ExternalAuditServer"), _T("none"), 1, 1))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("ExternalAuditPort"), _T("514"), 1, 1))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("ExternalAuditFacility"), _T("13"), 1, 1))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("ExternalAuditSeverity"), _T("5"), 1, 1))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("ExternalAuditTag"), _T("netxmsd-audit"), 1, 1))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE metadata SET var_value='207' WHERE var_name='SchemaVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V205 to V206
//

static BOOL H_UpgradeFromV205(int currVersion, int newVersion)
{
	if (g_iSyntax == DB_SYNTAX_ORACLE)
	{
		static TCHAR oraBatch[] =
			_T("ALTER TABLE audit_log MODIFY message null\n")
			_T("ALTER TABLE event_log MODIFY event_message null\n")
			_T("ALTER TABLE event_log MODIFY user_tag null\n")
			_T("ALTER TABLE syslog MODIFY hostname null\n")
			_T("ALTER TABLE syslog MODIFY msg_tag null\n")
			_T("ALTER TABLE syslog MODIFY msg_text null\n")
			_T("ALTER TABLE snmp_trap_log MODIFY trap_varlist null\n")
			_T("<END>");

		if (!SQLBatch(oraBatch))
			if (!g_bIgnoreErrors)
				return FALSE;
	}

	bool clearLogs = GetYesNo(_T("This database upgrade requires log conversion. This can take significant amount of time ")
	                          _T("(up to few hours for large databases). If preserving all log records is not very important, it is ")
	                          _T("recommended to clear logs befor conversion. Clear logs?"));

	if (clearLogs)
	{
		if (!SQLQuery(_T("DELETE FROM audit_log")))
			if (!g_bIgnoreErrors)
				return FALSE;

		if (!SQLQuery(_T("DELETE FROM event_log")))
			if (!g_bIgnoreErrors)
				return FALSE;

		if (!SQLQuery(_T("DELETE FROM syslog")))
			if (!g_bIgnoreErrors)
				return FALSE;

		if (!SQLQuery(_T("DELETE FROM snmp_trap_log")))
			if (!g_bIgnoreErrors)
				return FALSE;
	}
	else
	{
		// Convert event log
		if (!ConvertStrings(_T("event_log"), _T("event_id"), _T("event_message")))
			if (!g_bIgnoreErrors)
				return FALSE;
		if (!ConvertStrings(_T("event_log"), _T("event_id"), _T("user_tag")))
			if (!g_bIgnoreErrors)
				return FALSE;

		// Convert audit log
		if (!ConvertStrings(_T("audit_log"), _T("record_id"), _T("message")))
			if (!g_bIgnoreErrors)
				return FALSE;

		// Convert syslog
		if (!ConvertStrings(_T("syslog"), _T("msg_id"), _T("msg_text")))
			if (!g_bIgnoreErrors)
				return FALSE;

		// Convert SNMP trap log
		if (!ConvertStrings(_T("snmp_trap_log"), _T("trap_id"), _T("trap_varlist")))
			if (!g_bIgnoreErrors)
				return FALSE;
	}

	if (!SQLQuery(_T("UPDATE metadata SET var_value='206' WHERE var_name='SchemaVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V204 to V205
//

static BOOL H_UpgradeFromV204(int currVersion, int newVersion)
{
	if (!CreateTable(_T("CREATE TABLE usm_credentials (")
		              _T("id integer not null,")
	                 _T("user_name varchar(255) not null,")
						  _T("auth_method integer not null,")
						  _T("priv_method integer not null,")
						  _T("auth_password varchar(255),")
						  _T("priv_password varchar(255),")
						  _T("PRIMARY KEY(id))")))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE metadata SET var_value='205' WHERE var_name='SchemaVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V203 to V204
//

static BOOL H_UpgradeFromV203(int currVersion, int newVersion)
{
	static TCHAR batch[] = 
		_T("ALTER TABLE object_properties ADD location_type integer\n")
		_T("ALTER TABLE object_properties ADD latitude varchar(20)\n")
		_T("ALTER TABLE object_properties ADD longitude varchar(20)\n")
		_T("UPDATE object_properties SET location_type=0\n")
		_T("ALTER TABLE object_properties DROP COLUMN image_id\n")
		_T("<END>");

	if (!SQLBatch(batch))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("ConnectionPoolBaseSize"), _T("5"), 1, 1))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("ConnectionPoolMaxSize"), _T("20"), 1, 1))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("ConnectionPoolCooldownTime"), _T("300"), 1, 1))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (g_iSyntax == DB_SYNTAX_ORACLE)
	{
		if (!SQLQuery(_T("ALTER TABLE object_properties MODIFY comments null\n")))
			if (!g_bIgnoreErrors)
				return FALSE;
	}

	if (!ConvertStrings(_T("object_properties"), _T("object_id"), _T("comments")))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!SQLQuery(_T("UPDATE metadata SET var_value='204' WHERE var_name='SchemaVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V202 to V203
//

static BOOL H_UpgradeFromV202(int currVersion, int newVersion)
{
	static TCHAR batch[] = 
		_T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,matching_oid,description,confirmation_text)")
			_T(" VALUES (20,'&Info->Topology table (LLDP)',2,'Topology Table',1,' ','Show topology table (LLDP)','#00')\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr)")
			_T(" VALUES (20,0,'Chassis ID','.1.0.8802.1.1.2.1.4.1.1.5',0,0)\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr)")
			_T(" VALUES (20,1,'Local port','.1.0.8802.1.1.2.1.4.1.1.2',5,0)\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr)")
			_T(" VALUES (20,2,'System name','.1.0.8802.1.1.2.1.4.1.1.9',0,0)\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr)")
			_T(" VALUES (20,3,'System description','.1.0.8802.1.1.2.1.4.1.1.10',0,0)\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr)")
			_T(" VALUES (20,4,'Remote port ID','.1.0.8802.1.1.2.1.4.1.1.7',4,0)\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr)")
			_T(" VALUES (20,5,'Remote port description','.1.0.8802.1.1.2.1.4.1.1.8',0,0)\n")
		_T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (20,-2147483648)\n")
		_T("<END>");

	if (!SQLBatch(batch))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE metadata SET var_value='203' WHERE var_name='SchemaVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V201 to V202
//

static BOOL H_UpgradeFromV201(int currVersion, int newVersion)
{
	if (g_iSyntax == DB_SYNTAX_ORACLE)
	{
		static TCHAR oraBatch[] =
			_T("ALTER TABLE alarms MODIFY message null\n")
			_T("ALTER TABLE alarms MODIFY alarm_key null\n")
			_T("ALTER TABLE alarms MODIFY hd_ref null\n")
			_T("<END>");

		if (!SQLBatch(oraBatch))
			if (!g_bIgnoreErrors)
				return FALSE;
	}

	if (!ConvertStrings(_T("alarms"), _T("alarm_id"), _T("message")))
      if (!g_bIgnoreErrors)
         return FALSE;
	if (!ConvertStrings(_T("alarms"), _T("alarm_id"), _T("alarm_key")))
      if (!g_bIgnoreErrors)
         return FALSE;
	if (!ConvertStrings(_T("alarms"), _T("alarm_id"), _T("hd_ref")))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!SQLQuery(_T("UPDATE metadata SET var_value='202' WHERE var_name='SchemaVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V200 to V201
//

static BOOL H_UpgradeFromV200(int currVersion, int newVersion)
{
	static TCHAR batch[] =
		_T("ALTER TABLE nodes ADD usm_auth_password varchar(127)\n")
		_T("ALTER TABLE nodes ADD usm_priv_password varchar(127)\n")
		_T("ALTER TABLE nodes ADD usm_methods integer\n")
		_T("UPDATE nodes SET usm_auth_password='#00',usm_priv_password='#00',usm_methods=0\n")
		_T("<END>");
		
	if (!SQLBatch(batch))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE metadata SET var_value='201' WHERE var_name='SchemaVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V92 to V200
//      or from V93 to V201
//      or from V94 to V202
//      or from V95 to V203
//      or from V96 to V204
//      or from V97 to V205
//      or from V98 to V206
//      or from V99 to V207
//

static BOOL H_UpgradeFromV9x(int currVersion, int newVersion)
{
	if (!CreateTable(_T("CREATE TABLE ap_common (")
		              _T("id integer not null,")
		              _T("policy_type integer not null,")
		              _T("version integer not null,")
						  _T("description $SQL:TEXT not null,")
						  _T("PRIMARY KEY(id))")))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateTable(_T("CREATE TABLE ap_bindings (")
		              _T("policy_id integer not null,")
		              _T("node_id integer not null,")
						  _T("PRIMARY KEY(policy_id,node_id))")))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateTable(_T("CREATE TABLE ap_config_files (")
		              _T("policy_id integer not null,")
		              _T("file_name varchar(63) not null,")
						  _T("file_content $SQL:TEXT not null,")
						  _T("PRIMARY KEY(policy_id))")))
		if (!g_bIgnoreErrors)
			return FALSE;

	TCHAR query[256];
	_sntprintf(query, 256, _T("UPDATE metadata SET var_value='%d' WHERE var_name='SchemaVersion'"), newVersion);
	if (!SQLQuery(query))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V100 to V214
//      or from V101 to V214
//      or from V102 to V214
//      or from V103 to V214
//      or from V104 to V214
//

static BOOL H_UpgradeFromV10x(int currVersion, int newVersion)
{
	if (!H_UpgradeFromV9x(currVersion, 207))
		return FALSE;

	// Now database at V207 level
	// V100 already has changes V209 -> V210, but missing V207 -> V209 and V210 -> V214 changes
	// V101 already has changes V209 -> V211, but missing V207 -> V209 and V211 -> V214 changes
	// V102 already has changes V209 -> V212, but missing V207 -> V209 and V212 -> V214 changes
	// V103 already has changes V209 -> V213, but missing V207 -> V209 and V213 -> V214 changes
	// V104 already has changes V209 -> V214, but missing V207 -> V209 changes

	if (!H_UpgradeFromV207(207, 208))
		return FALSE;

	if (!H_UpgradeFromV208(208, 209))
		return FALSE;

	if (currVersion == 100)
		if (!H_UpgradeFromV210(210, 211))
			return FALSE;

	if (currVersion < 102)
		if (!H_UpgradeFromV211(211, 212))
			return FALSE;

	if (currVersion < 103)
		if (!H_UpgradeFromV212(212, 213))
			return FALSE;

	if (currVersion < 104)
		if (!H_UpgradeFromV213(213, 214))
			return FALSE;

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='214' WHERE var_name='SchemaVersion'")));
   return TRUE;
}


//
// Upgrade from V105 to V217
//

static BOOL H_UpgradeFromV105(int currVersion, int newVersion)
{
	if (!H_UpgradeFromV10x(currVersion, 214))
		return FALSE;

	// V105 already have V216 -> V217 changes, but missing V207 -> V209 and V214 -> V216 changes
	if (!H_UpgradeFromV214(214, 215))
		return FALSE;

	if (!H_UpgradeFromV215(215, 216))
		return FALSE;

	CHK_EXEC(SQLQuery(_T("UPDATE metadata SET var_value='217' WHERE var_name='SchemaVersion'")));
   return TRUE;
}


//
// Upgrade from V91 to V92
//

static BOOL H_UpgradeFromV91(int currVersion, int newVersion)
{
	static TCHAR batch[] =
		_T("DROP TABLE images\n")
		_T("DROP TABLE default_images\n")
		_T("<END>");
		
	if (!SQLBatch(batch))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE metadata SET var_value='92' WHERE var_name='SchemaVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V90 to V91
//

static BOOL H_UpgradeFromV90(int currVersion, int newVersion)
{
	if (!CreateTable(_T("CREATE TABLE userdb_custom_attributes (")
		              _T("object_id integer not null,")
	                 _T("attr_name varchar(255) not null,")
						  _T("attr_value $SQL:TEXT not null,")
						  _T("PRIMARY KEY(object_id,attr_name))")))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE metadata SET var_value='91' WHERE var_name='SchemaVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V89 to V90
//

static BOOL H_UpgradeFromV89(int currVersion, int newVersion)
{
	static TCHAR m_szBatch[] =
		_T("ALTER TABLE items ADD base_units integer\n")
		_T("ALTER TABLE items ADD unit_multiplier integer\n")
		_T("ALTER TABLE items ADD custom_units_name varchar(63)\n")
		_T("ALTER TABLE items ADD perftab_settings $SQL:TEXT\n")
		_T("UPDATE items SET base_units=0,unit_multiplier=1,custom_units_name='#00',perftab_settings='#00'\n")
		_T("<END>");
		
	if (!SQLBatch(m_szBatch))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE metadata SET var_value='90' WHERE var_name='SchemaVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V88 to V89
//

static BOOL H_UpgradeFromV88(int currVersion, int newVersion)
{
	static TCHAR m_szBatch[] =
		_T("ALTER TABLE containers ADD enable_auto_bind integer\n")
		_T("ALTER TABLE containers ADD auto_bind_filter $SQL:TEXT\n")
		_T("UPDATE containers SET enable_auto_bind=0,auto_bind_filter='#00'\n")
		_T("ALTER TABLE cluster_resources ADD current_owner integer\n")
		_T("UPDATE cluster_resources SET current_owner=0\n")
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) VALUES (")
			_T("52,'SYS_DB_QUERY_FAILED',4,1,'Database query failed (Query: %1; Error: %2)',")
			_T("'Generated when SQL query to backend database failed.#0D#0A")
			_T("Parameters:#0D#0A   1) Query#0D#0A   2) Error message')\n")
		_T("<END>");
		
	if (!SQLBatch(m_szBatch))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE metadata SET var_value='89' WHERE var_name='SchemaVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V87 to V88
//

static BOOL H_UpgradeFromV87(int currVersion, int newVersion)
{
	static TCHAR m_szBatch[] =
		_T("ALTER TABLE templates ADD enable_auto_apply integer\n")
		_T("ALTER TABLE templates ADD apply_filter $SQL:TEXT\n")
		_T("UPDATE templates SET enable_auto_apply=0,apply_filter='#00'\n")
		_T("<END>");
		
	if (!SQLBatch(m_szBatch))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE metadata SET var_value='88' WHERE var_name='SchemaVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V86 to V87
//

static BOOL MoveConfigToMetadata(const TCHAR *cfgVar, const TCHAR *mdVar)
{
	TCHAR query[1024], buffer[256];
	DB_RESULT hResult;
	BOOL success;

	_sntprintf(query, 1024, _T("SELECT var_value FROM config WHERE var_name='%s'"), cfgVar);
	hResult = SQLSelect(query);
	if (hResult != NULL)
	{
		if (DBGetNumRows(hResult) > 0)
		{
			DBGetField(hResult, 0, 0, buffer, 256);
			DecodeSQLString(buffer);
			_sntprintf(query, 1024, _T("INSERT INTO metadata (var_name,var_value) VALUES ('%s','%s')"),
						  mdVar, buffer);
			DBFreeResult(hResult);
			success = SQLQuery(query);
			if (success)
			{
				_sntprintf(query, 1024, _T("DELETE FROM config WHERE var_name='%s'"), cfgVar);
				success = SQLQuery(query);
			}
		}
		else
		{
			success = TRUE;	// Variable missing in 'config' table, nothing to move
		}
	}
	else
	{
		success = FALSE;
	}
	return success;
}

static BOOL H_UpgradeFromV86(int currVersion, int newVersion)
{
	if (!CreateTable(_T("CREATE TABLE metadata (")
		              _T("var_name varchar(63) not null,")
						  _T("var_value varchar(255) not null,")
						  _T("PRIMARY KEY(var_name))")))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!MoveConfigToMetadata(_T("DBFormatVersion"), _T("SchemaVersion")))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!MoveConfigToMetadata(_T("DBSyntax"), _T("Syntax")))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!MoveConfigToMetadata(_T("IDataTableCreationCommand"), _T("IDataTableCreationCommand")))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!MoveConfigToMetadata(_T("IDataIndexCreationCommand_0"), _T("IDataIndexCreationCommand_0")))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!MoveConfigToMetadata(_T("IDataIndexCreationCommand_1"), _T("IDataIndexCreationCommand_1")))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!MoveConfigToMetadata(_T("IDataIndexCreationCommand_2"), _T("IDataIndexCreationCommand_2")))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!MoveConfigToMetadata(_T("IDataIndexCreationCommand_3"), _T("IDataIndexCreationCommand_3")))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE metadata SET var_value='87' WHERE var_name='SchemaVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V85 to V86
//

static BOOL H_UpgradeFromV85(int currVersion, int newVersion)
{
	static TCHAR m_szBatch[] =
		_T("DROP TABLE alarm_grops\n")
		_T("DROP TABLE alarm_group_map\n")
		_T("DROP TABLE alarm_change_log\n")
		_T("DROP TABLE lpp\n")
		_T("DROP TABLE lpp_associations\n")
		_T("DROP TABLE lpp_rulesets\n")
		_T("DROP TABLE lpp_rules\n")
		_T("DROP TABLE lpp_groups\n")
		_T("<END>");
		
	if (!SQLBatch(m_szBatch))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='86' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V84 to V85
//

static BOOL H_UpgradeFromV84(int currVersion, int newVersion)
{
	static TCHAR m_szBatch[] =
		_T("ALTER TABLE nodes ADD use_ifxtable integer\n")
		_T("UPDATE nodes SET use_ifxtable=0\n")
		_T("<END>");
		
	if (!SQLBatch(m_szBatch))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("UseIfXTable"), _T("1"), 1, 0))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("SMTPRetryCount"), _T("1"), 1, 0))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='85' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V83 to V84
//

static BOOL H_UpgradeFromV83(int currVersion, int newVersion)
{
	if (!CreateConfigParam(_T("EnableAgentRegistration"), _T("1"), 1, 0))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("AnonymousFileAccess"), _T("0"), 1, 0))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("EnableISCListener"), _T("0"), 1, 1))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("ReceiveForwardedEvents"), _T("0"), 1, 0))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='84' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V82 to V83
//

static BOOL H_UpgradeFromV82(int currVersion, int newVersion)
{
	// Fix incorrect alarm timeouts
	if (!SQLQuery(_T("UPDATE alarms SET timeout=0,timeout_event=43")))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='83' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V81 to V82
//

static BOOL H_UpgradeFromV81(int currVersion, int newVersion)
{
	if (!CreateTable(_T("CREATE TABLE config_clob (")
	                 _T("var_name varchar(63) not null,")
	                 _T("var_value $SQL:TEXT not null,")
	                 _T("PRIMARY KEY(var_name))")))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='82' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V80 to V81
//

static BOOL H_UpgradeFromV80(int currVersion, int newVersion)
{
	DB_RESULT hResult;
	TCHAR query[1024], buffer[1024];
	int i;
		
	// Update dci_schedules table			
	hResult = SQLSelect(_T("SELECT item_id,schedule FROM dci_schedules"));
	if (hResult != NULL)
	{
		if (!SQLQuery(_T("DROP TABLE dci_schedules")))
		   if (!g_bIgnoreErrors)
		      return FALSE;

		if (!CreateTable(_T("CREATE TABLE dci_schedules (")
							  _T("schedule_id integer not null,")
							  _T("item_id integer not null,")
							  _T("schedule varchar(255) not null,")
							  _T("PRIMARY KEY(item_id,schedule_id))")))
			if (!g_bIgnoreErrors)
				return FALSE;

		for(i = 0; i < DBGetNumRows(hResult); i++)
		{
			_sntprintf(query, 1024, _T("INSERT INTO dci_schedules (item_id,schedule_id,schedule) VALUES(%d,%d,'%s')"),
			           DBGetFieldULong(hResult, i, 0), i + 1, DBGetField(hResult, i, 1, buffer, 1024));
			if (!SQLQuery(query))
				if (!g_bIgnoreErrors)
				   return FALSE;
		}
		DBFreeResult(hResult);
	}
	else
	{
      if (!g_bIgnoreErrors)
         return FALSE;
	}
	
	// Update address_lists table			
	hResult = SQLSelect(_T("SELECT list_type,community_id,addr_type,addr1,addr2 FROM address_lists"));
	if (hResult != NULL)
	{
		if (!SQLQuery(_T("DROP TABLE address_lists")))
		   if (!g_bIgnoreErrors)
		      return FALSE;

		if (!CreateTable(_T("CREATE TABLE address_lists (")
							  _T("list_type integer not null,")
							  _T("community_id integer not null,")
							  _T("addr_type integer not null,")
							  _T("addr1 varchar(15) not null,")
							  _T("addr2 varchar(15) not null,")
							  _T("PRIMARY KEY(list_type,community_id,addr_type,addr1,addr2))")))
			if (!g_bIgnoreErrors)
				return FALSE;

		for(i = 0; i < DBGetNumRows(hResult); i++)
		{
			_sntprintf(query, 1024, _T("INSERT INTO address_lists (list_type,community_id,addr_type,addr1,addr2) VALUES(%d,%d,%d,'%s','%s')"),
			           DBGetFieldULong(hResult, i, 0), DBGetFieldULong(hResult, i, 1), 
						  DBGetFieldULong(hResult, i, 2), DBGetField(hResult, i, 3, buffer, 64),
						  DBGetField(hResult, i, 4, &buffer[128], 64));
			if (!SQLQuery(query))
				if (!g_bIgnoreErrors)
				   return FALSE;
		}

		DBFreeResult(hResult);
	}
	else
	{
      if (!g_bIgnoreErrors)
         return FALSE;
	}
         
	// Create new tables
	if (!CreateTable(_T("CREATE TABLE object_custom_attributes (")
	                 _T("object_id integer not null,")
	                 _T("attr_name varchar(127) not null,")
	                 _T("attr_value $SQL:TEXT not null,")
	                 _T("PRIMARY KEY(object_id,attr_name))")))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!CreateTable(_T("CREATE TABLE web_maps (")
	                 _T("id integer not null,")
	                 _T("title varchar(63) not null,")
	                 _T("properties $SQL:TEXT not null,")
	                 _T("data $SQL:TEXT not null,")
	                 _T("PRIMARY KEY(id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='81' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V79 to V80
//

static BOOL H_UpgradeFromV79(int currVersion, int newVersion)
{
	static TCHAR m_szBatch[] =
		_T("ALTER TABLE nodes ADD uname varchar(255)\n")
		_T("UPDATE nodes SET uname='#00'\n")
		_T("<END>");
		
	if (!SQLBatch(m_szBatch))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='80' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V78 to V79
//

static BOOL H_UpgradeFromV78(int currVersion, int newVersion)
{
	static TCHAR m_szBatch[] =
		_T("DELETE FROM config WHERE var_name='RetainCustomInterfaceNames'\n")
		_T("DROP TABLE modules\n")
		_T("<END>");
	static TCHAR m_szMySQLBatch[] =
		_T("ALTER TABLE users MODIFY COLUMN cert_mapping_data text not null\n")
		_T("ALTER TABLE user_profiles MODIFY COLUMN var_value text not null\n")
		_T("ALTER TABLE object_properties MODIFY COLUMN comments text not null\n")
		_T("ALTER TABLE network_services MODIFY COLUMN check_request text not null\n")
		_T("ALTER TABLE network_services MODIFY COLUMN check_responce text not null\n")
		_T("ALTER TABLE conditions MODIFY COLUMN script text not null\n")
		_T("ALTER TABLE container_categories MODIFY COLUMN description text not null\n")
		_T("ALTER TABLE items MODIFY COLUMN transformation text not null\n")
		_T("ALTER TABLE event_cfg MODIFY COLUMN description text not null\n")
		_T("ALTER TABLE actions MODIFY COLUMN action_data text not null\n")
		_T("ALTER TABLE event_policy MODIFY COLUMN comments text not null\n")
		_T("ALTER TABLE event_policy MODIFY COLUMN script text not null\n")
		_T("ALTER TABLE alarm_change_log MODIFY COLUMN info_text text not null\n")
		_T("ALTER TABLE alarm_notes MODIFY COLUMN note_text text not null\n")
		_T("ALTER TABLE object_tools MODIFY COLUMN tool_data text not null\n")
		_T("ALTER TABLE syslog MODIFY COLUMN msg_text text not null\n")
		_T("ALTER TABLE script_library MODIFY COLUMN script_code text not null\n")
		_T("ALTER TABLE snmp_trap_log MODIFY COLUMN trap_varlist text not null\n")
		_T("ALTER TABLE maps MODIFY COLUMN description text not null\n")
		_T("ALTER TABLE agent_configs MODIFY COLUMN config_file text not null\n")
		_T("ALTER TABLE agent_configs MODIFY COLUMN config_filter text not null\n")
		_T("ALTER TABLE graphs MODIFY COLUMN config text not null\n")
		_T("ALTER TABLE certificates MODIFY COLUMN cert_data text not null\n")
		_T("ALTER TABLE certificates MODIFY COLUMN subject text not null\n")
		_T("ALTER TABLE certificates MODIFY COLUMN comments text not null\n")
		_T("ALTER TABLE audit_log MODIFY COLUMN message text not null\n")
		_T("ALTER TABLE situations MODIFY COLUMN comments text not null\n")
		_T("<END>");

	if (!SQLBatch(m_szBatch))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (g_iSyntax == DB_SYNTAX_MYSQL)
	{
		if (!SQLBatch(m_szMySQLBatch))
			if (!g_bIgnoreErrors)
				return FALSE;
	}

	if (!SQLQuery(_T("UPDATE config SET var_value='79' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V77 to V78
//

static BOOL H_UpgradeFromV77(int currVersion, int newVersion)
{
	if (!CreateTable(_T("CREATE TABLE trusted_nodes (")
	                 _T("source_object_id integer not null,")
	                 _T("target_node_id integer not null,")
	                 _T("PRIMARY KEY(source_object_id,target_node_id))")))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("CheckTrustedNodes"), _T("1"), 1, 1))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='78' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V76 to V77
//

static BOOL H_UpgradeFromV76(int currVersion, int newVersion)
{
	DB_RESULT hResult;
	int i, count, seq;
	DWORD id, lastId;
	TCHAR query[1024];

	hResult = SQLSelect(_T("SELECT condition_id,dci_id,node_id,dci_func,num_polls FROM cond_dci_map ORDER BY condition_id"));
	if (hResult == NULL)
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("DROP TABLE cond_dci_map")))
		if (!g_bIgnoreErrors)
			goto error;

	if (!CreateTable(_T("CREATE TABLE cond_dci_map (")
	                 _T("condition_id integer not null,")
	                 _T("sequence_number integer not null,")
	                 _T("dci_id integer not null,")
	                 _T("node_id integer not null,")
	                 _T("dci_func integer not null,")
	                 _T("num_polls integer not null,")
	                 _T("PRIMARY KEY(condition_id,sequence_number))")))
		if (!g_bIgnoreErrors)
			goto error;

	count = DBGetNumRows(hResult);
	for(i = 0, seq = 0, lastId = 0; i < count; i++, seq++)
	{
		id = DBGetFieldULong(hResult, i, 0);
		if (id != lastId)
		{
			seq = 0;
			lastId = id;
		}
		_sntprintf(query, 1024, _T("INSERT INTO cond_dci_map (condition_id,sequence_number,dci_id,node_id,dci_func,num_polls) VALUES (%d,%d,%d,%d,%d,%d)"),
		           id, seq, DBGetFieldULong(hResult, i, 1), DBGetFieldULong(hResult, i, 2),
					  DBGetFieldULong(hResult, i, 3), DBGetFieldULong(hResult, i, 4));
		if (!SQLQuery(query))
			if (!g_bIgnoreErrors)
				goto error;
	}

	DBFreeResult(hResult);

	if (!SQLQuery(_T("UPDATE config SET var_value='77' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;

error:
	DBFreeResult(hResult);
	return FALSE;
}


//
// Upgrade from V75 to V76
//

static BOOL H_UpgradeFromV75(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) VALUES (")
			_T("50,'SYS_NETWORK_CONN_LOST',4,1,'NetXMS server network connectivity lost',")
			_T("'Generated when system detects loss of network connectivity based on beacon ")
			_T("probing.#0D#0AParameters:#0D#0A   1) Number of beacons')\n")
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) VALUES (")
			_T("51,'SYS_NETWORK_CONN_RESTORED',0,1,'NetXMS server network connectivity restored',")
			_T("'Generated when system detects restoration of network connectivity based on ")
			_T("beacon probing.#0D#0AParameters:#0D#0A   1) Number of beacons')\n")
      _T("<END>");

	if (!SQLBatch(m_szBatch))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("AgentCommandTimeout"), _T("2000"), 1, 1))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("BeaconHosts"), _T(""), 1, 1))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("BeaconTimeout"), _T("1000"), 1, 1))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("BeaconPollingInterval"), _T("1000"), 1, 1))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='76' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V74 to V75
//

static BOOL H_UpgradeFromV74(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("ALTER TABLE address_lists ADD community_id integer\n")
		_T("UPDATE address_lists SET community_id=0\n")
      _T("<END>");

	if (!SQLBatch(m_szBatch))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateTable(_T("CREATE TABLE snmp_communities (")
	                 _T("id integer not null,")
	                 _T("community varchar(255) not null,")
	                 _T("PRIMARY KEY(id))")))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("UseInterfaceAliases"), _T("0"), 1, 0))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!CreateConfigParam(_T("SyncNodeNamesWithDNS"), _T("0"), 1, 0))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='75' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V73 to V74
//

static BOOL H_UpgradeFromV73(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) ")
			_T("VALUES (48,'SYS_EVENT_STORM_DETECTED',3,1,'Event storm detected (Events per second: %1)',")
			_T("'Generated when system detects an event storm.#0D#0AParameters:#0D#0A")
			_T("   1) Events per second#0D#0A   2) Duration#0D#0A   3) Threshold')\n")
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) ")
			_T("VALUES (49,'SYS_EVENT_STORM_ENDED',0,1,'Event storm ended',")
			_T("'Generated when system clears event storm condition.#0D#0AParameters:#0D#0A")
			_T("   1) Events per second#0D#0A   2) Duration#0D#0A   3) Threshold')\n")
		_T("DELETE FROM config WHERE var_name='NumberOfEventProcessors'\n")
		_T("DELETE FROM config WHERE var_name='EventStormThreshold'\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("EnableEventStormDetection"), _T("0"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!CreateConfigParam(_T("EventStormEventsPerSecond"), _T("100"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!CreateConfigParam(_T("EventStormDuration"), _T("15"), 1, 1))
		if (!g_bIgnoreErrors)
			return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='74' WHERE var_name='DBFormatVersion'")))
		if (!g_bIgnoreErrors)
			return FALSE;

   return TRUE;
}


//
// Upgrade from V72 to V73
//

static BOOL H_UpgradeFromV72(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("ALTER TABLE event_policy ADD situation_id integer\n")
		_T("ALTER TABLE event_policy ADD situation_instance varchar(255)\n")
		_T("UPDATE event_policy SET situation_id=0,situation_instance='#00'\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!CreateTable(_T("CREATE TABLE policy_situation_attr_list (")
	                 _T("rule_id integer not null,")
	                 _T("situation_id integer not null,")
	                 _T("attr_name varchar(255) not null,")
	                 _T("attr_value varchar(255) not null,")
	                 _T("PRIMARY KEY(rule_id,situation_id,attr_name))")))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!CreateTable(_T("CREATE TABLE situations (")
	                 _T("id integer not null,")
	                 _T("name varchar(127) not null,")
	                 _T("comments $SQL:TEXT not null,")
	                 _T("PRIMARY KEY(id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("RetainCustomInterfaceNames"), _T("0"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("AllowDirectSMS"), _T("0"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("EventStormThreshold"), _T("0"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='73' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V71 to V72
//

static BOOL H_UpgradeFromV71(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("ALTER TABLE items ADD proxy_node integer\n")
		_T("UPDATE items SET proxy_node=0\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='72' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V70 to V71
//

static BOOL H_UpgradeFromV70(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("ALTER TABLE nodes ADD required_polls integer\n")
		_T("UPDATE nodes SET required_polls=0\n")
		_T("ALTER TABLE interfaces ADD required_polls integer\n")
		_T("UPDATE interfaces SET required_polls=0\n")
		_T("ALTER TABLE network_services ADD required_polls integer\n")
		_T("UPDATE network_services SET required_polls=0\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("PollCountForStatusChange"), _T("1"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='71' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V69 to V70
//

static BOOL H_UpgradeFromV69(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("ALTER TABLE snmp_trap_cfg ADD user_tag varchar(63)\n")
		_T("UPDATE snmp_trap_cfg SET user_tag='#00'\n")
		_T("ALTER TABLE event_log ADD user_tag varchar(63)\n")
		_T("UPDATE event_log SET user_tag='#00'\n")
      _T("<END>");
	int n;
	TCHAR buffer[64];

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

	// Convert event log retention time from seconds to days
	n = ConfigReadInt(_T("EventLogRetentionTime"), 5184000) / 86400;
	_sntprintf(buffer, 64, _T("%d"), max(n, 1));
   if (!CreateConfigParam(_T("EventLogRetentionTime"), buffer, 1, 0, TRUE))
      if (!g_bIgnoreErrors)
         return FALSE;

	// Convert event log retention time from seconds to days
	n = ConfigReadInt(_T("SyslogRetentionTime"), 5184000) / 86400;
	_sntprintf(buffer, 64, _T("%d"), max(n, 1));
   if (!CreateConfigParam(_T("SyslogRetentionTime"), buffer, 1, 0, TRUE))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='70' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V68 to V69
//

static BOOL H_UpgradeFromV68(int currVersion, int newVersion)
{
	if (!CreateTable(_T("CREATE TABLE audit_log (")
	                 _T("record_id integer not null,")
	                 _T("timestamp integer not null,")
	                 _T("subsystem varchar(32) not null,")
	                 _T("success integer not null,")
	                 _T("user_id integer not null,")
	                 _T("workstation varchar(63) not null,")
	                 _T("object_id integer not null,")
	                 _T("message $SQL:TEXT not null,")
	                 _T("PRIMARY KEY(record_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("EnableAuditLog"), _T("1"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("AuditLogRetentionTime"), _T("90"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='69' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V67 to V68
//

static BOOL H_UpgradeFromV67(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("ALTER TABLE thresholds ADD repeat_interval integer\n")
		_T("UPDATE thresholds SET repeat_interval=-1\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("ThresholdRepeatInterval"), _T("0"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='68' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V66 to V67
//

static BOOL H_UpgradeFromV66(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("ALTER TABLE subnets ADD synthetic_mask integer\n")
		_T("UPDATE subnets SET synthetic_mask=0\n")
		_T("ALTER TABLE interfaces ADD synthetic_mask integer\n")
		_T("UPDATE interfaces SET synthetic_mask=0\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='67' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V65 to V66
//

static BOOL H_UpgradeFromV65(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("ALTER TABLE submap_links ADD port1 varchar(255)\n")
		_T("ALTER TABLE submap_links ADD port2 varchar(255)\n")
		_T("UPDATE submap_links SET port1='#00',port2='#00'\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='66' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V64 to V65
//

static BOOL H_UpgradeFromV64(int currVersion, int newVersion)
{
   static TCHAR m_szPGSQLBatch[] =
		_T("ALTER TABLE nodes ADD new_community varchar(127)\n")
		_T("UPDATE nodes SET new_community=community\n")
		_T("ALTER TABLE nodes DROP COLUMN community\n")
		_T("ALTER TABLE nodes RENAME COLUMN new_community TO community\n")
		_T("ALTER TABLE nodes ALTER COLUMN community SET NOT NULL\n")
      _T("<END>");

	switch(g_iSyntax)
	{
		case DB_SYNTAX_MYSQL:
		case DB_SYNTAX_ORACLE:
			if (!SQLQuery(_T("ALTER TABLE nodes MODIFY community varchar(127)")))
		      if (!g_bIgnoreErrors)
					return FALSE;
			break;
		case DB_SYNTAX_PGSQL:
			if (g_bTrace)
				ShowQuery(_T("ALTER TABLE nodes ALTER COLUMN community TYPE varchar(127)"));

			if (!DBQuery(g_hCoreDB, _T("ALTER TABLE nodes ALTER COLUMN community TYPE varchar(127)")))
			{
				// Assume that we are using PostgreSQL oldest than 8.x
				if (!SQLBatch(m_szPGSQLBatch))
					if (!g_bIgnoreErrors)
						return FALSE;
			}
			break;
		case DB_SYNTAX_MSSQL:
			if (!SQLQuery(_T("ALTER TABLE nodes ALTER COLUMN community varchar(127)")))
		      if (!g_bIgnoreErrors)
					return FALSE;
			break;
		case DB_SYNTAX_SQLITE:
			_tprintf(_T("WARNING: Due to limitations of SQLite requested operation cannot be completed\nYou system will still be limited to use SNMP commonity strings not longer than 32 characters.\n"));
			break;
		default:
			_tprintf(_T("INTERNAL ERROR: Unknown database syntax %d\n"), g_iSyntax);
			break;
	}

	if (!SQLQuery(_T("UPDATE config SET var_value='65' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V63 to V64
//

static BOOL H_UpgradeFromV63(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) VALUES (15,'.1.3.6.1.4.1.45.3.29.*',3,0)\n")
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) VALUES (16,'.1.3.6.1.4.1.45.3.41.*',3,0)\n")
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) VALUES (17,'.1.3.6.1.4.1.45.3.45.*',3,0)\n")
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) VALUES (18,'.1.3.6.1.4.1.45.3.43.*',3,0)\n")
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) VALUES (19,'.1.3.6.1.4.1.45.3.57.*',3,0)\n")
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) VALUES (20,'.1.3.6.1.4.1.45.3.49.*',3,0)\n")
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) VALUES (21,'.1.3.6.1.4.1.45.3.54.*',3,0)\n")
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) VALUES (22,'.1.3.6.1.4.1.45.3.63.*',3,0)\n")
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) VALUES (23,'.1.3.6.1.4.1.45.3.64.*',3,0)\n")
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) VALUES (24,'.1.3.6.1.4.1.45.3.53.*',3,0)\n")
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) VALUES (25,'.1.3.6.1.4.1.45.3.59.*',3,0)\n")
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) VALUES (26,'.1.3.6.1.4.1.45.3.39.*',3,0)\n")
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) VALUES (27,'.1.3.6.1.4.1.45.3.65.*',3,0)\n")
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) VALUES (28,'.1.3.6.1.4.1.45.3.66.*',3,0)\n")
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) VALUES (29,'.1.3.6.1.4.1.45.3.44.*',4,0)\n")
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) VALUES (30,'.1.3.6.1.4.1.45.3.47.*',4,0)\n")
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) VALUES (31,'.1.3.6.1.4.1.45.3.48.*',4,0)\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='64' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V62 to V63
//

static BOOL H_UpgradeFromV62(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) ")
			_T("VALUES (45,'SYS_IF_UNKNOWN',1,1,")
			_T("'Interface \"%2\" changed state to UNKNOWN (IP Addr: %3/%4, IfIndex: %5)',")
			_T("'Generated when interface goes to unknown state.#0D#0A")
			_T("Please note that source of event is node, not an interface itself.#0D#0A")
			_T("Parameters:#0D#0A   1) Interface object ID#0D#0A   2) Interface name#0D#0A")
			_T("   3) Interface IP address#0D#0A   4) Interface netmask#0D#0A")
			_T("   5) Interface index')\n")
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) ")
			_T("VALUES (46,'SYS_IF_DISABLED',0,1,")
			_T("'Interface \"%2\" disabled (IP Addr: %3/%4, IfIndex: %5)',")
			_T("'Generated when interface administratively disabled.#0D#0A")
			_T("Please note that source of event is node, not an interface itself.#0D#0A")
			_T("Parameters:#0D#0A   1) Interface object ID#0D#0A   2) Interface name#0D#0A")
			_T("   3) Interface IP address#0D#0A   4) Interface netmask#0D#0A")
			_T("   5) Interface index')\n")
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) ")
			_T("VALUES (47,'SYS_IF_TESTING',0,1,")
			_T("'Interface \"%2\" is testing (IP Addr: %3/%4, IfIndex: %5)',")
			_T("'Generated when interface goes to testing state.#0D#0A")
			_T("Please note that source of event is node, not an interface itself.#0D#0A")
			_T("Parameters:#0D#0A   1) Interface object ID#0D#0A   2) Interface name#0D#0A")
			_T("   3) Interface IP address#0D#0A   4) Interface netmask#0D#0A")
			_T("   5) Interface index')\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='63' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V61 to V62
//

static BOOL H_UpgradeFromV61(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("UPDATE event_policy SET alarm_key=alarm_ack_key WHERE alarm_severity=6\n")
		_T("ALTER TABLE event_policy DROP COLUMN alarm_ack_key\n")
		_T("ALTER TABLE event_policy ADD alarm_timeout integer\n")
		_T("ALTER TABLE event_policy ADD alarm_timeout_event integer\n")
		_T("UPDATE event_policy SET alarm_timeout=0,alarm_timeout_event=43\n")
		_T("ALTER TABLE alarms ADD timeout integer\n")
		_T("ALTER TABLE alarms ADD timeout_event integer\n")
		_T("UPDATE alarms SET timeout=0,timeout_event=43\n")
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) ")
			_T("VALUES (43,'SYS_ALARM_TIMEOUT',1,1,'Alarm timeout expired (ID: %1; Text: %2)',")
			_T("'Generated when alarm timeout expires.#0D#0AParameters:#0D#0A")
			_T("   1) Alarm ID#0D#0A   2) Alarm message#0D#0A   3) Alarm key#0D#0A   4) Event code')\n")
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) ")
			_T("VALUES (44,'SYS_LOG_RECORD_MATCHED',1,1,")
			_T("'Log record matched (Policy: %1; File: %2; Record: %4)',")
			_T("'Default event for log record match.#0D#0AParameters:#0D#0A")
			_T("   1) Policy name#0D#0A   2) Log file name#0D#0A   3) Matching regular expression#0D#0A")
			_T("   4) Matched record#0D#0A   5 .. 9) Reserved#0D#0A")
			_T("   10 .. 99) Substrings extracted by regular expression')\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!CreateTable(_T("CREATE TABLE lpp_groups (")
	                 _T("lpp_group_id integer not null,")
	                 _T("lpp_group_name varchar(63) not null,")
	                 _T("parent_group integer not null,")
	                 _T("PRIMARY KEY(lpp_group_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!CreateTable(_T("CREATE TABLE lpp (")
	                 _T("lpp_id integer not null,")
	                 _T("lpp_group_id integer not null,")
	                 _T("lpp_name varchar(63) not null,")
	                 _T("lpp_version integer not null,")
	                 _T("lpp_flags integer not null,")
	                 _T("PRIMARY KEY(lpp_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!CreateTable(_T("CREATE TABLE lpp_associations (")
	                 _T("lpp_id integer not null,")
	                 _T("node_id integer not null,")
	                 _T("log_file varchar(255) not null)")))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!CreateTable(_T("CREATE TABLE lpp_rulesets (")
	                 _T("ruleset_id integer not null,")
	                 _T("ruleset_name varchar(63),")
	                 _T("PRIMARY KEY(ruleset_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!CreateTable(_T("CREATE TABLE lpp_rules (")
	                 _T("lpp_id integer not null,")
	                 _T("rule_number integer not null,")
	                 _T("ruleset_id integer not null,")
	                 _T("msg_id_start integer not null,")
	                 _T("msg_id_end integer not null,")
	                 _T("severity integer not null,")
	                 _T("source_name varchar(255) not null,")
	                 _T("msg_text_regexp varchar(255) not null,")
	                 _T("event_code integer not null,")
	                 _T("PRIMARY KEY(lpp_id,rule_number))")))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!SQLQuery(_T("UPDATE config SET var_value='62' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V60 to V61
//

static BOOL H_UpgradeFromV60(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("DELETE FROM object_tools WHERE tool_id=14\n")
		_T("DELETE FROM object_tools_table_columns WHERE tool_id=14\n")
		_T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,matching_oid,description,confirmation_text) ")
			_T("VALUES (14,'&Info->Topology table (Nortel)',2,'Topology table',1,' ','Show topology table (Nortel protocol)','#00')\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
			_T("VALUES (14,0,'Peer IP','.1.3.6.1.4.1.45.1.6.13.2.1.1.3',3,0)\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
			_T("VALUES (14,1,'Peer MAC','.1.3.6.1.4.1.45.1.6.13.2.1.1.5',4,0)\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
			_T("VALUES (14,2,'Slot','.1.3.6.1.4.1.45.1.6.13.2.1.1.1',1,0)\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
			_T("VALUES (14,3,'Port','.1.3.6.1.4.1.45.1.6.13.2.1.1.2',1,0)\n")
		_T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,matching_oid,description,confirmation_text) ")
			_T("VALUES (17,'&Info->AR&P cache (SNMP)',2,'ARP Cache',1,' ','Show ARP cache','#00')\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
			_T("VALUES (17,0,'IP Address','.1.3.6.1.2.1.4.22.1.3',3,0)\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
			_T("VALUES (17,1,'MAC Address','.1.3.6.1.2.1.4.22.1.2',4,0)\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
			_T("VALUES (17,2,'Interface','.1.3.6.1.2.1.4.22.1.1',5,0)\n")
		_T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,matching_oid,description,confirmation_text) ")
			_T("VALUES (18,'&Info->AR&P cache (Agent)',3,")
			_T("'ARP Cache#7FNet.ArpCache#7F(.*) (.*) (.*)',2,' ','Show ARP cache','#00')\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
			_T("VALUES (18,0,'IP Address','.1.3.6.1.2.1.4.22.1.3',0,2)\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
			_T("VALUES (18,1,'MAC Address','.1.3.6.1.2.1.4.22.1.2',0,1)\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
			_T("VALUES (18,2,'Interface','.1.3.6.1.2.1.4.22.1.1',5,3)\n")
		_T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,matching_oid,description,confirmation_text) ")
			_T("VALUES (19,'&Info->&Routing table (SNMP)',2,'Routing Table',1,' ','Show IP routing table','#00')\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
			_T("VALUES (19,0,'Destination','.1.3.6.1.2.1.4.21.1.1',3,0)\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
			_T("VALUES (19,1,'Mask','.1.3.6.1.2.1.4.21.1.11',3,0)\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
			_T("VALUES (19,2,'Next hop','.1.3.6.1.2.1.4.21.1.7',3,0)\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
			_T("VALUES (19,3,'Metric','.1.3.6.1.2.1.4.21.1.3',1,0)\n")
		_T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
			_T("VALUES (19,4,'Interface','.1.3.6.1.2.1.4.21.1.2',5,0)\n")
		_T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (17,-2147483648)\n")
		_T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (18,-2147483648)\n")
		_T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (19,-2147483648)\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("TopologyExpirationTime"), _T("900"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("TopologyDiscoveryRadius"), _T("3"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='61' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V59 to V60
//

static BOOL H_UpgradeFromV59(int currVersion, int newVersion)
{
	if (!CreateTable(_T("CREATE TABLE certificates (")
	                 _T("cert_id integer not null,")
	                 _T("cert_type integer not null,")
	                 _T("cert_data $SQL:TEXT not null,")
	                 _T("subject $SQL:TEXT not null,")
	                 _T("comments $SQL:TEXT not null,")
	                 _T("PRIMARY KEY(cert_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("SNMPRequestTimeout"), _T("2000"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='60' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V58 to V59
//

static BOOL H_UpgradeFromV58(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("ALTER TABLE users ADD cert_mapping_method integer\n")
		_T("ALTER TABLE users ADD cert_mapping_data $SQL:TEXT\n")
		_T("UPDATE users SET cert_mapping_method=0\n")
		_T("UPDATE users SET cert_mapping_data='#00'\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("InternalCA"), _T("0"), 1, 1))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='59' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V57 to V58
//

static BOOL H_UpgradeFromV57(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("ALTER TABLE object_properties ADD is_system integer\n")
		_T("UPDATE object_properties SET is_system=0\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!CreateTable(_T("CREATE TABLE graphs (")
	                 _T("graph_id integer not null,")
	                 _T("owner_id integer not null,")
	                 _T("name varchar(255) not null,")
	                 _T("config $SQL:TEXT not null,")
	                 _T("PRIMARY KEY(graph_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!CreateTable(_T("CREATE TABLE graph_acl (")
	                 _T("graph_id integer not null,")
	                 _T("user_id integer not null,")
	                 _T("user_rights integer not null,")
	                 _T("PRIMARY KEY(graph_id,user_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='58' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V56 to V57
//

static BOOL H_UpgradeFromV56(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("ALTER TABLE items ADD resource_id integer\n")
		_T("UPDATE items SET resource_id=0\n")
		_T("ALTER TABLE nodes ADD snmp_proxy integer\n")
		_T("UPDATE nodes SET snmp_proxy=0\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='57' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V55 to V56
//

static BOOL H_UpgradeFromV55(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) VALUES")
			_T(" (38,'SYS_CLUSTER_RESOURCE_MOVED',1,1,")
			_T("'Cluster resource \"%2\" moved from node %4 to node %6',")
			_T("'Generated when cluster resource moved between nodes.#0D#0A")
			_T("Parameters:#0D#0A   1) Resource ID#0D#0A")
			_T("   2) Resource name#0D#0A   3) Previous owner node ID#0D#0A")
			_T("   4) Previous owner node name#0D#0A   5) New owner node ID#0D#0A")
			_T("   6) New owner node name')\n")
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) VALUES")
			_T(" (39,'SYS_CLUSTER_RESOURCE_DOWN',3,1,")
			_T("'Cluster resource \"%2\" is down (last owner was %4)',")
			_T("'Generated when cluster resource goes down.#0D#0A")
			_T("Parameters:#0D#0A   1) Resource ID#0D#0A   2) Resource name#0D#0A")
			_T("   3) Last owner node ID#0D#0A   4) Last owner node name')\n")
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) VALUES")
			_T(" (40,'SYS_CLUSTER_RESOURCE_UP',0,1,")
			_T("'Cluster resource \"%2\" is up (new owner is %4)',")
			_T("'Generated when cluster resource goes up.#0D#0A")
			_T("Parameters:#0D#0A   1) Resource ID#0D#0A   2) Resource name#0D#0A")
			_T("   3) New owner node ID#0D#0A   4) New owner node name')\n")
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) VALUES")
			_T(" (41,'SYS_CLUSTER_DOWN',4,1,'Cluster is down',")
			_T("'Generated when cluster goes down.#0D#0AParameters:#0D#0A   No message-specific parameters')\n")
		_T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) VALUES")
			_T(" (42,'SYS_CLUSTER_UP',0,1,'Cluster is up',")
			_T("'Generated when cluster goes up.#0D#0AParameters:#0D#0A   No message-specific parameters')\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!CreateTable(_T("CREATE TABLE cluster_resources (")
	                 _T("cluster_id integer not null,")
	                 _T("resource_id integer not null,")
	                 _T("resource_name varchar(255) not null,")
	                 _T("ip_addr varchar(15) not null,")
	                 _T("PRIMARY KEY(cluster_id,resource_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='56' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V54 to V55
//

static BOOL H_UpgradeFromV54(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
		_T("ALTER TABLE containers DROP COLUMN description\n")
		_T("ALTER TABLE nodes DROP COLUMN description\n")
		_T("ALTER TABLE templates DROP COLUMN description\n")
		_T("ALTER TABLE zones DROP COLUMN description\n")
		_T("INSERT INTO images (image_id,name,file_name_png,file_hash_png,")
			_T("file_name_ico,file_hash_ico) VALUES (16,'Obj.Cluster',")
			_T("'cluster.png','<invalid_hash>','cluster.ico','<invalid_hash>')\n")
		_T("INSERT INTO default_images (object_class,image_id) VALUES (14,16)\n")
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) ")
			_T("VALUES (12,'.1.3.6.1.4.1.45.3.46.*',3,0)\n")
		_T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) ")
			_T("VALUES (13,'.1.3.6.1.4.1.45.3.52.*',3,0)\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE clusters (")
	                 _T("id integer not null,")
	                 _T("cluster_type integer not null,")
	                 _T("PRIMARY KEY(id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!CreateTable(_T("CREATE TABLE cluster_members (")
	                 _T("cluster_id integer not null,")
	                 _T("node_id integer not null,")
	                 _T("PRIMARY KEY(cluster_id,node_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

	if (!CreateTable(_T("CREATE TABLE cluster_sync_subnets (")
	                 _T("cluster_id integer not null,")
	                 _T("subnet_addr varchar(15) not null,")
	                 _T("subnet_mask varchar(15) not null,")
	                 _T("PRIMARY KEY(cluster_id,subnet_addr))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateConfigParam(_T("WindowsConsoleUpgradeURL"), _T("http://www.netxms.org/download/netxms-%version%.exe"), 1, 0))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLQuery(_T("UPDATE config SET var_value='55' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;

   return TRUE;
}


//
// Upgrade from V53 to V54
//

static BOOL H_UpgradeFromV53(int currVersion, int newVersion)
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

   if (!CreateConfigParam(_T("ActiveDiscoveryInterval"), _T("7200"), 1, 1))
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

static BOOL H_UpgradeFromV52(int currVersion, int newVersion)
{
   DB_RESULT hResult;
   int i, nCount;
   DWORD dwId;
   TCHAR szQuery[1024];
   static const TCHAR *pszNewIdx[] =
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
      _tprintf(_T("   * idata_%d\n"), dwId);

      // Drop old indexes
      _sntprintf(szQuery, 1024, _T("DROP INDEX idx_idata_%d_timestamp"), dwId);
      DBQuery(g_hCoreDB, szQuery);

      // Create new index
      _sntprintf(szQuery, 1024, pszNewIdx[g_iSyntax], dwId, dwId);
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

static BOOL H_UpgradeFromV51(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
	   _T("UPDATE object_tools SET tool_data='Configured ICMP targets#7FICMP.TargetList#7F^(.*) (.*) (.*) (.*) (.*)' WHERE tool_id=12\n")
	   _T("UPDATE object_tools_table_columns SET col_number=4 WHERE col_number=3 AND tool_id=12\n")
	   _T("UPDATE object_tools_table_columns SET col_number=3 WHERE col_number=2 AND tool_id=12\n")
   	_T("UPDATE object_tools_table_columns SET col_substr=5 WHERE col_number=1 AND tool_id=12\n")
	   _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
		   _T("VALUES (12,2,'Packet size','',0,4)\n")
      _T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,matching_oid,description,confirmation_text) ")
	      _T("VALUES (16,'&Info->Active &user sessions',3,")
            _T("'Active User Sessions#7FSystem.ActiveUserSessions#7F^\"(.*)\" \"(.*)\" \"(.*)\"',")
            _T("2,'','Show list of active user sessions','#00')\n")
      _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
	      _T("VALUES (16,0,'User','',0,1)\n")
      _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
	      _T("VALUES (16,1,'Terminal','',0,2)\n")
      _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
	      _T("VALUES (16,2,'From','',0,3)\n")
      _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (16,-2147483648)\n")
      _T("<END>");

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

static BOOL H_UpgradeFromV50(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE event_groups ADD range_start integer\n")
      _T("ALTER TABLE event_groups ADD range_END integer\n")
      _T("UPDATE event_groups SET range_start=0,range_end=0\n")
      _T("<END>");

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

static BOOL H_UpgradeFromV49(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE object_tools ADD confirmation_text varchar(255)\n")
      _T("UPDATE object_tools SET confirmation_text='#00'\n")
      _T("UPDATE object_tools SET flags=10 WHERE tool_id=1 OR tool_id=2 OR tool_id=4\n")
      _T("UPDATE object_tools SET confirmation_text='Host #25OBJECT_NAME#25 (#25OBJECT_IP_ADDR#25) will be shut down. Are you sure?' WHERE tool_id=1\n")
      _T("UPDATE object_tools SET confirmation_text='Host #25OBJECT_NAME#25 (#25OBJECT_IP_ADDR#25) will be restarted. Are you sure?' WHERE tool_id=2\n")
      _T("UPDATE object_tools SET confirmation_text='NetXMS agent on host #25OBJECT_NAME#25 (#25OBJECT_IP_ADDR#25) will be restarted. Are you sure?' WHERE tool_id=4\n")
      _T("<END>");

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

static BOOL H_UpgradeFromV48(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE items ADD all_thresholds integer\n")
      _T("UPDATE items SET all_thresholds=0\n")
      _T("ALTER TABLE thresholds ADD rearm_event_code integer\n")
      _T("UPDATE thresholds SET rearm_event_code=18\n")
      _T("<END>");

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

static BOOL H_UpgradeFromV47(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE event_policy ADD script $SQL:TEXT\n")
      _T("UPDATE event_policy SET script='#00'\n")
      _T("<END>");

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

static BOOL H_UpgradeFromV46(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE object_properties ADD comments $SQL:TEXT\n")
      _T("UPDATE object_properties SET comments='#00'\n")
      _T("ALTER TABLE nodes DROP COLUMN discovery_flags\n")
      _T("DROP TABLE alarm_notes\n")
      _T("ALTER TABLE alarms ADD alarm_state integer\n")
      _T("ALTER TABLE alarms ADD hd_state integer\n")
      _T("ALTER TABLE alarms ADD hd_ref varchar(63)\n")
      _T("ALTER TABLE alarms ADD creation_time integer\n")
      _T("ALTER TABLE alarms ADD last_change_time integer\n")
      _T("ALTER TABLE alarms ADD original_severity integer\n")
      _T("ALTER TABLE alarms ADD current_severity integer\n")
      _T("ALTER TABLE alarms ADD repeat_count integer\n")
      _T("ALTER TABLE alarms ADD term_by integer\n")
      _T("UPDATE alarms SET hd_state=0,hd_ref='#00',creation_time=alarm_timestamp,")
         _T("last_change_time=alarm_timestamp,original_severity=severity,")
         _T("current_severity=severity,repeat_count=1,term_by=ack_by\n")
      _T("UPDATE alarms SET alarm_state=0 WHERE is_ack=0\n")
      _T("UPDATE alarms SET alarm_state=2 WHERE is_ack<>0\n")
      _T("ALTER TABLE alarms DROP COLUMN severity\n")
      _T("ALTER TABLE alarms DROP COLUMN alarm_timestamp\n")
      _T("ALTER TABLE alarms DROP COLUMN is_ack\n")
      _T("ALTER TABLE thresholds ADD current_state integer\n")
      _T("UPDATE thresholds SET current_state=0\n")
      _T("<END>");
   static TCHAR m_szBatch2[] =
      _T("CREATE INDEX idx_alarm_notes_alarm_id ON alarm_notes(alarm_id)\n")
      _T("CREATE INDEX idx_alarm_change_log_alarm_id ON alarm_change_log(alarm_id)\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE alarm_notes (")
		                 _T("note_id integer not null,")
		                 _T("alarm_id integer not null,")
		                 _T("change_time integer not null,")
		                 _T("user_id integer not null,")
		                 _T("note_text $SQL:TEXT not null,")
		                 _T("PRIMARY KEY(note_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE alarm_change_log	(")
                  	  _T("change_id $SQL:INT64 not null,")
		                 _T("change_time integer not null,")
		                 _T("alarm_id integer not null,")
		                 _T("opcode integer not null,")
		                 _T("user_id integer not null,")
		                 _T("info_text $SQL:TEXT not null,")
		                 _T("PRIMARY KEY(change_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE alarm_grops (")
		                 _T("alarm_group_id integer not null,")
		                 _T("group_name varchar(255) not null,")
		                 _T("PRIMARY KEY(alarm_group_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE alarm_group_map (")
		                 _T("alarm_group_id integer not null,")
                       _T("alarm_id integer not null,")
		                 _T("PRIMARY KEY(alarm_group_id,alarm_id))")))
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

static BOOL H_UpgradeFromV45(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("UPDATE object_tools_table_columns SET col_format=5 WHERE tool_id=5 AND col_number=1\n")
	   _T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) ")
	      _T("VALUES (2,'.1.3.6.1.4.1.45.3.26.*',3,0)\n")
	   _T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) ")
	      _T("VALUES (3,'.1.3.6.1.4.1.45.3.30.*',3,0)\n")
	   _T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) ")
	      _T("VALUES (4,'.1.3.6.1.4.1.45.3.31.*',3,0)\n")
	   _T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) ")
	      _T("VALUES (5,'.1.3.6.1.4.1.45.3.32.*',3,0)\n")
	   _T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) ")
	      _T("VALUES (6,'.1.3.6.1.4.1.45.3.33.*',3,0)\n")
	   _T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) ")
	      _T("VALUES (7,'.1.3.6.1.4.1.45.3.34.*',3,0)\n")
	   _T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) ")
	      _T("VALUES (8,'.1.3.6.1.4.1.45.3.35.*',3,0)\n")
	   _T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) ")
	      _T("VALUES (9,'.1.3.6.1.4.1.45.3.36.*',3,0)\n")
	   _T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) ")
	      _T("VALUES (10,'.1.3.6.1.4.1.45.3.40.*',3,0)\n")
	   _T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) ")
	      _T("VALUES (11,'.1.3.6.1.4.1.45.3.61.*',3,0)\n")
	   _T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,matching_oid,description) ")
		   _T("VALUES (14,'&Info->Topology table (Nortel)',2 ,'Topology table',1,'','Show topology table (Nortel protocol)')\n")
	   _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
		   _T("VALUES (14,0,'Peer IP','.1.3.6.1.4.1.45.1.6.13.2.1.1.3',3 ,0)\n")
	   _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
		   _T("VALUES (14,1,'Peer MAC','.1.3.6.1.4.1.45.1.6.13.2.1.1.5',4 ,0)\n")
	   _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
		   _T("VALUES (14,2,'Port','.1.3.6.1.4.1.45.1.6.13.2.1.1.2',5 ,0)\n")
	   _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (14,-2147483648)\n")
	   _T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,matching_oid,description) ")
		   _T("VALUES (15,'&Info->Topology table (CDP)',2 ,'Topology table',1,'','Show topology table (CDP)')\n")
	   _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
		   _T("VALUES (15,0,'Device ID','.1.3.6.1.4.1.9.9.23.1.2.1.1.6',0 ,0)\n")
	   _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
		   _T("VALUES (15,1,'IP Address','.1.3.6.1.4.1.9.9.23.1.2.1.1.4',3 ,0)\n")
	   _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
		   _T("VALUES (15,2,'Platform','.1.3.6.1.4.1.9.9.23.1.2.1.1.8',0 ,0)\n")
	   _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
		   _T("VALUES (15,3,'Version','.1.3.6.1.4.1.9.9.23.1.2.1.1.5',0 ,0)\n")
	   _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
		   _T("VALUES (15,4,'Port','.1.3.6.1.4.1.9.9.23.1.2.1.1.7',0 ,0)\n")
	   _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (15,-2147483648)\n")
      _T("<END>");

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

static BOOL H_UpgradeFromV44(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
	   _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) ")
         _T("VALUES (36,'SYS_DB_CONN_LOST',4,1,")
			_T("'Lost connection with backend database engine',")
			_T("'Generated if connection with backend database engine is lost.#0D#0A")
			_T("Parameters:#0D#0A   No message-specific parameters')\n")
	   _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) ")
         _T("VALUES (37,'SYS_DB_CONN_RESTORED',0,1,")
			_T("'Connection with backend database engine restored',")
			_T("'Generated when connection with backend database engine restored.#0D#0A")
			_T("Parameters:#0D#0A   No message-specific parameters')\n")
      _T("<END>");

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

static BOOL H_UpgradeFromV43(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("DELETE FROM object_tools WHERE tool_id=8000\n")
      _T("DELETE FROM object_tools_table_columns WHERE tool_id=8000\n")
      _T("DELETE FROM object_tools_acl WHERE tool_id=8000\n")
      _T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,")
         _T("matching_oid,description) VALUES (13,'&Info->&Process list',3,")
         _T("'Process List#7FSystem.ProcessList#7F^([0-9]+) (.*)',2,'',")
         _T("'Show list of currently running processes')\n")
	   _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,")
         _T("col_oid,col_format,col_substr) VALUES (13,0,'PID','',0,1)\n")
	   _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,")
         _T("col_oid,col_format,col_substr) VALUES (13,1,'Name','',0,2)\n")
	   _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (13,-2147483648)\n")
      _T("<END>");

   if (!CreateTable(_T("CREATE TABLE agent_configs (")
		                 _T("config_id integer not null,")
		                 _T("config_name varchar(255) not null,")
		                 _T("config_file $SQL:TEXT not null,")
		                 _T("config_filter $SQL:TEXT not null,")
		                 _T("sequence_number integer not null,")
		                 _T("PRIMARY KEY(config_id))")))
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

static BOOL H_UpgradeFromV42(int currVersion, int newVersion)
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

static BOOL H_UpgradeFromV41(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
	   _T("INSERT INTO images (image_id,name,file_name_png,file_hash_png,")
         _T("file_name_ico,file_hash_ico) VALUES (15,'Obj.Condition',")
         _T("'condition.png','<invalid_hash>','condition.ico','<invalid_hash>')\n")
	   _T("INSERT INTO default_images (object_class,image_id) VALUES (13, 15)\n")
	   _T("INSERT INTO oid_to_type (pair_id,snmp_oid,node_type,node_flags) ")
	      _T("VALUES (1,'.1.3.6.1.4.1.3224.1.*',2,0)\n")
	   _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) ")
         _T("VALUES (34,'SYS_CONDITION_ACTIVATED',2,1,'Condition \"%2\" activated',")
			_T("'Default event for condition activation.#0D#0AParameters:#0D#0A")
			_T("   1) Condition object ID#0D#0A   2) Condition object name#0D#0A")
			_T("   3) Previous condition status#0D#0A   4) Current condition status')\n")
	   _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) ")
         _T("VALUES (35,'SYS_CONDITION_DEACTIVATED',0,1,'Condition \"%2\" deactivated',")
			_T("'Default event for condition deactivation.#0D#0AParameters:#0D#0A")
			_T("   1) Condition object ID#0D#0A   2) Condition object name#0D#0A")
			_T("   3) Previous condition status#0D#0A   4) Current condition status')\n")
      _T("<END>");

   if (!CreateTable(_T("CREATE TABLE conditions	(")
		                 _T("id integer not null,")
		                 _T("activation_event integer not null,")
		                 _T("deactivation_event integer not null,")
		                 _T("source_object integer not null,")
		                 _T("active_status integer not null,")
		                 _T("inactive_status integer not null,")
		                 _T("script $SQL:TEXT not null,")
		                 _T("PRIMARY KEY(id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE cond_dci_map (")
		                 _T("condition_id integer not null,")
		                 _T("dci_id integer not null,")
		                 _T("node_id integer not null,")
		                 _T("dci_func integer not null,")
		                 _T("num_polls integer not null,")
		                 _T("PRIMARY KEY(condition_id,dci_id))")))
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

static BOOL H_UpgradeFromV40(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE users ADD guid varchar(36)\n")
      _T("ALTER TABLE users ADD auth_method integer\n")
      _T("ALTER TABLE user_groups ADD guid varchar(36)\n")
      _T("UPDATE users SET auth_method=0\n")
      _T("<END>");
   DB_RESULT hResult;
   int i, nCount;
   DWORD dwId;
   uuid_t guid;
   TCHAR szQuery[256], szGUID[64];

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   // Generate GUIDs for users and groups
   _tprintf(_T("Generating GUIDs...\n"));
   
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

static BOOL H_UpgradeFromV39(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE users ADD grace_logins integer\n")
      _T("UPDATE users SET grace_logins=5\n")
      _T("<END>");

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

static BOOL H_UpgradeFromV38(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
	   _T("INSERT INTO maps (map_id,map_name,description,root_object_id) ")
		   _T("VALUES (1,'Default','Default network map',1)\n")
	   _T("INSERT INTO map_access_lists (map_id,user_id,access_rights) VALUES (1,-2147483648,1)\n")
      _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) ")
         _T("VALUES (33,'SYS_SCRIPT_ERROR',2,1,")
		   _T("'Script (%1) execution error: %2',")
		   _T("'Generated when server encounters NXSL script execution error.#0D#0A")
		   _T("Parameters:#0D#0A")
		   _T("   1) Script name#0D#0A")
		   _T("   2) Error text#0D#0A")
		   _T("   3) DCI ID if script is DCI transformation script, or 0 otherwise')\n")
      _T("<END>");

   if (!CreateTable(_T("CREATE TABLE maps	(")
		                 _T("map_id integer not null,")
		                 _T("map_name varchar(255) not null,")
		                 _T("description $SQL:TEXT not null,")
		                 _T("root_object_id integer not null,")
		                 _T("PRIMARY KEY(map_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE map_access_lists (")
		                 _T("map_id integer not null,")
		                 _T("user_id integer not null,")
		                 _T("access_rights integer not null,")
		                 _T("PRIMARY KEY(map_id,user_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE submaps (")
		                 _T("map_id integer not null,")
		                 _T("submap_id integer not null,")
		                 _T("attributes integer not null,")	
		                 _T("PRIMARY KEY(map_id,submap_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE submap_object_positions (")
		                 _T("map_id integer not null,")
		                 _T("submap_id integer not null,")
		                 _T("object_id integer not null,")
		                 _T("x integer not null,")
		                 _T("y integer not null,")
		                 _T("PRIMARY KEY(map_id,submap_id,object_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE submap_links (")
		                 _T("map_id integer not null,")
		                 _T("submap_id integer not null,")
		                 _T("object_id1 integer not null,")
		                 _T("object_id2 integer not null,")
		                 _T("link_type integer not null,")
		                 _T("PRIMARY KEY(map_id,submap_id,object_id1,object_id2))")))
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

static BOOL H_UpgradeFromV37(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
	   _T("CREATE INDEX idx_event_log_event_timestamp ON event_log(event_timestamp)\n")
	   _T("CREATE INDEX idx_syslog_msg_timestamp ON syslog(msg_timestamp)\n")
      _T("CREATE INDEX idx_snmp_trap_log_trap_timestamp ON snmp_trap_log(trap_timestamp)\n")
      _T("<END>");

   if (!CreateTable(_T("CREATE TABLE snmp_trap_log (")
		                 _T("trap_id $SQL:INT64 not null,")
		                 _T("trap_timestamp integer not null,")
		                 _T("ip_addr varchar(15) not null,")
		                 _T("object_id integer not null,")
		                 _T("trap_oid varchar(255) not null,")
		                 _T("trap_varlist $SQL:TEXT not null,")
		                 _T("PRIMARY KEY(trap_id))")))

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

static BOOL H_UpgradeFromV36(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("DROP TABLE new_nodes\n")
      _T("DELETE FROM config WHERE var_name='NewNodePollingInterval'\n")
      _T("INSERT INTO script_library (script_id,script_name,script_code) ")
	      _T("VALUES (1,'Filter::SNMP','sub main()#0D#0A{#0D#0A   return $1->isSNMP;#0D#0A}#0D#0A')\n")
      _T("INSERT INTO script_library (script_id,script_name,script_code) ")
	      _T("VALUES (2,'Filter::Agent','sub main()#0D#0A{#0D#0A   return $1->isAgent;#0D#0A}#0D#0A')\n")
      _T("INSERT INTO script_library (script_id,script_name,script_code) ")
	      _T("VALUES (3,'Filter::AgentOrSNMP','sub main()#0D#0A{#0D#0A   return $1->isAgent || $1->isSNMP;#0D#0A}#0D#0A')\n")
      _T("INSERT INTO script_library (script_id,script_name,script_code) ")
	      _T("VALUES (4,'DCI::SampleTransform','sub dci_transform()#0D#0A{#0D#0A   return $1 + 1;#0D#0A}#0D#0A')\n")
      _T("<END>");

   if (!CreateTable(_T("CREATE TABLE script_library (")
		              _T("script_id integer not null,")
		              _T("script_name varchar(63) not null,")
		              _T("script_code $SQL:TEXT not null,")
		              _T("PRIMARY KEY(script_id))")))
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

static BOOL H_UpgradeFromV35(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE nodes ADD proxy_node integer\n")
      _T("UPDATE nodes SET proxy_node=0\n")
      _T("ALTER TABLE object_tools ADD matching_oid varchar(255)\n")
      _T("UPDATE object_tools SET matching_oid='#00'\n")
      _T("<END>");

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

static BOOL H_UpgradeFromV34(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE object_properties DROP COLUMN status_alg\n")
      _T("ALTER TABLE object_properties ADD status_calc_alg integer\n")
	   _T("ALTER TABLE object_properties ADD status_prop_alg integer\n")
	   _T("ALTER TABLE object_properties ADD status_fixed_val integer\n")
	   _T("ALTER TABLE object_properties ADD status_shift integer\n")
	   _T("ALTER TABLE object_properties ADD status_translation varchar(8)\n")
	   _T("ALTER TABLE object_properties ADD status_single_threshold integer\n")
	   _T("ALTER TABLE object_properties ADD status_thresholds varchar(8)\n")
      _T("UPDATE object_properties SET status_calc_alg=0,status_prop_alg=0,")
         _T("status_fixed_val=0,status_shift=0,status_translation='01020304',")
         _T("status_single_threshold=75,status_thresholds='503C2814'\n")
      _T("DELETE FROM config WHERE var_name='StatusCalculationAlgorithm'\n")
      _T("<END>");

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

static BOOL H_UpgradeFromV33(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE items ADD adv_schedule integer\n")
      _T("UPDATE items SET adv_schedule=0\n")
      _T("<END>");
   
   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE dci_schedules (")
		                 _T("item_id integer not null,")
                       _T("schedule varchar(255) not null)")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE syslog (")
                       _T("msg_id $SQL:INT64 not null,")
		                 _T("msg_timestamp integer not null,")
		                 _T("facility integer not null,")
		                 _T("severity integer not null,")
		                 _T("source_object_id integer not null,")
		                 _T("hostname varchar(127) not null,")
		                 _T("msg_tag varchar(32) not null,")
		                 _T("msg_text $SQL:TEXT not null,")
		                 _T("PRIMARY KEY(msg_id))")))
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

static BOOL H_UpgradeFromV32(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,description) ")
	      _T("VALUES (5,'&Info->&Switch forwarding database (FDB)',2 ,'Forwarding database',0,'Show switch forwarding database')\n")
      _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
	      _T("VALUES (5,0,'MAC Address','.1.3.6.1.2.1.17.4.3.1.1',4 ,0)\n")
      _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
	      _T("VALUES (5,1,'Port','.1.3.6.1.2.1.17.4.3.1.2',1 ,0)\n")
      _T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,description) ")
	      _T("VALUES (6,'&Connect->Open &web browser',4 ,'http://%OBJECT_IP_ADDR%',0,'Open embedded web browser to node')\n")
      _T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,description) ")
	      _T("VALUES (7,'&Connect->Open &web browser (HTTPS)',4 ,'https://%OBJECT_IP_ADDR%',0,'Open embedded web browser to node using HTTPS')\n")
      _T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,description) ")
	      _T("VALUES (8,'&Info->&Agent->&Subagent list',3 ,'Subagent List#7FAgent.SubAgentList#7F^(.*) (.*) (.*) (.*)',0,'Show list of loaded subagents')\n")
      _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
	      _T("VALUES (8,0,'Name','',0 ,1)\n")
      _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
	      _T("VALUES (8,1,'Version','',0 ,2)\n")
      _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
	      _T("VALUES (8,2,'File','',0 ,4)\n")
      _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
	      _T("VALUES (8,3,'Module handle','',0 ,3)\n")
      _T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,description) ")
	      _T("VALUES (9,'&Info->&Agent->Supported &parameters',3 ,'Supported parameters#7FAgent.SupportedParameters#7F^(.*)',0,'Show list of parameters supported by agent')\n")
      _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
	      _T("VALUES (9,0,'Parameter','',0 ,1)\n")
      _T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,description) ")
	      _T("VALUES (10,'&Info->&Agent->Supported &enums',3 ,'Supported enums#7FAgent.SupportedEnums#7F^(.*)',0,'Show list of enums supported by agent')\n")
      _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
	      _T("VALUES (10,0,'Parameter','',0 ,1)\n")
      _T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,description) ")
	      _T("VALUES (11,'&Info->&Agent->Supported &actions',3 ,'Supported actions#7FAgent.ActionList#7F^(.*) (.*) #22(.*)#22.*',0,'Show list of actions supported by agent')\n")
      _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
	      _T("VALUES (11,0,'Name','',0 ,1)\n")
      _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
	      _T("VALUES (11,1,'Type','',0 ,2)\n")
      _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
	      _T("VALUES (11,2,'Data','',0 ,3)\n")
      _T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,description) ")
	      _T("VALUES (12,'&Info->&Agent->Configured &ICMP targets',3 ,'Configured ICMP targets#7FICMP.TargetList#7F^(.*) (.*) (.*) (.*)',0,'Show list of actions supported by agent')\n")
      _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
	      _T("VALUES (12,0,'IP Address','',0 ,1)\n")
      _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
	      _T("VALUES (12,1,'Name','',0 ,4)\n")
      _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
	      _T("VALUES (12,2,'Last RTT','',0 ,2)\n")
      _T("INSERT INTO object_tools_table_columns (tool_id,col_number,col_name,col_oid,col_format,col_substr) ")
	      _T("VALUES (12,4,'Average RTT','',0 ,3)\n")
      _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (5,-2147483648)\n")
      _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (6,-2147483648)\n")
      _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (7,-2147483648)\n")
      _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (8,-2147483648)\n")
      _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (9,-2147483648)\n")
      _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (10,-2147483648)\n")
      _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (11,-2147483648)\n")
      _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (12,-2147483648)\n")
      _T("<END>");

   if (!CreateTable(_T("CREATE TABLE object_tools_table_columns (")
                  	  _T("tool_id integer not null,")
		                 _T("col_number integer not null,")
		                 _T("col_name varchar(255),")
		                 _T("col_oid varchar(255),")
		                 _T("col_format integer,")
		                 _T("col_substr integer,")
		                 _T("PRIMARY KEY(tool_id,col_number))")))
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

static BOOL H_UpgradeFromV31(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,description) ")
	      _T("VALUES (1,'&Shutdown system',1 ,'System.Shutdown',0,'Shutdown target node via NetXMS agent')\n")
      _T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,description) ")
	      _T("VALUES (2,'&Restart system',1 ,'System.Restart',0,'Restart target node via NetXMS agent')\n")
      _T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,description) ")
	      _T("VALUES (3,'&Wakeup node',0 ,'wakeup',0,'Wakeup node using Wake-On-LAN magic packet')\n")
      _T("INSERT INTO object_tools (tool_id,tool_name,tool_type,tool_data,flags,description) ")
	      _T("VALUES (4,'Restart &agent',1 ,'Agent.Restart',0,'Restart NetXMS agent on target node')\n")
      _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (1,-2147483648)\n")
      _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (2,-2147483648)\n")
      _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (3,-2147483648)\n")
      _T("INSERT INTO object_tools_acl (tool_id,user_id) VALUES (4,-2147483648)\n")
      _T("<END>");

   if (!CreateTable(_T("CREATE TABLE object_tools (")
	                    _T("tool_id integer not null,")
	                    _T("tool_name varchar(255) not null,")
	                    _T("tool_type integer not null,")
	                    _T("tool_data $SQL:TEXT,")
	                    _T("description varchar(255),")
	                    _T("flags integer not null,")
	                    _T("PRIMARY KEY(tool_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE object_tools_acl (")
	                    _T("tool_id integer not null,")
	                    _T("user_id integer not null,")
	                    _T("PRIMARY KEY(tool_id,user_id))")))
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

static BOOL H_UpgradeFromV30(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
	   _T("INSERT INTO default_images (object_class,image_id) ")
		   _T("VALUES (12, 14)\n")
	   _T("INSERT INTO images (image_id,name,file_name_png,file_hash_png,")
         _T("file_name_ico,file_hash_ico) VALUES (14,'Obj.VPNConnector',")
         _T("'vpnc.png','<invalid_hash>','vpnc.ico','<invalid_hash>')\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE vpn_connectors (")
		                 _T("id integer not null,")
		                 _T("node_id integer not null,")
		                 _T("peer_gateway integer not null,")
		                 _T("PRIMARY KEY(id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE vpn_connector_networks (")
		                 _T("vpn_id integer not null,")
		                 _T("network_type integer not null,")
		                 _T("ip_addr varchar(15) not null,")
		                 _T("ip_netmask varchar(15) not null,")
		                 _T("PRIMARY KEY(vpn_id,ip_addr))")))
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

static BOOL H_UpgradeFromV29(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE object_properties ADD status_alg integer\n")
      _T("UPDATE object_properties SET status_alg=-1\n")
      _T("<END>");

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

static BOOL H_UpgradeFromV28(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE nodes ADD zone_guid integer\n")
      _T("ALTER TABLE subnets ADD zone_guid integer\n")
      _T("UPDATE nodes SET zone_guid=0\n")
      _T("UPDATE subnets SET zone_guid=0\n")
      _T("INSERT INTO default_images (object_class,image_id) VALUES (6,13)\n")
      _T("INSERT INTO images (image_id,name,file_name_png,file_hash_png,")
         _T("file_name_ico,file_hash_ico) VALUES (13,'Obj.Zone','zone.png',")
         _T("'<invalid_hash>','zone.ico','<invalid_hash>')\n")
      _T("<END>");

   if (!CreateTable(_T("CREATE TABLE zones (")
	                    _T("id integer not null,")
	                    _T("zone_guid integer not null,")
	                    _T("zone_type integer not null,")
	                    _T("controller_ip varchar(15) not null,")
	                    _T("description $SQL:TEXT,")
	                    _T("PRIMARY KEY(id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE zone_ip_addr_list (")
	                    _T("zone_id integer not null,")
	                    _T("ip_addr varchar(15) not null,")
	                    _T("PRIMARY KEY(zone_id,ip_addr))")))
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

static BOOL H_UpgradeFromV27(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE users ADD system_access integer\n")
      _T("UPDATE users SET system_access=access\n")
      _T("ALTER TABLE users DROP COLUMN access\n")
      _T("ALTER TABLE user_groups ADD system_access integer\n")
      _T("UPDATE user_groups SET system_access=access\n")
      _T("ALTER TABLE user_groups DROP COLUMN access\n")
      _T("<END>");

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
   static const TCHAR *m_pszTableNames[] = { _T("nodes"), _T("interfaces"), _T("subnets"),
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
      _sntprintf(szQuery, 1024, _T("INSERT INTO object_properties (object_id,name,")
                                   _T("status,is_deleted,image_id,inherit_access_rights,")
                                   _T("last_modified) VALUES (%d,'%s',%d,%d,%d,%d,%ld)"),
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

static BOOL H_UpgradeFromV26(int currVersion, int newVersion)
{
   DB_RESULT hResult;
   DWORD i, dwNumObjects, dwId;
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE nodes DROP COLUMN name\n")
      _T("ALTER TABLE nodes DROP COLUMN status\n")
      _T("ALTER TABLE nodes DROP COLUMN is_deleted\n")
      _T("ALTER TABLE nodes DROP COLUMN image_id\n")
      _T("ALTER TABLE interfaces DROP COLUMN name\n")
      _T("ALTER TABLE interfaces DROP COLUMN status\n")
      _T("ALTER TABLE interfaces DROP COLUMN is_deleted\n")
      _T("ALTER TABLE interfaces DROP COLUMN image_id\n")
      _T("ALTER TABLE subnets DROP COLUMN name\n")
      _T("ALTER TABLE subnets DROP COLUMN status\n")
      _T("ALTER TABLE subnets DROP COLUMN is_deleted\n")
      _T("ALTER TABLE subnets DROP COLUMN image_id\n")
      _T("ALTER TABLE network_services DROP COLUMN name\n")
      _T("ALTER TABLE network_services DROP COLUMN status\n")
      _T("ALTER TABLE network_services DROP COLUMN is_deleted\n")
      _T("ALTER TABLE network_services DROP COLUMN image_id\n")
      _T("ALTER TABLE containers DROP COLUMN name\n")
      _T("ALTER TABLE containers DROP COLUMN status\n")
      _T("ALTER TABLE containers DROP COLUMN is_deleted\n")
      _T("ALTER TABLE containers DROP COLUMN image_id\n")
      _T("ALTER TABLE templates DROP COLUMN name\n")
      _T("ALTER TABLE templates DROP COLUMN is_deleted\n")
      _T("ALTER TABLE templates DROP COLUMN image_id\n")
      _T("DROP TABLE access_options\n")
      _T("DELETE FROM config WHERE var_name='TopologyRootObjectName'\n")
      _T("DELETE FROM config WHERE var_name='TopologyRootImageId'\n")
      _T("DELETE FROM config WHERE var_name='ServiceRootObjectName'\n")
      _T("DELETE FROM config WHERE var_name='ServiceRootImageId'\n")
      _T("DELETE FROM config WHERE var_name='TemplateRootObjectName'\n")
      _T("DELETE FROM config WHERE var_name='TemplateRootImageId'\n")
      _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) ")
         _T("VALUES (31,'SYS_SNMP_OK',0,1,'Connectivity with SNMP agent restored',")
		   _T("'Generated when connectivity with node#27s SNMP agent restored.#0D#0A")
		   _T("Parameters:#0D#0A   No message-specific parameters')\n")
      _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) ")
		   _T("VALUES (32,'SYS_AGENT_OK',0,1,'Connectivity with native agent restored',")
		   _T("'Generated when connectivity with node#27s native agent restored.#0D#0A")
		   _T("Parameters:#0D#0A   No message-specific parameters')\n")
      _T("<END>");

   if (!CreateTable(_T("CREATE TABLE object_properties (")
	                    _T("object_id integer not null,")
	                    _T("name varchar(63) not null,")
	                    _T("status integer not null,")
	                    _T("is_deleted integer not null,")
	                    _T("image_id integer,")
	                    _T("last_modified integer not null,")
	                    _T("inherit_access_rights integer not null,")
	                    _T("PRIMARY KEY(object_id))")))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE user_profiles (")
	                    _T("user_id integer not null,")
	                    _T("var_name varchar(255) not null,")
	                    _T("var_value $SQL:TEXT,")
	                    _T("PRIMARY KEY(user_id,var_name))")))
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
               _sntprintf(szQuery, 1024, _T("INSERT INTO object_properties (object_id,name,")
                                            _T("status,is_deleted,image_id,inherit_access_rights,")
                                            _T("last_modified) VALUES (%d,'%s',5,0,%d,%d,%ld)"),
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

static BOOL H_UpgradeFromV25(int currVersion, int newVersion)
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

static BOOL H_UpgradeFromV24(int currVersion, int newVersion)
{
   DB_RESULT hResult;
   int i, iNumRows;
   DWORD dwNodeId;
   TCHAR szQuery[256];

   if (GetYesNo(_T("Create indexes on existing IDATA tables?")))
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

static BOOL H_UpgradeFromV23(int currVersion, int newVersion)
{
   DB_RESULT hResult;
   TCHAR szQuery[256];
   int i, iNumRows;

   if (!CreateTable(_T("CREATE TABLE raw_dci_values (")
                       _T("	item_id integer not null,")
	                    _T("   raw_value varchar(255),")
	                    _T("   last_poll_time integer,")
	                    _T("   PRIMARY KEY(item_id))")))
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
         _sntprintf(szQuery, 256, _T("INSERT INTO raw_dci_values (item_id,raw_value,last_poll_time) VALUES (%d,'#00',1)"),
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

static BOOL H_UpgradeFromV22(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE items ADD template_item_id integer\n")
      _T("UPDATE items SET template_item_id=0\n")
      _T("CREATE INDEX idx_sequence ON thresholds(sequence_number)\n")
      _T("<END>");

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

static BOOL H_UpgradeFromV21(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) ")
         _T("VALUES (4009,'DC_HDD_TEMP_WARNING',1,1,")
		   _T("'Temperature of hard disk %6 is above warning level of %3 (current: %4)',")
		   _T("'Custom data collection threshold event.#0D#0AParameters:#0D#0A")
		   _T("   1) Parameter name#0D#0A   2) Item description#0D#0A")
         _T("   3) Threshold value#0D#0A   4) Actual value#0D#0A")
         _T("   5) Data collection item ID#0D#0A   6) Instance')\n")
      _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) ")
      	_T("VALUES (4010,'DC_HDD_TEMP_MAJOR',3,1,")
		   _T("'Temperature of hard disk %6 is above %3 (current: %4)',")
		   _T("'Custom data collection threshold event.#0D#0AParameters:#0D#0A")
		   _T("   1) Parameter name#0D#0A   2) Item description#0D#0A")
         _T("   3) Threshold value#0D#0A   4) Actual value#0D#0A")
         _T("   5) Data collection item ID#0D#0A   6) Instance')\n")
      _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) ")
	      _T("VALUES (4011,'DC_HDD_TEMP_CRITICAL',4,1,")
		   _T("'Temperature of hard disk %6 is above critical level of %3 (current: %4)',")
		   _T("'Custom data collection threshold event.#0D#0AParameters:#0D#0A")
		   _T("   1) Parameter name#0D#0A   2) Item description#0D#0A")
         _T("   3) Threshold value#0D#0A   4) Actual value#0D#0A")
         _T("   5) Data collection item ID#0D#0A   6) Instance')\n")
      _T("<END>");

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

static BOOL H_UpgradeFromV20(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description)") 
         _T(" VALUES (30,'SYS_SMS_FAILURE',1,1,'Unable to send SMS to phone %1',")
		   _T("'Generated when server is unable to send SMS.#0D#0A")
		   _T("   Parameters:#0D#0A   1) Phone number')\n")
      _T("ALTER TABLE nodes ADD node_flags integer\n")
      _T("<END>");
   static TCHAR m_szBatch2[] =
      _T("ALTER TABLE nodes DROP COLUMN is_snmp\n")
      _T("ALTER TABLE nodes DROP COLUMN is_agent\n")
      _T("ALTER TABLE nodes DROP COLUMN is_bridge\n")
      _T("ALTER TABLE nodes DROP COLUMN is_router\n")
      _T("ALTER TABLE nodes DROP COLUMN is_local_mgmt\n")
      _T("ALTER TABLE nodes DROP COLUMN is_ospf\n")
      _T("CREATE INDEX idx_item_id ON thresholds(item_id)\n")
      _T("<END>");
   static DWORD m_dwFlag[] = { NF_IS_SNMP, NF_IS_NATIVE_AGENT, NF_IS_BRIDGE,
                               NF_IS_ROUTER, NF_IS_LOCAL_MGMT, NF_IS_OSPF };
   DB_RESULT hResult;
   int i, j, iNumRows;
   DWORD dwFlags, dwNodeId;
   TCHAR szQuery[256];

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   // Convert "is_xxx" fields into one _T("node_flags") field
   hResult = SQLSelect(_T("SELECT id,is_snmp,is_agent,is_bridge,is_router,")
                          _T("is_local_mgmt,is_ospf FROM nodes"));
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

   if (GetYesNo(_T("Create indexes on existing IDATA tables?")))
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

static BOOL H_UpgradeFromV19(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE nodes ADD poller_node_id integer\n")
      _T("ALTER TABLE nodes ADD is_ospf integer\n")
      _T("UPDATE nodes SET poller_node_id=0,is_ospf=0\n")
      _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description)")
         _T(" VALUES (28,'SYS_NODE_DOWN',4,1,'Node down',")
		   _T("'Generated when node is not responding to management server.#0D#0A")
		   _T("Parameters:#0D#0A   No event-specific parameters')\n")
      _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description)")
         _T(" VALUES (29,'SYS_NODE_UP',0,1,'Node up',")
		   _T("'Generated when communication with the node re-established.#0D#0A")
		   _T("Parameters:#0D#0A   No event-specific parameters')\n")
      _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description)")
         _T(" VALUES (25,'SYS_SERVICE_DOWN',3,1,'Network service #22%1#22 is not responding',")
		   _T("'Generated when network service is not responding to management server as ")
         _T("expected.#0D#0AParameters:#0D#0A   1) Service name#0D0A")
		   _T("   2) Service object ID#0D0A   3) Service type')\n")
      _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description)")
         _T(" VALUES (26,'SYS_SERVICE_UP',0,1,")
         _T("'Network service #22%1#22 returned to operational state',")
		   _T("'Generated when network service responds as expected after failure.#0D#0A")  
         _T("Parameters:#0D#0A   1) Service name#0D0A")
		   _T("   2) Service object ID#0D0A   3) Service type')\n")
      _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description)")
         _T(" VALUES (27,'SYS_SERVICE_UNKNOWN',1,1,")
		   _T("'Status of network service #22%1#22 is unknown',")
		   _T("'Generated when management server is unable to determine state of the network ")
         _T("service due to agent or server-to-agent communication failure.#0D#0A")
         _T("Parameters:#0D#0A   1) Service name#0D0A")
		   _T("   2) Service object ID#0D0A   3) Service type')\n")
      _T("INSERT INTO images (image_id,name,file_name_png,file_hash_png,file_name_ico,")
         _T("file_hash_ico) VALUES (12,'Obj.NetworkService','network_service.png',")
         _T("'<invalid_hash>','network_service.ico','<invalid_hash>')\n")
      _T("INSERT INTO default_images (object_class,image_id) VALUES (11,12)\n")
      _T("<END>");
   
   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE network_services (")
	                    _T("id integer not null,")
	                    _T("name varchar(63),")
	                    _T("status integer,")
                       _T("is_deleted integer,")
	                    _T("node_id integer not null,")
	                    _T("service_type integer,")
	                    _T("ip_bind_addr varchar(15),")
	                    _T("ip_proto integer,")
	                    _T("ip_port integer,")
	                    _T("check_request $SQL:TEXT,")
	                    _T("check_responce $SQL:TEXT,")
	                    _T("poller_node_id integer not null,")
	                    _T("image_id integer not null,")
	                    _T("PRIMARY KEY(id))")))
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

static BOOL H_UpgradeFromV18(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE nodes ADD platform_name varchar(63)\n")
      _T("UPDATE nodes SET platform_name=''\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!CreateTable(_T("CREATE TABLE agent_pkg (")
	                       _T("pkg_id integer not null,")
	                       _T("pkg_name varchar(63),")
	                       _T("version varchar(31),")
	                       _T("platform varchar(63),")
	                       _T("pkg_file varchar(255),")
	                       _T("description varchar(255),")
	                       _T("PRIMARY KEY(pkg_id))")))
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

static BOOL H_UpgradeFromV17(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE nodes DROP COLUMN inherit_access_rights\n")
      _T("ALTER TABLE nodes ADD agent_version varchar(63)\n")
      _T("UPDATE nodes SET agent_version='' WHERE is_agent=0\n")
      _T("UPDATE nodes SET agent_version='<unknown>' WHERE is_agent=1\n")
      _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,")
         _T("message,description) SELECT event_id,name,severity,flags,")
         _T("message,description FROM events\n")
      _T("DROP TABLE events\n")
      _T("DROP TABLE event_group_members\n")
      _T("CREATE TABLE event_group_members (group_id integer not null,")
	      _T("event_code integer not null,	PRIMARY KEY(group_id,event_code))\n")
      _T("ALTER TABLE alarms ADD source_event_code integer\n")
      _T("UPDATE alarms SET source_event_code=source_event_id\n")
      _T("ALTER TABLE alarms DROP COLUMN source_event_id\n")
      _T("ALTER TABLE alarms ADD source_event_id bigint\n")
      _T("UPDATE alarms SET source_event_id=0\n")
      _T("ALTER TABLE snmp_trap_cfg ADD event_code integer not null default 0\n")
      _T("UPDATE snmp_trap_cfg SET event_code=event_id\n")
      _T("ALTER TABLE snmp_trap_cfg DROP COLUMN event_id\n")
      _T("DROP TABLE event_log\n")
      _T("CREATE TABLE event_log (event_id bigint not null,event_code integer,")
	      _T("event_timestamp integer,event_source integer,event_severity integer,")
	      _T("event_message varchar(255),root_event_id bigint default 0,")
	      _T("PRIMARY KEY(event_id))\n")
      _T("<END>");
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

      if (!SQLQuery(_T("CREATE TABLE policy_event_list (")
                       _T("rule_id integer not null,")
                       _T("event_code integer not null,")
                       _T("PRIMARY KEY(rule_id,event_code))")))
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
      _T("CREATE TABLE event_cfg (event_code integer not null,")
	      _T("event_name varchar(63) not null,severity integer,flags integer,")
	      _T("message varchar(255),description %s,PRIMARY KEY(event_code))"),
              g_pszSqlType[g_iSyntax][SQL_TYPE_TEXT]);
   if (!SQLQuery(szQuery))
      if (!g_bIgnoreErrors)
         return FALSE;

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;

   _sntprintf(szQuery, 4096,
      _T("CREATE TABLE modules (module_id integer not null,")
	      _T("module_name varchar(63),exec_name varchar(255),")
	      _T("module_flags integer not null default 0,description %s,")
	      _T("license_key varchar(255),PRIMARY KEY(module_id))"),
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

static BOOL H_UpgradeFromV16(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("DROP TABLE locks\n")
      _T("CREATE TABLE snmp_trap_cfg (trap_id integer not null,snmp_oid varchar(255) not null,")
	      _T("event_id integer not null,description varchar(255),PRIMARY KEY(trap_id))\n")
      _T("CREATE TABLE snmp_trap_pmap (trap_id integer not null,parameter integer not null,")
         _T("snmp_oid varchar(255),description varchar(255),PRIMARY KEY(trap_id,parameter))\n")
      _T("INSERT INTO events (event_id,name,severity,flags,message,description) VALUES ")
	      _T("(500, 'SNMP_UNMATCHED_TRAP',	0, 1,	'SNMP trap received: %1 (Parameters: %2)',")
		   _T("'Generated when system receives an SNMP trap without match in trap ")
         _T("configuration table#0D#0AParameters:#0D#0A   1) SNMP trap OID#0D#0A")
		   _T("   2) Trap parameters')\n")
      _T("INSERT INTO events (event_id,name,severity,flags,message,description) VALUES ")
	      _T("(501, 'SNMP_COLD_START', 0, 1, 'System was cold-started',")
		   _T("'Generated when system receives a coldStart SNMP trap#0D#0AParameters:#0D#0A")
		   _T("   1) SNMP trap OID')\n")
      _T("INSERT INTO events (event_id,name,severity,flags,message,description) VALUES ")
	      _T("(502, 'SNMP_WARM_START', 0, 1, 'System was warm-started',")
		   _T("'Generated when system receives a warmStart SNMP trap#0D#0A")
		   _T("Parameters:#0D#0A   1) SNMP trap OID')\n")
      _T("INSERT INTO events (event_id,name,severity,flags,message,description) VALUES ")
	      _T("(503, 'SNMP_LINK_DOWN', 3, 1, 'Link is down',")
		   _T("'Generated when system receives a linkDown SNMP trap#0D#0A")
		   _T("Parameters:#0D#0A   1) SNMP trap OID#0D#0A   2) Interface index')\n")
      _T("INSERT INTO events (event_id,name,severity,flags,message,description) VALUES ")
	      _T("(504, 'SNMP_LINK_UP', 0, 1, 'Link is up',")
		   _T("'Generated when system receives a linkUp SNMP trap#0D#0AParameters:#0D#0A")
		   _T("   1) SNMP trap OID#0D#0A   2) Interface index')\n")
      _T("INSERT INTO events (event_id,name,severity,flags,message,description) VALUES ")
	      _T("(505, 'SNMP_AUTH_FAILURE', 1, 1, 'SNMP authentication failure',")
		   _T("'Generated when system receives an authenticationFailure SNMP trap#0D#0A")
		   _T("Parameters:#0D#0A   1) SNMP trap OID')\n")
      _T("INSERT INTO events (event_id,name,severity,flags,message,description) VALUES ")
	      _T("(506, 'SNMP_EGP_NEIGHBOR_LOSS',	1, 1,	'EGP neighbor loss',")
		   _T("'Generated when system receives an egpNeighborLoss SNMP trap#0D#0A")
		   _T("Parameters:#0D#0A   1) SNMP trap OID')\n")
      _T("INSERT INTO snmp_trap_cfg (trap_id,snmp_oid,event_id,description) ")
         _T("VALUES (1,'.1.3.6.1.6.3.1.1.5.1',501,'Generic coldStart trap')\n")
      _T("INSERT INTO snmp_trap_cfg (trap_id,snmp_oid,event_id,description) ")
         _T("VALUES (2,'.1.3.6.1.6.3.1.1.5.2',502,'Generic warmStart trap')\n")
      _T("INSERT INTO snmp_trap_cfg (trap_id,snmp_oid,event_id,description) ")
         _T("VALUES (3,'.1.3.6.1.6.3.1.1.5.3',503,'Generic linkDown trap')\n")
      _T("INSERT INTO snmp_trap_cfg (trap_id,snmp_oid,event_id,description) ")
         _T("VALUES (4,'.1.3.6.1.6.3.1.1.5.4',504,'Generic linkUp trap')\n")
      _T("INSERT INTO snmp_trap_cfg (trap_id,snmp_oid,event_id,description) ")
         _T("VALUES (5,'.1.3.6.1.6.3.1.1.5.5',505,'Generic authenticationFailure trap')\n")
      _T("INSERT INTO snmp_trap_cfg (trap_id,snmp_oid,event_id,description) ")
         _T("VALUES (6,'.1.3.6.1.6.3.1.1.5.6',506,'Generic egpNeighborLoss trap')\n")
      _T("INSERT INTO snmp_trap_pmap (trap_id,parameter,snmp_oid,description) ")
         _T("VALUES (3,1,'.1.3.6.1.2.1.2.2.1.1','Interface index')\n")
      _T("INSERT INTO snmp_trap_pmap (trap_id,parameter,snmp_oid,description) ")
         _T("VALUES (4,1,'.1.3.6.1.2.1.2.2.1.1','Interface index')\n")
      _T("<END>");

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

/**
 * Upgrade from V15 to V16
 */
static BOOL H_UpgradeFromV15(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("INSERT INTO events (event_id,name,severity,flags,message,description) VALUES ")
	      _T("(4005, 'DC_MAILBOX_TOO_LARGE', 1, 1,")
		   _T("'Mailbox #22%6#22 exceeds size limit (allowed size: %3; actual size: %4)',")
		   _T("'Custom data collection threshold event.#0D#0AParameters:#0D#0A")
		   _T("   1) Parameter name#0D#0A   2) Item description#0D#0A")
         _T("   3) Threshold value#0D#0A   4) Actual value#0D#0A")
         _T("   5) Data collection item ID#0D#0A   6) Instance')\n")
      _T("INSERT INTO events (event_id,name,severity,flags,message,description) VALUES ")
	      _T("(4006, 'DC_AGENT_VERSION_CHANGE',	0, 1,")
         _T("'NetXMS agent version was changed from %3 to %4',")
         _T("'Custom data collection threshold event.#0D#0AParameters:#0D#0A")
		   _T("   1) Parameter name#0D#0A   2) Item description#0D#0A")
         _T("   3) Threshold value#0D#0A   4) Actual value#0D#0A")
         _T("   5) Data collection item ID#0D#0A   6) Instance')\n")
      _T("INSERT INTO events (event_id,name,severity,flags,message,description) VALUES ")
	      _T("(4007, 'DC_HOSTNAME_CHANGE',	1, 1,")
         _T("'Host name was changed from %3 to %4',")
         _T("'Custom data collection threshold event.#0D#0AParameters:#0D#0A")
		   _T("   1) Parameter name#0D#0A   2) Item description#0D#0A")
         _T("   3) Threshold value#0D#0A   4) Actual value#0D#0A")
         _T("   5) Data collection item ID#0D#0A   6) Instance')\n")
      _T("INSERT INTO events (event_id,name,severity,flags,message,description) VALUES ")
	      _T("(4008, 'DC_FILE_CHANGE',	1, 1,")
         _T("'File #22%6#22 was changed',")
         _T("'Custom data collection threshold event.#0D#0AParameters:#0D#0A")
		   _T("   1) Parameter name#0D#0A   2) Item description#0D#0A")
         _T("   3) Threshold value#0D#0A   4) Actual value#0D#0A")
         _T("   5) Data collection item ID#0D#0A   6) Instance')\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;
   if (!SQLQuery(_T("UPDATE config SET var_value='16' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;
   return TRUE;
}

/**
 * Upgrade from V14 to V15
 */
static BOOL H_UpgradeFromV14(int currVersion, int newVersion)
{
   static TCHAR m_szBatch[] =
      _T("ALTER TABLE items ADD instance varchar(255)\n")
      _T("UPDATE items SET instance=''\n")
      _T("INSERT INTO config (var_name,var_value,is_visible,need_server_restart) VALUES ")
         _T("('SMTPServer','localhost',1,0)\n")
      _T("INSERT INTO config (var_name,var_value,is_visible,need_server_restart) VALUES ")
         _T("('SMTPFromAddr','netxms@localhost',1,0)\n")
      _T("INSERT INTO events (event_id,name,severity,flags,message,description) VALUES ")
	      _T("(4003, 'DC_AGENT_RESTARTED',	0, 1,")
         _T("'NetXMS agent was restarted within last 5 minutes',")
         _T("'Custom data collection threshold event.#0D#0AParameters:#0D#0A")
		   _T("   1) Parameter name#0D#0A   2) Item description#0D#0A")
         _T("   3) Threshold value#0D#0A   4) Actual value#0D#0A")
         _T("   5) Data collection item ID#0D#0A   6) Instance')\n")
      _T("INSERT INTO events (event_id,name,severity,flags,message,description) VALUES ")
	      _T("(4004, 'DC_SERVICE_NOT_RUNNING', 3, 1,")
		   _T("'Service #22%6#22 is not running',")
		   _T("'Custom data collection threshold event.#0D#0AParameters:#0D#0A")
		   _T("   1) Parameter name#0D#0A   2) Item description#0D#0A")
         _T("   3) Threshold value#0D#0A   4) Actual value#0D#0A")
         _T("   5) Data collection item ID#0D#0A   6) Instance')\n")
      _T("UPDATE events SET ")
         _T("description='Generated when threshold value reached ")
         _T("for specific data collection item.#0D#0A")
         _T("   1) Parameter name#0D#0A   2) Item description#0D#0A")
         _T("   3) Threshold value#0D#0A   4) Actual value#0D#0A")
         _T("   5) Data collection item ID#0D#0A   6) Instance' WHERE ")
         _T("event_id=17 OR (event_id>=4000 AND event_id<5000)\n")
      _T("UPDATE events SET ")
         _T("description='Generated when threshold check is rearmed ")
         _T("for specific data collection item.#0D#0A")
		   _T("Parameters:#0D#0A   1) Parameter name#0D#0A")
		   _T("   2) Item description#0D#0A   3) Data collection item ID' ")
         _T("WHERE event_id=18\n")
      _T("<END>");

   if (!SQLBatch(m_szBatch))
      if (!g_bIgnoreErrors)
         return FALSE;
   if (!SQLQuery(_T("UPDATE config SET var_value='15' WHERE var_name='DBFormatVersion'")))
      if (!g_bIgnoreErrors)
         return FALSE;
   return TRUE;
}

/**
 * Upgrade map
 */
static struct
{
   int version;
   int newVersion;
   BOOL (* fpProc)(int, int);
} m_dbUpgradeMap[] =
{
   { 14, 15, H_UpgradeFromV14 },
   { 15, 16, H_UpgradeFromV15 },
   { 16, 17, H_UpgradeFromV16 },
   { 17, 18, H_UpgradeFromV17 },
   { 18, 19, H_UpgradeFromV18 },
   { 19, 20, H_UpgradeFromV19 },
   { 20, 21, H_UpgradeFromV20 },
   { 21, 22, H_UpgradeFromV21 },
   { 22, 23, H_UpgradeFromV22 },
   { 23, 24, H_UpgradeFromV23 },
   { 24, 25, H_UpgradeFromV24 },
   { 25, 26, H_UpgradeFromV25 },
   { 26, 27, H_UpgradeFromV26 },
   { 27, 28, H_UpgradeFromV27 },
   { 28, 29, H_UpgradeFromV28 },
   { 29, 30, H_UpgradeFromV29 },
   { 30, 31, H_UpgradeFromV30 },
   { 31, 32, H_UpgradeFromV31 },
   { 32, 33, H_UpgradeFromV32 },
   { 33, 34, H_UpgradeFromV33 },
   { 34, 35, H_UpgradeFromV34 },
   { 35, 36, H_UpgradeFromV35 },
   { 36, 37, H_UpgradeFromV36 },
   { 37, 38, H_UpgradeFromV37 },
   { 38, 39, H_UpgradeFromV38 },
   { 39, 40, H_UpgradeFromV39 },
   { 40, 41, H_UpgradeFromV40 },
   { 41, 42, H_UpgradeFromV41 },
   { 42, 43, H_UpgradeFromV42 },
   { 43, 44, H_UpgradeFromV43 },
   { 44, 45, H_UpgradeFromV44 },
   { 45, 46, H_UpgradeFromV45 },
   { 46, 47, H_UpgradeFromV46 },
   { 47, 48, H_UpgradeFromV47 },
   { 48, 49, H_UpgradeFromV48 },
   { 49, 50, H_UpgradeFromV49 },
   { 50, 51, H_UpgradeFromV50 },
   { 51, 52, H_UpgradeFromV51 },
   { 52, 53, H_UpgradeFromV52 },
   { 53, 54, H_UpgradeFromV53 },
   { 54, 55, H_UpgradeFromV54 },
   { 55, 56, H_UpgradeFromV55 },
   { 56, 57, H_UpgradeFromV56 },
   { 57, 58, H_UpgradeFromV57 },
   { 58, 59, H_UpgradeFromV58 },
   { 59, 60, H_UpgradeFromV59 },
   { 60, 61, H_UpgradeFromV60 },
   { 61, 62, H_UpgradeFromV61 },
   { 62, 63, H_UpgradeFromV62 },
   { 63, 64, H_UpgradeFromV63 },
   { 64, 65, H_UpgradeFromV64 },
   { 65, 66, H_UpgradeFromV65 },
   { 66, 67, H_UpgradeFromV66 },
   { 67, 68, H_UpgradeFromV67 },
   { 68, 69, H_UpgradeFromV68 },
   { 69, 70, H_UpgradeFromV69 },
   { 70, 71, H_UpgradeFromV70 },
   { 71, 72, H_UpgradeFromV71 },
   { 72, 73, H_UpgradeFromV72 },
   { 73, 74, H_UpgradeFromV73 },
   { 74, 75, H_UpgradeFromV74 },
   { 75, 76, H_UpgradeFromV75 },
   { 76, 77, H_UpgradeFromV76 },
   { 77, 78, H_UpgradeFromV77 },
   { 78, 79, H_UpgradeFromV78 },
   { 79, 80, H_UpgradeFromV79 },
   { 80, 81, H_UpgradeFromV80 },
   { 81, 82, H_UpgradeFromV81 },
   { 82, 83, H_UpgradeFromV82 },
   { 83, 84, H_UpgradeFromV83 },
   { 84, 85, H_UpgradeFromV84 },
	{ 85, 86, H_UpgradeFromV85 },
	{ 86, 87, H_UpgradeFromV86 },
	{ 87, 88, H_UpgradeFromV87 },
	{ 88, 89, H_UpgradeFromV88 },
	{ 89, 90, H_UpgradeFromV89 },
	{ 90, 91, H_UpgradeFromV90 },
	{ 91, 92, H_UpgradeFromV91 },
	{ 92, 200, H_UpgradeFromV9x },
	{ 93, 201, H_UpgradeFromV9x },
	{ 94, 202, H_UpgradeFromV9x },
	{ 95, 203, H_UpgradeFromV9x },
	{ 96, 204, H_UpgradeFromV9x },
	{ 97, 205, H_UpgradeFromV9x },
	{ 98, 206, H_UpgradeFromV9x },
	{ 99, 207, H_UpgradeFromV9x },
	{ 100, 214, H_UpgradeFromV10x },
	{ 101, 214, H_UpgradeFromV10x },
	{ 102, 214, H_UpgradeFromV10x },
	{ 103, 214, H_UpgradeFromV10x },
	{ 104, 214, H_UpgradeFromV10x },
	{ 105, 217, H_UpgradeFromV105 },
	{ 200, 201, H_UpgradeFromV200 },
	{ 201, 202, H_UpgradeFromV201 },
	{ 202, 203, H_UpgradeFromV202 },
	{ 203, 204, H_UpgradeFromV203 },
	{ 204, 205, H_UpgradeFromV204 },
	{ 205, 206, H_UpgradeFromV205 },
	{ 206, 207, H_UpgradeFromV206 },
	{ 207, 208, H_UpgradeFromV207 },
	{ 208, 209, H_UpgradeFromV208 },
	{ 209, 210, H_UpgradeFromV209 },
	{ 210, 211, H_UpgradeFromV210 },
	{ 211, 212, H_UpgradeFromV211 },
	{ 212, 213, H_UpgradeFromV212 },
	{ 213, 214, H_UpgradeFromV213 },
	{ 214, 215, H_UpgradeFromV214 },
	{ 215, 216, H_UpgradeFromV215 },
	{ 216, 217, H_UpgradeFromV216 },
	{ 217, 218, H_UpgradeFromV217 },
	{ 218, 219, H_UpgradeFromV218 },
	{ 219, 220, H_UpgradeFromV219 },
	{ 220, 221, H_UpgradeFromV220 },
	{ 221, 222, H_UpgradeFromV221 },
	{ 222, 223, H_UpgradeFromV222 },
	{ 223, 224, H_UpgradeFromV223 },
	{ 224, 225, H_UpgradeFromV224 },
	{ 225, 226, H_UpgradeFromV225 },
	{ 226, 227, H_UpgradeFromV226 },
	{ 227, 228, H_UpgradeFromV227 },
	{ 228, 229, H_UpgradeFromV228 },
	{ 229, 230, H_UpgradeFromV229 },
	{ 230, 231, H_UpgradeFromV230 },
	{ 231, 232, H_UpgradeFromV231 },
	{ 232, 238, H_UpgradeFromV232toV238 },
	{ 233, 234, H_UpgradeFromV233 },
	{ 234, 235, H_UpgradeFromV234 },
	{ 235, 236, H_UpgradeFromV235 },
	{ 236, 237, H_UpgradeFromV236 },
	{ 237, 238, H_UpgradeFromV237 },
	{ 238, 239, H_UpgradeFromV238 },
	{ 239, 240, H_UpgradeFromV239 },
	{ 240, 241, H_UpgradeFromV240 },
	{ 241, 242, H_UpgradeFromV241 },
	{ 242, 243, H_UpgradeFromV242 },
	{ 243, 244, H_UpgradeFromV243 },
	{ 244, 245, H_UpgradeFromV244 },
	{ 245, 246, H_UpgradeFromV245 },
	{ 246, 247, H_UpgradeFromV246 },
	{ 247, 248, H_UpgradeFromV247 },
	{ 248, 249, H_UpgradeFromV248 },
	{ 249, 250, H_UpgradeFromV249 },
	{ 250, 251, H_UpgradeFromV250 },
	{ 251, 252, H_UpgradeFromV251 },
	{ 252, 253, H_UpgradeFromV252 },
	{ 253, 254, H_UpgradeFromV253 },
	{ 254, 255, H_UpgradeFromV254 },
	{ 255, 256, H_UpgradeFromV255 },
	{ 256, 257, H_UpgradeFromV256 },
	{ 257, 258, H_UpgradeFromV257 },
	{ 258, 259, H_UpgradeFromV258 },
	{ 259, 260, H_UpgradeFromV259 },
	{ 260, 261, H_UpgradeFromV260 },
	{ 261, 262, H_UpgradeFromV261 },
	{ 262, 263, H_UpgradeFromV262 },
	{ 263, 264, H_UpgradeFromV263 },
	{ 264, 265, H_UpgradeFromV264 },
	{ 265, 266, H_UpgradeFromV265 },
	{ 266, 267, H_UpgradeFromV266 },
	{ 267, 268, H_UpgradeFromV267 },
	{ 268, 269, H_UpgradeFromV268 },
	{ 269, 270, H_UpgradeFromV269 },
   { 270, 271, H_UpgradeFromV270 },
   { 271, 272, H_UpgradeFromV271 },
   { 272, 273, H_UpgradeFromV272 },
   { 273, 274, H_UpgradeFromV273 },
   { 274, 275, H_UpgradeFromV274 },
   { 0, 0, NULL }
};

/**
 * Upgrade database to new version
 */
void UpgradeDatabase()
{
   DB_RESULT hResult;
   LONG i, iVersion = 0;
   BOOL bLocked = FALSE;
   TCHAR szTemp[MAX_DB_STRING];

   _tprintf(_T("Upgrading database...\n"));

   // Get database format version
	iVersion = DBGetSchemaVersion(g_hCoreDB);
   if (iVersion == DB_FORMAT_VERSION)
   {
      _tprintf(_T("Your database format is up to date\n"));
   }
   else if (iVersion > DB_FORMAT_VERSION)
   {
       _tprintf(_T("Your database has format version %d, this tool is compiled for version %d.\n")
                   _T("You need to upgrade your server before using this database.\n"),
                (int)iVersion, DB_FORMAT_VERSION);
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
               if (m_dbUpgradeMap[i].version == iVersion)
                  break;
            if (m_dbUpgradeMap[i].fpProc == NULL)
            {
               _tprintf(_T("Unable to find upgrade procedure for version %d\n"), (int)iVersion);
               break;
            }
            _tprintf(_T("Upgrading from version %d to %d\n"), (int)iVersion, m_dbUpgradeMap[i].newVersion);
            DBBegin(g_hCoreDB);
            if (m_dbUpgradeMap[i].fpProc(iVersion, m_dbUpgradeMap[i].newVersion))
            {
               DBCommit(g_hCoreDB);
               iVersion = DBGetSchemaVersion(g_hCoreDB);
            }
            else
            {
               _tprintf(_T("Rolling back last stage due to upgrade errors...\n"));
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
