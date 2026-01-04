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
** File: incidents.cpp
**
**/

#include "aitools.h"
#include <nms_users.h>
#include <nms_incident.h>
#include <nms_alarm.h>
#include <nms_topo.h>

/**
 * Get incident details with full context
 */
std::string F_GetIncidentDetails(json_t *arguments, uint32_t userId)
{
   uint32_t incidentId = json_object_get_uint32(arguments, "incident_id", 0);
   if (incidentId == 0)
      return std::string("Incident ID must be provided");

   shared_ptr<Incident> incident = FindIncidentById(incidentId);
   if (incident == nullptr)
      return std::string("Incident not found");

   // Check access rights on source object
   shared_ptr<NetObj> sourceObject = FindObjectById(incident->getSourceObjectId());
   if ((sourceObject == nullptr) || !sourceObject->checkAccessRights(userId, OBJECT_ACCESS_READ | OBJECT_ACCESS_MANAGE_INCIDENTS))
      return std::string("Access denied or source object not found");

   // Build response JSON
   json_t *root = incident->toJson();

   // Add source object details
   json_t *sourceObjectJson = json_object();
   json_object_set_new(sourceObjectJson, "id", json_integer(sourceObject->getId()));
   json_object_set_new(sourceObjectJson, "name", json_string_t(sourceObject->getName()));
   json_object_set_new(sourceObjectJson, "className", json_string_t(sourceObject->getObjectClassName()));
   json_object_set_new(sourceObjectJson, "status", json_integer(sourceObject->getStatus()));
   json_object_set_new(root, "sourceObject", sourceObjectJson);

   // Add linked alarm details
   json_t *alarmsArray = json_array();
   const IntegerArray<uint32_t>& linkedAlarms = incident->getLinkedAlarms();
   for (int i = 0; i < linkedAlarms.size(); i++)
   {
      Alarm *alarm = FindAlarmById(linkedAlarms.get(i));
      if (alarm != nullptr)
      {
         json_t *alarmJson = json_object();
         json_object_set_new(alarmJson, "id", json_integer(alarm->getAlarmId()));
         json_object_set_new(alarmJson, "severity", json_string(AlarmSeverityTextFromCode(alarm->getCurrentSeverity())));
         json_object_set_new(alarmJson, "message", json_string_t(alarm->getMessage()));
         json_object_set_new(alarmJson, "sourceObject", json_integer(alarm->getSourceObject()));
         json_object_set_new(alarmJson, "sourceObjectName", json_string_w(GetObjectName(alarm->getSourceObject(), L"unknown")));
         json_object_set_new(alarmJson, "state", json_integer(alarm->getState()));
         json_object_set_new(alarmJson, "creationTime", json_time_string(alarm->getCreationTime()));
         json_object_set_new(alarmJson, "lastChangeTime", json_time_string(alarm->getLastChangeTime()));
         json_array_append_new(alarmsArray, alarmJson);
         delete alarm;
      }
   }
   json_object_set_new(root, "linkedAlarmDetails", alarmsArray);

   // Add comments
   json_t *commentsArray = json_array();
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT id,creation_time,user_id,comment_text FROM incident_comments WHERE incident_id=? ORDER BY creation_time"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, incidentId);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         for (int i = 0; i < count; i++)
         {
            json_t *comment = json_object();
            json_object_set_new(comment, "id", json_integer(DBGetFieldULong(hResult, i, 0)));
            json_object_set_new(comment, "creationTime", json_time_string(DBGetFieldULong(hResult, i, 1)));
            json_object_set_new(comment, "userId", json_integer(DBGetFieldULong(hResult, i, 2)));
            TCHAR *text = DBGetField(hResult, i, 3, nullptr, 0);
            json_object_set_new(comment, "text", json_string_t(text));
            MemFree(text);
            json_array_append_new(commentsArray, comment);
         }
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);
   json_object_set_new(root, "comments", commentsArray);

   return JsonToString(root);
}

/**
 * Get events related to incident (within time window)
 */
