/*
** nxwsget - command line tool used to query web services via NetXMS agent
** Copyright (C) 2004-2023 Raden Solutions
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
** File: nxwsget.cpp
**
**/

#include <nms_util.h>
#include <nms_agent.h>
#include <nxsrvapi.h>
#include <netxms-version.h>

NETXMS_EXECUTABLE_HEADER(nxwsget)

#define MAX_CRED_LEN       128

/**
 * Static fields
 */
static uint32_t s_interval = 0;
static uint32_t s_retentionTime = 60;
static TCHAR s_login[MAX_CRED_LEN] = _T("");
static TCHAR s_password[MAX_CRED_LEN] = _T("");
static TCHAR *s_requestData = nullptr;
static HttpRequestMethod s_httpRequestMethod = HttpRequestMethod::_GET;
static WebServiceAuthType s_authType = WebServiceAuthType::NONE;
static StringMap s_headers;
static bool s_verifyCert = true;
static bool s_verifyHost = true;
static bool s_followLocation = false;
static bool s_parseAsPlainText = false;
static WebServiceRequestType s_requestType = WebServiceRequestType::PARAMETER;

/**
 * Callback for printing results
 */
static EnumerationCallbackResult PrintParameterResults(const TCHAR *key, const void *value, void *data)
{
   WriteToTerminalEx(_T("%s = %s\n"), key, value);
   return _CONTINUE;
}

/**
 * Get service parameter
 */
static int QueryWebService(AgentConnection *pConn, const TCHAR *url, const StringList& parameters)
{
   uint32_t rcc = ERR_SUCCESS;
   switch (s_requestType)
   {
      case WebServiceRequestType::PARAMETER:
      {
         StringMap results;
         rcc = pConn->queryWebServiceParameters(url, s_httpRequestMethod, s_requestData, (pConn->getCommandTimeout() - 500) / 1000,
            s_retentionTime, (s_login[0] == 0) ? nullptr : s_login, (s_password[0] == 0) ? nullptr: s_password, s_authType,
            s_headers, parameters, s_verifyCert, s_verifyHost, s_followLocation, s_parseAsPlainText, &results);
         if (rcc == ERR_SUCCESS)
         {
            results.forEach(PrintParameterResults, nullptr);
         }
         break;
      }
      case WebServiceRequestType::LIST:
      {
         StringList results;
         rcc = pConn->queryWebServiceList(url, s_httpRequestMethod, s_requestData, (pConn->getCommandTimeout() - 500) / 1000,
            s_retentionTime, (s_login[0] == 0) ? nullptr: s_login, (s_password[0] == 0) ? nullptr: s_password, s_authType,
            s_headers, parameters.get(0), s_verifyCert, s_verifyHost, s_followLocation, s_parseAsPlainText, &results);
         if (rcc == ERR_SUCCESS)
         {
            for (int i = 0; i < results.size(); i++)
               WriteToTerminalEx(_T("%s\n"), results.get(i));
         }
         break;
      }
   }

   if (rcc != ERR_SUCCESS)
   {
      WriteToTerminalEx(_T("%d: %s\n"), rcc, AgentErrorCodeToText(rcc));
   }
   fflush(stdout);
   return (rcc == ERR_SUCCESS) ? 0 : 1;
}

/**
 * Parse nxwsget specific parameters
 */
