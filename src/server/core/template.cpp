/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2010 Victor Kirhenshtein
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
** File: template.cpp
**
**/

#include "nxcore.h"


//
// Redefined status calculation for template group
//

void TemplateGroup::calculateCompoundStatus(BOOL bForcedRecalc)
{
   m_iStatus = STATUS_NORMAL;
}


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
	m_applyFilter = NULL;
	m_applyFilterSource = NULL;
   m_iStatus = STATUS_NORMAL;
	m_mutexDciAccess = MutexCreateRecursive();
}


//
// Constructor for new template object
//

Template::Template(const TCHAR *pszName)
         :NetObj()
{
   nx_strncpy(m_szName, pszName, MAX_OBJECT_NAME);
   m_dwNumItems = 0;
   m_ppItems = NULL;
   m_dwDCILockStatus = INVALID_INDEX;
   m_dwVersion = 0x00010000;  // Initial version is 1.0
	m_applyFilter = NULL;
	m_applyFilterSource = NULL;
   m_iStatus = STATUS_NORMAL;
   m_bIsHidden = TRUE;
	m_mutexDciAccess = MutexCreateRecursive();
}


//
// Create template object from import file
//

Template::Template(ConfigEntry *config)
         :NetObj()
{
   m_bIsHidden = TRUE;
   m_dwDCILockStatus = INVALID_INDEX;
   m_iStatus = STATUS_NORMAL;
	m_mutexDciAccess = MutexCreateRecursive();

	// Name and version
	nx_strncpy(m_szName, config->getSubEntryValue(_T("name"), 0, _T("Unnamed Template")), MAX_OBJECT_NAME);
	m_dwVersion = config->getSubEntryValueUInt(_T("version"), 0, 0x00010000);

	// Auto-apply filter
	m_applyFilter = NULL;
	m_applyFilterSource = NULL;
	setAutoApplyFilter(config->getSubEntryValue(_T("filter")));

	// Data collection
	ConfigEntry *dcRoot = config->findEntry(_T("dataCollection"));
	if (dcRoot != NULL)
	{
		ConfigEntryList *dcis = dcRoot->getSubEntries(_T("dci#*"));
		m_dwNumItems = (DWORD)dcis->getSize();
		m_ppItems = (DCItem **)malloc(sizeof(DCItem *) * m_dwNumItems);
		for(int i = 0; i < dcis->getSize(); i++)
		{
			m_ppItems[i] = new DCItem(dcis->getEntry(i), this);
		}
		delete dcis;
	}
	else
	{
		m_dwNumItems = 0;
		m_ppItems = NULL;
	}
}


//
// Destructor
//

Template::~Template()
{
   destroyItems();
	delete m_applyFilter;
	safe_free(m_applyFilterSource);
	MutexDestroy(m_mutexDciAccess);
}


//
// Destroy all related data collection items
//

void Template::destroyItems()
{
   DWORD i;

   for(i = 0; i < m_dwNumItems; i++)
      delete m_ppItems[i];
   safe_free(m_ppItems);
   m_dwNumItems = 0;
   m_ppItems = NULL;
}


//
// Set auto apply filter
//

