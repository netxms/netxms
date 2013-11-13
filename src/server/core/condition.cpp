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
** File: container.cpp
**
**/

#include "nxcore.h"

/**
 * Default constructor
 */
Condition::Condition() : NetObj()
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
Condition::Condition(bool hidden) : NetObj()
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
Condition::~Condition()
{
   safe_free(m_dciList);
   safe_free(m_scriptSource);
   delete m_script;
}

/**
 * Load object from database
 */
BOOL Condition::CreateFromDB(UINT32 dwId)
{
   TCHAR szQuery[512];
   DB_RESULT hResult;
   UINT32 i;

   m_dwId = dwId;

   if (!loadCommonProperties())
      return FALSE;

   // Load properties
   _sntprintf(szQuery, 512, _T("SELECT activation_event,deactivation_event,")
                            _T("source_object,active_status,inactive_status,")
                            _T("script FROM conditions WHERE id=%d"), dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult == NULL)
      return FALSE;     // Query failed

   if (DBGetNumRows(hResult) == 0)
   {
      // No object with given ID in database
      DBFreeResult(hResult);
      return FALSE;
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
   m_script = (NXSL_Program *)NXSLCompile(m_scriptSource, szQuery, 512);
   if (m_script == NULL)
      nxlog_write(MSG_COND_SCRIPT_COMPILATION_ERROR, EVENTLOG_ERROR_TYPE,
               "dss", m_dwId, m_szName, szQuery);

   // Load DCI map
   _sntprintf(szQuery, 512, _T("SELECT dci_id,node_id,dci_func,num_polls ")
                            _T("FROM cond_dci_map WHERE condition_id=%d ORDER BY sequence_number"), dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult == NULL)
      return FALSE;     // Query failed

   m_dciCount = DBGetNumRows(hResult);
   if (m_dciCount > 0)
   {
      m_dciList = (INPUT_DCI *)malloc(sizeof(INPUT_DCI) * m_dciCount);
      for(i = 0; i < m_dciCount; i++)
      {
         m_dciList[i].id = DBGetFieldULong(hResult, i, 0);
         m_dciList[i].nodeId = DBGetFieldULong(hResult, i, 1);
         m_dciList[i].function = DBGetFieldLong(hResult, i, 2);
         m_dciList[i].polls = DBGetFieldLong(hResult, i, 3);
      }
   }
   DBFreeResult(hResult);

   return loadACLFromDB();
}

/**
 * Save object to database
 */
BOOL Condition::SaveToDB(DB_HANDLE hdb)
{
   TCHAR *pszEscScript, *pszQuery;
   DB_RESULT hResult;
   BOOL bNewObject = TRUE;
   UINT32 i;

   LockData();

   saveCommonProperties(hdb);

   pszEscScript = EncodeSQLString(CHECK_NULL_EX(m_scriptSource));
	size_t qlen = _tcslen(pszEscScript) + 1024;
   pszQuery = (TCHAR *)malloc(sizeof(TCHAR) * qlen);

   // Check for object's existence in database
   _sntprintf(pszQuery, qlen, _T("SELECT id FROM conditions WHERE id=%d"), m_dwId);
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
                m_dwId, m_activationEventCode, m_deactivationEventCode,
                m_sourceObject, m_activeStatus, m_inactiveStatus, pszEscScript);
   }
   else
   {
      _sntprintf(pszQuery, qlen,
			                 _T("UPDATE conditions SET activation_event=%d,")
                          _T("deactivation_event=%d,source_object=%d,active_status=%d,")
                          _T("inactive_status=%d,script='%s' WHERE id=%d"),
                m_activationEventCode, m_deactivationEventCode, m_sourceObject,
                m_activeStatus, m_inactiveStatus, pszEscScript, m_dwId);
   }
   free(pszEscScript);
   DBQuery(hdb, pszQuery);

   // Save DCI mapping
   _sntprintf(pszQuery, qlen, _T("DELETE FROM cond_dci_map WHERE condition_id=%d"), m_dwId);
   DBQuery(hdb, pszQuery);
   for(i = 0; i < m_dciCount; i++)
   {
      _sntprintf(pszQuery, qlen, _T("INSERT INTO cond_dci_map (condition_id,sequence_number,dci_id,node_id,")
                                 _T("dci_func,num_polls) VALUES (%d,%d,%d,%d,%d,%d)"),
                m_dwId, i, m_dciList[i].id, m_dciList[i].nodeId,
                m_dciList[i].function, m_dciList[i].polls);
      DBQuery(hdb, pszQuery);
   }
   free(pszQuery);

   // Save access list
   saveACLToDB(hdb);

   // Unlock object and clear modification flag
   m_isModified = false;
   UnlockData();
   return TRUE;
}

/**
 * Delete object from database
 */
bool Condition::deleteFromDB(DB_HANDLE hdb)
{
   bool success = NetObj::deleteFromDB(hdb);
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM conditions WHERE id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM cond_dci_map WHERE condition_id=?"));
   return success;
}

/**
 * Create NXCP message from object
 */
