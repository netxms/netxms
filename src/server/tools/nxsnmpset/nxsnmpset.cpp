/* 
** nxsnmpget - command line tool used to retrieve parameters from SNMP agent
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
** $module: nxsnmpget.cpp
**
**/

#include <nms_common.h>
#include <nms_agent.h>
#include <nms_util.h>
#include <nxsnmp.h>


//
// Static data
//

static char m_szCommunity[256] = "public";
static WORD m_wPort = 161;
static DWORD m_dwVersion = SNMP_VERSION_2C;
static DWORD m_dwTimeout = 3000;


//
// Get data
//

static int SetVariables(int argc, char *argv[])
{
   SNMP_Transport *pTransport;
   SNMP_PDU *request, *response;
   SNMP_Variable *pVar;
   DWORD dwResult;
   int i, iExit = 0;

   // Initialize WinSock
#ifdef _WIN32
   WSADATA wsaData;
   WSAStartup(2, &wsaData);
#endif

   // Create SNMP transport
   pTransport = new SNMP_Transport;
   dwResult = pTransport->CreateUDPTransport(argv[0], 0, m_wPort);
   if (dwResult != SNMP_ERR_SUCCESS)
   {
      printf("Unable to create UDP transport: %s\n", SNMPGetErrorText(dwResult));
      iExit = 2;
   }
   else
   {
      // Create request
      request = new SNMP_PDU(SNMP_SET_REQUEST, m_szCommunity, getpid(), m_dwVersion);
      for(i = 1; i < argc; i += 2)
      {
         if (SNMPIsCorrectOID(argv[i]))
         {
            pVar = new SNMP_Variable(argv[i]);
            pVar->SetValueFromString(ASN_OCTET_STRING, argv[i + 1]);
            request->BindVariable(pVar);
         }
         else
         {
            printf("Invalid OID: %s\n", argv[i]);
            iExit = 4;
         }
      }

      // Send request and process response
      if (iExit == 0)
      {
         if ((dwResult = pTransport->DoRequest(request, &response, m_dwTimeout, 3)) == SNMP_ERR_SUCCESS)
         {
            if (response->GetErrorCode() != 0)
            {
               printf("SET operation failed (error code %d)\n", response->GetErrorCode());
            }
            delete response;
         }
         else
         {
            printf("%s\n", SNMPGetErrorText(dwResult));
            iExit = 3;
         }
      }

      delete request;
   }

   delete pTransport;
   return iExit;
}


//
// Startup
//

int main(int argc, char *argv[])
{
   int ch, iExit = 1;
   DWORD dwValue;
   char *eptr;
   BOOL bStart = TRUE;

   // Parse command line
   opterr = 1;
   while((ch = getopt(argc, argv, "c:hp:v:")) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
            printf("Usage: nxsnmpset [<options>] <host> <variable> <value>\n"
                   "Valid options are:\n"
                   "   -c <string>  : Specify community string. Default is \"public\".\n"
                   "   -h           : Display help and exit.\n"
                   "   -p <port>    : Specify agent's port number. Default is 161.\n"
                   "   -v <version> : Specify SNMP version (valid values is 1 and 2c).\n"
                   "   -w <seconds> : Specify request timeout (default is 3 seconds)\n"
                   "\n");
            iExit = 0;
            bStart = FALSE;
            break;
         case 'c':   // Community
            nx_strncpy(m_szCommunity, optarg, 256);
            break;
         case 'p':   // Port number
            dwValue = strtoul(optarg, &eptr, 0);
            if ((*eptr != 0) || (dwValue > 65535) || (dwValue == 0))
            {
               printf("Invalid port number %s\n", optarg);
               bStart = FALSE;
            }
            else
            {
               m_wPort = (WORD)dwValue;
            }
            break;
         case 'v':   // Version
            if (!strcmp(optarg, "1"))
            {
               m_dwVersion = SNMP_VERSION_1;
            }
            else if (!stricmp(optarg, "2c"))
            {
               m_dwVersion = SNMP_VERSION_2C;
            }
            else
            {
               printf("Invalid SNMP version %s\n", optarg);
               bStart = FALSE;
            }
            break;
         case 'w':   // Timeout
            dwValue = strtoul(optarg, &eptr, 0);
            if ((*eptr != 0) || (dwValue > 60) || (dwValue == 0))
            {
               printf("Invalid timeout value %s\n", optarg);
               bStart = FALSE;
            }
            else
            {
               m_dwTimeout = dwValue;
            }
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
      if (argc - optind < 3)
      {
         printf("Required argument(s) missing.\nUse nxsnmpset -h to get complete command line syntax.\n");
      }
      else if ((argc - optind) % 2 != 1)
      {
         printf("Missing value for variable.\nUse nxsnmpset -h to get complete command line syntax.\n");
      }
      else
      {
         iExit = SetVariables(argc - optind, &argv[optind]);
      }
   }

   return iExit;
}
