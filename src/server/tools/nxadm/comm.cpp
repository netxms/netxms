/* 
** NetXMS - Network Management System
** Local administration tool
** Copyright (C) 2003-2015 Victor Kirhenshtein
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
** $module: comm.cpp
**
**/

#include "nxadm.h"

/**
 * Max message size
 */
#define MAX_MSG_SIZE    4194304

/**
 * Global data
 */
SOCKET g_hSocket = -1;
DWORD g_dwRqId = 1;

/**
 * Message receiver
 */
static SocketMessageReceiver *s_receiver = NULL;

/**
 * Connect to server
 */
BOOL Connect()
{
   struct sockaddr_in sa;

   // Create socket
   g_hSocket = socket(AF_INET, SOCK_STREAM, 0);
   if (g_hSocket == INVALID_SOCKET)
   {
      printf("Error creating socket\n");
      return FALSE;
   }

   // Fill in address structure
   memset(&sa, 0, sizeof(sa));
   sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
   sa.sin_family = AF_INET;
   sa.sin_port = htons(LOCAL_ADMIN_PORT);

   // Connect to server
   if (connect(g_hSocket, (struct sockaddr *)&sa, sizeof(sa)) == -1)
   {
      printf("Cannot establish connection with server\n");
      closesocket(g_hSocket);
      g_hSocket = -1;
      return FALSE;
   }

   // Initialize receiver
   s_receiver = new SocketMessageReceiver(g_hSocket, 4096, MAX_MSG_SIZE);
   return TRUE;
}

/**
 * Disconnect from server
 */
void Disconnect()
{
   delete_and_null(s_receiver);
   if (g_hSocket != -1)
   {
      shutdown(g_hSocket, 2);
      closesocket(g_hSocket);
      g_hSocket = -1;
   }
}

/**
 * Send message to server
 */
void SendMsg(NXCPMessage *pMsg)
{
   NXCP_MESSAGE *pRawMsg;

   pRawMsg = pMsg->serialize();
   SendEx(g_hSocket, pRawMsg, ntohl(pRawMsg->size), 0, NULL);
   free(pRawMsg);
}

/**
 * Receive message
 */
NXCPMessage *RecvMsg()
{
   MessageReceiverResult result;
   return s_receiver->readMessage(INFINITE, &result);
}