void Condition::CreateMessage(CSCPMessage *pMsg)
{
   UINT32 i, dwId;

   NetObj::CreateMessage(pMsg);
   pMsg->SetVariable(VID_SCRIPT, CHECK_NULL_EX(m_scriptSource));
   pMsg->SetVariable(VID_ACTIVATION_EVENT, m_activationEventCode);
   pMsg->SetVariable(VID_DEACTIVATION_EVENT, m_deactivationEventCode);
   pMsg->SetVariable(VID_SOURCE_OBJECT, m_sourceObject);
   pMsg->SetVariable(VID_ACTIVE_STATUS, (WORD)m_activeStatus);
   pMsg->SetVariable(VID_INACTIVE_STATUS, (WORD)m_inactiveStatus);
   pMsg->SetVariable(VID_NUM_ITEMS, m_dciCount);
   for(i = 0, dwId = VID_DCI_LIST_BASE; (i < m_dciCount) && (dwId < (VID_DCI_LIST_LAST + 1)); i++)
   {
      pMsg->SetVariable(dwId++, m_dciList[i].id);
      pMsg->SetVariable(dwId++, m_dciList[i].nodeId);
      pMsg->SetVariable(dwId++, (WORD)m_dciList[i].function);
      pMsg->SetVariable(dwId++, (WORD)m_dciList[i].polls);
      pMsg->SetVariable(dwId++, (WORD)GetDCObjectType(m_dciList[i].nodeId, m_dciList[i].id));
      dwId += 5;
   }
}

/**
 * Modify object from NXCP message
 */
UINT32 Condition::ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked)
{
   UINT32 i, dwId;
   NetObj *pObject;

   if (!bAlreadyLocked)
      LockData();

   // Change script
   if (pRequest->IsVariableExist(VID_SCRIPT))
   {
      TCHAR szError[1024];

      safe_free(m_scriptSource);
      delete m_script;
      m_scriptSource = pRequest->GetVariableStr(VID_SCRIPT);
      m_script = (NXSL_Program *)NXSLCompile(m_scriptSource, szError, 1024);
      if (m_script == NULL)
         nxlog_write(MSG_COND_SCRIPT_COMPILATION_ERROR, EVENTLOG_ERROR_TYPE,
                  "dss", m_dwId, m_szName, szError);
   }

   // Change activation event
   if (pRequest->IsVariableExist(VID_ACTIVATION_EVENT))
      m_activationEventCode = pRequest->GetVariableLong(VID_ACTIVATION_EVENT);

   // Change deactivation event
   if (pRequest->IsVariableExist(VID_DEACTIVATION_EVENT))
      m_deactivationEventCode = pRequest->GetVariableLong(VID_DEACTIVATION_EVENT);

   // Change source object
   if (pRequest->IsVariableExist(VID_SOURCE_OBJECT))
      m_sourceObject = pRequest->GetVariableLong(VID_SOURCE_OBJECT);

   // Change active status
   if (pRequest->IsVariableExist(VID_ACTIVE_STATUS))
      m_activeStatus = pRequest->GetVariableShort(VID_ACTIVE_STATUS);

   // Change inactive status
   if (pRequest->IsVariableExist(VID_INACTIVE_STATUS))
      m_inactiveStatus = pRequest->GetVariableShort(VID_INACTIVE_STATUS);

   // Change DCI list
   if (pRequest->IsVariableExist(VID_NUM_ITEMS))
   {
      safe_free(m_dciList);
      m_dciCount = pRequest->GetVariableLong(VID_NUM_ITEMS);
      if (m_dciCount > 0)
      {
         m_dciList = (INPUT_DCI *)malloc(sizeof(INPUT_DCI) * m_dciCount);
         for(i = 0, dwId = VID_DCI_LIST_BASE; (i < m_dciCount) && (dwId < (VID_DCI_LIST_LAST + 1)); i++)
         {
            m_dciList[i].id = pRequest->GetVariableLong(dwId++);
            m_dciList[i].nodeId = pRequest->GetVariableLong(dwId++);
            m_dciList[i].function = pRequest->GetVariableShort(dwId++);
            m_dciList[i].polls = pRequest->GetVariableShort(dwId++);
            dwId += 6;
         }

         // Update cache size of DCIs
         for(i = 0; i < m_dciCount; i++)
         {
            pObject = FindObjectById(m_dciList[i].nodeId);
            if (pObject != NULL)
            {
               if (pObject->Type() == OBJECT_NODE)
               {
                  DCObject *pItem = ((Node *)pObject)->getDCObjectById(m_dciList[i].id);
                  if ((pItem != NULL) && (pItem->getType() == DCO_TYPE_ITEM))
                  {
                     ((DCItem *)pItem)->updateCacheSize(m_dwId);
                  }
               }
            }
         }
      }
      else
      {
         m_dciList = NULL;
      }
   }

   return NetObj::ModifyFromMessage(pRequest, TRUE);
}

/**
 * Lock for polling
 */
void Condition::lockForPoll()
{
   incRefCount();
   m_queuedForPolling = TRUE;
}

/**
 * This method should be called by poller thread when poll finish
 */
