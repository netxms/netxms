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
** File: container.cpp
**
**/

#include "nxcore.h"

/**
 * Default constructor
 */
ConditionObject::ConditionObject() : super()
{
   m_scriptSource = NULL;
   m_script = NULL;
   m_dciList = NULL;
   m_dciCount = 0;
   m_sourceObject = 0;
   m_activeStatus = STATUS_MAJOR;
   m_inactiveStatus = STATUS_NORMAL;
   m_isActive = false;
   m_lastPoll = 0;
   m_queuedForPolling = false;
   m_activationEventCode = EVENT_CONDITION_ACTIVATED;
   m_deactivationEventCode = EVENT_CONDITION_DEACTIVATED;
}

/**
 * Constructor for new objects
 */
ConditionObject::ConditionObject(bool hidden) : super()
{
   m_scriptSource = NULL;
   m_script = NULL;
   m_dciList = NULL;
   m_dciCount = 0;
   m_sourceObject = 0;
   m_activeStatus = STATUS_MAJOR;
   m_inactiveStatus = STATUS_NORMAL;
   m_isHidden = hidden;
   m_isActive = false;
   m_lastPoll = 0;
   m_queuedForPolling = false;
   m_activationEventCode = EVENT_CONDITION_ACTIVATED;
   m_deactivationEventCode = EVENT_CONDITION_DEACTIVATED;
}

/**
 * Destructor
 */
ConditionObject::~ConditionObject()
{
   free(m_dciList);
   free(m_scriptSource);
   delete m_script;
}

/**
 * Load object from database
 */
bool ConditionObject::loadFromDatabase(DB_HANDLE hdb, UINT32 dwId)
{
   TCHAR szQuery[512];
   DB_RESULT hResult;

   m_id = dwId;

   if (!loadCommonProperties(hdb))
      return false;

   // Load properties
   _sntprintf(szQuery, 512, _T("SELECT activation_event,deactivation_event,")
                            _T("source_object,active_status,inactive_status,")
                            _T("script FROM conditions WHERE id=%d"), dwId);
   hResult = DBSelect(hdb, szQuery);
   if (hResult == NULL)
      return false;     // Query failed

   if (DBGetNumRows(hResult) == 0)
   {
      // No object with given ID in database
      DBFreeResult(hResult);
      return false;
   }

   m_activationEventCode = DBGetFieldULong(hResult, 0, 0);
   m_deactivationEventCode = DBGetFieldULong(hResult, 0, 1);
   m_sourceObject = DBGetFieldULong(hResult, 0, 2);
   m_activeStatus = DBGetFieldLong(hResult, 0, 3);
   m_inactiveStatus = DBGetFieldLong(hResult, 0, 4);
   m_scriptSource = DBGetField(hResult, 0, 5, NULL, 0);
   DecodeSQLString(m_scriptSource);

   DBFreeResult(hResult);

   // Compile script
   NXSL_Program *p = (NXSL_Program *)NXSLCompile(m_scriptSource, szQuery, 512, NULL);
   if (p != NULL)
   {
      m_script = new NXSL_VM(new NXSL_ServerEnv);
      if (!m_script->load(p))
      {
         nxlog_write(MSG_COND_SCRIPT_COMPILATION_ERROR, EVENTLOG_ERROR_TYPE, "dss", m_id, m_name, m_script->getErrorText());
         delete_and_null(m_script);
      }
      delete p;
   }
   else
   {
      m_script = NULL;
      nxlog_write(MSG_COND_SCRIPT_COMPILATION_ERROR, EVENTLOG_ERROR_TYPE, "dss", m_id, m_name, szQuery);
   }

   // Load DCI map
   _sntprintf(szQuery, 512, _T("SELECT dci_id,node_id,dci_func,num_polls ")
                            _T("FROM cond_dci_map WHERE condition_id=%d ORDER BY sequence_number"), dwId);
   hResult = DBSelect(hdb, szQuery);
   if (hResult == NULL)
      return false;     // Query failed

   m_dciCount = DBGetNumRows(hResult);
   if (m_dciCount > 0)
   {
      m_dciList = (INPUT_DCI *)malloc(sizeof(INPUT_DCI) * m_dciCount);
      for(int i = 0; i < m_dciCount; i++)
      {
         m_dciList[i].id = DBGetFieldULong(hResult, i, 0);
         m_dciList[i].nodeId = DBGetFieldULong(hResult, i, 1);
         m_dciList[i].function = DBGetFieldLong(hResult, i, 2);
         m_dciList[i].polls = DBGetFieldLong(hResult, i, 3);
      }
   }
   DBFreeResult(hResult);

   return loadACLFromDB(hdb);
}

