/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
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

#define REQUEST_TIMEOUT 10000

#define DEBUG_TAG       _T("agent.tunnel")

/**
 * Event parameter names for SYS_UNBOUND_TUNNEL, SYS_TUNNEL_OPEN, and SYS_TUNNEL_CLOSED events
 */
static const TCHAR *s_eventParamNames[] =
   {
      _T("tunnelId"), _T("ipAddress"), _T("systemName"), _T("hostName"),
      _T("platformName"), _T("systemInfo"), _T("agentVersion"),
      _T("agentId"), _T("idleTimeout")
   };

/**
 * Event parameter names for SYS_TUNNEL_AGENT_ID_MISMATCH event
 */
static const TCHAR *s_eventParamNamesAgentIdMismatch[] =
   {
      _T("tunnelId"), _T("ipAddress"), _T("systemName"), _T("hostName"),
      _T("platformName"), _T("systemInfo"), _T("agentVersion"),
      _T("tunnelAgentId"), _T("nodeAgentId")
   };

/**
 * Tunnel registration
 */
static RefCountHashMap<uint32_t, AgentTunnel> s_boundTunnels(Ownership::True);
static ObjectRefArray<AgentTunnel> s_unboundTunnels(16, 16);
static Mutex s_tunnelListLock;

/**
 * Execute tunnel establishing hook script in the separate thread
 */
static void ExecuteScriptInBackground(NXSL_VM *vm)
{
   if (!vm->run())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Tunnel hook script execution error (%s)"), vm->getErrorText());
   }
   delete vm;
}

/**
 * Execute hook script when bound tunnel established
 */
static void ExecuteTunnelHookScript(AgentTunnel *tunnel)
{
   shared_ptr<NetObj> node = tunnel->isBound() ? FindObjectById(tunnel->getNodeId(), OBJECT_NODE) : shared_ptr<NetObj>();
   ScriptVMHandle vm = CreateServerScriptVM(tunnel->isBound() ? _T("Hook::OpenBoundTunnel") : _T("Hook::OpenUnboundTunnel"), node);
   if (!vm.isValid())
   {
      tunnel->debugPrintf(5, _T("Hook script %s"), (vm.failureReason() == ScriptVMFailureReason::SCRIPT_IS_EMPTY) ? _T("is empty") : _T("not found"));
      return;
   }

   vm->setGlobalVariable("$tunnel", vm->createValue(new NXSL_Object(vm, &g_nxslTunnelClass, tunnel)));
   ThreadPoolExecute(g_mainThreadPool, ExecuteScriptInBackground, vm.vm());
}

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
      PostSystemEventWithNames(EVENT_TUNNEL_CLOSED, tunnel->getNodeId(), "dAsssssG", s_eventParamNames,
               tunnel->getId(), &tunnel->getAddress(), tunnel->getSystemName(), tunnel->getHostname(),
               tunnel->getPlatformName(), tunnel->getSystemInfo(), tunnel->getAgentVersion(),
               &tunnel->getAgentId());

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
AgentTunnel *GetTunnelForNode(uint32_t nodeId)
{
   s_tunnelListLock.lock();
   AgentTunnel *t = s_boundTunnels.get(nodeId);
   s_tunnelListLock.unlock();
   return t;
}

/**
 * Bind agent tunnel
 */
