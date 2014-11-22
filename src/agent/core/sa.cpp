/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003-2014 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be usefu,,
** but ITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: sa.cpp
**
**/

#include "nxagentd.h"

/**
 * Session agent list
 */
static ObjectArray<SessionAgentConnector> s_agents(8, 8, false);
static RWLOCK s_lock = RWLockCreate();

/**
 * Register session agent
 */
static void RegisterSessionAgent(SessionAgentConnector *c)
{
   RWLockWriteLock(s_lock, INFINITE);
   s_agents.add(c);
   RWLockUnlock(s_lock);
}

/**
 * Unregister session agent
 */
static void UnregisterSessionAgent(UINT32 id)
{
   RWLockWriteLock(s_lock, INFINITE);
   for(int i = 0; i < s_agents.size(); i++)
   {
      if (s_agents.get(i)->getId() == id)
      {
         s_agents.remove(i);
         break;
      }
   }
   RWLockUnlock(s_lock);
}

/**
 * Session agent connector read thread
 */
THREAD_RESULT THREAD_CALL SessionAgentConnector::readThreadStarter(void *arg)
{
   ((SessionAgentConnector *)arg)->readThread();

   // When SessionAgentConnector::ReadThread exits, all other
   // threads are already stopped, so we can safely destroy
   // session object
   UnregisterSessionAgent(((SessionAgentConnector *)arg)->getId());
   ((SessionAgentConnector *)arg)->decRefCount();
   return THREAD_OK;
}

/**
 * Connector constructor
 */
SessionAgentConnector::SessionAgentConnector(UINT32 id, SOCKET s)
{
   m_id = id;
   m_socket = s;
   m_sessionId = 0;
   m_sessionName = NULL;
   m_sessionState = USER_SESSION_OTHER;
   m_userName = NULL;
   m_mutex = MutexCreate();
   m_requestId = 0;
}

/**
 * Connector destructor
 */
SessionAgentConnector::~SessionAgentConnector()
{
   MutexDestroy(m_mutex);
   closesocket(m_socket);
   safe_free(m_sessionName);
   safe_free(m_userName);
}

/**
 * Start all threads
 */
void SessionAgentConnector::run()
{
   ThreadCreate(readThreadStarter, 0, this);
}

/**
 * Disconnect session
 */
void SessionAgentConnector::disconnect()
{
   shutdown(m_socket, SHUT_RDWR);
}

/**
 * Send message
 */
bool SessionAgentConnector::sendMessage(NXCPMessage *msg)
{
   NXCP_MESSAGE *rawMsg = msg->createMessage();
   bool success = (SendEx(m_socket, rawMsg, ntohl(rawMsg->size), 0, m_mutex) == ntohl(rawMsg->size));
   free(rawMsg);
   return success;
}

/**
 * Reading thread
 */
void SessionAgentConnector::readThread()
{
   NXCPEncryptionContext *dummyCtx = NULL;
   RecvNXCPMessage(0, NULL, &m_msgBuffer, 0, NULL, NULL, 0);
   UINT32 rawMsgSize = 65536;
   NXCP_MESSAGE *rawMsg = (NXCP_MESSAGE *)malloc(rawMsgSize);
   while(1)
   {
      int err = RecvNXCPMessageEx(m_socket, &rawMsg, &m_msgBuffer, &rawMsgSize, &dummyCtx, NULL, 300000, 4 * 1024 * 1024);
      if (err <= 0)
         break;

      // Check if message is too large
      if (err == 1)
         continue;

      // Check for decryption failure
      if (err == 2)
         continue;

      // Check for timeout
      if (err == 3)
      {
         DebugPrintf(INVALID_INDEX, 5, _T("Session agent connector %d stopped by timeout"), m_id);
         break;
      }

      // Check that actual received packet size is equal to encoded in packet
      if ((int)ntohl(rawMsg->size) != err)
      {
         DebugPrintf(INVALID_INDEX, 5, _T("Session agent connector %d: actual message size doesn't match wSize value (%d,%d)"), m_id, err, ntohl(rawMsg->size));
         continue;   // Bad packet, wait for next
      }

      if (g_debugLevel >= 8)
      {
         String msgDump = NXCPMessage::dump(rawMsg, NXCP_VERSION);
         DebugPrintf(INVALID_INDEX, 8, _T("SA-%d: Message dump:\n%s"), m_id, (const TCHAR *)msgDump);
      }

      UINT16 flags = ntohs(rawMsg->flags);
      if (!(flags & MF_BINARY))
      {
         // Create message object from raw message
         NXCPMessage *msg = new NXCPMessage(rawMsg);
         if (msg->getCode() == CMD_LOGIN)
         {
            m_sessionId = msg->getFieldAsUInt32(VID_SESSION_ID);
            m_sessionState = msg->getFieldAsInt16(VID_SESSION_STATE);

            safe_free(m_sessionName);
            m_sessionName = msg->getFieldAsString(VID_NAME);

            safe_free(m_userName);
            m_userName = msg->getFieldAsString(VID_USER_NAME);

            delete msg;
            DebugPrintf(INVALID_INDEX, 5, _T("Session agent connector %d: login as %s@%s [%d]"), m_id, getUserName(), getSessionName(), m_sessionId);
         }
         else
         {
            m_msgQueue.put(msg);
         }
      }
   }
   free(rawMsg);

   DebugPrintf(INVALID_INDEX, 5, _T("Session agent connector %d stopped"), m_id);
}

