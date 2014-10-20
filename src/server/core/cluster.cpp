/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
Cluster::Cluster() : DataCollectionTarget()
{
	m_dwClusterType = 0;
	m_dwNumSyncNets = 0;
	m_pSyncNetList= NULL;
	m_dwNumResources = 0;
	m_pResourceList = NULL;
	m_tmLastPoll = 0;
	m_dwFlags = 0;
	m_zoneId = 0;
}

/**
 * Cluster class new object constructor
 */
Cluster::Cluster(const TCHAR *pszName, UINT32 zoneId) : DataCollectionTarget(pszName)
{
	m_dwClusterType = 0;
	m_dwNumSyncNets = 0;
	m_pSyncNetList= NULL;
	m_dwNumResources = 0;
	m_pResourceList = NULL;
	m_tmLastPoll = 0;
	m_dwFlags = 0;
	m_zoneId = zoneId;
}

/**
 * Destructor
 */
Cluster::~Cluster()
{
	safe_free(m_pSyncNetList);
	safe_free(m_pResourceList);
}

/**
 * Create object from database data
 */
BOOL Cluster::loadFromDatabase(UINT32 dwId)
{
	TCHAR szQuery[256];
   BOOL bResult = FALSE;
	DB_RESULT hResult;
	UINT32 dwNodeId;
	NetObj *pObject;
	int i, nRows;

   m_id = dwId;

   if (!loadCommonProperties())
   {
      DbgPrintf(2, _T("Cannot load common properties for cluster object %d"), dwId);
      return FALSE;
   }

	_sntprintf(szQuery, 256, _T("SELECT cluster_type,zone_guid FROM clusters WHERE id=%d"), (int)m_id);
	hResult = DBSelect(g_hCoreDB, szQuery);
	if (hResult == NULL)
		return FALSE;

	m_dwClusterType = DBGetFieldULong(hResult, 0, 0);
	m_zoneId = DBGetFieldULong(hResult, 0, 1);
	DBFreeResult(hResult);

   // Load DCI and access list
   loadACLFromDB();
   loadItemsFromDB();
   for(i = 0; i < m_dcObjects->size(); i++)
      if (!m_dcObjects->get(i)->loadThresholdsFromDB())
         return FALSE;

   if (!m_isDeleted)
   {
		// Load member nodes
		_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT node_id FROM cluster_members WHERE cluster_id=%d"), m_id);
		hResult = DBSelect(g_hCoreDB, szQuery);
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
                  AddChild(pObject);
                  pObject->AddParent(this);
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
				bResult = TRUE;
			DBFreeResult(hResult);
		}

		// Load sync net list
		if (bResult)
		{
			_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT subnet_addr,subnet_mask FROM cluster_sync_subnets WHERE cluster_id=%d"), m_id);
			hResult = DBSelect(g_hCoreDB, szQuery);
			if (hResult != NULL)
			{
				m_dwNumSyncNets = DBGetNumRows(hResult);
				if (m_dwNumSyncNets > 0)
				{
					m_pSyncNetList = (IP_NETWORK *)malloc(sizeof(IP_NETWORK) * m_dwNumSyncNets);
					for(i = 0; i < (int)m_dwNumSyncNets; i++)
					{
						m_pSyncNetList[i].dwAddr = DBGetFieldIPAddr(hResult, i, 0);
						m_pSyncNetList[i].dwMask = DBGetFieldIPAddr(hResult, i, 1);
					}
				}
				DBFreeResult(hResult);
			}
			else
			{
				bResult = FALSE;
			}
		}

		// Load resources
		if (bResult)
		{
			_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT resource_id,resource_name,ip_addr,current_owner FROM cluster_resources WHERE cluster_id=%d"), m_id);
			hResult = DBSelect(g_hCoreDB, szQuery);
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
						m_pResourceList[i].dwIpAddr = DBGetFieldIPAddr(hResult, i, 2);
						m_pResourceList[i].dwCurrOwner = DBGetFieldULong(hResult, i, 3);
					}
				}
				DBFreeResult(hResult);
			}
			else
			{
				bResult = FALSE;
			}
		}
	}
   else
   {
      bResult = TRUE;
   }

   return bResult;
}

/**
 * Save object to database
 */
