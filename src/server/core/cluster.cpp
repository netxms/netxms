/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
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
Cluster::Cluster() : super()
{
	m_dwClusterType = 0;
   m_syncNetworks = new ObjectArray<InetAddress>(8, 8, Ownership::True);
	m_dwNumResources = 0;
	m_pResourceList = NULL;
	m_zoneUIN = 0;
}

/**
 * Cluster class new object constructor
 */
Cluster::Cluster(const TCHAR *pszName, int32_t zoneUIN) : super(pszName)
{
	m_dwClusterType = 0;
   m_syncNetworks = new ObjectArray<InetAddress>(8, 8, Ownership::True);
	m_dwNumResources = 0;
	m_pResourceList = NULL;
	m_zoneUIN = zoneUIN;
}

/**
 * Destructor
 */
Cluster::~Cluster()
{
   delete m_syncNetworks;
	MemFree(m_pResourceList);
}

/**
 * Create object from database data
 */
bool Cluster::loadFromDatabase(DB_HANDLE hdb, UINT32 dwId)
{
	TCHAR szQuery[256];
   bool bResult = false;
	DB_RESULT hResult;
	UINT32 dwNodeId;
	int i, nRows;

   m_id = dwId;

   if (!loadCommonProperties(hdb) || !super::loadFromDatabase(hdb, dwId))
   {
      nxlog_debug(2, _T("Cannot load common properties for cluster object %d"), dwId);
      return false;
   }

	_sntprintf(szQuery, 256, _T("SELECT cluster_type,zone_guid FROM clusters WHERE id=%d"), (int)m_id);
	hResult = DBSelect(hdb, szQuery);
	if (hResult == NULL)
		return false;

	m_dwClusterType = DBGetFieldULong(hResult, 0, 0);
	m_zoneUIN = DBGetFieldULong(hResult, 0, 1);
	DBFreeResult(hResult);

   // Load DCI and access list
   loadACLFromDB(hdb);
   loadItemsFromDB(hdb);
   for(i = 0; i < m_dcObjects->size(); i++)
      if (!m_dcObjects->get(i)->loadThresholdsFromDB(hdb))
         return false;
   loadDCIListForCleanup(hdb);

   if (!m_isDeleted)
   {
		// Load member nodes
		_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT node_id FROM cluster_members WHERE cluster_id=%d"), m_id);
		hResult = DBSelect(hdb, szQuery);
		if (hResult != NULL)
		{
			nRows = DBGetNumRows(hResult);
			for(i = 0; i < nRows; i++)
			{
				dwNodeId = DBGetFieldULong(hResult, i, 0);
				shared_ptr<NetObj> object = FindObjectById(dwNodeId, OBJECT_NODE);
				if (object != nullptr)
				{
               addChild(object);
               object->addParent(self());
				}
				else
				{
               nxlog_write(NXLOG_ERROR, _T("Inconsistent database: cluster object %s [%u] has reference to non-existent node object [%u]"), m_name, m_id, dwNodeId);
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
			if (hResult != NULL)
			{
				int count = DBGetNumRows(hResult);
				for(i = 0; i < count; i++)
				{
               InetAddress *addr = new InetAddress(DBGetFieldInetAddr(hResult, i, 0));
               addr->setMaskBits(DBGetFieldLong(hResult, i, 1));
               m_syncNetworks->add(addr);
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
   lockProperties();
   bool success = saveCommonProperties(hdb);

   if (success)
      success = super::saveToDatabase(hdb);

   if (!success)
   {
      unlockProperties();
      return false;
   }


   DB_STATEMENT hStmt;
   if (IsDatabaseRecordExist(hdb, _T("clusters"), _T("id"), m_id))
      hStmt = DBPrepare(hdb, _T("UPDATE clusters SET cluster_type=?,zone_guid=? WHERE id=?"));
	else
      hStmt = DBPrepare(hdb, _T("INSERT INTO clusters (cluster_type,zone_guid,id) VALUES (?,?,?)"));
   if (hStmt != NULL)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwClusterType);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_zoneUIN);
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_id);
      success = DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
   else
   {
      success = false;
   }

   if (success)
   {
      success = saveACLToDB(hdb);
   }
   unlockProperties();

   if (success && (m_modified & MODIFY_DATA_COLLECTION))
   {
		readLockDciAccess();
      for(int i = 0; (i < m_dcObjects->size()) && success; i++)
         success = m_dcObjects->get(i)->saveToDatabase(hdb);
		unlockDciAccess();
      if (success)
         success = saveDCIListForCleanup(hdb);
   }

   if (success)
   {
		// Save cluster members list
      hStmt = DBPrepare(hdb, _T("DELETE FROM cluster_members WHERE cluster_id=?"));
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }

      if (success)
      {
         hStmt = DBPrepare(hdb, _T("INSERT INTO cluster_members (cluster_id,node_id) VALUES (?,?)"));
         if (hStmt != NULL)
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

   if (success)
   {
		// Save sync net list
      hStmt = DBPrepare(hdb, _T("DELETE FROM cluster_sync_subnets WHERE cluster_id=?"));
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }

      if (success)
      {
         hStmt = DBPrepare(hdb, _T("INSERT INTO cluster_sync_subnets (cluster_id,subnet_addr,subnet_mask) VALUES (?,?,?)"));
         if (hStmt != NULL)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            lockProperties();
            for(int i = 0; (i < m_syncNetworks->size()) && success; i++)
            {
               const InetAddress *net = m_syncNetworks->get(i);
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
      hStmt = DBPrepare(hdb, _T("DELETE FROM cluster_resources WHERE cluster_id=?"));
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }

      if (success)
      {
         hStmt = DBPrepare(hdb, _T("INSERT INTO cluster_resources (cluster_id,resource_id,resource_name,ip_addr,current_owner) VALUES (?,?,?,?,?)"));
         if (hStmt != NULL)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            lockProperties();
				for(UINT32 i = 0; (i < m_dwNumResources) && success; i++)
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
   }
   return success;
}

/**
 * Create CSCP message with object's data
 */
void Cluster::fillMessageInternal(NXCPMessage *pMsg, UINT32 userId)
{
	UINT32 i, dwId;

	super::fillMessageInternal(pMsg, userId);
   pMsg->setField(VID_CLUSTER_TYPE, m_dwClusterType);
	pMsg->setField(VID_ZONE_UIN, m_zoneUIN);

   pMsg->setField(VID_NUM_SYNC_SUBNETS, (UINT32)m_syncNetworks->size());
   for(i = 0, dwId = VID_SYNC_SUBNETS_BASE; i < (UINT32)m_syncNetworks->size(); i++)
      pMsg->setField(dwId++, *(m_syncNetworks->get(i)));

   pMsg->setField(VID_NUM_RESOURCES, m_dwNumResources);
	for(i = 0, dwId = VID_RESOURCE_LIST_BASE; i < m_dwNumResources; i++, dwId += 6)
	{
		pMsg->setField(dwId++, m_pResourceList[i].dwId);
		pMsg->setField(dwId++, m_pResourceList[i].szName);
		pMsg->setField(dwId++, m_pResourceList[i].ipAddr);
		pMsg->setField(dwId++, m_pResourceList[i].dwCurrOwner);
	}
}

/**
 * Modify object from message
 */
UINT32 Cluster::modifyFromMessageInternal(NXCPMessage *pRequest)
{
   if (pRequest->isFieldExist(VID_FLAGS))
      m_flags = pRequest->getFieldAsUInt32(VID_FLAGS);

   // Change cluster type
   if (pRequest->isFieldExist(VID_CLUSTER_TYPE))
      m_dwClusterType = pRequest->getFieldAsUInt32(VID_CLUSTER_TYPE);

   // Change sync subnets
   if (pRequest->isFieldExist(VID_NUM_SYNC_SUBNETS))
	{
      m_syncNetworks->clear();
      int count = pRequest->getFieldAsInt32(VID_NUM_SYNC_SUBNETS);
      UINT32 fieldId = VID_SYNC_SUBNETS_BASE;
      for(int i = 0; i < count; i++)
      {
         m_syncNetworks->add(new InetAddress(pRequest->getFieldAsInetAddress(fieldId++)));
      }
	}

   // Change resource list
   if (pRequest->isFieldExist(VID_NUM_RESOURCES))
	{
		UINT32 i, j, dwId, dwCount;
		CLUSTER_RESOURCE *pList;

      dwCount = pRequest->getFieldAsUInt32(VID_NUM_RESOURCES);
		if (dwCount > 0)
		{
			pList = (CLUSTER_RESOURCE *)malloc(sizeof(CLUSTER_RESOURCE) * dwCount);
			memset(pList, 0, sizeof(CLUSTER_RESOURCE) * dwCount);
			for(i = 0, dwId = VID_RESOURCE_LIST_BASE; i < dwCount; i++, dwId += 7)
			{
				pList[i].dwId = pRequest->getFieldAsUInt32(dwId++);
				pRequest->getFieldAsString(dwId++, pList[i].szName, MAX_DB_STRING);
				pList[i].ipAddr = pRequest->getFieldAsInetAddress(dwId++);
			}

			// Update current owner information in existing resources
			for(i = 0; i < m_dwNumResources; i++)
			{
				for(j = 0; j < dwCount; j++)
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
		m_dwNumResources = dwCount;
	}

   return super::modifyFromMessageInternal(pRequest);
}

/**
 * Check if given address is within sync network
 */
bool Cluster::isSyncAddr(const InetAddress& addr)
{
	bool bRet = false;

	lockProperties();
	for(int i = 0; i < m_syncNetworks->size(); i++)
	{
		if (m_syncNetworks->get(i)->contain(addr))
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
	UINT32 i;
	bool bRet = false;

	lockProperties();
	for(i = 0; i < m_dwNumResources; i++)
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
void Cluster::configurationPoll(PollerInfo *poller, ClientSession *pSession, UINT32 dwRqId)
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

   m_pollRequestor = pSession;
   sendPollerMsg(dwRqId, _T("Applying templates\r\n"));
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ClusterConfPoll(%s): applying templates"), m_name);
   if (ConfigReadBoolean(_T("ClusterTemplateAutoApply"), false))
      applyUserTemplates();

   sendPollerMsg(dwRqId, _T("Updating container bindings\r\n"));
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ClusterConfPoll(%s): Updating container bindings"), m_name);
   if (ConfigReadBoolean(_T("ClusterContainerAutoBind"), false))
      updateContainerMembership();

   poller->setStatus(_T("hook"));
   executeHookScript(_T("ConfigurationPoll"));

   sendPollerMsg(dwRqId, _T("Configuration poll finished\r\n"));
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
void Cluster::statusPoll(PollerInfo *poller, ClientSession *pSession, UINT32 dwRqId)
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

   UINT32 modified = 0;

   // Create polling list
	SharedObjectArray<DataCollectionTarget> pollList(getChildList().size(), 16);
   readLockChildList();
   int i;
   for(i = 0; i < getChildList().size(); i++)
   {
      shared_ptr<NetObj> object = getChildList().getShared(i);
      if ((object->getStatus() != STATUS_UNMANAGED) && object->isDataCollectionTarget())
      {
         static_cast<DataCollectionTarget*>(object.get())->lockForStatusPoll();
         pollList.add(static_pointer_cast<DataCollectionTarget>(object));
      }
   }
   unlockChildList();

	// Perform status poll on all member nodes
   m_pollRequestor = pSession;
   sendPollerMsg(dwRqId, _T("Polling member nodes\r\n"));
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ClusterStatusPoll(%s): Polling member nodes"), m_name);
	bool allDown = true;
	for(i = 0; i < pollList.size(); i++)
	{
	   DataCollectionTarget *object = pollList.get(i);
		object->statusPollPollerEntry(poller, pSession, dwRqId);
		if ((object->getObjectClass() == OBJECT_NODE) && !static_cast<Node*>(object)->isDown())
			allDown = false;
	}

	if (allDown)
	{
		if (!(m_state & CLSF_DOWN))
		{
		   m_state |= CLSF_DOWN;
			PostSystemEvent(EVENT_CLUSTER_DOWN, m_id, nullptr);
		}
	}
	else
	{
		if (m_state & CLSF_DOWN)
		{
		   m_state &= ~CLSF_DOWN;
			PostSystemEvent(EVENT_CLUSTER_UP, m_id, nullptr);
		}
	}

	// Check for cluster resource movement
	if (!allDown)
	{
      BYTE *resourceFound = reinterpret_cast<BYTE*>(MemAllocLocal(m_dwNumResources));
      memset(resourceFound, 0, m_dwNumResources);

      poller->setStatus(_T("resource poll"));
	   sendPollerMsg(dwRqId, _T("Polling resources\r\n"));
	   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ClusterStatusPoll(%s): polling resources"), m_name);
		for(i = 0; i < pollList.size(); i++)
		{
		   if (pollList.get(i)->getObjectClass() != OBJECT_NODE)
		      continue;

		   Node *node = static_cast<Node*>(pollList.get(i));
		   InterfaceList *pIfList = node->getInterfaceList();
			if (pIfList != NULL)
			{
				lockProperties();
				for(int j = 0; j < pIfList->size(); j++)
				{
					for(UINT32 k = 0; k < m_dwNumResources; k++)
					{
                  if (pIfList->get(j)->hasAddress(m_pResourceList[k].ipAddr))
						{
							if (m_pResourceList[k].dwCurrOwner != node->getId())
							{
						      sendPollerMsg(dwRqId, _T("Resource %s owner changed\r\n"), m_pResourceList[k].szName);
						      nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ClusterStatusPoll(%s): Resource %s owner changed"),
											 m_name, m_pResourceList[k].szName);

								// Resource moved or go up
								if (m_pResourceList[k].dwCurrOwner == 0)
								{
									// Resource up
									PostSystemEvent(EVENT_CLUSTER_RESOURCE_UP, m_id, "dsds",
												 m_pResourceList[k].dwId, m_pResourceList[k].szName,
												 node->getId(), node->getName());
								}
								else
								{
									// Moved
									shared_ptr<NetObj> pObject = FindObjectById(m_pResourceList[k].dwCurrOwner);
									PostSystemEvent(EVENT_CLUSTER_RESOURCE_MOVED, m_id, "dsdsds",
												 m_pResourceList[k].dwId, m_pResourceList[k].szName,
												 m_pResourceList[k].dwCurrOwner,
												 (pObject != NULL) ? pObject->getName() : _T("<unknown>"),
												 node->getId(), node->getName());
								}
								m_pResourceList[k].dwCurrOwner = node->getId();
								modified |= MODIFY_CLUSTER_RESOURCES;
							}
							resourceFound[k] = 1;
						}
					}
				}
				unlockProperties();
				delete pIfList;
			}
			else
			{
		      sendPollerMsg(dwRqId, _T("Cannot get interface list from %s\r\n"), node->getName());
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
				PostSystemEvent(EVENT_CLUSTER_RESOURCE_DOWN, m_id, "dsds",
							 m_pResourceList[i].dwId, m_pResourceList[i].szName,
							 m_pResourceList[i].dwCurrOwner,
							 (pObject != nullptr) ? pObject->getName() : _T("<unknown>"));
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

   sendPollerMsg(dwRqId, _T("Status poll finished\r\n"));
   nxlog_debug_tag(DEBUG_TAG_CONF_POLL, 5, _T("ClusterStatusPoll(%s): finished"), m_name);

   pollerUnlock();
}

/**
 * Check if node is current owner of resource
 */
bool Cluster::isResourceOnNode(UINT32 dwResource, UINT32 dwNode)
{
	bool bRet = FALSE;

	lockProperties();
	for(UINT32 i = 0; i < m_dwNumResources; i++)
	{
		if (m_pResourceList[i].dwId == dwResource)
		{
			if (m_pResourceList[i].dwCurrOwner == dwNode)
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
UINT32 Cluster::getResourceOwnerInternal(UINT32 id, const TCHAR *name)
{
   UINT32 ownerId = 0;
   lockProperties();
   for(UINT32 i = 0; i < m_dwNumResources; i++)
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
UINT32 Cluster::collectAggregatedData(DCItem *item, TCHAR *buffer)
{
   readLockChildList();
   ObjectArray<ItemValue> values(getChildList().size(), 32, Ownership::True);
   for(int i = 0; i < getChildList().size(); i++)
   {
      if (getChildList().get(i)->getObjectClass() != OBJECT_NODE)
         continue;

      Node *node = static_cast<Node*>(getChildList().get(i));
      shared_ptr<DCObject> dco = node->getDCObjectByTemplateId(item->getId(), 0);
      if ((dco != NULL) &&
          (dco->getType() == DCO_TYPE_ITEM) &&
          (dco->getStatus() == ITEM_STATUS_ACTIVE) &&
          ((dco->getErrorCount() == 0) || dco->isAggregateWithErrors()) &&
          dco->matchClusterResource())
      {
         ItemValue *v = static_cast<DCItem*>(dco.get())->getInternalLastValue();
         if (v != NULL)
         {
            // Immediately after server startup cache may be filled with placeholder values
            // They can be identified by timestamp equals 1
            if (v->getTimeStamp() > 1)
               values.add(v);
            else
               delete v;
         }
      }
   }
   unlockChildList();

   UINT32 rcc = DCE_SUCCESS;
   if (!values.isEmpty())
   {
      ItemValue result;
      switch(item->getAggregationFunction())
      {
         case DCF_FUNCTION_AVG:
            CalculateItemValueAverage(result, item->getDataType(), values.getBuffer(), values.size());
            break;
         case DCF_FUNCTION_MAX:
            CalculateItemValueMax(result, item->getDataType(), values.getBuffer(), values.size());
            break;
         case DCF_FUNCTION_MIN:
            CalculateItemValueMin(result, item->getDataType(), values.getBuffer(), values.size());
            break;
         case DCF_FUNCTION_SUM:
            CalculateItemValueTotal(result, item->getDataType(), values.getBuffer(), values.size());
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
UINT32 Cluster::collectAggregatedData(DCTable *table, Table **result)
{
   readLockChildList();
   Table **values = MemAllocArray<Table*>(getChildList().size());
   int valueCount = 0;
   for(int i = 0; i < getChildList().size(); i++)
   {
      if (getChildList().get(i)->getObjectClass() != OBJECT_NODE)
         continue;

      Node *node = static_cast<Node*>(getChildList().get(i));
      shared_ptr<DCObject> dco = node->getDCObjectByTemplateId(table->getId(), 0);
      if ((dco != NULL) &&
          (dco->getType() == DCO_TYPE_TABLE) &&
          (dco->getStatus() == ITEM_STATUS_ACTIVE) &&
          ((dco->getErrorCount() == 0) || dco->isAggregateWithErrors()) &&
          dco->matchClusterResource())
      {
         Table *v = static_cast<DCTable*>(dco.get())->getLastValue();
         if (v != NULL)
            values[valueCount++] = v;
      }
   }
   unlockChildList();

   UINT32 rcc = DCE_SUCCESS;
   if (valueCount > 0)
   {
      *result = new Table(values[0]);
      for(int i = 1; i < valueCount; i++)
         table->mergeValues(*result, values[i], i);
   }
   else
   {
      rcc = DCE_COLLECTION_ERROR;
   }

   for(int i = 0; i < valueCount; i++)
      values[i]->decRefCount();
   MemFree(values);

   return rcc;
}

/**
 * Unbind cluster from template
 */
void Cluster::unbindFromTemplate(const shared_ptr<DataCollectionOwner>& templateObject, bool removeDCI)
{
   super::unbindFromTemplate(templateObject, removeDCI);
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
NXSL_Value *Cluster::createNXSLObject(NXSL_VM *vm) const
{
   return vm->createValue(new NXSL_Object(vm, &g_nxslClusterClass, new shared_ptr<Cluster>(self())));
}

/**
 * Get cluster nodes as NXSL array
 */
NXSL_Array *Cluster::getNodesForNXSL(NXSL_VM *vm)
{
   NXSL_Array *nodes = new NXSL_Array(vm);
   int index = 0;

   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      if (getChildList().get(i)->getObjectClass() == OBJECT_NODE)
      {
         nodes->set(index++, getChildList().get(i)->createNXSLObject(vm));
      }
   }
   unlockChildList();

   return nodes;
}

/**
 * Serialize object to JSON
 */
json_t *Cluster::toJson()
{
   json_t *root = super::toJson();

   lockProperties();
   json_object_set_new(root, "clusterType", json_integer(m_dwClusterType));
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
