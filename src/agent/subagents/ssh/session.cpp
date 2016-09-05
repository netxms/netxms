/*
** NetXMS SSH subagent
** Copyright (C) 2004-2016 Victor Kirhenshtein
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
SSHSession::SSHSession(const InetAddress& addr, UINT16 port, INT32 id)
{
   m_id = id;
   m_addr = addr;
   m_port = port;
   m_session = NULL;
   m_lastAccess = 0;
   m_user[0] = 0;
   m_busy = false;
   _sntprintf(m_name, MAX_SSH_SESSION_NAME_LEN, _T("nobody@%s:%d/%d"), (const TCHAR *)m_addr.toString(), m_port, m_id);
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
bool SSHSession::match(const InetAddress& addr, UINT16 port, const TCHAR *user) const
{
   return addr.equals(m_addr) && ((unsigned int)port == m_port) && !_tcscmp(m_user, user);
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
bool SSHSession::connect(const TCHAR *user, const TCHAR *password)
{
   if (m_session != NULL)
      return false;  // already connected

   m_session = ssh_new();
   if (m_session == NULL)
      return false;  // cannot create session

   bool success = false;

   char hostname[64];
   ssh_options_set(m_session, SSH_OPTIONS_HOST, m_addr.toStringA(hostname));
   ssh_options_set(m_session, SSH_OPTIONS_PORT, &m_port);
   long timeout = (long)g_sshConnectTimeout * (long)1000;   // convert milliseconds to microseconds
   ssh_options_set(m_session, SSH_OPTIONS_TIMEOUT_USEC, &timeout);
#ifdef UNICODE
   char mbuser[256];
   WideCharToMultiByte(CP_UTF8, 0, user, -1, mbuser, 256, NULL, NULL);
   ssh_options_set(m_session, SSH_OPTIONS_USER, mbuser);
#else
   ssh_options_set(m_session, SSH_OPTIONS_USER, user);
#endif

   if (ssh_connect(m_session) == SSH_OK)
   {
#ifdef UNICODE
      char mbpassword[256];
      WideCharToMultiByte(CP_UTF8, 0, password, -1, mbpassword, 256, NULL, NULL);
      if (ssh_userauth_password(m_session, NULL, mbpassword) == SSH_AUTH_SUCCESS)
#else
      if (ssh_userauth_password(m_session, NULL, password) == SSH_AUTH_SUCCESS)
#endif
      {
         success = true;
      }
      else
      {
         nxlog_debug(6, _T("SSH: login as %s on %s:%d failed"), user, (const TCHAR *)m_addr.toString(), m_port);
      }
   }
   else
   {
      nxlog_debug(6, _T("SSH: connect to %s:%d failed"), (const TCHAR *)m_addr.toString(), m_port);
   }

   if (success)
   {
      nx_strncpy(m_user, user, MAX_SSH_LOGIN_LEN);
      _sntprintf(m_name, MAX_SSH_SESSION_NAME_LEN, _T("%s@%s:%d/%d"), m_user, (const TCHAR *)m_addr.toString(), m_port, m_id);
      m_lastAccess = time(NULL);
   }
   else
   {
      if (ssh_is_connected(m_session))
         ssh_disconnect(m_session);
      ssh_free(m_session);
      m_session = NULL;
   }
   return success;
}

/**
 * Disconnect from server
 */
void SSHSession::disconnect()
{
   if (m_session == NULL)
      return;

   if (ssh_is_connected(m_session))
      ssh_disconnect(m_session);
   ssh_free(m_session);
   m_session = NULL;
}

/**
 * Execute command and capture output
 */
StringList *SSHSession::execute(const TCHAR *command)
{
   if ((m_session == NULL) || !ssh_is_connected(m_session))
      return NULL;

   ssh_channel channel = ssh_channel_new(m_session);
   if (channel == NULL)
      return NULL;

   StringList *output = NULL;
   if (ssh_channel_open_session(channel) == SSH_OK)
   {
#ifdef UNICODE
      char *mbcmd = UTF8StringFromWideString(command);
      if (ssh_channel_request_exec(channel, mbcmd) == SSH_OK)
#else
      if (ssh_channel_request_exec(channel, command) == SSH_OK)
#endif
      {
         output = new StringList();
         char buffer[8192];
         int nbytes = ssh_channel_read(channel, buffer, sizeof(buffer) - 1, 0);
         int offset = 0;
         while(nbytes > 0)
         {
            buffer[nbytes + offset] = 0;
            char *curr = buffer;
            char *eol = strchr(curr, '\n');
            while(eol != NULL)
            {
               *eol = 0;
               char *cr = strchr(curr, '\r');
               if (cr != NULL)
                  *cr = 0;
               output->addMBString(curr);
               curr = eol + 1;
               eol = strchr(curr, '\n');
            }
            offset = (int)strlen(curr);
            if (offset > 0)
               memmove(buffer, curr, offset);
            nbytes = ssh_channel_read(channel, &buffer[offset], sizeof(buffer) - offset - 1, 0);
         }
         if (nbytes == 0)
         {
            if (offset > 0)
            {
               buffer[offset] = 0;
               char *cr = strchr(buffer, '\r');
               if (cr != NULL)
                  *cr = 0;
               output->addMBString(buffer);
            }
            ssh_channel_send_eof(channel);
         }
         else
         {
            delete_and_null(output);
         }
      }
      else
      {
         nxlog_debug(6, _T("SSH: command \"%s\" execution on %s:%d failed"), command, (const TCHAR *)m_addr.toString(), m_port);
      }
      ssh_channel_close(channel);
   }
   else
   {
      nxlog_debug(6, _T("SSH: cannot open channel on %s:%d"), (const TCHAR *)m_addr.toString(), m_port);
   }
   ssh_channel_free(channel);
   m_lastAccess = time(NULL);
   return output;
}
