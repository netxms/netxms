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
** File: agent_tunnel.h
**
**/

#ifndef _agent_tunnel_h_
#define _agent_tunnel_h_

#include <nxsrvapi.h>

class AgentTunnel;

/**
 * Agent tunnel communication channel
 */
class LIBNXSRV_EXPORTABLE AgentTunnelCommChannel : public AbstractCommChannel
{
private:
   weak_ptr<AgentTunnel> m_tunnel;
   uint32_t m_id;
   bool m_active;
   RingBuffer m_buffer;
#ifdef _WIN32
   CRITICAL_SECTION m_bufferLock;
   CONDITION_VARIABLE m_dataCondition;
#else
   pthread_mutex_t m_bufferLock;
   pthread_cond_t m_dataCondition;
#endif
   struct
   {
      void (*callback)(BackgroundSocketPollResult, AbstractCommChannel*, void*);
      void *context;
   } m_pollers[16];
   int m_pollerCount;

public:
   AgentTunnelCommChannel(const shared_ptr<AgentTunnel>& tunnel, uint32_t id);
   virtual ~AgentTunnelCommChannel();

   virtual ssize_t send(const void *data, size_t size, Mutex *mutex = nullptr) override;
   virtual ssize_t recv(void *buffer, size_t size, uint32_t timeout = INFINITE) override;
   virtual int poll(uint32_t timeout, bool write = false) override;
   virtual void backgroundPoll(uint32_t timeout, void (*callback)(BackgroundSocketPollResult, AbstractCommChannel*, void*), void *context) override;
   virtual int shutdown() override;
   virtual void close() override;

   uint32_t getId() const { return m_id; }

   void putData(const BYTE *data, size_t size);

   void detach()
   {
      m_active = false;
      m_tunnel.reset();
   }
};

/**
 * Tunnel state
 */
enum AgentTunnelState
{
   AGENT_TUNNEL_INIT = 0,
   AGENT_TUNNEL_UNBOUND = 1,
   AGENT_TUNNEL_BOUND = 2,
   AGENT_TUNNEL_SHUTDOWN = 3
};

/**
 * Agent tunnel base class - initiator-agnostic session core shared by inbound
 * (agent to server) and outbound (server to agent) tunnels
 */
class LIBNXSRV_EXPORTABLE AgentTunnel
{
protected:
   weak_ptr<AgentTunnel> m_self;
   uint32_t m_id;
   uuid m_guid;
   InetAddress m_address;
   SOCKET m_socket;
   BackgroundSocketPollerHandle *m_socketPoller;
   TlsMessageReceiver *m_messageReceiver;
   TCHAR m_threadPoolKey[12];
   SSL_CTX *m_context;
   SSL *m_ssl;
   Mutex m_sslLock;
   Mutex m_writeLock;
   MsgWaitQueue m_queue;
   VolatileCounter m_requestId;
   uint32_t m_commandTimeout;
   uint32_t m_nodeId;
   int32_t m_zoneUIN;
   TCHAR *m_certificateSubject;
   TCHAR *m_certificateIssuer;
   time_t m_certificateExpirationTime;
   time_t m_certificateIssueTime;
   AgentTunnelState m_state;
   time_t m_startTime;
   GenericId<HARDWARE_ID_LENGTH> m_hardwareId;
   TCHAR *m_serialNumber;
   TCHAR *m_systemName;
   TCHAR m_hostname[MAX_DNS_NAME];
   TCHAR *m_platformName;
   TCHAR *m_systemInfo;
   TCHAR *m_agentVersion;
   TCHAR *m_agentBuildTag;
   uuid m_agentId;
   bool m_userAgentInstalled;
   bool m_agentProxy;
   bool m_snmpProxy;
   bool m_snmpTrapProxy;
   bool m_syslogProxy;
   bool m_extProvCertificate;
   StructArray<MacAddress> m_macAddressList;
   SharedHashMap<uint32_t, AgentTunnelCommChannel> m_channels;
   Mutex m_channelLock;

   AgentTunnel(SSL_CTX *context, SSL *ssl, SOCKET sock, const InetAddress& addr, uint32_t nodeId, int32_t zoneUIN,
            BackgroundSocketPollerHandle *socketPoller);

   bool readSocket();
   MessageReceiverResult readMessage(bool allowSocketRead);
   void finalize();
   void processMessage(NXCPMessage *msg);
   static void socketPollerCallback(BackgroundSocketPollResult pollResult, SOCKET hSocket, AgentTunnel *tunnel);

   int sslWrite(const void *data, size_t size);
   bool sendMessage(const NXCPMessage& msg);
   NXCPMessage *waitForMessage(uint16_t code, uint32_t id) { return m_queue.waitForMessage(code, id, m_commandTimeout); }
   NXCPMessage *waitForMessage(uint16_t code, uint32_t id, uint32_t timeout) { return m_queue.waitForMessage(code, id, timeout); }

   void processChannelClose(uint32_t channelId);

   void setup(const NXCPMessage *request);

   virtual bool processCustomMessage(NXCPMessage *msg);  // returns true if message was consumed
   virtual void onSetupComplete();
   virtual void onTunnelClose();

public:
   virtual ~AgentTunnel();