void Template::setAutoApplyFilter(const TCHAR *filter)
{
	LockData();
	safe_free(m_applyFilterSource);
	delete m_applyFilter;
	if (filter != NULL)
	{
		TCHAR error[256];

		m_applyFilterSource = _tcsdup(filter);
		m_applyFilter = NXSLCompile(m_applyFilterSource, error, 256);
		if (m_applyFilter == NULL)
			nxlog_write(MSG_TEMPLATE_SCRIPT_COMPILATION_ERROR, EVENTLOG_WARNING_TYPE, "dss", m_dwId, m_szName, error);
	}
	else
	{
		m_applyFilterSource = NULL;
		m_applyFilter = NULL;
	}
	Modify();
	UnlockData();
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

   if (!loadCommonProperties())
      return FALSE;

   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT version,enable_auto_apply,apply_filter FROM templates WHERE id=%d"), dwId);
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
	if (DBGetFieldLong(hResult, 0, 1))
	{
		m_applyFilterSource = DBGetField(hResult, 0, 2, NULL, 0);
		if (m_applyFilterSource != NULL)
		{
			TCHAR error[256];

			DecodeSQLString(m_applyFilterSource);
			m_applyFilter = NXSLCompile(m_applyFilterSource, error, 256);
			if (m_applyFilter == NULL)
				nxlog_write(MSG_TEMPLATE_SCRIPT_COMPILATION_ERROR, EVENTLOG_WARNING_TYPE, "dss", m_dwId, m_szName, error);
		}
	}
   DBFreeResult(hResult);

   // Load DCI and access list
   loadACLFromDB();
   loadItemsFromDB();
   for(i = 0; i < (int)m_dwNumItems; i++)
      if (!m_ppItems[i]->loadThresholdsFromDB())
         bResult = FALSE;

   // Load related nodes list
   if (!m_bIsDeleted)
   {
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT node_id FROM dct_node_map WHERE template_id=%d"), m_dwId);
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
                  nxlog_write(MSG_DCT_MAP_NOT_NODE, EVENTLOG_ERROR_TYPE, "dd", m_dwId, dwNodeId);
               }
            }
            else
            {
               nxlog_write(MSG_INVALID_DCT_MAP, EVENTLOG_ERROR_TYPE, "dd", m_dwId, dwNodeId);
            }
         }
         DBFreeResult(hResult);
      }
   }

	m_iStatus = STATUS_NORMAL;

   return bResult;
}


//
// Save object to database
//

BOOL Template::SaveToDB(DB_HANDLE hdb)
{
   TCHAR *pszQuery, *pszEscScript, query2[256];
   DB_RESULT hResult;
   DWORD i, len;
   BOOL bNewObject = TRUE;

   // Lock object's access
   LockData();

   saveCommonProperties(hdb);

   // Check for object's existence in database
   _sntprintf(query2, 128, _T("SELECT id FROM templates WHERE id=%d"), m_dwId);
   hResult = DBSelect(hdb, query2);
   if (hResult != 0)
   {
      if (DBGetNumRows(hResult) > 0)
         bNewObject = FALSE;
      DBFreeResult(hResult);
   }

   // Form and execute INSERT or UPDATE query
	pszEscScript = (m_applyFilterSource != NULL) ? EncodeSQLString(m_applyFilterSource) : _tcsdup(_T("#00"));
	len = (DWORD)_tcslen(pszEscScript) + 256;
	pszQuery = (TCHAR *)malloc(sizeof(TCHAR) * len);
   if (bNewObject)
      _sntprintf(pszQuery, len, _T("INSERT INTO templates (id,version,enable_auto_apply,apply_filter) VALUES (%d,%d,%d,'%s')"),
		           m_dwId, m_dwVersion, (m_applyFilterSource != NULL) ? 1 : 0, pszEscScript);
   else
      _sntprintf(pszQuery, len, _T("UPDATE templates SET version=%d,enable_auto_apply=%d,apply_filter='%s' WHERE id=%d"),
                 m_dwVersion, (m_applyFilterSource != NULL) ? 1 : 0, pszEscScript, m_dwId);
	free(pszEscScript);
   DBQuery(hdb, pszQuery);
	free(pszQuery);

   // Update members list
   _sntprintf(query2, 256, _T("DELETE FROM dct_node_map WHERE template_id=%d"), m_dwId);
   DBQuery(hdb, query2);
   LockChildList(FALSE);
   for(i = 0; i < m_dwChildCount; i++)
   {
      _sntprintf(query2, 256, _T("INSERT INTO dct_node_map (template_id,node_id) VALUES (%d,%d)"), m_dwId, m_pChildList[i]->Id());
      DBQuery(hdb, query2);
   }
   UnlockChildList();

   // Save access list
   saveACLToDB(hdb);

   UnlockData();

   // Save data collection items
	lockDciAccess();
   for(i = 0; i < m_dwNumItems; i++)
      m_ppItems[i]->saveToDB(hdb);
	unlockDciAccess();

   // Clear modifications flag
	LockData();
   m_bIsModified = FALSE;
	UnlockData();

   return TRUE;
}


