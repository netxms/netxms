/*
** NetXMS - Network Management System
** Copyright (C) 2024-2026 Raden Solutions
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
** File: dc_container.cpp
**
**/

#include "nxcore.h"

/**
 * Load from database
 */
bool DataCollectionContainer::loadFromDatabase(DB_HANDLE hdb, uint32_t id, DB_STATEMENT *preparedStatements)
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
bool DataCollectionContainer::saveToDatabase(DB_HANDLE hdb)
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
bool DataCollectionContainer::deleteFromDatabase(DB_HANDLE hdb)
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
uint32_t DataCollectionContainer::modifyFromMessageInternal(const NXCPMessage& msg, ClientSession *session)
{
   AutoBindTarget::modifyFromMessage(msg);
   return super::modifyFromMessageInternal(msg, session);
}

/**
 * Fill message with object fields
 */
void DataCollectionContainer::fillMessageUnlocked(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageUnlocked(msg, userId);
   AutoBindTarget::fillMessage(msg);
}

/**
 * Return STATUS_NORMAL as additional status so that empty container will have NORMAL status instead of UNKNOWN.
 */
int DataCollectionContainer::getAdditionalMostCriticalStatus(StringBuffer *explanation)
{
   return STATUS_NORMAL;
}

/**
 * Called by client session handler to check if threshold summary should be shown for this object.
 */
bool DataCollectionContainer::showThresholdSummary() const
{
   return true;
}

/**
 * Serialize object to JSON
 */
json_t *DataCollectionContainer::toJson(bool includeSensitiveData)
{
   json_t *root = super::toJson(includeSensitiveData);
   AutoBindTarget::toJson(root);
   return root;
}

/**
 * Post-load hook
 */
void DataCollectionContainer::postLoad()
{
   super::postLoad();
   ContainerBase::postLoad();
}

/**
 * Perform automatic object binding
 */
void DataCollectionContainer::autobindPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId)
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
   nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 5, _T("Starting autobind poll of %s %s [%u]"), getObjectClassName(), m_name, m_id);
   poller->setStatus(_T("checking objects"));

   runContainerAutoBindPoll();

   pollerUnlock();
   nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 5, _T("Finished autobind poll of %s %s [%u])"), getObjectClassName(), m_name, m_id);
}

/**
 * Get instances for instance discovery DCO
 */
StringMap *DataCollectionContainer::getInstanceList(DCObject *dco)
{
   return getInstanceListFromSourceNodeOrSelf(dco);
}

/**
 * Enter maintenance mode
 */
void DataCollectionContainer::enterMaintenanceMode(uint32_t userId, const TCHAR *comments)
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
void DataCollectionContainer::leaveMaintenanceMode(uint32_t userId)
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
