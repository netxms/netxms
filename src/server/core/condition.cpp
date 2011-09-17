/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2011 Victor Kirhenshtein
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


//
// Constructor
//

Condition::Condition()
          :NetObj()
{
   m_pszScript = NULL;
   m_pDCIList = NULL;
   m_dwDCICount = 0;
   m_pszScript = 0;
   m_dwSourceObject = 0;
   m_nActiveStatus = STATUS_MAJOR;
   m_nInactiveStatus = STATUS_NORMAL;
   m_pCompiledScript = NULL;
   m_bIsActive = FALSE;
   m_tmLastPoll = 0;
   m_bQueuedForPolling = FALSE;
   m_dwActivationEventCode = EVENT_CONDITION_ACTIVATED;
   m_dwDeactivationEventCode = EVENT_CONDITION_DEACTIVATED;
}


//
// Constructor for new objects
//

Condition::Condition(BOOL bHidden)
          :NetObj()
{
   m_pszScript = NULL;
   m_pDCIList = NULL;
   m_dwDCICount = 0;
   m_pszScript = 0;
   m_dwSourceObject = 0;
   m_nActiveStatus = STATUS_MAJOR;
   m_nInactiveStatus = STATUS_NORMAL;
   m_bIsHidden = bHidden;
   m_pCompiledScript = NULL;
   m_bIsActive = FALSE;
   m_tmLastPoll = 0;
   m_bQueuedForPolling = FALSE;
   m_dwActivationEventCode = EVENT_CONDITION_ACTIVATED;
   m_dwDeactivationEventCode = EVENT_CONDITION_DEACTIVATED;
}


//
// Destructor
//

Condition::~Condition()
{
   safe_free(m_pDCIList);
   safe_free(m_pszScript);
   delete m_pCompiledScript;
}


//
// Load object from database
//

BOOL Condition::CreateFromDB(DWORD dwId)
{
   TCHAR szQuery[512];
   DB_RESULT hResult;
   DWORD i;

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

   m_dwActivationEventCode = DBGetFieldULong(hResult, 0, 0);
   m_dwDeactivationEventCode = DBGetFieldULong(hResult, 0, 1);
   m_dwSourceObject = DBGetFieldULong(hResult, 0, 2);
   m_nActiveStatus = DBGetFieldLong(hResult, 0, 3);
   m_nInactiveStatus = DBGetFieldLong(hResult, 0, 4);
   m_pszScript = DBGetField(hResult, 0, 5, NULL, 0);
   DecodeSQLString(m_pszScript);
   
   DBFreeResult(hResult);

   // Compile script
   m_pCompiledScript = (NXSL_Program *)NXSLCompile(m_pszScript, szQuery, 512);
   if (m_pCompiledScript == NULL)
      nxlog_write(MSG_COND_SCRIPT_COMPILATION_ERROR, EVENTLOG_ERROR_TYPE,
               "dss", m_dwId, m_szName, szQuery);

   // Load DCI map
   _sntprintf(szQuery, 512, _T("SELECT dci_id,node_id,dci_func,num_polls ")
                            _T("FROM cond_dci_map WHERE condition_id=%d ORDER BY sequence_number"), dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult == NULL)
      return FALSE;     // Query failed

   m_dwDCICount = DBGetNumRows(hResult);
   if (m_dwDCICount > 0)
   {
      m_pDCIList = (INPUT_DCI *)malloc(sizeof(INPUT_DCI) * m_dwDCICount);
      for(i = 0; i < m_dwDCICount; i++)
      {
         m_pDCIList[i].dwId = DBGetFieldULong(hResult, i, 0);
         m_pDCIList[i].dwNodeId = DBGetFieldULong(hResult, i, 1);
         m_pDCIList[i].nFunction = DBGetFieldLong(hResult, i, 2);
         m_pDCIList[i].nPolls = DBGetFieldLong(hResult, i, 3);
      }
   }
   DBFreeResult(hResult);

   return loadACLFromDB();
}


//
// Save object to database
//