BOOL Cluster::saveToDatabase(DB_HANDLE hdb)
{
	TCHAR szQuery[4096], szIpAddr[16], szNetMask[16];
   BOOL bResult;
   UINT32 i;

   // Lock object's access
   lockProperties();

   saveCommonProperties(hdb);

   if (IsDatabaseRecordExist(hdb, _T("clusters"), _T("id"), m_id))
      _sntprintf(szQuery, 4096,
                 _T("UPDATE clusters SET cluster_type=%d,zone_guid=%d WHERE id=%d"),
                 (int)m_dwClusterType, (int)m_zoneId, (int)m_id);
	else
      _sntprintf(szQuery, 4096,
                 _T("INSERT INTO clusters (id,cluster_type,zone_guid) VALUES (%d,%d,%d)"),
                 (int)m_id, (int)m_dwClusterType, (int)m_zoneId);
   bResult = DBQuery(hdb, szQuery);

   // Save data collection items
   if (bResult)
   {
		lockDciAccess(false);
      for(i = 0; i < (UINT32)m_dcObjects->size(); i++)
         m_dcObjects->get(i)->saveToDB(hdb);
		unlockDciAccess();

		// Save cluster members list
		if (DBBegin(hdb))
		{
			_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM cluster_members WHERE cluster_id=%d"), m_id);
			DBQuery(hdb, szQuery);
			LockChildList(FALSE);
			for(i = 0; i < m_dwChildCount; i++)
			{
				if (m_pChildList[i]->getObjectClass() == OBJECT_NODE)
				{
					_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("INSERT INTO cluster_members (cluster_id,node_id) VALUES (%d,%d)"),
								 m_id, m_pChildList[i]->getId());
					bResult = DBQuery(hdb, szQuery);
					if (!bResult)
						break;
				}
			}
			UnlockChildList();
			if (bResult)
				DBCommit(hdb);
			else
				DBRollback(hdb);
		}
		else
		{
			bResult = FALSE;
		}

		// Save sync net list
		if (bResult)
		{
			if (DBBegin(hdb))
			{
				_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM cluster_sync_subnets WHERE cluster_id=%d"), m_id);
				DBQuery(hdb, szQuery);
				for(i = 0; i < m_dwNumSyncNets; i++)
				{
					_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("INSERT INTO cluster_sync_subnets (cluster_id,subnet_addr,subnet_mask) VALUES (%d,'%s','%s')"),
								 m_id, IpToStr(m_pSyncNetList[i].dwAddr, szIpAddr),
								 IpToStr(m_pSyncNetList[i].dwMask, szNetMask));
					bResult = DBQuery(hdb, szQuery);
					if (!bResult)
						break;
				}
				if (bResult)
					DBCommit(hdb);
				else
					DBRollback(hdb);
			}
			else
			{
				bResult = FALSE;
			}
		}

		// Save resource list
		if (bResult)
		{
			if (DBBegin(hdb))
			{
				_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM cluster_resources WHERE cluster_id=%d"), m_id);
				DBQuery(hdb, szQuery);
				for(i = 0; i < m_dwNumResources; i++)
				{
					_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("INSERT INTO cluster_resources (cluster_id,resource_id,resource_name,ip_addr,current_owner) VALUES (%d,%d,%s,'%s',%d)"),
					           m_id, m_pResourceList[i].dwId, (const TCHAR *)DBPrepareString(hdb, m_pResourceList[i].szName),
								  IpToStr(m_pResourceList[i].dwIpAddr, szIpAddr),
								  m_pResourceList[i].dwCurrOwner);
					bResult = DBQuery(hdb, szQuery);
					if (!bResult)
						break;
				}
				if (bResult)
					DBCommit(hdb);
				else
					DBRollback(hdb);
			}
			else
			{
				bResult = FALSE;
			}
		}
   }

   // Save access list
   saveACLToDB(hdb);

   // Clear modifications flag and unlock object
	if (bResult)
		m_isModified = false;
   unlockProperties();

   return bResult;
}

/**
 * Delete object from database
 */
bool Cluster::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = DataCollectionTarget::deleteFromDatabase(hdb);
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
void Cluster::fillMessage(CSCPMessage *pMsg)
{
	UINT32 i, dwId;

   DataCollectionTarget::fillMessage(pMsg);
   pMsg->SetVariable(VID_CLUSTER_TYPE, m_dwClusterType);
	pMsg->SetVariable(VID_ZONE_ID, m_zoneId);
	pMsg->SetVariable(VID_NUM_SYNC_SUBNETS, m_dwNumSyncNets);
	if (m_dwNumSyncNets > 0)
		pMsg->setFieldInt32Array(VID_SYNC_SUBNETS, m_dwNumSyncNets * 2, (UINT32 *)m_pSyncNetList);
	pMsg->SetVariable(VID_NUM_RESOURCES, m_dwNumResources);
	for(i = 0, dwId = VID_RESOURCE_LIST_BASE; i < m_dwNumResources; i++, dwId += 6)
	{
		pMsg->SetVariable(dwId++, m_pResourceList[i].dwId);
		pMsg->SetVariable(dwId++, m_pResourceList[i].szName);
		pMsg->SetVariable(dwId++, m_pResourceList[i].dwIpAddr);
		pMsg->SetVariable(dwId++, m_pResourceList[i].dwCurrOwner);
	}
}

