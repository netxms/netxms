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
   m_hCondWriteThreadStopped = ConditionCreate();
   m_hCondProcessingThreadStopped = ConditionCreate();
   m_hCondUpdateThreadStopped = ConditionCreate();
   m_hMutexSendEvents = MutexCreate();
   m_dwFlags = 0;
   m_dwHostAddr = dwHostAddr;
   strcpy(m_szUserName, "<not logged in>");
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

   // Unlock locked components
   if (m_dwFlags & CSF_EVENT_DB_LOCKED)
      UnlockComponent(CID_EVENT_DB);
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
// Post message to send queue
//

void ClientSession::SendMessage(CSCPMessage *pMsg)
{
   m_pSendQueue->Put(pMsg->CreateMessage());
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

   while(1)
   {
      pMsg = (CSCPMessage *)m_pMessageQueue->GetOrBlock();
      if (pMsg == INVALID_POINTER_VALUE)    // Session termination indicator
         break;

      DebugPrintf("Received message with code %d\n", pMsg->GetCode());
      if ((m_iState != STATE_AUTHENTICATED) && (pMsg->GetCode() != CMD_LOGIN))
      {
         delete pMsg;
         continue;
      }

      switch(pMsg->GetCode())
      {
         case CMD_LOGIN:
            if (m_iState != STATE_AUTHENTICATED)
            {
               BYTE szPassword[SHA_DIGEST_LENGTH];
               char *pszLogin, szBuffer[16];
               
               pszLogin = pMsg->GetVariableStr(VID_LOGIN_NAME);
               pMsg->GetVariableBinary(VID_PASSWORD, szPassword, SHA_DIGEST_LENGTH);

               if (AuthenticateUser(pszLogin, szPassword, &m_dwUserId, &m_dwSystemAccess))
                  m_iState = STATE_AUTHENTICATED;

               if (m_iState == STATE_AUTHENTICATED)
               {
                  sprintf(m_szUserName, "%s@%s", pszLogin, IpToStr(m_dwHostAddr, szBuffer));
               }

               MemFree(pszLogin);

               // Send reply
               pReply = new CSCPMessage;
               pReply->SetCode(CMD_LOGIN_RESP);
               pReply->SetId(pMsg->GetId());
               pReply->SetVariable(VID_LOGIN_RESULT, (DWORD)(m_iState == STATE_AUTHENTICATED));
               SendMessage(pReply);
               delete pReply;
            }
            else
            {
            }
            break;
         case CMD_GET_OBJECTS:
            SendAllObjects();
            break;
         case CMD_GET_EVENTS:
            SendAllEvents();
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
               UnlockComponent(CID_EVENT_DB);
               m_dwFlags &= ~CSF_EVENT_DB_LOCKED;
            }
            break;
         default:
            break;
      }
      delete pMsg;
   }
   ConditionSet(m_hCondProcessingThreadStopped);
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
   msg.SetCode(CMD_OPEN_EVENT_DB);
   msg.SetId(dwRqId);

   if (!LockComponent(CID_EVENT_DB, m_dwIndex, m_szUserName, NULL, szBuffer))
   {
      msg.SetVariable(VID_RCC, RCC_COMPONENT_LOCKED);
      msg.SetVariable(VID_LOCKED_BY, szBuffer);
      SendMessage(&msg);
   }
   else
   {
      m_dwFlags |= CSF_EVENT_DB_LOCKED;

      msg.SetVariable(VID_RCC, RCC_SUCCESS);
      SendMessage(&msg);
      msg.DeleteAllVariables();

      // Prepare data message
      msg.SetCode(CMD_EVENT_DB_RECORD);
      msg.SetId(dwRqId);

      hResult = DBAsyncSelect(g_hCoreDB, "SELECT id,name,severity,flags,message,description FROM events");
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
   }
}


//
// Send all objects to client
//

void ClientSession::SendAllObjects(void)
{
   DWORD i;
   CSCPMessage msg;

   // Prepare message
   msg.SetCode(CMD_OBJECT);

   // Send objects, one per message
   ObjectsGlobalLock();
   for(i = 0; i < g_dwIdIndexSize; i++)
   {
      g_pIndexById[i].pObject->CreateMessage(&msg);
      SendMessage(&msg);
      msg.DeleteAllVariables();
   }
   ObjectsGlobalUnlock();

   // Send end of list notification
   msg.SetCode(CMD_OBJECT_LIST_END);
   SendMessage(&msg);
}


//
// Send all events to client
//

void ClientSession::SendAllEvents(void)
{
   CSCPMessage msg;
   DB_ASYNC_RESULT hResult;
   NXC_EVENT event;

   MutexLock(m_hMutexSendEvents, INFINITE);

   // Retrieve events from database
   hResult = DBAsyncSelect(g_hCoreDB, "SELECT event_id,timestamp,source,severity,message FROM EventLog ORDER BY timestamp");
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
         m_pSendQueue->Put(CreateRawCSCPMessage(CMD_EVENT, 0, sizeof(NXC_EVENT), &event, NULL));
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

   pUpdate = (UPDATE_INFO *)malloc(sizeof(UPDATE_INFO));
   pUpdate->dwCategory = INFO_CAT_EVENT;
   pUpdate->pData = malloc(sizeof(NXC_EVENT));
   pEvent->PrepareMessage((NXC_EVENT *)pUpdate->pData);
   m_pUpdateQueue->Put(pUpdate);
}


//
// Handler for object changes
//

void ClientSession::OnObjectChange(DWORD dwObjectId)
{
}


//
// Update processing thread
//

void ClientSession::UpdateThread(void)
{
   UPDATE_INFO *pUpdate;

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