BOOL Condition::SaveToDB(DB_HANDLE hdb)
{
   TCHAR *pszEscScript, *pszQuery;
   DB_RESULT hResult;
   BOOL bNewObject = TRUE;
   DWORD i;

   LockData();

   saveCommonProperties(hdb);

   pszEscScript = EncodeSQLString(CHECK_NULL_EX(m_pszScript));
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
                m_dwId, m_dwActivationEventCode, m_dwDeactivationEventCode,
                m_dwSourceObject, m_nActiveStatus, m_nInactiveStatus, pszEscScript);
   }
   else
   {
      _sntprintf(pszQuery, qlen,
			                 _T("UPDATE conditions SET activation_event=%d,")
                          _T("deactivation_event=%d,source_object=%d,active_status=%d,")
                          _T("inactive_status=%d,script='%s' WHERE id=%d"),
                m_dwActivationEventCode, m_dwDeactivationEventCode, m_dwSourceObject,
                m_nActiveStatus, m_nInactiveStatus, pszEscScript, m_dwId);
   }
   free(pszEscScript);
   DBQuery(hdb, pszQuery);

   // Save DCI mapping
   _sntprintf(pszQuery, qlen, _T("DELETE FROM cond_dci_map WHERE condition_id=%d"), m_dwId);
   DBQuery(hdb, pszQuery);
   for(i = 0; i < m_dwDCICount; i++)
   {
      _sntprintf(pszQuery, qlen, _T("INSERT INTO cond_dci_map (condition_id,sequence_number,dci_id,node_id,")
                                 _T("dci_func,num_polls) VALUES (%d,%d,%d,%d,%d,%d)"),
                m_dwId, i, m_pDCIList[i].dwId, m_pDCIList[i].dwNodeId,
                m_pDCIList[i].nFunction, m_pDCIList[i].nPolls);
      DBQuery(hdb, pszQuery);
   }
   free(pszQuery);

   // Save access list
   saveACLToDB(hdb);

   // Unlock object and clear modification flag
   m_bIsModified = FALSE;
   UnlockData();
   return TRUE;
}


//
// Delete object from database
//

BOOL Condition::DeleteFromDB()
{
   TCHAR szQuery[128];
   BOOL bSuccess;

   bSuccess = NetObj::DeleteFromDB();
   if (bSuccess)
   {
      _sntprintf(szQuery, 128, _T("DELETE FROM conditions WHERE id=%d"), m_dwId);
      QueueSQLRequest(szQuery);
      _sntprintf(szQuery, 128, _T("DELETE FROM cond_dci_map WHERE condition_id=%d"), m_dwId);
      QueueSQLRequest(szQuery);
   }
   return bSuccess;
}


//
// Create NXCP message from object
//

void Condition::CreateMessage(CSCPMessage *pMsg)
{
   DWORD i, dwId;

   NetObj::CreateMessage(pMsg);
   pMsg->SetVariable(VID_SCRIPT, CHECK_NULL_EX(m_pszScript));
   pMsg->SetVariable(VID_ACTIVATION_EVENT, m_dwActivationEventCode);
   pMsg->SetVariable(VID_DEACTIVATION_EVENT, m_dwDeactivationEventCode);
   pMsg->SetVariable(VID_SOURCE_OBJECT, m_dwSourceObject);
   pMsg->SetVariable(VID_ACTIVE_STATUS, (WORD)m_nActiveStatus);
   pMsg->SetVariable(VID_INACTIVE_STATUS, (WORD)m_nInactiveStatus);
   pMsg->SetVariable(VID_NUM_ITEMS, m_dwDCICount);
   for(i = 0, dwId = VID_DCI_LIST_BASE; (i < m_dwDCICount) && (dwId < (VID_DCI_LIST_LAST + 1)); i++)
   {
      pMsg->SetVariable(dwId++, m_pDCIList[i].dwId);
      pMsg->SetVariable(dwId++, m_pDCIList[i].dwNodeId);
      pMsg->SetVariable(dwId++, (WORD)m_pDCIList[i].nFunction);
      pMsg->SetVariable(dwId++, (WORD)m_pDCIList[i].nPolls);
      dwId += 6;
   }
}


//
// Modify object from NXCP message
//