/**
 * Save object to database
 */
bool ConditionObject::saveToDatabase(DB_HANDLE hdb)
{
   TCHAR *pszEscScript, *pszQuery;
   DB_RESULT hResult;
   BOOL bNewObject = TRUE;

   lockProperties();

   saveCommonProperties(hdb);

   if (m_modified & MODIFY_OTHER)
   {
      pszEscScript = EncodeSQLString(CHECK_NULL_EX(m_scriptSource));
      size_t qlen = _tcslen(pszEscScript) + 1024;
      pszQuery = (TCHAR *)malloc(sizeof(TCHAR) * qlen);

      // Check for object's existence in database
      _sntprintf(pszQuery, qlen, _T("SELECT id FROM conditions WHERE id=%d"), m_id);
      hResult = DBSelect(hdb, pszQuery);
      if (hResult != NULL)
      {
         if (DBGetNumRows(hResult) > 0)
            bNewObject = FALSE;
         DBFreeResult(hResult);
      }

      // Form and execute INSERT or UPDATE query
      if (bNewObject)
      {
         _sntprintf(pszQuery, qlen,
                             _T("INSERT INTO conditions (id,activation_event,")
                             _T("deactivation_event,source_object,active_status,")
                             _T("inactive_status,script) VALUES (%d,%d,%d,%d,%d,%d,'%s')"),
                   m_id, m_activationEventCode, m_deactivationEventCode,
                   m_sourceObject, m_activeStatus, m_inactiveStatus, pszEscScript);
      }
      else
      {
         _sntprintf(pszQuery, qlen,
                             _T("UPDATE conditions SET activation_event=%d,")
                             _T("deactivation_event=%d,source_object=%d,active_status=%d,")
                             _T("inactive_status=%d,script='%s' WHERE id=%d"),
                   m_activationEventCode, m_deactivationEventCode, m_sourceObject,
                   m_activeStatus, m_inactiveStatus, pszEscScript, m_id);
      }
      free(pszEscScript);
      DBQuery(hdb, pszQuery);

      // Save DCI mapping
      _sntprintf(pszQuery, qlen, _T("DELETE FROM cond_dci_map WHERE condition_id=%d"), m_id);
      DBQuery(hdb, pszQuery);
      for(int i = 0; i < m_dciCount; i++)
      {
         _sntprintf(pszQuery, qlen, _T("INSERT INTO cond_dci_map (condition_id,sequence_number,dci_id,node_id,")
                                    _T("dci_func,num_polls) VALUES (%d,%d,%d,%d,%d,%d)"),
                   m_id, i, m_dciList[i].id, m_dciList[i].nodeId,
                   m_dciList[i].function, m_dciList[i].polls);
         DBQuery(hdb, pszQuery);
      }
      free(pszQuery);
   }

   // Save access list
   saveACLToDB(hdb);

   // Unlock object and clear modification flag
   m_modified = 0;
   unlockProperties();
   return true;
}

/**
 * Delete object from database
 */
bool ConditionObject::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM conditions WHERE id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM cond_dci_map WHERE condition_id=?"));
   return success;
}

/**
 * Create NXCP message from object
 */
