/*
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2003-2011 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: session.cpp
**
**/

#include "libnxcl.h"

#ifdef _WIN32
#define close	_close
#endif


//
// Session class constructor
//

NXCL_Session::NXCL_Session()
{
   int i;

   m_dwFlags = 0;
   m_dwMsgId = 0;
   m_dwTimeStamp = 0;
   m_pEventHandler = NULL;
   m_dwState = STATE_DISCONNECTED;
   m_dwCommandTimeout = 30000;    // Default timeout is 30 seconds
   m_dwNumObjects = 0;
   m_pIndexById = NULL;
   m_mutexIndexAccess = MutexCreate();
   m_dwReceiverBufferSize = 4194304;     // 4MB
   m_hSocket = -1;
   m_pItemList = NULL;
   m_szLastLock[0] = 0;
   m_pClientData = NULL;
	m_szServerTimeZone[0] = 0;
	m_mutexSendMsg = MutexCreate();

   m_ppEventTemplates = NULL;
   m_dwNumTemplates = 0;
   m_mutexEventAccess = MutexCreate();

   m_dwNumUsers = 0;
   m_pUserList = NULL;

   m_hRecvThread = INVALID_THREAD_HANDLE;
   m_hWatchdogThread = INVALID_THREAD_HANDLE;
   m_pCtx = NULL;

   m_hCurrFile = -1;
   m_dwFileRqId = 0;
   m_condFileRq = ConditionCreate(FALSE);
   m_mutexFileRq = MutexCreate();

   m_condStopThreads = ConditionCreate(TRUE);

   for(i = 0; i < SYNC_OP_COUNT; i++)
   {
      m_mutexSyncOpAccess[i] = MutexCreate();
      m_dwSyncExitCode[i] = 0;
#ifdef _WIN32
      m_condSyncOp[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
#else
      pthread_mutex_init(&m_mutexSyncOp[i], NULL);
      pthread_cond_init(&m_condSyncOp[i], NULL);
      m_bSyncFinished[i] = FALSE;
#endif
   }
}


//
// Session class destructor
//

NXCL_Session::~NXCL_Session()
{
   int i;

   disconnect();

   // Wait for receiver thread termination
   if (m_hRecvThread != INVALID_THREAD_HANDLE)
      ThreadJoin(m_hRecvThread);

   MutexDestroy(m_mutexIndexAccess);

   MutexLock(m_mutexEventAccess);
   MutexUnlock(m_mutexEventAccess);
   MutexDestroy(m_mutexEventAccess);

   ConditionSet(m_condFileRq);
   MutexLock(m_mutexFileRq);
   MutexUnlock(m_mutexFileRq);
   MutexDestroy(m_mutexFileRq);
   ConditionDestroy(m_condFileRq);

   ConditionDestroy(m_condStopThreads);

   for(i = 0; i < SYNC_OP_COUNT; i++)
   {
      MutexDestroy(m_mutexSyncOpAccess[i]);
#ifdef _WIN32
      CloseHandle(m_condSyncOp[i]);
#else
      pthread_mutex_destroy(&m_mutexSyncOp[i]);
      pthread_cond_destroy(&m_condSyncOp[i]);
#endif
   }

	MutexDestroy(m_mutexSendMsg);

	if (m_pCtx != NULL)
		m_pCtx->decRefCount();
}


//
// Disconnect session
//

void NXCL_Session::disconnect()
{
   // Terminate watchdog thread
   ConditionSet(m_condStopThreads);
   if (m_hWatchdogThread != INVALID_THREAD_HANDLE)
   {
      ThreadJoin(m_hWatchdogThread);
      m_hWatchdogThread = INVALID_THREAD_HANDLE;
   }
   ConditionReset(m_condStopThreads);

   // Close socket
   shutdown(m_hSocket, SHUT_RDWR);
   closesocket(m_hSocket);
   m_hSocket = -1;

   // Clear message wait queue
   m_msgWaitQueue.clear();

   // Cleanup
   destroyAllObjects();
   destroyEventDB();
   destroyUserDB();

	if (m_pCtx != NULL)
	{
		m_pCtx->decRefCount();
		m_pCtx = NULL;
	}
}


//
// Destroy all objects
//

void NXCL_Session::destroyAllObjects()
{
   UINT32 i;

   MutexLock(m_mutexIndexAccess);
   for(i = 0; i < m_dwNumObjects; i++)
      DestroyObject(m_pIndexById[i].pObject);
   m_dwNumObjects = 0;
   safe_free(m_pIndexById);
   m_pIndexById = NULL;
   MutexUnlock(m_mutexIndexAccess);
}


//
// Wait for specific message
//

CSCPMessage *NXCL_Session::WaitForMessage(WORD wCode, UINT32 dwId, UINT32 dwTimeOut)
{
   if (m_dwFlags & NXC_SF_CONN_BROKEN)
      return NULL;
   return m_msgWaitQueue.waitForMessage(wCode, dwId,
      dwTimeOut == 0 ? m_dwCommandTimeout : dwTimeOut);
}


//
// Wait for specific raw message
//

CSCP_MESSAGE *NXCL_Session::WaitForRawMessage(WORD wCode, UINT32 dwId, UINT32 dwTimeOut)
{
   if (m_dwFlags & NXC_SF_CONN_BROKEN)
      return NULL;
   return m_msgWaitQueue.waitForRawMessage(wCode, dwId,
      dwTimeOut == 0 ? m_dwCommandTimeout : dwTimeOut);
}


//
// Wait for request completion notification and extract result code
// from recived message
//

UINT32 NXCL_Session::WaitForRCC(UINT32 dwRqId, UINT32 dwTimeOut)
{
   CSCPMessage *pResponse;
   UINT32 dwRetCode;

   if (m_dwFlags & NXC_SF_CONN_BROKEN)
   {
      dwRetCode = RCC_CONNECTION_BROKEN;
   }
   else
   {
      pResponse = WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId, dwTimeOut);
      if (pResponse != NULL)
      {
         dwRetCode = pResponse->GetVariableLong(VID_RCC);
         if (dwRetCode == RCC_COMPONENT_LOCKED)
         {
            _tcscpy(m_szLastLock, _T("<unknown>"));
            if (pResponse->isFieldExist(VID_LOCKED_BY))
            {
               pResponse->GetVariableStr(VID_LOCKED_BY, m_szLastLock, MAX_LOCKINFO_LEN);
            }
         }
         delete pResponse;
      }
      else
      {
         dwRetCode = RCC_TIMEOUT;
      }
   }
   return dwRetCode;
}


