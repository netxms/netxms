/*
** NetXMS - Network Management System
** Copyright (C) 2023-2026 Raden Solutions
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
** File: tcpproxy.cpp
**
**/

#include "webapi.h"

#define DEBUG_TAG _T("webapi.tcpproxy")

/**
 * Token validity period in seconds
 */
static const int TCP_PROXY_TOKEN_VALIDITY = 30;

/**
 * Pending TCP proxy session (waiting for WebSocket connection)
 */
struct PendingTcpProxySession
{
   shared_ptr<AgentConnectionEx> agentConnection;
   uint32_t agentChannelId;
   uint32_t userId;
   uint64_t systemAccessRights;
   TCHAR workstation[64];
   time_t creationTime;
   uint32_t proxyNodeId;
   InetAddress targetAddress;
   uint16_t targetPort;
   TCHAR proxyNodeName[MAX_OBJECT_NAME];
};

/**
 * Pending sessions storage
 */
static HashMap<uuid, PendingTcpProxySession> s_pendingSessions(Ownership::True);
static Mutex s_pendingSessionsLock;

/**
 * WebSocket TCP proxy session
 */
class TcpProxyWebSocketSession : public TcpProxyCallback
{
private:
   SOCKET m_socket;
   Mutex m_socketMutex;
   shared_ptr<AgentConnectionEx> m_agentConn;
   uint32_t m_agentChannelId;
   uint32_t m_userId;
   uint64_t m_systemAccessRights;
   TCHAR m_workstation[64];
   bool m_closed;
   Mutex m_closeMutex;

   void sendTextFrame(const char *text);

public:
   TcpProxyWebSocketSession(SOCKET socket, PendingTcpProxySession *pending);
   virtual ~TcpProxyWebSocketSession();

   void run();
   void close();

   virtual void onTcpProxyData(AgentConnectionEx *conn, uint32_t channelId, const void *data, size_t size, bool errorIndicator) override;
   virtual void onTcpProxyAgentDisconnect(AgentConnectionEx *conn) override;
};

/**
 * Constructor - create session from pending session
 */
TcpProxyWebSocketSession::TcpProxyWebSocketSession(SOCKET socket, PendingTcpProxySession *pending)
{
   m_socket = socket;
   m_agentConn = pending->agentConnection;
   m_agentChannelId = pending->agentChannelId;
   m_userId = pending->userId;
   m_systemAccessRights = pending->systemAccessRights;
   wcslcpy(m_workstation, pending->workstation, 64);
   m_closed = false;

   // Set callback to this session
   m_agentConn->setTcpProxyCallback(this);

   // Send connected message
   char json[256];
   snprintf(json, sizeof(json), "{\"type\":\"connected\",\"channelId\":%u}", m_agentChannelId);
   sendTextFrame(json);

   nxlog_debug_tag(DEBUG_TAG, 3, _T("TCP proxy WebSocket session started (channel %u, user %u)"),
         m_agentChannelId, m_userId);
}

/**
 * Destructor
 */
TcpProxyWebSocketSession::~TcpProxyWebSocketSession()
{
   if (m_agentConn != nullptr)
   {
      m_agentConn->setTcpProxyCallback(nullptr);
   }
}

/**
 * Send text frame to WebSocket client
 */
void TcpProxyWebSocketSession::sendTextFrame(const char *text)
{
   m_socketMutex.lock();
   SendWebsocketFrame(static_cast<int>(m_socket), text, strlen(text));
   m_socketMutex.unlock();
}

/**
 * Main loop - read WebSocket frames and forward to agent
 */
void TcpProxyWebSocketSession::run()
{
   while (!m_closed)
   {
      ByteStream buffer;
      BYTE frameType;

      if (!ReadWebsocketFrame(static_cast<int>(m_socket), &buffer, &frameType))
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("TCP proxy channel %u: WebSocket read error"), m_agentChannelId);
         close();
         break;
      }

      if (frameType == 0x08)  // Close frame
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("TCP proxy channel %u: received close frame"), m_agentChannelId);
         close();
         break;
      }
      else if (frameType == 0x02)  // Binary frame - forward to agent
      {
         if ((m_agentConn != nullptr) && m_agentConn->isConnected())
         {
            size_t dataSize = buffer.size();
            if (dataSize > 0)
            {
               // Build and send NXCP binary message
               size_t msgSize = dataSize + NXCP_HEADER_SIZE;
               if (msgSize % 8 != 0)
                  msgSize += 8 - msgSize % 8;

               NXCP_MESSAGE *msg = static_cast<NXCP_MESSAGE*>(MemAlloc(msgSize));
               msg->code = htons(CMD_TCP_PROXY_DATA);
               msg->flags = htons(MF_BINARY);
               msg->id = htonl(m_agentChannelId);
               msg->numFields = htonl(static_cast<uint32_t>(dataSize));
               msg->size = htonl(static_cast<uint32_t>(msgSize));
               memcpy(msg->fields, buffer.buffer(), dataSize);
               m_agentConn->postRawMessage(msg);
            }
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("TCP proxy channel %u: agent disconnected, closing"), m_agentChannelId);
            close();
            break;
         }
      }
      else if (frameType == 0x09)  // Ping frame
      {
         // Send pong frame
         m_socketMutex.lock();
         BYTE pong[2] = { 0x8A, 0x00 };  // FIN + pong opcode, no payload
         SendEx(static_cast<int>(m_socket), pong, 2, 0, nullptr);
         m_socketMutex.unlock();
      }
      // Ignore other frame types
   }
}