/**
 * Test connection with session agent
 */
bool SessionAgentConnector::testConnection()
{
   NXCPMessage msg;

   msg.setCode(CMD_KEEPALIVE);
   msg.setId(nextRequestId());
   if (!sendMessage(&msg))
      return false;

   NXCPMessage *response = m_msgQueue.waitForMessage(CMD_REQUEST_COMPLETED, msg.getId(), 5000);
   if (response == NULL)
      return false;

   delete response;
   return true;
}

/**
 * Take screenshot via session agent
 */
void SessionAgentConnector::takeScreenshot(NXCPMessage *masterResponse)
{
   NXCPMessage msg;

   msg.setCode(CMD_TAKE_SCREENSHOT);
   msg.setId(nextRequestId());
   if (!sendMessage(&msg))
   {
      masterResponse->setField(VID_RCC, ERR_CONNECTION_BROKEN);
      return;
   }

   NXCPMessage *response = m_msgQueue.waitForMessage(CMD_REQUEST_COMPLETED, msg.getId(), 5000);
   if (response == NULL)
   {
      masterResponse->setField(VID_RCC, ERR_REQUEST_TIMEOUT);
      return;
   }

   UINT32 rcc = response->getFieldAsUInt32(VID_RCC);
   if (rcc == ERR_SUCCESS)
   {
      masterResponse->setField(VID_RCC, ERR_SUCCESS);
      size_t imageSize;
      BYTE *image = response->getBinaryFieldPtr(VID_FILE_DATA, &imageSize);
      if (image != NULL)
         masterResponse->setField(VID_FILE_DATA, image, (UINT32)imageSize);
   }
   else
   {
      masterResponse->setField(VID_RCC, rcc);
   }

   delete response;
}

/**
 * Session agent listener thread
 */
