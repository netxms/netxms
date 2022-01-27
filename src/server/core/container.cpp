/*
** NetXMS - Network Management System
** Copyright (C) 2003-2022 Victor Kirhenshtein
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
** File: container.cpp
**
**/

#include "nxcore.h"

/**
 * Default abstract container class constructor
 */
AbstractContainer::AbstractContainer() : super()
{
   m_pdwChildIdList = NULL;
   m_dwChildIdListSize = 0;
}

/**
 * "Normal" abstract container class constructor
 */
AbstractContainer::AbstractContainer(const TCHAR *pszName, UINT32 dwCategory) : super()
{
   _tcslcpy(m_name, pszName, MAX_OBJECT_NAME);
   m_pdwChildIdList = NULL;
   m_dwChildIdListSize = 0;
   m_isHidden = true;
   setCreationTime();
}

/**
 * Abstract container class destructor
 */
AbstractContainer::~AbstractContainer()
{
   MemFree(m_pdwChildIdList);
}

/**
 * Link child objects after loading from database
 * This method is expected to be called only at startup, so we don't lock
 */
void AbstractContainer::linkObjects()
{
   super::linkObjects();
   if (m_dwChildIdListSize > 0)
   {
      // Find and link child objects
      for(UINT32 i = 0; i < m_dwChildIdListSize; i++)
      {
         shared_ptr<NetObj> object = FindObjectById(m_pdwChildIdList[i]);
         if (object != nullptr)
            linkObject(object);
         else
            nxlog_write(NXLOG_ERROR, _T("Inconsistent database: container object %s [%u] has reference to non-existing child object [%u]"),
                     m_name, m_id, m_pdwChildIdList[i]);
      }

      // Cleanup
      MemFreeAndNull(m_pdwChildIdList);
      m_dwChildIdListSize = 0;
   }
}

/**
 * Calculate status for compound object based on childs' status
 */
void AbstractContainer::calculateCompoundStatus(BOOL bForcedRecalc)
{
	super::calculateCompoundStatus(bForcedRecalc);

	if ((m_status == STATUS_UNKNOWN) && (m_dwChildIdListSize == 0))
   {
		lockProperties();
		m_status = STATUS_NORMAL;
		setModified(MODIFY_RUNTIME);
		unlockProperties();
	}
}

/**
 * Create NXCP message with object's data
 */
void AbstractContainer::fillMessageInternal(NXCPMessage *msg, UINT32 userId)
{
   super::fillMessageInternal(msg, userId);
}

/**
 * Modify object from message
 */
uint32_t AbstractContainer::modifyFromMessageInternal(const NXCPMessage& msg)
{
   if (msg.isFieldExist(VID_FLAGS))
		m_flags = msg.getFieldAsUInt32(VID_FLAGS);

   return super::modifyFromMessageInternal(msg);
}

/**
 * Serialize object to JSON
 */
json_t *AbstractContainer::toJson()
{
   json_t *root = super::toJson();
   lockProperties();
   json_object_set_new(root, "flags", json_integer(m_flags));
   unlockProperties();
   return root;
}

/**
 * Create abstract container object from database data.
 *
 * @param dwId object ID
 */
bool AbstractContainer::loadFromDatabase(DB_HANDLE hdb, UINT32 dwId)
{
   TCHAR szQuery[256];
   DB_RESULT hResult;
   UINT32 i;

   m_id = dwId;

   if (!loadCommonProperties(hdb))
      return false;

   // Load access list
   loadACLFromDB(hdb);

   // Load child list for later linkage
   if (!m_isDeleted)
   {
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT object_id FROM container_members WHERE container_id=%d"), m_id);
      hResult = DBSelect(hdb, szQuery);
      if (hResult != NULL)
      {
         m_dwChildIdListSize = DBGetNumRows(hResult);
         if (m_dwChildIdListSize > 0)
         {
            m_pdwChildIdList = (UINT32 *)malloc(sizeof(UINT32) * m_dwChildIdListSize);
            for(i = 0; i < m_dwChildIdListSize; i++)
               m_pdwChildIdList[i] = DBGetFieldULong(hResult, i, 0);
         }
         DBFreeResult(hResult);
      }
   }

   return true;
}

