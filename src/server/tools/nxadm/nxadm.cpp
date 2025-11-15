/* 
** NetXMS - Network Management System
** Local administration tool
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
** File: nxadm.cpp
**
**/

#include "nxadm.h"
#include <nxcldefs.h>
#include <netxms_getopt.h>
#include <netxms-editline.h>
#include <netxms-version.h>

NETXMS_EXECUTABLE_HEADER(nxadm)

/**
 * Display help
 */
static void Help()
{
   printf("NetXMS Server Administration Tool  Version " NETXMS_VERSION_STRING_A "\n"
          "Copyright (c) 2004-2025 Raden Solutions\n\n"
          "Usage: nxadm [-u <login>] [-P|-p <password>] -c <command>\n"
          "       nxadm [-u <login>] [-P|-p <password>] -i\n"
          "       nxadm [-u <login>] [-P|-p <password>] [-r] -s <script>\n"
          "       nxadm -P\n"
          "       nxadm -p <db password>\n"
          "       nxadm -R\n"
          "       nxadm -h\n"
          "       nxadm -v\n\n"
          "Options:\n"
          "   -c <command>   Execute given command at server debug console and disconnect\n"
          "   -i             Connect to server debug console in interactive mode\n"
          "   -h             Display help and exit\n"
          "   -p <password>  Provide database password for server startup or\n"
          "                  user's password for console access\n"
          "   -P             Provide database password for server startup or\n"
          "                  user's password for console access (password read from terminal)\n"
          "   -r             Use script's return value as exit code\n"
          "   -R             Check if server is running (started and completed initialization)\n"
          "   -s <script>    Execute given NXSL script and disconnect\n"
          "   -u name        User name for authentication\n"
          "   -v             Display version and exit\n\n");
}

/**
 * Get server error text
 */
static const char *GetServerErrorText(uint32_t rcc)
{
   switch(rcc)
   {
      case RCC_SUCCESS:
         return "Success";
      case RCC_ACCESS_DENIED:
         return "Access denied";
      case RCC_NOT_IMPLEMENTED:
         return "Command not implemented";
      case RCC_RESOURCE_NOT_AVAILABLE:
         return "Server initialization is not completed yet";
   }
   return "Internal error";
}

/**
 * Send database password
 */
static bool SendPassword(const TCHAR *password)
{
   NXCPMessage msg(CMD_SET_DB_PASSWORD, g_requestId++);
   msg.setField(VID_PASSWORD, password);
   SendMsg(msg);

   bool success = false;
   NXCPMessage *response = RecvMsg();
   if (response != nullptr)
   {
      uint32_t rcc = response->getFieldAsUInt32(VID_RCC);
      if (rcc != 0)
         printf("ERROR: server error %u (%s)\n", rcc, GetServerErrorText(rcc));
      delete response;
      success = (rcc == 0);
   }
   else
   {
      printf("ERROR: no response from server\n");
   }

   return success;
}

/**
 * Check if server is running
 */
static int ServerRunCheck()
{
   NXCPMessage msg(CMD_GET_SERVER_INFO, g_requestId++);
   SendMsg(msg);

   int exitCode;
   NXCPMessage *response = RecvMsg();
   if (response != nullptr)
   {
      uint32_t rcc = response->getFieldAsUInt32(VID_RCC);
      if (rcc == RCC_SUCCESS)
      {
         WriteToTerminal(L"Server is running\n");
         exitCode = 0;  // Server is running
      }
      else if (rcc == RCC_RESOURCE_NOT_AVAILABLE)
      {
         WriteToTerminal(L"Server is starting\n");
         exitCode = 1;  // Server is starting
      }
      else
      {
         WriteToTerminalEx(L"ERROR: server error %u (%s)\n", rcc, GetServerErrorText(rcc));
         exitCode = 3;  // Other error
      }
      delete response;
   }
   else
   {
      WriteToTerminal(L"ERROR: no response from server\n");
      exitCode = 2;  // Communication error
   }

   return exitCode;
}

