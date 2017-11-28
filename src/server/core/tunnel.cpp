/*
** NetXMS - Network Management System
** Copyright (C) 2003-2017 Victor Kirhenshtein
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
** File: tunnel.cpp
**/

#include "nxcore.h"
#include <socket_listener.h>
#include <agent_tunnel.h>

#define MAX_MSG_SIZE    268435456

#define DEBUG_TAG       _T("agent.tunnel")

/**
 * Tunnel registration
 */
static RefCountHashMap<UINT32, AgentTunnel> s_boundTunnels(true);
static ObjectRefArray<AgentTunnel> s_unboundTunnels(16, 16);
static Mutex s_tunnelListLock;

/**
 * Register tunnel
 */
static void RegisterTunnel(AgentTunnel *tunnel)
{
   tunnel->incRefCount();
   s_tunnelListLock.lock();
   if (tunnel->isBound())
   {
      s_boundTunnels.set(tunnel->getNodeId(), tunnel);
      tunnel->decRefCount(); // set already increased ref count
   }
   else
   {
      s_unboundTunnels.add(tunnel);
   }
   s_tunnelListLock.unlock();
}

/**
 * Unregister tunnel
 */
static void UnregisterTunnel(AgentTunnel *tunnel)
{
   tunnel->debugPrintf(4, _T("Tunnel unregistered"));
   s_tunnelListLock.lock();
   if (tunnel->isBound())
   {
      // Check that current tunnel for node is tunnel being unregistered
      // New tunnel could be established while old one still finishing
      // outstanding requests
      if (s_boundTunnels.peek(tunnel->getNodeId()) == tunnel)
         s_boundTunnels.remove(tunnel->getNodeId());
   }
   else
   {
      s_unboundTunnels.remove(tunnel);
      tunnel->decRefCount();
   }
   s_tunnelListLock.unlock();
}

/**
 * Get tunnel for node. Caller must decrease reference counter on tunnel.
 */
AgentTunnel *GetTunnelForNode(UINT32 nodeId)
{
   s_tunnelListLock.lock();
   AgentTunnel *t = s_boundTunnels.get(nodeId);
   s_tunnelListLock.unlock();
   return t;
}

/**
 * Bind agent tunnel
 */
UINT32 BindAgentTunnel(UINT32 tunnelId, UINT32 nodeId)
{
   AgentTunnel *tunnel = NULL;
   s_tunnelListLock.lock();
   for(int i = 0; i < s_unboundTunnels.size(); i++)
   {
      if (s_unboundTunnels.get(i)->getId() == tunnelId)
      {
         tunnel = s_unboundTunnels.get(i);
         tunnel->incRefCount();
         break;
      }
   }
   s_tunnelListLock.unlock();

   if (tunnel == NULL)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("BindAgentTunnel: unbound tunnel with ID %d not found"), tunnelId);
      return RCC_INVALID_TUNNEL_ID;
   }

   UINT32 rcc = tunnel->bind(nodeId);
   tunnel->decRefCount();
   return rcc;
}

/**
 * Bind agent tunnel from node
 */
UINT32 UnbindAgentTunnel(UINT32 nodeId)
{
   Node *node = (Node *)FindObjectById(nodeId, OBJECT_NODE);
   if (node == NULL)
      return RCC_INVALID_OBJECT_ID;

   if (node->getTunnelId().isNull())
      return RCC_SUCCESS;  // tunnel is not set

   node->setTunnelId(uuid::NULL_UUID);

   AgentTunnel *tunnel = GetTunnelForNode(nodeId);
   if (tunnel != NULL)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("UnbindAgentTunnel(%s): shutting down existing tunnel"), node->getName());
      tunnel->shutdown();
      tunnel->decRefCount();
   }

   return RCC_SUCCESS;
}

/**
 * Get list of agent tunnels into NXCP message
 */
void GetAgentTunnels(NXCPMessage *msg)
{
   s_tunnelListLock.lock();
   UINT32 fieldId = VID_ELEMENT_LIST_BASE;

   for(int i = 0; i < s_unboundTunnels.size(); i++)
   {
      s_unboundTunnels.get(i)->fillMessage(msg, fieldId);
      fieldId += 64;
   }

   Iterator<AgentTunnel> *it = s_boundTunnels.iterator();
   while(it->hasNext())
   {
      it->next()->fillMessage(msg, fieldId);
      fieldId += 64;
   }
   delete it;

   msg->setField(VID_NUM_ELEMENTS, (UINT32)(s_unboundTunnels.size() + s_boundTunnels.size()));
   s_tunnelListLock.unlock();
}

/**
 * Show tunnels in console
 */
