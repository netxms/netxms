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
   m_hCondWriteThreadStopped = ConditionCreate(FALSE);
   m_hCondProcessingThreadStopped = ConditionCreate(FALSE);
   m_hCondUpdateThreadStopped = ConditionCreate(FALSE);
   m_hMutexSendEvents = MutexCreate();
   m_hMutexSendObjects = MutexCreate();
   m_dwFlags = 0;
   m_dwHostAddr = dwHostAddr;
   strcpy(m_szUserName, "<not logged in>");
   m_dwUserId = INVALID_INDEX;
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
   if (m_pMsgBuffer != NULL)
      free(m_pMsgBuffer);
   ConditionDestroy(m_hCondWriteThreadStopped);
   ConditionDestroy(m_hCondProcessingThreadStopped);
   ConditionDestroy(m_hCondUpdateThreadStopped);
   MutexDestroy(m_hMutexSendEvents);
   MutexDestroy(m_hMutexSendObjects);
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

   // Initialize raw message receiving function
   RecvCSCPMessage(0, NULL, m_pMsgBuffer);

   pRawMsg = (CSCP_MESSAGE *)malloc(65536);
   while(1)
   {
      if ((iErr = RecvCSCPMessage(m_hSocket, pRawMsg, m_pMsgBuffer)) <= 0)
         break;

      // Check that actual received packet size is equal to encoded in packet
      if (ntohs(pRawMsg->wSize) != iErr)
      {
         DebugPrintf("Actual message size doesn't match wSize value (%d,%d)\n", iErr, ntohs(pRawMsg->wSize));
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
   m_pSendQueue->Put(INVALID_POINTER_VALUE);
   m_pMessageQueue->Put(INVALID_POINTER_VALUE);

   // Wait for other threads to finish
   ConditionWait(m_hCondWriteThreadStopped, INFINITE);
   ConditionWait(m_hCondProcessingThreadStopped, INFINITE);

   // Remove all locks created by this session
   RemoveAllSessionLocks(m_dwIndex);
}


//
// WriteThread()
//

void ClientSession::WriteThread(void)
{
   CSCP_MESSAGE *pMsg;

   while(1)
   {
      pMsg = (CSCP_MESSAGE *)m_pSendQueue->GetOrBlock();
      if (pMsg == INVALID_POINTER_VALUE)    // Session termination indicator
         break;

      if (send(m_hSocket, (const char *)pMsg, ntohs(pMsg->wSize), 0) <= 0)
      {
         MemFree(pMsg);
         break;
      }
      MemFree(pMsg);
   }
   ConditionSet(m_hCondWriteThreadStopped);
}


//
// Message processing thread
//

void ClientSession::ProcessingThread(void)
{
   CSCPMessage *pMsg, *pReply;
   char szBuffer[128];

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
            if (m_dwFlags & CSF_EVENT_DB_LOCKED)
            {
               // Check if event configuration DB has been modified
               if (m_dwFlags & CSF_EVENT_DB_MODIFIED)
                  ReloadEvents();
               UnlockComponent(CID_EVENT_DB);
               m_dwFlags &= ~CSF_EVENT_DB_LOCKED;
            }
            // Send reply
            pReply = new CSCPMessage;
            pReply->SetCode(CMD_REQUEST_COMPLETED);
            pReply->SetId(pMsg->GetId());
            pReply->SetVariable(VID_RCC, RCC_SUCCESS);
            SendMessage(pReply);
            delete pReply;
            break;
         case CMD_SET_EVENT_INFO:
            SetEventInfo(pMsg);
            break;
         case CMD_MODIFY_OBJECT:
            ModifyObject(pMsg);
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
         default:
            break;
      }
      delete pMsg;
   }
   ConditionSet(m_hCondProcessingThreadStopped);
}


//
// Authenticate client
//

