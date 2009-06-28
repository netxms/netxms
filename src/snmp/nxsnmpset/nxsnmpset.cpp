/* 
** nxsnmpset - command line tool used to set parameters on SNMP agent
** Copyright (C) 2004-2009 Victor Kirhenshtein
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
** File: nxsnmpset.cpp
**
**/

#include <nms_common.h>
#include <nms_agent.h>
#include <nms_util.h>
#include <nxsnmp.h>


//
// Static data
//

static char m_community[256] = "public";
static char m_user[256] = "";
static char m_authPassword[256] = "";
static char m_encryptionPassword[256] = "";
static WORD m_port = 161;
static DWORD m_snmpVersion = SNMP_VERSION_2C;
static DWORD m_timeout = 3000;
static DWORD m_type = ASN_OCTET_STRING;


//
// Get data
//

static int SetVariables(int argc, char *argv[])
{
   SNMP_UDPTransport *pTransport;
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
   pTransport = new SNMP_UDPTransport;
   dwResult = pTransport->createUDPTransport(argv[0], 0, m_port);
   if (dwResult != SNMP_ERR_SUCCESS)
   {
      printf("Unable to create UDP transport: %s\n", SNMPGetErrorText(dwResult));
      iExit = 2;
   }
   else
   {
      // Create request
		if (m_snmpVersion == SNMP_VERSION_3)
			request = new SNMP_PDU(SNMP_SET_REQUEST, new SNMP_SecurityContext(m_user, m_authPassword, m_encryptionPassword), getpid(), SNMP_VERSION_3);
		else
			request = new SNMP_PDU(SNMP_SET_REQUEST, m_community, getpid(), m_snmpVersion);
      for(i = 1; i < argc; i += 2)
      {
         if (SNMPIsCorrectOID(argv[i]))
         {
            pVar = new SNMP_Variable(argv[i]);
            pVar->SetValueFromString(m_type, argv[i + 1]);
            request->bindVariable(pVar);
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
         if ((dwResult = pTransport->doRequest(request, &response, m_timeout, 3)) == SNMP_ERR_SUCCESS)
         {
            if (response->getErrorCode() != 0)
            {
               printf("SET operation failed (error code %d)\n", response->getErrorCode());
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
	while((ch = getopt(argc, argv, "A:c:E:hp:t:u:v:w:")) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
            printf("Usage: nxsnmpset [<options>] <host> <variable> <value>\n"
                   "Valid options are:\n"
                   "   -A <passwd>  : User's authentication password for SNMP v3 USM\n"
                   "   -c <string>  : Community string. Default is \"public\"\n"
                   "   -E <passwd>  : User's encryption password for SNMP v3 USM\n"
                   "   -h           : Display help and exit\n"
                   "   -p <port>    : Agent's port number. Default is 161\n"
                   "   -t <type>    : Specify variable's data type. Default is octet string.\n"
                   "   -u <user>    : User name for SNMP v3 USM\n"
                   "   -v <version> : SNMP version to use (valid values is 1, 2c, and 3)\n"
                   "   -w <seconds> : Request timeout (default is 3 seconds)\n"
                   "Note: You can specify data type either as number or in symbolic form.\n"
                   "      Valid symbolic representations are following:\n"
                   "         INTEGER, STRING, OID, IPADDR, COUNTER32, GAUGE32,\n"
                   "         TIMETICKS, COUNTER64, UINT32.\n"
                   "\n");
            iExit = 0;
            bStart = FALSE;
            break;
         case 't':
            // First, check for numeric representation
            dwValue = strtoul(optarg, &eptr, 0);
            if (*eptr != 0)
            {
               // Try to resolve from symbolic form
               dwValue = SNMPResolveDataType(optarg);
               if (dwValue == ASN_NULL)
               {
                  printf("Invalid data type %s\n", optarg);
                  bStart = FALSE;
               }
               else
               {
                  m_type = dwValue;
               }
            }
            else
            {
               m_type = dwValue;
            }
            break;
         case 'c':   // Community
            nx_strncpy(m_community, optarg, 256);
            break;
         case 'u':   // User
            nx_strncpy(m_user, optarg, 256);
            break;
         case 'A':   // authentication password
            nx_strncpy(m_authPassword, optarg, 256);
            break;
         case 'E':   // encription password
            nx_strncpy(m_encryptionPassword, optarg, 256);
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
               m_port = (WORD)dwValue;
            }
            break;
         case 'v':   // Version
            if (!strcmp(optarg, "1"))
            {
               m_snmpVersion = SNMP_VERSION_1;
            }
            else if (!stricmp(optarg, "2c"))
            {
               m_snmpVersion = SNMP_VERSION_2C;
            }
            else if (!stricmp(optarg, "3"))
            {
               m_snmpVersion = SNMP_VERSION_3;
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
               m_timeout = dwValue;
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
