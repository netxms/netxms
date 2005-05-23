/* 
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2004 Victor Kirhenshtein
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
** $module: session.cpp
**
**/

#include "libnxcl.h"


//
// Session class constructor
//

NXCL_Session::NXCL_Session()
{
   m_dwFlags = 0;
   m_dwMsgId = 0;
   m_dwTimeStamp = 0;
   m_pEventHandler = NULL;
   m_dwState = STATE_DISCONNECTED;
   m_dwCommandTimeout = 10000;    // Default timeout is 10 seconds
   m_dwNumObjects = 0;
   m_pIndexById = NULL;
   m_mutexIndexAccess = MutexCreate();
   m_dwReceiverBufferSize = 4194304;     // 4MB
   m_hSocket = -1;
   m_pItemList = NULL;

   m_ppEventTemplates = NULL;
   m_dwNumTemplates = 0;
   m_mutexEventAccess = MutexCreate();

   m_dwNumUsers = 0;
   m_pUserList = NULL;

   m_hRecvThread = INVALID_THREAD_HANDLE;

#ifdef _WIN32
   m_condSyncOp = CreateEvent(NULL, FALSE, FALSE, NULL);
#else
   pthread_mutex_init(&m_mutexSyncOp, NULL);
   pthread_cond_init(&m_condSyncOp, NULL);
#endif
}


//
// Session class destructor
//

NXCL_Session::~NXCL_Session()
{
   Disconnect();

   // Wait for receiver thread termination
   if (m_hRecvThread != INVALID_THREAD_HANDLE)
      ThreadJoin(m_hRecvThread);

   MutexDestroy(m_mutexIndexAccess);

   MutexLock(m_mutexEventAccess, INFINITE);
   MutexUnlock(m_mutexEventAccess);
   MutexDestroy(m_mutexEventAccess);

#ifdef _WIN32
   CloseHandle(m_condSyncOp);
#else
   pthread_mutex_destroy(&m_mutexSyncOp);
   pthread_cond_destroy(&m_condSyncOp);
#endif
}


//
// Disconnect session
//

void NXCL_Session::Disconnect(void)
{
   // Close socket
   shutdown(m_hSocket, SHUT_RDWR);
   closesocket(m_hSocket);
   m_hSocket = -1;

   // Clear message wait queue
   m_msgWaitQueue.Clear();

   // Cleanup
   DestroyAllObjects();
   DestroyEventDB();
   DestroyUserDB();
}


//
// Destroy all objects
//

void NXCL_Session::DestroyAllObjects(void)
{
   DWORD i;

   MutexLock(m_mutexIndexAccess, INFINITE);
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

CSCPMessage *NXCL_Session::WaitForMessage(WORD wCode, DWORD dwId, DWORD dwTimeOut)
{
   return m_msgWaitQueue.WaitForMessage(wCode, dwId, 
      dwTimeOut == 0 ? m_dwCommandTimeout : dwTimeOut);
}


//
// Wait for specific raw message
//

CSCP_MESSAGE *NXCL_Session::WaitForRawMessage(WORD wCode, DWORD dwId, DWORD dwTimeOut)
{
   return m_msgWaitQueue.WaitForRawMessage(wCode, dwId, 
      dwTimeOut == 0 ? m_dwCommandTimeout : dwTimeOut);
}


//
// Wait for request completion notification and extract result code
// from recived message
//

DWORD NXCL_Session::WaitForRCC(DWORD dwRqId, DWORD dwTimeOut)
{
   CSCPMessage *pResponce;
   DWORD dwRetCode;

   pResponce = WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId, dwTimeOut);
   if (pResponce != NULL)
   {
      dwRetCode = pResponce->GetVariableLong(VID_RCC);
      delete pResponce;
   }
   else
   {
      dwRetCode = RCC_TIMEOUT;
   }
   return dwRetCode;
}


//
// Send CSCP message
//

BOOL NXCL_Session::SendMsg(CSCPMessage *pMsg)
{
   CSCP_MESSAGE *pRawMsg;
   BOOL bResult;
   TCHAR szBuffer[128];

   DebugPrintf(_T("SendMsg(\"%s\"), id:%ld)"), CSCPMessageCodeName(pMsg->GetCode(), szBuffer), pMsg->GetId());
   pRawMsg = pMsg->CreateMessage();
   bResult = (SendEx(m_hSocket, (char *)pRawMsg, ntohl(pRawMsg->dwSize), 0) == (int)ntohl(pRawMsg->dwSize));
   free(pRawMsg);
   return bResult;
}