uint32_t BindAgentTunnel(uint32_t tunnelId, uint32_t nodeId, uint32_t userId)
{
   AgentTunnel *tunnel = nullptr;
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

   if (tunnel == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("BindAgentTunnel: unbound tunnel with ID %u not found"), tunnelId);
      return RCC_INVALID_TUNNEL_ID;
   }

   TCHAR userName[MAX_USER_NAME];
   nxlog_debug_tag(DEBUG_TAG, 4, _T("BindAgentTunnel: processing bind request %u -> %u by user %s"),
            tunnelId, nodeId, ResolveUserId(userId, userName, true));
   uint32_t rcc = tunnel->bind(nodeId, userId);
   tunnel->decRefCount();
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

   if (static_cast<Node&>(*node).getTunnelId().isNull())
      return RCC_SUCCESS;  // tunnel is not set

   TCHAR userName[MAX_USER_NAME];
   nxlog_debug_tag(DEBUG_TAG, 4, _T("UnbindAgentTunnel: processing unbind request for node %u by user %s"),
            nodeId, ResolveUserId(userId, userName, true));

   TCHAR subject[256];
   _sntprintf(subject, 256, _T("OU=%s,CN=%s"), node->getGuid().toString().cstr(), static_cast<Node&>(*node).getTunnelId().toString().cstr());
   LogCertificateAction(REVOKE_CERTIFICATE, userId, nodeId, node->getGuid(), CERT_TYPE_AGENT,
            (static_cast<Node&>(*node).getAgentCertificateSubject() != nullptr) ? static_cast<Node&>(*node).getAgentCertificateSubject() : subject, 0);

   static_cast<Node&>(*node).setTunnelId(uuid::NULL_UUID, nullptr);

   AgentTunnel *tunnel = GetTunnelForNode(nodeId);
   if (tunnel != nullptr)
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
            _T("ID   | Node ID | Peer IP Address          | System Name              | Hostname                 | Platform Name    | Agent Version | Agent Build Tag\n")
            _T("-----+---------+--------------------------+--------------------------+--------------------------+------------------+---------------+--------------------------\n"));
   Iterator<AgentTunnel> *it = s_boundTunnels.iterator();
   while(it->hasNext())
   {
      AgentTunnel *t = it->next();
      TCHAR ipAddrBuffer[64];
      ConsolePrintf(console, _T("%4d | %7u | %-24s | %-24s | %-24s | %-16s | %-13s | %s\n"), t->getId(), t->getNodeId(),
               t->getAddress().toString(ipAddrBuffer), t->getSystemName(), t->getHostname(), t->getPlatformName(), t->getAgentVersion(), t->getAgentBuildTag());
   }
   delete it;

   ConsolePrintf(console,
            _T("\n\x1b[1mUNBOUND TUNNELS\x1b[0m\n")
            _T("ID   | Peer IP Address          | System Name              | Hostname                 | Platform Name    | Agent Version | Agent Build Tag\n")
            _T("-----+--------------------------+--------------------------+--------------------------+------------------+---------------+------------------------------------\n"));
   for(int i = 0; i < s_unboundTunnels.size(); i++)
   {
      const AgentTunnel *t = s_unboundTunnels.get(i);
      TCHAR ipAddrBuffer[64];
      ConsolePrintf(console, _T("%4d | %-24s | %-24s | %-24s | %-16s | %-13s | %s\n"), t->getId(), t->getAddress().toString(ipAddrBuffer),
               t->getSystemName(), t->getHostname(), t->getPlatformName(), t->getAgentVersion(), t->getAgentBuildTag());
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
AgentTunnel::AgentTunnel(SSL_CTX *context, SSL *ssl, SOCKET sock, const InetAddress& addr,
         uint32_t nodeId, int32_t zoneUIN, const TCHAR *certificateSubject, const TCHAR *certificateIssuer,
         time_t certificateExpirationTime) : RefCountObject(), m_channels(Ownership::True)
{
   m_id = InterlockedIncrement(&s_nextTunnelId);
   m_address = addr;
   m_socket = sock;
   m_context = context;
   m_ssl = ssl;
   m_sslLock = MutexCreate();
   m_writeLock = MutexCreate();
   m_requestId = 0;
   m_nodeId = nodeId;
   m_zoneUIN = zoneUIN;
   m_certificateSubject = MemCopyString(certificateSubject);
   m_certificateIssuer = MemCopyString(certificateIssuer);
   m_certificateExpirationTime = certificateExpirationTime;
   m_state = AGENT_TUNNEL_INIT;
   m_systemName = nullptr;
   m_platformName = nullptr;
   m_systemInfo = nullptr;
   m_agentVersion = nullptr;
   m_agentBuildTag = nullptr;
   m_bindRequestId = 0;
   m_bindUserId = 0;
   m_channelLock = MutexCreate();
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
   m_channels.clear();
   shutdown();
   SSL_CTX_free(m_context);
   SSL_free(m_ssl);
   MutexDestroy(m_sslLock);
   MutexDestroy(m_writeLock);
   closesocket(m_socket);
   MemFree(m_systemName);
   MemFree(m_platformName);
   MemFree(m_systemInfo);
   MemFree(m_agentVersion);
   MemFree(m_agentBuildTag);
   MemFree(m_certificateIssuer);
   MemFree(m_certificateSubject);
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

      if (nxlog_get_debug_level_tag(DEBUG_TAG) >= 6)
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
               if (channel != nullptr)
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
            msg = nullptr; // prevent message deletion
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
void AgentTunnel::recvThreadStarter(AgentTunnel *tunnel)
{
   ThreadSetName("TunnelReceiver");
   tunnel->recvThread();
   tunnel->decRefCount();
}

/**
 * Write to SSL
 */
int AgentTunnel::sslWrite(const void *data, size_t size)
{
   bool canRetry;
   int bytes;
   MutexLock(m_writeLock);
   do
   {
      canRetry = false;
      MutexLock(m_sslLock);
      bytes = SSL_write(m_ssl, data, (int)size);
      if (bytes <= 0)
      {
         int err = SSL_get_error(m_ssl, bytes);
         if ((err == SSL_ERROR_WANT_READ) || (err == SSL_ERROR_WANT_WRITE))
         {
            MutexUnlock(m_sslLock);
            SocketPoller sp(err == SSL_ERROR_WANT_WRITE);
            sp.add(m_socket);
            if (sp.poll(REQUEST_TIMEOUT) > 0)
               canRetry = true;
            MutexLock(m_sslLock);
         }
         else
         {
            debugPrintf(7, _T("SSL_write error (bytes=%d ssl_err=%d socket_err=%d)"), bytes, err, WSAGetLastError());
            if (err == SSL_ERROR_SSL)
               LogOpenSSLErrorStack(7);
         }
      }
      MutexUnlock(m_sslLock);
   }
   while(canRetry);
   MutexUnlock(m_writeLock);
   return bytes;
}

/**
 * Send message on tunnel
 */
bool AgentTunnel::sendMessage(NXCPMessage *msg)
{
   if (m_state == AGENT_TUNNEL_SHUTDOWN)
      return false;

   if (nxlog_get_debug_level_tag(DEBUG_TAG) >= 6)
   {
      TCHAR buffer[64];
      debugPrintf(6, _T("Sending message %s"), NXCPMessageCodeName(msg->getCode(), buffer));
   }
   NXCP_MESSAGE *data = msg->serialize(true);
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
   incRefCount();
   ThreadCreate(AgentTunnel::recvThreadStarter, this);
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
static void BackgroundRenewCertificate(AgentTunnel *tunnel)
{
   uint32_t rcc = tunnel->renewCertificate();
   if (rcc == RCC_SUCCESS)
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Agent certificate successfully renewed for %s (%s)"),
               tunnel->getDisplayName(), tunnel->getAddress().toString().cstr());
   else
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Agent certificate renewal failed for %s (%s) with error %u"),
               tunnel->getDisplayName(), tunnel->getAddress().toString().cstr(), rcc);
   tunnel->decRefCount();
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
         debugPrintf(4, _T("   Externally provisioned...: %s"), m_extProvCertificate ? _T("YES") : _T("NO"));
         debugPrintf(4, _T("   Certificate subject......: %s"), CHECK_NULL(m_certificateSubject));
         debugPrintf(4, _T("   Certificate issuer.......: %s"), CHECK_NULL(m_certificateIssuer));
      }

      ExecuteTunnelHookScript(this);
      if (m_state == AGENT_TUNNEL_BOUND)
      {
         PostSystemEventWithNames(EVENT_TUNNEL_OPEN, m_nodeId, "dAsssssG", s_eventParamNames,
                  m_id, &m_address, m_systemName, m_hostname, m_platformName, m_systemInfo,
                  m_agentVersion, &m_agentId);
         if (!m_extProvCertificate && (m_certificateExpirationTime - time(nullptr) <= 2592000)) // 30 days
         {
            debugPrintf(4, _T("Certificate will expire soon, requesting renewal"));
            incRefCount();
            ThreadPoolExecute(g_mainThreadPool, BackgroundRenewCertificate, this);
         }
      }
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
uint32_t AgentTunnel::bind(uint32_t nodeId, uint32_t userId)
{
   if ((m_state != AGENT_TUNNEL_UNBOUND) || (m_bindRequestId != 0))
      return RCC_OUT_OF_STATE_REQUEST;

   shared_ptr<NetObj> node = FindObjectById(nodeId, OBJECT_NODE);
   if (node == nullptr)
      return RCC_INVALID_OBJECT_ID;

   if (!static_cast<Node&>(*node).getAgentId().equals(m_agentId))
   {
      debugPrintf(3, _T("Node agent ID (%s) do not match tunnel agent ID (%s) on bind"),
               static_cast<Node&>(*node).getAgentId().toString().cstr(), m_agentId.toString().cstr());
      PostSystemEventWithNames(EVENT_TUNNEL_AGENT_ID_MISMATCH, nodeId, "dAsssssGG", s_eventParamNamesAgentIdMismatch,
               m_id, &m_address, m_systemName, m_hostname, m_platformName, m_systemInfo,
               m_agentVersion, &static_cast<Node&>(*node).getAgentId(), &m_agentId);
   }

   uint32_t rcc = initiateCertificateRequest(node->getGuid(), userId);
   if (rcc == RCC_SUCCESS)
   {
      debugPrintf(4, _T("Bind successful, resetting tunnel"));
      static_cast<Node&>(*node).setNewTunnelBindFlag();
      NXCPMessage msg(CMD_RESET_TUNNEL, InterlockedIncrement(&m_requestId));
      sendMessage(&msg);
   }
   return AgentErrorToRCC(rcc);
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
   NXCPMessage msg;
   msg.setCode(CMD_BIND_AGENT_TUNNEL);
   msg.setId(InterlockedIncrement(&m_requestId));
   msg.setField(VID_SERVER_ID, g_serverId);
   msg.setField(VID_GUID, nodeGuid);
   m_guid = uuid::generate();
   msg.setField(VID_TUNNEL_GUID, m_guid);

   TCHAR buffer[256];
   if (GetServerCertificateCountry(buffer, 256))
      msg.setField(VID_COUNTRY, buffer);
   if (GetServerCertificateOrganization(buffer, 256))
      msg.setField(VID_ORGANIZATION, buffer);

   m_bindRequestId = msg.getId();
   m_bindGuid = nodeGuid;
   m_bindUserId = userId;
   sendMessage(&msg);

   NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, msg.getId());
   if (response == nullptr)
      return RCC_TIMEOUT;

   uint32_t rcc = response->getFieldAsUInt32(VID_RCC);
   delete response;
   if (rcc == ERR_SUCCESS)
   {
      debugPrintf(4, _T("Certificate successfully issued and transferred to agent"));
   }
   else
   {
      debugPrintf(4, _T("Certificate cannot be issued: agent error %d (%s)"), rcc, AgentErrorCodeToText(rcc));
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
            X509 *cert = IssueCertificate(certRequest, ou, cn, 365);
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
   if (response == nullptr)
   {
      debugPrintf(4, _T("createChannel: request timeout"));
      return nullptr;
   }

   UINT32 rcc = response->getFieldAsUInt32(VID_RCC);
   if (rcc != ERR_SUCCESS)
   {
      delete response;
      debugPrintf(4, _T("createChannel: agent error %d (%s)"), rcc, AgentErrorCodeToText(rcc));
      return nullptr;
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
   if (ch != nullptr)
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
   InitializeConditionVariable(&m_dataCondition);
#else
   pthread_mutex_init(&m_bufferLock, nullptr);
   pthread_cond_init(&m_dataCondition, nullptr);
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
#else
   pthread_mutex_destroy(&m_bufferLock);
   pthread_cond_destroy(&m_dataCondition);
#endif
}

/**
 * Send data
 */
ssize_t AgentTunnelCommChannel::send(const void *data, size_t size, MUTEX mutex)
{
   return m_active ? m_tunnel->sendChannelData(m_id, data, size) : -1;
}

/**
 * Receive data
 */
ssize_t AgentTunnelCommChannel::recv(void *buffer, size_t size, UINT32 timeout)
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
int AgentTunnelCommChannel::poll(UINT32 timeout, bool write)
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
 * Shutdown channel
 */
int AgentTunnelCommChannel::shutdown()
{
   m_active = false;
#ifdef _WIN32
   WakeAllConditionVariable(&m_dataCondition);
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
   WakeAllConditionVariable(&m_dataCondition);
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

static int DecodeTLSVersion(int version)
{
   int protoVersion = 0;
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
         protoVersion = TLS1_3_VERSION;
         break;
   }
   return protoVersion;
}

/**
 * Setup tunnel
 */
static void SetupTunnel(ConnectionRequest *request)
{
   SSL_CTX *context = nullptr;
   SSL *ssl = nullptr;
   AgentTunnel *tunnel = nullptr;
   int rc;
   uint32_t nodeId = 0;
   int32_t zoneUIN = 0;
   X509 *cert = nullptr;
   time_t certExpTime = 0;
   StringBuffer certSubject, certIssuer;
   int version;

   // Setup secure connection
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   const SSL_METHOD *method = TLS_method();
#else
   const SSL_METHOD *method = SSLv23_method();
#endif
   if (method == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("SetupTunnel(%s): cannot obtain TLS method"), (const TCHAR *)request->addr.toString());
      goto failure;
   }

   context = SSL_CTX_new((SSL_METHOD *)method);
   if (context == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("SetupTunnel(%s): cannot create TLS context"), (const TCHAR *)request->addr.toString());
      goto failure;
   }
#ifdef SSL_OP_NO_COMPRESSION
   SSL_CTX_set_options(context, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION);
#else
   SSL_CTX_set_options(context, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
#endif
   version = ConfigReadInt(_T("AgentTunnels.TLS.MinVersion"), 2);
   if (!SSL_CTX_set_min_proto_version(context, DecodeTLSVersion(version)))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("SetupTunnel(%s): cannot set minimal protocol version"), (const TCHAR *)request->addr.toString());
      goto failure;
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("SetupTunnel(%s): Tunnel TLS protocol version set to 1.%d"), (const TCHAR *)request->addr.toString(), version);
   }
   if (!SetupServerTlsContext(context))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("SetupTunnel(%s): cannot configure TLS context"), (const TCHAR *)request->addr.toString());
      goto failure;
   }

   ssl = SSL_new(context);
   if (ssl == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("SetupTunnel(%s): cannot create SSL object"), (const TCHAR *)request->addr.toString());
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
   if (cert != nullptr)
   {
      certExpTime = GetCertificateExpirationTime(cert);
      certSubject = GetCertificateSubjectString(cert);
      certIssuer = GetCertificateIssuerString(cert);
      if (ValidateAgentCertificate(cert))
      {
         TCHAR ou[256], cn[256];
         if (GetCertificateOU(cert, ou, 256) && GetCertificateCN(cert, cn, 256))
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("SetupTunnel(%s): certificate OU=%s CN=%s"), request->addr.toString().cstr(), ou, cn);
            uuid nodeGuid = uuid::parse(ou);
            uuid tunnelGuid = uuid::parse(cn);
            if (!nodeGuid.isNull() && !tunnelGuid.isNull())
            {
               shared_ptr<NetObj> node = FindObjectByGUID(nodeGuid, OBJECT_NODE);
               if (node != nullptr)
               {
                  if (tunnelGuid.equals(static_cast<Node&>(*node).getTunnelId()))
                  {
                     nxlog_debug_tag(DEBUG_TAG, 4, _T("SetupTunnel(%s): Tunnel attached to node %s [%d]"),
                              request->addr.toString().cstr(), node->getName(), node->getId());
                     if (node->getRuntimeFlags() & NDF_NEW_TUNNEL_BIND)
                     {
                        static_cast<Node&>(*node).clearNewTunnelBindFlag();
                        static_cast<Node&>(*node).setRecheckCapsFlag();
                        static_cast<Node&>(*node).forceConfigurationPoll();
                     }
                     nodeId = node->getId();
                     zoneUIN = node->getZoneUIN();
                  }
                  else
                  {
                     nxlog_debug_tag(DEBUG_TAG, 4, _T("SetupTunnel(%s): Tunnel ID %s is not valid for node %s [%d]"),
                              request->addr.toString().cstr(), tunnelGuid.toString().cstr(), node->getName(), node->getId());
                  }
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG, 4, _T("SetupTunnel(%s): Node with GUID %s not found"),
                           request->addr.toString().cstr(), nodeGuid.toString().cstr());
               }
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG, 4, _T("SetupTunnel(%s): Certificate OU or CN is not a valid GUID"), request->addr.toString().cstr());
            }
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("SetupTunnel(%s): Cannot get certificate OU and CN"), request->addr.toString().cstr());
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("SetupTunnel(%s): Agent certificate validation failed"), request->addr.toString().cstr());
      }
      X509_free(cert);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("SetupTunnel(%s): Agent certificate not provided"), request->addr.toString().cstr());
   }

   tunnel = new AgentTunnel(context, ssl, request->sock, request->addr, nodeId, zoneUIN,
         !certSubject.isEmpty() ? certSubject.cstr() : nullptr, !certIssuer.isEmpty() ? certIssuer.cstr() : nullptr, certExpTime);
   RegisterTunnel(tunnel);
   tunnel->start();
   tunnel->decRefCount();

   delete request;
   return;