void Condition::endPoll()
{
   LockData();
   m_queuedForPolling = FALSE;
   m_lastPoll = time(NULL);
   UnlockData();
   decRefCount();
}

/**
 * Check condition
 */
void Condition::check()
{
   NXSL_ServerEnv *pEnv;
   NXSL_Value **ppValueList, *pValue;
   NetObj *pObject;
   UINT32 i, dwNumValues;
   int iOldStatus = m_iStatus;

   if ((m_script == NULL) || (m_iStatus == STATUS_UNMANAGED))
      return;

   pEnv = new NXSL_ServerEnv;

   LockData();
   ppValueList = (NXSL_Value **)malloc(sizeof(NXSL_Value *) * m_dciCount);
   memset(ppValueList, 0, sizeof(NXSL_Value *) * m_dciCount);
   for(i = 0; i < m_dciCount; i++)
   {
      pObject = FindObjectById(m_dciList[i].nodeId);
      if (pObject != NULL)
      {
         if (pObject->Type() == OBJECT_NODE)
         {
            DCObject *pItem = ((Node *)pObject)->getDCObjectById(m_dciList[i].id);
            if (pItem != NULL)
            {
               if (pItem->getType() == DCO_TYPE_ITEM)
               {
                  ppValueList[i] = ((DCItem *)pItem)->getValueForNXSL(m_dciList[i].function, m_dciList[i].polls);
               }
               else if (pItem->getType() == DCO_TYPE_TABLE)
               {
                  Table *t = ((DCTable *)pItem)->getLastValue();
                  if (t != NULL)
                  {
                     ppValueList[i] = new NXSL_Value(new NXSL_Object(&g_nxslTableClass, t));
                  }
               }
            }
         }
      }
      if (ppValueList[i] == NULL)
         ppValueList[i] = new NXSL_Value;
   }
   dwNumValues = m_dciCount;
   UnlockData();

	// Create array from values
	NXSL_Array *array = new NXSL_Array;
	for(i = 0; i < dwNumValues; i++)
	{
		array->set(i + 1, new NXSL_Value(ppValueList[i]));
	}
   m_script->setGlobalVariable(_T("$values"), new NXSL_Value(array));

   DbgPrintf(6, _T("Running evaluation script for condition %d \"%s\""),
             m_dwId, m_szName);
   if (m_script->run(pEnv, dwNumValues, ppValueList) == 0)
   {
      pValue = m_script->getResult();
      if (pValue->getValueAsInt32() == 0)
      {
         if (m_isActive)
         {
            // Deactivate condition
            LockData();
            m_iStatus = m_inactiveStatus;
            m_isActive = FALSE;
            Modify();
            UnlockData();

            PostEvent(m_deactivationEventCode,
                      (m_sourceObject == 0) ? g_dwMgmtNode : m_sourceObject,
                      "dsdd", m_dwId, m_szName, iOldStatus, m_iStatus);

            DbgPrintf(6, _T("Condition %d \"%s\" deactivated"),
                      m_dwId, m_szName);
         }
         else
         {
            DbgPrintf(6, _T("Condition %d \"%s\" still inactive"),
                      m_dwId, m_szName);
            LockData();
            if (m_iStatus != m_inactiveStatus)
            {
               m_iStatus = m_inactiveStatus;
               Modify();
            }
            UnlockData();
         }
      }
      else
      {
         if (!m_isActive)
         {
            // Activate condition
            LockData();
            m_iStatus = m_activeStatus;
            m_isActive = TRUE;
            Modify();
            UnlockData();

            PostEvent(m_activationEventCode,
                      (m_sourceObject == 0) ? g_dwMgmtNode : m_sourceObject,
                      "dsdd", m_dwId, m_szName, iOldStatus, m_iStatus);

            DbgPrintf(6, _T("Condition %d \"%s\" activated"),
                      m_dwId, m_szName);
         }
         else
         {
            DbgPrintf(6, _T("Condition %d \"%s\" still active"),
                      m_dwId, m_szName);
            LockData();
            if (m_iStatus != m_activeStatus)
            {
               m_iStatus = m_activeStatus;
               Modify();
            }
            UnlockData();
         }
      }
   }
   else
   {
      nxlog_write(MSG_COND_SCRIPT_EXECUTION_ERROR, EVENTLOG_ERROR_TYPE,
               "dss", m_dwId, m_szName, m_script->getErrorText());

      LockData();
      if (m_iStatus != STATUS_UNKNOWN)
      {
         m_iStatus = STATUS_UNKNOWN;
         Modify();
      }
      UnlockData();
   }
   free(ppValueList);

   // Cause parent object(s) to recalculate it's status
   if (iOldStatus != m_iStatus)
   {
      LockParentList(FALSE);
      for(i = 0; i < m_dwParentCount; i++)
         m_pParentList[i]->calculateCompoundStatus();
      UnlockParentList();
   }
}

/**
 * Determine DCI cache size required by condition object
 */
int Condition::getCacheSizeForDCI(UINT32 itemId, bool noLock)
{
   UINT32 i;
   int nSize = 0;

   if (!noLock)
      LockData();
   for(i = 0; i < m_dciCount; i++)
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
      UnlockData();
   return nSize;
}
