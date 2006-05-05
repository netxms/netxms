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
** $module: nxsnmpwalk.cpp
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

int GetData(char *pszHost, char *pszRootOid)
{
   SNMP_Transport *pTransport;
   SNMP_PDU *pRqPDU, *pRespPDU;
   DWORD dwResult, dwRootLen, dwNameLen;
   DWORD pdwRootName[MAX_OID_LEN], pdwName[MAX_OID_LEN];
   TCHAR szBuffer[1024];
   int i, iExit = 0;
   BOOL bRunning = TRUE;

   // Initialize WinSock
#ifdef _WIN32
   WSADATA wsaData;
   WSAStartup(2, &wsaData);
#endif

   // Create SNMP transport
   pTransport = new SNMP_Transport;
   dwResult = pTransport->CreateUDPTransport(pszHost, 0, m_wPort);
   if (dwResult != SNMP_ERR_SUCCESS)
   {
      printf("Unable to create UDP transport: %s\n", SNMPGetErrorText(dwResult));
      iExit = 2;
   }
   else
   {
      // Get root
      dwRootLen = SNMPParseOID(pszRootOid, pdwRootName, MAX_OID_LEN);
      if (dwRootLen == 0)
      {
         dwResult = SNMP_ERR_BAD_OID;
      }
      else
      {
         memcpy(pdwName, pdwRootName, dwRootLen * sizeof(DWORD));
         dwNameLen = dwRootLen;

         // Walk the MIB
         while(bRunning)
         {
            pRqPDU = new SNMP_PDU(SNMP_GET_NEXT_REQUEST, m_szCommunity, getpid(), m_dwVersion);
            pRqPDU->BindVariable(new SNMP_Variable(pdwName, dwNameLen));
            dwResult = pTransport->DoRequest(pRqPDU, &pRespPDU, m_dwTimeout, 3);

            // Analyze response
            if (dwResult == SNMP_ERR_SUCCESS)
            {
               if ((pRespPDU->GetNumVariables() > 0) &&
                   (pRespPDU->GetErrorCode() == 0))
               {
                  SNMP_Variable *pVar = pRespPDU->GetVariable(0);

                  if ((pVar->GetType() != ASN_NO_SUCH_OBJECT) &&
                      (pVar->GetType() != ASN_NO_SUCH_INSTANCE))
                  {
                     // Should we stop walking?
                     if ((pVar->GetName()->Length() < dwRootLen) ||
                         (memcmp(pdwRootName, pVar->GetName()->GetValue(), dwRootLen * sizeof(DWORD))) ||
                         ((pVar->GetName()->Length() == dwNameLen) &&
                          (!memcmp(pVar->GetName()->GetValue(), pdwName, pVar->GetName()->Length() * sizeof(DWORD)))))
                     {
                        bRunning = FALSE;
                        delete pRespPDU;
                        delete pRqPDU;
                        break;
                     }
                     memcpy(pdwName, pVar->GetName()->GetValue(), 
                            pVar->GetName()->Length() * sizeof(DWORD));
                     dwNameLen = pVar->GetName()->Length();

                     // Print OID and value
                     pVar->GetValueAsString(szBuffer, 1024);
                     for(i = 0; szBuffer[i] != 0; i++)
                        if (szBuffer[i] < ' ')
                           szBuffer[i] = '.';
                     printf("%s [%02X]: %s\n", pVar->GetName()->GetValueAsText(),
                            pVar->GetType(), szBuffer);
                  }
                  else
                  {
                     dwResult = SNMP_ERR_NO_OBJECT;
                     bRunning = FALSE;
                  }
               }
               else
               {
                  dwResult = SNMP_ERR_AGENT;
                  bRunning = FALSE;
               }
               delete pRespPDU;
            }
            else
            {
               bRunning = FALSE;
            }
            delete pRqPDU;
         }
      }

      if (dwResult != SNMP_ERR_SUCCESS)
      {
         printf("SNMP Error: %s\n", SNMPGetErrorText(dwResult));
         iExit = 3;
      }
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
            printf("Usage: nxsnmpwalk [<options>] <host> <start_oid>\n"
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
      if (argc - optind < 2)
      {
         printf("Required argument(s) missing.\nUse nxsnmpwalk -h to get complete command line syntax.\n");
      }
      else
      {
         iExit = GetData(argv[optind], argv[optind + 1]);
      }
   }

   return iExit;
}
