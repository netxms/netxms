/*
** NetXMS - Network Management System
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
** File: tunnel.cpp
**/

#include "nxcore.h"
#include <socket_listener.h>
#include <nxcore_agent_tunnel.h>
#include <nms_users.h>

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
static SharedHashMap<uint32_t, AgentTunnel> s_boundTunnels;   // Bound tunnels (inbound and outbound) indexed by node ID
static SharedObjectArray<InboundAgentTunnel> s_unboundTunnels(16, 16);
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
static void RegisterTunnel(const shared_ptr<InboundAgentTunnel>& tunnel)
{
   shared_ptr<AgentTunnel> replacedTunnel;
   s_tunnelListLock.lock();
   s_tunnels.set(tunnel->getId(), tunnel);
   if (tunnel->isBound())
   {
      replacedTunnel = s_boundTunnels.getShared(tunnel->getNodeId());
      tunnel->setReplacedTunnel(replacedTunnel);
      s_boundTunnels.set(tunnel->getNodeId(), tunnel);
   }
   else
   {
      s_unboundTunnels.add(tunnel);
   }
   s_tunnelListLock.unlock();

   // Agent-initiated tunnel takes precedence over server-initiated one
   if ((replacedTunnel != nullptr) && !replacedTunnel->isInbound())
      replacedTunnel->shutdown();
}

/**
 * Unregister tunnel
 */