/**
 * Login to the server
 */
static bool Login(const TCHAR *login, const TCHAR *password)
{
   NXCPMessage msg(CMD_LOGIN, g_requestId++);
   msg.setField(VID_LOGIN_NAME, login);
   msg.setField(VID_PASSWORD, CHECK_NULL_EX(password));
   SendMsg(msg);

   NXCPMessage *response = RecvMsg();
   if (response == nullptr)
   {
      printf("Authentication failed (no response from server)\n");
      return false;
   }

   uint32_t rcc = response->getFieldAsUInt32(VID_RCC);
   delete response;
   if (rcc != RCC_SUCCESS)
   {
      printf("Authentication failed (server error %u: %s)\n", rcc, GetServerErrorText(rcc));
      return false;
   }
   return true;
}

/**
 * Execute command
 */
static bool ExecCommand(const TCHAR *command, const TCHAR *login, const TCHAR *password)
{
   bool connClosed = false;

   if (login != nullptr)
   {
      if (!Login(login, password))
         return false;
   }

   NXCPMessage msg(CMD_ADM_REQUEST, g_requestId++);
   msg.setField(VID_COMMAND, command);
   SendMsg(msg);

   while(true)
   {
      NXCPMessage *response = RecvMsg();
      if (response == nullptr)
      {
         printf("Connection closed\n");
         connClosed = true;
         break;
      }

      uint16_t code = response->getCode();
      if (code == CMD_ADM_MESSAGE)
      {
         TCHAR *text = response->getFieldAsString(VID_MESSAGE);
         if (text != nullptr)
         {
            WriteToTerminal(text);
            MemFree(text);
         }
      }
      else if (code == CMD_REQUEST_COMPLETED)
      {
         uint32_t rcc = response->getFieldAsUInt32(VID_RCC);
         if (rcc != RCC_SUCCESS)
            printf("Server error %u (%s)\n", rcc, GetServerErrorText(rcc));
         delete response;
         break;
      }
      delete response;
   }

   return connClosed;
}


/**
 * Execute script
 */
static int ExecScript(const TCHAR *script, const TCHAR *login, const TCHAR *password, bool useReturnValue)
{
   if (login != nullptr)
   {
      if (!Login(login, password))
         return RCC_ACCESS_DENIED;
   }

   NXCPMessage msg(CMD_ADM_REQUEST, g_requestId++);
   msg.setField(VID_SCRIPT, script);
   SendMsg(msg);

   int rcc;
   while(true)
   {
      NXCPMessage *response = RecvMsg();
      if (response == nullptr)
      {
         printf("Connection closed\n");
         rcc = RCC_COMM_FAILURE;
         break;
      }

      uint16_t code = response->getCode();
      if (code == CMD_ADM_MESSAGE)
      {
         TCHAR *text = response->getFieldAsString(VID_MESSAGE);
         if (text != nullptr)
         {
            WriteToTerminal(text);
            MemFree(text);
         }
      }
      else if (code == CMD_REQUEST_COMPLETED)
      {
         rcc = response->getFieldAsInt32(VID_RCC);
         if (rcc == RCC_SUCCESS)
         {
            if (useReturnValue)
               rcc = response->getFieldAsInt32(VID_EXECUTION_RESULT);
         }
         else
         {
            printf("Server error %u (%s)\n", rcc, GetServerErrorText(rcc));
         }
         delete response;
         break;
      }
      delete response;
   }

   return rcc;
}

#if HAVE_LIBEDIT

/**
 * Get command line prompt
 */
static wchar_t *Prompt(EditLine *el)
{
   //static TCHAR prompt[] = _T("\1\x1b[33mnetxmsd:\x1b[0m\1 ");
   static TCHAR prompt[] = _T("netxmsd: ");
   return prompt;
}

#endif

/**
 * Interactive mode loop
 */
