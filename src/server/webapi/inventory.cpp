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
#include <entity_mib.h>

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
 * ENTITY-MIB physical component class names (entPhysicalClass)
 */
static CodeLookupElement s_componentClassNames[] =
{
   { COMPONENT_CLASS_OTHER, L"Other" },
   { COMPONENT_CLASS_UNKNOWN, L"Unknown" },
   { COMPONENT_CLASS_CHASSIS, L"Chassis" },
   { COMPONENT_CLASS_BACKPLANE, L"Backplane" },
   { COMPONENT_CLASS_CONTAINER, L"Container" },
   { COMPONENT_CLASS_POWER_SUPPLY, L"PowerSupply" },
   { COMPONENT_CLASS_FAN, L"Fan" },
   { COMPONENT_CLASS_SENSOR, L"Sensor" },
   { COMPONENT_CLASS_MODULE, L"Module" },
   { COMPONENT_CLASS_PORT, L"Port" },
   { COMPONENT_CLASS_STACK, L"Stack" },
   { COMPONENT_CLASS_CPU, L"CPU" },
   { 0, nullptr }
};

/**
 * Serialize physical component and its children to JSON
 */
static json_t *PhysicalComponentToJson(const Component& c, const Node& node)
{
   json_t *json = json_object();
   json_object_set_new(json, "index", json_integer(c.getIndex()));
   json_object_set_new(json, "class", json_string_t(CodeToText(c.getClass(), s_componentClassNames, L"Unknown")));
   json_object_set_new(json, "position", json_integer(c.getPosition()));
   json_object_set_new(json, "name", json_string_t(c.getName()));
   json_object_set_new(json, "description", json_string_t(c.getDescription()));
   json_object_set_new(json, "model", json_string_t(c.getModel()));
   json_object_set_new(json, "serialNumber", json_string_t(c.getSerial()));
   json_object_set_new(json, "vendor", json_string_t(c.getVendor()));
   json_object_set_new(json, "firmware", json_string_t(c.getFirmware()));
   json_object_set_new(json, "interfaceIndex", json_integer(c.getIfIndex()));
   json_object_set_new(json, "interfaceName", (c.getIfIndex() != 0) ? InterfaceNameToJson(node, c.getIfIndex(), false) : json_null());

   json_t *children = json_array();
   const ObjectArray<Component>& childList = c.getChildren();
   for(int i = 0; i < childList.size(); i++)
      json_array_append_new(children, PhysicalComponentToJson(*childList.get(i), node));
   json_object_set_new(json, "children", children);
   return json;
}

/**
 * Handler for GET /v1/objects/:object-id/physical-components
 */
int H_PhysicalComponents(Context *context)
{
   int httpCode;
   shared_ptr<Node> node = LoadNodeForTableRequest(context, &httpCode);
   if (node == nullptr)
      return httpCode;

   shared_ptr<ComponentTree> components = node->getComponents();
   const Component *root = (components != nullptr) ? components->getRoot() : nullptr;

   json_t *output = json_object();
   json_object_set_new(output, "timestamp", (root != nullptr) ? json_time_string(components->getTimestamp()) : json_null());
   json_object_set_new(output, "root", (root != nullptr) ? PhysicalComponentToJson(*root, *node) : json_null());
   context->setResponseData(output);
   json_decref(output);
   return 200;
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
