/* 
** nxupload - command line tool used to upload files to NetXMS agent
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
** $module: nxupload.cpp
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
// Do agent upgrade
//

static int UpgradeAgent(AgentConnection &conn, TCHAR *pszPkgName, BOOL bVerbose)
{
   DWORD dwError;
   int i;
   BOOL bConnected = FALSE;

   dwError = conn.StartUpgrade(pszPkgName);
   if (dwError == ERR_SUCCESS)
   {
      conn.Disconnect();

      if (bVerbose)
      {
         printf("Agent upgrade started, waiting for completion...\n"
                "[............................................................]\r[");
         fflush(stdout);
         for(i = 0; i < 120; i += 2)
         {
            ThreadSleep(2);
            putc('*', stdout);
            fflush(stdout);
            if ((i % 20 == 0) && (i > 30))
            {
               if (conn.Connect(FALSE))
               {
                  bConnected = TRUE;
                  break;   // Connected successfully
               }
            }
         }
         putc('\n', stdout);
      }
      else
      {
         ThreadSleep(20);
         for(i = 20; i < 120; i += 20)
         {
            ThreadSleep(20);
            if (conn.Connect(FALSE))
            {
               bConnected = TRUE;
               break;   // Connected successfully
            }
         }
      }

      // Last attempt to reconnect
      if (!bConnected)
         bConnected = conn.Connect(bVerbose);

      if (bConnected && bVerbose)
      {
         printf("Successfully established connection to agent after upgrade\n");
      }
      else
      {
         fprintf(stderr, "Failed to establish connection to the agent after upgrade\n");
      }
   }
   else
   {
      if (bVerbose)
         fprintf(stderr, "%d: %s\n", dwError, AgentErrorCodeToText(dwError));
   }

   return bConnected ? 0 : 1;
}


//
// Startup
//

int main(int argc, char *argv[])
{
   char *eptr;
   BOOL bStart = TRUE, bVerbose = TRUE, bUpgrade = TRUE;
   int i, ch, iExitCode = 3;
   int iAuthMethod = AUTH_NONE;
   WORD wPort = AGENT_LISTEN_PORT;
   DWORD dwAddr, dwTimeout = 3000;
   char szSecret[MAX_SECRET_LENGTH] = "";
   INT64 nElapsedTime;

   // Parse command line
   opterr = 1;
   while((ch = getopt(argc, argv, "a:hp:qs:uvw:")) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
            printf("Usage: nxupload [<options>] <host> <file>\n"
                   "Valid options are:\n"
                   "   -a <auth>    : Authentication method. Valid methods are \"none\",\n"
                   "                  \"plain\", \"md5\" and \"sha1\". Default is \"none\".\n"
                   "   -h           : Display help and exit.\n"
                   "   -p <port>    : Specify agent's port number. Default is %d.\n"
                   "   -q           : Quiet mode.\n"
                   "   -s <secret>  : Specify shared secret for authentication.\n"
                   "   -u           : Start agent upgrade from uploaded package.\n"
                   "   -v           : Display version and exit.\n"
                   "   -w <seconds> : Specify command timeout (default is 3 seconds)\n"
                   "\n", wPort);
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
         case 'p':   // Port number
            i = strtol(optarg, &eptr, 0);
            if ((*eptr != 0) || (i < 0) || (i > 65535))
            {
               printf("Invalid port number \"%s\"\n", optarg);
               bStart = FALSE;
            }
            else
            {
               wPort = (WORD)i;
            }
            break;
         case 'q':   // Quiet mode
            bVerbose = FALSE;
            break;
         case 's':   // Shared secret
            strncpy(szSecret, optarg, MAX_SECRET_LENGTH - 1);
            break;
         case 'u':   // Upgrade agent
            bUpgrade = TRUE;
            break;
         case 'v':   // Print version and exit
            printf("NetXMS UPLOAD command-line utility Version " NETXMS_VERSION_STRING "\n");
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
      if (argc - optind < 2)
      {
         if (bVerbose)
            printf("Required argument(s) missing.\nUse nxupload -h to get complete command line syntax.\n");
         bStart = FALSE;
      }
      else if ((iAuthMethod != AUTH_NONE) && (szSecret[0] == 0))
      {
         if (bVerbose)
            fprintf(stderr, "Shared secret not specified or empty\n");
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
            memcpy(&dwAddr, hs->h_addr, sizeof(DWORD));
         }
         else
         {
            dwAddr = inet_addr(argv[optind]);
         }
         if ((dwAddr == 0) || (dwAddr == INADDR_NONE))
         {
            if (bVerbose)
               fprintf(stderr, "Invalid host name or address specified\n");
         }
         else
         {
            AgentConnection conn(dwAddr, wPort, iAuthMethod, szSecret);

            conn.SetCommandTimeout(dwTimeout);
            if (conn.Connect(bVerbose))
            {
               DWORD dwError;

               nElapsedTime = GetCurrentTimeMs();
               dwError = conn.UploadFile(argv[optind + 1]);
               nElapsedTime = GetCurrentTimeMs() - nElapsedTime;
               if (bVerbose)
               {
                  if (dwError == ERR_SUCCESS)
                  {
                     QWORD qwBytes;

                     qwBytes = FileSize(argv[optind + 1]);
                     printf("File transferred successfully\n"
#ifdef _WIN32
                            "%I64u"
#else
                            "%llu"
#endif
                            " bytes in %d.%03d seconds (%.2f KB/sec)\n",
                            qwBytes, (long)(nElapsedTime / 1000), 
                            (long)(nElapsedTime % 1000), 
                            ((double)((INT64)qwBytes / 1024) / (double)nElapsedTime) * 1000);
                  }
                  else
                  {
                     fprintf(stderr, "%d: %s\n", dwError, AgentErrorCodeToText(dwError));
                  }
               }

               if (bUpgrade && (dwError == RCC_SUCCESS))
               {
                  iExitCode = UpgradeAgent(conn, argv[optind + 1], bVerbose);
               }
               else
               {
                  iExitCode = (dwError == ERR_SUCCESS) ? 0 : 1;
               }
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