void ShowAgentTunnels(CONSOLE_CTX console)
{
   s_tunnelListLock.lock();

   ConsolePrintf(console,
            _T("\n\x1b[1mBOUND TUNNELS\x1b[0m\n")
            _T("ID   | Node ID | Peer IP Address          | System Name              | Platform Name    | Agent Version\n")
            _T("-----+---------+--------------------------+--------------------------+------------------+------------------------\n"));
   Iterator<AgentTunnel> *it = s_boundTunnels.iterator();
   while(it->hasNext())
   {
      AgentTunnel *t = it->next();
      TCHAR ipAddrBuffer[64];
      ConsolePrintf(console, _T("%4d | %7u | %-24s | %-24s | %-16s | %s\n"), t->getId(), t->getNodeId(), t->getAddress().toString(ipAddrBuffer), t->getSystemName(), t->getPlatformName(), t->getAgentVersion());
   }
   delete it;

   ConsolePrintf(console,
            _T("\n\x1b[1mUNBOUND TUNNELS\x1b[0m\n")
            _T("ID   | Peer IP Address          | System Name              | Platform Name    | Agent Version\n")
            _T("-----+--------------------------+--------------------------+------------------+------------------------\n"));
   for(int i = 0; i < s_unboundTunnels.size(); i++)
   {
      const AgentTunnel *t = s_unboundTunnels.get(i);
      TCHAR ipAddrBuffer[64];
      ConsolePrintf(console, _T("%4d | %-24s | %-24s | %-16s | %s\n"), t->getId(), t->getAddress().toString(ipAddrBuffer), t->getSystemName(), t->getPlatformName(), t->getAgentVersion());
   }

   s_tunnelListLock.unlock();
}

/**
 * Next free tunnel ID
 */
static VolatileCounter s_nextTunnelId = 0;

/**
 * Agent tunnel constructor
 */
AgentTunnel::AgentTunnel(SSL_CTX *context, SSL *ssl, SOCKET sock, const InetAddress& addr, UINT32 nodeId, UINT32 zoneUIN) : RefCountObject(), m_channels(true)
{
   m_id = InterlockedIncrement(&s_nextTunnelId);
   m_address = addr;
   m_socket = sock;
   m_context = context;
   m_ssl = ssl;
   m_sslLock = MutexCreate();
   m_requestId = 0;
   m_nodeId = nodeId;
   m_zoneUIN = zoneUIN;
   m_state = AGENT_TUNNEL_INIT;
   m_systemName = NULL;
   m_platformName = NULL;
   m_systemInfo = NULL;
   m_agentVersion = NULL;
   m_bindRequestId = 0;
   m_channelLock = MutexCreate();
}

/**
 * Agent tunnel destructor
 */
AgentTunnel::~AgentTunnel()
{
   m_channels.clear();
   shutdown();
   SSL_CTX_free(m_context);
   SSL_free(m_ssl);
   MutexDestroy(m_sslLock);
   closesocket(m_socket);
   free(m_systemName);
   free(m_platformName);
   free(m_systemInfo);
   free(m_agentVersion);
   MutexDestroy(m_channelLock);
   debugPrintf(4, _T("Tunnel destroyed"));
}

/**
 * Debug output
 */
void AgentTunnel::debugPrintf(int level, const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   nxlog_debug_tag_object2(DEBUG_TAG, m_id, level, format, args);
   va_end(args);
}

/**
 * Tunnel receiver thread
 */
void AgentTunnel::recvThread()
{
   TlsMessageReceiver receiver(m_socket, m_ssl, m_sslLock, 4096, MAX_MSG_SIZE);
   while(true)
   {
      MessageReceiverResult result;
      NXCPMessage *msg = receiver.readMessage(60000, &result);
      if (result != MSGRECV_SUCCESS)
      {
         if (result == MSGRECV_CLOSED)
            debugPrintf(4, _T("Tunnel closed by peer"));
         else
            debugPrintf(4, _T("Communication error (%s)"), AbstractMessageReceiver::resultToText(result));
         break;
      }

      if (nxlog_get_debug_level() >= 6)
      {
         TCHAR buffer[64];
         debugPrintf(6, _T("Received message %s"), NXCPMessageCodeName(msg->getCode(), buffer));
      }

      switch(msg->getCode())
      {
         case CMD_KEEPALIVE:
            {
               NXCPMessage response(CMD_KEEPALIVE, msg->getId());
               sendMessage(&response);
            }
            break;
         case CMD_SETUP_AGENT_TUNNEL:
            setup(msg);
            break;
         case CMD_REQUEST_CERTIFICATE:
            processCertificateRequest(msg);
            break;
         case CMD_CHANNEL_DATA:
            if (msg->isBinary())
            {
               MutexLock(m_channelLock);
               AgentTunnelCommChannel *channel = m_channels.get(msg->getId());
               MutexUnlock(m_channelLock);
               if (channel != NULL)
               {
                  channel->putData(msg->getBinaryData(), msg->getBinaryDataSize());
                  channel->decRefCount();
               }
               else
               {
                  debugPrintf(6, _T("Received channel data for non-existing channel %u"), msg->getId());
               }
            }
            break;
         case CMD_CLOSE_CHANNEL:    // channel close notification
            processChannelClose(msg->getFieldAsUInt32(VID_CHANNEL_ID));
            break;
         default:
            m_queue.put(msg);
            msg = NULL; // prevent message deletion
            break;
      }
      delete msg;
   }

   UnregisterTunnel(this);
   m_state = AGENT_TUNNEL_SHUTDOWN;

   // shutdown all channels
   MutexLock(m_channelLock);
   Iterator<AgentTunnelCommChannel> *it = m_channels.iterator();
   while(it->hasNext())
      it->next()->shutdown();
   delete it;
   m_channels.clear();
   MutexUnlock(m_channelLock);

   debugPrintf(4, _T("Receiver thread stopped"));
}

