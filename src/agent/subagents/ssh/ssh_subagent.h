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
** File: ssh.h
**
**/

#ifndef _ssh_subagent_h_
#define _ssh_subagent_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_agent.h>
#include <libssh/libssh.h>

#define DEBUG_TAG _T("ssh")

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
 * SSH session
 */
class SSHSession
{
private:
   int32_t m_id;
   StringBuffer m_name;
   InetAddress m_addr;
   uint16_t m_port;
   StringBuffer m_login;
   ssh_session m_session;
   time_t m_lastAccess;
   bool m_busy;

   bool execute(const TCHAR *command, StringList* output, ActionExecutionContext* context);

public:
   SSHSession(const InetAddress& addr, uint16_t port, int32_t id = 0);
   ~SSHSession();

   bool connect(const TCHAR *user, const TCHAR *password, const shared_ptr<KeyPair>& keys);
   void disconnect();
   bool isConnected() const { return (m_session != nullptr) && ssh_is_connected(m_session); }

   const TCHAR *getName() const { return m_name.cstr(); }
   time_t getLastAccessTime() const { return m_lastAccess; }
   bool isBusy() const { return m_busy; }

   bool match(const InetAddress& addr, uint16_t port, const TCHAR *login) const;

   bool acquire();
   void release();

   StringList *execute(const TCHAR *command);
   bool execute(const TCHAR *command, const shared_ptr<ActionExecutionContext>& context);

   /**
    * Open interactive channel with PTY and shell for network device CLI access
    * @return SSH channel handle on success, nullptr on failure
    */
   ssh_channel openInteractiveChannel();

   /**
    * Test if command execution channel is available
    * @return true if command execution works, false otherwise
    */
   bool testCommandChannel(const char *command = nullptr);

   /**
    * Test if session can open new channels (lightweight connectivity test)
    * @return true if session is usable, false otherwise
    */
   bool testSession();
};

/**
 * SSH channel proxy - forwards raw data between server and SSH PTY channel
 */
class SSHChannelProxy
{
private:
   uint32_t m_channelId;
   ssh_channel m_sshChannel;
   AbstractCommSession *m_commSession;
   THREAD m_readerThread;
   bool m_running;

   void readerThreadInternal();
   static void readerThreadStarter(SSHChannelProxy *proxy);

public:
   SSHChannelProxy(uint32_t channelId, ssh_channel channel, AbstractCommSession *session);
   ~SSHChannelProxy();

   void start();
   void stop();

   uint32_t getChannelId() const { return m_channelId; }

   void writeToChannel(const BYTE *data, size_t size);
};

/* Channel proxy management */
void InitializeSSHChannelProxyManager();
void ShutdownSSHChannelProxyManager();
SSHChannelProxy *CreateSSHChannelProxy(uint32_t channelId, ssh_channel channel, AbstractCommSession *session);
SSHChannelProxy *FindSSHChannelProxy(uint32_t channelId);
void DeleteSSHChannelProxy(uint32_t channelId);
bool HandleSSHChannelCommand(uint32_t command, NXCPMessage *request, NXCPMessage *response, AbstractCommSession *session);

/* Key functions */
shared_ptr<KeyPair> GetSshKey(AbstractCommSession *session, uint32_t id);

/* Session pool */
void InitializeSessionPool();
void ShutdownSessionPool();
SSHSession *AcquireSession(const InetAddress& addr, uint16_t port, const TCHAR *user, const TCHAR *password, const shared_ptr<KeyPair>& keys);
void ReleaseSession(SSHSession *session);

/* handlers */
LONG H_SSHCommand(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_SSHCommandList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session);
uint32_t H_SSHCommandAction(const shared_ptr<ActionExecutionContext>& context);
LONG H_SSHConnection(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_SSHCheckCommandMode(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);

/* globals */
extern uint32_t g_sshConnectTimeout;
extern uint32_t g_sshSessionIdleTimeout;
extern char g_sshConfigFile[];
extern bool g_sshChannelReadBugWorkaround;

#endif
