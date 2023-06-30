/* 
** nxsnmpset - command line tool used to set parameters on SNMP agent
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
** File: nxsnmpset.cpp
**
**/

#include <nms_common.h>
#include <nms_agent.h>
#include <nms_util.h>
#include <netxms-version.h>
#include <netxms_getopt.h>
#include <nxsnmp.h>

NETXMS_EXECUTABLE_HEADER(nxsnmpset)

//
// Static data
//
static char s_community[256] = "public";
static char s_user[256] = "";
static char s_authPassword[256] = "";
static char s_encryptionPassword[256] = "";
static SNMP_AuthMethod s_authMethod = SNMP_AUTH_NONE;
static SNMP_EncryptionMethod s_encryptionMethod = SNMP_ENCRYPT_NONE;
static uint16_t s_port = 161;
static SNMP_Version s_snmpVersion = SNMP_VERSION_2C;
static uint32_t s_timeout = 3000;
static uint32_t s_type = ASN_OCTET_STRING;
static bool s_base64 = false;
static bool s_hexString = false;

/**
 * Set variables
 */
static int SetVariables(int argc, TCHAR *argv[])
{
   int exitCode = 0;

   // Create SNMP transport
   auto transport = new SNMP_UDPTransport;
   uint32_t snmpResult = transport->createUDPTransport(argv[0], s_port);
   if (snmpResult == SNMP_ERR_SUCCESS)
   {
      transport->setSnmpVersion(s_snmpVersion);
		if (s_snmpVersion == SNMP_VERSION_3)
			transport->setSecurityContext(new SNMP_SecurityContext(s_user, s_authPassword, s_encryptionPassword, s_authMethod, s_encryptionMethod));
		else
			transport->setSecurityContext(new SNMP_SecurityContext(s_community));

		// Create request
      auto request = new SNMP_PDU(SNMP_SET_REQUEST, GetCurrentProcessId(), s_snmpVersion);
      for(int i = 1; i < argc; i += 2)
      {
         TCHAR *p = _tcschr(argv[i], _T('@'));
         uint32_t type = s_type;
         if (p != nullptr)
         {
            *p = 0;
            p++;
            type = SnmpResolveDataType(p);
            if (type == ASN_NULL)
            {
               _tprintf(_T("Invalid data type: %s\n"), p);
               exitCode = 5;
            }
         }
         if (SnmpIsCorrectOID(argv[i]))
         {
            auto varbind = new SNMP_Variable(argv[i]);
            if (s_hexString)
            {
               BYTE value[4096];
               size_t size = StrToBin(argv[i + 1], value, sizeof(value));
               varbind->setValueFromByteArray(type, value, size);
            }
            else if (s_base64)
            {
#ifdef UNICODE
               char *in = MBStringFromWideString(argv[i + 1]);
#else
               const char *in = argv[i + 1];
#endif
               size_t ilen = strlen(in);
               size_t olen = 3 * (ilen / 4) + 8;
               char *out = MemAllocArray<char>(olen);
               if (base64_decode(in, ilen, out, &olen))
               {
                  varbind->setValueFromByteArray(type, reinterpret_cast<BYTE*>(out), olen);
               }
               MemFree(out);
#ifdef UNICODE
               MemFree(in);
#endif
            }
            else
            {
               varbind->setValueFromString(type, argv[i + 1]);
            }
            request->bindVariable(varbind);
         }
         else
         {
            _tprintf(_T("Invalid OID: %s\n"), argv[i]);
            exitCode = 4;
         }
      }

      // Send request and process response
      if (exitCode == 0)
      {
         SNMP_PDU *response;
         if ((snmpResult = transport->doRequest(request, &response, s_timeout, SnmpGetDefaultRetryCount())) == SNMP_ERR_SUCCESS)
         {
            if (response->getErrorCode() != 0)
            {
               _tprintf(_T("SET operation failed (%s)\n"), SnmpGetProtocolErrorText(response->getErrorCode()));
            }
            delete response;
         }
         else
         {
            _tprintf(_T("%s\n"), SnmpGetErrorText(snmpResult));
            exitCode = 3;
         }
      }

      delete request;
   }
   else
   {
      _tprintf(_T("Unable to create UDP transport: %s\n"), SnmpGetErrorText(snmpResult));
      exitCode = 2;
   }

   delete transport;
   return exitCode;
}

/**
 * Startup
 */