/**
 * Tunnel receiver thread starter
 */
THREAD_RESULT THREAD_CALL AgentTunnel::recvThreadStarter(void *arg)
{
   ThreadSetName("TunnelReceiver");
   ((AgentTunnel *)arg)->recvThread();
   ((AgentTunnel *)arg)->decRefCount();
   return THREAD_OK;
}

/**
 * Write to SSL
 */
int AgentTunnel::sslWrite(const void *data, size_t size)
{
   bool canRetry;
   int bytes;
   MutexLock(m_sslLock);
   do
   {
      canRetry = false;
      bytes = SSL_write(m_ssl, data, (int)size);
      if (bytes <= 0)
      {
         int err = SSL_get_error(m_ssl, bytes);
         if ((err == SSL_ERROR_WANT_READ) || (err == SSL_ERROR_WANT_WRITE))
         {
            SocketPoller sp(err == SSL_ERROR_WANT_WRITE);
            sp.add(m_socket);
            if (sp.poll(5000) > 0)
               canRetry = true;
         }
         else
         {
            debugPrintf(7, _T("SSL_write error (bytes=%d ssl_err=%d errno=%d)"), bytes, err, errno);
            if (err == SSL_ERROR_SSL)
               LogOpenSSLErrorStack(7);
         }
      }
   }
   while(canRetry);
   MutexUnlock(m_sslLock);
   return bytes;
}

/**
 * Send message on tunnel
 */
bool AgentTunnel::sendMessage(NXCPMessage *msg)
{
   if (m_state == AGENT_TUNNEL_SHUTDOWN)
      return false;

   if (nxlog_get_debug_level() >= 6)
   {
      TCHAR buffer[64];
      debugPrintf(6, _T("Sending message %s"), NXCPMessageCodeName(msg->getCode(), buffer));
   }
   NXCP_MESSAGE *data = msg->serialize(true);
   bool success = (sslWrite(data, ntohl(data->size)) == ntohl(data->size));
   free(data);
   return success;
}

/**
 * Start tunnel
 */
void AgentTunnel::start()
{
   debugPrintf(4, _T("Tunnel started"));
   incRefCount();
   SetSocketNonBlocking(m_socket);
   ThreadCreate(AgentTunnel::recvThreadStarter, 0, this);
}

/**
 * Shutdown tunnel
 */
void AgentTunnel::shutdown()
{
   if (m_socket != INVALID_SOCKET)
      ::shutdown(m_socket, SHUT_RDWR);
   m_state = AGENT_TUNNEL_SHUTDOWN;
   debugPrintf(4, _T("Tunnel shutdown"));
}

/**
 * Process setup request
 */
void AgentTunnel::setup(const NXCPMessage *request)
{
   NXCPMessage response;
   response.setCode(CMD_REQUEST_COMPLETED);
   response.setId(request->getId());

   if (m_state == AGENT_TUNNEL_INIT)
   {
      m_systemName = request->getFieldAsString(VID_SYS_NAME);
      m_systemInfo = request->getFieldAsString(VID_SYS_DESCRIPTION);
      m_platformName = request->getFieldAsString(VID_PLATFORM_NAME);
      m_agentVersion = request->getFieldAsString(VID_AGENT_VERSION);

      m_state = (m_nodeId != 0) ? AGENT_TUNNEL_BOUND : AGENT_TUNNEL_UNBOUND;
      response.setField(VID_RCC, ERR_SUCCESS);
      response.setField(VID_IS_ACTIVE, m_state == AGENT_TUNNEL_BOUND);

      // For bound tunnels zone UIN taken from node object
      if (m_state != AGENT_TUNNEL_BOUND)
         m_zoneUIN = request->getFieldAsUInt32(VID_ZONE_UIN);

      debugPrintf(3, _T("%s tunnel initialized"), (m_state == AGENT_TUNNEL_BOUND) ? _T("Bound") : _T("Unbound"));
      debugPrintf(5, _T("   System name:        %s"), m_systemName);
      debugPrintf(5, _T("   System information: %s"), m_systemInfo);
      debugPrintf(5, _T("   Platform name:      %s"), m_platformName);
      debugPrintf(5, _T("   Agent version:      %s"), m_agentVersion);
      debugPrintf(5, _T("   Zone UIN:           %u"), m_zoneUIN);
   }
   else
   {
      response.setField(VID_RCC, ERR_OUT_OF_STATE_REQUEST);
   }

   sendMessage(&response);
}

