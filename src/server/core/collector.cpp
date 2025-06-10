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
** File: collector.cpp
**
**/

#include "nxcore.h"

/**
 * Load from database
 */
bool Collector::loadFromDatabase(DB_HANDLE hdb, uint32_t id, DB_STATEMENT *preparedStatements)
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
bool Collector::saveToDatabase(DB_HANDLE hdb)
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
bool Collector::deleteFromDatabase(DB_HANDLE hdb)
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
uint32_t Collector::modifyFromMessageInternal(const NXCPMessage& msg, ClientSession *session)
{
   AutoBindTarget::modifyFromMessage(msg);
   return super::modifyFromMessageInternal(msg, session);
}

/**
 * Fill message with object fields
 */
void Collector::fillMessageUnlocked(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageUnlocked(msg, userId);
   AutoBindTarget::fillMessage(msg);
}

/**
 * Return STATUS_NORMAL as additional status so that empty collector will have NORMAL status instead of UNKNOWN.
 */
int Collector::getAdditionalMostCriticalStatus()
{
   return STATUS_NORMAL;
}

/**
 * Called by client session handler to check if threshold summary should be shown for this object.
 */
bool Collector::showThresholdSummary() const
{
   return true;
}

/**
 * Post-load hook
 */
void Collector::postLoad()
{
   super::postLoad();
   ContainerBase::postLoad();
}

/**
 * Perform automatic object binding
 */
void Collector::autobindPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId)
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
   nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 5, _T("Starting autobind poll of collector %s [%u]"), m_name, m_id);
   poller->setStatus(_T("checking objects"));

   if (!isAutoBindEnabled())
   {
      sendPollerMsg(_T("Automatic object binding is disabled\r\n"));
      nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 5, _T("Finished autobind poll of collector %s [%u])"), m_name, m_id);
      pollerUnlock();
      return;
   }

   NXSL_VM *cachedFilterVM = nullptr;
   unique_ptr<SharedObjectArray<NetObj>> objects = getObjectsForAutoBind(_T("ContainerAutoBind"));
   for (int i = 0; i < objects->size(); i++)
   {
      shared_ptr<NetObj> object = objects->getShared(i);
      if (object->getId() == m_id)
         continue;

      AutoBindDecision decision = isApplicable(&cachedFilterVM, object, this);
      if ((decision == AutoBindDecision_Ignore) || ((decision == AutoBindDecision_Unbind) && !isAutoUnbindEnabled()))
         continue;   // Decision cannot affect checks

      if ((decision == AutoBindDecision_Bind) && !isDirectChild(object->getId()))
      {
         sendPollerMsg(_T("   Binding object %s\r\n"), object->getName());
         nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 4, _T("Collector::autobindPoll(): binding object \"%s\" [%u] to collector \"%s\" [%u]"), object->getName(), object->getId(), m_name, m_id);
         linkObjects(self(), object);
         EventBuilder(EVENT_CONTAINER_AUTOBIND, g_dwMgmtNode)
            .param(_T("nodeId"), object->getId(), EventBuilder::OBJECT_ID_FORMAT)
            .param(_T("nodeName"), object->getName())
            .param(_T("containerId"), m_id, EventBuilder::OBJECT_ID_FORMAT)
            .param(_T("containerName"), m_name)
            .post();
         calculateCompoundStatus();
      }
      else if ((decision == AutoBindDecision_Unbind) && isDirectChild(object->getId()))
      {
         sendPollerMsg(_T("   Removing object %s\r\n"), object->getName());
         nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 4, _T("Collector::autobindPoll(): removing object \"%s\" [%u] from collector \"%s\" [%u]"), object->getName(), object->getId(), m_name, m_id);
         unlinkObjects(this, object.get());
         EventBuilder(EVENT_CONTAINER_AUTOUNBIND, g_dwMgmtNode)
            .param(_T("nodeId"), object->getId(), EventBuilder::OBJECT_ID_FORMAT)
            .param(_T("nodeName"), object->getName())
            .param(_T("containerId"), m_id, EventBuilder::OBJECT_ID_FORMAT)
            .param(_T("containerName"), m_name)
            .post();
         calculateCompoundStatus();
      }
   }
   delete cachedFilterVM;

   pollerUnlock();
   nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 5, _T("Finished autobind poll of collector %s [%u])"), m_name, m_id);
}

/**
 * Get instances for instance discovery DCO
 */
StringMap *Collector::getInstanceList(DCObject *dco)
{
   shared_ptr<Node> sourceNode;
   uint32_t sourceNodeId = getEffectiveSourceNode(dco);
   if (sourceNodeId != 0)
   {
      sourceNode = static_pointer_cast<Node>(FindObjectById(dco->getSourceNode(), OBJECT_NODE));
      if (sourceNode == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG_INSTANCE_POLL, 6, _T("Collector::getInstanceList(%s [%u]): source node [%u] not found"), dco->getName().cstr(), dco->getId(), sourceNodeId);
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
NXSL_Value *Collector::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(vm->createObject(&g_nxslCollectorClass, new shared_ptr<Collector>(self())));
}

/**
 * Enter maintenance mode
 */
void Collector::enterMaintenanceMode(uint32_t userId, const TCHAR *comments)
{
   super::enterMaintenanceMode(userId, comments);

   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *object = getChildList().get(i);
      if ((object->getObjectClass() == OBJECT_NODE) && (object->getStatus() != STATUS_UNMANAGED))
         object->enterMaintenanceMode(userId, comments);
   }
   unlockChildList();
}

/**
 * Leave maintenance mode
 */
void Collector::leaveMaintenanceMode(uint32_t userId)
{
   super::leaveMaintenanceMode(userId);

   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *object = getChildList().get(i);
      if ((object->getObjectClass() == OBJECT_NODE) && (object->getStatus() != STATUS_UNMANAGED))
         object->leaveMaintenanceMode(userId);
   }
   unlockChildList();
}
