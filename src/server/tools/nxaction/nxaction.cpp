/* 
** nxaction - command line tool used to execute preconfigured actions 
**            on NetXMS agent
** Copyright (C) 2004-2015 Victor Kirhenshtein
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
** File: nxaction.cpp
**
**/

#include <nms_common.h>
#include <nms_agent.h>
#include <nms_util.h>
#include <nxsrvapi.h>

#ifndef _WIN32
#include <netdb.h>
#endif

/**
 * Output callback
 */
static void OutputCallback(ActionCallbackEvent e, const TCHAR *data, void *arg)
{
   if (e == ACE_DATA)
      _tprintf(_T("%s"), data);
}

/**
 * Startup
 */
int main(int argc, char *argv[])
{
   char *eptr;
   BOOL bStart = TRUE, bVerbose = TRUE;
   bool showOutput = false;
   int i, ch, iExitCode = 3;
   int iAuthMethod = AUTH_NONE;
#ifdef _WITH_ENCRYPTION
   int iEncryptionPolicy = ENCRYPTION_ALLOWED;
#else
   int iEncryptionPolicy = ENCRYPTION_DISABLED;
#endif
   WORD wPort = AGENT_LISTEN_PORT;
   UINT32 dwTimeout = 5000, dwConnTimeout = 30000, dwError;
   TCHAR szSecret[MAX_SECRET_LENGTH] = _T("");
   TCHAR szKeyFile[MAX_PATH] = DEFAULT_DATA_DIR DFILE_KEYS;
   RSA *pServerKey = NULL;

   InitThreadLibrary();

   // Parse command line
   opterr = 1;
	while((ch = getopt(argc, argv, "a:e:hK:op:qs:vw:W:")) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
            _tprintf(_T("Usage: nxaction [<options>] <host> <action> [<action args>]\n")
                     _T("Valid options are:\n")
                     _T("   -a <auth>    : Authentication method. Valid methods are \"none\",\n")
                     _T("                  \"plain\", \"md5\" and \"sha1\". Default is \"none\".\n")
#ifdef _WITH_ENCRYPTION
                     _T("   -e <policy>  : Set encryption policy. Possible values are:\n")
                     _T("                    0 = Encryption disabled;\n")
                     _T("                    1 = Encrypt connection only if agent requires encryption;\n")
                     _T("                    2 = Encrypt connection if agent supports encryption;\n")
                     _T("                    3 = Force encrypted connection;\n")
                     _T("                  Default value is 1.\n")
#endif
                     _T("   -h           : Display help and exit.\n")
#ifdef _WITH_ENCRYPTION
                     _T("   -K <file>    : Specify server's key file\n")
                     _T("                  (default is ") DEFAULT_DATA_DIR DFILE_KEYS _T(").\n")
#endif
                     _T("   -o           : Show action's output.\n")
                     _T("   -p <port>    : Specify agent's port number. Default is %d.\n")
                     _T("   -q           : Quiet mode.\n")
                     _T("   -s <secret>  : Specify shared secret for authentication.\n")
                     _T("   -v           : Display version and exit.\n")
                     _T("   -w <seconds> : Set command timeout (default is 5 seconds)\n")
                     _T("   -W <seconds> : Set connection timeout (default is 30 seconds)\n")
                     _T("\n"), wPort);
            bStart = FALSE;
            break;
         case 'a':   // Auth method
            if (!strcmp(optarg, "none"))
               iAuthMethod = AUTH_NONE;
            else if (!strcmp(optarg, "plain"))
               iAuthMethod = AUTH_PLAINTEXT;
            else if (!strcmp(optarg, "md5"))
               iAuthMethod = AUTH_MD5_HASH;
            else if (!strcmp(optarg, "sha1"))
               iAuthMethod = AUTH_SHA1_HASH;
            else
            {
               _tprintf(_T("Invalid authentication method \"%hs\"\n"), optarg);
               bStart = FALSE;
            }
            break;
         case 'o':
            showOutput = true;
            break;
         case 'p':   // Port number
            i = strtol(optarg, &eptr, 0);
            if ((*eptr != 0) || (i < 0) || (i > 65535))
            {
               _tprintf(_T("Invalid port number \"%hs\"\n"), optarg);
               bStart = FALSE;
            }
            else
            {
               wPort = (WORD)i;
            }
            break;
         case 'q':   // Quiet mode
            bVerbose = FALSE;
            break;
         case 's':   // Shared secret
#ifdef UNICODE
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, szSecret, MAX_SECRET_LENGTH);
				szSecret[MAX_SECRET_LENGTH - 1] = 0;
#else
            nx_strncpy(szSecret, optarg, MAX_SECRET_LENGTH);
#endif
            break;
         case 'v':   // Print version and exit
            _tprintf(_T("NetXMS ACTION command-line utility Version ") NETXMS_VERSION_STRING _T("\n"));
            bStart = FALSE;
            break;
         case 'w':   // Command timeout
            i = strtol(optarg, &eptr, 0);
            if ((*eptr != 0) || (i < 1) || (i > 120))
            {
               _tprintf(_T("Invalid timeout \"%hs\"\n"), optarg);
               bStart = FALSE;
            }
            else
            {
               dwTimeout = (UINT32)i * 1000;  // Convert to milliseconds
            }
            break;
         case 'W':   // Connect timeout
            i = strtol(optarg, &eptr, 0);
            if ((*eptr != 0) || (i < 1) || (i > 120))
            {
               _tprintf(_T("Invalid timeout \"%hs\"\n"), optarg);
               bStart = FALSE;
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
               _tprintf(_T("Invalid encryption policy %d\n"), iEncryptionPolicy);
               bStart = FALSE;
            }
            break;
         case 'K':
#ifdef UNICODE
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, szKeyFile, MAX_PATH);
				szKeyFile[MAX_PATH - 1] = 0;
