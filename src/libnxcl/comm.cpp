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
// Static data
//

static SOCKET m_hSocket = -1;
static DWORD m_dwMsgId;
static char m_szServer[MAX_SERVER_NAME];
static char m_szLogin[MAX_LOGIN_NAME];
static BYTE m_szPassword[SHA_DIGEST_LENGTH];
static MsgWaitQueue m_msgWaitQueue;


//
// Get symbolic name for message code
// This function is intended to be used from receiver thread only,
// so we don't care about reentrancy
//

static char *MessageCodeName(WORD wCode)
{
   static char *pszMsgNames[] =
   {
      "CMD_LOGIN",
      "CMD_LOGIN_RESP",
      "CMD_KEEPALIVE",
      "CMD_EVENT",
      "CMD_GET_OBJECTS",
      "CMD_OBJECT",
      "CMD_DELETE_OBJECT",
      "CMD_MODIFY_OBJECT",
      "CMD_OBJECT_LIST_END",
      "CMD_OBJECT_UPDATE",
      "CMD_GET_EVENTS",
      "CMD_EVENT_LIST_END",
      "CMD_GET_CONFIG_VARLIST",
      "CMD_SET_CONFIG_VARIABLE",
      "CMD_CONFIG_VARIABLE",
      "CMD_CONFIG_VARLIST_END",
      "CMD_DELETE_CONFIG_VARIABLE",
      "CMD_NOTIFY",
      "CMD_OPEN_EPP",
      "CMD_CLOSE_EPP",
      "CMD_INSTALL_EPP",
      "CMD_SAVE_EPP",
      "CMD_EPP_RECORD",
      "CMD_OPEN_EVENT_DB",
      "CMD_CLOSE_EVENT_DB",
      "CMD_SET_EVENT_INFO",
      "CMD_EVENT_DB_RECORD",
      "CMD_EVENT_DB_EOF",
      "CMD_REQUEST_COMPLETED",
      "CMD_LOAD_USER_DB",
      "CMD_USER_DATA",
      "CMD_GROUP_DATA",
      "CMD_USER_DB_EOF",
      "CMD_UPDATE_USER",
      "CMD_DELETE_USER",
      "CMD_CREATE_USER",
      "CMD_LOCK_USER_DB",
      "CMD_UNLOCK_USER_DB"
   };
   static char szBuffer[32];

   if ((wCode >= CMD_LOGIN) && (wCode <= CMD_UNLOCK_USER_DB))
      return pszMsgNames[wCode - CMD_LOGIN];
   sprintf(szBuffer, "CMD_UNKNOWN(%d)", wCode);
   return szBuffer;
}


//
// Send raw message
//

static BOOL SendRawMsg(CSCP_MESSAGE *pMsg)
{
   return send(m_hSocket, (char *)pMsg, ntohs(pMsg->wSize), 0) == (int)ntohs(pMsg->wSize);
}


//
// Send message
//

BOOL SendMsg(CSCPMessage *pMsg)
{
   CSCP_MESSAGE *pRawMsg;
   BOOL bResult;

   DebugPrintf("SendMsg(\"%s\", id:%ld)", MessageCodeName(pMsg->GetCode()), pMsg->GetId());
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

   // Initialize raw message receiving function
   pMsgBuffer = (CSCP_BUFFER *)MemAlloc(sizeof(CSCP_BUFFER));
   RecvCSCPMessage(0, NULL, pMsgBuffer);

   // Allocate space for raw message
   pRawMsg = (CSCP_MESSAGE *)MemAlloc(65536);

   // Message receiving loop
   while(1)
   {
      // Receive raw message
      if ((iErr = RecvCSCPMessage(m_hSocket, pRawMsg, pMsgBuffer)) <= 0)
         break;

      // Check that actual received packet size is equal to encoded in packet
      if (ntohs(pRawMsg->wSize) != iErr)
      {
         DebugPrintf("RecvMsg: Bad packet length [wSize=%d ActualSize=%d]", ntohs(pRawMsg->wSize), iErr);
         continue;   // Bad packet, wait for next
      }

      // Create message object from raw message
      if (IsBinaryMsg(pRawMsg))
      {
         // Convert numeric fields to host byte order
         pRawMsg->wCode = ntohs(pRawMsg->wCode) & 0x0FFF;   // Clear flag bits from code
         pRawMsg->wSize = ntohs(pRawMsg->wSize);
         pRawMsg->dwId = ntohl(pRawMsg->dwId);

         DebugPrintf("RecvRawMsg(\"%s\", id:%ld)", MessageCodeName(pRawMsg->wCode), pRawMsg->dwId);

         // Process message
         switch(pRawMsg->wCode)
         {
            case CMD_EVENT:
               ProcessEvent(NULL, pRawMsg);
               break;
            default:    // We ignore unknown raw messages
               break;
         }
      }
      else
      {
         pMsg = new CSCPMessage(pRawMsg);
         bMsgNotNeeded = TRUE;
         DebugPrintf("RecvMsg(\"%s\", id:%ld)", MessageCodeName(pMsg->GetCode()), pMsg->GetId());

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
            default:
               m_msgWaitQueue.Put(pMsg);
               bMsgNotNeeded = FALSE;
               break;
         }
         if (bMsgNotNeeded)
            delete pMsg;
      }
   }

   ChangeState(STATE_DISCONNECTED);
   MemFree(pRawMsg);
   MemFree(pMsgBuffer);
}


