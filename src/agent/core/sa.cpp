/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003-2024 Victor Kirhenshtein
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

#define DEBUG_TAG _T("sa")

/**
 * Session agent list
 */
static SharedObjectArray<SessionAgentConnector> s_agents(8, 8);
static RWLock s_lock;
static int s_sessionAgentCount = 0;
static int s_userAgentCount = 0;

/**
 * List of active user support application notifications
 */
static HashMap<ServerObjectKey, UserAgentNotification> s_userAgentNotifications(Ownership::True);
static Mutex s_userAgentNotificationsLock;

/**
 * Register session agent
 */
static void RegisterSessionAgent(const shared_ptr<SessionAgentConnector>& newConnector)
{
   s_lock.writeLock();
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
               nxlog_debug_tag(DEBUG_TAG, 4, _T("RegisterSessionAgent: forcing shutdown of session agent in session %s"), curr->getSessionName());
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
            nxlog_debug_tag(DEBUG_TAG, 4, _T("RegisterSessionAgent: forcing shutdown of session agent in session %s"), newConnector->getSessionName());
            newConnector->shutdown(false);
         }
      }
   }
   s_lock.unlock();
}

/**
 * Unregister session agent
 */
static void UnregisterSessionAgent(uint32_t id)
{
   s_lock.writeLock();
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
   s_lock.unlock();
}

/**
 * Connector constructor
 */
SessionAgentConnector::SessionAgentConnector(uint32_t id, SOCKET s) : m_mutex(MutexType::FAST)
{
   m_id = id;
   m_socket = s;
   m_sessionId = 0;
   m_sessionName = nullptr;
   m_sessionState = USER_SESSION_OTHER;
   m_userName = nullptr;
   m_clientName = nullptr;
   m_processId = 0;
   m_userAgent = false;
   m_requestId = 0;
}

/**
 * Connector destructor
 */
SessionAgentConnector::~SessionAgentConnector()
{
   closesocket(m_socket);
   MemFree(m_sessionName);
   MemFree(m_userName);
   MemFree(m_clientName);
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
   bool success = (SendEx(m_socket, rawMsg, ntohl(rawMsg->size), 0, &m_mutex) == ntohl(rawMsg->size));
   MemFree(rawMsg);
   return success;
}

/**
 * Reading thread
 */
