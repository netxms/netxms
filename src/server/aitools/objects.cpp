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
** File: objects.cpp
**
**/

#include "aitools.h"
#include <unordered_set>

/**
 * Convert interface admin state to text
 */
static const char *InterfaceAdminStateToText(uint16_t state)
{
   switch(state)
   {
      case IF_ADMIN_STATE_UP:
         return "UP";
      case IF_ADMIN_STATE_DOWN:
         return "DOWN";
      case IF_ADMIN_STATE_TESTING:
         return "TESTING";
      default:
         return "UNKNOWN";
   }
}

/**
 * Convert interface operational state to text
 */
static const char *InterfaceOperStateToText(uint16_t state)
{
   switch(state)
   {
      case IF_OPER_STATE_UP:
         return "UP";
      case IF_OPER_STATE_DOWN:
         return "DOWN";
      case IF_OPER_STATE_TESTING:
         return "TESTING";
      case IF_OPER_STATE_DORMANT:
         return "DORMANT";
      case IF_OPER_STATE_NOT_PRESENT:
         return "NOT_PRESENT";
      default:
         return "UNKNOWN";
   }
}

/**
 * Get object details
 */
std::string F_GetObject(json_t *arguments, uint32_t userId)
{
   const char *objectName = json_object_get_string_utf8(arguments, "object", nullptr);
   if ((objectName == nullptr) || (objectName[0] == 0))
      return std::string("Object name or ID must be provided");

   shared_ptr<NetObj> object = FindObjectByNameOrId(objectName);
   if ((object == nullptr) || !object->checkAccessRights(userId, OBJECT_ACCESS_READ))
   {
      char buffer[256];
      snprintf(buffer, 256, "Object with name or ID \"%s\" is not known or not accessible", objectName);
      return std::string(buffer);
   }

   return JsonToString(object->toJson());
}

/**
 * Create object summary JSON document
 */
static inline json_t *CreateObjectSummary(const NetObj& object)
{
   json_t *jsonObject = json_object();
   json_object_set_new(jsonObject, "id", json_integer(object.getId()));
   json_object_set_new(jsonObject, "guid", object.getGuid().toJson());
   json_object_set_new(jsonObject, "class", json_string(object.getObjectClassNameA()));
   json_object_set_new(jsonObject, "name", json_string_t(object.getName()));
   json_object_set_new(jsonObject, "alias", json_string_t(object.getAlias()));
   json_object_set_new(jsonObject, "category", json_integer(object.getCategoryId()));
   json_object_set_new(jsonObject, "timestamp", json_time_string(object.getTimeStamp()));
   json_object_set_new(jsonObject, "status", json_string_t(GetStatusAsText(object.getStatus(), false)));
   json_object_set_new(jsonObject, "customAttributes", object.getCustomAttributesAsJson());
   json_object_set_new(jsonObject, "comments", json_string_t(object.getComments()));
   json_object_set_new(jsonObject, "aiHint", json_string_t(object.getAIHint()));

   if (object.getObjectClass() == OBJECT_NODE)
   {
      const Node& node = static_cast<const Node&>(object);
      InetAddress ipAddr = node.getPrimaryIpAddress();
      if (ipAddr.isValid())
      {
         char addrBuffer[64];
         json_object_set_new(jsonObject, "primaryIpAddress", json_string(ipAddr.toStringA(addrBuffer)));
      }

      json_t *state = json_object();
      uint32_t stateFlags = node.getState();
      json_object_set_new(state, "agentUnreachable", json_boolean(stateFlags & NSF_AGENT_UNREACHABLE));
      json_object_set_new(state, "snmpUnreachable", json_boolean(stateFlags & NSF_SNMP_UNREACHABLE));
      json_object_set_new(state, "ethernetIpUnreachable", json_boolean(stateFlags & NSF_ETHERNET_IP_UNREACHABLE));
      json_object_set_new(state, "cacheModeNotSupported", json_boolean(stateFlags & NSF_CACHE_MODE_NOT_SUPPORTED));
      json_object_set_new(state, "snmpTrapFlood", json_boolean(stateFlags & NSF_SNMP_TRAP_FLOOD));
      json_object_set_new(state, "icmpUnreachable", json_boolean(stateFlags & NSF_ICMP_UNREACHABLE));
      json_object_set_new(state, "sshUnreachable", json_boolean(stateFlags & NSF_SSH_UNREACHABLE));
      json_object_set_new(state, "modbusUnreachable", json_boolean(stateFlags & NSF_MODBUS_UNREACHABLE));
      json_object_set_new(state, "unreachable", json_boolean(stateFlags & DCSF_UNREACHABLE));
      json_object_set_new(state, "networkPathProblem", json_boolean(stateFlags & DCSF_NETWORK_PATH_PROBLEM));
      json_object_set_new(jsonObject, "state", state);
   }
   else if (object.getObjectClass() == OBJECT_INTERFACE)
   {
      const Interface& iface = static_cast<const Interface&>(object);
      json_object_set_new(jsonObject, "ianaIfType", json_integer(iface.getIfType()));
      json_object_set_new(jsonObject, "macAddress", iface.getMacAddress().toJson());
      json_object_set_new(jsonObject, "mtu", json_integer(iface.getMTU()));
      json_object_set_new(jsonObject, "speed", json_integer(iface.getSpeed()));
      json_object_set_new(jsonObject, "adminState", json_string(InterfaceAdminStateToText(iface.getAdminState())));
      json_object_set_new(jsonObject, "operState", json_string(InterfaceOperStateToText(iface.getOperState())));
   }

   return jsonObject;
}

