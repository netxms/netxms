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

/**
 * Tunnel state
 */
enum AgentTunnelState
{
   AGENT_TUNNEL_INIT = 0,
   AGENT_TUNNEL_UNBOUND = 1,
   AGENT_TUNNEL_BOUND = 2
};

/**
 * Agent tunnel
 */
class AgentTunnel : public RefCountObject
{
protected:
   INT32 m_id;
   InetAddress m_address;
   SOCKET m_socket;
   SSL_CTX *m_context;
   SSL *m_ssl;
   MUTEX m_sslLock;
   THREAD m_recvThread;
   UINT32 m_nodeId;
   AgentTunnelState m_state;
   TCHAR *m_systemName;
   TCHAR *m_platformName;
   TCHAR *m_systemInfo;
   TCHAR *m_agentVersion;
   
   void debugPrintf(int level, const TCHAR *format, ...);
   
   void recvThread();
   static THREAD_RESULT THREAD_CALL recvThreadStarter(void *arg);
   
   void setup(const NXCPMessage *request);

public:
   AgentTunnel(SSL_CTX *context, SSL *ssl, SOCKET sock, const InetAddress& addr, UINT32 nodeId);
   virtual ~AgentTunnel();
   
   void start();
   bool sendMessage(NXCPMessage *msg);

   UINT32 getId() const { return m_id; }
   const InetAddress& getAddress() const { return m_address; }
   const TCHAR *getSystemName() const { return m_systemName; }
   const TCHAR *getSystemInfo() const { return m_systemInfo; }
   const TCHAR *getPlatformName() const { return m_platformName; }
   const TCHAR *getAgentVersion() const { return m_agentVersion; }
   bool isBound() const { return m_nodeId != 0; }
   UINT32 getNodeId() const { return m_nodeId; }
};

/**
 * Setup server side TLS context
 */
bool SetupServerTlsContext(SSL_CTX *context);

#endif