void SessionAgentConnector::readThread()
{
   SocketMessageReceiver receiver(m_socket, 32768, 4 * 1024 * 1024);
   while(true)
   {
      MessageReceiverResult result;
      NXCPMessage *msg = receiver.readMessage(300000, &result);
      if ((result == MSGRECV_CLOSED) || (result == MSGRECV_COMM_FAILURE) || (result == MSGRECV_TIMEOUT) || (result == MSGRECV_PROTOCOL_ERROR))
      {
         nxlog_debug_tag_object(DEBUG_TAG, m_id, 5, _T("Session agent connector %d stopped (%s)"), m_id, AbstractMessageReceiver::resultToText(result));
         break;
      }

      if (msg == nullptr)
         continue;   // For other errors, wait for next message

      if (nxlog_get_debug_level() >= 8)
      {
         NXCP_MESSAGE *rawMsg = msg->serialize(false);
         String msgDump = NXCPMessage::dump(rawMsg, NXCP_VERSION);
         nxlog_debug_tag_object(DEBUG_TAG, m_id, 8, _T("Message dump:\n%s"), msgDump.cstr());
         MemFree(rawMsg);
      }

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
         nxlog_debug_tag_object(DEBUG_TAG, m_id, 5, _T("Session agent connector %d: login as %s@%s [%d] (%s client)"),
            m_id, getUserName(), getSessionName(), m_sessionId, m_userAgent ? _T("extended") : _T("basic"));

         RegisterSessionAgent(self());

         if (m_userAgent)
         {
            updateUserAgentEnvironment();
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

   UnregisterSessionAgent(m_id);
   nxlog_debug_tag_object(DEBUG_TAG, m_id, 5, _T("Session agent connector %d unregistered"), m_id);
}

/**
 * Test connection with session agent
 */
bool SessionAgentConnector::testConnection()
{
   NXCPMessage msg(CMD_KEEPALIVE, nextRequestId());
   if (!sendMessage(&msg))
      return false;

   NXCPMessage *response = m_msgQueue.waitForMessage(CMD_REQUEST_COMPLETED, msg.getId(), 5000);
   if (response == nullptr)
      return false;

   delete response;
   return true;
}

/**
 * Shutdown session agent with optional delayed restart
 */
void SessionAgentConnector::shutdown(bool restart)
{
   NXCPMessage msg(CMD_SHUTDOWN, nextRequestId());
   msg.setField(VID_RESTART, restart);
   sendMessage(&msg);
}

/**
 * Take screenshot via session agent
 */
void SessionAgentConnector::takeScreenshot(NXCPMessage *masterResponse)
{
   NXCPMessage msg(CMD_TAKE_SCREENSHOT, nextRequestId());
   if (!sendMessage(&msg))
   {
      masterResponse->setField(VID_RCC, ERR_CONNECTION_BROKEN);
      return;
   }

   NXCPMessage *response = m_msgQueue.waitForMessage(CMD_REQUEST_COMPLETED, msg.getId(), 5000);
   if (response == nullptr)
   {
      masterResponse->setField(VID_RCC, ERR_REQUEST_TIMEOUT);
      return;
   }

   uint32_t rcc = response->getFieldAsUInt32(VID_RCC);
   if (rcc == ERR_SUCCESS)
   {
      masterResponse->setField(VID_RCC, ERR_SUCCESS);
      size_t imageSize;
      const BYTE *image = response->getBinaryFieldPtr(VID_FILE_DATA, &imageSize);
      if (image != nullptr)
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
      return nullptr;
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
      if (e != nullptr)
      {
         nxlog_debug_tag_object(DEBUG_TAG, m_id, 5, _T("Adding local policy to user agent configuration"));
         config.addSubTree(_T("/"), e);
      }

      NXCPMessage msg(CMD_WRITE_AGENT_CONFIG_FILE, nextRequestId());
      msg.setField(VID_CONFIG_FILE_DATA, config.createXml());
      msg.setField(VID_FILE_STORE, g_szFileStore);
      sendMessage(&msg);
   }
   else
   {
      nxlog_debug_tag_object(DEBUG_TAG, m_id, 4, _T("Cannot load user agent configuration from %s"), g_userAgentPolicyDirectory);
   }
}

/**
 * Send environment for user agent
 */
void SessionAgentConnector::updateUserAgentEnvironment()
{
   shared_ptr<Config> config = g_config;
   unique_ptr<ObjectArray<ConfigEntry>> entrySet = config->getSubEntries(_T("/ENV"), _T("*"));
   if (entrySet == nullptr)
      return;

   NXCPMessage msg(CMD_UPDATE_ENVIRONMENT, nextRequestId());
   msg.setField(VID_NUM_ELEMENTS, entrySet->size());

   uint32_t fieldId = VID_ELEMENT_LIST_BASE;
   for (int i = 0; i < entrySet->size(); i++)
   {
      ConfigEntry *e = entrySet->get(i);
      msg.setField(fieldId++, e->getName());
      msg.setField(fieldId++, e->getValue());
   }
   sendMessage(&msg);
}

/**
 * Send all user support application notifications (assumes that lock on notification list already acquired)
 */
void SessionAgentConnector::updateUserAgentNotifications()
{
   NXCPMessage msg(CMD_UPDATE_UA_NOTIFICATIONS, nextRequestId());

   uint32_t count = 0, baseId = VID_UA_NOTIFICATION_BASE;
   Iterator<UserAgentNotification> it = s_userAgentNotifications.begin();
   while (it.hasNext())
   {
      UserAgentNotification *n = it.next();
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
   NXCPMessage msg(CMD_ADD_UA_NOTIFICATION, nextRequestId());
   n->fillMessage(&msg, VID_UA_NOTIFICATION_BASE);
   sendMessage(&msg);
}

/**
 * Remove specific notification
 */
void SessionAgentConnector::removeUserAgentNotification(const ServerObjectKey& id)
{
   NXCPMessage msg(CMD_RECALL_UA_NOTIFICATION, nextRequestId());
   msg.setField(VID_UA_NOTIFICATION_BASE, id.objectId);
   msg.setField(VID_UA_NOTIFICATION_BASE + 1, id.serverId);
   sendMessage(&msg);
}

/**
 * Session agent listener thread
 */
static void SessionAgentListener()
{
   SOCKET hSocket, hClientSocket;
   struct sockaddr_in servAddr;
   int iNumErrors = 0, nRet;
   socklen_t iSize;
   TCHAR szBuffer[256];
   uint32_t id = 1;

   // Create socket
   if ((hSocket = CreateSocket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
   {
      TCHAR buffer[1024];
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Unable to open socket (%s)"), GetLastSocketErrorText(buffer, 1024));
      nxlog_debug_tag(DEBUG_TAG, 1, _T("Session agent connector terminated (socket error)"));
      return;
   }

	SetSocketExclusiveAddrUse(hSocket);
	SetSocketReuseFlag(hSocket);
#ifndef _WIN32
   fcntl(hSocket, F_SETFD, fcntl(hSocket, F_GETFD) | FD_CLOEXEC);
#endif

   // Fill in local address structure
   memset(&servAddr, 0, sizeof(struct sockaddr_in));
   servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(0x7F000001);
   servAddr.sin_port = htons(g_sessionAgentPort);

   // Bind socket
	nxlog_debug_tag(DEBUG_TAG, 1, _T("Trying to bind on %s:%d"), IpToStr(ntohl(servAddr.sin_addr.s_addr), szBuffer), ntohs(servAddr.sin_port));
   if (bind(hSocket, (struct sockaddr *)&servAddr, sizeof(struct sockaddr_in)) != 0)
   {
      TCHAR buffer[1024];
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Unable to bind socket (%s)"), GetLastSocketErrorText(buffer, 1024));
      nxlog_debug_tag(DEBUG_TAG, 1, _T("Session agent connector terminated (socket error)"));
      return;
   }

   // Set up queue
   listen(hSocket, SOMAXCONN);
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Session agent connector listening on port %d"), (int)g_sessionAgentPort);

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
               nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Unable to accept incoming connection (%s)"), GetLastSocketErrorText(buffer, 1024));
            }
            iNumErrors++;
            if (iNumErrors > 1000)
            {
               nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Too many consecutive errors on accept() call"));
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
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Incoming session agent connection"));

         // Create new session structure and threads
		   CreateSessionAgentConnector(id++, hClientSocket);
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
            nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Call to select() failed (%s)"), GetLastSocketErrorText(buffer, 1024));
            ThreadSleepMs(100);
         }
      }
   }

   closesocket(hSocket);
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Session agent connector thread terminated"));
}

/**
 * Session agent listener thread
 */
static void SessionAgentWatchdog()
{
   while(!(g_dwFlags & AF_SHUTDOWN))
   {
      s_lock.readLock();

      for(int i = 0; i < s_agents.size(); i++)
      {
         SessionAgentConnector *c = s_agents.get(i);
         if (!c->testConnection())
         {
            nxlog_debug_tag_object(DEBUG_TAG, c->getId(), 5, _T("Session agent connector %d failed connection test"), c->getId());
            c->disconnect();
         }
      }

      s_lock.unlock();

      ThreadSleep(30);
   }
}

/**
 * Start session agent connector
 */
void StartSessionAgentConnector()
{
   if (g_sessionAgentPort != 0)
   {
	   ThreadCreate(SessionAgentListener);
	   ThreadCreate(SessionAgentWatchdog);
   }
   else
   {
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Session agent connector disabled"));
   }
}

/**
 * Acquire session agent connector
 */
shared_ptr<SessionAgentConnector> AcquireSessionAgentConnector(const TCHAR *sessionName)
{
   shared_ptr<SessionAgentConnector> c;

   s_lock.readLock();
   for(int i = 0; i < s_agents.size(); i++)
   {
      if (!_tcsicmp(s_agents.get(i)->getSessionName(), sessionName))
      {
         c = s_agents.getShared(i);
         break;
      }
   }
   s_lock.unlock();

   return c;
}

/**
 * Acquire session agent connector
 */
shared_ptr<SessionAgentConnector> AcquireSessionAgentConnector(uint32_t sessionId)
{
   shared_ptr<SessionAgentConnector> c;

   s_lock.readLock();
   for (int i = 0; i < s_agents.size(); i++)
   {
      if (s_agents.get(i)->getSessionId() == sessionId)
      {
         c = s_agents.getShared(i);
         break;
      }
   }
   s_lock.unlock();

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

   s_lock.readLock();
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
   s_lock.unlock();

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
   s_lock.readLock();
   for (int i = 0; i < s_agents.size(); i++)
   {
      SessionAgentConnector *c = s_agents.get(i);
      if (c->isUserAgent())
         c->updateUserAgentConfig();
   }
   s_lock.unlock();
}

/**
 * Update environment on all connected user agents
 */
void UpdateUserAgentsEnvironment()
{
   s_lock.readLock();
   for (int i = 0; i < s_agents.size(); i++)
   {
      SessionAgentConnector *c = s_agents.get(i);
      if (c->isUserAgent())
         c->updateUserAgentEnvironment();
   }
   s_lock.unlock();
}

/**
 * Shutdown session agent with optional delayed restart
 */
void ShutdownSessionAgents(bool restart)
{
   s_lock.readLock();
   for (int i = 0; i < s_agents.size(); i++)
   {
      s_agents.get(i)->shutdown(restart);
   }
   s_lock.unlock();
}

/**
 * Check if user agent component is installed
 */
bool IsUserAgentInstalled()
{
#ifdef _WIN32
   if (g_config->getValueAsBoolean(_T("/CORE/ForceReportUserAgent"), false))
      return true;

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
uint32_t AddUserAgentNotification(uint64_t serverId, NXCPMessage *request)
{
   UserAgentNotification *n = new UserAgentNotification(serverId, request, VID_UA_NOTIFICATION_BASE);
   DB_HANDLE db = GetLocalDatabaseHandle();

   s_userAgentNotificationsLock.lock();
   if (!n->isInstant())
   {
      s_userAgentNotifications.set(n->getId(), n);
      n->saveToDatabase(db);
   }

   s_lock.readLock();
   for (int i = 0; i < s_agents.size(); i++)
   {
      SessionAgentConnector *c = s_agents.get(i);
      if (c->isUserAgent())
         c->addUserAgentNotification(n);
   }
   s_lock.unlock();

   if (n->isInstant())
      delete n;

   s_userAgentNotificationsLock.unlock();

   return RCC_SUCCESS;
}

/**
 * Remove user support application notification
 */
uint32_t RemoveUserAgentNotification(uint64_t serverId, NXCPMessage *request)
{
   ServerObjectKey id = ServerObjectKey(serverId, request->getFieldAsUInt32(VID_UA_NOTIFICATION_BASE));
   
   s_userAgentNotificationsLock.lock();
   s_userAgentNotifications.remove(id);
   s_userAgentNotificationsLock.unlock();

   TCHAR query[256];
   _sntprintf(query, 256, _T("DELETE FROM user_agent_notifications WHERE server_id=") INT64_FMT _T(" AND notification_id=%u"), serverId, id.objectId);
   DB_HANDLE db = GetLocalDatabaseHandle();
   DBQuery(db, query);

   s_lock.readLock();
   for (int i = 0; i < s_agents.size(); i++)
   {
      SessionAgentConnector *c = s_agents.get(i);
      if (c->isUserAgent())
         c->removeUserAgentNotification(id);
   }
   s_lock.unlock();

   return RCC_SUCCESS;
}

/**
 * Update complete list of user support application notifications
 */
uint32_t UpdateUserAgentNotifications(uint64_t serverId, NXCPMessage *request)
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

   s_lock.readLock();
   for (int i = 0; i < s_agents.size(); i++)
   {
      SessionAgentConnector *c = s_agents.get(i);
      if (c->isUserAgent())
         c->updateUserAgentNotifications();
   }
   s_lock.unlock();

   s_userAgentNotificationsLock.unlock();

   return RCC_SUCCESS;
}

/**
 * Load user support application notifications from local database
 */
void LoadUserAgentNotifications()
{
   DB_HANDLE db = GetLocalDatabaseHandle();
   if (db == nullptr)
   {
      nxlog_write(NXLOG_WARNING, _T("Cannot load user support application notifications (local database is unavailable)"));
      return;
   }

   TCHAR query[1024];
   _sntprintf(query, 1024, _T("SELECT server_id,notification_id,message,start_time,end_time,on_startup FROM user_agent_notifications"));

   DB_RESULT hResult = DBSelect(db, query);
   if (hResult != nullptr)
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
   shared_ptr<SessionAgentConnector> connector = AcquireSessionAgentConnector(sessionId);
   if (connector == nullptr)
      return false;

   bool success = connector->getScreenInfo(width, height, bpp);
   return success;
}