std::string F_GetIncidentRelatedEvents(json_t *arguments, uint32_t userId)
{
   // Check system access rights for event log
   uint64_t systemAccessRights = GetEffectiveSystemRights(userId);
   if ((systemAccessRights & SYSTEM_ACCESS_VIEW_EVENT_LOG) == 0)
      return std::string("User does not have rights to view event log");

   uint32_t incidentId = json_object_get_uint32(arguments, "incident_id", 0);
   if (incidentId == 0)
      return std::string("Incident ID must be provided");

   uint32_t timeWindowSeconds = json_object_get_uint32(arguments, "time_window_seconds", 3600);

   shared_ptr<Incident> incident = FindIncidentById(incidentId);
   if (incident == nullptr)
      return std::string("Incident not found");

   // Check access rights on source object
   shared_ptr<NetObj> sourceObject = FindObjectById(incident->getSourceObjectId());
   if ((sourceObject == nullptr) || !sourceObject->checkAccessRights(userId, OBJECT_ACCESS_READ))
      return std::string("Access denied or source object not found");

   // Collect object IDs to query (source object + all alarm source objects)
   IntegerArray<uint32_t> objectIds;
   objectIds.add(incident->getSourceObjectId());

   const IntegerArray<uint32_t>& linkedAlarms = incident->getLinkedAlarms();
   for (int i = 0; i < linkedAlarms.size(); i++)
   {
      Alarm *alarm = FindAlarmById(linkedAlarms.get(i));
      if (alarm != nullptr)
      {
         if (!objectIds.contains(alarm->getSourceObject()))
            objectIds.add(alarm->getSourceObject());
         delete alarm;
      }
   }

   // Build query for events around incident creation time
   time_t incidentTime = incident->getCreationTime();
   time_t startTime = incidentTime - timeWindowSeconds;
   time_t endTime = incidentTime + timeWindowSeconds;

   // Build object ID list for SQL IN clause
   StringBuffer objectIdList;
   for (int i = 0; i < objectIds.size(); i++)
   {
      if (i > 0)
         objectIdList.append(_T(","));
      objectIdList.append(objectIds.get(i));
   }

   json_t *eventsArray = json_array();
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   wchar_t query[1024];
   nx_swprintf(query, 1024,
      L"SELECT event_id,event_code,event_timestamp,event_source,event_severity,event_message "
      L"FROM event_log WHERE event_source IN (%s) AND event_timestamp BETWEEN " INT64_FMT L" AND " INT64_FMT
      L" ORDER BY event_timestamp DESC LIMIT 100",
      objectIdList.cstr(), static_cast<int64_t>(startTime), static_cast<int64_t>(endTime));

   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for (int i = 0; i < count; i++)
      {
         json_t *event = json_object();
         json_object_set_new(event, "id", json_integer(DBGetFieldInt64(hResult, i, 0)));
         uint32_t eventCode = DBGetFieldUInt32(hResult, i, 1);
         json_object_set_new(event, "code", json_integer(eventCode));
         json_object_set_new(event, "timestamp", json_time_string(DBGetFieldUInt32(hResult, i, 2)));
         uint32_t eventSourceId = DBGetFieldULong(hResult, i, 3);
         json_object_set_new(event, "sourceId", json_integer(eventSourceId));
         json_object_set_new(event, "sourceName", json_string_w(GetObjectName(eventSourceId, L"unknown")));
         json_object_set_new(event, "severity", json_integer(DBGetFieldLong(hResult, i, 4)));
         TCHAR *message = DBGetField(hResult, i, 5, nullptr, 0);
         json_object_set_new(event, "message", json_string_t(message));
         MemFree(message);
         TCHAR eventName[MAX_DB_STRING];
         if (EventNameFromCode(eventCode, eventName))
            json_object_set_new(event, "name", json_string_t( eventName));
         json_array_append_new(eventsArray, event);
      }
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);

   return JsonToString(eventsArray);
}

/**
 * Query incidents for an object (shared implementation)
 * @param object Target object
 * @param openOnly If true, return only open incidents (state < 4); if false, return all incidents
 * @param maxCount Maximum number of incidents to return (0 = unlimited)
 */
