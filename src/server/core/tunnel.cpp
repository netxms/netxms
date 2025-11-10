/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
#include <nms_users.h>

#define MAX_MSG_SIZE    268435456

#define REQUEST_TIMEOUT 10000

#define DEBUG_TAG       _T("agent.tunnel")

/**
 * Externally provisioned certificate mapping
 */
static SharedStringObjectMap<Node> s_certificateMappings;
static Mutex s_certificateMappingsLock;

/**
 * Tunnel registration
 */
static SharedHashMap<uint32_t, AgentTunnel> s_tunnels;   // All tunnels indexed by tunnel ID
static SharedHashMap<uint32_t, AgentTunnel> s_boundTunnels;
static SharedObjectArray<AgentTunnel> s_unboundTunnels(16, 16);
static Mutex s_tunnelListLock(MutexType::FAST);
static VolatileCounter s_activeSetupCalls = 0;  // Number of tunnel setup calls currently running

/**
 * Socket pollers
 */
static ObjectArray<BackgroundSocketPollerHandle> s_pollers(64, 64, Ownership::True);
static uint32_t s_maxTunnelsPerPoller = MIN(SOCKET_POLLER_MAX_SOCKETS - 1, 256);
static Mutex s_pollerListLock(MutexType::FAST);

/**
 * Execute tunnel establishing hook script in the separate thread
 */
static void ExecuteScriptInBackground(NXSL_VM *vm, const TCHAR *scriptName)
{
   if (!vm->run())
   {
      ReportScriptError(SCRIPT_CONTEXT_TUNNEL, nullptr, 0, vm->getErrorText(), scriptName);
   }
   delete vm;
}

/**
 * Execute hook script when bound tunnel established
 */
static void ExecuteTunnelHookScript(const shared_ptr<AgentTunnel>& tunnel)
{
   const TCHAR *scriptName = tunnel->isBound() ? _T("Hook::OpenBoundTunnel") : _T("Hook::OpenUnboundTunnel");
   shared_ptr<NetObj> node = tunnel->isBound() ? FindObjectById(tunnel->getNodeId(), OBJECT_NODE) : shared_ptr<NetObj>();
   ScriptVMHandle vm = CreateServerScriptVM(scriptName, node);
   if (!vm.isValid())
   {
      tunnel->debugPrintf(5, _T("Hook script %s"), (vm.failureReason() == ScriptVMFailureReason::SCRIPT_IS_EMPTY) ? _T("is empty") : _T("not found"));
      return;
   }

   vm->setGlobalVariable("$tunnel", vm->createValue(vm->createObject(&g_nxslTunnelClass, new shared_ptr<AgentTunnel>(tunnel))));
   ThreadPoolExecute(g_mainThreadPool, ExecuteScriptInBackground, vm.vm(), scriptName);
}

/**
 * Register tunnel
 */
static void RegisterTunnel(const shared_ptr<AgentTunnel>& tunnel)
{
   s_tunnelListLock.lock();
   s_tunnels.set(tunnel->getId(), tunnel);
   if (tunnel->isBound())
   {
      tunnel->setReplacedTunnel(s_boundTunnels.getShared(tunnel->getNodeId()));
      s_boundTunnels.set(tunnel->getNodeId(), tunnel);
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
      EventBuilder(EVENT_TUNNEL_CLOSED, tunnel->getNodeId())
         .param(_T("tunnelId"), tunnel->getId())
         .param(_T("ipAddress"), tunnel->getAddress())
         .param(_T("systemName"), tunnel->getSystemName())
         .param(_T("hostName"), tunnel->getHostname())
         .param(_T("platformName"), tunnel->getPlatformName())
         .param(_T("systemInfo"), tunnel->getSystemInfo())
         .param(_T("agentVersion"), tunnel->getAgentVersion())
         .param(_T("agentId"), tunnel->getAgentId())
         .post();

      // Check that current tunnel for node is tunnel being unregistered
      // New tunnel could be established while old one still finishing
      // outstanding requests
      AgentTunnel *registeredTunnel = s_boundTunnels.get(tunnel->getNodeId());
      if ((registeredTunnel != nullptr) && (registeredTunnel->getId() == tunnel->getId()))
         s_boundTunnels.remove(tunnel->getNodeId());
   }
   else
   {
      for(int i = 0; i < s_unboundTunnels.size(); i++)
         if (s_unboundTunnels.get(i)->getId() == tunnel->getId())
         {
            s_unboundTunnels.remove(i);
            break;
         }
   }
   s_tunnels.remove(tunnel->getId());
   s_tunnelListLock.unlock();
   NotifyClientSessions(NX_NOTIFY_AGENT_TUNNEL_CLOSED, tunnel->getId(), NXC_CHANNEL_AGENT_TUNNELS);
}

/**
 * Get tunnel for node. Caller must decrease reference counter on tunnel.
 */
shared_ptr<AgentTunnel> GetTunnelForNode(uint32_t nodeId)
{
   s_tunnelListLock.lock();
   shared_ptr<AgentTunnel> tunnel = s_boundTunnels.getShared(nodeId);
   s_tunnelListLock.unlock();
   return tunnel;
}

/**
 * Bind agent tunnel
 */
uint32_t BindAgentTunnel(uint32_t tunnelId, uint32_t nodeId, uint32_t userId)
{
   shared_ptr<AgentTunnel> tunnel;
   s_tunnelListLock.lock();
   for(int i = 0; i < s_unboundTunnels.size(); i++)
   {
      if (s_unboundTunnels.get(i)->getId() == tunnelId)
      {
         tunnel = s_unboundTunnels.getShared(i);
         break;
      }
   }
   s_tunnelListLock.unlock();

   if (tunnel == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("BindAgentTunnel: unbound tunnel with ID %u not found"), tunnelId);
      return RCC_INVALID_TUNNEL_ID;
   }

   TCHAR userName[MAX_USER_NAME];
   nxlog_debug_tag(DEBUG_TAG, 4, _T("BindAgentTunnel: processing bind request %u -> %u by user %s"),
            tunnelId, nodeId, ResolveUserId(userId, userName, true));
   uint32_t rcc = tunnel->bind(nodeId, userId);
   return rcc;
}

/**
 * Unbind agent tunnel from node
 */
