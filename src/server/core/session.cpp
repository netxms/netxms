/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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

#include "nms_core.h"

#ifdef _WIN32
# include <direct.h>
#else
# include <dirent.h>
#endif


//
// Fill CSCP message with user data
//

static void FillUserInfoMessage(CSCPMessage *pMsg, NMS_USER *pUser)
{
   pMsg->SetVariable(VID_USER_ID, pUser->dwId);
   pMsg->SetVariable(VID_USER_NAME, pUser->szName);
   pMsg->SetVariable(VID_USER_FLAGS, pUser->wFlags);
   pMsg->SetVariable(VID_USER_SYS_RIGHTS, pUser->wSystemRights);
   pMsg->SetVariable(VID_USER_FULL_NAME, pUser->szFullName);
   pMsg->SetVariable(VID_USER_DESCRIPTION, pUser->szDescription);
}


//
// Fill CSCP message with user group data
//

static void FillGroupInfoMessage(CSCPMessage *pMsg, NMS_USER_GROUP *pGroup)
{
   DWORD i, dwId;

   pMsg->SetVariable(VID_USER_ID, pGroup->dwId);
   pMsg->SetVariable(VID_USER_NAME, pGroup->szName);
   pMsg->SetVariable(VID_USER_FLAGS, pGroup->wFlags);
   pMsg->SetVariable(VID_USER_SYS_RIGHTS, pGroup->wSystemRights);
   pMsg->SetVariable(VID_USER_DESCRIPTION, pGroup->szDescription);
   pMsg->SetVariable(VID_NUM_MEMBERS, pGroup->dwNumMembers);
   for(i = 0, dwId = VID_GROUP_MEMBER_BASE; i < pGroup->dwNumMembers; i++, dwId++)
      pMsg->SetVariable(dwId, pGroup->pMembers[i]);
}


//
// Client session class constructor
//

ClientSession::ClientSession(SOCKET hSocket, DWORD dwHostAddr)
{
   m_pSendQueue = new Queue;
   m_pMessageQueue = new Queue;
   m_pUpdateQueue = new Queue;
   m_hSocket = hSocket;
   m_dwIndex = INVALID_INDEX;
   m_iState = STATE_CONNECTED;
   m_pMsgBuffer = (CSCP_BUFFER *)malloc(sizeof(CSCP_BUFFER));
   m_mutexWriteThreadRunning = MutexCreate();
   m_mutexProcessingThreadRunning = MutexCreate();
   m_mutexUpdateThreadRunning = MutexCreate();
   m_mutexSendEvents = MutexCreate();
   m_mutexSendObjects = MutexCreate();
   m_mutexSendAlarms = MutexCreate();
   m_mutexSendActions = MutexCreate();
   m_dwFlags = 0;
   m_dwHostAddr = dwHostAddr;
   strcpy(m_szUserName, "<not logged in>");
   m_dwUserId = INVALID_INDEX;
   m_dwOpenDCIListSize = 0;
   m_pOpenDCIList = NULL;
   m_ppEPPRuleList = NULL;
}


//
// Destructor
//

ClientSession::~ClientSession()
{
   shutdown(m_hSocket, 2);
   closesocket(m_hSocket);
   delete m_pSendQueue;
   delete m_pMessageQueue;
   delete m_pUpdateQueue;
   safe_free(m_pMsgBuffer);
   MutexDestroy(m_mutexWriteThreadRunning);
   MutexDestroy(m_mutexProcessingThreadRunning);
   MutexDestroy(m_mutexUpdateThreadRunning);
   MutexDestroy(m_mutexSendEvents);
   MutexDestroy(m_mutexSendObjects);
   MutexDestroy(m_mutexSendAlarms);
   MutexDestroy(m_mutexSendActions);
   safe_free(m_pOpenDCIList);
   if (m_ppEPPRuleList != NULL)
   {
      DWORD i;

      if (m_dwFlags & CSF_EPP_UPLOAD)  // Aborted in the middle of EPP transfer
         for(i = 0; i < m_dwRecordsUploaded; i++)
            delete m_ppEPPRuleList[i];
      free(m_ppEPPRuleList);
   }
}


//
// Print debug information
//

void ClientSession::DebugPrintf(char *szFormat, ...)
{
   if ((g_dwFlags & AF_STANDALONE) && (g_dwFlags & AF_DEBUG_CSCP))
   {
      va_list args;

      printf("*CSCP(%d)* ", m_dwIndex);
      va_start(args, szFormat);
      vprintf(szFormat, args);
      va_end(args);
   }
}


//
// ReadThread()
//

void ClientSession::ReadThread(void)
{
   CSCP_MESSAGE *pRawMsg;
   CSCPMessage *pMsg;
   int iErr;
   DWORD i;
   NetObj *pObject;

   // Initialize raw message receiving function
   RecvCSCPMessage(0, NULL, m_pMsgBuffer, 0);

   pRawMsg = (CSCP_MESSAGE *)malloc(65536);
   while(1)
   {
      if ((iErr = RecvCSCPMessage(m_hSocket, pRawMsg, m_pMsgBuffer, 65536)) <= 0)
         break;

      // Check if message is too large
      if (iErr == 1)
         continue;

      // Check that actual received packet size is equal to encoded in packet
      if ((int)ntohl(pRawMsg->dwSize) != iErr)
      {
         DebugPrintf("Actual message size doesn't match wSize value (%d,%d)\n", iErr, ntohl(pRawMsg->dwSize));
         continue;   // Bad packet, wait for next
      }

      // Create message object from raw message
      pMsg = new CSCPMessage(pRawMsg);
      m_pMessageQueue->Put(pMsg);
   }
   if (iErr < 0)
      WriteLog(MSG_SESSION_CLOSED, EVENTLOG_WARNING_TYPE, "e", WSAGetLastError());
   free(pRawMsg);

   // Notify other threads to exit
   m_pSendQueue->Clear();
   m_pSendQueue->Put(INVALID_POINTER_VALUE);
   m_pMessageQueue->Clear();
   m_pMessageQueue->Put(INVALID_POINTER_VALUE);
   m_pUpdateQueue->Clear();
   m_pUpdateQueue->Put(INVALID_POINTER_VALUE);

   // Wait for other threads to finish
   MutexLock(m_mutexWriteThreadRunning, INFINITE);
   MutexUnlock(m_mutexWriteThreadRunning);

   MutexLock(m_mutexProcessingThreadRunning, INFINITE);
   MutexUnlock(m_mutexProcessingThreadRunning);

   MutexLock(m_mutexUpdateThreadRunning, INFINITE);
   MutexUnlock(m_mutexUpdateThreadRunning);

   // Remove all locks created by this session
   RemoveAllSessionLocks(m_dwIndex);
   for(i = 0; i < m_dwOpenDCIListSize; i++)
   {
      pObject = FindObjectById(m_pOpenDCIList[i]);
      if (pObject != NULL)
         if (pObject->Type() == OBJECT_NODE)
            ((Node *)pObject)->UnlockDCIList(m_dwIndex);
   }

   DebugPrintf("Session closed\n");
}


//
// WriteThread()
//

void ClientSession::WriteThread(void)
{
   CSCP_MESSAGE *pMsg;
   char szBuffer[128];

   MutexLock(m_mutexWriteThreadRunning, INFINITE);
   while(1)
   {
      pMsg = (CSCP_MESSAGE *)m_pSendQueue->GetOrBlock();
      if (pMsg == INVALID_POINTER_VALUE)    // Session termination indicator
         break;

      DebugPrintf("Sending message %s\n", CSCPMessageCodeName(ntohs(pMsg->wCode), szBuffer));
      if (send(m_hSocket, (const char *)pMsg, ntohl(pMsg->dwSize), 0) <= 0)
      {
         safe_free(pMsg);
         break;
      }
      safe_free(pMsg);
   }
   MutexUnlock(m_mutexWriteThreadRunning);
}


//
// Update processing thread
//

