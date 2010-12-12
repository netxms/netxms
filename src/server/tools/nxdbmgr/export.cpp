/* 
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2010 Victor Kirhenshtein
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
** File: export.cpp
**
**/

#include "nxdbmgr.h"
#include "sqlite3.h"


//
// Tables for export/import/clear
//

const TCHAR *g_tables[] = 
{
	_T("config"),
	_T("config_clob"),
	_T("users"),
	_T("user_groups"),
	_T("user_group_members"),
	_T("user_profiles"),
	_T("userdb_custom_attributes"),
	_T("object_properties"),
	_T("object_custom_attributes"),
	_T("zones"),
	_T("zone_ip_addr_list"),
	_T("nodes"),
	_T("clusters"),
	_T("cluster_members"),
	_T("cluster_sync_subnets"),
	_T("cluster_resources"),
	_T("subnets"),
	_T("interfaces"),
	_T("network_services"),
	_T("vpn_connectors"),
	_T("vpn_connector_networks"),
	_T("containers"),
	_T("conditions"),
	_T("cond_dci_map"),
	_T("templates"),
	_T("dct_node_map"),
	_T("nsmap"),
	_T("container_members"),
	_T("container_categories"),
	_T("acl"),
	_T("trusted_nodes"),
	_T("items"),
	_T("dci_schedules"),
	_T("raw_dci_values"),
	_T("event_cfg"),
	_T("event_log"),
	_T("actions"),
	_T("event_groups"),
	_T("event_group_members"),
	_T("event_policy"),
	_T("policy_source_list"),
	_T("policy_event_list"),
	_T("policy_action_list"),
	_T("policy_time_range_list"),
	_T("policy_situation_attr_list"),
	_T("time_ranges"),
	_T("deleted_objects"),
	_T("thresholds"),
	_T("alarms"),
	_T("alarm_notes"),
	_T("oid_to_type"),
	_T("snmp_trap_cfg"),
	_T("snmp_trap_pmap"),
	_T("agent_pkg"),
	_T("object_tools"),
	_T("object_tools_acl"),
	_T("object_tools_table_columns"),
	_T("syslog"),
	_T("script_library"),
	_T("snmp_trap_log"),
	_T("maps"),
	_T("map_access_lists"),
	_T("submaps"),
	_T("submap_object_positions"),
	_T("submap_links"),
	_T("agent_configs"),
	_T("address_lists"),
	_T("graphs"),
	_T("graph_acl"),
	_T("certificates"),
	_T("audit_log"),
	_T("situations"),
	_T("snmp_communities"),
	_T("web_maps"),
	_T("ap_common"),
	_T("ap_bindings"),
	_T("ap_config_files"),
	_T("usm_credentials"),
	NULL
};


//
// Escape string for SQLite
//

static TCHAR *EscapeString(const TCHAR *str)
{
	int len = (int)_tcslen(str) + 3;   // + two quotes and \0 at the end
	int bufferSize = len + 128;
	TCHAR *out = (TCHAR *)malloc(bufferSize * sizeof(TCHAR));
	out[0] = _T('\'');

	const TCHAR *src = str;
	int outPos;
	for(outPos = 1; *src != 0; src++)
	{
		if (*src == _T('\''))
		{
			len++;
			if (len >= bufferSize)
			{
				bufferSize += 128;
				out = (TCHAR *)realloc(out, bufferSize * sizeof(TCHAR));
			}
			out[outPos++] = _T('\'');
			out[outPos++] = _T('\'');
		}
		else
		{
			out[outPos++] = *src;
		}
	}
	out[outPos++] = _T('\'');
	out[outPos++] = 0;

	return out;
}


//
// Export single database table
//