//
// Connect to server
//

BOOL Connect(void)
{
   struct sockaddr_in servAddr;
   CSCPMessage msg, *pResp;
   BOOL bSuccess;

   ChangeState(STATE_CONNECTING);

   // Reset unique message ID
   m_dwMsgId = 1;

   // Prepare address structure
   memset(&servAddr, 0, sizeof(struct sockaddr_in));
   servAddr.sin_family = AF_INET;
   servAddr.sin_addr.s_addr = inet_addr(m_szServer);
   if (servAddr.sin_addr.s_addr == INADDR_NONE)
   {
      struct hostent *hs;

      hs = gethostbyname(m_szServer);
      if (hs == NULL)
      {
         return FALSE;  // Unable to resolve host name
      }
      else
      {
         memcpy(&servAddr.sin_addr, hs->h_addr, hs->h_length);
      }
   }
   servAddr.sin_port = htons((WORD)SERVER_LISTEN_PORT);

   // Create socket
   if ((m_hSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
   {
      return FALSE;
   }

   // Connect to target
   if (connect(m_hSocket, (struct sockaddr *)&servAddr, sizeof(struct sockaddr_in)) != 0)
   {
      closesocket(m_hSocket);
      m_hSocket = -1;
      return FALSE;
   }

   // Start receiver thread
   ThreadCreate(NetReceiver, 0, NULL);
   
   // Prepare login message
   msg.SetId(m_dwMsgId++);
   msg.SetCode(CMD_LOGIN);
   msg.SetVariable(VID_LOGIN_NAME, m_szLogin);
   msg.SetVariable(VID_PASSWORD, m_szPassword, SHA_DIGEST_LENGTH);

   if (!SendMsg(&msg))
   {
      // Message send failed, drop connection
      shutdown(m_hSocket, 2);
      closesocket(m_hSocket);
      m_hSocket = -1;
      return FALSE;
   }

   // Receive responce message
   pResp = m_msgWaitQueue.WaitForMessage(CMD_LOGIN_RESP, msg.GetId(), 2000);
   if (pResp == NULL)   // Connection is broken or timed out
   {
      shutdown(m_hSocket, 2);
      closesocket(m_hSocket);
      m_hSocket = -1;
      return FALSE;
   }

   bSuccess = pResp->GetVariableLong(VID_LOGIN_RESULT);
   delete pResp;

   CallEventHandler(NXC_EVENT_LOGIN_RESULT, bSuccess, NULL);
   if (!bSuccess)
   {
      shutdown(m_hSocket, 2);
      closesocket(m_hSocket);
      m_hSocket = -1;
   }
   return bSuccess;
}


//
// Initiate connection to server
//

BOOL LIBNXCL_EXPORTABLE NXCConnect(char *szServer, char *szLogin, char *szPassword)
{
   if (g_dwState != STATE_DISCONNECTED)
      return FALSE;

   strcpy(m_szServer, szServer);
   strcpy(m_szLogin, szLogin);
   CreateSHA1Hash(szPassword, m_szPassword);

   CreateRequest(RQ_CONNECT, NULL, FALSE);

   return TRUE;
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

CSCPMessage *WaitForMessage(DWORD dwCode, DWORD dwId, DWORD dwTimeOut)
{
   return m_msgWaitQueue.WaitForMessage(dwCode, dwId, dwTimeOut);
}
