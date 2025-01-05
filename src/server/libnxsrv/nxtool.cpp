/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Raden Solutions
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
#include <netxms_getopt.h>
#include <netxms-version.h>

/**
 * Debug writer
 */
static void DebugWriter(const TCHAR *tag, const TCHAR *format, va_list args)
{
   _tprintf(_T("[%-19s] "), (tag != nullptr) ? tag : _T(""));
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
   InitNetXMSProcess(true);

   TCHAR keyFile[MAX_PATH];
   GetNetXMSDirectory(nxDirData, keyFile);
   _tcslcat(keyFile, DFILE_KEYS, MAX_PATH);

   TCHAR *eptr;
   bool start = true, useProxy = false;
   int i, ch, iExitCode = 3;
#ifdef _WITH_ENCRYPTION
   int iEncryptionPolicy = ENCRYPTION_PREFERRED;
#else
   int iEncryptionPolicy = ENCRYPTION_DISABLED;
#endif
   uint16_t agentPort = AGENT_LISTEN_PORT, proxyPort = AGENT_LISTEN_PORT;
   TCHAR *secret = nullptr;
   uint32_t dwTimeout = 5000, dwConnTimeout = 30000, dwError;
   TCHAR szProxy[MAX_OBJECT_NAME] = _T("");
   TCHAR *proxySecret = nullptr;
   RSA_KEY serverKey = nullptr;

#if defined(UNICODE) && !defined(_WIN32)
   WCHAR **wargv = MemAllocArray<WCHAR*>(tool->argc);
   for (i = 0; i < tool->argc; i++)
      wargv[i] = WideStringFromMBStringSysLocale(tool->argv[i]);
#define _targv wargv
#else
#define _targv tool->argv
#endif

   // Parse command line
   _topterr = 1;
   char options[128] = "D:e:hK:O:p:s:S:vw:W:X:";
   strlcat(options, tool->additionalOptions, 128);

   while ((ch = _tgetopt(tool->argc, _targv, options)) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
            _tprintf(_T("%s\n")
                     _T("Common options:\n")
                     _T("   -D level     : Set debug level (0..9 or off, default is off).\n")
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
         case 'D':   // debug level
            if (!_tcsicmp(_toptarg, _T("off")))
            {
               nxlog_set_debug_writer(nullptr);
               nxlog_set_debug_level(0);
            }
            else
            {
               nxlog_set_debug_writer(DebugWriter);
               nxlog_set_debug_level((int)_tcstol(_toptarg, nullptr, 0));
            }
            break;
         case 'p':   // Agent's port number
         case 'O':   // Proxy agent's port number
            i = _tcstol(_toptarg, &eptr, 0);
            if ((*eptr != 0) || (i < 0) || (i > 65535))
            {
               _tprintf(_T("Invalid port number \"%s\"\n"), _toptarg);
               start = false;
            }
            else
            {
               if (ch == 'p')
                  agentPort = (uint16_t)i;
               else
                  proxyPort = (uint16_t)i;
            }
            break;
         case 's':   // Shared secret
            secret = MemCopyString(_toptarg);
            break;
         case 'S':   // Shared secret for proxy agent
            proxySecret = MemCopyString(_toptarg);
            break;
         case 'v':   // Print version and exit
            _tprintf(_T("NetXMS %s Version ") NETXMS_VERSION_STRING _T("\n"), tool->displayName);
            start = false;
            break;
         case 'w':   // Command timeout
            i = _tcstol(_toptarg, &eptr, 0);
            if ((*eptr != 0) || (i < 1) || (i > 120))
            {
               _tprintf(_T("Invalid timeout \"%s\"\n"), _toptarg);
               start = false;
            }
            else
            {
               dwTimeout = (uint32_t)i * 1000;  // Convert to milliseconds
            }
            break;
         case 'W':   // Connection timeout
            i = _tcstol(_toptarg, &eptr, 0);
            if ((*eptr != 0) || (i < 1) || (i > 120))
            {
               _tprintf(_T("Invalid timeout \"%s\"\n"), _toptarg);
               start = false;
            }
            else
            {
               dwConnTimeout = (uint32_t)i * 1000;  // Convert to milliseconds
            }
            break;
#ifdef _WITH_ENCRYPTION
         case 'e':
            iEncryptionPolicy = _tcstol(_toptarg, nullptr, 0);
            if ((iEncryptionPolicy < 0) ||
                (iEncryptionPolicy > 3))
            {
               _tprintf(_T("ERROR: Invalid encryption policy %d\n"), iEncryptionPolicy);
               start = false;
            }
            break;
         case 'K':
            _tcslcpy(keyFile, _toptarg, MAX_PATH);
            break;
#else
         case 'e':
         case 'K':
            _tprintf(_T("ERROR: This tool was compiled without encryption support\n"));
            start = false;
            break;
#endif
         case 'X':   // Use proxy
            _tcslcpy(szProxy, _toptarg, MAX_OBJECT_NAME);
            useProxy = true;
            break;
         case '?':
            start = false;
            break;
         default:
            start = tool->parseAdditionalOptionCb(ch, _toptarg);
            break;
      }
   }

   // Check parameter correctness
   if (start)
   {
      if (tool->isArgMissingCb(tool->argc - _toptind))
      {
         _tprintf(_T("Required argument(s) missing.\nRun with -h option to get complete command line syntax.\n"));
         start = false;
      }

      // Load server key if requested
#ifdef _WITH_ENCRYPTION
      if ((iEncryptionPolicy != ENCRYPTION_DISABLED) && start)
      {
         if (InitCryptoLib(0xFFFF))
         {
            serverKey = RSALoadKey(keyFile);
            if (serverKey == nullptr)
            {
               serverKey = RSAGenerateKey(2048);
               if (serverKey == nullptr)
               {
                  _tprintf(_T("Cannot load server RSA key from \"%s\" or generate new key\n"), keyFile);
                  if (iEncryptionPolicy == ENCRYPTION_REQUIRED)
                     start = false;
               }
            }
         }
         else
         {
            _tprintf(_T("Error initializing cryptography module\n"));
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
         InetAddress addr = InetAddress::resolveHostName(_targv[_toptind]);
         InetAddress proxyAddr = useProxy ? InetAddress::resolveHostName(szProxy) : InetAddress();
         if (!addr.isValid())
         {
            _ftprintf(stderr, _T("Invalid host name or address \"%s\"\n"), _targv[_toptind]);
         }
         else if (useProxy && !proxyAddr.isValid())
         {
            _ftprintf(stderr, _T("Invalid host name or address \"%s\"\n"), szProxy);
         }
         else
         {
            auto conn = make_shared<AgentConnection>(addr, agentPort, secret);
            conn->setConnectionTimeout(dwConnTimeout);
            conn->setCommandTimeout(dwTimeout);
            conn->setEncryptionPolicy(iEncryptionPolicy);
            if (useProxy)
               conn->setProxy(proxyAddr, proxyPort, proxySecret);
            if (conn->connect(serverKey, &dwError))
            {
               iExitCode = tool->executeCommandCb(conn.get(), tool->argc, _targv, _toptind, serverKey);
            }
            else
            {
               WriteToTerminalEx(_T("%d: %s\n"), dwError, AgentErrorCodeToText(dwError));
               iExitCode = 2;
            }
         }
      }
   }

#if defined(UNICODE) && !defined(_WIN32)
   for(i = 0; i < tool->argc; i++)
      MemFree(wargv[i]);
   MemFree(wargv);
#endif

   RSAFree(serverKey);
   return iExitCode;
}
