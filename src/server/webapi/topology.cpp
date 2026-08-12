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
** File: topology.cpp
**
**/

#include "webapi.h"
#include <netxms_maps.h>
#include <nms_topo.h>

/**
 * Serialize NetworkMapObjectList to JSON. Only objects the caller has READ access to are included;
 * links referencing a filtered-out object are dropped so that no metadata of inaccessible objects leaks.
 */
static json_t *TopologyToJson(const NetworkMapObjectList& topology, uint32_t userId)
{
   json_t *output = json_object();

   // Objects - include summary for each referenced object the caller is allowed to read
   json_t *objects = json_array();
   IntegerArray<uint32_t> accessibleObjects;
   const IntegerArray<uint32_t>& objectList = topology.getObjects();
   for (int i = 0; i < objectList.size(); i++)
   {
      shared_ptr<NetObj> object = FindObjectById(objectList.get(i));
      if ((object != nullptr) && object->checkAccessRights(userId, OBJECT_ACCESS_READ))
      {
         accessibleObjects.add(object->getId());
         json_array_append_new(objects, CreateObjectSummary(*object));
      }
   }
   json_object_set_new(output, "objects", objects);

   // Links - skip any link that references an object the caller cannot read
   json_t *links = json_array();
   const ObjectArray<ObjLink>& linkList = topology.getLinks();
   for (int i = 0; i < linkList.size(); i++)
   {
      ObjLink *l = linkList.get(i);
      if (!accessibleObjects.contains(l->object1) || !accessibleObjects.contains(l->object2))
         continue;
      json_t *link = json_object();
      json_object_set_new(link, "object1", json_integer(l->object1));
      json_object_set_new(link, "object2", json_integer(l->object2));
      json_object_set_new(link, "interface1", json_integer(l->iface1));
      json_object_set_new(link, "interface2", json_integer(l->iface2));
      json_object_set_new(link, "type", json_integer(l->type));
      json_object_set_new(link, "name", json_string_t(l->name));
      json_object_set_new(link, "port1", json_string_t(l->port1));
      json_object_set_new(link, "port2", json_string_t(l->port2));
      json_object_set_new(link, "flags", json_integer(l->flags));
      json_array_append_new(links, link);
   }
   json_object_set_new(output, "links", links);

   return output;
}

/**
 * Handler for GET /v1/objects/:object-id/topology/l2
 */
int H_TopologyL2(Context *context)
{
   uint32_t objectId = context->getPlaceholderValueAsUInt32(_T("object-id"));
   if (objectId == 0)
      return 400;

   shared_ptr<NetObj> object = FindObjectById(objectId);
   if (object == nullptr)
      return 404;

   if (!object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ))
      return 403;

   if (object->getObjectClass() != OBJECT_NODE)
   {
      context->setErrorResponse("Object is not a node");
      return 400;
   }

   int radius = context->getQueryParameterAsInt32("radius", 0);
   bool useL1Topology = context->getQueryParameterAsBoolean("useL1Topology", false);

   uint32_t rcc;
   shared_ptr<NetworkMapObjectList> topology = static_cast<Node&>(*object).getAndUpdateL2Topology(&rcc, radius, useL1Topology);
   if (topology == nullptr)
   {
      context->setErrorResponse("Unable to build L2 topology");
      return 500;
   }

   json_t *output = TopologyToJson(*topology, context->getUserId());
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for GET /v1/objects/:object-id/topology/ip
 */
int H_TopologyIP(Context *context)
{
   uint32_t objectId = context->getPlaceholderValueAsUInt32(_T("object-id"));
   if (objectId == 0)
      return 400;

   shared_ptr<NetObj> object = FindObjectById(objectId);
   if (object == nullptr)
      return 404;

   if (!object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ))
      return 403;

   if (object->getObjectClass() != OBJECT_NODE)
   {
      context->setErrorResponse("Object is not a node");
      return 400;
   }

   int radius = context->getQueryParameterAsInt32("radius", 0);

   unique_ptr<NetworkMapObjectList> topology = BuildIPTopology(static_pointer_cast<Node>(object), nullptr, radius, true);
   json_t *output = TopologyToJson(*topology, context->getUserId());
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for GET /v1/objects/:object-id/topology/ospf
 */
