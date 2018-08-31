/*
** NetXMS database manager library
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

#include "libnxdbmgr.h"
#include <nxevent.h>

/**
 * Generate GUIDs
 */
bool LIBNXDBMGR_EXPORTABLE GenerateGUID(const TCHAR *table, const TCHAR *idColumn, const TCHAR *guidColumn, const GUID_MAPPING *mapping)
{
   TCHAR query[256];
   _sntprintf(query, 256, _T("SELECT %s FROM %s"), idColumn, table);
	DB_RESULT hResult = SQLSelect(query);
	if (hResult == NULL)
      return false;

   uuid_t guid;
   TCHAR buffer[64];

	int count = DBGetNumRows(hResult);
	for(int i = 0; i < count; i++)
	{
		const TCHAR *guidText = NULL;
		UINT32 id = DBGetFieldULong(hResult, i, 0);
		if (mapping != NULL)
		{
		   for(int j = 0; mapping[j].guid != NULL; j++)
		      if (mapping[j].id == id)
		      {
		         guidText = mapping[j].guid;
		         break;
		      }
		}
		if (guidText == NULL)
		{
		   _uuid_generate(guid);
		   guidText = _uuid_to_string(guid, buffer);
		}
		_sntprintf(query, 256, _T("UPDATE %s SET %s='%s' WHERE %s=%d"), table, guidColumn, guidText, idColumn, id);
		if (!SQLQuery(query))
      {
      	DBFreeResult(hResult);
         return false;
      }
	}
	DBFreeResult(hResult);
   return true;
}

/**
 * Create table
 */
bool LIBNXDBMGR_EXPORTABLE CreateTable(const TCHAR *pszQuery)
{
   String query(pszQuery);

   if (g_dbSyntax != DB_SYNTAX_UNKNOWN)
   {
      query.replace(_T("$SQL:TEXT"), g_sqlTypes[g_dbSyntax][SQL_TYPE_TEXT]);
      query.replace(_T("$SQL:TXT4K"), g_sqlTypes[g_dbSyntax][SQL_TYPE_TEXT4K]);
      query.replace(_T("$SQL:INT64"), g_sqlTypes[g_dbSyntax][SQL_TYPE_INT64]);
   }
   if (g_dbSyntax == DB_SYNTAX_MYSQL)
      query.append(g_tableSuffix);
   return SQLQuery(query);
}

/**
 * Check if configuration variable exist
 */
static bool IsConfigurationVariableExist(const TCHAR *name)
{
   bool found = false;
   TCHAR query[256];
   _sntprintf(query, 256, _T("SELECT var_value FROM config WHERE var_name='%s'"), name);
   DB_RESULT hResult = DBSelect(g_dbHandle, query);
   if (hResult != 0)
   {
      found = (DBGetNumRows(hResult) > 0);
      DBFreeResult(hResult);
   }
   return found;
}

/**
 * Create configuration parameter if it doesn't exist (unless forceUpdate set to true)
 */
bool LIBNXDBMGR_EXPORTABLE CreateConfigParam(const TCHAR *name, const TCHAR *value, const TCHAR *description, char dataType, bool isVisible, bool needRestart, bool isPublic, bool forceUpdate)
{
   bool success = true;
   TCHAR szQuery[3024];
   if (!IsConfigurationVariableExist(name))
   {
      INT32 major, minor;
      if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
         return false;
      if ((major > 0) || (minor >= 454))
         _sntprintf(szQuery, 3024, _T("INSERT INTO config (var_name,var_value,default_value,is_visible,need_server_restart,is_public,data_type,description) VALUES (%s,%s,%s,%d,%d,'%c','%c',%s)"),
                    (const TCHAR *)DBPrepareString(g_dbHandle, name, 63),
                    (const TCHAR *)DBPrepareString(g_dbHandle, value, 2000),
                    (const TCHAR *)DBPrepareString(g_dbHandle, value, 2000),
                    isVisible ? 1 : 0, needRestart ? 1 : 0,
                    isPublic ? _T('Y') : _T('N'), dataType, (const TCHAR *)DBPrepareString(g_dbHandle, description, 255));
      else
         _sntprintf(szQuery, 3024, _T("INSERT INTO config (var_name,var_value,is_visible,need_server_restart,is_public,data_type,description) VALUES (%s,%s,%d,%d,'%c','%c',%s)"),
                    (const TCHAR *)DBPrepareString(g_dbHandle, name, 63),
                    (const TCHAR *)DBPrepareString(g_dbHandle, value, 2000), isVisible ? 1 : 0, needRestart ? 1 : 0,
                    isPublic ? _T('Y') : _T('N'), dataType, (const TCHAR *)DBPrepareString(g_dbHandle, description, 255));
      success = SQLQuery(szQuery);
   }
	else if (forceUpdate)
	{
      _sntprintf(szQuery, 3024, _T("UPDATE config SET var_value=%s WHERE var_name=%s"),
                 (const TCHAR *)DBPrepareString(g_dbHandle, value, 2000), (const TCHAR *)DBPrepareString(g_dbHandle, name, 63));
      success = SQLQuery(szQuery);
	}
   return success;
}