uint32_t UnbindAgentTunnel(uint32_t nodeId, uint32_t userId)
{
   shared_ptr<NetObj> node = FindObjectById(nodeId, OBJECT_NODE);
   if (node == nullptr)
      return RCC_INVALID_OBJECT_ID;

   wchar_t userName[MAX_USER_NAME];
   nxlog_debug_tag(DEBUG_TAG, 4, L"UnbindAgentTunnel: processing unbind request for node %s [%u] by user %s",
            node->getName(), nodeId, ResolveUserId(userId, userName, true));

   if (static_cast<Node&>(*node).getTunnelId().isNull())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"UnbindAgentTunnel: node %s [%u] does not have assigned tunnel ID", node->getName(), nodeId);

      // Node still can have active tunnel if it was attached by IP address
      shared_ptr<AgentTunnel> tunnel = GetTunnelForNode(nodeId);
      if (tunnel != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, L"UnbindAgentTunnel(%s): shutting down existing tunnel", node->getName());
         tunnel->shutdown();
      }
      return RCC_SUCCESS;  // tunnel is not set
   }

   wchar_t subject[256];
   _sntprintf(subject, 256, _T("OU=%s,CN=%s"), node->getGuid().toString().cstr(), static_cast<Node&>(*node).getTunnelId().toString().cstr());
   LogCertificateAction(REVOKE_CERTIFICATE, userId, nodeId, node->getGuid(), CERT_TYPE_AGENT,
            (static_cast<Node&>(*node).getAgentCertificateSubject() != nullptr) ? static_cast<Node&>(*node).getAgentCertificateSubject() : subject, 0);

   static_cast<Node&>(*node).setTunnelId(uuid::NULL_UUID, nullptr);

   shared_ptr<AgentTunnel> tunnel = GetTunnelForNode(nodeId);
   if (tunnel != nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"UnbindAgentTunnel(%s): shutting down existing tunnel", node->getName());
      tunnel->shutdown();
   }

   return RCC_SUCCESS;
}

/**
 * Get list of agent tunnels into NXCP message
 */
void GetAgentTunnels(NXCPMessage *msg)
{
   s_tunnelListLock.lock();

   uint32_t fieldId = VID_ELEMENT_LIST_BASE;
   for(int i = 0; i < s_unboundTunnels.size(); i++)
   {
      s_unboundTunnels.get(i)->fillMessage(msg, fieldId);
      fieldId += 64;
   }

   auto it = s_boundTunnels.begin();
   while(it.hasNext())
   {
      it.next()->fillMessage(msg, fieldId);
      fieldId += 64;
   }

   msg->setField(VID_NUM_ELEMENTS, static_cast<uint32_t>(s_unboundTunnels.size() + s_boundTunnels.size()));
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
            _T(" ID  | Node ID | EP  | Chan. | Peer IP Address          | System Name              | Hostname                 | Platform Name    | Agent Version | Agent Build Tag\n")
            _T("-----+---------+-----+-------+--------------------------+--------------------------+--------------------------+------------------+---------------+--------------------------\n"));
   for(const shared_ptr<AgentTunnel>& t : s_boundTunnels)
   {
      TCHAR ipAddrBuffer[64];
      ConsolePrintf(console, _T("%4d | %7u | %-3s | %5d | %-24s | %-24s | %-24s | %-16s | %-13s | %s\n"), t->getId(), t->getNodeId(),
               t->isExtProvCertificate() ? _T("YES") : _T("NO"), t->getChannelCount(), t->getAddress().toString(ipAddrBuffer), t->getSystemName(),
               t->getHostname(), t->getPlatformName(), t->getAgentVersion(), t->getAgentBuildTag());
   }

   ConsolePrintf(console,
            _T("\n\x1b[1mUNBOUND TUNNELS\x1b[0m\n")
            _T(" ID  | EP  | Peer IP Address          | System Name              | Hostname                 | Platform Name    | Agent Version | Agent Build Tag\n")
            _T("-----+-----+--------------------------+--------------------------+--------------------------+------------------+---------------+------------------------------------\n"));
   for(const shared_ptr<AgentTunnel>& t : s_unboundTunnels)
   {
      TCHAR ipAddrBuffer[64];
      ConsolePrintf(console, _T("%4d | %-3s | %-24s | %-24s | %-24s | %-16s | %-13s | %s\n"), t->getId(), t->isExtProvCertificate() ? _T("YES") : _T("NO"),
               t->getAddress().toString(ipAddrBuffer), t->getSystemName(), t->getHostname(), t->getPlatformName(), t->getAgentVersion(), t->getAgentBuildTag());
   }

   s_tunnelListLock.unlock();
}

/**
 * Create shared tunnel object
 */
shared_ptr<AgentTunnel> AgentTunnel::create(SSL_CTX *context, SSL *ssl, SOCKET sock, const InetAddress& addr, uint32_t nodeId,
         int32_t zoneUIN, const TCHAR *certificateSubject, const TCHAR *certificateIssuer, time_t certificateExpirationTime,
         time_t certificateIssueTime, BackgroundSocketPollerHandle *socketPoller)
{
   shared_ptr<AgentTunnel> tunnel = make_shared<AgentTunnel>(context, ssl, sock, addr, nodeId, zoneUIN, certificateSubject,
            certificateIssuer, certificateExpirationTime, certificateIssueTime, socketPoller);
   tunnel->m_self = tunnel;
   return tunnel;
}

/**
 * Next free tunnel ID
 */
static VolatileCounter s_nextTunnelId = 0;

/**
 * Agent tunnel constructor
 */
AgentTunnel::AgentTunnel(SSL_CTX *context, SSL *ssl, SOCKET sock, const InetAddress& addr,
         uint32_t nodeId, int32_t zoneUIN, const TCHAR *certificateSubject, const TCHAR *certificateIssuer,
         time_t certificateExpirationTime, time_t certificateIssueTime, BackgroundSocketPollerHandle *socketPoller) : m_channelLock(MutexType::FAST)
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
   m_nodeId = nodeId;
   m_zoneUIN = zoneUIN;
   m_certificateSubject = MemCopyString(certificateSubject);
   m_certificateIssuer = MemCopyString(certificateIssuer);
   m_certificateExpirationTime = certificateExpirationTime;
   m_certificateIssueTime = certificateIssueTime;
   m_state = AGENT_TUNNEL_INIT;
   m_serialNumber = nullptr;
   m_systemName = nullptr;
   m_platformName = nullptr;
   m_systemInfo = nullptr;
   m_agentVersion = nullptr;
   m_agentBuildTag = nullptr;
   m_bindRequestId = 0;
   m_bindUserId = 0;
   m_hostname[0] = 0;
   m_startTime = time(nullptr);
   m_userAgentInstalled = false;
   m_agentProxy = false;
   m_snmpProxy = false;
   m_snmpTrapProxy = false;
   m_syslogProxy = false;
   m_extProvCertificate = false;
   m_resetPending = false;
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
         debugPrintf(4, _T("Tunnel closed by peer"));
      else
         debugPrintf(4, _T("Communication error (%s)"), AbstractMessageReceiver::resultToText(result));
      return result;
   }

   if (nxlog_get_debug_level_tag(DEBUG_TAG) >= 6)
   {
      TCHAR buffer[64];
      debugPrintf(6, _T("Received message %s (%u)"), NXCPMessageCodeName(msg->getCode(), buffer), msg->getId());
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
      case CMD_KEEPALIVE:
         {
            NXCPMessage response(CMD_KEEPALIVE, msg->getId());
            sendMessage(response);
         }
         break;
      case CMD_SETUP_AGENT_TUNNEL:
         setup(msg);
         break;
      case CMD_REQUEST_CERTIFICATE:
         processCertificateRequest(msg);
         break;
      case CMD_CLOSE_CHANNEL:    // channel close notification
         processChannelClose(msg->getFieldAsUInt32(VID_CHANNEL_ID));
         break;
      default:
         m_queue.put(msg);
         msg = nullptr; // prevent message deletion
         break;
   }
   delete msg;
}

