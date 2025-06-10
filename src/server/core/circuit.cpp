/*
** NetXMS - Network Management System
** Copyright (C) 2024-2025 Raden Solutions
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
** File: circuit.cpp
**
**/

#include "nxcore.h"

/**
 * Load from database
 */
bool Circuit::loadFromDatabase(DB_HANDLE hdb, uint32_t id, DB_STATEMENT *preparedStatements)
{
   m_id = id;

   if (!loadCommonProperties(hdb, preparedStatements))
      return false;

   if (!super::loadFromDatabase(hdb, id, preparedStatements))
      return false;

   if (!AutoBindTarget::loadFromDatabase(hdb, id))
      return false;

   if (!Pollable::loadFromDatabase(hdb, id))
      return false;

   if (!m_isDeleted)
      ContainerBase::loadFromDatabase(hdb, id);

   // Load DCI and access list
   loadACLFromDB(hdb, preparedStatements);
   loadItemsFromDB(hdb, preparedStatements);
   for(int i = 0; i < m_dcObjects.size(); i++)
      if (!m_dcObjects.get(i)->loadThresholdsFromDB(hdb, preparedStatements))
         return false;
   loadDCIListForCleanup(hdb);

   m_runtimeFlags |= ODF_CONFIGURATION_POLL_PASSED; //Do not have configuration poll, but has instance discovery

   return true;
}

/**
 * Save to database
 */
bool Circuit::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);

   if (success)
      success = ContainerBase::saveToDatabase(hdb);

   if (success && (m_modified & MODIFY_OTHER))
      success = AutoBindTarget::saveToDatabase(hdb);

   return success;
}

/**
 * Delete from database
 */
bool Circuit::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);

   if (success)
      success = ContainerBase::deleteFromDatabase(hdb);

   if (success)
      success = AutoBindTarget::deleteFromDatabase(hdb);

   return success;
}

/**
 * Modify object from message
 */
uint32_t Circuit::modifyFromMessageInternal(const NXCPMessage& msg, ClientSession *session)
{
   AutoBindTarget::modifyFromMessage(msg);
   return super::modifyFromMessageInternal(msg, session);
}

/**
 * Fill message with object fields
 */
void Circuit::fillMessageUnlocked(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageUnlocked(msg, userId);
   AutoBindTarget::fillMessage(msg);
}

/**
 * Called by client session handler to check if threshold summary should be shown for this object.
 */
bool Circuit::showThresholdSummary() const
{
   return true;
}

/**
 * Post-load hook
 */
void Circuit::postLoad()
{
   super::postLoad();
   ContainerBase::postLoad();
}

/**
 * Perform automatic object binding
 */
void Circuit::autobindPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId)
{
   poller->setStatus(_T("wait for lock"));
   pollerLock(autobind);

   if (IsShutdownInProgress())
   {
      pollerUnlock();
      return;
   }

   m_pollRequestor = session;
   m_pollRequestId = rqId;
   nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 5, _T("Starting autobind poll of circuit %s [%u]"), m_name, m_id);
   poller->setStatus(_T("checking objects"));

   if (!isAutoBindEnabled())
   {
      sendPollerMsg(_T("Automatic object binding is disabled\r\n"));
      nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 5, _T("Finished autobind poll of circuit %s [%u])"), m_name, m_id);
      pollerUnlock();
      return;
   }

   NXSL_VM *cachedFilterVM = nullptr;
   unique_ptr<SharedObjectArray<NetObj>> objects = g_idxObjectById.getObjects([] (NetObj *object) -> bool { return object->getObjectClass() == OBJECT_INTERFACE; });
   for (int i = 0; i < objects->size(); i++)
   {
      shared_ptr<NetObj> iface = objects->getShared(i);

      AutoBindDecision decision = isApplicable(&cachedFilterVM, iface, this);
      if ((decision == AutoBindDecision_Ignore) || ((decision == AutoBindDecision_Unbind) && !isAutoUnbindEnabled()))
         continue;   // Decision cannot affect checks

      if ((decision == AutoBindDecision_Bind) && !isDirectChild(iface->getId()))
      {
         sendPollerMsg(_T("   Binding interface %s\r\n"), iface->getName());
         nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 4, _T("Circuit::autobindPoll(): binding interface \"%s\" [%u] to circuit \"%s\" [%u]"), iface->getName(), iface->getId(), m_name, m_id);
         linkObjects(self(), iface);
         EventBuilder(EVENT_CIRCUIT_AUTOBIND, g_dwMgmtNode)
            .param(_T("interfaceId"), iface->getId(), EventBuilder::OBJECT_ID_FORMAT)
            .param(_T("interfaceName"), iface->getName())
            .param(_T("circuitId"), m_id, EventBuilder::OBJECT_ID_FORMAT)
            .param(_T("circuitName"), m_name)
            .param(_T("nodeId"), static_cast<Interface&>(*iface).getParentNodeId(), EventBuilder::OBJECT_ID_FORMAT)
            .param(_T("nodeName"), static_cast<Interface&>(*iface).getParentNodeName())
            .post();
         calculateCompoundStatus();
      }
      else if ((decision == AutoBindDecision_Unbind) && isDirectChild(iface->getId()))
      {
         sendPollerMsg(_T("   Removing interface %s\r\n"), iface->getName());
         nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 4, _T("Circuit::autobindPoll(): removing interface \"%s\" [%u] from circuit \"%s\" [%u]"), iface->getName(), iface->getId(), m_name, m_id);
         unlinkObjects(this, iface.get());
         EventBuilder(EVENT_CIRCUIT_AUTOUNBIND, g_dwMgmtNode)
            .param(_T("interfaceId"), iface->getId(), EventBuilder::OBJECT_ID_FORMAT)
            .param(_T("interfaceName"), iface->getName())
            .param(_T("circuitId"), m_id, EventBuilder::OBJECT_ID_FORMAT)
            .param(_T("circuitName"), m_name)
            .param(_T("nodeId"), static_cast<Interface&>(*iface).getParentNodeId(), EventBuilder::OBJECT_ID_FORMAT)
            .param(_T("nodeName"), static_cast<Interface&>(*iface).getParentNodeName())
            .post();
         calculateCompoundStatus();
      }
   }
   delete cachedFilterVM;

   pollerUnlock();
   nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 5, _T("Finished autobind poll of circuit %s [%u])"), m_name, m_id);
}

