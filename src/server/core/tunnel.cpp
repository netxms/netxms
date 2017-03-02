/*
** NetXMS - Network Management System
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
** File: tunnel.cpp
**/

#include "nxcore.h"
#include <agent_tunnel.h>

#define MAX_MSG_SIZE    268435456

/**
 * Next free tunnel ID
 */
static VolatileCounter s_nextTunnelId = 0;

/**
 * Agent tunnel constructor
 */
AgentTunnel::AgentTunnel(SSL_CTX *context, SSL *ssl, SOCKET sock, const InetAddress& addr, UINT32 nodeId) : RefCountObject()
{
   m_id = InterlockedIncrement(&s_nextTunnelId);
   m_address = addr;
   m_socket = sock;
   m_context = context;
   m_ssl = ssl;
   m_sslLock = MutexCreate();
   m_recvThread = INVALID_THREAD_HANDLE;
   m_nodeId = nodeId;
   m_state = AGENT_TUNNEL_INIT;
   m_systemName = NULL;
   m_platformName = NULL;
   m_systemInfo = NULL;
   m_agentVersion = NULL;
}

/**
 * Agent tunnel destructor
 */
AgentTunnel::~AgentTunnel()
{
   ThreadJoin(m_recvThread);
   SSL_CTX_free(m_context);
   SSL_free(m_ssl);
   MutexDestroy(m_sslLock);
   closesocket(m_socket);
   free(m_systemName);
   free(m_platformName);
   free(m_systemInfo);
   free(m_agentVersion);
   debugPrintf(4, _T("Tunnel destroyed"));
}

/**
 * Debug output
 */
void AgentTunnel::debugPrintf(int level, const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   TCHAR buffer[8192];
   _vsntprintf(buffer, 8192, format, args);
   va_end(args);
   nxlog_debug(level, _T("[TUN-%d] %s"), m_id, buffer);
}

/**
 * Tunnel receiver thread
 */
void AgentTunnel::recvThread()
{
   TlsMessageReceiver receiver(m_socket, m_ssl, m_sslLock, 4096, MAX_MSG_SIZE);
   while(true)
   {
      MessageReceiverResult result;
      NXCPMessage *msg = receiver.readMessage(60000, &result);
      if (result != MSGRECV_SUCCESS)
      {
         if (result == MSGRECV_CLOSED)
            debugPrintf(4, _T("Tunnel closed by peer"));
         else
            debugPrintf(4, _T("Communication error (%s)"), AbstractMessageReceiver::resultToText(result));
         break;
      }

      if (nxlog_get_debug_level() >= 6)
      {
         TCHAR buffer[64];
         debugPrintf(6, _T("Received message %s"), NXCPMessageCodeName(msg->getCode(), buffer));
      }

      switch(msg->getCode())
      {
         case CMD_KEEPALIVE:
            break;
         case CMD_SETUP_AGENT_TUNNEL:
            setup(msg);
            break;
      }
   }
   debugPrintf(5, _T("Receiver thread stopped"));
}

/**
 * Tunnel receiver thread starter
 */
THREAD_RESULT THREAD_CALL AgentTunnel::recvThreadStarter(void *arg)
{
   ((AgentTunnel *)arg)->recvThread();
   return THREAD_OK;
}

/**
 * Send message on tunnel
 */
bool AgentTunnel::sendMessage(NXCPMessage *msg)
{
   NXCP_MESSAGE *data = msg->createMessage(true);
   MutexLock(m_sslLock);
   bool success = (SSL_write(m_ssl, data, ntohl(data->size)) == ntohl(data->size));
   MutexUnlock(m_sslLock);
   free(data);
   return success;
}

/**
 * Start tunel
 */
void AgentTunnel::start()
{
   incRefCount();
   debugPrintf(4, _T("Tunnel started"));
   m_recvThread = ThreadCreateEx(AgentTunnel::recvThreadStarter, 0, this);
}

/**
 * Process setup request
 */
