/*
** NetXMS - Network Management System
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
** File: dctarget.cpp
**
**/

#include "nxcore.h"

/**
 * Default constructor
 */
DataCollectionTarget::DataCollectionTarget() : Template()
{
}

/**
 * Constructor for creating new data collection capable objects
 */
DataCollectionTarget::DataCollectionTarget(const TCHAR *name) : Template(name)
{
}

/**
 * Destructor
 */
DataCollectionTarget::~DataCollectionTarget()
{
}

/**
 * Delete object from database
 */
bool DataCollectionTarget::deleteFromDB(DB_HANDLE hdb)
{
   bool success = Template::deleteFromDB(hdb);
   if (success)
   {
      TCHAR query[256];
      _sntprintf(query, 256, _T("DROP TABLE idata_%d"), (int)m_dwId);
      success = DBQuery(hdb, query) ? true : false;
      if (success)
      {
         _sntprintf(query, 256, _T("DROP TABLE tdata_rows_%d"), (int)m_dwId);
         success = DBQuery(hdb, query) ? true : false;
      }
      if (success)
      {
         _sntprintf(query, 256, _T("DROP TABLE tdata_records_%d"), (int)m_dwId);
         success = DBQuery(hdb, query) ? true : false;
      }
      if (success)
      {
         _sntprintf(query, 256, _T("DROP TABLE tdata_%d"), (int)m_dwId);
         success = DBQuery(hdb, query) ? true : false;
      }
   }
   return success;
}

/**
 * Create NXCP message with object's data
 */
void DataCollectionTarget::CreateMessage(CSCPMessage *msg)
{
   Template::CreateMessage(msg);
}

/**
 * Modify object from message
 */
UINT32 DataCollectionTarget::ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked)
{
   if (!bAlreadyLocked)
      LockData();

   return Template::ModifyFromMessage(pRequest, TRUE);
}

/**
 * Update cache for all DCI's
 */
void DataCollectionTarget::updateDciCache()
{
	lockDciAccess(false);
   for(int i = 0; i < m_dcObjects->size(); i++)
	{
		if (m_dcObjects->get(i)->getType() == DCO_TYPE_ITEM)
		{
			((DCItem *)m_dcObjects->get(i))->updateCacheSize();
		}
	}
	unlockDciAccess();
}

/**
 * Clean expired DCI data
 */
void DataCollectionTarget::cleanDCIData()
{
   lockDciAccess(false);
   for(int i = 0; i < m_dcObjects->size(); i++)
      m_dcObjects->get(i)->deleteExpiredData();
   unlockDciAccess();
}

/**
 * Get last collected values of given table
 */
UINT32 DataCollectionTarget::getTableLastValues(UINT32 dciId, CSCPMessage *msg)
{
	UINT32 rcc = RCC_INVALID_DCI_ID;

   lockDciAccess(false);

   for(int i = 0; i < m_dcObjects->size(); i++)
	{
		DCObject *object = m_dcObjects->get(i);
		if ((object->getId() == dciId) && (object->getType() == DCO_TYPE_TABLE))
		{
			((DCTable *)object)->fillLastValueMessage(msg);
			rcc = RCC_SUCCESS;
			break;
		}
	}

   unlockDciAccess();
   return rcc;
}

/**
 * Apply DCI from template
 * pItem passed to this method should be a template's DCI
 */