void ClientSession::Login(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   BYTE szPassword[SHA_DIGEST_LENGTH];
   char szLogin[MAX_USER_NAME], szBuffer[32];

   // Prepare responce message
   msg.SetCode(CMD_LOGIN_RESP);
   msg.SetId(pRequest->GetId());

   if (m_iState != STATE_AUTHENTICATED)
   {
      
      pRequest->GetVariableStr(VID_LOGIN_NAME, szLogin, MAX_USER_NAME);
      pRequest->GetVariableBinary(VID_PASSWORD, szPassword, SHA_DIGEST_LENGTH);

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
      while(DBFetch(hResult))
      {
         msg.SetVariable(VID_EVENT_ID, DBGetFieldAsyncULong(hResult, 0));
         msg.SetVariable(VID_NAME, DBGetFieldAsync(hResult, 1, szBuffer, 1024));
         msg.SetVariable(VID_SEVERITY, DBGetFieldAsyncULong(hResult, 2));
         msg.SetVariable(VID_FLAGS, DBGetFieldAsyncULong(hResult, 3));
         msg.SetVariable(VID_MESSAGE, DBGetFieldAsync(hResult, 4, szBuffer, 1024));
         msg.SetVariable(VID_DESCRIPTION, DBGetFieldAsync(hResult, 5, szBuffer, 1024));
         SendMessage(&msg);
         msg.DeleteAllVariables();
      }
      DBFreeAsyncResult(hResult);

      // Send end-of-list indicator
      msg.SetCode(CMD_EVENT_DB_EOF);
      SendMessage(&msg);
   }
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

   MutexLock(m_hMutexSendObjects, INFINITE);

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

   MutexUnlock(m_hMutexSendObjects);
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

   MutexLock(m_hMutexSendEvents, INFINITE);

   // Retrieve events from database
   hResult = DBAsyncSelect(g_hCoreDB, "SELECT event_id,timestamp,source,severity,message FROM event_log ORDER BY timestamp");
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
         m_pSendQueue->Put(CreateRawCSCPMessage(CMD_EVENT, dwRqId, sizeof(NXC_EVENT), &event, NULL));
      }
      DBFreeAsyncResult(hResult);
   }

   // Send end of list notification
   msg.SetCode(CMD_EVENT_LIST_END);
   SendMessage(&msg);

   MutexUnlock(m_hMutexSendEvents);
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
      hResult = DBSelect(g_hCoreDB, "SELECT name,value FROM config");
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
// Update processing thread
//

void ClientSession::UpdateThread(void)
{
   UPDATE_INFO *pUpdate;
   CSCPMessage msg;

   while(1)
   {
      pUpdate = (UPDATE_INFO *)m_pUpdateQueue->GetOrBlock();
      if (pUpdate == INVALID_POINTER_VALUE)    // Session termination indicator
         break;

      switch(pUpdate->dwCategory)
      {
         case INFO_CAT_EVENT:
            MutexLock(m_hMutexSendEvents, INFINITE);
            m_pSendQueue->Put(CreateRawCSCPMessage(CMD_EVENT, 0, sizeof(NXC_EVENT), pUpdate->pData, NULL));
            MutexUnlock(m_hMutexSendEvents);
            free(pUpdate->pData);
            break;
         case INFO_CAT_OBJECT_CHANGE:
            MutexLock(m_hMutexSendObjects, INFINITE);
            msg.SetId(0);
            msg.SetCode(CMD_OBJECT_UPDATE);
            ((NetObj *)pUpdate->pData)->CreateMessage(&msg);
            SendMessage(&msg);
            MutexUnlock(m_hMutexSendObjects);
            msg.DeleteAllVariables();
            ((NetObj *)pUpdate->pData)->DecRefCount();
            break;
         default:
            break;
      }

      free(pUpdate);
   }
   ConditionSet(m_hCondUpdateThreadStopped);
}


//
// Send notification message to server
//