void AgentTunnel::setup(const NXCPMessage *request)
{
   NXCPMessage response;
   response.setCode(CMD_REQUEST_COMPLETED);
   response.setId(request->getId());

   if (m_state == AGENT_TUNNEL_INIT)
   {
      m_systemName = request->getFieldAsString(VID_SYS_NAME);
      m_systemInfo = request->getFieldAsString(VID_SYS_DESCRIPTION);
      m_platformName = request->getFieldAsString(VID_PLATFORM_NAME);
      m_agentVersion = request->getFieldAsString(VID_AGENT_VERSION);

      m_state = (m_nodeId != 0) ? AGENT_TUNNEL_BOUND : AGENT_TUNNEL_UNBOUND;
      response.setField(VID_RCC, ERR_SUCCESS);
      response.setField(VID_IS_ACTIVE, m_state == AGENT_TUNNEL_BOUND);
   }
   else
   {
      response.setField(VID_RCC, ERR_OUT_OF_STATE_REQUEST);
   }

   sendMessage(&response);
}

/**
 * Incoming connection data
 */
struct ConnectionRequest
{
   SOCKET sock;
   InetAddress addr;
};

/**
 * Setup tunnel
 */
static void SetupTunnel(void *arg)
{
   ConnectionRequest *request = (ConnectionRequest *)arg;

   SSL_CTX *context = NULL;
   SSL *ssl = NULL;
   AgentTunnel *tunnel = NULL;
   int rc;
   UINT32 nodeId = 0;
   X509 *cert = NULL;

   // Setup secure connection
   const SSL_METHOD *method = SSLv23_method();
   if (method == NULL)
   {
      nxlog_debug(4, _T("SetupTunnel(%s): cannot obtain TLS method"), (const TCHAR *)request->addr.toString());
      goto failure;
   }

   context = SSL_CTX_new(method);
   if (context == NULL)
   {
      nxlog_debug(4, _T("SetupTunnel(%s): cannot create TLS context"), (const TCHAR *)request->addr.toString());
      goto failure;
   }
   SSL_CTX_set_options(context, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION);
   if (!SetupServerTlsContext(context))
   {
      nxlog_debug(4, _T("SetupTunnel(%s): cannot configure TLS context"), (const TCHAR *)request->addr.toString());
      goto failure;
   }

   ssl = SSL_new(context);
   if (ssl == NULL)
   {
      nxlog_debug(4, _T("SetupTunnel(%s): cannot create SSL object"), (const TCHAR *)request->addr.toString());
      goto failure;
   }

   SSL_set_accept_state(ssl);
   SSL_set_fd(ssl, request->sock);

retry:
   rc = SSL_do_handshake(ssl);
   if (rc != 1)
   {
      int sslErr = SSL_get_error(ssl, rc);
      if (sslErr == SSL_ERROR_WANT_READ)
      {
         SocketPoller poller;
         poller.add(request->sock);
         if (poller.poll(5000) > 0)
            goto retry;
         nxlog_debug(4, _T("SetupTunnel(%s): TLS handshake failed (timeout)"), (const TCHAR *)request->addr.toString());
      }
      else
      {
         char buffer[128];
         nxlog_debug(4, _T("SetupTunnel(%s): TLS handshake failed (%hs)"),
                     (const TCHAR *)request->addr.toString(), ERR_error_string(sslErr, buffer));
      }
      goto failure;
   }

   cert = SSL_get_peer_certificate(ssl);
   if (cert != NULL)
   {
      if (ValidateAgentCertificate(cert))
      {
         TCHAR cn[256];
         if (GetCertificateCN(cert, cn, 256))
         {
            uuid guid = uuid::parse(cn);
            if (!guid.isNull())
            {
               Node *node = (Node *)FindObjectByGUID(guid, OBJECT_NODE);
               if (node != NULL)
               {
                  nxlog_debug(4, _T("SetupTunnel(%s): Tunnel attached to node %s [%d]"), (const TCHAR *)request->addr.toString(), node->getName(), node->getId());
                  nodeId = node->getId();
               }
               else
               {
                  nxlog_debug(4, _T("SetupTunnel(%s): Node with GUID %s not found"), (const TCHAR *)request->addr.toString(), (const TCHAR *)guid.toString());
               }
            }
            else
            {
               nxlog_debug(4, _T("SetupTunnel(%s): Certificate CN is not a valid GUID"), (const TCHAR *)request->addr.toString());
            }
         }
         else
         {
            nxlog_debug(4, _T("SetupTunnel(%s): Cannot get certificate CN"), (const TCHAR *)request->addr.toString());
         }
      }
      else
      {
         nxlog_debug(4, _T("SetupTunnel(%s): Agent certificate validation failed"), (const TCHAR *)request->addr.toString());
      }
      X509_free(cert);
   }
   else
   {
      nxlog_debug(4, _T("SetupTunnel(%s): Agent certificate not provided"), (const TCHAR *)request->addr.toString());
   }

   tunnel = new AgentTunnel(context, ssl, request->sock, request->addr, nodeId);
   tunnel->start();
   tunnel->decRefCount();

   delete request;
   return;

failure:
   if (ssl != NULL)
      SSL_free(ssl);
   if (context != NULL)
      SSL_CTX_free(context);
   shutdown(request->sock, SHUT_RDWR);
   closesocket(request->sock);
   delete request;
}

