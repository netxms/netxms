/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
** File: chassis.cpp
**
**/

#include "nxcore.h"

/**
 * Default constructor
 */
Chassis::Chassis() : super(Pollable::NONE)
{
   m_controllerId = 0;
   m_rackId = 0;
   m_rackPosition = 0;
   m_rackHeight = 1;
   m_rackOrientation = FILL;
}

/**
 * Create new chassis object
 */
Chassis::Chassis(const TCHAR *name, uint32_t controllerId) : super(name, Pollable::NONE)
{
   m_controllerId = controllerId;
   m_rackId = 0;
   m_rackPosition = 0;
   m_rackHeight = 1;
   m_rackOrientation = FILL;
}

/**
 * Destructor
 */
Chassis::~Chassis()
{
}

/**
 * Called by client session handler to check if threshold summary should be shown for this object.
 */
bool Chassis::showThresholdSummary() const
{
   return true;
}

/**
 * Update rack binding. Runtime callers must queue this through PHYSICAL_BINDING_TASK_KEY.
 */
void Chassis::updateRackBinding()
{
   lockProperties();
   uint32_t rackId = m_rackId;
   unlockProperties();

   bool rackFound = false;
   SharedObjectArray<NetObj> deleteList;

   readLockParentList();
   for(int i = 0; i < getParentList().size(); i++)
   {
      NetObj *object = getParentList().get(i);
      if (object->getObjectClass() != OBJECT_RACK)
         continue;
      if (object->getId() == rackId)
      {
         rackFound = true;
         continue;
      }
      deleteList.add(getParentList().getShared(i));
   }
   unlockParentList();

   for(int n = 0; n < deleteList.size(); n++)
   {
      NetObj *rack = deleteList.get(n);
      nxlog_debug_tag(DEBUG_TAG_OBJECT_RELATIONS, 5, _T("Chassis::updateRackBinding(%s [%u]): delete incorrect rack binding %s [%d]"), m_name, m_id, rack->getName(), rack->getId());
      unlinkObjects(rack, this);
   }

   if (!rackFound && (rackId != 0))
   {
      shared_ptr<Rack> rack = static_pointer_cast<Rack>(FindObjectById(rackId, OBJECT_RACK));
      if (rack != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG_OBJECT_RELATIONS, 5, _T("Chassis::updateRackBinding(%s [%u]): add rack binding %s [%d]"), m_name, m_id, rack->getName(), rack->getId());
         linkObjects(rack, self());
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_OBJECT_RELATIONS, 5, _T("Chassis::updateRackBinding(%s [%u]): rack object [%d] not found"), m_name, m_id, rackId);
      }
   }
}

/**
 * Update controller binding. Runtime callers must queue this through PHYSICAL_BINDING_TASK_KEY.
 */
void Chassis::updateControllerBinding()
{
   lockProperties();
   uint32_t controllerId = m_controllerId;
   bool bindUnderController = (m_flags & CHF_BIND_UNDER_CONTROLLER) != 0;
   unlockProperties();

   bool controllerFound = false;

   IntegerArray<uint32_t> unbindList;
   readLockParentList();
   for(int i = 0; i < getParentList().size(); i++)
   {
      NetObj *object = getParentList().get(i);
      if (object->getObjectClass() != OBJECT_NODE)
         continue;
      if (bindUnderController && (object->getId() == controllerId))
      {
         controllerFound = true;
      }
      else
      {
         unbindList.add(object->getId());
      }
   }
   unlockParentList();

   if (bindUnderController && !controllerFound)
   {
      shared_ptr<NetObj> controller = FindObjectById(controllerId, OBJECT_NODE);
      if (controller == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG_OBJECT_RELATIONS, 4, _T("Chassis::updateControllerBinding(%s [%u]): controller node with ID %u not found"), m_name, m_id, controllerId);
      }
      else if (isChild(controllerId))
      {
         // Re-checked here because the front door check ran earlier: a placement request accepted
         // in between can make this chassis a parent of the controller, and linking now would close a cycle
         nxlog_debug_tag(DEBUG_TAG_OBJECT_RELATIONS, 5, _T("Chassis::updateControllerBinding(%s [%u]): controller node %s [%u] is a descendant, binding skipped"), m_name, m_id, controller->getName(), controllerId);
      }
      else
      {
         linkObjects(controller, self());
      }
   }

   for(int i = 0; i < unbindList.size(); i++)
   {
      shared_ptr<NetObj> object = FindObjectById(unbindList.get(i));
      if (object != nullptr)
      {
         unlinkObjects(object.get(), this);
      }
   }
}

