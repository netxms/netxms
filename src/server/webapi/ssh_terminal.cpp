/*
** NetXMS - Network Management System
** Copyright (C) 2026 Raden Solutions
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
** File: ssh_terminal.cpp
**
**/

#include "webapi.h"
#include <nddrv.h>

#define DEBUG_TAG L"webapi.ssh"

/**
 * Token validity period in seconds
 */
static const int SSH_TERMINAL_TOKEN_VALIDITY = 30;

/**
 * Maximum amount of PTY output (in bytes) queued for delivery to the client. Exceeding it means that the client
 * is not reading (or not connected); session is closed in that case.
 */
static const size_t SSH_TERMINAL_OUTPUT_QUEUE_LIMIT = 1024 * 1024;

/**
 * Interval (milliseconds) of keepalive requests to agent while terminal is idle; must be below agent's SessionIdleTimeout
 */
static const uint32_t AGENT_KEEPALIVE_INTERVAL = 30000;

/**
 * WebSocket session for interactive SSH terminal. SSH session is terminated on the node's SSH proxy agent;
 * PTY data is relayed between WebSocket binary frames and agent SSH channel.
 *
 * All writes to the WebSocket are done by a dedicated writer thread. Agent SSH channel data handler and agent
 * disconnect handler are invoked on agent receiver thread while agent connection holds its handler lock, so they
 * only queue data and state changes for the writer thread and never block on socket I/O or call closeSSHChannel().
 * Reader thread (run()) forwards client input to the agent and completes the close when the socket is shut down
 * by the writer thread or by the client.
 */
class SshTerminalWebSocketSession : public TcpProxyCallback
{
private:
   shared_ptr<AgentConnectionEx> m_agentConn;
   uint32_t m_agentChannelId;
   uint32_t m_nodeId;
   uint32_t m_userId;
   wchar_t m_workstation[64];
   time_t m_creationTime;
   MHD_UpgradeResponseHandle *m_responseHandle;
   SOCKET m_socket;
   THREAD m_writerThread;
   Condition m_outputCondition;  // Set when there is something for writer thread to do
   Mutex m_mutex;                // Protects state below
   bool m_started;
   bool m_closing;               // close() in progress or completed
   bool m_channelReleased;       // agent side has already released the channel (close notification received)
   bool m_pongPending;
   const char *m_closeReason;    // Set when session has to be closed because of remote close, agent disconnect or output overflow
   ByteStream m_outputQueue;     // PTY output not yet sent to client

   void writerThread();
   void processControlMessage(const ByteStream& frame);

public:
   SshTerminalWebSocketSession(const shared_ptr<AgentConnectionEx>& agentConn, uint32_t nodeId, uint32_t userId, const wchar_t *workstation);
   virtual ~SshTerminalWebSocketSession();

   void onChannelData(const BYTE *data, size_t size, bool errorIndicator);

   void start(MHD_UpgradeResponseHandle *responseHandle, SOCKET s);
   void run();
   void close();

   uint32_t getAgentChannelId() const { return m_agentChannelId; }
   void setAgentChannelId(uint32_t channelId) { m_agentChannelId = channelId; }
   time_t getCreationTime() const { return m_creationTime; }

   virtual void onTcpProxyData(AgentConnectionEx *conn, uint32_t channelId, const void *data, size_t size, bool errorIndicator) override {}
   virtual void onTcpProxyAgentDisconnect(AgentConnectionEx *conn) override;
};

/**
 * Constructor
 */
SshTerminalWebSocketSession::SshTerminalWebSocketSession(const shared_ptr<AgentConnectionEx>& agentConn, uint32_t nodeId, uint32_t userId,
      const wchar_t *workstation) : m_agentConn(agentConn), m_outputCondition(false), m_mutex(MutexType::FAST)
{
   m_agentChannelId = 0;
   m_nodeId = nodeId;
   m_userId = userId;
   wcslcpy(m_workstation, workstation, 64);
   m_creationTime = time(nullptr);
   m_responseHandle = nullptr;
   m_socket = INVALID_SOCKET;
   m_writerThread = INVALID_THREAD_HANDLE;
   m_started = false;
   m_closing = false;
   m_channelReleased = false;
   m_pongPending = false;
   m_closeReason = nullptr;
   m_agentConn->setTcpProxyCallback(this);   // for agent disconnect notifications
}

/**
 * Destructor
 */
SshTerminalWebSocketSession::~SshTerminalWebSocketSession()
{
   m_agentConn->setTcpProxyCallback(nullptr);
}

/**
 * Process data from agent SSH channel (size 0 indicates channel closure). Called on agent receiver thread.
 */
