/* 
** nxsnmpwalk - command line tool used to retrieve parameters from SNMP agent
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
** File: nxsnmpwalk.cpp
**
**/

#include <nms_common.h>
#include <nms_agent.h>
#include <nms_util.h>
#include <nxsnmp.h>

NETXMS_EXECUTABLE_HEADER(nxsnmpwalk)

/**
 * Static data
 */
static char m_community[256] = "public";
static char m_user[256] = "";
static char m_authPassword[256] = "";
static char m_encryptionPassword[256] = "";
static char m_contextName[256] = "";
static int m_authMethod = SNMP_AUTH_NONE;
static int m_encryptionMethod = SNMP_ENCRYPT_NONE;
static UINT16 m_port = 161;
static SNMP_Version m_snmpVersion = SNMP_VERSION_2C;
static UINT32 m_timeout = 3000;

/**
 * Walk callback
 */
static UINT32 WalkCallback(SNMP_Variable *var, SNMP_Transport *transport, void *userArg)
{
   TCHAR buffer[1024], typeName[256];
   bool convert = true;
   var->getValueAsPrintableString(buffer, 1024, &convert);
   _tprintf(_T("%s [%s]: %s\n"), (const TCHAR *)var->getName().toString(),
            convert ? _T("Hex-STRING") : SNMPDataTypeName(var->getType(), typeName, 256),
            buffer);
   return SNMP_ERR_SUCCESS;
}

/**
 * Get data
 */
static int DoWalk(TCHAR *pszHost, TCHAR *pszRootOid)
{
   // Initialize WinSock
#ifdef _WIN32
   WSADATA wsaData;
   WSAStartup(2, &wsaData);
#endif

   // Create SNMP transport
   SNMP_UDPTransport *transport = new SNMP_UDPTransport();
   UINT32 dwResult = transport->createUDPTransport(pszHost, m_port);
   if (dwResult != SNMP_ERR_SUCCESS)
   {
      _tprintf(_T("Unable to create UDP transport: %s\n"), SNMPGetErrorText(dwResult));
      delete transport;
      return 2;
   }

   transport->setSnmpVersion(m_snmpVersion);
   if (m_snmpVersion == SNMP_VERSION_3)
   {
      SNMP_SecurityContext *context = new SNMP_SecurityContext(m_user, m_authPassword, m_encryptionPassword, m_authMethod, m_encryptionMethod);
      if (m_contextName[0] != 0)
         context->setContextNameA(m_contextName);
      transport->setSecurityContext(context);
   }
   else
   {
      transport->setSecurityContext(new SNMP_SecurityContext(m_community));
   }

   int iExit = 0;
   dwResult = SnmpWalk(transport, pszRootOid, WalkCallback, NULL);
   if (dwResult != SNMP_ERR_SUCCESS)
   {
      _tprintf(_T("SNMP Error: %s\n"), SNMPGetErrorText(dwResult));
      iExit = 3;
   }

   delete transport;
   return iExit;
}

/**
 * Startup
 */
int main(int argc, char *argv[])
{
   int ch, iExit = 1;
   UINT32 dwValue;
   char *eptr;
   BOOL bStart = TRUE;

   InitNetXMSProcess(true);

   // Parse command line
   opterr = 1;
	while((ch = getopt(argc, argv, "a:A:c:e:E:hn:p:u:v:w:")) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
            _tprintf(_T("Usage: nxsnmpwalk [<options>] <host> <start_oid>\n")
                     _T("Valid options are:\n")
						   _T("   -a <method>  : Authentication method for SNMP v3 USM. Valid methods are MD5 and SHA1\n")
                     _T("   -A <passwd>  : User's authentication password for SNMP v3 USM\n")
                     _T("   -c <string>  : Community string. Default is \"public\"\n")
						   _T("   -e <method>  : Encryption method for SNMP v3 USM. Valid methods are DES and AES\n")
                     _T("   -E <passwd>  : User's encryption password for SNMP v3 USM\n")
                     _T("   -h           : Display help and exit\n")
						   _T("   -n <name>    : SNMP v3 context name\n")
                     _T("   -p <port>    : Agent's port number. Default is 161\n")
                     _T("   -u <user>    : User name for SNMP v3 USM\n")
                     _T("   -v <version> : SNMP version to use (valid values is 1, 2c, and 3)\n")
                     _T("   -w <seconds> : Request timeout (default is 3 seconds)\n")
                     _T("\n"));
            iExit = 0;
            bStart = FALSE;
            break;
         case 'c':   // Community
            strncpy(m_community, optarg, 256);
				m_community[255] = 0;
            break;
         case 'u':   // User
            strncpy(m_user, optarg, 256);
				m_user[255] = 0;
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
            strncpy(m_authPassword, optarg, 256);
				m_authPassword[255] = 0;
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
            strncpy(m_encryptionPassword, optarg, 256);
				m_encryptionPassword[255] = 0;
				if (strlen(m_encryptionPassword) < 8)
				{
               _tprintf(_T("Encryption password should be at least 8 characters long\n"));
					bStart = FALSE;
				}
            break;
         case 'n':   // context name
            strncpy(m_contextName, optarg, 256);
				m_contextName[255] = 0;
            break;
         case 'p':   // Port number
            dwValue = strtoul(optarg, &eptr, 0);
            if ((*eptr != 0) || (dwValue > 65535) || (dwValue == 0))
            {
               _tprintf(_T("Invalid port number %hs\n"), optarg);
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
               _tprintf(_T("Invalid SNMP version %hs\n"), optarg);
               bStart = FALSE;
            }
            break;
         case 'w':   // Timeout
            dwValue = strtoul(optarg, &eptr, 0);
            if ((*eptr != 0) || (dwValue > 60) || (dwValue == 0))
            {
               _tprintf(_T("Invalid timeout value %hs\n"), optarg);
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
      if (argc - optind < 2)
      {
         _tprintf(_T("Required argument(s) missing.\nUse nxsnmpwalk -h to get complete command line syntax.\n"));
      }
      else
      {
#ifdef UNICODE
			WCHAR *host = WideStringFromMBStringSysLocale(argv[optind]);
			WCHAR *rootOid = WideStringFromMBStringSysLocale(argv[optind + 1]);
         iExit = DoWalk(host, rootOid);
			free(host);
			free(rootOid);
#else
         iExit = DoWalk(argv[optind], argv[optind + 1]);
#endif
      }
   }

   return iExit;
}