/**
 * Find objects using various criteria
 */
std::string F_FindObjects(json_t *arguments, uint32_t userId)
{
   uint32_t parentId = json_object_get_uint32(arguments, "parentId");
   int32_t zoneUIN = json_object_get_int32(arguments, "zoneUIN");

   wchar_t name[256];
   utf8_to_wchar(json_object_get_string_utf8(arguments, "name", ""), -1, name, 256);

   InetAddress ipAddressFilter;
   const char *ipAddressText = json_object_get_string_utf8(arguments, "ipAddress", nullptr);
   if (ipAddressText != nullptr)
   {
      ipAddressFilter = InetAddress::parse(ipAddressText);
      if (!ipAddressFilter.isValid())
      {
         return std::string("Invalid IP address");
      }
   }

   std::unordered_set<int> classFilter;
   json_t *classes = json_object_get(arguments, "classes");
   if (json_is_array(classes))
   {
      size_t i;
      json_t *c;
      json_array_foreach(classes, i, c)
      {
         int n = NetObj::getObjectClassByNameA(json_string_value(c));
         if (n == OBJECT_GENERIC)
         {
            return std::string("Invalid object class ") + json_string_value(c);
         }
         classFilter.insert(n);
      }
   }
   else if (json_is_string(classes))
   {
      int n = NetObj::getObjectClassByNameA(json_string_value(classes));
      if (n == OBJECT_GENERIC)
      {
         return std::string("Invalid object class ") + json_string_value(classes);
      }
      classFilter.insert(n);
   }

   if ((parentId == 0) && (zoneUIN == 0) && (name[0] == 0) && classFilter.empty() && !ipAddressFilter.isValid())
      return std::string("At least one search criteria should be set");

   unique_ptr<SharedObjectArray<NetObj>> objects = g_idxObjectById.getObjects(
      [userId, parentId, zoneUIN, name, &classFilter, ipAddressFilter] (NetObj *object) -> bool
      {
         if (object->isUnpublished() || object->isDeleted() || !object->checkAccessRights(userId, OBJECT_ACCESS_READ))
            return false;
         if ((zoneUIN != 0) && (object->getZoneUIN() != zoneUIN))
            return false;
         if (!classFilter.empty() && (classFilter.count(object->getObjectClass()) == 0))
            return false;
         if ((name[0] != 0) &&
             (wcsistr(object->getName(), name) == nullptr) && (wcsistr(object->getAlias(), name) == nullptr) &&
             !FuzzyMatchStringsIgnoreCase(name, object->getName()) && !FuzzyMatchStringsIgnoreCase(name, object->getAlias()))
            return false;
         if (ipAddressFilter.isValid() && !object->getPrimaryIpAddress().equals(ipAddressFilter))
            return false;
         return (parentId != 0) ? object->isParent(parentId) : true;
      });

   json_t *output = json_array();
   for(int i = 0; i < objects->size(); i++)
   {
      json_array_append_new(output, CreateObjectSummary(*objects->get(i)));
   }
   return JsonToString(output);
}