/**
 * Finalize tunnel closure
 */
void AgentTunnel::finalize()
{
   m_state = AGENT_TUNNEL_SHUTDOWN;
   UnregisterTunnel(this);

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
 * Background certificate renewal
 */
static void BackgroundRenewCertificate(const shared_ptr<AgentTunnel>& tunnel)
{
   uint32_t rcc = tunnel->renewCertificate();
   if (rcc == RCC_SUCCESS)
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Agent certificate successfully renewed for %s (%s)"),
               tunnel->getDisplayName(), tunnel->getAddress().toString().cstr());
   else
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Agent certificate renewal failed for %s (%s) with error %u"),
               tunnel->getDisplayName(), tunnel->getAddress().toString().cstr(), rcc);
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
      m_hardwareId = ((hardwareId != nullptr) && (size == HARDWARE_ID_LENGTH)) ? NodeHardwareId(hardwareId) : NodeHardwareId();
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

      ExecuteTunnelHookScript(self());

      auto msg = new NXCPMessage(CMD_AGENT_TUNNEL_UPDATE, 0);
      fillMessage(msg, VID_ELEMENT_LIST_BASE);
      msg->setField(VID_NOTIFICATION_CODE, NX_NOTIFY_AGENT_TUNNEL_OPEN);
      ThreadPoolExecute(g_clientThreadPool,
         [msg] () -> void
         {
            NotifyClientSessions(*msg, NXC_CHANNEL_AGENT_TUNNELS);
            delete msg;
         });

      if (m_state == AGENT_TUNNEL_BOUND)
      {
         EventBuilder(EVENT_TUNNEL_OPEN, m_nodeId)
            .param(_T("tunnelId"), m_id)
            .param(_T("ipAddress"), m_address)
            .param(_T("systemName"), m_systemName)
            .param(_T("hostName"), m_hostname)
            .param(_T("platformName"), m_platformName)
            .param(_T("systemInfo"), m_systemInfo)
            .param(_T("agentVersion"), m_agentVersion)
            .param(_T("agentId"), m_agentId)
            .post();

         int32_t reissueInterval = ConfigReadInt(_T("AgentTunnels.Certificates.ReissueInterval"), 30) * 86400;
         time_t now = time(nullptr);
         if (!m_extProvCertificate && (m_certificateExpirationTime > 0) &&
             ((m_certificateExpirationTime - now <= 2592000) || (now - m_certificateIssueTime >= reissueInterval))) // 30 days
         {
            debugPrintf(4, _T("Certificate will expire soon, requesting renewal"));
            ThreadPoolExecute(g_mainThreadPool, BackgroundRenewCertificate, self());
         }

         if (m_replacedTunnel != nullptr)
         {
            if (!m_replacedTunnel->getHardwareId().equals(m_hardwareId) ||
                !m_replacedTunnel->getAgentId().equals(m_agentId) ||
                _tcscmp(m_replacedTunnel->getHostname(), m_hostname) ||
                _tcscmp(m_replacedTunnel->getSystemName(), m_systemName))
            {
               // Old and new tunnels seems to be from different machines but binding to same node
               debugPrintf(3, _T("Host data mismatch with existing tunnel (IP address: %s -> %s, Hostname: \"%s\" -> \"%s\", System name: \"%s\" -> \"%s\")"),
                        m_replacedTunnel->getAddress().toString().cstr(), m_address.toString().cstr(),
                        m_replacedTunnel->getHostname(), m_hostname, m_replacedTunnel->getSystemName(), m_systemName);
               EventBuilder(EVENT_TUNNEL_HOST_DATA_MISMATCH, m_nodeId)
                  .param(_T("tunnelId"), m_id)
                  .param(_T("oldIPAddress"), m_replacedTunnel->getAddress())
                  .param(_T("newIPAddress"), m_address)
                  .param(_T("oldSystemName"), m_replacedTunnel->getSystemName())
                  .param(_T("newSystemName"), m_systemName)
                  .param(_T("oldHostName"), m_replacedTunnel->getHostname())
                  .param(_T("newHostName"), m_hostname)
                  .param(_T("oldHardwareId"), m_replacedTunnel->getHardwareId().toString())
                  .param(_T("newHardwareId"), m_hardwareId.toString())
                  .post();
            }
            m_replacedTunnel.reset();
         }
      }
   }
   else
   {
      response.setField(VID_RCC, ERR_OUT_OF_STATE_REQUEST);
   }

   sendMessage(response);
}

/**
 * Bind tunnel to node
 */
uint32_t AgentTunnel::bind(uint32_t nodeId, uint32_t userId)
{
   if ((m_state != AGENT_TUNNEL_UNBOUND) || (m_bindRequestId != 0) || m_extProvCertificate)
      return RCC_OUT_OF_STATE_REQUEST;

   shared_ptr<NetObj> node = FindObjectById(nodeId, OBJECT_NODE);
   if (node == nullptr)
      return RCC_INVALID_OBJECT_ID;

   if (!static_cast<Node&>(*node).getAgentId().isNull() && !static_cast<Node&>(*node).getAgentId().equals(m_agentId))
   {
      debugPrintf(3, _T("Node agent ID (%s) do not match tunnel agent ID (%s) on bind"),
               static_cast<Node&>(*node).getAgentId().toString().cstr(), m_agentId.toString().cstr());
      EventBuilder(EVENT_TUNNEL_AGENT_ID_MISMATCH, nodeId)
         .param(_T("tunnelId"), m_id)
         .param(_T("ipAddress"), m_address)
         .param(_T("systemName"), m_systemName)
         .param(_T("hostName"), m_hostname)
         .param(_T("platformName"), m_platformName)
         .param(_T("systemInfo"), m_systemInfo)
         .param(_T("agentVersion"), m_agentVersion)
         .param(_T("tunnelAgentId"), static_cast<Node&>(*node).getAgentId())
         .param(_T("nodeAgentId"), m_agentId)
         .post();
   }

   uint32_t rcc = initiateCertificateRequest(node->getGuid(), userId);
   if (rcc == RCC_SUCCESS)
   {
      debugPrintf(4, _T("Bind successful, resetting tunnel"));
      m_resetPending = true;
      static_cast<Node&>(*node).setNewTunnelBindFlag();
      NXCPMessage msg(CMD_RESET_TUNNEL, InterlockedIncrement(&m_requestId));
      sendMessage(msg);
   }
   return rcc;
}

/**
 * Renew agent certificate
 */
uint32_t AgentTunnel::renewCertificate()
{
   shared_ptr<NetObj> node = FindObjectById(m_nodeId, OBJECT_NODE);
   if (node == nullptr)
      return RCC_INTERNAL_ERROR;  // Cannot reissue certificate because node object is not found
   return initiateCertificateRequest(node->getGuid(), 0);
}

/**
 * Initiate certificate request by agent. This method will return when certificate issuing process is completed.
 */