static std::string QueryObjectIncidents(const shared_ptr<NetObj>& object, bool openOnly, uint32_t maxCount)
{
   json_t *incidentsArray = json_array();
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   wchar_t query[512];
   if (openOnly)
   {
      nx_swprintf(query, 512,
         L"SELECT id,creation_time,last_change_time,state,assigned_user_id,title,close_time "
         L"FROM incidents WHERE source_object_id=%u AND state<4 ORDER BY creation_time DESC",
         object->getId());
   }
   else if (maxCount > 0)
   {
      nx_swprintf(query, 512,
         L"SELECT id,creation_time,last_change_time,state,assigned_user_id,title,close_time "
         L"FROM incidents WHERE source_object_id=%u ORDER BY creation_time DESC LIMIT %u",
         object->getId(), maxCount);
   }
   else
   {
      nx_swprintf(query, 512,
         L"SELECT id,creation_time,last_change_time,state,assigned_user_id,title,close_time "
         L"FROM incidents WHERE source_object_id=%u ORDER BY creation_time DESC",
         object->getId());
   }

   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for (int i = 0; i < count; i++)
      {
         json_t *inc = json_object();
         json_object_set_new(inc, "id", json_integer(DBGetFieldULong(hResult, i, 0)));
         json_object_set_new(inc, "creationTime", json_time_string(DBGetFieldULong(hResult, i, 1)));
         json_object_set_new(inc, "lastChangeTime", json_time_string(DBGetFieldULong(hResult, i, 2)));
         int state = DBGetFieldLong(hResult, i, 3);
         json_object_set_new(inc, "state", json_integer(state));
         json_object_set_new(inc, "stateName", json_string_t(GetIncidentStateName(state)));
         uint32_t assignedUserId = DBGetFieldULong(hResult, i, 4);
         json_object_set_new(inc, "assignedUserId", json_integer(assignedUserId));
         if (assignedUserId != 0)
         {
            TCHAR userName[MAX_USER_NAME];
            json_object_set_new(inc, "assignedUserName", json_string_t(ResolveUserId(assignedUserId, userName, true)));
         }
         TCHAR title[256];
         DBGetField(hResult, i, 5, title, 256);
         json_object_set_new(inc, "title", json_string_t(title));
         time_t closeTime = DBGetFieldULong(hResult, i, 6);
         if (closeTime > 0)
            json_object_set_new(inc, "closeTime", json_time_string(closeTime));
         json_array_append_new(incidentsArray, inc);
      }
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);

   json_t *result = json_object();
   json_object_set_new(result, "objectId", json_integer(object->getId()));
   json_object_set_new(result, "objectName", json_string_t(object->getName()));
   json_object_set_new(result, "incidents", incidentsArray);
   json_object_set_new(result, "count", json_integer(json_array_size(incidentsArray)));

   return JsonToString(result);
}

/**
 * Get historical incidents for an object
 */
std::string F_GetIncidentHistory(json_t *arguments, uint32_t userId)
{
   const char *objectName = json_object_get_string_utf8(arguments, "object", nullptr);
   if ((objectName == nullptr) || (objectName[0] == 0))
      return std::string("Object name or ID must be provided");

   shared_ptr<NetObj> object = FindObjectByNameOrId(objectName);
   if ((object == nullptr) || !object->checkAccessRights(userId, OBJECT_ACCESS_MANAGE_INCIDENTS))
      return std::string("Object not found or access denied");

   uint32_t maxCount = json_object_get_uint32(arguments, "max_count", 20);
   if (maxCount > 100)
      maxCount = 100;

   return QueryObjectIncidents(object, false, maxCount);
}

/**
 * Get topology context for incident (network topology: neighbors, subnets, routing)
 */
