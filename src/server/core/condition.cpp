/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
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
ConditionObject::ConditionObject() : super(), m_dciList(0, 8)
{
   m_scriptSource = nullptr;
   m_script = nullptr;
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
ConditionObject::ConditionObject(bool hidden) : super(), m_dciList(0, 8)
{
   m_scriptSource = nullptr;
   m_script = nullptr;
   m_sourceObject = 0;
   m_activeStatus = STATUS_MAJOR;
   m_inactiveStatus = STATUS_NORMAL;
   m_isHidden = hidden;
   m_isActive = false;
   m_lastPoll = 0;
   m_queuedForPolling = false;
   m_activationEventCode = EVENT_CONDITION_ACTIVATED;
   m_deactivationEventCode = EVENT_CONDITION_DEACTIVATED;
   setCreationTime();
}

/**
 * Destructor
 */
ConditionObject::~ConditionObject()
{
   MemFree(m_scriptSource);
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
   if (hResult == nullptr)
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
   m_scriptSource = DBGetField(hResult, 0, 5, nullptr, 0);

   DBFreeResult(hResult);

   // Compile script
   m_script = NXSLCompile(m_scriptSource, szQuery, 512, nullptr);
   if (m_script == nullptr)
      nxlog_write(NXLOG_ERROR, _T("Failed to compile evaluation script for condition object %s [%u] (%s)"), m_name, m_id, szQuery);

   // Load DCI map
   _sntprintf(szQuery, 512, _T("SELECT dci_id,node_id,dci_func,num_polls FROM cond_dci_map WHERE condition_id=%u ORDER BY sequence_number"), dwId);
   hResult = DBSelect(hdb, szQuery);
   if (hResult == nullptr)
      return false;     // Query failed

   int dciCount = DBGetNumRows(hResult);
   for(int i = 0; i < dciCount; i++)
   {
      INPUT_DCI dci;
      dci.id = DBGetFieldULong(hResult, i, 0);
      dci.nodeId = DBGetFieldULong(hResult, i, 1);
      dci.function = DBGetFieldLong(hResult, i, 2);
      dci.polls = DBGetFieldLong(hResult, i, 3);
      m_dciList.add(dci);
   }
   DBFreeResult(hResult);

   return loadACLFromDB(hdb);
}

/**
 * Save object to database
 */
bool ConditionObject::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);
   if (success && (m_modified & MODIFY_OTHER))
   {
      static const TCHAR *columns[] = { _T("activation_event"), _T("deactivation_event"),
               _T("source_object"), _T("active_status"), _T("inactive_status"), _T("script"), nullptr };
      DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("conditions"), _T("id"), m_id, columns);
      if (hStmt != nullptr)
      {
         lockProperties();

         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_activationEventCode);
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_deactivationEventCode);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_sourceObject);
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_activeStatus);
         DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, m_inactiveStatus);
         DBBind(hStmt, 6, DB_SQLTYPE_TEXT, m_scriptSource, DB_BIND_STATIC);
         DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_id);
         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);

         unlockProperties();
      }
      else
      {
         success = false;
      }

      if (success)
         success = executeQueryOnObject(hdb, _T("DELETE FROM cond_dci_map WHERE condition_id=?"));

      lockProperties();
      if (success && !m_dciList.isEmpty())
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO cond_dci_map (condition_id,sequence_number,dci_id,node_id,dci_func,num_polls) VALUES (?,?,?,?,?,?)"), m_dciList.size() > 1);
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            for(int i = 0; (i < m_dciList.size()) && success; i++)
            {
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, i);
               INPUT_DCI *dci = m_dciList.get(i);
               DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, dci->id);
               DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, dci->nodeId);
               DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, dci->function);
               DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, dci->polls);
               success = DBExecute(hStmt);
            }
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
         }
      }
      unlockProperties();
   }
   return success;
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
   pMsg->setField(VID_NUM_ITEMS, m_dciList.size());
   uint32_t fieldId = VID_DCI_LIST_BASE;
   for(int i = 0; (i < m_dciList.size()) && (fieldId < (VID_DCI_LIST_LAST + 1)); i++)
   {
      INPUT_DCI *dci = m_dciList.get(i);
      pMsg->setField(fieldId++, dci->id);
      pMsg->setField(fieldId++, dci->nodeId);
      pMsg->setField(fieldId++, static_cast<uint16_t>(dci->function));
      pMsg->setField(fieldId++, static_cast<uint16_t>(dci->polls));
      pMsg->setField(fieldId++, static_cast<uint16_t>(GetDCObjectType(dci->nodeId, dci->id)));
      fieldId += 5;
   }
}

