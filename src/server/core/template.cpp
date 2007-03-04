/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: template.cpp
**
**/

#include "nxcore.h"


//
// Template object constructor
//

Template::Template()
         :NetObj()
{
   m_dwNumItems = 0;
   m_ppItems = NULL;
   m_dwDCILockStatus = INVALID_INDEX;
   m_dwVersion = 0x00010000;  // Initial version is 1.0
}


//
// Constructor for new template object
//

Template::Template(TCHAR *pszName)
         :NetObj()
{
   nx_strncpy(m_szName, pszName, MAX_OBJECT_NAME);
   m_dwNumItems = 0;
   m_ppItems = NULL;
   m_dwDCILockStatus = INVALID_INDEX;
   m_dwVersion = 0x00010000;  // Initial version is 1.0
   m_bIsHidden = TRUE;
}


//
// Destructor
//

Template::~Template()
{
   DestroyItems();
}


//
// Destroy all related data collection items
//

void Template::DestroyItems(void)
{
   DWORD i;

   for(i = 0; i < m_dwNumItems; i++)
      delete m_ppItems[i];
   safe_free(m_ppItems);
   m_dwNumItems = 0;
   m_ppItems = NULL;
}


//
// Create object from database data
//

BOOL Template::CreateFromDB(DWORD dwId)
{
   TCHAR szQuery[256];
   DB_RESULT hResult;
   DWORD i, dwNumNodes, dwNodeId;
   NetObj *pObject;
   BOOL bResult = TRUE;

   m_dwId = dwId;

   if (!LoadCommonProperties())
      return FALSE;

   _stprintf(szQuery, _T("SELECT version FROM templates WHERE id=%d"), dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult == NULL)
      return FALSE;     // Query failed

   if (DBGetNumRows(hResult) == 0)
   {
      // No object with given ID in database
      DBFreeResult(hResult);
      return FALSE;
   }

   m_dwVersion = DBGetFieldULong(hResult, 0, 0);
   DBFreeResult(hResult);

   // Load DCI and access list
   LoadACLFromDB();
   LoadItemsFromDB();
   for(i = 0; i < (int)m_dwNumItems; i++)
      if (!m_ppItems[i]->LoadThresholdsFromDB())
         bResult = FALSE;

   // Load related nodes list
   if (!m_bIsDeleted)
   {
      _stprintf(szQuery, _T("SELECT node_id FROM dct_node_map WHERE template_id=%d"), m_dwId);
      hResult = DBSelect(g_hCoreDB, szQuery);
      if (hResult != NULL)
      {
         dwNumNodes = DBGetNumRows(hResult);
         for(i = 0; i < dwNumNodes; i++)
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
                  WriteLog(MSG_DCT_MAP_NOT_NODE, EVENTLOG_ERROR_TYPE, "dd", m_dwId, dwNodeId);
               }
            }
            else
            {
               WriteLog(MSG_INVALID_DCT_MAP, EVENTLOG_ERROR_TYPE, "dd", m_dwId, dwNodeId);
            }
         }
         DBFreeResult(hResult);
      }
   }

   return bResult;
}


//
// Save object to database
//

BOOL Template::SaveToDB(DB_HANDLE hdb)
{
   TCHAR szQuery[1024];
   DB_RESULT hResult;
   DWORD i;
   BOOL bNewObject = TRUE;

   // Lock object's access
   LockData();

   SaveCommonProperties(hdb);

   // Check for object's existence in database
   _stprintf(szQuery, _T("SELECT id FROM templates WHERE id=%d"), m_dwId);
   hResult = DBSelect(hdb, szQuery);
   if (hResult != 0)
   {
      if (DBGetNumRows(hResult) > 0)
         bNewObject = FALSE;
      DBFreeResult(hResult);
   }

   // Form and execute INSERT or UPDATE query
   if (bNewObject)
      sprintf(szQuery, "INSERT INTO templates (id,version) VALUES (%d,%d)",
              m_dwId, m_dwVersion);
   else
      sprintf(szQuery, "UPDATE templates SET version=%d WHERE id=%d",
              m_dwVersion, m_dwId);
   DBQuery(hdb, szQuery);

   // Update members list
   sprintf(szQuery, "DELETE FROM dct_node_map WHERE template_id=%d", m_dwId);
   DBQuery(hdb, szQuery);
   LockChildList(FALSE);
   for(i = 0; i < m_dwChildCount; i++)
   {
      sprintf(szQuery, "INSERT INTO dct_node_map (template_id,node_id) VALUES (%d,%d)", m_dwId, m_pChildList[i]->Id());
      DBQuery(hdb, szQuery);
   }
   UnlockChildList();

   // Save data collection items
   for(i = 0; i < m_dwNumItems; i++)
      m_ppItems[i]->SaveToDB(hdb);

   // Save access list
   SaveACLToDB(hdb);

   // Clear modifications flag and unlock object
   m_bIsModified = FALSE;
   UnlockData();

   return TRUE;
}


