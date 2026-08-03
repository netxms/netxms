/*
** NetXMS - Network Management System
** Server Library
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
** File: agenttunnel.cpp
**/

#include "libnxsrv.h"
#include <agent_tunnel.h>

#include <openssl/ssl.h>

#define MAX_MSG_SIZE    268435456
#define REQUEST_TIMEOUT 10000
#define KEEPALIVE_INTERVAL 30000

#define DEBUG_TAG       _T("agent.tunnel")

/**
 * Next free tunnel ID
 */
static VolatileCounter s_nextTunnelId = 0;

/**
 * Agent tunnel constructor
 */
AgentTunnel::AgentTunnel(SSL_CTX *context, SSL *ssl, SOCKET sock, const InetAddress& addr, uint32_t nodeId,
         int32_t zoneUIN, BackgroundSocketPollerHandle *socketPoller) : m_channelLock(MutexType::FAST)
{
   m_id = InterlockedIncrement(&s_nextTunnelId);
   m_address = addr;
   m_socket = sock;
   m_socketPoller = socketPoller;
   m_messageReceiver = nullptr;
   _sntprintf(m_threadPoolKey, 12, _T("TN%u"), m_id);
   m_context = context;
   m_ssl = ssl;
   m_requestId = 0;
   m_commandTimeout = REQUEST_TIMEOUT;
   m_nodeId = nodeId;
   m_zoneUIN = zoneUIN;
   m_certificateSubject = nullptr;
   m_certificateIssuer = nullptr;
   m_certificateExpirationTime = 0;
   m_certificateIssueTime = 0;
   m_state = AGENT_TUNNEL_INIT;
   m_serialNumber = nullptr;
   m_systemName = nullptr;
   m_platformName = nullptr;
   m_systemInfo = nullptr;
   m_agentVersion = nullptr;
   m_agentBuildTag = nullptr;
   m_hostname[0] = 0;
   m_startTime = time(nullptr);
   m_userAgentInstalled = false;
   m_agentProxy = false;
   m_snmpProxy = false;
   m_snmpTrapProxy = false;
   m_syslogProxy = false;
   m_extProvCertificate = false;
}

/**
 * Agent tunnel destructor
 */
