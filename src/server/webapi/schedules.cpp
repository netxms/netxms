/*
** NetXMS - Network Management System
** Copyright (C) 2023-2026 Raden Solutions
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
** File: schedules.cpp
**
**/

#include "webapi.h"
#include <nxcore_schedule.h>

/**
 * Check if user has any scheduled task access rights
 */
static bool CheckScheduledTaskAccess(Context *context)
{
   return context->checkSystemAccessRights(SYSTEM_ACCESS_ALL_SCHEDULED_TASKS) ||
          context->checkSystemAccessRights(SYSTEM_ACCESS_USER_SCHEDULED_TASKS) ||
          context->checkSystemAccessRights(SYSTEM_ACCESS_OWN_SCHEDULED_TASKS);
}

/**
 * Parse task ID from URL placeholder
 */
static uint64_t GetTaskIdFromPlaceholder(Context *context)
{
   const TCHAR *v = context->getPlaceholderValue(L"task-id");
   return (v != nullptr) ? _tcstoull(v, nullptr, 0) : 0;
}

/**
 * Parse execution time from JSON value (accepts integer Unix timestamp or ISO 8601 string)
 */
static time_t ParseExecutionTime(json_t *value)
{
   if (json_is_integer(value))
      return static_cast<time_t>(json_integer_value(value));
   if (json_is_string(value))
      return ParseTimestamp(json_string_value(value));
   return 0;
}

/**
 * Filter callback for finding task by ID
 */
static bool FilterTaskById(const ScheduledTask *task, void *context)
{
   return task->getId() == *static_cast<uint64_t*>(context);
}

/**
 * Handler for GET /v1/scheduled-tasks
 */
