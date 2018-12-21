/*
** NetXMS - Network Management System
** Copyright (C) 2003-2018 Victor Kirhenshtein
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
Chassis::Chassis() : super()
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
Chassis::Chassis(const TCHAR *name, UINT32 controllerId) : super(name)
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
bool Chassis::showThresholdSummary()
{
   return true;
}

/**
 * Update rack binding
 */
void Chassis::updateRackBinding()
{
   bool rackFound = false;
   ObjectArray<NetObj> deleteList(16, 16, false);

   lockParentList(true);
   for(int i = 0; i < m_parentList->size(); i++)
   {
      NetObj *object = m_parentList->get(i);
      if (object->getObjectClass() != OBJECT_RACK)
         continue;
      if (object->getId() == m_rackId)
      {
         rackFound = true;
         continue;
      }
      object->incRefCount();
      deleteList.add(object);
   }
   unlockParentList();

   for(int n = 0; n < deleteList.size(); n++)
   {
      NetObj *rack = deleteList.get(n);
      DbgPrintf(5, _T("Chassis::updateRackBinding(%s [%d]): delete incorrect rack binding %s [%d]"), m_name, m_id, rack->getName(), rack->getId());
      rack->deleteChild(this);
      deleteParent(rack);
      rack->decRefCount();
   }

   if (!rackFound && (m_rackId != 0))
   {
      Rack *rack = (Rack *)FindObjectById(m_rackId, OBJECT_RACK);
      if (rack != NULL)
      {
         DbgPrintf(5, _T("Chassis::updateRackBinding(%s [%d]): add rack binding %s [%d]"), m_name, m_id, rack->getName(), rack->getId());
         rack->addChild(this);
         addParent(rack);
      }
      else
      {
         DbgPrintf(5, _T("Chassis::updateRackBinding(%s [%d]): rack object [%d] not found"), m_name, m_id, m_rackId);
      }
   }
}

/**
 * Update controller binding
 */
void Chassis::updateControllerBinding()
{
   bool controllerFound = false;

   lockParentList(true);
   for(int i = 0; i < m_parentList->size(); i++)
   {
      NetObj *object = m_parentList->get(i);
      if (object->getId() == m_controllerId)
      {
         controllerFound = true;
         break;
      }
   }
   unlockParentList();

   if ((m_flags & CHF_BIND_UNDER_CONTROLLER) && !controllerFound)
   {
      NetObj *controller = FindObjectById(m_controllerId);
      if (controller != NULL)
      {
         controller->addChild(this);
         addParent(controller);
      }
      else
      {
         nxlog_debug(4, _T("Chassis::updateControllerBinding(%s [%d]): controller object with ID %d not found"), m_name, m_id, m_controllerId);
      }
   }
   else if (!(m_flags & CHF_BIND_UNDER_CONTROLLER) && controllerFound)
   {
      NetObj *controller = FindObjectById(m_controllerId);
      if (controller != NULL)
      {
         controller->deleteChild(this);
         deleteParent(controller);
      }
   }
}

/**
 * Create NXCP message with object's data
 */
void Chassis::fillMessageInternal(NXCPMessage *msg, UINT32 userId)
{
   super::fillMessageInternal(msg, userId);
   msg->setField(VID_CONTROLLER_ID, m_controllerId);
   msg->setField(VID_RACK_ID, m_rackId);
   msg->setField(VID_RACK_IMAGE_FRONT, m_rackImageFront);
   msg->setField(VID_RACK_IMAGE_REAR, m_rackImageRear);
   msg->setField(VID_RACK_POSITION, m_rackPosition);
   msg->setField(VID_RACK_HEIGHT, m_rackHeight);
   msg->setField(VID_RACK_ORIENTATION, static_cast<INT16>(m_rackOrientation));
}

/**
 * Modify object from NXCP message
 */
UINT32 Chassis::modifyFromMessageInternal(NXCPMessage *request)
{
   if (request->isFieldExist(VID_CONTROLLER_ID))
      m_controllerId = request->getFieldAsUInt32(VID_CONTROLLER_ID);
   if (request->isFieldExist(VID_RACK_ID))
   {
      m_rackId = request->getFieldAsUInt32(VID_RACK_ID);
      updateRackBinding();
   }
   if (request->isFieldExist(VID_RACK_IMAGE_FRONT))
      m_rackImageFront = request->getFieldAsGUID(VID_RACK_IMAGE_FRONT);
   if (request->isFieldExist(VID_RACK_IMAGE_REAR))
      m_rackImageFront = request->getFieldAsGUID(VID_RACK_IMAGE_REAR);
   if (request->isFieldExist(VID_RACK_POSITION))
      m_rackPosition = request->getFieldAsInt16(VID_RACK_POSITION);
   if (request->isFieldExist(VID_RACK_HEIGHT))
      m_rackHeight = request->getFieldAsInt16(VID_RACK_HEIGHT);
   if (request->isFieldExist(VID_RACK_ORIENTATION))
      m_rackOrientation = static_cast<RackOrientation>(request->getFieldAsInt16(VID_RACK_ORIENTATION));

   return super::modifyFromMessageInternal(request);
}