/**
 * Bind tunnel to node
 */
UINT32 AgentTunnel::bind(UINT32 nodeId)
{
   if ((m_state != AGENT_TUNNEL_UNBOUND) || (m_bindRequestId != 0))
      return RCC_OUT_OF_STATE_REQUEST;

   Node *node = (Node *)FindObjectById(nodeId, OBJECT_NODE);
   if (node == NULL)
      return RCC_INVALID_OBJECT_ID;

   NXCPMessage msg;
   msg.setCode(CMD_BIND_AGENT_TUNNEL);
   msg.setId(InterlockedIncrement(&m_requestId));
   msg.setField(VID_SERVER_ID, g_serverId);
   msg.setField(VID_GUID, node->getGuid());
   m_guid = uuid::generate();
   msg.setField(VID_TUNNEL_GUID, m_guid);

   TCHAR buffer[256];
   if (GetServerCertificateCountry(buffer, 256))
      msg.setField(VID_COUNTRY, buffer);
   if (GetServerCertificateOrganization(buffer, 256))
      msg.setField(VID_ORGANIZATION, buffer);

   m_bindRequestId = msg.getId();
   m_bindGuid = node->getGuid();
   sendMessage(&msg);

   NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, msg.getId());
   if (response == NULL)
      return RCC_TIMEOUT;

   UINT32 rcc = response->getFieldAsUInt32(VID_RCC);
   delete response;
   if (rcc == ERR_SUCCESS)
   {
      debugPrintf(4, _T("Bind successful, resetting tunnel"));
      node->setNewTunnelBindFlag();
      msg.setCode(CMD_RESET_TUNNEL);
      msg.setId(InterlockedIncrement(&m_requestId));
      sendMessage(&msg);
   }
   else
   {
      debugPrintf(4, _T("Bind failed: agent error %d (%s)"), rcc, AgentErrorCodeToText(rcc));
   }
   return AgentErrorToRCC(rcc);
}

/**
 * Process certificate request
 */
void AgentTunnel::processCertificateRequest(NXCPMessage *request)
{
   NXCPMessage response(CMD_NEW_CERTIFICATE, request->getId());

   if ((request->getId() == m_bindRequestId) && (m_bindRequestId != 0) && (m_state == AGENT_TUNNEL_UNBOUND))
   {
      size_t certRequestLen;
      const BYTE *certRequestData = request->getBinaryFieldPtr(VID_CERTIFICATE, &certRequestLen);
      if (certRequestData != NULL)
      {
         X509_REQ *certRequest = d2i_X509_REQ(NULL, &certRequestData, (long)certRequestLen);
         if (certRequest != NULL)
         {
            char *ou = m_bindGuid.toString().getUTF8String();
            char *cn = m_guid.toString().getUTF8String();
            X509 *cert = IssueCertificate(certRequest, ou, cn, 365);
            free(ou);
            free(cn);
            if (cert != NULL)
            {
               BYTE *buffer = NULL;
               int len = i2d_X509(cert, &buffer);
               if (len > 0)
               {
                  response.setField(VID_RCC, ERR_SUCCESS);
                  response.setField(VID_CERTIFICATE, buffer, len);
                  OPENSSL_free(buffer);
                  debugPrintf(4, _T("Certificate issued"));

                  Node *node = (Node *)FindObjectByGUID(m_bindGuid, OBJECT_NODE);
                  if (node != NULL)
                  {
                     node->setTunnelId(m_guid);
                  }
               }
               else
               {
                  debugPrintf(4, _T("Cannot encode certificate"));
                  response.setField(VID_RCC, ERR_ENCRYPTION_ERROR);
               }
               X509_free(cert);
            }
            else
            {
               debugPrintf(4, _T("Cannot issue certificate"));
               response.setField(VID_RCC, ERR_ENCRYPTION_ERROR);
            }
         }
         else
         {
            debugPrintf(4, _T("Cannot decode certificate request data"));
            response.setField(VID_RCC, ERR_BAD_ARGUMENTS);
         }
      }
      else
      {
         debugPrintf(4, _T("Missing certificate request data"));
         response.setField(VID_RCC, ERR_BAD_ARGUMENTS);
      }
   }
   else
   {
      response.setField(VID_RCC, ERR_OUT_OF_STATE_REQUEST);
   }

   sendMessage(&response);
}