std::string F_GetIncidentTopologyContext(json_t *arguments, uint32_t userId)
{
   uint32_t incidentId = json_object_get_uint32(arguments, "incident_id", 0);
   if (incidentId == 0)
      return std::string("Incident ID must be provided");

   shared_ptr<Incident> incident = FindIncidentById(incidentId);
   if (incident == nullptr)
      return std::string("Incident not found");

   shared_ptr<NetObj> sourceObject = FindObjectById(incident->getSourceObjectId());
   if ((sourceObject == nullptr) || !sourceObject->checkAccessRights(userId, OBJECT_ACCESS_READ))
      return std::string("Access denied or source object not found");

   json_t *root = json_object();

   // Source object info
   json_t *sourceJson = json_object();
   json_object_set_new(sourceJson, "id", json_integer(sourceObject->getId()));
   json_object_set_new(sourceJson, "name", json_string_t(sourceObject->getName()));
   json_object_set_new(sourceJson, "className", json_string_t(sourceObject->getObjectClassName()));
   json_object_set_new(sourceJson, "status", json_integer(sourceObject->getStatus()));
   json_object_set_new(root, "sourceObject", sourceJson);

   // For Node objects, get network topology information
   if (sourceObject->getObjectClass() == OBJECT_NODE)
   {
      Node *node = static_cast<Node*>(sourceObject.get());

      // Get link layer neighbors (L2 topology from LLDP/CDP/STP discovery)
      json_t *neighborsArray = json_array();
      shared_ptr<LinkLayerNeighbors> neighbors = node->getLinkLayerNeighbors();
      if (neighbors != nullptr)
      {
         HashSet<uint32_t> addedNodes;
         for (int i = 0; i < neighbors->size(); i++)
         {
            LL_NEIGHBOR_INFO *info = neighbors->getConnection(i);
            if ((info != nullptr) && !addedNodes.contains(info->objectId))
            {
               addedNodes.put(info->objectId);
               shared_ptr<NetObj> peerNode = FindObjectById(info->objectId);
               if ((peerNode != nullptr) && peerNode->checkAccessRights(userId, OBJECT_ACCESS_READ))
               {
                  json_t *neighbor = json_object();
                  json_object_set_new(neighbor, "id", json_integer(peerNode->getId()));
                  json_object_set_new(neighbor, "name", json_string_t(peerNode->getName()));
                  json_object_set_new(neighbor, "className", json_string_t(peerNode->getObjectClassName()));
                  json_object_set_new(neighbor, "status", json_integer(peerNode->getStatus()));

                  // Get local interface info
                  shared_ptr<Interface> localIface = static_pointer_cast<Interface>(FindObjectById(info->ifLocal));
                  if (localIface != nullptr)
                     json_object_set_new(neighbor, "localInterface", json_string_t(localIface->getName()));

                  // Get remote interface info
                  shared_ptr<Interface> remoteIface = static_pointer_cast<Interface>(FindObjectById(info->ifRemote));
                  if (remoteIface != nullptr)
                     json_object_set_new(neighbor, "remoteInterface", json_string_t(remoteIface->getName()));

                  json_array_append_new(neighborsArray, neighbor);
               }
            }
         }
      }
      json_object_set_new(root, "linkLayerNeighbors", neighborsArray);

      // Get parent subnets
      json_t *subnetsArray = json_array();
      unique_ptr<SharedObjectArray<NetObj>> parents = sourceObject->getParents();
      for (int i = 0; i < parents->size(); i++)
      {
         NetObj *parent = parents->get(i);
         if ((parent->getObjectClass() == OBJECT_SUBNET) && parent->checkAccessRights(userId, OBJECT_ACCESS_READ))
         {
            Subnet *subnet = static_cast<Subnet*>(parent);
            json_t *subnetJson = json_object();
            json_object_set_new(subnetJson, "id", json_integer(subnet->getId()));
            json_object_set_new(subnetJson, "name", json_string_t(subnet->getName()));
            json_object_set_new(subnetJson, "ipAddress", json_string_t(subnet->getIpAddress().toString()));
            json_object_set_new(subnetJson, "status", json_integer(subnet->getStatus()));
            json_array_append_new(subnetsArray, subnetJson);
         }
      }
      json_object_set_new(root, "subnets", subnetsArray);

      // Get routing table (next hops)
      json_t *routesArray = json_array();
      shared_ptr<RoutingTable> routingTable = node->getCachedRoutingTable();
      if (routingTable != nullptr)
      {
         // Only include default and major routes (not all /32 host routes)
         int routeCount = 0;
         for (int i = 0; (i < routingTable->size()) && (routeCount < 20); i++)
         {
            const ROUTE *route = routingTable->get(i);
            // Include default route and routes with mask <= 24 bits
            if ((route->destination.getMaskBits() <= 24) || route->destination.isAnyLocal())
            {
               json_t *routeJson = json_object();
               json_object_set_new(routeJson, "destination", json_string_t(route->destination.toString()));
               json_object_set_new(routeJson, "nextHop", json_string_t(route->nextHop.toString()));
               json_object_set_new(routeJson, "metric", json_integer(route->metric));

               // Resolve next hop to node if possible
               shared_ptr<Node> nextHopNode = FindNodeByIP(0, route->nextHop);
               if ((nextHopNode != nullptr) && nextHopNode->checkAccessRights(userId, OBJECT_ACCESS_READ))
               {
                  json_object_set_new(routeJson, "nextHopNodeId", json_integer(nextHopNode->getId()));
                  json_object_set_new(routeJson, "nextHopNodeName", json_string_t(nextHopNode->getName()));
                  json_object_set_new(routeJson, "nextHopNodeStatus", json_integer(nextHopNode->getStatus()));
               }

               json_array_append_new(routesArray, routeJson);
               routeCount++;
            }
         }
      }
      json_object_set_new(root, "routes", routesArray);

      // Get interface peers (direct connections discovered via interface peer tracking)
      json_t *peersArray = json_array();
      unique_ptr<SharedObjectArray<NetObj>> children = sourceObject->getChildren();
      HashSet<uint32_t> addedPeers;
      for (int i = 0; i < children->size(); i++)
      {
         NetObj *child = children->get(i);
         if (child->getObjectClass() == OBJECT_INTERFACE)
         {
            Interface *iface = static_cast<Interface*>(child);
            uint32_t peerNodeId = iface->getPeerNodeId();
            if ((peerNodeId != 0) && !addedPeers.contains(peerNodeId))
            {
               addedPeers.put(peerNodeId);
               shared_ptr<NetObj> peerNode = FindObjectById(peerNodeId);
               if ((peerNode != nullptr) && peerNode->checkAccessRights(userId, OBJECT_ACCESS_READ))
               {
                  json_t *peer = json_object();
                  json_object_set_new(peer, "id", json_integer(peerNode->getId()));
                  json_object_set_new(peer, "name", json_string_t(peerNode->getName()));
                  json_object_set_new(peer, "className", json_string_t(peerNode->getObjectClassName()));
                  json_object_set_new(peer, "status", json_integer(peerNode->getStatus()));
                  json_object_set_new(peer, "localInterface", json_string_t(iface->getName()));

                  shared_ptr<Interface> peerIface = static_pointer_cast<Interface>(FindObjectById(iface->getPeerInterfaceId()));
                  if (peerIface != nullptr)
                     json_object_set_new(peer, "remoteInterface", json_string_t(peerIface->getName()));

                  json_array_append_new(peersArray, peer);
               }
            }
         }
      }
      json_object_set_new(root, "interfacePeers", peersArray);
   }
   else
   {
      // For non-node objects, just return parent subnets/containers for context
      json_t *parentsArray = json_array();
      unique_ptr<SharedObjectArray<NetObj>> parents = sourceObject->getParents();
      for (int i = 0; i < parents->size(); i++)
      {
         NetObj *parent = parents->get(i);
         if (parent->checkAccessRights(userId, OBJECT_ACCESS_READ))
         {
            json_t *parentJson = json_object();
            json_object_set_new(parentJson, "id", json_integer(parent->getId()));
            json_object_set_new(parentJson, "name", json_string_t(parent->getName()));
            json_object_set_new(parentJson, "className", json_string_t(parent->getObjectClassName()));
            json_object_set_new(parentJson, "status", json_integer(parent->getStatus()));
            json_array_append_new(parentsArray, parentJson);
         }
      }
      json_object_set_new(root, "parents", parentsArray);
   }

   return JsonToString(root);
}