static BOOL ExportTable(sqlite3 *db, const TCHAR *name)
{
	String query;
	TCHAR buffer[256];
	char *errmsg;
	DB_ASYNC_RESULT hResult;
	int i, columnCount = 0;
	BOOL success = TRUE;

	_tprintf(_T("Exporting table %s\n"), name);

	if (sqlite3_exec(db, "BEGIN", NULL, NULL, &errmsg) == SQLITE_OK)
	{
		_sntprintf(buffer, 256, _T("SELECT * FROM %s"), name);

		hResult = SQLAsyncSelect(buffer);
		if (hResult != NULL)
		{
			while(DBFetch(hResult))
			{
				query = _T("");

				// Column names
				columnCount = DBGetColumnCountAsync(hResult);
				query.addFormattedString(_T("INSERT INTO %s ("), name);
				for(i = 0; i < columnCount; i++)
				{
					DBGetColumnNameAsync(hResult, i, buffer, 256);
					query += buffer;
					query += _T(",");
				}
				query.shrink();
				query += _T(") VALUES (");

				// Data
				TCHAR data[8192];
				for(i = 0; i < columnCount; i++)
				{
					TCHAR *escapedString = EscapeString(DBGetFieldAsync(hResult, i, data, 8192));
					query.addDynamicString(escapedString);
					query += _T(",");
				}
				query.shrink();
				query += _T(")");

				char *utf8query = query.getUTF8String();
				if (sqlite3_exec(db, utf8query, NULL, NULL, &errmsg) != SQLITE_OK)
				{
					free(utf8query);
					printf("ERROR: SQLite query failed: %s\n   Query: %s\n", errmsg, utf8query);
					sqlite3_free(errmsg);
					success = FALSE;
					break;
				}
				free(utf8query);
			}
			DBFreeAsyncResult(g_hCoreDB, hResult);

			if (success)
			{
				if (sqlite3_exec(db, "COMMIT", NULL, NULL, &errmsg) != SQLITE_OK)
				{
					printf("ERROR: Cannot commit transaction: %s", errmsg);
					sqlite3_free(errmsg);
					success = FALSE;
				}
			}
			else
			{
				if (sqlite3_exec(db, "ROLLBACK", NULL, NULL, &errmsg) != SQLITE_OK)
				{
					printf("ERROR: Cannot rollback transaction: %s", errmsg);
					sqlite3_free(errmsg);
				}
			}
		}
		else
		{
			success = FALSE;
			if (sqlite3_exec(db, "ROLLBACK", NULL, NULL, &errmsg) != SQLITE_OK)
			{
				printf("ERROR: Cannot rollback transaction: %s", errmsg);
				sqlite3_free(errmsg);
			}
		}
	}
	else
	{
		success = FALSE;
		printf("ERROR: Cannot start transaction: %s", errmsg);
		sqlite3_free(errmsg);
	}

	return success;
}


//
// Callback for getting schema version
//

static int GetSchemaVersionCB(void *arg, int cols, char **data, char **names)
{
	*((int *)arg) = strtol(data[0], NULL, 10);
	return 0;
}


//
// Callback for getting idata_xx table creation query
//

static int GetIDataQueryCB(void *arg, int cols, char **data, char **names)
{
	strncpy((char *)arg, data[0], MAX_DB_STRING);
	((char *)arg)[MAX_DB_STRING - 1] = 0;
	return 0;
}


//
// Export database
//

