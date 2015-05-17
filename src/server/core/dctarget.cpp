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
   m_pingLastTimeStamp = 0;
   m_pingTime = PING_TIME_TIMEOUT;
}

/**
 * Constructor for creating new data collection capable objects
 */
DataCollectionTarget::DataCollectionTarget(const TCHAR *name) : Template(name)
{
   m_pingLastTimeStamp = 0;
   m_pingTime = PING_TIME_TIMEOUT;
}

/**
 * Destructor
 */
DataCollectionTarget::~DataCollectionTarget()
{
   m_pingLastTimeStamp = 0;
   m_pingTime = PING_TIME_TIMEOUT;
}

/**
 * Delete object from database
 */
bool DataCollectionTarget::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = Template::deleteFromDatabase(hdb);
   if (success)
   {
      const TCHAR *cascade = _T("");
      switch(g_dbSyntax)
      {
         case DB_SYNTAX_ORACLE:
            cascade = _T("CASCADE CONSTRAINTS PURGE");
            break;
         case DB_SYNTAX_PGSQL:
            cascade = _T("CASCADE");
            break;
      }

      TCHAR query[256];
      _sntprintf(query, 256, _T("DROP TABLE idata_%d"), (int)m_id);
      QueueSQLRequest(query);

      _sntprintf(query, 256, _T("DROP TABLE tdata_rows_%d %s"), (int)m_id, cascade);
      QueueSQLRequest(query);

      _sntprintf(query, 256, _T("DROP TABLE tdata_records_%d %s"), (int)m_id, cascade);
      QueueSQLRequest(query);

      _sntprintf(query, 256, _T("DROP TABLE tdata_%d %s"), (int)m_id, cascade);
      QueueSQLRequest(query);
   }
   return success;
}

/**
 * Create NXCP message with object's data
 */
void DataCollectionTarget::fillMessageInternal(NXCPMessage *msg)
{
   Template::fillMessageInternal(msg);

   // Sent all DCIs marked for display on overview page
   UINT32 fieldId = VID_OVERVIEW_DCI_LIST_BASE;
   UINT32 count = 0;
   lockDciAccess(false);
   for(int i = 0; i < m_dcObjects->size(); i++)
	{
      DCObject *dci = m_dcObjects->get(i);
      if ((dci->getType() == DCO_TYPE_ITEM) && dci->isShowInObjectOverview() && (dci->getStatus() == ITEM_STATUS_ACTIVE))
		{
         count++;
         ((DCItem *)dci)->fillLastValueMessage(msg, fieldId);
         fieldId += 50;
		}
	}
   unlockDciAccess();
   msg->setField(VID_OVERVIEW_DCI_COUNT, count);
}

/**
 * Modify object from message
 */