uint32_t AgentTunnel::initiateCertificateRequest(const uuid& nodeGuid, uint32_t userId)
{
   NXCPMessage request(CMD_BIND_AGENT_TUNNEL, InterlockedIncrement(&m_requestId));
   request.setField(VID_SERVER_ID, g_serverId);
   request.setField(VID_GUID, nodeGuid);
   m_guid = uuid::generate();
   request.setField(VID_TUNNEL_GUID, m_guid);

   TCHAR buffer[256];
   if (GetServerCertificateCountry(buffer, 256))
      request.setField(VID_COUNTRY, buffer);
   if (GetServerCertificateOrganization(buffer, 256))
      request.setField(VID_ORGANIZATION, buffer);

   m_bindRequestId = request.getId();
   m_bindGuid = nodeGuid;
   m_bindUserId = userId;
   sendMessage(request);

   NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, request.getId(), 60000);  // 60 seconds timeout on certificate operations
   if (response == nullptr)
   {
      debugPrintf(4, _T("Certificate cannot be issued: request timeout"));
      m_bindRequestId = 0;
      return RCC_TIMEOUT;
   }

   uint32_t rcc = response->getFieldAsUInt32(VID_RCC);
   delete response;
   if (rcc == ERR_SUCCESS)
   {
      debugPrintf(4, _T("Certificate successfully issued and transferred to agent"));
   }
   else
   {
      debugPrintf(4, _T("Certificate cannot be issued: agent error %u (%s)"), rcc, AgentErrorCodeToText(rcc));
      m_bindRequestId = 0;
   }
   return AgentErrorToRCC(rcc);
}

/**
 * Process certificate request
 */