AgentTunnel::~AgentTunnel()
{
   shutdown();
   SSL_CTX_free(m_context);
   SSL_free(m_ssl);
   closesocket(m_socket);
   MemFree(m_serialNumber);
   MemFree(m_systemName);
   MemFree(m_platformName);
   MemFree(m_systemInfo);
   MemFree(m_agentVersion);
   MemFree(m_agentBuildTag);
   MemFree(m_certificateIssuer);
   MemFree(m_certificateSubject);
   delete m_messageReceiver;
   InterlockedDecrement(&m_socketPoller->usageCount);
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
 * Read data from socket
 */
bool AgentTunnel::readSocket()
{
   MessageReceiverResult result = readMessage(true);
   while(result == MSGRECV_SUCCESS)
      result = readMessage(false);
   return (result == MSGRECV_WANT_READ) || (result == MSGRECV_WANT_WRITE);
}

/**
 * Read single message from socket
 */
MessageReceiverResult AgentTunnel::readMessage(bool allowSocketRead)
{
   MessageReceiverResult result;
   NXCPMessage *msg = m_messageReceiver->readMessage(0, &result, allowSocketRead);
   if ((result == MSGRECV_WANT_READ) || (result == MSGRECV_WANT_WRITE))
      return result;

   if (result != MSGRECV_SUCCESS)
   {
      if (result == MSGRECV_CLOSED)
         debugPrintf(4, L"Tunnel closed by peer");
      else
         debugPrintf(4, L"Communication error (%s)", AbstractMessageReceiver::resultToText(result));
      return result;
   }

   if (nxlog_get_debug_level_tag(DEBUG_TAG) >= 6)
   {
      wchar_t buffer[64];
      debugPrintf(6, L"Received message %s (%u)", NXCPMessageCodeName(msg->getCode(), buffer), msg->getId());
   }

   if (msg->getCode() == CMD_CHANNEL_DATA)
   {
      if (msg->isBinary())
      {
         m_channelLock.lock();
         shared_ptr<AgentTunnelCommChannel> channel = m_channels.getShared(msg->getId());
         m_channelLock.unlock();
         if (channel != nullptr)
         {
            channel->putData(msg->getBinaryData(), msg->getBinaryDataSize());
         }
         else
         {
            debugPrintf(6, _T("Received channel data for non-existing channel %u"), msg->getId());
         }
      }
      delete msg;
   }
   else
   {
      ThreadPoolExecuteSerialized(g_agentConnectionThreadPool, m_threadPoolKey, self(), &AgentTunnel::processMessage, msg);
   }
   return MSGRECV_SUCCESS;
}

/**
 * Process incoming message
 */
void AgentTunnel::processMessage(NXCPMessage *msg)
{
   switch(msg->getCode())
   {
      case CMD_SETUP_AGENT_TUNNEL:
         setup(msg);
         break;
      case CMD_CLOSE_CHANNEL:    // channel close notification
         processChannelClose(msg->getFieldAsUInt32(VID_CHANNEL_ID));
         break;
      default:
         if (!processCustomMessage(msg))
         {
            m_queue.put(msg);
         }
         msg = nullptr; // ownership transferred to handler or wait queue
         break;
   }
   delete msg;
}

/**
 * Process tunnel message not handled by base class. Returns true if message was consumed
 * (implementation takes ownership of consumed messages).
 */
bool AgentTunnel::processCustomMessage(NXCPMessage *msg)
{
   return false;
}

/**
 * Called when tunnel setup request is successfully processed
 */
void AgentTunnel::onSetupComplete()
{
}

/**
 * Called when tunnel is being closed, before shutting down tunnel channels
 */
void AgentTunnel::onTunnelClose()
{
}

/**
 * Finalize tunnel closure
 */
void AgentTunnel::finalize()
{
   m_state = AGENT_TUNNEL_SHUTDOWN;
   onTunnelClose();

   // shutdown all channels
   m_channelLock.lock();
   auto it = m_channels.begin();
   while(it.hasNext())
   {
      AgentTunnelCommChannel *channel = it.next().get();
      channel->shutdown();
      channel->detach();
   }
   m_channels.clear();
   m_channelLock.unlock();

   debugPrintf(4, _T("Tunnel closure completed"));
}

/**
 * Socket poller callback
 */
void AgentTunnel::socketPollerCallback(BackgroundSocketPollResult pollResult, SOCKET hSocket, AgentTunnel *tunnel)
{
   if (pollResult == BackgroundSocketPollResult::SUCCESS)
   {
      if (tunnel->readSocket())
      {
         tunnel->m_socketPoller->poller.poll(hSocket, 60000, socketPollerCallback, tunnel);
         return;
      }
   }
   else
   {
      tunnel->debugPrintf(5, _T("Socket poll error (%d)"), static_cast<int>(pollResult));
   }
   ThreadPoolExecuteSerialized(g_agentConnectionThreadPool, tunnel->m_threadPoolKey, tunnel->self(), &AgentTunnel::finalize);
}

/**
 * Write to SSL
 */
int AgentTunnel::sslWrite(const void *data, size_t size)
{
   bool canRetry;
   int bytes;
   m_writeLock.lock();
   do
   {
      canRetry = false;
      m_sslLock.lock();
      bytes = SSL_write(m_ssl, data, (int)size);
      if (bytes <= 0)
      {
         int err = SSL_get_error(m_ssl, bytes);
         if ((err == SSL_ERROR_WANT_READ) || (err == SSL_ERROR_WANT_WRITE))
         {
            m_sslLock.unlock();
            SocketPoller sp(err == SSL_ERROR_WANT_WRITE);
            sp.add(m_socket);
            if (sp.poll(REQUEST_TIMEOUT) > 0)
               canRetry = true;
            m_sslLock.lock();
         }
         else
         {
            debugPrintf(7, _T("SSL_write error (bytes=%d ssl_err=%d socket_err=%d)"), bytes, err, WSAGetLastError());
            if (err == SSL_ERROR_SSL)
               LogOpenSSLErrorStack(7);
         }
      }
      m_sslLock.unlock();
   }
   while(canRetry);
   m_writeLock.unlock();
   return bytes;
}

/**
 * Send message on tunnel
 */
bool AgentTunnel::sendMessage(const NXCPMessage& msg)
{
   if (m_state == AGENT_TUNNEL_SHUTDOWN)
      return false;

   if (nxlog_get_debug_level_tag(DEBUG_TAG) >= 6)
   {
      TCHAR buffer[64];
      debugPrintf(6, _T("Sending message %s (%u)"), NXCPMessageCodeName(msg.getCode(), buffer), msg.getId());
   }
   NXCP_MESSAGE *data = msg.serialize(true);
   bool success = (sslWrite(data, ntohl(data->size)) == static_cast<int>(ntohl(data->size)));
   MemFree(data);
   return success;
}

/**
 * Start tunnel
 */
void AgentTunnel::start()
{
   debugPrintf(4, _T("Tunnel started"));
   m_messageReceiver = new TlsMessageReceiver(m_socket, m_ssl, &m_sslLock, 4096, MAX_MSG_SIZE);
   m_socketPoller->poller.poll(m_socket, 60000, socketPollerCallback, this);
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
   NXCPMessage response(CMD_REQUEST_COMPLETED, request->getId());

   if (m_state == AGENT_TUNNEL_INIT)
   {
      m_systemName = request->getFieldAsString(VID_SYS_NAME);
      m_systemInfo = request->getFieldAsString(VID_SYS_DESCRIPTION);
      m_platformName = request->getFieldAsString(VID_PLATFORM_NAME);
      m_agentId = request->getFieldAsGUID(VID_AGENT_ID);
      m_userAgentInstalled = request->getFieldAsBoolean(VID_USERAGENT_INSTALLED);
      m_agentProxy = request->getFieldAsBoolean(VID_AGENT_PROXY);
      m_snmpProxy = request->getFieldAsBoolean(VID_SNMP_PROXY);
      m_snmpTrapProxy = request->getFieldAsBoolean(VID_SNMP_TRAP_PROXY);
      m_syslogProxy = request->getFieldAsBoolean(VID_SYSLOG_PROXY);
      m_extProvCertificate = request->getFieldAsBoolean(VID_EXTPROV_CERTIFICATE);
      request->getFieldAsString(VID_HOSTNAME, m_hostname, MAX_DNS_NAME);
      m_agentVersion = request->getFieldAsString(VID_AGENT_VERSION);
      m_agentBuildTag = request->getFieldAsString(VID_AGENT_BUILD_TAG);
      if (m_agentBuildTag == nullptr)
      {
         // Agents before 3.0 release return tag as version
         m_agentBuildTag = MemCopyString(m_agentVersion);
         TCHAR *p = _tcsrchr(m_agentVersion, _T('-'));
         if (p != nullptr)
            *p = 0;  // Remove git commit hash from version string
      }
      size_t size;
      const BYTE *hardwareId = request->getBinaryFieldPtr(VID_HARDWARE_ID, &size);
      m_hardwareId = ((hardwareId != nullptr) && (size == HARDWARE_ID_LENGTH)) ?
               GenericId<HARDWARE_ID_LENGTH>(hardwareId, HARDWARE_ID_LENGTH) : GenericId<HARDWARE_ID_LENGTH>(HARDWARE_ID_LENGTH);
      m_serialNumber = request->getFieldAsString(VID_SERIAL_NUMBER);

      int count = request->getFieldAsInt32(VID_MAC_ADDR_COUNT);
      uint32_t fieldId = VID_MAC_ADDR_LIST_BASE;
      for(int i = 0; i < count; i++)
         m_macAddressList.add(request->getFieldAsMacAddress(fieldId++));

      m_state = (m_nodeId != 0) ? AGENT_TUNNEL_BOUND : AGENT_TUNNEL_UNBOUND;
      response.setField(VID_RCC, ERR_SUCCESS);
      response.setField(VID_IS_ACTIVE, m_state == AGENT_TUNNEL_BOUND);

      // For bound tunnels zone UIN taken from node object
      if (m_state != AGENT_TUNNEL_BOUND)
         m_zoneUIN = request->getFieldAsUInt32(VID_ZONE_UIN);

      TCHAR hardwareIdText[HARDWARE_ID_LENGTH * 2 + 1];
      debugPrintf(3, _T("%s tunnel initialized"), (m_state == AGENT_TUNNEL_BOUND) ? _T("Bound") : _T("Unbound"));
      debugPrintf(4, _T("   System name..............: %s"), m_systemName);
      debugPrintf(4, _T("   Hostname.................: %s"), m_hostname);
      debugPrintf(4, _T("   System information.......: %s"), m_systemInfo);
      debugPrintf(4, _T("   Platform name............: %s"), m_platformName);
      debugPrintf(4, _T("   Hardware ID..............: %s"), BinToStr(m_hardwareId.value(), HARDWARE_ID_LENGTH, hardwareIdText));
      debugPrintf(4, _T("   Serial number............: %s"), m_serialNumber);
      if (!m_macAddressList.isEmpty())
      {
         StringBuffer sb;
         for(int i = 0; i < m_macAddressList.size(); i++)
         {
            if (i > 0)
               sb.append(_T(", "));
            sb.append(m_macAddressList.get(i)->toString());
         }
         debugPrintf(4, _T("   MAC addresses............: %s"), sb.cstr());
      }
      debugPrintf(4, _T("   Agent ID.................: %s"), m_agentId.toString().cstr());
      debugPrintf(4, _T("   Agent version............: %s"), m_agentVersion);
      debugPrintf(4, _T("   Zone UIN.................: %d"), m_zoneUIN);
      debugPrintf(4, _T("   Agent proxy..............: %s"), m_agentProxy ? _T("YES") : _T("NO"));
      debugPrintf(4, _T("   SNMP proxy...............: %s"), m_snmpProxy ? _T("YES") : _T("NO"));
      debugPrintf(4, _T("   SNMP trap proxy..........: %s"), m_snmpTrapProxy ? _T("YES") : _T("NO"));
      debugPrintf(4, _T("   Syslog proxy.............: %s"), m_syslogProxy ? _T("YES") : _T("NO"));
      debugPrintf(4, _T("   User agent...............: %s"), m_userAgentInstalled ? _T("YES") : _T("NO"));
      if (m_certificateExpirationTime != 0)
      {
         debugPrintf(4, _T("   Certificate expires at...: %s"), FormatTimestamp(m_certificateExpirationTime).cstr());
         debugPrintf(4, _T("   Certificate issued at....: %s"), FormatTimestamp(m_certificateIssueTime).cstr());
         debugPrintf(4, _T("   Externally provisioned...: %s"), m_extProvCertificate ? _T("YES") : _T("NO"));
         debugPrintf(4, _T("   Certificate subject......: %s"), CHECK_NULL(m_certificateSubject));
         debugPrintf(4, _T("   Certificate issuer.......: %s"), CHECK_NULL(m_certificateIssuer));
      }

      onSetupComplete();
   }
   else
   {
      response.setField(VID_RCC, ERR_OUT_OF_STATE_REQUEST);
   }

   sendMessage(response);
}

/**
 * Create channel
 */
shared_ptr<AgentTunnelCommChannel> AgentTunnel::createChannel()
{
   if (m_state != AGENT_TUNNEL_BOUND)
   {
      debugPrintf(4, _T("createChannel: tunnel is not in bound state"));
      return shared_ptr<AgentTunnelCommChannel>();
   }

   NXCPMessage request(CMD_CREATE_CHANNEL, InterlockedIncrement(&m_requestId));
   if (!sendMessage(request))
   {
      debugPrintf(4, _T("createChannel: cannot send setup message"));
      return shared_ptr<AgentTunnelCommChannel>();
   }

   NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, request.getId());
   if (response == nullptr)
   {
      debugPrintf(4, _T("createChannel: request timeout"));
      return shared_ptr<AgentTunnelCommChannel>();
   }

   uint32_t rcc = response->getFieldAsUInt32(VID_RCC);
   if (rcc != ERR_SUCCESS)
   {
      delete response;
      debugPrintf(4, _T("createChannel: agent error %u (%s)"), rcc, AgentErrorCodeToText(rcc));
      return shared_ptr<AgentTunnelCommChannel>();
   }

   shared_ptr<AgentTunnelCommChannel> channel = make_shared<AgentTunnelCommChannel>(self(), response->getFieldAsUInt32(VID_CHANNEL_ID));
   delete response;
   m_channelLock.lock();
   if (m_state == AGENT_TUNNEL_BOUND)
   {
      m_channels.set(channel->getId(), channel);
   }
   else
   {
      channel.reset();
   }
   m_channelLock.unlock();

   if (channel != nullptr)
      debugPrintf(4, _T("createChannel: new channel created (ID=%d)"), channel->getId());
   else
      debugPrintf(4, _T("createChannel: tunnel disconnected during channel setup"));

   return channel;
}

