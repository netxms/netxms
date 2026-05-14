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
#include <unordered_map>

/**
 * Description of one supported poll kind
 */
struct PollKind
{
   PollerType type;
   bool (Pollable::*isAvailable)() const;
   void (Pollable::*doForcedPoll)(PollerInfo*);
};

/**
 * Map of poll type name (as accepted from AI) to its kind descriptor
 */
static const std::unordered_map<std::string, PollKind>& GetPollKinds()
{
   static const std::unordered_map<std::string, PollKind> kinds = {
      { "status",             { PollerType::STATUS,             &Pollable::isStatusPollAvailable,            &Pollable::doForcedStatusPoll } },
      { "configuration",      { PollerType::CONFIGURATION,      &Pollable::isConfigurationPollAvailable,     &Pollable::doForcedConfigurationPoll } },
      { "instance-discovery", { PollerType::INSTANCE_DISCOVERY, &Pollable::isInstanceDiscoveryPollAvailable, &Pollable::doForcedInstanceDiscoveryPoll } },
      { "topology",           { PollerType::TOPOLOGY,           &Pollable::isTopologyPollAvailable,          &Pollable::doForcedTopologyPoll } },
      { "routing-table",      { PollerType::ROUTING_TABLE,      &Pollable::isRoutingTablePollAvailable,      &Pollable::doForcedRoutingTablePoll } },
      { "discovery",          { PollerType::DISCOVERY,          &Pollable::isDiscoveryPollAvailable,         &Pollable::doForcedDiscoveryPoll } },
      { "autobind",           { PollerType::AUTOBIND,           &Pollable::isAutobindPollAvailable,          &Pollable::doForcedAutobindPoll } },
      { "map-update",         { PollerType::MAP_UPDATE,         &Pollable::isMapUpdatePollAvailable,         &Pollable::doForcedMapUpdatePoll } }
   };
   return kinds;
}

/**
 * Shared state between AI handler thread and worker (sync wait path).
 * Held by shared_ptr in both, so it outlives whichever finishes first.
 */
struct PollWaitState
{
   PollerOutputBuffer buffer;
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
   PollerInfo *poller;
   void (Pollable::*pollMethod)(PollerInfo*);
   shared_ptr<PollWaitState> waitState; // nullptr in async mode
};

/**
 * Thread pool worker entry point
 */
static void ForcePollWorker(const shared_ptr<ForcePollContext>& ctx)
{
   Pollable *pollable = ctx->object->getAsPollable();
   if (pollable != nullptr)
      (pollable->*ctx->pollMethod)(ctx->poller);

   if (ctx->waitState != nullptr)
   {
      // Detach capture buffer from object before signaling completion
      ctx->object->setPollerOutputBuffer(nullptr);
      ctx->waitState->done.set();
   }
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
 * Build JSON array of valid poll type names for error responses
 */
static json_t *BuildValidPollTypesArray()
{
   json_t *array = json_array();
   for (const auto& it : GetPollKinds())
      json_array_append_new(array, json_string(it.first.c_str()));
   return array;
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
      json_object_set_new(response, "validTypes", BuildValidPollTypesArray());
      return JsonToString(response);
   }

   const auto& kinds = GetPollKinds();
   auto kindIt = kinds.find(pollTypeStr);
   if (kindIt == kinds.end())
   {
      json_t *response = json_object();
      char errorMsg[256];
      snprintf(errorMsg, sizeof(errorMsg), "Unknown poll type '%s'", pollTypeStr);
      json_object_set_new(response, "error", json_string(errorMsg));
      json_object_set_new(response, "validTypes", BuildValidPollTypesArray());
      return JsonToString(response);
   }
   const PollKind& kind = kindIt->second;

   shared_ptr<NetObj> object = FindObjectByNameOrId(arguments, "object");
   if (object == nullptr)
      return std::string("Object not found");
   if (!object->checkAccessRights(userId, OBJECT_ACCESS_READ))
      return std::string("Access denied");

   Pollable *pollable = object->getAsPollable();
   if ((pollable == nullptr) || !(pollable->*kind.isAvailable)())
   {
      char nameUtf8[256];
      wchar_to_utf8(object->getName(), -1, nameUtf8, sizeof(nameUtf8));
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
   ctx->poller = RegisterPoller(kind.type, object);
   ctx->pollMethod = kind.doForcedPoll;

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

   // Sync wait: attach capture buffer to the object, run poll on pool, wait on condition.
   auto waitState = make_shared<PollWaitState>();
   ctx->waitState = waitState;
   object->setPollerOutputBuffer(&waitState->buffer);

   ThreadPoolExecute(g_pollerThreadPool, ForcePollWorker, ctx);

   bool completed = waitState->done.wait(static_cast<uint32_t>(timeoutSeconds) * 1000);
   int64_t durationMs = GetCurrentTimeMs() - waitState->startTimeMs;

   json_t *response = json_object();
   json_object_set_new(response, "accepted", json_true());
   json_object_set_new(response, "waited", json_true());
   json_object_set_new(response, "object", BuildObjectSummary(*object));
   json_object_set_new(response, "pollType", json_string(pollTypeStr));

   if (completed)
   {
      json_object_set_new(response, "completed", json_true());
      json_object_set_new(response, "durationMs", json_integer(durationMs));
      StringBuffer output = waitState->buffer.snapshot();
      json_object_set_new(response, "pollerOutput", json_string_t(output.cstr()));
   }
   else
   {
      // Worker is still running. Worker holds its own shared_ptr to waitState, so the
      // buffer (which the object still references via m_pollerOutputBuffer) stays alive
      // until the worker finishes and clears the pointer.
      json_object_set_new(response, "completed", json_false());
      json_object_set_new(response, "timedOut", json_true());
      json_object_set_new(response, "timeoutSeconds", json_integer(timeoutSeconds));
      StringBuffer output = waitState->buffer.snapshot();
      json_object_set_new(response, "partialPollerOutput", json_string_t(output.cstr()));
      json_object_set_new(response, "message", json_string("Poll is still running; partial diagnostic output captured."));
   }

   return JsonToString(response);
}