int H_ScheduledTasks(Context *context)
{
   if (!CheckScheduledTaskAccess(context))
      return 403;

   json_t *output = GetScheduledTasks(context->getUserId(), context->getSystemAccessRights());
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for GET /v1/scheduled-tasks/:task-id
 */
int H_ScheduledTaskDetails(Context *context)
{
   if (!CheckScheduledTaskAccess(context))
      return 403;

   uint64_t taskId = GetTaskIdFromPlaceholder(context);
   if (taskId == 0)
      return 400;

   json_t *tasks = GetScheduledTasks(context->getUserId(), context->getSystemAccessRights(), FilterTaskById, &taskId);
   if (json_array_size(tasks) == 0)
   {
      json_decref(tasks);
      return 404;
   }

   json_t *task = json_array_get(tasks, 0);
   json_incref(task);
   json_decref(tasks);

   context->setResponseData(task);
   json_decref(task);
   return 200;
}

/**
 * Handler for POST /v1/scheduled-tasks
 */
int H_ScheduledTaskCreate(Context *context)
{
   if (!CheckScheduledTaskAccess(context))
      return 403;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   json_t *jsonHandlerId = json_object_get(request, "taskHandlerId");
   if (!json_is_string(jsonHandlerId))
   {
      context->setErrorResponse("Missing or invalid taskHandlerId field");
      return 400;
   }

   TCHAR taskHandlerId[256];
   utf8_to_wchar(json_string_value(jsonHandlerId), -1, taskHandlerId, 256);

   TCHAR parameters[4096] = L"";
   json_t *jsonParams = json_object_get(request, "parameters");
   if (json_is_string(jsonParams))
      utf8_to_wchar(json_string_value(jsonParams), -1, parameters, 4096);

   uint32_t objectId = json_integer_value(json_object_get(request, "objectId"));

   TCHAR comments[256] = L"";
   json_t *jsonComments = json_object_get(request, "comments");
   if (json_is_string(jsonComments))
      utf8_to_wchar(json_string_value(jsonComments), -1, comments, 256);

   TCHAR taskKey[256] = L"";
   json_t *jsonTaskKey = json_object_get(request, "taskKey");
   if (json_is_string(jsonTaskKey))
      utf8_to_wchar(json_string_value(jsonTaskKey), -1, taskKey, 256);

   uint32_t rcc;
   json_t *jsonSchedule = json_object_get(request, "schedule");
   json_t *jsonExecTime = json_object_get(request, "executionTime");

   if (json_is_string(jsonSchedule))
   {
      TCHAR schedule[256];
      utf8_to_wchar(json_string_value(jsonSchedule), -1, schedule, 256);
      rcc = AddRecurrentScheduledTask(taskHandlerId, schedule, parameters, nullptr,
               context->getUserId(), objectId, context->getSystemAccessRights(),
               comments, taskKey[0] != 0 ? taskKey : nullptr, false);
   }
   else if (jsonExecTime != nullptr)
   {
      time_t execTime = ParseExecutionTime(jsonExecTime);
      if (execTime == 0)
      {
         context->setErrorResponse("Invalid executionTime format");
         return 400;
      }
      rcc = AddOneTimeScheduledTask(taskHandlerId, execTime, parameters, nullptr,
               context->getUserId(), objectId, context->getSystemAccessRights(),
               comments, taskKey[0] != 0 ? taskKey : nullptr, false);
   }
   else
   {
      context->setErrorResponse("Either schedule or executionTime must be provided");
      return 400;
   }

   if (rcc == RCC_ACCESS_DENIED)
      return 403;
   if (rcc != RCC_SUCCESS)
   {
      context->setErrorResponse("Database failure");
      return 500;
   }

   return 201;
}

/**
 * Handler for PUT /v1/scheduled-tasks/:task-id
 */
int H_ScheduledTaskUpdate(Context *context)
{
   if (!CheckScheduledTaskAccess(context))
      return 403;

   uint64_t taskId = GetTaskIdFromPlaceholder(context);
   if (taskId == 0)
      return 400;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
      return 400;

   json_t *jsonHandlerId = json_object_get(request, "taskHandlerId");
   if (!json_is_string(jsonHandlerId))
   {
      context->setErrorResponse("Missing or invalid taskHandlerId field");
      return 400;
   }

   TCHAR taskHandlerId[256];
   utf8_to_wchar(json_string_value(jsonHandlerId), -1, taskHandlerId, 256);

   TCHAR parameters[4096] = L"";
   json_t *jsonParams = json_object_get(request, "parameters");
   if (json_is_string(jsonParams))
      utf8_to_wchar(json_string_value(jsonParams), -1, parameters, 4096);

   uint32_t objectId = json_integer_value(json_object_get(request, "objectId"));

   TCHAR comments[256] = L"";
   json_t *jsonComments = json_object_get(request, "comments");
   if (json_is_string(jsonComments))
      utf8_to_wchar(json_string_value(jsonComments), -1, comments, 256);

   bool disabled = json_is_true(json_object_get(request, "disabled"));

   uint32_t rcc;
   json_t *jsonSchedule = json_object_get(request, "schedule");
   json_t *jsonExecTime = json_object_get(request, "executionTime");

   if (json_is_string(jsonSchedule))
   {
      TCHAR schedule[256];
      utf8_to_wchar(json_string_value(jsonSchedule), -1, schedule, 256);
      rcc = UpdateRecurrentScheduledTask(taskId, taskHandlerId, schedule, parameters, nullptr,
               comments, context->getUserId(), objectId, context->getSystemAccessRights(), disabled);
   }
   else if (jsonExecTime != nullptr)
   {
      time_t execTime = ParseExecutionTime(jsonExecTime);
      if (execTime == 0)
      {
         context->setErrorResponse("Invalid executionTime format");
         return 400;
      }
      rcc = UpdateOneTimeScheduledTask(taskId, taskHandlerId, execTime, parameters, nullptr,
               comments, context->getUserId(), objectId, context->getSystemAccessRights(), disabled);
   }
   else
   {
      context->setErrorResponse("Either schedule or executionTime must be provided");
      return 400;
   }

   if (rcc == RCC_ACCESS_DENIED)
      return 403;
   if (rcc == RCC_INVALID_OBJECT_ID)
      return 404;
   if (rcc != RCC_SUCCESS)
   {
      context->setErrorResponse("Database failure");
      return 500;
   }

   // Return updated task
   json_t *tasks = GetScheduledTasks(context->getUserId(), context->getSystemAccessRights(), FilterTaskById, &taskId);
   if (json_array_size(tasks) > 0)
   {
      json_t *task = json_array_get(tasks, 0);
      json_incref(task);
      json_decref(tasks);
      context->setResponseData(task);
      json_decref(task);
   }
   else
   {
      json_decref(tasks);
   }
   return 200;
}

/**
 * Handler for DELETE /v1/scheduled-tasks/:task-id
 */
int H_ScheduledTaskDelete(Context *context)
{
   if (!CheckScheduledTaskAccess(context))
      return 403;

   uint64_t taskId = GetTaskIdFromPlaceholder(context);
   if (taskId == 0)
      return 400;

   uint32_t rcc = DeleteScheduledTask(taskId, context->getUserId(), context->getSystemAccessRights());
   if (rcc == RCC_ACCESS_DENIED)
      return 403;
   if (rcc == RCC_INVALID_OBJECT_ID)
      return 404;
   if (rcc != RCC_SUCCESS)
   {
      context->setErrorResponse("Database failure");
      return 500;
   }

   return 204;
}

/**
 * Handler for GET /v1/scheduled-task-handlers
 */
int H_ScheduledTaskHandlers(Context *context)
{
   if (!CheckScheduledTaskAccess(context))
      return 403;

   json_t *output = GetSchedulerTaskHandlersAsJson(context->getSystemAccessRights());
   context->setResponseData(output);
   json_decref(output);
   return 200;
}