void ClientSession::UpdateThread(void)
{
   UPDATE_INFO *pUpdate;
   CSCPMessage msg;

   MutexLock(m_mutexUpdateThreadRunning, INFINITE);
   while(1)
   {
      pUpdate = (UPDATE_INFO *)m_pUpdateQueue->GetOrBlock();
      if (pUpdate == INVALID_POINTER_VALUE)    // Session termination indicator
         break;

      switch(pUpdate->dwCategory)
      {
         case INFO_CAT_EVENT:
            MutexLock(m_mutexSendEvents, INFINITE);
            m_pSendQueue->Put(CreateRawCSCPMessage(CMD_EVENT, 0, sizeof(NXC_EVENT), pUpdate->pData, NULL));
            MutexUnlock(m_mutexSendEvents);
            free(pUpdate->pData);
            break;
         case INFO_CAT_OBJECT_CHANGE:
            MutexLock(m_mutexSendObjects, INFINITE);
            msg.SetCode(CMD_OBJECT_UPDATE);
            msg.SetId(0);
            ((NetObj *)pUpdate->pData)->CreateMessage(&msg);
            SendMessage(&msg);
            MutexUnlock(m_mutexSendObjects);
            msg.DeleteAllVariables();
            ((NetObj *)pUpdate->pData)->DecRefCount();
            break;
         case INFO_CAT_ALARM:
            MutexLock(m_mutexSendAlarms, INFINITE);
            msg.SetCode(CMD_ALARM_UPDATE);
            msg.SetId(0);
            msg.SetVariable(VID_NOTIFICATION_CODE, pUpdate->dwCode);
            msg.SetVariable(VID_ALARM_ID, ((NXC_ALARM *)pUpdate->pData)->dwAlarmId);
            if (pUpdate->dwCode == NX_NOTIFY_NEW_ALARM)
               FillAlarmInfoMessage(&msg, (NXC_ALARM *)pUpdate->pData);
            SendMessage(&msg);
            MutexUnlock(m_mutexSendAlarms);
            msg.DeleteAllVariables();
            free(pUpdate->pData);
            break;
         case INFO_CAT_ACTION:
            MutexLock(m_mutexSendActions, INFINITE);
            msg.SetCode(CMD_ACTION_DB_UPDATE);
            msg.SetId(0);
            msg.SetVariable(VID_NOTIFICATION_CODE, pUpdate->dwCode);
            msg.SetVariable(VID_ACTION_ID, ((NXC_ACTION *)pUpdate->pData)->dwId);
            if (pUpdate->dwCode != NX_NOTIFY_ACTION_DELETED)
               FillActionInfoMessage(&msg, (NXC_ACTION *)pUpdate->pData);
            SendMessage(&msg);
            MutexUnlock(m_mutexSendActions);
            msg.DeleteAllVariables();
            free(pUpdate->pData);
            break;
         default:
            break;
      }

      free(pUpdate);
   }
   MutexUnlock(m_mutexUpdateThreadRunning);
}


//
// Message processing thread
//

void ClientSession::ProcessingThread(void)
{
   CSCPMessage *pMsg;
   char szBuffer[128];

   MutexLock(m_mutexProcessingThreadRunning, INFINITE);
   while(1)
   {
      pMsg = (CSCPMessage *)m_pMessageQueue->GetOrBlock();
      if (pMsg == INVALID_POINTER_VALUE)    // Session termination indicator
         break;

      DebugPrintf("Received message %s\n", CSCPMessageCodeName(pMsg->GetCode(), szBuffer));
      if ((m_iState != STATE_AUTHENTICATED) && (pMsg->GetCode() != CMD_LOGIN))
      {
         delete pMsg;
         continue;
      }

      switch(pMsg->GetCode())
      {
         case CMD_LOGIN:
            Login(pMsg);
            break;
         case CMD_GET_OBJECTS:
            SendAllObjects(pMsg->GetId());
            break;
         case CMD_GET_EVENTS:
            SendAllEvents(pMsg->GetId());
            break;
         case CMD_GET_CONFIG_VARLIST:
            SendAllConfigVars();
            break;
         case CMD_OPEN_EVENT_DB:
            SendEventDB(pMsg->GetId());
            break;
         case CMD_CLOSE_EVENT_DB:
            CloseEventDB(pMsg->GetId());
            break;
         case CMD_SET_EVENT_INFO:
            SetEventInfo(pMsg);
            break;
         case CMD_MODIFY_OBJECT:
            ModifyObject(pMsg);
            break;
         case CMD_SET_OBJECT_MGMT_STATUS:
            ChangeObjectMgmtStatus(pMsg);
            break;
         case CMD_LOAD_USER_DB:
            SendUserDB(pMsg->GetId());
            break;
         case CMD_CREATE_USER:
            CreateUser(pMsg);
            break;
         case CMD_UPDATE_USER:
            UpdateUser(pMsg);
            break;
         case CMD_DELETE_USER:
            DeleteUser(pMsg);
            break;
         case CMD_LOCK_USER_DB:
            LockUserDB(pMsg->GetId(), TRUE);
            break;
         case CMD_UNLOCK_USER_DB:
            LockUserDB(pMsg->GetId(), FALSE);
            break;
         case CMD_SET_PASSWORD:
            SetPassword(pMsg);
            break;
         case CMD_GET_NODE_DCI_LIST:
            OpenNodeDCIList(pMsg);
            break;
         case CMD_UNLOCK_NODE_DCI_LIST:
            CloseNodeDCIList(pMsg);
            break;
         case CMD_CREATE_NEW_DCI:
         case CMD_MODIFY_NODE_DCI:
         case CMD_DELETE_NODE_DCI:
            ModifyNodeDCI(pMsg);
            break;
         case CMD_GET_DCI_DATA:
            GetCollectedData(pMsg);
            break;
         case CMD_OPEN_EPP:
            OpenEPP(pMsg->GetId());
            break;
         case CMD_CLOSE_EPP:
            CloseEPP(pMsg->GetId());
            break;
         case CMD_SAVE_EPP:
            SaveEPP(pMsg);
            break;
         case CMD_EPP_RECORD:
            ProcessEPPRecord(pMsg);
            break;
         case CMD_GET_MIB_LIST:
            SendMIBList(pMsg->GetId());
            break;
         case CMD_GET_MIB:
            SendMIB(pMsg);
            break;
         case CMD_CREATE_OBJECT:
            CreateObject(pMsg);
            break;
         case CMD_BIND_OBJECT:
            ChangeObjectBinding(pMsg, TRUE);
            break;
         case CMD_UNBIND_OBJECT:
            ChangeObjectBinding(pMsg, FALSE);
            break;
         case CMD_GET_EVENT_NAMES:
            SendEventNames(pMsg->GetId());
            break;
         case CMD_GET_IMAGE_LIST:
            SendImageCatalogue(this, pMsg->GetId(), pMsg->GetVariableShort(VID_IMAGE_FORMAT));
            break;
         case CMD_LOAD_IMAGE_FILE:
            SendImageFile(this, pMsg->GetId(), pMsg->GetVariableLong(VID_IMAGE_ID),
                          pMsg->GetVariableShort(VID_IMAGE_FORMAT));
            break;
         case CMD_GET_DEFAULT_IMAGE_LIST:
            SendDefaultImageList(this, pMsg->GetId());
            break;
         case CMD_GET_ALL_ALARMS:
            SendAllAlarms(pMsg->GetId(), pMsg->GetVariableShort(VID_IS_ACK));
            break;
         case CMD_GET_ALARM:
            break;
         case CMD_ACK_ALARM:
            AcknowlegeAlarm(pMsg);
            break;
         case CMD_DELETE_ALARM:
            break;
         case CMD_LOCK_ACTION_DB:
            LockActionDB(pMsg->GetId(), TRUE);
            break;
         case CMD_UNLOCK_ACTION_DB:
            LockActionDB(pMsg->GetId(), FALSE);
            break;
         case CMD_CREATE_ACTION:
            CreateAction(pMsg);
            break;
         case CMD_MODIFY_ACTION:
            UpdateAction(pMsg);
            break;
         case CMD_DELETE_ACTION:
            DeleteAction(pMsg);
            break;
         case CMD_LOAD_ACTIONS:
            SendAllActions(pMsg->GetId());
            break;
         case CMD_GET_CONTAINER_CAT_LIST:
            SendContainerCategories(pMsg->GetId());
            break;
         default:
            break;
      }
      delete pMsg;
   }
   MutexUnlock(m_mutexProcessingThreadRunning);
}


//
// Authenticate client
//