//
// Send CSCP message
//

BOOL NXCL_Session::SendMsg(CSCPMessage *pMsg)
{
   CSCP_MESSAGE *pRawMsg;
   NXCP_ENCRYPTED_MESSAGE *pEnMsg;
   BOOL bResult;
   TCHAR szBuffer[128];

   if (m_dwFlags & NXC_SF_CONN_BROKEN)
      return FALSE;

   DebugPrintf(_T("SendMsg(\"%s\", id:%d)"), NXCPMessageCodeName(pMsg->GetCode(), szBuffer), pMsg->GetId());
   pRawMsg = pMsg->createMessage();
	MutexLock(m_mutexSendMsg);
   if (m_pCtx != NULL)
   {
      pEnMsg = CSCPEncryptMessage(m_pCtx, pRawMsg);
      if (pEnMsg != NULL)
      {
         bResult = (SendEx(m_hSocket, (char *)pEnMsg, ntohl(pEnMsg->dwSize), 0, NULL) == (int)ntohl(pEnMsg->dwSize));
         free(pEnMsg);
      }
      else
      {
         bResult = FALSE;
      }
   }
   else
   {
      bResult = (SendEx(m_hSocket, (char *)pRawMsg, ntohl(pRawMsg->dwSize), 0, NULL) == (int)ntohl(pRawMsg->dwSize));
   }
	MutexUnlock(m_mutexSendMsg);
   free(pRawMsg);
   return bResult;
}


//
// Wait for synchronization operation completion
//

