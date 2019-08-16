/*
** nxget - command line tool used to retrieve parameters from NetXMS agent
** Copyright (C) 2004-2018 Victor Kirhenshtein
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
** File: nxget.cpp
**
**/

#include <nms_common.h>
#include <nms_agent.h>
#include <nms_util.h>
#include <nxcpapi.h>
#include <nxsrvapi.h>

#ifndef _WIN32
#include <netdb.h>
#endif

NETXMS_EXECUTABLE_HEADER(nxget)

#define MAX_LINE_SIZE      4096

/**
 * Table output delimiter
 */
static TCHAR s_tableOutputDelimiter = 0;

/**
 * Operations
 */
enum Operation
{
   CMD_GET            = 0,
   CMD_LIST           = 1,
   CMD_CHECK_SERVICE  = 2,
   CMD_GET_PARAMS     = 3,
   CMD_GET_CONFIG     = 4,
   CMD_TABLE          = 5,
   CMD_GET_SCREENSHOT = 6
};

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
 * Get single parameter
 */
static int Get(AgentConnection *pConn, const TCHAR *pszParam, BOOL bShowName)
{
   UINT32 dwError;
   TCHAR szBuffer[1024];

   dwError = pConn->getParameter(pszParam, 1024, szBuffer);
   if (dwError == ERR_SUCCESS)
   {
      if (bShowName)
         _tprintf(_T("%s = %s\n"), pszParam, szBuffer);
      else
         _tprintf(_T("%s\n"), szBuffer);
   }
   else
   {
      _tprintf(_T("%d: %s\n"), dwError, AgentErrorCodeToText(dwError));
   }
   fflush(stdout);
   return (dwError == ERR_SUCCESS) ? 0 : 1;
}

/**
 * Get list of values for enum parameters
 */
static int List(AgentConnection *pConn, const TCHAR *pszParam)
{
   StringList *data;
   UINT32 rcc = pConn->getList(pszParam, &data);
   if (rcc == ERR_SUCCESS)
   {
      for(int i = 0; i < data->size(); i++)
         _tprintf(_T("%s\n"), data->get(i));
      delete data;
   }
   else
   {
      _tprintf(_T("%d: %s\n"), rcc, AgentErrorCodeToText(rcc));
   }
   return (rcc == ERR_SUCCESS) ? 0 : 1;
}

/**
 * Get table value
 */
static int GetTable(AgentConnection *pConn, const TCHAR *pszParam)
{
	Table *table;
   UINT32 rcc = pConn->getTable(pszParam, &table);
   if (rcc == ERR_SUCCESS)
   {
      if (s_tableOutputDelimiter != 0)
         table->dump(stdout, true, s_tableOutputDelimiter);
      else
         table->writeToTerminal();
		delete table;
   }
   else
   {
      _tprintf(_T("%u: %s\n"), rcc, AgentErrorCodeToText(rcc));
   }
   return (rcc == ERR_SUCCESS) ? 0 : 1;
}

/**
 * Check network service state
 */
static int CheckService(AgentConnection *pConn, int serviceType, UINT32 dwServiceAddr,
                        WORD wProto, WORD wPort, const TCHAR *pszRequest, const TCHAR *pszResponse)
{
   UINT32 dwStatus, dwError;

   dwError = pConn->checkNetworkService(&dwStatus, dwServiceAddr, serviceType, wPort,
                                        wProto, pszRequest, pszResponse);
   if (dwError == ERR_SUCCESS)
   {
      printf("Service status: %d\n", dwStatus);
   }
   else
   {
      _tprintf(_T("%d: %s\n"), dwError, AgentErrorCodeToText(dwError));
   }
   return (dwError == ERR_SUCCESS) ? 0 : 1;
}

/**
 * List supported parameters
 */
