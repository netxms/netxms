/*
** NetXMS SSH subagent
** Copyright (C) 2004-2025 Raden Solutions
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
** File: ssh_proxy.cpp
**
**/

#include "ssh_subagent.h"

/**
 * Channel proxy list and lock
 */
static ObjectArray<SSHChannelProxy> s_channelProxies(16, 16, Ownership::True);
static Mutex s_channelProxyLock;

/**
 * Initialize SSH channel proxy manager
 */
void InitializeSSHChannelProxyManager()
{
   nxlog_debug_tag(DEBUG_TAG, 2, _T("SSH channel proxy manager initialized"));
}

/**
 * Shutdown SSH channel proxy manager
 */
void ShutdownSSHChannelProxyManager()
{
   s_channelProxyLock.lock();
   for(int i = 0; i < s_channelProxies.size(); i++)
   {
      s_channelProxies.get(i)->stop();
   }
   s_channelProxies.clear();
   s_channelProxyLock.unlock();
   nxlog_debug_tag(DEBUG_TAG, 2, _T("SSH channel proxy manager shut down"));
}

/**
 * Create new SSH channel proxy
 */
SSHChannelProxy *CreateSSHChannelProxy(uint32_t channelId, ssh_channel channel, AbstractCommSession *session)
{
   SSHChannelProxy *proxy = new SSHChannelProxy(channelId, channel, session);
   s_channelProxyLock.lock();
   s_channelProxies.add(proxy);
   s_channelProxyLock.unlock();
   proxy->start();
   nxlog_debug_tag(DEBUG_TAG, 5, _T("SSH channel proxy %u created"), channelId);
   return proxy;
}

/**
 * Find SSH channel proxy by ID
 */
SSHChannelProxy *FindSSHChannelProxy(uint32_t channelId)
{
   SSHChannelProxy *proxy = nullptr;
   s_channelProxyLock.lock();
   for(int i = 0; i < s_channelProxies.size(); i++)
   {
      if (s_channelProxies.get(i)->getChannelId() == channelId)
      {
         proxy = s_channelProxies.get(i);
         break;
      }
   }
   s_channelProxyLock.unlock();
   return proxy;
}

/**
 * Delete SSH channel proxy
 */
void DeleteSSHChannelProxy(uint32_t channelId)
{
   s_channelProxyLock.lock();
   for(int i = 0; i < s_channelProxies.size(); i++)
   {
      if (s_channelProxies.get(i)->getChannelId() == channelId)
      {
         s_channelProxies.get(i)->stop();
         s_channelProxies.remove(i);
         nxlog_debug_tag(DEBUG_TAG, 5, _T("SSH channel proxy %u deleted"), channelId);
         break;
      }
   }
   s_channelProxyLock.unlock();
}

/**
 * SSHChannelProxy constructor
 */
SSHChannelProxy::SSHChannelProxy(uint32_t channelId, ssh_channel channel, AbstractCommSession *session)
{
   m_channelId = channelId;
   m_sshChannel = channel;
   m_commSession = session;
   m_readerThread = INVALID_THREAD_HANDLE;
   m_running = false;
}

/**
 * SSHChannelProxy destructor
 */
SSHChannelProxy::~SSHChannelProxy()
{
   stop();
   if (m_sshChannel != nullptr)
   {
      ssh_channel_close(m_sshChannel);
      ssh_channel_free(m_sshChannel);
   }
}

/**
 * Reader thread starter
 */
void SSHChannelProxy::readerThreadStarter(SSHChannelProxy *proxy)
{
   proxy->readerThreadInternal();
}

/**
 * Reader thread
 */
void SSHChannelProxy::readerThreadInternal()
{
   BYTE buffer[65536];

   nxlog_debug_tag(DEBUG_TAG, 6, _T("SSH channel proxy %u reader thread started"), m_channelId);

   while(m_running)
   {
      // Read with 100ms timeout for responsive shutdown
      int nbytes = ssh_channel_read_timeout(m_sshChannel,
                                            buffer + NXCP_HEADER_SIZE,
                                            sizeof(buffer) - NXCP_HEADER_SIZE,
                                            0, 100);
      if (nbytes > 0)
      {
         // Build and send NXCP message (same pattern as TcpProxy)
         NXCP_MESSAGE *header = reinterpret_cast<NXCP_MESSAGE*>(buffer);
         header->code = htons(CMD_SSH_CHANNEL_DATA);
         header->id = htonl(m_channelId);
         header->flags = htons(MF_BINARY);
         header->numFields = htonl(static_cast<uint32_t>(nbytes));

         uint32_t size = NXCP_HEADER_SIZE + nbytes;
         if ((size % 8) != 0)
            size += 8 - size % 8;
         header->size = htonl(size);

         m_commSession->postRawMessage(header);
      }
      else if (nbytes < 0)
      {
         // Error or channel closed
         if (!ssh_channel_is_eof(m_sshChannel))
         {
            nxlog_debug_tag(DEBUG_TAG, 6, _T("SSH channel proxy %u read error"), m_channelId);
         }
         break;
      }
      // nbytes == 0 means timeout, continue loop
   }

   // Notify server of channel closure
   if (m_running)
   {
      NXCPMessage msg(CMD_CLOSE_SSH_CHANNEL, 0, m_commSession->getProtocolVersion());
      msg.setField(VID_CHANNEL_ID, m_channelId);
      msg.setField(VID_ERROR_INDICATOR, !ssh_channel_is_eof(m_sshChannel));
      m_commSession->postMessage(&msg);
   }

   nxlog_debug_tag(DEBUG_TAG, 6, _T("SSH channel proxy %u reader thread stopped"), m_channelId);
}