bool DataCollectionTarget::applyTemplateItem(UINT32 dwTemplateId, DCObject *dcObject)
{
   bool bResult = true;

   lockDciAccess(true);	// write lock

   DbgPrintf(5, _T("Applying DCO \"%s\" to target \"%s\""), dcObject->getName(), m_szName);

   // Check if that template item exists
	int i;
   for(i = 0; i < m_dcObjects->size(); i++)
      if ((m_dcObjects->get(i)->getTemplateId() == dwTemplateId) &&
          (m_dcObjects->get(i)->getTemplateItemId() == dcObject->getId()))
         break;   // Item with specified id already exist

   if (i == m_dcObjects->size())
   {
      // New item from template, just add it
		DCObject *newObject;
		switch(dcObject->getType())
		{
			case DCO_TYPE_ITEM:
				newObject = new DCItem((DCItem *)dcObject);
				break;
			case DCO_TYPE_TABLE:
				newObject = new DCTable((DCTable *)dcObject);
				break;
			default:
				newObject = NULL;
				break;
		}
		if (newObject != NULL)
		{
			newObject->setTemplateId(dwTemplateId, dcObject->getId());
			newObject->changeBinding(CreateUniqueId(IDG_ITEM), this, TRUE);
			bResult = addDCObject(newObject, true);
		}
   }
   else
   {
      // Update existing item unless it is disabled
      DCObject *curr = m_dcObjects->get(i);
		if (curr->getStatus() != ITEM_STATUS_DISABLED || (g_flags & AF_APPLY_TO_DISABLED_DCI_FROM_TEMPLATE))
		{
			curr->updateFromTemplate(dcObject);
			DbgPrintf(9, _T("DCO \"%s\" NOT disabled or ApplyDCIFromTemplateToDisabledDCI set, updated (%d)"),
				dcObject->getName(), curr->getStatus());
         if ((curr->getType() == DCO_TYPE_ITEM) && (((DCItem *)curr)->getInstanceDiscoveryMethod() != IDM_NONE))
         {
            updateInstanceDiscoveryItems((DCItem *)curr);
         }
		}
		else
		{
			DbgPrintf(9, _T("DCO \"%s\" is disabled and ApplyDCIFromTemplateToDisabledDCI not set, no update (%d)"),
				dcObject->getName(), curr->getStatus());
		}
   }

   unlockDciAccess();

	if (bResult)
	{
		LockData();
		m_isModified = true;
		UnlockData();
	}
   return bResult;
}

/**
 * Clean deleted template items from target's DCI list
 * Arguments is template id and list of valid template item ids.
 * all items related to given template and not presented in list should be deleted.
 */
void DataCollectionTarget::cleanDeletedTemplateItems(UINT32 dwTemplateId, UINT32 dwNumItems, UINT32 *pdwItemList)
{
   UINT32 i, j, dwNumDeleted, *pdwDeleteList;

   lockDciAccess(true);  // write lock

   pdwDeleteList = (UINT32 *)malloc(sizeof(UINT32) * m_dcObjects->size());
   dwNumDeleted = 0;

   for(i = 0; i < (UINT32)m_dcObjects->size(); i++)
      if (m_dcObjects->get(i)->getTemplateId() == dwTemplateId)
      {
         for(j = 0; j < dwNumItems; j++)
            if (m_dcObjects->get(i)->getTemplateItemId() == pdwItemList[j])
               break;

         // Delete DCI if it's not in list
         if (j == dwNumItems)
            pdwDeleteList[dwNumDeleted++] = m_dcObjects->get(i)->getId();
      }

   for(i = 0; i < dwNumDeleted; i++)
      deleteDCObject(pdwDeleteList[i], false);

   unlockDciAccess();
   free(pdwDeleteList);
}

/**
 * Unbind data collection target from template, i.e either remove DCI
 * association with template or remove these DCIs at all
 */
void DataCollectionTarget::unbindFromTemplate(UINT32 dwTemplateId, BOOL bRemoveDCI)
{
   UINT32 i;

   if (bRemoveDCI)
   {
      lockDciAccess(true);  // write lock

		UINT32 *pdwDeleteList = (UINT32 *)malloc(sizeof(UINT32) * m_dcObjects->size());
		UINT32 dwNumDeleted = 0;

      for(i = 0; i < (UINT32)m_dcObjects->size(); i++)
         if (m_dcObjects->get(i)->getTemplateId() == dwTemplateId)
         {
            pdwDeleteList[dwNumDeleted++] = m_dcObjects->get(i)->getId();
         }

		for(i = 0; i < dwNumDeleted; i++)
			deleteDCObject(pdwDeleteList[i], false);

      unlockDciAccess();

		safe_free(pdwDeleteList);
   }
   else
   {
      lockDciAccess(false);

      for(int i = 0; i < m_dcObjects->size(); i++)
         if (m_dcObjects->get(i)->getTemplateId() == dwTemplateId)
         {
            m_dcObjects->get(i)->setTemplateId(0, 0);
         }

      unlockDciAccess();
   }
}

