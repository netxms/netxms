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
   static NXCPEncryptionContext *dummyCtx = NULL;

   SOCKET sock = CAST_FROM_POINTER(pArg, SOCKET);

   struct __console_ctx ctx;
   ctx.hSocket = sock;
	ctx.socketMutex = MutexCreate();
   ctx.pMsg = NULL;
	ctx.session = NULL;
   ctx.output = NULL;

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

      NXCPMessage response(CMD_ADM_MESSAGE, request->getId());
      if (request->getCode() == CMD_ADM_REQUEST)
      {
         TCHAR command[256];
         request->getFieldAsString(VID_COMMAND, command, 256);

         ctx.pMsg = &response;
         int exitCode = ProcessConsoleCommand(command, &ctx);
         switch(exitCode)
         {
            case CMD_EXIT_SHUTDOWN:
               InitiateShutdown();
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

      response.setCode(CMD_REQUEST_COMPLETED);
      NXCP_MESSAGE *rawMsgOut = response.serialize();
		SendEx(sock, rawMsgOut, ntohl(rawMsgOut->size), 0, ctx.socketMutex);
      free(rawMsgOut);
      delete request;
   }

close_session:
   shutdown(sock, 2);
   closesocket(sock);
	MutexDestroy(ctx.socketMutex);
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
   if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
   {
      nxlog_write(MSG_SOCKET_FAILED, EVENTLOG_ERROR_TYPE, "s", _T("LocalAdminListener"));
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
      nxlog_write(MSG_BIND_ERROR, EVENTLOG_ERROR_TYPE, "dse", LOCAL_ADMIN_PORT, _T("LocalAdminListener"), WSAGetLastError());
      closesocket(sock);
      /* TODO: we should initiate shutdown from here */
      return THREAD_OK;
   }

   // Set up queue
   listen(sock, SOMAXCONN);

   // Wait for connection requests
   while(!IsShutdownInProgress())
   {
      iSize = sizeof(struct sockaddr_in);
      if ((sockClient = accept(sock, (struct sockaddr *)&servAddr, &iSize)) == -1)
      {
         int error;

#ifdef _WIN32
         error = WSAGetLastError();
         if (error != WSAEINTR)
#else
         error = errno;
         if (error != EINTR)
#endif
            nxlog_write(MSG_ACCEPT_ERROR, EVENTLOG_ERROR_TYPE, "e", error);
         errorCount++;
         if (errorCount > 1000)
         {
            nxlog_write(MSG_TOO_MANY_ACCEPT_ERRORS, EVENTLOG_WARNING_TYPE, NULL);
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
   return THREAD_OK;
}