/**
 * Add comment to incident
 */
std::string F_AddIncidentComment(json_t *arguments, uint32_t userId)
{
   uint32_t incidentId = json_object_get_uint32(arguments, "incident_id", 0);
   if (incidentId == 0)
      return std::string("Incident ID must be provided");

   const char *text = json_object_get_string_utf8(arguments, "text", nullptr);
   if ((text == nullptr) || (text[0] == 0))
      return std::string("Comment text must be provided");

   shared_ptr<Incident> incident = FindIncidentById(incidentId);
   if (incident == nullptr)
      return std::string("Incident not found");

   // Check access rights on source object
   shared_ptr<NetObj> sourceObject = FindObjectById(incident->getSourceObjectId());
   if ((sourceObject == nullptr) || !sourceObject->checkAccessRights(userId, OBJECT_ACCESS_MANAGE_INCIDENTS))
      return std::string("Access denied or source object not found");

   uint32_t commentId = 0;
   uint32_t rcc = AddIncidentComment(incidentId, String(text, "utf-8"), userId, &commentId, true);
   if (rcc != RCC_SUCCESS)
   {
      char buffer[128];
      snprintf(buffer, 128, "Failed to add comment: error code %u", rcc);
      return std::string(buffer);
   }

   json_t *result = json_object();
   json_object_set_new(result, "success", json_true());
   json_object_set_new(result, "commentId", json_integer(commentId));
   return JsonToString(result);
}

/**
 * Link alarm to incident
 */