/**
 * Get list of DCIs to be shown on performance tab
 */
UINT32 DataCollectionTarget::getPerfTabDCIList(CSCPMessage *pMsg)
{
	lockDciAccess(false);

	UINT32 dwId = VID_SYSDCI_LIST_BASE, dwCount = 0;
   for(int i = 0; i < m_dcObjects->size(); i++)
	{
		DCObject *object = m_dcObjects->get(i);
      if ((object->getPerfTabSettings() != NULL) &&
          object->hasValue() &&
          (object->getStatus() == ITEM_STATUS_ACTIVE) &&
          object->matchClusterResource())
		{
			pMsg->SetVariable(dwId++, object->getId());
			pMsg->SetVariable(dwId++, object->getDescription());
			pMsg->SetVariable(dwId++, (WORD)object->getStatus());
			pMsg->SetVariable(dwId++, object->getPerfTabSettings());
			pMsg->SetVariable(dwId++, (WORD)object->getType());
			pMsg->SetVariable(dwId++, object->getTemplateItemId());
			if (object->getType() == DCO_TYPE_ITEM)
			{
				pMsg->SetVariable(dwId++, ((DCItem *)object)->getInstance());
				dwId += 3;
			}
			else
			{
				dwId += 4;
			}
			dwCount++;
		}
	}
   pMsg->SetVariable(VID_NUM_ITEMS, dwCount);

	unlockDciAccess();
   return RCC_SUCCESS;
}

/**
 * Get threshold violation summary into NXCP message
 */
UINT32 DataCollectionTarget::getThresholdSummary(CSCPMessage *msg, UINT32 baseId)
{
	UINT32 varId = baseId;

	msg->SetVariable(varId++, m_dwId);
	UINT32 countId = varId++;
	UINT32 count = 0;

	lockDciAccess(false);
   for(int i = 0; i < m_dcObjects->size(); i++)
	{
		DCObject *object = m_dcObjects->get(i);
		if (object->hasValue() && (object->getType() == DCO_TYPE_ITEM) && object->getStatus() == ITEM_STATUS_ACTIVE)
		{
			if (((DCItem *)object)->hasActiveThreshold())
			{
				((DCItem *)object)->fillLastValueMessage(msg, varId);
				varId += 50;
				count++;
			}
		}
	}
	unlockDciAccess();
	msg->SetVariable(countId, count);
   return varId;
}

/**
 * Process new DCI value
 */
bool DataCollectionTarget::processNewDCValue(DCObject *dco, time_t currTime, void *value)
{
	lockDciAccess(false);
	bool result = dco->processNewValue(currTime, value);
	unlockDciAccess();
   return result;
}

/**
 * Check if data collection is disabled
 */
bool DataCollectionTarget::isDataCollectionDisabled()
{
	return false;
}

/**
 * Put items which requires polling into the queue
 */
void DataCollectionTarget::queueItemsForPolling(Queue *pPollerQueue)
{
   if ((m_iStatus == STATUS_UNMANAGED) || isDataCollectionDisabled() || m_isDeleted)
      return;  // Do not collect data for unmanaged objects or if data collection is disabled

   time_t currTime = time(NULL);

   lockDciAccess(false);
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
		DCObject *object = m_dcObjects->get(i);
      if (object->isReadyForPolling(currTime))
      {
         object->setBusyFlag(TRUE);
         incRefCount();   // Increment reference count for each queued DCI
         pPollerQueue->Put(object);
			DbgPrintf(8, _T("DataCollectionTarget(%s)->QueueItemsForPolling(): item %d \"%s\" added to queue"), m_szName, object->getId(), object->getName());
      }
   }
   unlockDciAccess();
}

/**
 * Get value for server's internal parameter
 */
