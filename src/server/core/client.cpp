/* 
** Project X - Network Management System
** Copyright (C) 2003 Victor Kirhenshtein
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
** $module: client.cpp
**
**/

#include "nms_core.h"

#define IsDebugComm() ((g_dwFlags & AF_STANDALONE) && (g_dwFlags & AF_DEBUG_CSCP))


//
// Client session class constructor
//

ClientSession::ClientSession(SOCKET hSocket)
{
   m_pSendQueue = new Queue;
   m_pMessageQueue = new Queue;
   m_hSocket = hSocket;
}


//
// Destructor
//

ClientSession::~ClientSession()
{
   delete m_pSendQueue;
   delete m_pMessageQueue;
}


//
// Post message to send queue
//

void ClientSession::SendMessage(CSCP_MESSAGE *pMsg)
{
   m_pSendQueue->Put(pMsg);
}


//
// ReadThread()
//

void ClientSession::ReadThread(void)
{
   CSCP_MESSAGE *pRawMsg;
   CSCPMessage *pMsg;
   int iErr;

   pRawMsg = (CSCP_MESSAGE *)malloc(65536);
   while(1)
   {
      if ((iErr = recv(m_hSocket, (char *)pRawMsg, 65536, 0)) <= 0)
         break;

      // Check that actual received packet size is equal to encoded in packet
      if (ntohs(pRawMsg->wSize) != iErr)
      {
         if (IsDebugComm())
            printf("*CSCP* Actual message size doesn't match wSize value (%d,%d)\n", iErr, ntohs(pRawMsg->wSize));
         continue;   // Bad packet, wait for next
      }

      // Create message object from raw message
      pMsg = new CSCPMessage(pRawMsg);
      m_pMessageQueue->Put(pMsg);
   }
   if (iErr < 0)
      WriteLog(MSG_SESSION_CLOSED, EVENTLOG_WARNING_TYPE, "e", WSAGetLastError());
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
      if (send(m_hSocket, (const char *)pMsg, pMsg->wSize, 0) <= 0)
      {
         free(pMsg);
         break;
      }
      free(pMsg);
   }
}


//
// Message processing thread
//

void ClientSession::ProcessingThread(void)
{
   CSCPMessage *pMsg;

   while(1)
   {
      pMsg = (CSCPMessage *)m_pMessageQueue->GetOrBlock();

      if (IsDebugComm())
         printf("*CSCP* Received message with code %d\n", pMsg->GetCode());

      switch(pMsg->GetCode())
      {
         case CMD_LOGIN:
            break;
         default:
            break;
      }
      delete pMsg;
   }
}


//
// Client communication read thread
//

static void ReadThread(void *pArg)
{
   ((ClientSession *)pArg)->ReadThread();
}


//
// Client communication write thread
//

static void WriteThread(void *pArg)
{
   ((ClientSession *)pArg)->WriteThread();
}


//
// Received message processing thread
//

static void ProcessingThread(void *pArg)
{
   ((ClientSession *)pArg)->ProcessingThread();
}


//
// Listener thread
//

void ClientListener(void *)
{
   SOCKET sock, sockClient;
   struct sockaddr_in servAddr;
   int iSize, errorCount = 0;
   WORD wListenPort;
   ClientSession *pSession;

   // Read configuration
   wListenPort = (WORD)ConfigReadInt("ClientListenerPort", SERVER_LISTEN_PORT);

   // Create socket
   if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
   {
      WriteLog(MSG_SOCKET_FAILED, EVENTLOG_ERROR_TYPE, "s", "ClientListener");
      return;
   }

   // Fill in local address structure
   memset(&servAddr, 0, sizeof(struct sockaddr_in));
   servAddr.sin_family = AF_INET;
   servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
   servAddr.sin_port = htons(wListenPort);

   // Bind socket
   if (bind(sock, (struct sockaddr *)&servAddr, sizeof(struct sockaddr_in)) != 0)
   {
      WriteLog(MSG_BIND_ERROR, EVENTLOG_ERROR_TYPE, "dse", wListenPort, "ClientListener", WSAGetLastError());
      exit(1);
   }

   // Set up queue
   listen(sock, SOMAXCONN);

   // Wait for connection requests
   while(!ShutdownInProgress())
   {
      iSize = sizeof(struct sockaddr_in);
      if ((sockClient = accept(sock, (struct sockaddr *)&servAddr, &iSize)) == -1)
      {
         int error;

#ifdef _WIN32
         error = WSAGetLastError();
         if (error != WSAEINTR)
#else
         error = errno;
         if (error != EINTR)
#endif
            WriteLog(MSG_ACCEPT_ERROR, EVENTLOG_ERROR_TYPE, "e", error);
         errorCount++;
         if (errorCount > 1000)
         {
            WriteLog(MSG_TOO_MANY_ACCEPT_ERRORS, EVENTLOG_WARNING_TYPE, NULL);
            errorCount = 0;
         }
         ThreadSleepMs(500);
      }

      errorCount = 0;     // Reset consecutive errors counter

      // Create new session structure and threads
      pSession = new ClientSession(sockClient);
      ThreadCreate(ReadThread, 0, (void *)pSession);
      ThreadCreate(WriteThread, 0, (void *)pSession);
      ThreadCreate(ProcessingThread, 0, (void *)pSession);
   }

   closesocket(sock);
}