UINT32 NXCL_Session::WaitForSync(int nSyncOp, UINT32 dwTimeOut)
{
#ifdef _WIN32
   UINT32 dwRetCode;

   dwRetCode = WaitForSingleObject(m_condSyncOp[nSyncOp], dwTimeOut);
   MutexUnlock(m_mutexSyncOpAccess[nSyncOp]);
   return (dwRetCode == WAIT_TIMEOUT) ? RCC_TIMEOUT : m_dwSyncExitCode[nSyncOp];
#else
   int iRetCode;
   UINT32 dwResult;

   pthread_mutex_lock(&m_mutexSyncOp[nSyncOp]);
   if (!m_bSyncFinished[nSyncOp])
   {
      if (dwTimeOut != INFINITE)
	   {
#if HAVE_PTHREAD_COND_RELTIMEDWAIT_NP
		   struct timespec timeout;

		   timeout.tv_sec = dwTimeOut / 1000;
		   timeout.tv_nsec = (dwTimeOut % 1000) * 1000000;
		   iRetCode = pthread_cond_reltimedwait_np(&m_condSyncOp[nSyncOp], &m_mutexSyncOp[nSyncOp], &timeout);
#else
		   struct timeval now;
		   struct timespec timeout;

		   gettimeofday(&now, NULL);
		   timeout.tv_sec = now.tv_sec + (dwTimeOut / 1000);
		   timeout.tv_nsec = (now.tv_usec + (dwTimeOut % 1000) * 1000) * 1000;
		   iRetCode = pthread_cond_timedwait(&m_condSyncOp[nSyncOp], &m_mutexSyncOp[nSyncOp], &timeout);
#endif
	   }
	   else
      {
         iRetCode = pthread_cond_wait(&m_condSyncOp[nSyncOp], &m_mutexSyncOp[nSyncOp]);
      }
      dwResult = (iRetCode == 0) ? m_dwSyncExitCode[nSyncOp] : RCC_TIMEOUT;
   }
   else
   {
      dwResult = m_dwSyncExitCode[nSyncOp];
   }
   pthread_mutex_unlock(&m_mutexSyncOp[nSyncOp]);
   MutexUnlock(m_mutexSyncOpAccess[nSyncOp]);
   return dwResult;
#endif
}


//
// Prepare for synchronization operation
//

void NXCL_Session::PrepareForSync(int nSyncOp)
{
   MutexLock(m_mutexSyncOpAccess[nSyncOp]);
   m_dwSyncExitCode[nSyncOp] = RCC_SYSTEM_FAILURE;
#ifdef _WIN32
   ResetEvent(m_condSyncOp[nSyncOp]);
#else
   m_bSyncFinished[nSyncOp] = FALSE;
#endif
}


//
// Complete synchronization operation
//

void NXCL_Session::CompleteSync(int nSyncOp, UINT32 dwRetCode)
{
#ifdef _WIN32
   m_dwSyncExitCode[nSyncOp] = dwRetCode;
   SetEvent(m_condSyncOp[nSyncOp]);
#else
   pthread_mutex_lock(&m_mutexSyncOp[nSyncOp]);
   m_dwSyncExitCode[nSyncOp] = dwRetCode;
   m_bSyncFinished[nSyncOp] = TRUE;
   pthread_cond_signal(&m_condSyncOp[nSyncOp]);
   pthread_mutex_unlock(&m_mutexSyncOp[nSyncOp]);
#endif
}

/**
 * Process DCIs coming from server
 */
