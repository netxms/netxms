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
** File: cluster.cpp
**
**/

#include "nxcore.h"

#define DEBUG_TAG_CONF_POLL         _T("poll.conf")
#define DEBUG_TAG_STATUS_POLL       _T("poll.status")

/**
 * Cluster class default constructor
 */
Cluster::Cluster() : super(Pollable::STATUS | Pollable::CONFIGURATION | Pollable::AUTOBIND), AutoBindTarget(this), m_syncNetworks(0, 8, Ownership::True)
{
	m_clusterType = 0;
	m_dwNumResources = 0;
	m_pResourceList = nullptr;
	m_zoneUIN = 0;
}

/**
 * Cluster class new object constructor
 */
Cluster::Cluster(const TCHAR *pszName, int32_t zoneUIN) : super(pszName, Pollable::STATUS | Pollable::CONFIGURATION | Pollable::AUTOBIND), AutoBindTarget(this), m_syncNetworks(0, 8, Ownership::True)
{
	m_clusterType = 0;
	m_dwNumResources = 0;
	m_pResourceList = nullptr;
	m_zoneUIN = zoneUIN;
}

/**
 * Destructor
 */
Cluster::~Cluster()
{
	MemFree(m_pResourceList);
}

/**
 * Create object from database data
 */
bool Cluster::loadFromDatabase(DB_HANDLE hdb, uint32_t id, DB_STATEMENT *preparedStatements)
{
	TCHAR szQuery[256];
   bool bResult = false;
	DB_RESULT hResult;
	int i, nRows;

   m_id = id;

   if (!loadCommonProperties(hdb, preparedStatements) || !super::loadFromDatabase(hdb, id, preparedStatements))
      return false;

   if (Pollable::loadFromDatabase(hdb, m_id))
   {
      if (static_cast<uint32_t>(time(nullptr) - m_configurationPollState.getLastCompleted()) < getCustomAttributeAsUInt32(_T("SysConfig:Objects.ConfigurationPollingInterval"), g_configurationPollingInterval))
         m_runtimeFlags |= ODF_CONFIGURATION_POLL_PASSED;
   }

	_sntprintf(szQuery, 256, _T("SELECT cluster_type,zone_guid FROM clusters WHERE id=%d"), (int)m_id);
	hResult = DBSelect(hdb, szQuery);
	if (hResult == nullptr)
		return false;

	m_clusterType = DBGetFieldULong(hResult, 0, 0);
	m_zoneUIN = DBGetFieldULong(hResult, 0, 1);
	DBFreeResult(hResult);

   // Load DCI and access list
   loadACLFromDB(hdb, preparedStatements);
   loadItemsFromDB(hdb, preparedStatements);
   for(i = 0; i < m_dcObjects.size(); i++)
      if (!m_dcObjects.get(i)->loadThresholdsFromDB(hdb, preparedStatements))
         return false;
   loadDCIListForCleanup(hdb);

   if (!m_isDeleted)
   {
		// Load member nodes
		_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT node_id FROM cluster_members WHERE cluster_id=%d"), m_id);
		hResult = DBSelect(hdb, szQuery);
		if (hResult != nullptr)
		{
			nRows = DBGetNumRows(hResult);
			for(i = 0; i < nRows; i++)
			{
				uint32_t nodeId = DBGetFieldULong(hResult, i, 0);
				shared_ptr<NetObj> node = FindObjectById(nodeId, OBJECT_NODE);
				if (node != nullptr)
				{
				   linkObjects(self(), node);
				}
				else
				{
               nxlog_write(NXLOG_ERROR, _T("Inconsistent database: cluster object %s [%u] has reference to non-existent node object [%u]"), m_name, m_id, nodeId);
					break;
				}
			}
			if (i == nRows)
				bResult = true;
			DBFreeResult(hResult);
		}

		// Load sync net list
		if (bResult)
		{
			_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT subnet_addr,subnet_mask FROM cluster_sync_subnets WHERE cluster_id=%d"), m_id);
			hResult = DBSelect(hdb, szQuery);
			if (hResult != nullptr)
			{
				int count = DBGetNumRows(hResult);
				for(i = 0; i < count; i++)
				{
               InetAddress *addr = new InetAddress(DBGetFieldInetAddr(hResult, i, 0));
               addr->setMaskBits(DBGetFieldLong(hResult, i, 1));
               m_syncNetworks.add(addr);
				}
				DBFreeResult(hResult);
			}
			else
			{
				bResult = false;
			}
		}

		// Load resources
		if (bResult)
		{
			_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT resource_id,resource_name,ip_addr,current_owner FROM cluster_resources WHERE cluster_id=%d"), m_id);
			hResult = DBSelect(hdb, szQuery);
			if (hResult != NULL)
			{
				m_dwNumResources = DBGetNumRows(hResult);
				if (m_dwNumResources > 0)
				{
					m_pResourceList = (CLUSTER_RESOURCE *)malloc(sizeof(CLUSTER_RESOURCE) * m_dwNumResources);
					for(i = 0; i < (int)m_dwNumResources; i++)
					{
						m_pResourceList[i].dwId = DBGetFieldULong(hResult, i, 0);
						DBGetField(hResult, i, 1, m_pResourceList[i].szName, MAX_DB_STRING);
						m_pResourceList[i].ipAddr = DBGetFieldInetAddr(hResult, i, 2);
						m_pResourceList[i].dwCurrOwner = DBGetFieldULong(hResult, i, 3);
					}
				}
				DBFreeResult(hResult);
			}
			else
			{
				bResult = false;
			}
		}

	   if (bResult)
	      bResult = AutoBindTarget::loadFromDatabase(hdb, m_id);
	}
   else
   {
      bResult = true;
   }

   return bResult;
}