void AgentTunnel::processCertificateRequest(NXCPMessage *request)
{
   NXCPMessage response(CMD_NEW_CERTIFICATE, request->getId());

   if ((request->getId() == m_bindRequestId) && (m_bindRequestId != 0))
   {
      size_t certRequestLen;
      const BYTE *certRequestData = request->getBinaryFieldPtr(VID_CERTIFICATE, &certRequestLen);
      if (certRequestData != nullptr)
      {
         X509_REQ *certRequest = d2i_X509_REQ(nullptr, &certRequestData, (long)certRequestLen);
         if (certRequest != nullptr)
         {
            char *ou = m_bindGuid.toString().getUTF8String();
            char *cn = m_guid.toString().getUTF8String();
            int32_t days = ConfigReadInt(_T("AgentTunnels.Certificates.ValidityPeriod"), 90);
            X509 *cert = IssueCertificate(certRequest, ou, cn, days);
            MemFree(ou);
            MemFree(cn);
            if (cert != nullptr)
            {
               LogCertificateAction(ISSUE_CERTIFICATE, m_bindUserId, m_nodeId, m_bindGuid, CERT_TYPE_AGENT, cert);

               BYTE *buffer = nullptr;
               int len = i2d_X509(cert, &buffer);
               if (len > 0)
               {
                  response.setField(VID_RCC, ERR_SUCCESS);
                  response.setField(VID_CERTIFICATE, buffer, len);
                  OPENSSL_free(buffer);
                  debugPrintf(4, _T("New certificate issued"));

                  shared_ptr<NetObj> node = FindObjectByGUID(m_bindGuid, OBJECT_NODE);
                  if (node != nullptr)
                  {
                     static_cast<Node&>(*node).setTunnelId(m_guid, GetCertificateSubjectString(cert));
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
            X509_REQ_free(certRequest);
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
   MUTEXATTR_SETTYPE(&a, PTHREAD_MUTEX_ADAPTIVE_NP);
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
 * Incoming connection data
 */
struct ConnectionRequest
{
   SOCKET sock;
   InetAddress addr;
};

/**
 * Convert internal TLS version code to OpenSSL version code
 */
static long DecodeTLSVersion(int version)
{
   long protoVersion = 0;
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   switch(version)
   {
      case 0:
         protoVersion = TLS1_VERSION;
         break;
      case 1:
         protoVersion = TLS1_1_VERSION;
         break;
      case 2:
         protoVersion = TLS1_2_VERSION;
         break;
      case 3:
#if OPENSSL_VERSION_NUMBER >= 0x10101000L
         protoVersion = TLS1_3_VERSION;
#endif
         break;
   }
#else
   switch(version)
   {
      case 3:
#if OPENSSL_VERSION_NUMBER >= 0x10001000L
         protoVersion |= SSL_OP_NO_TLSv1_2;
#endif
         /* no break */
      case 2:
#if OPENSSL_VERSION_NUMBER >= 0x10001000L
         protoVersion |= SSL_OP_NO_TLSv1_1;
#endif
         /* no break */
      case 1:
         protoVersion |= SSL_OP_NO_TLSv1;
         /* no break */
   }
#endif
   return protoVersion;
}

/**
 * Report tunnel error
 */
static inline void ReportError(const TCHAR *debugPrefix, const InetAddress& origin, const TCHAR *format, ...)
{
   TCHAR text[4096];
   va_list args;
   va_start(args, format);
   _vsntprintf(text, 4096, format, args);
   va_end(args);

   nxlog_debug_tag(DEBUG_TAG, 4, _T("SetupTunnel(%s): %s"), debugPrefix, text);
   EventBuilder(EVENT_TUNNEL_SETUP_ERROR, g_dwMgmtNode)
      .param(_T("ipAddress"), origin)
      .param(_T("error"), text)
      .post();
}

/**
 * Setup tunnel
 */
static void SetupTunnel(ConnectionRequest *request)
{
   SSL_CTX *context = nullptr;
   SSL *ssl = nullptr;
   BackgroundSocketPollerHandle *sp = nullptr;
   shared_ptr<AgentTunnel> tunnel;
   int rc;
   uint32_t nodeId = 0;
   int32_t zoneUIN = 0;
   X509 *cert = nullptr;
   time_t certExpTime = 0;
   time_t certIssueTime = 0;
   StringBuffer certSubject, certIssuer;
   int version;

   //Debugging variables
   TCHAR debugPrefix[128];
   request->addr.toString(debugPrefix);

   // Setup secure connection
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   const SSL_METHOD *method = TLS_method();
#else
   const SSL_METHOD *method = SSLv23_method();
#endif
   if (method == nullptr)
   {
      ReportError(debugPrefix, request->addr, _T("Cannot obtain TLS method"));
      goto failure;
   }

   context = SSL_CTX_new((SSL_METHOD *)method);
   if (context == nullptr)
   {
      ReportError(debugPrefix, request->addr, _T("Cannot create TLS context"));
      goto failure;
   }
#ifdef SSL_OP_NO_COMPRESSION
   SSL_CTX_set_options(context, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION);
#else
   SSL_CTX_set_options(context, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
#endif

   version = ConfigReadInt(_T("AgentTunnels.TLS.MinVersion"), 2);
#if OPENSSL_VERSION_NUMBER < 0x10101000L
   if (version >= 3)
   {
      ReportError(debugPrefix, request->addr, _T("Cannot set minimal TLS version to 1.%d (not supported by server)"), version);
      goto failure;
   }
#endif
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   if (!SSL_CTX_set_min_proto_version(context, static_cast<int>(DecodeTLSVersion(version))))
   {
      ReportError(debugPrefix, request->addr, _T("Cannot set minimal TLS version to 1.%d"), version);
      goto failure;
   }
#else
   SSL_CTX_set_options(context, SSL_CTX_get_options(context) | DecodeTLSVersion(version));
#endif
   nxlog_debug_tag(DEBUG_TAG, 4, _T("SetupTunnel(%s): minimal TLS version set to 1.%d"), debugPrefix, version);

   if (!SetupServerTlsContext(context))
   {
      ReportError(debugPrefix, request->addr, _T("Cannot configure TLS context"));
      goto failure;
   }

   ssl = SSL_new(context);
   if (ssl == nullptr)
   {
      ReportError(debugPrefix, request->addr, _T("Cannot configure TLS context"));
      goto failure;
   }

   SSL_set_accept_state(ssl);
   SSL_set_fd(ssl, (int)request->sock);
   SetSocketNonBlocking(request->sock);

retry:
   rc = SSL_do_handshake(ssl);
   if (rc != 1)
   {
      int sslErr = SSL_get_error(ssl, rc);
      if ((sslErr == SSL_ERROR_WANT_READ) || (sslErr == SSL_ERROR_WANT_WRITE))
      {
         SocketPoller poller(sslErr == SSL_ERROR_WANT_WRITE);
         poller.add(request->sock);
         if (poller.poll(REQUEST_TIMEOUT) > 0)
            goto retry;
         ReportError(debugPrefix, request->addr, _T("TLS handshake failed (timeout)"));
      }
      else if (sslErr == SSL_ERROR_SYSCALL)
      {
         TCHAR buffer[256];
         ReportError(debugPrefix, request->addr,  _T("TLS handshake failed (SSL_ERROR_SYSCALL: %s)"),
               GetLastSocketErrorText(buffer, 256));
      }
      else
      {
         char buffer[128];
         ReportError(debugPrefix, request->addr,  _T("TLS handshake failed (%hs)"), ERR_error_string(sslErr, buffer));
      }
      goto failure;
   }

   cert = SSL_get_peer_certificate(ssl);
   if (cert != nullptr)
   {
      bool nodeFound = false;

      String dp = GetCertificateCRLDistributionPoint(cert);
      if (!dp.isEmpty())
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("SetupTunnel(%s): certificate CRL DP: %s"), request->addr.toString().cstr(), dp.cstr());
         char *url = dp.getUTF8String();
         AddRemoteCRL(url, true);
         MemFree(url);
      }

#if (OPENSSL_VERSION_NUMBER >= 0x10100000L) && !defined(LIBRESSL_VERSION_NUMBER)
      STACK_OF(X509) *chain = SSL_get0_verified_chain(ssl);
      if ((chain != nullptr) && (sk_X509_num(chain) > 1))
      {
         X509 *issuer = sk_X509_value(chain, 1);
         if (CheckCertificateRevocation(cert, issuer))
         {
            ReportError(debugPrefix, request->addr,  _T("Certificate is revoked"));
            X509_free(cert);
            goto failure;
         }
      }
#else
      nxlog_debug_tag(DEBUG_TAG, 4, _T("SetupTunnel(%s): CRL check is not implemented for this OpenSSL version"), debugPrefix);
#endif

      certExpTime = GetCertificateExpirationTime(cert);
      certIssueTime = GetCertificateIssueTime(cert);
      certSubject = GetCertificateSubjectString(cert);
      certIssuer = GetCertificateIssuerString(cert);
      TCHAR ou[256], cn[256];
      if (GetCertificateOU(cert, ou, 256) && GetCertificateCN(cert, cn, 256))
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("SetupTunnel(%s): certificate OU=%s CN=%s"), debugPrefix, ou, cn);
         uuid nodeGuid = uuid::parse(ou);
         uuid tunnelGuid = uuid::parse(cn);
         if (!nodeGuid.isNull() && !tunnelGuid.isNull())
         {
            shared_ptr<NetObj> node = FindObjectByGUID(nodeGuid, OBJECT_NODE);
            if (node != nullptr)
            {
               if (tunnelGuid.equals(static_cast<Node&>(*node).getTunnelId()))
               {
                  nxlog_debug_tag(DEBUG_TAG, 4, _T("SetupTunnel(%s): Tunnel attached to node %s [%u]"),
                        debugPrefix, node->getName(), node->getId());
                  if (node->getRuntimeFlags() & NDF_NEW_TUNNEL_BIND)
                  {
                     static_cast<Node&>(*node).clearNewTunnelBindFlag();
                     static_cast<Node&>(*node).setRecheckCapsFlag();
                     static_cast<Node&>(*node).forceConfigurationPoll();
                  }
                  nodeId = node->getId();
                  zoneUIN = node->getZoneUIN();
                  nodeFound = true;
               }
               else
               {
                  ReportError(debugPrefix, request->addr,  _T("Tunnel ID %s is not valid for node %s [%u]"),
                        tunnelGuid.toString().cstr(), node->getName(), node->getId());
               }
            }
            else
            {
               ReportError(debugPrefix, request->addr,  _T("Node with GUID %s not found"),
                     nodeGuid.toString().cstr());
            }
         }
         else
         {
            ReportError(debugPrefix, request->addr,  _T("Certificate OU or CN is not a valid GUID"));
         }
      }
      else
      {
         ReportError(debugPrefix, request->addr,  _T("Cannot get certificate OU and CN"));
         cn[0] = 0;
      }

      // Attempt to lookup externally provisioned certificates
      if (!nodeFound && ((cn[0] != 0) || GetCertificateCN(cert, cn, 256)))
      {
         s_certificateMappingsLock.lock();

         CertificateMappingMethod method = MAP_CERTIFICATE_BY_CN;
         shared_ptr<Node> node = s_certificateMappings.getShared(cn);
         if (node == nullptr)
         {
            method = MAP_CERTIFICATE_BY_SUBJECT;
            node = s_certificateMappings.getShared(certSubject);
         }
         if (node == nullptr)
         {
            method = MAP_CERTIFICATE_BY_TEMPLATE_ID;
            node = s_certificateMappings.getShared(GetCertificateTemplateId(cert));
         }
         if (node == nullptr)
         {
            method = MAP_CERTIFICATE_BY_PUBKEY;
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
            EVP_PKEY *pkey = X509_get0_pubkey(cert);
#else
            EVP_PKEY *pkey = X509_get_pubkey(cert);
#endif
            if (pkey != nullptr)
            {
               int pkeyLen = i2d_PublicKey(pkey, nullptr);
               auto buffer = MemAllocArray<unsigned char>(pkeyLen + 1);
               auto in = buffer;
               i2d_PublicKey(pkey, &in);

               TCHAR *pkeyText = MemAllocString(pkeyLen * 2 + 1);
               BinToStr(buffer, pkeyLen, pkeyText);

               node = s_certificateMappings.getShared(pkeyText);

               MemFree(pkeyText);
               MemFree(buffer);
#if OPENSSL_VERSION_NUMBER < 0x10100000L
               EVP_PKEY_free(pkey);
#endif
            }
         }

         s_certificateMappingsLock.unlock();

         if ((node != nullptr) && (node->getAgentCertificateMappingMethod() == method))
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("SetupTunnel(%s): Tunnel attached to node %s [%u] using externally provisioned certificate"),
                  debugPrefix, node->getName(), node->getId());
            if (node->getRuntimeFlags() & NDF_NEW_TUNNEL_BIND)
            {
               static_cast<Node&>(*node).clearNewTunnelBindFlag();
               static_cast<Node&>(*node).setRecheckCapsFlag();
               static_cast<Node&>(*node).forceConfigurationPoll();
            }
            nodeId = node->getId();
            zoneUIN = node->getZoneUIN();
            static_cast<Node&>(*node).setTunnelId(uuid::NULL_UUID, certSubject);
         }
      }

      X509_free(cert);
   }

   if ((nodeId == 0) && ConfigReadBoolean(L"AgentTunnels.BindByIPAddress", false))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"SetupTunnel(%s): Attempt to find node by IP address", debugPrefix);
      shared_ptr<Node> node = FindNodeByIP(0, request->addr);
      if (node != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, L"SetupTunnel(%s): Tunnel attached to node %s [%u] using IP address",
               debugPrefix, node->getName(), node->getId());
         nodeId = node->getId();
         zoneUIN = node->getZoneUIN();
         static_cast<Node&>(*node).setTunnelId(uuid::NULL_UUID, nullptr);
      }
   }

   if ((nodeId == 0) && (cert == nullptr))
   {
      ReportError(debugPrefix, request->addr, _T("Agent certificate not provided"));
   }

   // Select socket poller
   s_pollerListLock.lock();
   for(int i = 0; i < s_pollers.size(); i++)
   {
      BackgroundSocketPollerHandle *p = s_pollers.get(i);
      if (static_cast<uint32_t>(InterlockedIncrement(&p->usageCount)) < s_maxTunnelsPerPoller)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("SetupTunnel(%s): assigned to poller #%d"), debugPrefix, i);
         sp = p;
         break;
      }
      InterlockedDecrement(&p->usageCount);
   }
   if (sp == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("SetupTunnel(%s): assigned to poller #%d"), debugPrefix, s_pollers.size());
      sp = new BackgroundSocketPollerHandle();
      sp->usageCount = 1;
      s_pollers.add(sp);
   }
   s_pollerListLock.unlock();

   tunnel = AgentTunnel::create(context, ssl, request->sock, request->addr, nodeId, zoneUIN,
         !certSubject.isEmpty() ? certSubject.cstr() : nullptr, !certIssuer.isEmpty() ? certIssuer.cstr() : nullptr,
         certExpTime, certIssueTime, sp);
   RegisterTunnel(tunnel);
   tunnel->start();

   delete request;
   InterlockedDecrement(&s_activeSetupCalls);
   return;