//
// Wait for synchronization operation completion
//

DWORD NXCL_Session::WaitForSync(DWORD dwTimeOut)
{
#ifdef _WIN32
   DWORD dwRetCode;

   dwRetCode = WaitForSingleObject(m_condSyncOp, dwTimeOut);
   return (dwRetCode == WAIT_TIMEOUT) ? RCC_TIMEOUT : m_dwSyncExitCode;
#else
   int iRetCode;
   DWORD dwResult;

   pthread_mutex_lock(&m_mutexSyncOp);
   if (!(m_dwFlags & NXC_SF_SYNC_FINISHED))
   {
      if (dwTimeOut != INFINITE)
	   {
#if HAVE_PTHREAD_COND_RELTIMEDWAIT_NP
		   struct timespec timeout;

		   timeout.tv_sec = dwTimeOut / 1000;
		   timeout.tv_nsec = (dwTimeOut % 1000) * 1000000;
		   iRetCode = pthread_cond_reltimedwait_np(&m_condSyncOp, &m_mutexSyncOp, &timeout);
#else
		   struct timeval now;
		   struct timespec timeout;

		   gettimeofday(&now, NULL);
		   timeout.tv_sec = now.tv_sec + (dwTimeOut / 1000);
		   timeout.tv_nsec = ( now.tv_usec + ( dwTimeOut % 1000 ) * 1000) * 1000;
		   iRetCode = pthread_cond_timedwait(&m_condSyncOp, &m_mutexSyncOp, &timeout);
#endif
	   }
	   else
      {
         iRetCode = pthread_cond_wait(&m_condSyncOp, &m_mutexSyncOp);
      }
      dwResult = (iRetCode == 0) ? m_dwSyncExitCode : RCC_TIMEOUT;
   }
   else
   {
      dwResult = m_dwSyncExitCode;
   }
   pthread_mutex_unlock(&m_mutexSyncOp);
   return dwResult;
#endif
}


//
// Prepare for synchronization operation
//

void NXCL_Session::PrepareForSync(void)
{
   m_dwSyncExitCode = RCC_SYSTEM_FAILURE;
#ifdef _WIN32
   ResetEvent(m_condSyncOp);
#else
   m_dwFlags &= ~NXC_SF_SYNC_FINISHED;
#endif
}


//
// Complete synchronization operation
//

void NXCL_Session::CompleteSync(DWORD dwRetCode)
{
#ifdef _WIN32
   m_dwSyncExitCode = dwRetCode;
   SetEvent(m_condSyncOp);
#else
   pthread_mutex_lock(&m_mutexSyncOp);
   m_dwSyncExitCode = dwRetCode;
   m_dwFlags |= NXC_SF_SYNC_FINISHED;
   pthread_cond_signal(&m_condSyncOp);
   pthread_mutex_unlock(&m_mutexSyncOp);
#endif
}


//
// Process DCIs coming from server
//

