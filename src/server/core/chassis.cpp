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
 * Update rack binding
 */
void Chassis::updateRackBinding()
{
   bool rackFound = false;
   SharedObjectArray<NetObj> deleteList;

   writeLockParentList();
   for(int i = 0; i < getParentList().size(); i++)
   {
      NetObj *object = getParentList().get(i);
      if (object->getObjectClass() != OBJECT_RACK)
         continue;
      if (object->getId() == m_rackId)
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

   if (!rackFound && (m_rackId != 0))
   {
      shared_ptr<Rack> rack = static_pointer_cast<Rack>(FindObjectById(m_rackId, OBJECT_RACK));
      if (rack != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG_OBJECT_RELATIONS, 5, _T("Chassis::updateRackBinding(%s [%u]): add rack binding %s [%d]"), m_name, m_id, rack->getName(), rack->getId());
         linkObjects(rack, self());
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_OBJECT_RELATIONS, 5, _T("Chassis::updateRackBinding(%s [%u]): rack object [%d] not found"), m_name, m_id, m_rackId);
      }
   }
}

/**
 * Update controller binding
 */
void Chassis::updateControllerBinding()
{
   bool controllerFound = false;

   IntegerArray<uint32_t> unbindList;
   readLockParentList();
   for(int i = 0; i < getParentList().size(); i++)
   {
      NetObj *object = getParentList().get(i);
      if (object->getObjectClass() != OBJECT_NODE)
         continue;
      if ((m_flags & CHF_BIND_UNDER_CONTROLLER) && (object->getId() == m_controllerId))
      {
         controllerFound = true;
      }
      else
      {
         unbindList.add(object->getId());
      }
   }
   unlockParentList();

   if ((m_flags & CHF_BIND_UNDER_CONTROLLER) && !controllerFound)
   {
      shared_ptr<NetObj> controller = FindObjectById(m_controllerId, OBJECT_NODE);
      if (controller != nullptr)
      {
         linkObjects(controller, self());
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_OBJECT_RELATIONS, 4, _T("Chassis::updateControllerBinding(%s [%u]): controller node with ID %u not found"), m_name, m_id, m_controllerId);
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
      ThreadPoolExecute(g_mainThreadPool, this, &Chassis::updateRackBinding);
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
 * Update object flags
 */
void Chassis::updateFlags(uint32_t flags, uint32_t mask)
{
   super::updateFlags(flags, mask);

   if (mask & CHF_BIND_UNDER_CONTROLLER)
   {
      ThreadPoolExecute(g_mainThreadPool, this, &Chassis::updateControllerBinding);
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
   if ((dco->getDataSource() != DS_INTERNAL) && (dco->getDataSource() != DS_PUSH_AGENT) && (dco->getDataSource() != DS_SCRIPT))
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
   updateControllerBinding();
}

/**
 * Serialize object to JSON
 */
json_t *Chassis::toJson()
{
   json_t *root = super::toJson();

   lockProperties();
   json_object_set_new(root, "controllerId", json_integer(m_controllerId));
   json_object_set_new(root, "rackHeight", json_integer(m_rackHeight));
   json_object_set_new(root, "rackPosition", json_integer(m_rackPosition));
   json_object_set_new(root, "rackOrientation", json_integer(m_rackPosition));
   json_object_set_new(root, "rackId", json_integer(m_rackId));
   json_object_set_new(root, "rackImageFront", m_rackImageFront.toJson());
   json_object_set_new(root, "rackImageRear", m_rackImageRear.toJson());
   unlockProperties();

   return root;
}
