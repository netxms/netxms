/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
** File: comm.cpp
**
**/

#include "nxagentd.h"
#include <socket_listener.h>

/**
 * Setup inbound tunnel on incoming TLS connection (implemented in tunnel.cpp)
 */
void SetupInboundTunnel(SOCKET s, const InetAddress& peer);

/**
 * Session list
 */
SharedObjectArray<CommSession> g_sessions;
Mutex g_sessionLock(MutexType::FAST);

/**
 * Static data
 */
static Mutex s_mutexWatchdogActive;
static VolatileCounter s_messageId = static_cast<int32_t>(time(NULL));

/**
 * Generate new message ID
 */
uint32_t GenerateMessageId()
{
   return static_cast<uint32_t>(InterlockedIncrement(&s_messageId));
}

/**
 * Initialize session list
 */
void InitSessionList()
{
   if (g_maxCommSessions == 0)  // default value
   {
      g_maxCommSessions = (g_dwFlags & (AF_ENABLE_PROXY | AF_ENABLE_SNMP_PROXY)) ? 1024 : 32;
   }
   else
   {
      g_maxCommSessions = MIN(MAX(g_maxCommSessions, 2), 4096);
   }
   nxlog_debug_tag(DEBUG_TAG_COMM, 2, _T("Maximum number of sessions set to %d"), g_maxCommSessions);
}

/**
 * Validates server's address
 */
bool IsValidServerAddress(const InetAddress &addr, bool *pbMasterServer, bool *pbControlServer, bool *pbUpgradeServer, bool forceResolve)
{
   for(int i = 0; i < g_serverList.size(); i++)
	{
      ServerInfo *s = g_serverList.get(i);
      if (s->match(addr, forceResolve))
      {
         *pbMasterServer = s->isMaster();
         *pbControlServer = s->isControl();
         *pbUpgradeServer = s->isUpgrade();
         return true;
      }
	}
   return false;
}

/**
 * Register new session in list
 */
bool RegisterSession(const shared_ptr<CommSession>& session)
{
   g_sessionLock.lock();
   if (g_sessions.size() < static_cast<int>(g_maxCommSessions))
   {
      g_sessions.add(session);
      g_sessionLock.unlock();
      session->debugPrintf(4, _T("Session registered (control=%s, master=%s, upgrade=%s)"),
            BooleanToString(session->isControlServer()), BooleanToString(session->isMasterServer()),
            BooleanToString(session->isUpgradeServer()));
      return true;
   }

   g_sessionLock.unlock();
   nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_COMM, _T("Too many communication sessions open - unable to accept new connection"));
   return false;
}

/**
 * Unregister session
 */
void UnregisterSession(uint32_t id)
{
   g_sessionLock.lock();
   for(int i = 0; i < g_sessions.size(); i++)
      if (g_sessions.get(i)->getId() == id)
      {
         g_sessions.get(i)->debugPrintf(4, _T("Session unregistered"));
         g_sessions.remove(i);
      }
   g_sessionLock.unlock();
}

/**
 * Enumerates active agent sessions. Callback will be called for each valid session.
 * Callback must return _STOP to stop enumeration or _CONTINUE to continue.
 *
 * @return true if enumeration was stopped by callback
 */
bool EnumerateSessions(EnumerationCallbackResult (*callback)(AbstractCommSession *, void *), void *data)
{
   bool result = false;
   g_sessionLock.lock();
   for(int i = 0; i < g_sessions.size(); i++)
   {
      if (callback(g_sessions.get(i), data) == _STOP)
      {
         result = true;
         break;
      }
   }
   g_sessionLock.unlock();
   return result;
}

/**
 * Find server session by server ID. Caller must call decRefCount() for session object when finished.
 */
shared_ptr<AbstractCommSession> FindServerSessionByServerId(uint64_t serverId)
{
   shared_ptr<AbstractCommSession> session;
   g_sessionLock.lock();
   for(int i = 0; i < g_sessions.size(); i++)
   {
      if (g_sessions.get(i)->getServerId() == serverId)
      {
         session = g_sessions.getShared(i);
         break;
      }
   }
   g_sessionLock.unlock();
   return session;
}