//
// Delete object from database
//

BOOL Template::DeleteFromDB()
{
   TCHAR szQuery[256];
   BOOL bSuccess;

   bSuccess = NetObj::DeleteFromDB();
   if (bSuccess)
   {
      if (Type() == OBJECT_TEMPLATE)
      {
         _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM templates WHERE id=%d"), m_dwId);
         QueueSQLRequest(szQuery);
         _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM dct_node_map WHERE template_id=%d"), m_dwId);
         QueueSQLRequest(szQuery);
      }
      else
      {
         _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM dct_node_map WHERE node_id=%d"), m_dwId);
         QueueSQLRequest(szQuery);
      }
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM items WHERE node_id=%d"), m_dwId);
      QueueSQLRequest(szQuery);
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("UPDATE items SET template_id=0 WHERE template_id=%d"), m_dwId);
      QueueSQLRequest(szQuery);
   }
   return bSuccess;
}


//
// Load data collection items from database
//

void Template::loadItemsFromDB()
{
   TCHAR szQuery[512];
   DB_RESULT hResult;

   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR),
	           _T("SELECT item_id,name,source,datatype,polling_interval,retention_time,")
              _T("status,delta_calculation,transformation,template_id,description,")
              _T("instance,template_item_id,flags,resource_id,")
              _T("proxy_node,base_units,unit_multiplier,custom_units_name,")
	           _T("perftab_settings,system_tag,snmp_port,snmp_raw_value_type FROM items WHERE node_id=%d"), m_dwId);
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

bool Template::addItem(DCItem *pItem, bool alreadyLocked)
{
   DWORD i;
   bool success = false;

   if (!alreadyLocked)
      lockDciAccess(); // write lock

   // Check if that item exists
   for(i = 0; i < m_dwNumItems; i++)
      if (m_ppItems[i]->getId() == pItem->getId())
         break;   // Item with specified id already exist
   
   if (i == m_dwNumItems)     // Add new item
   {
      m_dwNumItems++;
      m_ppItems = (DCItem **)realloc(m_ppItems, sizeof(DCItem *) * m_dwNumItems);
      m_ppItems[i] = pItem;
      m_ppItems[i]->setLastPollTime(0);    // Cause item to be polled immediatelly
      if (m_ppItems[i]->getStatus() != ITEM_STATUS_DISABLED)
         m_ppItems[i]->setStatus(ITEM_STATUS_ACTIVE, false);
      m_ppItems[i]->setBusyFlag(FALSE);
      success = true;
   }

   if (!alreadyLocked)
      unlockDciAccess();

	if (success)
	{
		LockData();
      Modify();
		UnlockData();
	}
   return success;
}


//
// Delete item from node
//

bool Template::deleteItem(DWORD dwItemId, bool needLock)
{
   DWORD i;
   bool success = false;

	if (needLock)
		lockDciAccess();  // write lock

   // Check if that item exists
   for(i = 0; i < m_dwNumItems; i++)
      if (m_ppItems[i]->getId() == dwItemId)
      {
         // Destroy item
			DbgPrintf(7, _T("Template::DeleteItem: deleting DCI %d from object %d"), m_ppItems[i]->getId(), m_dwId);
         if (m_ppItems[i]->prepareForDeletion())
			{
				// Physically delete DCI only if it is not busy
				// Busy DCIs will be deleted by data collector
				m_ppItems[i]->deleteFromDB();
				delete m_ppItems[i];
			}
			else
			{
				DbgPrintf(7, _T("Template::DeleteItem: destruction of DCI %d delayed"), m_ppItems[i]->getId());
			}
         m_dwNumItems--;
         memmove(&m_ppItems[i], &m_ppItems[i + 1], sizeof(DCItem *) * (m_dwNumItems - i));
         success = true;
			DbgPrintf(7, _T("Template::DeleteItem: DCI deleted from object %d"), m_dwId);
         break;
      }

	if (needLock)
	   unlockDciAccess();
   return success;
}


//
// Modify data collection item from NXCP message
//

