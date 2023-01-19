/* 
** NetXMS - Network Management System
** Local administration tool
** Copyright (C) 2003-2023 Victor Kirhenshtein
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
SOCKET g_socket = INVALID_SOCKET;
uint32_t g_requestId = 1;

/**
 * Message receiver
 */
static SocketMessageReceiver *s_receiver = nullptr;

/**
 * Connect to server
 */
bool Connect()
{
   struct sockaddr_in sa;

   // Create socket
   g_socket = CreateSocket(AF_INET, SOCK_STREAM, 0);
   if (g_socket == INVALID_SOCKET)
   {
      printf("Error creating socket\n");
      return false;
   }

   // Fill in address structure
   memset(&sa, 0, sizeof(sa));
   sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
   sa.sin_family = AF_INET;
   sa.sin_port = htons(LOCAL_ADMIN_PORT);

   // Connect to server
   if (connect(g_socket, (struct sockaddr *)&sa, sizeof(sa)) == -1)
   {
      printf("Cannot establish connection with server\n");
      closesocket(g_socket);
      g_socket = -1;
      return false;
   }

   // Initialize receiver
   s_receiver = new SocketMessageReceiver(g_socket, 4096, MAX_MSG_SIZE);
   return true;
}

/**
 * Disconnect from server
 */
void Disconnect()
{
   delete_and_null(s_receiver);
   if (g_socket != -1)
   {
      shutdown(g_socket, 2);
      closesocket(g_socket);
      g_socket = -1;
   }
}

/**
 * Send message to server
 */
void SendMsg(const NXCPMessage& msg)
{
   NXCP_MESSAGE *rawMsg = msg.serialize();
   SendEx(g_socket, rawMsg, ntohl(rawMsg->size), 0, nullptr);
   MemFree(rawMsg);
}

/**
 * Receive message
 */
NXCPMessage *RecvMsg()
{
   MessageReceiverResult result;
   return s_receiver->readMessage(INFINITE, &result);
}