/**
 * Process channel close notification from agent
 */
void AgentTunnel::processChannelClose(uint32_t channelId)
{
   debugPrintf(4, _T("processChannelClose: notification of channel %u closure"), channelId);

   m_channelLock.lock();
   shared_ptr<AgentTunnelCommChannel> channel = m_channels.getShared(channelId);
   m_channelLock.unlock();
   if (channel != nullptr)
   {
      channel->shutdown();
   }
}

/**
 * Close channel
 */
void AgentTunnel::closeChannel(AgentTunnelCommChannel *channel)
{
   if (m_state == AGENT_TUNNEL_SHUTDOWN)
      return;

   debugPrintf(4, _T("closeChannel: request to close channel %u"), channel->getId());

   m_channelLock.lock();
   m_channels.remove(channel->getId());
   m_channelLock.unlock();

   // Inform agent that channel is closing
   NXCPMessage msg(CMD_CLOSE_CHANNEL, InterlockedIncrement(&m_requestId));
   msg.setField(VID_CHANNEL_ID, channel->getId());
   sendMessage(msg);
}

/**
 * Send channel data
 */
ssize_t AgentTunnel::sendChannelData(uint32_t id, const void *data, size_t len)
{
   NXCP_MESSAGE *msg = CreateRawNXCPMessage(CMD_CHANNEL_DATA, id, 0, data, len, nullptr, false);
   ssize_t rc = sslWrite(msg, ntohl(msg->size));
   if (rc == static_cast<ssize_t>(ntohl(msg->size)))
      rc = len;  // adjust number of bytes to exclude tunnel overhead
   MemFree(msg);
   return rc;
}