std::string F_LinkAlarmToIncident(json_t *arguments, uint32_t userId)
{
   uint32_t incidentId = json_object_get_uint32(arguments, "incident_id", 0);
   if (incidentId == 0)
      return std::string("Incident ID must be provided");

   uint32_t alarmId = json_object_get_uint32(arguments, "alarm_id", 0);
   if (alarmId == 0)
      return std::string("Alarm ID must be provided");

   shared_ptr<Incident> incident = FindIncidentById(incidentId);
   if (incident == nullptr)
      return std::string("Incident not found");

   // Check access rights on source object
   shared_ptr<NetObj> sourceObject = FindObjectById(incident->getSourceObjectId());
   if ((sourceObject == nullptr) || !sourceObject->checkAccessRights(userId, OBJECT_ACCESS_MANAGE_INCIDENTS))
      return std::string("Access denied or source object not found");

   uint32_t rcc = LinkAlarmToIncident(incidentId, alarmId, userId);
   if (rcc == RCC_ALARM_ALREADY_IN_INCIDENT)
      return std::string("Alarm is already linked to another incident");
   if (rcc != RCC_SUCCESS)
   {
      char buffer[128];
      snprintf(buffer, 128, "Failed to link alarm: error code %u", rcc);
      return std::string(buffer);
   }

   json_t *result = json_object();
   json_object_set_new(result, "success", json_true());
   return JsonToString(result);
}

/**
 * Link multiple alarms to incident
 */
std::string F_LinkAlarmsToIncident(json_t *arguments, uint32_t userId)
{
   uint32_t incidentId = json_object_get_uint32(arguments, "incident_id", 0);
   if (incidentId == 0)
      return std::string("Incident ID must be provided");

   json_t *alarmIds = json_object_get(arguments, "alarm_ids");
   if ((alarmIds == nullptr) || !json_is_array(alarmIds))
      return std::string("Alarm IDs array must be provided");

   shared_ptr<Incident> incident = FindIncidentById(incidentId);
   if (incident == nullptr)
      return std::string("Incident not found");

   // Check access rights on source object
   shared_ptr<NetObj> sourceObject = FindObjectById(incident->getSourceObjectId());
   if ((sourceObject == nullptr) || !sourceObject->checkAccessRights(userId, OBJECT_ACCESS_MANAGE_INCIDENTS))
      return std::string("Access denied or source object not found");

   int linkedCount = 0;
   json_t *failures = json_array();

   size_t index;
   json_t *value;
   json_array_foreach(alarmIds, index, value)
   {
      uint32_t alarmId = static_cast<uint32_t>(json_integer_value(value));
      if (alarmId == 0)
         continue;

      uint32_t rcc = LinkAlarmToIncident(incidentId, alarmId, userId);
      if (rcc == RCC_SUCCESS)
      {
         linkedCount++;
      }
      else
      {
         json_t *failure = json_object();
         json_object_set_new(failure, "alarmId", json_integer(alarmId));
         json_object_set_new(failure, "errorCode", json_integer(rcc));
         if (rcc == RCC_ALARM_ALREADY_IN_INCIDENT)
            json_object_set_new(failure, "reason", json_string("Alarm already in another incident"));
         else
            json_object_set_new(failure, "reason", json_string("Failed to link"));
         json_array_append_new(failures, failure);
      }
   }

   json_t *result = json_object();
   json_object_set_new(result, "linkedCount", json_integer(linkedCount));
   json_object_set_new(result, "failures", failures);
   return JsonToString(result);
}

/**
 * Assign incident to user
 */
std::string F_AssignIncident(json_t *arguments, uint32_t userId)
{
   uint32_t incidentId = json_object_get_uint32(arguments, "incident_id", 0);
   if (incidentId == 0)
      return std::string("Incident ID must be provided");

   shared_ptr<Incident> incident = FindIncidentById(incidentId);
   if (incident == nullptr)
      return std::string("Incident not found");

   // Check access rights on source object
   shared_ptr<NetObj> sourceObject = FindObjectById(incident->getSourceObjectId());
   if ((sourceObject == nullptr) || !sourceObject->checkAccessRights(userId, OBJECT_ACCESS_MANAGE_INCIDENTS))
      return std::string("Access denied or source object not found");

   // Get assignee - either by ID or name
   uint32_t assigneeId = json_object_get_uint32(arguments, "user_id", 0);
   if (assigneeId == 0)
   {
      const char *userName = json_object_get_string_utf8(arguments, "user_name", nullptr);
      if ((userName != nullptr) && (userName[0] != 0))
      {
         assigneeId = ResolveUserName(String(userName, "utf-8"));
         if (assigneeId == 0)
            return std::string("User not found");
      }
   }

   if (assigneeId == 0)
      return std::string("User ID or user name must be provided");

   uint32_t rcc = AssignIncident(incidentId, assigneeId, userId);
   if (rcc != RCC_SUCCESS)
   {
      char buffer[128];
      snprintf(buffer, 128, "Failed to assign incident: error code %u", rcc);
      return std::string(buffer);
   }

   json_t *result = json_object();
   json_object_set_new(result, "success", json_true());
   json_object_set_new(result, "assignedUserId", json_integer(assigneeId));
   return JsonToString(result);
}