/**
 * Start maintenance period for object
 */
std::string F_StartMaintenance(json_t *arguments, uint32_t userId)
{
   const char *objectName = json_object_get_string_utf8(arguments, "object", nullptr);
   if ((objectName == nullptr) || (objectName[0] == 0))
      return std::string("Object name or ID must be provided");

   shared_ptr<NetObj> object = FindObjectByNameOrId(objectName);
   if ((object == nullptr) || !object->checkAccessRights(userId, OBJECT_ACCESS_READ))
   {
      char buffer[256];
      snprintf(buffer, 256, "Object with name or ID \"%s\" is not known", objectName);
      return std::string(buffer);
   }

   if (!object->checkAccessRights(userId, OBJECT_ACCESS_MAINTENANCE))
      return std::string("Access denied");

   if (!object->isMaintenanceApplicable())
      return std::string("Maintenance mode is not applicable for this object");

   if (object->isInMaintenanceMode())
      return std::string("Object is already in maintenance mode");

   String message(json_object_get_string_utf8(arguments, "message", "Maintenance period started via AI assistant"), "utf-8");
   object->enterMaintenanceMode(userId, message);
   return std::string("Maintenance period started for object");
}

/**
 * End maintenance period for object
 */
std::string F_EndMaintenance(json_t *arguments, uint32_t userId)
{
   const char *objectName = json_object_get_string_utf8(arguments, "object", nullptr);
   if ((objectName == nullptr) || (objectName[0] == 0))
      return std::string("Object name or ID must be provided");

   shared_ptr<NetObj> object = FindObjectByNameOrId(objectName);
   if ((object == nullptr) || !object->checkAccessRights(userId, OBJECT_ACCESS_READ))
   {
      char buffer[256];
      snprintf(buffer, 256, "Object with name or ID \"%s\" is not known", objectName);
      return std::string(buffer);
   }

   if (!object->checkAccessRights(userId, OBJECT_ACCESS_MAINTENANCE))
      return std::string("Access denied");

   if (!object->isInMaintenanceMode())
      return std::string("Object is not in maintenance mode");

   object->leaveMaintenanceMode(userId);
   return std::string("Maintenance period ended for object");
}

/**
 * Get list of hardware components for given node
 */
std::string F_GetNodeHardwareComponents(json_t *arguments, uint32_t userId)
{
   const char *objectName = json_object_get_string_utf8(arguments, "object", nullptr);
   if ((objectName == nullptr) || (objectName[0] == 0))
      return std::string("Object name or ID must be provided");

   shared_ptr<NetObj> object = FindObjectByNameOrId(objectName, OBJECT_NODE);
   if ((object == nullptr) || !object->checkAccessRights(userId, OBJECT_ACCESS_READ))
   {
      char buffer[256];
      snprintf(buffer, 256, "Node with name or ID \"%s\" is not known", objectName);
      return std::string(buffer);
   }

   Node *node = static_cast<Node*>(object.get());
   json_t *components = node->getHardwareComponentsAsJSON();
   if (components == nullptr)
   {
      return std::string("No hardware component information available for this node");
   }

   return JsonToString(components);
}

/**
 * Get list of interfaces for given node
 */