void NXCL_Session::processDCI(CSCPMessage *pMsg)
{
	if (pMsg->isEndOfSequence())
	{
      CompleteSync(SYNC_DCI_LIST, RCC_SUCCESS);
	}
	else if (m_pItemList != NULL)
   {
      UINT32 i, j, dwId;

      i = m_pItemList->dwNumItems;
      m_pItemList->dwNumItems++;
      m_pItemList->pItems = (NXC_DCI *)realloc(m_pItemList->pItems,
                                    sizeof(NXC_DCI) * m_pItemList->dwNumItems);
      m_pItemList->pItems[i].dwId = pMsg->GetVariableLong(VID_DCI_ID);
      m_pItemList->pItems[i].dwTemplateId = pMsg->GetVariableLong(VID_TEMPLATE_ID);
      m_pItemList->pItems[i].dwResourceId = pMsg->GetVariableLong(VID_RESOURCE_ID);
      m_pItemList->pItems[i].dwProxyNode = pMsg->GetVariableLong(VID_AGENT_PROXY);
      m_pItemList->pItems[i].iDataType = (BYTE)pMsg->GetVariableShort(VID_DCI_DATA_TYPE);
      m_pItemList->pItems[i].iPollingInterval = (int)pMsg->GetVariableLong(VID_POLLING_INTERVAL);
      m_pItemList->pItems[i].iRetentionTime = (int)pMsg->GetVariableLong(VID_RETENTION_TIME);
      m_pItemList->pItems[i].iSource = (BYTE)pMsg->GetVariableShort(VID_DCI_SOURCE_TYPE);
      m_pItemList->pItems[i].iStatus = (BYTE)pMsg->GetVariableShort(VID_DCI_STATUS);
      m_pItemList->pItems[i].iDeltaCalculation = (BYTE)pMsg->GetVariableShort(VID_DCI_DELTA_CALCULATION);
		m_pItemList->pItems[i].wFlags = pMsg->GetVariableShort(VID_FLAGS);
		m_pItemList->pItems[i].wSnmpRawType = (BYTE)pMsg->GetVariableShort(VID_SNMP_RAW_VALUE_TYPE);
		m_pItemList->pItems[i].pszFormula = pMsg->GetVariableStr(VID_TRANSFORMATION_SCRIPT);
      pMsg->GetVariableStr(VID_NAME, m_pItemList->pItems[i].szName, MAX_ITEM_NAME);
      pMsg->GetVariableStr(VID_DESCRIPTION, m_pItemList->pItems[i].szDescription, MAX_DB_STRING);
      pMsg->GetVariableStr(VID_INSTANCE, m_pItemList->pItems[i].szInstance, MAX_DB_STRING);
		pMsg->GetVariableStr(VID_SYSTEM_TAG, m_pItemList->pItems[i].szSystemTag, MAX_DB_STRING);
		m_pItemList->pItems[i].nBaseUnits = (int)pMsg->GetVariableShort(VID_BASE_UNITS);
		m_pItemList->pItems[i].nMultiplier = (int)pMsg->GetVariableLong(VID_MULTIPLIER);
		m_pItemList->pItems[i].pszCustomUnitName = pMsg->GetVariableStr(VID_CUSTOM_UNITS_NAME);
		m_pItemList->pItems[i].pszPerfTabSettings = pMsg->GetVariableStr(VID_PERFTAB_SETTINGS);
		m_pItemList->pItems[i].nSnmpPort = pMsg->GetVariableShort(VID_SNMP_PORT);
      m_pItemList->pItems[i].dwNumSchedules = pMsg->GetVariableLong(VID_NUM_SCHEDULES);
      m_pItemList->pItems[i].comments = pMsg->GetVariableStr(VID_COMMENTS);
      m_pItemList->pItems[i].ppScheduleList = (TCHAR **)malloc(sizeof(TCHAR *) * m_pItemList->pItems[i].dwNumSchedules);
      for(j = 0, dwId = VID_DCI_SCHEDULE_BASE; j < m_pItemList->pItems[i].dwNumSchedules; j++, dwId++)
         m_pItemList->pItems[i].ppScheduleList[j] = pMsg->GetVariableStr(dwId);
      m_pItemList->pItems[i].dwNumThresholds = pMsg->GetVariableLong(VID_NUM_THRESHOLDS);
      m_pItemList->pItems[i].pThresholdList =
         (NXC_DCI_THRESHOLD *)malloc(sizeof(NXC_DCI_THRESHOLD) * m_pItemList->pItems[i].dwNumThresholds);
      for(j = 0, dwId = VID_DCI_THRESHOLD_BASE; j < m_pItemList->pItems[i].dwNumThresholds; j++, dwId++)
      {
			m_pItemList->pItems[i].pThresholdList[j].id = pMsg->GetVariableLong(dwId++);
         m_pItemList->pItems[i].pThresholdList[j].activationEvent = pMsg->GetVariableLong(dwId++);
         m_pItemList->pItems[i].pThresholdList[j].rearmEvent = pMsg->GetVariableLong(dwId++);
         m_pItemList->pItems[i].pThresholdList[j].function = pMsg->GetVariableShort(dwId++);
         m_pItemList->pItems[i].pThresholdList[j].operation = pMsg->GetVariableShort(dwId++);
         m_pItemList->pItems[i].pThresholdList[j].sampleCount = pMsg->GetVariableLong(dwId++);
         m_pItemList->pItems[i].pThresholdList[j].script = pMsg->GetVariableStr(dwId++);
         m_pItemList->pItems[i].pThresholdList[j].repeatInterval = (LONG)pMsg->GetVariableLong(dwId++);
			pMsg->GetVariableStr(dwId++, m_pItemList->pItems[i].pThresholdList[j].value, MAX_STRING_VALUE);
      }
   }
}

/**
 * Load data collection items list for specified node
 * This function is NOT REENTRANT
 */
