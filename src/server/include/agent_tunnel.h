/* 
** NetXMS - Network Management System
** Server Core
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
** File: nxcore.h
**
**/

#ifndef _agent_tunnel_h_
#define _agent_tunnel_h_

#include <nms_util.h>

class AgentTunnel;

/**
 * Agent tunnel communication channel
 */
class AgentTunnelCommChannel : public AbstractCommChannel
{
private:
   AgentTunnel *m_tunnel;
   UINT32 m_id;
   bool m_active;
   RingBuffer m_buffer;
#ifdef _WIN32
   CRITICAL_SECTION m_bufferLock;
   HANDLE m_dataCondition;
#else
   pthread_mutex_t m_bufferLock;
   pthread_cond_t m_dataCondition;
#endif

protected:
   virtual ~AgentTunnelCommChannel();

public:
   AgentTunnelCommChannel(AgentTunnel *tunnel, UINT32 id);

   virtual int send(const void *data, size_t size, MUTEX mutex = INVALID_MUTEX_HANDLE);
   virtual int recv(void *buffer, size_t size, UINT32 timeout = INFINITE);
   virtual int poll(UINT32 timeout, bool write = false);
   virtual int shutdown();
   virtual void close();

   UINT32 getId() const { return m_id; }

   void putData(const BYTE *data, size_t size);
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
 * Agent tunnel
 */
class AgentTunnel : public RefCountObject
{
protected:
   INT32 m_id;
   uuid m_guid;
   InetAddress m_address;
   SOCKET m_socket;
   SSL_CTX *m_context;
   SSL *m_ssl;
   MUTEX m_sslLock;
   MsgWaitQueue m_queue;
   VolatileCounter m_requestId;
   UINT32 m_nodeId;
   UINT32 m_zoneUIN;
   AgentTunnelState m_state;
   TCHAR *m_systemName;
   TCHAR *m_platformName;
   TCHAR *m_systemInfo;
   TCHAR *m_agentVersion;
   UINT32 m_bindRequestId;
   uuid m_bindGuid;
   RefCountHashMap<UINT32, AgentTunnelCommChannel> m_channels;
   MUTEX m_channelLock;
   
   virtual ~AgentTunnel();

   void recvThread();
   static THREAD_RESULT THREAD_CALL recvThreadStarter(void *arg);
   
   int sslWrite(const void *data, size_t size);
   bool sendMessage(NXCPMessage *msg);
   NXCPMessage *waitForMessage(UINT16 code, UINT32 id) { return m_queue.waitForMessage(code, id, 5000); }

   void processCertificateRequest(NXCPMessage *request);
   void processChannelClose(UINT32 channelId);

   void setup(const NXCPMessage *request);

public:
   AgentTunnel(SSL_CTX *context, SSL *ssl, SOCKET sock, const InetAddress& addr, UINT32 nodeId, UINT32 zoneUIN);
   
   void start();
   void shutdown();
   UINT32 bind(UINT32 nodeId);
   AgentTunnelCommChannel *createChannel();
   void closeChannel(AgentTunnelCommChannel *channel);
   int sendChannelData(UINT32 id, const void *data, size_t len);

   UINT32 getId() const { return m_id; }
   const InetAddress& getAddress() const { return m_address; }
   const TCHAR *getSystemName() const { return m_systemName; }
   const TCHAR *getSystemInfo() const { return m_systemInfo; }
   const TCHAR *getPlatformName() const { return m_platformName; }
   const TCHAR *getAgentVersion() const { return m_agentVersion; }
   UINT32 getZoneUIN() const { return m_zoneUIN; }
   bool isBound() const { return m_nodeId != 0; }
   UINT32 getNodeId() const { return m_nodeId; }
   UINT32 getState() const { return m_state; }

   void fillMessage(NXCPMessage *msg, UINT32 baseId) const;

   void debugPrintf(int level, const TCHAR *format, ...);
};

/**
 * Setup server side TLS context
 */
bool SetupServerTlsContext(SSL_CTX *context);

/**
 * Get tunnel for node
 */
AgentTunnel *GetTunnelForNode(UINT32 nodeId);

#endif
