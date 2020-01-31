/*
** nxget - command line tool used to retrieve parameters from NetXMS agent
** Copyright (C) 2004-2020 Raden Solutions
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

#include <nms_util.h>
#include <nms_agent.h>
#include <nxsrvapi.h>

#ifndef _WIN32
#include <netdb.h>
#endif

NETXMS_EXECUTABLE_HEADER(nxget)

#define MAX_LINE_SIZE      4096

/**
 * Operations
 */
enum Operation
{
   CMD_GET,
   CMD_LIST,
   CMD_CHECK_SERVICE,
   CMD_GET_PARAMS,
   CMD_GET_CONFIG,
   CMD_TABLE,
   CMD_GET_SCREENSHOT,
   CMD_FILE_SET_INFO
};

/**
 * Static fields
 */
static TCHAR s_tableOutputDelimiter = 0;
static Operation s_operation = CMD_GET;
static bool s_showNames = false;
static bool s_batchMode = false;
static int s_interval = 0;
static int s_serviceType = NETSRV_SSH;
static WORD s_servicePort = 0, s_serviceProto = 0;
static UINT32 s_serviceAddr = 0;
static TCHAR s_serviceResponse[MAX_DB_STRING] = _T(""), s_serviceRequest[MAX_DB_STRING] = _T("");

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
         WriteToTerminalEx(_T("%s = %s\n"), pszParam, szBuffer);
      else
         WriteToTerminalEx(_T("%s\n"), szBuffer);
   }
   else
   {
      WriteToTerminalEx(_T("%d: %s\n"), dwError, AgentErrorCodeToText(dwError));
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
         WriteToTerminalEx(_T("%s\n"), data->get(i));
      delete data;
   }
   else
   {
      WriteToTerminalEx(_T("%d: %s\n"), rcc, AgentErrorCodeToText(rcc));
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
      WriteToTerminalEx(_T("%u: %s\n"), rcc, AgentErrorCodeToText(rcc));
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
      WriteToTerminalEx(_T("Service status: %d\n"), dwStatus);
   }
   else
   {
      WriteToTerminalEx(_T("%d: %s\n"), dwError, AgentErrorCodeToText(dwError));
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
         WriteToTerminalEx(_T("%s %s \"%s\"\n"), p->getName(),
            pszDataType[(p->getDataType() < 6) && (p->getDataType() >= 0) ? p->getDataType() : 6],
            p->getDescription());
      }
      delete paramList;
		delete tableList;
   }
   else
   {
      WriteToTerminalEx(_T("%d: %s\n"), dwError, AgentErrorCodeToText(dwError));
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
      WriteToTerminalEx(_T("%d: %s\n"), dwError, AgentErrorCodeToText(dwError));
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
   MemFree(wname);
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
      WriteToTerminalEx(_T("%d: %s\n"), dwError, AgentErrorCodeToText(dwError));
   }
   return (dwError == ERR_SUCCESS) ? 0 : 1;
}

/**
 * Get information about file set
 */
static int GetFileSetInfo(AgentConnection *conn, int argc, char **argv)
{
   StringList fileSet;
   for(int i = 0; i < argc; i++)
   {
#ifdef UNICODE
      fileSet.addPreallocated(WideStringFromMBStringSysLocale(argv[i]));
#else
      fileSet.add(argv[i]);
#endif
   }

   ObjectArray<RemoteFileInfo> *info;
   UINT32 rcc = conn->getFileSetInfo(fileSet, true, &info);
   if (rcc == ERR_SUCCESS)
   {
      for(int i = 0; i < info->size(); i++)
      {
         if (i > 0)
            WriteToTerminal(_T("\n"));
         RemoteFileInfo *rfi = info->get(i);
         WriteToTerminalEx(_T("\x1b[1m%s\x1b[0m\n   Status: \x1b[%s;1m%s\x1b[0m\n"),
                  rfi->name(), rfi->isValid() ? _T("32") : _T("31"), AgentErrorCodeToText(rfi->status()));
         if (rfi->isValid())
         {
            TCHAR hash[64];
            WriteToTerminalEx(_T("   Size:  ") UINT64_FMT _T("\n   Time:  ") INT64_FMT _T("\n   Hash:  %s\n"),
                     rfi->size(), static_cast<INT64>(rfi->modificationTime()),
                     BinToStr(rfi->hash(), MD5_DIGEST_SIZE, hash));
         }
      }
      delete info;
   }
   else
   {
      WriteToTerminalEx(_T("%d: %s\n"), rcc, AgentErrorCodeToText(rcc));
   }
   return (rcc == ERR_SUCCESS) ? 0 : 1;
}

