/* 
** NetXMS - Network Management System
** Local administration tool
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
** $module: comm.cpp
**
**/

#include "nxadm.h"


//
// Global data
//

SOCKET g_hSocket = -1;


//
// Connect to server
//

BOOL Connect(void)
{
   struct sockaddr_in sa;

   // Create socket
   g_hSocket = socket(AF_INET, SOCK_STREAM, 0);
   if (g_hSocket == -1)
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

   return TRUE;
}


//
// Disconnect from server
//

void Disconnect(void)
{
   if (g_hSocket != -1)
   {
      shutdown(g_hSocket, 2);
      closesocket(g_hSocket);
      g_hSocket = -1;
   }
}


//
// Send command to server
//

BOOL SendCommand(WORD wCmd)
{
   return SendEx(g_hSocket, (char *)&wCmd, sizeof(WORD), 0) == sizeof(WORD);
}


//
// Receive responce from server
//

WORD RecvResponce(void)
{
   WORD wResp;

   return (recv(g_hSocket, (char *)&wResp, sizeof(WORD), 0) == sizeof(WORD)) ? wResp : LA_RESP_ERROR;
}


//
// Send string to server
//

BOOL SendString(char *szString)
{
   WORD wLen;

   wLen = strlen(szString);
   if (SendEx(g_hSocket, (char *)&wLen, sizeof(WORD), 0) != 2)
      return FALSE;

   return SendEx(g_hSocket, szString, wLen, 0) == wLen;
}


//
// Receive string from client
//

int RecvString(char *pBuffer, int iBufSize)
{
   int iError;
   WORD wSize = 0;

   // Receive string length
   iError = recv(g_hSocket, (char *)&wSize, 2, 0);

   if (wSize == 0xFFFF)
      return 1;   // Server responds with an error instead of string

   if ((iError != 2) || (wSize > iBufSize - 1))
      return -1;  // Comm error

   iError = recv(g_hSocket, pBuffer, wSize, 0);
   if (iError != wSize)
      return -1;
   pBuffer[iError] = 0;
   return 0;
}


//
// Receive double word from server
//

BOOL RecvDWord(DWORD *pBuffer)
{
   return recv(g_hSocket, (char *)pBuffer, sizeof(DWORD), 0) == sizeof(DWORD);
}


//
// Send double word to server
//

BOOL SendDWord(DWORD dwValue)
{
   return SendEx(g_hSocket, (char *)&dwValue, sizeof(DWORD), 0) == sizeof(DWORD);
}