void ConditionObject::fillMessageInternal(NXCPMessage *pMsg, UINT32 userId)
{
   super::fillMessageInternal(pMsg, userId);

   pMsg->setField(VID_SCRIPT, CHECK_NULL_EX(m_scriptSource));
   pMsg->setField(VID_ACTIVATION_EVENT, m_activationEventCode);
   pMsg->setField(VID_DEACTIVATION_EVENT, m_deactivationEventCode);
   pMsg->setField(VID_SOURCE_OBJECT, m_sourceObject);
   pMsg->setField(VID_ACTIVE_STATUS, (WORD)m_activeStatus);
   pMsg->setField(VID_INACTIVE_STATUS, (WORD)m_inactiveStatus);
   pMsg->setField(VID_NUM_ITEMS, m_dciCount);
   UINT32 dwId = VID_DCI_LIST_BASE;
   for(int i = 0; (i < m_dciCount) && (dwId < (VID_DCI_LIST_LAST + 1)); i++)
   {
      pMsg->setField(dwId++, m_dciList[i].id);
      pMsg->setField(dwId++, m_dciList[i].nodeId);
      pMsg->setField(dwId++, (WORD)m_dciList[i].function);
      pMsg->setField(dwId++, (WORD)m_dciList[i].polls);
      pMsg->setField(dwId++, (WORD)GetDCObjectType(m_dciList[i].nodeId, m_dciList[i].id));
      dwId += 5;
   }
}

/**
 * Modify object from NXCP message
 */
UINT32 ConditionObject::modifyFromMessageInternal(NXCPMessage *pRequest)
{
   NetObj *pObject;

   // Change script
   if (pRequest->isFieldExist(VID_SCRIPT))
   {
      TCHAR szError[1024];

      free(m_scriptSource);
      delete m_script;
      m_scriptSource = pRequest->getFieldAsString(VID_SCRIPT);
      NXSL_Program *p = (NXSL_Program *)NXSLCompile(m_scriptSource, szError, 1024, NULL);
      if (p != NULL)
      {
         m_script = new NXSL_VM(new NXSL_ServerEnv);
         if (!m_script->load(p))
         {
            nxlog_write(MSG_COND_SCRIPT_COMPILATION_ERROR, EVENTLOG_ERROR_TYPE, "dss", m_id, m_name, m_script->getErrorText());
            delete_and_null(m_script);
         }
         delete p;
      }
      else
      {
         m_script = NULL;
         nxlog_write(MSG_COND_SCRIPT_COMPILATION_ERROR, EVENTLOG_ERROR_TYPE, "dss", m_id, m_name, szError);
      }
   }

   // Change activation event
   if (pRequest->isFieldExist(VID_ACTIVATION_EVENT))
      m_activationEventCode = pRequest->getFieldAsUInt32(VID_ACTIVATION_EVENT);

   // Change deactivation event
   if (pRequest->isFieldExist(VID_DEACTIVATION_EVENT))
      m_deactivationEventCode = pRequest->getFieldAsUInt32(VID_DEACTIVATION_EVENT);

   // Change source object
   if (pRequest->isFieldExist(VID_SOURCE_OBJECT))
      m_sourceObject = pRequest->getFieldAsUInt32(VID_SOURCE_OBJECT);

   // Change active status
   if (pRequest->isFieldExist(VID_ACTIVE_STATUS))
      m_activeStatus = pRequest->getFieldAsUInt16(VID_ACTIVE_STATUS);

   // Change inactive status
   if (pRequest->isFieldExist(VID_INACTIVE_STATUS))
      m_inactiveStatus = pRequest->getFieldAsUInt16(VID_INACTIVE_STATUS);

   // Change DCI list
   if (pRequest->isFieldExist(VID_NUM_ITEMS))
   {
      free(m_dciList);
      m_dciCount = pRequest->getFieldAsUInt32(VID_NUM_ITEMS);
      if (m_dciCount > 0)
      {
         m_dciList = (INPUT_DCI *)malloc(sizeof(INPUT_DCI) * m_dciCount);
         UINT32 dwId = VID_DCI_LIST_BASE;
         for(int i = 0; (i < m_dciCount) && (dwId < (VID_DCI_LIST_LAST + 1)); i++)
         {
            m_dciList[i].id = pRequest->getFieldAsUInt32(dwId++);
            m_dciList[i].nodeId = pRequest->getFieldAsUInt32(dwId++);
            m_dciList[i].function = pRequest->getFieldAsUInt16(dwId++);
            m_dciList[i].polls = pRequest->getFieldAsUInt16(dwId++);
            dwId += 6;
         }

         // Update cache size of DCIs
         for(int i = 0; i < m_dciCount; i++)
         {
            pObject = FindObjectById(m_dciList[i].nodeId);
            if (pObject != NULL)
            {
               if ((pObject->getObjectClass() == OBJECT_NODE) ||
                   (pObject->getObjectClass() == OBJECT_CLUSTER) ||
                   (pObject->getObjectClass() == OBJECT_MOBILEDEVICE) ||
                   (pObject->getObjectClass() == OBJECT_SENSOR))
               {
                  ((DataCollectionTarget *)pObject)->updateDCItemCacheSize(m_dciList[i].id, m_id);
               }
            }
         }
      }
      else
      {
         m_dciList = NULL;
      }
   }

   return super::modifyFromMessageInternal(pRequest);
}

