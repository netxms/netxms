/* 
** NetXMS - Network Management System
** Server Library
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
** $module: agent.cpp
**
**/

#include "libnxsrv.h"
#include <stdarg.h>


//
// Constants
//

#define RECEIVER_BUFFER_SIZE        262144


//
// Receiver thread starter
//

void AgentConnection::ReceiverThreadStarter(void *pArg)
{
   ((AgentConnection *)pArg)->ReceiverThread();
}


//
// Default constructor for AgentConnection - normally shouldn't be used
//

AgentConnection::AgentConnection()
{
   m_dwAddr = inet_addr("127.0.0.1");
   m_wPort = AGENT_LISTEN_PORT;
   m_iAuthMethod = AUTH_NONE;
   m_szSecret[0] = 0;
   m_hSocket = -1;
   m_tLastCommandTime = 0;
   m_dwNumDataLines = 0;
   m_ppDataLines = NULL;
   m_pMsgWaitQueue = new MsgWaitQueue;
   m_dwRequestId = 0;
   m_dwCommandTimeout = 10000;   // Default timeout 10 seconds
}


//
// Normal constructor for AgentConnection
//

AgentConnection::AgentConnection(DWORD dwAddr, WORD wPort, int iAuthMethod, char *szSecret)
{
   m_dwAddr = dwAddr;
   m_wPort = wPort;
   m_iAuthMethod = iAuthMethod;
   if (szSecret != NULL)
      strncpy(m_szSecret, szSecret, MAX_SECRET_LENGTH);
   else
      m_szSecret[0] = 0;
   m_hSocket = -1;
   m_tLastCommandTime = 0;
   m_dwNumDataLines = 0;
   m_ppDataLines = NULL;
   m_pMsgWaitQueue = new MsgWaitQueue;
   m_dwRequestId = 0;
   m_dwCommandTimeout = 10000;   // Default timeout 10 seconds
}


//
// Destructor
//

AgentConnection::~AgentConnection()
{
   if (m_hSocket != -1)
      closesocket(m_hSocket);
   DestroyResultData();
   delete m_pMsgWaitQueue;
}


//
// Print message. This function is virtual and can be overrided in derived classes.
// Default implementation will print message to stdout.
//

void AgentConnection::PrintMsg(char *pszFormat, ...)
{
   va_list args;

   va_start(args, pszFormat);
   vprintf(pszFormat, args);
   va_end(args);
   printf("\n");
}


//
// Receiver thread
//

void AgentConnection::ReceiverThread(void)
{
   CSCPMessage *pMsg;
   CSCP_MESSAGE *pRawMsg;
   CSCP_BUFFER *pMsgBuffer;
   int iErr;
   char szBuffer[128];

   // Initialize raw message receiving function
   pMsgBuffer = (CSCP_BUFFER *)MemAlloc(sizeof(CSCP_BUFFER));
   RecvCSCPMessage(0, NULL, pMsgBuffer, 0);

   // Allocate space for raw message
   pRawMsg = (CSCP_MESSAGE *)MemAlloc(RECEIVER_BUFFER_SIZE);

   // Message receiving loop
   while(1)
   {
      // Receive raw message
      if ((iErr = RecvCSCPMessage(m_hSocket, pRawMsg, pMsgBuffer, RECEIVER_BUFFER_SIZE)) <= 0)
         break;

      // Check if we get too large message
      if (iErr == 1)
      {
         PrintMsg("Received too large message %s (%ld bytes)", 
                  CSCPMessageCodeName(ntohs(pRawMsg->wCode), szBuffer),
                  ntohl(pRawMsg->dwSize));
         continue;
      }

      // Check that actual received packet size is equal to encoded in packet
      if ((int)ntohl(pRawMsg->dwSize) != iErr)
      {
         PrintMsg("RecvMsg: Bad packet length [dwSize=%d ActualSize=%d]", ntohl(pRawMsg->dwSize), iErr);
         continue;   // Bad packet, wait for next
      }

      // Create message object from raw message
      pMsg = new CSCPMessage(pRawMsg);
      if (pMsg->GetCode() == CMD_TRAP)
      {
         OnTrap(pMsg);
         delete pMsg;
      }
      else
      {
         m_pMsgWaitQueue->Put(pMsg);
      }
   }

   MemFree(pRawMsg);
   MemFree(pMsgBuffer);
}


//
// Connect to agent
//

BOOL AgentConnection::Connect(BOOL bVerbose)
{
   struct sockaddr_in sa;
   char szBuffer[256];
   BOOL bSuccess = FALSE;

   // Create socket
   m_hSocket = socket(AF_INET, SOCK_STREAM, 0);
   if (m_hSocket == -1)
   {
      PrintMsg("Call to socket() failed");
      goto connect_cleanup;
   }

   // Fill in address structure
   memset(&sa, 0, sizeof(sa));
   sa.sin_addr.s_addr = m_dwAddr;
   sa.sin_family = AF_INET;
   sa.sin_port = htons(m_wPort);

   // Connect to server
   if (connect(m_hSocket, (struct sockaddr *)&sa, sizeof(sa)) == -1)
   {
      if (bVerbose)
         PrintMsg("Cannot establish connection with agent %s", IpToStr(m_dwAddr, szBuffer));
      goto connect_cleanup;
   }

   // Start receiver thread
   ThreadCreate(ReceiverThreadStarter, 0, this);

   // Authenticate itself to agent
   switch(m_iAuthMethod)
   {
      case AUTH_PLAINTEXT:
         break;
      default:
         break;
   }

   // Test connectivity
   if (Nop() != ERR_SUCCESS)
   {
      PrintMsg("Communication with agent %s failed", IpToStr(m_dwAddr, szBuffer));
      goto connect_cleanup;
   }

   bSuccess = TRUE;

connect_cleanup:
   if (!bSuccess)
   {
      if (m_hSocket != -1)
      {
         shutdown(m_hSocket, 2);
         closesocket(m_hSocket);
         m_hSocket = -1;
      }
   }
   return bSuccess;
}


