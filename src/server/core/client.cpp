/* 
** NetXMS - Network Management System
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


//
// Constants
//

#define MAX_CLIENT_SESSIONS   128


//
// Static data
//

static ClientSession *m_pSessionList[MAX_CLIENT_SESSIONS];
static MUTEX m_hSessionListAccess;


//
// Register new session in list
//

static BOOL RegisterSession(ClientSession *pSession)
{
   DWORD i;

   MutexLock(m_hSessionListAccess, INFINITE);
   for(i = 0; i < MAX_CLIENT_SESSIONS; i++)
      if (m_pSessionList[i] == NULL)
      {
         m_pSessionList[i] = pSession;
         pSession->SetIndex(i);
         MutexUnlock(m_hSessionListAccess);
         return TRUE;
      }

   MutexUnlock(m_hSessionListAccess);
   WriteLog(MSG_TOO_MANY_SESSIONS, EVENTLOG_WARNING_TYPE, NULL);
   return FALSE;
}


//
// Unregister session
//

static void UnregisterSession(DWORD dwIndex)
{
   MutexLock(m_hSessionListAccess, INFINITE);
   m_pSessionList[dwIndex] = NULL;
   MutexUnlock(m_hSessionListAccess);
}


//
// Client communication read thread
//

static void ReadThread(void *pArg)
{
   ((ClientSession *)pArg)->ReadThread();

   // When ClientSession::ReadThread exits, all other session
   // threads are already stopped, so we can safely destroy
   // session object
   UnregisterSession(((ClientSession *)pArg)->GetIndex());
   delete (ClientSession *)pArg;
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
// Information update processing thread
//

static void UpdateThread(void *pArg)
{
   ((ClientSession *)pArg)->UpdateThread();
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

   // Create session list access mutex
   m_hSessionListAccess = MutexCreate();

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
      if (!RegisterSession(pSession))
      {
         delete pSession;
      }
      else
      {
         ThreadCreate(ReadThread, 0, (void *)pSession);
         ThreadCreate(WriteThread, 0, (void *)pSession);
         ThreadCreate(ProcessingThread, 0, (void *)pSession);
         ThreadCreate(UpdateThread, 0, (void *)pSession);
      }
   }

   closesocket(sock);
}


//
// Dump client sessions to screen
//

void DumpSessions(void)
{
   int i;

   for(i = 0; i < MAX_CLIENT_SESSIONS; i++)
      if (m_pSessionList[i] != NULL)
         printf("%d\n", i);
}


//
// Enumerate active sessions
//

void EnumerateClientSessions(void (*pHandler)(ClientSession *, void *), void *pArg)
{
   int i;

   MutexLock(m_hSessionListAccess, INFINITE);
   for(i = 0; i < MAX_CLIENT_SESSIONS; i++)
      if (m_pSessionList[i] != NULL)
         pHandler(m_pSessionList[i], pArg);
   MutexUnlock(m_hSessionListAccess);
}
