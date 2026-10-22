/*
** NetXMS SSH subagent
** Copyright (C) 2004-2026 Raden Solutions
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
   // Proxies are deleted outside the lock: destructor joins the reader thread, which may itself be waiting for the lock
   ObjectArray<SSHChannelProxy> proxies(0, 16, Ownership::True);
   s_channelProxyLock.lock();
   while(s_channelProxies.size() > 0)
   {
      proxies.add(s_channelProxies.get(0));
      s_channelProxies.unlink(0);
   }
   s_channelProxyLock.unlock();
   proxies.clear();
   nxlog_debug_tag(DEBUG_TAG, 2, _T("SSH channel proxy manager shut down"));
}

/**
 * Create new SSH channel proxy
 */
static void CreateSSHChannelProxy(uint32_t channelId, ssh_channel channel, SSHSession *sshSession, AbstractCommSession *session)
{
   SSHChannelProxy *proxy = new SSHChannelProxy(channelId, channel, sshSession, session);
   s_channelProxyLock.lock();
   s_channelProxies.add(proxy);
   s_channelProxyLock.unlock();
   proxy->start();
   nxlog_debug_tag(DEBUG_TAG, 5, _T("SSH channel proxy %u created"), channelId);
}

/**
 * Find SSH channel proxy by owning communication session and channel ID. Channel IDs are allocated by the server
 * per connection, so the same ID can be in use by channels of different sessions at the same time; a session can
 * only reach channels it has opened itself. Caller must hold s_channelProxyLock for the whole time the returned
 * pointer is in use, because proxy can be deleted by its own reader thread when remote side closes the channel.
 */
static SSHChannelProxy *FindSSHChannelProxy(AbstractCommSession *session, uint32_t channelId)
{
   for(int i = 0; i < s_channelProxies.size(); i++)
   {
      SSHChannelProxy *proxy = s_channelProxies.get(i);
      if ((proxy->getChannelId() == channelId) && (proxy->getCommSession() == session))
         return proxy;
   }
   return nullptr;
}

/**
 * Remove SSH channel proxy from the list without deleting it. Returns false if proxy is not in the list.
 */
static bool UnlinkSSHChannelProxy(SSHChannelProxy *proxy)
{
   s_channelProxyLock.lock();
   bool unlinked = s_channelProxies.unlink(proxy);
   s_channelProxyLock.unlock();
   return unlinked;
}

/**
 * Delete SSH channel proxy (server-initiated close)
 */
static void DeleteSSHChannelProxy(AbstractCommSession *session, uint32_t channelId)
{
   s_channelProxyLock.lock();
   SSHChannelProxy *proxy = FindSSHChannelProxy(session, channelId);
   if (proxy != nullptr)
      s_channelProxies.unlink(proxy);
   s_channelProxyLock.unlock();

   // Deleted outside the lock: destructor joins the reader thread, which may itself be waiting for the lock
   if (proxy != nullptr)
   {
      delete proxy;
      nxlog_debug_tag(DEBUG_TAG, 5, _T("SSH channel proxy %u deleted"), channelId);
   }
}

/**
 * SSHChannelProxy constructor
 */
SSHChannelProxy::SSHChannelProxy(uint32_t channelId, ssh_channel channel, SSHSession *sshSession, AbstractCommSession *session) : m_commSession(session->self())
{
   m_channelId = channelId;
   m_sshChannel = channel;
   m_sshSession = sshSession;
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
   ReleaseSession(m_sshSession);
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
      else if (ssh_channel_is_eof(m_sshChannel))
      {
         // libssh returns 0 both on timeout and at EOF; remote side closed the channel (e.g. shell exit)
         nxlog_debug_tag(DEBUG_TAG, 6, _T("SSH channel proxy %u reached end of stream"), m_channelId);
         break;
      }
      // nbytes == 0 without EOF means timeout, continue loop
   }

   nxlog_debug_tag(DEBUG_TAG, 6, _T("SSH channel proxy %u reader thread stopped"), m_channelId);

   if (!m_running)
      return;  // stopped by server-initiated close or subagent shutdown; owner deletes this object

   // Remote side closed the channel: notify server and dispose of this proxy right away, so that libssh channel
   // and the session it belongs to are released without waiting for the server. Deletion happens on the reader
   // thread itself, so the thread handle is detached. If server-initiated close is running concurrently, proxy
   // is already unlinked by that thread, which will delete it after this thread exits.
   NXCPMessage msg(CMD_CLOSE_SSH_CHANNEL, 0, m_commSession->getProtocolVersion());
   msg.setField(VID_CHANNEL_ID, m_channelId);
   msg.setField(VID_ERROR_INDICATOR, !ssh_channel_is_eof(m_sshChannel));
   m_commSession->postMessage(&msg);

   if (UnlinkSSHChannelProxy(this))
   {
      m_running = false;
      ThreadDetach(m_readerThread);
      m_readerThread = INVALID_THREAD_HANDLE;
      nxlog_debug_tag(DEBUG_TAG, 5, _T("SSH channel proxy %u deleted (channel closed by remote side)"), m_channelId);
      delete this;
   }
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
 * Change PTY size of the SSH channel
 */