//
// Delete object from database
//

BOOL Template::DeleteFromDB(void)
{
   char szQuery[256];
   BOOL bSuccess;

   bSuccess = NetObj::DeleteFromDB();
   if (bSuccess)
   {
      if (Type() == OBJECT_TEMPLATE)
      {
         sprintf(szQuery, "DELETE FROM templates WHERE id=%d", m_dwId);
         QueueSQLRequest(szQuery);
         sprintf(szQuery, "DELETE FROM dct_node_map WHERE template_id=%d", m_dwId);
         QueueSQLRequest(szQuery);
      }
      else
      {
         sprintf(szQuery, "DELETE FROM dct_node_map WHERE node_id=%d", m_dwId);
         QueueSQLRequest(szQuery);
      }
      sprintf(szQuery, "DELETE FROM items WHERE node_id=%d", m_dwId);
      QueueSQLRequest(szQuery);
      sprintf(szQuery, "UPDATE items SET template_id=0 WHERE template_id=%d", m_dwId);
      QueueSQLRequest(szQuery);
   }
   return bSuccess;
}


//
// Load data collection items from database
//

void Template::LoadItemsFromDB(void)
{
   char szQuery[256];
   DB_RESULT hResult;

   sprintf(szQuery, "SELECT item_id,name,source,datatype,polling_interval,retention_time,"
                    "status,delta_calculation,transformation,template_id,description,"
                    "instance,template_item_id,adv_schedule,all_thresholds,resource_id "
                    "FROM items WHERE node_id=%d", m_dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);

   if (hResult != 0)
   {
      int i, iRows;

      iRows = DBGetNumRows(hResult);
      if (iRows > 0)
      {
         m_dwNumItems = iRows;
         m_ppItems = (DCItem **)malloc(sizeof(DCItem *) * iRows);
         for(i = 0; i < iRows; i++)
            m_ppItems[i] = new DCItem(hResult, i, this);
      }
      DBFreeResult(hResult);
   }
}


//
// Add item to node
//

BOOL Template::AddItem(DCItem *pItem, BOOL bLocked)
{
   DWORD i;
   BOOL bResult = FALSE;

   if (!bLocked)
      LockData();

   // Check if that item exists
   for(i = 0; i < m_dwNumItems; i++)
      if (m_ppItems[i]->Id() == pItem->Id())
         break;   // Item with specified id already exist
   
   if (i == m_dwNumItems)     // Add new item
   {
      m_dwNumItems++;
      m_ppItems = (DCItem **)realloc(m_ppItems, sizeof(DCItem *) * m_dwNumItems);
      m_ppItems[i] = pItem;
      m_ppItems[i]->SetLastPollTime(0);    // Cause item to be polled immediatelly
      if (m_ppItems[i]->Status() != ITEM_STATUS_DISABLED)
         m_ppItems[i]->SetStatus(ITEM_STATUS_ACTIVE);
      m_ppItems[i]->SetBusyFlag(FALSE);
      Modify();
      bResult = TRUE;
   }

   if (!bLocked)
      UnlockData();
   return bResult;
}


//
// Delete item from node
//

BOOL Template::DeleteItem(DWORD dwItemId, BOOL bNeedLock)
{
   DWORD i;
   BOOL bResult = FALSE;

	if (bNeedLock)
			LockData();

   // Check if that item exists
   for(i = 0; i < m_dwNumItems; i++)
      if (m_ppItems[i]->Id() == dwItemId)
      {
         // Destroy item
         m_ppItems[i]->PrepareForDeletion();
         m_ppItems[i]->DeleteFromDB();
         delete m_ppItems[i];
         m_dwNumItems--;
         memmove(&m_ppItems[i], &m_ppItems[i + 1], sizeof(DCItem *) * (m_dwNumItems - i));
         bResult = TRUE;
         break;
      }

	if (bNeedLock)
	   UnlockData();
   return bResult;
}


//
// Modify data collection item from CSCP message
//