/**
 * Create NXCP message with object's data
 */
void Chassis::fillMessageLocked(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageLocked(msg, userId);
   msg->setField(VID_CONTROLLER_ID, m_controllerId);
   msg->setField(VID_PHYSICAL_CONTAINER_ID, m_rackId);
   msg->setField(VID_RACK_IMAGE_FRONT, m_rackImageFront);
   msg->setField(VID_RACK_IMAGE_REAR, m_rackImageRear);
   msg->setField(VID_RACK_POSITION, m_rackPosition);
   msg->setField(VID_RACK_HEIGHT, m_rackHeight);
   msg->setField(VID_RACK_ORIENTATION, static_cast<INT16>(m_rackOrientation));
}

/**
 * Modify object from NXCP message
 */
uint32_t Chassis::modifyFromMessageInternal(const NXCPMessage& msg, ClientSession *session)
{
   if (msg.isFieldExist(VID_CONTROLLER_ID))
      m_controllerId = msg.getFieldAsUInt32(VID_CONTROLLER_ID);
   if (msg.isFieldExist(VID_PHYSICAL_CONTAINER_ID))
   {
      m_rackId = msg.getFieldAsUInt32(VID_PHYSICAL_CONTAINER_ID);
      ThreadPoolExecuteSerialized(g_mainThreadPool, PHYSICAL_BINDING_TASK_KEY, this, &Chassis::updateRackBinding);
   }
   if (msg.isFieldExist(VID_RACK_IMAGE_FRONT))
      m_rackImageFront = msg.getFieldAsGUID(VID_RACK_IMAGE_FRONT);
   if (msg.isFieldExist(VID_RACK_IMAGE_REAR))
      m_rackImageRear = msg.getFieldAsGUID(VID_RACK_IMAGE_REAR);
   if (msg.isFieldExist(VID_RACK_POSITION))
      m_rackPosition = msg.getFieldAsInt16(VID_RACK_POSITION);
   if (msg.isFieldExist(VID_RACK_HEIGHT))
      m_rackHeight = msg.getFieldAsInt16(VID_RACK_HEIGHT);
   if (msg.isFieldExist(VID_RACK_ORIENTATION))
      m_rackOrientation = static_cast<RackOrientation>(msg.getFieldAsInt16(VID_RACK_ORIENTATION));

   return super::modifyFromMessageInternal(msg, session);
}

/**
 * Modify object from JSON document
 */