/**
 * Create channel
 */
AgentTunnelCommChannel *AgentTunnel::createChannel()
{
   NXCPMessage request(CMD_CREATE_CHANNEL, InterlockedIncrement(&m_requestId));
   sendMessage(&request);
   NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, request.getId());
   if (response == NULL)
   {
      debugPrintf(4, _T("createChannel: request timeout"));
      return NULL;
   }

   UINT32 rcc = response->getFieldAsUInt32(VID_RCC);
   if (rcc != ERR_SUCCESS)
   {
      delete response;
      debugPrintf(4, _T("createChannel: agent error %d (%s)"), rcc, AgentErrorCodeToText(rcc));
      return NULL;
   }

   AgentTunnelCommChannel *channel = new AgentTunnelCommChannel(this, response->getFieldAsUInt32(VID_CHANNEL_ID));
   delete response;
   MutexLock(m_channelLock);
   m_channels.set(channel->getId(), channel);
   MutexUnlock(m_channelLock);
   debugPrintf(4, _T("createChannel: new channel created (ID=%d)"), channel->getId());
   return channel;
}

/**
 * Process channel close notification from agent
 */
void AgentTunnel::processChannelClose(UINT32 channelId)
{
   debugPrintf(4, _T("processChannelClose: notification of channel %d closure"), channelId);

   MutexLock(m_channelLock);
   AgentTunnelCommChannel *ch = m_channels.get(channelId);
   MutexUnlock(m_channelLock);
   if (ch != NULL)
   {
      ch->shutdown();
      ch->decRefCount();
   }
}

/**
 * Close channel
 */
void AgentTunnel::closeChannel(AgentTunnelCommChannel *channel)
{
   if (m_state == AGENT_TUNNEL_SHUTDOWN)
      return;

   debugPrintf(4, _T("closeChannel: request to close channel %d"), channel->getId());

   MutexLock(m_channelLock);
   m_channels.remove(channel->getId());
   MutexUnlock(m_channelLock);

   // Inform agent that channel is closing
   NXCPMessage msg(CMD_CLOSE_CHANNEL, InterlockedIncrement(&m_requestId));
   msg.setField(VID_CHANNEL_ID, channel->getId());
   sendMessage(&msg);
}

/**
 * Send channel data
 */
int AgentTunnel::sendChannelData(UINT32 id, const void *data, size_t len)
{
   NXCP_MESSAGE *msg = CreateRawNXCPMessage(CMD_CHANNEL_DATA, id, 0, data, len, NULL, false);
   int rc = sslWrite(msg, ntohl(msg->size));
   if (rc == ntohl(msg->size))
      rc = (int)len;  // adjust number of bytes to exclude tunnel overhead
   free(msg);
   return rc;
}

/**
 * Fill NXCP message with tunnel data
 */
void AgentTunnel::fillMessage(NXCPMessage *msg, UINT32 baseId) const
{
   msg->setField(baseId, m_id);
   msg->setField(baseId + 1, m_guid);
   msg->setField(baseId + 2, m_nodeId);
   msg->setField(baseId + 3, m_address);
   msg->setField(baseId + 4, m_systemName);
   msg->setField(baseId + 5, m_systemInfo);
   msg->setField(baseId + 6, m_platformName);
   msg->setField(baseId + 7, m_agentVersion);
   MutexLock(m_channelLock);
   msg->setField(baseId + 8, m_channels.size());
   MutexUnlock(m_channelLock);
   msg->setField(baseId + 9, m_zoneUIN);
}

/**
 * Channel constructor
 */
AgentTunnelCommChannel::AgentTunnelCommChannel(AgentTunnel *tunnel, UINT32 id) : m_buffer(65536, 65536)
{
   tunnel->incRefCount();
   m_tunnel = tunnel;
   m_id = id;
   m_active = true;
#ifdef _WIN32
   InitializeCriticalSectionAndSpinCount(&m_bufferLock, 4000);
   m_dataCondition = CreateEvent(NULL, TRUE, FALSE, NULL);
#else
   pthread_mutex_init(&m_bufferLock, NULL);
   pthread_cond_init(&m_dataCondition, NULL);
#endif
}

/**
 * Channel destructor
 */
