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
#include <nxsrvapi.h>

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
static WORD m_wPort = AGENT_LISTEN_PORT;
static BOOL m_bVerbose = TRUE;
static BOOL m_bTrace = FALSE;
static DWORD m_dwTimeout = 3000;


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
            AgentConnection conn(m_dwAddr);

            if (conn.Connect(m_bVerbose))
            {
               dwError = conn.GetParameter(argv[optind + 1], 1024, szBuffer);
               conn.Disconnect();
               if (dwError == ERR_SUCCESS)
                  printf("%s\n", szBuffer);
               else
                  printf("Error %d\n", dwError);
            }
         }
      }
   }

   return iExitCode;
}