UINT32 DataCollectionTarget::getInternalItem(const TCHAR *param, size_t bufSize, TCHAR *buffer)
{
   UINT32 dwError = DCE_SUCCESS;

   if (!_tcsicmp(param, _T("Status")))
   {
      _sntprintf(buffer, bufSize, _T("%d"), m_iStatus);
   }
   else if (!_tcsicmp(param, _T("Dummy")) || MatchString(_T("Dummy(*)"), param, FALSE))
   {
      _tcscpy(buffer, _T("0"));
   }
   else if (MatchString(_T("ChildStatus(*)"), param, FALSE))
   {
      TCHAR *pEnd, szArg[256];
      UINT32 i, dwId;
      NetObj *pObject = NULL;

      AgentGetParameterArg(param, 1, szArg, 256);
      dwId = _tcstoul(szArg, &pEnd, 0);
      if (*pEnd != 0)
      {
         // Argument is object's name
         dwId = 0;
      }

      // Find child object with requested ID or name
      LockChildList(FALSE);
      for(i = 0; i < m_dwChildCount; i++)
      {
         if (((dwId == 0) && (!_tcsicmp(m_pChildList[i]->Name(), szArg))) ||
             (dwId == m_pChildList[i]->Id()))
         {
            pObject = m_pChildList[i];
            break;
         }
      }
      UnlockChildList();

      if (pObject != NULL)
      {
         _sntprintf(buffer, bufSize, _T("%d"), pObject->Status());
      }
      else
      {
         dwError = DCE_NOT_SUPPORTED;
      }
   }
   else if (MatchString(_T("ConditionStatus(*)"), param, FALSE))
   {
      TCHAR *pEnd, szArg[256];
      UINT32 dwId;
      NetObj *pObject = NULL;

      AgentGetParameterArg(param, 1, szArg, 256);
      dwId = _tcstoul(szArg, &pEnd, 0);
      if (*pEnd == 0)
		{
			pObject = FindObjectById(dwId);
			if (pObject != NULL)
				if (pObject->Type() != OBJECT_CONDITION)
					pObject = NULL;
		}
		else
      {
         // Argument is object's name
			pObject = FindObjectByName(szArg, OBJECT_CONDITION);
      }

      if (pObject != NULL)
      {
			if (pObject->isTrustedNode(m_dwId))
			{
				_sntprintf(buffer, bufSize, _T("%d"), pObject->Status());
			}
			else
			{
	         dwError = DCE_NOT_SUPPORTED;
			}
      }
      else
      {
         dwError = DCE_NOT_SUPPORTED;
      }
   }
   else if (MatchString(_T("PingTime(*)"), param, FALSE))
   {
      TCHAR *pEnd, szArg[256];
      UINT32 i, dwId;
      NetObj *pObject = NULL;

      AgentGetParameterArg(param, 1, szArg, 256);
      dwId = _tcstoul(szArg, &pEnd, 0);
      if (*pEnd != 0)
      {
         // Argument is object's name
         dwId = 0;
      }

      // Find child object with requested ID or name
      LockChildList(FALSE);
      for(i = 0; i < m_dwChildCount; i++)
      {
         if (((dwId == 0) && (!_tcsicmp(m_pChildList[i]->Name(), szArg))) ||
             (dwId == m_pChildList[i]->Id()))
         {
            pObject = m_pChildList[i];
            break;
         }
      }
      UnlockChildList();

      if (pObject != NULL)
      {
         _sntprintf(buffer, bufSize, _T("%d"), ((Interface *)pObject)->getPingTime());
      }
      else
      {
         dwError = DCE_NOT_SUPPORTED;
      }
   }
   else if (!_tcsicmp(_T("PingTime"), param))
   {
      NetObj *pObject = NULL;

      // Find child object with requested ID or name
      LockChildList(FALSE);
      for(int i = 0; i < (int)m_dwChildCount; i++)
      {
         if (m_pChildList[i]->IpAddr() == m_dwIpAddr)
         {
            pObject = m_pChildList[i];
            break;
         }
      }
      UnlockChildList();

      if (pObject != NULL)
      {
         _sntprintf(buffer, bufSize, _T("%d"), ((Interface *)pObject)->getPingTime());
      }
      else
      {
         dwError = DCE_NOT_SUPPORTED;
      }
   }
   else
   {
      dwError = DCE_NOT_SUPPORTED;
   }

   return dwError;
}

