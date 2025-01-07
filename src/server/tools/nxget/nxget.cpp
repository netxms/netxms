/*
** nxget - command line tool used to retrieve parameters from NetXMS agent
** Copyright (C) 2004-2024 Raden Solutions
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
#include <netxms-version.h>

NETXMS_EXECUTABLE_HEADER(nxget)

#define MAX_LINE_SIZE      4096

/**
 * Operations
 */
enum class Operation
{
   CHECK_SERVICE,
   FILE_SET_INFO,
   GET,
   GET_CONFIG,
   GET_LIST,
   GET_PARAMS,
   GET_SCREENSHOT,
   GET_SYSTEM_TIME,
   GET_TABLE,
   GET_USER_SESSIONS
};

/**
 * Static fields
 */
static TCHAR s_tableOutputDelimiter = 0;
static Operation s_operation = Operation::GET;
static bool s_showNames = false;
static bool s_batchMode = false;
static bool s_fixedType = false;
static int s_interval = 0;
static int s_serviceType = NETSRV_SSH;
static uint16_t s_servicePort = 0, s_serviceProto = 0;
static InetAddress s_serviceAddr;
static TCHAR s_serviceResponse[MAX_DB_STRING] = _T(""), s_serviceRequest[MAX_DB_STRING] = _T("");

/**
 * Get list of values for enum parameters
 */
