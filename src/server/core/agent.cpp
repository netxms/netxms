/* 
** Project X - Network Management System
** Copyright (C) 2003 Victor Kirhenshtein
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
** $module: agent.cpp
**
**/

#include "nms_core.h"


//
// Default constructor for AgentConnection - normally shouldn't be used
//

AgentConnection::AgentConnection()
{
   m_dwAddr = inet_addr("127.0.0.1");
   m_wPort = AGENT_LISTEN_PORT;
   m_iAuthMethod = AUTH_NONE;
   m_szSecret[0] = 0;
   m_hSocket = -1;
   m_tLastCommandTime = 0;
   m_dwNumDataLines = 0;
   m_ppDataLines = NULL;
}


//
// Normal constructor for AgentConnection
//

AgentConnection::AgentConnection(DWORD dwAddr, WORD wPort, int iAuthMethod, char *szSecret)
{
   m_dwAddr = dwAddr;
   m_wPort = wPort;
   m_iAuthMethod = iAuthMethod;
   if (szSecret != NULL)
      strncpy(m_szSecret, szSecret, MAX_SECRET_LENGTH);
   else
      m_szSecret[0] = 0;
   m_hSocket = -1;
   m_tLastCommandTime = 0;
   m_dwNumDataLines = 0;
   m_ppDataLines = NULL;
}


//
// Destructor
//

AgentConnection::~AgentConnection()
{
   if (m_hSocket != -1)
      closesocket(m_hSocket);
   DestroyResultData();
}


//
// Connect to agent
//

BOOL AgentConnection::Connect(BOOL bVerbose)
{
   struct sockaddr_in sa;
   char szBuffer[256], szSignature[32];
   int iError;

   // Initialize line receive buffer
   m_szNetBuffer[0] = 0;

   // Create socket
   m_hSocket = socket(AF_INET, SOCK_STREAM, 0);
   if (m_hSocket == -1)
   {
      WriteLog(MSG_SOCKET_FAILED, EVENTLOG_ERROR_TYPE, "s", "AgentConnection::Connect()");
      return FALSE;
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
         WriteLog(MSG_AGENT_CONNECT_FAILED, EVENTLOG_ERROR_TYPE, "s", IpToStr(m_dwAddr, szBuffer));
      closesocket(m_hSocket);
      m_hSocket = -1;
      return FALSE;
   }

   // Retrieve agent hello
   iError = RecvLine(255, szBuffer);
   if (iError <= 0)
   {
      if (bVerbose)
         WriteLog(MSG_AGENT_CONNECT_FAILED, EVENTLOG_ERROR_TYPE, "s", IpToStr(m_dwAddr, szBuffer));
      shutdown(m_hSocket, 2);
      closesocket(m_hSocket);
      m_hSocket = -1;
      return FALSE;
   }

   sprintf(szSignature, "+%d NMS_Agent_09180431", AGENT_PROTOCOL_VERSION);
   if (memcmp(szBuffer, szSignature, strlen(szSignature)))
   {
      WriteLog(MSG_AGENT_BAD_HELLO, EVENTLOG_ERROR_TYPE, "s", IpToStr(m_dwAddr, szBuffer));
      shutdown(m_hSocket, 2);
      closesocket(m_hSocket);
      m_hSocket = -1;
      return FALSE;
   }

   // Authenticate itself to agent
   if (m_iAuthMethod != AUTH_NONE)
   {
   }

   // Test connectivity
   if (ExecuteCommand("NOP") != ERR_SUCCESS)
   {
      WriteLog(MSG_AGENT_COMM_FAILED, EVENTLOG_ERROR_TYPE, "s", IpToStr(m_dwAddr, szBuffer));
      shutdown(m_hSocket, 2);
      closesocket(m_hSocket);
      m_hSocket = -1;
      return FALSE;
   }

   return TRUE;
}


//
// Disconnect from agent
//

void AgentConnection::Disconnect(void)
{
   ExecuteCommand("QUIT");
   shutdown(m_hSocket, 2);
   closesocket(m_hSocket);
   m_hSocket = -1;
   DestroyResultData();
}


//
// Receive line from socket
//

int AgentConnection::RecvLine(int iBufSize, char *szBuffer)
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

DWORD AgentConnection::ExecuteCommand(char *szCmd, BOOL bExpectData, BOOL bMultiLineData)
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

   m_tLastCommandTime = time(NULL);
   return ERR_SUCCESS;
}


//
// Destroy command execuion results data
//

void AgentConnection::DestroyResultData(void)
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

void AgentConnection::AddDataLine(char *szLine)
{
   m_dwNumDataLines++;
   m_ppDataLines = (char **)realloc(m_ppDataLines, sizeof(char *) * m_dwNumDataLines);
   m_ppDataLines[m_dwNumDataLines - 1] = strdup(szLine);
}


//
// Get interface list from agent
//