static void UnregisterTunnel(AgentTunnel *tunnel)
{
   s_tunnelListLock.lock();
   if (s_tunnels.get(tunnel->getId()) == nullptr)
   {
      s_tunnelListLock.unlock();
      return;  // already unregistered
   }
   tunnel->debugPrintf(4, _T("Tunnel unregistered"));
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
 * Unregister outbound tunnel (used as tunnel close callback)
 */
void UnregisterOutboundTunnel(OutboundAgentTunnel *tunnel)
{
   UnregisterTunnel(tunnel);
}

/**
 * Register outbound tunnel established by node. Returns effective tunnel for the node - either
 * given tunnel or already registered one (in that case given tunnel is shut down).
 */
shared_ptr<AgentTunnel> RegisterOutboundTunnel(const shared_ptr<OutboundAgentTunnel>& tunnel)
{
   s_tunnelListLock.lock();
   shared_ptr<AgentTunnel> existing = s_boundTunnels.getShared(tunnel->getNodeId());
   if (existing == nullptr)
   {
      s_tunnels.set(tunnel->getId(), tunnel);
      s_boundTunnels.set(tunnel->getNodeId(), tunnel);
   }
   s_tunnelListLock.unlock();

   if (existing != nullptr)
   {
      // Another tunnel (likely agent-initiated) was registered for this node while establishing - keep it
      tunnel->debugPrintf(4, _T("Tunnel for node %u already registered, dropping new outbound tunnel"), tunnel->getNodeId());
      tunnel->shutdown();
      return existing;
   }

   if (tunnel->getState() == AGENT_TUNNEL_SHUTDOWN)
   {
      // Tunnel was closed between establishment and registration
      UnregisterTunnel(tunnel.get());
      return shared_ptr<AgentTunnel>();
   }

   tunnel->debugPrintf(3, _T("Outbound tunnel registered for node %u"), tunnel->getNodeId());

   EventBuilder(EVENT_TUNNEL_OPEN, tunnel->getNodeId())
      .param(_T("tunnelId"), tunnel->getId())
      .param(_T("ipAddress"), tunnel->getAddress())
      .param(_T("systemName"), tunnel->getSystemName())
      .param(_T("hostName"), tunnel->getHostname())
      .param(_T("platformName"), tunnel->getPlatformName())
      .param(_T("systemInfo"), tunnel->getSystemInfo())
      .param(_T("agentVersion"), tunnel->getAgentVersion())
      .param(_T("agentId"), tunnel->getAgentId())
      .post();

   auto msg = new NXCPMessage(CMD_AGENT_TUNNEL_UPDATE, 0);
   tunnel->fillMessage(msg, VID_ELEMENT_LIST_BASE);
   msg->setField(VID_NOTIFICATION_CODE, NX_NOTIFY_AGENT_TUNNEL_OPEN);
   ThreadPoolExecute(g_clientThreadPool,
      [msg] () -> void
      {
         NotifyClientSessions(*msg, NXC_CHANNEL_AGENT_TUNNELS);
         delete msg;
      });

   return tunnel;
}

/**
 * Bind agent tunnel
 */
uint32_t BindAgentTunnel(uint32_t tunnelId, uint32_t nodeId, uint32_t userId)
{
   shared_ptr<InboundAgentTunnel> tunnel;
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
            _T(" ID  | Node ID | Dir | EP  | Chan. | Peer IP Address          | System Name              | Hostname                 | Platform Name    | Agent Version | Agent Build Tag\n")
            _T("-----+---------+-----+-----+-------+--------------------------+--------------------------+--------------------------+------------------+---------------+--------------------------\n"));
   for(const shared_ptr<AgentTunnel>& t : s_boundTunnels)
   {
      TCHAR ipAddrBuffer[64];
      ConsolePrintf(console, _T("%4d | %7u | %-3s | %-3s | %5d | %-24s | %-24s | %-24s | %-16s | %-13s | %s\n"), t->getId(), t->getNodeId(),
               t->isInbound() ? _T("IN") : _T("OUT"), t->isExtProvCertificate() ? _T("YES") : _T("NO"), t->getChannelCount(),
               t->getAddress().toString(ipAddrBuffer), t->getSystemName(), t->getHostname(), t->getPlatformName(), t->getAgentVersion(), t->getAgentBuildTag());
   }

   ConsolePrintf(console,
            _T("\n\x1b[1mUNBOUND TUNNELS\x1b[0m\n")
            _T(" ID  | EP  | Peer IP Address          | System Name              | Hostname                 | Platform Name    | Agent Version | Agent Build Tag\n")
            _T("-----+-----+--------------------------+--------------------------+--------------------------+------------------+---------------+------------------------------------\n"));
   for(const shared_ptr<InboundAgentTunnel>& t : s_unboundTunnels)
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
shared_ptr<InboundAgentTunnel> InboundAgentTunnel::create(SSL_CTX *context, SSL *ssl, SOCKET sock, const InetAddress& addr, uint32_t nodeId,
         int32_t zoneUIN, const TCHAR *certificateSubject, const TCHAR *certificateIssuer, time_t certificateExpirationTime,
         time_t certificateIssueTime, BackgroundSocketPollerHandle *socketPoller)
{
   shared_ptr<InboundAgentTunnel> tunnel = make_shared<InboundAgentTunnel>(context, ssl, sock, addr, nodeId, zoneUIN, certificateSubject,
            certificateIssuer, certificateExpirationTime, certificateIssueTime, socketPoller);
   tunnel->m_self = tunnel;
   return tunnel;
}

/**
 * Inbound agent tunnel constructor
 */
InboundAgentTunnel::InboundAgentTunnel(SSL_CTX *context, SSL *ssl, SOCKET sock, const InetAddress& addr,
         uint32_t nodeId, int32_t zoneUIN, const TCHAR *certificateSubject, const TCHAR *certificateIssuer,
         time_t certificateExpirationTime, time_t certificateIssueTime, BackgroundSocketPollerHandle *socketPoller)
         : AgentTunnel(context, ssl, sock, addr, nodeId, zoneUIN, socketPoller)
{
   m_certificateSubject = MemCopyString(certificateSubject);
   m_certificateIssuer = MemCopyString(certificateIssuer);
   m_certificateExpirationTime = certificateExpirationTime;
   m_certificateIssueTime = certificateIssueTime;
   m_bindRequestId = 0;
   m_bindUserId = 0;
   m_resetPending = false;
   setCommandTimeout(g_agentCommandTimeout);
}

/**
 * Process tunnel messages specific to inbound tunnels
 */
bool InboundAgentTunnel::processCustomMessage(NXCPMessage *msg)
{
   switch(msg->getCode())
   {
      case CMD_KEEPALIVE:
         {
            NXCPMessage response(CMD_KEEPALIVE, msg->getId());
            sendMessage(response);
         }
         delete msg;
         return true;
      case CMD_REQUEST_CERTIFICATE:
         processCertificateRequest(msg);
         delete msg;
         return true;
   }
   return false;
}

/**
 * Close handler for inbound tunnels
 */
void InboundAgentTunnel::onTunnelClose()
{
   UnregisterTunnel(this);
}

/**
 * Background certificate renewal
 */
static void BackgroundRenewCertificate(const shared_ptr<InboundAgentTunnel>& tunnel)
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
 * Setup completion handler for inbound tunnels
 */
void InboundAgentTunnel::onSetupComplete()
{
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
               .param(_T("oldHardwareId"), NodeHardwareId(m_replacedTunnel->getHardwareId().value()).toString())
               .param(_T("newHardwareId"), NodeHardwareId(m_hardwareId.value()).toString())
               .post();
         }
         m_replacedTunnel.reset();
      }
   }
}

/**
 * Bind tunnel to node
 */
uint32_t InboundAgentTunnel::bind(uint32_t nodeId, uint32_t userId)
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
uint32_t InboundAgentTunnel::renewCertificate()
{
   shared_ptr<NetObj> node = FindObjectById(m_nodeId, OBJECT_NODE);
   if (node == nullptr)
      return RCC_INTERNAL_ERROR;  // Cannot reissue certificate because node object is not found
   return initiateCertificateRequest(node->getGuid(), 0);
}

