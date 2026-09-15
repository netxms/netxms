/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
** File: background_tasks.cpp
**
**/

#include "webapi.h"
#include <nxtask.h>

/**
 * Background task state names (indexed by BackgroundTaskState value)
 */
static const char *s_taskStateNames[] = { "pending", "running", "completed", "failed" };

/**
 * Handler for GET /v1/background-tasks/:task-id
 */
int H_BackgroundTaskDetails(Context *context)
{
   uint64_t taskId = wcstoull(context->getPlaceholderValue(_T("task-id")), nullptr, 10);
   shared_ptr<BackgroundTask> task = GetBackgroundTask(taskId);
   if (task == nullptr)
      return 404;

   json_t *output = json_object();
   json_object_set_new(output, "id", json_integer(static_cast<json_int_t>(task->getId())));
   json_object_set_new(output, "description", json_string_w(task->getDescription()));
   json_object_set_new(output, "state", json_string(s_taskStateNames[static_cast<int>(task->getState())]));
   json_object_set_new(output, "progress", json_integer(task->getProgress()));
   json_object_set_new(output, "completionTime", json_time_string(task->getCompletionTime()));
   json_object_set_new(output, "failureReason", task->isFailed() ? json_string_w(task->getFailureReson()) : json_null());
   context->setResponseData(output);
   json_decref(output);
   return 200;
}
