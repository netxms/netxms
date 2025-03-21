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
** File: session.cpp
**/

#include "ssh_subagent.h"

/**
 * SSH session constructor
 */
SSHSession::SSHSession(const InetAddress& addr, uint16_t port, int32_t id)
{
   m_id = id;
   m_addr = addr;
   m_port = port;
   m_session = nullptr;
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
 * SSH session destructor
 */
SSHSession::~SSHSession()
{
   disconnect();
}

/**
 * Check if session match for given target
 */
bool SSHSession::match(const InetAddress& addr, uint16_t port, const TCHAR *login) const
{
   return addr.equals(m_addr) && (port == m_port) && !_tcscmp(m_login, login);
}

/**
 * Acquire session
 */
bool SSHSession::acquire()
{
   if (m_busy || !isConnected())
      return false;
   m_busy = true;
   return true;
}

/**
 * Release session
 */
void SSHSession::release()
{
   m_busy = false;
}

/**
 * Connect to server
 */
bool SSHSession::connect(const TCHAR *user, const TCHAR *password, const shared_ptr<KeyPair>& keys)
{
   if (m_session != nullptr)
      return false;  // already connected

   m_session = ssh_new();
   if (m_session == nullptr)
      return false;  // cannot create session

   bool success = false;

   char hostname[64];
   ssh_options_set(m_session, SSH_OPTIONS_HOST, m_addr.toStringA(hostname));
   unsigned int port = m_port;
   ssh_options_set(m_session, SSH_OPTIONS_PORT, &port);
   long timeout = (long)g_sshConnectTimeout * (long)1000;   // convert milliseconds to microseconds
   ssh_options_set(m_session, SSH_OPTIONS_TIMEOUT_USEC, &timeout);
#ifdef UNICODE
   char mbuser[256];
   wchar_to_utf8(user, -1, mbuser, 256);
   ssh_options_set(m_session, SSH_OPTIONS_USER, mbuser);
#else
   ssh_options_set(m_session, SSH_OPTIONS_USER, user);
#endif

   if (ssh_options_parse_config(m_session, (g_sshConfigFile[0] != 0) ? g_sshConfigFile : nullptr) != 0)
      nxlog_debug_tag(DEBUG_TAG, 6, _T("SSHSession::connect: config load for %s:%d failed (%hs)"), (const TCHAR *)m_addr.toString(), m_port, ssh_get_error(m_session));

   if (ssh_connect(m_session) == SSH_OK)
   {
      if (keys != nullptr)
      {
         ssh_key pkey;
         if (ssh_pki_import_pubkey_base64(keys->publicKey, keys->type, &pkey) == SSH_OK)
         {
            nxlog_debug_tag(DEBUG_TAG, 7, _T("SSHSession::connect: try to login with public key"));

            if (ssh_userauth_try_publickey(m_session, NULL, pkey) == SSH_AUTH_SUCCESS)
            {
               success = true;
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG, 6, _T("SSHSession::connect: login with key as %s on %s:%d failed (%hs)"), user, (const TCHAR *)m_addr.toString(), m_port, ssh_get_error(m_session));
            }
            ssh_key_free(pkey);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 6, _T("SSHSession::connect: SSH key load for %s:%d failed (%hs)"), (const TCHAR *)m_addr.toString(), m_port, ssh_get_error(m_session));
         }

         if (ssh_pki_import_privkey_base64(keys->privateKey, nullptr, nullptr, nullptr, &pkey) == SSH_OK)
         {
            nxlog_debug_tag(DEBUG_TAG, 7, _T("SSHSession::connect: try to login with private key"));

            if (ssh_userauth_publickey(m_session, NULL, pkey) == SSH_AUTH_SUCCESS)
            {
               success = true;
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG, 6, _T("SSHSession::connect: login with key as %s on %s:%d failed (%hs)"), user, (const TCHAR *)m_addr.toString(), m_port, ssh_get_error(m_session));
            }
            ssh_key_free(pkey);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 6, _T("SSHSession::connect: SSH key load for %s:%d failed (%hs)"), (const TCHAR *)m_addr.toString(), m_port, ssh_get_error(m_session));
         }
      }

      if (!success)
      {
         nxlog_debug_tag(DEBUG_TAG, 7, _T("SSHSession::connect: try to login with password"));
#ifdef UNICODE
         char mbpassword[256];
         wchar_to_utf8(password, -1, mbpassword, 256);
         if (ssh_userauth_password(m_session, NULL, mbpassword) == SSH_AUTH_SUCCESS)
#else
         if (ssh_userauth_password(m_session, NULL, password) == SSH_AUTH_SUCCESS)
#endif
         {
            success = true;
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 6, _T("SSHSession::connect: login with password as %s on %s:%d failed (%hs)"), user, m_addr.toString().cstr(), m_port, ssh_get_error(m_session));
         }
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("SSHSession::connect: connect to %s:%d failed (%hs)"), m_addr.toString().cstr(), m_port, ssh_get_error(m_session));
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
      nxlog_debug_tag(DEBUG_TAG, 2, _T("session null"));
      if (ssh_is_connected(m_session))
         ssh_disconnect(m_session);
      ssh_free(m_session);
      m_session = nullptr;
   }
   return success;
}

