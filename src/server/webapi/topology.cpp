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