void SshTerminalWebSocketSession::onChannelData(const BYTE *data, size_t size, bool errorIndicator)
{
   LockGuard lockGuard(m_mutex);
   if (size == 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"SSH terminal channel %u closed by remote side (%s)", m_agentChannelId, errorIndicator ? L"error" : L"normal");
      m_channelReleased = true;
      if (m_closing)
         return;
      if (m_closeReason == nullptr)
         m_closeReason = errorIndicator ? "error" : "normal";
   }
   else if (m_closing || (m_closeReason != nullptr))
   {
      return;  // session is being closed, drop output
   }
   else if (m_outputQueue.size() + size > SSH_TERMINAL_OUTPUT_QUEUE_LIMIT)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"SSH terminal channel %u: output queue limit exceeded (client is not reading), closing session", m_agentChannelId);
      m_outputQueue.clear();
      m_closeReason = "overflow";
      if (m_started)
         shutdown(m_socket, SHUT_RDWR);  // wake up reader thread and writer thread blocked in send
   }
   else
   {
      m_outputQueue.write(data, size);
   }
   if (m_started)
      m_outputCondition.set();
}

/**
 * Handle agent disconnect. Called on agent receiver thread.
 */
void SshTerminalWebSocketSession::onTcpProxyAgentDisconnect(AgentConnectionEx *conn)
{
   LockGuard lockGuard(m_mutex);
   nxlog_debug_tag(DEBUG_TAG, 5, L"SSH terminal channel %u: agent disconnected", m_agentChannelId);
   if (m_closing || (m_closeReason != nullptr))
      return;
   m_closeReason = "agent_disconnect";
   if (m_started)
      m_outputCondition.set();
}

/**
 * Start session: attach WebSocket and start writer thread, which delivers output received while session was pending
 * (and close notification, if channel was already closed by then).
 */
void SshTerminalWebSocketSession::start(MHD_UpgradeResponseHandle *responseHandle, SOCKET s)
{
   m_mutex.lock();
   m_responseHandle = responseHandle;
   m_socket = s;
   m_started = true;
   m_mutex.unlock();

   m_writerThread = ThreadCreateEx(this, &SshTerminalWebSocketSession::writerThread);
   m_outputCondition.set();   // deliver anything queued while session was pending

   nxlog_debug_tag(DEBUG_TAG, 3, L"SSH terminal WebSocket session started (node %u, channel %u, user %u)", m_nodeId, m_agentChannelId, m_userId);
}

/**
 * Writer thread - the only place where frames are written to the WebSocket. Sends queued PTY output and pong frames;
 * when session has to be closed because of remote close, agent disconnect or output overflow, notifies the client and
 * shuts down the socket to wake up the reader thread, which then completes the close. When close() is in progress,
 * sends close frame and exits.
 */
void SshTerminalWebSocketSession::writerThread()
{
   static const char connected[] = "{\"type\":\"connected\"}";
   SendWebsocketFrame(m_socket, connected, sizeof(connected) - 1);

   while(true)
   {
      m_outputCondition.wait(INFINITE);

      m_mutex.lock();
      size_t size = m_outputQueue.size();
      BYTE *data = (size > 0) ? m_outputQueue.takeBuffer() : nullptr;
      bool pong = m_pongPending;
      m_pongPending = false;
      const char *closeReason = m_closeReason;
      bool closing = m_closing;
      m_mutex.unlock();

      if (size > 0)
      {
         SendWebsocketBinaryFrame(m_socket, data, size);
         MemFree(data);
      }
      if (pong)
      {
         BYTE frame[2] = { 0x8A, 0x00 };  // FIN + pong opcode, no payload
         SendEx(m_socket, frame, 2, 0, nullptr);
      }
      if (closeReason != nullptr)
      {
         char json[128];
         snprintf(json, sizeof(json), "{\"type\":\"close\",\"reason\":\"%s\"}", closeReason);
         SendWebsocketFrame(m_socket, json, strlen(json));
         SendWebsocketCloseFrame(m_socket, WS_CLOSE_NORMAL);
         shutdown(m_socket, SHUT_RDWR);
         break;
      }
      if (closing)
      {
         SendWebsocketCloseFrame(m_socket, WS_CLOSE_NORMAL);
         break;
      }
   }
}

/**
 * Process control message from client
 */