UINT32 DataCollectionTarget::modifyFromMessageInternal(NXCPMessage *pRequest)
{
   return Template::modifyFromMessageInternal(pRequest);
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
void DataCollectionTarget::cleanDCIData(DB_HANDLE hdb)
{
   String queryItems = _T("DELETE FROM idata_");
   queryItems.append(m_id);
   queryItems.append(_T(" WHERE "));

   String queryTables = _T("DELETE FROM tdata_");
   queryTables.append(m_id);
   queryTables.append(_T(" WHERE "));

   int itemCount = 0;
   int tableCount = 0;
   time_t now = time(NULL);

   lockDciAccess(false);
   for(int i = 0; i < m_dcObjects->size(); i++)
   {
      DCObject *o = m_dcObjects->get(i);
      if (o->getType() == DCO_TYPE_ITEM)
      {
         if (itemCount > 0)
            queryItems.append(_T(" OR "));
         queryItems.append(_T("(item_id="));
         queryItems.append(o->getId());
         queryItems.append(_T(" AND idata_timestamp<"));
         queryItems.append((INT64)(now - o->getRetentionTime() * 86400));
         queryItems.append(_T(')'));
         itemCount++;
      }
      else if (o->getType() == DCO_TYPE_TABLE)
      {
         if (tableCount > 0)
            queryTables.append(_T(" OR "));
         queryTables.append(_T("(item_id="));
         queryTables.append(o->getId());
         queryTables.append(_T(" AND tdata_timestamp<"));
         queryTables.append((INT64)(now - o->getRetentionTime() * 86400));
         queryTables.append(_T(')'));
         tableCount++;
      }
   }
   unlockDciAccess();

   if (itemCount > 0)
   {
      DbgPrintf(6, _T("DataCollectionTarget::cleanDCIData(%s [%d]): running query \"%s\""), m_name, m_id, (const TCHAR *)queryItems);
      DBQuery(hdb, queryItems);
   }

   if (tableCount > 0)
   {
      DbgPrintf(6, _T("DataCollectionTarget::cleanDCIData(%s [%d]): running query \"%s\""), m_name, m_id, (const TCHAR *)queryTables);
      DBQuery(hdb, queryTables);
   }
}

/**
 * Get last collected values of given table
 */
UINT32 DataCollectionTarget::getTableLastValues(UINT32 dciId, NXCPMessage *msg)
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
 * dcObject passed to this method should be a template's DCI
 */
bool DataCollectionTarget::applyTemplateItem(UINT32 dwTemplateId, DCObject *dcObject)
{
   bool bResult = true;

   lockDciAccess(true);	// write lock

   DbgPrintf(5, _T("Applying DCO \"%s\" to target \"%s\""), dcObject->getName(), m_name);

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
		if ((curr->getStatus() != ITEM_STATUS_DISABLED) || (g_flags & AF_APPLY_TO_DISABLED_DCI_FROM_TEMPLATE))
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
		lockProperties();
		m_isModified = true;
		unlockProperties();
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
void DataCollectionTarget::unbindFromTemplate(UINT32 dwTemplateId, bool removeDCI)
{
   UINT32 i;

   if (removeDCI)
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
UINT32 DataCollectionTarget::getPerfTabDCIList(NXCPMessage *pMsg)
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
			pMsg->setField(dwId++, object->getId());
			pMsg->setField(dwId++, object->getDescription());
			pMsg->setField(dwId++, (WORD)object->getStatus());
			pMsg->setField(dwId++, object->getPerfTabSettings());
			pMsg->setField(dwId++, (WORD)object->getType());
			pMsg->setField(dwId++, object->getTemplateItemId());
			if (object->getType() == DCO_TYPE_ITEM)
			{
				pMsg->setField(dwId++, ((DCItem *)object)->getInstance());
            if ((object->getTemplateItemId() != 0) && (object->getTemplateId() == m_id))
            {
               // DCI created via instance discovery - send ID of root template item
               // to allow UI to resolve double template case
               // (template -> instance discovery item on node -> actual item on node)
               DCObject *src = getDCObjectById(object->getTemplateItemId(), false);
               pMsg->setField(dwId++, (src != NULL) ? src->getTemplateItemId() : 0);
               dwId += 2;
            }
            else
            {
				   dwId += 3;
            }
			}
			else
			{
				dwId += 4;
			}
			dwCount++;
		}
	}
   pMsg->setField(VID_NUM_ITEMS, dwCount);

	unlockDciAccess();
   return RCC_SUCCESS;
}

/**
 * Get threshold violation summary into NXCP message
 */
UINT32 DataCollectionTarget::getThresholdSummary(NXCPMessage *msg, UINT32 baseId)
{
	UINT32 varId = baseId;

	msg->setField(varId++, m_id);
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
	msg->setField(countId, count);
   return varId;
}

/**
 * Process new DCI value
 */
bool DataCollectionTarget::processNewDCValue(DCObject *dco, time_t currTime, const void *value)
{
   bool updateStatus;
	bool result = dco->processNewValue(currTime, value, &updateStatus);
	if (updateStatus)
	{
      calculateCompoundStatus(FALSE);
   }
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
			DbgPrintf(8, _T("DataCollectionTarget(%s)->QueueItemsForPolling(): item %d \"%s\" added to queue"), m_name, object->getId(), object->getName());
      }
   }
   unlockDciAccess();
}

/**
 * Get object from parameter
 */