BOOL Template::UpdateItem(DWORD dwItemId, CSCPMessage *pMsg, DWORD *pdwNumMaps, 
                          DWORD **ppdwMapIndex, DWORD **ppdwMapId)
{
   DWORD i;
   BOOL bResult = FALSE;

   LockData();

   // Check if that item exists
   for(i = 0; i < m_dwNumItems; i++)
      if (m_ppItems[i]->Id() == dwItemId)
      {
         m_ppItems[i]->UpdateFromMessage(pMsg, pdwNumMaps, ppdwMapIndex, ppdwMapId);
         bResult = TRUE;
         m_bIsModified = TRUE;
         break;
      }

   UnlockData();
   return bResult;
}


//
// Set status for group of DCIs
//

BOOL Template::SetItemStatus(DWORD dwNumItems, DWORD *pdwItemList, int iStatus)
{
   DWORD i, j;
   BOOL bResult = TRUE;

   LockData();
   for(i = 0; i < dwNumItems; i++)
   {
      for(j = 0; j < m_dwNumItems; j++)
      {
         if (m_ppItems[j]->Id() == pdwItemList[i])
         {
            m_ppItems[j]->SetStatus(iStatus);
            break;
         }
      }
      if (j == m_dwNumItems)
         bResult = FALSE;     // Invalid DCI ID provided
   }
   UnlockData();
   return bResult;
}


//
// Lock data collection items list
//

BOOL Template::LockDCIList(DWORD dwSessionId, TCHAR *pszNewOwner, TCHAR *pszCurrOwner)
{
   BOOL bSuccess;

   LockData();
   if (m_dwDCILockStatus == INVALID_INDEX)
   {
      m_dwDCILockStatus = dwSessionId;
      m_bDCIListModified = FALSE;
      nx_strncpy(m_szCurrDCIOwner, pszNewOwner, MAX_SESSION_NAME);
      bSuccess = TRUE;
   }
   else
   {
      if (pszCurrOwner != NULL)
         _tcscpy(pszCurrOwner, m_szCurrDCIOwner);
      bSuccess = FALSE;
   }
   UnlockData();
   return bSuccess;
}


//
// Unlock data collection items list
//

BOOL Template::UnlockDCIList(DWORD dwSessionId)
{
   BOOL bSuccess = FALSE;

   LockData();
   if (m_dwDCILockStatus == dwSessionId)
   {
      m_dwDCILockStatus = INVALID_INDEX;
      if (m_bDCIListModified && (Type() == OBJECT_TEMPLATE))
      {
         m_dwVersion++;
         Modify();
      }
      m_bDCIListModified = FALSE;
      bSuccess = TRUE;
   }
   UnlockData();
   return bSuccess;
}


//
// Send DCI list to client
//

void Template::SendItemsToClient(ClientSession *pSession, DWORD dwRqId)
{
   CSCPMessage msg;
   DWORD i;

   // Prepare message
   msg.SetId(dwRqId);
   msg.SetCode(CMD_NODE_DCI);

   LockData();

   // Walk through items list
   for(i = 0; i < m_dwNumItems; i++)
   {
		if ((_tcsnicmp(m_ppItems[i]->Description(), _T("@system."), 8)) ||
			 (Type() == OBJECT_TEMPLATE))
		{
			m_ppItems[i]->CreateMessage(&msg);
			pSession->SendMessage(&msg);
			msg.DeleteAllVariables();
		}
   }

   UnlockData();

   // Send end-of-list indicator
   msg.SetCode(CMD_NODE_DCI_LIST_END);
   pSession->SendMessage(&msg);
}


//
// Get DCI item's type
//

int Template::GetItemType(DWORD dwItemId)
{
   DWORD i;
   int iType = -1;

   LockData();
   // Check if that item exists
   for(i = 0; i < m_dwNumItems; i++)
      if (m_ppItems[i]->Id() == dwItemId)
      {
         iType = m_ppItems[i]->DataType();
         break;
      }

   UnlockData();
   return iType;
}


//
// Get item by it's id
//

DCItem *Template::GetItemById(DWORD dwItemId)
{
   DWORD i;
   DCItem *pItem = NULL;

   LockData();
   // Check if that item exists
   for(i = 0; i < m_dwNumItems; i++)
      if (m_ppItems[i]->Id() == dwItemId)
      {
         pItem = m_ppItems[i];
         break;
      }

   UnlockData();
   return pItem;
}


//
// Get item by it's name (case-insensetive)
//

DCItem *Template::GetItemByName(TCHAR *pszName)
{
   DWORD i;
   DCItem *pItem = NULL;

   LockData();
   // Check if that item exists
   for(i = 0; i < m_dwNumItems; i++)
      if (!_tcsicmp(m_ppItems[i]->Name(), pszName))
      {
         pItem = m_ppItems[i];
         break;
      }

   UnlockData();
   return pItem;
}


