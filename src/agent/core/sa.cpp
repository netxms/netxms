/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003-2020 Victor Kirhenshtein
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
static ObjectArray<SessionAgentConnector> s_agents(8, 8, Ownership::False);
static RWLOCK s_lock = RWLockCreate();
static int s_sessionAgentCount = 0;
static int s_userAgentCount = 0;

/**
 * List of active user agent notifications
 */
static HashMap<ServerObjectKey, UserAgentNotification> s_userAgentNotifications(Ownership::True);
static Mutex s_userAgentNotificationsLock;

/**
 * Register session agent
 */
static void RegisterSessionAgent(SessionAgentConnector *newConnector)
{
   RWLockWriteLock(s_lock);
   bool idFound = false;
   for (int i = 0; i < s_agents.size(); i++)
   {
      if (s_agents.get(i)->getId() == newConnector->getId())
      {
         idFound = true;
         break;
      }
   }
   if (!idFound)
   {
      s_agents.add(newConnector);
      s_sessionAgentCount++;
      if (newConnector->isUserAgent())
      {
         s_userAgentCount++;

         // Stop session agent if it is running in same session as user agent
         for (int i = 0; i < s_agents.size(); i++)
         {
            SessionAgentConnector *curr = s_agents.get(i);
            if ((curr->getSessionId() == newConnector->getSessionId()) && !curr->isUserAgent())
            {
               nxlog_debug(4, _T("RegisterSessionAgent: forcing shutdown of session agent in session %s"), curr->getSessionName());
               curr->shutdown(false);
            }
         }
      }
      else
      {
         // Stop this agent if it is running in same session as user agent
         bool userAgentFound = false;
         for (int i = 0; i < s_agents.size(); i++)
         {
            SessionAgentConnector *curr = s_agents.get(i);
            if ((curr->getSessionId() == newConnector->getSessionId()) && curr->isUserAgent())
            {
               userAgentFound = true;
               break;
            }
         }
         if (userAgentFound)
         {
            nxlog_debug(4, _T("RegisterSessionAgent: forcing shutdown of session agent in session %s"), newConnector->getSessionName());
            newConnector->shutdown(false);
         }
      }
   }
   RWLockUnlock(s_lock);
}

/**
 * Unregister session agent
 */