static int ListParameters(AgentConnection *pConn)
{
   static const TCHAR *pszDataType[] = { _T("INT"), _T("UINT"), _T("INT64"), _T("UINT64"), _T("STRING"), _T("FLOAT"), _T("UNKNOWN") };

   ObjectArray<AgentParameterDefinition> *paramList;
   ObjectArray<AgentTableDefinition> *tableList;
   UINT32 dwError = pConn->getSupportedParameters(&paramList, &tableList);
   if (dwError == ERR_SUCCESS)
   {
      for(int i = 0; i < paramList->size(); i++)
      {
			AgentParameterDefinition *p = paramList->get(i);
         _tprintf(_T("%s %s \"%s\"\n"), p->getName(),
            pszDataType[(p->getDataType() < 6) && (p->getDataType() >= 0) ? p->getDataType() : 6],
            p->getDescription());
      }
      delete paramList;
		delete tableList;
   }
   else
   {
      _tprintf(_T("%d: %s\n"), dwError, AgentErrorCodeToText(dwError));
   }
   return (dwError == ERR_SUCCESS) ? 0 : 1;
}

/**
 * Get configuration file
 */
static int GetConfig(AgentConnection *pConn)
{
   UINT32 dwError, dwSize;
   TCHAR *pszFile;

   dwError = pConn->getConfigFile(&pszFile, &dwSize);
   if (dwError == ERR_SUCCESS)
   {
      TranslateStr(pszFile, _T("\r\n"), _T("\n"));
      _fputts(pszFile, stdout);
      if (dwSize > 0)
      {
         if (pszFile[dwSize - 1] != _T('\n'))
            fputc('\n', stdout);
      }
      else
      {
         fputc('\n', stdout);
      }
   }
   else
   {
      _tprintf(_T("%d: %s\n"), dwError, AgentErrorCodeToText(dwError));
   }
   return (dwError == ERR_SUCCESS) ? 0 : 1;
}

/**
 * Get screenshot
 */
static int GetScreenshot(AgentConnection *pConn, const char *sessionName, const char *fileName)
{
   BYTE *data;
   size_t size;
#ifdef UNICODE
   WCHAR *wname = WideStringFromMBString(sessionName);
   UINT32 dwError = pConn->takeScreenshot(wname, &data, &size);
   free(wname);
#else
   UINT32 dwError = pConn->takeScreenshot(sessionName, &data, &size);
#endif
   if (dwError == ERR_SUCCESS)
   {
      FILE *f = fopen(fileName, "wb");
      if (f != NULL)
      {
         if (data != NULL)
            fwrite(data, 1, size, f);
         fclose(f);
      }
      MemFree(data);
   }
   else
   {
      _tprintf(_T("%d: %s\n"), dwError, AgentErrorCodeToText(dwError));
   }
   return (dwError == ERR_SUCCESS) ? 0 : 1;
}

/**
 * Startup
 */
