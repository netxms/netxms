/* 
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2009 Victor Kirhenshtein
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

TCHAR *g_tables[] = 
{
	"config",
	"config_clob",
	"users",
	"user_groups",
	"user_group_members",
	"user_profiles",
	"object_properties",
	"object_custom_attributes",
	"zones",
	"zone_ip_addr_list",
	"nodes",
	"clusters",
	"cluster_members",
	"cluster_sync_subnets",
	"cluster_resources",
	"subnets",
	"interfaces",
	"network_services",
	"vpn_connectors",
	"vpn_connector_networks",
	"containers",
	"conditions",
	"cond_dci_map",
	"templates",
	"dct_node_map",
	"nsmap",
	"container_members",
	"container_categories",
	"acl",
	"trusted_nodes",
	"items",
	"thresholds",
	"dci_schedules",
	"raw_dci_values",
	"event_cfg",
	"event_log",
	"actions",
	"event_groups",
	"event_group_members",
	"time_ranges",
	"event_policy",
	"policy_source_list",
	"policy_event_list",
	"policy_action_list",
	"policy_situation_attr_list",
	"policy_time_range_list",
	"deleted_objects",
	"alarms",
	"alarm_notes",
	"images",
	"default_images",
	"oid_to_type",
	"snmp_trap_cfg",
	"snmp_trap_pmap",
	"agent_pkg",
	"object_tools",
	"object_tools_acl",
	"object_tools_table_columns",
	"syslog",
	"script_library",
	"snmp_trap_log",
	"maps",
	"map_access_lists",
	"submaps",
	"submap_object_positions",
	"submap_links",
	"agent_configs",
	"address_lists",
	"graphs",
	"graph_acl",
	"certificates",
	"audit_log",
	"situations",
	"snmp_communities",
	"web_maps",
	NULL
};


//
// Export single database table
//

static BOOL ExportTable(sqlite3 *db, const TCHAR *name)
{
	String query;
	TCHAR buffer[256];
	char *errmsg, *data;
	DB_ASYNC_RESULT hResult;
	int i, columnCount = 0;
	BOOL success = TRUE;

	printf("Exporting table %s\n", name);

	if (sqlite3_exec(db, "BEGIN", NULL, NULL, &errmsg) == SQLITE_OK)
	{
		_sntprintf(buffer, 256, _T("SELECT * FROM %s"), name);

		hResult = SQLAsyncSelect(buffer);
		if (hResult != NULL)
		{
			while(DBFetch(hResult))
			{
				query = "";

				// Column names
				columnCount = DBGetColumnCountAsync(hResult);
				query.AddFormattedString("INSERT INTO %s (", name);
				for(i = 0; i < columnCount; i++)
				{
					DBGetColumnNameAsync(hResult, i, buffer, 256);
					query += buffer;
					query += ",";
				}
				query.Shrink();
				query += ") VALUES (";

				// Data
				for(i = 0; i < columnCount; i++)
				{
					query += "'";
					data = (char *)malloc(8192);
					query.AddDynamicString(DBGetFieldAsync(hResult, i, data, 8192));
					query += "',";
				}
				query.Shrink();
				query += ")";

				if (sqlite3_exec(db, query, NULL, NULL, &errmsg) != SQLITE_OK)
				{
					printf("ERROR: SQLite query failed: %s\n   Query: %s\n", errmsg, (const TCHAR *)query);
					sqlite3_free(errmsg);
					success = FALSE;
					break;
				}
			}
			DBFreeAsyncResult(g_hCoreDB, hResult);
			if (sqlite3_exec(db, "COMMIT", NULL, NULL, &errmsg) == SQLITE_OK)
			{
				success = TRUE;
			}
			else
			{
				printf("ERROR: Cannot commit transaction: %s", errmsg);
				sqlite3_free(errmsg);
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
		printf("ERROR: Cannot start transaction: %s", errmsg);
		sqlite3_free(errmsg);
	}

	return success;
}


//
// Export database
//

void ExportDatabase(const char *file)
{
	sqlite3 *db;
	char *errmsg, buffer[MAX_PATH], queryTemplate[256], *data;
	DWORD size;
	int i, rowCount;
	DB_RESULT hResult;
	BOOL success = FALSE;

	if (!ValidateDatabase())
		return;

	// Create new SQLite database
	_unlink(file);
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

	data = (char *)LoadFile(buffer, &size);
	if (data == NULL)
	{
		printf("ERROR: cannot load schema file \"%s\"\n", buffer);
		goto cleanup;
	}

	if (sqlite3_exec(db, data, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		_tprintf(_T("ERROR: unable to apply database schema: %s\n"), errmsg);
		sqlite3_free(errmsg);
		goto cleanup;
	}

	free(data);

	// Check that dbschema_sqlite.sql and database have the same schema version
	/* TODO */

	// Export tables
	for(i = 0; g_tables[i] != NULL; i++)
	{
		if (!ExportTable(db, g_tables[i]))
			goto cleanup;
	}

	// Export tables with collected DCI data
	hResult = SQLSelect(_T("SELECT var_value FROM metadata WHERE var_name='IDataTableCreationCommand'"));
	if (hResult == NULL)
		goto cleanup;
	DBGetField(hResult, 0, 0, queryTemplate, 256);
	DBFreeResult(hResult);

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

		snprintf(buffer, MAX_PATH, "idata_%d", DBGetFieldLong(hResult, i, 0));
		if (!ExportTable(db, buffer))
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