/**
 * Create configuration parameter if it doesn't exist (unless forceUpdate set to true)
 */
bool LIBNXDBMGR_EXPORTABLE CreateConfigParam(const TCHAR *name, const TCHAR *value, bool isVisible, bool needRestart, bool forceUpdate)
{
   bool success = true;
   TCHAR szQuery[3024];
   if (!IsConfigurationVariableExist(name))
   {
      INT32 major, minor;
      if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
         return false;
      if ((major > 0) || (minor >= 454))
         _sntprintf(szQuery, 3024, _T("INSERT INTO config (var_name,var_value,default_value,is_visible,need_server_restart) VALUES (%s,%s,%s,%d,%d)"),
                    (const TCHAR *)DBPrepareString(g_dbHandle, name, 63),
                    (const TCHAR *)DBPrepareString(g_dbHandle, value, 2000),
                    (const TCHAR *)DBPrepareString(g_dbHandle, value, 2000),
                    isVisible ? 1 : 0, needRestart ? 1 : 0);
      else
         _sntprintf(szQuery, 3024, _T("INSERT INTO config (var_name,var_value,is_visible,need_server_restart) VALUES (%s,%s,%d,%d)"),
                    (const TCHAR *)DBPrepareString(g_dbHandle, name, 63),
                    (const TCHAR *)DBPrepareString(g_dbHandle, value, 2000), isVisible ? 1 : 0, needRestart ? 1 : 0);
      success = SQLQuery(szQuery);
   }
   else if (forceUpdate)
   {
      _sntprintf(szQuery, 3024, _T("UPDATE config SET var_value=%s WHERE var_name=%s"),
                 (const TCHAR *)DBPrepareString(g_dbHandle, value, 2000), (const TCHAR *)DBPrepareString(g_dbHandle, name, 63));
      success = SQLQuery(szQuery);
   }
   return success;
}

/**
 * Convert strings from # encoded form to normal form
 */
bool LIBNXDBMGR_EXPORTABLE ConvertStrings(const TCHAR *table, const TCHAR *idColumn, const TCHAR *idColumn2, const TCHAR *column, bool isStringId)
{
	DB_RESULT hResult;
	TCHAR *query;
	size_t queryLen = 512;
	BOOL success = FALSE;

	query = (TCHAR *)malloc(queryLen * sizeof(TCHAR));

	switch(g_dbSyntax)
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
			String newValue = DBPrepareString(g_dbHandle, value);
			if (newValue.length() + 256 > queryLen)
			{
				queryLen = newValue.length() + 256;
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
								  idColumn, (const TCHAR *)DBPrepareString(g_dbHandle, id),
								  idColumn2, (const TCHAR *)DBPrepareString(g_dbHandle, id2));
				}
				else
				{
					_sntprintf(query, queryLen, _T("UPDATE %s SET %s=%s WHERE %s=%s"), table, column,
								  (const TCHAR *)newValue, idColumn, (const TCHAR *)DBPrepareString(g_dbHandle, id));
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

bool LIBNXDBMGR_EXPORTABLE ConvertStrings(const TCHAR *table, const TCHAR *idColumn, const TCHAR *column)
{
	return ConvertStrings(table, idColumn, NULL, column, false);
}

/**
 * Create new event template
 */
bool LIBNXDBMGR_EXPORTABLE CreateEventTemplate(int code, const TCHAR *name, int severity, int flags, const TCHAR *guid, const TCHAR *message, const TCHAR *description)
{
	TCHAR query[4096];

	if (guid != NULL)
	{
      _sntprintf(query, 4096, _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description,guid) VALUES (%d,'%s',%d,%d,%s,%s,'%s')"),
                 code, name, severity, flags, (const TCHAR *)DBPrepareString(g_dbHandle, message),
                 (const TCHAR *)DBPrepareString(g_dbHandle, description), guid);
	}
	else
	{
      _sntprintf(query, 4096, _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,message,description) VALUES (%d,'%s',%d,%d,%s,%s)"),
                 code, name, severity, flags, (const TCHAR *)DBPrepareString(g_dbHandle, message),
                 (const TCHAR *)DBPrepareString(g_dbHandle, description));
	}
	return SQLQuery(query);
}