void SshTerminalWebSocketSession::processControlMessage(const ByteStream& frame)
{
   json_error_t error;
   json_t *msg = json_loadb(reinterpret_cast<const char*>(frame.buffer()), frame.size(), 0, &error);
   if (msg == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, L"SSH terminal channel %u: invalid control message (%hs)", m_agentChannelId, error.text);
      return;
   }

   const char *type = json_object_get_string_utf8(msg, "type", "");
   if (!strcmp(type, "resize"))
   {
      int columns = json_object_get_int32(msg, "columns", 0);
      int rows = json_object_get_int32(msg, "rows", 0);
      if ((columns >= 1) && (columns <= 1024) && (rows >= 1) && (rows <= 1024))
      {
         uint32_t rcc = m_agentConn->resizeSSHChannel(m_agentChannelId, columns, rows);
         nxlog_debug_tag(DEBUG_TAG, 6, L"SSH terminal channel %u resized to %dx%d (%s)", m_agentChannelId, columns, rows, AgentErrorCodeToText(rcc));
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 5, L"SSH terminal channel %u: invalid terminal size %dx%d in resize request", m_agentChannelId, columns, rows);
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 6, L"SSH terminal channel %u: unknown control message type \"%hs\"", m_agentChannelId, type);
   }
   json_decref(msg);
}

/**
 * Main loop - read WebSocket frames and forward to agent
 */
void SshTerminalWebSocketSession::run()
{
   int64_t nextKeepaliveTime = GetMonotonicClockTime() + AGENT_KEEPALIVE_INTERVAL;
   while(!IsShutdownInProgress())
   {
      int64_t now = GetMonotonicClockTime();
      if (now >= nextKeepaliveTime)
      {
         m_agentConn->nop();
         now = GetMonotonicClockTime();
         nextKeepaliveTime = now + AGENT_KEEPALIVE_INTERVAL;
      }

      SocketPoller sp;
      sp.add(m_socket);
      int rc = sp.poll(static_cast<uint32_t>(nextKeepaliveTime - now));
      if (rc == 0)
         continue;

      ByteStream buffer;
      BYTE frameType;
      if ((rc < 0) || !ReadWebsocketFrame(m_socket, &buffer, &frameType))
      {
         nxlog_debug_tag(DEBUG_TAG, 5, L"SSH terminal channel %u: WebSocket read error or socket shutdown", m_agentChannelId);
         break;
      }

      if (frameType == 0x08)  // Close frame
      {
         nxlog_debug_tag(DEBUG_TAG, 5, L"SSH terminal channel %u: received close frame", m_agentChannelId);
         break;
      }

      if (frameType == 0x02)  // Binary frame - terminal input
      {
         if (!m_agentConn->isConnected())
         {
            nxlog_debug_tag(DEBUG_TAG, 5, L"SSH terminal channel %u: agent disconnected", m_agentChannelId);
            break;
         }
         if (buffer.size() > 0)
            m_agentConn->sendSSHChannelData(m_agentChannelId, buffer.buffer(), buffer.size());
      }
      else if (frameType == 0x01)  // Text frame - control message
      {
         processControlMessage(buffer);
      }
      else if (frameType == 0x09)  // Ping frame
      {
         LockGuard lockGuard(m_mutex);
         m_pongPending = true;
         m_outputCondition.set();
      }
      // Ignore other frame types
   }
   close();
}

/**
 * Close session. Must not be called from agent channel data handler or agent disconnect handler.
 */
void SshTerminalWebSocketSession::close()
{
   m_mutex.lock();
   if (m_closing)
   {
      m_mutex.unlock();
      return;
   }
   m_closing = true;
   bool channelReleased = m_channelReleased;
   bool started = m_started;
   m_mutex.unlock();

   // closeSSHChannel() removes data handler first and waits for any handler call in progress, and setTcpProxyCallback()
   // waits for any disconnect handler call in progress, so no callbacks for this session can happen after this point
   if (!channelReleased && (m_agentChannelId != 0))
      m_agentConn->closeSSHChannel(m_agentChannelId);
   m_agentConn->setTcpProxyCallback(nullptr);

   if (started)
   {
      // Writer thread sends close frame (unless it has already sent close notification) and exits
      m_outputCondition.set();
      ThreadJoin(m_writerThread);
   }

   if (m_responseHandle != nullptr)
   {
      MHD_upgrade_action(m_responseHandle, MHD_UPGRADE_ACTION_CLOSE);
      m_responseHandle = nullptr;
   }

   if (started)
      WriteAuditLog(AUDIT_SYSCFG, true, m_userId, m_workstation, 0, m_nodeId, L"Closed SSH terminal session (agent channel %u)", m_agentChannelId);
   nxlog_debug_tag(DEBUG_TAG, 5, L"SSH terminal channel %u closed", m_agentChannelId);
}

/**
 * Pending sessions storage
 */
static HashMap<uuid, SshTerminalWebSocketSession> s_pendingSessions(Ownership::True);
static Mutex s_pendingSessionsLock;

