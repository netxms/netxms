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
** File: data_export.cpp
**
** DCI history export: start export task, query its state, download produced files.
**/

#include "webapi.h"
#include <nxtask.h>

/**
 * Background task state names (indexed by BackgroundTaskState value)
 */
static const char *s_taskStateNames[] = { "pending", "running", "completed", "failed" };

/**
 * Parse tier name
 */
static bool ParseTierName(const char *name, DciTier *tier)
{
   if ((name == nullptr) || !stricmp(name, "raw"))
      *tier = DCI_TIER_RAW;
   else if (!stricmp(name, "hourly"))
      *tier = DCI_TIER_HOURLY;
   else if (!stricmp(name, "daily"))
      *tier = DCI_TIER_DAILY;
   else
      return false;
   return true;
}

/**
 * Parse export request from JSON document. Returns HTTP status (0 on success).
 */
static int ParseExportRequest(Context *context, DataExportRequest *exportRequest)
{
   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      context->setErrorResponse("Invalid request document");
      return 400;
   }

   exportRequest->rootObjectId = json_object_get_uint32(request, "rootObjectId");

   json_t *patterns = json_object_get(request, "tagPatterns");
   if (json_is_array(patterns))
   {
      size_t index;
      json_t *pattern;
      json_array_foreach(patterns, index, pattern)
      {
         if (json_is_string(pattern))
            exportRequest->tagPatterns.addUTF8String(json_string_value(pattern));
      }
   }

   json_t *dciList = json_object_get(request, "dciList");
   if (json_is_array(dciList))
   {
      size_t index;
      json_t *ref;
      json_array_foreach(dciList, index, ref)
      {
         if (json_is_object(ref))
            exportRequest->dciList.push_back(std::make_pair(json_object_get_uint32(ref, "ownerId"), json_object_get_uint32(ref, "dciId")));
      }
   }

   json_t *timeFrom = json_object_get(request, "timeFrom");
   json_t *timeTo = json_object_get(request, "timeTo");
   exportRequest->timeFrom = json_is_integer(timeFrom) ? json_integer_value(timeFrom) * 1000 : static_cast<int64_t>(ParseTimestamp(json_object_get_string_utf8(request, "timeFrom", ""), 0)) * 1000;
   exportRequest->timeTo = json_is_integer(timeTo) ? json_integer_value(timeTo) * 1000 : static_cast<int64_t>(ParseTimestamp(json_object_get_string_utf8(request, "timeTo", ""), 0)) * 1000;
   if ((exportRequest->timeFrom <= 0) || (exportRequest->timeTo <= exportRequest->timeFrom))
   {
      context->setErrorResponse("Invalid time period");
      return 400;
   }
   exportRequest->timeTo += 999;  // Period end is inclusive at millisecond resolution

   if (!ParseTierName(json_object_get_string_utf8(request, "tier", nullptr), &exportRequest->tier))
   {
      context->setErrorResponse("Invalid tier");
      return 400;
   }

   const char *format = json_object_get_string_utf8(request, "format", "csv");
   if (stricmp(format, "csv"))
   {
      context->setErrorResponse("Unsupported format");
      return 400;
   }
   exportRequest->format = DATA_EXPORT_FORMAT_CSV;
   exportRequest->anonymize = json_object_get_boolean(request, "anonymize", false);
   exportRequest->keepMapping = exportRequest->anonymize && json_object_get_boolean(request, "keepMapping", false);

   shared_ptr<NetObj> rootObject = FindObjectById(exportRequest->rootObjectId);
   if (exportRequest->dciList.empty())
   {
      if (rootObject == nullptr)
      {
         context->setErrorResponse("Invalid root object ID");
         return 404;
      }
      if (exportRequest->tagPatterns.isEmpty())
      {
         context->setErrorResponse("Tag patterns or DCI list required");
         return 400;
      }
   }
   if ((rootObject != nullptr) && !rootObject->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ))
   {
      context->writeAuditLog(AUDIT_OBJECTS, false, rootObject->getId(), L"Access denied on DCI data export");
      return 403;
   }
   return 0;
}

/**
 * Handler for POST /v1/data-export/metrics
 *
 * Resolves metrics that the export request in the body would select, without running the export. Request body is the
 * same as for POST /v1/data-export (format, anonymize and keepMapping are ignored).
 */
