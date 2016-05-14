/* 
** NetXMS - Network Management System
** Local administration tool
** Copyright (C) 2003-2016 Victor Kirhenshtein
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

#if !defined(_WIN32) && HAVE_READLINE_READLINE_H && HAVE_READLINE && !defined(UNICODE)
#include <readline/readline.h>
#include <readline/history.h>
#define USE_READLINE 1
#endif

/**
 * Display help
 */
static void Help()
{
   _tprintf(_T("NetXMS Server Administration Tool  Version ") NETXMS_VERSION_STRING _T("\n")
            _T("Copyright (c) 2004-2016 Raden Solutions\n\n")
            _T("Usage: nxadm -c <command>\n")
            _T("       nxadm -i\n")
            _T("       nxadm -h\n\n")
            _T("Options:\n")
            _T("   -c  Execute given command at server debug console and disconnect\n")
            _T("   -i  Connect to server debug console in interactive mode\n")
            _T("   -h  Display help and exit\n\n"));
}

/**
 * Execute command
 */
static bool ExecCommand(const TCHAR *command)
{
   bool connClosed = false;

   NXCPMessage msg;
   msg.setCode(CMD_ADM_REQUEST);
   msg.setId(g_dwRqId++);
   msg.setField(VID_COMMAND, command);
   SendMsg(&msg);

   while(true)
   {
      NXCPMessage *response = RecvMsg();
      if (response == NULL)
      {
         _tprintf(_T("Connection closed\n"));
         connClosed = true;
         break;
      }

      UINT16 code = response->getCode();
      if(code == CMD_ADM_MESSAGE)
      {
         TCHAR *text = response->getFieldAsString(VID_MESSAGE);
         if (text != NULL)
         {
            WriteToTerminal(text);
         }
      }
      delete response;
      if (code == CMD_REQUEST_COMPLETED)
         break;
   }

   return connClosed;
}

/**
 * Interactive mode loop
 */
static void Shell()
{
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
      if (fgets(szCommand, 255, stdin) == NULL)
         break;   // Error reading stdin
      ptr = strchr(szCommand, '\n');
      if (ptr != NULL)
         *ptr = 0;
      ptr = szCommand;
#endif

      if (ptr != NULL)
      {
#ifdef UNICODE
         WCHAR wcCommand[256];
#if HAVE_MBSTOWCS
         mbstowcs(wcCommand, ptr, 255);
#else
         MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, ptr, -1, wcCommand, 256);
#endif
         wcCommand[255] = 0;
         StrStrip(wcCommand);
         if (wcCommand[0] != 0)
         {
            if (ExecCommand(wcCommand))
#else
         StrStrip(ptr);
         if (*ptr != 0)
         {
            if (ExecCommand(ptr))
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

/**
 * Entry point
 */
int main(int argc, char *argv[])
{
   int iError, ch;
   BOOL bStart = TRUE, bCmdLineOK = FALSE;
   TCHAR *command = NULL;

   InitNetXMSProcess();

#ifdef _WIN32
   WSADATA wsaData;
   WSAStartup(0x0002, &wsaData);
#endif

   if (argc > 1)
   {
      // Parse command line
      opterr = 1;
      while((ch = getopt(argc, argv, "c:ih")) != -1)
      {
         switch(ch)
         {
            case 'h':
               Help();
               bStart = FALSE;
               iError = 0;
               break;
            case 'c':
#ifdef UNICODE
               command = WideStringFromMBStringSysLocale(optarg);
#else
               command = optarg;
#endif
               bCmdLineOK = TRUE;
               break;
            case 'i':
               command = NULL;
               bCmdLineOK = TRUE;
               break;
            case '?':
               bStart = FALSE;
               iError = 1;
               break;
            default:
               break;
         }
      }

      if (bStart && bCmdLineOK)
      {
         if (Connect())
         {
            if (command == NULL)
            {
               Shell();
            }
            else
            {
               ExecCommand(command);
            }
            Disconnect();
            iError = 0;
         }
         else
         {
            iError = 2;
         }
      }
      else if (bStart && !bCmdLineOK)
      {
         Help();
         iError = 1;
      }
   }
   else
   {
      Help();
      iError = 1;
   }
#ifdef UNICODE
   free(command);
#endif
   return iError;
}