static void Shell(const TCHAR *login, const TCHAR *password)
{
   NXCPMessage msg(CMD_GET_SERVER_INFO, g_requestId++);
   SendMsg(msg);
   NXCPMessage *response = RecvMsg();
   if (response == nullptr)
   {
      printf("Connection closed\n");
      return;
   }

   bool authenticationRequired = response->getFieldAsBoolean(VID_AUTH_TYPE);
   delete response;

   TCHAR loginBuffer[256], passwordBuffer[MAX_PASSWORD];
   if (authenticationRequired && (login == nullptr))
   {
      printf("Server requires authentication. Please provide login and password.\n\nLogin: ");
      fflush(stdout);

      if (_fgetts(loginBuffer, 255, stdin) == nullptr)
         return;   // Error reading stdin

      TCHAR *nl = _tcschr(loginBuffer, '\n');
      if (nl != nullptr)
         *nl = 0;

      if (!ReadPassword(_T("Password: "), passwordBuffer, MAX_PASSWORD))
      {
         printf("Cannot read password from terminal\n");
         return;
      }

      login = loginBuffer;
      password = passwordBuffer;
   }

   if (login != nullptr)
   {
      if (!Login(login, password))
         return;
   }

   printf("\nNetXMS Server Remote Console V" NETXMS_VERSION_STRING_A " Ready\n"
          "Enter \"help\" for command list\n\n");

#if HAVE_LIBEDIT
   HistoryT *h = history_tinit();

   HistEventT ev;
   history_t(h, &ev, H_SETSIZE, 100);  // Remember 100 events
   history_t(h, &ev, H_LOAD, ".nxadm_history");

   EditLine *el = el_init("nxadm", stdin, stdout, stderr);
   el_tset(el, EL_PROMPT, Prompt);
   el_tset(el, EL_EDITOR, "emacs");
   el_tset(el, EL_HIST, history_t, h);
   el_tset(el, EL_SIGNAL, 1);
   el_tset(el, EL_BIND, _T("^K"), _T("ed-kill-line"), nullptr);
   el_tset(el, EL_BIND, _T("^L"), _T("ed-clear-screen"), nullptr);
   el_source(el, nullptr);
#endif

   TCHAR command[1024];
   while(true)
   {
#if HAVE_LIBEDIT
      int numchars;
      const TCHAR *line = el_tgets(el, &numchars);
      if ((line == nullptr) || (numchars == 0))
         break;

      _tcslcpy(command, line, 1024);
#else
      WriteToTerminal(_T("\x1b[33mnetxmsd:\x1b[0m "));
      fflush(stdout);

      if (_fgetts(command, 1023, stdin) == nullptr)
         break;   // Error reading stdin
#endif

      Trim(command);
      if (command[0] == '!')
      {
#if HAVE_LIBEDIT
         HistEventT p;
         int rc;
         if (command[1] == '!')
         {
            rc = history_t(h, &p, H_CURR);
         }
         else
         {
            int n = _tcstol(&command[1], nullptr, 10);
            if (n < 0)
            {
               n = -n;
               while(n-- > 0)
               {
                  rc = history_t(h, &p, H_NEXT);
               }
            }
            else if (n > 0)
            {
               rc = history_t(h, &p, H_LAST);
               while((--n > 0) && (rc >= 0))
               {
                  rc = history_t(h, &p, H_PREV);
               }
            }
            else
            {
               rc = -1;
            }
            HistEventT tmp;
            history_t(h, &tmp, H_FIRST);
         }
         if (rc >= 0)
         {
            line = p.str;
            _tcslcpy(command, p.str, 256);
            Trim(command);
            WriteToTerminal(line);
         }
         else
         {
            command[0] = 0;
         }
#else
         command[0] = 0;
#endif
      }
      if (command[0] != 0)
      {
#if HAVE_LIBEDIT
         history_t(h, &ev, H_ENTER, line);
#endif
         if (ExecCommand(command, nullptr, nullptr))
            break;
      }
      else
      {
#if !HAVE_LIBEDIT
         WriteToTerminal(_T("\n"));
#endif
      }
   }

#if HAVE_LIBEDIT
   el_end(el);
   history_t(h, &ev, H_SAVE, ".nxadm_history");
   history_tend(h);
#endif
}