uint32_t Chassis::modifyFromJSONInternal(json_t *json, GenericClientSession *session)
{
   // Stage into a local so a subsequent rejection of the placement group leaves m_controllerId untouched
   uint32_t controllerId = m_controllerId;
   if (!json_object_update_integer(json, "controllerId", &controllerId))
      return RCC_INVALID_ARGUMENT;
   // Validate only when the document carries the key: a stored id may legitimately be stale or point
   // at a node placed inside this chassis, and such an object must stay patchable
   if ((json_object_get(json, "controllerId") != nullptr) && (controllerId != 0))
   {
      if (FindObjectById(controllerId, OBJECT_NODE) == nullptr)
         return RCC_INVALID_OBJECT_ID;

      // Loop check is done even when CHF_BIND_UNDER_CONTROLLER is clear, because setting the flag
      // later links without further validation. isChild takes child list locks, so drop property lock around it.
      unlockProperties();
      bool loop = isChild(controllerId);
      lockProperties();
      if (loop)
         return RCC_OBJECT_LOOP;
   }

   json_t *physicalPlacement = json_object_get(json, "physicalPlacement");
   if (physicalPlacement != nullptr)
   {
      // Stage into locals and drop property lock for the call (see ModifyPhysicalPlacementFromJson)
      uint32_t rackId = m_rackId;
      int16_t position = m_rackPosition;
      int16_t height = m_rackHeight;
      RackOrientation orientation = m_rackOrientation;
      uuid imageFront = m_rackImageFront;
      uuid imageRear = m_rackImageRear;
      PhysicalPlacementRef placementRef = { &rackId, &position, &height, &orientation, &imageFront, &imageRear };
      unlockProperties();
      // A chassis can only be placed in a rack, never inside another chassis
      uint32_t rcc = ModifyPhysicalPlacementFromJson(physicalPlacement, this, placementRef, false);
      lockProperties();
      if (rcc != RCC_SUCCESS)
         return rcc;
      m_rackId = rackId;
      m_rackPosition = position;
      m_rackHeight = height;
      m_rackOrientation = orientation;
      m_rackImageFront = imageFront;
      m_rackImageRear = imageRear;

      // Rebinds are scheduled at the point the id is committed, not after the result code:
      // the caller marks the object as modified regardless of the result, so a committed id reaches
      // the database even when a later property is rejected, and a deferred rebind would be skipped
      if (json_object_get(physicalPlacement, "containerId") != nullptr)
         ThreadPoolExecuteSerialized(g_mainThreadPool, PHYSICAL_BINDING_TASK_KEY, this, &Chassis::updateRackBinding);
   }

   m_controllerId = controllerId;
   if (json_object_get(json, "controllerId") != nullptr)
      ThreadPoolExecuteSerialized(g_mainThreadPool, PHYSICAL_BINDING_TASK_KEY, this, &Chassis::updateControllerBinding);

   return super::modifyFromJSONInternal(json, session);
}

/**
 * Update object flags
 */
void Chassis::updateFlags(uint32_t flags, uint32_t mask)
{
   super::updateFlags(flags, mask);

   if (mask & CHF_BIND_UNDER_CONTROLLER)
   {
      ThreadPoolExecuteSerialized(g_mainThreadPool, PHYSICAL_BINDING_TASK_KEY, this, &Chassis::updateControllerBinding);
   }
}

/**
 * Save to database
 */
bool Chassis::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);
   if (success && (m_modified & MODIFY_OTHER))
   {
      static const TCHAR *columns[] = { _T("controller_id"), _T("rack_id"), _T("rack_image_front"), _T("rack_image_rear"), _T("rack_position"), _T("rack_height"), _T("rack_orientation"), nullptr };
      DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("chassis"), _T("id"), m_id, columns);
      if (hStmt != nullptr)
      {
         lockProperties();
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_controllerId);
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_rackId);
         DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_rackImageFront);
         DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_rackImageRear);
         DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, m_rackPosition);
         DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_rackHeight);
         DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_rackOrientation);
         DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_id);
         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);
         unlockProperties();
      }
      else
      {
         success = false;
      }
   }
   return success;
}

/**
 * Delete from database
 */
bool Chassis::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if (success)
   {
      success = executeQueryOnObject(hdb, _T("DELETE FROM chassis WHERE id=?"));
   }
   return success;
}

/**
 * Load from database
 */
bool Chassis::loadFromDatabase(DB_HANDLE hdb, uint32_t id, DB_STATEMENT *preparedStatements)
{
   m_id = id;

   if (!loadCommonProperties(hdb, preparedStatements) || !super::loadFromDatabase(hdb, id, preparedStatements))
      return false;

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT controller_id,rack_id,rack_image_front,rack_position,rack_height,rack_orientation,rack_image_rear FROM chassis WHERE id=?"));
   if (hStmt == nullptr)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult == nullptr)
   {
      DBFreeStatement(hStmt);
      return false;
   }

   m_controllerId = DBGetFieldULong(hResult, 0, 0);
   m_rackId = DBGetFieldULong(hResult, 0, 1);
   m_rackImageFront = DBGetFieldGUID(hResult, 0, 2);
   m_rackPosition = DBGetFieldULong(hResult, 0, 3);
   m_rackHeight = DBGetFieldULong(hResult, 0, 4);
   m_rackOrientation = static_cast<RackOrientation>(DBGetFieldLong(hResult, 0, 5));
   m_rackImageRear = DBGetFieldGUID(hResult, 0, 6);

   DBFreeResult(hResult);
   DBFreeStatement(hStmt);

   loadACLFromDB(hdb, preparedStatements);
   loadItemsFromDB(hdb, preparedStatements);
   for(int i = 0; i < m_dcObjects.size(); i++)
      if (!m_dcObjects.get(i)->loadThresholdsFromDB(hdb, preparedStatements))
         return false;
   loadDCIListForCleanup(hdb);

   updateRackBinding();
   return true;
}