/**
 * Called by client session handler to check if threshold summary should be shown for this object.
 */
bool Cluster::showThresholdSummary() const
{
   return true;
}

/**
 * Save object to database
 */
bool Cluster::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);

   if (success && (m_modified & MODIFY_OTHER))
   {
      static const wchar_t *columns[] = { L"cluster_type", L"zone_guid", nullptr };
      DB_STATEMENT hStmt = DBPrepareMerge(hdb, L"clusters", L"id", m_id, columns);
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_clusterType);
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_zoneUIN);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_id);
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
		// Save cluster members list
      success = executeQueryOnObject(hdb, L"DELETE FROM cluster_members WHERE cluster_id=?");
      if (success)
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, L"INSERT INTO cluster_members (cluster_id,node_id) VALUES (?,?)");
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            readLockChildList();
            for(int i = 0; (i < getChildList().size()) && success; i++)
            {
               if (getChildList().get(i)->getObjectClass() != OBJECT_NODE)
                  continue;

               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, getChildList().get(i)->getId());
               success = DBExecute(hStmt);
            }
            unlockChildList();
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
         }
		}
   }

   if (success && (m_modified & MODIFY_OTHER))
   {
		// Save sync net list
      success = executeQueryOnObject(hdb, _T("DELETE FROM cluster_sync_subnets WHERE cluster_id=?"));
      if (success && !m_syncNetworks.isEmpty())
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO cluster_sync_subnets (cluster_id,subnet_addr,subnet_mask) VALUES (?,?,?)"), m_syncNetworks.size() > 1);
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            lockProperties();
            for(int i = 0; (i < m_syncNetworks.size()) && success; i++)
            {
               const InetAddress *net = m_syncNetworks.get(i);
               DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, net->toString(), DB_BIND_TRANSIENT);
               DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, net->getMaskBits());
               success = DBExecute(hStmt);
            }
            unlockProperties();
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
         }
		}
   }

   if (success && (m_modified & MODIFY_CLUSTER_RESOURCES))
   {
		// Save resource list
      success = executeQueryOnObject(hdb, _T("DELETE FROM cluster_resources WHERE cluster_id=?"));
      if (success)
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO cluster_resources (cluster_id,resource_id,resource_name,ip_addr,current_owner) VALUES (?,?,?,?,?)"));
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            lockProperties();
				for(uint32_t i = 0; (i < m_dwNumResources) && success; i++)
				{
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_pResourceList[i].dwId);
		         DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_pResourceList[i].szName, DB_BIND_STATIC);
               DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_pResourceList[i].ipAddr.toString(), DB_BIND_TRANSIENT);
               DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, m_pResourceList[i].dwCurrOwner);
               success = DBExecute(hStmt);
				}
				unlockProperties();
				DBFreeStatement(hStmt);
			}
			else
			{
	         success = false;
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
bool Cluster::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if (success)
   {
      success = executeQueryOnObject(hdb, _T("DELETE FROM clusters WHERE id=?"));
      if (success)
         success = executeQueryOnObject(hdb, _T("DELETE FROM cluster_members WHERE cluster_id=?"));
      if (success)
         success = executeQueryOnObject(hdb, _T("DELETE FROM cluster_sync_subnets WHERE cluster_id=?"));
      if (success)
         success = AutoBindTarget::deleteFromDatabase(hdb);
   }
   return success;
}

