/*
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
** File: condition.cpp
**
**/

#include "nxcore.h"

#define DEBUG_TAG _T("obj.condition")

/**
 * Default constructor
 */
ConditionObject::ConditionObject() : super(), Pollable(this, Pollable::STATUS), m_dciList(0, 8)
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
ConditionObject::ConditionObject(bool hidden) : super(), Pollable(this, Pollable::STATUS), m_dciList(0, 8)
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
bool ConditionObject::loadFromDatabase(DB_HANDLE hdb, uint32_t id, DB_STATEMENT *preparedStatements)
{
   TCHAR szQuery[512];
   DB_RESULT hResult;

   m_id = id;

   if (!loadCommonProperties(hdb, preparedStatements))
      return false;

   // Load properties
   _sntprintf(szQuery, 512, _T("SELECT activation_event,deactivation_event,source_object,active_status,inactive_status,script FROM conditions WHERE id=%d"), id);
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
   m_script = CompileServerScript(m_scriptSource, SCRIPT_CONTEXT_OBJECT, this, 0, _T("Condition::%s"), m_name);

   // Load DCI map
   _sntprintf(szQuery, 512, _T("SELECT dci_id,node_id,dci_func,num_polls FROM cond_dci_map WHERE condition_id=%u ORDER BY sequence_number"), id);
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

   return loadACLFromDB(hdb, preparedStatements);
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
void ConditionObject::fillMessageLocked(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageLocked(msg, userId);

   msg->setField(VID_SCRIPT, CHECK_NULL_EX(m_scriptSource));
   msg->setField(VID_ACTIVATION_EVENT, m_activationEventCode);
   msg->setField(VID_DEACTIVATION_EVENT, m_deactivationEventCode);
   msg->setField(VID_SOURCE_OBJECT, m_sourceObject);
   msg->setField(VID_ACTIVE_STATUS, (WORD)m_activeStatus);
   msg->setField(VID_INACTIVE_STATUS, (WORD)m_inactiveStatus);
   msg->setField(VID_NUM_ITEMS, m_dciList.size());
}

/**
 * Fill NXCP message with object's data - stage 2
 */
void ConditionObject::fillMessageUnlocked(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageUnlocked(msg, userId);

   // Create a copy of input DCI list and fill message with condition object properties unlocked
   // to avoid possible deadlock inside GetDCObjectType (which will lock DCI list on node while
   // other thread my attempt to lock condition object properties within ConditionObject::getCacheSizeForDCI
   // called from DataCollectionTarget::updateDCItemCacheSize)
   lockProperties();
   StructArray<INPUT_DCI> dciList(m_dciList);
   unlockProperties();
   uint32_t fieldId = VID_DCI_LIST_BASE;
   for(int i = 0; (i < dciList.size()) && (fieldId < (VID_DCI_LIST_LAST + 1)); i++)
   {
      INPUT_DCI *dci = dciList.get(i);
      msg->setField(fieldId++, dci->id);
      msg->setField(fieldId++, dci->nodeId);
      msg->setField(fieldId++, static_cast<uint16_t>(dci->function));
      msg->setField(fieldId++, static_cast<uint16_t>(dci->polls));
      msg->setField(fieldId++, static_cast<uint16_t>(GetDCObjectType(dci->nodeId, dci->id)));
      fieldId += 5;
   }
}

/**
 * Modify object from NXCP message
 */
uint32_t ConditionObject::modifyFromMessageInternal(const NXCPMessage& msg, ClientSession *session)
{
   // Change script
   if (msg.isFieldExist(VID_SCRIPT))
   {
      MemFree(m_scriptSource);
      delete m_script;
      m_scriptSource = msg.getFieldAsString(VID_SCRIPT);
      m_script = CompileServerScript(m_scriptSource, SCRIPT_CONTEXT_OBJECT, this, 0, _T("Condition::%s"), m_name);
   }

   // Change activation event
   if (msg.isFieldExist(VID_ACTIVATION_EVENT))
      m_activationEventCode = msg.getFieldAsUInt32(VID_ACTIVATION_EVENT);

   // Change deactivation event
   if (msg.isFieldExist(VID_DEACTIVATION_EVENT))
      m_deactivationEventCode = msg.getFieldAsUInt32(VID_DEACTIVATION_EVENT);

   // Change source object
   if (msg.isFieldExist(VID_SOURCE_OBJECT))
      m_sourceObject = msg.getFieldAsUInt32(VID_SOURCE_OBJECT);

   // Change active status
   if (msg.isFieldExist(VID_ACTIVE_STATUS))
      m_activeStatus = msg.getFieldAsUInt16(VID_ACTIVE_STATUS);

   // Change inactive status
   if (msg.isFieldExist(VID_INACTIVE_STATUS))
      m_inactiveStatus = msg.getFieldAsUInt16(VID_INACTIVE_STATUS);

   // Change DCI list
   if (msg.isFieldExist(VID_NUM_ITEMS))
   {
      m_dciList.clear();
      int dciCount = msg.getFieldAsInt32(VID_NUM_ITEMS);
      uint32_t fieldId = VID_DCI_LIST_BASE;
      for(int i = 0; (i < dciCount) && (fieldId < (VID_DCI_LIST_LAST + 1)); i++)
      {
         INPUT_DCI dci;
         dci.id = msg.getFieldAsUInt32(fieldId++);
         dci.nodeId = msg.getFieldAsUInt32(fieldId++);
         dci.function = msg.getFieldAsUInt16(fieldId++);
         dci.polls = msg.getFieldAsUInt16(fieldId++);
         m_dciList.add(dci);
         fieldId += 6;
      }

      // Update cache size of DCIs
      for(int i = 0; i < m_dciList.size(); i++)
      {
         shared_ptr<NetObj> object = FindObjectById(m_dciList.get(i)->nodeId);
         if ((object != nullptr) && object->isDataCollectionTarget())
         {
            ThreadPoolExecute(g_mainThreadPool, static_pointer_cast<DataCollectionTarget>(object), &DataCollectionTarget::updateDCItemCacheSize, m_dciList.get(i)->id);
         }
      }
   }

   return super::modifyFromMessageInternal(msg, session);
}

/**
 * Lock for polling
 */
bool ConditionObject::lockForStatusPoll()
{
   if ((m_status != STATUS_UNMANAGED) && (!m_queuedForPolling) && (!m_isDeleted) && (static_cast<uint32_t>(time(nullptr) - m_lastPoll) > g_conditionPollingInterval))
   {
      m_queuedForPolling = true;
      return true;
   }

   return false;
}

/**
 * Poller entry point
 */
void ConditionObject::statusPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId)
{
   check();
   lockProperties();
   m_queuedForPolling = false;
   m_lastPoll = time(nullptr);
   unlockProperties();
}