void NXCL_Session::ProcessDCI(CSCPMessage *pMsg)
{
   switch(pMsg->GetCode())
   {
      case CMD_NODE_DCI_LIST_END:
         CompleteSync(RCC_SUCCESS);
         break;
      case CMD_NODE_DCI:
         if (m_pItemList != NULL)
         {
            DWORD i, j, dwId;
            DCI_THRESHOLD dct;

            i = m_pItemList->dwNumItems;
            m_pItemList->dwNumItems++;
            m_pItemList->pItems = (NXC_DCI *)realloc(m_pItemList->pItems, 
                                          sizeof(NXC_DCI) * m_pItemList->dwNumItems);
            m_pItemList->pItems[i].dwId = pMsg->GetVariableLong(VID_DCI_ID);
            m_pItemList->pItems[i].iDataType = (BYTE)pMsg->GetVariableShort(VID_DCI_DATA_TYPE);
            m_pItemList->pItems[i].iPollingInterval = (int)pMsg->GetVariableLong(VID_POLLING_INTERVAL);
            m_pItemList->pItems[i].iRetentionTime = (int)pMsg->GetVariableLong(VID_RETENTION_TIME);
            m_pItemList->pItems[i].iSource = (BYTE)pMsg->GetVariableShort(VID_DCI_SOURCE_TYPE);
            m_pItemList->pItems[i].iStatus = (BYTE)pMsg->GetVariableShort(VID_DCI_STATUS);
            m_pItemList->pItems[i].iDeltaCalculation = (BYTE)pMsg->GetVariableShort(VID_DCI_DELTA_CALCULATION);
            m_pItemList->pItems[i].pszFormula = pMsg->GetVariableStr(VID_DCI_FORMULA);
            pMsg->GetVariableStr(VID_NAME, m_pItemList->pItems[i].szName, MAX_ITEM_NAME);
            pMsg->GetVariableStr(VID_DESCRIPTION, m_pItemList->pItems[i].szDescription,
                                 MAX_DB_STRING);
            pMsg->GetVariableStr(VID_INSTANCE, m_pItemList->pItems[i].szInstance,
                                 MAX_DB_STRING);
            m_pItemList->pItems[i].dwNumThresholds = pMsg->GetVariableLong(VID_NUM_THRESHOLDS);
            m_pItemList->pItems[i].pThresholdList = 
               (NXC_DCI_THRESHOLD *)malloc(sizeof(NXC_DCI_THRESHOLD) * m_pItemList->pItems[i].dwNumThresholds);
            for(j = 0, dwId = VID_DCI_THRESHOLD_BASE; j < m_pItemList->pItems[i].dwNumThresholds; j++, dwId++)
            {
               pMsg->GetVariableBinary(dwId, (BYTE *)&dct, sizeof(DCI_THRESHOLD));
               m_pItemList->pItems[i].pThresholdList[j].dwId = ntohl(dct.dwId);
               m_pItemList->pItems[i].pThresholdList[j].dwEvent = ntohl(dct.dwEvent);
               m_pItemList->pItems[i].pThresholdList[j].dwArg1 = ntohl(dct.dwArg1);
               m_pItemList->pItems[i].pThresholdList[j].dwArg2 = ntohl(dct.dwArg2);
               m_pItemList->pItems[i].pThresholdList[j].wFunction = ntohs(dct.wFunction);
               m_pItemList->pItems[i].pThresholdList[j].wOperation = ntohs(dct.wOperation);
               switch(m_pItemList->pItems[i].iDataType)
               {
                  case DCI_DT_INT:
                  case DCI_DT_UINT:
                     m_pItemList->pItems[i].pThresholdList[j].value.dwInt32 = ntohl(dct.value.dwInt32);
                     break;
                  case DCI_DT_INT64:
                  case DCI_DT_UINT64:
                     m_pItemList->pItems[i].pThresholdList[j].value.qwInt64 = ntohq(dct.value.qwInt64);
                     break;
                  case DCI_DT_FLOAT:
                     m_pItemList->pItems[i].pThresholdList[j].value.dFloat = ntohd(dct.value.dFloat);
                     break;
                  case DCI_DT_STRING:
                     _tcscpy(m_pItemList->pItems[i].pThresholdList[j].value.szString, dct.value.szString);
                     break;
                  default:
                     break;
               }
            }
         }
         break;
      default:
         break;
   }
}


//
// Load data collection items list for specified node
// This function is NOT REENTRANT
//

DWORD NXCL_Session::OpenNodeDCIList(DWORD dwNodeId, NXC_DCI_LIST **ppItemList)
{
   CSCPMessage msg;
   DWORD dwRetCode, dwRqId;

   dwRqId = CreateRqId();
   PrepareForSync();

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
      dwRetCode = WaitForSync(INFINITE);
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
      free(m_pItemList);
   }

   m_pItemList = NULL;
   return dwRetCode;
}


//
// Load event configuration database
//