/**
 * Save to database
 */
bool Chassis::saveToDatabase(DB_HANDLE hdb)
{
   lockProperties();
   bool success = saveCommonProperties(hdb);
   if (success)
   {
      DB_STATEMENT hStmt;
      if (IsDatabaseRecordExist(hdb, _T("chassis"), _T("id"), m_id))
         hStmt = DBPrepare(hdb, _T("UPDATE chassis SET controller_id=?,rack_id=?,")
                                _T("rack_image_front=?,rack_position=?,rack_height=?,")
                                _T("rack_orientation=?,rack_image_rear=? WHERE id=?"));
      else
         hStmt = DBPrepare(hdb, _T("INSERT INTO chassis (controller_id,rack_id,rack_image_front,")
                                _T("rack_position,rack_height,rack_orientation,rack_image_rear,id)")
                                _T("VALUES (?,?,?,?,?,?,?,?)"));
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_controllerId);
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_rackId);
         DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_rackImageFront);
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_rackPosition);
         DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, m_rackHeight);
         DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_rackOrientation);
         DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, m_rackImageRear);
         DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_id);
         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }
   unlockProperties();

   if (success)
   {
      lockDciAccess(false);
      for(int i = 0; (i < m_dcObjects->size()) && success; i++)
         success = m_dcObjects->get(i)->saveToDatabase(hdb);
      unlockDciAccess();
   }

   if (success)
   {
      success = saveACLToDB(hdb);
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
bool Chassis::loadFromDatabase(DB_HANDLE hdb, UINT32 id)
{
   m_id = id;
   if (!loadCommonProperties(hdb))
   {
      nxlog_debug(2, _T("Cannot load common properties for chassis object %d"), id);
      return false;
   }

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT controller_id,rack_id,rack_image_front,")
                                       _T("rack_position,rack_height,rack_orientation,")
                                       _T("rack_image_rear FROM chassis WHERE id=?"));
   if (hStmt == NULL)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult == NULL)
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

   loadACLFromDB(hdb);
   loadItemsFromDB(hdb);
   for(int i = 0; i < m_dcObjects->size(); i++)
      if (!m_dcObjects->get(i)->loadThresholdsFromDB(hdb))
         return false;

   updateRackBinding();
   return true;
}


/**
 * Link related objects after loading from database
 */
void Chassis::linkObjects()
{
   super::linkObjects();
   updateControllerBinding();
}

/**
 * Called when data collection configuration changed
 */
void Chassis::onDataCollectionChange()
{
   Node *controller = (Node *)FindObjectById(m_controllerId, OBJECT_NODE);
   if (controller == NULL)
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
   Node *controller = (Node *)FindObjectById(m_controllerId, OBJECT_NODE);
   if (controller == NULL)
   {
      nxlog_debug(4, _T("Chassis::collectProxyInfo(%s [%d]): cannot find controller node object with id %d"), m_name, m_id, m_controllerId);
      return;
   }

   bool snmpProxy = (controller->getEffectiveSnmpProxy() == info->proxyId);
   bool isTarget = false;

   lockDciAccess(false);
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
      DCObject *dco = m_dcObjects->get(i);
      if (dco->getStatus() == ITEM_STATUS_DISABLED)
         continue;

      if (((snmpProxy && (dco->getDataSource() == DS_SNMP_AGENT) && (dco->getSourceNode() == 0)) ||
           ((dco->getDataSource() == DS_NATIVE_AGENT) && (dco->getSourceNode() == info->proxyId))) &&
          dco->hasValue() && (dco->getAgentCacheMode() == AGENT_CACHE_ON))
      {
         addProxyDataCollectionElement(info, dco);
         if (dco->getDataSource() == DS_SNMP_AGENT)
            isTarget = true;
      }
   }
   unlockDciAccess();

   if (isTarget)
      addProxySnmpTarget(info, controller);
}

/**
 * Create NXSL object for this object
 */
NXSL_Value *Chassis::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(new NXSL_Object(vm, &g_nxslChassisClass, this));
}

/**
 * Get effective source node for given data collection object
 */
UINT32 Chassis::getEffectiveSourceNode(DCObject *dco)
{
   if (dco->getSourceNode() != 0)
      return dco->getSourceNode();
   if ((dco->getDataSource() == DS_NATIVE_AGENT) ||
       (dco->getDataSource() == DS_SMCLP) ||
       (dco->getDataSource() == DS_SNMP_AGENT) ||
       (dco->getDataSource() == DS_WINPERF))
   {
      return m_controllerId;
   }
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
   json_object_set_new(root, "controllerId", json_integer(m_controllerId));
   json_object_set_new(root, "rackHeight", json_integer(m_rackHeight));
   json_object_set_new(root, "rackPosition", json_integer(m_rackPosition));
   json_object_set_new(root, "rackOrientation", json_integer(m_rackPosition));
   json_object_set_new(root, "rackId", json_integer(m_rackId));
   json_object_set_new(root, "rackImageFront", m_rackImageFront.toJson());
   json_object_set_new(root, "rackImageRear", m_rackImageRear.toJson());
   return root;
}
