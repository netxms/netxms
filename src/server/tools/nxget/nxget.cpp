/* 
** nxget - command line tool used tp retrieve parameters from NetXMS agent
** Copyright (C) 2004 Victor Kirhenshtein
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
** $module: nxget.cpp
**
**/

#include <nms_common.h>
#include <nms_agent.h>
#include <nxclapi.h>


//
// Constants
//

#define MAX_LINE_SIZE      4096


//
// Static data
//

static DWORD m_dwAddr;
static int m_iAuthMethod = AUTH_NONE;
static char m_szSecret[MAX_SECRET_LENGTH] = "";
static SOCKET m_hSocket;
static WORD m_wPort = AGENT_LISTEN_PORT;
static DWORD m_dwNumDataLines = 0;
static char **m_ppDataLines = NULL;
static char m_szNetBuffer[MAX_LINE_SIZE * 2];


//
// Destroy command execuion results data
//

void DestroyResultData(void)
{
   DWORD i;

   if (m_ppDataLines != NULL)
   {
      for(i = 0; i < m_dwNumDataLines; i++)
         if (m_ppDataLines[i] != NULL)
            free(m_ppDataLines[i]);
      free(m_ppDataLines);
      m_ppDataLines = NULL;
   }
   m_dwNumDataLines = 0;
}


//
// Add new line to result data
//

void AddDataLine(char *szLine)
{
   m_dwNumDataLines++;
   m_ppDataLines = (char **)realloc(m_ppDataLines, sizeof(char *) * m_dwNumDataLines);
   m_ppDataLines[m_dwNumDataLines - 1] = strdup(szLine);
}


//
// Receive line from socket
//

int RecvLine(int iBufSize, char *szBuffer)
{
   char *pChar;
	struct timeval timeout;
	FD_SET rdfs;
   int iSize;

   pChar = strchr(m_szNetBuffer, '\r');
   if (pChar != NULL)    // Newline found in already received chunk
   {
      *pChar = 0;
      pChar++;
      if (*pChar == '\n')
         pChar++;
      strncpy(szBuffer, m_szNetBuffer, iBufSize);
      memmove(m_szNetBuffer, pChar, strlen(pChar) + 1);
      return strlen(szBuffer);
   }

   strncpy(szBuffer, m_szNetBuffer, iBufSize);
   szBuffer[iBufSize - 1] = 0;

   while(1)
   {
      // Wait for data
      FD_ZERO(&rdfs);
      FD_SET(m_hSocket, &rdfs);
      timeout.tv_sec = 3;
      timeout.tv_usec = 0;
      if (select(m_hSocket + 1, &rdfs, NULL, NULL, &timeout) == 0)
      {
         return -65535;
      }

      // Receive data
      iSize = recv(m_hSocket, m_szNetBuffer, 1023, 0);
      if (iSize <= 0)
         return iSize;
      m_szNetBuffer[iSize] = 0;

      pChar = strchr(m_szNetBuffer, '\r');
      if (pChar != NULL)    // Newline found in this chunk
      {
         *pChar = 0;
         pChar++;
         if (*pChar == '\n')
            pChar++;
         if ((int)strlen(szBuffer) + (int)strlen(m_szNetBuffer) >= (iBufSize - 1))
            break;      // Buffer is too small, stop receiving
         strcat(szBuffer, m_szNetBuffer);
         memmove(m_szNetBuffer, pChar, strlen(pChar) + 1);
         break;
      }

      if ((int)strlen(szBuffer) + (int)strlen(m_szNetBuffer) >= (iBufSize - 1))
         break;      // Buffer is too small, stop receiving
      strcat(szBuffer, m_szNetBuffer);
   }

   return strlen(szBuffer);
}


//
// Execute command
// If pdwBufferSize and pszReplyBuffer is NULL, result will not be stored
//