int H_TopologyOSPF(Context *context)
{
   uint32_t objectId = context->getPlaceholderValueAsUInt32(_T("object-id"));
   if (objectId == 0)
      return 400;

   shared_ptr<NetObj> object = FindObjectById(objectId);
   if (object == nullptr)
      return 404;

   if (!object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ))
      return 403;

   if (object->getObjectClass() != OBJECT_NODE)
   {
      context->setErrorResponse("Object is not a node");
      return 400;
   }

   unique_ptr<NetworkMapObjectList> topology = BuildOSPFTopology(static_pointer_cast<Node>(object), nullptr, -1);
   if (topology == nullptr)
   {
      context->setErrorResponse("Unable to build OSPF topology");
      return 500;
   }

   json_t *output = TopologyToJson(*topology, context->getUserId());
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for GET /v1/objects/:object-id/topology/internal
 */
int H_TopologyInternal(Context *context)
{
   uint32_t objectId = context->getPlaceholderValueAsUInt32(_T("object-id"));
   if (objectId == 0)
      return 400;

   shared_ptr<NetObj> object = FindObjectById(objectId);
   if (object == nullptr)
      return 404;

   if (!object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ))
      return 403;

   if (!object->isDataCollectionTarget())
   {
      context->setErrorResponse("Object is not a data collection target");
      return 400;
   }

   shared_ptr<NetworkMapObjectList> topology = static_cast<DataCollectionTarget&>(*object).buildInternalCommunicationTopology();
   if (topology == nullptr)
   {
      context->setErrorResponse("Unable to build internal communication topology");
      return 500;
   }

   json_t *output = TopologyToJson(*topology, context->getUserId());
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Load node object for a node-scoped table request. Sets *httpCode and returns nullptr on failure.
 */
static shared_ptr<Node> LoadNodeForTableRequest(Context *context, int *httpCode)
{
   uint32_t objectId = context->getPlaceholderValueAsUInt32(_T("object-id"));
   if (objectId == 0)
   {
      *httpCode = 400;
      return shared_ptr<Node>();
   }

   shared_ptr<NetObj> object = FindObjectById(objectId);
   if (object == nullptr)
   {
      *httpCode = 404;
      return shared_ptr<Node>();
   }

   if (!object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ))
   {
      *httpCode = 403;
      return shared_ptr<Node>();
   }

   if (object->getObjectClass() != OBJECT_NODE)
   {
      context->setErrorResponse("Object is not a node");
      *httpCode = 400;
      return shared_ptr<Node>();
   }

   return static_pointer_cast<Node>(object);
}

/**
 * Resolve NIC vendor from OUI database. Returns JSON null if unknown.
 */
static json_t *VendorToJson(const MacAddress& macAddr)
{
   const wchar_t *vendor = FindVendorByMac(macAddr);
   return (vendor != nullptr) ? json_string_t(vendor) : json_null();
}

/**
 * Resolve interface name by index. Returns JSON null if there is no matching interface object.
 * When preferParent is set, an interface that is a member of an Ethernet or LAG parent interface
 * is reported under the parent's name (same rule as ForwardingDatabase::interfaceIndexToName).
 */
static json_t *InterfaceNameToJson(const Node& node, uint32_t ifIndex, bool preferParent)
{
   shared_ptr<Interface> iface = node.findInterfaceByIndex(ifIndex);
   if (iface == nullptr)
      return json_null();

   if (preferParent && (iface->getParentInterfaceId() != 0))
   {
      shared_ptr<Interface> parent = static_pointer_cast<Interface>(FindObjectById(iface->getParentInterfaceId(), OBJECT_INTERFACE));
      if ((parent != nullptr) &&
          ((parent->getIfType() == IFTYPE_ETHERNET_CSMACD) || (parent->getIfType() == IFTYPE_IEEE8023ADLAG)))
         return json_string_t(parent->getName());
   }

   return json_string_t(iface->getName());
}