/**
 * Find server session by session ID. Caller must call decRefCount() for session object when finished.
 */
shared_ptr<AbstractCommSession> FindServerSessionById(uint32_t id)
{
   shared_ptr<AbstractCommSession> session;
   g_sessionLock.lock();
   for(int i = 0; i < g_sessions.size(); i++)
   {
      if (g_sessions.get(i)->getId() == id)
      {
         session = g_sessions.getShared(i);
         break;
      }
   }
   g_sessionLock.unlock();
   return session;
}

/**
 * Find server session using comparator callback. Caller must call decRefCount() for session object when finished.
 */
shared_ptr<AbstractCommSession> FindServerSession(bool (*comparator)(AbstractCommSession *, void *), void *userData)
{
   shared_ptr<AbstractCommSession> session;
   g_sessionLock.lock();
   for(int i = 0; i < g_sessions.size(); i++)
   {
      if (comparator(g_sessions.get(i), userData))
      {
         session = g_sessions.getShared(i);
         break;
      }
   }
   g_sessionLock.unlock();
   return session;
}

/**
 * Send notification message to all connected servers
 */
void NotifyConnectedServers(const TCHAR *notificationCode)
{
   NXCPMessage msg(CMD_NOTIFY, 0);
   msg.setField(VID_NOTIFICATION_CODE, notificationCode);

   g_sessionLock.lock();
   for(int j = 0; j < g_sessions.size(); j++)
   {
      CommSession *session = g_sessions.get(j);
      if (session->canAcceptTraps())
         session->sendMessage(msg);
   }
   g_sessionLock.unlock();
}

/**
 * Incoming connection request
 */
struct IncomingConnectionRequest
{
   SOCKET socket;
   InetAddress addr;
   bool masterServer;
   bool controlServer;
   bool upgradeServer;
};

#ifdef _WITH_ENCRYPTION

/**
 * Reject legacy NXCP connection when TLS is required by agent configuration. Speaks minimal NXCP
 * so that connecting server can learn agent capabilities and the reason for rejection: responds to
 * protocol version negotiation normally (advertising TLS support) and to the first regular request
 * with ERR_TLS_REQUIRED, then closes the connection.
 */
static void RejectLegacyConnection(SOCKET socket, const InetAddress& addr)
{
   int protocolVersion = NXCP_VERSION;
   SocketMessageReceiver receiver(socket, 4096, MAX_AGENT_MSG_SIZE);
   int64_t deadline = GetMonotonicClockTime() + 10000;
   while(GetMonotonicClockTime() < deadline)
   {
      MessageReceiverResult result;
      NXCPMessage *msg = receiver.readMessage(1000, &result);
      if (msg == nullptr)
      {
         if (result == MSGRECV_TIMEOUT)
            continue;
         break;
      }

      if (msg->isControl() && (msg->getCode() == CMD_GET_NXCP_CAPS))
      {
         uint32_t peerNXCPVersion = msg->getEncodedProtocolVersion(); // Before NXCP version 5 encoded version will be 0, assume version 4
         protocolVersion = (peerNXCPVersion == 0) ? 4 : MIN(peerNXCPVersion, NXCP_VERSION);

         BYTE rawResponse[NXCP_HEADER_SIZE];
         NXCP_MESSAGE *response = reinterpret_cast<NXCP_MESSAGE*>(rawResponse);
         response->id = htonl(msg->getId());
         response->code = htons((uint16_t)CMD_NXCP_CAPS);
         response->flags = htons(MF_CONTROL | MF_NXCP_VERSION(protocolVersion));
         response->numFields = htonl((protocolVersion << 24) | NXCP_CAP_TLS_TUNNEL);
         response->size = htonl(NXCP_HEADER_SIZE);
         SendEx(socket, response, NXCP_HEADER_SIZE, 0, nullptr);
         delete msg;
      }
      else if (!msg->isControl() && !msg->isBinary())
      {
         NXCPMessage response(CMD_REQUEST_COMPLETED, msg->getId(), protocolVersion);
         response.setField(VID_RCC, ERR_TLS_REQUIRED);
         NXCP_MESSAGE *rawResponse = response.serialize(false);
         SendEx(socket, rawResponse, ntohl(rawResponse->size), 0, nullptr);
         MemFree(rawResponse);
         delete msg;
         break;   // rejection delivered
      }
      else
      {
         delete msg;
      }
   }

   shutdown(socket, SHUT_RDWR);
   closesocket(socket);
}

