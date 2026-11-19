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
** File: polling.cpp
**
**/

#include "aitools.h"

/**
 * Shared state between AI handler thread and worker (sync wait path).
 * Held by shared_ptr in both, so it outlives whichever finishes first.
 */
struct PollWaitState
{
   PollOutputBuffer output;
   Condition done;
   int64_t startTimeMs;

   PollWaitState() : done(true) { startTimeMs = GetCurrentTimeMs(); }
};

/**
 * Worker context for both async and sync paths
 */
struct ForcePollContext
{
   shared_ptr<NetObj> object;
   int pollType;
   shared_ptr<PollWaitState> waitState; // nullptr in async mode
};

/**
 * Thread pool worker entry point
 */
static void ForcePollWorker(const shared_ptr<ForcePollContext>& ctx)
{
   ExecuteForcedPoll(ctx->object, ctx->pollType, (ctx->waitState != nullptr) ? &ctx->waitState->output : nullptr);
   if (ctx->waitState != nullptr)
      ctx->waitState->done.set();
}

/**
 * Build object summary JSON for response
 */
static json_t *BuildObjectSummary(const NetObj& object)
{
   json_t *json = json_object();
   json_object_set_new(json, "id", json_integer(object.getId()));
   json_object_set_new(json, "name", json_string_t(object.getName()));
   json_object_set_new(json, "class", json_string(object.getObjectClassNameA()));
   return json;
}

/**
 * Force immediate poll on a monitored object.
 */
std::string F_ForcePoll(json_t *arguments, uint32_t userId)
{
   const char *pollTypeStr = json_object_get_string_utf8(arguments, "pollType", nullptr);
   if ((pollTypeStr == nullptr) || (pollTypeStr[0] == 0))
   {
      json_t *response = json_object();
      json_object_set_new(response, "error", json_string("pollType is required"));
      json_object_set_new(response, "validTypes", GetPollTypeNames());
      return JsonToString(response);
   }

   int pollType = PollTypeFromName(pollTypeStr);
   if (pollType == -1)
   {
      json_t *response = json_object();
      char errorMsg[256];
      snprintf(errorMsg, sizeof(errorMsg), "Unknown poll type '%s'", pollTypeStr);
      json_object_set_new(response, "error", json_string(errorMsg));
      json_object_set_new(response, "validTypes", GetPollTypeNames());
      return JsonToString(response);
   }

   shared_ptr<NetObj> object = FindObjectByNameOrId(arguments, "object");
   if (object == nullptr)
      return std::string("Object not found");

   uint32_t rcc = ValidateForcedPoll(*object, pollType, userId);
   if (rcc == RCC_ACCESS_DENIED)
      return std::string("Access denied");
   if (rcc != RCC_SUCCESS)
   {
      char nameUtf8[256];
      wchar_to_utf8(object->getName(), -1, nameUtf8, sizeof(nameUtf8) - 1);
      nameUtf8[sizeof(nameUtf8) - 1] = 0;
      char errorMsg[512];
      snprintf(errorMsg, sizeof(errorMsg), "Poll type '%s' is not applicable to %s '%s'",
               pollTypeStr, object->getObjectClassNameA(), nameUtf8);
      return std::string(errorMsg);
   }

   bool wait = json_object_get_boolean(arguments, "wait", false);
   int timeoutSeconds = json_object_get_int32(arguments, "timeoutSeconds", 30);
   if (timeoutSeconds < 1)
      timeoutSeconds = 1;
   else if (timeoutSeconds > 300)
      timeoutSeconds = 300;

   nxlog_debug_tag(DEBUG_TAG, 4, L"Force '%hs' poll for object [%u] %s by AI assistant (user [%u], wait=%s)",
            pollTypeStr, object->getId(), object->getName(), userId, wait ? L"true" : L"false");

   auto ctx = make_shared<ForcePollContext>();
   ctx->object = object;
   ctx->pollType = pollType;

   if (!wait)
   {
      ThreadPoolExecute(g_pollerThreadPool, ForcePollWorker, ctx);

      json_t *response = json_object();
      json_object_set_new(response, "accepted", json_true());
      json_object_set_new(response, "waited", json_false());
      json_object_set_new(response, "object", BuildObjectSummary(*object));
      json_object_set_new(response, "pollType", json_string(pollTypeStr));
      json_object_set_new(response, "message", json_string("Poll scheduled. Inspect object state after a short delay."));
      return JsonToString(response);
   }

   // Sync wait: run poll on pool with capture buffer as output writer, wait on condition.
   // Worker holds its own reference to wait state, so buffer stays valid until poll completes even if wait times out.
   auto waitState = make_shared<PollWaitState>();
   ctx->waitState = waitState;
   ThreadPoolExecute(g_pollerThreadPool, ForcePollWorker, ctx);

   bool completed = waitState->done.wait(static_cast<uint32_t>(timeoutSeconds) * 1000);
   int64_t durationMs = GetCurrentTimeMs() - waitState->startTimeMs;

   json_t *response = json_object();
   json_object_set_new(response, "accepted", json_true());
   json_object_set_new(response, "waited", json_true());
   json_object_set_new(response, "object", BuildObjectSummary(*object));
   json_object_set_new(response, "pollType", json_string(pollTypeStr));

   StringBuffer output = waitState->output.snapshot();
   if (completed)
   {
      json_object_set_new(response, "completed", json_true());
      json_object_set_new(response, "durationMs", json_integer(durationMs));
      json_object_set_new(response, "pollOutput", json_string_w(output));
   }
   else
   {
      json_object_set_new(response, "completed", json_false());
      json_object_set_new(response, "timedOut", json_true());
      json_object_set_new(response, "timeoutSeconds", json_integer(timeoutSeconds));
      json_object_set_new(response, "partialPollOutput", json_string_w(output));
      json_object_set_new(response, "message", json_string("Poll is still running; partial poll output captured."));
   }

   return JsonToString(response);
}
