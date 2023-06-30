/* 
** nxsnmpwalk - command line tool used to retrieve parameters from SNMP agent
** Copyright (C) 2004-2023 Victor Kirhenshtein
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
#include <netxms-version.h>
#include <netxms_getopt.h>
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
static SNMP_AuthMethod m_authMethod = SNMP_AUTH_NONE;
static SNMP_EncryptionMethod m_encryptionMethod = SNMP_ENCRYPT_NONE;
static uint16_t m_port = 161;
static SNMP_Version m_snmpVersion = SNMP_VERSION_2C;
static uint32_t m_timeout = 3000;
static const char *s_codepage = nullptr;

/**
 * Walk callback
 */
static uint32_t WalkCallback(SNMP_Variable *var, SNMP_Transport *transport, void *context)
{
   TCHAR buffer[1024], typeName[256];
   if (var->getType() == ASN_OPAQUE)
   {
      SNMP_Variable *subvar = var->decodeOpaque();
      if (subvar != nullptr)
      {
         bool convert = true;
         subvar->getValueAsPrintableString(buffer, 1024, &convert, s_codepage);
         _tprintf(_T("%s [OPAQUE]: [%s]: %s\n"), (const TCHAR *)var->getName().toString(),
               convert ? _T("Hex-STRING") : SnmpDataTypeName(subvar->getType(), typeName, 256), buffer);
         delete subvar;
      }
      else
      {
         _tprintf(_T("%s [OPAQUE]:\n"), var->getName().toString().cstr());
      }
   }
   else
   {
      bool convert = true;
      var->getValueAsPrintableString(buffer, 1024, &convert, s_codepage);
      _tprintf(_T("%s [%s]: %s\n"), var->getName().toString().cstr(),
            convert ? _T("Hex-STRING") : SnmpDataTypeName(var->getType(), typeName, 256), buffer);
   }
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
   SNMP_UDPTransport transport;
   uint32_t result = transport.createUDPTransport(pszHost, m_port);
   if (result != SNMP_ERR_SUCCESS)
   {
      _tprintf(_T("Unable to create UDP transport: %s\n"), SnmpGetErrorText(result));
      return 2;
   }

   transport.setSnmpVersion(m_snmpVersion);
   if (m_snmpVersion == SNMP_VERSION_3)
   {
      SNMP_SecurityContext *context = new SNMP_SecurityContext(m_user, m_authPassword, m_encryptionPassword, m_authMethod, m_encryptionMethod);
      if (m_contextName[0] != 0)
         context->setContextName(m_contextName);
      transport.setSecurityContext(context);
   }
   else
   {
      transport.setSecurityContext(new SNMP_SecurityContext(m_community));
   }

   result = SnmpWalk(&transport, pszRootOid, WalkCallback, nullptr);
   if (result != SNMP_ERR_SUCCESS)
   {
      _tprintf(_T("SNMP Error: %s\n"), SnmpGetErrorText(result));
      return 3;
   }

   return 0;
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
	while((ch = getopt(argc, argv, "a:A:c:C:e:E:hn:p:u:v:w:")) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
            _tprintf(_T("Usage: nxsnmpwalk [<options>] <host> <start_oid>\n")
                     _T("Valid options are:\n")
                     _T("   -a <method>   : Authentication method for SNMP v3 USM. Valid methods are MD5, SHA1, SHA224, SHA256, SHA384, SHA512\n")
                     _T("   -A <passwd>   : User's authentication password for SNMP v3 USM\n")
                     _T("   -c <string>   : Community string. Default is \"public\"\n")
                     _T("   -C <codepage> : Codepage for remote system\n")
						   _T("   -e <method>   : Encryption method for SNMP v3 USM. Valid methods are DES and AES\n")
                     _T("   -E <passwd>   : User's encryption password for SNMP v3 USM\n")
                     _T("   -h            : Display help and exit\n")
						   _T("   -n <name>     : SNMP v3 context name\n")
                     _T("   -p <port>     : Agent's port number. Default is 161\n")
                     _T("   -u <user>     : User name for SNMP v3 USM\n")
                     _T("   -v <version>  : SNMP version to use (valid values is 1, 2c, and 3)\n")
                     _T("   -w <seconds>  : Request timeout (default is 3 seconds)\n")
                     _T("\n"));
            iExit = 0;
            bStart = FALSE;
            break;
         case 'c':   // Community
            strlcpy(m_community, optarg, 256);
            break;
         case 'C':
            s_codepage = optarg;
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
               m_encryptionMethod = SNMP_ENCRYPT_AES_128;
            }
            else if (!stricmp(optarg, "aes-128"))
            {
               m_encryptionMethod = SNMP_ENCRYPT_AES_128;
            }
            else if (!stricmp(optarg, "aes-192"))
            {
               m_encryptionMethod = SNMP_ENCRYPT_AES_192;
            }
            else if (!stricmp(optarg, "aes-256"))
            {
               m_encryptionMethod = SNMP_ENCRYPT_AES_256;
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
         case 'n':   // context name
            strlcpy(m_contextName, optarg, 256);
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
			MemFree(host);
			MemFree(rootOid);
#else
         iExit = DoWalk(argv[optind], argv[optind + 1]);
#endif
      }
   }

   return iExit;
}
