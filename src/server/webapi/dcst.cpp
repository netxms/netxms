/*
** NetXMS - Network Management System
** Copyright (C) 2023 Raden Solutions
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
** File: dcst.cpp
**
**/

#include "webapi.h"

/**
 * Get list of summary tables
 */
int H_SummaryTables(Context *context)
{
   int responseCode;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT id,menu_path,title,flags,guid FROM dci_summary_tables"));
   if (hResult != nullptr)
   {
      json_t *output = json_array();

      TCHAR buffer[256];
      int32_t count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         json_t *element = json_object();
         json_object_set_new(element, "id", json_integer(DBGetFieldULong(hResult, i, 0)));
         json_object_set_new(element, "menuPath", json_string_t(DBGetField(hResult, i, 1, buffer, 256)));
         json_object_set_new(element, "title", json_string_t(DBGetField(hResult, i, 2, buffer, 256)));
         json_object_set_new(element, "flags", json_integer(DBGetFieldULong(hResult, i, 3)));
         json_object_set_new(element, "guid", json_string_t(DBGetFieldGUID(hResult, i, 4).toString()));
         json_array_append_new(output, element);
      }
      DBFreeResult(hResult);

      context->setResponseData(output);
      json_decref(output);
      responseCode = 200;
   }
   else
   {
      context->setErrorResponse("Database failure");
      responseCode = 500;
   }
   DBConnectionPoolReleaseConnection(hdb);
   return responseCode;
}

/**
 * Build serialized column string from JSON columns array
 * Format: name^#^dciName^#^flags^#^separator separated by ^~^
 */
static StringBuffer BuildColumnListFromJson(json_t *columns)
{
   StringBuffer result;
   if (!json_is_array(columns))
      return result;

   size_t index;
   json_t *column;
   json_array_foreach(columns, index, column)
   {
      if (!json_is_object(column))
         continue;

      if (index > 0)
         result.append(_T("^~^"));

      TCHAR name[MAX_DB_STRING], dciName[MAX_PARAM_NAME], separator[16];
      utf8_to_tchar(json_object_get_string_utf8(column, "name", ""), -1, name, MAX_DB_STRING);
      utf8_to_tchar(json_object_get_string_utf8(column, "dciName", ""), -1, dciName, MAX_PARAM_NAME);
      utf8_to_tchar(json_object_get_string_utf8(column, "separator", ";"), -1, separator, 16);
      uint32_t flags = json_object_get_uint32(column, "flags");

      result.appendFormattedString(_T("%s^#^%s^#^%u^#^%s"), name, dciName, flags, separator);
   }
   return result;
}

/**
 * Get summary table details
 */
int H_SummaryTableDetails(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_SUMMARY_TBLS))
      return 403;

   uint32_t tableId = context->getPlaceholderValueAsUInt32(L"table-id");
   if (tableId == 0)
   {
      context->setErrorResponse("Invalid summary table ID");
      return 400;
   }

   uint32_t rcc;
   json_t *details = GetSummaryTableDetails(tableId, &rcc);
   if (details == nullptr)
   {
      if (rcc == RCC_INVALID_SUMMARY_TABLE_ID)
         return 404;
      context->setErrorResponse("Database failure");
      return 500;
   }

   context->setResponseData(details);
   json_decref(details);
   return 200;
}

/**
 * Create summary table
 */
int H_SummaryTableCreate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_SUMMARY_TBLS))
      return 403;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   json_t *jsonTitle = json_object_get(request, "title");
   if (!json_is_string(jsonTitle))
   {
      context->setErrorResponse("Missing or invalid title field");
      return 400;
   }

   TCHAR title[MAX_DB_STRING];
   utf8_to_tchar(json_string_value(jsonTitle), -1, title, MAX_DB_STRING);

   TCHAR menuPath[MAX_DB_STRING];
   utf8_to_tchar(json_object_get_string_utf8(request, "menuPath", ""), -1, menuPath, MAX_DB_STRING);

   TCHAR *nodeFilter = json_object_get_string_t(request, "nodeFilter", _T(""));

   uint32_t flags = json_object_get_uint32(request, "flags");

   TCHAR tableDciName[MAX_PARAM_NAME];
   utf8_to_tchar(json_object_get_string_utf8(request, "tableDciName", ""), -1, tableDciName, MAX_PARAM_NAME);

   StringBuffer columnList = BuildColumnListFromJson(json_object_get(request, "columns"));

   uint32_t newId;
   uint32_t rcc = ModifySummaryTable(0, menuPath, title, nodeFilter, flags, columnList.cstr(), tableDciName, &newId);
   MemFree(nodeFilter);

   if (rcc == RCC_SUCCESS)
   {
      context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"DCI summary table %u created", newId);
      json_t *output = json_object();
      json_object_set_new(output, "id", json_integer(newId));
      context->setResponseData(output);
      json_decref(output);
      return 201;
   }

   context->setErrorResponse("Database failure");
   return 500;
}