static THREAD_RESULT THREAD_CALL SessionAgentListener(void *arg)
{
   SOCKET hSocket, hClientSocket;
   struct sockaddr_in servAddr;
   int iNumErrors = 0, nRet;
   socklen_t iSize;
   TCHAR szBuffer[256];
   struct timeval tv;
   fd_set rdfs;
   UINT32 id = 1;

   // Create socket
   if ((hSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
   {
      nxlog_write(MSG_SOCKET_ERROR, EVENTLOG_ERROR_TYPE, "e", WSAGetLastError());
      DebugPrintf(INVALID_INDEX, 1, _T("Session agent connector terminated (socket error)"));
      return THREAD_OK;
   }

	SetSocketExclusiveAddrUse(hSocket);
	SetSocketReuseFlag(hSocket);
#ifndef _WIN32
   fcntl(hSocket, F_SETFD, fcntl(hSocket, F_GETFD) | FD_CLOEXEC);
#endif

   // Fill in local address structure
   memset(&servAddr, 0, sizeof(struct sockaddr_in));
   servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
   servAddr.sin_port = htons(g_sessionAgentPort);

   // Bind socket
	DebugPrintf(INVALID_INDEX, 1, _T("Trying to bind on %s:%d"), IpToStr(ntohl(servAddr.sin_addr.s_addr), szBuffer), ntohs(servAddr.sin_port));
   if (bind(hSocket, (struct sockaddr *)&servAddr, sizeof(struct sockaddr_in)) != 0)
   {
      nxlog_write(MSG_BIND_ERROR, EVENTLOG_ERROR_TYPE, "e", WSAGetLastError());
      DebugPrintf(INVALID_INDEX, 1, _T("Session agent connector terminated (socket error)"));
      return THREAD_OK;
   }

   // Set up queue
   listen(hSocket, SOMAXCONN);
   DebugPrintf(INVALID_INDEX, 1, _T("Session agent connector listening on port %d"), (int)g_sessionAgentPort);

   // Wait for connection requests
   while(!(g_dwFlags & AF_SHUTDOWN))
   {
      tv.tv_sec = 1;
      tv.tv_usec = 0;
      FD_ZERO(&rdfs);
      FD_SET(hSocket, &rdfs);
      nRet = select(SELECT_NFDS(hSocket + 1), &rdfs, NULL, NULL, &tv);
      if ((nRet > 0) && (!(g_dwFlags & AF_SHUTDOWN)))
      {
         iSize = sizeof(struct sockaddr_in);
         if ((hClientSocket = accept(hSocket, (struct sockaddr *)&servAddr, &iSize)) == -1)
         {
            int error = WSAGetLastError();

            if (error != WSAEINTR)
               nxlog_write(MSG_ACCEPT_ERROR, EVENTLOG_ERROR_TYPE, "e", error);
            iNumErrors++;
            if (iNumErrors > 1000)
            {
               nxlog_write(MSG_TOO_MANY_ERRORS, EVENTLOG_WARNING_TYPE, NULL);
               iNumErrors = 0;
            }
            ThreadSleepMs(500);
            continue;
         }

         // Socket should be closed on successful exec
#ifndef _WIN32
         fcntl(hClientSocket, F_SETFD, fcntl(hClientSocket, F_GETFD) | FD_CLOEXEC);
#endif

         iNumErrors = 0;     // Reset consecutive errors counter
         DebugPrintf(INVALID_INDEX, 5, _T("Incoming session agent connection"));

         // Create new session structure and threads
		   SessionAgentConnector *c = new SessionAgentConnector(id++, hClientSocket);
         RegisterSessionAgent(c);
         c->run();
      }
      else if (nRet == -1)
      {
         int error = WSAGetLastError();

         // On AIX, select() returns ENOENT after SIGINT for unknown reason
#ifdef _WIN32
         if (error != WSAEINTR)
#else
         if ((error != EINTR) && (error != ENOENT))
#endif
         {
            nxlog_write(MSG_SELECT_ERROR, EVENTLOG_ERROR_TYPE, "e", error);
            ThreadSleepMs(100);
         }
      }
   }

   closesocket(hSocket);
   DebugPrintf(INVALID_INDEX, 1, _T("Session agent connector thread terminated"));
   return THREAD_OK;
}

/**
 * Session agent listener thread
 */
static THREAD_RESULT THREAD_CALL SessionAgentWatchdog(void *arg)
{
   while(!(g_dwFlags & AF_SHUTDOWN))
   {
      RWLockReadLock(s_lock, INFINITE);

      for(int i = 0; i < s_agents.size(); i++)
      {
         SessionAgentConnector *c = s_agents.get(i);
         if (!c->testConnection())
         {
            DebugPrintf(INVALID_INDEX, 5, _T("Session agent connector %d failed connection test"), c->getId());
            c->disconnect();
         }
      }

      RWLockUnlock(s_lock);

      ThreadSleep(30);
   }
   return THREAD_OK;
}

/**
 * Start session agent connector
 */
void StartSessionAgentConnector()
{
   if (g_sessionAgentPort != 0)
   {
	   ThreadCreate(SessionAgentListener, 0, NULL);
	   ThreadCreate(SessionAgentWatchdog, 0, NULL);
   }
   else
   {
      DebugPrintf(INVALID_INDEX, 1, _T("Session agent connector disabled"));
   }
}

/**
 * Acquire session agent connector
 */
SessionAgentConnector *AcquireSessionAgentConnector(const TCHAR *sessionName)
{
   SessionAgentConnector *c = NULL;

   RWLockReadLock(s_lock, INFINITE);
   for(int i = 0; i < s_agents.size(); i++)
   {
      if (!_tcsicmp(s_agents.get(i)->getSessionName(), sessionName))
      {
         c = s_agents.get(i);
         c->incRefCount();
         break;
      }
   }
   RWLockUnlock(s_lock);

   return c;
}

/**
 * Get table of registered session agents
 */
LONG H_SessionAgents(const TCHAR *cmd, const TCHAR *arg, Table *value)
{
   value->addColumn(_T("SESSION_ID"), DCI_DT_UINT, _T("Session ID"), true);
   value->addColumn(_T("SESSION_NAME"), DCI_DT_STRING, _T("Session"));
   value->addColumn(_T("USER_NAME"), DCI_DT_STRING, _T("User"));
   value->addColumn(_T("STATE"), DCI_DT_INT, _T("State"));

   RWLockReadLock(s_lock, INFINITE);
   for(int i = 0; i < s_agents.size(); i++)
   {
      SessionAgentConnector *c = s_agents.get(i);
      value->addRow();
      value->set(0, c->getSessionId());
      value->set(1, c->getSessionName());
      value->set(2, c->getUserName());
      value->set(3, c->getSessionState());
   }
   RWLockUnlock(s_lock);

   return SYSINFO_RC_SUCCESS;
}
