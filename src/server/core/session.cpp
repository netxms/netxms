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

ClientSession::ClientSession(SOCKET hSocket)
{
   m_pSendQueue = new Queue;
   m_pMessageQueue = new Queue;
   m_hSocket = hSocket;
   m_dwIndex = INVALID_INDEX;
   m_iState = STATE_CONNECTED;
   m_pMsgBuffer = (CSCP_BUFFER *)malloc(sizeof(CSCP_BUFFER));
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
   if (m_pMsgBuffer != NULL)
      free(m_pMsgBuffer);
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
   m_pSendQueue->Put(NULL);
   m_pMessageQueue->Put(NULL);
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
      if (pMsg == NULL)    // Session termination indicator
         break;

      if (send(m_hSocket, (const char *)pMsg, ntohs(pMsg->wSize), 0) <= 0)
      {
         MemFree(pMsg);
         break;
      }
      MemFree(pMsg);
   }
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
      if (pMsg == NULL)    // Session termination indicator
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
               char *pszLogin = pMsg->GetVariableStr(VID_LOGIN_NAME);
               char *pszPassword = pMsg->GetVariableStr(VID_PASSWORD);

               if (AuthenticateUser(pszLogin, pszPassword, &m_dwUserId, &m_dwSystemAccess))
                  m_iState = STATE_AUTHENTICATED;

               MemFree(pszLogin);
               MemFree(pszPassword);

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
         default:
            break;
      }
      delete pMsg;
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