int main(int argc, char *argv[])
{
   char *eptr;
   BOOL start = TRUE, batchMode = FALSE, showNames = FALSE, useProxy = FALSE;
   int i, ch, iPos, iExitCode = 3, iInterval = 0;
   int authMethod = AUTH_NONE, proxyAuth = AUTH_NONE, serviceType = NETSRV_SSH;
#ifdef _WITH_ENCRYPTION
   int iEncryptionPolicy = ENCRYPTION_PREFERRED;
#else
   int iEncryptionPolicy = ENCRYPTION_DISABLED;
#endif
   Operation operation = CMD_GET;
   WORD agentPort = AGENT_LISTEN_PORT, proxyPort = AGENT_LISTEN_PORT;
   WORD wServicePort = 0, wServiceProto = 0;
   UINT32 dwTimeout = 5000, dwConnTimeout = 30000, dwServiceAddr = 0, dwError;
   TCHAR szSecret[MAX_SECRET_LENGTH] = _T(""), szRequest[MAX_DB_STRING] = _T("");
   TCHAR keyFile[MAX_PATH];
   TCHAR szResponse[MAX_DB_STRING] = _T("");
   char szProxy[MAX_OBJECT_NAME] = "";
	TCHAR szProxySecret[MAX_SECRET_LENGTH] = _T("");
   RSA *pServerKey = NULL;
#ifdef UNICODE
	WCHAR *wcValue;
#endif

   InitNetXMSProcess(true);
#ifdef _WIN32
	SetExceptionHandler(SEHDefaultConsoleHandler, NULL, NULL, _T("nxget"), FALSE, FALSE);
#endif
	nxlog_set_debug_writer(DebugWriter);

   GetNetXMSDirectory(nxDirData, keyFile);
   _tcscat(keyFile, DFILE_KEYS);

   // Parse command line
   opterr = 1;
   while((ch = getopt(argc, argv, "a:A:bCd:D:e:Ehi:IK:lno:O:p:P:r:R:s:S:t:Tvw:W:X:Z:")) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
            _tprintf(_T("Usage: nxget [<options>] <host> [<parameter> [<parameter> ...]]\n")
                     _T("Valid options are:\n")
                     _T("   -a auth      : Authentication method. Valid methods are \"none\",\n")
                     _T("                  \"plain\", \"md5\" and \"sha1\". Default is \"none\".\n")
                     _T("   -A auth      : Authentication method for proxy agent.\n")
                     _T("   -b           : Batch mode - get all parameters listed on command line.\n")
                     _T("   -C           : Get agent's configuration file\n")
                     _T("   -d delimiter : Print table content as delimited text.\n")
                     _T("   -D level     : Set debug level (default is 0).\n")
#ifdef _WITH_ENCRYPTION
                     _T("   -e policy    : Set encryption policy. Possible values are:\n")
                     _T("                    0 = Encryption disabled;\n")
                     _T("                    1 = Encrypt connection only if agent requires encryption;\n")
                     _T("                    2 = Encrypt connection if agent supports encryption;\n")
                     _T("                    3 = Force encrypted connection;\n")
                     _T("                  Default value is 1.\n")
#endif
                     _T("   -E file      : Take screenshot. First parameter is file name, second (optional) is session name.\n")
                     _T("   -h           : Display help and exit.\n")
                     _T("   -i seconds   : Get specified parameter(s) continously with given interval.\n")
                     _T("   -I           : Get list of supported parameters.\n")
#ifdef _WITH_ENCRYPTION
                     _T("   -K file      : Specify server's key file\n")
                     _T("                  (default is %s).\n")
#endif
                     _T("   -l           : Requested parameter is a list.\n")
                     _T("   -n           : Show parameter's name in result.\n")
                     _T("   -o proto     : Protocol number to be used for service check.\n")
                     _T("   -O port      : Proxy agent's port number. Default is %d.\n")
                     _T("   -p port      : Agent's port number. Default is %d.\n")
                     _T("   -P port      : Network service port (to be used wth -S option).\n")
                     _T("   -r string    : Service check request string.\n")
                     _T("   -R string    : Service check expected response string.\n")
                     _T("   -s secret    : Shared secret for authentication.\n")
                     _T("   -S addr      : Check state of network service at given address.\n")
                     _T("   -t type      : Set type of service to be checked.\n")
				         _T("                  Possible types are: custom, ssh, pop3, smtp, ftp, http, https, telnet.\n")
                     _T("   -T           : Requested parameter is a table.\n")
                     _T("   -v           : Display version and exit.\n")
                     _T("   -w seconds   : Set command timeout (default is 5 seconds).\n")
                     _T("   -W seconds   : Set connection timeout (default is 30 seconds).\n")
                     _T("   -X addr      : Use proxy agent at given address.\n")
                     _T("   -Z secret    : Shared secret for proxy agent authentication.\n")
                     _T("\n"),
#ifdef _WITH_ENCRYPTION
                     keyFile,
#endif
                     agentPort, agentPort);
            start = FALSE;
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
               start = FALSE;
            }
            if (ch == 'a')
               authMethod = i;
            else
               proxyAuth = i;
            break;
         case 'b':   // Batch mode
            batchMode = TRUE;
            break;
         case 'd':   // Dump table
            s_tableOutputDelimiter = *optarg;
            break;
         case 'D':   // debug level
            nxlog_set_debug_level((int)strtol(optarg, NULL, 0));
            break;
         case 'i':   // Interval
            i = strtol(optarg, &eptr, 0);
            if ((*eptr != 0) || (i <= 0))
            {
               printf("Invalid interval \"%s\"\n", optarg);
               start = FALSE;
            }
            else
            {
               iInterval = i;
            }
            break;
         case 'E':
            operation = CMD_GET_SCREENSHOT;
            break;
         case 'I':
            operation = CMD_GET_PARAMS;
            break;
         case 'C':
            operation = CMD_GET_CONFIG;
            break;
         case 'l':
            operation = CMD_LIST;
            break;
         case 'T':
            operation = CMD_TABLE;
            break;
         case 'n':   // Show names
            showNames = TRUE;
            break;
         case 'p':   // Agent's port number
         case 'P':   // Port number for service check
         case 'O':   // Proxy agent's port number
            i = strtol(optarg, &eptr, 0);
            if ((*eptr != 0) || (i < 0) || (i > 65535))
            {
               printf("Invalid port number \"%s\"\n", optarg);
               start = FALSE;
            }
            else
            {
               if (ch == 'p')
                  agentPort = (WORD)i;
               else if (ch == 'O')
                  proxyPort = (WORD)i;
               else
                  wServicePort = (WORD)i;
            }
            break;
         case 'r':   // Service check request string
#ifdef UNICODE
#if HAVE_MBSTOWCS
            mbstowcs(szRequest, optarg, MAX_DB_STRING);
#else
	         MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, szRequest, MAX_DB_STRING);
