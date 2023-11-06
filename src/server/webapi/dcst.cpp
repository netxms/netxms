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