/**
 * Fill NXCP message with tunnel data
 */
void AgentTunnel::fillMessage(NXCPMessage *msg, uint32_t baseId) const
{
   msg->setField(baseId, m_id);
   msg->setField(baseId + 1, m_guid);
   msg->setField(baseId + 2, m_nodeId);
   msg->setField(baseId + 3, m_address);
   msg->setField(baseId + 4, m_systemName);
   msg->setField(baseId + 5, m_systemInfo);
   msg->setField(baseId + 6, m_platformName);
   msg->setField(baseId + 7, m_agentVersion);
   m_channelLock.lock();
   msg->setField(baseId + 8, m_channels.size());
   m_channelLock.unlock();
   msg->setField(baseId + 9, m_zoneUIN);
   msg->setField(baseId + 10, m_hostname);
   msg->setField(baseId + 11, m_agentId);
   msg->setField(baseId + 12, m_userAgentInstalled);
   msg->setField(baseId + 13, m_agentProxy);
   msg->setField(baseId + 14, m_snmpProxy);
   msg->setField(baseId + 15, m_snmpTrapProxy);
   msg->setFieldFromTime(baseId + 16, m_certificateExpirationTime);
   msg->setField(baseId + 17, m_hardwareId.value(), HARDWARE_ID_LENGTH);
   msg->setField(baseId + 18, m_syslogProxy);
   msg->setField(baseId + 19, m_extProvCertificate);
   msg->setField(baseId + 20, m_certificateIssuer);
   msg->setField(baseId + 21, m_certificateSubject);
   msg->setFieldFromTime(baseId + 22, m_startTime);
   msg->setField(baseId + 23, m_serialNumber);
   msg->setField(baseId + 24, isInbound());
}