/**
 * Cleanup expired pending sessions
 */
void CleanupExpiredSshTerminalSessions()
{
   time_t now = time(nullptr);

   ObjectArray<SshTerminalWebSocketSession> expiredSessions(0, 16, Ownership::True);
   s_pendingSessionsLock.lock();
   StructArray<uuid> expiredTokens;
   s_pendingSessions.forEach(
      [now, &expiredTokens, &expiredSessions] (const uuid& token, SshTerminalWebSocketSession *session) -> EnumerationCallbackResult
      {
         if (now - session->getCreationTime() > SSH_TERMINAL_TOKEN_VALIDITY)
         {
            expiredTokens.add(token);
            expiredSessions.add(session);
         }
         return _CONTINUE;
      });
   for(int i = 0; i < expiredTokens.size(); i++)
      s_pendingSessions.unlink(*expiredTokens.get(i));
   s_pendingSessionsLock.unlock();

   // Close outside of pending sessions lock - closing agent channel waits for agent response
   for(int i = 0; i < expiredSessions.size(); i++)
   {
      SshTerminalWebSocketSession *session = expiredSessions.get(i);
      nxlog_debug_tag(DEBUG_TAG, 5, L"Closing SSH terminal session with expired token (channel %u)", session->getAgentChannelId());
      session->close();
   }

   ThreadPoolScheduleRelative(g_mainThreadPool, 60000, CleanupExpiredSshTerminalSessions);
}

/**
 * Handler for POST /v1/objects/:object-id/ssh-terminal - open SSH terminal session and return WebSocket token
 */
