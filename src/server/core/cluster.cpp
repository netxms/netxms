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
** File: cluster.cpp
**
**/

#include "nxcore.h"

/**
 * Cluster class default constructor
 */
Cluster::Cluster() : super()
{
	m_dwClusterType = 0;
   m_syncNetworks = new ObjectArray<InetAddress>(8, 8, true);
	m_dwNumResources = 0;
	m_pResourceList = NULL;
	m_zoneUIN = 0;
}

/**
 * Cluster class new object constructor
 */
Cluster::Cluster(const TCHAR *pszName, UINT32 zoneUIN) : super(pszName)
{
	m_dwClusterType = 0;
   m_syncNetworks = new ObjectArray<InetAddress>(8, 8, true);
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
	NetObj *pObject;
	int i, nRows;

   m_id = dwId;
   if (!loadCommonProperties(hdb))
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
				pObject = FindObjectById(dwNodeId);
				if (pObject != NULL)
				{
					if (pObject->getObjectClass() == OBJECT_NODE)
					{
                  addChild(pObject);
                  pObject->addParent(this);
					}
					else
					{
                  nxlog_write(MSG_CLUSTER_MEMBER_NOT_NODE, EVENTLOG_ERROR_TYPE, "dd", m_id, dwNodeId);
						break;
					}
				}
				else
				{
               nxlog_write(MSG_INVALID_CLUSTER_MEMBER, EVENTLOG_ERROR_TYPE, "dd", m_id, dwNodeId);
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
bool Cluster::showThresholdSummary()
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
		lockDciAccess(false);
      for(int i = 0; (i < m_dcObjects->size()) && success; i++)
         success = m_dcObjects->get(i)->saveToDatabase(hdb);
		unlockDciAccess();
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
            lockChildList(false);
            for(int i = 0; (i < m_childList->size()) && success; i++)
            {
               if (m_childList->get(i)->getObjectClass() != OBJECT_NODE)
                  continue;

               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_childList->get(i)->getId());
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

   // Clear modifications flag
   lockProperties();
   m_modified = 0;
   unlockProperties();
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
   if (m_runtimeFlags & DCDF_DELETE_IN_PROGRESS)
   {
      if (dwRqId == 0)
         m_runtimeFlags &= ~DCDF_QUEUED_FOR_STATUS_POLL;
      return;
   }

   poller->setStatus(_T("wait for lock"));
   pollerLock();

   if (IsShutdownInProgress())
   {
      pollerUnlock();
      return;
   }

   m_pollRequestor = pSession;
   sendPollerMsg(dwRqId, _T("CLUSTER STATUS POLL [%s]: Applying templates\r\n"), m_name);
   DbgPrintf(6, _T("CLUSTER STATUS POLL [%s]: Applying templates"), m_name);
   if (ConfigReadBoolean(_T("ClusterTemplateAutoApply"), false))
      applyUserTemplates();

   sendPollerMsg(dwRqId, _T("CLUSTER STATUS POLL [%s]: Updating container bindings\r\n"), m_name);
   DbgPrintf(6, _T("CLUSTER STATUS POLL [%s]: Updating container bindings"), m_name);
   if (ConfigReadBoolean(_T("ClusterContainerAutoBind"), false))
      updateContainerMembership();

   lockProperties();
   m_lastConfigurationPoll = time(NULL);
   m_runtimeFlags &= ~DCDF_QUEUED_FOR_CONFIGURATION_POLL;
   unlockProperties();

   poller->setStatus(_T("hook"));
   executeHookScript(_T("ConfigurationPoll"));

   sendPollerMsg(dwRqId, _T("CLUSTER CONFIGURATION POLL [%s]: Finished\r\n"), m_name);
   DbgPrintf(6, _T("CLUSTER CONFIGURATION POLL [%s]: Finished"), m_name);

   pollerUnlock();
   m_runtimeFlags |= DCDF_CONFIGURATION_POLL_PASSED;
}

/**
 * Status poll
 */
void Cluster::statusPoll(PollerInfo *poller, ClientSession *pSession, UINT32 dwRqId)
{
   if (m_runtimeFlags & DCDF_DELETE_IN_PROGRESS)
   {
      if (dwRqId == 0)
         m_runtimeFlags &= ~DCDF_QUEUED_FOR_STATUS_POLL;
      return;
   }

   poller->setStatus(_T("wait for lock"));
   pollerLock();

   if (IsShutdownInProgress())
   {
      pollerUnlock();
      return;
   }

   UINT32 modified = 0;

   // Create polling list
	ObjectArray<DataCollectionTarget> pollList(m_childList->size(), 16, false);
   lockChildList(false);
   int i;
   for(i = 0; i < m_childList->size(); i++)
   {
      NetObj *object = m_childList->get(i);
      if ((object->getStatus() != STATUS_UNMANAGED) && object->isDataCollectionTarget())
      {
         object->incRefCount();
         static_cast<DataCollectionTarget*>(object)->lockForStatusPoll();
         pollList.add(static_cast<DataCollectionTarget*>(object));
      }
   }
   unlockChildList();

	// Perform status poll on all member nodes
   m_pollRequestor = pSession;
   sendPollerMsg(dwRqId, _T("CLUSTER STATUS POLL [%s]: Polling member nodes\r\n"), m_name);
	DbgPrintf(6, _T("CLUSTER STATUS POLL [%s]: Polling member nodes"), m_name);
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
			PostEvent(EVENT_CLUSTER_DOWN, m_id, NULL);
		}
	}
	else
	{
		if (m_state & CLSF_DOWN)
		{
		   m_state &= ~CLSF_DOWN;
			PostEvent(EVENT_CLUSTER_UP, m_id, NULL);
		}
	}

	// Check for cluster resource movement
	if (!allDown)
	{
		BYTE *resourceFound = (BYTE *)calloc(m_dwNumResources, 1);

      poller->setStatus(_T("resource poll"));
	   sendPollerMsg(dwRqId, _T("CLUSTER STATUS POLL [%s]: Polling resources\r\n"), m_name);
		DbgPrintf(6, _T("CLUSTER STATUS POLL [%s]: Polling resources"), m_name);
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
						      sendPollerMsg(dwRqId, _T("CLUSTER STATUS POLL [%s]: Resource %s owner changed\r\n"), m_name, m_pResourceList[k].szName);
								DbgPrintf(5, _T("CLUSTER STATUS POLL [%s]: Resource %s owner changed"),
											 m_name, m_pResourceList[k].szName);

								// Resource moved or go up
								if (m_pResourceList[k].dwCurrOwner == 0)
								{
									// Resource up
									PostEvent(EVENT_CLUSTER_RESOURCE_UP, m_id, "dsds",
												 m_pResourceList[k].dwId, m_pResourceList[k].szName,
												 node->getId(), node->getName());
								}
								else
								{
									// Moved
									NetObj *pObject = FindObjectById(m_pResourceList[k].dwCurrOwner);
									PostEvent(EVENT_CLUSTER_RESOURCE_MOVED, m_id, "dsdsds",
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
		      sendPollerMsg(dwRqId, _T("CLUSTER STATUS POLL [%s]: Cannot get interface list from %s\r\n"), m_name, node->getName());
				DbgPrintf(6, _T("CLUSTER STATUS POLL [%s]: Cannot get interface list from %s"),
							 m_name, node->getName());
			}
		}

		// Check for missing virtual addresses
		lockProperties();
		for(i = 0; i < (int)m_dwNumResources; i++)
		{
			if ((!resourceFound[i]) && (m_pResourceList[i].dwCurrOwner != 0))
			{
				NetObj *pObject = FindObjectById(m_pResourceList[i].dwCurrOwner);
				PostEvent(EVENT_CLUSTER_RESOURCE_DOWN, m_id, "dsds",
							 m_pResourceList[i].dwId, m_pResourceList[i].szName,
							 m_pResourceList[i].dwCurrOwner,
							 (pObject != NULL) ? pObject->getName() : _T("<unknown>"));
				m_pResourceList[i].dwCurrOwner = 0;
            modified |= MODIFY_CLUSTER_RESOURCES;
			}
		}
		unlockProperties();
		free(resourceFound);
	}

   // Execute hook script
   poller->setStatus(_T("hook"));
   executeHookScript(_T("StatusPoll"));

   calculateCompoundStatus(true);
   poller->setStatus(_T("cleanup"));

   // Cleanup
	for(i = 0; i < pollList.size(); i++)
	{
		pollList.get(i)->decRefCount();
	}

	lockProperties();
	if (modified != 0)
		setModified(modified);
	m_lastStatusPoll = time(NULL);
	m_runtimeFlags &= ~DCDF_QUEUED_FOR_STATUS_POLL;
	unlockProperties();

   sendPollerMsg(dwRqId, _T("CLUSTER STATUS POLL [%s]: Finished\r\n"), m_name);
	DbgPrintf(6, _T("CLUSTER STATUS POLL [%s]: Finished"), m_name);

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
   lockChildList(false);
   ObjectArray<ItemValue> values(m_childList->size(), 32, true);
   for(int i = 0; i < m_childList->size(); i++)
   {
      if (m_childList->get(i)->getObjectClass() != OBJECT_NODE)
         continue;

      Node *node = static_cast<Node*>(m_childList->get(i));
      DCObject *dco = node->getDCObjectByTemplateId(item->getId(), 0);
      if ((dco != NULL) &&
          (dco->getType() == DCO_TYPE_ITEM) &&
          (dco->getStatus() == ITEM_STATUS_ACTIVE) &&
          ((dco->getErrorCount() == 0) || dco->isAggregateWithErrors()) &&
          dco->matchClusterResource())
      {
         ItemValue *v = static_cast<DCItem*>(dco)->getInternalLastValue();
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
   lockChildList(false);
   Table **values = (Table **)malloc(sizeof(Table *) * m_childList->size());
   int valueCount = 0;
   for(int i = 0; i < m_childList->size(); i++)
   {
      if (m_childList->get(i)->getObjectClass() != OBJECT_NODE)
         continue;

      Node *node = static_cast<Node*>(m_childList->get(i));
      DCObject *dco = node->getDCObjectByTemplateId(table->getId(), 0);
      if ((dco != NULL) &&
          (dco->getType() == DCO_TYPE_TABLE) &&
          (dco->getStatus() == ITEM_STATUS_ACTIVE) &&
          ((dco->getErrorCount() == 0) || dco->isAggregateWithErrors()) &&
          dco->matchClusterResource())
      {
         Table *v = ((DCTable *)dco)->getLastValue();
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
void Cluster::unbindFromTemplate(UINT32 dwTemplateId, bool removeDCI)
{
   super::unbindFromTemplate(dwTemplateId, removeDCI);
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
   return vm->createValue(new NXSL_Object(vm, &g_nxslClusterClass, this));
}

/**
 * Get cluster nodes as NXSL array
 */
NXSL_Array *Cluster::getNodesForNXSL(NXSL_VM *vm)
{
   NXSL_Array *nodes = new NXSL_Array(vm);
   int index = 0;

   lockChildList(false);
   for(int i = 0; i < m_childList->size(); i++)
   {
      if (m_childList->get(i)->getObjectClass() == OBJECT_NODE)
      {
         nodes->set(index++, m_childList->get(i)->createNXSLObject(vm));
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
   json_object_set_new(root, "clusterType", json_integer(m_dwClusterType));
   json_object_set_new(root, "syncNetworks", json_object_array(m_syncNetworks));
   json_object_set_new(root, "lastStatusPoll", json_integer(m_lastStatusPoll));
   json_object_set_new(root, "lastConfigurationPoll", json_integer(m_lastConfigurationPoll));
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
   return root;
}