#endif   /* _WITH_ENCRYPTION */

/**
 * Setup incoming connection. Detects protocol (TLS handshake vs legacy NXCP) by peeking at
 * first byte sent by the server, then routes connection to inbound tunnel setup or creates
 * communication session directly. Runs in communication thread pool because it can block
 * waiting for initial data.
 */
static void SetupIncomingConnection(IncomingConnectionRequest *request)
{
   // Wait for initial data from server to detect TLS ClientHello (0x16 is TLS handshake
   // record type and is never a valid first byte of NXCP message)
   BYTE firstByte = 0;
   SocketPoller sp;
   sp.add(request->socket);
   if (sp.poll(5000) > 0)
   {
      char b;
      if (recv(request->socket, &b, 1, MSG_PEEK) == 1)
         firstByte = static_cast<BYTE>(b);
   }

   TCHAR buffer[64];
   if (firstByte == 0x16)
   {
#ifdef _WITH_ENCRYPTION
      nxlog_debug_tag(DEBUG_TAG_COMM, 5, _T("TLS connection detected from %s"), request->addr.toString(buffer));
      SetupInboundTunnel(request->socket, request->addr);
#else
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_COMM, _T("Incoming TLS connection from %s rejected (agent built without encryption support)"), request->addr.toString(buffer));
      shutdown(request->socket, SHUT_RDWR);
      closesocket(request->socket);
#endif
   }
   else
   {
#ifdef _WITH_ENCRYPTION
      if (g_dwFlags & AF_REQUIRE_TLS)
      {
         nxlog_debug_tag(DEBUG_TAG_COMM, 5, _T("Rejecting legacy connection from %s (TLS required by agent configuration)"), request->addr.toString(buffer));
         RejectLegacyConnection(request->socket, request->addr);
         delete request;
         return;
      }
#endif
      shared_ptr<SocketCommChannel> channel = make_shared<SocketCommChannel>(request->socket, nullptr, Ownership::True);
      shared_ptr<CommSession> session = MakeSharedCommSession<CommSession>(channel, request->addr, request->masterServer, request->controlServer, request->upgradeServer);
      if (RegisterSession(session))
      {
         nxlog_debug_tag(DEBUG_TAG_COMM, 9, _T("Session registered for %s"), request->addr.toString(buffer));
         session->run();
      }
   }
   delete request;
}

/**
 * Agent's main TCP listener
 */
class AgentListener : public StreamSocketListener
{
private:
   // Access rights of the peer accepted by the last isConnectionAllowed() call; valid for
   // the processConnection() call that immediately follows it on the listener thread
   bool m_masterServer;
   bool m_controlServer;
   bool m_upgradeServer;

protected:
   virtual bool isStopConditionReached() override
   {
      return (g_dwFlags & AF_SHUTDOWN) != 0;
   }

   virtual bool isConnectionAllowed(const InetAddress& peer) override
   {
      return IsValidServerAddress(peer, &m_masterServer, &m_controlServer, &m_upgradeServer, false);
   }