void ExportDatabase(const char *file)
{
	sqlite3 *db;
	char *errmsg, buffer[MAX_PATH], queryTemplate[MAX_DB_STRING], *data;
	TCHAR idataTable[128];
	DWORD size;
	int i, rowCount, version = 0;
	DB_RESULT hResult;
	BOOL success = FALSE;

	if (!ValidateDatabase())
		return;

	// Create new SQLite database
	unlink(file);
	if (sqlite3_open(file, &db) != SQLITE_OK)
	{
		printf("ERROR: unable to open output file\n");
		return;
	}

	// Setup database schema
#ifdef _WIN32
	HKEY hKey;

   // Read installation data from registry
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\NetXMS\\Server"), 0,
                    KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
   {
      size = (MAX_PATH - 16) * sizeof(TCHAR);
      if (RegQueryValueEx(hKey, _T("InstallPath"), NULL, NULL, 
                          (BYTE *)buffer, &size) == ERROR_SUCCESS)
      {
			strcat(buffer, "\\lib\\sql\\dbschema_sqlite.sql");
			success = TRUE;
      }
      RegCloseKey(hKey);
   }

	if (!success)
	{
		// Try to use path to nxdbmgr executable as base
		if (GetModuleFileNameA(NULL, buffer, MAX_PATH - 32) != 0)
		{
			char *p;

			p = strrchr(buffer, '\\');
			if (p != NULL)
				*p = 0;
			p = strrchr(buffer, '\\');
			if (p != NULL)
				*p = 0;
			strcat(buffer, "\\lib\\sql\\dbschema_sqlite.sql");
			success = TRUE;
		}
	}

	if (!success)
	{
		printf("ERROR: unable to determine path to schema file\n");
		goto cleanup;
	}

	success = FALSE;	// Reset success flag
#else
	strcpy(buffer, DATADIR "/sql/dbschema_sqlite.sql");
#endif

	data = (char *)LoadFileA(buffer, &size);
	if (data == NULL)
	{
		printf("ERROR: cannot load schema file \"%s\"\n", buffer);
		goto cleanup;
	}

	if (sqlite3_exec(db, data, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		printf("ERROR: unable to apply database schema: %s\n", errmsg);
		sqlite3_free(errmsg);
		goto cleanup;
	}

	free(data);

	// Check that dbschema_sqlite.sql and database have the same schema version
	if (sqlite3_exec(db, "SELECT var_value FROM metadata WHERE var_name='SchemaVersion'", GetSchemaVersionCB, &version, &errmsg) != SQLITE_OK)
	{
		printf("ERROR: SQL query failed (%s)\n", errmsg);
		sqlite3_free(errmsg);
		goto cleanup;
	}
	if (version != DBGetSchemaVersion(g_hCoreDB))
	{
		printf("ERROR: Schema version mismatch between dbschema_sqlite.sql and your database. Please check that NetXMS server installed correctly.\n");
		goto cleanup;
	}

	// Export tables
	for(i = 0; g_tables[i] != NULL; i++)
	{
		if (!ExportTable(db, g_tables[i]))
			goto cleanup;
	}

	// Export tables with collected DCI data
	if (sqlite3_exec(db, "SELECT var_value FROM metadata WHERE var_name='IDataTableCreationCommand'",
	                 GetIDataQueryCB, queryTemplate, &errmsg) != SQLITE_OK)
	{
		printf("ERROR: SQL query failed (%s)\n", errmsg);
		sqlite3_free(errmsg);
		goto cleanup;
	}

	hResult = SQLSelect(_T("SELECT id FROM nodes"));
	if (hResult == NULL)
		goto cleanup;

	rowCount = DBGetNumRows(hResult);
	for(i = 0; i < rowCount; i++)
	{
		snprintf(buffer, MAX_PATH, queryTemplate, DBGetFieldLong(hResult, i, 0));
		if (sqlite3_exec(db, buffer, NULL, NULL, &errmsg) != SQLITE_OK)
		{
			_tprintf(_T("ERROR: SQL query failed: %s (%s)\n"), buffer, errmsg);
			sqlite3_free(errmsg);
			DBFreeResult(hResult);
			goto cleanup;
		}

		_sntprintf(idataTable, 128, _T("idata_%d"), DBGetFieldLong(hResult, i, 0));
		if (!ExportTable(db, idataTable))
		{
			DBFreeResult(hResult);
			goto cleanup;
		}
	}

	DBFreeResult(hResult);

	success = TRUE;

cleanup:
	sqlite3_close(db);
	printf(success ? "Database export complete.\n" : "Database export failed.\n");
}