/**
 * Save object to database
 *
 * @param hdb database connection handle
 */
bool AbstractContainer::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);

   if (success && (m_modified & MODIFY_OTHER))
   {
      static const TCHAR *columns[] = { _T("object_class"), NULL };
      DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("object_containers"), _T("id"), m_id, columns);
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (LONG)getObjectClass());
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_id);
         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }

   if (success && (m_modified & MODIFY_RELATIONS))
   {
      success = executeQueryOnObject(hdb, _T("DELETE FROM container_members WHERE container_id=?"));
      readLockChildList();
      if (success && !getChildList().isEmpty())
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO container_members (container_id,object_id) VALUES (?,?)"));
         if (hStmt != NULL)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            for(int i = 0; (i < getChildList().size()) && success; i++)
            {
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, getChildList().get(i)->getId());
               success = DBExecute(hStmt);
            }
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
         }
      }
      unlockChildList();
   }

   return success;
}

/**
 * Delete object from database
 */
bool AbstractContainer::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM object_containers WHERE id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM container_members WHERE container_id=?"));
   return success;
}

/**
 * Create container object from database data.
 *
 * @param dwId object ID
 */
bool Container::loadFromDatabase(DB_HANDLE hdb, UINT32 dwId)
{
   bool success = super::loadFromDatabase(hdb, dwId);
   if (success)
      success = AutoBindTarget::loadFromDatabase(hdb, m_id);
   if (success)
      success = Pollable::loadFromDatabase(hdb, m_id);
   return success;
}

/**
 * Save object to database
 *
 * @param hdb database connection handle
 */
bool Container::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);
   if (success)
   {
      if (!IsDatabaseRecordExist(hdb, _T("object_containers"), _T("id"), m_id))
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO object_containers (id) VALUES (?)"));
         if (hStmt != NULL)
         {
            DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_id);
            success = DBExecute(hStmt);
            DBFreeStatement(hStmt);
         }
      }
   }

   if (success && (m_modified & MODIFY_OTHER))
      success = AutoBindTarget::saveToDatabase(hdb);

   return success;
}

/**
 * Delete object from database
 */
bool Container::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if(success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM object_containers WHERE id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM container_members WHERE container_id=?"));
   if (success)
      success = AutoBindTarget::deleteFromDatabase(hdb);
   return success;
}

/**
 * Called by client session handler to check if threshold summary should be shown for this object.
 */
bool Container::showThresholdSummary() const
{
   return true;
}

/**
 * Modify object from message
 */
uint32_t Container::modifyFromMessageInternal(const NXCPMessage& msg)
{
   AutoBindTarget::modifyFromMessage(msg);
   return super::modifyFromMessageInternal(msg);
}

/**
 * Fill message with object fields
 */
void Container::fillMessageInternal(NXCPMessage *msg, UINT32 userId)
{
   super::fillMessageInternal(msg, userId);
   AutoBindTarget::fillMessage(msg);
}

/**
 * Create NXSL object for this object
 */
NXSL_Value *Container::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(vm->createObject(&g_nxslContainerClass, new shared_ptr<Container>(self())));
}

/**
 * Lock container for autobind poll
 */
bool Container::lockForAutobindPoll()
{
   bool success = false;
   lockProperties();
   if (!m_isDeleted && !m_isDeleteInitiated && (m_status != STATUS_UNMANAGED) &&
       (static_cast<uint32_t>(time(nullptr) - m_autobindPollState.getLastCompleted()) > g_autobindPollingInterval))
   {
      success = m_autobindPollState.schedule();
   }
   unlockProperties();
   return success;
}

/**
 * Class filter data for object selection
 */