//
// Get item by it's index
//

DCItem *Template::GetItemByIndex(DWORD dwIndex)
{
   DCItem *pItem = NULL;

   LockData();

   if (dwIndex < m_dwNumItems)
      pItem = m_ppItems[dwIndex];

   UnlockData();
   return pItem;
}


//
// Redefined status calculation for template
//

void Template::CalculateCompoundStatus(BOOL bForcedRecalc)
{
   m_iStatus = STATUS_UNMANAGED;
}


//
// Create CSCP message with object's data
//

void Template::CreateMessage(CSCPMessage *pMsg)
{
   NetObj::CreateMessage(pMsg);
   pMsg->SetVariable(VID_TEMPLATE_VERSION, m_dwVersion);
}


//
// Modify object from message
//

DWORD Template::ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked)
{
   if (!bAlreadyLocked)
      LockData();

   // Change template version
   if (pRequest->IsVariableExist(VID_TEMPLATE_VERSION))
      m_dwVersion = pRequest->GetVariableLong(VID_TEMPLATE_VERSION);

   return NetObj::ModifyFromMessage(pRequest, TRUE);
}


//
// Apply template to node
//

BOOL Template::ApplyToNode(Node *pNode)
{
   DWORD i, *pdwItemList;
   BOOL bErrors = FALSE;

   // Link node to template
   if (!IsChild(pNode->Id()))
   {
      AddChild(pNode);
      pNode->AddParent(this);
   }

   pdwItemList = (DWORD *)malloc(sizeof(DWORD) * m_dwNumItems);
   DbgPrintf(AF_DEBUG_DC, "Apply %d items from template \"%s\" to node \"%s\"",
             m_dwNumItems, m_szName, pNode->Name());

   // Copy items
   for(i = 0; i < m_dwNumItems; i++)
   {
      if (m_ppItems[i] != NULL)
      {
         pdwItemList[i] = m_ppItems[i]->Id();
         if (!pNode->ApplyTemplateItem(m_dwId, m_ppItems[i]))
         {
            bErrors = TRUE;
         }
      }
      else
      {
         bErrors = TRUE;
      }
   }

   // Clean items deleted from template
   pNode->CleanDeletedTemplateItems(m_dwId, m_dwNumItems, pdwItemList);

   // Cleanup
   free(pdwItemList);

   return bErrors;
}


//
// Queue template update
//

void Template::QueueUpdate(void)
{
   DWORD i;
   TEMPLATE_UPDATE_INFO *pInfo;

   LockData();
   for(i = 0; i < m_dwChildCount; i++)
      if (m_pChildList[i]->Type() == OBJECT_NODE)
      {
         IncRefCount();
         pInfo = (TEMPLATE_UPDATE_INFO *)malloc(sizeof(TEMPLATE_UPDATE_INFO));
         pInfo->iUpdateType = APPLY_TEMPLATE;
         pInfo->pTemplate = this;
         pInfo->dwNodeId = m_pChildList[i]->Id();
         g_pTemplateUpdateQueue->Put(pInfo);
      }
   UnlockData();
}


//
// Queue template remove from node
//

void Template::QueueRemoveFromNode(DWORD dwNodeId, BOOL bRemoveDCI)
{
   TEMPLATE_UPDATE_INFO *pInfo;

   LockData();
   IncRefCount();
   pInfo = (TEMPLATE_UPDATE_INFO *)malloc(sizeof(TEMPLATE_UPDATE_INFO));
   pInfo->iUpdateType = REMOVE_TEMPLATE;
   pInfo->pTemplate = this;
   pInfo->dwNodeId = dwNodeId;
   pInfo->bRemoveDCI = bRemoveDCI;
   g_pTemplateUpdateQueue->Put(pInfo);
   UnlockData();
}


//
// Get list of events used by DCIs
//

DWORD *Template::GetDCIEventsList(DWORD *pdwCount)
{
   DWORD i, j, *pdwList;
   DCItem *pItem = NULL;

   pdwList = NULL;
   *pdwCount = 0;

   LockData();
   for(i = 0; i < m_dwNumItems; i++)
   {
      m_ppItems[i]->GetEventList(&pdwList, pdwCount);
   }
   UnlockData();

   // Clean list from duplicates
   for(i = 0; i < *pdwCount; i++)
   {
      for(j = i + 1; j < *pdwCount; j++)
      {
         if (pdwList[i] == pdwList[j])
         {
            (*pdwCount)--;
            memmove(&pdwList[j], &pdwList[j + 1], sizeof(DWORD) * (*pdwCount - j));
            j--;
         }
      }
   }

   return pdwList;
}