/**
 * Create NXCP message with object's data
 */
void Cluster::fillMessageLocked(NXCPMessage *msg, uint32_t userId)
{
	super::fillMessageLocked(msg, userId);
   msg->setField(VID_CLUSTER_TYPE, m_clusterType);
	msg->setField(VID_ZONE_UIN, m_zoneUIN);

   msg->setField(VID_NUM_SYNC_SUBNETS, (UINT32)m_syncNetworks.size());
   uint32_t fieldId = VID_SYNC_SUBNETS_BASE;
   for(int i = 0; i < m_syncNetworks.size(); i++)
      msg->setField(fieldId++, *m_syncNetworks.get(i));

   msg->setField(VID_NUM_RESOURCES, m_dwNumResources);
   fieldId = VID_RESOURCE_LIST_BASE;
	for(uint32_t i = 0; i < m_dwNumResources; i++, fieldId += 6)
	{
		msg->setField(fieldId++, m_pResourceList[i].dwId);
		msg->setField(fieldId++, m_pResourceList[i].szName);
		msg->setField(fieldId++, m_pResourceList[i].ipAddr);
		msg->setField(fieldId++, m_pResourceList[i].dwCurrOwner);
	}
}

/**
 * Create NXCP message with object's data
 */
void Cluster::fillMessageUnlocked(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageUnlocked(msg, userId);
   AutoBindTarget::fillMessage(msg);
}

/**
 * Change cluster's zone
 */
void Cluster::changeZone(uint32_t newZoneUIN)
{
   lockProperties();
   m_zoneUIN = newZoneUIN;
   setModified(MODIFY_OTHER);
   unlockProperties();

   readLockChildList();
   SharedObjectArray<NetObj> childList = SharedObjectArray<NetObj>(getChildList());
   unlockChildList();

   for(int i = 0; i < childList.size(); i++)
   {
      Node *node = static_cast<Node*>(childList.get(i));
      node->changeZone(newZoneUIN);
   }
}

/**
 * Modify object from message
 */
uint32_t Cluster::modifyFromMessageInternal(const NXCPMessage& msg, ClientSession *session)
{
   // Change cluster type
   if (msg.isFieldExist(VID_CLUSTER_TYPE))
      m_clusterType = msg.getFieldAsUInt32(VID_CLUSTER_TYPE);

   // Change sync subnets
   if (msg.isFieldExist(VID_NUM_SYNC_SUBNETS))
	{
      m_syncNetworks.clear();
      int count = msg.getFieldAsInt32(VID_NUM_SYNC_SUBNETS);
      uint32_t fieldId = VID_SYNC_SUBNETS_BASE;
      for(int i = 0; i < count; i++)
      {
         m_syncNetworks.add(new InetAddress(msg.getFieldAsInetAddress(fieldId++)));
      }
	}

   // Change resource list
   if (msg.isFieldExist(VID_NUM_RESOURCES))
	{
      uint32_t count = msg.getFieldAsUInt32(VID_NUM_RESOURCES);
		if (count > 0)
		{
			auto pList = MemAllocArray<CLUSTER_RESOURCE>(count);
			for(uint32_t i = 0, fieldId = VID_RESOURCE_LIST_BASE; i < count; i++, fieldId += 7)
			{
				pList[i].dwId = msg.getFieldAsUInt32(fieldId++);
				msg.getFieldAsString(fieldId++, pList[i].szName, MAX_DB_STRING);
				pList[i].ipAddr = msg.getFieldAsInetAddress(fieldId++);
			}

			// Update current owner information in existing resources
			for(uint32_t i = 0; i < m_dwNumResources; i++)
			{
				for(uint32_t j = 0; j < count; j++)
				{
					if (m_pResourceList[i].dwId == pList[j].dwId)
					{
						pList[j].dwCurrOwner = m_pResourceList[i].dwCurrOwner;
						break;
					}
				}
			}

			// Replace list
			MemFree(m_pResourceList);
			m_pResourceList = pList;
		}
		else
		{
			MemFreeAndNull(m_pResourceList);
		}
		m_dwNumResources = count;
	}

   AutoBindTarget::modifyFromMessage(msg);

   return super::modifyFromMessageInternal(msg, session);
}