UINT32 NXCL_Session::OpenNodeDCIList(UINT32 dwNodeId, NXC_DCI_LIST **ppItemList)
{
   CSCPMessage msg;
   UINT32 dwRetCode, dwRqId;

   dwRqId = CreateRqId();
   PrepareForSync(SYNC_DCI_LIST);

   m_pItemList = (NXC_DCI_LIST *)malloc(sizeof(NXC_DCI_LIST));
   m_pItemList->dwNodeId = dwNodeId;
   m_pItemList->dwNumItems = 0;
   m_pItemList->pItems = NULL;

   msg.SetCode(CMD_GET_NODE_DCI_LIST);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, dwNodeId);
   SendMsg(&msg);

   dwRetCode = WaitForRCC(dwRqId);

   if (dwRetCode == RCC_SUCCESS)
   {
      // Wait for DCI list end or for disconnection
      dwRetCode = WaitForSync(SYNC_DCI_LIST, INFINITE);
      if (dwRetCode == RCC_SUCCESS)
      {
         *ppItemList = m_pItemList;
      }
      else
      {
         free(m_pItemList);
      }
   }
   else
   {
      UnlockSyncOp(SYNC_DCI_LIST);
      free(m_pItemList);
   }

   m_pItemList = NULL;
   return dwRetCode;
}

/**
 * Load event configuration database
 */
UINT32 NXCL_Session::LoadEventDB()
{
   CSCPMessage msg;
   UINT32 dwRetCode, dwRqId;

   dwRqId = CreateRqId();
   PrepareForSync(SYNC_EVENT_DB);

   destroyEventDB();
   MutexLock(m_mutexEventAccess);

   msg.SetCode(CMD_LOAD_EVENT_DB);
   msg.SetId(dwRqId);
   SendMsg(&msg);

   dwRetCode = WaitForRCC(dwRqId);

   /* TODO: this probably should be recoded as loop with calls to WaitForMessage() */
   if (dwRetCode == RCC_SUCCESS)
      dwRetCode = WaitForSync(SYNC_EVENT_DB, INFINITE);
   else
      UnlockSyncOp(SYNC_EVENT_DB);

   MutexUnlock(m_mutexEventAccess);
   return dwRetCode;
}


//
// Destroy event template database
//

void NXCL_Session::destroyEventDB()
{
   UINT32 i;

   for(i = 0; i < m_dwNumTemplates; i++)
   {
      safe_free(m_ppEventTemplates[i]->pszDescription);
      safe_free(m_ppEventTemplates[i]->pszMessage);
      free(m_ppEventTemplates[i]);
   }
   safe_free(m_ppEventTemplates);
   m_dwNumTemplates = 0;
   m_ppEventTemplates = NULL;
}


//
// Delete record from list
//

void NXCL_Session::DeleteEDBRecord(UINT32 dwEventCode)
{
   UINT32 i;

   MutexLock(m_mutexEventAccess);
   for(i = 0; i < m_dwNumTemplates; i++)
      if (m_ppEventTemplates[i]->dwCode == dwEventCode)
      {
         m_dwNumTemplates--;
         safe_free(m_ppEventTemplates[i]->pszDescription);
         safe_free(m_ppEventTemplates[i]->pszMessage);
         free(m_ppEventTemplates[i]);
         memmove(&m_ppEventTemplates[i], m_ppEventTemplates[i + 1],
                 sizeof(NXC_EVENT_TEMPLATE *) * (m_dwNumTemplates - i));
         break;
      }
   MutexUnlock(m_mutexEventAccess);
}


//
// Add template to list
//

void NXCL_Session::AddEventTemplate(NXC_EVENT_TEMPLATE *pEventTemplate, BOOL bLock)
{
   if (bLock)
      MutexLock(m_mutexEventAccess);
   m_ppEventTemplates = (NXC_EVENT_TEMPLATE **)realloc(m_ppEventTemplates,
      sizeof(NXC_EVENT_TEMPLATE *) * (m_dwNumTemplates + 1));
   m_ppEventTemplates[m_dwNumTemplates] = pEventTemplate;
   m_dwNumTemplates++;
   if (bLock)
      MutexUnlock(m_mutexEventAccess);
}


//
// Get pointer to event templates list
//

BOOL NXCL_Session::GetEventDB(NXC_EVENT_TEMPLATE ***pppTemplateList, UINT32 *pdwNumRecords)
{
   *pppTemplateList = m_ppEventTemplates;
   *pdwNumRecords = m_dwNumTemplates;
   return TRUE;
}


//
// Resolve event id to name
//

const TCHAR *NXCL_Session::GetEventName(UINT32 dwId)
{
   UINT32 i;

   MutexLock(m_mutexEventAccess);
   for(i = 0; i < m_dwNumTemplates; i++)
      if (m_ppEventTemplates[i]->dwCode == dwId)
      {
         MutexUnlock(m_mutexEventAccess);
         return m_ppEventTemplates[i]->szName;
      }
   MutexUnlock(m_mutexEventAccess);
   return _T("<unknown>");
}


