/*
** NetXMS - Network Management System
** Copyright (C) 2026 Raden Solutions
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
** File: inventory.cpp
**
**/

#include "object_helpers.h"

/**
 * Set response for inventory list (null entries when inventory is not available for the node)
 */
static int SetInventoryResponse(Context *context, json_t *entries)
{
   json_t *output = json_object();
   json_object_set_new(output, "entries", (entries != nullptr) ? entries : json_null());
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for GET /v1/objects/:object-id/software-inventory
 */
int H_SoftwareInventory(Context *context)
{
   int httpCode;
   shared_ptr<Node> node = LoadNodeForTableRequest(context, &httpCode);
   if (node == nullptr)
      return httpCode;

   const char *filter = context->getQueryParameter("filter");
   return SetInventoryResponse(context, node->getSoftwarePackagesAsJSON(((filter != nullptr) && (*filter != 0)) ? String(filter, "utf-8").cstr() : nullptr));
}

/**
 * Handler for GET /v1/objects/:object-id/hardware-inventory
 */
int H_HardwareInventory(Context *context)
{
   int httpCode;
   shared_ptr<Node> node = LoadNodeForTableRequest(context, &httpCode);
   if (node == nullptr)
      return httpCode;

   return SetInventoryResponse(context, node->getHardwareComponentsAsJSON());
}

/**
 * Serialize user session to JSON
 */
static json_t *UserSessionToJson(const UserSession& s)
{
   json_t *json = json_object();
   json_object_set_new(json, "id", json_integer(s.id));
   json_object_set_new(json, "loginName", json_string_t(s.loginName));
   json_object_set_new(json, "sessionName", !s.sessionName.isEmpty() ? json_string_t(s.sessionName) : json_null());
   json_object_set_new(json, "terminal", json_string_t(s.terminal));
   json_object_set_new(json, "state", !s.state.isEmpty() ? json_string_t(s.state) : json_null());
   json_object_set_new(json, "connected", json_boolean(s.connected));
   json_object_set_new(json, "clientName", json_string_t(s.clientName));
   json_object_set_new(json, "clientAddress", s.clientAddress.isValid() ? s.clientAddress.toJson() : json_null());

   if ((s.displayWidth > 0) && (s.displayHeight > 0))
   {
      json_t *display = json_object();
      json_object_set_new(display, "width", json_integer(s.displayWidth));
      json_object_set_new(display, "height", json_integer(s.displayHeight));
      json_object_set_new(display, "colorDepth", json_integer(s.displayColorDepth));
      json_object_set_new(json, "display", display);
   }
   else
   {
      json_object_set_new(json, "display", json_null());
   }

   json_object_set_new(json, "connectTime", json_time_string(s.connectTime));
   json_object_set_new(json, "loginTime", json_time_string(s.loginTime));
   json_object_set_new(json, "idleTime", json_integer(s.idleTime));

   if (s.agentType >= 0)
   {
      json_t *agent = json_object();
      json_object_set_new(agent, "type", json_string((s.agentType == 1) ? "user" : "session"));
      json_object_set_new(agent, "pid", json_integer(s.agentPID));
      json_object_set_new(json, "sessionAgent", agent);
   }
   else
   {
      json_object_set_new(json, "sessionAgent", json_null());
   }
   return json;
}

/**
 * Handler for GET /v1/objects/:object-id/user-sessions
 */
int H_UserSessions(Context *context)
{
   int httpCode;
   shared_ptr<Node> node = LoadNodeForTableRequest(context, &httpCode);
   if (node == nullptr)
      return httpCode;

   if (!node->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ_AGENT))
      return 403;

   shared_ptr<AgentConnectionEx> conn = node->getAgentConnection();
   if (conn == nullptr)
   {
      context->setErrorResponse("No connection to agent");
      return 503;
   }

   ObjectArray<UserSession> *sessions;
   uint32_t rcc = conn->getUserSessions(&sessions);
   switch(rcc)
   {
      case ERR_SUCCESS:
         break;
      case ERR_UNKNOWN_METRIC:
      case ERR_UNSUPPORTED_METRIC:
         context->setErrorResponse("User session information is not supported by agent");
         return 404;
      case ERR_NOT_CONNECTED:
      case ERR_CONNECTION_BROKEN:
      case ERR_REQUEST_TIMEOUT:
         context->setErrorResponse("Cannot communicate with agent");
         return 503;
      default:
         context->setErrorResponse(AgentErrorCodeToText(rcc));
         return 502;
   }

   json_t *entries = json_array();
   for(int i = 0; i < sessions->size(); i++)
      json_array_append_new(entries, UserSessionToJson(*sessions->get(i)));
   delete sessions;

   json_t *output = json_object();
   json_object_set_new(output, "entries", entries);
   context->setResponseData(output);
   json_decref(output);
   return 200;
}