/**
 * Check if given address is within sync network
 */
bool Cluster::isSyncAddr(const InetAddress& addr)
{
	bool bRet = false;

	lockProperties();
	for(int i = 0; i < m_syncNetworks.size(); i++)
	{
		if (m_syncNetworks.get(i)->contains(addr))
		{
			bRet = true;
			break;
		}
	}
	unlockProperties();
	return bRet;
}

/**
 * Check if given address is a resource address
 */
bool Cluster::isVirtualAddr(const InetAddress& addr)
{
	bool bRet = false;

	lockProperties();
	for(uint32_t i = 0; i < m_dwNumResources; i++)
	{
      if (m_pResourceList[i].ipAddr.equals(addr))
		{
			bRet = true;
			break;
		}
	}
	unlockProperties();
	return bRet;
}

/**
 * Configuration poll
 */
void Cluster::configurationPoll(PollerInfo *poller, ClientSession *session, uint32_t requestId)
{
   lockProperties();
   if (m_isDeleteInitiated || IsShutdownInProgress())
   {
      m_configurationPollState.complete(0);
      unlockProperties();
      return;
   }
   unlockProperties();

   poller->setStatus(_T("wait for lock"));
   pollerLock(configuration);

   if (IsShutdownInProgress())
   {
      pollerUnlock();
      return;
   }

   m_pollRequestor = session;
   m_pollRequestId = requestId;

   poller->setStatus(_T("hook"));
   executeHookScript(_T("ConfigurationPoll"));

   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ClusterConfPoll(%s): applying templates"), m_name);
   if (ConfigReadBoolean(_T("Objects.Clusters.TemplateAutoApply"), false))
      applyTemplates();

   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ClusterConfPoll(%s): Updating container bindings"), m_name);
   if (ConfigReadBoolean(_T("Objects.Clusters.ContainerAutoBind"), false))
      updateContainerMembership();

   sendPollerMsg(_T("Bind cluster to subnets\r\n"));
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("ClusterConfPoll(%s): Bind cluster to subnets"), m_name);
   readLockChildList();
   HashSet<uint32_t> parentIds;
   SharedObjectArray<NetObj> addList;
   for(int i = 0; i < getChildList().size(); i++)
   {
      shared_ptr<NetObj> child = getChildList().getShared(i);
      unique_ptr<SharedObjectArray<NetObj>> parents = child->getParents(OBJECT_SUBNET);
      for (shared_ptr<NetObj> parent : *parents)
      {
         parentIds.put(parent->getId());
         if (!isParent(parent->getId()))
         {
            addList.add(parent);
         }
      }
   }
   unlockChildList();
   for (auto parent : addList)
   {
      linkObjects(parent, self());
      parent->calculateCompoundStatus();
      sendPollerMsg(_T("   Bind to subnet %s\r\n"), parent->getName());
      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 4, _T("ClusterConfPoll(%s): bind cluster %d \"%s\" to subnet %d \"%s\""),
            m_name, m_id, m_name, parent->getId(), parent->getName());
   }

   unique_ptr<SharedObjectArray<NetObj>> parents = getParents(OBJECT_SUBNET);
   for(int i = 0; i < parents->size(); i++)
   {
      shared_ptr<NetObj> parent = parents->getShared(i);
      if (!parentIds.contains(parent->getId()))
      {
         unlinkObjects(parent.get(), this);
         parent->calculateCompoundStatus();
         sendPollerMsg(_T("   Unbind from subnet %s\r\n"), parent->getName());
         nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 4, _T("ClusterConfPoll(%s): unbind cluster %d \"%s\" from subnet %d \"%s\""),
               m_name, m_id, m_name, parent->getId(), parent->getName());
      }
   }

   sendPollerMsg(_T("Configuration poll finished\r\n"));
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 6, _T("ClusterConfPoll(%s): finished"), m_name);

   lockProperties();
   m_runtimeFlags &= ~ODF_CONFIGURATION_POLL_PENDING;
   m_runtimeFlags |= ODF_CONFIGURATION_POLL_PASSED;
   unlockProperties();

   pollerUnlock();
}

/**
 * Status poll
 */