DWORD NXCL_Session::LoadEventDB(void)
{
   CSCPMessage msg;
   DWORD dwRetCode, dwRqId;

   dwRqId = CreateRqId();
   PrepareForSync();

   DestroyEventDB();
   MutexLock(m_mutexEventAccess, INFINITE);

   msg.SetCode(CMD_LOAD_EVENT_DB);
   msg.SetId(dwRqId);
   SendMsg(&msg);

   dwRetCode = WaitForRCC(dwRqId);

   /* TODO: this probably should be recoded as loop with calls to WaitForMessage() */
   if (dwRetCode == RCC_SUCCESS)
      dwRetCode = WaitForSync(INFINITE);

   MutexUnlock(m_mutexEventAccess);
   return dwRetCode;
}


//
// Destroy event template database
//

void NXCL_Session::DestroyEventDB(void)
{
   DWORD i;

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

void NXCL_Session::DeleteEDBRecord(DWORD dwEventCode)
{
   DWORD i;

   MutexLock(m_mutexEventAccess, INFINITE);
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
      MutexLock(m_mutexEventAccess, INFINITE);
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

BOOL NXCL_Session::GetEventDB(NXC_EVENT_TEMPLATE ***pppTemplateList, DWORD *pdwNumRecords)
{
   *pppTemplateList = m_ppEventTemplates;
   *pdwNumRecords = m_dwNumTemplates;
   return TRUE;
}


//
// Resolve event id to name
//

const TCHAR *NXCL_Session::GetEventName(DWORD dwId)
{
   DWORD i;

   MutexLock(m_mutexEventAccess, INFINITE);
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

BOOL NXCL_Session::GetEventNameEx(DWORD dwId, TCHAR *pszBuffer, DWORD dwBufSize)
{
   DWORD i;

   MutexLock(m_mutexEventAccess, INFINITE);
   for(i = 0; i < m_dwNumTemplates; i++)
      if (m_ppEventTemplates[i]->dwCode == dwId)
      {
         _tcsncpy(pszBuffer, m_ppEventTemplates[i]->szName, dwBufSize);
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

int NXCL_Session::GetEventSeverity(DWORD dwId)
{
   DWORD i;

   MutexLock(m_mutexEventAccess, INFINITE);
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

BOOL NXCL_Session::GetEventText(DWORD dwId, TCHAR *pszBuffer, DWORD dwBufSize)
{
   DWORD i;

   MutexLock(m_mutexEventAccess, INFINITE);
   for(i = 0; i < m_dwNumTemplates; i++)
      if (m_ppEventTemplates[i]->dwCode == dwId)
      {
         _tcsncpy(pszBuffer, m_ppEventTemplates[i]->pszMessage, dwBufSize);
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

void NXCL_Session::DestroyUserDB(void)
{
   DWORD i;

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

void NXCL_Session::ProcessUserDBRecord(CSCPMessage *pMsg)
{
   switch(pMsg->GetCode())
   {
      case CMD_USER_DB_EOF:
         CompleteSync(RCC_SUCCESS);
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

void NXCL_Session::ProcessUserDBUpdate(CSCPMessage *pMsg)
{
   int iCode;
   DWORD dwUserId;
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
      CallEventHandler(NXC_EVENT_USER_DB_CHANGED, iCode, pUser);
}


//
// Find user in database by ID
//

NXC_USER *NXCL_Session::FindUserById(DWORD dwId)
{
   DWORD i;
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

BOOL NXCL_Session::GetUserDB(NXC_USER **ppUserList, DWORD *pdwNumUsers)
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

DWORD NXCL_Session::LoadUserDB(void)
{
   CSCPMessage msg;
   DWORD dwRetCode, dwRqId;

   dwRqId = CreateRqId();
   PrepareForSync();
   DestroyUserDB();

   msg.SetCode(CMD_LOAD_USER_DB);
   msg.SetId(dwRqId);
   SendMsg(&msg);

   dwRetCode = WaitForRCC(dwRqId);

   if (dwRetCode == RCC_SUCCESS)
   {
      dwRetCode = WaitForSync(INFINITE);
      if (dwRetCode == RCC_SUCCESS)
         m_dwFlags |= NXC_SF_USERDB_LOADED;
   }

   return dwRetCode;
}


//
// Send file to server
//

DWORD NXCL_Session::SendFile(DWORD dwRqId, TCHAR *pszFileName)
{
   return SendFileOverCSCP(m_hSocket, dwRqId, pszFileName) ? RCC_SUCCESS : RCC_IO_ERROR;
}
