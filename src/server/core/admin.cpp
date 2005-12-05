/* $Id: admin.cpp,v 1.14 2005-12-05 20:28:47 victor Exp $ */

/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: admin.cpp
**
**/

#include "nxcore.h"
#include <local_admin.h>


//
// Constants
//

#define MAX_MSG_SIZE       32768


//
// Request processing thread
//

static THREAD_RESULT THREAD_CALL ProcessingThread(void *pArg)
{
   SOCKET sock = (SOCKET)pArg;
   int iError;
   CSCP_MESSAGE *pRawMsg, *pRawMsgOut;
   CSCP_BUFFER *pRecvBuffer;
   CSCPMessage *pRequest, response;
   TCHAR szCmd[256];
   struct __console_ctx ctx;
   static CSCP_ENCRYPTION_CONTEXT *pDummyCtx = NULL;

   pRawMsg = (CSCP_MESSAGE *)malloc(MAX_MSG_SIZE);
   pRecvBuffer = (CSCP_BUFFER *)malloc(sizeof(CSCP_BUFFER));
   RecvCSCPMessage(0, NULL, pRecvBuffer, 0, NULL, NULL, 0);
   ctx.hSocket = sock;
   ctx.pMsg = &response;

   while(1)
   {
      iError = RecvCSCPMessage(sock, pRawMsg, pRecvBuffer, MAX_MSG_SIZE, &pDummyCtx, NULL, INFINITE);
      if (iError <= 0)
         break;   // Communication error or closed connection

      if (iError == 1)
         continue;   // Too big message

      pRequest = new CSCPMessage(pRawMsg);
      pRequest->GetVariableStr(VID_COMMAND, szCmd, 256);

      response.SetCode(CMD_ADM_MESSAGE);
      response.SetId(pRequest->GetId());
      if (ProcessConsoleCommand(szCmd, &ctx))
      {
         InitiateShutdown();
      }

      response.SetCode(CMD_REQUEST_COMPLETED);
      pRawMsgOut = response.CreateMessage();
      SendEx(sock, pRawMsgOut, ntohl(pRawMsgOut->dwSize), 0);
      
      free(pRawMsgOut);
      delete pRequest;
   }

   shutdown(sock, 2);
   closesocket(sock);
   return THREAD_OK;
}


//
// Local administrative interface listener thread
//

THREAD_RESULT THREAD_CALL LocalAdminListener(void *pArg)
{
   SOCKET sock, sockClient;
   struct sockaddr_in servAddr;
   int errorCount = 0;
   socklen_t iSize;

   // Create socket
   if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
   {
      WriteLog(MSG_SOCKET_FAILED, EVENTLOG_ERROR_TYPE, "s", "LocalAdminListener");
      return THREAD_OK;
   }

	SetSocketReuseFlag(sock);

   // Fill in local address structure
   memset(&servAddr, 0, sizeof(struct sockaddr_in));
   servAddr.sin_family = AF_INET;
   servAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
   servAddr.sin_port = htons(LOCAL_ADMIN_PORT);

   // Bind socket
   if (bind(sock, (struct sockaddr *)&servAddr, sizeof(struct sockaddr_in)) != 0)
   {
      WriteLog(MSG_BIND_ERROR, EVENTLOG_ERROR_TYPE, "dse", LOCAL_ADMIN_PORT, "LocalAdminListener", WSAGetLastError());
      closesocket(sock);
      /* TODO: we should initiate shutdown from here */
      return THREAD_OK;
   }

   // Set up queue
   listen(sock, SOMAXCONN);

   // Wait for connection requests
   while(!ShutdownInProgress())
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
            WriteLog(MSG_ACCEPT_ERROR, EVENTLOG_ERROR_TYPE, "e", error);
         errorCount++;
         if (errorCount > 1000)
         {
            WriteLog(MSG_TOO_MANY_ACCEPT_ERRORS, EVENTLOG_WARNING_TYPE, NULL);
            errorCount = 0;
         }
         ThreadSleepMs(500);
      }

      errorCount = 0;     // Reset consecutive errors counter

      // Create new session structure and threads
      ThreadCreate(ProcessingThread, 0, (void *)sockClient);
   }

   closesocket(sock);
   return THREAD_OK;
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.13  2005/12/03 22:53:04  victor
- Added function RecvEx
- Added timeout parameter to RecvCSCPMessage
- Other minor changes

Revision 1.12  2005/08/17 12:09:25  victor
responce changed to response (issue #37)

Revision 1.11  2005/06/19 21:39:20  victor
Encryption between server and agent fully working

Revision 1.10  2005/06/19 19:20:40  victor
- Added encryption foundation
- Encryption between server and agent almost working

Revision 1.9  2005/04/07 15:50:58  victor
- Implemented save and restore for DCI graph windows
- More commands added to server console

Revision 1.8  2005/04/06 16:16:25  victor
Local administrator interface completely rewritten

Revision 1.7  2005/02/02 22:32:16  alk
condTimedWait fixed
file transfers fixed
agent upgrade script fixed

Revision 1.6  2005/01/18 15:51:42  alk
+ sockets reuse (*nix only)


*/
