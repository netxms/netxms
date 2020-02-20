/*
** nxwsget - command line tool used to retrieve web service parameters
**           from NetXMS agent
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
** File: nxwsget.cpp
**
**/

#include <nms_util.h>
#include <nms_agent.h>
#include <nxsrvapi.h>

NETXMS_EXECUTABLE_HEADER(nxwsget)

#define MAX_LINE_SIZE      4096
#define MAX_CRED_LEN       128

/**
 * Static fields
 */
static UINT32 s_interval = 0;
static UINT32 s_retentionTime = 60;
static TCHAR s_login[MAX_CRED_LEN] = _T("");
static TCHAR s_password[MAX_CRED_LEN] = _T("");
static WebServiceAuthType s_authType = WebServiceAuthType::NONE;
static StringMap s_headers;
static bool s_verifyCert = true;

/**
 * Callback for printing results
 */
static EnumerationCallbackResult PrintResults(const TCHAR *key, const void *value, void *data)
{
   WriteToTerminalEx(_T("%s = %s\n"), key, value);
   return _CONTINUE;
}

/**
 * Get service parameter
 */
static int QueryWebService(AgentConnection *pConn, const TCHAR *url, const StringList& parameters)
{
   StringMap results;
   UINT32 rcc = pConn->queryWebService(url, s_retentionTime,
            (s_login[0] == 0) ? NULL: s_login, (s_password[0] == 0) ? NULL: s_password,
            s_authType, s_headers, parameters, s_verifyCert, &results);
   if (rcc == ERR_SUCCESS)
   {
      results.forEach(PrintResults, NULL);
   }
   else
   {
      WriteToTerminalEx(_T("%d: %s\n"), rcc, AgentErrorCodeToText(rcc));
   }
   fflush(stdout);
   return (rcc == ERR_SUCCESS) ? 0 : 1;
}

/**
 * Parse nxwsget specific parameters
 */
static bool ParseAdditionalOptionCb(const char ch, const char *optarg)
{
   int i;
   bool start = true;
   char *eptr;
   TCHAR header[1024];
   TCHAR *s;

   switch(ch)
   {
      case 'c':   // disable certificate check
         s_verifyCert = false;
         break;
      case 'D':   // debug level
         nxlog_set_debug_level((int)strtol(optarg, NULL, 0));
         break;
      case 'i':   // Interval
         i = strtol(optarg, &eptr, 0);
         if ((*eptr != 0) || (i <= 0))
         {
            _tprintf(_T("Invalid interval \"%hs\"\n"), optarg);
            start = false;
         }
         else
         {
            s_interval = i;
         }
         break;
      case 'H':   // Header
#ifdef UNICODE
#if HAVE_MBSTOWCS
         mbstowcs(header, optarg, 1024);
#else
         MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, header, 1024);
#endif
         header[1023] = 0;
#else
         strlcpy(header, optarg, 1024);
#endif
         s = _tcschr(header, _T(':'));
         if (s != NULL)
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
      case 'L':   // Login
#ifdef UNICODE
#if HAVE_MBSTOWCS
         mbstowcs(s_login, optarg, MAX_CRED_LEN);
#else
         MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, s_login, MAX_CRED_LEN);
#endif
         s_login[MAX_CRED_LEN - 1] = 0;
#else
         strlcpy(s_login, optarg, MAX_CRED_LEN);
#endif
         break;
      case 'P':   // Password
#ifdef UNICODE
#if HAVE_MBSTOWCS
         mbstowcs(s_password, optarg, MAX_CRED_LEN);
#else
         MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, s_password, MAX_CRED_LEN);
#endif
         s_password[MAX_CRED_LEN - 1] = 0;
#else
         strlcpy(s_password, optarg, MAX_CRED_LEN);
#endif
         break;
      case 'r':   // Retention time
         s_retentionTime = strtol(optarg, &eptr, 0);
         if ((*eptr != 0) || (s_retentionTime < 1))
         {
            printf("Invalid retention time \"%s\"\n", optarg);
            start = false;
         }
         break;
      case 't':
         if (!strcmp(optarg, "any"))
            s_authType = WebServiceAuthType::ANY;
         else if (!strcmp(optarg, "anysafe"))
            s_authType = WebServiceAuthType::ANYSAFE;
         else if (!strcmp(optarg, "basic"))
            s_authType = WebServiceAuthType::BASIC;
         else if (!strcmp(optarg, "bearer"))
            s_authType = WebServiceAuthType::BEARER;
         else if (!strcmp(optarg, "digest"))
            s_authType = WebServiceAuthType::DIGEST;
         else if (!strcmp(optarg, "none"))
            s_authType = WebServiceAuthType::NONE;
         else if (!strcmp(optarg, "ntlm"))
            s_authType = WebServiceAuthType::NTLM;
         else
         {
            _tprintf(_T("Invalid authentication method \"%hs\"\n"), optarg);
            start = false;
         }
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
static int ExecuteCommandCb(AgentConnection *conn, int argc, char *argv[], RSA *serverKey)
{
   int exitCode = 3, pos;

   do
   {
      TCHAR *url;
      StringList parameters;
      pos = optind + 1;
#ifdef UNICODE
      url = WideStringFromMBStringSysLocale(argv[pos++]);
#else
      url = MemCopyStringA(argv[pos++]);
#endif
      while (pos < argc)
      {
#ifdef UNICODE
         parameters.addPreallocated(WideStringFromMBStringSysLocale(argv[pos++]));
#else
         parameters.add(argv[pos++]);
#endif
      }
      exitCode = QueryWebService(conn, url, parameters);
      ThreadSleep(s_interval);
      MemFree(url);
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
   tool.mainHelpText = _T("Usage: Usage: nxwsget [<options>] <host> <URL> <path> [<path> ...]\n\n")
                       _T("Tool specific options:\n")
                       _T("   -c           : Do not verify service certificate.\n")
                       _T("   -H header    : HTTP header (can be used multiple times).\n")
                       _T("   -i seconds   : Query service continuously with given interval.\n")
                       _T("   -L login     : Web service login name.\n")
                       _T("   -P passwod   : Web service password.\n")
                       _T("   -r seconds   : Cache retention time.\n")
                       _T("   -t auth      : HTTP authentication type. Valid methods are \"none\", \"basic\", \"digest\",\n")
                       _T("                  \"ntlm\", \"bearer\", \"any\", or \"anysafe\". Default is \"none\".\n");
   tool.additionalOptions = "cH:i:L:P:r:t:";
   tool.executeCommandCb = &ExecuteCommandCb;
   tool.parseAdditionalOptionCb = &ParseAdditionalOptionCb;
   tool.isArgMissingCb = &IsArgMissingCb;

   return ExecuteServerCommandLineTool(&tool);
}