DWORD Condition::ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked)
{
   DWORD i, dwId;
   NetObj *pObject;
   DCItem *pItem;

   if (!bAlreadyLocked)
      LockData();

   // Change script
   if (pRequest->IsVariableExist(VID_SCRIPT))
   {
      TCHAR szError[1024];

      safe_free(m_pszScript);
      delete m_pCompiledScript;
      m_pszScript = pRequest->GetVariableStr(VID_SCRIPT);
      m_pCompiledScript = (NXSL_Program *)NXSLCompile(m_pszScript, szError, 1024);
      if (m_pCompiledScript == NULL)
         nxlog_write(MSG_COND_SCRIPT_COMPILATION_ERROR, EVENTLOG_ERROR_TYPE,
                  "dss", m_dwId, m_szName, szError);
   }

   // Change activation event
   if (pRequest->IsVariableExist(VID_ACTIVATION_EVENT))
      m_dwActivationEventCode = pRequest->GetVariableLong(VID_ACTIVATION_EVENT);

   // Change deactivation event
   if (pRequest->IsVariableExist(VID_DEACTIVATION_EVENT))
      m_dwDeactivationEventCode = pRequest->GetVariableLong(VID_DEACTIVATION_EVENT);

   // Change source object
   if (pRequest->IsVariableExist(VID_SOURCE_OBJECT))
      m_dwSourceObject = pRequest->GetVariableLong(VID_SOURCE_OBJECT);

   // Change active status
   if (pRequest->IsVariableExist(VID_ACTIVE_STATUS))
      m_nActiveStatus = pRequest->GetVariableShort(VID_ACTIVE_STATUS);

   // Change inactive status
   if (pRequest->IsVariableExist(VID_INACTIVE_STATUS))
      m_nInactiveStatus = pRequest->GetVariableShort(VID_INACTIVE_STATUS);

   // Change DCI list
   if (pRequest->IsVariableExist(VID_NUM_ITEMS))
   {
      safe_free(m_pDCIList);
      m_dwDCICount = pRequest->GetVariableLong(VID_NUM_ITEMS);
      if (m_dwDCICount > 0)
      {
         m_pDCIList = (INPUT_DCI *)malloc(sizeof(INPUT_DCI) * m_dwDCICount);
         for(i = 0, dwId = VID_DCI_LIST_BASE; (i < m_dwDCICount) && (dwId < (VID_DCI_LIST_LAST + 1)); i++)
         {
            m_pDCIList[i].dwId = pRequest->GetVariableLong(dwId++);
            m_pDCIList[i].dwNodeId = pRequest->GetVariableLong(dwId++);
            m_pDCIList[i].nFunction = pRequest->GetVariableShort(dwId++);
            m_pDCIList[i].nPolls = pRequest->GetVariableShort(dwId++);
            dwId += 6;
         }

         // Update cache size of DCIs
         for(i = 0; i < m_dwDCICount; i++)
         {
            pObject = FindObjectById(m_pDCIList[i].dwNodeId);
            if (pObject != NULL)
            {
               if (pObject->Type() == OBJECT_NODE)
               {
                  pItem = ((Node *)pObject)->getItemById(m_pDCIList[i].dwId);
                  if (pItem != NULL)
                  {
                     pItem->updateCacheSize(m_dwId);
                  }
               }
            }
         }
      }
      else
      {
         m_pDCIList = NULL;
      }
   }

   return NetObj::ModifyFromMessage(pRequest, TRUE);
}


//
// Lock for polling
//

void Condition::LockForPoll()
{
   IncRefCount();
   m_bQueuedForPolling = TRUE;
}


//
// This method should be callsed by poller thread when poll finish
//

void Condition::EndPoll()
{
   LockData();
   m_bQueuedForPolling = FALSE;
   m_tmLastPoll = time(NULL);
   UnlockData();
   DecRefCount();
}


//
// Check condition
//

