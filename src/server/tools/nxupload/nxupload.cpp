/* 
** nxupload - command line tool used to upload files to NetXMS agent
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
** $module: nxupload.cpp
**
**/

#include <nms_common.h>
#include <nms_agent.h>
#include <nms_util.h>
#include <nxclapi.h>
#include <nxsrvapi.h>

#ifndef _WIN32
#include <netdb.h>
#endif


//
// Do agent upgrade
//

static int UpgradeAgent(AgentConnection &conn, TCHAR *pszPkgName, BOOL bVerbose, RSA *pServerKey)
{
   UINT32 dwError;
   int i;
   BOOL bConnected = FALSE;

   dwError = conn.startUpgrade(pszPkgName);
   if (dwError == ERR_SUCCESS)
   {
      conn.disconnect();

      if (bVerbose)
      {
         _tprintf(_T("Agent upgrade started, waiting for completion...\n")
                  _T("[............................................................]\r["));
         fflush(stdout);
         for(i = 0; i < 120; i += 2)
         {
            ThreadSleep(2);
            _puttc(_T('*'), stdout);
            fflush(stdout);
            if ((i % 20 == 0) && (i > 30))
            {
               if (conn.connect(pServerKey, FALSE))
               {
                  bConnected = TRUE;
                  break;   // Connected successfully
               }
            }
         }
         _puttc(_T('\n'), stdout);
      }
      else
      {
         ThreadSleep(20);
         for(i = 20; i < 120; i += 20)
         {
            ThreadSleep(20);
            if (conn.connect(pServerKey, FALSE))
            {
               bConnected = TRUE;
               break;   // Connected successfully
            }
         }
      }

      // Last attempt to reconnect
      if (!bConnected)
         bConnected = conn.connect(pServerKey, FALSE);

      if (bConnected && bVerbose)
      {
         _tprintf(_T("Successfully established connection to agent after upgrade\n"));
      }
      else
      {
         _ftprintf(stderr, _T("Failed to establish connection to the agent after upgrade\n"));
      }
   }
   else
   {
      if (bVerbose)
         _ftprintf(stderr, _T("%d: %s\n"), dwError, AgentErrorCodeToText(dwError));
   }

   return bConnected ? 0 : 1;
}


//
// Upload progress callback
//

static void ProgressCallback(INT64 bytesTransferred, void *cbArg)
{
#ifdef _WIN32
	_tprintf(_T("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b%-16I64d"), bytesTransferred);
#else
	_tprintf(_T("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b%-16lld"), bytesTransferred);
#endif
}


//
// Startup
//