/**
 * Close the session
 */
void TcpProxyWebSocketSession::close()
{
   m_closeMutex.lock();
   if (m_closed)
   {
      m_closeMutex.unlock();
      return;
   }
   m_closed = true;
   m_closeMutex.unlock();

   // Close agent channel
   if ((m_agentConn != nullptr) && (m_agentChannelId != 0))
   {
      m_agentConn->closeTcpProxy(m_agentChannelId);
      m_agentConn->setTcpProxyCallback(nullptr);
   }

   // Send close frame
   m_socketMutex.lock();
   SendWebsocketCloseFrame(static_cast<int>(m_socket), WS_CLOSE_NORMAL);
   m_socketMutex.unlock();

   nxlog_debug_tag(DEBUG_TAG, 5, _T("TCP proxy channel %u closed"), m_agentChannelId);
}

/**
 * Process TCP proxy data from agent
 */
void TcpProxyWebSocketSession::onTcpProxyData(AgentConnectionEx *conn, uint32_t channelId, const void *data, size_t size, bool errorIndicator)
{
   if (m_closed)
      return;

   if (size > 0)
   {
      // Send binary frame to WebSocket client
      m_socketMutex.lock();
      SendWebsocketBinaryFrame(static_cast<int>(m_socket), data, size);
      m_socketMutex.unlock();
   }
   else
   {
      // Connection closed by remote end
      nxlog_debug_tag(DEBUG_TAG, 5, _T("TCP proxy channel %u: remote end closed connection (%s)"),
            m_agentChannelId, errorIndicator ? _T("error") : _T("normal"));

      char json[128];
      snprintf(json, sizeof(json), "{\"type\":\"close\",\"reason\":\"%s\"}", errorIndicator ? "error" : "normal");
      sendTextFrame(json);
      close();
   }
}

/**
 * Handle agent disconnect
 */
void TcpProxyWebSocketSession::onTcpProxyAgentDisconnect(AgentConnectionEx *conn)
{
   if (m_closed)
      return;

   nxlog_debug_tag(DEBUG_TAG, 5, _T("TCP proxy channel %u: agent disconnected"), m_agentChannelId);
   sendTextFrame("{\"type\":\"close\",\"reason\":\"agent_disconnect\"}");
   close();
}

/**
 * Cleanup expired pending sessions
 */
static void CleanupExpiredTcpProxySessions()
{
   time_t now = time(nullptr);

   s_pendingSessionsLock.lock();

   StructArray<uuid> expiredTokens;
   s_pendingSessions.forEach(
      [now, &expiredTokens](const uuid& token, PendingTcpProxySession *session) -> EnumerationCallbackResult
      {
         if (now - session->creationTime > TCP_PROXY_TOKEN_VALIDITY * 2)
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("Cleaning up expired TCP proxy session token"));
            session->agentConnection->closeTcpProxy(session->agentChannelId);
            expiredTokens.add(token);
         }
         return _CONTINUE;
      });

   for (int i = 0; i < expiredTokens.size(); i++)
      s_pendingSessions.remove(*expiredTokens.get(i));

   s_pendingSessionsLock.unlock();
}