/**
 * Lock for polling
 */
void ConditionObject::lockForPoll()
{
   m_queuedForPolling = TRUE;
}

/**
 * Poller entry point
 */
void ConditionObject::doPoll(PollerInfo *poller)
{
   poller->startExecution();
   check();
   lockProperties();
   m_queuedForPolling = FALSE;
   m_lastPoll = time(NULL);
   unlockProperties();
   delete poller;
}

/**
 * Check condition
 */
void ConditionObject::check()
{
   NXSL_Value **ppValueList, *pValue;
   NetObj *pObject;
   int iOldStatus = m_status;

   if ((m_script == NULL) || (m_status == STATUS_UNMANAGED) || IsShutdownInProgress())
      return;

   lockProperties();
   ppValueList = (NXSL_Value **)malloc(sizeof(NXSL_Value *) * m_dciCount);
   memset(ppValueList, 0, sizeof(NXSL_Value *) * m_dciCount);
   for(int i = 0; i < m_dciCount; i++)
   {
      pObject = FindObjectById(m_dciList[i].nodeId);
      if (pObject != NULL)
      {
         if (pObject->getObjectClass() == OBJECT_NODE)
         {
            DCObject *pItem = ((Node *)pObject)->getDCObjectById(m_dciList[i].id, 0);
            if (pItem != NULL)
            {
               if (pItem->getType() == DCO_TYPE_ITEM)
               {
                  ppValueList[i] = ((DCItem *)pItem)->getValueForNXSL(m_script, m_dciList[i].function, m_dciList[i].polls);
               }
               else if (pItem->getType() == DCO_TYPE_TABLE)
               {
                  Table *t = ((DCTable *)pItem)->getLastValue();
                  if (t != NULL)
                  {
                     ppValueList[i] = m_script->createValue(new NXSL_Object(m_script, &g_nxslTableClass, t));
                  }
               }
            }
         }
      }
      if (ppValueList[i] == NULL)
         ppValueList[i] = m_script->createValue();
   }
   int numValues = m_dciCount;
   unlockProperties();

	// Create array from values
	NXSL_Array *array = new NXSL_Array(m_script);
	for(int i = 0; i < numValues; i++)
	{
		array->set(i + 1, m_script->createValue(ppValueList[i]));
	}
   m_script->setGlobalVariable("$values", m_script->createValue(array));

   DbgPrintf(6, _T("Running evaluation script for condition %d \"%s\""), m_id, m_name);
   if (m_script->run(numValues, ppValueList))
   {
      pValue = m_script->getResult();
      if (pValue->getValueAsInt32() == 0)
      {
         if (m_isActive)
         {
            // Deactivate condition
            lockProperties();
            m_status = m_inactiveStatus;
            m_isActive = FALSE;
            setModified(MODIFY_RUNTIME);
            unlockProperties();

            PostEvent(m_deactivationEventCode,
                      (m_sourceObject == 0) ? g_dwMgmtNode : m_sourceObject,
                      "dsdd", m_id, m_name, iOldStatus, m_status);

            DbgPrintf(6, _T("Condition %d \"%s\" deactivated"),
                      m_id, m_name);
         }
         else
         {
            DbgPrintf(6, _T("Condition %d \"%s\" still inactive"),
                      m_id, m_name);
            lockProperties();
            if (m_status != m_inactiveStatus)
            {
               m_status = m_inactiveStatus;
               setModified(MODIFY_RUNTIME);
            }
            unlockProperties();
         }
      }
      else
      {
         if (!m_isActive)
         {
            // Activate condition
            lockProperties();
            m_status = m_activeStatus;
            m_isActive = TRUE;
            setModified(MODIFY_RUNTIME);
            unlockProperties();

            PostEvent(m_activationEventCode,
                      (m_sourceObject == 0) ? g_dwMgmtNode : m_sourceObject,
                      "dsdd", m_id, m_name, iOldStatus, m_status);

            DbgPrintf(6, _T("Condition %d \"%s\" activated"),
                      m_id, m_name);
         }
         else
         {
            DbgPrintf(6, _T("Condition %d \"%s\" still active"),
                      m_id, m_name);
            lockProperties();
            if (m_status != m_activeStatus)
            {
               m_status = m_activeStatus;
               setModified(MODIFY_RUNTIME);
            }
            unlockProperties();
         }
      }
   }
   else
   {
      nxlog_write(MSG_COND_SCRIPT_EXECUTION_ERROR, EVENTLOG_ERROR_TYPE,
               "dss", m_id, m_name, m_script->getErrorText());

      lockProperties();
      if (m_status != STATUS_UNKNOWN)
      {
         m_status = STATUS_UNKNOWN;
         setModified(MODIFY_RUNTIME);
      }
      unlockProperties();
   }
   free(ppValueList);

   // Cause parent object(s) to recalculate it's status
   if (iOldStatus != m_status)
   {
      lockParentList(false);
      for(int i = 0; i < m_parentList->size(); i++)
         m_parentList->get(i)->calculateCompoundStatus();
      unlockParentList();
   }
}

