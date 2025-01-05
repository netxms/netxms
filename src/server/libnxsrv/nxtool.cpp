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

   wchar_t keyFile[MAX_PATH];
   GetNetXMSDirectory(nxDirData, keyFile);
   wcslcat(keyFile, DFILE_KEYS, MAX_PATH);

   wchar_t *eptr;
   bool start = true, useProxy = false;
   int i, ch, iExitCode = 3;
   int iEncryptionPolicy = ENCRYPTION_PREFERRED;
   uint16_t agentPort = AGENT_LISTEN_PORT, proxyPort = AGENT_LISTEN_PORT;
   wchar_t *secret = nullptr;
   uint32_t dwTimeout = 5000, dwConnTimeout = 30000, dwError;
   wchar_t szProxy[MAX_OBJECT_NAME] = L"";
   wchar_t *proxySecret = nullptr;
   RSA_KEY serverKey = nullptr;

#ifndef _WIN32
   wchar_t **wargv = MemAllocArray<wchar_t*>(tool->argc);
   for (i = 0; i < tool->argc; i++)
      wargv[i] = WideStringFromMBStringSysLocale(tool->argv[i]);
#else
#define wargv tool->argv
#endif

   // Parse command line
   _topterr = 1;
   char options[128] = "D:e:hK:O:p:s:S:vw:W:X:";
   strlcat(options, tool->additionalOptions, 128);

   while ((ch = getoptW(tool->argc, wargv, options)) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
            WriteToTerminalEx(L"%s\n"
               L"Common options:\n"
               L"   -D level     : Set debug level (0..9 or off, default is off).\n"
               L"   -e policy    : Set encryption policy. Possible values are:\n"
               L"                    0 = Encryption disabled;\n"
               L"                    1 = Encrypt connection only if agent requires encryption;\n"
               L"                    2 = Encrypt connection if agent supports encryption;\n"
               L"                    3 = Force encrypted connection;\n"
               L"                  Default value is 1.\n"
               L"   -h           : Display help and exit.\n"
               L"   -K file      : Specify server's key file\n"
               L"                  (default is %s).\n"
               L"   -O port      : Proxy agent's port number. Default is %d.\n"
               L"   -p port      : Agent's port number. Default is %d.\n"
               L"   -s secret    : Shared secret for agent authentication.\n"
               L"   -S secret    : Shared secret for proxy agent authentication.\n"
               L"   -v           : Display version and exit.\n"
               L"   -w seconds   : Set command timeout (default is 5 seconds).\n"
               L"   -W seconds   : Set connection timeout (default is 30 seconds).\n"
               L"   -X addr      : Use proxy agent at given address.\n"
               L"\n",
               tool->mainHelpText, keyFile, agentPort, agentPort);
            start = false;
            break;
         case 'D':   // debug level
            if (!wcsicmp(optargW, L"off"))
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
               WriteToTerminalEx(L"Invalid port number \"%s\"\n", optargW);
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
            secret = MemCopyStringW(optargW);
            break;
         case 'S':   // Shared secret for proxy agent
            proxySecret = MemCopyStringW(optargW);
            break;
         case 'v':   // Print version and exit
            WriteToTerminalEx(L"NetXMS %s Version " NETXMS_VERSION_STRING L"\n", tool->displayName);
            start = false;
            break;
         case 'w':   // Command timeout
            i = _tcstol(optargW, &eptr, 0);
            if ((*eptr != 0) || (i < 1) || (i > 120))
            {
               WriteToTerminalEx(L"Invalid timeout \"%s\"\n", optargW);
               start = false;
            }
            else
            {
               dwTimeout = (uint32_t)i * 1000;  // Convert to milliseconds
            }
            break;
         case 'W':   // Connection timeout
            i = wcstol(optargW, &eptr, 0);
            if ((*eptr != 0) || (i < 1) || (i > 120))
            {
               WriteToTerminalEx(L"Invalid timeout \"%s\"\n", optargW);
               start = false;
            }
            else
            {
               dwConnTimeout = (uint32_t)i * 1000;  // Convert to milliseconds
            }
            break;
         case 'e':
            iEncryptionPolicy = wcstol(optargW, nullptr, 0);
            if ((iEncryptionPolicy < 0) ||
                (iEncryptionPolicy > 3))
            {
               WriteToTerminalEx(L"Invalid encryption policy %d\n", iEncryptionPolicy);
               start = false;
            }
            break;
         case 'K':
            wcslcpy(keyFile, optargW, MAX_PATH);
            break;
         case 'X':   // Use proxy
            wcslcpy(szProxy, optargW, MAX_OBJECT_NAME);
            useProxy = true;
            break;
         case '?':
            start = false;
            break;
         default:
            start = tool->parseAdditionalOptionCb(ch, optargW);
            break;
      }
   }

   // Check parameter correctness
   if (start)
   {
      if (tool->isArgMissingCb(tool->argc - _toptind))
      {
         WriteToTerminalEx(L"Required argument(s) missing.\nRun with -h option to get complete command line syntax.\n");
         start = false;
      }

      // Load server key if requested
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
                  WriteToTerminalEx(L"Cannot load server RSA key from \"%s\" or generate new key\n", keyFile);
                  if (iEncryptionPolicy == ENCRYPTION_REQUIRED)
                     start = false;
               }
            }
         }
         else
         {
            WriteToTerminal(L"Error initializing cryptography module\n");
            if (iEncryptionPolicy == ENCRYPTION_REQUIRED)
               start = false;
         }
      }

      // If everything is ok, start communications
      if (start)
      {
         // Initialize WinSock
#ifdef _WIN32
         WSADATA wsaData;
         WSAStartup(2, &wsaData);
#endif
         InetAddress addr = InetAddress::resolveHostName(wargv[optindW]);
         InetAddress proxyAddr = useProxy ? InetAddress::resolveHostName(szProxy) : InetAddress();
         if (!addr.isValid())
         {
            _ftprintf(stderr, _T("Invalid host name or address \"%s\"\n"), wargv[optindW]);
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
               iExitCode = tool->executeCommandCb(conn.get(), tool->argc, wargv, optindW, serverKey);
            }
            else
            {
               WriteToTerminalEx(_T("%d: %s\n"), dwError, AgentErrorCodeToText(dwError));
               iExitCode = 2;
            }
         }
      }
   }

#ifndef _WIN32
   for(i = 0; i < tool->argc; i++)
      MemFree(wargv[i]);
   MemFree(wargv);
#endif

   RSAFree(serverKey);
   return iExitCode;
}
