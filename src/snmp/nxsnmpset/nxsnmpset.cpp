/* 
** nxsnmpset - command line tool used to set parameters on SNMP agent
** Copyright (C) 2004-2020 Victor Kirhenshtein
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
#include <netxms_getopt.h>
#include <nxsnmp.h>

NETXMS_EXECUTABLE_HEADER(nxsnmpset)

//
// Static data
//
static char m_community[256] = "public";
static char m_user[256] = "";
static char m_authPassword[256] = "";
static char m_encryptionPassword[256] = "";
static SNMP_AuthMethod m_authMethod = SNMP_AUTH_NONE;
static SNMP_EncryptionMethod m_encryptionMethod = SNMP_ENCRYPT_NONE;
static uint16_t m_port = 161;
static SNMP_Version m_snmpVersion = SNMP_VERSION_2C;
static uint32_t m_timeout = 3000;
static uint32_t m_type = ASN_OCTET_STRING;

/**
 * Set variables
 */
static int SetVariables(int argc, TCHAR *argv[])
{
   int iExit = 0;

   // Initialize WinSock
#ifdef _WIN32
   WSADATA wsaData;
   WSAStartup(2, &wsaData);
#endif

   // Create SNMP transport
   auto pTransport = new SNMP_UDPTransport;
   uint32_t dwResult = pTransport->createUDPTransport(argv[0], m_port);
   if (dwResult != SNMP_ERR_SUCCESS)
   {
      _tprintf(_T("Unable to create UDP transport: %s\n"), SNMPGetErrorText(dwResult));
      iExit = 2;
   }
   else
   {
      pTransport->setSnmpVersion(m_snmpVersion);
		if (m_snmpVersion == SNMP_VERSION_3)
			pTransport->setSecurityContext(new SNMP_SecurityContext(m_user, m_authPassword, m_encryptionPassword, m_authMethod, m_encryptionMethod));
		else
			pTransport->setSecurityContext(new SNMP_SecurityContext(m_community));

		// Create request
      auto request = new SNMP_PDU(SNMP_SET_REQUEST, GetCurrentProcessId(), m_snmpVersion);
      for(int i = 1; i < argc; i += 2)
      {
         TCHAR *p = _tcschr(argv[i], _T('@'));
         UINT32 type = m_type;
         if (p != NULL)
         {
            *p = 0;
            p++;
            type = SNMPResolveDataType(p);
            if (type == ASN_NULL)
            {
               _tprintf(_T("Invalid data type: %s\n"), p);
               iExit = 5;
            }
         }
         if (SNMPIsCorrectOID(argv[i]))
         {
            auto pVar = new SNMP_Variable(argv[i]);
            pVar->setValueFromString(type, argv[i + 1]);
            request->bindVariable(pVar);
         }
         else
         {
            _tprintf(_T("Invalid OID: %s\n"), argv[i]);
            iExit = 4;
         }
      }

      // Send request and process response
      if (iExit == 0)
      {
         SNMP_PDU *response;
         if ((dwResult = pTransport->doRequest(request, &response, m_timeout, 3)) == SNMP_ERR_SUCCESS)
         {
            if (response->getErrorCode() != 0)
            {
               _tprintf(_T("SET operation failed (%s)\n"), SNMPGetProtocolErrorText(response->getErrorCode()));
            }
            delete response;
         }
         else
         {
            _tprintf(_T("%s\n"), SNMPGetErrorText(dwResult));
            iExit = 3;
         }
      }

      delete request;
   }

   delete pTransport;
   return iExit;
}

/**
 * Startup
 */