int main(int argc, char *argv[])
{
   int ch, exitCode = 1;
   uint32_t value;
   char *eptr;
   bool start = true;

   InitNetXMSProcess(true);

   // Parse command line
   opterr = 1;
	while((ch = getopt(argc, argv, "a:A:Bc:e:E:hHp:t:u:v:w:")) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
            _tprintf(_T("Usage: nxsnmpset [<options>] <host> <variable>[@<type>] <value>\n")
                     _T("Valid options are:\n")
						   _T("   -a <method>  : Authentication method for SNMP v3 USM. Valid methods are MD5, SHA1, SHA224, SHA256, SHA384, SHA512\n")
                     _T("   -A <passwd>  : User's authentication password for SNMP v3 USM\n")
                     _T("   -B           : Provided value is a base64 encoded raw value\n")
                     _T("   -c <string>  : Community string. Default is \"public\"\n")
						   _T("   -e <method>  : Encryption method for SNMP v3 USM. Valid methods are DES and AES\n")
                     _T("   -E <passwd>  : User's encryption password for SNMP v3 USM\n")
                     _T("   -h           : Display help and exit\n")
                     _T("   -H           : Provided value is a raw value encoded as hexadecimal string\n")
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
            exitCode = 0;
            start = false;
            break;
         case 't':
            // First, check for numeric representation
            value = strtoul(optarg, &eptr, 0);
            if (*eptr != 0)
            {
               // Try to resolve from symbolic form
#ifdef UNICODE
					WCHAR *wdt = WideStringFromMBString(optarg);
               value = SnmpResolveDataType(wdt);
					MemFree(wdt);
#else
               value = SnmpResolveDataType(optarg);
#endif
               if (value == ASN_NULL)
               {
                  _tprintf(_T("Invalid data type %hs\n"), optarg);
                  start = false;
               }
               else
               {
                  s_type = value;
               }
            }
            else
            {
               s_type = value;
            }
            break;
         case 'c':   // Community
            strlcpy(s_community, optarg, 256);
            break;
         case 'u':   // User
            strlcpy(s_user, optarg, 256);
            break;
			case 'a':   // authentication method
				if (!stricmp(optarg, "md5"))
				{
					s_authMethod = SNMP_AUTH_MD5;
				}
				else if (!stricmp(optarg, "sha1"))
				{
					s_authMethod = SNMP_AUTH_SHA1;
				}
            else if (!stricmp(optarg, "sha224"))
            {
               s_authMethod = SNMP_AUTH_SHA224;
            }
            else if (!stricmp(optarg, "sha256"))
            {
               s_authMethod = SNMP_AUTH_SHA256;
            }
            else if (!stricmp(optarg, "sha384"))
            {
               s_authMethod = SNMP_AUTH_SHA384;
            }
            else if (!stricmp(optarg, "sha512"))
            {
               s_authMethod = SNMP_AUTH_SHA512;
            }
				else if (!stricmp(optarg, "none"))
				{
					s_authMethod = SNMP_AUTH_NONE;
				}
				else
				{
               _tprintf(_T("Invalid authentication method %hs\n"), optarg);
					start = false;
				}
				break;
         case 'A':   // authentication password
            strlcpy(s_authPassword, optarg, 256);
				if (strlen(s_authPassword) < 8)
				{
               _tprintf(_T("Authentication password should be at least 8 characters long\n"));
					start = false;
				}
            break;
			case 'e':   // encryption method
				if (!stricmp(optarg, "des"))
				{
					s_encryptionMethod = SNMP_ENCRYPT_DES;
				}
            else if (!stricmp(optarg, "aes"))
            {
               s_encryptionMethod = SNMP_ENCRYPT_AES_128;
            }
            else if (!stricmp(optarg, "aes-128"))
            {
               s_encryptionMethod = SNMP_ENCRYPT_AES_128;
            }
            else if (!stricmp(optarg, "aes-192"))
            {
               s_encryptionMethod = SNMP_ENCRYPT_AES_192;
            }
            else if (!stricmp(optarg, "aes-256"))
            {
               s_encryptionMethod = SNMP_ENCRYPT_AES_256;
            }
				else if (!stricmp(optarg, "none"))
				{
					s_encryptionMethod = SNMP_ENCRYPT_NONE;
				}
				else
				{
               _tprintf(_T("Invalid encryption method %hs\n"), optarg);
					start = false;
				}
				break;
         case 'E':   // encription password
            strlcpy(s_encryptionPassword, optarg, 256);
				if (strlen(s_encryptionPassword) < 8)
				{
               _tprintf(_T("Encryption password should be at least 8 characters long\n"));
					start = false;
				}
            break;
         case 'p':   // Port number
            value = strtoul(optarg, &eptr, 0);
            if ((*eptr != 0) || (value > 65535) || (value == 0))
            {
               _tprintf(_T("Invalid port number %hs\n"), optarg);
               start = false;
            }
            else
            {
               s_port = static_cast<uint16_t>(value);
            }
            break;
         case 'v':   // Version
            if (!strcmp(optarg, "1"))
            {
               s_snmpVersion = SNMP_VERSION_1;
            }
            else if (!stricmp(optarg, "2c"))
            {
               s_snmpVersion = SNMP_VERSION_2C;
            }
            else if (!stricmp(optarg, "3"))
            {
               s_snmpVersion = SNMP_VERSION_3;
            }
            else
            {
               _tprintf(_T("Invalid SNMP version %hs\n"), optarg);
               start = false;
            }
            break;
         case 'w':   // Timeout
            value = strtoul(optarg, &eptr, 0);
            if ((*eptr != 0) || (value > 60) || (value == 0))
            {
               _tprintf(_T("Invalid timeout value %hs\n"), optarg);
               start = false;
            }
            else
            {
               s_timeout = value;
            }
            break;
         case 'B':
            s_base64 = true;
            break;
         case 'H':
            s_hexString = true;
            break;
         case '?':
            start = false;
            break;
         default:
            break;
      }
   }

   if (start && s_base64 && s_hexString)
   {
      start = false;
      _tprintf(_T("Options -B and -H are mutually exclusive.\n"));
   }

   if (start)
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
         // Initialize WinSock
#ifdef _WIN32
         WSADATA wsaData;
         WSAStartup(2, &wsaData);
#endif

#ifdef UNICODE
			WCHAR *wargv[256];
			for(int i = optind; i < argc; i++)
				wargv[i - optind] = WideStringFromMBStringSysLocale(argv[i]);
         exitCode = SetVariables(argc - optind, wargv);
			for(int i = 0; i < argc - optind; i++)
				MemFree(wargv[i]);
#else
         exitCode = SetVariables(argc - optind, &argv[optind]);
#endif
      }
   }

   return exitCode;
}