//
// Resolve event id to name using application-provided buffer
//

BOOL NXCL_Session::GetEventNameEx(UINT32 dwId, TCHAR *pszBuffer, UINT32 dwBufSize)
{
   UINT32 i;

   MutexLock(m_mutexEventAccess);
   for(i = 0; i < m_dwNumTemplates; i++)
      if (m_ppEventTemplates[i]->dwCode == dwId)
      {
         nx_strncpy(pszBuffer, m_ppEventTemplates[i]->szName, dwBufSize);
         MutexUnlock(m_mutexEventAccess);
         return TRUE;
      }
   MutexUnlock(m_mutexEventAccess);
   *pszBuffer = 0;
   return FALSE;
}


//
// Get severity for given event id. Will return -1 for unknown id.
//

int NXCL_Session::GetEventSeverity(UINT32 dwId)
{
   UINT32 i;

   MutexLock(m_mutexEventAccess);
   for(i = 0; i < m_dwNumTemplates; i++)
      if (m_ppEventTemplates[i]->dwCode == dwId)
      {
         MutexUnlock(m_mutexEventAccess);
         return (int)(m_ppEventTemplates[i]->dwSeverity);
      }
   MutexUnlock(m_mutexEventAccess);
   return -1;
}


//
// Get text of event template with given ID.
// If there are no template with such ID, empty string will be returned.
//

BOOL NXCL_Session::GetEventText(UINT32 dwId, TCHAR *pszBuffer, UINT32 dwBufSize)
{
   UINT32 i;

   MutexLock(m_mutexEventAccess);
   for(i = 0; i < m_dwNumTemplates; i++)
      if (m_ppEventTemplates[i]->dwCode == dwId)
      {
         nx_strncpy(pszBuffer, m_ppEventTemplates[i]->pszMessage, dwBufSize);
         MutexUnlock(m_mutexEventAccess);
         return TRUE;
      }
   MutexUnlock(m_mutexEventAccess);
   *pszBuffer = 0;
   return FALSE;
}


//
// Destroy user database
//

void NXCL_Session::destroyUserDB()
{
   UINT32 i;

   for(i = 0; i < m_dwNumUsers; i++)
      safe_free(m_pUserList[i].pdwMemberList);
   safe_free(m_pUserList);
   m_pUserList = NULL;
   m_dwNumUsers = 0;
   m_dwFlags &= ~NXC_SF_USERDB_LOADED;
}


//
// Process user record from network
//

void NXCL_Session::processUserDBRecord(CSCPMessage *pMsg)
{
   switch(pMsg->GetCode())
   {
      case CMD_USER_DB_EOF:
         CompleteSync(SYNC_USER_DB, RCC_SUCCESS);
         break;
      case CMD_USER_DATA:
      case CMD_GROUP_DATA:
         m_pUserList = (NXC_USER *)realloc(m_pUserList, sizeof(NXC_USER) * (m_dwNumUsers + 1));
         memset(&m_pUserList[m_dwNumUsers], 0, sizeof(NXC_USER));
         UpdateUserFromMessage(pMsg, &m_pUserList[m_dwNumUsers]);
         m_dwNumUsers++;
         break;
      default:
         break;
   }
}


//
// Process user database update
//

void NXCL_Session::processUserDBUpdate(CSCPMessage *pMsg)
{
   int iCode;
   UINT32 dwUserId;
   NXC_USER *pUser;

   iCode = pMsg->GetVariableShort(VID_UPDATE_TYPE);
   dwUserId = pMsg->GetVariableLong(VID_USER_ID);
   pUser = FindUserById(dwUserId);

   switch(iCode)
   {
      case USER_DB_CREATE:
         if (pUser == NULL)
         {
            // No user with this id, create one
            m_pUserList = (NXC_USER *)realloc(m_pUserList, sizeof(NXC_USER) * (m_dwNumUsers + 1));
            memset(&m_pUserList[m_dwNumUsers], 0, sizeof(NXC_USER));

            // Process common fields
            m_pUserList[m_dwNumUsers].dwId = dwUserId;
            pMsg->GetVariableStr(VID_USER_NAME, m_pUserList[m_dwNumUsers].szName, MAX_USER_NAME);
            pUser = &m_pUserList[m_dwNumUsers];
            m_dwNumUsers++;
         }
         break;
      case USER_DB_MODIFY:
         if (pUser == NULL)
         {
            // No user with this id, create one
            m_pUserList = (NXC_USER *)realloc(m_pUserList, sizeof(NXC_USER) * (m_dwNumUsers + 1));
            memset(&m_pUserList[m_dwNumUsers], 0, sizeof(NXC_USER));
            pUser = &m_pUserList[m_dwNumUsers];
            m_dwNumUsers++;
         }
         UpdateUserFromMessage(pMsg, pUser);
         break;
      case USER_DB_DELETE:
         if (pUser != NULL)
            pUser->wFlags |= UF_DELETED;
         break;
      default:
         break;
   }

   if (pUser != NULL)
      callEventHandler(NXC_EVENT_USER_DB_CHANGED, iCode, pUser);
}


