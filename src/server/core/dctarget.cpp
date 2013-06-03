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
BOOL DataCollectionTarget::DeleteFromDB()
{
   TCHAR szQuery[256];
   BOOL bSuccess;

   bSuccess = Template::DeleteFromDB();
   if (bSuccess)
   {
      _sntprintf(szQuery, 256, _T("DROP TABLE idata_%d"), (int)m_dwId);
      QueueSQLRequest(szQuery);
      _sntprintf(szQuery, 256, _T("DROP TABLE tdata_%d"), (int)m_dwId);
      QueueSQLRequest(szQuery);
   }
   return bSuccess;
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
DWORD DataCollectionTarget::ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked)
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
	lockDciAccess();
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
   lockDciAccess();
   for(int i = 0; i < m_dcObjects->size(); i++)
      m_dcObjects->get(i)->deleteExpiredData();
   unlockDciAccess();
}

/**
 * Get last collected values of given table
 */
DWORD DataCollectionTarget::getTableLastValues(DWORD dciId, CSCPMessage *msg)
{
	DWORD rcc = RCC_INVALID_DCI_ID;

   lockDciAccess();

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
bool DataCollectionTarget::applyTemplateItem(DWORD dwTemplateId, DCObject *dcObject)
{
   bool bResult = true;

   lockDciAccess();	// write lock

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
		if (m_dcObjects->get(i)->getStatus() != ITEM_STATUS_DISABLED || (g_dwFlags & AF_APPLY_TO_DISABLED_DCI_FROM_TEMPLATE))
		{
			m_dcObjects->get(i)->updateFromTemplate(dcObject);
			DbgPrintf(9, _T("DCO \"%s\" NOT disabled or ApplyDCIFromTemplateToDisabledDCI set, updated (%d)"), 
				dcObject->getName(), m_dcObjects->get(i)->getStatus());
		}
		else
		{
			DbgPrintf(9, _T("DCO \"%s\" is disabled and ApplyDCIFromTemplateToDisabledDCI not set, no update (%d)"), 
				dcObject->getName(), m_dcObjects->get(i)->getStatus());
		}
   }

   unlockDciAccess();

	if (bResult)
	{
		LockData();
		m_bIsModified = TRUE;
		UnlockData();
	}
   return bResult;
}

/**
 * Clean deleted template items from target's DCI list
 * Arguments is template id and list of valid template item ids.
 * all items related to given template and not presented in list should be deleted.
 */
void DataCollectionTarget::cleanDeletedTemplateItems(DWORD dwTemplateId, DWORD dwNumItems, DWORD *pdwItemList)
{
   DWORD i, j, dwNumDeleted, *pdwDeleteList;

   lockDciAccess();  // write lock

   pdwDeleteList = (DWORD *)malloc(sizeof(DWORD) * m_dcObjects->size());
   dwNumDeleted = 0;

   for(i = 0; i < (DWORD)m_dcObjects->size(); i++)
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
void DataCollectionTarget::unbindFromTemplate(DWORD dwTemplateId, BOOL bRemoveDCI)
{
   DWORD i;

   if (bRemoveDCI)
   {
      lockDciAccess();  // write lock

		DWORD *pdwDeleteList = (DWORD *)malloc(sizeof(DWORD) * m_dcObjects->size());
		DWORD dwNumDeleted = 0;

      for(i = 0; i < (DWORD)m_dcObjects->size(); i++)
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
      lockDciAccess();

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
DWORD DataCollectionTarget::getPerfTabDCIList(CSCPMessage *pMsg)
{
	lockDciAccess();

	DWORD dwId = VID_SYSDCI_LIST_BASE, dwCount = 0;
   for(int i = 0; i < m_dcObjects->size(); i++)
	{
		DCObject *object = m_dcObjects->get(i);
		if ((object->getPerfTabSettings() != NULL) && object->hasValue())
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
DWORD DataCollectionTarget::getThresholdSummary(CSCPMessage *msg, DWORD baseId)
{
	DWORD varId = baseId;

	msg->SetVariable(varId++, m_dwId);
	DWORD countId = varId++;
	DWORD count = 0;

	lockDciAccess();
   for(int i = 0; i < m_dcObjects->size(); i++)
	{
		DCObject *object = m_dcObjects->get(i);
		if (object->hasValue() && (object->getType() == DCO_TYPE_ITEM))
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
void DataCollectionTarget::processNewDCValue(DCObject *dco, time_t currTime, void *value)
{
	lockDciAccess();
	dco->processNewValue(currTime, value);
	unlockDciAccess();
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
   if ((m_iStatus == STATUS_UNMANAGED) || isDataCollectionDisabled() || m_bIsDeleted)
      return;  // Do not collect data for unmanaged objects or if data collection is disabled

   time_t currTime = time(NULL);

   lockDciAccess();
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
DWORD DataCollectionTarget::getInternalItem(const TCHAR *szParam, DWORD dwBufSize, TCHAR *szBuffer)
{
   DWORD dwError = DCE_SUCCESS;

   if (!_tcsicmp(szParam, _T("Status")))
   {
      _sntprintf(szBuffer, dwBufSize, _T("%d"), m_iStatus);
   }
   else if (!_tcsicmp(szParam, _T("Dummy")))
   {
      _tcscpy(szBuffer, _T("0"));
   }
   else if (MatchString(_T("ChildStatus(*)"), szParam, FALSE))
   {
      TCHAR *pEnd, szArg[256];
      DWORD i, dwId;
      NetObj *pObject = NULL;

      AgentGetParameterArg(szParam, 1, szArg, 256);
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
         _sntprintf(szBuffer, dwBufSize, _T("%d"), pObject->Status());
      }
      else
      {
         dwError = DCE_NOT_SUPPORTED;
      }
   }
   else if (MatchString(_T("ConditionStatus(*)"), szParam, FALSE))
   {
      TCHAR *pEnd, szArg[256];
      DWORD dwId;
      NetObj *pObject = NULL;

      AgentGetParameterArg(szParam, 1, szArg, 256);
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
				_sntprintf(szBuffer, dwBufSize, _T("%d"), pObject->Status());
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
   else
   {
      dwError = DCE_NOT_SUPPORTED;
   }

   return dwError;
}

/**
 * Get last (current) DCI values for summary table.
 */
void DataCollectionTarget::getLastValuesSummary(SummaryTable *tableDefinition, Table *tableData)
{
   bool rowAdded = false;
   lockDciAccess();
   for(int i = 0; i < tableDefinition->getNumColumns(); i++)
   {
      SummaryTableColumn *tc = tableDefinition->getColumn(i);
      for(int j = 0; j < m_dcObjects->size(); j++)
	   {
		   DCObject *object = m_dcObjects->get(j);
         if ((object->getType() == DCO_TYPE_ITEM) && object->hasValue() && 
             ((tc->m_flags & COLUMN_DEFINITION_REGEXP_MATCH) ? 
               RegexpMatch(object->getName(), tc->m_dciName, FALSE) :
               !_tcsicmp(object->getName(), tc->m_dciName)
             ))
         {
            if (!rowAdded)
            {
               tableData->addRow();
               tableData->set(0, m_szName);
               rowAdded = true;
            }
            tableData->set(i + 1, ((DCItem *)object)->getLastValue());
            tableData->setColumnFormat(i + 1, ((DCItem *)object)->getDataType());
         }
      }
   }
   unlockDciAccess();
}