void SSHChannelProxy::resize(int columns, int rows)
{
   if (m_sshChannel != nullptr && m_running)
   {
      if (ssh_channel_change_pty_size(m_sshChannel, columns, rows) != SSH_OK)
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("SSH channel proxy %u PTY resize to %dx%d failed"), m_channelId, columns, rows);
      }
   }
}

/**
 * Handle SSH channel command from server
 * Called for CMD_SETUP_SSH_CHANNEL, CMD_SSH_CHANNEL_DATA, CMD_RESIZE_SSH_CHANNEL, CMD_CLOSE_SSH_CHANNEL
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

            char terminalType[32] = "";
            request->getFieldAsUtf8String(VID_TERMINAL_TYPE, terminalType, 32);
            if (terminalType[0] == 0)
               strcpy(terminalType, "vt100");

            // PTY size; 200x24 if not provided by server (wide enough for parsing long output lines)
            int columns = request->getFieldAsInt32(VID_WIDTH);
            int rows = request->getFieldAsInt32(VID_HEIGHT);
            if ((columns < 1) || (columns > 1024))
               columns = 200;
            if ((rows < 1) || (rows > 1024))
               rows = 24;

            // Get or create SSH session - always create new session for interactive channels
            // because some devices (like Cisco) don't support multiple channels per session
            SSHSession *sshSession = AcquireSession(addr, port, user, password, keys, true);
            if (sshSession == nullptr)
            {
               TCHAR ipAddrText[64];
               nxlog_debug_tag(DEBUG_TAG, 5, _T("CMD_SETUP_SSH_CHANNEL: cannot create SSH session to %s:%u"),
                               addr.toString(ipAddrText), port);
               response->setField(VID_RCC, ERR_REMOTE_CONNECT_FAILED);
               return true;
            }

            // Open interactive channel
            ssh_channel channel = sshSession->openInteractiveChannel(terminalType, columns, rows);
            if (channel == nullptr)
            {
               TCHAR ipAddrText[64];
               nxlog_debug_tag(DEBUG_TAG, 5, _T("CMD_SETUP_SSH_CHANNEL: cannot open interactive channel on %s:%u"),
                               addr.toString(ipAddrText), port);
               ReleaseSession(sshSession);
               response->setField(VID_RCC, ERR_SSH_CHANNEL_OPEN_FAILED);
               return true;
            }

            CreateSSHChannelProxy(channelId, channel, sshSession, session);
            response->setField(VID_RCC, ERR_SUCCESS);
            response->setField(VID_CHANNEL_ID, channelId);

            TCHAR ipAddrText[64];
            nxlog_debug_tag(DEBUG_TAG, 5, _T("CMD_SETUP_SSH_CHANNEL: channel %u opened to %s:%u"), channelId, addr.toString(ipAddrText), port);

            // Note: We don't release the SSH session - it stays acquired while the channel is open
            // The session will be released when the channel is closed
            return true;
         }

      case CMD_SSH_CHANNEL_DATA:
         {
            // Binary message - response is nullptr
            uint32_t channelId = request->getId();
            s_channelProxyLock.lock();
            SSHChannelProxy *proxy = FindSSHChannelProxy(session, channelId);
            if (proxy != nullptr)
            {
               proxy->writeToChannel(request->getBinaryData(), request->getBinaryDataSize());
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG, 6, _T("CMD_SSH_CHANNEL_DATA: channel %u not found"), channelId);
            }
            s_channelProxyLock.unlock();
            return true;
         }

      case CMD_RESIZE_SSH_CHANNEL:
         {
            if (response == nullptr)
               return false;

            uint32_t channelId = request->getFieldAsUInt32(VID_CHANNEL_ID);
            int columns = request->getFieldAsInt32(VID_WIDTH);
            int rows = request->getFieldAsInt32(VID_HEIGHT);
            if ((columns < 1) || (columns > 1024) || (rows < 1) || (rows > 1024))
            {
               response->setField(VID_RCC, ERR_BAD_ARGUMENTS);
               return true;
            }

            s_channelProxyLock.lock();
            SSHChannelProxy *proxy = FindSSHChannelProxy(session, channelId);
            if (proxy != nullptr)
            {
               proxy->resize(columns, rows);
               response->setField(VID_RCC, ERR_SUCCESS);
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG, 6, _T("CMD_RESIZE_SSH_CHANNEL: channel %u not found"), channelId);
               response->setField(VID_RCC, ERR_INVALID_OBJECT);
            }
            s_channelProxyLock.unlock();
            return true;
         }

      case CMD_CLOSE_SSH_CHANNEL:
         {
            if (response == nullptr)
               return false;

            uint32_t channelId = request->getFieldAsUInt32(VID_CHANNEL_ID);
            DeleteSSHChannelProxy(session, channelId);
            response->setField(VID_RCC, ERR_SUCCESS);
            return true;
         }

      default:
         return false;
   }
}