/**
 * Channel constructor
 */
AgentTunnelCommChannel::AgentTunnelCommChannel(const shared_ptr<AgentTunnel>& tunnel, uint32_t id) : m_tunnel(tunnel), m_buffer(65536, 65536)
{
   m_id = id;
   m_active = true;
#ifdef _WIN32
   InitializeCriticalSectionAndSpinCount(&m_bufferLock, 4000);
   InitializeConditionVariable(&m_dataCondition);
#else
#if HAVE_DECL_PTHREAD_MUTEX_ADAPTIVE_NP
   pthread_mutexattr_t a;
   pthread_mutexattr_init(&a);
   pthread_mutexattr_settype(&a, PTHREAD_MUTEX_ADAPTIVE_NP);
   pthread_mutex_init(&m_bufferLock, &a);
   pthread_mutexattr_destroy(&a);
#else
   pthread_mutex_init(&m_bufferLock, nullptr);
#endif
   pthread_cond_init(&m_dataCondition, nullptr);
#endif
   memset(m_pollers, 0, sizeof(m_pollers));
   m_pollerCount = 0;
}

/**
 * Channel destructor
 */
AgentTunnelCommChannel::~AgentTunnelCommChannel()
{
#ifdef _WIN32
   DeleteCriticalSection(&m_bufferLock);
#else
   pthread_mutex_destroy(&m_bufferLock);
   pthread_cond_destroy(&m_dataCondition);
#endif
}

/**
 * Send data
 */
ssize_t AgentTunnelCommChannel::send(const void *data, size_t size, Mutex *mutex)
{
   if (!m_active)
      return -1;
   shared_ptr<AgentTunnel> tunnel = m_tunnel.lock();
   return (tunnel != nullptr) ? tunnel->sendChannelData(m_id, data, size) : -1;
}

/**
 * Receive data
 */
ssize_t AgentTunnelCommChannel::recv(void *buffer, size_t size, uint32_t timeout)
{
   if (!m_active)
      return 0;

#ifdef _WIN32
   EnterCriticalSection(&m_bufferLock);
#else
   pthread_mutex_lock(&m_bufferLock);
#endif
   if (m_buffer.isEmpty())
   {
      if (timeout == 0)
      {
#ifdef _WIN32
         LeaveCriticalSection(&m_bufferLock);
#else
         pthread_mutex_unlock(&m_bufferLock);
#endif
         return -4;  // WANT READ
      }

#ifdef _WIN32
      // SleepConditionVariableCS is subject to spurious wakeups so we need a loop here
      BOOL signalled = FALSE;
      do
      {
         int64_t startTime = GetCurrentTimeMs();
         signalled = SleepConditionVariableCS(&m_dataCondition, &m_bufferLock, timeout);
         if (signalled)
            break;
         timeout -= std::min(timeout, static_cast<uint32_t>(GetCurrentTimeMs() - startTime));
      } while (timeout > 0);
#elif HAVE_PTHREAD_COND_RELTIMEDWAIT_NP
      struct timespec ts;
      ts.tv_sec = timeout / 1000;
      ts.tv_nsec = (timeout % 1000) * 1000000;
      bool signalled = (pthread_cond_reltimedwait_np(&m_dataCondition, &m_bufferLock, &ts) == 0);
#else
      struct timeval now;
      struct timespec ts;
      gettimeofday(&now, nullptr);
      ts.tv_sec = now.tv_sec + (timeout / 1000);
      now.tv_usec += (timeout % 1000) * 1000;
      ts.tv_sec += now.tv_usec / 1000000;
      ts.tv_nsec = (now.tv_usec % 1000000) * 1000;
      bool signalled = (pthread_cond_timedwait(&m_dataCondition, &m_bufferLock, &ts) == 0);
#endif
      if (!signalled)
      {
#ifdef _WIN32
         LeaveCriticalSection(&m_bufferLock);
#else
         pthread_mutex_unlock(&m_bufferLock);
#endif
         return -2;  // timeout
      }

      if (!m_active) // closed while waiting
      {
#ifdef _WIN32
         LeaveCriticalSection(&m_bufferLock);
#else
         pthread_mutex_unlock(&m_bufferLock);
#endif
         return 0;
      }
   }

   size_t bytes = m_buffer.read((BYTE *)buffer, size);
#ifdef _WIN32
   LeaveCriticalSection(&m_bufferLock);
#else
   pthread_mutex_unlock(&m_bufferLock);
#endif
   return (int)bytes;
}

/**
 * Poll for data
 */