//
// Disconnect from agent
//

void AgentConnection::Disconnect(void)
{
   if (m_hSocket != -1)
   {
      shutdown(m_hSocket, 2);
      closesocket(m_hSocket);
      m_hSocket = -1;
   }
   DestroyResultData();
}


//
// Destroy command execuion results data
//

void AgentConnection::DestroyResultData(void)
{
   DWORD i;

   if (m_ppDataLines != NULL)
   {
      for(i = 0; i < m_dwNumDataLines; i++)
         if (m_ppDataLines[i] != NULL)
            MemFree(m_ppDataLines[i]);
      MemFree(m_ppDataLines);
      m_ppDataLines = NULL;
   }
   m_dwNumDataLines = 0;
}


//
// Get interface list from agent
//

INTERFACE_LIST *AgentConnection::GetInterfaceList(void)
{
   return NULL;
}


//
// Get parameter value
//

DWORD AgentConnection::GetParameter(char *pszParam, DWORD dwBufSize, char *pszBuffer)
{
   CSCPMessage msg, *pResponce;
   DWORD dwRqId, dwRetCode;

   dwRqId = m_dwRequestId++;
   msg.SetCode(CMD_GET_PARAMETER);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_PARAMETER, pszParam);
   if (SendMessage(&msg))
   {
      pResponce = WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId, m_dwCommandTimeout);
      if (pResponce != NULL)
      {
         dwRetCode = pResponce->GetVariableLong(VID_RCC);
         if (dwRetCode == ERR_SUCCESS)
            pResponce->GetVariableStr(VID_VALUE, pszBuffer, dwBufSize);
         delete pResponce;
      }
      else
      {
         dwRetCode = ERR_REQUEST_TIMEOUT;
      }
   }
   else
   {
      dwRetCode = ERR_CONNECTION_BROKEN;
   }

   return dwRetCode;
}


//
// Get ARP cache
//

ARP_CACHE *AgentConnection::GetArpCache(void)
{
   return NULL;
}


//
// Send dummy command to agent (can be used for keepalive)
//

DWORD AgentConnection::Nop(void)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = m_dwRequestId++;
   msg.SetCode(CMD_KEEPALIVE);
   msg.SetId(dwRqId);
   if (SendMessage(&msg))
      return WaitForRCC(dwRqId, m_dwCommandTimeout);
   else
      return ERR_CONNECTION_BROKEN;
}


//
// Wait for request completion code
//

DWORD AgentConnection::WaitForRCC(DWORD dwRqId, DWORD dwTimeOut)
{
   CSCPMessage *pMsg;
   DWORD dwRetCode;

   pMsg = m_pMsgWaitQueue->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId, dwTimeOut);
   if (pMsg != NULL)
   {
      dwRetCode = pMsg->GetVariableLong(VID_RCC);
      delete pMsg;
   }
   else
   {
      dwRetCode = ERR_REQUEST_TIMEOUT;
   }
   return dwRetCode;
}


//
// Send message to agent
//

BOOL AgentConnection::SendMessage(CSCPMessage *pMsg)
{
   CSCP_MESSAGE *pRawMsg;
   BOOL bResult;

   pRawMsg = pMsg->CreateMessage();
   bResult = (send(m_hSocket, (char *)pRawMsg, ntohl(pRawMsg->dwSize), 0) == (int)ntohl(pRawMsg->dwSize));
   MemFree(pRawMsg);
   return bResult;
}


//
// Trap handler. Should be overriden in derived classes to implement
// actual trap processing. Default implementation do nothing.
//

void AgentConnection::OnTrap(CSCPMessage *pMsg)
{
}


//
// Get list of values
//

DWORD AgentConnection::GetList(char *pszParam)
{
   CSCPMessage msg, *pResponce;
   DWORD i, dwRqId, dwRetCode;

   DestroyResultData();
   dwRqId = m_dwRequestId++;
   msg.SetCode(CMD_GET_LIST);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_PARAMETER, pszParam);
   if (SendMessage(&msg))
   {
      pResponce = WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId, m_dwCommandTimeout);
      if (pResponce != NULL)
      {
         dwRetCode = pResponce->GetVariableLong(VID_RCC);
         if (dwRetCode == ERR_SUCCESS)
         {
            m_dwNumDataLines = pResponce->GetVariableLong(VID_NUM_STRINGS);
            m_ppDataLines = (char **)MemAlloc(sizeof(char *) * m_dwNumDataLines);
            for(i = 0; i < m_dwNumDataLines; i++)
               m_ppDataLines[i] = pResponce->GetVariableStr(VID_ENUM_VALUE_BASE + i);
         }
         delete pResponce;
      }
      else
      {
         dwRetCode = ERR_REQUEST_TIMEOUT;
      }
   }
   else
   {
      dwRetCode = ERR_CONNECTION_BROKEN;
   }

   return dwRetCode;
}