NetObj *DataCollectionTarget::objectFromParameter(const TCHAR *param)
{
   TCHAR *eptr, arg[256];
   AgentGetParameterArg(param, 1, arg, 256);
   UINT32 objectId = _tcstoul(arg, &eptr, 0);
   if (*eptr != 0)
   {
      // Argument is object's name
      objectId = 0;
   }

   // Find child object with requested ID or name
   NetObj *object = NULL;
   LockChildList(FALSE);
   for(UINT32 i = 0; i < m_dwChildCount; i++)
   {
      if (((objectId == 0) && (!_tcsicmp(m_pChildList[i]->getName(), arg))) ||
          (objectId == m_pChildList[i]->getId()))
      {
         object = m_pChildList[i];
         break;
      }
   }
   UnlockChildList();
   return object;
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
      NetObj *object = objectFromParameter(param);
      if (object != NULL)
      {
         _sntprintf(buffer, bufSize, _T("%d"), object->Status());
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
				if (pObject->getObjectClass() != OBJECT_CONDITION)
					pObject = NULL;
		}
		else
      {
         // Argument is object's name
			pObject = FindObjectByName(szArg, OBJECT_CONDITION);
      }

      if (pObject != NULL)
      {
			if (pObject->isTrustedNode(m_id))
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
   else
   {
      dwError = DCE_NOT_SUPPORTED;
   }

   return dwError;
}

/**
 * Parse value list
 */
static bool ParseValueList(TCHAR **start, ObjectArray<NXSL_Value> &args)
{
   TCHAR *p = *start;

   *p = 0;
   p++;

   TCHAR *s = p;
   int state = 1; // normal text

   for(; state > 0; p++)
   {
      switch(*p)
      {
         case '"':
            if (state == 1)
            {
               state = 2;
               s = p + 1;
            }
            else
            {
               state = 3;
               *p = 0;
               args.add(new NXSL_Value(s));
            }
            break;
         case ',':
            if (state == 1)
            {
               *p = 0;
               Trim(s);
               args.add(new NXSL_Value(s));
               s = p + 1;
            }
            else if (state == 3)
            {
               state = 1;
               s = p + 1;
            }
            break;
         case 0:
            if (state == 1)
            {
               Trim(s);
               args.add(new NXSL_Value(s));
               state = 0;
            }
            else
            {
               state = -1; // error
            }
            break;
         case ' ':
            break;
         case ')':
            if (state == 1)
            {
               *p = 0;
               Trim(s);
               args.add(new NXSL_Value(s));
               state = 0;
            }
            else if (state == 3)
            {
               state = 0;
            }
            break;
         case '\\':
            if (state == 2)
            {
               memmove(p, p + 1, _tcslen(p) * sizeof(TCHAR));
               switch(*p)
               {
                  case 'r':
                     *p = '\r';
                     break;
                  case 'n':
                     *p = '\n';
                     break;
                  case 't':
                     *p = '\t';
                     break;
                  default:
                     break;
               }
            }
            else if (state == 3)
            {
               state = -1;
            }
            break;
         case '%':
            if ((state == 1) && (*(p + 1) == '('))
            {
               p++;
               ObjectArray<NXSL_Value> elements(16, 16, false);
               if (ParseValueList(&p, elements))
               {
                  NXSL_Array *array = new NXSL_Array();
                  for(int i = 0; i < elements.size(); i++)
                  {
                     array->set(i, elements.get(i));
                  }
                  args.add(new NXSL_Value(array));
                  state = 3;
               }
               else
               {
                  state = -1;
                  elements.clear();
               }
            }
            else if (state == 3)
            {
               state = -1;
            }
            break;
         default:
            if (state == 3)
               state = -1;
            break;
      }
   }

   *start = p - 1;
   return (state != -1);
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

      if (!ParseValueList(&p, args))
      {
         // argument parsing error
         args.clear();
         return DCE_NOT_SUPPORTED;
      }
   }

   UINT32 rc = DCE_NOT_SUPPORTED;
   NXSL_VM *vm = g_pScriptLibrary->createVM(name, new NXSL_ServerEnv);
   if (vm != NULL)
   {
      vm->setGlobalVariable(_T("$object"), new NXSL_Value(new NXSL_Object(&g_nxslNetObjClass, this)));
      if (getObjectClass() == OBJECT_NODE)
      {
         vm->setGlobalVariable(_T("$node"), new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, this)));
      }
      vm->setGlobalVariable(_T("$isCluster"), new NXSL_Value((getObjectClass() == OBJECT_CLUSTER) ? 1 : 0));
      if (vm->run(&args))
      {
         NXSL_Value *value = vm->getResult();
         const TCHAR *dciValue = value->getValueAsCString();
         nx_strncpy(buffer, CHECK_NULL_EX(dciValue), bufSize);
         rc = DCE_SUCCESS;
      }
      else
      {
			DbgPrintf(4, _T("DataCollectionTarget(%s)->getScriptItem(%s): Script execution error: %s"), m_name, param, vm->getErrorText());
			PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", name, vm->getErrorText(), m_id);
         rc = DCE_COMM_ERROR;
      }
      delete vm;
   }
   else
   {
      args.setOwner(true);
   }
   DbgPrintf(7, _T("DataCollectionTarget(%s)->getScriptItem(%s): rc=%d"), m_name, param, rc);
   return rc;
}