/**
 * Determine DCI cache size required by condition object
 */
int ConditionObject::getCacheSizeForDCI(UINT32 itemId, bool noLock)
{
   int nSize = 0;

   if (!noLock)
      lockProperties();
   for(int i = 0; i < m_dciCount; i++)
   {
      if (m_dciList[i].id == itemId)
      {
         switch(m_dciList[i].function)
         {
            case F_LAST:
               nSize = 1;
               break;
            case F_DIFF:
               nSize = 2;
               break;
            default:
               nSize = m_dciList[i].polls;
               break;
         }
         break;
      }
   }
   if (!noLock)
      unlockProperties();
   return nSize;
}

/**
 * Serialize object to JSON
 */
json_t *ConditionObject::toJson()
{
   json_t *root = super::toJson();
   json_t *inputs = json_array();
   for(int i = 0; i < m_dciCount; i++)
   {
      json_t *dci = json_object();
      json_object_set_new(dci, "id", json_integer(m_dciList[i].id));
      json_object_set_new(dci, "nodeId", json_integer(m_dciList[i].nodeId));
      json_object_set_new(dci, "function", json_integer(m_dciList[i].function));
      json_object_set_new(dci, "polls", json_integer(m_dciList[i].polls));
      json_array_append_new(inputs, dci);
   }
   json_object_set_new(root, "inputs", inputs);
   json_object_set_new(root, "script", json_string_t(m_scriptSource));
   json_object_set_new(root, "activationEventCode", json_integer(m_activationEventCode));
   json_object_set_new(root, "deactivationEventCode", json_integer(m_deactivationEventCode));
   json_object_set_new(root, "sourceObject", json_integer(m_sourceObject));
   json_object_set_new(root, "activeStatus", json_integer(m_activeStatus));
   json_object_set_new(root, "inactiveStatus", json_integer(m_inactiveStatus));
   json_object_set_new(root, "isActive", json_boolean(m_isActive));
   json_object_set_new(root, "lastPoll", json_integer(m_lastPoll));
   return root;
}
