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
** $module: comm.cpp
**
**/

#include "libnxcl.h"


//
// Unique message ID
//

DWORD g_dwMsgId;


//
// Static data
//

static SOCKET m_hSocket = -1;
static MsgWaitQueue m_msgWaitQueue;
static DWORD m_dwReceiverBufferSize = 4194304;     // 4MB


//
// Send raw message
//

static BOOL SendRawMsg(CSCP_MESSAGE *pMsg)
{
   return send(m_hSocket, (char *)pMsg, ntohl(pMsg->dwSize), 0) == (int)ntohl(pMsg->dwSize);
}


//
// Send message
//

BOOL SendMsg(CSCPMessage *pMsg)
{
   CSCP_MESSAGE *pRawMsg;
   BOOL bResult;
   char szBuffer[128];

   DebugPrintf("SendMsg(\"%s\", id:%ld)", CSCPMessageCodeName(pMsg->GetCode(), szBuffer), pMsg->GetId());
   pRawMsg = pMsg->CreateMessage();
   bResult = SendRawMsg(pRawMsg);
   MemFree(pRawMsg);
   return bResult;
}


//
// Network receiver thread
//

static void NetReceiver(void *pArg)
{
   CSCPMessage *pMsg;
   CSCP_MESSAGE *pRawMsg;
   CSCP_BUFFER *pMsgBuffer;
   int iErr;
   BOOL bMsgNotNeeded;
   char szBuffer[128];

   // Initialize raw message receiving function
   pMsgBuffer = (CSCP_BUFFER *)MemAlloc(sizeof(CSCP_BUFFER));
   RecvCSCPMessage(0, NULL, pMsgBuffer, 0);

   // Allocate space for raw message
   pRawMsg = (CSCP_MESSAGE *)MemAlloc(m_dwReceiverBufferSize);

   // Message receiving loop
   while(1)
   {
      // Receive raw message
      if ((iErr = RecvCSCPMessage(m_hSocket, pRawMsg, pMsgBuffer, m_dwReceiverBufferSize)) <= 0)
         break;

      // Check if we get too large message
      if (iErr == 1)
      {
         DebugPrintf("Received too large message %s (%ld bytes)", 
                     CSCPMessageCodeName(ntohs(pRawMsg->wCode), szBuffer),
                     ntohl(pRawMsg->dwSize));
         continue;
      }

      // Check that actual received packet size is equal to encoded in packet
      if ((int)ntohl(pRawMsg->dwSize) != iErr)
      {
         DebugPrintf("RecvMsg: Bad packet length [dwSize=%d ActualSize=%d]", ntohl(pRawMsg->dwSize), iErr);
         continue;   // Bad packet, wait for next
      }

      // Create message object from raw message
      if (IsBinaryMsg(pRawMsg))
      {
         // Convert numeric fields to host byte order
         pRawMsg->wCode = ntohs(pRawMsg->wCode);
         pRawMsg->wFlags = ntohs(pRawMsg->wFlags);
         pRawMsg->dwSize = ntohl(pRawMsg->dwSize);
         pRawMsg->dwId = ntohl(pRawMsg->dwId);
         pRawMsg->dwNumVars = ntohl(pRawMsg->dwNumVars);

         DebugPrintf("RecvRawMsg(\"%s\", id:%ld)", CSCPMessageCodeName(pRawMsg->wCode, szBuffer), pRawMsg->dwId);

         // Process message
         switch(pRawMsg->wCode)
         {
            case CMD_EVENT:
               ProcessEvent(NULL, pRawMsg);
               break;
            default:    // Put unknown raw messages into the wait queue
               m_msgWaitQueue.Put((CSCP_MESSAGE *)nx_memdup(pRawMsg, pRawMsg->dwSize));
               break;
         }
      }
      else
      {
         pMsg = new CSCPMessage(pRawMsg);
         bMsgNotNeeded = TRUE;
         DebugPrintf("RecvMsg(\"%s\", id:%ld)", CSCPMessageCodeName(pMsg->GetCode(), szBuffer), pMsg->GetId());

         // Process message
         switch(pMsg->GetCode())
         {
            case CMD_KEEPALIVE:     // Keepalive message, ignore it
               break;
            case CMD_OBJECT:        // Object information
            case CMD_OBJECT_UPDATE:
            case CMD_OBJECT_LIST_END:
               ProcessObjectUpdate(pMsg);
               break;
            case CMD_EVENT_LIST_END:
               ProcessEvent(pMsg, NULL);
               break;
            case CMD_EVENT_DB_RECORD:
            case CMD_EVENT_DB_EOF:
               ProcessEventDBRecord(pMsg);
               break;
            case CMD_USER_DATA:
            case CMD_GROUP_DATA:
            case CMD_USER_DB_EOF:
               ProcessUserDBRecord(pMsg);
               break;
            case CMD_USER_DB_UPDATE:
               ProcessUserDBUpdate(pMsg);
               break;
            case CMD_NODE_DCI:
            case CMD_NODE_DCI_LIST_END:
               ProcessDCI(pMsg);
               break;
            case CMD_NOTIFY:
               CallEventHandler(NXC_EVENT_NOTIFICATION, 
                                pMsg->GetVariableLong(VID_NOTIFICATION_CODE), NULL);
               break;
            default:
               m_msgWaitQueue.Put(pMsg);
               bMsgNotNeeded = FALSE;
               break;
         }
         if (bMsgNotNeeded)
            delete pMsg;
      }
   }

   DebugPrintf("Network receiver thread stopped");
   ChangeState(STATE_DISCONNECTED);
   MemFree(pRawMsg);
   MemFree(pMsgBuffer);
}


