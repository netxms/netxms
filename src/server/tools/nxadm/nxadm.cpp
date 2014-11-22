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
** File: nxadm.cpp
**
**/

#include "nxadm.h"

#if !defined(_WIN32) && HAVE_READLINE_READLINE_H && HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#define USE_READLINE 1
#endif


//
// Display help
//

static void Help()
{
   printf("NetXMS Administartor Tool  Version " NETXMS_VERSION_STRING_A "\n"
          "Copyright (c) 2004, 2005, 2006, 2007 Victor Kirhenshtein\n\n"
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

static BOOL ExecCommand(char *pszCmd)
{
   NXCPMessage msg, *pResponse;
   BOOL bConnClosed = FALSE;
   WORD wCode;
   TCHAR *pszText;

   msg.setCode(CMD_ADM_REQUEST);
   msg.setId(g_dwRqId++);
#ifdef UNICODE
   WCHAR *wcmd = WideStringFromMBString(pszCmd);
   msg.setField(VID_COMMAND, wcmd);
   free(wcmd);
#else
   msg.setField(VID_COMMAND, pszCmd);
#endif
   SendMsg(&msg);

   while(1)
   {
      pResponse = RecvMsg();
      if (pResponse == NULL)
      {
         printf("Connection closed\n");
         bConnClosed = TRUE;
         break;
      }

      wCode = pResponse->getCode();
      switch(wCode)
      {
         case CMD_ADM_MESSAGE:
            pszText = pResponse->getFieldAsString(VID_MESSAGE);
            if (pszText != NULL)
            {
#if defined(_WIN32) || !defined(UNICODE)
               WriteToTerminal(pszText);
#else
	       char *mbText = MBStringFromWideString(pszText);
	       fputs(mbText, stdout);
	       free(mbText);
#endif
               free(pszText);
            }
            break;
         default:
            break;
      }
      delete pResponse;
      if (wCode == CMD_REQUEST_COMPLETED)
         break;
   }

   return bConnClosed;
}

/**
 * Interactive mode loop
 */
static void Shell()
{
   char *ptr, szCommand[256];

   printf("\nNetXMS Server Remote Console V" NETXMS_VERSION_STRING_A " Ready\n"
          "Enter \"help\" for command list\n\n");

#if USE_READLINE
   // Initialize readline library if we use it
   rl_bind_key('\t', RL_INSERT_CAST rl_insert);
#endif
      
   while(1)
   {
#if USE_READLINE
      ptr = readline("\x1b[33mnetxmsd:\x1b[0m ");
#else
#ifdef _WIN32
      WriteToTerminal(_T("\x1b[33mnetxmsd:\x1b[0m "));
#else
      fputs("\x1b[33mnetxmsd:\x1b[0m ", stdout);
#endif
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
         StrStripA(ptr);
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
   BOOL bStart = TRUE, bCmdLineOK = FALSE;
   char *pszCmd;

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
               bCmdLineOK = TRUE;
               break;
            case 'i':
               pszCmd = NULL;
               bCmdLineOK = TRUE;
               break;
            case '?':
               bStart = FALSE;
               break;
            default:
               break;
         }
      }

      if (bStart && bCmdLineOK)
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
   return iError;
}