DWORD ExecuteCommand(char *szCmd, BOOL bExpectData = FALSE, BOOL bMultiLineData = FALSE)
{
   char szBuffer[MAX_LINE_SIZE];
   int iError;
	struct timeval timeout;
	FD_SET rdfs;

   if (m_hSocket == -1)
      return ERR_NOT_CONNECTED;

   // Send command
   sprintf(szBuffer, "%s\r\n", szCmd);
   iError = send(m_hSocket, szBuffer, strlen(szBuffer), 0);
   if (iError != (int)strlen(szBuffer))
   {
      if (iError <= 0)     // Connection closed or broken
      {
         closesocket(m_hSocket);
         m_hSocket = -1;
      }
      return ERR_CONNECTION_BROKEN;
   }

   // Wait for responce
   FD_ZERO(&rdfs);
   FD_SET(m_hSocket, &rdfs);
   timeout.tv_sec = 3;
   timeout.tv_usec = 0;
   if (select(m_hSocket + 1, &rdfs, NULL, NULL, &timeout) == 0)
   {
      return ERR_REQUEST_TIMEOUT;
   }

   // Receive responce
   iError = RecvLine(255, szBuffer);
   if (iError <= 0)     // Connection closed or broken
   {
      closesocket(m_hSocket);
      m_hSocket = -1;
      return ERR_CONNECTION_BROKEN;
   }

   // Check for error
   if (memicmp(szBuffer, "+OK", 3))
   {
      if (szBuffer[0] == '-')
      {
         char *pSep;

         pSep = strchr(szBuffer, ' ');
         if (pSep)
            *pSep = 0;
         pSep = strchr(szBuffer, '\r');
         if (pSep)
            *pSep = 0;
         return strtoul(&szBuffer[1], NULL, 10);
      }
      else
      {
         return ERR_BAD_RESPONCE;
      }
   }

   // Retrieve data if needed
   if (bExpectData)
   {
      // Initialize result store
      DestroyResultData();

      iError = RecvLine(MAX_LINE_SIZE, szBuffer);
      if (iError <= 0)
         return ERR_CONNECTION_BROKEN;

      if (bMultiLineData)
      {
         while(strcmp(szBuffer, "."))
         {
            AddDataLine(szBuffer);
            iError = RecvLine(MAX_LINE_SIZE, szBuffer);
            if (iError <= 0)
               return ERR_CONNECTION_BROKEN;
         }
      }
      else
      {
         AddDataLine(szBuffer);
      }
   }

   return ERR_SUCCESS;
}


//
// Get parameter value
//

DWORD GetParameter(const char *szParam, DWORD dwBufSize, char *szBuffer)
{
   char szRequest[256];
   DWORD dwError;

   sprintf(szRequest, "GET %s", szParam);
   dwError = ExecuteCommand(szRequest, TRUE);
   if (dwError == ERR_SUCCESS)
   {
      szBuffer[dwBufSize - 1] = 0;
      strncpy(szBuffer, m_ppDataLines[0], dwBufSize - 1);
      DestroyResultData();
   }

   return dwError;
}


//
// Connect to agent
//

BOOL Connect(BOOL bVerbose)
{
   struct sockaddr_in sa;
   char szBuffer[256], szSignature[32];
   int iError;
   BOOL bSuccess = FALSE;

   // Initialize line receive buffer
   m_szNetBuffer[0] = 0;

   // Create socket
   m_hSocket = socket(AF_INET, SOCK_STREAM, 0);
   if (m_hSocket == -1)
   {
      fprintf(stderr, "Unable to create socket\n");
      goto connect_cleanup;
   }

   // Fill in address structure
   memset(&sa, 0, sizeof(sa));
   sa.sin_addr.s_addr = m_dwAddr;
   sa.sin_family = AF_INET;
   sa.sin_port = htons(m_wPort);

   // Connect to server
   if (connect(m_hSocket, (struct sockaddr *)&sa, sizeof(sa)) == -1)
   {
      if (bVerbose)
         fprintf(stderr, "Unable to establish connection with the agent\n");
      goto connect_cleanup;
   }

   // Retrieve agent hello
   iError = RecvLine(255, szBuffer);
   if (iError <= 0)
   {
      if (bVerbose)
         fprintf(stderr, "Communication failed\n");
      goto connect_cleanup;
   }

   sprintf(szSignature, "+%d NMS_Agent_09180431", AGENT_PROTOCOL_VERSION);
   if (memcmp(szBuffer, szSignature, strlen(szSignature)))
   {
      fprintf(stderr, "Invalid HELLO packet received from agent\n");
      goto connect_cleanup;
   }

   // Authenticate itself to agent
   if (m_iAuthMethod != AUTH_NONE)
   {
   }

   // Test connectivity
   if (ExecuteCommand("NOP") != ERR_SUCCESS)
   {
      fprintf(stderr, "Communication with agent failed\n");
      goto connect_cleanup;
   }

   bSuccess = TRUE;

connect_cleanup:
   if (!bSuccess)
   {
      if (m_hSocket != -1)
      {
         shutdown(m_hSocket, 2);
         closesocket(m_hSocket);
         m_hSocket = -1;
      }
   }
   return bSuccess;
}


//
// Disconnect from agent
//

void Disconnect(void)
{
   if (m_hSocket != -1)
   {
      ExecuteCommand("QUIT");
      shutdown(m_hSocket, 2);
      closesocket(m_hSocket);
      m_hSocket = -1;
   }
   DestroyResultData();
}


//
// Startup
//

int main(int argc, char *argv[])
{
   char szBuffer[1024];
   DWORD dwError;

   // Parse command line
   if (argc < 3)
   {
      printf("Usage: nxget <host> <parameter>\n");
      return 1;
   }

   // Initialize WinSock
#ifdef _WIN32
   WSADATA wsaData;

   WSAStartup(2, &wsaData);
#endif

   // Connect to agent
   m_dwAddr = inet_addr(argv[1]);
   if (Connect(TRUE))
   {
      dwError = GetParameter(argv[2], 1024, szBuffer);
      Disconnect();
      puts(szBuffer);
   }
   return 0;
}