void Condition::check()
{
   NXSL_ServerEnv *pEnv;
   NXSL_Value **ppValueList, *pValue;
   NetObj *pObject;
   DCItem *pItem;
   DWORD i, dwNumValues;
   int iOldStatus = m_iStatus;

   if ((m_pCompiledScript == NULL) || (m_iStatus == STATUS_UNMANAGED))
      return;

   pEnv = new NXSL_ServerEnv;

   LockData();
   ppValueList = (NXSL_Value **)malloc(sizeof(NXSL_Value *) * m_dwDCICount);
   memset(ppValueList, 0, sizeof(NXSL_Value *) * m_dwDCICount);
   for(i = 0; i < m_dwDCICount; i++)
   {
      pObject = FindObjectById(m_pDCIList[i].dwNodeId);
      if (pObject != NULL)
      {
         if (pObject->Type() == OBJECT_NODE)
         {
            pItem = ((Node *)pObject)->getItemById(m_pDCIList[i].dwId);
            if (pItem != NULL)
            {
               ppValueList[i] = pItem->getValueForNXSL(m_pDCIList[i].nFunction, m_pDCIList[i].nPolls);
            }
         }
      }
      if (ppValueList[i] == NULL)
         ppValueList[i] = new NXSL_Value;
   }
   dwNumValues = m_dwDCICount;
   UnlockData();

	// Create array from values
	NXSL_Array *array = new NXSL_Array;
	for(i = 0; i < dwNumValues; i++)
	{
		array->set(i + 1, new NXSL_Value(ppValueList[i]));
	}
   m_pCompiledScript->setGlobalVariable(_T("$values"), new NXSL_Value(array));

   DbgPrintf(6, _T("Running evaluation script for condition %d \"%s\""),
             m_dwId, m_szName);
   if (m_pCompiledScript->run(pEnv, dwNumValues, ppValueList) == 0)
   {
      pValue = m_pCompiledScript->getResult();
      if (pValue->getValueAsInt32() == 0)
      {
         if (m_bIsActive)
         {
            // Deactivate condition
            LockData();
            m_iStatus = m_nInactiveStatus;
            m_bIsActive = FALSE;
            Modify();
            UnlockData();

            PostEvent(m_dwDeactivationEventCode,
                      (m_dwSourceObject == 0) ? g_dwMgmtNode : m_dwSourceObject,
                      "dsdd", m_dwId, m_szName, iOldStatus, m_iStatus);

            DbgPrintf(6, _T("Condition %d \"%s\" deactivated"),
                      m_dwId, m_szName);
         }
         else
         {
            DbgPrintf(6, _T("Condition %d \"%s\" still inactive"),
                      m_dwId, m_szName);
            LockData();
            if (m_iStatus != m_nInactiveStatus)
            {
               m_iStatus = m_nInactiveStatus;
               Modify();
            }
            UnlockData();
         }
      }
      else
      {
         if (!m_bIsActive)
         {
            // Activate condition
            LockData();
            m_iStatus = m_nActiveStatus;
            m_bIsActive = TRUE;
            Modify();
            UnlockData();

            PostEvent(m_dwActivationEventCode,
                      (m_dwSourceObject == 0) ? g_dwMgmtNode : m_dwSourceObject,
                      "dsdd", m_dwId, m_szName, iOldStatus, m_iStatus);

            DbgPrintf(6, _T("Condition %d \"%s\" activated"),
                      m_dwId, m_szName);
         }
         else
         {
            DbgPrintf(6, _T("Condition %d \"%s\" still active"),
                      m_dwId, m_szName);
            LockData();
            if (m_iStatus != m_nActiveStatus)
            {
               m_iStatus = m_nActiveStatus;
               Modify();
            }
            UnlockData();
         }
      }
   }
   else
   {
      nxlog_write(MSG_COND_SCRIPT_EXECUTION_ERROR, EVENTLOG_ERROR_TYPE,
               "dss", m_dwId, m_szName, m_pCompiledScript->getErrorText());

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


//
// Determine DCI cache size required by condition object
//

int Condition::getCacheSizeForDCI(DWORD dwItemId, BOOL bNoLock)
{
   DWORD i;
   int nSize = 0;

   if (!bNoLock)
      LockData();
   for(i = 0; i < m_dwDCICount; i++)
   {
      if (m_pDCIList[i].dwId == dwItemId)
      {
         switch(m_pDCIList[i].nFunction)
         {
            case F_LAST:
               nSize = 1;
               break;
            case F_DIFF:
               nSize = 2;
               break;
            default:
               nSize = m_pDCIList[i].nPolls;
               break;
         }
         break;
      }
   }
   if (!bNoLock)
      UnlockData();
   return nSize;
}
