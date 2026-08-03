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
** File: nxcore_agent_tunnel.h
**
**/

#ifndef _nxcore_agent_tunnel_h_
#define _nxcore_agent_tunnel_h_

#include <agent_tunnel.h>

#define UNBOUND_TUNNEL_PROCESSOR_TASK_ID _T("AgentTunnels.ProcessUnbound")
#define RENEW_AGENT_CERTIFICATES_TASK_ID _T("AgentTunnels.RenewCertificates")

/**
 * Inbound (agent to server) tunnel with bind/unbind workflow and certificate issuance.
 * Session core is implemented by AgentTunnel base class in libnxsrv.
 */
class InboundAgentTunnel : public AgentTunnel
{
protected:
   uint32_t m_bindRequestId;
   uuid m_bindGuid;
   uint32_t m_bindUserId;
   bool m_resetPending;
   shared_ptr<AgentTunnel> m_replacedTunnel; // Tunnel that was replaced by this tunnel

   void processCertificateRequest(NXCPMessage *request);

   uint32_t initiateCertificateRequest(const uuid& nodeGuid, uint32_t userId);

   virtual bool processCustomMessage(NXCPMessage *msg) override;
   virtual void onSetupComplete() override;
   virtual void onTunnelClose() override;

public:
   static shared_ptr<InboundAgentTunnel> create(SSL_CTX *context, SSL *ssl, SOCKET sock, const InetAddress& addr, uint32_t nodeId,
            int32_t zoneUIN, const TCHAR *certificateSubject, const TCHAR *certificateIssuer, time_t certificateExpirationTime,
            time_t certificateIssueTime, BackgroundSocketPollerHandle *socketPoller);

   InboundAgentTunnel(SSL_CTX *context, SSL *ssl, SOCKET sock, const InetAddress& addr, uint32_t nodeId, int32_t zoneUIN,
            const TCHAR *certificateSubject, const TCHAR *certificateIssuer, time_t certificateExpirationTime,
            time_t certificateIssueTime, BackgroundSocketPollerHandle *socketPoller);

   shared_ptr<InboundAgentTunnel> self() { return static_pointer_cast<InboundAgentTunnel>(m_self.lock()); }

   virtual bool isInbound() const override { return true; }

   uint32_t bind(uint32_t nodeId, uint32_t userId);
   uint32_t renewCertificate();
   void setReplacedTunnel(const shared_ptr<AgentTunnel>& tunnel) { m_replacedTunnel = tunnel; }

   bool isResetPending() const { return m_resetPending; }
};

/**
 * Get tunnel for node
 */
shared_ptr<AgentTunnel> GetTunnelForNode(uint32_t nodeId);

/**
 * Register outbound tunnel established by node. Returns effective tunnel for the node - either
 * given tunnel or already registered one (in that case given tunnel is shut down).
 */
shared_ptr<AgentTunnel> RegisterOutboundTunnel(const shared_ptr<OutboundAgentTunnel>& tunnel);

/**
 * Unregister outbound tunnel (used as tunnel close callback)
 */
void UnregisterOutboundTunnel(OutboundAgentTunnel *tunnel);

/**
 * Setup TLS context for outbound agent tunnel (presents server certificate to agent if available)
 */
bool SetupAgentTunnelTlsContext(SSL_CTX *context);

/**
 * Tunnel capability filter
 */
enum class TunnelCapabilityFilter
{
   ANY = 0,
   AGENT_PROXY = 1,
   SNMP_PROXY = 2,
   SNMP_TRAP_PROXY = 3,
   SYSLOG_PROXY = 4,
   USER_AGENT = 5
};

/**
 * Get tunnel type by type
 */
int GetTunnelCount(TunnelCapabilityFilter filter, bool boundTunnels);

/**
 * Update agent certificate mapping index for externally provisioned certificates
 */
void UpdateAgentCertificateMappingIndex(const shared_ptr<Node>& node, const TCHAR *oldValue, const TCHAR *newValue);

#endif