void Cluster::statusPoll(PollerInfo *poller, ClientSession *session, uint32_t requestId)
{
   lockProperties();
   if (m_isDeleteInitiated || IsShutdownInProgress())
   {
      m_statusPollState.complete(0);
      unlockProperties();
      return;
   }
   unlockProperties();

   poller->setStatus(_T("wait for lock"));
   pollerLock(status);

   if (IsShutdownInProgress())
   {
      pollerUnlock();
      return;
   }

   uint32_t modified = 0;

   // Create polling list
	SharedObjectArray<NetObj> pollList(getChildList().size(), 16);
   readLockChildList();
   int i;
   for(i = 0; i < getChildList().size(); i++)
   {
      shared_ptr<NetObj> object = getChildList().getShared(i);
      if ((object->getStatus() != STATUS_UNMANAGED) && object->isPollable() && object->getAsPollable()->isStatusPollAvailable())
      {
         object->getAsPollable()->lockForStatusPoll();
         pollList.add(object);
      }
   }
   unlockChildList();

	// Perform status poll on all member nodes
   m_pollRequestor = session;
   m_pollRequestId = requestId;

   sendPollerMsg(_T("Polling member nodes\r\n"));
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ClusterStatusPoll(%s): Polling member nodes"), m_name);
	bool allDown = true;
	for(i = 0; i < pollList.size(); i++)
	{
		NetObj *object = pollList.get(i);
		poller->setStatus(_T("child poll"));
		object->getAsPollable()->doStatusPoll(poller, session, requestId);
		if ((object->getObjectClass() == OBJECT_NODE) && !static_cast<Node*>(object)->isDown())
			allDown = false;
	}

	if (allDown)
	{
		if (!(m_state & CLSF_DOWN))
		{
		   m_state |= CLSF_DOWN;
			PostSystemEvent(EVENT_CLUSTER_DOWN, m_id);
		}
	}
	else
	{
		if (m_state & CLSF_DOWN)
		{
		   m_state &= ~CLSF_DOWN;
			PostSystemEvent(EVENT_CLUSTER_UP, m_id);
		}
	}

	// Check for cluster resource movement
	if (!allDown)
	{
      BYTE *resourceFound = reinterpret_cast<BYTE*>(MemAllocLocal(m_dwNumResources));
      memset(resourceFound, 0, m_dwNumResources);

      poller->setStatus(_T("resource poll"));
	   sendPollerMsg(_T("Polling resources\r\n"));
	   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ClusterStatusPoll(%s): polling resources"), m_name);
		for(i = 0; i < pollList.size(); i++)
		{
		   if (pollList.get(i)->getObjectClass() != OBJECT_NODE)
		      continue;

		   Node *node = static_cast<Node*>(pollList.get(i));
		   InterfaceList *ifList = node->getInterfaceList();
			if (ifList != nullptr)
			{
				lockProperties();
				for(int j = 0; j < ifList->size(); j++)
				{
					for(uint32_t k = 0; k < m_dwNumResources; k++)
					{
                  if (ifList->get(j)->hasAddress(m_pResourceList[k].ipAddr))
						{
							if (m_pResourceList[k].dwCurrOwner != node->getId())
							{
						      sendPollerMsg(_T("Resource %s owner changed\r\n"), m_pResourceList[k].szName);
						      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ClusterStatusPoll(%s): Resource %s owner changed"),
											 m_name, m_pResourceList[k].szName);

								// Resource moved or go up
								if (m_pResourceList[k].dwCurrOwner == 0)
								{
									// Resource up
                           EventBuilder(EVENT_CLUSTER_RESOURCE_UP, m_id).
                              param(_T("resourceId"), m_pResourceList[k].dwId).
                              param(_T("resourceName"), m_pResourceList[k].szName).
                              param(_T("newOwnerNodeId"), node->getId()).
                              param(_T("newOwnerNodeName"), node->getName()).post();
								}
								else
								{
									// Moved
									shared_ptr<NetObj> pObject = FindObjectById(m_pResourceList[k].dwCurrOwner);
                           EventBuilder(EVENT_CLUSTER_RESOURCE_MOVED, m_id).
                              param(_T("resourceId"), m_pResourceList[k].dwId).
                              param(_T("resourceName"), m_pResourceList[k].szName).
                              param(_T("previousOwnerNodeId"), m_pResourceList[k].dwCurrOwner).
                              param(_T("previousOwnerNodeName"), (pObject != NULL) ? pObject->getName() : _T("<unknown>")).
                              param(_T("newOwnerNodeId"), node->getId()).
                              param(_T("newOwnerNodeName"), node->getName()).post();

								}
								m_pResourceList[k].dwCurrOwner = node->getId();
								modified |= MODIFY_CLUSTER_RESOURCES;
							}
							resourceFound[k] = 1;
						}
					}
				}
				unlockProperties();
				delete ifList;
			}
			else
			{
		      sendPollerMsg(_T("Cannot get interface list from %s\r\n"), node->getName());
		      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ClusterStatusPoll(%s): Cannot get interface list from %s"),
							 m_name, node->getName());
			}
		}

		// Check for missing virtual addresses
		lockProperties();
		for(i = 0; i < (int)m_dwNumResources; i++)
		{
			if ((!resourceFound[i]) && (m_pResourceList[i].dwCurrOwner != 0))
			{
				shared_ptr<NetObj> pObject = FindObjectById(m_pResourceList[i].dwCurrOwner);
            EventBuilder(EVENT_CLUSTER_RESOURCE_DOWN, m_id)
               .param(_T("resourceId"), m_pResourceList[i].dwId)
               .param(_T("resourceName"), m_pResourceList[i].szName)
               .param(_T("lastOwnerNodeId"), m_pResourceList[i].dwCurrOwner)
               .param(_T("lastOwnerNodeName"), (pObject != nullptr) ? pObject->getName() : _T("<unknown>"))
               .post();
				m_pResourceList[i].dwCurrOwner = 0;
            modified |= MODIFY_CLUSTER_RESOURCES;
			}
		}
		unlockProperties();
		MemFreeLocal(resourceFound);
	}

   // Execute hook script
   poller->setStatus(_T("hook"));
   executeHookScript(_T("StatusPoll"));

   calculateCompoundStatus(true);
   poller->setStatus(_T("cleanup"));

	lockProperties();
	if (modified != 0)
		setModified(modified);
	unlockProperties();

   sendPollerMsg(_T("Status poll finished\r\n"));
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ClusterStatusPoll(%s): finished"), m_name);

   pollerUnlock();
}

