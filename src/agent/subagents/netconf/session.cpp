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
** File: session.cpp
**/

#include "netconf_subagent.h"

/**
 * NETCONF session constructor
 */
NETCONFSession::NETCONFSession(const InetAddress& addr, uint16_t port, int32_t id)
{
   m_id = id;
   m_addr = addr;
   m_port = port;
   m_ssh = nullptr;
   m_channel = nullptr;
   m_peerSessionId = 0;
   m_messageId = 0;
   m_lastAccess = 0;
   m_busy = false;
   m_name.append(_T("nobody@"));
   m_name.append(m_addr.toString());
   m_name.append(_T(':'));
   m_name.append(m_port);
   m_name.append(_T('/'));
   m_name.append(m_id);
}

/**
 * NETCONF session destructor
 */
NETCONFSession::~NETCONFSession()
{
   disconnect();
}

/**
 * Check if session match for given target
 */
bool NETCONFSession::match(const InetAddress& addr, uint16_t port, const TCHAR *login) const
{
   return addr.equals(m_addr) && (port == m_port) && !_tcscmp(m_login, login);
}

/**
 * Acquire session
 */
bool NETCONFSession::acquire()
{
   if (m_busy || !isConnected())
      return false;
   m_busy = true;
   return true;
}

/**
 * Release session
 */
void NETCONFSession::release()
{
   m_busy = false;
}

/**
 * Authenticate on SSH transport level (public key, then password, then keyboard-interactive)
 */
bool NETCONFSession::authenticate(const TCHAR *user, const TCHAR *password, const shared_ptr<KeyPair>& keys)
{
   bool success = false;

   if (keys != nullptr)
   {
      ssh_key pkey;
      if (ssh_pki_import_pubkey_base64(keys->publicKey, keys->type, &pkey) == SSH_OK)
      {
         nxlog_debug_tag(DEBUG_TAG, 7, _T("NETCONFSession::authenticate: try to login with public key"));
         if (ssh_userauth_try_publickey(m_ssh, nullptr, pkey) == SSH_AUTH_SUCCESS)
            success = true;
         else
            nxlog_debug_tag(DEBUG_TAG, 6, _T("NETCONFSession::authenticate: login with key as %s on %s:%d failed (%hs)"), user, m_addr.toString().cstr(), m_port, ssh_get_error(m_ssh));
         ssh_key_free(pkey);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("NETCONFSession::authenticate: SSH public key load for %s:%d failed (%hs)"), m_addr.toString().cstr(), m_port, ssh_get_error(m_ssh));
      }

      if (ssh_pki_import_privkey_base64(keys->privateKey, nullptr, nullptr, nullptr, &pkey) == SSH_OK)
      {
         nxlog_debug_tag(DEBUG_TAG, 7, _T("NETCONFSession::authenticate: try to login with private key"));
         if (ssh_userauth_publickey(m_ssh, nullptr, pkey) == SSH_AUTH_SUCCESS)
            success = true;
         else
            nxlog_debug_tag(DEBUG_TAG, 6, _T("NETCONFSession::authenticate: login with key as %s on %s:%d failed (%hs)"), user, m_addr.toString().cstr(), m_port, ssh_get_error(m_ssh));
         ssh_key_free(pkey);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("NETCONFSession::authenticate: SSH private key load for %s:%d failed (%hs)"), m_addr.toString().cstr(), m_port, ssh_get_error(m_ssh));
      }
   }

   if (!success)
   {
#ifdef UNICODE
      char mbpassword[256];
      wchar_to_utf8(password, -1, mbpassword, 256);
      const char *utf8password = mbpassword;
#else
      const char *utf8password = password;
#endif

      nxlog_debug_tag(DEBUG_TAG, 7, _T("NETCONFSession::authenticate: try to login with password"));
      if (ssh_userauth_password(m_ssh, nullptr, utf8password) == SSH_AUTH_SUCCESS)
      {
         success = true;
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("NETCONFSession::authenticate: login with password as %s on %s:%d failed (%hs)"), user, m_addr.toString().cstr(), m_port, ssh_get_error(m_ssh));

         nxlog_debug_tag(DEBUG_TAG, 7, _T("NETCONFSession::authenticate: try keyboard-interactive auth"));
         int rc = ssh_userauth_kbdint(m_ssh, nullptr, nullptr);
         while(rc == SSH_AUTH_INFO)
         {
            int nprompts = ssh_userauth_kbdint_getnprompts(m_ssh);
            for(int i = 0; i < nprompts; i++)
               ssh_userauth_kbdint_setanswer(m_ssh, i, utf8password);
            rc = ssh_userauth_kbdint(m_ssh, nullptr, nullptr);
         }
         if (rc == SSH_AUTH_SUCCESS)
            success = true;
         else
            nxlog_debug_tag(DEBUG_TAG, 6, _T("NETCONFSession::authenticate: keyboard-interactive auth as %s on %s:%d failed (%hs)"), user, m_addr.toString().cstr(), m_port, ssh_get_error(m_ssh));
      }
   }

   return success;
}