AgentTunnelCommChannel::~AgentTunnelCommChannel()
{
   m_tunnel->decRefCount();
#ifdef _WIN32
   DeleteCriticalSection(&m_bufferLock);
   CloseHandle(m_dataCondition);
#else
   pthread_mutex_destroy(&m_bufferLock);
   pthread_cond_destroy(&m_dataCondition);
#endif
}

/**
 * Send data
 */
int AgentTunnelCommChannel::send(const void *data, size_t size, MUTEX mutex)
{
   return m_active ? m_tunnel->sendChannelData(m_id, data, size) : -1;
}

/**
 * Receive data
 */
int AgentTunnelCommChannel::recv(void *buffer, size_t size, UINT32 timeout)
{
   if (!m_active)
      return 0;

#ifdef _WIN32
   EnterCriticalSection(&m_bufferLock);
   if (m_buffer.isEmpty())
   {
retry_wait:
      LeaveCriticalSection(&m_bufferLock);
      if (WaitForSingleObject(m_dataCondition, timeout) == WAIT_TIMEOUT)
         return -2;

      if (!m_active)
         return 0;   // closed while waiting

      EnterCriticalSection(&m_bufferLock);
      if (m_buffer.isEmpty())
      {
         ResetEvent(m_dataCondition);
         goto retry_wait;
      }
   }
#else
   pthread_mutex_lock(&m_bufferLock);
   if (m_buffer.isEmpty())
   {
#if HAVE_PTHREAD_COND_RELTIMEDWAIT_NP
      struct timespec ts;
      ts.tv_sec = timeout / 1000;
      ts.tv_nsec = (timeout % 1000) * 1000000;
      int rc = pthread_cond_reltimedwait_np(&m_dataCondition, &m_bufferLock, &ts);
#else
      struct timeval now;
      struct timespec ts;
      gettimeofday(&now, NULL);
      ts.tv_sec = now.tv_sec + (timeout / 1000);
      now.tv_usec += (timeout % 1000) * 1000;
      ts.tv_sec += now.tv_usec / 1000000;
      ts.tv_nsec = (now.tv_usec % 1000000) * 1000;
      int rc = pthread_cond_timedwait(&m_dataCondition, &m_bufferLock, &ts);
#endif
      if (rc != 0)
      {
         pthread_mutex_unlock(&m_bufferLock);
         return -2;  // timeout
      }

      if (!m_active) // closed while waiting
      {
         pthread_mutex_unlock(&m_bufferLock);
         return 0;
      }
   }
#endif

   size_t bytes = m_buffer.read((BYTE *)buffer, size);
#ifdef _WIN32
   if (m_buffer.isEmpty())
      ResetEvent(m_dataCondition);
   LeaveCriticalSection(&m_bufferLock);
#else
   pthread_mutex_unlock(&m_bufferLock);
#endif
   return (int)bytes;
}

/**
 * Poll for data
 */
int AgentTunnelCommChannel::poll(UINT32 timeout, bool write)
{
   if (write)
      return 1;

   if (!m_active)
      return -1;

   int rc = 0;

#ifdef _WIN32
   EnterCriticalSection(&m_bufferLock);
   if (m_buffer.isEmpty())
   {
retry_wait:
      LeaveCriticalSection(&m_bufferLock);
      if (WaitForSingleObject(m_dataCondition, timeout) == WAIT_TIMEOUT)
         return 0;

      if (!m_active)
         return -1;

      EnterCriticalSection(&m_bufferLock);
      if (m_buffer.isEmpty())
      {
         ResetEvent(m_dataCondition);
         goto retry_wait;
      }
   }
   LeaveCriticalSection(&m_bufferLock);
#else
   pthread_mutex_lock(&m_bufferLock);
   if (m_buffer.isEmpty())
   {
#if HAVE_PTHREAD_COND_RELTIMEDWAIT_NP
      struct timespec ts;
      ts.tv_sec = timeout / 1000;
      ts.tv_nsec = (timeout % 1000) * 1000000;
      rc = pthread_cond_reltimedwait_np(&m_dataCondition, &m_bufferLock, &ts);
#else
      struct timeval now;
      struct timespec ts;
      gettimeofday(&now, NULL);
      ts.tv_sec = now.tv_sec + (timeout / 1000);
      now.tv_usec += (timeout % 1000) * 1000;
      ts.tv_sec += now.tv_usec / 1000000;
      ts.tv_nsec = (now.tv_usec % 1000000) * 1000;
      rc = pthread_cond_timedwait(&m_dataCondition, &m_bufferLock, &ts);
#endif
   }
   pthread_mutex_unlock(&m_bufferLock);
#endif

   return (rc == 0) ? 1 : 0;
}

/**
 * Shutdown channel
 */