static int SetupTCPProxy(Context *context, uint32_t nodeId, uint32_t proxyId, InetAddress ipAddr, uint16_t port)
{
   if (!(context->getSystemAccessRights() & SYSTEM_ACCESS_SETUP_TCP_PROXY))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("TCP proxy setup denied for user %u: no SYSTEM_ACCESS_SETUP_TCP_PROXY right"), context->getUserId());
      context->setErrorResponse("Access denied - no TCP proxy permission");
      return 403;
   }

   // Validate port
   if (port == 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("TCP proxy setup failed: port not specified"));
      context->setErrorResponse("Port not specified");
      return 400;
   }

   // Resolve target address and proxy node
   shared_ptr<NetObj> aclObject, proxyObject;

   if (proxyId != 0)
   {
      proxyObject = FindObjectById(proxyId);
      aclObject = proxyObject;
   }
   else
   {
      shared_ptr<NetObj> targetNode = FindObjectById(nodeId, OBJECT_NODE);
      if (targetNode != nullptr)
      {
         ipAddr = targetNode->getPrimaryIpAddress();
         proxyId = static_cast<Node&>(*targetNode).getEffectiveTcpProxy();
         proxyObject = FindObjectById(proxyId);
         aclObject = targetNode;
      }
   }

   if ((aclObject == nullptr) || (proxyObject == nullptr))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("TCP proxy setup failed: invalid object ID (proxyId=%u, nodeId=%u)"), proxyId, nodeId);
      context->setErrorResponse("Invalid object ID");
      return 404;
   }

   // Check object access rights
   if (!aclObject->checkAccessRights(context->getUserId(), OBJECT_ACCESS_CONTROL))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("TCP proxy setup denied for user %u: no control access to object %u"),
            context->getUserId(), aclObject->getId());
      context->setErrorResponse("Access denied - no control access to object");
      return 403;
   }

   // Resolve proxy node
   shared_ptr<Node> proxyNode;
   if (proxyObject->getObjectClass() == OBJECT_NODE)
   {
      proxyNode = static_pointer_cast<Node>(proxyObject);
   }
   else if (proxyObject->getObjectClass() == OBJECT_ZONE)
   {
      proxyNode = static_pointer_cast<Node>(FindObjectById(static_cast<Zone&>(*proxyObject).getProxyNodeId(nullptr, false), OBJECT_NODE));
      if (proxyNode == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("TCP proxy setup failed: zone %s has no available proxy nodes"), proxyObject->getName());
         context->setErrorResponse("No proxy node available");
         return 503;
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("TCP proxy setup failed: object %u is not a node or zone"), proxyObject->getId());
      context->setErrorResponse("Invalid proxy object type");
      return 400;
   }

   // Create agent connection
   nxlog_debug_tag(DEBUG_TAG, 4, _T("Setting up TCP proxy to %s:%u via node %s [%u]"),
         ipAddr.toString().cstr(), port, proxyNode->getName(), proxyNode->getId());

   shared_ptr<AgentConnectionEx> agentConn = proxyNode->createAgentConnection();
   if (agentConn == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("TCP proxy setup failed: cannot connect to agent on node %s [%u]"),
            proxyNode->getName(), proxyNode->getId());
      context->setErrorResponse("Cannot connect to agent");
      return 502;
   }

   uint32_t agentChannelId;
   uint32_t rcc = agentConn->setupTcpProxy(ipAddr, port, &agentChannelId);

   if (rcc != ERR_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("TCP proxy setup failed: agent error %u"), rcc);
      context->setErrorResponse("Agent connection failed");
      return 502;
   }

   // Generate token (UUID)
   uuid token = uuid::generate();

   // Store pending session
   auto *pending = new PendingTcpProxySession();
   pending->agentConnection = agentConn;
   pending->agentChannelId = agentChannelId;
   pending->userId = context->getUserId();
   pending->systemAccessRights = context->getSystemAccessRights();
   _tcslcpy(pending->workstation, context->getWorkstation(), 64);
   pending->creationTime = time(nullptr);
   pending->proxyNodeId = proxyNode->getId();
   pending->targetAddress = ipAddr;
   pending->targetPort = port;
   _tcslcpy(pending->proxyNodeName, proxyNode->getName(), MAX_OBJECT_NAME);

   s_pendingSessionsLock.lock();
   s_pendingSessions.set(token, pending);
   s_pendingSessionsLock.unlock();

   // Write audit log
   WriteAuditLog(AUDIT_SYSCFG, true, context->getUserId(), context->getWorkstation(), 0, proxyNode->getId(),
         _T("Created TCP proxy session to %s port %u via %s [%u] (WebSocket pending)"),
         ipAddr.toString().cstr(), port, proxyNode->getName(), proxyNode->getId());

   nxlog_debug_tag(DEBUG_TAG, 3, _T("TCP proxy session token created for %s:%u via node %s [%u] (channel %u)"),
         ipAddr.toString().cstr(), port, proxyNode->getName(), proxyNode->getId(), agentChannelId);

   // Return token
   json_t *response = json_object();
   char tokenStr[64];
   token.toStringA(tokenStr);
   json_object_set_new(response, "token", json_string(tokenStr));
   json_object_set_new(response, "expiresIn", json_integer(TCP_PROXY_TOKEN_VALIDITY));

   char wsUrl[128];
   snprintf(wsUrl, sizeof(wsUrl), "/v1/tcp-proxy/%s", tokenStr);
   json_object_set_new(response, "wsUrl", json_string(wsUrl));

   context->setResponseData(response);
   json_decref(response);
   return 201;
}