bool Template::updateItem(DWORD dwItemId, CSCPMessage *pMsg, DWORD *pdwNumMaps, 
                          DWORD **ppdwMapIndex, DWORD **ppdwMapId)
{
   DWORD i;
   bool success = false;

   lockDciAccess();

   // Check if that item exists
   for(i = 0; i < m_dwNumItems; i++)
      if (m_ppItems[i]->getId() == dwItemId)
      {
         m_ppItems[i]->updateFromMessage(pMsg, pdwNumMaps, ppdwMapIndex, ppdwMapId);
         success = true;
         m_bIsModified = TRUE;
         break;
      }

   unlockDciAccess();
   return success;
}


//
// Set status for group of DCIs
//

bool Template::setItemStatus(DWORD dwNumItems, DWORD *pdwItemList, int iStatus)
{
   DWORD i, j;
   bool success = true;

   lockDciAccess();
   for(i = 0; i < dwNumItems; i++)
   {
      for(j = 0; j < m_dwNumItems; j++)
      {
         if (m_ppItems[j]->getId() == pdwItemList[i])
         {
            m_ppItems[j]->setStatus(iStatus, true);
            break;
         }
      }
      if (j == m_dwNumItems)
         success = false;     // Invalid DCI ID provided
   }
   unlockDciAccess();
   return success;
}


//
// Lock data collection items list
//

BOOL Template::LockDCIList(DWORD dwSessionId, const TCHAR *pszNewOwner, TCHAR *pszCurrOwner)
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

   lockDciAccess();

   // Walk through items list
   for(i = 0; i < m_dwNumItems; i++)
   {
		if ((_tcsnicmp(m_ppItems[i]->getDescription(), _T("@system."), 8)) ||
			 (Type() == OBJECT_TEMPLATE))
		{
			m_ppItems[i]->createMessage(&msg);
			pSession->sendMessage(&msg);
			msg.DeleteAllVariables();
		}
   }

   unlockDciAccess();

   // Send end-of-list indicator
	msg.SetEndOfSequence();
   pSession->sendMessage(&msg);
}


//
// Get DCI item's type
//

int Template::getItemType(DWORD dwItemId)
{
   DWORD i;
   int iType = -1;

   lockDciAccess();
   // Check if that item exists
   for(i = 0; i < m_dwNumItems; i++)
      if (m_ppItems[i]->getId() == dwItemId)
      {
         iType = m_ppItems[i]->getDataType();
         break;
      }

   unlockDciAccess();
   return iType;
}


//
// Get item by it's id
//

DCItem *Template::getItemById(DWORD dwItemId)
{
   DWORD i;
   DCItem *pItem = NULL;

   lockDciAccess();
   // Check if that item exists
   for(i = 0; i < m_dwNumItems; i++)
      if (m_ppItems[i]->getId() == dwItemId)
      {
         pItem = m_ppItems[i];
         break;
      }

   unlockDciAccess();
   return pItem;
}


//
// Get item by it's name (case-insensetive)
//

DCItem *Template::getItemByName(const TCHAR *pszName)
{
   DWORD i;
   DCItem *pItem = NULL;

   lockDciAccess();
   // Check if that item exists
   for(i = 0; i < m_dwNumItems; i++)
      if (!_tcsicmp(m_ppItems[i]->getName(), pszName))
      {
         pItem = m_ppItems[i];
         break;
      }

   unlockDciAccess();
   return pItem;
}


//
// Get item by it's description (case-insensetive)
//

DCItem *Template::getItemByDescription(const TCHAR *pszDescription)
{
   DWORD i;
   DCItem *pItem = NULL;

   lockDciAccess();
   // Check if that item exists
   for(i = 0; i < m_dwNumItems; i++)
      if (!_tcsicmp(m_ppItems[i]->getDescription(), pszDescription))
      {
         pItem = m_ppItems[i];
         break;
      }

   unlockDciAccess();
   return pItem;
}


//
// Get item by it's index
//

DCItem *Template::getItemByIndex(DWORD dwIndex)
{
   DCItem *pItem = NULL;

   lockDciAccess();

   if (dwIndex < m_dwNumItems)
      pItem = m_ppItems[dwIndex];

   unlockDciAccess();
   return pItem;
}