int AgentTunnelCommChannel::shutdown()
{
   m_active = false;
#ifdef _WIN32
   EnterCriticalSection(&m_bufferLock);
   SetEvent(m_dataCondition);
   LeaveCriticalSection(&m_bufferLock);
#else
   pthread_cond_broadcast(&m_dataCondition);
#endif
   return 0;
}

/**
 * Close channel
 */
void AgentTunnelCommChannel::close()
{
   m_active = false;
#ifdef _WIN32
   EnterCriticalSection(&m_bufferLock);
   SetEvent(m_dataCondition);
   LeaveCriticalSection(&m_bufferLock);
#else
   pthread_cond_broadcast(&m_dataCondition);
#endif
   m_tunnel->closeChannel(this);
}

/**
 * Put data into buffer
 */
void AgentTunnelCommChannel::putData(const BYTE *data, size_t size)
{
#ifdef _WIN32
   EnterCriticalSection(&m_bufferLock);
#else
   pthread_mutex_lock(&m_bufferLock);
#endif
   m_buffer.write(data, size);
#ifdef _WIN32
   SetEvent(m_dataCondition);
   LeaveCriticalSection(&m_bufferLock);
#else
   pthread_cond_broadcast(&m_dataCondition);
   pthread_mutex_unlock(&m_bufferLock);
#endif
}

/**
 * Incoming connection data
 */
struct ConnectionRequest
{
   SOCKET sock;
   InetAddress addr;
};

/**
 * Setup tunnel
 */
static void SetupTunnel(void *arg)
{
   ConnectionRequest *request = (ConnectionRequest *)arg;

   SSL_CTX *context = NULL;
   SSL *ssl = NULL;
   AgentTunnel *tunnel = NULL;
   int rc;
   UINT32 nodeId = 0;
   UINT32 zoneUIN = 0;
   X509 *cert = NULL;

   // Setup secure connection
   const SSL_METHOD *method = SSLv23_method();
   if (method == NULL)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("SetupTunnel(%s): cannot obtain TLS method"), (const TCHAR *)request->addr.toString());
      goto failure;
   }

   context = SSL_CTX_new((SSL_METHOD *)method);
   if (context == NULL)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("SetupTunnel(%s): cannot create TLS context"), (const TCHAR *)request->addr.toString());
      goto failure;
   }
#ifdef SSL_OP_NO_COMPRESSION
   SSL_CTX_set_options(context, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION);
#else
   SSL_CTX_set_options(context, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
#endif
   if (!SetupServerTlsContext(context))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("SetupTunnel(%s): cannot configure TLS context"), (const TCHAR *)request->addr.toString());
      goto failure;
   }

   ssl = SSL_new(context);
   if (ssl == NULL)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("SetupTunnel(%s): cannot create SSL object"), (const TCHAR *)request->addr.toString());
      goto failure;
   }

   SSL_set_accept_state(ssl);
   SSL_set_fd(ssl, (int)request->sock);

retry:
   rc = SSL_do_handshake(ssl);
   if (rc != 1)
   {
      int sslErr = SSL_get_error(ssl, rc);
      if (sslErr == SSL_ERROR_WANT_READ)
      {
         SocketPoller poller;
         poller.add(request->sock);
         if (poller.poll(5000) > 0)
            goto retry;
         nxlog_debug_tag(DEBUG_TAG, 4, _T("SetupTunnel(%s): TLS handshake failed (timeout)"), (const TCHAR *)request->addr.toString());
      }
      else
      {
         char buffer[128];
         nxlog_debug_tag(DEBUG_TAG, 4, _T("SetupTunnel(%s): TLS handshake failed (%hs)"),
                     (const TCHAR *)request->addr.toString(), ERR_error_string(sslErr, buffer));
      }
      goto failure;
   }

   cert = SSL_get_peer_certificate(ssl);
   if (cert != NULL)
   {
      if (ValidateAgentCertificate(cert))
      {
         TCHAR ou[256], cn[256];
         if (GetCertificateOU(cert, ou, 256) && GetCertificateCN(cert, cn, 256))
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("SetupTunnel(%s): certificate OU=%s CN=%s"), (const TCHAR *)request->addr.toString(), ou, cn);
            uuid nodeGuid = uuid::parse(ou);
            uuid tunnelGuid = uuid::parse(cn);
            if (!nodeGuid.isNull() && !tunnelGuid.isNull())
            {
               Node *node = (Node *)FindObjectByGUID(nodeGuid, OBJECT_NODE);
               if (node != NULL)
               {
                  if (tunnelGuid.equals(node->getTunnelId()))
                  {
                     nxlog_debug_tag(DEBUG_TAG, 4, _T("SetupTunnel(%s): Tunnel attached to node %s [%d]"), (const TCHAR *)request->addr.toString(), node->getName(), node->getId());
                     if (node->getRuntimeFlags() & NDF_NEW_TUNNEL_BIND)
                     {
                        node->clearNewTunnelBindFlag();
                        node->setRecheckCapsFlag();
                        node->forceConfigurationPoll();
                     }
                     nodeId = node->getId();
                     zoneUIN = node->getZoneUIN();
                  }
                  else
                  {
                     nxlog_debug_tag(DEBUG_TAG, 4, _T("SetupTunnel(%s): Tunnel ID %s is not valid for node %s [%d]"),
                              (const TCHAR *)request->addr.toString(), (const TCHAR *)tunnelGuid.toString(),
                              node->getName(), node->getId());
                  }
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG, 4, _T("SetupTunnel(%s): Node with GUID %s not found"), (const TCHAR *)request->addr.toString(), (const TCHAR *)nodeGuid.toString());
               }
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG, 4, _T("SetupTunnel(%s): Certificate OU or CN is not a valid GUID"), (const TCHAR *)request->addr.toString());
            }
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("SetupTunnel(%s): Cannot get certificate OU and CN"), (const TCHAR *)request->addr.toString());
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("SetupTunnel(%s): Agent certificate validation failed"), (const TCHAR *)request->addr.toString());
      }
      X509_free(cert);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("SetupTunnel(%s): Agent certificate not provided"), (const TCHAR *)request->addr.toString());
   }

   tunnel = new AgentTunnel(context, ssl, request->sock, request->addr, nodeId, zoneUIN);
   RegisterTunnel(tunnel);
   tunnel->start();
   tunnel->decRefCount();

   delete request;
   return;