/**
 * Suggest incident assignee based on responsible users
 */
std::string F_SuggestIncidentAssignee(json_t *arguments, uint32_t userId)
{
   uint32_t incidentId = json_object_get_uint32(arguments, "incident_id", 0);
   if (incidentId == 0)
      return std::string("Incident ID must be provided");

   shared_ptr<Incident> incident = FindIncidentById(incidentId);
   if (incident == nullptr)
      return std::string("Incident not found");

   shared_ptr<NetObj> sourceObject = FindObjectById(incident->getSourceObjectId());
   if ((sourceObject == nullptr) || !sourceObject->checkAccessRights(userId, OBJECT_ACCESS_READ))
      return std::string("Access denied or source object not found");

   // Get responsible users for the object (including inherited from parents)
   unique_ptr<StructArray<ResponsibleUser>> responsibleUsers = sourceObject->getAllResponsibleUsers();

   json_t *result = json_object();

   if ((responsibleUsers != nullptr) && (responsibleUsers->size() > 0))
   {
      // Return the first responsible user as suggestion
      ResponsibleUser *ru = responsibleUsers->get(0);
      json_object_set_new(result, "suggestedUserId", json_integer(ru->userId));

      // Get user name
      unique_ptr<ObjectArray<UserDatabaseObject>> users = FindUserDBObjects(*responsibleUsers);
      if ((users != nullptr) && (users->size() > 0))
      {
         json_object_set_new(result, "suggestedUserName", json_string_t(users->get(0)->getName()));
      }

      json_object_set_new(result, "reason", json_string("Responsible user for source object"));
      json_object_set_new(result, "totalResponsibleUsers", json_integer(responsibleUsers->size()));
   }
   else
   {
      json_object_set_new(result, "suggestedUserId", json_integer(0));
      json_object_set_new(result, "reason", json_string("No responsible users configured for this object"));
   }

   return JsonToString(result);
}

/**
 * Get open (not closed) incidents for an object
 */
std::string F_GetOpenIncidents(json_t *arguments, uint32_t userId)
{
   const char *objectName = json_object_get_string_utf8(arguments, "object", nullptr);
   if ((objectName == nullptr) || (objectName[0] == 0))
      return std::string("Object name or ID must be provided");

   shared_ptr<NetObj> object = FindObjectByNameOrId(objectName);
   if ((object == nullptr) || !object->checkAccessRights(userId, OBJECT_ACCESS_MANAGE_INCIDENTS))
      return std::string("Object not found or access denied");

   return QueryObjectIncidents(object, true, 0);
}

/**
 * Create a new incident
 */
std::string F_CreateIncident(json_t *arguments, uint32_t userId)
{
   const char *title = json_object_get_string_utf8(arguments, "title", nullptr);
   if ((title == nullptr) || (title[0] == 0))
      return std::string("Incident title must be provided");

   const char *objectName = json_object_get_string_utf8(arguments, "source_object", nullptr);
   if ((objectName == nullptr) || (objectName[0] == 0))
      return std::string("Source object name or ID must be provided");

   shared_ptr<NetObj> object = FindObjectByNameOrId(objectName);
   if (object == nullptr)
      return std::string("Source object not found");

   if (!object->checkAccessRights(userId, OBJECT_ACCESS_MANAGE_INCIDENTS))
      return std::string("Access denied - insufficient permissions to manage incidents on this object");

   const char *initialComment = json_object_get_string_utf8(arguments, "initial_comment", nullptr);
   uint32_t sourceAlarmId = json_object_get_uint32(arguments, "source_alarm_id", 0);

   // Convert strings to TCHAR
   String titleStr(title, "utf-8");
   String commentStr = (initialComment != nullptr) ? String(initialComment, "utf-8") : String();

   uint32_t incidentId = 0;
   uint32_t rcc = CreateIncident(object->getId(), titleStr.cstr(),
      commentStr.isEmpty() ? nullptr : commentStr.cstr(), sourceAlarmId, userId, &incidentId);

   if (rcc != RCC_SUCCESS)
   {
      char buffer[128];
      if (rcc == RCC_ALARM_ALREADY_IN_INCIDENT)
         snprintf(buffer, 128, "Failed to create incident: alarm is already linked to another incident");
      else
         snprintf(buffer, 128, "Failed to create incident: error code %u", rcc);
      return std::string(buffer);
   }

   json_t *result = json_object();
   json_object_set_new(result, "success", json_true());
   json_object_set_new(result, "incidentId", json_integer(incidentId));
   json_object_set_new(result, "sourceObjectId", json_integer(object->getId()));
   json_object_set_new(result, "sourceObjectName", json_string_t(object->getName()));
   return JsonToString(result);
}

