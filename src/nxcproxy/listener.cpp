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
** File: listener.cpp
**
**/

#include "nxcproxy.h"

/**
 * Listener thread
 */
THREAD_RESULT THREAD_CALL ListenerThread(void *)
{
   SOCKET hSocket, hClientSocket;
   struct sockaddr_in servAddr;
   int iNumErrors = 0, nRet;
   socklen_t iSize;
   TCHAR szBuffer[256];
   struct timeval tv;
   fd_set rdfs;

   // Create socket
   if ((hSocket = CreateSocket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
   {
      TCHAR buffer[1024];
      nxlog_write(NXLOG_ERROR, _T("Cannot create socket for listener (%s)"), GetLastSocketErrorText(buffer, 1024));
      exit(1);
   }

   SetSocketExclusiveAddrUse(hSocket);
   SetSocketReuseFlag(hSocket);

   // Fill in local address structure
   memset(&servAddr, 0, sizeof(struct sockaddr_in));
   servAddr.sin_family = AF_INET;
   if (!_tcscmp(g_listenAddress, _T("*")))
   {
      servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
   }
   else
   {
      servAddr.sin_addr.s_addr = InetAddress::resolveHostName(g_listenAddress, AF_INET).getAddressV4();
      if (servAddr.sin_addr.s_addr == htonl(INADDR_NONE))
         servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
   }
   servAddr.sin_port = htons(g_listenPort);

   // Bind socket
   nxlog_debug(1, _T("Trying to bind on %s:%d"), IpToStr(ntohl(servAddr.sin_addr.s_addr), szBuffer), ntohs(servAddr.sin_port));
   if (bind(hSocket, (struct sockaddr *)&servAddr, sizeof(struct sockaddr_in)) != 0)
   {
      TCHAR buffer[1024];
      nxlog_write(NXLOG_ERROR, _T("Cannot bind socket for listener (%s)"), GetLastSocketErrorText(buffer, 1024));
      exit(1);
   }

   // Set up queue
   listen(hSocket, SOMAXCONN);
   nxlog_write(NXLOG_INFO, _T("Listening on socket %s:%d"), (const TCHAR *)InetAddress(ntohl(servAddr.sin_addr.s_addr)).toString(), g_listenPort);

   // Wait for connection requests
   while(!(g_flags & AF_SHUTDOWN))
   {
      tv.tv_sec = 1;
      tv.tv_usec = 0;
      FD_ZERO(&rdfs);
      FD_SET(hSocket, &rdfs);
      nRet = select(SELECT_NFDS(hSocket + 1), &rdfs, NULL, NULL, &tv);
      if ((nRet > 0) && (!(g_flags & AF_SHUTDOWN)))
      {
         iSize = sizeof(struct sockaddr_in);
         if ((hClientSocket = accept(hSocket, (struct sockaddr *)&servAddr, &iSize)) == -1)
         {
            int error = WSAGetLastError();

            if (error != WSAEINTR)
            {
               nxlog_debug(1, _T("accept() error %d"), error);
            }
            iNumErrors++;
            if (iNumErrors > 1000)
            {
               nxlog_debug(1, _T("Too many consecutive errors in accept() call"));
               iNumErrors = 0;
            }
            ThreadSleepMs(500);
            continue;
         }

         iNumErrors = 0;     // Reset consecutive errors counter
         nxlog_debug(5, _T("Incoming connection from %s"), IpToStr(ntohl(servAddr.sin_addr.s_addr), szBuffer));

         SetSocketNonBlocking(hClientSocket);

         ProxySession *session = new ProxySession(hClientSocket);
         session->run();
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
            TCHAR buffer[1024];
            nxlog_debug(1, _T("select() error (%s)"), GetLastSocketErrorText(buffer, 1024));
            ThreadSleepMs(100);
         }
      }
   }

   closesocket(hSocket);
   nxlog_debug(1, _T("Listener thread terminated"));
   return THREAD_OK;
}
