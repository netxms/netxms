/* 
** nxget - command line tool used to retrieve parameters from NetXMS agent
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
#include <nxsrvapi.h>

#ifndef _WIN32
#include <netdb.h>
#endif


//
// Constants
//

#define MAX_LINE_SIZE      4096

#define CMD_GET            0
#define CMD_LIST           1
#define CMD_CHECK_SERVICE  2
#define CMD_GET_PARAMS     3


//
// Static data
//

static DWORD m_dwAddr;
static int m_iAuthMethod = AUTH_NONE;
static BOOL m_bVerbose = TRUE;


//
// Get single parameter
//

static int Get(AgentConnection *pConn, char *pszParam, BOOL bShowName)
{
   DWORD dwError;
   char szBuffer[1024];

   dwError = pConn->GetParameter(pszParam, 1024, szBuffer);
   if (dwError == ERR_SUCCESS)
   {
      if (bShowName)
         printf("%s = %s\n", pszParam, szBuffer);
      else
         printf("%s\n", szBuffer);
   }
   else
   {
      printf("%d: %s\n", dwError, AgentErrorCodeToText(dwError));
   }
   return (dwError == ERR_SUCCESS) ? 0 : 1;
}


//
// Get list of values for enum parameters
//

static int List(AgentConnection *pConn, char *pszParam)
{
   DWORD i, dwNumLines, dwError;

   dwError = pConn->GetList(pszParam);
   if (dwError == ERR_SUCCESS)
   {
      dwNumLines = pConn->GetNumDataLines();
      for(i = 0; i < dwNumLines; i++)
         printf("%s\n", pConn->GetDataLine(i));
   }
   else
   {
      printf("%d: %s\n", dwError, AgentErrorCodeToText(dwError));
   }
   return (dwError == ERR_SUCCESS) ? 0 : 1;
}


//
// Check network service state
//

static int CheckService(AgentConnection *pConn, int iServiceType, DWORD dwServiceAddr,
                        WORD wProto, WORD wPort, char *pszRequest, char *pszResponce)
{
   DWORD dwStatus, dwError;

   dwError = pConn->CheckNetworkService(&dwStatus, dwServiceAddr, iServiceType, wPort,
                                        wProto, pszRequest, pszResponce);
   if (dwError == ERR_SUCCESS)
   {
      printf("Service status: %d\n", dwStatus);
   }
   else
   {
      printf("%d: %s\n", dwError, AgentErrorCodeToText(dwError));
   }
   return (dwError == ERR_SUCCESS) ? 0 : 1;
}


//
// List supported parameters
//

static int ListParameters(AgentConnection *pConn)
{
   DWORD i, dwNumParams, dwError;
   NXC_AGENT_PARAM *pParamList;
   static char *pszDataType[] = { "INT", "UINT", "INT64", "UINT64", "STRING", "FLOAT", "UNKNOWN" };

   dwError = pConn->GetSupportedParameters(&dwNumParams, &pParamList);
   if (dwError == ERR_SUCCESS)
   {
      for(i = 0; i < dwNumParams; i++)
      {
         printf("%s %s \"%s\"\n", pParamList[i].szName,
            pszDataType[(pParamList[i].iDataType < 6) && (pParamList[i].iDataType >= 0) ? pParamList[i].iDataType : 6],
            pParamList[i].szDescription);
      }
      safe_free(pParamList);
   }
   else
   {
      printf("%d: %s\n", dwError, AgentErrorCodeToText(dwError));
   }
   return (dwError == ERR_SUCCESS) ? 0 : 1;
}


//
// Startup
//

int main(int argc, char *argv[])
{
   char *eptr;
   BOOL bStart = TRUE, bBatchMode = FALSE, bShowNames = FALSE;
   int i, ch, iPos, iExitCode = 3, iCommand = CMD_GET, iInterval = 0;
   int iAuthMethod = AUTH_NONE, iServiceType = NETSRV_SSH;
   WORD wAgentPort = AGENT_LISTEN_PORT, wServicePort = 0, wServiceProto = 0;
   DWORD dwTimeout = 3000, dwServiceAddr = 0;
   char szSecret[MAX_SECRET_LENGTH] = "", szRequest[MAX_DB_STRING] = "", szResponce[MAX_DB_STRING] = "";

   // Parse command line
   opterr = 1;
   while((ch = getopt(argc, argv, "a:bi:hIlnp:P:qr:R:s:S:t:vw:")) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
            printf("Usage: nxget [<options>] <host> [<parameter> [<parameter> ...]]\n"
                   "Valid options are:\n"
                   "   -a <auth>    : Authentication method. Valid methods are \"none\",\n"
                   "                  \"plain\", \"md5\" and \"sha1\". Default is \"none\".\n"
                   "   -b           : Batch mode - get all parameters listed on command line.\n"
                   "   -h           : Display help and exit.\n"
                   "   -i <seconds> : Get specified parameter(s) continously with given interval.\n"
                   "   -I           : Get list of supported parameters.\n"
                   "   -l           : Get list of values for enum parameter.\n"
                   "   -n           : Show parameter's name in result.\n"
                   "   -p <port>    : Specify agent's port number. Default is %d.\n"
                   "   -P <port>    : Specify network service port (to be used wth -S option).\n"
                   "   -q           : Quiet mode.\n"
                   "   -r <string>  : Service check request string.\n"
                   "   -R <string>  : Service check expected responce string.\n"
                   "   -s <secret>  : Specify shared secret for authentication.\n"
                   "   -S <addr>    : Check state of network service at given address.\n"
                   "   -t <type>    : Set type of service to be checked.\n"
                   "   -T <proto>   : Protocol number to be used for service check.\n"
                   "   -v           : Display version and exit.\n"
                   "   -w <seconds> : Specify command timeout (default is 3 seconds).\n"
                   "\n", wAgentPort);
            bStart = FALSE;
            break;
         case 'a':   // Auth method
            if (!strcmp(optarg, "none"))
               iAuthMethod = AUTH_NONE;
            else if (!strcmp(optarg, "plain"))
               iAuthMethod = AUTH_PLAINTEXT;
            else if (!strcmp(optarg, "md5"))
               iAuthMethod = AUTH_MD5_HASH;
            else if (!strcmp(optarg, "sha1"))
               iAuthMethod = AUTH_SHA1_HASH;
            else
            {
               printf("Invalid authentication method \"%s\"\n", optarg);
               bStart = FALSE;
            }
            break;
         case 'b':   // Batch mode
            bBatchMode = TRUE;
            break;
         case 'i':   // Interval
            i = strtol(optarg, &eptr, 0);
            if ((*eptr != 0) || (i <= 0))
            {
               printf("Invalid interval \"%s\"\n", optarg);
               bStart = FALSE;
            }
            else
            {
               iInterval = i;
            }
            break;
         case 'I':
            iCommand = CMD_GET_PARAMS;
            break;
         case 'l':
            iCommand = CMD_LIST;
            break;
         case 'n':   // Show names
            bShowNames = TRUE;
            break;
         case 'p':   // Agent's port number
         case 'P':   // Port number for service check
            i = strtol(optarg, &eptr, 0);
            if ((*eptr != 0) || (i < 0) || (i > 65535))
            {
               printf("Invalid port number \"%s\"\n", optarg);
               bStart = FALSE;
            }
            else
            {
               if (ch == 'p')
                  wAgentPort = (WORD)i;
               else
                  wServicePort = (WORD)i;
            }
            break;
         case 'q':   // Quiet mode
            m_bVerbose = FALSE;
            break;
         case 'r':   // Service check request string
            strncpy(szRequest, optarg, MAX_DB_STRING);
            break;
         case 'R':   // Service check responce string
            strncpy(szResponce, optarg, MAX_DB_STRING);
            break;
         case 's':   // Shared secret
            strncpy(szSecret, optarg, MAX_SECRET_LENGTH);
            break;
         case 'S':   // Check service
            iCommand = CMD_CHECK_SERVICE;
            dwServiceAddr = ntohl(inet_addr(optarg));
            if ((dwServiceAddr == INADDR_NONE) || (dwServiceAddr == INADDR_ANY))
            {
               printf("Invalid IP address \"%s\"\n", optarg);
               bStart = FALSE;
            }
            break;
         case 't':   // Service type
            iServiceType = strtol(optarg, &eptr, 0);
            if (*eptr != 0)
            {
               printf("Invalid service type \"%s\"\n", optarg);
               bStart = FALSE;
            }
            break;
         case 'T':   // Protocol number for service check
            i = strtol(optarg, &eptr, 0);
            if ((*eptr != 0) || (i < 0) || (i > 65535))
            {
               printf("Invalid protocol number \"%s\"\n", optarg);
               bStart = FALSE;
            }
            else
            {
               wServiceProto = (WORD)i;
            }
            break;
         case 'v':   // Print version and exit
            printf("NetXMS GET command-line utility Version " NETXMS_VERSION_STRING "\n");
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
               dwTimeout = (DWORD)i * 1000;  // Convert to milliseconds
            }
            break;
         case '?':
            bStart = FALSE;
            break;
         default:
            break;
      }
   }

   // Check parameter correctness
   if (bStart)
   {
      if (argc - optind < (((iCommand == CMD_CHECK_SERVICE) || (iCommand == CMD_GET_PARAMS)) ? 1 : 2))
      {
         printf("Required argument(s) missing.\nUse nxget -h to get complete command line syntax.\n");
         bStart = FALSE;
      }
      else if ((iAuthMethod != AUTH_NONE) && (szSecret[0] == 0))
      {
         printf("Shared secret not specified or empty\n");
         bStart = FALSE;
      }

      // If everything is ok, start communications
      if (bStart)
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
            AgentConnection conn(m_dwAddr, wAgentPort, iAuthMethod, szSecret);

            conn.SetCommandTimeout(dwTimeout);
            if (conn.Connect(m_bVerbose))
            {
               do
               {
                  switch(iCommand)
                  {
                     case CMD_GET:
                        iPos = optind + 1;
                        do
                        {
                           iExitCode = Get(&conn, argv[iPos++], bShowNames);
                        } while((iExitCode == 0) && (bBatchMode) && (iPos < argc));
                        break;
                     case CMD_LIST:
                        iExitCode = List(&conn, argv[optind + 1]);
                        break;
                     case CMD_CHECK_SERVICE:
                        iExitCode = CheckService(&conn, iServiceType, dwServiceAddr,
                                                 wServiceProto, wServicePort, szRequest, szResponce);
                        break;
                     case CMD_GET_PARAMS:
                        iExitCode = ListParameters(&conn);
                        break;
                     default:
                        break;
                  }
                  ThreadSleep(iInterval);
               }
               while(iInterval > 0);
               conn.Disconnect();
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