failure:
   if (ssl != nullptr)
      SSL_free(ssl);
   if (context != nullptr)
      SSL_CTX_free(context);
   shutdown(request->sock, SHUT_RDWR);
   closesocket(request->sock);
   delete request;
   InterlockedDecrement(&s_activeSetupCalls);
}

/**
 * Tunnel listener lock
 */
static Mutex s_tunnelListenerLock;

/**
 * Client listener class
 */
class TunnelListener : public StreamSocketListener
{
protected:
   virtual ConnectionProcessingResult processConnection(SOCKET s, const InetAddress& peer);
   virtual bool isStopConditionReached();

public:
   TunnelListener(UINT16 port) : StreamSocketListener(port) { setName(_T("AgentTunnels")); }
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
   if (InterlockedIncrement(&s_activeSetupCalls) > 64)
   {
      InterlockedDecrement(&s_activeSetupCalls);
      nxlog_debug_tag(DEBUG_TAG, 6, _T("Rejecting incoming connection because too many tunnel setup calls already running"));
      return CPR_COMPLETED;
   }

   ConnectionRequest *request = new ConnectionRequest();
   request->sock = s;
   request->addr = peer;
   ThreadPoolExecute(g_agentConnectionThreadPool, SetupTunnel, request);
   return CPR_BACKGROUND;
}

/**
 * Tunnel listener
 */
void TunnelListenerThread()
{
   ThreadSetName("TunnelListener");

   if (!IsServerCertificateLoaded())
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Tunnel listener cannot start because server certificate is not loaded"));
      return;
   }

   s_maxTunnelsPerPoller = ConfigReadULong(_T("AgentTunnels.MaxTunnelsPerPoller"), s_maxTunnelsPerPoller);
   if (s_maxTunnelsPerPoller > SOCKET_POLLER_MAX_SOCKETS - 1)
      s_maxTunnelsPerPoller = SOCKET_POLLER_MAX_SOCKETS - 1;

   s_tunnelListenerLock.lock();
   uint16_t listenPort = static_cast<uint16_t>(ConfigReadULong(_T("AgentTunnels.ListenPort"), 4703));
   TunnelListener listener(listenPort);
   listener.setListenAddress(g_szListenAddress);
   if (!listener.initialize())
   {
      s_tunnelListenerLock.unlock();
      return;
   }

   listener.mainLoop();
   listener.shutdown();

   nxlog_debug_tag(DEBUG_TAG, 1, _T("Tunnel listener thread terminated"));
   s_tunnelListenerLock.unlock();
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
   auto it = s_boundTunnels.begin();
   while(it.hasNext())
   {
      it.next()->shutdown();
   }
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

   s_pollerListLock.lock();
   s_pollers.clear();
   s_pollerListLock.unlock();

   nxlog_debug_tag(DEBUG_TAG, 2, _T("All agent tunnels unregistered"));
}

/**
 * Find matching node for tunnel using agent ID and hardware ID
 */
static bool MatchTunnelToNodeStage1(NetObj *object, AgentTunnel *tunnel)
{
   Node *node = static_cast<Node*>(object);

   if (!node->getTunnelId().isNull())
   {
      // Already have bound tunnel
      if (GetTunnelForNode(node->getId()) != nullptr)
      {
         // Node already have active tunnel, should not match
         return false;
      }
   }

   if (!node->getAgentId().isNull() && node->getAgentId().equals(tunnel->getAgentId()) && node->getHardwareId().equals(tunnel->getHardwareId()))
   {
      TCHAR ipAddrText[64];
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Found matching node %s [%d] for unbound tunnel from %s (%s) using agent ID and hardware ID"),
               node->getName(), node->getId(), tunnel->getDisplayName(), tunnel->getAddress().toString(ipAddrText));
      return true;
   }

   return false;
}

/**
 * Find matching node for tunnel using only hardware ID
 */