/**
 * Get last (current) DCI values for summary table.
 */
void DataCollectionTarget::getDciValuesSummary(SummaryTable *tableDefinition, Table *tableData)
{
   int offset = tableDefinition->isMultiInstance() ? 2 : 1;
   int baseRow = tableData->getNumRows();
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
            int row;
            if (tableDefinition->isMultiInstance())
            {
               // Find instance
               const TCHAR *instance = ((DCItem *)object)->getInstance();
               for(row = baseRow; row < tableData->getNumRows(); row++)
               {
                  const TCHAR *v = tableData->getAsString(row, 1);
                  if (!_tcscmp(CHECK_NULL_EX(v), instance))
                     break;
               }
               if (row == tableData->getNumRows())
               {
                  tableData->addRow();
                  tableData->set(0, m_name);
                  tableData->set(1, instance);
                  tableData->setObjectId(tableData->getNumRows() - 1, m_id);
               }
            }
            else
            {
               if (!rowAdded)
               {
                  tableData->addRow();
                  tableData->set(0, m_name);
                  tableData->setObjectId(tableData->getNumRows() - 1, m_id);
                  rowAdded = true;
               }
               row = tableData->getNumRows() - 1;
            }
            tableData->setStatusAt(row, i + offset, ((DCItem *)object)->getThresholdSeverity());
            tableData->getColumnDefinitions()->get(i + offset)->setDataType(((DCItem *)object)->getDataType());
            if (tableDefinition->getAggregationFunction() == F_LAST)
            {
               tableData->setAt(row, i + offset, ((DCItem *)object)->getLastValue());
            }
            else
            {
               tableData->setAt(row, i + offset,
                  ((DCItem *)object)->getAggregateValue(
                     tableDefinition->getAggregationFunction(),
                     tableDefinition->getPeriodStart(),
                     tableDefinition->getPeriodEnd()));
            }

            if (!tableDefinition->isMultiInstance())
               break;
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

/**
 * Returns most critical status of DCI used for
 * status calculation
 */
int DataCollectionTarget::getMostCriticalDCIStatus()
{
   int status = -1;
   lockDciAccess(false);
   for(int i = 0; i < m_dcObjects->size(); i++)
	{
		DCObject *curr = m_dcObjects->get(i);
      if (curr->isStatusDCO() && (curr->getType() == DCO_TYPE_ITEM) &&
          curr->hasValue() && (curr->getStatus() == ITEM_STATUS_ACTIVE))
      {
         if (getObjectClass() == OBJECT_CLUSTER && !curr->isAggregateOnCluster())
            continue; // Calculated only on those that are agregated on cluster

         ItemValue *value = ((DCItem *)curr)->getInternalLastValue();
         if (value != NULL && (INT32)*value >= STATUS_NORMAL && (INT32)*value <= STATUS_CRITICAL)
            status = max(status, (INT32)*value);
         delete value;
      }
	}
   unlockDciAccess();
   return (status == -1) ? STATUS_UNKNOWN : status;
}

/**
 * Calculate compound status
 */
void DataCollectionTarget::calculateCompoundStatus(BOOL bForcedRecalc)
{
   NetObj::calculateCompoundStatus(bForcedRecalc);
}

/**
 * Returns last ping time
 */
UINT32 DataCollectionTarget::getPingTime()
{
   if ((time(NULL) - m_pingLastTimeStamp) > g_dwStatusPollingInterval)
   {
      updatePingData();
      DbgPrintf(7, _T("DataCollectionTarget::getPingTime: update ping time is required! Last ping time %d."), m_pingLastTimeStamp);
   }
   return m_pingTime;
}

/**
 * Update ping data
 */
void DataCollectionTarget::updatePingData()
{
   m_pingLastTimeStamp = 0;
   m_pingTime = PING_TIME_TIMEOUT;
}