/**
 * Create an incident from multiple alarms
 */
std::string F_CreateIncidentFromAlarms(json_t *arguments, uint32_t userId)
{
   const char *title = json_object_get_string_utf8(arguments, "title", nullptr);
   if ((title == nullptr) || (title[0] == 0))
      return std::string("Incident title must be provided");

   json_t *alarmIds = json_object_get(arguments, "alarm_ids");
   if ((alarmIds == nullptr) || !json_is_array(alarmIds) || (json_array_size(alarmIds) == 0))
      return std::string("Alarm IDs array must be provided and non-empty");

   // Get first alarm to determine source object
   uint32_t firstAlarmId = static_cast<uint32_t>(json_integer_value(json_array_get(alarmIds, 0)));
   if (firstAlarmId == 0)
      return std::string("Invalid first alarm ID");

   Alarm *firstAlarm = FindAlarmById(firstAlarmId);
   if (firstAlarm == nullptr)
      return std::string("First alarm not found");

   uint32_t sourceObjectId = firstAlarm->getSourceObject();
   delete firstAlarm;

   shared_ptr<NetObj> sourceObject = FindObjectById(sourceObjectId);
   if (sourceObject == nullptr)
      return std::string("Source object for first alarm not found");

   if (!sourceObject->checkAccessRights(userId, OBJECT_ACCESS_MANAGE_INCIDENTS))
      return std::string("Access denied - insufficient permissions to manage incidents on source object");

   const char *initialComment = json_object_get_string_utf8(arguments, "initial_comment", nullptr);

   // Convert strings to TCHAR
   String titleStr(title, "utf-8");
   String commentStr = (initialComment != nullptr) ? String(initialComment, "utf-8") : String();

   // Create incident with first alarm
   uint32_t incidentId = 0;
   uint32_t rcc = CreateIncident(sourceObjectId, titleStr.cstr(),
      commentStr.isEmpty() ? nullptr : commentStr.cstr(), firstAlarmId, userId, &incidentId);

   if (rcc != RCC_SUCCESS)
   {
      char buffer[128];
      if (rcc == RCC_ALARM_ALREADY_IN_INCIDENT)
         snprintf(buffer, 128, "Failed to create incident: first alarm is already linked to another incident");
      else
         snprintf(buffer, 128, "Failed to create incident: error code %u", rcc);
      return std::string(buffer);
   }

   // Link remaining alarms
   int linkedCount = 1;  // First alarm already linked
   json_t *failures = json_array();

   for (size_t i = 1; i < json_array_size(alarmIds); i++)
   {
      uint32_t alarmId = static_cast<uint32_t>(json_integer_value(json_array_get(alarmIds, i)));
      if (alarmId == 0)
         continue;

      rcc = LinkAlarmToIncident(incidentId, alarmId, userId);
      if (rcc == RCC_SUCCESS)
      {
         linkedCount++;
      }
      else
      {
         json_t *failure = json_object();
         json_object_set_new(failure, "alarmId", json_integer(alarmId));
         json_object_set_new(failure, "errorCode", json_integer(rcc));
         if (rcc == RCC_ALARM_ALREADY_IN_INCIDENT)
            json_object_set_new(failure, "reason", json_string("Alarm already in another incident"));
         else
            json_object_set_new(failure, "reason", json_string("Failed to link"));
         json_array_append_new(failures, failure);
      }
   }

   json_t *result = json_object();
   json_object_set_new(result, "success", json_true());
   json_object_set_new(result, "incidentId", json_integer(incidentId));
   json_object_set_new(result, "sourceObjectId", json_integer(sourceObjectId));
   json_object_set_new(result, "sourceObjectName", json_string_t(sourceObject->getName()));
   json_object_set_new(result, "linkedAlarmCount", json_integer(linkedCount));
   if (json_array_size(failures) > 0)
      json_object_set_new(result, "linkFailures", failures);
   else
      json_decref(failures);
   return JsonToString(result);
}