void ClientSession::Login(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   BYTE szPassword[SHA1_DIGEST_SIZE];
   char szLogin[MAX_USER_NAME], szBuffer[32];

   // Prepare responce message
   msg.SetCode(CMD_LOGIN_RESP);
   msg.SetId(pRequest->GetId());

   if (m_iState != STATE_AUTHENTICATED)
   {
      
      pRequest->GetVariableStr(VID_LOGIN_NAME, szLogin, MAX_USER_NAME);
      pRequest->GetVariableBinary(VID_PASSWORD, szPassword, SHA1_DIGEST_SIZE);

      if (AuthenticateUser(szLogin, szPassword, &m_dwUserId, &m_dwSystemAccess))
      {
         m_iState = STATE_AUTHENTICATED;
         sprintf(m_szUserName, "%s@%s", szLogin, IpToStr(m_dwHostAddr, szBuffer));
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
         DebugPrintf("User %s authenticated\n", m_szUserName);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
   }

   // Send responce
   SendMessage(&msg);
}


//
// Send event configuration to client
//

void ClientSession::SendEventDB(DWORD dwRqId)
{
   DB_ASYNC_RESULT hResult;
   CSCPMessage msg;
   char szBuffer[1024];

   // Prepare responce message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);

   if (!CheckSysAccessRights(SYSTEM_ACCESS_VIEW_EVENT_DB))
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      SendMessage(&msg);
   }
   else if (!LockComponent(CID_EVENT_DB, m_dwIndex, m_szUserName, NULL, szBuffer))
   {
      msg.SetVariable(VID_RCC, RCC_COMPONENT_LOCKED);
      msg.SetVariable(VID_LOCKED_BY, szBuffer);
      SendMessage(&msg);
   }
   else
   {
      m_dwFlags |= CSF_EVENT_DB_LOCKED;
      m_dwFlags &= ~CSF_EVENT_DB_MODIFIED;

      msg.SetVariable(VID_RCC, RCC_SUCCESS);
      SendMessage(&msg);
      msg.DeleteAllVariables();

      // Prepare data message
      msg.SetCode(CMD_EVENT_DB_RECORD);
      msg.SetId(dwRqId);

      hResult = DBAsyncSelect(g_hCoreDB, "SELECT event_id,name,severity,flags,message,description FROM events");
      if (hResult != NULL)
      {
         while(DBFetch(hResult))
         {
            msg.SetVariable(VID_EVENT_ID, DBGetFieldAsyncULong(hResult, 0));
            msg.SetVariable(VID_NAME, DBGetFieldAsync(hResult, 1, szBuffer, 1024));
            msg.SetVariable(VID_SEVERITY, DBGetFieldAsyncULong(hResult, 2));
            msg.SetVariable(VID_FLAGS, DBGetFieldAsyncULong(hResult, 3));

            DBGetFieldAsync(hResult, 4, szBuffer, 1024);
            DecodeSQLString(szBuffer);
            msg.SetVariable(VID_MESSAGE, szBuffer);

            DBGetFieldAsync(hResult, 5, szBuffer, 1024);
            DecodeSQLString(szBuffer);
            msg.SetVariable(VID_DESCRIPTION, szBuffer);

            SendMessage(&msg);
            msg.DeleteAllVariables();
         }
         DBFreeAsyncResult(hResult);
      }

      // Send end-of-list indicator
      msg.SetCode(CMD_EVENT_DB_EOF);
      SendMessage(&msg);
   }
}


//
// Close event configuration database
//

void ClientSession::CloseEventDB(DWORD dwRqId)
{
   CSCPMessage msg;

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);

   if (m_dwFlags & CSF_EVENT_DB_LOCKED)
   {
      // Check if event configuration DB has been modified
      if (m_dwFlags & CSF_EVENT_DB_MODIFIED)
      {
         ReloadEvents();

         // Notify clients on event database change
         EnumerateClientSessions(NotifyClient, (void *)NX_NOTIFY_EVENTDB_CHANGED);
      }
      UnlockComponent(CID_EVENT_DB);
      m_dwFlags &= ~CSF_EVENT_DB_LOCKED;
      msg.SetVariable(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
   }
   SendMessage(&msg);
}


//
// Send all objects to client
//

void ClientSession::SendAllObjects(DWORD dwRqId)
{
   DWORD i;
   CSCPMessage msg;

   // Send confirmation message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_RCC, RCC_SUCCESS);
   SendMessage(&msg);
   msg.DeleteAllVariables();

   MutexLock(m_mutexSendObjects, INFINITE);

   // Prepare message
   msg.SetCode(CMD_OBJECT);

   // Send objects, one per message
   ObjectsGlobalLock();
   for(i = 0; i < g_dwIdIndexSize; i++)
      if (g_pIndexById[i].pObject->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         g_pIndexById[i].pObject->CreateMessage(&msg);
         SendMessage(&msg);
         msg.DeleteAllVariables();
      }
   ObjectsGlobalUnlock();

   // Send end of list notification
   msg.SetCode(CMD_OBJECT_LIST_END);
   SendMessage(&msg);

   MutexUnlock(m_mutexSendObjects);
}


//
// Send all events to client
//

void ClientSession::SendAllEvents(DWORD dwRqId)
{
   CSCPMessage msg;
   DB_ASYNC_RESULT hResult;
   NXC_EVENT event;

   // Send confirmation message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_RCC, RCC_SUCCESS);
   SendMessage(&msg);
   msg.DeleteAllVariables();

   MutexLock(m_mutexSendEvents, INFINITE);

   // Retrieve events from database
   hResult = DBAsyncSelect(g_hCoreDB, "SELECT event_id,event_timestamp,event_source,"
                                      "event_severity,event_message FROM event_log "
                                      "ORDER BY event_timestamp");
   if (hResult != NULL)
   {
      // Send events, one per message
      while(DBFetch(hResult))
      {
         event.dwEventId = htonl(DBGetFieldAsyncULong(hResult, 0));
         event.dwTimeStamp = htonl(DBGetFieldAsyncULong(hResult, 1));
         event.dwSourceId = htonl(DBGetFieldAsyncULong(hResult, 2));
         event.dwSeverity = htonl(DBGetFieldAsyncULong(hResult, 3));
         DBGetFieldAsync(hResult, 4, event.szMessage, MAX_EVENT_MSG_LENGTH);
         DecodeSQLString(event.szMessage);
         m_pSendQueue->Put(CreateRawCSCPMessage(CMD_EVENT, dwRqId, sizeof(NXC_EVENT), &event, NULL));
      }
      DBFreeAsyncResult(hResult);
   }

   // Send end of list notification
   msg.SetCode(CMD_EVENT_LIST_END);
   SendMessage(&msg);

   MutexUnlock(m_mutexSendEvents);
}


//
// Send all configuration variables to client
//

void ClientSession::SendAllConfigVars(void)
{
   DWORD i, dwNumRecords;
   CSCPMessage msg;
   DB_RESULT hResult;

   // Check user rights
   if ((m_dwUserId != 0) && ((m_dwSystemAccess & SYSTEM_ACCESS_VIEW_CONFIG) == 0))
   {
      // Access denied
      msg.SetCode(CMD_CONFIG_VARLIST_END);
      msg.SetVariable(VID_ERROR, (DWORD)1);
      SendMessage(&msg);
   }
   else
   {
      // Prepare message
      msg.SetCode(CMD_CONFIG_VARIABLE);

      // Retrieve configuration variables from database
      hResult = DBSelect(g_hCoreDB, "SELECT var_name,var_value FROM config");
      if (hResult != NULL)
      {
         // Send events, one per message
         dwNumRecords = DBGetNumRows(hResult);
         for(i = 0; i < dwNumRecords; i++)
         {
            msg.SetVariable(VID_NAME, DBGetField(hResult, i, 0));
            msg.SetVariable(VID_VALUE, DBGetField(hResult, i, 1));
            SendMessage(&msg);
            msg.DeleteAllVariables();
         }
         DBFreeResult(hResult);
      }

      // Send end of list notification
      msg.SetCode(CMD_CONFIG_VARLIST_END);
      msg.SetVariable(VID_ERROR, (DWORD)0);
      SendMessage(&msg);
   }
}


//
// Close session forcibly
//

void ClientSession::Kill(void)
{
   // We shutdown socket connection, which will cause
   // read thread to stop, and other threads will follow
   shutdown(m_hSocket, 2);
}


//
// Handler for new events
//

void ClientSession::OnNewEvent(Event *pEvent)
{
   UPDATE_INFO *pUpdate;

   if (m_iState == STATE_AUTHENTICATED)
   {
      pUpdate = (UPDATE_INFO *)malloc(sizeof(UPDATE_INFO));
      pUpdate->dwCategory = INFO_CAT_EVENT;
      pUpdate->pData = malloc(sizeof(NXC_EVENT));
      pEvent->PrepareMessage((NXC_EVENT *)pUpdate->pData);
      m_pUpdateQueue->Put(pUpdate);
   }
}


//
// Handler for object changes
//

void ClientSession::OnObjectChange(NetObj *pObject)
{
   UPDATE_INFO *pUpdate;

   if (m_iState == STATE_AUTHENTICATED)
      if (pObject->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         pUpdate = (UPDATE_INFO *)malloc(sizeof(UPDATE_INFO));
         pUpdate->dwCategory = INFO_CAT_OBJECT_CHANGE;
         pUpdate->pData = pObject;
         m_pUpdateQueue->Put(pUpdate);
         pObject->IncRefCount();
      }
}


//
// Send notification message to server
//

