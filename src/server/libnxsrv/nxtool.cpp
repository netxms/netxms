/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: nxtool.cpp
**
**/

#include "libnxsrv.h"

/**
 * Debug writer
 */
static void DebugWriter(const TCHAR *tag, const TCHAR *format, va_list args)
{
   if (tag != NULL)
      _tprintf(_T("<%s> "), tag);
   _vtprintf(format, args);
   _fputtc(_T('\n'), stdout);
}

/**
 * Run server-side command line tool.
 * Function initializes process, parses command line and creates connection.
 * parseAdditionalOptionCb is called for tool-specific attributes
 * validateArgCountCb is called for parameter count validation
 * executeCommandCb is called to execute command
 */
int ExecuteServerCommandLineTool(ServerCommandLineTool *tool)
{
   char *eptr;
   bool start = true, useProxy = false;
   int i, ch, iExitCode = 3, iInterval = 0;
   int authMethod = AUTH_NONE, proxyAuth = AUTH_NONE;
#ifdef _WITH_ENCRYPTION
   int iEncryptionPolicy = ENCRYPTION_PREFERRED;
#else
   int iEncryptionPolicy = ENCRYPTION_DISABLED;
#endif
   WORD agentPort = AGENT_LISTEN_PORT, proxyPort = AGENT_LISTEN_PORT;
   UINT32 dwTimeout = 5000, dwConnTimeout = 30000, dwError;
   TCHAR szSecret[MAX_SECRET_LENGTH] = _T(""), szRequest[MAX_DB_STRING] = _T("");
   TCHAR keyFile[MAX_PATH];
   TCHAR szResponse[MAX_DB_STRING] = _T("");
   char szProxy[MAX_OBJECT_NAME] = "";
   TCHAR szProxySecret[MAX_SECRET_LENGTH] = _T("");
   RSA *serverKey = NULL;

   InitNetXMSProcess(true);
   nxlog_set_debug_writer(DebugWriter);

   GetNetXMSDirectory(nxDirData, keyFile);
   _tcslcat(keyFile, DFILE_KEYS, MAX_PATH);

   // Parse command line
   opterr = 1;
   char options[128] = "a:A:D:e:hK:O:p:s:S:vw:W:X:";
   strlcat(options, tool->additionalOptions, 128);

   while((ch = getopt(tool->argc, tool->argv, options)) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
            _tprintf(_T("%s\n")
                     _T("Common options:\n")
                     _T("   -a auth      : Authentication method for agent. Valid methods are \"none\",\n")
                     _T("                  \"plain\", \"md5\" and \"sha1\". Default is \"none\".\n")
                     _T("   -A auth      : Authentication method for proxy agent.\n")
                     _T("   -D level     : Set debug level (default is 0).\n")
#ifdef _WITH_ENCRYPTION
                     _T("   -e policy    : Set encryption policy. Possible values are:\n")
                     _T("                    0 = Encryption disabled;\n")
                     _T("                    1 = Encrypt connection only if agent requires encryption;\n")
                     _T("                    2 = Encrypt connection if agent supports encryption;\n")
                     _T("                    3 = Force encrypted connection;\n")
                     _T("                  Default value is 1.\n")
#endif
                     _T("   -h           : Display help and exit.\n")
#ifdef _WITH_ENCRYPTION
                     _T("   -K file      : Specify server's key file\n")
                     _T("                  (default is %s).\n")
#endif
                     _T("   -O port      : Proxy agent's port number. Default is %d.\n")
                     _T("   -p port      : Agent's port number. Default is %d.\n")
                     _T("   -s secret    : Shared secret for agent authentication.\n")
                     _T("   -S secret    : Shared secret for proxy agent authentication.\n")
                     _T("   -v           : Display version and exit.\n")
                     _T("   -w seconds   : Set command timeout (default is 5 seconds).\n")
                     _T("   -W seconds   : Set connection timeout (default is 30 seconds).\n")
                     _T("   -X addr      : Use proxy agent at given address.\n")
                     _T("\n"),
                     tool->mainHelpText,
#ifdef _WITH_ENCRYPTION
                     keyFile,
#endif
                     agentPort, agentPort);
            start = false;
            break;
         case 'a':   // Auth method
         case 'A':
            if (!strcmp(optarg, "none"))
               i = AUTH_NONE;
            else if (!strcmp(optarg, "plain"))
               i = AUTH_PLAINTEXT;
            else if (!strcmp(optarg, "md5"))
               i = AUTH_MD5_HASH;
            else if (!strcmp(optarg, "sha1"))
               i = AUTH_SHA1_HASH;
            else
            {
               printf("Invalid authentication method \"%s\"\n", optarg);
               start = false;
            }
            if (ch == 'a')
               authMethod = i;
            else
               proxyAuth = i;
            break;
         case 'D':   // debug level
            nxlog_set_debug_level((int)strtol(optarg, NULL, 0));
            break;
         case 'p':   // Agent's port number
         case 'O':   // Proxy agent's port number
            i = strtol(optarg, &eptr, 0);
            if ((*eptr != 0) || (i < 0) || (i > 65535))
            {
               printf("Invalid port number \"%s\"\n", optarg);
               start = false;
            }
            else
            {
               if (ch == 'p')
                  agentPort = (WORD)i;
               else
                  proxyPort = (WORD)i;
            }
            break;
         case 's':   // Shared secret
#ifdef UNICODE
#if HAVE_MBSTOWCS
            mbstowcs(szSecret, optarg, MAX_SECRET_LENGTH);
#else
            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, szSecret, MAX_SECRET_LENGTH);
#endif
            szSecret[MAX_SECRET_LENGTH - 1] = 0;
#else
            strlcpy(szSecret, optarg, MAX_SECRET_LENGTH);
#endif
            break;
         case 'S':   // Shared secret for proxy agent
#ifdef UNICODE
#if HAVE_MBSTOWCS
            mbstowcs(szProxySecret, optarg, MAX_SECRET_LENGTH);
#else
            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, szProxySecret, MAX_SECRET_LENGTH);