/**
 * Check if node is current owner of resource
 */
bool Cluster::isResourceOnNode(uint32_t resourceId, uint32_t nodeId)
{
	bool bRet = false;

	lockProperties();
	for(uint32_t i = 0; i < m_dwNumResources; i++)
	{
		if (m_pResourceList[i].dwId == resourceId)
		{
			if (m_pResourceList[i].dwCurrOwner == nodeId)
				bRet = true;
			break;
		}
	}
	unlockProperties();
	return bRet;
}

/**
 * Get node ID of resource owner
 */
uint32_t Cluster::getResourceOwnerInternal(uint32_t id, const TCHAR *name)
{
   uint32_t ownerId = 0;
   lockProperties();
   for(uint32_t i = 0; i < m_dwNumResources; i++)
   {
      if (((name != NULL) && !_tcsicmp(name, m_pResourceList[i].szName)) ||
          (m_pResourceList[i].dwId == id))
      {
         ownerId = m_pResourceList[i].dwCurrOwner;
         break;
      }
   }
   unlockProperties();
   return ownerId;
}

/**
 * Collect aggregated data for cluster nodes - single value
 */
uint32_t Cluster::collectAggregatedData(const DCItem& dci, TCHAR *buffer)
{
   readLockChildList();
   ObjectArray<ItemValue> values(getChildList().size(), 32, Ownership::True);
   for(int i = 0; i < getChildList().size(); i++)
   {
      if (getChildList().get(i)->getObjectClass() != OBJECT_NODE)
         continue;

      Node *node = static_cast<Node*>(getChildList().get(i));
      shared_ptr<DCObject> dco = node->getDCObjectByTemplateId(dci.getId(), 0);
      if ((dco != nullptr) &&
          (dco->getType() == DCO_TYPE_ITEM) &&
          (dco->getStatus() == ITEM_STATUS_ACTIVE) &&
          ((dco->getErrorCount() == 0) || dco->isAggregateWithErrors()) &&
          dco->matchClusterResource())
      {
         ItemValue *v = static_cast<DCItem*>(dco.get())->getInternalLastValue();
         if (v != nullptr)
         {
            // Immediately after server startup cache may be filled with placeholder values
            // They can be identified by timestamp equals 1
            if (v->getTimeStamp().asMilliseconds() > 1)
               values.add(v);
            else
               delete v;
         }
      }
   }
   unlockChildList();

   uint32_t rcc = DCE_SUCCESS;
   if (!values.isEmpty())
   {
      ItemValue result;
      switch(dci.getAggregationFunction())
      {
         case DCF_FUNCTION_AVG:
            CalculateItemValueAverage(&result, dci.getTransformedDataType(), values.getBuffer(), values.size());
            break;
         case DCF_FUNCTION_MAX:
            CalculateItemValueMax(&result, dci.getTransformedDataType(), values.getBuffer(), values.size());
            break;
         case DCF_FUNCTION_MIN:
            CalculateItemValueMin(&result, dci.getTransformedDataType(), values.getBuffer(), values.size());
            break;
         case DCF_FUNCTION_SUM:
            CalculateItemValueTotal(&result, dci.getTransformedDataType(), values.getBuffer(), values.size());
            break;
         default:
            rcc = DCE_NOT_SUPPORTED;
            break;
      }
      _tcslcpy(buffer, result.getString(), MAX_RESULT_LENGTH);
   }
   else
   {
      rcc = DCE_COLLECTION_ERROR;
   }

   return rcc;
}