static void UnregisterSessionAgent(UINT32 id)
{
   RWLockWriteLock(s_lock);
   for(int i = 0; i < s_agents.size(); i++)
   {
      if (s_agents.get(i)->getId() == id)
      {
         s_sessionAgentCount--;
         if (s_agents.get(i)->isUserAgent())
            s_userAgentCount--;
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
   static_cast<SessionAgentConnector*>(arg)->readThread();

   // When SessionAgentConnector::ReadThread exits, all other
   // threads are already stopped, so we can safely destroy
   // session object
   UnregisterSessionAgent(static_cast<SessionAgentConnector*>(arg)->getId());
   static_cast<SessionAgentConnector*>(arg)->decRefCount();
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
   m_clientName = NULL;
   m_processId = 0;
   m_userAgent = false;
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
   MemFree(m_sessionName);
   MemFree(m_userName);
   MemFree(m_clientName);
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
   ::shutdown(m_socket, SHUT_RDWR);
}

/**
 * Send message
 */
bool SessionAgentConnector::sendMessage(const NXCPMessage *msg)
{
   NXCP_MESSAGE *rawMsg = msg->serialize();
   bool success = (SendEx(m_socket, rawMsg, ntohl(rawMsg->size), 0, m_mutex) == ntohl(rawMsg->size));
   MemFree(rawMsg);
   return success;
}

/**
 * Reading thread
 */
void SessionAgentConnector::readThread()
{
   NXCPEncryptionContext *dummyCtx = NULL;
   NXCPInitBuffer(&m_msgBuffer);
   UINT32 rawMsgSize = 65536;
   NXCP_MESSAGE *rawMsg = (NXCP_MESSAGE *)malloc(rawMsgSize);
   while(1)
   {
      ssize_t err = RecvNXCPMessageEx(m_socket, &rawMsg, &m_msgBuffer, &rawMsgSize, &dummyCtx, NULL, 300000, 4 * 1024 * 1024);
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
         DebugPrintf(5, _T("Session agent connector %d stopped by timeout"), m_id);
         break;
      }

      // Check that actual received packet size is equal to encoded in packet
      if ((int)ntohl(rawMsg->size) != err)
      {
         DebugPrintf(5, _T("SA-%d: actual message size doesn't match wSize value (%d,%d)"), m_id, err, ntohl(rawMsg->size));
         continue;   // Bad packet, wait for next
      }

      if (nxlog_get_debug_level() >= 8)
      {
         String msgDump = NXCPMessage::dump(rawMsg, NXCP_VERSION);
         DebugPrintf(8, _T("SA-%d: Message dump:\n%s"), m_id, (const TCHAR *)msgDump);
      }

      UINT16 flags = ntohs(rawMsg->flags);
      if (!(flags & MF_BINARY))
      {
         // Create message object from raw message
         NXCPMessage *msg = NXCPMessage::deserialize(rawMsg);
         if (msg != NULL)
         {
            if (msg->getCode() == CMD_LOGIN)
            {
               m_processId = msg->getFieldAsUInt32(VID_PROCESS_ID);
               m_sessionId = msg->getFieldAsUInt32(VID_SESSION_ID);
               m_sessionState = msg->getFieldAsInt16(VID_SESSION_STATE);
               m_userAgent = msg->getFieldAsBoolean(VID_USERAGENT);
               msg->getFieldAsString(VID_NAME, &m_sessionName);
               msg->getFieldAsString(VID_USER_NAME, &m_userName);
               msg->getFieldAsString(VID_CLIENT_INFO, &m_clientName);

               delete msg;
               DebugPrintf(5, _T("Session agent connector %d: login as %s@%s [%d] (%s client)"), 
                  m_id, getUserName(), getSessionName(), m_sessionId, m_userAgent ? _T("extended") : _T("basic"));

               RegisterSessionAgent(this);

               if (m_userAgent)
               {
                  updateUserAgentConfig();

                  s_userAgentNotificationsLock.lock();
                  updateUserAgentNotifications();
                  s_userAgentNotificationsLock.unlock();
               }
            }
            else
            {
               m_msgQueue.put(msg);
            }
         }
         else
         {
            DebugPrintf(5, _T("SA-%d: message deserialization error"), m_id);
         }
      }
   }
   MemFree(rawMsg);

   DebugPrintf(5, _T("Session agent connector %d stopped"), m_id);
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
 * Shutdown session agent with optional delayed restart
 */
void SessionAgentConnector::shutdown(bool restart)
{
   NXCPMessage msg;
   msg.setCode(CMD_SHUTDOWN);
   msg.setId(nextRequestId());
   msg.setField(VID_RESTART, restart);
   sendMessage(&msg);
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
      const BYTE *image = response->getBinaryFieldPtr(VID_FILE_DATA, &imageSize);
      if (image != NULL)
      {
         masterResponse->disableCompression();  // image is already compressed
         masterResponse->setField(VID_FILE_DATA, image, imageSize);
      }
   }
   else
   {
      masterResponse->setField(VID_RCC, rcc);
   }

   delete response;
}

/**
 * Get screen information via session agent
 */
bool SessionAgentConnector::getScreenInfo(uint32_t *width, uint32_t *height, uint32_t *bpp)
{
   NXCPMessage msg(CMD_GET_SCREEN_INFO, nextRequestId());
   if (!sendMessage(&msg))
      return false;

   NXCPMessage *response = m_msgQueue.waitForMessage(CMD_REQUEST_COMPLETED, msg.getId(), 5000);
   if (response == nullptr)
      return false;

   *width = response->getFieldAsUInt32(VID_SCREEN_WIDTH);
   *height = response->getFieldAsUInt32(VID_SCREEN_HEIGHT);
   *bpp = response->getFieldAsUInt32(VID_SCREEN_BPP);

   delete response;
   return true;
}

/**
 * Config merge strategy for support application
 */
static ConfigEntry *SupportAppMergeStrategy(ConfigEntry *parent, const TCHAR *name)
{
   if (!_tcsicmp(name, _T("item")))
      return NULL;
   return parent->findEntry(name);
}

/**
 * Send configuration for user agent
 */
void SessionAgentConnector::updateUserAgentConfig()
{
   Config config;
   config.setTopLevelTag(_T("SupportAppPolicy"));
   config.setMergeStrategy(SupportAppMergeStrategy);
   if (config.loadConfigDirectory(g_userAgentPolicyDirectory, _T("SupportAppPolicy"), "SupportAppPolicy", true, true))
   {
      shared_ptr<Config> agentConfig = g_config;
      const ConfigEntry *e = agentConfig->getEntry(_T("/UserAgent"));
      if (e != NULL)
      {
         nxlog_debug(5, _T("SA-%d: adding local policy to user agent configuration"), m_id);
         config.addSubTree(_T("/"), e);
      }

      NXCPMessage msg(CMD_UPDATE_AGENT_CONFIG, nextRequestId());
      msg.setField(VID_CONFIG_FILE_DATA, config.createXml());
      msg.setField(VID_FILE_STORE, g_szFileStore);
      sendMessage(&msg);
   }
   else
   {
      nxlog_debug(4, _T("SA-%d: cannot load user agent configuration from %s"), m_id, g_userAgentPolicyDirectory);
   }
}

/**
 * Send all user agent notifications (assumes that lock on notification list already acquired)
 */
void SessionAgentConnector::updateUserAgentNotifications()
{
   NXCPMessage msg;
   msg.setCode(CMD_UPDATE_UA_NOTIFICATIONS);
   msg.setId(nextRequestId());

   UINT32 count = 0, baseId = VID_UA_NOTIFICATION_BASE;
   Iterator<UserAgentNotification> *it = s_userAgentNotifications.iterator();
   while (it->hasNext())
   {
      UserAgentNotification *n = it->next();
      n->fillMessage(&msg, baseId);
      baseId += 10;
      count++;
   }
   msg.setField(VID_UA_NOTIFICATION_COUNT, count);
   sendMessage(&msg);
}

/**
 * Add new notification
 */
void SessionAgentConnector::addUserAgentNotification(UserAgentNotification *n)
{
   NXCPMessage msg;
   msg.setCode(CMD_ADD_UA_NOTIFICATION);
   msg.setId(nextRequestId());
   n->fillMessage(&msg, VID_UA_NOTIFICATION_BASE);
   sendMessage(&msg);
}

/**
 * Remove specific notification
 */
void SessionAgentConnector::removeUserAgentNotification(const ServerObjectKey& id)
{
   NXCPMessage msg;
   msg.setCode(CMD_RECALL_UA_NOTIFICATION);
   msg.setId(nextRequestId());
   msg.setField(VID_UA_NOTIFICATION_BASE, id.objectId);
   msg.setField(VID_UA_NOTIFICATION_BASE + 1, id.serverId);
   sendMessage(&msg);
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
   UINT32 id = 1;

   // Create socket
   if ((hSocket = CreateSocket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
   {
      TCHAR buffer[1024];
      nxlog_write(NXLOG_ERROR, _T("Unable to open socket (%s)"), GetLastSocketErrorText(buffer, 1024));
      nxlog_debug(1, _T("Session agent connector terminated (socket error)"));
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
	DebugPrintf(1, _T("Trying to bind on %s:%d"), IpToStr(ntohl(servAddr.sin_addr.s_addr), szBuffer), ntohs(servAddr.sin_port));
   if (bind(hSocket, (struct sockaddr *)&servAddr, sizeof(struct sockaddr_in)) != 0)
   {
      TCHAR buffer[1024];
      nxlog_write(NXLOG_ERROR, _T("Unable to bind socket (%s)"), GetLastSocketErrorText(buffer, 1024));
      nxlog_debug(1, _T("Session agent connector terminated (socket error)"));
      return THREAD_OK;
   }

   // Set up queue
   listen(hSocket, SOMAXCONN);
   nxlog_debug(1, _T("Session agent connector listening on port %d"), (int)g_sessionAgentPort);

   // Wait for connection requests
   SocketPoller sp;
   while(!(g_dwFlags & AF_SHUTDOWN))
   {
      sp.reset();
      sp.add(hSocket);
      nRet = sp.poll(1000);
      if ((nRet > 0) && (!(g_dwFlags & AF_SHUTDOWN)))
      {
         iSize = sizeof(struct sockaddr_in);
         if ((hClientSocket = accept(hSocket, (struct sockaddr *)&servAddr, &iSize)) == -1)
         {
            int error = WSAGetLastError();

            if (error != WSAEINTR)
            {
               TCHAR buffer[1024];
               nxlog_write(NXLOG_ERROR, _T("Unable to accept incoming connection (%s)"), GetLastSocketErrorText(buffer, 1024));
            }
            iNumErrors++;
            if (iNumErrors > 1000)
            {
               nxlog_write(NXLOG_WARNING, _T("Too many consecutive errors on accept() call"));
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
         nxlog_debug(5, _T("Incoming session agent connection"));

         // Create new session structure and threads
		   SessionAgentConnector *c = new SessionAgentConnector(id++, hClientSocket);
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
            TCHAR buffer[1024];
            nxlog_write(NXLOG_ERROR, _T("Call to select() failed (%s)"), GetLastSocketErrorText(buffer, 1024));
            ThreadSleepMs(100);
         }
      }
   }

   closesocket(hSocket);
   DebugPrintf(1, _T("Session agent connector thread terminated"));
   return THREAD_OK;
}

/**
 * Session agent listener thread
 */
static THREAD_RESULT THREAD_CALL SessionAgentWatchdog(void *arg)
{
   while(!(g_dwFlags & AF_SHUTDOWN))
   {
      RWLockReadLock(s_lock);

      for(int i = 0; i < s_agents.size(); i++)
      {
         SessionAgentConnector *c = s_agents.get(i);
         if (!c->testConnection())
         {
            DebugPrintf(5, _T("Session agent connector %d failed connection test"), c->getId());
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
      DebugPrintf(1, _T("Session agent connector disabled"));
   }
}

/**
 * Acquire session agent connector
 */
SessionAgentConnector *AcquireSessionAgentConnector(const TCHAR *sessionName)
{
   SessionAgentConnector *c = NULL;

   RWLockReadLock(s_lock);
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
 * Acquire session agent connector
 */
SessionAgentConnector *AcquireSessionAgentConnector(uint32_t sessionId)
{
   SessionAgentConnector *c = NULL;

   RWLockReadLock(s_lock);
   for (int i = 0; i < s_agents.size(); i++)
   {
      if (s_agents.get(i)->getSessionId() == sessionId)
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
LONG H_SessionAgents(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   value->addColumn(_T("SESSION_ID"), DCI_DT_UINT, _T("Session ID"), true);
   value->addColumn(_T("SESSION_NAME"), DCI_DT_STRING, _T("Session"));
   value->addColumn(_T("USER_NAME"), DCI_DT_STRING, _T("User"));
   value->addColumn(_T("CLIENT_NAME"), DCI_DT_STRING, _T("Client"));
   value->addColumn(_T("STATE"), DCI_DT_INT, _T("State"));
   value->addColumn(_T("AGENT_TYPE"), DCI_DT_INT, _T("Agent type"));
   value->addColumn(_T("AGENT_PID"), DCI_DT_INT, _T("Agent PID"));

   RWLockReadLock(s_lock);
   for(int i = 0; i < s_agents.size(); i++)
   {
      SessionAgentConnector *c = s_agents.get(i);
      value->addRow();
      value->set(0, c->getSessionId());
      value->set(1, c->getSessionName());
      value->set(2, c->getUserName());
      value->set(3, c->getClientName());
      value->set(4, c->getSessionState());
      value->set(5, c->isUserAgent() ? 1 : 0);
      value->set(6, c->getProcessId());
   }
   RWLockUnlock(s_lock);

   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for number of connected session agents
 */
LONG H_SessionAgentCount(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   ret_int(value, (*arg == _T('U')) ? s_userAgentCount : s_sessionAgentCount);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Update configuration on all connected user agents
 */
void UpdateUserAgentsConfiguration()
{
   RWLockReadLock(s_lock);
   for (int i = 0; i < s_agents.size(); i++)
   {
      SessionAgentConnector *c = s_agents.get(i);
      if (c->isUserAgent())
         c->updateUserAgentConfig();
   }
   RWLockUnlock(s_lock);
}

/**
 * Shutdown session agent with optional delayed restart
 */
void ShutdownSessionAgents(bool restart)
{
   RWLockReadLock(s_lock);
   for (int i = 0; i < s_agents.size(); i++)
   {
      s_agents.get(i)->shutdown(restart);
   }
   RWLockUnlock(s_lock);
}

/**
 * Check if user agent component is installed
 */
bool IsUserAgentInstalled()
{
#ifdef _WIN32
   HKEY hKey;
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\NetXMS\\Agent"), 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
      return false;

   DWORD size = sizeof(DWORD);
   DWORD value = 0;
   RegQueryValueEx(hKey, _T("WithUserAgent"), NULL, NULL, (BYTE *)&value, &size);
   RegCloseKey(hKey);

   return value != 0;
#else
   return false;
#endif
}

/**
 * Notify on new user agent message
 */
UINT32 AddUserAgentNotification(UINT64 serverId, NXCPMessage *request)
{
   UserAgentNotification *n = new UserAgentNotification(serverId, request, VID_UA_NOTIFICATION_BASE);
   DB_HANDLE db = GetLocalDatabaseHandle();

   s_userAgentNotificationsLock.lock();
   if (n->getStartTime() != 0)
   {
      s_userAgentNotifications.set(n->getId(), n);
      n->saveToDatabase(db);
   }

   RWLockReadLock(s_lock);
   for (int i = 0; i < s_agents.size(); i++)
   {
      SessionAgentConnector *c = s_agents.get(i);
      if (c->isUserAgent())
         c->addUserAgentNotification(n);
   }
   RWLockUnlock(s_lock);

   if (n->getStartTime() == 0)
      delete n;

   s_userAgentNotificationsLock.unlock();

   return RCC_SUCCESS;
}

/**
 * Remove user agent notification
 */
UINT32 RemoveUserAgentNotification(UINT64 serverId, NXCPMessage *request)
{
   ServerObjectKey id = ServerObjectKey(serverId, request->getFieldAsUInt32(VID_UA_NOTIFICATION_BASE));
   
   s_userAgentNotificationsLock.lock();
   s_userAgentNotifications.remove(id);
   s_userAgentNotificationsLock.unlock();

   TCHAR query[256];
   _sntprintf(query, 256, _T("DELETE FROM user_agent_notifications WHERE server_id=") INT64_FMT _T(" AND notification_id=%u"), serverId, id.objectId);
   DB_HANDLE db = GetLocalDatabaseHandle();
   DBQuery(db, query);

   RWLockReadLock(s_lock);
   for (int i = 0; i < s_agents.size(); i++)
   {
      SessionAgentConnector *c = s_agents.get(i);
      if (c->isUserAgent())
         c->removeUserAgentNotification(id);
   }
   RWLockUnlock(s_lock);

   return RCC_SUCCESS;
}

/**
 * Update complete list of user agent notifications
 */
UINT32 UpdateUserAgentNotifications(UINT64 serverId, NXCPMessage *request)
{
   DB_HANDLE db = GetLocalDatabaseHandle();

   s_userAgentNotificationsLock.lock();

   // FIXME: only delete notifications from current server
   s_userAgentNotifications.clear();

   TCHAR query[256];
   _sntprintf(query, sizeof(query) / sizeof(TCHAR), _T("DELETE FROM user_agent_notifications WHERE server_id=") INT64_FMT, serverId);
   DBQuery(db, query);

   int count = request->getFieldAsInt32(VID_UA_NOTIFICATION_COUNT);
   int baseId = VID_UA_NOTIFICATION_BASE;
   for (int i = 0; i < count; i++, baseId += 10)
   {
      UserAgentNotification *n = new UserAgentNotification(serverId, request, baseId);
      s_userAgentNotifications.set(n->getId(), n);
      n->saveToDatabase(db);
   }

   RWLockReadLock(s_lock);
   for (int i = 0; i < s_agents.size(); i++)
   {
      SessionAgentConnector *c = s_agents.get(i);
      if (c->isUserAgent())
         c->updateUserAgentNotifications();
   }
   RWLockUnlock(s_lock);

   s_userAgentNotificationsLock.unlock();

   return RCC_SUCCESS;
}

/**
 * Load user agent notifications from local database
 */
void LoadUserAgentNotifications()
{
   DB_HANDLE db = GetLocalDatabaseHandle();
   if (db == NULL)
   {
      nxlog_write(NXLOG_WARNING, _T("Cannot load user agent notifications (local database is unavailable)"));
      return;
   }

   TCHAR query[1024];
   _sntprintf(query, 1024, _T("SELECT server_id,notification_id,message,start_time,end_time,on_startup FROM user_agent_notifications"));

   DB_RESULT hResult = DBSelect(db, query);
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UserAgentNotification *n = 
            new UserAgentNotification(DBGetFieldUInt64(hResult, i, 0),
                  DBGetFieldULong(hResult, i, 1),
                  DBGetField(hResult, i, 2, NULL, 0),
                  (time_t)DBGetFieldInt64(hResult, i, 3),
                  (time_t)DBGetFieldInt64(hResult, i, 4),
                  DBGetFieldULong(hResult, i, 5) ? true : false);

         s_userAgentNotifications.set(n->getId(), n);
      }
      DBFreeResult(hResult);
   }
}

/**
 * Get screen information for session
 */
bool GetScreenInfoForUserSession(uint32_t sessionId, uint32_t *width, uint32_t *height, uint32_t *bpp)
{
   SessionAgentConnector *connector = AcquireSessionAgentConnector(sessionId);
   if (connector == nullptr)
      return false;

   bool success = connector->getScreenInfo(width, height, bpp);
   connector->decRefCount();
   return success;
}
