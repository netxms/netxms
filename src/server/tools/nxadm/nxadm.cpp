/* 
** NetXMS - Network Management System
** Local administration tool
** Copyright (C) 2003-2022 Victor Kirhenshtein
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
#include <netxms-version.h>

#if !defined(_WIN32) && HAVE_READLINE_READLINE_H && HAVE_READLINE && !defined(UNICODE)
#include <readline/readline.h>
#include <readline/history.h>
#define USE_READLINE 1
#endif

NETXMS_EXECUTABLE_HEADER(nxadm)

/**
 * Display help
 */
static void Help()
{
   _tprintf(_T("NetXMS Server Administration Tool  Version ") NETXMS_VERSION_STRING _T("\n")
            _T("Copyright (c) 2004-2022 Raden Solutions\n\n")
            _T("Usage: nxadm [-u <login>] [-P|-p <password>] -c <command>\n")
            _T("       nxadm [-u <login>] [-P|-p <password>] -i\n")
            _T("       nxadm [-u <login>] [-P|-p <password>] [-r] -s <script>\n")
            _T("       nxadm -P\n")
            _T("       nxadm -p <db password>\n")
            _T("       nxadm -h\n")
            _T("       nxadm -v\n\n")
            _T("Options:\n")
            _T("   -c <command>   Execute given command at server debug console and disconnect\n")
            _T("   -i             Connect to server debug console in interactive mode\n")
            _T("   -h             Display help and exit\n")
            _T("   -p <password>  Provide database password for server startup or\n")
            _T("                  user's password for console access\n")
            _T("   -P             Provide database password for server startup or\n")
            _T("                  user's password for console access (password read from terminal)\n")
            _T("   -r             Use script's return value as exit code\n")
            _T("   -s <script>    Execute given NXSL script and disconnect\n")
            _T("   -u name        User name for authentication\n")
            _T("   -v             Display version and exit\n\n"));
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
         _tprintf(_T("ERROR: server error %u\n"), rcc);
      delete response;
      success = (rcc == 0);
   }
   else
   {
      _tprintf(_T("ERROR: no response from server\n"));
   }

   return success;
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
      _tprintf(_T("Authentication failed (no response from server)\n"));
      return false;
   }

   uint32_t rcc = response->getFieldAsUInt32(VID_RCC);
   delete response;
   if (rcc != RCC_SUCCESS)
   {
      _tprintf(_T("Authentication failed (server error %u)\n"), rcc);
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
         _tprintf(_T("Connection closed\n"));
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
            _tprintf(_T("Server error %u\n"), rcc);
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
         _tprintf(_T("Connection closed\n"));
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
            _tprintf(_T("Server error %d\n"), rcc);
         }
         delete response;
         break;
      }
      delete response;
   }

   return rcc;
}

/**
 * Interactive mode loop
 */
static void Shell(const TCHAR *login, const TCHAR *password)
{
   if (login != nullptr)
   {
      if (!Login(login, password))
         return;
   }

   char *ptr;

   _tprintf(_T("\nNetXMS Server Remote Console V") NETXMS_VERSION_STRING _T(" Ready\n")
            _T("Enter \"help\" for command list\n\n"));

#if USE_READLINE
   // Initialize readline library if we use it
   rl_bind_key('\t', RL_INSERT_CAST rl_insert);
#endif
      
   while(1)
   {
#if USE_READLINE
      ptr = readline("\x1b[33mnetxmsd:\x1b[0m ");
#else
      WriteToTerminal(_T("\x1b[33mnetxmsd:\x1b[0m "));
      fflush(stdout);
      char szCommand[256];
      if (fgets(szCommand, 255, stdin) == nullptr)
         break;   // Error reading stdin
      ptr = strchr(szCommand, '\n');
      if (ptr != nullptr)
         *ptr = 0;
      ptr = szCommand;
#endif

      if (ptr != nullptr)
      {
#ifdef UNICODE
         WCHAR wcCommand[256];
         MultiByteToWideCharSysLocale(ptr, wcCommand, 255);
         wcCommand[255] = 0;
         TrimW(wcCommand);
         if (wcCommand[0] != 0)
         {
            if (ExecCommand(wcCommand, nullptr, nullptr))
#else
         TrimA(ptr);
         if (*ptr != 0)
         {
            if (ExecCommand(ptr, nullptr, nullptr))
#endif
               break;
#if USE_READLINE
            add_history(ptr);
#endif
         }
#if USE_READLINE
         free(ptr);
#endif
      }
      else
      {
         _tprintf(_T("\n"));
      }
   }

#if USE_READLINE
   free(ptr);
#endif
}

enum WorkMode
{
   UNDEFINED,
   SHELL,
   COMMAND,
   SCRIPT,
   DB_PASSWORD
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
      while((ch = getopt(argc, argv, "c:ihp:Prs:u:v")) != -1)
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
               command = NULL;
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
               _tprintf(_T("NetXMS Server Local Administration Tool Version ") NETXMS_VERSION_STRING _T("\n"));
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
                  _tprintf(_T("Cannot read password from terminal\n"));
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
            }

            Disconnect();
         }
         else
         {
            exitCode = 2;
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