int H_DataExportMetrics(Context *context)
{
   DataExportRequest exportRequest;
   int rc = ParseExportRequest(context, &exportRequest);
   if (rc != 0)
      return rc;

   std::vector<DataExportMetricInfo> metrics;
   uint32_t skipped;
   ResolveDataExportSelection(exportRequest, context->getUserId(), metrics, &skipped);

   json_t *list = json_array();
   for(const DataExportMetricInfo& m : metrics)
      json_array_append_new(list, m.toJson());
   json_t *output = json_object();
   json_object_set_new(output, "metrics", list);
   json_object_set_new(output, "skippedCount", json_integer(skipped));
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for POST /v1/data-export
 *
 * Request body:
 * {
 *    "rootObjectId": 123,                      // required unless "dciList" is given
 *    "tagPatterns": ["eed.*", "power.in"],     // glob patterns matched against DCI system tag
 *    "dciList": [{"ownerId": 1, "dciId": 2}],  // explicit selection, takes precedence over patterns
 *    "timeFrom": "2026-01-01T00:00:00Z",       // ISO 8601 or UNIX time
 *    "timeTo": "2026-02-01T00:00:00Z",
 *    "tier": "raw" | "hourly" | "daily",
 *    "format": "csv",
 *    "anonymize": false,
 *    "keepMapping": false
 * }
 */
int H_DataExportStart(Context *context)
{
   DataExportRequest exportRequest;
   int rc = ParseExportRequest(context, &exportRequest);
   if (rc != 0)
      return rc;

   shared_ptr<BackgroundTask> task = StartDataExport(exportRequest, context->getUserId());
   if (task == nullptr)
   {
      context->setErrorResponse("Cannot start export task");
      return 500;
   }

   shared_ptr<NetObj> rootObject = FindObjectById(exportRequest.rootObjectId);
   wchar_t timeFromText[64], timeToText[64];
   wchar_t *patternText = exportRequest.tagPatterns.join(L" ");
   context->writeAuditLog(AUDIT_OBJECTS, true, (rootObject != nullptr) ? rootObject->getId() : 0,
      L"DCI data export started (task " UINT64_FMT L"): root object \"%s\", %d explicit DCIs, tag patterns \"%s\", period %s - %s, tier %d%s",
      task->getId(), (rootObject != nullptr) ? rootObject->getName() : L"", static_cast<int>(exportRequest.dciList.size()), patternText,
      FormatTimestampMs(exportRequest.timeFrom, timeFromText), FormatTimestampMs(exportRequest.timeTo, timeToText),
      static_cast<int>(exportRequest.tier), exportRequest.anonymize ? L", anonymized" : L"");
   MemFree(patternText);

   json_t *output = json_object();
   json_object_set_new(output, "taskId", json_integer(static_cast<json_int_t>(task->getId())));
   context->setResponseData(output);
   json_decref(output);
   return 201;
}

/**
 * Find export task by placeholder and check ownership. Returns HTTP status (0 on success).
 */
static int FindExportTask(Context *context, shared_ptr<BackgroundTask> *task, DataExportInfo *info)
{
   uint64_t taskId = wcstoull(context->getPlaceholderValue(L"task-id"), nullptr, 10);
   *task = GetBackgroundTask(taskId);
   if ((*task == nullptr) || !GetDataExportInfo(taskId, info))
      return 404;
   if (info->userId != context->getUserId())
   {
      context->writeAuditLog(AUDIT_OBJECTS, false, 0, L"Access denied on DCI data export produced by another user (task " UINT64_FMT L")", taskId);
      return 403;
   }
   return 0;
}

/**
 * Handler for GET /v1/data-export/:task-id
 */
int H_DataExportState(Context *context)
{
   shared_ptr<BackgroundTask> task;
   DataExportInfo info;
   int rc = FindExportTask(context, &task, &info);
   if (rc != 0)
      return rc;

   json_t *output = json_object();
   json_object_set_new(output, "taskId", json_integer(static_cast<json_int_t>(task->getId())));
   json_object_set_new(output, "state", json_string(s_taskStateNames[static_cast<int>(task->getState())]));
   json_object_set_new(output, "progress", json_integer(task->getProgress()));
   json_object_set_new(output, "failureReason", task->isFailed() ? json_string_w(task->getFailureReson()) : json_null());
   if (task->getState() == BackgroundTaskState::COMPLETED)
   {
      json_object_set_new(output, "metricCount", json_integer(info.metricCount));
      json_object_set_new(output, "skippedCount", json_integer(info.skippedCount));
      json_object_set_new(output, "sampleCount", json_integer(static_cast<json_int_t>(info.sampleCount)));
      json_object_set_new(output, "archiveSize", json_integer(static_cast<json_int_t>(FileSize(info.archiveFile))));
      json_object_set_new(output, "mappingAvailable", json_boolean(!info.mappingFile.isEmpty()));
   }
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for GET /v1/data-export/:task-id/file?type=archive|mapping
 */
int H_DataExportFile(Context *context)
{
   shared_ptr<BackgroundTask> task;
   DataExportInfo info;
   int rc = FindExportTask(context, &task, &info);
   if (rc != 0)
      return rc;

   if (task->getState() != BackgroundTaskState::COMPLETED)
   {
      context->setErrorResponse("Export is not completed");
      return 409;
   }

   const char *type = context->getQueryParameter("type");
   bool mapping = (type != nullptr) && !stricmp(type, "mapping");
   const MutableString& fileName = mapping ? info.mappingFile : info.archiveFile;
   if (fileName.isEmpty())
      return 404;

   size_t size;
   BYTE *data = LoadFile(fileName, &size);
   if (data == nullptr)
      return 404;

   wchar_t disposition[128];
   nx_swprintf(disposition, 128, L"attachment; filename=\"netxms-export-" UINT64_FMT L"%s\"", task->getId(), mapping ? L"-mapping.csv" : L".zip");
   context->setResponseHeader(L"Content-Disposition", disposition);
   context->setResponseData(data, size, mapping ? "text/csv; charset=utf-8" : "application/zip");
   MemFree(data);

   MarkDataExportFileDownloaded(task->getId(), mapping ? DATA_EXPORT_FILE_MAPPING : DATA_EXPORT_FILE_ARCHIVE);
   return 200;
}