#endif
            szProxySecret[MAX_SECRET_LENGTH - 1] = 0;
#else
            strlcpy(szProxySecret, optarg, MAX_SECRET_LENGTH);
#endif
            break;
         case 'v':   // Print version and exit
            _tprintf(_T("NetXMS GET command-line utility Version ") NETXMS_VERSION_STRING _T("\n"));
            start = false;
            break;
         case 'w':   // Command timeout
            i = strtol(optarg, &eptr, 0);
            if ((*eptr != 0) || (i < 1) || (i > 120))
            {
               _tprintf(_T("Invalid timeout \"%hs\"\n"), optarg);
               start = false;
            }
            else
            {
               dwTimeout = (UINT32)i * 1000;  // Convert to milliseconds
            }
            break;
         case 'W':   // Connection timeout
            i = strtol(optarg, &eptr, 0);
            if ((*eptr != 0) || (i < 1) || (i > 120))
            {
               printf("Invalid timeout \"%s\"\n", optarg);
               start = false;
            }
            else
            {
               dwConnTimeout = (UINT32)i * 1000;  // Convert to milliseconds
            }
            break;
#ifdef _WITH_ENCRYPTION
         case 'e':
            iEncryptionPolicy = atoi(optarg);
            if ((iEncryptionPolicy < 0) ||
                (iEncryptionPolicy > 3))
            {
               printf("Invalid encryption policy %d\n", iEncryptionPolicy);
               start = false;
            }
            break;
         case 'K':
#ifdef UNICODE
#if HAVE_MBSTOWCS
            mbstowcs(keyFile, optarg, MAX_PATH);
#else
            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, keyFile, MAX_PATH);
#endif
            keyFile[MAX_PATH - 1] = 0;
#else
            strlcpy(keyFile, optarg, MAX_PATH);
#endif
            break;
#else
         case 'e':
         case 'K':
            printf("ERROR: This tool was compiled without encryption support\n");
            start = false;
            break;
#endif
         case 'X':   // Use proxy
            strncpy(szProxy, optarg, MAX_OBJECT_NAME);
            szProxy[MAX_OBJECT_NAME - 1] = 0;
            useProxy = true;
            break;
         case '?':
            start = false;
            break;
         default:
            start = tool->parseAdditionalOptionCb(ch, optarg);
            break;
      }
   }

   // Check parameter correctness
   if (start)
   {
      if (tool->isArgMissingCb(tool->argc - optind))
      {
         printf("Required argument(s) missing.\nUse nxget -h to get complete command line syntax.\n");
         start = false;
      }
      else if ((authMethod != AUTH_NONE) && (szSecret[0] == 0))
      {
         printf("Shared secret not specified or empty\n");
         start = false;
      }

      // Load server key if requested
#ifdef _WITH_ENCRYPTION
      if ((iEncryptionPolicy != ENCRYPTION_DISABLED) && start)
      {
         if (InitCryptoLib(0xFFFF))
         {
            serverKey = LoadRSAKeys(keyFile);
            if (serverKey == NULL)
            {
               serverKey = RSAGenerateKey(2048);
               if (serverKey == NULL)
               {
                  _tprintf(_T("Cannot load server RSA key from \"%s\" or generate new key\n"), keyFile);
                  if (iEncryptionPolicy == ENCRYPTION_REQUIRED)
                     start = false;
               }
            }
         }
         else
         {
            printf("Error initializing cryptography module\n");
            if (iEncryptionPolicy == ENCRYPTION_REQUIRED)
               start = false;
         }
      }
#endif

      // If everything is ok, start communications
      if (start)
      {
         // Initialize WinSock
#ifdef _WIN32
         WSADATA wsaData;
         WSAStartup(2, &wsaData);
#endif
         InetAddress addr = InetAddress::resolveHostName(tool->argv[optind]);
         InetAddress proxyAddr = useProxy ? InetAddress::resolveHostName(szProxy) : InetAddress();
         if (!addr.isValid())
         {
            fprintf(stderr, "Invalid host name or address \"%s\"\n", tool->argv[optind]);
         }
         else if (useProxy && !proxyAddr.isValid())
         {
            fprintf(stderr, "Invalid host name or address \"%s\"\n", szProxy);
         }
         else
         {
            DecryptPassword(_T("netxms"), szSecret, szSecret, MAX_SECRET_LENGTH);
            AgentConnection *conn = new AgentConnection(addr, agentPort, authMethod, szSecret);

            conn->setConnectionTimeout(dwConnTimeout);
            conn->setCommandTimeout(dwTimeout);
            conn->setEncryptionPolicy(iEncryptionPolicy);
            if (useProxy)
               conn->setProxy(proxyAddr, proxyPort, proxyAuth, szProxySecret);
            if (conn->connect(serverKey, &dwError))
            {
               iExitCode = tool->executeCommandCb(conn, tool->argc, tool->argv, serverKey);
            }
            else
            {
               WriteToTerminalEx(_T("%d: %s\n"), dwError, AgentErrorCodeToText(dwError));
               iExitCode = 2;
            }
            conn->decRefCount();
         }
      }
   }

   return iExitCode;
}