void ClientSession::Notify(DWORD dwCode, DWORD dwData)
{
   CSCPMessage msg;

   msg.SetCode(CMD_NOTIFY);
   msg.SetVariable(VID_NOTIFICATION_CODE, dwCode);
   msg.SetVariable(VID_NOTIFICATION_DATA, dwData);
   SendMessage(&msg);
}


//
// Update event template
//

void ClientSession::SetEventInfo(CSCPMessage *pRequest)
{
   CSCPMessage msg;

   // Prepare reply message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Check if we have event configuration database opened
   if (!(m_dwFlags & CSF_EVENT_DB_LOCKED))
   {
      msg.SetVariable(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
   }
   else
   {
      // Check access rights
      if (CheckSysAccessRights(SYSTEM_ACCESS_EDIT_EVENT_DB))
      {
         char szQuery[4096], szName[MAX_EVENT_NAME];
         DWORD dwEventId;
         BOOL bEventExist = FALSE;
         DB_RESULT hResult;

         // Check if event with specific id exists
         dwEventId = pRequest->GetVariableLong(VID_EVENT_ID);
         sprintf(szQuery, "SELECT event_id FROM events WHERE event_id=%ld", dwEventId);
         hResult = DBSelect(g_hCoreDB, szQuery);
         if (hResult != NULL)
         {
            if (DBGetNumRows(hResult) > 0)
               bEventExist = TRUE;
            DBFreeResult(hResult);
         }

         // Prepare and execute SQL query
         pRequest->GetVariableStr(VID_NAME, szName, MAX_EVENT_NAME);
         if (IsValidObjectName(szName))
         {
            char szMessage[MAX_DB_STRING], *pszDescription, *pszEscMsg, *pszEscDescr;

            pRequest->GetVariableStr(VID_MESSAGE, szMessage, MAX_DB_STRING);
            pszEscMsg = EncodeSQLString(szMessage);

            pszDescription = pRequest->GetVariableStr(VID_DESCRIPTION);
            pszEscDescr = EncodeSQLString(pszDescription);
            safe_free(pszDescription);

            if (bEventExist)
            {
               sprintf(szQuery, "UPDATE events SET name='%s',severity=%ld,flags=%ld,message='%s',description='%s' WHERE event_id=%ld",
                       szName, pRequest->GetVariableLong(VID_SEVERITY), pRequest->GetVariableLong(VID_FLAGS),
                       pszEscMsg, pszEscDescr, dwEventId);
            }
            else
            {
               sprintf(szQuery, "INSERT INTO events SET event_id,name,severity,flags,message,description VALUES (%ld,'%s',%ld,%ld,'%s','%s')",
                       dwEventId, szName, pRequest->GetVariableLong(VID_SEVERITY),
                       pRequest->GetVariableLong(VID_FLAGS), pszEscMsg, pszEscDescr);
            }

            free(pszEscMsg);
            free(pszEscDescr);

            if (DBQuery(g_hCoreDB, szQuery))
            {
               msg.SetVariable(VID_RCC, RCC_SUCCESS);
               m_dwFlags |= CSF_EVENT_DB_MODIFIED;
            }
            else
            {
               msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
            }
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_NAME);
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      }
   }

   // Send responce
   SendMessage(&msg);
}


//
// Modify object
//

void ClientSession::ModifyObject(CSCPMessage *pRequest)
{
   DWORD dwObjectId, dwResult = RCC_SUCCESS;
   NetObj *pObject;
   CSCPMessage msg;

   // Prepare reply message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   dwObjectId = pRequest->GetVariableLong(VID_OBJECT_ID);
   pObject = FindObjectById(dwObjectId);
   if (pObject != NULL)
   {
      if (pObject->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
      {
         // If user attempts to change object's ACL, check
         // if he has OBJECT_ACCESS_CONTROL permission
         if (pRequest->IsVariableExist(VID_ACL_SIZE))
            if (!pObject->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_CONTROL))
               dwResult = RCC_ACCESS_DENIED;

         // If allowed, change object and set completion code
         if (dwResult != RCC_ACCESS_DENIED)
            dwResult = pObject->ModifyFromMessage(pRequest);
         msg.SetVariable(VID_RCC, dwResult);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send responce
   SendMessage(&msg);
}


//
// Send users database to client
//

void ClientSession::SendUserDB(DWORD dwRqId)
{
   CSCPMessage msg;
   DWORD i;

   // Prepare responce message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_RCC, RCC_SUCCESS);
   SendMessage(&msg);

   // Send users
   msg.SetCode(CMD_USER_DATA);
   for(i = 0; i < g_dwNumUsers; i++)
   {
      FillUserInfoMessage(&msg, &g_pUserList[i]);
      SendMessage(&msg);
      msg.DeleteAllVariables();
   }

   // Send groups
   msg.SetCode(CMD_GROUP_DATA);
   for(i = 0; i < g_dwNumGroups; i++)
   {
      FillGroupInfoMessage(&msg, &g_pGroupList[i]);
      SendMessage(&msg);
      msg.DeleteAllVariables();
   }

   // Send end-of-database notification
   msg.SetCode(CMD_USER_DB_EOF);
   SendMessage(&msg);
}


//
// Create new user
//

void ClientSession::CreateUser(CSCPMessage *pRequest)
{
   CSCPMessage msg;

   // Prepare responce message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Check user rights
   if (!(m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_USERS))
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }
   else if (!(m_dwFlags & CSF_USER_DB_LOCKED))
   {
      // User database have to be locked before any
      // changes to user database can be made
      msg.SetVariable(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
   }
   else
   {
      DWORD dwResult, dwUserId;
      BOOL bIsGroup;
      char szUserName[MAX_USER_NAME];

      pRequest->GetVariableStr(VID_USER_NAME, szUserName, MAX_USER_NAME);
      if (IsValidObjectName(szUserName))
      {
         bIsGroup = pRequest->GetVariableShort(VID_IS_GROUP);
         dwResult = CreateNewUser(szUserName, bIsGroup, &dwUserId);
         msg.SetVariable(VID_RCC, dwResult);
         if (dwResult == RCC_SUCCESS)
            msg.SetVariable(VID_USER_ID, dwUserId);   // Send id of new user to client
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_NAME);
      }
   }

   // Send responce
   SendMessage(&msg);
}


//
// Update existing user's data
//

void ClientSession::UpdateUser(CSCPMessage *pRequest)
{
   CSCPMessage msg;

   // Prepare responce message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Check user rights
   if (!(m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_USERS))
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }
   else if (!(m_dwFlags & CSF_USER_DB_LOCKED))
   {
      // User database have to be locked before any
      // changes to user database can be made
      msg.SetVariable(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
   }
   else
   {
      DWORD dwUserId, dwResult;

      dwUserId = pRequest->GetVariableLong(VID_USER_ID);
      if (dwUserId & GROUP_FLAG)
      {
         NMS_USER_GROUP group;
         DWORD i, dwId;

         group.dwId = dwUserId;
         pRequest->GetVariableStr(VID_USER_DESCRIPTION, group.szDescription, MAX_USER_DESCR);
         pRequest->GetVariableStr(VID_USER_NAME, group.szName, MAX_USER_NAME);
         group.wFlags = pRequest->GetVariableShort(VID_USER_FLAGS);
         group.wSystemRights = pRequest->GetVariableShort(VID_USER_SYS_RIGHTS);
         group.dwNumMembers = pRequest->GetVariableLong(VID_NUM_MEMBERS);
         group.pMembers = (DWORD *)malloc(sizeof(DWORD) * group.dwNumMembers);
         for(i = 0, dwId = VID_GROUP_MEMBER_BASE; i < group.dwNumMembers; i++, dwId++)
            group.pMembers[i] = pRequest->GetVariableLong(dwId);
         dwResult = ModifyGroup(&group);
         safe_free(group.pMembers);
      }
      else
      {
         NMS_USER user;

         user.dwId = dwUserId;
         pRequest->GetVariableStr(VID_USER_DESCRIPTION, user.szDescription, MAX_USER_DESCR);
         pRequest->GetVariableStr(VID_USER_FULL_NAME, user.szFullName, MAX_USER_FULLNAME);
         pRequest->GetVariableStr(VID_USER_NAME, user.szName, MAX_USER_NAME);
         user.wFlags = pRequest->GetVariableShort(VID_USER_FLAGS);
         user.wSystemRights = pRequest->GetVariableShort(VID_USER_SYS_RIGHTS);
         dwResult = ModifyUser(&user);
      }
      msg.SetVariable(VID_RCC, dwResult);
   }

   // Send responce
   SendMessage(&msg);
}


//
// Delete user
//

void ClientSession::DeleteUser(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   DWORD dwUserId;

   // Prepare responce message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Check user rights
   if (!(m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_USERS))
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }
   else if (!(m_dwFlags & CSF_USER_DB_LOCKED))
   {
      // User database have to be locked before any
      // changes to user database can be made
      msg.SetVariable(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
   }
   else
   {
      // Get Id of user to be deleted
      dwUserId = pRequest->GetVariableLong(VID_USER_ID);

      if (dwUserId != 0)
      {
         DWORD dwResult;

         dwResult = DeleteUserFromDB(dwUserId);
         msg.SetVariable(VID_RCC, dwResult);
      }
      else
      {
         // Nobody can delete system administrator account
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      }
   }

   // Send responce
   SendMessage(&msg);
}


//
// Lock/unlock user database
//

void ClientSession::LockUserDB(DWORD dwRqId, BOOL bLock)
{
   CSCPMessage msg;
   char szBuffer[256];

   // Prepare responce message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);

   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_USERS)
   {
      if (bLock)
      {
         if (!LockComponent(CID_USER_DB, m_dwIndex, m_szUserName, NULL, szBuffer))
         {
            msg.SetVariable(VID_RCC, RCC_COMPONENT_LOCKED);
            msg.SetVariable(VID_LOCKED_BY, szBuffer);
         }
         else
         {
            m_dwFlags |= CSF_USER_DB_LOCKED;
            msg.SetVariable(VID_RCC, RCC_SUCCESS);
         }
      }
      else
      {
         if (m_dwFlags & CSF_USER_DB_LOCKED)
         {
            UnlockComponent(CID_USER_DB);
            m_dwFlags &= ~CSF_USER_DB_LOCKED;
         }
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
      }
   }
   else
   {
      // Current user has no rights for user account management
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send responce
   SendMessage(&msg);
}


//
// Notify client on user database update
//

void ClientSession::OnUserDBUpdate(int iCode, DWORD dwUserId, NMS_USER *pUser, NMS_USER_GROUP *pGroup)
{
   CSCPMessage msg;

   if (m_iState == STATE_AUTHENTICATED)
   {
      msg.SetCode(CMD_USER_DB_UPDATE);
      msg.SetId(0);
      msg.SetVariable(VID_UPDATE_TYPE, (WORD)iCode);

      switch(iCode)
      {
         case USER_DB_CREATE:
            msg.SetVariable(VID_USER_ID, dwUserId);
            if (dwUserId & GROUP_FLAG)
               msg.SetVariable(VID_USER_NAME, pGroup->szName);
            else
               msg.SetVariable(VID_USER_NAME, pUser->szName);
            break;
         case USER_DB_MODIFY:
            if (dwUserId & GROUP_FLAG)
               FillGroupInfoMessage(&msg, pGroup);
            else
               FillUserInfoMessage(&msg, pUser);
            break;
         default:
            msg.SetVariable(VID_USER_ID, dwUserId);
            break;
      }

      SendMessage(&msg);
   }
}


//
// Change management status for the object
//

void ClientSession::ChangeObjectMgmtStatus(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   DWORD dwObjectId;
   NetObj *pObject;

   // Prepare responce message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get object id and check access rights
   dwObjectId = pRequest->GetVariableLong(VID_OBJECT_ID);
   pObject = FindObjectById(dwObjectId);
   if (pObject != NULL)
   {
      if (pObject->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
      {
         BOOL bIsManaged;

         bIsManaged = (BOOL)pRequest->GetVariableShort(VID_MGMT_STATUS);
         pObject->SetMgmtStatus(bIsManaged);
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send responce
   SendMessage(&msg);
}


// 
// Set user's password
//

void ClientSession::SetPassword(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   DWORD dwUserId;

   // Prepare responce message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   dwUserId = pRequest->GetVariableLong(VID_USER_ID);

   if (((m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_USERS) &&
        !((dwUserId == 0) && (m_dwUserId != 0))) ||   // Only administrator can change password for UID 0
       (dwUserId == m_dwUserId))     // User can change password for itself
   {
      DWORD dwResult;
      BYTE szPassword[SHA1_DIGEST_SIZE];

      pRequest->GetVariableBinary(VID_PASSWORD, szPassword, SHA1_DIGEST_SIZE);
      dwResult = SetUserPassword(dwUserId, szPassword);
      msg.SetVariable(VID_RCC, dwResult);
   }
   else
   {
      // Current user has no rights to change password for specific user
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send responce
   SendMessage(&msg);
}


//
// Send node's DCIs to client and lock data collection settings
//

void ClientSession::OpenNodeDCIList(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   DWORD dwObjectId;
   NetObj *pObject;
   BOOL bSuccess = FALSE;

   // Prepare responce message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get node id and check object class and access rights
   dwObjectId = pRequest->GetVariableLong(VID_OBJECT_ID);
   pObject = FindObjectById(dwObjectId);
   if (pObject != NULL)
   {
      if (pObject->Type() == OBJECT_NODE)
      {
         if (pObject->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         {
            // Try to lock DCI list
            bSuccess = ((Node *)pObject)->LockDCIList(m_dwIndex);
            msg.SetVariable(VID_RCC, bSuccess ? RCC_SUCCESS : RCC_COMPONENT_LOCKED);

            // Modify list of open nodes DCI lists
            if (bSuccess)
            {
               m_pOpenDCIList = (DWORD *)realloc(m_pOpenDCIList, sizeof(DWORD) * (m_dwOpenDCIListSize + 1));
               m_pOpenDCIList[m_dwOpenDCIListSize] = dwObjectId;
               m_dwOpenDCIListSize++;
            }
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send responce
   SendMessage(&msg);

   // If DCI list was successfully locked, send it to client
   if (bSuccess)
      ((Node *)pObject)->SendItemsToClient(this, pRequest->GetId());
}


//
// Unlock node's data collection settings
//

void ClientSession::CloseNodeDCIList(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   DWORD dwObjectId;
   NetObj *pObject;

   // Prepare responce message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get node id and check object class and access rights
   dwObjectId = pRequest->GetVariableLong(VID_OBJECT_ID);
   pObject = FindObjectById(dwObjectId);
   if (pObject != NULL)
   {
      if (pObject->Type() == OBJECT_NODE)
      {
         if (pObject->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         {
            BOOL bSuccess;

            // Try to unlock DCI list
            bSuccess = ((Node *)pObject)->UnlockDCIList(m_dwIndex);
            msg.SetVariable(VID_RCC, bSuccess ? RCC_SUCCESS : RCC_OUT_OF_STATE_REQUEST);

            // Modify list of open nodes DCI lists
            if (bSuccess)
            {
               DWORD i;

               for(i = 0; i < m_dwOpenDCIListSize; i++)
                  if (m_pOpenDCIList[i] == dwObjectId)
                  {
                     m_dwOpenDCIListSize--;
                     memmove(&m_pOpenDCIList[i], &m_pOpenDCIList[i + 1],
                             sizeof(DWORD) * (m_dwOpenDCIListSize - i));
                     break;
                  }
            }
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send responce
   SendMessage(&msg);
}


//
// Create, modify, or delete data collection item for node
//

void ClientSession::ModifyNodeDCI(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   DWORD dwObjectId;
   NetObj *pObject;

   // Prepare responce message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get node id and check object class and access rights
   dwObjectId = pRequest->GetVariableLong(VID_OBJECT_ID);
   pObject = FindObjectById(dwObjectId);
   if (pObject != NULL)
   {
      if (pObject->Type() == OBJECT_NODE)
      {
         if (((Node *)pObject)->IsLockedBySession(m_dwIndex))
         {
            if (pObject->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
            {
               DWORD i, dwItemId, dwNumMaps, *pdwMapId, *pdwMapIndex;
               DCItem *pItem;
               BOOL bSuccess;

               switch(pRequest->GetCode())
               {
                  case CMD_CREATE_NEW_DCI:
                     // Create dummy DCI
                     pItem = new DCItem(CreateUniqueId(IDG_ITEM), "no name", DS_INTERNAL, 
                                        DCI_DT_INTEGER, 60, 30, (Node *)pObject);
                     pItem->SetStatus(ITEM_STATUS_DISABLED);
                     if (((Node *)pObject)->AddItem(pItem))
                     {
                        msg.SetVariable(VID_RCC, RCC_SUCCESS);
                        // Return new item id to client
                        msg.SetVariable(VID_DCI_ID, pItem->Id());
                     }
                     else  // Unable to add item to node
                     {
                        delete pItem;
                        msg.SetVariable(VID_RCC, RCC_DUPLICATE_DCI);
                     }
                     break;
                  case CMD_MODIFY_NODE_DCI:
                     dwItemId = pRequest->GetVariableLong(VID_DCI_ID);
                     bSuccess = ((Node *)pObject)->UpdateItem(dwItemId, pRequest, &dwNumMaps,
                                                              &pdwMapIndex, &pdwMapId);
                     if (bSuccess)
                     {
                        msg.SetVariable(VID_RCC, RCC_SUCCESS);

                        // Send index to id mapping for newly created thresholds to client
                        msg.SetVariable(VID_DCI_NUM_MAPS, dwNumMaps);
                        for(i = 0; i < dwNumMaps; i++)
                        {
                           pdwMapId[i] = htonl(pdwMapId[i]);
                           pdwMapIndex[i] = htonl(pdwMapIndex[i]);
                        }
                        msg.SetVariable(VID_DCI_MAP_IDS, (BYTE *)pdwMapId, sizeof(DWORD) * dwNumMaps);
                        msg.SetVariable(VID_DCI_MAP_INDEXES, (BYTE *)pdwMapIndex, sizeof(DWORD) * dwNumMaps);
                        safe_free(pdwMapId);
                        safe_free(pdwMapIndex);
                     }
                     else
                     {
                        msg.SetVariable(VID_RCC, RCC_INVALID_DCI_ID);
                     }
                     break;
                  case CMD_DELETE_NODE_DCI:
                     dwItemId = pRequest->GetVariableLong(VID_DCI_ID);
                     bSuccess = ((Node *)pObject)->DeleteItem(dwItemId);
                     msg.SetVariable(VID_RCC, bSuccess ? RCC_SUCCESS : RCC_INVALID_DCI_ID);
                     break;
               }
            }
            else  // User doesn't have MODIFY rights on object
            {
               msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
            }
         }
         else  // Nodes DCI list not locked by this session
         {
            msg.SetVariable(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
         }
      }
      else     // Object is not a node
      {
         msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
      }
   }
   else  // No object with given ID
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send responce
   SendMessage(&msg);
}


//
// Get collected data
//

void ClientSession::GetCollectedData(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   DWORD dwObjectId;
   NetObj *pObject;
   BOOL bSuccess = FALSE;
   static DWORD m_dwRowSize[] = { 8, 12, 260, 12 };

   // Prepare responce message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get node id and check object class and access rights
   dwObjectId = pRequest->GetVariableLong(VID_OBJECT_ID);
   pObject = FindObjectById(dwObjectId);
   if (pObject != NULL)
   {
      if (pObject->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         DB_ASYNC_RESULT hResult;
         DWORD dwItemId, dwMaxRows, dwTimeFrom, dwTimeTo;
         DWORD dwAllocatedRows = 100, dwNumRows = 0;
         char szQuery[512], szCond[256];
         int iPos = 0, iType;
         DCI_DATA_HEADER *pData = NULL;
         DCI_DATA_ROW *pCurr;

         // Send CMD_REQUEST_COMPLETED message
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
         SendMessage(&msg);

         // Get request parameters
         dwItemId = pRequest->GetVariableLong(VID_DCI_ID);
         dwMaxRows = pRequest->GetVariableLong(VID_MAX_ROWS);
         dwTimeFrom = pRequest->GetVariableLong(VID_TIME_FROM);
         dwTimeTo = pRequest->GetVariableLong(VID_TIME_TO);

         szCond[0] = 0;
         if (dwTimeFrom != 0)
         {
            sprintf(szCond, " AND idata_timestamp>=%d", dwTimeFrom);
            iPos = strlen(szCond);
         }
         if (dwTimeTo != 0)
         {
            sprintf(&szCond[iPos], " AND idata_timestamp<=%d", dwTimeTo);
         }

         sprintf(szQuery, "SELECT idata_timestamp,idata_value FROM idata_%d WHERE item_id=%d%s ORDER BY idata_timestamp DESC",
                 dwObjectId, dwItemId, szCond);
         hResult = DBAsyncSelect(g_hCoreDB, szQuery);
         if (hResult != NULL)
         {
            // Get item's data type to determine actual row size
            iType = ((Node *)pObject)->GetItemType(dwItemId);

            // Allocate initial memory block and prepare data header
            pData = (DCI_DATA_HEADER *)malloc(dwAllocatedRows * m_dwRowSize[iType] + sizeof(DCI_DATA_HEADER));
            pData->dwDataType = htonl((DWORD)iType);
            pData->dwItemId = htonl(dwItemId);

            // Fill memory block with records
            pCurr = (DCI_DATA_ROW *)(((char *)pData) + sizeof(DCI_DATA_HEADER));
            while(DBFetch(hResult))
            {
               if ((dwMaxRows > 0) && (dwNumRows >= dwMaxRows))
                  break;

               // Extend buffer if we are at the end
               if (dwNumRows == dwAllocatedRows)
               {
                  dwAllocatedRows += 50;
                  pData = (DCI_DATA_HEADER *)realloc(pData, 
                     dwAllocatedRows * m_dwRowSize[iType] + sizeof(DCI_DATA_HEADER));
                  pCurr = (DCI_DATA_ROW *)(((char *)pData) + sizeof(DCI_DATA_HEADER) + m_dwRowSize[iType] * dwNumRows);
               }

               dwNumRows++;

               pCurr->dwTimeStamp = htonl(DBGetFieldAsyncULong(hResult, 0));
               switch(iType)
               {
                  case DCI_DT_INTEGER:
                     pCurr->value.dwInteger = htonl(DBGetFieldAsyncULong(hResult, 1));
                     break;
                  case DCI_DT_FLOAT:
                     pCurr->value.dFloat = htond(DBGetFieldAsyncDouble(hResult, 1));
                     break;
                  case DCI_DT_STRING:
                     DBGetFieldAsync(hResult, 1, pCurr->value.szString, MAX_DCI_STRING_VALUE);
                     break;
               }
               pCurr = (DCI_DATA_ROW *)(((char *)pCurr) + m_dwRowSize[iType]);
            }
            DBFreeAsyncResult(hResult);
            pData->dwNumRows = htonl(dwNumRows);

            // Prepare and send raw message with fetched data
            m_pSendQueue->Put(
               CreateRawCSCPMessage(CMD_DCI_DATA, pRequest->GetId(), 
                                    dwNumRows * m_dwRowSize[iType] + sizeof(DCI_DATA_HEADER),
                                    pData, NULL));
            free(pData);
            bSuccess = TRUE;
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else  // No object with given ID
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send responce
   if (!bSuccess)
      SendMessage(&msg);
}


//
// Open event processing policy
//

void ClientSession::OpenEPP(DWORD dwRqId)
{
   CSCPMessage msg;
   char szBuffer[256];
   BOOL bSuccess = FALSE;

   // Prepare responce message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);

   if (m_dwSystemAccess & SYSTEM_ACCESS_EPP)
   {
      if (!LockComponent(CID_EPP, m_dwIndex, m_szUserName, NULL, szBuffer))
      {
         msg.SetVariable(VID_RCC, RCC_COMPONENT_LOCKED);
         msg.SetVariable(VID_LOCKED_BY, szBuffer);
      }
      else
      {
         m_dwFlags |= CSF_EPP_LOCKED;
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
         msg.SetVariable(VID_NUM_RULES, g_pEventPolicy->NumRules());
         bSuccess = TRUE;
      }
   }
   else
   {
      // Current user has no rights for event policy management
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send responce
   SendMessage(&msg);

   // Send policy to client
   if (bSuccess)
      g_pEventPolicy->SendToClient(this, dwRqId);
}


//
// Close event processing policy
//

void ClientSession::CloseEPP(DWORD dwRqId)
{
   CSCPMessage msg;

   // Prepare responce message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);

   if (m_dwSystemAccess & SYSTEM_ACCESS_EPP)
   {
      if (m_dwFlags & CSF_EPP_LOCKED)
      {
         UnlockComponent(CID_EPP);
         m_dwFlags &= ~CSF_EPP_LOCKED;
      }
      msg.SetVariable(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      // Current user has no rights for event policy management
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send responce
   SendMessage(&msg);
}


//
// Save event processing policy
//

void ClientSession::SaveEPP(CSCPMessage *pRequest)
{
   CSCPMessage msg;

   // Prepare responce message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   if (m_dwSystemAccess & SYSTEM_ACCESS_EPP)
   {
      if (m_dwFlags & CSF_EPP_LOCKED)
      {
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
         m_dwNumRecordsToUpload = pRequest->GetVariableLong(VID_NUM_RULES);
         m_dwRecordsUploaded = 0;
         if (m_dwNumRecordsToUpload == 0)
         {
            g_pEventPolicy->ReplacePolicy(0, NULL);
            g_pEventPolicy->SaveToDB();
         }
         else
         {
            m_dwFlags |= CSF_EPP_UPLOAD;
            m_ppEPPRuleList = (EPRule **)malloc(sizeof(EPRule *) * m_dwNumRecordsToUpload);
            memset(m_ppEPPRuleList, 0, sizeof(EPRule *) * m_dwNumRecordsToUpload);
         }
         DebugPrintf("Accepted EPP upload request for %d rules\n", m_dwNumRecordsToUpload);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
      }
   }
   else
   {
      // Current user has no rights for event policy management
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send responce
   SendMessage(&msg);
}


//
// Process EPP rule received from client
//

void ClientSession::ProcessEPPRecord(CSCPMessage *pRequest)
{
   if (!(m_dwFlags & CSF_EPP_LOCKED))
   {
      CSCPMessage msg;

      msg.SetCode(CMD_REQUEST_COMPLETED);
      msg.SetId(pRequest->GetId());
      msg.SetVariable(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
      SendMessage(&msg);
   }
   else
   {
      if (m_dwRecordsUploaded < m_dwNumRecordsToUpload)
      {
         m_ppEPPRuleList[m_dwRecordsUploaded] = new EPRule(pRequest);
         m_dwRecordsUploaded++;
         if (m_dwRecordsUploaded == m_dwNumRecordsToUpload)
         {
            CSCPMessage msg;

            // All records received, replace event policy...
            DebugPrintf("Replacing event processing policy with a new one at %p (%d rules)\n",
                        m_ppEPPRuleList, m_dwNumRecordsToUpload);
            g_pEventPolicy->ReplacePolicy(m_dwNumRecordsToUpload, m_ppEPPRuleList);
            g_pEventPolicy->SaveToDB();
            m_ppEPPRuleList = NULL;
            
            // ... and send final confirmation
            msg.SetCode(CMD_REQUEST_COMPLETED);
            msg.SetId(pRequest->GetId());
            msg.SetVariable(VID_RCC, RCC_SUCCESS);
            SendMessage(&msg);

            m_dwFlags &= ~CSF_EPP_UPLOAD;
         }
      }
   }
}


//
// Send list of available MIB files to client
//

void ClientSession::SendMIBList(DWORD dwRqId)
{
   CSCPMessage msg;
   DWORD dwId1, dwId2, dwNumFiles;
   DIR *dir;
   int iBufPos;
#ifdef _WIN32
   struct direct *dptr;
#else
   struct dirent *dptr;
#endif
   char szBuffer[MAX_PATH];
   BYTE md5Hash[MD5_DIGEST_SIZE];

   // Prepare responce message
   msg.SetCode(CMD_MIB_LIST);
   msg.SetId(dwRqId);

   // Read directory
   dwNumFiles = 0;
   strcpy(szBuffer, g_szDataDir);
   strcat(szBuffer, DDIR_MIBS);
   dir = opendir(szBuffer);
   if (dir != NULL)
   {
#ifdef _WIN32
      strcat(szBuffer, "\\");
#else
      strcat(szBuffer, "/");
#endif
      iBufPos = strlen(szBuffer);
      dwId1 = VID_MIB_NAME_BASE;
      dwId2 = VID_MIB_HASH_BASE;
      while((dptr = readdir(dir)) != NULL)
      {
         strcpy(&szBuffer[iBufPos], dptr->d_name);
         if (CalculateFileMD5Hash(szBuffer, md5Hash))
         {
            msg.SetVariable(dwId1++, dptr->d_name);
            msg.SetVariable(dwId2++, md5Hash, MD5_DIGEST_SIZE);
            dwNumFiles++;
         }
      }
      closedir(dir);
   }

   msg.SetVariable(VID_NUM_MIBS, dwNumFiles);

   // Send responce
   SendMessage(&msg);
}


//
// Send requested MIB file to client
//

void ClientSession::SendMIB(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   BYTE *pFile;
   DWORD dwFileSize;
   char szBuffer[MAX_PATH], szMIB[MAX_PATH];

   // Prepare responce message
   msg.SetCode(CMD_MIB);
   msg.SetId(pRequest->GetId());

   // Get name of the requested file
   pRequest->GetVariableStr(VID_MIB_NAME, szMIB, MAX_PATH);

   // Load file into memory
   strcpy(szBuffer, g_szDataDir);
   strcat(szBuffer, DDIR_MIBS);
#ifdef _WIN32
   strcat(szBuffer, "\\");
#else
   strcat(szBuffer, "/");
#endif
   strcat(szBuffer, szMIB);
   pFile = LoadFile(szBuffer, &dwFileSize);
   if (pFile != NULL)
   {
      msg.SetVariable(VID_RCC, RCC_SUCCESS);
      msg.SetVariable(VID_MIB_FILE_SIZE, dwFileSize);
      msg.SetVariable(VID_MIB_FILE, pFile, dwFileSize);
      free(pFile);
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_SYSTEM_FAILURE);
   }

   // Send responce
   SendMessage(&msg);
}


//
// Send list of event name/identifier pairs
//

void ClientSession::SendEventNames(DWORD dwRqId)
{
   CSCPMessage msg;
   DB_RESULT hResult;

   msg.SetCode(CMD_EVENT_NAME_LIST);
   msg.SetId(dwRqId);
   hResult = DBSelect(g_hCoreDB, "SELECT event_id,name,severity FROM events");
   if (hResult != NULL)
   {
      DWORD i, dwNumEvents;
      NXC_EVENT_NAME *pList;

      dwNumEvents = DBGetNumRows(hResult);
      msg.SetVariable(VID_NUM_EVENTS, dwNumEvents);
      if (dwNumEvents > 0)
      {
         pList = (NXC_EVENT_NAME *)malloc(sizeof(NXC_EVENT_NAME) * dwNumEvents);
         for(i = 0; i < dwNumEvents; i++)
         {
            pList[i].dwEventId = htonl(DBGetFieldULong(hResult, i, 0));
            pList[i].dwSeverity = htonl(DBGetFieldLong(hResult, i, 2));
            strcpy(pList[i].szName, DBGetField(hResult, i, 1));
         }
         msg.SetVariable(VID_EVENT_NAME_TABLE, (BYTE *)pList, sizeof(NXC_EVENT_NAME) * dwNumEvents);
         free(pList);
      }
      DBFreeResult(hResult);
      msg.SetVariable(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
   }
   SendMessage(&msg);
}


//
// Create new object
//

void ClientSession::CreateObject(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   NetObj *pObject, *pParent;
   int iClass;
   char *pDescription, szObjectName[MAX_OBJECT_NAME];

   // Prepare responce message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Find parent object
   pParent = FindObjectById(pRequest->GetVariableLong(VID_PARENT_ID));
   if (pParent != NULL)
   {
      // User should have create access to parent object
      if (pParent->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_CREATE))
      {
         // Parent object should be container or service root
         if ((pParent->Type() == OBJECT_CONTAINER) ||
             (pParent->Type() == OBJECT_SERVICEROOT))
         {
            iClass = pRequest->GetVariableShort(VID_OBJECT_CLASS);
            pRequest->GetVariableStr(VID_OBJECT_NAME, szObjectName, MAX_OBJECT_NAME);
            if (IsValidObjectName(szObjectName))
            {
               if ((iClass == OBJECT_NODE) || (iClass == OBJECT_CONTAINER))
               {
                  // Create new object
                  switch(iClass)
                  {
                     case OBJECT_NODE:
                        pObject = PollNewNode(pRequest->GetVariableLong(VID_IP_ADDRESS),
                                              pRequest->GetVariableLong(VID_IP_NETMASK),
                                              DF_DEFAULT);
                        break;
                     case OBJECT_CONTAINER:
                        pDescription = pRequest->GetVariableStr(VID_DESCRIPTION);
                        pObject = new Container(szObjectName, 
                                                pRequest->GetVariableLong(VID_CATEGORY),
                                                pDescription);
                        safe_free(pDescription);
                        NetObjInsert(pObject, TRUE);
                        break;
                  }

                  // If creation was successful do binding
                  if (pObject != NULL)
                  {
                     pParent->AddChild(pObject);
                     pObject->AddParent(pParent);
                     msg.SetVariable(VID_RCC, RCC_SUCCESS);
                     msg.SetVariable(VID_OBJECT_ID, pObject->Id());
                  }
                  else
                  {
                     msg.SetVariable(VID_RCC, RCC_OBJECT_CREATION_FAILED);
                  }
               }
               else
               {
                  msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
               }
            }
            else
            {
               msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_NAME);
            }
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send responce
   SendMessage(&msg);
}


//
// Bind/unbind object
//

void ClientSession::ChangeObjectBinding(CSCPMessage *pRequest, BOOL bBind)
{
   CSCPMessage msg;
   NetObj *pParent, *pChild;

   // Prepare responce message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get parent and child objects
   pParent = FindObjectById(pRequest->GetVariableLong(VID_PARENT_ID));
   pChild = FindObjectById(pRequest->GetVariableLong(VID_CHILD_ID));

   // Check access rights and change binding
   if ((pParent != NULL) && (pChild != NULL))
   {
      // User should have modify access to both objects
      if ((pParent->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY)) &&
          (pChild->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY)))
      {
         // Parent object should be container or service root
         if ((pParent->Type() == OBJECT_CONTAINER) ||
             (pParent->Type() == OBJECT_SERVICEROOT))
         {
            if (bBind)
            {
               // Prevent loops
               if (!pChild->IsChild(pParent->Id()))
               {
                  pParent->AddChild(pChild);
                  pChild->AddParent(pParent);
                  msg.SetVariable(VID_RCC, RCC_SUCCESS);
               }
               else
               {
                  msg.SetVariable(VID_RCC, RCC_OBJECT_LOOP);
               }
            }
            else
            {
               pParent->DeleteChild(pChild);
               pChild->DeleteParent(pParent);
               msg.SetVariable(VID_RCC, RCC_SUCCESS);
            }
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send responce
   SendMessage(&msg);
}


//
// Process changes in alarms
//

void ClientSession::OnAlarmUpdate(DWORD dwCode, NXC_ALARM *pAlarm)
{
   UPDATE_INFO *pUpdate;
   NetObj *pObject;

   if (m_iState == STATE_AUTHENTICATED)
   {
      pObject = FindObjectById(pAlarm->dwSourceObject);
      if (pObject != NULL)
         if (pObject->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_READ_ALARMS))
         {
            pUpdate = (UPDATE_INFO *)malloc(sizeof(UPDATE_INFO));
            pUpdate->dwCategory = INFO_CAT_ALARM;
            pUpdate->dwCode = dwCode;
            pUpdate->pData = nx_memdup(pAlarm, sizeof(NXC_ALARM));
            m_pUpdateQueue->Put(pUpdate);
         }
   }
}


//
// Send all alarms to client
//

void ClientSession::SendAllAlarms(DWORD dwRqId, BOOL bIncludeAck)
{
   MutexLock(m_mutexSendAlarms, INFINITE);
   g_alarmMgr.SendAlarmsToClient(dwRqId, bIncludeAck, this);
   MutexUnlock(m_mutexSendAlarms);
}


//
// Acknowlege alarm
//

void ClientSession::AcknowlegeAlarm(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   NetObj *pObject;
   DWORD dwAlarmId;

   // Prepare responce message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get alarm id and it's source object
   dwAlarmId = pRequest->GetVariableLong(VID_ALARM_ID);
   pObject = g_alarmMgr.GetAlarmSourceObject(dwAlarmId);
   if (pObject != NULL)
   {
      // User should have "acknowlege alarm" right to the object
      if (pObject->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_ACK_ALARMS))
      {
         g_alarmMgr.AckById(dwAlarmId, m_dwUserId);
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      // Normally, for existing alarms pObject will not be NULL,
      // so we assume that alarm id is invalid
      msg.SetVariable(VID_RCC, RCC_INVALID_ALARM_ID);
   }

   // Send responce
   SendMessage(&msg);
}


//
// Lock/unlock action configuration database
//

void ClientSession::LockActionDB(DWORD dwRqId, BOOL bLock)
{
   CSCPMessage msg;
   char szBuffer[256];

   // Prepare responce message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);

   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_ACTIONS)
   {
      if (bLock)
      {
         if (!LockComponent(CID_ACTION_DB, m_dwIndex, m_szUserName, NULL, szBuffer))
         {
            msg.SetVariable(VID_RCC, RCC_COMPONENT_LOCKED);
            msg.SetVariable(VID_LOCKED_BY, szBuffer);
         }
         else
         {
            m_dwFlags |= CSF_ACTION_DB_LOCKED;
            msg.SetVariable(VID_RCC, RCC_SUCCESS);
         }
      }
      else
      {
         if (m_dwFlags & CSF_ACTION_DB_LOCKED)
         {
            UnlockComponent(CID_ACTION_DB);
            m_dwFlags &= ~CSF_ACTION_DB_LOCKED;
         }
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
      }
   }
   else
   {
      // Current user has no rights for action management
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send responce
   SendMessage(&msg);
}


//
// Create new action
//

void ClientSession::CreateAction(CSCPMessage *pRequest)
{
   CSCPMessage msg;

   // Prepare responce message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Check user rights
   if (!(m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_ACTIONS))
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }
   else if (!(m_dwFlags & CSF_ACTION_DB_LOCKED))
   {
      // Action database have to be locked before any
      // changes can be made
      msg.SetVariable(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
   }
   else
   {
      DWORD dwResult, dwActionId;
      char szActionName[MAX_USER_NAME];

      pRequest->GetVariableStr(VID_ACTION_NAME, szActionName, MAX_OBJECT_NAME);
      if (IsValidObjectName(szActionName))
      {
         dwResult = CreateNewAction(szActionName, &dwActionId);
         msg.SetVariable(VID_RCC, dwResult);
         if (dwResult == RCC_SUCCESS)
            msg.SetVariable(VID_ACTION_ID, dwActionId);   // Send id of new action to client
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_NAME);
      }
   }

   // Send responce
   SendMessage(&msg);
}


//
// Update existing action's data
//

void ClientSession::UpdateAction(CSCPMessage *pRequest)
{
   CSCPMessage msg;

   // Prepare responce message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Check user rights
   if (!(m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_ACTIONS))
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }
   else if (!(m_dwFlags & CSF_ACTION_DB_LOCKED))
   {
      // Action database have to be locked before any
      // changes can be made
      msg.SetVariable(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
   }
   else
   {
      msg.SetVariable(VID_RCC, ModifyActionFromMessage(pRequest));
   }

   // Send responce
   SendMessage(&msg);
}


//
// Delete action
//

void ClientSession::DeleteAction(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   DWORD dwActionId;

   // Prepare responce message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Check user rights
   if (!(m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_ACTIONS))
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }
   else if (!(m_dwFlags & CSF_ACTION_DB_LOCKED))
   {
      // Action database have to be locked before any
      // changes can be made
      msg.SetVariable(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
   }
   else
   {
      // Get Id of action to be deleted
      dwActionId = pRequest->GetVariableLong(VID_ACTION_ID);
      msg.SetVariable(VID_RCC, DeleteActionFromDB(dwActionId));
   }

   // Send responce
   SendMessage(&msg);
}


//
// Process changes in actions
//

void ClientSession::OnActionDBUpdate(DWORD dwCode, NXC_ACTION *pAction)
{
   UPDATE_INFO *pUpdate;

   if (m_iState == STATE_AUTHENTICATED)
   {
      if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_ACTIONS)
      {
         pUpdate = (UPDATE_INFO *)malloc(sizeof(UPDATE_INFO));
         pUpdate->dwCategory = INFO_CAT_ACTION;
         pUpdate->dwCode = dwCode;
         pUpdate->pData = nx_memdup(pAction, sizeof(NXC_ACTION));
         m_pUpdateQueue->Put(pUpdate);
      }
   }
}


//
// Send all actions to client
//

void ClientSession::SendAllActions(DWORD dwRqId)
{
   CSCPMessage msg;

   // Prepare responce message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);

   // Check user rights
   if ((m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_ACTIONS) ||
       (m_dwSystemAccess & SYSTEM_ACCESS_EPP))
   {
      msg.SetVariable(VID_RCC, RCC_SUCCESS);
      SendMessage(&msg);
      MutexLock(m_mutexSendActions, INFINITE);
      SendActionsToClient(this, dwRqId);
      MutexUnlock(m_mutexSendActions);
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      SendMessage(&msg);
   }
}


//
// Send list of configured container categories to client
//

void ClientSession::SendContainerCategories(DWORD dwRqId)
{
   CSCPMessage msg;
   DWORD i;

   // Prepare responce message
   msg.SetCode(CMD_CONTAINER_CAT_DATA);
   msg.SetId(dwRqId);

   for(i = 0; i < g_dwNumCategories; i++)
   {
      msg.SetVariable(VID_CATEGORY_ID, g_pContainerCatList[i].dwCatId);
      msg.SetVariable(VID_CATEGORY_NAME, g_pContainerCatList[i].szName);
      msg.SetVariable(VID_IMAGE_ID, g_pContainerCatList[i].dwImageId);
      msg.SetVariable(VID_DESCRIPTION, g_pContainerCatList[i].pszDescription);
      SendMessage(&msg);
      msg.DeleteAllVariables();
   }

   // Send end-of-list indicator
   msg.SetVariable(VID_CATEGORY_ID, (DWORD)0);
   SendMessage(&msg);
}