/**
 * Send NETCONF message to peer using currently negotiated framing
 */
bool NETCONFSession::sendMessage(const pugi::xml_document& document)
{
   ByteStream data(8192);
   NETCONF_EncodeMessage(document, m_decoder.getVersion(), data);

   size_t size;
   const BYTE *bytes = data.buffer(&size);
   size_t offset = 0;
   while(offset < size)
   {
      int written = ssh_channel_write(m_channel, bytes + offset, static_cast<uint32_t>(size - offset));
      if (written <= 0)
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("NETCONFSession::sendMessage: write to %s:%d failed (%hs)"), m_addr.toString().cstr(), m_port, ssh_get_error(m_ssh));
         return false;
      }
      offset += written;
   }
   return true;
}

/**
 * Read next NETCONF message from peer. Returns dynamically allocated message text
 * (caller is responsible for calling MemFree on it) or nullptr on failure or timeout.
 */
char *NETCONFSession::readMessage(uint32_t timeout)
{
   char *message = m_decoder.readMessage();
   if (message != nullptr)
      return message;

   int64_t deadline = GetCurrentTimeMs() + timeout;
   char buffer[16384];
   while(true)
   {
      int64_t remaining = deadline - GetCurrentTimeMs();
      if (remaining <= 0)
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("NETCONFSession::readMessage: timeout reading message from %s:%d"), m_addr.toString().cstr(), m_port);
         return nullptr;
      }

      int nbytes = ssh_channel_read_timeout(m_channel, buffer, sizeof(buffer), 0, static_cast<int>(remaining));
      if (nbytes <= 0)
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("NETCONFSession::readMessage: read from %s:%d failed (%hs)"), m_addr.toString().cstr(), m_port, ssh_get_error(m_ssh));
         return nullptr;
      }

      m_decoder.feed(buffer, nbytes);
      if (m_decoder.isError())
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("NETCONFSession::readMessage: framing error in stream from %s:%d"), m_addr.toString().cstr(), m_port);
         return nullptr;
      }

      message = m_decoder.readMessage();
      if (message != nullptr)
         return message;
   }
}

/**
 * Perform hello exchange (always in end-of-message framing) and switch to negotiated framing
 */
bool NETCONFSession::performHandshake()
{
   pugi::xml_document hello;
   NETCONF_BuildHelloMessage(hello);
   if (!sendMessage(hello))
      return false;

   char *peerHello = readMessage(g_netconfConnectTimeout);
   if (peerHello == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("NETCONFSession::performHandshake: no hello message from %s:%d"), m_addr.toString().cstr(), m_port);
      return false;
   }

   bool success = m_peerCapabilities.parseHelloMessage(peerHello, strlen(peerHello), &m_peerSessionId);
   MemFree(peerHello);
   if (!success)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("NETCONFSession::performHandshake: invalid hello message from %s:%d"), m_addr.toString().cstr(), m_port);
      return false;
   }

   m_decoder.setVersion(m_peerCapabilities.getProtocolVersion());
   nxlog_debug_tag(DEBUG_TAG, 6, _T("NETCONFSession::performHandshake: session %u established with %s:%d (version %s, %d capabilities)"),
         m_peerSessionId, m_addr.toString().cstr(), m_port,
         (m_decoder.getVersion() == NetconfVersion::NETCONF_1_1) ? _T("1.1") : _T("1.0"), m_peerCapabilities.size());
   return true;
}

/**
 * Connect to device: SSH transport, "netconf" subsystem, hello exchange
 */