/**
 * Start the reader thread
 */
void SSHChannelProxy::start()
{
   m_running = true;
   m_readerThread = ThreadCreateEx(SSHChannelProxy::readerThreadStarter, this);
}

/**
 * Stop the reader thread
 */
void SSHChannelProxy::stop()
{
   m_running = false;
   if (m_readerThread != INVALID_THREAD_HANDLE)
   {
      ThreadJoin(m_readerThread);
      m_readerThread = INVALID_THREAD_HANDLE;
   }
}

/**
 * Write data to the SSH channel
 */
void SSHChannelProxy::writeToChannel(const BYTE *data, size_t size)
{
   if (m_sshChannel != nullptr && m_running)
   {
      int written = ssh_channel_write(m_sshChannel, data, static_cast<uint32_t>(size));
      if (written < 0)
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("SSH channel proxy %u write error"), m_channelId);
      }
   }
}

/**
 * Handle SSH channel command from server
 * Called for CMD_SETUP_SSH_CHANNEL, CMD_SSH_CHANNEL_DATA, CMD_CLOSE_SSH_CHANNEL
 * For binary messages (CMD_SSH_CHANNEL_DATA), response will be nullptr
 */
bool HandleSSHChannelCommand(uint32_t command, NXCPMessage *request, NXCPMessage *response, AbstractCommSession *session)
{
   switch(command)
   {
      case CMD_SETUP_SSH_CHANNEL:
         {
            if (response == nullptr)
               return false;

            InetAddress addr = request->getFieldAsInetAddress(VID_IP_ADDRESS);
            uint16_t port = request->getFieldAsUInt16(VID_PORT);
            if (port == 0)
               port = SSH_PORT;
            uint32_t channelId = request->getFieldAsUInt32(VID_CHANNEL_ID);

            TCHAR user[256], password[256];
            request->getFieldAsString(VID_USER_NAME, user, 256);
            request->getFieldAsString(VID_PASSWORD, password, 256);

            uint32_t keyId = request->getFieldAsUInt32(VID_SSH_KEY_ID);
            shared_ptr<KeyPair> keys;
            if (keyId != 0)
            {
               keys = GetSshKey(session, keyId);
            }

            // Get or create SSH session
            SSHSession *sshSession = AcquireSession(addr, port, user, password, keys);
            if (sshSession == nullptr)
            {
               TCHAR ipAddrText[64];
               nxlog_debug_tag(DEBUG_TAG, 5, _T("CMD_SETUP_SSH_CHANNEL: cannot create SSH session to %s:%u"),
                               addr.toString(ipAddrText), port);
               response->setField(VID_RCC, ERR_REMOTE_CONNECT_FAILED);
               return true;
            }

            // Open interactive channel
            ssh_channel channel = sshSession->openInteractiveChannel();
            if (channel == nullptr)
            {
               TCHAR ipAddrText[64];
               nxlog_debug_tag(DEBUG_TAG, 5, _T("CMD_SETUP_SSH_CHANNEL: cannot open interactive channel on %s:%u"),
                               addr.toString(ipAddrText), port);
               ReleaseSession(sshSession);
               response->setField(VID_RCC, ERR_SSH_CHANNEL_OPEN_FAILED);
               return true;
            }

            // Create proxy
            SSHChannelProxy *proxy = CreateSSHChannelProxy(channelId, channel, session);
            if (proxy == nullptr)
            {
               ssh_channel_close(channel);
               ssh_channel_free(channel);
               ReleaseSession(sshSession);
               response->setField(VID_RCC, ERR_INTERNAL_ERROR);
               return true;
            }

            TCHAR ipAddrText[64];
            nxlog_debug_tag(DEBUG_TAG, 5, _T("CMD_SETUP_SSH_CHANNEL: channel %u opened to %s:%u"),
                            channelId, addr.toString(ipAddrText), port);

            response->setField(VID_RCC, ERR_SUCCESS);
            response->setField(VID_CHANNEL_ID, channelId);

            // Note: We don't release the SSH session - it stays acquired while the channel is open
            // The session will be released when the channel is closed
            return true;
         }

      case CMD_SSH_CHANNEL_DATA:
         {
            // Binary message - response is nullptr
            uint32_t channelId = request->getId();
            SSHChannelProxy *proxy = FindSSHChannelProxy(channelId);
            if (proxy != nullptr)
            {
               proxy->writeToChannel(request->getBinaryData(), request->getBinaryDataSize());
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG, 6, _T("CMD_SSH_CHANNEL_DATA: channel %u not found"), channelId);
            }
            return true;
         }

      case CMD_CLOSE_SSH_CHANNEL:
         {
            if (response == nullptr)
               return false;

            uint32_t channelId = request->getFieldAsUInt32(VID_CHANNEL_ID);
            DeleteSSHChannelProxy(channelId);
            response->setField(VID_RCC, ERR_SUCCESS);
            return true;
         }

      default:
         return false;
   }
}