static bool MatchTunnelToNodeStage2(NetObj *object, AgentTunnel *tunnel)
{
   Node *node = static_cast<Node*>(object);

   if (!node->getTunnelId().isNull())
   {
      // Already have bound tunnel
      if (GetTunnelForNode(node->getId()) != nullptr)
      {
         // Node already have active tunnel, should not match
         return false;
      }
   }

   if (node->getHardwareId().equals(tunnel->getHardwareId()))
   {
      TCHAR ipAddrText[64];
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Found matching node %s [%d] for unbound tunnel from %s (%s) using hardware ID"),
               node->getName(), node->getId(), tunnel->getDisplayName(), tunnel->getAddress().toString(ipAddrText));
      return true;
   }

   return false;
}

/**
 * Find matching node for tunnel using name or address
 */
static bool MatchTunnelToNodeStage3(NetObj *object, AgentTunnel *tunnel)
{
   Node *node = static_cast<Node*>(object);

   if (!node->getTunnelId().isNull())
      return false;

   if (IsZoningEnabled() && (tunnel->getZoneUIN() != node->getZoneUIN()))
      return false;  // Wrong zone

   if (node->getIpAddress().equals(tunnel->getAddress()) ||
       !_tcsicmp(tunnel->getHostname(), node->getPrimaryHostName()) ||
       !_tcsicmp(tunnel->getHostname(), node->getName()) ||
       !_tcsicmp(tunnel->getSystemName(), node->getPrimaryHostName()) ||
       !_tcsicmp(tunnel->getSystemName(), node->getName()))
   {
      if (node->isNativeAgent())
      {
         // Additional checks if agent already reachable on that node
         shared_ptr<AgentConnectionEx> conn = node->getAgentConnection();
         if (conn != nullptr)
         {
            TCHAR agentVersion[MAX_RESULT_LENGTH] = _T(""), hostName[MAX_RESULT_LENGTH] = _T(""), fqdn[MAX_RESULT_LENGTH] = _T("");
            conn->getParameter(_T("Agent.Version"), agentVersion, MAX_RESULT_LENGTH);
            conn->getParameter(_T("System.Hostname"), hostName, MAX_RESULT_LENGTH);
            conn->getParameter(_T("System.FQDN"), fqdn, MAX_RESULT_LENGTH);

            if (_tcscmp(agentVersion, tunnel->getAgentVersion()))
            {
               nxlog_debug_tag(DEBUG_TAG, 4, _T("Agent version mismatch (%s != %s) for node %s [%d] and unbound tunnel from %s (%s)"),
                        agentVersion, tunnel->getAgentVersion(), node->getName(), node->getId(),
                        tunnel->getDisplayName(), (const TCHAR *)tunnel->getAddress().toString());
               return false;
            }
            if (_tcscmp(hostName, tunnel->getSystemName()))
            {
               nxlog_debug_tag(DEBUG_TAG, 4, _T("System name mismatch (%s != %s) for node %s [%d] and unbound tunnel from %s (%s)"),
                        hostName, tunnel->getSystemName(), node->getName(), node->getId(),
                        tunnel->getDisplayName(), (const TCHAR *)tunnel->getAddress().toString());
               return false;
            }
            if (_tcscmp(fqdn, tunnel->getHostname()))
            {
               nxlog_debug_tag(DEBUG_TAG, 4, _T("Host name mismatch (%s != %s) for node %s [%d] and unbound tunnel from %s (%s)"),
                        fqdn, tunnel->getHostname(), node->getName(), node->getId(),
                        tunnel->getDisplayName(), (const TCHAR *)tunnel->getAddress().toString());
               return false;
            }
         }
      }

      nxlog_debug_tag(DEBUG_TAG, 4, _T("Found matching node %s [%d] for unbound tunnel from %s (%s)"),
               node->getName(), node->getId(), tunnel->getDisplayName(), tunnel->getAddress().toString().cstr());
      return true;   // Match by IP address or name
   }

   return false;
}

/**
 * Find matching node for tunnel
 */
static shared_ptr<Node> FindMatchingNode(AgentTunnel *tunnel)
{
   auto node = static_pointer_cast<Node>(g_idxNodeById.find(MatchTunnelToNodeStage1, tunnel));
   if ((node == nullptr) && !tunnel->getHardwareId().isNull())
      node = static_pointer_cast<Node>(g_idxNodeById.find(MatchTunnelToNodeStage2, tunnel));
   if (node == nullptr)
      node = static_pointer_cast<Node>(g_idxNodeById.find(MatchTunnelToNodeStage3, tunnel));
   return node;
}

/**
 * Finish automatic node creation
 */
static void FinishNodeCreation(const shared_ptr<Node>& node)
{
   int retryCount = 36;
   while(node->getTunnelId().isNull() && (retryCount-- > 0))
      ThreadSleep(5);

   if (!node->getTunnelId().isNull())
   {
      node->setMgmtStatus(true);
      node->forceConfigurationPoll();
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Node creation completed (%s [%d])"), node->getName(), node->getId());
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Tunnel was not re-established after binding for new node %s [%d]"), node->getName(), node->getId());
   }
}

/**
 * Timeout action for unbound tunnels
 */
enum TimeoutAction
{
   RESET = 0,
   GENERATE_EVENT = 1,
   BIND_NODE = 2,
   BIND_OR_CREATE_NODE = 3
};

/**
 * Scheduled task for automatic binding of unbound tunnels
 */