/**
 * Parser for nxget specific options
 */
static bool ParseAdditionalOptionCb(const char ch, const char *optarg)
{
   int i;
   bool start = true;
   char *eptr;
   switch(ch)
   {
      case 'b':   // Batch mode
         s_batchMode = TRUE;
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
            start = false;
         }
         else
         {
            s_interval = i;
         }
         break;
      case 'E':
         s_operation = CMD_GET_SCREENSHOT;
         break;
      case 'F':
         s_operation = CMD_FILE_SET_INFO;
         break;
      case 'I':
         s_operation = CMD_GET_PARAMS;
         break;
      case 'C':
         s_operation = CMD_GET_CONFIG;
         break;
      case 'l':
         s_operation = CMD_LIST;
         break;
      case 'T':
         s_operation = CMD_TABLE;
         break;
      case 'n':   // Show names
         s_showNames = TRUE;
         break;
      case 'N':   // Check service
         s_operation = CMD_CHECK_SERVICE;
         s_serviceAddr = ntohl(inet_addr(optarg));
         if ((s_serviceAddr == INADDR_NONE) || (s_serviceAddr == INADDR_ANY))
         {
            _tprintf(_T("Invalid IP address \"%hs\"\n"), optarg);
            start = FALSE;
         }
         break;
      case 'P':   // Port number for service check
         i = strtol(optarg, &eptr, 0);
         if ((*eptr != 0) || (i < 0) || (i > 65535))
         {
            printf("Invalid port number \"%s\"\n", optarg);
            start = FALSE;
         }
         else
         {
            s_servicePort = (WORD)i;
         }
         break;
      case 'r':   // Service check request string
#ifdef UNICODE
#if HAVE_MBSTOWCS
         mbstowcs(s_serviceRequest, optarg, MAX_DB_STRING);
#else
         MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, s_serviceRequest, MAX_DB_STRING);
#endif
         s_serviceRequest[MAX_DB_STRING - 1] = 0;
#else
         strlcpy(s_serviceRequest, optarg, MAX_DB_STRING);
#endif
         break;
      case 'R':   // Service check response string
#ifdef UNICODE
#if HAVE_MBSTOWCS
         mbstowcs(s_serviceResponse, optarg, MAX_DB_STRING);
#else
         MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, s_serviceResponse, MAX_DB_STRING);
#endif
         s_serviceResponse[MAX_DB_STRING - 1] = 0;
#else
         strlcpy(s_serviceResponse, optarg, MAX_DB_STRING);
#endif
         break;
      case 't':   // Service type
         s_serviceType = strtol(optarg, &eptr, 0);
         if (*eptr != 0)
         {
            if (!stricmp(optarg, "custom"))
            {
               s_serviceType = NETSRV_CUSTOM;
            }
            else if (!stricmp(optarg, "ftp"))
            {
               s_serviceType = NETSRV_FTP;
            }
            else if (!stricmp(optarg, "http"))
            {
               s_serviceType = NETSRV_HTTP;
            }
            else if (!stricmp(optarg, "https"))
            {
               s_serviceType = NETSRV_HTTPS;
            }
            else if (!stricmp(optarg, "pop3"))
            {
               s_serviceType = NETSRV_POP3;
            }
            else if (!stricmp(optarg, "smtp"))
            {
               s_serviceType = NETSRV_SMTP;
            }
            else if (!stricmp(optarg, "ssh"))
            {
               s_serviceType = NETSRV_SSH;
            }
            else if (!stricmp(optarg, "telnet"))
            {
               s_serviceType = NETSRV_TELNET;
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
            s_serviceProto = (WORD)i;
         }
         break;
      default:
         break;
   }
   return start;
}

/**
 * Validates argument count
 */
static bool IsArgMissingCb(int currentCount)
{
   return currentCount < (((s_operation == CMD_CHECK_SERVICE) || (s_operation == CMD_GET_PARAMS) || (s_operation == CMD_GET_CONFIG)) ? 1 : 2);
}