/**
 * Handler for POST /v1/tcp-proxy - create TCP proxy session and return token
 */
int H_TcpProxyCreate(Context *context)
{
   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      context->setErrorResponse("Missing or invalid JSON body");
      return 400;
   }

   uint32_t proxyId = json_object_get_uint32(request, "proxyId", 0);
   uint32_t nodeId = json_object_get_uint32(request, "nodeId", 0);
   uint16_t port = static_cast<uint16_t>(json_object_get_uint32(request, "port", 0));

   InetAddress ipAddr;
   json_t *addressJson = json_object_get(request, "address");
   if (json_is_string(addressJson))
   {
      ipAddr = InetAddress::parse(json_string_value(addressJson));
   }

   return SetupTCPProxy(context, nodeId, proxyId, ipAddr, port);
}

/**
 * Handler for POST /v1/objects/{objectId}/remote-control - create remote control session
 */
int H_ObjectRemoteControl(Context *context)
{
   uint32_t objectId = context->getPlaceholderValueAsUInt32(_T("object-id"));
   if (objectId == 0)
      return 400;

   shared_ptr<NetObj> object = FindObjectById(objectId);
   if (object == nullptr)
      return 404;

   if (object->getObjectClass() != OBJECT_NODE)
      return 400;

   if (!object->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ | OBJECT_ACCESS_CONTROL))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Remote control request denied for user %u: no read/control access to object %u"), context->getUserId(), objectId);
      context->setErrorResponse("Access denied - no read/control access to object");
      return 403;
   }

   Node *node = static_cast<Node*>(object.get());
   if (node->getCapabilities() & NC_IS_LOCAL_VNC)
      return SetupTCPProxy(context, 0, objectId, InetAddress::LOOPBACK, node->getVncPort());

   uint32_t proxyId = node->getEffectiveVncProxy();
   return SetupTCPProxy(context, objectId, proxyId, node->getPrimaryIpAddress(), node->getVncPort());
}

/**
 * WebSocket upgrade handler for TCP proxy with token
 */
void WS_TcpProxyConnect(void *cls, MHD_Connection *connection, void *con_cls,
                        const char *extra_in, size_t extra_in_size, MHD_socket sock,
                        MHD_UpgradeResponseHandle *urh)
{
   Context *context = static_cast<Context*>(cls);

   // Get token from path placeholder
   const TCHAR *tokenStr = context->getPlaceholderValue(_T("token"));
   if (tokenStr == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("TCP proxy WebSocket connection rejected: no token provided"));
      SendWebsocketCloseFrame(static_cast<int>(sock), WS_CLOSE_POLICY_VIOLATION);
      MHD_upgrade_action(urh, MHD_UPGRADE_ACTION_CLOSE);
      return;
   }

   uuid token = uuid::parse(tokenStr);
   if (token.isNull())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("TCP proxy WebSocket connection rejected: invalid token format"));
      SendWebsocketCloseFrame(static_cast<int>(sock), WS_CLOSE_POLICY_VIOLATION);
      MHD_upgrade_action(urh, MHD_UPGRADE_ACTION_CLOSE);
      return;
   }

   // Look up and remove pending session (token is single-use)
   s_pendingSessionsLock.lock();
   PendingTcpProxySession *pending = s_pendingSessions.get(token);
   if (pending != nullptr)
   {
      s_pendingSessions.unlink(token);  // Remove from map without deleting
   }
   s_pendingSessionsLock.unlock();

   // Validate token
   if (pending == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("TCP proxy WebSocket connection rejected: token not found"));
      SendWebsocketCloseFrame(static_cast<int>(sock), WS_CLOSE_POLICY_VIOLATION);
      MHD_upgrade_action(urh, MHD_UPGRADE_ACTION_CLOSE);
      return;
   }

   // Check expiration
   if (time(nullptr) - pending->creationTime > TCP_PROXY_TOKEN_VALIDITY)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("TCP proxy WebSocket connection rejected: token expired"));
      pending->agentConnection->closeTcpProxy(pending->agentChannelId);
      delete pending;
      SendWebsocketCloseFrame(static_cast<int>(sock), WS_CLOSE_POLICY_VIOLATION);
      MHD_upgrade_action(urh, MHD_UPGRADE_ACTION_CLOSE);
      return;
   }

   nxlog_debug_tag(DEBUG_TAG, 4, _T("TCP proxy WebSocket connection established for channel %u"),
         pending->agentChannelId);

   // Create WebSocket session with pre-established agent connection
   auto session = new TcpProxyWebSocketSession(sock, pending);

   // Run session (blocks until closed)
   session->run();

   delete session;
   delete pending;
   MHD_upgrade_action(urh, MHD_UPGRADE_ACTION_CLOSE);
}