static bool ParseAdditionalOptionCb(const char ch, const TCHAR *optarg)
{
   int i;
   bool start = true;
   TCHAR *eptr;
   TCHAR header[1024];
   TCHAR *s;

   switch(ch)
   {
      case 'a':
         if (!_tcsicmp(optarg, _T("any")))
            s_authType = WebServiceAuthType::ANY;
         else if (!_tcsicmp(optarg, _T("anysafe")))
            s_authType = WebServiceAuthType::ANYSAFE;
         else if (!_tcsicmp(optarg, _T("basic")))
            s_authType = WebServiceAuthType::BASIC;
         else if (!_tcsicmp(optarg, _T("bearer")))
            s_authType = WebServiceAuthType::BEARER;
         else if (!_tcsicmp(optarg, _T("digest")))
            s_authType = WebServiceAuthType::DIGEST;
         else if (!_tcsicmp(optarg, _T("none")))
            s_authType = WebServiceAuthType::NONE;
         else if (!_tcsicmp(optarg, _T("ntlm")))
            s_authType = WebServiceAuthType::NTLM;
         else
         {
            _tprintf(_T("Invalid authentication method \"%s\"\n"), optarg);
            start = false;
         }
         break;
      case 'c':   // disable certificate check
         s_verifyCert = false;
         break;
      case 'C':   // disable certificate's host check
         s_verifyHost = false;
         break;
      case 'd':   // request data
         s_requestData = MemCopyString(optarg);
         break;
      case 'F':   // follow location
         s_followLocation = true;
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
      case 'H':   // Header
         _tcslcpy(header, optarg, 1024);
         s = _tcschr(header, _T(':'));
         if (s != nullptr)
         {
            *s++ = 0;
            while(_istspace(*s))
               s++;
            s_headers.set(header, s);
         }
         else
         {
            s_headers.set(header, _T(""));
         }
         break;
      case 'l':
         s_requestType = WebServiceRequestType::LIST;
         break;
      case 'm':
         if (!_tcsicmp(optarg, _T("GET")))
            s_httpRequestMethod = HttpRequestMethod::_GET;
         else if (!_tcsicmp(optarg, _T("POST")))
            s_httpRequestMethod = HttpRequestMethod::_POST;
         else if (!_tcsicmp(optarg, _T("PUT")))
            s_httpRequestMethod = HttpRequestMethod::_PUT;
         else if (!_tcsicmp(optarg, _T("PATCH")))
            s_httpRequestMethod = HttpRequestMethod::_PATCH;
         else if (!_tcsicmp(optarg, _T("DELETE")))
            s_httpRequestMethod = HttpRequestMethod::_DELETE;
         else
         {
            _tprintf(_T("Invalid HTTP request method \"%s\"\n"), optarg);
            start = false;
         }
         break;
      case 'L':   // Login
         _tcslcpy(s_login, optarg, MAX_CRED_LEN);
         break;
      case 'P':   // Password
         _tcslcpy(s_password, optarg, MAX_CRED_LEN);
         break;
      case 'r':   // Retention time
         s_retentionTime = _tcstol(optarg, &eptr, 0);
         if ((*eptr != 0) || (s_retentionTime < 1))
         {
            _tprintf(_T("Invalid retention time \"%s\"\n"), optarg);
            start = false;
         }
         break;
      case 't':   // debug level
         s_parseAsPlainText = true;
         break;
      default:
         break;
   }
   return start;
}

/**
 * Validate parameter count
 */
static bool IsArgMissingCb(int currentCount)
{
   return currentCount < 3;
}

/**
 * Execute command callback
 */
static int ExecuteCommandCb(AgentConnection *conn, int argc, TCHAR **argv, int optind, RSA_KEY serverKey)
{
   int exitCode = 3, pos;
   do
   {
      StringList parameters;
      pos = optind + 1;
      const TCHAR *url = argv[pos++];
      while (pos < argc)
         parameters.add(argv[pos++]);
      exitCode = QueryWebService(conn, url, parameters);
      ThreadSleep(s_interval);
   }
   while(s_interval > 0);
   return exitCode;
}

/**
 * Startup
 */
#ifdef _WIN32
int _tmain(int argc, TCHAR *argv[])
#else
int main(int argc, char *argv[])
#endif
{
   ServerCommandLineTool tool;
   tool.argc = argc;
   tool.argv = argv;
   tool.displayName = _T("Web Service Query Tool");
   tool.mainHelpText = _T("Usage: nxwsget [<options>] <host> <URL> <path> [<path> ...]\n\n")
                       _T("Tool specific options:\n")
                       _T("   -a auth      : HTTP authentication type. Valid methods are \"none\", \"basic\", \"digest\",\n")
                       _T("                  \"ntlm\", \"bearer\", \"any\", or \"anysafe\". Default is \"none\".\n")
                       _T("   -c           : Do not verify service certificate.\n")
                       _T("   -C           : Do not verify certificate's name against host.\n")
                       _T("   -d data      : Request data.\n")
                       _T("   -F           : Follow location header that the server sends as part of a 3xx response.\n")
                       _T("   -H header    : HTTP header (can be used multiple times).\n")
                       _T("   -i seconds   : Query service continuously with given interval.\n")
                       _T("   -l           : Requested parameter is a list.\n")
                       _T("   -m method    : HTTP request method. Valid methods are GET, POST, PUT, PATCH, DELETE.\n")
                       _T("   -L login     : Web service login name.\n")
                       _T("   -P passwod   : Web service password.\n")
                       _T("   -r seconds   : Cache retention time.\n")
                       _T("   -t           : Use text parsing.\n");
   tool.additionalOptions = "a:cCd:FH:i:lm:L:P:r:t";
   tool.executeCommandCb = &ExecuteCommandCb;
   tool.parseAdditionalOptionCb = &ParseAdditionalOptionCb;
   tool.isArgMissingCb = &IsArgMissingCb;

   return ExecuteServerCommandLineTool(&tool);
}