//
// Create management pack record
//

void Template::CreateNXMPRecord(String &str)
{
   DWORD i;

   str.AddFormattedString(_T("\t@TEMPLATE %s\n\t{\n\t\t@DCI_LIST\n\t\t{\n"), m_szName);

   LockData();
   for(i = 0; i < m_dwNumItems; i++)
      m_ppItems[i]->CreateNXMPRecord(str);
   UnlockData();

   str += _T("\t\t}\n\t}\n");
}


//
// Validate template agains specific DCI list
//

void Template::ValidateDCIList(DCI_CFG *cfg)
{
	DWORD i, j, dwNumDeleted, *pdwDeleteList;

	LockData();

	pdwDeleteList = (DWORD *)malloc(sizeof(DWORD) * m_dwNumItems);
	dwNumDeleted = 0;

	for(i = 0; i < m_dwNumItems; i++)
	{
		for(j = 0; cfg[j].pszName != NULL; j++)
		{
			if (!_tcsicmp(m_ppItems[i]->Description(), cfg[j].pszName))
			{
				m_ppItems[i]->SystemModify(cfg[j].pszParam, cfg[j].nOrigin,
					                        cfg[j].nRetention, cfg[j].nInterval,
													cfg[j].nDataType);
				cfg[j].nFound = 1;
				break;
			}
		}

		// Mark non-existing items
		if (cfg[j].pszName == NULL)
         pdwDeleteList[dwNumDeleted++] = m_ppItems[i]->Id();
	}

	// Delete unneeded items
	for(i = 0; i < dwNumDeleted; i++)
		DeleteItem(pdwDeleteList[i], FALSE);

	// Create missing items
	for(i = 0; cfg[i].pszName != NULL; i++)
	{
		if (!cfg[i].nFound)
		{
			AddItem(new DCItem(CreateUniqueId(IDG_ITEM), cfg[i].pszParam,
				                cfg[i].nOrigin, cfg[i].nDataType,
									 cfg[i].nInterval, cfg[i].nRetention,
									 this, cfg[i].pszName), TRUE);
		}
	}

	UnlockData();
}


//
// Validate system template
//

void Template::ValidateSystemTemplate(void)
{
	if (!_tcsicmp(m_szName, _T("@System.Agent")))
	{
		DCI_CFG dciCfgAgent[] =
		{
			{ _T("@system.cpu_usage"), _T("System.CPU.Usage"), 60, 1, DCI_DT_INT, DS_NATIVE_AGENT, 0 },
			{ _T("@system.load_avg"), _T("System.CPU.LoadAvg"), 60, 1, DCI_DT_FLOAT, DS_NATIVE_AGENT, 0 },
			{ _T("@system.usedmem"), _T("System.Memory.Physical.Used"), 60, 1, DCI_DT_UINT64, DS_NATIVE_AGENT, 0 },
			{ _T("@system.totalmem"), _T("System.Memory.Physical.Total"), 60, 1, DCI_DT_UINT64, DS_NATIVE_AGENT, 0 },
			{ _T("@system.disk_queue"), _T("System.IO.DiskQueue"), 60, 1, DCI_DT_FLOAT, DS_NATIVE_AGENT, 0 },
			{ NULL, NULL, 0, 0, 0, 0 }
		};

		ValidateDCIList(dciCfgAgent);
	}
	else if (!_tcsicmp(m_szName, _T("@System.SNMP")))
	{
		DCI_CFG dciCfgSNMP[] =
		{
			{ _T("@system.cpu_usage.cisco"), _T(".1.3.6.1.4.1.9.9.109.1.1.1.1.7.0"),
			  60, 1, DCI_DT_INT, DS_SNMP_AGENT, 0 },		// Cisco devices
			{ _T("@system.cpu_usage.passport"), _T(".1.3.6.1.4.1.2272.1.1.20.0"),
			  60, 1, DCI_DT_INT, DS_SNMP_AGENT, 0 },		// Nortel Passport switches
			{ _T("@system.cpu_usage.netscreen"), _T(".1.3.6.1.4.1.3224.16.1.2.0"),
			  60, 1, DCI_DT_INT, DS_SNMP_AGENT, 0 },		// Netscreen devices
			{ _T("@system.cpu_usage.ipso"), _T(".1.3.6.1.4.1.94.1.21.1.7.1.0"),
			  60, 1, DCI_DT_INT, DS_SNMP_AGENT, 0 },		// Nokia IPSO
			{ NULL, NULL, 0, 0, 0, 0 }
		};

		ValidateDCIList(dciCfgSNMP);
	}
}