/**
 * Modify object from message
 */
UINT32 Cluster::modifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked)
{
   if (!bAlreadyLocked)
      lockProperties();

   // Change cluster type
   if (pRequest->isFieldExist(VID_CLUSTER_TYPE))
      m_dwClusterType = pRequest->GetVariableLong(VID_CLUSTER_TYPE);

   // Change sync subnets
   if (pRequest->isFieldExist(VID_NUM_SYNC_SUBNETS))
	{
      m_dwNumSyncNets = pRequest->GetVariableLong(VID_NUM_SYNC_SUBNETS);
		if (m_dwNumSyncNets > 0)
		{
			m_pSyncNetList = (IP_NETWORK *)realloc(m_pSyncNetList, sizeof(IP_NETWORK) * m_dwNumSyncNets);
			pRequest->getFieldAsInt32Array(VID_SYNC_SUBNETS, m_dwNumSyncNets * 2, (UINT32 *)m_pSyncNetList);
		}
		else
		{
			safe_free_and_null(m_pSyncNetList);
		}
	}

   // Change resource list
   if (pRequest->isFieldExist(VID_NUM_RESOURCES))
	{
		UINT32 i, j, dwId, dwCount;
		CLUSTER_RESOURCE *pList;

      dwCount = pRequest->GetVariableLong(VID_NUM_RESOURCES);
		if (dwCount > 0)
		{
			pList = (CLUSTER_RESOURCE *)malloc(sizeof(CLUSTER_RESOURCE) * dwCount);
			memset(pList, 0, sizeof(CLUSTER_RESOURCE) * dwCount);
			for(i = 0, dwId = VID_RESOURCE_LIST_BASE; i < dwCount; i++, dwId += 7)
			{
				pList[i].dwId = pRequest->GetVariableLong(dwId++);
				pRequest->GetVariableStr(dwId++, pList[i].szName, MAX_DB_STRING);
				pList[i].dwIpAddr = pRequest->GetVariableLong(dwId++);
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
			safe_free(m_pResourceList);
			m_pResourceList = pList;
		}
		else
		{
			safe_free_and_null(m_pResourceList);
		}
		m_dwNumResources = dwCount;
	}

   return DataCollectionTarget::modifyFromMessage(pRequest, TRUE);
}

/**
 * Check if given address is within sync network
 */
bool Cluster::isSyncAddr(UINT32 dwAddr)
{
	UINT32 i;
	bool bRet = false;

	lockProperties();
	for(i = 0; i < m_dwNumSyncNets; i++)
	{
		if ((dwAddr & m_pSyncNetList[i].dwMask) == m_pSyncNetList[i].dwAddr)
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
bool Cluster::isVirtualAddr(UINT32 dwAddr)
{
	UINT32 i;
	bool bRet = false;

	lockProperties();
	for(i = 0; i < m_dwNumResources; i++)
	{
		if (m_pResourceList[i].dwIpAddr == dwAddr)
		{
			bRet = true;
			break;
		}
	}
	unlockProperties();
	return bRet;
}

/**
 * Status poll
 */
void Cluster::statusPoll(ClientSession *pSession, UINT32 dwRqId, int nPoller)
{
	UINT32 i, j, k, dwPollListSize;
	InterfaceList *pIfList;
	BOOL bModified = FALSE, bAllDown;
	BYTE *pbResourceFound;
	NetObj **ppPollList;

   // Create polling list
   ppPollList = (NetObj **)malloc(sizeof(NetObj *) * m_dwChildCount);
   LockChildList(FALSE);
   for(i = 0, dwPollListSize = 0; i < m_dwChildCount; i++)
      if ((m_pChildList[i]->Status() != STATUS_UNMANAGED) &&
			 (m_pChildList[i]->getObjectClass() == OBJECT_NODE))
      {
         m_pChildList[i]->incRefCount();
			((Node *)m_pChildList[i])->lockForStatusPoll();
         ppPollList[dwPollListSize++] = m_pChildList[i];
      }
   UnlockChildList();

	// Perform status poll on all member nodes
	DbgPrintf(6, _T("CLUSTER STATUS POLL [%s]: Polling member nodes"), m_name);
	for(i = 0, bAllDown = TRUE; i < dwPollListSize; i++)
	{
		((Node *)ppPollList[i])->statusPoll(pSession, dwRqId, nPoller);
		if (!((Node *)ppPollList[i])->isDown())
			bAllDown = FALSE;
	}

	if (bAllDown)
	{
		if (!(m_dwFlags & CLF_DOWN))
		{
			m_dwFlags |= CLF_DOWN;
			PostEvent(EVENT_CLUSTER_DOWN, m_id, NULL);
		}
	}
	else
	{
		if (m_dwFlags & CLF_DOWN)
		{
			m_dwFlags &= ~CLF_DOWN;
			PostEvent(EVENT_CLUSTER_UP, m_id, NULL);
		}
	}

	// Check for cluster resource movement
	if (!bAllDown)
	{
		pbResourceFound = (BYTE *)malloc(m_dwNumResources);
		memset(pbResourceFound, 0, m_dwNumResources);

		DbgPrintf(6, _T("CLUSTER STATUS POLL [%s]: Polling resources"), m_name);
		for(i = 0; i < dwPollListSize; i++)
		{
			pIfList = ((Node *)ppPollList[i])->getInterfaceList();
			if (pIfList != NULL)
			{
				lockProperties();
				for(j = 0; j < (UINT32)pIfList->size(); j++)
				{
					for(k = 0; k < m_dwNumResources; k++)
					{
						if (m_pResourceList[k].dwIpAddr == pIfList->get(j)->dwIpAddr)
						{
							if (m_pResourceList[k].dwCurrOwner != ppPollList[i]->getId())
							{
								DbgPrintf(5, _T("CLUSTER STATUS POLL [%s]: Resource %s owner changed"),
											 m_name, m_pResourceList[k].szName);

								// Resource moved or go up
								if (m_pResourceList[k].dwCurrOwner == 0)
								{
									// Resource up
									PostEvent(EVENT_CLUSTER_RESOURCE_UP, m_id, "dsds",
												 m_pResourceList[k].dwId, m_pResourceList[k].szName,
												 ppPollList[i]->getId(), ppPollList[i]->getName());
								}
								else
								{
									// Moved
									NetObj *pObject;

									pObject = FindObjectById(m_pResourceList[k].dwCurrOwner);
									PostEvent(EVENT_CLUSTER_RESOURCE_MOVED, m_id, "dsdsds",
												 m_pResourceList[k].dwId, m_pResourceList[k].szName,
												 m_pResourceList[k].dwCurrOwner,
												 (pObject != NULL) ? pObject->getName() : _T("<unknown>"),
												 ppPollList[i]->getId(), ppPollList[i]->getName());
								}
								m_pResourceList[k].dwCurrOwner = ppPollList[i]->getId();
								bModified = TRUE;
							}
							pbResourceFound[k] = 1;
						}
					}
				}
				unlockProperties();
				delete pIfList;
			}
			else
			{
				DbgPrintf(6, _T("CLUSTER STATUS POLL [%s]: Cannot get interface list from %s"),
							 m_name, ppPollList[i]->getName());
			}
		}

		// Check for missing virtual addresses
		lockProperties();
		for(i = 0; i < m_dwNumResources; i++)
		{
			if ((!pbResourceFound[i]) && (m_pResourceList[i].dwCurrOwner != 0))
			{
				NetObj *pObject;

				pObject = FindObjectById(m_pResourceList[i].dwCurrOwner);
				PostEvent(EVENT_CLUSTER_RESOURCE_DOWN, m_id, "dsds",
							 m_pResourceList[i].dwId, m_pResourceList[i].szName,
							 m_pResourceList[i].dwCurrOwner,
							 (pObject != NULL) ? pObject->getName() : _T("<unknown>"));
				m_pResourceList[i].dwCurrOwner = 0;
				bModified = TRUE;
			}
		}
		unlockProperties();
		safe_free(pbResourceFound);
	}

	// Cleanup
	for(i = 0; i < dwPollListSize; i++)
	{
		ppPollList[i]->decRefCount();
	}
	safe_free(ppPollList);

	lockProperties();
	if (bModified)
		setModified();
	m_tmLastPoll = time(NULL);
	m_dwFlags &= ~CLF_QUEUED_FOR_STATUS_POLL;
	unlockProperties();

	DbgPrintf(6, _T("CLUSTER STATUS POLL [%s]: Finished"), m_name);
}

/**
 * Check if node is current owner of resource
 */
bool Cluster::isResourceOnNode(UINT32 dwResource, UINT32 dwNode)
{
	bool bRet = FALSE;
	UINT32 i;

	lockProperties();
	for(i = 0; i < m_dwNumResources; i++)
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
 * Collect aggregated data for cluster nodes - single value
 */
UINT32 Cluster::collectAggregatedData(DCItem *item, TCHAR *buffer)
{
   LockChildList(TRUE);
   ItemValue **values = (ItemValue **)malloc(sizeof(ItemValue *) * m_dwChildCount);
   int valueCount = 0;
   for(UINT32 i = 0; i < m_dwChildCount; i++)
   {
      if (m_pChildList[i]->getObjectClass() != OBJECT_NODE)
         continue;

      Node *node = (Node *)m_pChildList[i];
      DCObject *dco = node->getDCObjectByTemplateId(item->getId());
      if ((dco != NULL) && 
          (dco->getType() == DCO_TYPE_ITEM) && 
          (dco->getStatus() == ITEM_STATUS_ACTIVE) && 
          (dco->getErrorCount() == 0) &&
          dco->matchClusterResource())
      {
         ItemValue *v = ((DCItem *)dco)->getInternalLastValue();
         if (v != NULL)
            values[valueCount++] = v;
      }
   }
   UnlockChildList();

   UINT32 rcc = DCE_SUCCESS;
   if (valueCount > 0)
   {
      ItemValue result;
      switch(item->getAggregationFunction())
      {
         case DCF_FUNCTION_AVG:
            CalculateItemValueAverage(result, item->getDataType(), valueCount, values);
            break;
         case DCF_FUNCTION_MAX:
            CalculateItemValueMax(result, item->getDataType(), valueCount, values);
            break;
         case DCF_FUNCTION_MIN:
            CalculateItemValueMin(result, item->getDataType(), valueCount, values);
            break;
         case DCF_FUNCTION_SUM:
            CalculateItemValueTotal(result, item->getDataType(), valueCount, values);
            break;
         default:
            rcc = DCE_NOT_SUPPORTED;
            break;
      }
      nx_strncpy(buffer, result.getString(), MAX_RESULT_LENGTH);
   }
   else
   {
      rcc = DCE_COMM_ERROR;
   }

   for(int i = 0; i < valueCount; i++)
      delete values[i];
   safe_free(values);

   return rcc;
}

/**
 * Collect aggregated data for cluster nodes - table
 */
UINT32 Cluster::collectAggregatedData(DCTable *table, Table **result)
{
   LockChildList(TRUE);
   Table **values = (Table **)malloc(sizeof(Table *) * m_dwChildCount);
   int valueCount = 0;
   for(UINT32 i = 0; i < m_dwChildCount; i++)
   {
      if (m_pChildList[i]->getObjectClass() != OBJECT_NODE)
         continue;

      Node *node = (Node *)m_pChildList[i];
      DCObject *dco = node->getDCObjectByTemplateId(table->getId());
      if ((dco != NULL) && 
          (dco->getType() == DCO_TYPE_TABLE) && 
          (dco->getStatus() == ITEM_STATUS_ACTIVE) && 
          (dco->getErrorCount() == 0) &&
          dco->matchClusterResource())
      {
         Table *v = ((DCTable *)dco)->getLastValue();
         if (v != NULL)
            values[valueCount++] = v;
      }
   }
   UnlockChildList();

   UINT32 rcc = DCE_SUCCESS;
   if (valueCount > 0)
   {
      *result = new Table(values[0]);
      for(int i = 1; i < valueCount; i++)
         table->mergeValues(*result, values[i], i);
   }
   else
   {
      rcc = DCE_COMM_ERROR;
   }

   for(int i = 0; i < valueCount; i++)
      values[i]->decRefCount();
   safe_free(values);

   return rcc;
}

/**
 * Unbind cluster from template
 */
void Cluster::unbindFromTemplate(UINT32 dwTemplateId, BOOL bRemoveDCI)
{
   DataCollectionTarget::unbindFromTemplate(dwTemplateId, bRemoveDCI);
   queueUpdate();
}