/**
 * Check if an event pair is handled by any EPP rules
 */
bool LIBNXDBMGR_EXPORTABLE IsEventPairInUse(UINT32 code1, UINT32 code2)
{
   TCHAR query[256];
   _sntprintf(query, 256, _T("SELECT count(*) FROM policy_event_list WHERE event_code=%d OR event_code=%d"), code1, code2);
   DB_RESULT hResult = SQLSelect(query);
   if (hResult == NULL)
      return false;
   bool inUse = (DBGetFieldLong(hResult, 0, 0) > 0);
   DBFreeResult(hResult);
   return inUse;
}

/**
 * Return the next free EPP rule ID
 */
int LIBNXDBMGR_EXPORTABLE NextFreeEPPruleID()
{
   int ruleId = 1;
	DB_RESULT hResult = SQLSelect(_T("SELECT max(rule_id) FROM event_policy"));
	if (hResult != NULL)
	{
	   ruleId = DBGetFieldLong(hResult, 0, 0) + 1;
		DBFreeResult(hResult);
	}
	return ruleId;
}

/**
 * Add event to EPP rule by rule GUID
 */
bool LIBNXDBMGR_EXPORTABLE AddEventToEPPRule(const TCHAR *guid, UINT32 eventCode)
{
   TCHAR query[256];
   _sntprintf(query, 256, _T("SELECT rule_id FROM event_policy WHERE rule_guid='%s'"), guid);
   DB_RESULT hResult = SQLSelect(query);
   if (hResult == NULL)
      return false;

   bool success = true;
   if (DBGetNumRows(hResult) > 0)
   {
      _sntprintf(query, 256, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), DBGetFieldLong(hResult, 0, 0), eventCode);
      success = SQLQuery(query);
   }
   DBFreeResult(hResult);
   return success;
}

/**
 * Create library script
 */
bool LIBNXDBMGR_EXPORTABLE CreateLibraryScript(UINT32 id, const TCHAR *guid, const TCHAR *name, const TCHAR *code)
{
   // Check if script exists
   TCHAR query[256];
   _sntprintf(query, 256, _T("SELECT script_id FROM script_library WHERE script_id=%d OR script_name=%s"),
              id, (const TCHAR *)DBPrepareString(g_dbHandle, name));
   DB_RESULT hResult = SQLSelect(query);
   if (hResult == NULL)
      return false;
   bool exist = (DBGetNumRows(hResult) > 0);
   DBFreeResult(hResult);
   if (exist)
      return true;

   DB_STATEMENT hStmt;
   if (guid != NULL)
   {
      hStmt = DBPrepare(g_dbHandle, _T("INSERT INTO script_library (script_id,guid,script_name,script_code) VALUES (?,?,?,?)"));
      if (hStmt == NULL)
         return false;

      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, guid, DB_BIND_STATIC);
      DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, name, DB_BIND_STATIC);
      DBBind(hStmt, 4, DB_SQLTYPE_TEXT, code, DB_BIND_STATIC);
   }
   else
   {
      hStmt = DBPrepare(g_dbHandle, _T("INSERT INTO script_library (script_id,script_name,script_code) VALUES (?,?,?)"));
      if (hStmt == NULL)
         return false;

      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, name, DB_BIND_STATIC);
      DBBind(hStmt, 3, DB_SQLTYPE_TEXT, code, DB_BIND_STATIC);
   }
   bool success = SQLExecute(hStmt);
   DBFreeStatement(hStmt);
   return success;
}