int H_ObjectSshTerminalCreate(Context *context)
{
   if (!(context->getSystemAccessRights() & SYSTEM_ACCESS_SETUP_TCP_PROXY))
   {
      context->writeAuditLog(AUDIT_SYSCFG, false, 0, L"Access denied on setting up SSH terminal");
      context->setErrorResponse("Access denied - no TCP proxy permission");
      return 403;
   }

   uint32_t objectId = context->getPlaceholderValueAsUInt32(L"object-id");
   shared_ptr<NetObj> object = FindObjectById(objectId);
   if (object == nullptr)
   {
      context->setErrorResponse("Invalid object ID");
      return 404;
   }
   if (object->getObjectClass() != OBJECT_NODE)
   {
      context->setErrorResponse("SSH terminal is only supported for nodes");
      return 400;
   }
   if (!object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_CONTROL))
   {
      context->writeAuditLog(AUDIT_SYSCFG, false, objectId, L"Access denied on setting up SSH terminal");
      context->setErrorResponse("Access denied - no control access to object");
      return 403;
   }
   Node *node = static_cast<Node*>(object.get());

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      context->setErrorResponse("Missing or invalid JSON body");
      return 400;
   }

   int columns = json_object_get_int32(request, "columns", 0);
   int rows = json_object_get_int32(request, "rows", 0);
   if ((columns < 1) || (columns > 1024) || (rows < 1) || (rows > 1024))
   {
      context->setErrorResponse("Invalid terminal size (columns and rows must be in range 1..1024)");
      return 400;
   }

   // Credentials: provided by client, or stored node credentials if login is not provided (absent or null)
   MutableString login, password;
   uint32_t keyId;
   if (json_is_string(json_object_get(request, "login")))
   {
      login = json_object_get_string(request, "login", L"");
      password = json_object_get_string(request, "password", L"");
      keyId = json_object_get_uint32(request, "keyId", 0);
   }
   else
   {
      login = node->getSshLogin().cstr();
      password = node->getSshPassword().cstr();
      keyId = node->getSshKeyId();
   }

   char terminalType[32];
   strlcpy(terminalType, json_object_get_string_utf8(request, "terminalType", ""), sizeof(terminalType));
   if (terminalType[0] == 0)
   {
      SSHDriverHints hints;
      if (node->getDriver() != nullptr)
         node->getDriver()->getSSHDriverHints(&hints);
      strlcpy(terminalType, hints.terminalType, sizeof(terminalType));
   }

   uint32_t proxyId = node->getEffectiveSshProxy();
   shared_ptr<Node> proxyNode = static_pointer_cast<Node>(FindObjectById(proxyId, OBJECT_NODE));
   if (proxyNode == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"SSH terminal to node %s [%u] requested but its SSH proxy [%u] is not available", node->getName(), node->getId(), proxyId);
      context->setErrorResponse("SSH proxy not available");
      return 503;
   }

   shared_ptr<AgentConnectionEx> conn = proxyNode->createAgentConnection();
   if (conn == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"SSH terminal to node %s [%u] via %s [%u] setup failed (no connection to agent)",
            node->getName(), node->getId(), proxyNode->getName(), proxyNode->getId());
      context->setErrorResponse("Cannot connect to SSH proxy agent");
      return 502;
   }

   nxlog_debug_tag(DEBUG_TAG, 4, L"Setting up SSH terminal to %s [%u] (%s:%u, user \"%s\", key %u, term %hs, size %dx%d) via node %s [%u]",
         node->getName(), node->getId(), node->getIpAddress().toString().cstr(), node->getSshPort(), login.cstr(), keyId, terminalType,
         columns, rows, proxyNode->getName(), proxyNode->getId());

   // Data handler is registered before channel setup so that output produced right after login is not lost
   auto session = new SshTerminalWebSocketSession(conn, node->getId(), context->getUserId(), context->getWorkstation());
   uint32_t agentChannelId = 0;
   uint32_t rcc = conn->openSSHChannel(node->getIpAddress(), node->getSshPort(), login.cstr(), password.cstr(), keyId, terminalType, columns, rows,
      [session] (const BYTE *data, size_t size, bool errorIndicator)
      {
         session->onChannelData(data, size, errorIndicator);
      }, &agentChannelId);
   if (rcc != ERR_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"SSH terminal to node %s [%u] via %s [%u] setup failed (%s)",
            node->getName(), node->getId(), proxyNode->getName(), proxyNode->getId(), AgentErrorCodeToText(rcc));
      delete session;   // data handler already removed by openSSHChannel()
      context->setErrorResponse(AgentErrorCodeToText(rcc));
      return (rcc == ERR_REQUEST_TIMEOUT) ? 504 : 502;
   }
   session->setAgentChannelId(agentChannelId);

   uuid token = uuid::generate();
   s_pendingSessionsLock.lock();
   s_pendingSessions.set(token, session);
   s_pendingSessionsLock.unlock();

   context->writeAuditLog(AUDIT_SYSCFG, true, node->getId(), L"Created SSH terminal session to %s port %u as user \"%s\" via %s [%u] (agent channel %u)",
         node->getIpAddress().toString().cstr(), node->getSshPort(), login.cstr(), proxyNode->getName(), proxyNode->getId(), agentChannelId);
   nxlog_debug_tag(DEBUG_TAG, 3, L"SSH terminal session token created for node %s [%u] via %s [%u] (channel %u)",
         node->getName(), node->getId(), proxyNode->getName(), proxyNode->getId(), agentChannelId);

   char tokenText[64];
   token.toStringA(tokenText);
   char wsUrl[128];
   snprintf(wsUrl, sizeof(wsUrl), "/v1/ssh-terminal/%s", tokenText);

   json_t *response = json_object();
   json_object_set_new(response, "token", json_string(tokenText));
   json_object_set_new(response, "expiresIn", json_integer(SSH_TERMINAL_TOKEN_VALIDITY));
   json_object_set_new(response, "wsUrl", json_string(wsUrl));
   context->setResponseData(response);
   json_decref(response);
   return 201;
}

/**
 * WebSocket upgrade handler for SSH terminal with token
 */
void WS_SshTerminalConnect(void *cls, MHD_Connection *connection, void *con_cls, const char *extra_in, size_t extra_in_size,
      MHD_socket sock, MHD_UpgradeResponseHandle *responseHandle)
{
   Context *context = static_cast<Context*>(cls);

   const wchar_t *tokenText = context->getPlaceholderValue(L"token");
   uuid token = (tokenText != nullptr) ? uuid::parse(tokenText) : uuid::NULL_UUID;
   if (token.isNull())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"SSH terminal WebSocket connection rejected: missing or invalid token");
      RejectWebsocketConnection(sock, responseHandle);
      return;
   }

   // Token is single-use
   s_pendingSessionsLock.lock();
   SshTerminalWebSocketSession *session = s_pendingSessions.get(token);
   if (session != nullptr)
      s_pendingSessions.unlink(token);
   s_pendingSessionsLock.unlock();

   if (session == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"SSH terminal WebSocket connection rejected: token not found");
      RejectWebsocketConnection(sock, responseHandle);
      return;
   }

   if (time(nullptr) - session->getCreationTime() > SSH_TERMINAL_TOKEN_VALIDITY)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"SSH terminal WebSocket connection rejected: token expired");
      session->close();
      delete session;
      RejectWebsocketConnection(sock, responseHandle);
      return;
   }

   session->start(responseHandle, static_cast<SOCKET>(sock));
   ThreadCreate(
      [session] () -> void
      {
         session->run();
         delete session;
      });
}