failure:
   if (ssl != NULL)
      SSL_free(ssl);
   if (context != NULL)
      SSL_CTX_free(context);
   shutdown(request->sock, SHUT_RDWR);
   closesocket(request->sock);
   delete request;
}

/**
 * Tunnel listener lock
 */
static Mutex s_tunnelListenerLock;

/**
 * Client listener class
 */
class TunnelListener : public SocketListener
{
protected:
   virtual ConnectionProcessingResult processConnection(SOCKET s, const InetAddress& peer);
   virtual bool isStopConditionReached();

public:
   TunnelListener(UINT16 port) : SocketListener(port) { setName(_T("AgentTunnels")); }
};

/**
 * Listener stop condition
 */
bool TunnelListener::isStopConditionReached()
{
   return IsShutdownInProgress();
}

/**
 * Process incoming connection
 */
ConnectionProcessingResult TunnelListener::processConnection(SOCKET s, const InetAddress& peer)
{
   ConnectionRequest *request = new ConnectionRequest();
   request->sock = s;
   request->addr = peer;
   ThreadPoolExecute(g_mainThreadPool, SetupTunnel, request);
   return CPR_BACKGROUND;
}

/**
 * Tunnel listener
 */
THREAD_RESULT THREAD_CALL TunnelListenerThread(void *arg)
{
   ThreadSetName("TunnelListener");
   s_tunnelListenerLock.lock();
   UINT16 listenPort = (UINT16)ConfigReadULong(_T("AgentTunnelListenPort"), 4703);
   TunnelListener listener(listenPort);
   listener.setListenAddress(g_szListenAddress);
   if (!listener.initialize())
   {
      s_tunnelListenerLock.unlock();
      return THREAD_OK;
   }

   listener.mainLoop();
   listener.shutdown();

   nxlog_debug_tag(DEBUG_TAG, 1, _T("Tunnel listener thread terminated"));
   s_tunnelListenerLock.unlock();
   return THREAD_OK;
}

/**
 * Close all active agent tunnels
 */
void CloseAgentTunnels()
{
   nxlog_debug_tag(DEBUG_TAG, 2, _T("Closing active agent tunnels..."));

   // Wait for listener thread
   s_tunnelListenerLock.lock();
   s_tunnelListenerLock.unlock();

   s_tunnelListLock.lock();
   Iterator<AgentTunnel> *it = s_boundTunnels.iterator();
   while(it->hasNext())
   {
      AgentTunnel *t = it->next();
      t->shutdown();
   }
   delete it;
   for(int i = 0; i < s_unboundTunnels.size(); i++)
      s_unboundTunnels.get(i)->shutdown();
   s_tunnelListLock.unlock();

   bool wait = true;
   while(wait)
   {
      ThreadSleepMs(500);
      s_tunnelListLock.lock();
      if ((s_boundTunnels.size() == 0) && (s_unboundTunnels.size() == 0))
         wait = false;
      s_tunnelListLock.unlock();
   }

   nxlog_debug_tag(DEBUG_TAG, 2, _T("All agent tunnels unregistered"));
}