struct AutoBindClassFilterData
{
   bool processAccessPoints;
   bool processClusters;
   bool processMobileDevices;
   bool processSensors;
};

/**
 * Object filter for autobind
 */
static bool AutoBindObjectFilter(NetObj* object, AutoBindClassFilterData* filterData)
{
   return (object->getObjectClass() == OBJECT_NODE) ||
         (filterData->processAccessPoints && (object->getObjectClass() == OBJECT_ACCESSPOINT)) ||
         (filterData->processClusters && (object->getObjectClass() == OBJECT_CLUSTER)) ||
         (filterData->processMobileDevices && (object->getObjectClass() == OBJECT_MOBILEDEVICE)) ||
         (filterData->processSensors && (object->getObjectClass() == OBJECT_SENSOR));
}

/**
 * Perform automatic object binding
 */
void Container::autobindPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId)
{
   poller->setStatus(_T("wait for lock"));
   pollerLock(configuration);

   if (IsShutdownInProgress())
   {
      pollerUnlock();
      return;
   }

   m_pollRequestor = session;
   m_pollRequestId = rqId;
   nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 5, _T("Starting autobind poll of container %s [%u]"), m_name, m_id);
   poller->setStatus(_T("checking objects"));

   if (!isAutoBindEnabled())
   {
      sendPollerMsg(_T("Automatic object binding is disabled\r\n"));
      nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 5, _T("Finished autobind poll of container %s [%u])"), m_name, m_id);
      pollerUnlock();
      return;
   }

   AutoBindClassFilterData filterData;
   filterData.processAccessPoints = ConfigReadBoolean(_T("Objects.AccessPoints.ContainerAutoBind"), false);
   filterData.processClusters = ConfigReadBoolean(_T("Objects.Clusters.ContainerAutoBind"), false);
   filterData.processMobileDevices = ConfigReadBoolean(_T("Objects.MobileDevices.ContainerAutoBind"), false);
   filterData.processSensors = ConfigReadBoolean(_T("Objects.Sensors.ContainerAutoBind"), false);

   NXSL_VM *cachedFilterVM = nullptr;
   unique_ptr<SharedObjectArray<NetObj>> objects = g_idxObjectById.getObjects(AutoBindObjectFilter, &filterData);
   for (int i = 0; i < objects->size(); i++)
   {
      shared_ptr<NetObj> object = objects->getShared(i);

      AutoBindDecision decision = isApplicable(&cachedFilterVM, object);
      if ((decision == AutoBindDecision_Ignore) || ((decision == AutoBindDecision_Unbind) && !isAutoUnbindEnabled()))
         continue;   // Decision cannot affect checks

      if ((decision == AutoBindDecision_Bind) && !isDirectChild(object->getId()))
      {
         sendPollerMsg(_T("   Binding object %s\r\n"), object->getName());
         nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 4, _T("Container::autobindPoll(): binding object \"%s\" [%u] to container \"%s\" [%u]"), object->getName(), object->getId(), m_name, m_id);
         addChild(object);
         object->addParent(self());
         PostSystemEvent(EVENT_CONTAINER_AUTOBIND, g_dwMgmtNode, "isis", object->getId(), object->getName(), m_id, m_name);
         calculateCompoundStatus();
      }
      else if ((decision == AutoBindDecision_Unbind) && isDirectChild(object->getId()))
      {
         sendPollerMsg(_T("   Removing object %s\r\n"), object->getName());
         nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 4, _T("Container::autobindPoll(): removing object \"%s\" [%u] from container \"%s\" [%u]"), object->getName(), object->getId(), m_name, m_id);
         deleteChild(*object);
         object->deleteParent(*this);
         PostSystemEvent(EVENT_CONTAINER_AUTOUNBIND, g_dwMgmtNode, "isis", object->getId(), object->getName(), m_id, m_name);
         calculateCompoundStatus();
      }
   }
   delete cachedFilterVM;

   pollerUnlock();
   nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 5, _T("Finished autobind poll of container %s [%u])"), m_name, m_id);
}