#else
            nx_strncpy(szKeyFile, optarg, MAX_PATH);
#endif
            break;
#else
         case 'e':
         case 'K':
            _tprintf(_T("ERROR: This tool was compiled without encryption support\n"));
            bStart = FALSE;
            break;
#endif
         case '?':
            bStart = FALSE;
            break;
         default:
            break;
      }
   }

   // Check parameter correctness
   if (bStart)
   {
      if (argc - optind < 2)
      {
         _tprintf(_T("Required argument(s) missing.\nUse nxaction -h to get complete command line syntax.\n"));
         bStart = FALSE;
      }
      else if ((iAuthMethod != AUTH_NONE) && (szSecret[0] == 0))
      {
         _tprintf(_T("Shared secret not specified or empty\n"));
         bStart = FALSE;
      }

      // Load server key if requested
#ifdef _WITH_ENCRYPTION
      if ((iEncryptionPolicy != ENCRYPTION_DISABLED) && bStart)
      {
         if (InitCryptoLib(0xFFFF, NULL))
         {
            pServerKey = LoadRSAKeys(szKeyFile);
            if (pServerKey == NULL)
            {
               _tprintf(_T("Error loading RSA keys from \"%s\"\n"), szKeyFile);
               if (iEncryptionPolicy == ENCRYPTION_REQUIRED)
                  bStart = FALSE;
            }
         }
         else
         {
            _tprintf(_T("Error initializing cryptografy module\n"));
            if (iEncryptionPolicy == ENCRYPTION_REQUIRED)
               bStart = FALSE;
         }
      }
#endif

      // If everything is ok, start communications
      if (bStart)
      {
         // Initialize WinSock
#ifdef _WIN32
         WSADATA wsaData;
         WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

         // Resolve hostname
         InetAddress addr = InetAddress::resolveHostName(argv[optind]);
         if (!addr.isValid())
         {
            _tprintf(_T("Invalid host name or address specified\n"));
         }
         else
         {
            AgentConnection conn(addr, wPort, iAuthMethod, szSecret);

				conn.setConnectionTimeout(dwConnTimeout);
            conn.setCommandTimeout(dwTimeout);
            conn.setEncryptionPolicy(iEncryptionPolicy);
            if (conn.connect(pServerKey, bVerbose, &dwError))
            {
               UINT32 dwError;

#ifdef UNICODE
					WCHAR action[256], *args[256];
					int i, k;

					MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, argv[optind + 1], -1, action, 256);
					action[255] = 0;
					
					int count = min(argc - optind - 2, 256);
					for(i = 0, k = optind + 2; i < count; i++, k++)
						args[i] = WideStringFromMBString(argv[k]);
               dwError = conn.execAction(action, count, args, showOutput, OutputCallback);
					for(i = 0; i < count; i++)
						free(args[i]);
#else
               dwError = conn.execAction(argv[optind + 1], argc - optind - 2, &argv[optind + 2], showOutput, OutputCallback);
#endif
               if (bVerbose)
               {
                  if (dwError == ERR_SUCCESS)
                     _tprintf(_T("Action executed successfully\n"));
                  else
                     _tprintf(_T("%d: %s\n"), dwError, AgentErrorCodeToText(dwError));
               }
               iExitCode = (dwError == ERR_SUCCESS) ? 0 : 1;
               conn.disconnect();
            }
            else
            {
               _tprintf(_T("%d: %s\n"), dwError, AgentErrorCodeToText(dwError));
               iExitCode = 2;
            }
         }
      }
   }

   return iExitCode;
}