/**
 * Check condition
 */
void ConditionObject::check()
{
   if ((m_script == nullptr) || (m_status == STATUS_UNMANAGED) || IsShutdownInProgress())
      return;

   // Make copy of DCI list to avoid deadlock accessing nodes and DCIs on nodes
   // while holding lock on condition object properties
   lockProperties();
   int iOldStatus = m_status;
   StructArray<INPUT_DCI> dciList(m_dciList);
   ScriptVMHandle vm = CreateServerScriptVM(m_script, self());
   unlockProperties();

   if (!vm.isValid())
   {
      if (vm.failureReason() != ScriptVMFailureReason::SCRIPT_IS_EMPTY)
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("Cannot create VM for evaluation script for condition %s [%u] (%s)"), m_name, m_id, vm.failureReasonText());
         ReportScriptError(SCRIPT_CONTEXT_OBJECT, this, 0, vm.failureReasonText(), _T("Condition::%s"), m_name);
      }
      return;
   }

   ObjectRefArray<NXSL_Value> valueList(dciList.size(), 16);
   for(int i = 0; i < dciList.size(); i++)
   {
      INPUT_DCI *dci = dciList.get(i);
      NXSL_Value *value = nullptr;
      shared_ptr<NetObj> object = FindObjectById(dci->nodeId);
      if ((object != nullptr) && object->isDataCollectionTarget())
      {
         shared_ptr<DCObject> pItem = static_cast<DataCollectionTarget&>(*object).getDCObjectById(dci->id, 0);
         if (pItem != nullptr)
         {
            if (pItem->getType() == DCO_TYPE_ITEM)
            {
               value = static_cast<DCItem&>(*pItem).getValueForNXSL(vm, dci->function, dci->polls);
            }
            else if (pItem->getType() == DCO_TYPE_TABLE)
            {
               shared_ptr<Table> t = static_cast<DCTable&>(*pItem).getLastValue();
               if (t != nullptr)
               {
                  value = vm->createValue(vm->createObject(&g_nxslTableClass, new shared_ptr<Table>(t)));
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

   nxlog_debug_tag(DEBUG_TAG, 6, _T("Running evaluation script for condition %s [%u]"), m_name, m_id);
   if (vm->run(valueList))
   {
      NXSL_Value *pValue = vm->getResult();
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

            EventBuilder(m_deactivationEventCode, (m_sourceObject == 0) ? g_dwMgmtNode : m_sourceObject)
               .param(_T("conditionObjectId"), m_id)
               .param(_T("conditionObjectName"), m_name)
               .param(_T("previousConditionStatus"), iOldStatus)
               .param(_T("currentConditionStatus"), m_status)
               .post();

            nxlog_debug_tag(DEBUG_TAG, 6, _T("Condition \"%s\" [%u] deactivated"), m_name, m_id);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 6, _T("Condition \"%s\" [%u] still inactive"), m_name, m_id);
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

            EventBuilder(m_activationEventCode, (m_sourceObject == 0) ? g_dwMgmtNode : m_sourceObject)
               .param(_T("conditionObjectId"), m_id)
               .param(_T("conditionObjectName"), m_name)
               .param(_T("previousConditionStatus"), iOldStatus)
               .param(_T("currentConditionStatus"), m_status)
               .post();

            nxlog_debug_tag(DEBUG_TAG, 6, _T("Condition \"%s\" [%u] activated"), m_name, m_id);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 6, _T("Condition \"%s\" [%u] still active"), m_name, m_id);
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
      nxlog_debug_tag(DEBUG_TAG, 6, _T("Cannot execute evaluation script for condition %s [%u] (%s)"), m_name, m_id, vm->getErrorText());
      ReportScriptError(SCRIPT_CONTEXT_OBJECT, this, 0, vm->getErrorText(), _T("Condition::%s"), m_name);

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
 * Calculate object status based on children status
 */
void ConditionObject::calculateCompoundStatus(bool forcedRecalc)
{
   // Status of the condition object depends solely on check result, so recalculation should not do anything
}

/**
 * Determine DCI cache size required by condition object
 */
int ConditionObject::getCacheSizeForDCI(uint32_t itemId)
{
   int nSize = 0;

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