/**
 * Collect aggregated data for cluster nodes - table
 */
uint32_t Cluster::collectAggregatedData(const DCTable& dci, shared_ptr<Table> *result)
{
   readLockChildList();
   SharedObjectArray<Table> values(getChildList().size());
   for(int i = 0; i < getChildList().size(); i++)
   {
      if (getChildList().get(i)->getObjectClass() != OBJECT_NODE)
         continue;

      Node *node = static_cast<Node*>(getChildList().get(i));
      shared_ptr<DCObject> dco = node->getDCObjectByTemplateId(dci.getId(), 0);
      if ((dco != NULL) &&
          (dco->getType() == DCO_TYPE_TABLE) &&
          (dco->getStatus() == ITEM_STATUS_ACTIVE) &&
          ((dco->getErrorCount() == 0) || dco->isAggregateWithErrors()) &&
          dco->matchClusterResource())
      {
         shared_ptr<Table> v = static_cast<DCTable*>(dco.get())->getLastValue();
         if (v != nullptr)
            values.add(v);
      }
   }
   unlockChildList();

   uint32_t rcc = DCE_SUCCESS;
   if (!values.isEmpty())
   {
      *result = make_shared<Table>(*values.get(0));
      for(int i = 1; i < values.size(); i++)
         dci.mergeValues(result->get(), values.get(i), i);
   }
   else
   {
      rcc = DCE_COLLECTION_ERROR;
   }

   return rcc;
}

/**
 * Process template removal
 */
void Cluster::onTemplateRemove(const shared_ptr<DataCollectionOwner>& templateObject, bool removeDCI)
{
   super::onTemplateRemove(templateObject, removeDCI);
   queueUpdate();
}

/**
 * Called when data collection configuration changed
 */
void Cluster::onDataCollectionChange()
{
   queueUpdate();
}

/**
 * Create NXSL object for this object
 */
NXSL_Value *Cluster::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(vm->createObject(&g_nxslClusterClass, new shared_ptr<Cluster>(self())));
}

/**
 * Get cluster nodes as NXSL array
 */
NXSL_Value *Cluster::getNodesForNXSL(NXSL_VM *vm)
{
   NXSL_Array *nodes = new NXSL_Array(vm);

   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *object = getChildList().get(i);
      if (object->getObjectClass() == OBJECT_NODE)
      {
         nodes->append(object->createNXSLObject(vm));
      }
   }
   unlockChildList();

   return vm->createValue(nodes);
}

/**
 * Serialize object to JSON
 */
json_t *Cluster::toJson()
{
   json_t *root = super::toJson();
   AutoBindTarget::toJson(root);

   lockProperties();
   json_object_set_new(root, "clusterType", json_integer(m_clusterType));
   json_object_set_new(root, "syncNetworks", json_object_array(m_syncNetworks));
   json_object_set_new(root, "zoneUIN", json_integer(m_zoneUIN));

   json_t *resources = json_array();
   for(UINT32 i = 0; i < m_dwNumResources; i++)
   {
      json_t *r = json_object();
      json_object_set_new(r, "id", json_integer(m_pResourceList[i].dwId));
      json_object_set_new(r, "name", json_string_t(m_pResourceList[i].szName));
      json_object_set_new(r, "address", m_pResourceList[i].ipAddr.toJson());
      json_object_set_new(r, "currentOwner", json_integer(m_pResourceList[i].dwCurrOwner));
      json_array_append_new(resources, r);
   }
   json_object_set_new(root, "resources", resources);
   unlockProperties();

   return root;
}