/**
 * Get instances for instance discovery DCO
 */
StringMap *Circuit::getInstanceList(DCObject *dco)
{
   shared_ptr<Node> sourceNode;
   uint32_t sourceNodeId = getEffectiveSourceNode(dco);
   if (sourceNodeId != 0)
   {
      sourceNode = static_pointer_cast<Node>(FindObjectById(dco->getSourceNode(), OBJECT_NODE));
      if (sourceNode == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG_INSTANCE_POLL, 6, _T("Circuit::getInstanceList(%s [%u]): source node [%u] not found"), dco->getName().cstr(), dco->getId(), sourceNodeId);
         return nullptr;
      }
   }

   StringList *instances = nullptr;
   StringMap *instanceMap = nullptr;
   shared_ptr<Table> instanceTable;
   wchar_t tableName[MAX_DB_STRING], nameColumn[MAX_DB_STRING];
   switch(dco->getInstanceDiscoveryMethod())
   {
      case IDM_INTERNAL_TABLE:
         parseInstanceDiscoveryTableName(dco->getInstanceDiscoveryData(), tableName, nameColumn);
         if (sourceNode != nullptr)
         {
            sourceNode->getInternalTable(tableName, &instanceTable);
         }
         else
         {
            getInternalTable(tableName, &instanceTable);
         }
         break;
      case IDM_SCRIPT:
         if (sourceNode != nullptr)
         {
            sourceNode->getStringMapFromScript(dco->getInstanceDiscoveryData(), &instanceMap, this);
         }
         else
         {
            getStringMapFromScript(dco->getInstanceDiscoveryData(), &instanceMap, this);
         }
         break;
      case IDM_WEB_SERVICE:
         if (sourceNode != nullptr)
         {
            sourceNode->getListFromWebService(dco->getInstanceDiscoveryData(), &instances);
         }
         break;
      default:
         break;
   }
   if ((instances == nullptr) && (instanceMap == nullptr) && (instanceTable == nullptr))
      return nullptr;

   if (instanceTable != nullptr)
   {
      instanceMap = new StringMap();
      wchar_t buffer[1024];
      int nameColumnIndex = (nameColumn[0] != 0) ? instanceTable->getColumnIndex(nameColumn) : -1;
      for(int i = 0; i < instanceTable->getNumRows(); i++)
      {
         instanceTable->buildInstanceString(i, buffer, 1024);
         if (nameColumnIndex != -1)
         {
            const wchar_t *name = instanceTable->getAsString(i, nameColumnIndex, buffer);
            if (name != nullptr)
               instanceMap->set(buffer, name);
            else
               instanceMap->set(buffer, buffer);
         }
         else
         {
            instanceMap->set(buffer, buffer);
         }
      }
   }
   else if (instanceMap == nullptr)
   {
      instanceMap = new StringMap;
      for(int i = 0; i < instances->size(); i++)
         instanceMap->set(instances->get(i), instances->get(i));
   }
   delete instances;
   return instanceMap;
}

/**
 * Create NXSL object for this object
 */
NXSL_Value *Circuit::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(vm->createObject(&g_nxslCircuitClass, new shared_ptr<Circuit>(self())));
}

/**
 * Get list of interfaces for NXSL script
 */
NXSL_Array *Circuit::getInterfacesForNXSL(NXSL_VM *vm)
{
   NXSL_Array *interfaces = new NXSL_Array(vm);
   int index = 0;

   readLockChildList();
   for(int i = 0; i < getParentList().size(); i++)
   {
      NetObj *object = getChildList().get(i);
      if (object->getObjectClass() == OBJECT_INTERFACE)
      {
         interfaces->set(index++, object->createNXSLObject(vm));
      }
   }
   unlockChildList();

   return interfaces;
}