//
// Find user in database by ID
//

NXC_USER *NXCL_Session::FindUserById(UINT32 dwId)
{
   UINT32 i;
   NXC_USER *pUser = NULL;

   if (m_dwFlags & NXC_SF_USERDB_LOADED)
   {
      for(i = 0; i < m_dwNumUsers; i++)
         if (m_pUserList[i].dwId == dwId)
         {
            pUser = &m_pUserList[i];
            break;
         }
   }

   return pUser;
}


//
// Get pointer to user list and number of users
//

BOOL NXCL_Session::GetUserDB(NXC_USER **ppUserList, UINT32 *pdwNumUsers)
{
   if (!(m_dwFlags & NXC_SF_USERDB_LOADED))
      return FALSE;

   *ppUserList = m_pUserList;
   *pdwNumUsers = m_dwNumUsers;
   return TRUE;
}


//
// Load user database
// This function is NOT REENTRANT
//

UINT32 NXCL_Session::LoadUserDB(void)
{
   CSCPMessage msg;
   UINT32 dwRetCode, dwRqId;

   dwRqId = CreateRqId();
   PrepareForSync(SYNC_USER_DB);
   destroyUserDB();

   msg.SetCode(CMD_LOAD_USER_DB);
   msg.SetId(dwRqId);
   SendMsg(&msg);

   dwRetCode = WaitForRCC(dwRqId);

   if (dwRetCode == RCC_SUCCESS)
   {
      dwRetCode = WaitForSync(SYNC_USER_DB, INFINITE);
      if (dwRetCode == RCC_SUCCESS)
         m_dwFlags |= NXC_SF_USERDB_LOADED;
   }
   else
   {
      UnlockSyncOp(SYNC_USER_DB);
   }

   return dwRetCode;
}


//
// Send file to server
//

UINT32 NXCL_Session::SendFile(UINT32 dwRqId, TCHAR *pszFileName, long ofset)
{
   return SendFileOverNXCP(m_hSocket, dwRqId, pszFileName, m_pCtx, ofset, NULL, NULL, m_mutexSendMsg) ? RCC_SUCCESS : RCC_IO_ERROR;
}


//
// Set subscription status for given channel(s)
//

UINT32 NXCL_Session::SetSubscriptionStatus(UINT32 dwChannels, int nOperation)
{
   CSCPMessage msg;
   UINT32 dwRqId;

   dwRqId = CreateRqId();

   msg.SetCode(CMD_CHANGE_SUBSCRIPTION);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_FLAGS, dwChannels);
   msg.SetVariable(VID_OPERATION, (WORD)nOperation);
   SendMsg(&msg);

   return WaitForRCC(dwRqId);
}


//
// Prepares file transfer from server to client
//

UINT32 NXCL_Session::PrepareFileTransfer(const TCHAR *pszFile, UINT32 dwRqId)
{
   UINT32 dwResult;

   MutexLock(m_mutexFileRq);
   if (m_hCurrFile == -1)
   {
      m_hCurrFile = _topen(pszFile, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY,
                           S_IRUSR | S_IWUSR);
      dwResult = (m_hCurrFile != -1) ? RCC_SUCCESS : RCC_FILE_IO_ERROR;
      m_dwFileRqId = dwRqId;
      ConditionReset(m_condFileRq);
   }
   else
   {
      dwResult = RCC_TRANSFER_IN_PROGRESS;
   }
   MutexUnlock(m_mutexFileRq);
   return dwResult;
}


//
// Wait for file transfer completion
//