/**
 * Add node to the cluster
 */
bool Cluster::addNode(const shared_ptr<Node>& node)
{
   if (m_zoneUIN != node->getZoneUIN())
      return false;

   applyToTarget(node);
   node->setRecheckCapsFlag();
   node->forceConfigurationPoll();
   return true;
}

/**
 * Remove node form cluster
 */
void Cluster::removeNode(const shared_ptr<Node>& node)
{
   queueRemoveFromTarget(node->getId(), true);
   node->setRecheckCapsFlag();
   node->forceConfigurationPoll();
}

/**
 * Perform automatic object binding
 */
void Cluster::autobindPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId)
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
   nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 5, _T("Starting autobind poll of cluster %s [%u]"), m_name, m_id);
   poller->setStatus(_T("checking objects"));

   if (!isAutoBindEnabled())
   {
      sendPollerMsg(_T("Automatic object binding is disabled\r\n"));
      nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 5, _T("Finished autobind poll of cluster %s [%u])"), m_name, m_id);
      pollerUnlock();
      return;
   }

   NXSL_VM *cachedFilterVM = nullptr;
   unique_ptr<SharedObjectArray<NetObj>> nodes = g_idxNodeById.getObjects();
   for (int i = 0; i < nodes->size(); i++)
   {
      shared_ptr<NetObj> node = nodes->getShared(i);

      AutoBindDecision decision = isApplicable(&cachedFilterVM, node, this);
      if ((decision == AutoBindDecision_Ignore) || ((decision == AutoBindDecision_Unbind) && !isAutoUnbindEnabled()))
         continue;   // Decision cannot affect checks

      if ((decision == AutoBindDecision_Bind) && !isDirectChild(node->getId()))
      {
         sendPollerMsg(_T("   Adding node %s\r\n"), node->getName());
         if (addNode(static_pointer_cast<Node>(node)))
         {
            nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 4, _T("Cluster::autobindPoll(): binding node \"%s\" [%u] to cluster \"%s\" [%u]"), node->getName(), node->getId(), m_name, m_id);
            EventBuilder(EVENT_CLUSTER_AUTOADD, g_dwMgmtNode)
               .param(_T("nodeId"), node->getId(), EventBuilder::OBJECT_ID_FORMAT)
               .param(_T("nodeName"), node->getName())
               .param(_T("clusterId"), m_id, EventBuilder::OBJECT_ID_FORMAT)
               .param(_T("clusterName"), m_name)
               .post();

            calculateCompoundStatus();
         }
         else
         {
            sendPollerMsg(_T("   Node not added - node and cluster are in different zones\r\n"), node->getName());
            nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 4, _T("Cluster::autobindPoll(): node \"%s\" [%u] not added to cluster \"%s\" [%u] due to zone mismatch"), node->getName(), node->getId(), m_name, m_id);
         }
      }
      else if ((decision == AutoBindDecision_Unbind) && isDirectChild(node->getId()))
      {
         sendPollerMsg(_T("   Removing node %s\r\n"), node->getName());
         nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 4, _T("Cluster::autobindPoll(): removing node \"%s\" [%u] from cluster \"%s\" [%u]"), node->getName(), node->getId(), m_name, m_id);

         removeNode(static_pointer_cast<Node>(node));
         EventBuilder(EVENT_CLUSTER_AUTOREMOVE, g_dwMgmtNode)
            .param(_T("nodeId"), node->getId(), EventBuilder::OBJECT_ID_FORMAT)
            .param(_T("nodeName"), node->getName())
            .param(_T("clusterId"), m_id, EventBuilder::OBJECT_ID_FORMAT)
            .param(_T("clusterName"), m_name)
            .post();
         calculateCompoundStatus();
      }
   }
   delete cachedFilterVM;

   pollerUnlock();
   nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 5, _T("Finished autobind poll of container %s [%u])"), m_name, m_id);
}

/**
 * Enter maintenance mode
 */
void Cluster::enterMaintenanceMode(uint32_t userId, const TCHAR *comments)
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
void Cluster::leaveMaintenanceMode(uint32_t userId)
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