//
// Redefined status calculation for template
//

void Template::calculateCompoundStatus(BOOL bForcedRecalc)
{
   m_iStatus = STATUS_NORMAL;
}


//
// Create CSCP message with object's data
//

void Template::CreateMessage(CSCPMessage *pMsg)
{
   NetObj::CreateMessage(pMsg);
   pMsg->SetVariable(VID_TEMPLATE_VERSION, m_dwVersion);
	pMsg->SetVariable(VID_AUTO_APPLY, (WORD)((m_applyFilterSource != NULL) ? 1 : 0));
	pMsg->SetVariable(VID_APPLY_FILTER, CHECK_NULL_EX(m_applyFilterSource));
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

   // Change apply filter
	if (pRequest->GetVariableShort(VID_AUTO_APPLY))
	{
		safe_free(m_applyFilterSource);
		delete m_applyFilter;
		m_applyFilterSource = pRequest->GetVariableStr(VID_APPLY_FILTER);
		if (m_applyFilterSource != NULL)
		{
			TCHAR error[256];

			m_applyFilter = NXSLCompile(m_applyFilterSource, error, 256);
			if (m_applyFilter == NULL)
				nxlog_write(MSG_TEMPLATE_SCRIPT_COMPILATION_ERROR, EVENTLOG_WARNING_TYPE, "dss", m_dwId, m_szName, error);
		}
		else
		{
			m_applyFilter = NULL;
		}
	}
	else
	{
		delete_and_null(m_applyFilter);
		safe_free_and_null(m_applyFilterSource);
	}

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
   DbgPrintf(2, _T("Apply %d items from template \"%s\" to node \"%s\""),
             m_dwNumItems, m_szName, pNode->Name());

   // Copy items
   for(i = 0; i < m_dwNumItems; i++)
   {
      if (m_ppItems[i] != NULL)
      {
         pdwItemList[i] = m_ppItems[i]->getId();
         if (!pNode->applyTemplateItem(m_dwId, m_ppItems[i]))
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
   pNode->cleanDeletedTemplateItems(m_dwId, m_dwNumItems, pdwItemList);

   // Cleanup
   free(pdwItemList);

   return bErrors;
}


//
// Queue template update
//

void Template::queueUpdate()
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

void Template::queueRemoveFromNode(DWORD dwNodeId, BOOL bRemoveDCI)
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

DWORD *Template::getDCIEventsList(DWORD *pdwCount)
{
   DWORD i, j, *pdwList;
   DCItem *pItem = NULL;

   pdwList = NULL;
   *pdwCount = 0;

   lockDciAccess();
   for(i = 0; i < m_dwNumItems; i++)
   {
      m_ppItems[i]->getEventList(&pdwList, pdwCount);
   }
   unlockDciAccess();

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

   str.addFormattedString(_T("\t\t<template id=\"%d\">\n\t\t\t<name>%s</name>\n\t\t\t<dataCollection>\n"),
	                       m_dwId, (const TCHAR *)EscapeStringForXML2(m_szName));

   lockDciAccess();
   for(i = 0; i < m_dwNumItems; i++)
      m_ppItems[i]->createNXMPRecord(str);
   unlockDciAccess();

   str += _T("\t\t\t</dataCollection>\n");
	LockData();
	if (m_applyFilterSource != NULL)
	{
		str += _T("\t\t\t<filter>");
		str.addDynamicString(EscapeStringForXML(m_applyFilterSource, -1));
		str += _T("</filter>\n");
	}
	UnlockData();
	str += _T("\t\t</template>\n");
}


//
// Validate template agains specific DCI list
//

void Template::ValidateDCIList(DCI_CFG *cfg)
{
	DWORD i, j, dwNumDeleted, *pdwDeleteList;

	lockDciAccess(); // write lock

	pdwDeleteList = (DWORD *)malloc(sizeof(DWORD) * m_dwNumItems);
	dwNumDeleted = 0;

	for(i = 0; i < m_dwNumItems; i++)
	{
		for(j = 0; cfg[j].pszName != NULL; j++)
		{
			if (!_tcsicmp(m_ppItems[i]->getDescription(), cfg[j].pszName))
			{
				m_ppItems[i]->systemModify(cfg[j].pszParam, cfg[j].nOrigin,
					                        cfg[j].nRetention, cfg[j].nInterval,
													cfg[j].nDataType);
				cfg[j].nFound = 1;
				break;
			}
		}

		// Mark non-existing items
		if (cfg[j].pszName == NULL)
         pdwDeleteList[dwNumDeleted++] = m_ppItems[i]->getId();
	}

	// Delete unneeded items
	for(i = 0; i < dwNumDeleted; i++)
		deleteItem(pdwDeleteList[i], false);

	safe_free(pdwDeleteList);

	// Create missing items
	for(i = 0; cfg[i].pszName != NULL; i++)
	{
		if (!cfg[i].nFound)
		{
			addItem(new DCItem(CreateUniqueId(IDG_ITEM), cfg[i].pszParam,
				                cfg[i].nOrigin, cfg[i].nDataType,
									 cfg[i].nInterval, cfg[i].nRetention,
									 this, cfg[i].pszName), true);
		}
	}

	unlockDciAccess();
}


//
// Validate system template
//

void Template::ValidateSystemTemplate()
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
			{ _T("@system.cpu_usage.cisco.0"), _T(".1.3.6.1.4.1.9.9.109.1.1.1.1.7.0"),
			  60, 1, DCI_DT_INT, DS_SNMP_AGENT, 0 },		// Cisco devices
			{ _T("@system.cpu_usage.cisco.1"), _T(".1.3.6.1.4.1.9.9.109.1.1.1.1.7.1"),
			  60, 1, DCI_DT_INT, DS_SNMP_AGENT, 0 },		// Cisco devices - CPU #1
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


//
// Enumerate all DCIs
//

BOOL Template::EnumDCI(BOOL (* pfCallback)(DCItem *, DWORD, void *), void *pArg)
{
	DWORD i;
	BOOL bRet = TRUE;

	lockDciAccess();
	for(i = 0; i < m_dwNumItems; i++)
	{
		if (!pfCallback(m_ppItems[i], i, pArg))
		{
			bRet = FALSE;
			break;
		}
	}
	unlockDciAccess();
	return bRet;
}


//
// (Re)associate all DCIs
//

void Template::associateItems()
{
	DWORD i;

	lockDciAccess();
	for(i = 0; i < m_dwNumItems; i++)
		m_ppItems[i]->changeBinding(0, this, FALSE);
	unlockDciAccess();
}


//
// Prepare template for deletion
//

void Template::PrepareForDeletion(void)
{
	if (Type() == OBJECT_TEMPLATE)
	{
		DWORD i;
	
		LockChildList(FALSE);
		for(i = 0; i < m_dwChildCount; i++)
		{
			if (m_pChildList[i]->Type() == OBJECT_NODE)
				queueRemoveFromNode(m_pChildList[i]->Id(), TRUE);
		}
		UnlockChildList();
	}
	NetObj::PrepareForDeletion();
}


//
// Check if template should be automatically applied to node
//

BOOL Template::isApplicable(Node *node)
{
	NXSL_ServerEnv *pEnv;
	NXSL_Value *value;
	BOOL result = FALSE;

	LockData();
	if (m_applyFilter != NULL)
	{
		pEnv = new NXSL_ServerEnv;
		m_applyFilter->setGlobalVariable(_T("$node"), new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, node)));
		if (m_applyFilter->run(pEnv, 0, NULL) == 0)
		{
			value = m_applyFilter->getResult();
			result = ((value != NULL) && (value->getValueAsInt32() != 0));
		}
		else
		{
			TCHAR buffer[1024];

			_sntprintf(buffer, 1024, _T("Template::%s::%d"), m_szName, m_dwId);
			PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", buffer,
						 m_applyFilter->getErrorText(), m_dwId);
			nxlog_write(MSG_TEMPLATE_SCRIPT_EXECUTION_ERROR, EVENTLOG_WARNING_TYPE, "dss", m_dwId, m_szName, m_applyFilter->getErrorText());
		}
	}
	UnlockData();
	return result;
}