int AgentTunnelCommChannel::poll(uint32_t timeout, bool write)
{
   if (write)
      return 1;

   if (!m_active)
      return -1;

#ifdef _WIN32
   EnterCriticalSection(&m_bufferLock);
#else
   pthread_mutex_lock(&m_bufferLock);
#endif
   BOOL success;
   if (m_buffer.isEmpty())
   {
#ifdef _WIN32
      // SleepConditionVariableCS is subject to spurious wakeups so we need a loop here
      success = FALSE;
      do
      {
         int64_t startTime = GetCurrentTimeMs();
         success = SleepConditionVariableCS(&m_dataCondition, &m_bufferLock, timeout);
         if (success)
            break;
         timeout -= std::min(timeout, static_cast<uint32_t>(GetCurrentTimeMs() - startTime));
      } while (timeout > 0);
#elif HAVE_PTHREAD_COND_RELTIMEDWAIT_NP
      struct timespec ts;
      ts.tv_sec = timeout / 1000;
      ts.tv_nsec = (timeout % 1000) * 1000000;
      success = (pthread_cond_reltimedwait_np(&m_dataCondition, &m_bufferLock, &ts) == 0);
#else
      struct timeval now;
      struct timespec ts;
      gettimeofday(&now, nullptr);
      ts.tv_sec = now.tv_sec + (timeout / 1000);
      now.tv_usec += (timeout % 1000) * 1000;
      ts.tv_sec += now.tv_usec / 1000000;
      ts.tv_nsec = (now.tv_usec % 1000000) * 1000;
      success = (pthread_cond_timedwait(&m_dataCondition, &m_bufferLock, &ts) == 0);
#endif
   }
   else
   {
      success = TRUE;
   }
#ifdef _WIN32
   LeaveCriticalSection(&m_bufferLock);
#else
   pthread_mutex_unlock(&m_bufferLock);
#endif

   return success ? 1 : 0;
}

/**
 * Start background poll
 */
void AgentTunnelCommChannel::backgroundPoll(uint32_t timeout, void (*callback)(BackgroundSocketPollResult, AbstractCommChannel*, void*), void *context)
{
#ifdef _WIN32
   EnterCriticalSection(&m_bufferLock);
#else
   pthread_mutex_lock(&m_bufferLock);
#endif
   if (m_active)
   {
      if (m_buffer.isEmpty())
      {
         if (m_pollerCount < 16)
         {
            m_pollers[m_pollerCount].callback = callback;
            m_pollers[m_pollerCount].context = context;
            m_pollerCount++;
         }
         else
         {
            ThreadPoolExecute(g_agentConnectionThreadPool, callback, BackgroundSocketPollResult::FAILURE, static_cast<AbstractCommChannel*>(this), context);
         }
      }
      else
      {
         ThreadPoolExecute(g_agentConnectionThreadPool, callback, BackgroundSocketPollResult::SUCCESS, static_cast<AbstractCommChannel*>(this), context);
      }
   }
   else
   {
      ThreadPoolExecute(g_agentConnectionThreadPool, callback, BackgroundSocketPollResult::SHUTDOWN, static_cast<AbstractCommChannel*>(this), context);
   }
#ifdef _WIN32
   LeaveCriticalSection(&m_bufferLock);
#else
   pthread_mutex_unlock(&m_bufferLock);
#endif
}

/**
 * Shutdown channel
 */
int AgentTunnelCommChannel::shutdown()
{
#ifdef _WIN32
   EnterCriticalSection(&m_bufferLock);
#else
   pthread_mutex_lock(&m_bufferLock);
#endif
   m_active = false;
   if (m_pollerCount > 0)
   {
      for(int i = 0; i < m_pollerCount; i++)
         ThreadPoolExecute(g_agentConnectionThreadPool, m_pollers[i].callback, BackgroundSocketPollResult::SHUTDOWN, static_cast<AbstractCommChannel*>(this), m_pollers[i].context);
      m_pollerCount = 0;
   }
#ifdef _WIN32
   WakeAllConditionVariable(&m_dataCondition);
   LeaveCriticalSection(&m_bufferLock);
#else
   pthread_cond_broadcast(&m_dataCondition);
   pthread_mutex_unlock(&m_bufferLock);
#endif
   return 0;
}

/**
 * Close channel
 */
void AgentTunnelCommChannel::close()
{
   shutdown();
   shared_ptr<AgentTunnel> tunnel = m_tunnel.lock();
   if (tunnel != nullptr)
      tunnel->closeChannel(this);
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
   if (m_pollerCount > 0)
   {
      for(int i = 0; i < m_pollerCount; i++)
         ThreadPoolExecute(g_agentConnectionThreadPool, m_pollers[i].callback, BackgroundSocketPollResult::SUCCESS, static_cast<AbstractCommChannel*>(this), m_pollers[i].context);
      m_pollerCount = 0;
   }
#ifdef _WIN32
   WakeAllConditionVariable(&m_dataCondition);
   LeaveCriticalSection(&m_bufferLock);
#else
   pthread_cond_broadcast(&m_dataCondition);
   pthread_mutex_unlock(&m_bufferLock);
#endif
}

/**
 * Registry of active outbound tunnels (holds references so that tunnel objects stay alive
 * while socket poller is using them)
 */
static SharedHashMap<uint32_t, OutboundAgentTunnel> s_outboundTunnels;
static Mutex s_outboundTunnelListLock(MutexType::FAST);

/**
 * Socket pollers for outbound tunnels
 */