std::string F_GetNodeInterfaces(json_t *arguments, uint32_t userId)
{
   const char *objectName = json_object_get_string_utf8(arguments, "object", nullptr);
   if ((objectName == nullptr) || (objectName[0] == 0))
      return std::string("Object name or ID must be provided");

   shared_ptr<NetObj> object = FindObjectByNameOrId(objectName, OBJECT_NODE);
   if ((object == nullptr) || !object->checkAccessRights(userId, OBJECT_ACCESS_READ))
   {
      char buffer[256];
      snprintf(buffer, 256, "Node with name or ID \"%s\" is not known", objectName);
      return std::string(buffer);
   }

   Node *node = static_cast<Node*>(object.get());
   unique_ptr<SharedObjectArray<NetObj>> interfaces = node->getChildren(OBJECT_INTERFACE);

   json_t *output = json_array();
   for(int i = 0; i < interfaces->size(); i++)
   {
      json_array_append_new(output, interfaces->get(i)->toJson());
   }
   return JsonToString(output);
}

/**
 * Get list of software packaged for given node
 */
std::string F_GetNodeSoftwarePackages(json_t *arguments, uint32_t userId)
{
   const char *objectName = json_object_get_string_utf8(arguments, "object", nullptr);
   if ((objectName == nullptr) || (objectName[0] == 0))
      return std::string("Object name or ID must be provided");

   shared_ptr<NetObj> object = FindObjectByNameOrId(objectName, OBJECT_NODE);
   if ((object == nullptr) || !object->checkAccessRights(userId, OBJECT_ACCESS_READ))
   {
      char buffer[256];
      snprintf(buffer, 256, "Node with name or ID \"%s\" is not known", objectName);
      return std::string(buffer);
   }

   String filter(json_object_get_string_utf8(arguments, "filter", ""), "utf-8");

   Node *node = static_cast<Node*>(object.get());
   json_t *components = node->getSoftwarePackagesAsJSON(!filter.isEmpty() ? filter.cstr() : nullptr);
   if (components == nullptr)
   {
      return std::string("No software package information available for this node");
   }

   return JsonToString(components);
}

/**
 * Store AI agent data for an object
 */
std::string F_SetObjectAIData(json_t *arguments, uint32_t userId)
{
   const char *objectName = json_object_get_string_utf8(arguments, "object", nullptr);
   const char *key = json_object_get_string_utf8(arguments, "key", nullptr);
   json_t *value = json_object_get(arguments, "value");

   if ((objectName == nullptr) || (objectName[0] == 0))
      return std::string("Object name or ID must be provided");
   
   if ((key == nullptr) || (key[0] == 0))
      return std::string("Key must be provided");
   
   if (value == nullptr)
      return std::string("Value must be provided");

   shared_ptr<NetObj> object = FindObjectByNameOrId(objectName);
   if ((object == nullptr) || !object->checkAccessRights(userId, OBJECT_ACCESS_READ))
   {
      char buffer[256];
      snprintf(buffer, 256, "Object with name or ID \"%s\" is not known or not accessible", objectName);
      return std::string(buffer);
   }

   if (!object->checkAccessRights(userId, OBJECT_ACCESS_MODIFY))
      return std::string("Access denied");

   // if value is json object presented as string, parse it
   if (json_is_string(value))
   {
      const char *valueStr = json_string_value(value);
      json_error_t error;
      json_t *parsedValue = json_loads(valueStr, 0, &error);
      if (parsedValue != nullptr)
      {
         json_decref(value);
         value = parsedValue;
      }
      else
      {
         json_incref(value);
      }
   }

   if (json_is_object(value))
   {
      object->setAIData(key, value);
   }
   else
   {
      json_t *wrappedValue = json_object();
      json_object_set(wrappedValue, "data", value);
      object->setAIData(key, wrappedValue);
      json_decref(wrappedValue);
   }
   json_decref(value);

   return std::string("AI data stored successfully");
}

/**
 * Retrieve AI agent data for an object
 */