/**
 * Work mode
 */
enum WorkMode
{
   UNDEFINED,
   SHELL,
   COMMAND,
   SCRIPT,
   DB_PASSWORD,
   RUN_CHECK
};

/**
 * Entry point
 */
int main(int argc, char *argv[])
{
   int exitCode = 99;
   bool start = true;
   bool useScriptReturnValue = false;
   WorkMode workMode = UNDEFINED;
   TCHAR *command = nullptr, *password = nullptr, *loginName = nullptr;
   TCHAR passwordBuffer[MAX_PASSWORD];

   InitNetXMSProcess(true);

#ifdef _WIN32
   WSADATA wsaData;
   WSAStartup(0x0002, &wsaData);
#endif

   if (argc > 1)
   {
      // Parse command line
      opterr = 1;
      int ch;
      while((ch = getopt(argc, argv, "c:ihp:PrRs:u:v")) != -1)
      {
         switch(ch)
         {
            case 'c':
#ifdef UNICODE
               command = WideStringFromMBStringSysLocale(optarg);
#else
               command = optarg;
#endif
               workMode = COMMAND;
               break;
            case 'h':
               Help();
               start = false;
               exitCode = 0;
               break;
            case 'i':
               command = nullptr;
               workMode = SHELL;
               break;
            case 'p':
#ifdef UNICODE
               password = WideStringFromMBStringSysLocale(optarg);
#else
               password = optarg;
#endif
               break;
            case 'P':
               password = passwordBuffer;
               break;
            case 'r':
               useScriptReturnValue = true;
               break;
            case 'R':
               workMode = RUN_CHECK;
               break;
            case 's':
#ifdef UNICODE
               command = WideStringFromMBStringSysLocale(optarg);
#else
               command = optarg;
#endif
               workMode = SCRIPT;
               break;
            case 'u':
#ifdef UNICODE
               loginName = WideStringFromMBStringSysLocale(optarg);
#else
               loginName = optarg;
#endif
               break;
            case 'v':
               printf("NetXMS Server Local Administration Tool Version " NETXMS_VERSION_STRING_A "\n");
               start = false;
               exitCode = 0;
               break;
            case '?':
               start = false;
               exitCode = 1;
               break;
            default:
               break;
         }
      }

      if ((workMode == UNDEFINED) && (password != nullptr))
         workMode = DB_PASSWORD;

      if (start && (workMode != UNDEFINED))
      {
         if (Connect())
         {
            if (password == passwordBuffer)
            {
               if (!ReadPassword((workMode == DB_PASSWORD) ? _T("Database password: ") : _T("Password: "), passwordBuffer, MAX_PASSWORD))
               {
                  printf("Cannot read password from terminal\n");
                  exitCode = 4;
                  goto stop;
               }
            }

            switch(workMode)
            {
               case COMMAND:
                  ExecCommand(command, loginName, password);
                  exitCode = 0;
                  break;
               case SCRIPT:
                  exitCode = ExecScript(command, loginName, password, useScriptReturnValue);
                  break;
               case SHELL:
                  Shell(loginName, password);
                  exitCode = 0;
                  break;
               case DB_PASSWORD:
                  exitCode = SendPassword(password) ? 0 : 3;
                  break;
               case RUN_CHECK:
                  exitCode = ServerRunCheck();
                  break;
            }

            Disconnect();
         }
         else
         {
            exitCode = 2;  // Cannot connect to server
         }
      }
      else if (start && (workMode == UNDEFINED))
      {
         Help();
         exitCode = 1;
      }
   }
   else
   {
      Help();
      exitCode = 1;
   }

stop:
#ifdef UNICODE
   MemFree(command);
   MemFree(loginName);
   if (password != passwordBuffer)
      MemFree(password);
#endif
   return exitCode;
}