static ObjectArray<BackgroundSocketPollerHandle> s_outboundTunnelPollers(8, 8, Ownership::True);
static Mutex s_outboundTunnelPollerListLock(MutexType::FAST);

/**
 * Acquire socket poller for outbound tunnel
 */
static BackgroundSocketPollerHandle *AcquireOutboundTunnelPoller()
{
   LockGuard lockGuard(s_outboundTunnelPollerListLock);
   for(int i = 0; i < s_outboundTunnelPollers.size(); i++)
   {
      BackgroundSocketPollerHandle *p = s_outboundTunnelPollers.get(i);
      if (static_cast<uint32_t>(InterlockedIncrement(&p->usageCount)) < MIN(SOCKET_POLLER_MAX_SOCKETS - 1, 256))
         return p;
      InterlockedDecrement(&p->usageCount);
   }
   auto p = new BackgroundSocketPollerHandle();
   p->usageCount = 1;
   s_outboundTunnelPollers.add(p);
   return p;
}

/**
 * Outbound tunnel constructor
 */
OutboundAgentTunnel::OutboundAgentTunnel(SSL_CTX *context, SSL *ssl, SOCKET sock, const InetAddress& addr, uint32_t nodeId,
         int32_t zoneUIN, BackgroundSocketPollerHandle *socketPoller) : AgentTunnel(context, ssl, sock, addr, nodeId, zoneUIN, socketPoller),
         m_setupCondition(true)
{
}

/**
 * Setup completion handler for outbound tunnels
 */
void OutboundAgentTunnel::onSetupComplete()
{
   m_state = AGENT_TUNNEL_BOUND;   // Outbound tunnels are always created for known peer and are immediately active
   m_setupCondition.set();
}

/**
 * Close handler for outbound tunnels
 */
void OutboundAgentTunnel::onTunnelClose()
{
   if (m_closeCallback != nullptr)
      m_closeCallback(this);

   s_outboundTunnelListLock.lock();
   s_outboundTunnels.remove(m_id);
   s_outboundTunnelListLock.unlock();
}

/**
 * Keepalive timer for outbound tunnel. Agent will close inbound connection without traffic
 * on idle timeout, so server should ping the agent periodically.
 */
void OutboundAgentTunnel::keepaliveTimer(const shared_ptr<OutboundAgentTunnel>& tunnel)
{
   if (tunnel->m_state != AGENT_TUNNEL_BOUND)
      return;

   NXCPMessage msg(CMD_KEEPALIVE, InterlockedIncrement(&tunnel->m_requestId));
   if (tunnel->sendMessage(msg))
   {
      NXCPMessage *response = tunnel->waitForMessage(CMD_KEEPALIVE, msg.getId());
      if (response == nullptr)
      {
         tunnel->debugPrintf(4, _T("Keepalive check failed (request timeout)"));
         tunnel->shutdown();
         return;
      }
      delete response;
   }
   else
   {
      tunnel->debugPrintf(4, _T("Keepalive check failed (cannot send message)"));
      tunnel->shutdown();
      return;
   }

   ThreadPoolScheduleRelative(g_agentConnectionThreadPool, KEEPALIVE_INTERVAL, keepaliveTimer, tunnel);
}

/**
 * Establish outbound tunnel to agent
 */
shared_ptr<OutboundAgentTunnel> OutboundAgentTunnel::establish(const InetAddress& addr, uint16_t port, uint32_t nodeId, int32_t zoneUIN,
         const BYTE *expectedFingerprint, BYTE *actualFingerprint, AgentTunnelEstablishmentStatus *status,
         bool (*tlsContextSetup)(SSL_CTX*), std::function<void(OutboundAgentTunnel*)> closeCallback)
{
   TCHAR peerName[64];
   addr.toString(peerName);

   AgentTunnelEstablishmentStatus dummyStatus;
   if (status == nullptr)
      status = &dummyStatus;
   *status = AgentTunnelEstablishmentStatus::CONNECT_FAILED;

   SOCKET s = ConnectToHost(addr, port, REQUEST_TIMEOUT);
   if (s == INVALID_SOCKET)
   {
      TCHAR buffer[256];
      nxlog_debug_tag(DEBUG_TAG, 5, _T("EstablishOutboundTunnel(%s): cannot establish connection (%s)"), peerName, GetLastSocketErrorText(buffer, 256));
      return shared_ptr<OutboundAgentTunnel>();
   }

   *status = AgentTunnelEstablishmentStatus::TLS_HANDSHAKE_FAILED;

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   const SSL_METHOD *method = TLS_method();
#else
   const SSL_METHOD *method = SSLv23_method();
#endif
   SSL_CTX *context = (method != nullptr) ? SSL_CTX_new((SSL_METHOD *)method) : nullptr;
   if (context == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("EstablishOutboundTunnel(%s): cannot create TLS context"), peerName);
      closesocket(s);
      return shared_ptr<OutboundAgentTunnel>();
   }
#ifdef SSL_OP_NO_COMPRESSION
   SSL_CTX_set_options(context, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION);