/**
 * Add "nodeId" and "nodeName" fields for a referenced node, honouring the caller's access rights.
 * Unknown or inaccessible nodes are reported as id 0 with a null name.
 */
static void SetNodeReference(json_t *entry, const shared_ptr<Node>& node, uint32_t userId)
{
   if ((node != nullptr) && node->checkAccessRights(userId, OBJECT_ACCESS_READ))
   {
      json_object_set_new(entry, "nodeId", json_integer(node->getId()));
      json_object_set_new(entry, "nodeName", json_string_t(node->getName()));
   }
   else
   {
      json_object_set_new(entry, "nodeId", json_integer(0));
      json_object_set_new(entry, "nodeName", json_null());
   }
}

/**
 * Textual representation of IP routing protocol (RFC1213 ipRouteProto)
 */
static const char *RoutingProtocolToText(RoutingProtocol protocol)
{
   switch(protocol)
   {
      case ROUTING_PROTOCOL_OTHER:
         return "Other";
      case ROUTING_PROTOCOL_LOCAL:
         return "Local";
      case ROUTING_PROTOCOL_NETMGMT:
         return "Network Management";
      case ROUTING_PROTOCOL_ICMP:
         return "ICMP";
      case ROUTING_PROTOCOL_EGP:
         return "EGP";
      case ROUTING_PROTOCOL_GGP:
         return "GGP";
      case ROUTING_PROTOCOL_HELLO:
         return "HELLO";
      case ROUTING_PROTOCOL_RIP:
         return "RIP";
      case ROUTING_PROTOCOL_IS_IS:
         return "IS-IS";
      case ROUTING_PROTOCOL_ES_IS:
         return "ES-IS";
      case ROUTING_PROTOCOL_IGRP:
         return "IGRP";
      case ROUTING_PROTOCOL_BBN_SPF_IGP:
         return "BBN SPF IGP";
      case ROUTING_PROTOCOL_OSPF:
         return "OSPF";
      case ROUTING_PROTOCOL_BGP:
         return "BGP";
      case ROUTING_PROTOCOL_IDPR:
         return "IDPR";
      case ROUTING_PROTOCOL_EIGRP:
         return "EIGRP";
      case ROUTING_PROTOCOL_DVMRP:
         return "DVMRP";
      case ROUTING_PROTOCOL_RPL:
         return "RPL";
      case ROUTING_PROTOCOL_DHCP:
         return "DHCP";
      default:
         return "Unknown";
   }
}

/**
 * Textual representation of IP route type (RFC1213 ipRouteType)
 */
static const char *RouteTypeToText(uint32_t type)
{
   switch(type)
   {
      case 1:
         return "Other";
      case 2:
         return "Invalid";
      case 3:
         return "Direct";
      case 4:
         return "Indirect";
      default:
         return "Unknown";
   }
}

/**
 * Textual representation of switch forwarding database entry type (dot1dTpFdbStatus)
 */
static const char *FdbEntryTypeToText(uint16_t type)
{
   switch(type)
   {
      case 3:
         return "Dynamic";
      case 5:
         return "Static";
      case 6:
         return "Secure";
      default:
         return "Unknown";
   }
}

/**
 * Handler for GET /v1/objects/:object-id/arp-cache
 */