/**
 * Disconnect from server
 */
void SSHSession::disconnect()
{
   if (m_session == nullptr)
      return;

   if (ssh_is_connected(m_session))
      ssh_disconnect(m_session);
   ssh_free(m_session);
   m_session = nullptr;
}

/**
 * Check if ssh_channel_read returned "Remote channel is closed" and current libssh version
 * can contain bug in ssh_channel_read that cause return of such error on normal eof.
 * https://github.com/ParallelSSH/ssh-python/issues/23
 */
static inline bool CheckForChannelReadBug(ssh_session session)
{
   return g_sshChannelReadBugWorkaround && (strstr(ssh_get_error(session), "Remote channel is closed") != nullptr);
}

/**
 * Execute command and capture output
 */
StringList* SSHSession::execute(const TCHAR *command)
{
   auto output = new StringList();
   if (!execute(command, output, nullptr))
   {
      delete_and_null(output);
   }
   return output;
}

/**
 * Execute command and feed output to provided action context
 */
bool SSHSession::execute(const TCHAR *command, const shared_ptr<ActionExecutionContext>& context)
{
   return execute(command, nullptr, context.get());
}

/**
 * Execute command process output as requested
 */
bool SSHSession::execute(const TCHAR *command, StringList *output, ActionExecutionContext *context)
{
   if ((m_session == nullptr) || !ssh_is_connected(m_session))
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("SSHSession::execute: is not connected"));
      return false;
   }

   ssh_channel channel = ssh_channel_new(m_session);
   if (channel == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("SSHSession::execute: channel is null"));
      return false;
   }

   bool result = false;
   if (ssh_channel_open_session(channel) == SSH_OK)
   {
#ifdef UNICODE
      char *mbcmd = UTF8StringFromWideString(command);
      if (ssh_channel_request_exec(channel, mbcmd) == SSH_OK)
#else
      if (ssh_channel_request_exec(channel, command) == SSH_OK)
#endif
      {
         if (context != nullptr)
            context->markAsCompleted(ERR_SUCCESS);

         result = true;
         char buffer[8192];
         int nbytes = ssh_channel_read(channel, buffer, sizeof(buffer) - 1, 0);
         size_t offset = 0;
         while(nbytes > 0)
         {
            buffer[nbytes + offset] = 0;
            if (context != nullptr)
            {
               context->sendOutputUtf8(buffer);
            }
            else
            {
               char *curr = buffer;
               char *eol = strchr(curr, '\n');
               while(eol != nullptr)
               {
                  *eol = 0;
                  char *cr = strchr(curr, '\r');
                  if (cr != nullptr)
                     *cr = 0;
                  output->addMBString(curr);
                  curr = eol + 1;
                  eol = strchr(curr, '\n');
               }
               offset = strlen(curr);
               if (offset > 0)
                  memmove(buffer, curr, offset);
            }
            nbytes = ssh_channel_read(channel, &buffer[offset], static_cast<uint32_t>(sizeof(buffer) - offset - 1), 0);
         }
         if ((nbytes == 0) || CheckForChannelReadBug(m_session))
         {
            if (offset > 0)
            {
               buffer[offset] = 0;
               if (context != nullptr)
               {
                  context->sendOutputUtf8(buffer);
               }
               else
               {
                  char *curr = buffer;
                  char *eol = strchr(curr, '\n');
                  while(eol != nullptr)
                  {
                     *eol = 0;
                     char *cr = strchr(curr, '\r');
                     if (cr != nullptr)
                        *cr = 0;
                     output->addMBString(curr);
                     curr = eol + 1;
                     eol = strchr(curr, '\n');
                  }
                  offset = strlen(curr);
                  if (offset > 0)
                     output->addMBString(curr);
               }
            }
            ssh_channel_send_eof(channel);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 6, _T("SSHSession::execute: read error: %hs"), ssh_get_error(m_session));
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("SSHSession::execute: command \"%s\" execution on %s:%d failed"), command, m_addr.toString().cstr(), m_port);
      }
      ssh_channel_close(channel);
#ifdef UNICODE
      MemFree(mbcmd);
#endif
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("SSHSession::execute: cannot open channel on %s:%d"), m_addr.toString().cstr(), m_port);
   }
   ssh_channel_free(channel);
   m_lastAccess = time(nullptr);
   return result;
}