#else
   SSL_CTX_set_options(context, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
#endif

   if ((tlsContextSetup != nullptr) && !tlsContextSetup(context))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("EstablishOutboundTunnel(%s): cannot configure TLS context"), peerName);
      SSL_CTX_free(context);
      closesocket(s);
      return shared_ptr<OutboundAgentTunnel>();
   }

   SSL *ssl = SSL_new(context);
   if (ssl == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("EstablishOutboundTunnel(%s): cannot create SSL object"), peerName);
      SSL_CTX_free(context);
      closesocket(s);
      return shared_ptr<OutboundAgentTunnel>();
   }

   SSL_set_connect_state(ssl);
   SSL_set_fd(ssl, (int)s);
   SetSocketNonBlocking(s);

   while(true)
   {
      int rc = SSL_do_handshake(ssl);
      if (rc == 1)
         break;

      int sslErr = SSL_get_error(ssl, rc);
      if ((sslErr == SSL_ERROR_WANT_READ) || (sslErr == SSL_ERROR_WANT_WRITE))
      {
         SocketPoller poller(sslErr == SSL_ERROR_WANT_WRITE);
         poller.add(s);
         if (poller.poll(REQUEST_TIMEOUT) > 0)
            continue;
         nxlog_debug_tag(DEBUG_TAG, 5, _T("EstablishOutboundTunnel(%s): TLS handshake failed (timeout)"), peerName);
      }
      else
      {
         char buffer[128];
         nxlog_debug_tag(DEBUG_TAG, 5, _T("EstablishOutboundTunnel(%s): TLS handshake failed (%hs)"), peerName, ERR_error_string(sslErr, buffer));
         unsigned long error;
         while((error = ERR_get_error()) != 0)
         {
            ERR_error_string_n(error, buffer, sizeof(buffer));
            nxlog_debug_tag(DEBUG_TAG, 6, _T("EstablishOutboundTunnel(%s): caused by: %hs"), peerName, buffer);
         }
      }
      SSL_free(ssl);
      SSL_CTX_free(context);
      closesocket(s);
      return shared_ptr<OutboundAgentTunnel>();
   }

   // Verify agent certificate fingerprint
   X509 *cert = SSL_get_peer_certificate(ssl);
   if (cert == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("EstablishOutboundTunnel(%s): agent certificate not provided"), peerName);
      SSL_free(ssl);
      SSL_CTX_free(context);
      closesocket(s);
      return shared_ptr<OutboundAgentTunnel>();
   }

   BYTE fingerprint[SHA256_DIGEST_SIZE];
   bool digestSuccess = (X509_digest(cert, EVP_sha256(), fingerprint, nullptr) != 0);
   X509_free(cert);
   if (!digestSuccess)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("EstablishOutboundTunnel(%s): cannot calculate agent certificate fingerprint"), peerName);
      SSL_free(ssl);
      SSL_CTX_free(context);
      closesocket(s);
      return shared_ptr<OutboundAgentTunnel>();
   }

   if (actualFingerprint != nullptr)
      memcpy(actualFingerprint, fingerprint, SHA256_DIGEST_SIZE);

   if ((expectedFingerprint != nullptr) && memcmp(expectedFingerprint, fingerprint, SHA256_DIGEST_SIZE))
   {
      *status = AgentTunnelEstablishmentStatus::CERTIFICATE_MISMATCH;
      nxlog_debug_tag(DEBUG_TAG, 3, _T("EstablishOutboundTunnel(%s): agent certificate fingerprint mismatch"), peerName);
      SSL_free(ssl);
      SSL_CTX_free(context);
      closesocket(s);
      return shared_ptr<OutboundAgentTunnel>();
   }

   nxlog_debug_tag(DEBUG_TAG, 6, _T("EstablishOutboundTunnel(%s): TLS handshake completed"), peerName);

   shared_ptr<OutboundAgentTunnel> tunnel = shared_ptr<OutboundAgentTunnel>(new OutboundAgentTunnel(context, ssl, s, addr, nodeId, zoneUIN, AcquireOutboundTunnelPoller()));
   tunnel->m_self = tunnel;
   tunnel->m_closeCallback = closeCallback;

   s_outboundTunnelListLock.lock();
   s_outboundTunnels.set(tunnel->getId(), tunnel);
   s_outboundTunnelListLock.unlock();

   tunnel->start();

   // Agent sends tunnel setup request after TLS session establishment
   if (!tunnel->m_setupCondition.wait(REQUEST_TIMEOUT))
   {
      *status = AgentTunnelEstablishmentStatus::SETUP_TIMEOUT;
      nxlog_debug_tag(DEBUG_TAG, 4, _T("EstablishOutboundTunnel(%s): timeout waiting for tunnel setup request from agent"), peerName);
      tunnel->shutdown();
      return shared_ptr<OutboundAgentTunnel>();
   }

   ThreadPoolScheduleRelative(g_agentConnectionThreadPool, KEEPALIVE_INTERVAL, keepaliveTimer, tunnel);

   *status = AgentTunnelEstablishmentStatus::SUCCESS;
   nxlog_debug_tag(DEBUG_TAG, 4, _T("EstablishOutboundTunnel(%s): outbound tunnel established"), peerName);
   return tunnel;
}