//
// Connect to server
//

DWORD LIBNXCL_EXPORTABLE NXCConnect(char *szServer, char *szLogin, char *szPassword)
{
   struct sockaddr_in servAddr;
   CSCPMessage msg, *pResp;
   BYTE szPasswordHash[SHA1_DIGEST_SIZE];
   DWORD dwRetCode = RCC_COMM_FAILURE;

   if (g_dwState == STATE_DISCONNECTED)
   {
      ChangeState(STATE_CONNECTING);

      // Reset unique message ID
      g_dwMsgId = 1;

      // Prepare address structure
      memset(&servAddr, 0, sizeof(struct sockaddr_in));
      servAddr.sin_family = AF_INET;
      servAddr.sin_port = htons((WORD)SERVER_LISTEN_PORT);
      servAddr.sin_addr.s_addr = inet_addr(szServer);
      if (servAddr.sin_addr.s_addr == INADDR_NONE)
      {
         struct hostent *hs;

         hs = gethostbyname(szServer);
         if (hs != NULL)
            memcpy(&servAddr.sin_addr, hs->h_addr, hs->h_length);
      }

      if (servAddr.sin_addr.s_addr != INADDR_NONE)
      {
         // Create socket
         if ((m_hSocket = socket(AF_INET, SOCK_STREAM, 0)) != -1)
         {
            // Connect to target
            if (connect(m_hSocket, (struct sockaddr *)&servAddr, sizeof(struct sockaddr_in)) == 0)
            {
               // Start receiver thread
               ThreadCreate(NetReceiver, 0, NULL);
   
               // Prepare login message
               msg.SetId(g_dwMsgId++);
               msg.SetCode(CMD_LOGIN);
               msg.SetVariable(VID_LOGIN_NAME, szLogin);
               CalculateSHA1Hash((BYTE *)szPassword, strlen(szPassword), szPasswordHash);
               msg.SetVariable(VID_PASSWORD, szPasswordHash, SHA1_DIGEST_SIZE);

               if (SendMsg(&msg))
               {
                  // Receive responce message
                  pResp = m_msgWaitQueue.WaitForMessage(CMD_LOGIN_RESP, msg.GetId(), 2000);
                  if (pResp != NULL)
                  {
                     dwRetCode = pResp->GetVariableLong(VID_RCC);
                     delete pResp;
                  }
                  else
                  {
                     // Connection is broken or timed out
                     dwRetCode = RCC_TIMEOUT;
                  }
               }

               if (dwRetCode != RCC_SUCCESS)
                  shutdown(m_hSocket, 2);
            }

            if (dwRetCode != RCC_SUCCESS)
            {
               closesocket(m_hSocket);
               m_hSocket = -1;
            }
         }
      }
      CallEventHandler(NXC_EVENT_LOGIN_RESULT, dwRetCode, NULL);
      ChangeState(dwRetCode == RCC_SUCCESS ? STATE_IDLE : STATE_DISCONNECTED);
   }
   else
   {
      dwRetCode = RCC_OUT_OF_STATE_REQUEST;
   }

   return dwRetCode;
}


//
// Disconnect from server
//

void LIBNXCL_EXPORTABLE NXCDisconnect(void)
{
   // Close socket
   shutdown(m_hSocket, 2);
   closesocket(m_hSocket);

   // Clear message wait queue
   m_msgWaitQueue.Clear();
}


//
// Wait for specific message
//

CSCPMessage *WaitForMessage(WORD wCode, DWORD dwId, DWORD dwTimeOut)
{
   return m_msgWaitQueue.WaitForMessage(wCode, dwId, dwTimeOut);
}


//
// Wait for specific raw message
//

CSCP_MESSAGE *WaitForRawMessage(WORD wCode, DWORD dwId, DWORD dwTimeOut)
{
   return m_msgWaitQueue.WaitForRawMessage(wCode, dwId, dwTimeOut);
}


//
// Wait for request completion notification and extract result code
// from recived message
//

DWORD WaitForRCC(DWORD dwRqId)
{
   CSCPMessage *pResponce;
   DWORD dwRetCode;

   pResponce = WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId, 2000);
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