bool NETCONFSession::connect(const TCHAR *user, const TCHAR *password, const shared_ptr<KeyPair>& keys)
{
   if (m_ssh != nullptr)
      return false;  // already connected

   m_ssh = ssh_new();
   if (m_ssh == nullptr)
      return false;

   char hostname[64];
   ssh_options_set(m_ssh, SSH_OPTIONS_HOST, m_addr.toStringA(hostname));
   unsigned int port = m_port;
   ssh_options_set(m_ssh, SSH_OPTIONS_PORT, &port);
   long timeout = (long)g_netconfConnectTimeout * (long)1000;   // convert milliseconds to microseconds
   ssh_options_set(m_ssh, SSH_OPTIONS_TIMEOUT_USEC, &timeout);
#ifdef UNICODE
   char mbuser[256];
   wchar_to_utf8(user, -1, mbuser, 256);
   ssh_options_set(m_ssh, SSH_OPTIONS_USER, mbuser);
#else
   ssh_options_set(m_ssh, SSH_OPTIONS_USER, user);
#endif

   bool success = false;
   if (ssh_connect(m_ssh) == SSH_OK)
   {
      if (authenticate(user, password, keys))
      {
         m_channel = ssh_channel_new(m_ssh);
         if (m_channel != nullptr)
         {
            if (ssh_channel_open_session(m_channel) == SSH_OK)
            {
               if (ssh_channel_request_subsystem(m_channel, "netconf") == SSH_OK)
               {
                  success = performHandshake();
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG, 6, _T("NETCONFSession::connect: netconf subsystem request on %s:%d failed (%hs)"), m_addr.toString().cstr(), m_port, ssh_get_error(m_ssh));
               }
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG, 6, _T("NETCONFSession::connect: cannot open channel on %s:%d (%hs)"), m_addr.toString().cstr(), m_port, ssh_get_error(m_ssh));
            }
            if (!success)
            {
               ssh_channel_free(m_channel);
               m_channel = nullptr;
            }
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 6, _T("NETCONFSession::connect: cannot create channel on %s:%d"), m_addr.toString().cstr(), m_port);
         }
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("NETCONFSession::connect: connect to %s:%d failed (%hs)"), m_addr.toString().cstr(), m_port, ssh_get_error(m_ssh));
   }

   if (success)
   {
      m_login = user;
      m_name = user;
      m_name.append(_T('@'));
      m_name.append(m_addr.toString());
      m_name.append(_T(':'));
      m_name.append(m_port);
      m_name.append(_T('/'));
      m_name.append(m_id);
      m_lastAccess = time(nullptr);
   }
   else
   {
      if (ssh_is_connected(m_ssh))
         ssh_disconnect(m_ssh);
      ssh_free(m_ssh);
      m_ssh = nullptr;
   }
   return success;
}

/**
 * Disconnect from device
 */
void NETCONFSession::disconnect()
{
   if (m_channel != nullptr)
   {
      if (isConnected())
      {
         // Best effort close-session request, reply is not awaited
         pugi::xml_document rpc;
         NETCONF_BuildCloseSessionRequest(rpc, ++m_messageId);
         sendMessage(rpc);
      }
      ssh_channel_close(m_channel);
      ssh_channel_free(m_channel);
      m_channel = nullptr;
   }
   if (m_ssh != nullptr)
   {
      if (ssh_is_connected(m_ssh))
         ssh_disconnect(m_ssh);
      ssh_free(m_ssh);
      m_ssh = nullptr;
   }
}

/**
 * Execute single RPC and return raw rpc-reply document
 */
char *NETCONFSession::executeRpc(const char *content, uint32_t timeout, size_t *replySize)
{
   if (!isConnected())
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("NETCONFSession::executeRpc: session %s is not connected"), m_name.cstr());
      return nullptr;
   }

   uint32_t messageId = ++m_messageId;
   pugi::xml_document rpc;
   if (!NETCONF_BuildRpcRequest(rpc, messageId, content))
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("NETCONFSession::executeRpc: invalid RPC content"));
      return nullptr;
   }

   if (!sendMessage(rpc))
      return nullptr;

   int64_t deadline = GetCurrentTimeMs() + timeout;
   while(true)
   {
      int64_t remaining = deadline - GetCurrentTimeMs();
      if (remaining <= 0)
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("NETCONFSession::executeRpc: timeout waiting for reply from %s:%d"), m_addr.toString().cstr(), m_port);
         return nullptr;
      }

      char *message = readMessage(static_cast<uint32_t>(remaining));
      if (message == nullptr)
         return nullptr;

      pugi::xml_document reply;
      if (!reply.load_string(message))
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("NETCONFSession::executeRpc: malformed XML message from %s:%d"), m_addr.toString().cstr(), m_port);
         MemFree(message);
         return nullptr;
      }

      pugi::xml_node replyElement = NETCONF_FindChildByLocalName(reply, "rpc-reply");
      if (!replyElement)
      {
         // Not an RPC reply (e.g. asynchronous notification) - skip it
         nxlog_debug_tag(DEBUG_TAG, 7, _T("NETCONFSession::executeRpc: ignored non-reply message from %s:%d"), m_addr.toString().cstr(), m_port);
         MemFree(message);
         continue;
      }

      uint32_t replyId = replyElement.attribute("message-id").as_uint(0);
      if ((replyId != 0) && (replyId != messageId))
      {
         // Stale reply to a previous request - skip it
         nxlog_debug_tag(DEBUG_TAG, 7, _T("NETCONFSession::executeRpc: ignored reply with unexpected message ID %u (expected %u)"), replyId, messageId);
         MemFree(message);
         continue;
      }

      m_lastAccess = time(nullptr);
      if (replySize != nullptr)
         *replySize = strlen(message);
      return message;
   }
}