INTERFACE_LIST *AgentConnection::GetInterfaceList(void)
{
   INTERFACE_LIST *pIfList;
   DWORD i;
   char *pChar, *pBuf;

   if (ExecuteCommand("IFLIST", TRUE, TRUE) != ERR_SUCCESS)
      return NULL;

   pIfList = (INTERFACE_LIST *)malloc(sizeof(INTERFACE_LIST));
   pIfList->iNumEntries = m_dwNumDataLines;
   pIfList->pInterfaces = (INTERFACE_INFO *)malloc(sizeof(INTERFACE_INFO) * m_dwNumDataLines);
   memset(pIfList->pInterfaces, 0, sizeof(INTERFACE_INFO) * m_dwNumDataLines);
   for(i = 0; i < m_dwNumDataLines; i++)
   {
      pBuf = m_ppDataLines[i];

      // Index
      pChar = strchr(pBuf, ' ');
      if (pChar != NULL)
      {
         *pChar = 0;
         pIfList->pInterfaces[i].dwIndex = strtoul(pBuf, NULL, 10);
         pBuf = pChar + 1;
      }

      // Address and mask
      pChar = strchr(pBuf, ' ');
      if (pChar != NULL)
      {
         char *pSlash;

         *pChar = 0;
         pSlash = strchr(pBuf, '/');
         if (pSlash != NULL)
         {
            *pSlash = 0;
            pSlash++;
         }
         else     // Just a paranoia protection, should'n happen if agent working correctly
         {
            pSlash = "24";
         }
         pIfList->pInterfaces[i].dwIpAddr = inet_addr(pBuf);
         pIfList->pInterfaces[i].dwIpNetMask = htonl(~(0xFFFFFFFF >> strtoul(pSlash, NULL, 10)));
         pBuf = pChar + 1;
      }

      // Interface type
      pChar = strchr(pBuf, ' ');
      if (pChar != NULL)
      {
         *pChar = 0;
         pIfList->pInterfaces[i].dwIndex = strtoul(pBuf, NULL, 10);
         pBuf = pChar + 1;
      }

      // Name
      strncpy(pIfList->pInterfaces[i].szName, pBuf, MAX_OBJECT_NAME - 1);
   }

   DestroyResultData();

   CleanInterfaceList(pIfList);

   return pIfList;
}


//
// Get parameter value
//

DWORD AgentConnection::GetParameter(char *szParam, DWORD dwBufSize, char *szBuffer)
{
   char szRequest[256];
   DWORD dwError;

   sprintf(szRequest, "GET %s", szParam);
   dwError = ExecuteCommand(szRequest, TRUE);
   if (dwError != ERR_SUCCESS)
      return dwError;

   szBuffer[dwBufSize - 1] = 0;
   strncpy(szBuffer, m_ppDataLines[0], dwBufSize - 1);
   DestroyResultData();

   return ERR_SUCCESS;
}


//
// Get ARP cache
//

ARP_CACHE *AgentConnection::GetArpCache(void)
{
   ARP_CACHE *pArpCache;
   char szByte[4], *pBuf, *pChar;
   DWORD i, j;

   if (ExecuteCommand("ARP", TRUE, TRUE) != ERR_SUCCESS)
      return NULL;

   // Create empty structure
   pArpCache = (ARP_CACHE *)malloc(sizeof(ARP_CACHE));
   pArpCache->dwNumEntries = m_dwNumDataLines;
   pArpCache->pEntries = (ARP_ENTRY *)malloc(sizeof(ARP_ENTRY) * m_dwNumDataLines);
   memset(pArpCache->pEntries, 0, sizeof(ARP_ENTRY) * m_dwNumDataLines);

   szByte[2] = 0;

   // Parse data lines
   // Each line has form of XXXXXXXXXXXX a.b.c.d
   // where XXXXXXXXXXXX is a MAC address (12 hexadecimal digits)
   // and a.b.c.d is an IP address in decimal dotted notation
   for(i = 0; i < m_dwNumDataLines; i++)
   {
      pBuf = m_ppDataLines[i];
      if (strlen(pBuf) < 20)     // Invalid line
         continue;

      // MAC address
      for(j = 0; j < 6; j++)
      {
         memcpy(szByte, pBuf, 2);
         pArpCache->pEntries[i].bMacAddr[j] = (BYTE)strtol(szByte, NULL, 16);
         pBuf+=2;
      }

      // IP address
      while(*pBuf == ' ')
         pBuf++;
      pChar = strchr(pBuf, ' ');
      if (pChar != NULL)
         *pChar = 0;
      pArpCache->pEntries[i].dwIpAddr = inet_addr(pBuf);

      // Interface index
      if (pChar != NULL)
         pArpCache->pEntries[i].dwIndex = strtoul(pChar + 1, NULL, 10);
   }

   DestroyResultData();
   return pArpCache;
}