   virtual ConnectionProcessingResult processConnection(SOCKET s, const InetAddress& peer) override
   {
      // Hand connection over to processing thread for protocol detection
      // (it can block waiting for initial data and must not delay listener)
      auto request = new IncomingConnectionRequest();
      request->socket = s;
      request->addr = peer;
      request->masterServer = m_masterServer;
      request->controlServer = m_controlServer;
      request->upgradeServer = m_upgradeServer;
      ThreadPoolExecute(g_commThreadPool, SetupIncomingConnection, request);
      return CPR_BACKGROUND;
   }

public:
   AgentListener() : StreamSocketListener(g_wListenPort, (g_dwFlags & AF_DISABLE_IPV4) == 0, (g_dwFlags & AF_DISABLE_IPV6) == 0)
   {
      m_masterServer = false;
      m_controlServer = false;
      m_upgradeServer = false;
      setName(_T("Agent"));
   }
};

/**
 * Active listener (used for statistics)
 */
static AgentListener *s_listener = nullptr;
static Mutex s_listenerLock(MutexType::FAST);

/**
 * Handler for listener statistics
 */
LONG H_ListenerStats(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   LockGuard lockGuard(s_listenerLock);
   if (s_listener == nullptr)
   {
      ret_uint(value, 0);
      return SYSINFO_RC_SUCCESS;
   }
   switch(*arg)
   {
      case 'A':
         ret_uint(value, s_listener->getAcceptedConnections());
         break;
      case 'E':
         ret_uint(value, s_listener->getAcceptErrors());
         break;
      case 'R':
         ret_uint(value, s_listener->getRejectedConnections());
         break;
      default:
         return SYSINFO_RC_UNSUPPORTED;
   }
   return SYSINFO_RC_SUCCESS;
}

/**
 * TCP/IP Listener
 */
void ListenerThread()
{
   auto listener = new AgentListener();
   listener->setListenAddress(g_szListenAddress);
   if (!listener->initialize())
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_COMM, _T("Cannot bind at least one socket, terminating agent process"));
#ifdef _WIN32
      ExitProcess(99);
#else
      _exit(99);
#endif
   }

   s_listenerLock.lock();
   s_listener = listener;
   s_listenerLock.unlock();

   listener->mainLoop();

   // Wait for watchdog thread
   s_mutexWatchdogActive.lock();
   s_mutexWatchdogActive.unlock();

   s_listenerLock.lock();
   s_listener = nullptr;
   s_listenerLock.unlock();
   delete listener;

   nxlog_debug_tag(DEBUG_TAG_COMM, 1, _T("Listener thread terminated"));
}

/**
 * Session watchdog thread
 */
void SessionWatchdog()
{
   s_mutexWatchdogActive.lock();

   while(!AgentSleepAndCheckForShutdown(5000))
   {
      g_sessionLock.lock();
      int64_t now = GetMonotonicClockTime();
      int64_t timeout = static_cast<int64_t>(g_dwIdleTimeout) * _LL(1000);
      for(int i = 0; i < g_sessions.size(); i++)
      {
         CommSession *session = g_sessions.get(i);
         if (session->getTimeStamp() + timeout < now)
         {
            session->debugPrintf(4, _T("Session disconnected by watchdog (timestamp = ") INT64_FMT _T(", now = ") INT64_FMT _T(")"), session->getTimeStamp(), now);
            session->disconnect();
            g_sessions.remove(i);
            i--;
         }
      }
      g_sessionLock.unlock();
   }

   // Disconnect all sessions
   g_sessionLock.lock();
   for(int i = 0; i < g_sessions.size(); i++)
      g_sessions.get(i)->disconnect();
   g_sessionLock.unlock();

   ThreadSleep(1);
   s_mutexWatchdogActive.unlock();
   nxlog_debug_tag(DEBUG_TAG_COMM, 1, _T("Session Watchdog thread terminated"));
}

/**
 * Handler for Agent.ActiveConnections parameter
 */
LONG H_ActiveConnections(const TCHAR *pszCmd, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   g_sessionLock.lock();
   ret_int(pValue, g_sessions.size());
   g_sessionLock.unlock();
   return SYSINFO_RC_SUCCESS;
}