/**
 * Update summary table
 */
int H_SummaryTableUpdate(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_SUMMARY_TBLS))
      return 403;

   uint32_t tableId = context->getPlaceholderValueAsUInt32(L"table-id");
   if (tableId == 0)
   {
      context->setErrorResponse("Invalid summary table ID");
      return 400;
   }

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   bool exists = IsDatabaseRecordExist(hdb, _T("dci_summary_tables"), _T("id"), tableId);
   DBConnectionPoolReleaseConnection(hdb);
   if (!exists)
      return 404;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   json_t *jsonTitle = json_object_get(request, "title");
   if (!json_is_string(jsonTitle))
   {
      context->setErrorResponse("Missing or invalid title field");
      return 400;
   }

   TCHAR title[MAX_DB_STRING];
   utf8_to_tchar(json_string_value(jsonTitle), -1, title, MAX_DB_STRING);

   TCHAR menuPath[MAX_DB_STRING];
   utf8_to_tchar(json_object_get_string_utf8(request, "menuPath", ""), -1, menuPath, MAX_DB_STRING);

   TCHAR *nodeFilter = json_object_get_string_t(request, "nodeFilter", _T(""));

   uint32_t flags = json_object_get_uint32(request, "flags");

   TCHAR tableDciName[MAX_PARAM_NAME];
   utf8_to_tchar(json_object_get_string_utf8(request, "tableDciName", ""), -1, tableDciName, MAX_PARAM_NAME);

   StringBuffer columnList = BuildColumnListFromJson(json_object_get(request, "columns"));

   uint32_t newId;
   uint32_t rcc = ModifySummaryTable(tableId, menuPath, title, nodeFilter, flags, columnList.cstr(), tableDciName, &newId);
   MemFree(nodeFilter);

   if (rcc == RCC_SUCCESS)
   {
      context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"DCI summary table %u updated", tableId);
      return 200;
   }

   context->setErrorResponse("Database failure");
   return 500;
}

/**
 * Delete summary table
 */
int H_SummaryTableDelete(Context *context)
{
   if (!context->checkSystemAccessRights(SYSTEM_ACCESS_MANAGE_SUMMARY_TBLS))
      return 403;

   uint32_t tableId = context->getPlaceholderValueAsUInt32(L"table-id");
   if (tableId == 0)
   {
      context->setErrorResponse("Invalid summary table ID");
      return 400;
   }

   uint32_t rcc = DeleteSummaryTable(tableId);
   if (rcc == RCC_SUCCESS)
   {
      context->writeAuditLog(AUDIT_SYSCFG, true, 0, L"DCI summary table %u deleted", tableId);
      return 204;
   }

   context->setErrorResponse("Database failure");
   return 500;
}

/**
 * Common implementation for query execution
 */
static int ExecuteSummaryTableQuery(Context *context, uint32_t tableId, SummaryTable *tableDefinition)
{
   uint32_t rcc;
   Table *result = QuerySummaryTable(tableId, tableDefinition, json_object_get_uint32(context->getRequestDocument(), "objectId"), context->getUserId(), &rcc);
   if (result == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("ExecuteSummaryTableQuery(ad-hoc=%s, id=%u): call to QuerySummaryTable failed (RCC = %u)"), BooleanToString(tableDefinition != nullptr), tableId, rcc);
      json_t *response = json_object();
      json_object_set_new(response, "reason", json_string("Query error"));
      json_object_set_new(response, "errorCode", json_integer(rcc));
      context->setResponseData(response);
      json_decref(response);
      return (rcc == RCC_ACCESS_DENIED) ? 403: 500;
   }

   nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("ExecuteSummaryTableQuery(ad-hoc=%s, id=%u): %d rows in resulting table"), BooleanToString(tableDefinition != nullptr), tableId, result->getNumRows());
   json_t *response = result->toJson();
   context->setResponseData(response);
   json_decref(response);
   delete result;
   return 200;
}

/**
 * Query ad-hoc summary table
 */
int H_QueryAdHocSummaryTable(Context *context)
{
   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("H_QueryAdHocSummaryTable: empty request"));
      return 400;
   }

   json_t *definition = json_object_get(request, "tableDefinition");
   if (!json_is_object(definition))
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("H_QueryAdHocSummaryTable: missing table definition in request"));
      context->setErrorResponse("Missing summary table definition");
      return 400;
   }

   SummaryTable tableDefinition(definition);
   return ExecuteSummaryTableQuery(context, 0, &tableDefinition);
}

/**
 * Query summary table
 */
int H_QuerySummaryTable(Context *context)
{
   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("H_QuerySummaryTable: empty request"));
      return 400;
   }

   uint32_t tableId = context->getPlaceholderValueAsUInt32(_T("table-id"));
   if (tableId == 0)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("H_QuerySummaryTable: invalid summary table ID"));
      context->setErrorResponse("Invalid summary table ID");
      return 400;
   }

   return ExecuteSummaryTableQuery(context, tableId, nullptr);
}