void ProcessUnboundTunnels(const shared_ptr<ScheduledTaskParameters>& parameters)
{
   int timeout = ConfigReadInt(_T("AgentTunnels.UnboundTunnelTimeout"), 3600);
   if (timeout < 0)
      return;  // Auto bind disabled

   SharedObjectArray<AgentTunnel> processingList(16, 16);

   s_tunnelListLock.lock();
   time_t now = time(nullptr);
   for(int i = 0; i < s_unboundTunnels.size(); i++)
   {
      shared_ptr<AgentTunnel> t = s_unboundTunnels.getShared(i);
      nxlog_debug_tag(DEBUG_TAG, 9, _T("Checking tunnel from %s (%s): state=%d, startTime=%ld"),
               t->getDisplayName(), (const TCHAR *)t->getAddress().toString(), t->getState(), (long)t->getStartTime());
      if ((t->getState() == AGENT_TUNNEL_UNBOUND) && !t->isResetPending() && (t->getStartTime() + timeout <= now))
      {
         processingList.add(t);
      }
   }
   s_tunnelListLock.unlock();
   nxlog_debug_tag(DEBUG_TAG, 8, _T("%d unbound tunnels with expired idle timeout"), processingList.size());

   TimeoutAction action = static_cast<TimeoutAction>(ConfigReadInt(_T("AgentTunnels.UnboundTunnelTimeoutAction"), RESET));
   for(int i = 0; i < processingList.size(); i++)
   {
      AgentTunnel *t = processingList.get(i);
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Processing timeout for unbound tunnel from %s (%s) - action=%d"),
               t->getDisplayName(), t->getAddress().toString().cstr(), action);
      switch(action)
      {
         case RESET:
            t->shutdown();
            break;
         case GENERATE_EVENT:
            EventBuilder(EVENT_UNBOUND_TUNNEL, g_dwMgmtNode)
               .param(_T("tunnelId"), t->getId())
               .param(_T("ipAddress"), t->getAddress())
               .param(_T("systemName"), t->getSystemName())
               .param(_T("hostName"), t->getHostname())
               .param(_T("platformName"), t->getPlatformName())
               .param(_T("systemInfo"), t->getSystemInfo())
               .param(_T("agentVersion"), t->getAgentVersion())
               .param(_T("agentId"), t->getAgentId())
               .param(_T("idleTimeout"), timeout)
               .post();

            t->resetStartTime();
            break;
         case BIND_NODE:
         case BIND_OR_CREATE_NODE:
            if (t->isExtProvCertificate())
            {
               t->shutdown();
               break;
            }
            shared_ptr<Node> node = FindMatchingNode(t);
            if (node != nullptr)
            {
               nxlog_debug_tag(DEBUG_TAG, 4, _T("Binding tunnel %u from %s (%s) to existing node %s [%d]"),
                        t->getId(), t->getDisplayName(), (const TCHAR *)t->getAddress().toString(), node->getName(), node->getId());
               uint32_t rcc = BindAgentTunnel(t->getId(), node->getId(), 0);
               nxlog_debug_tag(DEBUG_TAG, 4, _T("Bind tunnel %u to existing node %s [%d]: RCC = %u"),
                        t->getId(), node->getName(), node->getId(), rcc);
               if (rcc != RCC_SUCCESS)
                  t->shutdown();
            }
            else if (action == BIND_OR_CREATE_NODE)
            {
               nxlog_debug_tag(DEBUG_TAG, 4, _T("Creating new node for tunnel %u from %s (%s)"),
                        t->getId(), t->getDisplayName(), (const TCHAR *)t->getAddress().toString());

               NewNodeData nd(InetAddress::NONE);  // use 0.0.0.0 to avoid direct communications by default
               _tcslcpy(nd.name, t->getSystemName(), MAX_OBJECT_NAME);
               nd.zoneUIN = t->getZoneUIN();
               nd.creationFlags = NXC_NCF_CREATE_UNMANAGED;
               nd.origin = NODE_ORIGIN_TUNNEL_AUTOBIND;
               nd.agentId = t->getAgentId();
               node = PollNewNode(&nd);
               if (node != nullptr)
               {
                  TCHAR containerName[MAX_OBJECT_NAME];
                  ConfigReadStr(_T("AgentTunnels.NewNodesContainer"), containerName, MAX_OBJECT_NAME, _T("New Tunnel Nodes"));
                  shared_ptr<NetObj> container = FindObjectByName(containerName, OBJECT_CONTAINER);
                  NetObj::linkObjects((container != nullptr) ? container : g_infrastructureServiceRoot, node);

                  uint32_t rcc = BindAgentTunnel(t->getId(), node->getId(), 0);
                  nxlog_debug_tag(DEBUG_TAG, 4, _T("Bind tunnel %u to new node %s [%d]: RCC = %u"),
                           t->getId(), node->getName(), node->getId(), rcc);
                  if (rcc == RCC_SUCCESS)
                  {
                     ThreadPoolScheduleRelative(g_mainThreadPool, 60000, FinishNodeCreation, node);
                  }
                  else
                  {
                     t->shutdown();
                     node->deleteObject();
                  }
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot create new node for tunnel %u"), t->getId());
                  t->shutdown();
               }
            }
            break;
      }
   }
}

/**
 * Scheduled task for automatic renewal of agent certificates
 */
void RenewAgentCertificates(const shared_ptr<ScheduledTaskParameters>& parameters)
{
   SharedObjectArray<AgentTunnel> processingList(16, 16);

   s_tunnelListLock.lock();
   time_t now = time(nullptr);
   int32_t reissueInterval = ConfigReadInt(_T("AgentTunnels.Certificates.ReissueInterval"), 30) * 86400;
   auto it = s_boundTunnels.begin();
   while(it.hasNext())
   {
      shared_ptr<AgentTunnel> t = it.next();
      if (!t->isExtProvCertificate() && (t->getCertificateExpirationTime() > 0) &&
          ((t->getCertificateExpirationTime() - now <= 2592000) || (now - t->getCertificateIssueTime() >= reissueInterval))) // 30 days
      {
         processingList.add(t);
      }
   }
   s_tunnelListLock.unlock();

   if (processingList.isEmpty())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("No tunnel requires certificate renewal"));
      return;
   }

   nxlog_debug_tag(DEBUG_TAG, 4, _T("%d tunnels selected for certificate renewal"), processingList.size());

   for(int i = 0; i < processingList.size(); i++)
   {
      AgentTunnel *t = processingList.get(i);
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Renewing certificate for tunnel from %s (%s)"), t->getDisplayName(), t->getAddress().toString().cstr());
      uint32_t rcc = t->renewCertificate();
      if (rcc == RCC_SUCCESS)
         nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Agent certificate successfully renewed for %s (%s)"),
                  t->getDisplayName(), t->getAddress().toString().cstr());
      else
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Agent certificate renewal failed for %s (%s) with error %u"),
                  t->getDisplayName(), t->getAddress().toString().cstr(), rcc);
   }
}

/**
 * Get number of bound agent tunnels matching given filter
 */
int GetTunnelCount(TunnelCapabilityFilter filter, bool boundTunnels)
{
   int count = 0;

   if (filter == TunnelCapabilityFilter::ANY)
   {
      s_tunnelListLock.lock();
      count = boundTunnels ? s_boundTunnels.size() : s_unboundTunnels.size();
      s_tunnelListLock.unlock();
   }
   else
   {
      s_tunnelListLock.lock();
      auto it = boundTunnels ? s_boundTunnels.begin() : s_unboundTunnels.begin();
      while (it.hasNext())
      {
         shared_ptr<AgentTunnel> t = it.next();
         if ((filter == TunnelCapabilityFilter::AGENT_PROXY && t->isAgentProxy()) ||
             (filter == TunnelCapabilityFilter::SNMP_PROXY && t->isSnmpProxy()) ||
             (filter == TunnelCapabilityFilter::SNMP_TRAP_PROXY && t->isSnmpTrapProxy()) ||
             (filter == TunnelCapabilityFilter::SYSLOG_PROXY && t->isSyslogProxy()) ||
             (filter == TunnelCapabilityFilter::USER_AGENT && t->isUserAgentInstalled()))
         {
            count++;
         }
      }
      s_tunnelListLock.unlock();
   }

   return count;
}

/**
 * Update agent certificate mapping index for externally provisioned certificates
 */
void UpdateAgentCertificateMappingIndex(const shared_ptr<Node>& node, const TCHAR *oldValue, const TCHAR *newValue)
{
   s_certificateMappingsLock.lock();
   if (oldValue != nullptr)
      s_certificateMappings.remove(oldValue);
   if (newValue != nullptr)
      s_certificateMappings.set(newValue, node);
   s_certificateMappingsLock.unlock();
}