/**
 * Post-load hook
 */
void Chassis::postLoad()
{
   super::postLoad();
   updateControllerBinding();
}

/**
 * Handler for object deletion
 */
void Chassis::onObjectDelete(const NetObj& object)
{
   lockProperties();

   if (object.getId() == m_controllerId)
   {
      m_controllerId = 0;
      setModified(MODIFY_OTHER);
      nxlog_debug_tag(DEBUG_TAG_OBJECT_RELATIONS, 3, _T("Chassis::onObjectDelete(%s [%u]): controller node %s [%u] deleted"), m_name, m_id, object.getName(), object.getId());
   }

   unlockProperties();

   super::onObjectDelete(object);
}

/**
 * Called when data collection configuration changed
 */
void Chassis::onDataCollectionChange()
{
   shared_ptr<Node> controller = static_pointer_cast<Node>(FindObjectById(m_controllerId, OBJECT_NODE));
   if (controller == nullptr)
   {
      nxlog_debug(4, _T("Chassis::onDataCollectionChange(%s [%d]): cannot find controller node object with id %d"), m_name, m_id, m_controllerId);
      return;
   }
   controller->relatedNodeDataCollectionChanged();
}

/**
 * Collect info for SNMP proxy and DCI source (proxy) nodes
 */
void Chassis::collectProxyInfo(ProxyInfo *info)
{
   if (m_status == STATUS_UNMANAGED)
      return;

   shared_ptr<Node> controller = static_pointer_cast<Node>(FindObjectById(m_controllerId, OBJECT_NODE));
   if (controller == nullptr)
   {
      nxlog_debug(4, _T("Chassis::collectProxyInfo(%s [%u]): cannot find controller node object with id %u"), m_name, m_id, m_controllerId);
      return;
   }

   uint32_t primarySnmpProxy = controller->getEffectiveSnmpProxy(ProxySelection::PRIMARY);
   bool snmpProxy = (primarySnmpProxy == info->proxyId);
   bool backupSnmpProxy = (controller->getEffectiveSnmpProxy(ProxySelection::BACKUP) == info->proxyId);
   bool isTarget = false;

   readLockDciAccess();
   for(int i = 0; i < m_dcObjects.size(); i++)
   {
      DCObject *dco = m_dcObjects.get(i);
      if (dco->getStatus() == ITEM_STATUS_DISABLED)
         continue;

      if ((((snmpProxy || backupSnmpProxy) && (dco->getDataSource() == DS_SNMP_AGENT) && (dco->getSourceNode() == 0)) ||
           ((dco->getDataSource() == DS_NATIVE_AGENT) && (dco->getSourceNode() == info->proxyId))) &&
          dco->hasValue() && (dco->getAgentCacheMode() == AGENT_CACHE_ON))
      {
         addProxyDataCollectionElement(info, dco, backupSnmpProxy && (dco->getDataSource() == DS_SNMP_AGENT) ? primarySnmpProxy : 0);
         if (dco->getDataSource() == DS_SNMP_AGENT)
            isTarget = true;
      }
   }
   unlockDciAccess();

   if (isTarget)
      addProxySnmpTarget(info, controller.get());
}

/**
 * Create NXSL object for this object
 */
NXSL_Value *Chassis::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(vm->createObject(&g_nxslChassisClass, new shared_ptr<Chassis>(self())));
}

/**
 * Get effective source node for given data collection object
 */
uint32_t Chassis::getEffectiveSourceNode(DCObject *dco)
{
   if (dco->getSourceNode() != 0)
      return dco->getSourceNode();
   if ((dco->getDataSource() != DS_INTERNAL) && (dco->getDataSource() != DS_PUSH_AGENT) && (dco->getDataSource() != DS_OTLP) && (dco->getDataSource() != DS_COMPUTED) && (dco->getDataSource() != DS_SCRIPT))
      return m_controllerId;
   return 0;
}