/**
 * Modify object from NXCP message
 */
UINT32 ConditionObject::modifyFromMessageInternal(NXCPMessage *pRequest)
{
   // Change script
   if (pRequest->isFieldExist(VID_SCRIPT))
   {
      TCHAR szError[1024];

      MemFree(m_scriptSource);
      delete m_script;
      m_scriptSource = pRequest->getFieldAsString(VID_SCRIPT);
      m_script = NXSLCompile(m_scriptSource, szError, 1024, nullptr);
      if (m_script == nullptr)
         nxlog_write(NXLOG_ERROR, _T("Failed to compile evaluation script for condition object %s [%u] (%s)"), m_name, m_id, szError);
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
      m_dciList.clear();
      int dciCount = pRequest->getFieldAsInt32(VID_NUM_ITEMS);
      uint32_t dwId = VID_DCI_LIST_BASE;
      for(int i = 0; (i < dciCount) && (dwId < (VID_DCI_LIST_LAST + 1)); i++)
      {
         INPUT_DCI dci;
         dci.id = pRequest->getFieldAsUInt32(dwId++);
         dci.nodeId = pRequest->getFieldAsUInt32(dwId++);
         dci.function = pRequest->getFieldAsUInt16(dwId++);
         dci.polls = pRequest->getFieldAsUInt16(dwId++);
         m_dciList.add(dci);
         dwId += 6;
      }

      // Update cache size of DCIs
      for(int i = 0; i < m_dciList.size(); i++)
      {
         shared_ptr<NetObj> object = FindObjectById(m_dciList.get(i)->nodeId);
         if ((object != nullptr) && object->isDataCollectionTarget())
         {
            ThreadPoolExecute(g_mainThreadPool, static_pointer_cast<DataCollectionTarget>(object), &DataCollectionTarget::updateDCItemCacheSize, m_dciList.get(i)->id, m_id);
         }
      }
   }

   return super::modifyFromMessageInternal(pRequest);
}

/**
 * Lock for polling
 */
void ConditionObject::lockForPoll()
{
   m_queuedForPolling = true;
}

/**
 * Poller entry point
 */
void ConditionObject::doPoll(PollerInfo *poller)
{
   poller->startExecution();
   check();
   lockProperties();
   m_queuedForPolling = false;
   m_lastPoll = time(nullptr);
   unlockProperties();
   delete poller;
}

/**
 * Check condition
 */
