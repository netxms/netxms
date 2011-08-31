/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2011 Victor Kirhenshtein
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


//
// Cluster class default constructor
//

Cluster::Cluster()
        :Template()
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


//
// Cluster class new object constructor
//

Cluster::Cluster(const TCHAR *pszName, DWORD zoneId)
        :Template(pszName)
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


//
// Destructor
//

Cluster::~Cluster()
{
	safe_free(m_pSyncNetList);
	safe_free(m_pResourceList);
}


//
// Create object from database data
//

BOOL Cluster::CreateFromDB(DWORD dwId)
{
	TCHAR szQuery[256];
   BOOL bResult = FALSE;
	DB_RESULT hResult;
	DWORD dwNodeId;
	NetObj *pObject;
	int i, nRows;

   m_dwId = dwId;

   if (!loadCommonProperties())
   {
      DbgPrintf(2, _T("Cannot load common properties for cluster object %d"), dwId);
      return FALSE;
   }

	_sntprintf(szQuery, 256, _T("SELECT cluster_type,zone_guid FROM clusters WHERE id=%d"), (int)m_dwId);
	hResult = DBSelect(g_hCoreDB, szQuery);
	if (hResult == NULL)
		return FALSE;

	m_dwClusterType = DBGetFieldULong(hResult, 0, 0);
	m_zoneId = DBGetFieldULong(hResult, 0, 1);
	DBFreeResult(hResult);

   // Load DCI and access list
   loadACLFromDB();
   loadItemsFromDB();
   for(i = 0; i < (int)m_dwNumItems; i++)
      if (!m_ppItems[i]->loadThresholdsFromDB())
         return FALSE;

   if (!m_bIsDeleted)
   {
		// Load member nodes
		_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT node_id FROM cluster_members WHERE cluster_id=%d"), m_dwId);
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
					if (pObject->Type() == OBJECT_NODE)
					{
                  AddChild(pObject);
                  pObject->AddParent(this);
					}
					else
					{
                  nxlog_write(MSG_CLUSTER_MEMBER_NOT_NODE, EVENTLOG_ERROR_TYPE, "dd", m_dwId, dwNodeId);
						break;
					}
				}
				else
				{
               nxlog_write(MSG_INVALID_CLUSTER_MEMBER, EVENTLOG_ERROR_TYPE, "dd", m_dwId, dwNodeId);
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
			_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT subnet_addr,subnet_mask FROM cluster_sync_subnets WHERE cluster_id=%d"), m_dwId);
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
			_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT resource_id,resource_name,ip_addr,current_owner FROM cluster_resources WHERE cluster_id=%d"), m_dwId);
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
						DecodeSQLString(m_pResourceList[i].szName);
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


//
// Save object to database
//

BOOL Cluster::SaveToDB(DB_HANDLE hdb)
{
	TCHAR szQuery[4096], szIpAddr[16], szNetMask[16], *pszEscName;
   DB_RESULT hResult;
   BOOL bResult, bNewObject = TRUE;
   DWORD i;

   // Lock object's access
   LockData();

   saveCommonProperties(hdb);

   // Check for object's existence in database
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT id FROM clusters WHERE id=%d"), m_dwId);
   hResult = DBSelect(hdb, szQuery);
   if (hResult != 0)
   {
      if (DBGetNumRows(hResult) > 0)
         bNewObject = FALSE;
      DBFreeResult(hResult);
   }
   if (bNewObject)
      _sntprintf(szQuery, 4096,
                 _T("INSERT INTO clusters (id,cluster_type,zone_guid) VALUES (%d,%d,%d)"),
                 (int)m_dwId, (int)m_dwClusterType, (int)m_zoneId);
   else
      _sntprintf(szQuery, 4096,
                 _T("UPDATE clusters SET cluster_type=%d,zone_guid=%d WHERE id=%d"),
                 (int)m_dwClusterType, (int)m_zoneId, (int)m_dwId);
   bResult = DBQuery(hdb, szQuery);

   // Save data collection items
   if (bResult)
   {
		lockDciAccess();
      for(i = 0; i < m_dwNumItems; i++)
         m_ppItems[i]->saveToDB(hdb);
		unlockDciAccess();

		// Save cluster members list
		if (DBBegin(hdb))
		{
			_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM cluster_members WHERE cluster_id=%d"), m_dwId);
			DBQuery(hdb, szQuery);
			LockChildList(FALSE);
			for(i = 0; i < m_dwChildCount; i++)
			{
				if (m_pChildList[i]->Type() == OBJECT_NODE)
				{
					_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("INSERT INTO cluster_members (cluster_id,node_id) VALUES (%d,%d)"),
								 m_dwId, m_pChildList[i]->Id());
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
				_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM cluster_sync_subnets WHERE cluster_id=%d"), m_dwId);
				DBQuery(hdb, szQuery);
				for(i = 0; i < m_dwNumSyncNets; i++)
				{
					_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("INSERT INTO cluster_sync_subnets (cluster_id,subnet_addr,subnet_mask) VALUES (%d,'%s','%s')"),
								 m_dwId, IpToStr(m_pSyncNetList[i].dwAddr, szIpAddr),
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
				_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM cluster_resources WHERE cluster_id=%d"), m_dwId);
				DBQuery(hdb, szQuery);
				for(i = 0; i < m_dwNumResources; i++)
				{
					pszEscName = EncodeSQLString(m_pResourceList[i].szName);
					_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("INSERT INTO cluster_resources (cluster_id,resource_id,resource_name,ip_addr,current_owner) VALUES (%d,%d,'%s','%s',%d)"),
								 m_dwId, m_pResourceList[i].dwId, pszEscName,
								 IpToStr(m_pResourceList[i].dwIpAddr, szIpAddr),
								 m_pResourceList[i].dwCurrOwner);
					free(pszEscName);
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
		m_bIsModified = FALSE;
   UnlockData();

   return bResult;
}


//
// Delete object from database
//

BOOL Cluster::DeleteFromDB()
{
   TCHAR szQuery[256];
   BOOL bSuccess;

   bSuccess = Template::DeleteFromDB();
   if (bSuccess)
   {
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM clusters WHERE id=%d"), m_dwId);
      QueueSQLRequest(szQuery);
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM cluster_members WHERE cluster_id=%d"), m_dwId);
      QueueSQLRequest(szQuery);
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM cluster_sync_subnets WHERE cluster_id=%d"), m_dwId);
      QueueSQLRequest(szQuery);
   }
   return bSuccess;
}


//
// Create CSCP message with object's data
//

void Cluster::CreateMessage(CSCPMessage *pMsg)
{
	DWORD i, dwId;

   Template::CreateMessage(pMsg);
   pMsg->SetVariable(VID_CLUSTER_TYPE, m_dwClusterType);
	pMsg->SetVariable(VID_ZONE_ID, m_zoneId);
	pMsg->SetVariable(VID_NUM_SYNC_SUBNETS, m_dwNumSyncNets);
	if (m_dwNumSyncNets > 0)
		pMsg->SetVariableToInt32Array(VID_SYNC_SUBNETS, m_dwNumSyncNets * 2, (DWORD *)m_pSyncNetList);
	pMsg->SetVariable(VID_NUM_RESOURCES, m_dwNumResources);
	for(i = 0, dwId = VID_RESOURCE_LIST_BASE; i < m_dwNumResources; i++, dwId += 6)
	{
		pMsg->SetVariable(dwId++, m_pResourceList[i].dwId);
		pMsg->SetVariable(dwId++, m_pResourceList[i].szName);
		pMsg->SetVariable(dwId++, m_pResourceList[i].dwIpAddr);
		pMsg->SetVariable(dwId++, m_pResourceList[i].dwCurrOwner);
	}
}


//
// Modify object from message
//

DWORD Cluster::ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked)
{
   if (!bAlreadyLocked)
      LockData();

   // Change cluster type
   if (pRequest->IsVariableExist(VID_CLUSTER_TYPE))
      m_dwClusterType = pRequest->GetVariableLong(VID_CLUSTER_TYPE);

   // Change sync subnets
   if (pRequest->IsVariableExist(VID_NUM_SYNC_SUBNETS))
	{
      m_dwNumSyncNets = pRequest->GetVariableLong(VID_NUM_SYNC_SUBNETS);
		if (m_dwNumSyncNets > 0)
		{
			m_pSyncNetList = (IP_NETWORK *)realloc(m_pSyncNetList, sizeof(IP_NETWORK) * m_dwNumSyncNets);
			pRequest->GetVariableInt32Array(VID_SYNC_SUBNETS, m_dwNumSyncNets * 2, (DWORD *)m_pSyncNetList);
		}
		else
		{
			safe_free_and_null(m_pSyncNetList);
		}
	}

   // Change resource list
   if (pRequest->IsVariableExist(VID_NUM_RESOURCES))
	{
		DWORD i, j, dwId, dwCount;
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

   return Template::ModifyFromMessage(pRequest, TRUE);
}


//
// Calculate compound status
//

void Cluster::calculateCompoundStatus(BOOL bForcedRecalc)
{
	NetObj::calculateCompoundStatus(bForcedRecalc);
}


//
// Check if given address is within sync network
//

BOOL Cluster::isSyncAddr(DWORD dwAddr)
{
	DWORD i;
	BOOL bRet = FALSE;

	LockData();
	for(i = 0; i < m_dwNumSyncNets; i++)
	{
		if ((dwAddr & m_pSyncNetList[i].dwMask) == m_pSyncNetList[i].dwAddr)
		{
			bRet = TRUE;
			break;
		}
	}
	UnlockData();
	return bRet;
}


//
// Check if given address is a resource address
//

BOOL Cluster::isVirtualAddr(DWORD dwAddr)
{
	DWORD i;
	BOOL bRet = FALSE;

	LockData();
	for(i = 0; i < m_dwNumResources; i++)
	{
		if (m_pResourceList[i].dwIpAddr == dwAddr)
		{
			bRet = TRUE;
			break;
		}
	}
	UnlockData();
	return bRet;
}


//
// Status poll
//

void Cluster::statusPoll(ClientSession *pSession, DWORD dwRqId, int nPoller)
{
	DWORD i, j, k, dwPollListSize;
	InterfaceList *pIfList;
	BOOL bModified = FALSE, bAllDown;
	BYTE *pbResourceFound;
	NetObj **ppPollList;

   // Create polling list
   ppPollList = (NetObj **)malloc(sizeof(NetObj *) * m_dwChildCount);
   LockChildList(FALSE);
   for(i = 0, dwPollListSize = 0; i < m_dwChildCount; i++)
      if ((m_pChildList[i]->Status() != STATUS_UNMANAGED) &&
			 (m_pChildList[i]->Type() == OBJECT_NODE))
      {
         m_pChildList[i]->IncRefCount();
			((Node *)m_pChildList[i])->lockForStatusPoll();
         ppPollList[dwPollListSize++] = m_pChildList[i];
      }
   UnlockChildList();

	// Perform status poll on all member nodes
	DbgPrintf(6, _T("CLUSTER STATUS POLL [%s]: Polling member nodes"), m_szName);
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
			PostEvent(EVENT_CLUSTER_DOWN, m_dwId, NULL);
		}
	}
	else
	{
		if (m_dwFlags & CLF_DOWN)
		{
			m_dwFlags &= ~CLF_DOWN;
			PostEvent(EVENT_CLUSTER_UP, m_dwId, NULL);
		}
	}

	// Check for cluster resource movement
	if (!bAllDown)
	{
		pbResourceFound = (BYTE *)malloc(m_dwNumResources);
		memset(pbResourceFound, 0, m_dwNumResources);

		DbgPrintf(6, _T("CLUSTER STATUS POLL [%s]: Polling resources"), m_szName);
		for(i = 0; i < dwPollListSize; i++)
		{
			pIfList = ((Node *)ppPollList[i])->getInterfaceList();
			if (pIfList != NULL)
			{
				LockData();
				for(j = 0; j < (DWORD)pIfList->getSize(); j++)
				{
					for(k = 0; k < m_dwNumResources; k++)
					{
						if (m_pResourceList[k].dwIpAddr == pIfList->get(j)->dwIpAddr)
						{
							if (m_pResourceList[k].dwCurrOwner != ppPollList[i]->Id())
							{
								DbgPrintf(5, _T("CLUSTER STATUS POLL [%s]: Resource %s owner changed"),
											 m_szName, m_pResourceList[k].szName);

								// Resource moved or go up
								if (m_pResourceList[k].dwCurrOwner == 0)
								{
									// Resource up
									PostEvent(EVENT_CLUSTER_RESOURCE_UP, m_dwId, "dsds",
												 m_pResourceList[k].dwId, m_pResourceList[k].szName,
												 ppPollList[i]->Id(), ppPollList[i]->Name());
								}
								else
								{
									// Moved
									NetObj *pObject;

									pObject = FindObjectById(m_pResourceList[k].dwCurrOwner);
									PostEvent(EVENT_CLUSTER_RESOURCE_MOVED, m_dwId, "dsdsds",
												 m_pResourceList[k].dwId, m_pResourceList[k].szName,
												 m_pResourceList[k].dwCurrOwner,
												 (pObject != NULL) ? pObject->Name() : _T("<unknown>"),
												 ppPollList[i]->Id(), ppPollList[i]->Name());
								}
								m_pResourceList[k].dwCurrOwner = ppPollList[i]->Id();
								bModified = TRUE;
							}
							pbResourceFound[k] = 1;
						}
					}
				}
				UnlockData();
				delete pIfList;
			}
			else
			{
				DbgPrintf(6, _T("CLUSTER STATUS POLL [%s]: Cannot get interface list from %s"),
							 m_szName, ppPollList[i]->Name());
			}
		}

		// Check for missing virtual addresses
		LockData();
		for(i = 0; i < m_dwNumResources; i++)
		{
			if ((!pbResourceFound[i]) && (m_pResourceList[i].dwCurrOwner != 0))
			{
				NetObj *pObject;

				pObject = FindObjectById(m_pResourceList[i].dwCurrOwner);
				PostEvent(EVENT_CLUSTER_RESOURCE_DOWN, m_dwId, "dsds",
							 m_pResourceList[i].dwId, m_pResourceList[i].szName,
							 m_pResourceList[i].dwCurrOwner,
							 (pObject != NULL) ? pObject->Name() : _T("<unknown>"));
				m_pResourceList[i].dwCurrOwner = 0;
				bModified = TRUE;
			}
		}
		UnlockData();
		safe_free(pbResourceFound);
	}

	// Cleanup
	for(i = 0; i < dwPollListSize; i++)
	{
		ppPollList[i]->DecRefCount();
	}
	safe_free(ppPollList);

	LockData();
	if (bModified)
		Modify();
	m_tmLastPoll = time(NULL);
	m_dwFlags &= ~CLF_QUEUED_FOR_STATUS_POLL;
	UnlockData();

	DbgPrintf(6, _T("CLUSTER STATUS POLL [%s]: Finished"), m_szName);
}


//
// Check if node is current owner of resource
//

BOOL Cluster::isResourceOnNode(DWORD dwResource, DWORD dwNode)
{
	BOOL bRet = FALSE;
	DWORD i;

	LockData();
	for(i = 0; i < m_dwNumResources; i++)
	{
		if (m_pResourceList[i].dwId == dwResource)
		{
			if (m_pResourceList[i].dwCurrOwner == dwNode)
				bRet = TRUE;
			break;
		}
	}
	UnlockData();
	return bRet;
}