int H_ArpCache(Context *context)
{
   int httpCode;
   shared_ptr<Node> node = LoadNodeForTableRequest(context, &httpCode);
   if (node == nullptr)
      return httpCode;

   json_t *entries = json_array();
   shared_ptr<ArpCache> arpCache = node->getArpCache(context->getQueryParameterAsBoolean("forceRead", false));
   for(int i = 0; (arpCache != nullptr) && (i < arpCache->size()); i++)
   {
      const ArpEntry *e = arpCache->get(i);

      json_t *entry = json_object();
      json_object_set_new(entry, "ipAddress", e->ipAddr.toJson());
      json_object_set_new(entry, "macAddress", json_string_t(e->macAddr.toString()));
      json_object_set_new(entry, "vendor", VendorToJson(e->macAddr));
      json_object_set_new(entry, "interfaceIndex", json_integer(e->ifIndex));
      json_object_set_new(entry, "interfaceName", InterfaceNameToJson(*node, e->ifIndex, false));
      SetNodeReference(entry, FindNodeByIP(node->getZoneUIN(), e->ipAddr), context->getUserId());
      json_array_append_new(entries, entry);
   }

   json_t *output = json_object();
   json_object_set_new(output, "timestamp", (arpCache != nullptr) ? json_time_string(arpCache->timestamp()) : json_null());
   json_object_set_new(output, "entries", entries);
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for GET /v1/objects/:object-id/routing-table
 */
int H_RoutingTable(Context *context)
{
   int httpCode;
   shared_ptr<Node> node = LoadNodeForTableRequest(context, &httpCode);
   if (node == nullptr)
      return httpCode;

   json_t *entries = json_array();
   shared_ptr<RoutingTable> routingTable = node->getRoutingTable();
   for(int i = 0; (routingTable != nullptr) && (i < routingTable->size()); i++)
   {
      const ROUTE *route = routingTable->get(i);

      json_t *entry = json_object();
      json_object_set_new(entry, "destination", route->destination.toJson());
      json_object_set_new(entry, "nextHop", route->nextHop.toJson());
      json_object_set_new(entry, "interfaceIndex", json_integer(route->ifIndex));
      json_object_set_new(entry, "interfaceName", InterfaceNameToJson(*node, route->ifIndex, false));
      json_object_set_new(entry, "type", json_integer(route->routeType));
      json_object_set_new(entry, "typeText", json_string(RouteTypeToText(route->routeType)));
      json_object_set_new(entry, "metric", json_integer(route->metric));
      json_object_set_new(entry, "protocol", json_integer(route->protocol));
      json_object_set_new(entry, "protocolText", json_string(RoutingProtocolToText(route->protocol)));
      json_array_append_new(entries, entry);
   }

   json_t *output = json_object();
   json_object_set_new(output, "timestamp", (routingTable != nullptr) ? json_time_string(routingTable->timestamp()) : json_null());
   json_object_set_new(output, "entries", entries);
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for GET /v1/objects/:object-id/switch-forwarding-database
 */
int H_SwitchForwardingDatabase(Context *context)
{
   int httpCode;
   shared_ptr<Node> node = LoadNodeForTableRequest(context, &httpCode);
   if (node == nullptr)
      return httpCode;

   json_t *entries = json_array();
   shared_ptr<ForwardingDatabase> fdb = node->getSwitchForwardingDatabase();
   for(int i = 0; (fdb != nullptr) && (i < fdb->getSize()); i++)
   {
      const ForwardingDatabaseEntry *e = fdb->getEntry(i);

      json_t *entry = json_object();
      json_object_set_new(entry, "macAddress", json_string_t(e->macAddr.toString()));
      json_object_set_new(entry, "vendor", VendorToJson(e->macAddr));
      json_object_set_new(entry, "bridgePort", json_integer(e->bridgePort));
      json_object_set_new(entry, "interfaceIndex", json_integer(e->ifIndex));
      json_object_set_new(entry, "interfaceName", InterfaceNameToJson(*node, e->ifIndex, true));
      json_object_set_new(entry, "vlanId", json_integer(e->vlanId));
      SetNodeReference(entry, static_pointer_cast<Node>(FindObjectById(e->nodeObject, OBJECT_NODE)), context->getUserId());
      json_object_set_new(entry, "type", json_integer(e->type));
      json_object_set_new(entry, "typeText", json_string(FdbEntryTypeToText(e->type)));
      json_array_append_new(entries, entry);
   }

   json_t *output = json_object();
   json_object_set_new(output, "timestamp", (fdb != nullptr) ? json_time_string(fdb->getTimeStamp()) : json_null());
   json_object_set_new(output, "entries", entries);
   context->setResponseData(output);
   json_decref(output);
   return 200;
}