/**
 * Update controller binding flag
 */
void Chassis::setBindUnderController(bool doBind)
{
   lockProperties();
   if (doBind)
      m_flags |= CHF_BIND_UNDER_CONTROLLER;
   else
      m_flags &= ~CHF_BIND_UNDER_CONTROLLER;
   setModified(MODIFY_COMMON_PROPERTIES, false);
   unlockProperties();
   ThreadPoolExecuteSerialized(g_mainThreadPool, PHYSICAL_BINDING_TASK_KEY, this, &Chassis::updateControllerBinding);
}

/**
 * Serialize object to JSON
 */
json_t *Chassis::toJson(bool includeSensitiveData)
{
   json_t *root = super::toJson(includeSensitiveData);

   lockProperties();
   json_object_set_new(root, "controllerId", json_integer(m_controllerId));
   json_object_set_new(root, "rackHeight", json_integer(m_rackHeight));
   json_object_set_new(root, "rackPosition", json_integer(m_rackPosition));
   json_object_set_new(root, "rackOrientation", json_integer(m_rackOrientation));
   json_object_set_new(root, "rackId", json_integer(m_rackId));
   json_object_set_new(root, "rackImageFront", m_rackImageFront.toJson());
   json_object_set_new(root, "rackImageRear", m_rackImageRear.toJson());

   // Placement group is always emitted so a client never has to tell "absent" from "unplaced"
   PhysicalPlacementRef placementRef = { &m_rackId, &m_rackPosition, &m_rackHeight, &m_rackOrientation, &m_rackImageFront, &m_rackImageRear };
   json_object_set_new(root, "physicalPlacement", PhysicalPlacementToJson(placementRef));
   unlockProperties();

   return root;
}

/**
 * Fill rack placement information into provided JSON object.
 * Returns rack position (>= 1 if placed in a rack), 0 otherwise.
 */
int Chassis::getRackPlacement(json_t *element) const
{
   lockProperties();
   int position = m_rackPosition;
   json_object_set_new(element, "rackPosition", json_integer(m_rackPosition));
   json_object_set_new(element, "rackHeight", json_integer(m_rackHeight));
   json_object_set_new(element, "rackOrientation", json_integer(m_rackOrientation));
   json_object_set_new(element, "rackImageFront", m_rackImageFront.toJson());
   json_object_set_new(element, "rackImageRear", m_rackImageRear.toJson());
   unlockProperties();
   return position;
}

/**
 * Build compact chassis layout: chassis metadata and placed component objects
 * (nodes) with their placement geometry. Components are filtered by the
 * requesting user's read access and only those carrying a placement are returned.
 */
json_t *Chassis::getChassisLayout(uint32_t userId)
{
   json_t *root = json_object();

   lockProperties();
   json_object_set_new(root, "chassisId", json_integer(m_id));
   json_object_set_new(root, "name", json_string_t(m_name));
   json_object_set_new(root, "controllerId", json_integer(m_controllerId));
   json_object_set_new(root, "status", json_integer(m_status));
   unlockProperties();

   json_t *objects = json_array();
   readLockChildList();
   const SharedObjectArray<NetObj>& children = getChildList();
   for(int i = 0; i < children.size(); i++)
   {
      NetObj *child = children.get(i);
      if (!child->checkAccessRights(userId, OBJECT_ACCESS_READ))
         continue;

      json_t *placement = child->getChassisPlacement();
      if (placement == nullptr)
         continue;

      json_t *element = json_object();
      json_object_set_new(element, "id", json_integer(child->getId()));
      json_object_set_new(element, "objectClass", json_integer(child->getObjectClass()));
      json_object_set_new(element, "name", json_string_t(child->getName()));
      json_object_set_new(element, "status", json_integer(child->getStatus()));
      json_object_set_new(element, "placement", placement);
      json_array_append_new(objects, element);
   }
   unlockChildList();
   json_object_set_new(root, "objects", objects);

   return root;
}
