/*
** NetXMS NETCONF subagent
** Copyright (C) 2026 Raden Solutions
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
** File: netconf_subagent.h
**
**/

#ifndef _netconf_subagent_h_
#define _netconf_subagent_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_agent.h>
#include <nxnetconf.h>
#include <libssh/libssh.h>

#define DEBUG_TAG _T("netconf")

/**
 * Key pair for SSH authentication
 */
struct KeyPair
{
   char *publicKey;
   char *pubKeySource;
   enum ssh_keytypes_e type;
   char *privateKey;

   KeyPair(char *privateKey, char *publicKey);
   KeyPair(const KeyPair& src) = delete;
   ~KeyPair();
};

/**
 * NETCONF session (SSH transport with "netconf" subsystem + NETCONF session layer)
 */
class NETCONFSession
{
private:
   int32_t m_id;
   StringBuffer m_name;
   InetAddress m_addr;
   uint16_t m_port;
   StringBuffer m_login;
   ssh_session m_ssh;
   ssh_channel m_channel;
   NETCONF_MessageDecoder m_decoder;
   NETCONF_CapabilitySet m_peerCapabilities;
   uint32_t m_peerSessionId;
   uint32_t m_messageId;
   time_t m_lastAccess;
   bool m_busy;

   bool authenticate(const TCHAR *user, const TCHAR *password, const shared_ptr<KeyPair>& keys);
   bool sendMessage(const pugi::xml_document& document);
   char *readMessage(uint32_t timeout);
   bool performHandshake();

public:
   NETCONFSession(const InetAddress& addr, uint16_t port, int32_t id = 0);
   ~NETCONFSession();

   bool connect(const TCHAR *user, const TCHAR *password, const shared_ptr<KeyPair>& keys);
   void disconnect();
   bool isConnected() const { return (m_channel != nullptr) && (ssh_channel_is_open(m_channel) != 0) && !ssh_channel_is_eof(m_channel); }

   const TCHAR *getName() const { return m_name.cstr(); }
   time_t getLastAccessTime() const { return m_lastAccess; }
   bool isBusy() const { return m_busy; }

   bool match(const InetAddress& addr, uint16_t port, const TCHAR *login) const;

   bool acquire();
   void release();

   const NETCONF_CapabilitySet& getPeerCapabilities() const { return m_peerCapabilities; }
   uint32_t getPeerSessionId() const { return m_peerSessionId; }

   /**
    * Execute single RPC. Content is placed inside rpc envelope; message ID is assigned by
    * the session. Returns raw rpc-reply document (caller is responsible for calling MemFree
    * on it) or nullptr on failure.
    */
   char *executeRpc(const char *content, uint32_t timeout, size_t *replySize = nullptr);
};

/* Key functions */
shared_ptr<KeyPair> GetSshKey(AbstractCommSession *session, uint32_t id);

/* Session pool */
void InitializeSessionPool();
void ShutdownSessionPool();
NETCONFSession *AcquireSession(const InetAddress& addr, uint16_t port, const TCHAR *user, const TCHAR *password, const shared_ptr<KeyPair>& keys);
void ReleaseSession(NETCONFSession *session, bool invalidate = false);

/* handlers */
LONG H_NETCONFCheckConnection(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_NETCONFCapabilities(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session);

/* message processing */
void ExecuteNETCONFRequest(const NXCPMessage& request, NXCPMessage *response, AbstractCommSession *session);

/* globals */
extern uint32_t g_netconfConnectTimeout;
extern uint32_t g_netconfRequestTimeout;
extern uint32_t g_netconfSessionIdleTimeout;

#endif