/**
 * Initiate certificate request by agent. This method will return when certificate issuing process is completed.
 */
uint32_t InboundAgentTunnel::initiateCertificateRequest(const uuid& nodeGuid, uint32_t userId)
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
void InboundAgentTunnel::processCertificateRequest(NXCPMessage *request)
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
            std::string ou = m_bindGuid.toString().getUTF8StdString();
            std::string cn = m_guid.toString().getUTF8StdString();
            int32_t days = ConfigReadInt(_T("AgentTunnels.Certificates.ValidityPeriod"), 90);

            // Agents up to and including 6.1.2 set PKCS#10 version field to 1 instead of the
            // RFC 2986 mandated 0. OpenSSL 3.5+ enforces the spec, so for those legacy agents
            // we ask IssueCertificate to verify the signature without the version check.
            bool acceptLegacyVersion = false;
            if (m_agentVersion != nullptr)
            {
               uint32_t major = 0, minor = 0, release = 0;
               if (_stscanf(m_agentVersion, _T("%u.%u.%u"), &major, &minor, &release) >= 2)
               {
                  uint32_t encoded = (major << 16) | (minor << 8) | release;
                  acceptLegacyVersion = (encoded <= ((6u << 16) | (1u << 8) | 2u));
               }
            }

            X509 *cert = IssueCertificate(certRequest, ou.c_str(), cn.c_str(), days, acceptLegacyVersion);
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
   EventBuilder(EVENT_TUNNEL_SETUP_ERROR, GetServerEventSourceId())
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
   shared_ptr<InboundAgentTunnel> tunnel;
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
         AddRemoteCRL(dp.getUTF8StdString().c_str(), true);
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

   tunnel = InboundAgentTunnel::create(context, ssl, request->sock, request->addr, nodeId, zoneUIN,
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
   virtual ConnectionProcessingResult processConnection(SOCKET s, const InetAddress& peer) override;
   virtual bool isStopConditionReached() override;

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

   WaitForServerStartupCompletion();

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

   SharedObjectArray<InboundAgentTunnel> processingList(16, 16);

   s_tunnelListLock.lock();
   time_t now = time(nullptr);
   for(int i = 0; i < s_unboundTunnels.size(); i++)
   {
      shared_ptr<InboundAgentTunnel> t = s_unboundTunnels.getShared(i);
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
      InboundAgentTunnel *t = processingList.get(i);
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Processing timeout for unbound tunnel from %s (%s) - action=%d"),
               t->getDisplayName(), t->getAddress().toString().cstr(), action);
      switch(action)
      {
         case RESET:
            t->shutdown();
            break;
         case GENERATE_EVENT:
            EventBuilder(EVENT_UNBOUND_TUNNEL, GetServerEventSourceId())
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
   SharedObjectArray<InboundAgentTunnel> processingList(16, 16);

   s_tunnelListLock.lock();
   time_t now = time(nullptr);
   int32_t reissueInterval = ConfigReadInt(_T("AgentTunnels.Certificates.ReissueInterval"), 30) * 86400;
   auto it = s_boundTunnels.begin();
   while(it.hasNext())
   {
      shared_ptr<AgentTunnel> t = it.next();
      if (t->isInbound() && !t->isExtProvCertificate() && (t->getCertificateExpirationTime() > 0) &&
          ((t->getCertificateExpirationTime() - now <= 2592000) || (now - t->getCertificateIssueTime() >= reissueInterval))) // 30 days
      {
         processingList.add(static_pointer_cast<InboundAgentTunnel>(t));
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
      InboundAgentTunnel *t = processingList.get(i);
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
      auto matchFilter = [filter] (const AgentTunnel& t) -> bool
      {
         return (filter == TunnelCapabilityFilter::AGENT_PROXY && t.isAgentProxy()) ||
                (filter == TunnelCapabilityFilter::SNMP_PROXY && t.isSnmpProxy()) ||
                (filter == TunnelCapabilityFilter::SNMP_TRAP_PROXY && t.isSnmpTrapProxy()) ||
                (filter == TunnelCapabilityFilter::SYSLOG_PROXY && t.isSyslogProxy()) ||
                (filter == TunnelCapabilityFilter::USER_AGENT && t.isUserAgentInstalled());
      };

      s_tunnelListLock.lock();
      if (boundTunnels)
      {
         auto it = s_boundTunnels.begin();
         while (it.hasNext())
         {
            if (matchFilter(*it.next()))
               count++;
         }
      }
      else
      {
         for (int i = 0; i < s_unboundTunnels.size(); i++)
         {
            if (matchFilter(*s_unboundTunnels.get(i)))
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
