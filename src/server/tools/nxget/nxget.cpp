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
#include <nms_util.h>
#include <nxclapi.h>

#ifndef _WIN32
#include <netdb.h>
#endif


//
// Constants
//

#define MAX_LINE_SIZE      4096
#define NXGET_VERSION      "2"

#define TRACE_IN           0
#define TRACE_OUT          1

#define CMD_GET            0
#define CMD_LIST_PARAMS    1
#define CMD_LIST_SUBAGENTS 2
#define CMD_LIST_ARP       3
#define CMD_LIST_IF        4


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
static BOOL m_bVerbose = TRUE;
static BOOL m_bTrace = FALSE;
static DWORD m_dwTimeout = 3;


//
// Print protocol trace information
//

static void Trace(int iDirection, char *pszLine)
{
   static char *pszDir[] = { "<<<", ">>>" };
   if (m_bTrace)
      printf("%s %s\n", pszDir[iDirection], pszLine);
}


//
// Destroy command execuion results data
//

static void DestroyResultData(void)
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
	fd_set rdfs;
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
      timeout.tv_sec = m_dwTimeout;
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
	fd_set rdfs;

   if (m_hSocket == -1)
      return ERR_NOT_CONNECTED;

   // Send command
   Trace(TRACE_OUT, szCmd);
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
   timeout.tv_sec = m_dwTimeout;
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
   Trace(TRACE_IN, szBuffer);

   // Check for error
   if (memcmp(szBuffer, "+OK", 3))
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
      Trace(TRACE_IN, szBuffer);

      if (bMultiLineData)
      {
         while(strcmp(szBuffer, "."))
         {
            AddDataLine(szBuffer);
            iError = RecvLine(MAX_LINE_SIZE, szBuffer);
            if (iError <= 0)
               return ERR_CONNECTION_BROKEN;
            Trace(TRACE_IN, szBuffer);
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

BOOL Connect(void)
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
      if (m_bVerbose)
         printf("Unable to create socket\n");
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
      if (m_bVerbose)
         printf("Unable to establish connection with the agent\n");
      goto connect_cleanup;
   }

   // Retrieve agent hello
   iError = RecvLine(255, szBuffer);
   if (iError <= 0)
   {
      if (m_bVerbose)
         printf("Communication failed\n");
      goto connect_cleanup;
   }
   Trace(TRACE_IN, szBuffer);

   sprintf(szSignature, "+%d NMS_Agent_09180431", AGENT_PROTOCOL_VERSION);
   if (memcmp(szBuffer, szSignature, strlen(szSignature)))
   {
      if (m_bVerbose)
         printf("Invalid HELLO packet received from agent\n");
      goto connect_cleanup;
   }

   // Authenticate itself to agent
   if (m_iAuthMethod != AUTH_NONE)
   {
   }

   // Test connectivity
   if (ExecuteCommand("NOP") != ERR_SUCCESS)
   {
      if (m_bVerbose)
         printf("Communication with agent failed\n");
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
}


//
// Startup
//

int main(int argc, char *argv[])
{
   char *eptr, szBuffer[1024];
   DWORD dwError;
   BOOL bStart = TRUE;
   int i, ch, iExitCode = 3, iCommand = CMD_GET;

   // Parse command line
   opterr = 1;
   while((ch = getopt(argc, argv, "a:hp:qs:tvw:AILS")) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
            printf("Usage: nxget [<options>] <host> <parameter>\n"
                   "   or: nxget [<options>] <command> <host>\n\n"
                   "Valid options are:\n"
                   "   -a <auth>    : Authentication method. Valid methods are \"none\",\n"
                   "                  \"plain-text\", \"md5\" and \"sha1\". Default is \"none\".\n"
                   "   -h           : Display help and exit.\n"
                   "   -p <port>    : Specify agent's port number. Default is %d.\n"
                   "   -q           : Quiet mode.\n"
                   "   -s <secret>  : Specify shared secret for authentication.\n"
                   "   -t           : Turn on protocol trace\n"
                   "   -v           : Display version and exit.\n"
                   "   -w <seconds> : Specify command timeout (default is 3 seconds)\n"
                   "\nValid commands are:\n"
                   "   -A           : Retrieve ARP cache\n"
                   "   -I           : Retrieve a list of interfaces\n"
                   "   -L           : Retrieve a list of supported parameters\n"
                   "   -S           : Retrieve a list of loaded subagents\n"
                   "\n", m_wPort);
            bStart = FALSE;
            break;
         case 'p':   // Port number
            i = strtol(optarg, &eptr, 0);
            if ((*eptr != 0) || (i < 0) || (i > 65535))
            {
               printf("Invalid port number \"%s\"\n", optarg);
               bStart = FALSE;
            }
            else
            {
               m_wPort = (WORD)i;
            }
            break;
         case 'q':   // Quiet mode
            m_bVerbose = FALSE;
            break;
         case 's':   // Shared secret
            strncpy(m_szSecret, optarg, MAX_SECRET_LENGTH - 1);
            break;
         case 't':   // Protocol trace
            m_bTrace = TRUE;
            break;
         case 'v':   // Print version and exit
            printf("NetXMS GET command-line utility Version " NETXMS_VERSION_STRING "." NXGET_VERSION "\n");
            bStart = FALSE;
            break;
         case 'w':   // Command timeout
            i = strtol(optarg, &eptr, 0);
            if ((*eptr != 0) || (i < 1) || (i > 120))
            {
               printf("Invalid timeout \"%s\"\n", optarg);
               bStart = FALSE;
            }
            else
            {
               m_dwTimeout = (DWORD)i;
            }
            break;
         case 'A':
            iCommand = CMD_LIST_ARP;
            break;
         case 'I':
            iCommand = CMD_LIST_IF;
            break;
         case 'L':
            iCommand = CMD_LIST_PARAMS;
            break;
         case 'S':
            iCommand = CMD_LIST_SUBAGENTS;
            break;
         case '?':
            bStart = FALSE;
            break;
         default:
            break;
      }
   }

   if (bStart)
   {
      if (argc - optind < (iCommand == CMD_GET ? 2 : 1))
      {
         printf("Required argument(s) missing.\nUse nxget -h to get complete command line syntax.\n");
      }
      else
      {
         struct hostent *hs;

         // Initialize WinSock
#ifdef _WIN32
         WSADATA wsaData;

         WSAStartup(2, &wsaData);
#endif

         // Resolve hostname
         hs = gethostbyname(argv[optind]);
         if (hs != NULL)
         {
            memcpy(&m_dwAddr, hs->h_addr, sizeof(DWORD));
         }
         else
         {
            m_dwAddr = inet_addr(argv[optind]);
         }
         if ((m_dwAddr == 0) || (m_dwAddr == INADDR_NONE))
         {
            fprintf(stderr, "Invalid host name or address specified\n");
         }
         else
         {
            // Connect to agent
            if (Connect())
            {
               switch(iCommand)
               {
                  case CMD_GET:
                     dwError = GetParameter(argv[optind + 1], 1024, szBuffer);
                     break;
                  case CMD_LIST_ARP:
                     dwError = ExecuteCommand("ARP", TRUE, TRUE);
                     break;
                  case CMD_LIST_IF:
                     dwError = ExecuteCommand("IFLIST", TRUE, TRUE);
                     break;
                  case CMD_LIST_PARAMS:
                     dwError = ExecuteCommand("LIST", TRUE, TRUE);
                     break;
                  case CMD_LIST_SUBAGENTS:
                     dwError = ExecuteCommand("SALIST", TRUE, TRUE);
                     break;
                  default:
                     break;
               }
               Disconnect();
               if (dwError == ERR_SUCCESS)
               {
                  switch(iCommand)
                  {
                     case CMD_GET:
                        puts(szBuffer);
                        break;
                     case CMD_LIST_ARP:
                     case CMD_LIST_IF:
                     case CMD_LIST_PARAMS:
                     case CMD_LIST_SUBAGENTS:
                        for(i = 0; i < (int)m_dwNumDataLines; i++)
                           printf("%s\n", m_ppDataLines[i]);
                        DestroyResultData();
                        break;
                     default:
                        break;
                  }
                  iExitCode = 0;
               }
               else
               {
                  if (m_bVerbose)
                     printf("Error %lu\n", dwError);
                  iExitCode = 1;
               }
            }
            else
            {
               iExitCode = 2;
            }
         }
      }
   }

   return iExitCode;
}