/**
 * Tunnel listener
 */
THREAD_RESULT THREAD_CALL TunnelListener(void *arg)
{
   UINT16 port = (UINT16)ConfigReadULong(_T("AgentTunnelListenPort"), 4703);

   // Create socket(s)
   SOCKET hSocket = socket(AF_INET, SOCK_STREAM, 0);
#ifdef WITH_IPV6
   SOCKET hSocket6 = socket(AF_INET6, SOCK_STREAM, 0);
#endif
   if ((hSocket == INVALID_SOCKET)
#ifdef WITH_IPV6
       && (hSocket6 == INVALID_SOCKET)
#endif
      )
   {
      nxlog_write(MSG_SOCKET_FAILED, EVENTLOG_ERROR_TYPE, "s", _T("TunnelListener"));
      return THREAD_OK;
   }

   SetSocketExclusiveAddrUse(hSocket);
   SetSocketReuseFlag(hSocket);
#ifndef _WIN32
   fcntl(hSocket, F_SETFD, fcntl(hSocket, F_GETFD) | FD_CLOEXEC);
#endif

#ifdef WITH_IPV6
   SetSocketExclusiveAddrUse(hSocket6);
   SetSocketReuseFlag(hSocket6);
#ifndef _WIN32
   fcntl(hSocket6, F_SETFD, fcntl(hSocket6, F_GETFD) | FD_CLOEXEC);
#endif
#ifdef IPV6_V6ONLY
   int on = 1;
   setsockopt(hSocket6, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&on, sizeof(int));
#endif
#endif

   // Fill in local address structure
   struct sockaddr_in servAddr;
   memset(&servAddr, 0, sizeof(struct sockaddr_in));
   servAddr.sin_family = AF_INET;

#ifdef WITH_IPV6
   struct sockaddr_in6 servAddr6;
   memset(&servAddr6, 0, sizeof(struct sockaddr_in6));
   servAddr6.sin6_family = AF_INET6;
#endif

   if (!_tcscmp(g_szListenAddress, _T("*")))
   {
      servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
#ifdef WITH_IPV6
      memset(servAddr6.sin6_addr.s6_addr, 0, 16);
#endif
   }
   else
   {
      InetAddress bindAddress = InetAddress::resolveHostName(g_szListenAddress, AF_INET);
      if (bindAddress.isValid() && (bindAddress.getFamily() == AF_INET))
      {
         servAddr.sin_addr.s_addr = htonl(bindAddress.getAddressV4());
      }
      else
      {
         servAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
      }
#ifdef WITH_IPV6
      bindAddress = InetAddress::resolveHostName(g_szListenAddress, AF_INET6);
      if (bindAddress.isValid() && (bindAddress.getFamily() == AF_INET6))
      {
         memcpy(servAddr6.sin6_addr.s6_addr, bindAddress.getAddressV6(), 16);
      }
      else
      {
         memset(servAddr6.sin6_addr.s6_addr, 0, 15);
         servAddr6.sin6_addr.s6_addr[15] = 1;
      }
#endif
   }
   servAddr.sin_port = htons(port);
#ifdef WITH_IPV6
   servAddr6.sin6_port = htons(port);
#endif

   // Bind socket
   TCHAR buffer[64];
   int bindFailures = 0;
   nxlog_debug(1, _T("Trying to bind on %s:%d"), SockaddrToStr((struct sockaddr *)&servAddr, buffer), ntohs(servAddr.sin_port));
   if (bind(hSocket, (struct sockaddr *)&servAddr, sizeof(struct sockaddr_in)) != 0)
   {
      nxlog_write(MSG_BIND_ERROR, EVENTLOG_ERROR_TYPE, "e", WSAGetLastError());
      bindFailures++;
   }

#ifdef WITH_IPV6
   nxlog_debug(1, _T("Trying to bind on [%s]:%d"), SockaddrToStr((struct sockaddr *)&servAddr6, buffer), ntohs(servAddr6.sin6_port));
   if (bind(hSocket6, (struct sockaddr *)&servAddr6, sizeof(struct sockaddr_in6)) != 0)
   {
      nxlog_write(MSG_BIND_ERROR, EVENTLOG_ERROR_TYPE, "dse", port, _T("TunnelListener"), WSAGetLastError());
      bindFailures++;
   }
#else
   bindFailures++;
#endif

   // Abort if cannot bind to socket
   if (bindFailures == 2)
   {
      return THREAD_OK;
   }

   // Set up queue
   if (listen(hSocket, SOMAXCONN) == 0)
   {
      nxlog_write(MSG_LISTENING_FOR_AGENTS, NXLOG_INFO, "ad", ntohl(servAddr.sin_addr.s_addr), port);
   }
   else
   {
      closesocket(hSocket);
      hSocket = INVALID_SOCKET;
   }
#ifdef WITH_IPV6
   if (listen(hSocket6, SOMAXCONN) == 0)
   {
      nxlog_write(MSG_LISTENING_FOR_AGENTS, NXLOG_INFO, "Hd", servAddr6.sin6_addr.s6_addr, port);
   }
   else
   {
      closesocket(hSocket6);
      hSocket6 = INVALID_SOCKET;
   }
#endif

   // Wait for connection requests
   SocketPoller sp;
   int errorCount = 0;
   while(!(g_flags & AF_SHUTDOWN))
   {
      sp.reset();
      if (hSocket != INVALID_SOCKET)
         sp.add(hSocket);
#ifdef WITH_IPV6
      if (hSocket6 != INVALID_SOCKET)
         sp.add(hSocket6);
#endif

      int nRet = sp.poll(1000);
      if ((nRet > 0) && (!(g_flags & AF_SHUTDOWN)))
      {
         char clientAddr[128];
         socklen_t size = 128;
#ifdef WITH_IPV6
         SOCKET hClientSocket = accept(sp.isSet(hSocket) ? hSocket : hSocket6, (struct sockaddr *)clientAddr, &size);
#else
         SOCKET hClientSocket = accept(hSocket, (struct sockaddr *)clientAddr, &size);
#endif
         if (hClientSocket == INVALID_SOCKET)
         {
            int error = WSAGetLastError();

            if (error != WSAEINTR)
               nxlog_write(MSG_ACCEPT_ERROR, NXLOG_ERROR, "e", error);
            errorCount++;
            if (errorCount > 1000)
            {
               nxlog_write(MSG_TOO_MANY_ACCEPT_ERRORS, NXLOG_WARNING, NULL);
               errorCount = 0;
            }
            ThreadSleepMs(500);
            continue;
         }

         // Socket should be closed on successful exec
#ifndef _WIN32
         fcntl(hClientSocket, F_SETFD, fcntl(hClientSocket, F_GETFD) | FD_CLOEXEC);
#endif

         errorCount = 0;     // Reset consecutive errors counter
         InetAddress addr = InetAddress::createFromSockaddr((struct sockaddr *)clientAddr);
         nxlog_debug(5, _T("TunnelListener: incoming connection from %s"), addr.toString(buffer));

         ConnectionRequest *request = new ConnectionRequest();
         request->sock = hClientSocket;
         request->addr = addr;
         ThreadPoolExecute(g_mainThreadPool, SetupTunnel, request);
      }
      else if (nRet == -1)
      {
         int error = WSAGetLastError();

         // On AIX, select() returns ENOENT after SIGINT for unknown reason
#ifdef _WIN32
         if (error != WSAEINTR)
#else
         if ((error != EINTR) && (error != ENOENT))
#endif
         {
            ThreadSleepMs(100);
         }
      }
   }

   closesocket(hSocket);
#ifdef WITH_IPV6
   closesocket(hSocket6);
#endif
   nxlog_debug(1, _T("Tunnel listener thread terminated"));
   return THREAD_OK;
}

/**
 * Close all active agent tunnels
 */
void CloseAgentTunnels()
{
}