failure:
   if (ssl != nullptr)
      SSL_free(ssl);
   if (context != nullptr)
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
   ConnectionRequest *request = new ConnectionRequest();
   request->sock = s;
   request->addr = peer;
   ThreadPoolExecute(g_mainThreadPool, SetupTunnel, request);
   return CPR_BACKGROUND;
}

/**
 * Tunnel listener
 */
void TunnelListenerThread()
{
   ThreadSetName("TunnelListener");
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

/**
 * Find matching node for tunnel using agent ID and hardware ID
 */
static bool MatchTunnelToNodeStage1(NetObj *object, AgentTunnel *tunnel)
{
   Node *node = static_cast<Node*>(object);

   if (!node->getTunnelId().isNull())
   {
      // Already have bound tunnel
      AgentTunnel *activeTunnel = GetTunnelForNode(node->getId());
      if (activeTunnel != nullptr)
      {
         // Node already have active tunnel, should not match
         activeTunnel->decRefCount();
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
      AgentTunnel *activeTunnel = GetTunnelForNode(node->getId());
      if (activeTunnel != nullptr)
      {
         // Node already have active tunnel, should not match
         activeTunnel->decRefCount();
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
      node->setMgmtStatus(TRUE);
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

   ObjectRefArray<AgentTunnel> processingList(16, 16);

   s_tunnelListLock.lock();
   time_t now = time(nullptr);
   for(int i = 0; i < s_unboundTunnels.size(); i++)
   {
      AgentTunnel *t = s_unboundTunnels.get(i);
      nxlog_debug_tag(DEBUG_TAG, 9, _T("Checking tunnel from %s (%s): state=%d, startTime=%ld"),
               t->getDisplayName(), (const TCHAR *)t->getAddress().toString(), t->getState(), (long)t->getStartTime());
      if ((t->getState() == AGENT_TUNNEL_UNBOUND) && (t->getStartTime() + timeout <= now))
      {
         t->incRefCount();
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
            PostSystemEventWithNames(EVENT_UNBOUND_TUNNEL, g_dwMgmtNode, "dAsssssGd", s_eventParamNames,
                     t->getId(), &t->getAddress(), t->getSystemName(), t->getHostname(), t->getPlatformName(),
                     t->getSystemInfo(), t->getAgentVersion(), &t->getAgentId(), timeout);
            t->resetStartTime();
            break;
         case BIND_NODE:
         case BIND_OR_CREATE_NODE:
            shared_ptr<Node> node = FindMatchingNode(t);
            if (node != nullptr)
            {
               nxlog_debug_tag(DEBUG_TAG, 4, _T("Binding tunnel from %s (%s) to existing node %s [%d]"),
                        t->getDisplayName(), (const TCHAR *)t->getAddress().toString(), node->getName(), node->getId());
               BindAgentTunnel(t->getId(), node->getId(), 0);
            }
            else if (action == BIND_OR_CREATE_NODE)
            {
               nxlog_debug_tag(DEBUG_TAG, 4, _T("Creating new node for tunnel from %s (%s)"),
                        t->getDisplayName(), (const TCHAR *)t->getAddress().toString());

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
                  if (container != nullptr)
                  {
                     container->addChild(node);
                     node->addParent(container);
                  }
                  else
                  {
                     g_infrastructureServiceRoot->addChild(node);
                     node->addParent(g_infrastructureServiceRoot);
                  }

                  if (BindAgentTunnel(t->getId(), node->getId(), 0) == RCC_SUCCESS)
                  {
                     ThreadPoolScheduleRelative(g_mainThreadPool, 60000, FinishNodeCreation, node);
                  }
               }
            }
            break;
      }
      t->decRefCount();
   }
}

/**
 * Scheduled task for automatic renewal of agent certificates
 */
void RenewAgentCertificates(const shared_ptr<ScheduledTaskParameters>& parameters)
{
   ObjectRefArray<AgentTunnel> processingList(16, 16);

   s_tunnelListLock.lock();
   time_t now = time(nullptr);
   auto it = s_boundTunnels.iterator();
   while(it->hasNext())
   {
      AgentTunnel *t = it->next();
      if (t->getCertificateExpirationTime() - now <= 2592000)  // 30 days
      {
         t->incRefCount();
         processingList.add(t);
      }
   }
   delete it;
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
      t->decRefCount();
   }
}

/**
 * Get number of bound agent tunnels matching given filter
 */
int GetTunnelCount(TunnelCapabilityFilter filter)
{
   int count = 0;

   if (filter == TunnelCapabilityFilter::ANY)
   {
      s_tunnelListLock.lock();
      count = s_boundTunnels.size();
      s_tunnelListLock.unlock();
   }
   else
   {
      s_tunnelListLock.lock();
      Iterator<AgentTunnel> *it = s_boundTunnels.iterator();
      while (it->hasNext())
      {
         AgentTunnel *t = it->next();
         if ((filter == TunnelCapabilityFilter::AGENT_PROXY && t->isAgentProxy()) ||
               (filter == TunnelCapabilityFilter::SNMP_PROXY && t->isSnmpProxy()) ||
               (filter == TunnelCapabilityFilter::SNMP_TRAP_PROXY && t->isSnmpTrapProxy()) ||
               (filter == TunnelCapabilityFilter::SYSLOG_PROXY && t->isSyslogProxy()) ||
               (filter == TunnelCapabilityFilter::USER_AGENT && t->isUserAgentInstalled()))
         {
            count++;
         }
      }
      delete it;
      s_tunnelListLock.unlock();
   }

   return count;
}
