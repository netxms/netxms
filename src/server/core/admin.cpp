/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2019 Victor Kirhenshtein
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
** File: admin.cpp
**
**/

#include "nxcore.h"
#include <local_admin.h>

/**
 * Max message size
 */
#define MAX_MSG_SIZE       65536

/**
 * DB password set condition
 */
extern Condition g_dbPasswordReady;

/**
 * Request processing thread
 */
static THREAD_RESULT THREAD_CALL ProcessingThread(void *pArg)
{
   SOCKET sock = CAST_FROM_POINTER(pArg, SOCKET);

   SocketConsole console(sock);
   SocketMessageReceiver receiver(sock, 4096, MAX_MSG_SIZE);
   while(true)
   {
      MessageReceiverResult result;
      NXCPMessage *request = receiver.readMessage(INFINITE, &result);

      // Receive error
      if (request == NULL)
      {
         if (result == MSGRECV_CLOSED)
            nxlog_debug(5, _T("LocalServerConsole: connection closed"));
         else
            nxlog_debug(5, _T("LocalServerConsole: message receiving error (%s)"), AbstractMessageReceiver::resultToText(result));
         break;
      }

      if (IsShutdownInProgress())
      {
         delete request;
         break;
      }

      if (request->getCode() == CMD_ADM_REQUEST)
      {
         TCHAR command[256];
         request->getFieldAsString(VID_COMMAND, command, 256);

         int exitCode = ProcessConsoleCommand(command, &console);
         switch(exitCode)
         {
            case CMD_EXIT_SHUTDOWN:
               InitiateShutdown(ShutdownReason::FROM_REMOTE_CONSOLE);
               break;
            case CMD_EXIT_CLOSE_SESSION:
               delete request;
               goto close_session;
            default:
               break;
         }
      }
      else if (request->getCode() == CMD_SET_DB_PASSWORD)
      {
         request->getFieldAsString(VID_PASSWORD, g_szDbPassword, MAX_PASSWORD);
         DecryptPassword(g_szDbLogin, g_szDbPassword, g_szDbPassword, MAX_PASSWORD);
         g_dbPasswordReady.set();
      }

      NXCPMessage response(CMD_REQUEST_COMPLETED, request->getId());
      NXCP_MESSAGE *rawMsgOut = response.serialize();
		SendEx(sock, rawMsgOut, ntohl(rawMsgOut->size), 0, console.getMutex());
      MemFree(rawMsgOut);
      delete request;
   }

close_session:
   shutdown(sock, 2);
   closesocket(sock);
   return THREAD_OK;
}

/**
 * Local administrative interface listener thread
 */
THREAD_RESULT THREAD_CALL LocalAdminListener(void *pArg)
{
   ThreadSetName("DebugConsole");

   SOCKET sock, sockClient;
   struct sockaddr_in servAddr;
   int errorCount = 0;
   socklen_t iSize;

   // Create socket
   if ((sock = CreateSocket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
   {
      TCHAR buffer[1024];
      nxlog_write(NXLOG_ERROR, _T("Unable to create socket for local admin interface (%s)"), GetLastSocketErrorText(buffer, 1024));
      return THREAD_OK;
   }

	SetSocketExclusiveAddrUse(sock);
	SetSocketReuseFlag(sock);
#ifndef _WIN32
   fcntl(sock, F_SETFD, fcntl(sock, F_GETFD) | FD_CLOEXEC);
#endif

   // Fill in local address structure
   memset(&servAddr, 0, sizeof(struct sockaddr_in));
   servAddr.sin_family = AF_INET;
   servAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
   servAddr.sin_port = htons(LOCAL_ADMIN_PORT);

   // Bind socket
   if (bind(sock, (struct sockaddr *)&servAddr, sizeof(struct sockaddr_in)) != 0)
   {
      TCHAR buffer[1024];
      nxlog_write(NXLOG_ERROR, _T("Unable to bind socket for local admin interface (%s)"), GetLastSocketErrorText(buffer, 1024));
      closesocket(sock);
      return THREAD_OK;
   }

   // Set up queue
   listen(sock, SOMAXCONN);

   nxlog_debug(1, _T("Local administration interface listener started"));

   // Wait for connection requests
   while(!IsShutdownInProgress())
   {
      SocketPoller sp;
      sp.add(sock);
      int pollError = sp.poll(2000);
      if (pollError <= 0)
      {
         if (pollError < 0)
         {
            if (SleepAndCheckForShutdown(30))
               break;
         }
         continue;
      }

      iSize = sizeof(struct sockaddr_in);
      if ((sockClient = accept(sock, (struct sockaddr *)&servAddr, &iSize)) == -1)
      {
         if (IsShutdownInProgress())
         {
            closesocket(sockClient);
            break;
         }
#ifdef _WIN32
         int error = WSAGetLastError();
         if (error != WSAEINTR)
#else
         int error = errno;
         if (error != EINTR)
#endif
         {
            TCHAR buffer[1024];
            nxlog_write(NXLOG_ERROR, _T("Unable to accept incoming connection (%s)"), GetLastSocketErrorText(buffer, 1024));
         }
         errorCount++;
         if (errorCount > 1000)
         {
            nxlog_write(NXLOG_WARNING, _T("Too many consecutive errors on accept() call"));
            errorCount = 0;
         }
         ThreadSleepMs(500);
         continue;
      }

      errorCount = 0;     // Reset consecutive errors counter

      // Create new session structure and threads
      ThreadCreate(ProcessingThread, 0, CAST_TO_POINTER(sockClient, void *));
   }

   closesocket(sock);
   nxlog_debug(1, _T("Local administration interface listener stopped"));
   return THREAD_OK;
}