void ConditionObject::check()
{
   NXSL_Value **ppValueList, *pValue;
   int iOldStatus = m_status;

   if ((m_script == nullptr) || (m_status == STATUS_UNMANAGED) || IsShutdownInProgress())
      return;

   // Make copy of DCI list to avoid deadlock accessing nodes and DCIs on nodes
   // while holding lock on condition object properties
   lockProperties();
   StructArray<INPUT_DCI> dciList(&m_dciList);
   ScriptVMHandle vm = CreateServerScriptVM(m_script, self());
   unlockProperties();

   if (!vm.isValid())
   {
      if (vm.failureReason() != ScriptVMFailureReason::SCRIPT_IS_EMPTY)
         nxlog_debug(6, _T("Cannot create VM for evaluation script for condition %s [%u]"), m_name, m_id);
      return;
   }

   ObjectRefArray<NXSL_Value> valueList(dciList.size(), 16);
   for(int i = 0; i < dciList.size(); i++)
   {
      INPUT_DCI *dci = dciList.get(i);
      NXSL_Value *value = nullptr;
      shared_ptr<NetObj> pObject = FindObjectById(dci->nodeId);
      if ((pObject != nullptr) && pObject->isDataCollectionTarget())
      {
         shared_ptr<DCObject> pItem = static_cast<DataCollectionTarget&>(*pObject).getDCObjectById(dci->id, 0);
         if (pItem != nullptr)
         {
            if (pItem->getType() == DCO_TYPE_ITEM)
            {
               value = static_cast<DCItem&>(*pItem).getValueForNXSL(vm, dci->function, dci->polls);
            }
            else if (pItem->getType() == DCO_TYPE_TABLE)
            {
               Table *t = static_cast<DCTable&>(*pItem).getLastValue();
               if (t != nullptr)
               {
                  value = vm->createValue(new NXSL_Object(vm, &g_nxslTableClass, t));
               }
            }
         }
      }
      valueList.add((value != nullptr) ? value : vm->createValue());
   }

	// Create array from values
	NXSL_Array *array = new NXSL_Array(vm);
	for(int i = 0; i < valueList.size(); i++)
		array->set(i + 1, vm->createValue(valueList.get(i)));
   vm->setGlobalVariable("$values", vm->createValue(array));

   nxlog_debug(6, _T("Running evaluation script for condition %s [%u]"), m_name, m_id);
   if (vm->run(valueList))
   {
      pValue = vm->getResult();
      if (pValue->isFalse())
      {
         if (m_isActive)
         {
            // Deactivate condition
            lockProperties();
            m_status = m_inactiveStatus;
            m_isActive = false;
            setModified(MODIFY_RUNTIME);
            unlockProperties();

            PostSystemEvent(m_deactivationEventCode,
                      (m_sourceObject == 0) ? g_dwMgmtNode : m_sourceObject,
                      "dsdd", m_id, m_name, iOldStatus, m_status);

            DbgPrintf(6, _T("Condition %d \"%s\" deactivated"),
                      m_id, m_name);
         }
         else
         {
            DbgPrintf(6, _T("Condition %d \"%s\" still inactive"), m_id, m_name);
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
            m_isActive = true;
            setModified(MODIFY_RUNTIME);
            unlockProperties();

            PostSystemEvent(m_activationEventCode,
                      (m_sourceObject == 0) ? g_dwMgmtNode : m_sourceObject,
                      "dsdd", m_id, m_name, iOldStatus, m_status);

            DbgPrintf(6, _T("Condition %d \"%s\" activated"), m_id, m_name);
         }
         else
         {
            DbgPrintf(6, _T("Condition %d \"%s\" still active"), m_id, m_name);
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
      nxlog_write(NXLOG_ERROR, _T("Failed to execute evaluation script for condition object %s [%u] (%s)"), m_name, m_id, vm->getErrorText());

      lockProperties();
      if (m_status != STATUS_UNKNOWN)
      {
         m_status = STATUS_UNKNOWN;
         setModified(MODIFY_RUNTIME);
      }
      unlockProperties();
   }

   vm.destroy();

   // Cause parent object(s) to recalculate it's status
   if (iOldStatus != m_status)
   {
      readLockParentList();
      for(int i = 0; i < getParentList().size(); i++)
         getParentList().get(i)->calculateCompoundStatus();
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
   for(int i = 0; i < m_dciList.size(); i++)
   {
      INPUT_DCI *dci = m_dciList.get(i);
      if (dci->id == itemId)
      {
         switch(dci->function)
         {
            case F_LAST:
               nSize = 1;
               break;
            case F_DIFF:
               nSize = 2;
               break;
            default:
               nSize = dci->polls;
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

   lockProperties();

   json_t *inputs = json_array();
   for(int i = 0; i < m_dciList.size(); i++)
   {
      INPUT_DCI *dci = m_dciList.get(i);
      json_t *dciJson = json_object();
      json_object_set_new(dciJson, "id", json_integer(dci->id));
      json_object_set_new(dciJson, "nodeId", json_integer(dci->nodeId));
      json_object_set_new(dciJson, "function", json_integer(dci->function));
      json_object_set_new(dciJson, "polls", json_integer(dci->polls));
      json_array_append_new(inputs, dciJson);
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

   unlockProperties();
   return root;
}
