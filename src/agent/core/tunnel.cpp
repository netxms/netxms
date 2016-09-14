/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2016 Victor Kirhenshtein
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
** File: tunnel.cpp
**
**/

#include "nxagentd.h"

/**
 * Tunnel class
 */
class Tunnel
{
private:
   InetAddress m_address;
   UINT16 m_port;
   TCHAR m_login[MAX_OBJECT_NAME];
   SOCKET m_socket;
   bool m_connected;
   UINT32 m_requestId;
   THREAD m_recvThread;
   MsgWaitQueue *m_queue;

   Tunnel(const InetAddress& addr, UINT16 port, const TCHAR *login);

   bool connectToServer();
   bool sendMessage(const NXCPMessage *msg);
   NXCPMessage *waitForMessage(UINT16 code, UINT32 id) { return (m_queue != NULL) ? m_queue->waitForMessage(code, id, 5000) : NULL; }
   void recvThread();

   static THREAD_RESULT THREAD_CALL recvThreadStarter(void *arg);

public:
   ~Tunnel();

   void checkConnection();
   void disconnect();

   const InetAddress& getAddress() const { return m_address; }
   const TCHAR *getLogin() const { return m_login; }

   static Tunnel *createFromConfig(TCHAR *config);
};

/**
 * Tunnel constructor
 */
Tunnel::Tunnel(const InetAddress& addr, UINT16 port, const TCHAR *login) : m_address(addr)
{
   m_port = port;
   nx_strncpy(m_login, login, MAX_OBJECT_NAME);
   m_socket = INVALID_SOCKET;
   m_connected = false;
   m_requestId = 0;
   m_recvThread = INVALID_THREAD_HANDLE;
   m_queue = NULL;
}

/**
 * Tunnel destructor
 */
Tunnel::~Tunnel()
{
   disconnect();
   if (m_socket != INVALID_SOCKET)
      closesocket(m_socket);
}

/**
 * Force disconnect
 */
void Tunnel::disconnect()
{
   if (m_socket != INVALID_SOCKET)
      shutdown(m_socket, SHUT_RDWR);
   m_connected = false;
   ThreadJoin(m_recvThread);
   delete_and_null(m_queue);
}

/**
 * Receiver thread starter
 */
THREAD_RESULT THREAD_CALL Tunnel::recvThreadStarter(void *arg)
{
   ((Tunnel *)arg)->recvThread();
   return THREAD_OK;
}

/**
 * Receiver thread
 */
void Tunnel::recvThread()
{
   SocketMessageReceiver receiver(m_socket, 8192, MAX_AGENT_MSG_SIZE);
   while(m_connected)
   {
      MessageReceiverResult result;
      NXCPMessage *msg = receiver.readMessage(1000, &result);
      if (msg != NULL)
      {
         m_queue->put(msg);
      }
      else if (result != MSGRECV_TIMEOUT)
      {
         nxlog_debug(4, _T("Receiver thread for tunnel %s@%s stopped (%s)"), \
                     m_login, (const TCHAR *)m_address.toString(), AbstractMessageReceiver::resultToText(result));
         break;
      }
   }
}

/**
 * Send message
 */
bool Tunnel::sendMessage(const NXCPMessage *msg)
{
   if (m_socket == INVALID_SOCKET)
      return false;

   NXCP_MESSAGE *data = msg->createMessage();
   bool success = (SendEx(m_socket, data, ntohl(data->size), 0, NULL) == ntohl(data->size));
   free(data);
   return success;
}

/**
 * Connect to server
 */