/**
 * Get parameter value from NXSL script
 */
UINT32 DataCollectionTarget::getScriptItem(const TCHAR *param, size_t bufSize, TCHAR *buffer)
{
   TCHAR name[256];
   nx_strncpy(name, param, 256);
   Trim(name);

   ObjectArray<NXSL_Value> args(16, 16, false);

   // Can be in form parameter(arg1, arg2, ... argN)
   TCHAR *p = _tcschr(name, _T('('));
   if (p != NULL)
   {
      if (name[_tcslen(name) - 1] != _T(')'))
         return DCE_NOT_SUPPORTED;
      name[_tcslen(name) - 1] = 0;

      *p = 0;
      p++;

      TCHAR *s;
      do
      {
         s = _tcschr(p, _T(','));
         if (s != NULL)
            *s = 0;
         Trim(p);
         args.add(new NXSL_Value(p));
         p = s + 1;
      } while(s != NULL);
   }

   UINT32 rc = DCE_NOT_SUPPORTED;
   NXSL_VM *vm = g_pScriptLibrary->createVM(name, new NXSL_ServerEnv);
   if (vm != NULL)
   {
      vm->setGlobalVariable(_T("$object"), new NXSL_Value(new NXSL_Object(&g_nxslNetObjClass, this)));
      if (Type() == OBJECT_NODE)
      {
         vm->setGlobalVariable(_T("$node"), new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, this)));
      }
      vm->setGlobalVariable(_T("$isCluster"), new NXSL_Value((Type() == OBJECT_CLUSTER) ? 1 : 0));
      if (vm->run(&args))
      {
         NXSL_Value *value = vm->getResult();
         nx_strncpy(buffer, value->getValueAsCString(), bufSize);
         rc = DCE_SUCCESS;
      }
      else
      {
			DbgPrintf(4, _T("DataCollectionTarget(%s)->getScriptItem(%s): Script execution error: %s"), m_szName, param, vm->getErrorText());
			PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", name, vm->getErrorText(), m_dwId);
         rc = DCE_COMM_ERROR;
      }
      delete vm;
   }
   else
   {
      args.setOwner(true);
   }
   DbgPrintf(7, _T("DataCollectionTarget(%s)->getScriptItem(%s): rc=%d"), m_szName, param, rc);
   return rc;
}

/**
 * Get last (current) DCI values for summary table.
 */
void DataCollectionTarget::getLastValuesSummary(SummaryTable *tableDefinition, Table *tableData)
{
   bool rowAdded = false;
   lockDciAccess(false);
   for(int i = 0; i < tableDefinition->getNumColumns(); i++)
   {
      SummaryTableColumn *tc = tableDefinition->getColumn(i);
      for(int j = 0; j < m_dcObjects->size(); j++)
	   {
		   DCObject *object = m_dcObjects->get(j);
         if ((object->getType() == DCO_TYPE_ITEM) && object->hasValue() &&
             (object->getStatus() == ITEM_STATUS_ACTIVE) &&
             ((tc->m_flags & COLUMN_DEFINITION_REGEXP_MATCH) ?
               RegexpMatch(object->getName(), tc->m_dciName, FALSE) :
               !_tcsicmp(object->getName(), tc->m_dciName)
             ))
         {
            if (!rowAdded)
            {
               tableData->addRow();
               tableData->set(0, m_szName);
               tableData->setObjectId(tableData->getNumRows() - 1, m_dwId);
               rowAdded = true;
            }
            tableData->set(i + 1, ((DCItem *)object)->getLastValue());
            tableData->setStatus(i + 1, ((DCItem *)object)->getThresholdSeverity());
            tableData->getColumnDefinitions()->get(i + 1)->setDataType(((DCItem *)object)->getDataType());
         }
      }
   }
   unlockDciAccess();
}

/**
 * Must return true if object is a possible event source
 */
bool DataCollectionTarget::isEventSource()
{
   return true;
}