   shared_ptr<AgentTunnel> self() { return m_self.lock(); }

   void start();
   void shutdown();
   void setCommandTimeout(uint32_t timeout) { m_commandTimeout = timeout; }
   shared_ptr<AgentTunnelCommChannel> createChannel();
   void closeChannel(AgentTunnelCommChannel *channel);
   ssize_t sendChannelData(uint32_t id, const void *data, size_t len);
   void resetStartTime() { m_startTime = time(nullptr); }

   virtual bool isInbound() const = 0;

   uint32_t getId() const { return m_id; }
   uuid getGUID() const { return m_guid; }
   const InetAddress& getAddress() const { return m_address; }
   const GenericId<HARDWARE_ID_LENGTH>& getHardwareId() const { return m_hardwareId; }
   const TCHAR *getSerialNumber() const { return CHECK_NULL_EX(m_serialNumber); }
   const TCHAR *getSystemName() const { return CHECK_NULL_EX(m_systemName); }
   const TCHAR *getHostname() const { return m_hostname; }
   const TCHAR *getDisplayName() const { return (m_hostname[0] != 0) ? m_hostname : CHECK_NULL_EX(m_systemName); }
   const TCHAR *getSystemInfo() const { return CHECK_NULL_EX(m_systemInfo); }
   const TCHAR *getPlatformName() const { return CHECK_NULL_EX(m_platformName); }
   const TCHAR *getAgentVersion() const { return CHECK_NULL_EX(m_agentVersion); }
   const TCHAR *getAgentBuildTag() const { return CHECK_NULL_EX(m_agentBuildTag); }
   const uuid& getAgentId() const { return m_agentId; }
   int32_t getZoneUIN() const { return m_zoneUIN; }
   bool isBound() const { return m_nodeId != 0; }
   uint32_t getNodeId() const { return m_nodeId; }
   time_t getCertificateExpirationTime() const { return m_certificateExpirationTime; }
   time_t getCertificateIssueTime() const { return m_certificateIssueTime; }
   const StructArray<MacAddress>& getMacAddressList() const { return m_macAddressList; }
   AgentTunnelState getState() const { return m_state; }
   time_t getStartTime() const { return m_startTime; }

   int getChannelCount()
   {
      m_channelLock.lock();
      int c = m_channels.size();
      m_channelLock.unlock();
      return c;
   }

   bool isAgentProxy() const { return m_agentProxy; }
   bool isSnmpProxy() const { return m_snmpProxy; }
   bool isSnmpTrapProxy() const { return m_snmpTrapProxy; }
   bool isSyslogProxy() const { return m_syslogProxy; }
   bool isUserAgentInstalled() const { return m_userAgentInstalled; }
   bool isExtProvCertificate() const { return m_extProvCertificate; }   // Check if certificate is externally provisioned

   void fillMessage(NXCPMessage *msg, uint32_t baseId) const;

   void debugPrintf(int level, const TCHAR *format, ...);
};

/**
 * Outbound (server to agent) tunnel establishment status
 */
enum class AgentTunnelEstablishmentStatus
{
   SUCCESS = 0,
   CONNECT_FAILED = 1,
   TLS_HANDSHAKE_FAILED = 2,
   CERTIFICATE_MISMATCH = 3,
   SETUP_TIMEOUT = 4
};

/**
 * Outbound (server to agent) tunnel established over TLS connection initiated by server
 */
class LIBNXSRV_EXPORTABLE OutboundAgentTunnel : public AgentTunnel
{
private:
   Condition m_setupCondition;
   std::function<void(OutboundAgentTunnel*)> m_closeCallback;

   OutboundAgentTunnel(SSL_CTX *context, SSL *ssl, SOCKET sock, const InetAddress& addr, uint32_t nodeId, int32_t zoneUIN,
            BackgroundSocketPollerHandle *socketPoller);

   static void keepaliveTimer(const shared_ptr<OutboundAgentTunnel>& tunnel);

protected:
   virtual void onSetupComplete() override;
   virtual void onTunnelClose() override;

public:
   /**
    * Establish outbound tunnel to agent. Performs TCP connect, TLS handshake, agent certificate
    * fingerprint check, and waits for tunnel setup completion. Peer certificate's SHA-256
    * fingerprint is always stored into actualFingerprint (SHA256_DIGEST_SIZE bytes) when
    * available. If expectedFingerprint is not null it is checked against the peer certificate
    * and mismatch fails the establishment. Optional tlsContextSetup callback can be used to
    * additionally configure TLS context (client certificate, minimal TLS version, etc.).
    * Optional closeCallback is called when tunnel is closed (before tunnel channels shutdown).
    */
   static shared_ptr<OutboundAgentTunnel> establish(const InetAddress& addr, uint16_t port, uint32_t nodeId, int32_t zoneUIN,
            const BYTE *expectedFingerprint, BYTE *actualFingerprint, AgentTunnelEstablishmentStatus *status,
            bool (*tlsContextSetup)(SSL_CTX*) = nullptr, std::function<void(OutboundAgentTunnel*)> closeCallback = nullptr);

   virtual bool isInbound() const override { return false; }
};

#endif