bool Tunnel::connectToServer()
{
   if (m_socket != INVALID_SOCKET)
      closesocket(m_socket);

   m_socket = socket(m_address.getFamily(), SOCK_STREAM, 0);
   if (m_socket == INVALID_SOCKET)
   {
      nxlog_debug(4, _T("Cannot create socket for tunnel %s@%s: %s"), m_login, (const TCHAR *)m_address.toString(), _tcserror(WSAGetLastError()));
      return false;
   }

   SockAddrBuffer sa;
   m_address.fillSockAddr(&sa, m_port);
   if (ConnectEx(m_socket, (struct sockaddr *)&sa, SA_LEN((struct sockaddr *)&sa), 5000) == -1)
   {
      nxlog_debug(4, _T("Cannot establish connection for tunnel %s@%s: %s"), m_login, (const TCHAR *)m_address.toString(), _tcserror(WSAGetLastError()));
      return false;
   }

   delete m_queue;
   m_queue = new MsgWaitQueue();
   m_recvThread = ThreadCreateEx(Tunnel::recvThreadStarter, 0, this);

   m_requestId = 1;

   NXCPMessage msg;
   msg.setCode(CMD_SETUP_AGENT_TUNNEL);
   msg.setId(m_requestId++);
   msg.setField(VID_LOGIN_NAME, m_login);
   msg.setField(VID_SHARED_SECRET, g_szSharedSecret);
   sendMessage(&msg);

   NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, msg.getId());
   if (response == NULL)
   {
      nxlog_debug(4, _T("Cannot establish connection for tunnel %s@%s: request timeout"), m_login, (const TCHAR *)m_address.toString());
      disconnect();
      return false;
   }

   UINT32 rcc = response->getFieldAsUInt32(VID_RCC);
   delete response;
   if (rcc != ERR_SUCCESS)
   {
      nxlog_debug(4, _T("Cannot establish connection for tunnel %s@%s: error %d"), m_login, (const TCHAR *)m_address.toString(), rcc);
      disconnect();
      return false;
   }

   m_connected = true;
   return true;
}

/**
 * Check tunnel connection and connect as needed
 */
void Tunnel::checkConnection()
{
   if (!m_connected)
   {
      if (connectToServer())
         nxlog_debug(3, _T("Tunnel %s@%s active"), m_login, (const TCHAR *)m_address.toString());
   }
   else
   {
      NXCPMessage msg;
      msg.setCode(CMD_KEEPALIVE);
      msg.setId(m_requestId++);
      if (sendMessage(&msg))
      {
         NXCPMessage *response = waitForMessage(CMD_KEEPALIVE, msg.getId());
         if (response == NULL)
         {
            disconnect();
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
            nxlog_debug(3, _T("Connection test failed for tunnel %s@%s"), m_login, (const TCHAR *)m_address.toString());
         }
         else
         {
            delete response;
         }
      }
      else
      {
         disconnect();
         closesocket(m_socket);
         m_socket = INVALID_SOCKET;
         nxlog_debug(3, _T("Connection test failed for tunnel %s@%s"), m_login, (const TCHAR *)m_address.toString());
      }
   }
}

/**
 * Create tunnel object from configuration record
 */
Tunnel *Tunnel::createFromConfig(TCHAR *config)
{
   TCHAR *a = _tcschr(config, _T('@'));
   if (a == NULL)
      return NULL;

   a++;
   int port = AGENT_TUNNEL_PORT;
   TCHAR *p = _tcschr(a, _T(':'));
   if (p != NULL)
   {
      *p = 0;
      p++;

      TCHAR *eptr;
      int port = _tcstol(p, &eptr, 10);
      if ((port < 1) || (port > 65535))
         return NULL;
   }

   InetAddress addr = InetAddress::resolveHostName(a);
   if (!addr.isValidUnicast())
      return NULL;

   return new Tunnel(addr, port, config);
}

/**
 * Configured tunnels
 */
static ObjectArray<Tunnel> s_tunnels;

/**
 * Parser server connection (tunnel) list
 */
void ParseTunnelList(TCHAR *list)
{
   TCHAR *curr, *next;
   for(curr = next = list; curr != NULL && *curr != 0; curr = next + 1)
   {
      next = _tcschr(curr, _T('\n'));
      if (next != NULL)
         *next = 0;
      StrStrip(curr);

      Tunnel *t = Tunnel::createFromConfig(curr);
      if (t != NULL)
      {
         s_tunnels.add(t);
         nxlog_debug(1, _T("Added server tunnel %s@%s"), t->getLogin(), (const TCHAR *)t->getAddress().toString());
      }
      else
      {
         nxlog_write(MSG_INVALID_TUNNEL_CONFIG, NXLOG_ERROR, "s", curr);
      }
   }
   free(list);
}

/**
 * Tunnel manager
 */
THREAD_RESULT THREAD_CALL TunnelManager(void *)
{
   if (s_tunnels.size() == 0)
   {
      nxlog_debug(3, _T("No tunnels configured, tunnel manager will not start"));
      return THREAD_OK;
   }

   nxlog_debug(3, _T("Tunnel manager started"));
   while(!AgentSleepAndCheckForShutdown(30000))
   {
      for(int i = 0; i < s_tunnels.size(); i++)
      {
         Tunnel *t = s_tunnels.get(i);
         t->checkConnection();
      }
   }
   nxlog_debug(3, _T("Tunnel manager stopped"));
   return THREAD_OK;
}