std::string F_GetObjectAIData(json_t *arguments, uint32_t userId)
{
   const char *objectName = json_object_get_string_utf8(arguments, "object", nullptr);
   const char *key = json_object_get_string_utf8(arguments, "key", nullptr);

   if ((objectName == nullptr) || (objectName[0] == 0))
      return std::string("Object name or ID must be provided");

   shared_ptr<NetObj> object = FindObjectByNameOrId(objectName);
   if ((object == nullptr) || !object->checkAccessRights(userId, OBJECT_ACCESS_READ))
   {
      char buffer[256];
      snprintf(buffer, 256, "Object with name or ID \"%s\" is not known or not accessible", objectName);
      return std::string(buffer);
   }

   json_t *data;
   if ((key == nullptr) || (key[0] == 0))
   {
      // Return all AI data if no key specified
      data = object->getAllAIData();
   }
   else
   {
      // Return specific key
      data = object->getAIData(key);
   }

   if (data == nullptr)
   {
      if ((key != nullptr) && (key[0] != 0))
         return std::string("AI data not found for the specified key");
      else
         return std::string("{}"); // Empty object if no AI data exists
   }

   return JsonToString(data);
}

/**
 * Remove AI agent data for an object
 */
std::string F_RemoveObjectAIData(json_t *arguments, uint32_t userId)
{
   const char *objectName = json_object_get_string_utf8(arguments, "object", nullptr);
   const char *key = json_object_get_string_utf8(arguments, "key", nullptr);

   if ((objectName == nullptr) || (objectName[0] == 0))
      return std::string("Object name or ID must be provided");

   if ((key == nullptr) || (key[0] == 0))
      return std::string("Key must be provided");

   shared_ptr<NetObj> object = FindObjectByNameOrId(objectName);
   if ((object == nullptr) || !object->checkAccessRights(userId, OBJECT_ACCESS_READ))
   {
      char buffer[256];
      snprintf(buffer, 256, "Object with name or ID \"%s\" is not known or not accessible", objectName);
      return std::string(buffer);
   }

   if (!object->checkAccessRights(userId, OBJECT_ACCESS_MODIFY))
      return std::string("Access denied");

   bool removed = object->removeAIData(key);
   if (removed)
      return std::string("AI data removed successfully");
   else
      return std::string("AI data not found for the specified key");
}

/**
 * List all AI data keys for an object
 */
std::string F_ListObjectAIDataKeys(json_t *arguments, uint32_t userId)
{
   const char *objectName = json_object_get_string_utf8(arguments, "object", nullptr);

   if ((objectName == nullptr) || (objectName[0] == 0))
      return std::string("Object name or ID must be provided");

   shared_ptr<NetObj> object = FindObjectByNameOrId(objectName);
   if ((object == nullptr) || !object->checkAccessRights(userId, OBJECT_ACCESS_READ))
   {
      char buffer[256];
      snprintf(buffer, 256, "Object with name or ID \"%s\" is not known or not accessible", objectName);
      return std::string(buffer);
   }

   json_t *keys = object->getAIDataKeys();
   if (keys == nullptr)
   {
      return std::string("[]"); // Empty array if no AI data exists
   }

   return JsonToString(keys);
}

/**
 * Explain object status
 */
std::string F_ExplainObjectStatus(json_t *arguments, uint32_t userId)
{
   const char *objectName = json_object_get_string_utf8(arguments, "object", nullptr);

   if ((objectName == nullptr) || (objectName[0] == 0))
      return std::string("Object name or ID must be provided");

   shared_ptr<NetObj> object = FindObjectByNameOrId(objectName);
   if ((object == nullptr) || !object->checkAccessRights(userId, OBJECT_ACCESS_READ))
   {
      char buffer[256];
      snprintf(buffer, 256, "Object with name or ID \"%s\" is not known or not accessible", objectName);
      return std::string(buffer);
   }

   return JsonToString(object->buildStatusExplanation());
}
