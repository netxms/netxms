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
** File: container.cpp
**
**/

#include "nxcore.h"

/**
 * Base container class destructor
 */
ContainerBase::~ContainerBase()
{
   MemFree(m_childIdList);
}

/**
 * Post-load hook. Link child objects after loading from database.
 * This method is expected to be called only at startup, so we don't lock
 */
void ContainerBase::postLoad()
{
   if (m_childIdList != nullptr)
   {
      // Find and link child objects
      int count = static_cast<int>(m_childIdList[0]);
      for(int i = 1; i <= count; i++)
      {
         shared_ptr<NetObj> object = FindObjectById(m_childIdList[i]);
         if (object != nullptr)
         {
            NetObj::linkObjects(m_this->self(), object);
         }
         else
         {
            nxlog_write(NXLOG_ERROR, _T("Inconsistent database: container object %s [%u] has reference to non-existing child object [%u]"), m_this->m_name, m_this->m_id, m_childIdList[i]);
         }
      }

      // Cleanup
      MemFreeAndNull(m_childIdList);
   }
}

/**
 * Create base container object from database data.
 *
 * @param hdb DB handle
 * @param id object ID
 */
void ContainerBase::loadFromDatabase(DB_HANDLE hdb, uint32_t id)
{
   TCHAR query[256];
   _sntprintf(query, sizeof(query) / sizeof(TCHAR), _T("SELECT object_id FROM container_members WHERE container_id=%u"), id);
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      if (count > 0)
      {
         m_childIdList = MemAllocArrayNoInit<uint32_t>(count + 1);
         m_childIdList[0] = static_cast<uint32_t>(count);
         for(int i = 0; i < count; i++)
            m_childIdList[i + 1] = DBGetFieldULong(hResult, i, 0);
      }
      DBFreeResult(hResult);
   }
}

/**
 * Save object to database
 *
 * @param hdb database connection handle
 */
bool ContainerBase::saveToDatabase(DB_HANDLE hdb)
{
   bool success = true;
   if (m_this->m_modified & MODIFY_OTHER)
   {
      static const TCHAR *columns[] = { _T("object_class"), nullptr };
      DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("object_containers"), _T("id"), m_this->m_id, columns);
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (LONG)m_this->getObjectClass());
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_this->m_id);
         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }

   if (success && (m_this->m_modified & MODIFY_RELATIONS))
   {
      success = m_this->executeQueryOnObject(hdb, _T("DELETE FROM container_members WHERE container_id=?"));
      m_this->readLockChildList();
      if (success && !m_this->getChildList().isEmpty())
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO container_members (container_id,object_id) VALUES (?,?)"));
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_this->m_id);
            for(int i = 0; (i < m_this->getChildList().size()) && success; i++)
            {
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_this->getChildList().get(i)->getId());
               success = DBExecute(hStmt);
            }
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
         }
      }
      m_this->unlockChildList();
   }

   return success;
}

/**
 * Delete object from database
 */
bool ContainerBase::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = m_this->executeQueryOnObject(hdb, _T("DELETE FROM object_containers WHERE id=?"));
   if (success)
      success = m_this->executeQueryOnObject(hdb, _T("DELETE FROM container_members WHERE container_id=?"));
   return success;
}

/**
 * "Normal" abstract container class constructor
 */
AbstractContainer::AbstractContainer(const TCHAR *name) : super(), ContainerBase(this)
{
   _tcslcpy(m_name, name, MAX_OBJECT_NAME);
   m_isHidden = true;
   setCreationTime();
}

/**
 * Post-load hook
 */
void AbstractContainer::postLoad()
{
   super::postLoad();
   ContainerBase::postLoad();
}

/**
 * Calculate status for compound object based on childs' status
 */
void AbstractContainer::calculateCompoundStatus(bool forcedRecalc)
{
	super::calculateCompoundStatus(forcedRecalc);

   lockProperties();
	if ((m_status == STATUS_UNKNOWN) && (getChildCount() == 0))
   {
		m_status = STATUS_NORMAL;
		setModified(MODIFY_RUNTIME);
	}
   unlockProperties();
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
bool AbstractContainer::loadFromDatabase(DB_HANDLE hdb, uint32_t id, DB_STATEMENT *preparedStatements)
{
   m_id = id;

   if (!loadCommonProperties(hdb, preparedStatements))
      return false;

   // Load access list
   loadACLFromDB(hdb, preparedStatements);

   // Load child list for later linkage
   if (!m_isDeleted)
   {
      ContainerBase::loadFromDatabase(hdb, id);
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
   if (super::saveToDatabase(hdb))
      return ContainerBase::saveToDatabase(hdb);
   return false;
}

/**
 * Delete object from database
 */
bool AbstractContainer::deleteFromDatabase(DB_HANDLE hdb)
{
   if (super::deleteFromDatabase(hdb))
      return ContainerBase::deleteFromDatabase(hdb);
   return false;
}

/**
 * Create container object from database data.
 *
 * @param dwId object ID
 */
bool Container::loadFromDatabase(DB_HANDLE hdb, uint32_t id, DB_STATEMENT *preparedStatements)
{
   bool success = super::loadFromDatabase(hdb, id, preparedStatements);
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
         if (hStmt != nullptr)
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
uint32_t Container::modifyFromMessageInternal(const NXCPMessage& msg, ClientSession *session)
{
   AutoBindTarget::modifyFromMessage(msg);
   return super::modifyFromMessageInternal(msg, session);
}

/**
 * Fill message with object fields
 */
void Container::fillMessageUnlocked(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageUnlocked(msg, userId);
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
 * Perform automatic object binding
 */
void Container::autobindPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId)
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
   nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 5, _T("Starting autobind poll of container %s [%u]"), m_name, m_id);
   poller->setStatus(_T("checking objects"));

   if (!isAutoBindEnabled())
   {
      sendPollerMsg(_T("Automatic object binding is disabled\r\n"));
      nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 5, _T("Finished autobind poll of container %s [%u])"), m_name, m_id);
      pollerUnlock();
      return;
   }

   NXSL_VM *cachedFilterVM = nullptr;
   unique_ptr<SharedObjectArray<NetObj>> objects = getObjectsForAutoBind(_T("ContainerAutoBind"));
   for (int i = 0; i < objects->size(); i++)
   {
      shared_ptr<NetObj> object = objects->getShared(i);

      AutoBindDecision decision = isApplicable(&cachedFilterVM, object, this);
      if ((decision == AutoBindDecision_Ignore) || ((decision == AutoBindDecision_Unbind) && !isAutoUnbindEnabled()))
         continue;   // Decision cannot affect checks

      if ((decision == AutoBindDecision_Bind) && !isDirectChild(object->getId()))
      {
         sendPollerMsg(_T("   Binding object %s\r\n"), object->getName());
         nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 4, _T("Container::autobindPoll(): binding object \"%s\" [%u] to container \"%s\" [%u]"), object->getName(), object->getId(), m_name, m_id);
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
         nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 4, _T("Container::autobindPoll(): removing object \"%s\" [%u] from container \"%s\" [%u]"), object->getName(), object->getId(), m_name, m_id);
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
   nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 5, _T("Finished autobind poll of container %s [%u])"), m_name, m_id);
}