/**
 * Execute command callback
 */
static int ExecuteCommandCb(AgentConnection *conn, int argc, char *argv[], RSA *serverKey)
{
   int exitCode = 3, pos;
#ifdef UNICODE
   WCHAR *wcValue;
#endif

   do
   {
      switch(s_operation)
      {
         case CMD_GET:
            pos = optind + 1;
            do
            {
#ifdef UNICODE
               wcValue = WideStringFromMBStringSysLocale(argv[pos++]);
               exitCode = Get(conn, wcValue, s_showNames);
               MemFree(wcValue);
#else
               exitCode = Get(conn, argv[pos++], s_showNames);
#endif
            } while((exitCode == 0) && (s_batchMode) && (pos < argc));
            break;
         case CMD_LIST:
#ifdef UNICODE
            wcValue = WideStringFromMBStringSysLocale(argv[optind + 1]);
            exitCode = List(conn, wcValue);
            MemFree(wcValue);
#else
            exitCode = List(conn, argv[optind + 1]);
#endif
            break;
         case CMD_TABLE:
#ifdef UNICODE
            wcValue = WideStringFromMBStringSysLocale(argv[optind + 1]);
            exitCode = GetTable(conn, wcValue);
            MemFree(wcValue);
#else
            exitCode = GetTable(conn, argv[optind + 1]);
#endif
            break;
         case CMD_CHECK_SERVICE:
            exitCode = CheckService(conn, s_serviceType, s_serviceAddr,
                                     s_serviceProto, s_servicePort, s_serviceRequest, s_serviceResponse);
            break;
         case CMD_GET_PARAMS:
            exitCode = ListParameters(conn);
            break;
         case CMD_GET_CONFIG:
            exitCode = GetConfig(conn);
            break;
         case CMD_GET_SCREENSHOT:
            exitCode = GetScreenshot(conn, (argc > optind + 2) ? argv[optind + 2] : "Console", argv[optind + 1]);
            break;
         case CMD_FILE_SET_INFO:
            exitCode = GetFileSetInfo(conn, argc - optind - 1, &argv[optind + 1]);
            break;
         default:
            break;
      }
      ThreadSleep(s_interval);
   }
   while(s_interval > 0);

   return exitCode;
}

/**
 * Startup
 */
int main(int argc, char *argv[])
{
   ServerCommandLineTool tool;
   tool.argc = argc;
   tool.argv = argv;
   tool.mainHelpText = _T("Usage: nxget [<options>] <host> [<parameter> [<parameter> ...]]\n")
                       _T("Tool specific options are:\n")
                       _T("   -b           : Batch mode - get all parameters listed on command line.\n")
                       _T("   -C           : Get agent's configuration file\n")
                       _T("   -d delimiter : Print table content as delimited text.\n")
                       _T("   -E           : Take screenshot. First parameter is file name, second (optional) is session name.\n")
                       _T("   -F           : Get information about given file set. Each parameter is separate file name.\n")
                       _T("   -i seconds   : Get specified parameter(s) continuously with given interval.\n")
                       _T("   -I           : Get list of supported parameters.\n")
                       _T("   -l           : Requested parameter is a list.\n")
                       _T("   -n           : Show parameter's name in result.\n")
                       _T("   -N addr      : Check state of network service at given address.\n")
                       _T("   -o proto     : Protocol number to be used for service check.\n")
                       _T("   -P port      : Network service port (to be used wth -N option).\n")
                       _T("   -r string    : Service check request string.\n")
                       _T("   -R string    : Service check expected response string.\n")
                       _T("   -t type      : Set type of service to be checked.\n")
                       _T("                  Possible types are: custom, ssh, pop3, smtp, ftp, http, https, telnet.\n")
                       _T("   -T           : Requested parameter is a table.\n");
   tool.additionalOptions = "bCd:EFi:IlnN:o:P:r:R:t:T";
   tool.executeCommandCb = &ExecuteCommandCb;
   tool.parseAdditionalOptionCb = &ParseAdditionalOptionCb;
   tool.isArgMissingCb = &IsArgMissingCb;

   return ExecuteServerCommandLineTool(&tool);
}
