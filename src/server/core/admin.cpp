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
// Send success or error notification
//

#define SEND_ERROR() \
   { \
      wCmd = LA_RESP_ERROR; \
      send(sock, (char *)&wCmd, sizeof(WORD), 0); \
   }
#define SEND_SUCCESS() \
   { \
      wCmd = LA_RESP_SUCCESS; \
      send(sock, (char *)&wCmd, sizeof(WORD), 0); \
   }



//
// Receive string from client
//

static BOOL RecvString(SOCKET sock, char *pBuffer, int iBufSize)
{
   int iError;
   WORD wSize = 0;

   // Receive string length
   iError = recv(sock, (char *)&wSize, 2, 0);
   if ((iError != 2) || (wSize > iBufSize - 1))
      return FALSE;

   iError = recv(sock, pBuffer, wSize, 0);
   if (iError != wSize)
      return FALSE;
   pBuffer[iError] = 0;
   return TRUE;
}


//
// Send string to client
//

static BOOL SendString(SOCKET sock, char *szString)
{
   WORD wLen;

   wLen = strlen(szString);
   if (send(sock, (char *)&wLen, sizeof(WORD), 0) != 2)
      return FALSE;

   return send(sock, szString, wLen, 0) == wLen;
}


//
// Request processing thread
//

static THREAD_RESULT THREAD_CALL ProcessingThread(void *pArg)
{
   SOCKET sock = (SOCKET)pArg;
   WORD wCmd;
   int iError;
   char szBuffer[256];
   DWORD dwTemp;

   while(1)
   {
      iError = recv(sock, (char *)&wCmd, sizeof(WORD), 0);
      if (iError != 2)
         break;   // Communication error or closed connection

      switch(wCmd)
      {
         case LA_CMD_LIST_CONFIG:
            SEND_ERROR();
            break;
         case LA_CMD_GET_CONFIG:
            // Receive variable name
            if (RecvString(sock, szBuffer, 256))
            {
               char szValue[256];

               if (ConfigReadStr(szBuffer, szValue, 255, ""))
                  SendString(sock, szValue);
               else
                  SEND_ERROR();
            }
            else
            {
               goto close_connection;
            }
            break;
         case LA_CMD_SET_CONFIG:
            // Receive variable name
            if (RecvString(sock, szBuffer, 256))
            {
               char szValue[256];

               // Receive new value
               if (RecvString(sock, szValue, 256))
               {
                  if (ConfigWriteStr(szBuffer, szValue, TRUE))
                  {
                     SEND_SUCCESS();
                  }
                  else
                  {
                     SEND_ERROR();
                  }
               }
               else
               {
                  goto close_connection;
               }
            }
            else
            {
               goto close_connection;
            }
            break;
         case LA_CMD_GET_FLAGS:
            // Send value of application flags
            send(sock, (char *)&g_dwFlags, sizeof(DWORD), 0);
            break;
         case LA_CMD_SET_FLAGS:
            iError = recv(sock, (char *)&dwTemp, sizeof(DWORD), 0);
            if (iError == sizeof(DWORD))
            {
               dwTemp &= ~AF_STANDALONE;    // Standalone flag shouldn't be changed
               g_dwFlags = dwTemp | (g_dwFlags & AF_STANDALONE);
               SEND_SUCCESS();
            }
            else
            {
               goto close_connection;
            }
            break;
         default:
            break;
      }
   }

close_connection:
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