void ClientSession::Notify(DWORD dwCode)
{
   CSCPMessage msg;

   msg.SetCode(CMD_NOTIFY);
   msg.SetVariable(VID_NOTIFICATION_CODE, dwCode);
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
         char szQuery[4096], *pszName, *pszMessage, *pszDescription;
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
         pszName = pRequest->GetVariableStr(VID_NAME);
         pszMessage = pRequest->GetVariableStr(VID_MESSAGE);
         pszDescription = pRequest->GetVariableStr(VID_DESCRIPTION);
         if (bEventExist)
            sprintf(szQuery, "UPDATE events SET name='%s',severity=%ld,flags=%ld,message='%s',description='%s' WHERE event_id=%ld",
                    pszName, pRequest->GetVariableLong(VID_SEVERITY), pRequest->GetVariableLong(VID_FLAGS),
                    pszMessage, pszDescription, dwEventId);
         else
            sprintf(szQuery, "INSERT INTO events SET event_id,name,severity,flags,message,description VALUES (%ld,'%s',%ld,%ld,'%s','%s')",
                    dwEventId, pszName, pRequest->GetVariableLong(VID_SEVERITY),
                    pRequest->GetVariableLong(VID_FLAGS), pszMessage, pszDescription);
         if (DBQuery(g_hCoreDB, szQuery))
         {
            msg.SetVariable(VID_RCC, RCC_SUCCESS);
            m_dwFlags |= CSF_EVENT_DB_MODIFIED;
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
         }

         MemFree(pszName);
         MemFree(pszMessage);
         MemFree(pszDescription);
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
   DWORD i, j, dwId;

   // Prepare responce message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_RCC, RCC_SUCCESS);
   SendMessage(&msg);

   // Send users
   msg.SetCode(CMD_USER_DATA);
   for(i = 0; i < g_dwNumUsers; i++)
   {
      msg.SetVariable(VID_USER_ID, g_pUserList[i].dwId);
      msg.SetVariable(VID_USER_NAME, g_pUserList[i].szName);
      msg.SetVariable(VID_USER_FLAGS, g_pUserList[i].wFlags);
      msg.SetVariable(VID_USER_SYS_RIGHTS, g_pUserList[i].wSystemRights);
      msg.SetVariable(VID_USER_FULL_NAME, g_pUserList[i].szFullName);
      msg.SetVariable(VID_USER_DESCRIPTION, g_pUserList[i].szDescription);
      SendMessage(&msg);
      msg.DeleteAllVariables();
   }

   // Send groups
   msg.SetCode(CMD_GROUP_DATA);
   for(i = 0; i < g_dwNumGroups; i++)
   {
      msg.SetVariable(VID_USER_ID, g_pGroupList[i].dwId);
      msg.SetVariable(VID_USER_NAME, g_pGroupList[i].szName);
      msg.SetVariable(VID_USER_FLAGS, g_pGroupList[i].wFlags);
      msg.SetVariable(VID_USER_SYS_RIGHTS, g_pGroupList[i].wSystemRights);
      msg.SetVariable(VID_USER_DESCRIPTION, g_pGroupList[i].szDescription);
      msg.SetVariable(VID_NUM_MEMBERS, g_pGroupList[i].dwNumMembers);
      for(j = 0, dwId = VID_GROUP_MEMBER_BASE; j < g_pGroupList[i].dwNumMembers; j++, dwId++)
         msg.SetVariable(dwId, g_pGroupList[i].pMembers[j]);
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
      bIsGroup = pRequest->GetVariableShort(VID_IS_GROUP);
      dwResult = CreateNewUser(szUserName, bIsGroup, &dwUserId);
      msg.SetVariable(VID_RCC, dwResult);
      if (dwResult == RCC_SUCCESS)
         msg.SetVariable(VID_USER_ID, dwUserId);   // Send id of new user to client
   }

   // Send responce
   SendMessage(&msg);
}


//
// Update existing user's data
//

void ClientSession::UpdateUser(CSCPMessage *pRequest)
{
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

   // Get Id of user to be deleted
   dwUserId = pRequest->GetVariableLong(VID_USER_ID);

   if (dwUserId != 0)
   {
      if (!(m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_USERS))
      {
         // Current user has no rights for user account management
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
         DWORD dwResult;

         dwResult = DeleteUserFromDB(dwUserId);
         msg.SetVariable(VID_RCC, dwResult);
      }
   }
   else
   {
      // Nobody can delete system administrator account
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
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
      msg.SetVariable(VID_USER_ID, dwUserId);

      switch(iCode)
      {
         case USER_DB_CREATE:
            if (dwUserId & GROUP_FLAG)
            {
               msg.SetVariable(VID_USER_NAME, pGroup->szName);
            }
            else
            {
               msg.SetVariable(VID_USER_NAME, pUser->szName);
            }
            break;
         default:
            break;
      }

      SendMessage(&msg);
   }
}
