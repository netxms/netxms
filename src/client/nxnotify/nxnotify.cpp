/* 
** NetXMS - Network Management System
** nxnotify - send notification via NetXMS server
** Copyright (C) 2003-2024 Raden Solutions
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
** File: nxnotify.cpp
**
**/

#include "nxnotify.h"
#include <netxms_getopt.h>
#include <netxms-version.h>

NETXMS_EXECUTABLE_HEADER(nxnotify)

/**
 * Callback function for debug printing
 */
static void DebugCallback(const TCHAR *pMsg)
{
   _tprintf(_T("NXCL: %s\n"), pMsg);
}

#ifdef _WIN32
#define CMDLINE_OPTIONS "DehP:u:vw:"
#else
#define CMDLINE_OPTIONS "c:DehP:u:vw:"
#endif

/**
 * main
 */
int main(int argc, char *argv[])
{
   TCHAR login[MAX_DB_STRING] = _T("guest"), password[MAX_DB_STRING] = _T("");
   bool isDebug = false;
   DWORD rcc, timeout = 3;
   int ch;

   InitNetXMSProcess(true, true);

   // Parse command line
   opterr = 1;
   while((ch = getopt(argc, argv, CMDLINE_OPTIONS)) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
            printf("Usage: nxnotify [<options>] <server> <channel> <recipient> [<subject>] <message> \n"
                   "Valid options are:\n"
#ifndef _WIN32
                   "   -c            : Codepage (default is " ICONV_DEFAULT_CODEPAGE ")\n"
#endif
                   "   -D            : Turn on debug mode.\n"
                   "   -e            : Encrypt session (for compatibility only, session is always encrypted).\n"
                   "   -h            : Display help and exit.\n"
                   "   -P <password> : Specify user's password. Default is empty password.\n"
                   "   -u <user>     : Login to server as <user>. Default is \"guest\".\n"
                   "   -v            : Display version and exit.\n"
                   "   -w <seconds>  : Specify command timeout (default is 3 seconds).\n"
                   "\n");
            return 0;
#ifndef _WIN32
         case 'c':
            SetDefaultCodepage(optarg);
            break;
#endif
         case 'D':
            isDebug = true;
            break;
         case 'e':
            // do nothing, server is always encrypted
            break;
         case 'u':
#ifdef UNICODE
            MultiByteToWideCharSysLocale(optarg, login, MAX_DB_STRING);
            login[MAX_DB_STRING - 1] = 0;
#else
            strlcpy(login, optarg, MAX_DB_STRING);
#endif
            break;
         case 'P':
#ifdef UNICODE
            MultiByteToWideCharSysLocale(optarg, password, MAX_DB_STRING);
            password[MAX_DB_STRING - 1] = 0;
#else
            strlcpy(password, optarg, MAX_DB_STRING);
#endif
            break;
         case 'w':
            timeout = strtoul(optarg, nullptr, 0);
            if ((timeout < 1) || (timeout > 120))
            {
               _tprintf(_T("Invalid timeout %hs\n"), optarg);
               return 1;
            }
            break;
         case 'v':
            _tprintf(_T("NetXMS notification Sender  Version ") NETXMS_VERSION_STRING _T("\n"));
            return 0;
         case '?':
            return 1;
         default:
            break;
      }
   }

   if (argc - optind < 3)
   {
      _tprintf(_T("Required arguments missing. Use nxnotify -h for help.\n"));
      return 1;
   }

#ifdef _WIN32
   WSADATA wsaData;

   if (WSAStartup(2, &wsaData) != 0)
   {
      _tprintf(_T("Unable to initialize Windows sockets\n"));
      return 4;
   }
#endif

   if (!NXCInitialize())
   {
      _tprintf(_T("Failed to initialize NetXMS client library\n"));
      return 2;
   }

   if (isDebug)
      NXCSetDebugCallback(DebugCallback);

#ifdef UNICODE
   WCHAR whost[256];
   MultiByteToWideCharSysLocale(argv[optind], whost, 256);
   whost[255] = 0;
#define _HOST whost
#else
#define _HOST argv[optind]
#endif

   NXCSession *session = new NXCSession();
   rcc = session->connect(_HOST, login, password, 0, _T("nxnotify/") NETXMS_VERSION_STRING);
   if (rcc != RCC_SUCCESS)
   {
      _tprintf(_T("Unable to connect to server: %s\n"), NXCGetErrorText(rcc));
      delete session;
      NXCShutdown();
      return 2;
   }

   session->setCommandTimeout(timeout * 1000);

   TCHAR *subject = nullptr;
   TCHAR *text = nullptr;
#ifdef UNICODE
   WCHAR *channel = WideStringFromMBStringSysLocale(argv[optind + 1]);
   WCHAR *rcpt = WideStringFromMBStringSysLocale(argv[optind + 2]);
   if ((argc - optind) > 4)
   {
      subject = WideStringFromMBStringSysLocale(argv[optind + 3]);
      text = WideStringFromMBStringSysLocale(argv[optind + 4]);
   }
   else
   {
      text = WideStringFromMBStringSysLocale(argv[optind + 3]);
   }
#else
   char *channel = argv[optind + 1];
   char *rcpt = argv[optind + 2];
   if ((argc - optind) > 3)
   {
      subject = argv[optind + 3];
      text = argv[optind + 4];
   }
   else
   {
      text = argv[optind + 3];
   }
#endif

   rcc = ((ServerController *)session->getController(CONTROLLER_SERVER))->sendNotification(channel, rcpt, subject, text);
   if (rcc != RCC_SUCCESS)
   {
      _tprintf(_T("Unable to send notification: %s\n"), NXCGetErrorText(rcc));
   }

#ifdef UNICODE
   MemFree(channel);
   MemFree(rcpt);
   MemFree(subject);
   MemFree(text);
#endif

   delete session;
   NXCShutdown();
   return (rcc == RCC_SUCCESS) ? 0 : 5;
}