static int GetList(AgentConnection *conn, const TCHAR *listName)
{
   StringList *data;
   uint32_t rcc = conn->getList(listName, &data);
   if (rcc == ERR_SUCCESS)
   {
      for(int i = 0; i < data->size(); i++)
      {
         _fputts(data->get(i), stdout);
         _fputtc(_T('\n'), stdout);
      }
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
static int GetTable(AgentConnection *conn, const wchar_t *tableName)
{
   Table *table;
   uint32_t rcc = conn->getTable(tableName, &table);
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
      if (!s_fixedType && (rcc == ERR_UNKNOWN_METRIC))
         return GetList(conn, tableName);
      WriteToTerminalEx(_T("%u: %s\n"), rcc, AgentErrorCodeToText(rcc));
   }
   return (rcc == ERR_SUCCESS) ? 0 : 1;
}

/**
 * Get single parameter
 */
static int Get(AgentConnection *conn, const wchar_t *metric)
{
   wchar_t buffer[1024];
   uint32_t rcc = conn->getParameter(metric, buffer, 1024);
   if (rcc == ERR_SUCCESS)
   {
      if (s_showNames)
         WriteToTerminalEx(_T("%s = %s\n"), metric, buffer);
      else
         WriteToTerminalEx(_T("%s\n"), buffer);
   }
   else
   {
      if (!s_fixedType && (rcc == ERR_UNKNOWN_METRIC))
         return GetTable(conn, metric);
      WriteToTerminalEx(_T("%d: %s\n"), rcc, AgentErrorCodeToText(rcc));
   }
   fflush(stdout);
   return (rcc == ERR_SUCCESS) ? 0 : 1;
}

/**
 * Check network service state
 */
static int CheckService(AgentConnection *pConn, int serviceType, const InetAddress& serviceAddr,
      uint16_t protocol, uint16_t port, const TCHAR *request, const TCHAR *response)
{
   UINT32 dwStatus;
   uint32_t rcc = pConn->checkNetworkService(&dwStatus, serviceAddr, serviceType, port, protocol, request, response);
   if (rcc == ERR_SUCCESS)
   {
      WriteToTerminalEx(_T("Service status: %d\n"), dwStatus);
   }
   else
   {
      WriteToTerminalEx(_T("%d: %s\n"), rcc, AgentErrorCodeToText(rcc));
   }
   return (rcc == ERR_SUCCESS) ? 0 : 1;
}

/**
 * List supported parameters
 */
static int ListParameters(AgentConnection *pConn)
{
   static const TCHAR *dataType[] = { _T("INT"), _T("UINT"), _T("INT64"), _T("UINT64"), _T("STRING"), _T("FLOAT"), _T("NULL"), _T("COUNTER32"), _T("COUNTER64"), _T("UNKNOWN") };

   ObjectArray<AgentParameterDefinition> *paramList;
   ObjectArray<AgentTableDefinition> *tableList;
   uint32_t rcc = pConn->getSupportedParameters(&paramList, &tableList);
   if (rcc == ERR_SUCCESS)
   {
      for(int i = 0; i < paramList->size(); i++)
      {
			AgentParameterDefinition *p = paramList->get(i);
         WriteToTerminalEx(_T("%s %s \"%s\"\n"), p->getName(),
            dataType[(p->getDataType() < 9) && (p->getDataType() >= 0) ? p->getDataType() : 9],
            p->getDescription());
      }
      delete paramList;
		delete tableList;
   }
   else
   {
      WriteToTerminalEx(_T("%d: %s\n"), rcc, AgentErrorCodeToText(rcc));
   }
   return (rcc == ERR_SUCCESS) ? 0 : 1;
}

/**
 * Get configuration file
 */
static int GetConfig(AgentConnection *pConn)
{
   TCHAR *content;
   size_t size;
   uint32_t rcc = pConn->readConfigFile(&content, &size);
   if (rcc == ERR_SUCCESS)
   {
      TranslateStr(content, _T("\r\n"), _T("\n"));
      _fputts(content, stdout);
      if (size > 0)
      {
         if (content[size - 1] != _T('\n'))
            fputc('\n', stdout);
      }
      else
      {
         fputc('\n', stdout);
      }
   }
   else
   {
      WriteToTerminalEx(_T("%d: %s\n"), rcc, AgentErrorCodeToText(rcc));
   }
   return (rcc == ERR_SUCCESS) ? 0 : 1;
}

/**
 * Get screenshot
 */
static int GetScreenshot(AgentConnection *pConn, const TCHAR *sessionName, const TCHAR *fileName)
{
   BYTE *data;
   size_t size;
   uint32_t rcc = pConn->takeScreenshot(sessionName, &data, &size);
   if (rcc == ERR_SUCCESS)
   {
      FILE *f = _tfopen(fileName, _T("wb"));
      if (f != nullptr)
      {
         if (data != nullptr)
            fwrite(data, 1, size, f);
         fclose(f);
      }
      MemFree(data);
   }
   else
   {
      WriteToTerminalEx(_T("%d: %s\n"), rcc, AgentErrorCodeToText(rcc));
   }
   return (rcc == ERR_SUCCESS) ? 0 : 1;
}

/**
 * Get information about file set
 */
static int GetFileSetInfo(AgentConnection *conn, int argc, TCHAR **argv)
{
   StringList fileSet;
   for(int i = 0; i < argc; i++)
      fileSet.add(argv[i]);

   ObjectArray<RemoteFileInfo> *info;
   uint32_t rcc = conn->getFileSetInfo(fileSet, true, &info);
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
 * Get user sessions
 */
static int GetUserSessions(AgentConnection *conn)
{
   ObjectArray<UserSession> *sessions;
   uint32_t rcc = conn->getUserSessions(&sessions);
   if (rcc != ERR_SUCCESS)
   {
      WriteToTerminalEx(_T("%d: %s\n"), rcc, AgentErrorCodeToText(rcc));
      return 1;
   }

   // Convert to table for simplified printing
   Table table;
   table.addColumn(_T("ID"), DCI_DT_UINT, _T("ID"), true);
   table.addColumn(_T("User Name"));
   table.addColumn(_T("Terminal"));
   table.addColumn(_T("State"));
   table.addColumn(_T("Client Name"));
   table.addColumn(_T("Client Address"));
   table.addColumn(_T("Display"));
   table.addColumn(_T("Connected at"));
   table.addColumn(_T("Logon at"));
   table.addColumn(_T("Idle Time"));
   table.addColumn(_T("Agent Type"));
   table.addColumn(_T("Agent PID"));

   for(int i = 0; i < sessions->size(); i++)
   {
      UserSession *s = sessions->get(i);
      table.addRow();
      table.set(0, s->id);
      table.set(1, s->loginName);
      table.set(2, s->terminal);
      table.set(3, s->connected ? _T("Active") : _T("Disconnected"));
      table.set(4, s->clientName);
      if (s->clientAddress.isValid())
         table.set(5, s->clientAddress.toString());
      if ((s->displayWidth > 0) && (s->displayHeight > 0) && (s->displayColorDepth > 0))
         table.set(6, StringBuffer().append(s->displayWidth).append(_T('x')).append(s->displayHeight).append(_T('x')).append(s->displayColorDepth));
      if (s->connectTime > 0)
         table.set(7, static_cast<int64_t>(s->connectTime));
      if (s->loginTime > 0)
         table.set(8, static_cast<int64_t>(s->loginTime));
      if ((s->connectTime > 0) || (s->loginTime > 0))
         table.set(9, static_cast<int64_t>(s->idleTime));
      if (s->agentType != -1)
         table.set(10, s->agentType);
      if (s->agentPID != 0)
         table.set(11, s->agentPID);
   }
   if (s_tableOutputDelimiter != 0)
      table.dump(stdout, true, s_tableOutputDelimiter);
   else
      table.writeToTerminal();

   delete sessions;
   return 0;
}

/**
 * Get remote system time
 */
static int GetSystemTime(AgentConnection *conn)
{
   int64_t remoteTime;
   int32_t offset;
   uint32_t rtt;
   bool allowSync;
   uint32_t rcc = conn->getRemoteSystemTime(&remoteTime, &offset, &rtt, &allowSync);
   if (rcc != ERR_SUCCESS)
   {
      WriteToTerminalEx(_T("%d: %s\n"), rcc, AgentErrorCodeToText(rcc));
      return 1;
   }

   time_t t = static_cast<time_t>(remoteTime / 1000);
   WriteToTerminalEx(_T("Time..........: %s.%03d\n"), FormatTimestamp(t).cstr(), static_cast<int>(remoteTime % 1000));
   WriteToTerminalEx(_T("UNIX time.....: ") INT64_FMT _T("\n"), remoteTime);
   WriteToTerminalEx(_T("Offset........: %d ms\n"), offset);
   WriteToTerminalEx(_T("RTT...........: %u ms\n"), rtt);
   WriteToTerminalEx(_T("Sync allowed..: %s\n"), BooleanToString(allowSync));

   return 0;
}

/**
 * Parser for nxget specific options
 */
static bool ParseAdditionalOptionCb(const char ch, const wchar_t *optarg)
{
   int i;
   bool start = true;
   TCHAR *eptr;
   switch(ch)
   {
      case 'b':   // Batch mode
         s_batchMode = true;
         s_fixedType = true;
         break;
      case 'd':   // Dump table
         s_tableOutputDelimiter = *optarg;
         break;
      case 'i':   // Interval
         i = _tcstol(optarg, &eptr, 0);
         if ((*eptr != 0) || (i <= 0))
         {
            _tprintf(_T("Invalid interval \"%s\"\n"), optarg);
            start = false;
         }
         else
         {
            s_interval = i;
         }
         break;
      case 'E':
         s_operation = Operation::GET_SCREENSHOT;
         break;
      case 'f':
         s_fixedType = true;
         break;
      case 'F':
         s_operation = Operation::FILE_SET_INFO;
         break;
      case 'I':
         s_operation = Operation::GET_PARAMS;
         break;
      case 'C':
         s_operation = Operation::GET_CONFIG;
         break;
      case 'l':
         s_operation = Operation::GET_LIST;
         s_fixedType = true;
         break;
      case 'n':   // Show names
         s_showNames = true;
         break;
      case 'N':   // Check service
         s_operation = Operation::CHECK_SERVICE;
         s_serviceAddr = InetAddress::parse(optarg);
         if (!s_serviceAddr.isValidUnicast())
         {
            _tprintf(_T("Invalid IP address \"%s\"\n"), optarg);
            start = false;
         }
         break;
      case 'o':   // Protocol number for service check
         i = _tcstol(optarg, &eptr, 0);
         if ((*eptr != 0) || (i < 0) || (i > 65535))
         {
            _tprintf(_T("Invalid protocol number \"%s\"\n"), optarg);
            start = false;
         }
         else
         {
            s_serviceProto = static_cast<uint16_t>(i);
         }
         break;
      case 'P':   // Port number for service check
         i = _tcstol(optarg, &eptr, 0);
         if ((*eptr != 0) || (i < 0) || (i > 65535))
         {
            _tprintf(_T("Invalid port number \"%s\"\n"), optarg);
            start = false;
         }
         else
         {
            s_servicePort = static_cast<uint16_t>(i);
         }
         break;
      case 'r':   // Service check request string
         _tcslcpy(s_serviceRequest, optarg, MAX_DB_STRING);
         break;
      case 'R':   // Service check response string
         _tcslcpy(s_serviceResponse, optarg, MAX_DB_STRING);
         break;
      case 't':   // Service type
         s_serviceType = _tcstol(optarg, &eptr, 0);
         if (*eptr != 0)
         {
            if (!_tcsicmp(optarg, _T("custom")))
            {
               s_serviceType = NETSRV_CUSTOM;
            }
            else if (!_tcsicmp(optarg, _T("ftp")))
            {
               s_serviceType = NETSRV_FTP;
            }
            else if (!_tcsicmp(optarg, _T("http")))
            {
               s_serviceType = NETSRV_HTTP;
            }
            else if (!_tcsicmp(optarg, _T("https")))
            {
               s_serviceType = NETSRV_HTTPS;
            }
            else if (!_tcsicmp(optarg, _T("pop3")))
            {
               s_serviceType = NETSRV_POP3;
            }
            else if (!_tcsicmp(optarg, _T("smtp")))
            {
               s_serviceType = NETSRV_SMTP;
            }
            else if (!_tcsicmp(optarg, _T("ssh")))
            {
               s_serviceType = NETSRV_SSH;
            }
            else if (!_tcsicmp(optarg, _T("telnet")))
            {
               s_serviceType = NETSRV_TELNET;
            }
            else
            {
               _tprintf(_T("Invalid service type \"%s\"\n"), optarg);
               start = false;
            }
         }
         break;
      case 'T':
         s_operation = Operation::GET_TABLE;
         s_fixedType = true;
         break;
      case 'U':
         s_operation = Operation::GET_USER_SESSIONS;
         break;
      case 'Y':
         s_operation = Operation::GET_SYSTEM_TIME;
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
   return currentCount < (((s_operation == Operation::CHECK_SERVICE) || (s_operation == Operation::GET_PARAMS) || (s_operation == Operation::GET_CONFIG) || (s_operation == Operation::GET_SYSTEM_TIME) || (s_operation == Operation::GET_USER_SESSIONS)) ? 1 : 2);
}

/**
 * Execute command callback
 */
static int ExecuteCommandCb(AgentConnection *conn, int argc, wchar_t **argv, int optind, RSA_KEY serverKey)
{
   int exitCode = 3, pos;

   do
   {
      switch(s_operation)
      {
         case Operation::CHECK_SERVICE:
            exitCode = CheckService(conn, s_serviceType, s_serviceAddr, s_serviceProto, s_servicePort, s_serviceRequest, s_serviceResponse);
            break;
         case Operation::FILE_SET_INFO:
            exitCode = GetFileSetInfo(conn, argc - optind - 1, &argv[optind + 1]);
            break;
         case Operation::GET:
            pos = optind + 1;
            do
            {
               exitCode = Get(conn, argv[pos++]);
            } while((exitCode == 0) && (s_batchMode) && (pos < argc));
            break;
         case Operation::GET_CONFIG:
            exitCode = GetConfig(conn);
            break;
         case Operation::GET_LIST:
            exitCode = GetList(conn, argv[optind + 1]);
            break;
         case Operation::GET_PARAMS:
            exitCode = ListParameters(conn);
            break;
         case Operation::GET_SCREENSHOT:
            exitCode = GetScreenshot(conn, (argc > optind + 2) ? argv[optind + 2] : L"Console", argv[optind + 1]);
            break;
         case Operation::GET_SYSTEM_TIME:
            exitCode = GetSystemTime(conn);
            break;
         case Operation::GET_TABLE:
            exitCode = GetTable(conn, argv[optind + 1]);
            break;
         case Operation::GET_USER_SESSIONS:
            exitCode = GetUserSessions(conn);
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
#ifdef _WIN32
int wmain(int argc, wchar_t *argv[])
#else
int main(int argc, char *argv[])
#endif
{
   ServerCommandLineTool tool;
   tool.argc = argc;
   tool.argv = argv;
   tool.displayName = L"Agent Query Tool";
   tool.mainHelpText = L"Usage: nxget [<options>] <host> [<parameter> [<parameter> ...]]\n"
                       L"Tool specific options are:\n"
                       L"   -b           : Batch mode - get all parameters listed on command line.\n"
                       L"   -C           : Get agent's configuration file\n"
                       L"   -d delimiter : Print table content as delimited text.\n"
                       L"   -E           : Take screenshot. First parameter is file name, second (optional) is session name.\n"
                       L"   -f           : Do not try lists and tables if requested metric does not exist.\n"
                       L"   -F           : Get information about given file set. Each parameter is separate file name.\n"
                       L"   -i seconds   : Get specified parameter(s) continuously with given interval.\n"
                       L"   -I           : Get list of supported parameters.\n"
                       L"   -l           : Requested parameter is a list.\n"
                       L"   -n           : Show parameter's name in result.\n"
                       L"   -N addr      : Check state of network service at given address.\n"
                       L"   -o proto     : Protocol number to be used for service check.\n"
                       L"   -P port      : Network service port (to be used wth -N option).\n"
                       L"   -r string    : Service check request string.\n"
                       L"   -R string    : Service check expected response string.\n"
                       L"   -t type      : Set type of service to be checked.\n"
                       L"                  Possible types are: custom, ssh, pop3, smtp, ftp, http, https, telnet.\n"
                       L"   -T           : Requested parameter is a table.\n"
                       L"   -U           : Get list of active user sessions.\n"
                       L"   -Y           : Read remote system time.\n";
   tool.additionalOptions = "bCd:EfFi:IlnN:o:P:r:R:t:TUY";
   tool.executeCommandCb = &ExecuteCommandCb;
   tool.parseAdditionalOptionCb = &ParseAdditionalOptionCb;
   tool.isArgMissingCb = &IsArgMissingCb;

   return ExecuteServerCommandLineTool(&tool);
}