#endif
				szRequest[MAX_DB_STRING - 1] = 0;
#else
            strlcpy(szRequest, optarg, MAX_DB_STRING);
#endif
            break;
         case 'R':   // Service check response string
#ifdef UNICODE
#if HAVE_MBSTOWCS
            mbstowcs(szResponse, optarg, MAX_DB_STRING);
#else
	         MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, szResponse, MAX_DB_STRING);
#endif
				szResponse[MAX_DB_STRING - 1] = 0;
#else
            strlcpy(szResponse, optarg, MAX_DB_STRING);
#endif
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
         case 'S':   // Check service
            operation = CMD_CHECK_SERVICE;
            dwServiceAddr = ntohl(inet_addr(optarg));
            if ((dwServiceAddr == INADDR_NONE) || (dwServiceAddr == INADDR_ANY))
            {
               _tprintf(_T("Invalid IP address \"%hs\"\n"), optarg);
               start = FALSE;
            }
            break;
         case 't':   // Service type
            serviceType = strtol(optarg, &eptr, 0);
            if (*eptr != 0)
            {
					if (!stricmp(optarg, "custom"))
					{
						serviceType = NETSRV_CUSTOM;
					}
					else if (!stricmp(optarg, "ftp"))
					{
						serviceType = NETSRV_FTP;
					}
					else if (!stricmp(optarg, "http"))
					{
						serviceType = NETSRV_HTTP;
					}
					else if (!stricmp(optarg, "https"))
					{
						serviceType = NETSRV_HTTPS;
					}
					else if (!stricmp(optarg, "pop3"))
					{
						serviceType = NETSRV_POP3;
					}
					else if (!stricmp(optarg, "smtp"))
					{
						serviceType = NETSRV_SMTP;
					}
					else if (!stricmp(optarg, "ssh"))
					{
						serviceType = NETSRV_SSH;
					}
					else if (!stricmp(optarg, "telnet"))
					{
						serviceType = NETSRV_TELNET;
					}
					else
					{
						_tprintf(_T("Invalid service type \"%hs\"\n"), optarg);
						start = FALSE;
					}
            }
            break;
         case 'o':   // Protocol number for service check
            i = strtol(optarg, &eptr, 0);
            if ((*eptr != 0) || (i < 0) || (i > 65535))
            {
               _tprintf(_T("Invalid protocol number \"%hs\"\n"), optarg);
               start = FALSE;
            }
            else
            {
               wServiceProto = (WORD)i;
            }
            break;
         case 'v':   // Print version and exit
            _tprintf(_T("NetXMS GET command-line utility Version ") NETXMS_VERSION_STRING _T("\n"));
            start = FALSE;
            break;
         case 'w':   // Command timeout
            i = strtol(optarg, &eptr, 0);
            if ((*eptr != 0) || (i < 1) || (i > 120))
            {
               _tprintf(_T("Invalid timeout \"%hs\"\n"), optarg);
               start = FALSE;
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
               start = FALSE;
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
               start = FALSE;
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
            start = FALSE;
            break;
#endif
         case 'X':   // Use proxy
            strncpy(szProxy, optarg, MAX_OBJECT_NAME);
				szProxy[MAX_OBJECT_NAME - 1] = 0;
            useProxy = TRUE;
            break;
         case 'Z':   // Shared secret for proxy agent
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
         case '?':
            start = FALSE;
            break;
         default:
            break;
      }
   }

   // Check parameter correctness
   if (start)
   {
      if (argc - optind < (((operation == CMD_CHECK_SERVICE) || (operation == CMD_GET_PARAMS) || (operation == CMD_GET_CONFIG)) ? 1 : 2))
      {
         printf("Required argument(s) missing.\nUse nxget -h to get complete command line syntax.\n");
         start = FALSE;
      }
      else if ((authMethod != AUTH_NONE) && (szSecret[0] == 0))
      {
         printf("Shared secret not specified or empty\n");
         start = FALSE;
      }

      // Load server key if requested
#ifdef _WITH_ENCRYPTION
      if ((iEncryptionPolicy != ENCRYPTION_DISABLED) && start)
      {
         if (InitCryptoLib(0xFFFF))
         {
            pServerKey = LoadRSAKeys(keyFile);
            if (pServerKey == NULL)
            {
               pServerKey = RSAGenerateKey(2048);
               if (pServerKey == NULL)
               {
                  _tprintf(_T("Cannot load server RSA key from \"%s\" or generate new key\n"), keyFile);
                  if (iEncryptionPolicy == ENCRYPTION_REQUIRED)
                     start = FALSE;
               }
            }
         }
         else
         {
            printf("Error initializing cryptography module\n");
            if (iEncryptionPolicy == ENCRYPTION_REQUIRED)
               start = FALSE;
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
         InetAddress addr = InetAddress::resolveHostName(argv[optind]);
         InetAddress proxyAddr = useProxy ? InetAddress::resolveHostName(szProxy) : InetAddress();
         if (!addr.isValid())
         {
            fprintf(stderr, "Invalid host name or address \"%s\"\n", argv[optind]);
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
            if (conn->connect(pServerKey, &dwError))
            {
               do
               {
                  switch(operation)
                  {
                     case CMD_GET:
                        iPos = optind + 1;
                        do
                        {
#ifdef UNICODE
									wcValue = WideStringFromMBStringSysLocale(argv[iPos++]);
                           iExitCode = Get(conn, wcValue, showNames);
									MemFree(wcValue);
#else
                           iExitCode = Get(conn, argv[iPos++], showNames);
#endif
                        } while((iExitCode == 0) && (batchMode) && (iPos < argc));
                        break;
                     case CMD_LIST:
#ifdef UNICODE
								wcValue = WideStringFromMBStringSysLocale(argv[optind + 1]);
                        iExitCode = List(conn, wcValue);
                        MemFree(wcValue);
#else
                        iExitCode = List(conn, argv[optind + 1]);
#endif
                        break;
                     case CMD_TABLE:
#ifdef UNICODE
								wcValue = WideStringFromMBStringSysLocale(argv[optind + 1]);
                        iExitCode = GetTable(conn, wcValue);
                        MemFree(wcValue);
#else
                        iExitCode = GetTable(conn, argv[optind + 1]);
#endif
                        break;
                     case CMD_CHECK_SERVICE:
                        iExitCode = CheckService(conn, serviceType, dwServiceAddr,
                                                 wServiceProto, wServicePort, szRequest, szResponse);
                        break;
                     case CMD_GET_PARAMS:
                        iExitCode = ListParameters(conn);
                        break;
                     case CMD_GET_CONFIG:
                        iExitCode = GetConfig(conn);
                        break;
                     case CMD_GET_SCREENSHOT:
                        iExitCode = GetScreenshot(conn, (argc > optind + 2) ? argv[optind + 2] : "Console", argv[optind + 1]);
                        break;
                     default:
                        break;
                  }
                  ThreadSleep(iInterval);
               }
               while(iInterval > 0);
            }
            else
            {
               _tprintf(_T("%d: %s\n"), dwError, AgentErrorCodeToText(dwError));
               iExitCode = 2;
            }
            conn->decRefCount();
         }
      }
   }

   return iExitCode;
}