UINT32 NXCL_Session::WaitForFileTransfer(UINT32 dwTimeout)
{
   UINT32 dwResult;
   BOOL bSuccess;

   MutexLock(m_mutexFileRq);
   if (m_hCurrFile != -1)
   {
      MutexUnlock(m_mutexFileRq);
      bSuccess = ConditionWait(m_condFileRq, dwTimeout);
      MutexLock(m_mutexFileRq);
      if (bSuccess)
      {
         dwResult = m_dwFileRqCompletion;
      }
      else
      {
         dwResult = RCC_TIMEOUT;
         if (m_hCurrFile != -1)
            close(m_hCurrFile);
      }
      m_hCurrFile = -1;
   }
   else
   {
      dwResult = RCC_INCOMPATIBLE_OPERATION;
   }
   MutexUnlock(m_mutexFileRq);
   return dwResult;
}

/**
 * Abort file transfer
 */
void NXCL_Session::abortFileTransfer()
{
   MutexLock(m_mutexFileRq);
   if (m_hCurrFile != -1)
   {
      close(m_hCurrFile);
      m_hCurrFile = -1;
      m_dwFileRqId = 0;
   }
   MutexUnlock(m_mutexFileRq);
}

/**
 * Execute simple command (command without arguments and returnning only RCC)
 */
UINT32 NXCL_Session::SimpleCommand(WORD wCmd)
{
   CSCPMessage msg;
   UINT32 dwRqId;

   dwRqId = CreateRqId();

   msg.SetCode(wCmd);
   msg.SetId(dwRqId);
   SendMsg(&msg);

   return WaitForRCC(dwRqId);
}

/**
 * Parse login response message
 */
void NXCL_Session::parseLoginMessage(CSCPMessage *pMsg)
{
   m_dwUserId = pMsg->GetVariableLong(VID_USER_ID);
   m_systemAccess = pMsg->GetVariableInt64(VID_USER_SYS_RIGHTS);
   if (pMsg->GetVariableShort(VID_CHANGE_PASSWD_FLAG))
      m_dwFlags |= NXC_SF_CHANGE_PASSWD;
   if (!pMsg->GetVariableShort(VID_DBCONN_STATUS))
      m_dwFlags |= NXC_SF_BAD_DBCONN;
}

/**
 * Start watchdog thread
 */
void NXCL_Session::StartWatchdogThread()
{
   if (m_hWatchdogThread == INVALID_THREAD_HANDLE)
      m_hWatchdogThread = ThreadCreateEx(NXCL_Session::watchdogThreadStarter, 0, this);
}


//
// Starter function for watchdog thread
//

THREAD_RESULT THREAD_CALL NXCL_Session::watchdogThreadStarter(void *pArg)
{
   ((NXCL_Session *)pArg)->watchdogThread();
   return THREAD_OK;
}


//
// Watchdog thread
//

void NXCL_Session::watchdogThread()
{
   CSCPMessage msg;
   UINT32 dwRqId;
   BOOL bConnBroken = FALSE;

   msg.SetCode(CMD_KEEPALIVE);
   while(1)
   {
      if (ConditionWait(m_condStopThreads, 30000))
         break;   // Need to stop

      // Send keepalive message
      dwRqId = CreateRqId();
      msg.SetId(dwRqId);

      if (SendMsg(&msg))
      {
         if (WaitForRCC(dwRqId) != RCC_SUCCESS)
         {
            bConnBroken = TRUE;
         }
      }
      else
      {
         bConnBroken = TRUE;
      }

      if (bConnBroken)
      {
         m_dwFlags |= NXC_SF_CONN_BROKEN;
         callEventHandler(NXC_EVENT_CONNECTION_BROKEN, 0, NULL);
         break;
      }
   }
}

/**
 * Handler for CMD_NOTIFY message
 */
void NXCL_Session::onNotify(CSCPMessage *pMsg)
{
   UINT32 dwCode;

   dwCode = pMsg->GetVariableLong(VID_NOTIFICATION_CODE);
   if (dwCode == NX_NOTIFY_SHUTDOWN)
   {
      // Stop watchdog and set broken connection flag
      ConditionSet(m_condStopThreads);
      if (m_hWatchdogThread != INVALID_THREAD_HANDLE)
      {
         ThreadJoin(m_hWatchdogThread);
         m_hWatchdogThread = INVALID_THREAD_HANDLE;
      }
      ConditionReset(m_condStopThreads);
      m_dwFlags |= NXC_SF_CONN_BROKEN;
   }
   callEventHandler(NXC_EVENT_NOTIFICATION, dwCode,
                    CAST_TO_POINTER(pMsg->GetVariableLong(VID_NOTIFICATION_DATA), void *));
}
