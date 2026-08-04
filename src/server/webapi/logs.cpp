/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Raden Solutions
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
** File: logs.cpp
**
**/

#include "webapi.h"
#include <nxcore_logs.h>

/**
 * Default and maximum number of records returned by single query
 */
#define DEFAULT_LOG_QUERY_LIMIT   1000
#define MAX_LOG_QUERY_LIMIT       10000

/**
 * Maximum number of records read from database by single query. Paging is done by
 * re-executing the query and skipping requested number of records, so this also limits
 * how far into the result set client can page.
 */
#define MAX_LOG_QUERY_WINDOW      100000

/**
 * Create JSON document with basic log information
 */
static json_t *CreateLogSummary(const NXCORE_LOG *log)
{
   json_t *json = json_object();
   json_object_set_new(json, "name", json_string_w(log->name));
   json_object_set_new(json, "description", json_string_a(log->aiDescription));
   json_object_set_new(json, "recordIdColumn", json_string_w(log->idColumn));
   json_object_set_new(json, "objectIdColumn", json_string_w(log->relatedObjectIdColumn));
   return json;
}

/**
 * Find log definition requested by client and check access rights. Sets appropriate
 * HTTP status and error response if log cannot be accessed.
 */
static const NXCORE_LOG *FindLogForRequest(Context *context, int *status)
{
   const wchar_t *name = context->getPlaceholderValue(L"log-name");
   const NXCORE_LOG *log = (name != nullptr) ? FindLogDefinition(name) : nullptr;
   if (log == nullptr)
   {
      context->setErrorResponse("Unknown log name");
      *status = 404;
      return nullptr;
   }

   if (!context->checkSystemAccessRights(log->requiredAccess))
   {
      context->setErrorResponse("Access denied");
      *status = 403;
      return nullptr;
   }

   return log;
}

/**
 * Handler for GET /v1/logs
 */
int H_Logs(Context *context)
{
   json_t *output = json_array();
   EnumerateLogDefinitions(
      [context, output] (const NXCORE_LOG *log) -> void
      {
         if (context->checkSystemAccessRights(log->requiredAccess))
            json_array_append_new(output, CreateLogSummary(log));
      });
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for GET /v1/logs/:log-name
 */
int H_LogDetails(Context *context)
{
   int status;
   const NXCORE_LOG *log = FindLogForRequest(context, &status);
   if (log == nullptr)
      return status;

   LogHandle handle(log);
   json_t *output = CreateLogSummary(log);
   json_object_set_new(output, "columns", handle.getColumnInfoAsJson());
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Read paging parameters from query request document
 */
static bool GetPagingParameters(json_t *request, int64_t *offset, int64_t *limit)
{
   *offset = json_object_get_int64(request, "offset", 0);
   *limit = json_object_get_int64(request, "limit", DEFAULT_LOG_QUERY_LIMIT);
   return (*offset >= 0) && (*limit >= 1) && (*limit <= MAX_LOG_QUERY_LIMIT) && (*offset + *limit <= MAX_LOG_QUERY_WINDOW);
}

/**
 * Handler for POST /v1/logs/:log-name/query
 */
int H_LogQuery(Context *context)
{
   int status;
   const NXCORE_LOG *log = FindLogForRequest(context, &status);
   if (log == nullptr)
      return status;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, L"H_LogQuery: empty request");
      return 400;
   }

   int64_t offset, limit;
   if (!GetPagingParameters(request, &offset, &limit))
   {
      context->setErrorResponse("Invalid offset or limit");
      return 400;
   }

   LogHandle handle(log);
   LogFilter filter(request, &handle);
   if (!filter.isValid())
   {
      context->setErrorResponse("Invalid filter definition");
      return 400;
   }

   json_t *records = handle.queryAsJson(&filter, offset, limit, context->getUserId());
   if (records == nullptr)
   {
      context->setErrorResponse("Database failure");
      return 500;
   }

   json_t *output = json_object();
   json_object_set_new(output, "columns", handle.getColumnInfoAsJson());
   json_object_set_new(output, "offset", json_integer(offset));
   json_object_set_new(output, "count", json_integer(json_array_size(records)));
   json_object_set_new(output, "records", records);
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for POST /v1/logs/:log-name/query-sql
 */
int H_LogQuerySql(Context *context)
{
   int status;
   const NXCORE_LOG *log = FindLogForRequest(context, &status);
   if (log == nullptr)
      return status;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, L"H_LogQuerySql: empty request");
      return 400;
   }

   int64_t offset, limit;
   if (!GetPagingParameters(request, &offset, &limit))
   {
      context->setErrorResponse("Invalid offset or limit");
      return 400;
   }

   LogHandle handle(log);
   LogFilter filter(request, &handle);
   if (!filter.isValid())
   {
      context->setErrorResponse("Invalid filter definition");
      return 400;
   }

   json_t *output = json_object();
   json_object_set_new(output, "query", json_string_t(handle.buildPagedQuerySql(&filter, offset, limit, context->getUserId())));
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for GET /v1/logs/:log-name/records/:record-id
 */
int H_LogRecord(Context *context)
{
   int status;
   const NXCORE_LOG *log = FindLogForRequest(context, &status);
   if (log == nullptr)
      return status;

   const wchar_t *id = context->getPlaceholderValue(L"record-id");
   wchar_t *eptr;
   int64_t recordId = (id != nullptr) ? wcstoll(id, &eptr, 10) : 0;
   if ((id == nullptr) || (*eptr != 0))
   {
      context->setErrorResponse("Invalid record ID");
      return 400;
   }

   LogHandle handle(log);
   json_t *record = nullptr;
   uint32_t rcc = handle.getRecordAsJson(recordId, &record);
   if (rcc == RCC_NO_SUCH_RECORD)
   {
      context->setErrorResponse("Record not found");
      return 404;
   }
   if (rcc != RCC_SUCCESS)
   {
      context->setErrorResponse("Database failure");
      return 500;
   }

   context->setResponseData(record);
   json_decref(record);
   return 200;
}
