/* 
** NetXMS client proxy
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
**
**/

#include "nxcproxy.h"

/**
 * Constructor
 */
ProxySession::ProxySession(SOCKET s)
{
   m_client = s;
   m_server = INVALID_SOCKET;
}

/**
 * Destructor
 */
ProxySession::~ProxySession()
{
}

/**
 * Run session
 */
void ProxySession::run()
{
   ThreadCreate(ProxySession::clientThreadStarter, 0, this);
}

/**
 * Client thread starter
 */
THREAD_RESULT THREAD_CALL ProxySession::clientThreadStarter(void *arg)
{
   ((ProxySession *)arg)->clientThread();
   // server thread is not running at this point, so it's safe to delete self
   delete (ProxySession *)arg;
   return THREAD_OK;
}

/**
 * Server thread starter
 */
THREAD_RESULT THREAD_CALL ProxySession::serverThreadStarter(void *arg)
{
   ((ProxySession *)arg)->serverThread();
   return THREAD_OK;
}

/**
 * Client thread
 */
void ProxySession::clientThread()
{
   DebugPrintf(7, _T("Client thread started, connecting to server"));
   m_server = socket(AF_INET, SOCK_STREAM, 0);
   if (m_server != INVALID_SOCKET)
   {
      // Fill in address structure
      struct sockaddr_in sa;
      memset(&sa, 0, sizeof(sa));
      sa.sin_addr.s_addr = ResolveHostName(g_serverAddress);
      if ((sa.sin_addr.s_addr != INADDR_NONE) && (sa.sin_addr.s_addr != INADDR_ANY))
      {
         sa.sin_family = AF_INET;
         sa.sin_port = htons(g_serverPort);
         if (connect(m_server, (struct sockaddr *)&sa, sizeof(sa)) != -1)
         {
            DebugPrintf(7, _T("Connected to server %s:%d"), g_serverAddress, (int)g_serverPort);
            THREAD serverThread = ThreadCreateEx(ProxySession::serverThreadStarter, 0, this);
            forward(m_client, m_server);
            ThreadJoin(serverThread);
         }
         closesocket(m_server);
      }
      else
      {
         DebugPrintf(4, _T("Cannot resolve server address %s"), g_serverAddress);
      }
   }

   shutdown(m_client, SHUT_RDWR);
   closesocket(m_client);
   DebugPrintf(7, _T("Client thread stopped"));
}

/**
 * Server thread
 */
void ProxySession::serverThread()
{
   DebugPrintf(7, _T("Server thread started"));
   SetSocketNonBlocking(m_server);
   forward(m_server, m_client);
   DebugPrintf(7, _T("Server thread stopped"));
}

/**
 * Forward messages between two sockets
 */
void ProxySession::forward(SOCKET rs, SOCKET ws)
{
   fd_set rdfs;
   struct timeval tv;
   char buffer[8192];

   while(true)
   {
      FD_ZERO(&rdfs);
      FD_SET(rs, &rdfs);
      tv.tv_sec = 0;
      tv.tv_usec = 5000000;   // Half-second timeout
      int ret = select(SELECT_NFDS(rs + 1), &rdfs, NULL, NULL, &tv);
      if (ret < 0)
         break;
      if (ret > 0)
      {
         ret = recv(rs, buffer, 8192, 0);
         if (ret <= 0)
            break;
         SendEx(ws, buffer, ret, 0, NULL);
      }
   }

   // shutdown other end
   shutdown(ws, SHUT_RDWR);
}