int main(int argc, char *argv[])
{
   int ch, iExit = 1;
   uint32_t value;
   char *eptr;
   BOOL bStart = TRUE;

   InitNetXMSProcess(true);

   // Parse command line
   opterr = 1;
	while((ch = getopt(argc, argv, "a:A:c:e:E:hp:t:u:v:w:")) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
            _tprintf(_T("Usage: nxsnmpset [<options>] <host> <variable>[@<type>] <value>\n")
                     _T("Valid options are:\n")
						   _T("   -a <method>  : Authentication method for SNMP v3 USM. Valid methods are MD5, SHA1, SHA224, SHA256, SHA384, SHA512\n")
                     _T("   -A <passwd>  : User's authentication password for SNMP v3 USM\n")
                     _T("   -c <string>  : Community string. Default is \"public\"\n")
						   _T("   -e <method>  : Encryption method for SNMP v3 USM. Valid methods are DES and AES\n")
                     _T("   -E <passwd>  : User's encryption password for SNMP v3 USM\n")
                     _T("   -h           : Display help and exit\n")
                     _T("   -p <port>    : Agent's port number. Default is 161\n")
                     _T("   -t <type>    : Specify variable's data type. Default is octet string.\n")
                     _T("   -u <user>    : User name for SNMP v3 USM\n")
                     _T("   -v <version> : SNMP version to use (valid values is 1, 2c, and 3)\n")
                     _T("   -w <seconds> : Request timeout (default is 3 seconds)\n")
                     _T("Note: You can specify data type either as number or in symbolic form.\n")
                     _T("      Valid symbolic representations are following:\n")
                     _T("         INTEGER, STRING, OID, IPADDR, COUNTER32, GAUGE32,\n")
                     _T("         TIMETICKS, COUNTER64, UINT32.\n")
                     _T("\n"));
            iExit = 0;
            bStart = FALSE;
            break;
         case 't':
            // First, check for numeric representation
            value = strtoul(optarg, &eptr, 0);
            if (*eptr != 0)
            {
               // Try to resolve from symbolic form
#ifdef UNICODE
					WCHAR *wdt = WideStringFromMBString(optarg);
               value = SNMPResolveDataType(wdt);
					MemFree(wdt);
#else
               value = SNMPResolveDataType(optarg);
#endif
               if (value == ASN_NULL)
               {
                  _tprintf(_T("Invalid data type %hs\n"), optarg);
                  bStart = FALSE;
               }
               else
               {
                  m_type = value;
               }
            }
            else
            {
               m_type = value;
            }
            break;
         case 'c':   // Community
            strlcpy(m_community, optarg, 256);
            break;
         case 'u':   // User
            strlcpy(m_user, optarg, 256);
            break;
			case 'a':   // authentication method
				if (!stricmp(optarg, "md5"))
				{
					m_authMethod = SNMP_AUTH_MD5;
				}
				else if (!stricmp(optarg, "sha1"))
				{
					m_authMethod = SNMP_AUTH_SHA1;
				}
            else if (!stricmp(optarg, "sha224"))
            {
               m_authMethod = SNMP_AUTH_SHA224;
            }
            else if (!stricmp(optarg, "sha256"))
            {
               m_authMethod = SNMP_AUTH_SHA256;
            }
            else if (!stricmp(optarg, "sha384"))
            {
               m_authMethod = SNMP_AUTH_SHA384;
            }
            else if (!stricmp(optarg, "sha512"))
            {
               m_authMethod = SNMP_AUTH_SHA512;
            }
				else if (!stricmp(optarg, "none"))
				{
					m_authMethod = SNMP_AUTH_NONE;
				}
				else
				{
               _tprintf(_T("Invalid authentication method %hs\n"), optarg);
					bStart = FALSE;
				}
				break;
         case 'A':   // authentication password
            strlcpy(m_authPassword, optarg, 256);
				if (strlen(m_authPassword) < 8)
				{
               _tprintf(_T("Authentication password should be at least 8 characters long\n"));
					bStart = FALSE;
				}
            break;
			case 'e':   // encryption method
				if (!stricmp(optarg, "des"))
				{
					m_encryptionMethod = SNMP_ENCRYPT_DES;
				}
				else if (!stricmp(optarg, "aes"))
				{
					m_encryptionMethod = SNMP_ENCRYPT_AES;
				}
				else if (!stricmp(optarg, "none"))
				{
					m_encryptionMethod = SNMP_ENCRYPT_NONE;
				}
				else
				{
               _tprintf(_T("Invalid encryption method %hs\n"), optarg);
					bStart = FALSE;
				}
				break;
         case 'E':   // encription password
            strlcpy(m_encryptionPassword, optarg, 256);
				if (strlen(m_encryptionPassword) < 8)
				{
               _tprintf(_T("Encryption password should be at least 8 characters long\n"));
					bStart = FALSE;
				}
            break;
         case 'p':   // Port number
            value = strtoul(optarg, &eptr, 0);
            if ((*eptr != 0) || (value > 65535) || (value == 0))
            {
               _tprintf(_T("Invalid port number %hs\n"), optarg);
               bStart = FALSE;
            }
            else
            {
               m_port = static_cast<uint16_t>(value);
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
               _tprintf(_T("Invalid SNMP version %hs\n"), optarg);
               bStart = FALSE;
            }
            break;
         case 'w':   // Timeout
            value = strtoul(optarg, &eptr, 0);
            if ((*eptr != 0) || (value > 60) || (value == 0))
            {
               _tprintf(_T("Invalid timeout value %hs\n"), optarg);
               bStart = FALSE;
            }
            else
            {
               m_timeout = value;
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
         _tprintf(_T("Required argument(s) missing.\nUse nxsnmpset -h to get complete command line syntax.\n"));
      }
      else if ((argc - optind) % 2 != 1)
      {
         _tprintf(_T("Missing value for variable.\nUse nxsnmpset -h to get complete command line syntax.\n"));
      }
      else
      {
#ifdef UNICODE
			WCHAR *wargv[256];
			for(int i = optind; i < argc; i++)
				wargv[i - optind] = WideStringFromMBStringSysLocale(argv[i]);
         iExit = SetVariables(argc - optind, wargv);
			for(int i = 0; i < argc - optind; i++)
				MemFree(wargv[i]);
#else
         iExit = SetVariables(argc - optind, &argv[optind]);
#endif
      }
   }

   return iExit;
}
