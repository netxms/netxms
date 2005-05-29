/* 
** NetXMS - Network Management System
** Local administration tool
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: nxadm.cpp
**
**/

#include "nxadm.h"

#if !defined(_WIN32) && HAVE_READLINE_READLINE_H
#include <readline/readline.h>
#include <readline/history.h>
#define USE_READLINE 1
#endif


//
// Display help
//

static void Help(void)
{
   printf("NetXMS Administartor Tool  Version " NETXMS_VERSION_STRING "\n"
          "Copyright (c) 2004, 2005 NetXMS Development Team\n\n"
          "Usage: nxadm -c <command>\n"
          "       nxadm -i\n"
          "       nxadm -h\n\n"
          "Options:\n"
          "   -c  Execute given command and disconnect\n"
          "   -i  Go to interactive mode\n"
          "   -h  Dispaly help and exit\n\n");
}


//
// Execute command
//

static BOOL ExecCommand(TCHAR *pszCmd)
{
   CSCPMessage msg, *pResponce;
   BOOL bConnClosed = FALSE;
   WORD wCode;
   TCHAR *pszText;

   msg.SetCode(CMD_ADM_REQUEST);
   msg.SetId(g_dwRqId++);
   msg.SetVariable(VID_COMMAND, pszCmd);
   SendMsg(&msg);

   while(1)
   {
      pResponce = RecvMsg();
      if (pResponce == NULL)
      {
         printf("Connection closed\n");
         bConnClosed = TRUE;
         break;
      }

      wCode = pResponce->GetCode();
      switch(wCode)
      {
         case CMD_ADM_MESSAGE:
            pszText = pResponce->GetVariableStr(VID_MESSAGE);
            if (pszText != NULL)
            {
               fputs(pszText, stdout);
               free(pszText);
            }
            break;
         default:
            break;
      }
      delete pResponce;
      if (wCode == CMD_REQUEST_COMPLETED)
         break;
   }

   return bConnClosed;
}


//
// Interactive mode loop
//

static void Shell(void)
{
   char *ptr, szCommand[256];

   printf("\nNetXMS Server Remote Console V" NETXMS_VERSION_STRING " Ready\n"
          "Enter \"help\" for command list\n\n");

#if USE_READLINE
   // Initialize readline library if we use it
# if (RL_READLINE_VERSION && RL_VERSION_MAJOR >= 5) || __FreeBSD__ >= 5
   rl_bind_key('\t', (rl_command_func_t *)rl_insert);
# else 
   rl_bind_key('\t', (Function *)rl_insert);
# endif
#endif
      
   while(1)
   {
#if USE_READLINE
      ptr = readline("netxmsd: ");
#else
      printf("netxmsd: ");
      fflush(stdout);
      if (fgets(szCommand, 255, stdin) == NULL)
         break;   // Error reading stdin
      ptr = strchr(szCommand, '\n');
      if (ptr != NULL)
         *ptr = 0;
      ptr = szCommand;
#endif

      if (ptr != NULL)
      {
         StrStrip(ptr);
         if (*ptr != 0)
         {
            if (ExecCommand(ptr))
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
         printf("\n");
      }
   }

#if USE_READLINE
   free(ptr);
#endif
}


//
// Entry point
//

int main(int argc, char *argv[])
{
   int iError, ch;
   BOOL bStart = TRUE;
   TCHAR *pszCmd;

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
               break;
            case 'c':
               pszCmd = optarg;
               break;
            case 'i':
               pszCmd = NULL;
               break;
            case '?':
               bStart = FALSE;
               break;
            default:
               break;
         }
      }

      if (bStart)
      {
         if (Connect())
         {
            if (pszCmd == NULL)
            {
               Shell();
            }
            else
            {
               ExecCommand(pszCmd);
            }
            Disconnect();
            iError = 0;
         }
         else
         {
            iError = 2;
         }
      }
   }
   else
   {
      Help();
      iError = 1;
   }
   return iError;
}
