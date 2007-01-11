/* $Id: cluster.cpp,v 1.2 2007-01-11 10:38:05 victor Exp $ */
/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006 Victor Kirhenshtein
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
}


//
// Cluster class new object constructor
//

Cluster::Cluster(TCHAR *pszName)
        :Template(pszName)
{
	m_dwClusterType = 0;
	m_dwNumSyncNets = 0;
	m_pSyncNetList= NULL;
	m_dwNumResources = 0;
	m_pResourceList = NULL;
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

   if (!LoadCommonProperties())
   {
      DbgPrintf(AF_DEBUG_OBJECTS, "Cannot load common properties for cluster object %d", dwId);
      return FALSE;
   }

   if (!m_bIsDeleted)
   {
		// Load member nodes
		_stprintf(szQuery, _T("SELECT node_id FROM cluster_members WHERE cluster_id=%d"), m_dwId);
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
                  WriteLog(MSG_CLUSTER_MEMBER_NOT_NODE, EVENTLOG_ERROR_TYPE, "dd", m_dwId, dwNodeId);
						break;
					}
				}
				else
				{
               WriteLog(MSG_INVALID_CLUSTER_MEMBER, EVENTLOG_ERROR_TYPE, "dd", m_dwId, dwNodeId);
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
			_stprintf(szQuery, _T("SELECT subnet_addr,subnet_mask FROM cluster_sync_subnets WHERE cluster_id=%d"), m_dwId);
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
			_stprintf(szQuery, _T("SELECT resource_id,resource_name,ip_addr FROM cluster_resources WHERE cluster_id=%d"), m_dwId);
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
						m_pResourceList[i].dwCurrOwner = 0;
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

   SaveCommonProperties(hdb);

   // Check for object's existence in database
   _stprintf(szQuery, _T("SELECT id FROM clusters WHERE id=%d"), m_dwId);
   hResult = DBSelect(hdb, szQuery);
   if (hResult != 0)
   {
      if (DBGetNumRows(hResult) > 0)
         bNewObject = FALSE;
      DBFreeResult(hResult);
   }
   if (bNewObject)
      _sntprintf(szQuery, 4096,
                 _T("INSERT INTO clusters (id,cluster_type) VALUES (%d,%d)"),
                 m_dwId, m_dwClusterType);
   else
      _sntprintf(szQuery, 4096,
                 _T("UPDATE clusters SET cluster_type=%d WHERE id=%d"),
                 m_dwClusterType, m_dwId);
   bResult = DBQuery(hdb, szQuery);

   // Save data collection items
   if (bResult)
   {
      for(i = 0; i < m_dwNumItems; i++)
         m_ppItems[i]->SaveToDB(hdb);

		// Save cluster members list
		if (DBBegin(hdb))
		{
			_stprintf(szQuery, _T("DELETE FROM cluster_members WHERE cluster_id=%d"), m_dwId);
			DBQuery(hdb, szQuery);
			LockChildList(FALSE);
			for(i = 0; i < m_dwChildCount; i++)
			{
				if (m_pChildList[i]->Type() == OBJECT_NODE)
				{
					_stprintf(szQuery, _T("INSERT INTO cluster_members (cluster_id,node_id) VALUES (%d,%d)"),
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
				_stprintf(szQuery, _T("DELETE FROM cluster_sync_subnets WHERE cluster_id=%d"), m_dwId);
				DBQuery(hdb, szQuery);
				for(i = 0; i < m_dwNumSyncNets; i++)
				{
					_stprintf(szQuery, _T("INSERT INTO cluster_sync_subnets (cluster_id,subnet_addr,subnet_mask) VALUES (%d,'%s','%s')"),
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
				_stprintf(szQuery, _T("DELETE FROM cluster_resources WHERE cluster_id=%d"), m_dwId);
				DBQuery(hdb, szQuery);
				for(i = 0; i < m_dwNumResources; i++)
				{
					pszEscName = EncodeSQLString(m_pResourceList[i].szName);
					_stprintf(szQuery, _T("INSERT INTO cluster_resources (cluster_id,resource_id,resource_name,ip_addr) VALUES (%d,%d,'%s','%s')"),
								 m_dwId, m_pResourceList[i].dwId, pszEscName,
								 IpToStr(m_pResourceList[i].dwIpAddr, szIpAddr));
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
   SaveACLToDB(hdb);

   // Clear modifications flag and unlock object
	if (bResult)
		m_bIsModified = FALSE;
   UnlockData();

   return bResult;
}


//
// Delete object from database
//

BOOL Cluster::DeleteFromDB(void)
{
   TCHAR szQuery[256];
   BOOL bSuccess;

   bSuccess = Template::DeleteFromDB();
   if (bSuccess)
   {
      _stprintf(szQuery, _T("DELETE FROM clusters WHERE id=%d"), m_dwId);
      QueueSQLRequest(szQuery);
      _stprintf(szQuery, _T("DELETE FROM cluster_members WHERE cluster_id=%d"), m_dwId);
      QueueSQLRequest(szQuery);
      _stprintf(szQuery, _T("DELETE FROM cluster_sync_subnets WHERE cluster_id=%d"), m_dwId);
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
// Check if given address is within sync network
//

BOOL Cluster::IsSyncAddr(DWORD dwAddr)
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

BOOL Cluster::IsVirtualAddr(DWORD dwAddr)
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

void Cluster::StatusPoll(ClientSession *pSession, DWORD dwRqId, int nPoller)
{
	DWORD i, j, k;
	INTERFACE_LIST *pIfList;
	BOOL bModified = FALSE;

	// Perform status poll on all member nodes
	LockChildList(FALSE);
	for(i = 0; i < m_dwChildCount; i++)
	{
		if (m_pChildList[i]->Type() == OBJECT_NODE)
		{
			((Node *)m_pChildList[i])->StatusPoll(pSession, dwRqId, nPoller);
		}
	}
	UnlockChildList();

	// Check for cluster resource movement
	LockChildList(FALSE);
	for(i = 0; i < m_dwChildCount; i++)
	{
		if (m_pChildList[i]->Type() == OBJECT_NODE)
		{
			pIfList = ((Node *)m_pChildList[i])->GetInterfaceList();
			if (pIfList != NULL)
			{
				for(j = 0; j < (DWORD)pIfList->iNumEntries; j++)
				{
					for(k = 0; k < m_dwNumResources; k++)
					{
						if (m_pResourceList[k].dwIpAddr == pIfList->pInterfaces[j].dwIpAddr)
						{
							if (m_pResourceList[k].dwCurrOwner != m_pChildList[i]->Id())
							{
								// Resource moved or go up
								if (m_pResourceList[k].dwCurrOwner == 0)
								{
									// Resource up
									PostEvent(EVENT_CLUSTER_RESOURCE_UP, m_dwId, "dsds",
									          m_pResourceList[k].dwId, m_pResourceList[k].szName,
                                     m_pChildList[i]->Id(), m_pChildList[i]->Name());
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
                                     m_pChildList[i]->Id(), m_pChildList[i]->Name());
								}
								m_pResourceList[k].dwCurrOwner = m_pChildList[i]->Id();
								bModified = TRUE;
							}
						}
					}
				}
				DestroyInterfaceList(pIfList);
			}
		}
	}
	UnlockChildList();

	LockData();
	if (bModified)
		Modify();
	m_tmLastPoll = time(NULL);
	m_bQueuedForPolling = FALSE;
	UnlockData();
}