int main(int argc, char *argv[])
{
   char *eptr;
   BOOL bStart = TRUE, bVerbose = TRUE, bUpgrade = FALSE;
   int i, ch, iExitCode = 3;
   int iAuthMethod = AUTH_NONE;
#ifdef _WITH_ENCRYPTION
   int iEncryptionPolicy = ENCRYPTION_ALLOWED;
#else
   int iEncryptionPolicy = ENCRYPTION_DISABLED;
#endif
   WORD wPort = AGENT_LISTEN_PORT;
   UINT32 dwAddr, dwTimeout = 5000, dwConnTimeout = 30000, dwError;
   INT64 nElapsedTime;
   TCHAR szSecret[MAX_SECRET_LENGTH] = _T("");
   TCHAR szKeyFile[MAX_PATH] = DEFAULT_DATA_DIR DFILE_KEYS;
   TCHAR szDestinationFile[MAX_PATH] = {0};
   RSA *pServerKey = NULL;

   // Parse command line
   opterr = 1;
	while((ch = getopt(argc, argv, "a:d:e:hK:p:qs:uvw:W:")) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
            _tprintf(_T("Usage: nxupload [<options>] <host> <file>\n")
                     _T("Valid options are:\n")
                     _T("   -a <auth>    : Authentication method. Valid methods are \"none\",\n")
                     _T("                  \"plain\", \"md5\" and \"sha1\". Default is \"none\".\n")
                     _T("   -d <file>    : Fully qualified destination file name\n")
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
                     _T("   -p <port>    : Specify agent's port number. Default is %d.\n")
                     _T("   -q           : Quiet mode.\n")
                     _T("   -s <secret>  : Specify shared secret for authentication.\n")
                     _T("   -u           : Start agent upgrade from uploaded package.\n")
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
         case 'u':   // Upgrade agent
            bUpgrade = TRUE;
            break;
         case 'v':   // Print version and exit
            _tprintf(_T("NetXMS UPLOAD command-line utility Version ") NETXMS_VERSION_STRING _T("\n"));
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
         case 'W':   // Connection timeout
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
            if (bVerbose)
               _tprintf(_T("ERROR: This tool was compiled without encryption support\n"));
            bStart = FALSE;
            break;
#endif
		 case 'd':
#ifdef UNICODE
	         MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, szDestinationFile, MAX_PATH);
				szDestinationFile[MAX_PATH - 1] = 0;
#else
            nx_strncpy(szDestinationFile, optarg, MAX_PATH);
#endif
            break;
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
         if (bVerbose)
            _tprintf(_T("Required argument(s) missing.\nUse nxupload -h to get complete command line syntax.\n"));
         bStart = FALSE;
      }
      else if ((iAuthMethod != AUTH_NONE) && (szSecret[0] == 0))
      {
         if (bVerbose)
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
               if (bVerbose)
                  _tprintf(_T("Error loading RSA keys from \"%s\"\n"), szKeyFile);
               if (iEncryptionPolicy == ENCRYPTION_REQUIRED)
                  bStart = FALSE;
            }
         }
         else
         {
            if (bVerbose)
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
         WSAStartup(2, &wsaData);
#endif

         dwAddr = ResolveHostNameA(argv[optind]);
         if ((dwAddr == INADDR_ANY) || (dwAddr == INADDR_NONE))
         {
            if (bVerbose)
               _tprintf(_T("Invalid host name or address specified\n"));
         }
         else
         {
            AgentConnection conn(dwAddr, wPort, iAuthMethod, szSecret);

				conn.setConnectionTimeout(dwConnTimeout);
            conn.setCommandTimeout(dwTimeout);
            conn.setEncryptionPolicy(iEncryptionPolicy);
            if (conn.connect(pServerKey, bVerbose, &dwError))
            {
               UINT32 dwError;

#ifdef UNICODE
					WCHAR fname[MAX_PATH];
					MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, argv[optind + 1], -1, fname, MAX_PATH);
					fname[MAX_PATH - 1] = 0;
#else
#define fname argv[optind + 1]
#endif
               nElapsedTime = GetCurrentTimeMs();
					if (bVerbose)
						_tprintf(_T("Upload:                 "));
					dwError = conn.uploadFile(
						fname, szDestinationFile[0] != 0 ? szDestinationFile : NULL,
						bVerbose ? ProgressCallback : NULL, NULL);
					if (bVerbose)
						_tprintf(_T("\r                        \r"));
               nElapsedTime = GetCurrentTimeMs() - nElapsedTime;
               if (bVerbose)
               {
                  if (dwError == ERR_SUCCESS)
                  {
                     QWORD qwBytes;

                     qwBytes = FileSize(fname);
                     _tprintf(_T("File transferred successfully\n") UINT64_FMT _T(" bytes in %d.%03d seconds (%.2f KB/sec)\n"),
                              qwBytes, (LONG)(nElapsedTime / 1000), 
                              (LONG)(nElapsedTime % 1000), 
                              ((double)((INT64)qwBytes / 1024) / (double)nElapsedTime) * 1000);
                  }
                  else
                  {
                     _tprintf(_T("%d: %s\n"), dwError, AgentErrorCodeToText(dwError));
                  }
               }

               if (bUpgrade && (dwError == RCC_SUCCESS))
               {
                  iExitCode = UpgradeAgent(conn, fname, bVerbose, pServerKey);
               }
               else
               {
                  iExitCode = (dwError == ERR_SUCCESS) ? 0 : 1;
               }
               conn.disconnect();
            }
            else
            {
               if (bVerbose)
                  _tprintf(_T("%d: %s\n"), dwError, AgentErrorCodeToText(dwError));
               iExitCode = 2;
            }
         }
      }
   }

   return iExitCode;
}
