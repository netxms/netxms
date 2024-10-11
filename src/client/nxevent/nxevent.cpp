/* 
** NetXMS - Network Management System
** Command line event sender
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
** File: nxevent.cpp
**
**/

#include "nxevent.h"
#include <netxms_getopt.h>
#include <netxms-version.h>

NETXMS_EXECUTABLE_HEADER(nxevent)

/**
 * Static data
 */
static bool s_debug = false;
static TCHAR s_server[MAX_DB_STRING] = _T("127.0.0.1");
static TCHAR s_login[MAX_DB_STRING] = _T("guest");
static TCHAR s_password[MAX_DB_STRING] = _T("");
static TCHAR s_userTag[MAX_USERTAG_LENGTH] = _T("");
static uint32_t s_eventCode = 0;
static TCHAR s_eventName[MAX_EVENT_NAME] = _T("");
static uint32_t s_objectId = 0;
static uint32_t s_timeout = 3;
static bool s_ignoreProtocolVersion = false;
static bool s_namedParameters = false;

/**
 * Callback function for debug printing
 */
static void DebugCallback(const TCHAR *pMsg)
{
   _tprintf(_T("*debug* %s\n"), pMsg);
}

/**
 * Send even in a loop
 */
static uint32_t SendEventInLoop(EventController *ctrl, uint32_t code, const TCHAR *name, uint32_t objectId, int argc, TCHAR **argv,
   const TCHAR *userTag, uint32_t repeatInterval, uint32_t repeatCount)
{
   uint32_t sendCount = 0;
   int64_t lastReportTime = GetCurrentTimeMs();
   int64_t startTime = lastReportTime;
   for(uint32_t i = 0; i < repeatCount; i++)
   {
      uint32_t rcc = ctrl->sendEvent(code, name, s_objectId, argc, argv, userTag, s_namedParameters);
      if (rcc != RCC_SUCCESS)
         return rcc;
      sendCount++;

      int64_t now = GetCurrentTimeMs();
      if (now - lastReportTime >= 2000)
      {
         uint32_t eventsPerSecond = sendCount * 1000 / static_cast<uint32_t>(now - lastReportTime);
         _tprintf(_T("%u events/sec\n"), eventsPerSecond);
         sendCount = 0;
         lastReportTime = GetCurrentTimeMs();
      }

      if (repeatInterval > 0)
         ThreadSleepMs(repeatInterval);
   }
   _tprintf(_T("Batch average: %u events/sec\n"), static_cast<uint32_t>(static_cast<int64_t>(repeatCount) * _LL(1000) / (GetCurrentTimeMs() - startTime)));
   return RCC_SUCCESS;
}

/**
 * Send event to server
 */
static uint32_t SendEvent(int iNumArgs, char **pArgList, bool repeat, uint32_t repeatInterval, uint32_t repeatCount)
{
   uint32_t rcc = RCC_SYSTEM_FAILURE;

   if (!NXCInitialize())
   {
      _tprintf(_T("Failed to initialize NetXMS client library\n"));
   }
   else
   {
      if (s_debug)
         NXCSetDebugCallback(DebugCallback);

      NXCSession *session = new NXCSession();
      static uint32_t protocolVersions[] = { CPV_INDEX_TRAP };
      rcc = session->connect(s_server, s_login, s_password,
            (s_ignoreProtocolVersion ? NXCF_IGNORE_PROTOCOL_VERSION : 0),
            _T("nxevent/") NETXMS_VERSION_STRING, s_ignoreProtocolVersion ? nullptr : protocolVersions,
            s_ignoreProtocolVersion ? 0 : sizeof(protocolVersions) / sizeof(uint32_t));
      if (rcc != RCC_SUCCESS)
      {
         _tprintf(_T("Unable to connect to server: %s\n"), NXCGetErrorText(rcc));
      }
      else
      {
         session->setCommandTimeout(s_timeout * 1000);
         EventController *ctrl = (EventController *)session->getController(CONTROLLER_EVENTS);
#ifdef UNICODE
			WCHAR **argList = MemAllocArray<WCHAR*>(iNumArgs);
			for(int i = 0; i < iNumArgs; i++)
				argList[i] = WideStringFromMBStringSysLocale(pArgList[i]);
			if (repeat)
			   rcc = SendEventInLoop(ctrl, s_eventCode, s_eventName, s_objectId, iNumArgs, argList, s_userTag, repeatInterval, repeatCount);
			else
			   rcc = ctrl->sendEvent(s_eventCode, s_eventName, s_objectId, iNumArgs, argList, s_userTag, s_namedParameters);
			for(int i = 0; i < iNumArgs; i++)
				MemFree(argList[i]);
			MemFree(argList);
#else
			if (repeat)
            rcc = SendEventInLoop(ctrl, s_eventCode, s_eventName, s_objectId, iNumArgs, pArgList, s_userTag, repeatInterval, repeatCount);
         else
			   rcc = ctrl->sendEvent(s_eventCode, s_eventName, s_objectId, iNumArgs, pArgList, s_userTag, s_namedParameters);
#endif
         if (rcc != RCC_SUCCESS)
            _tprintf(_T("Unable to send event: %s\n"), NXCGetErrorText(rcc));
      }
      delete session;
      NXCShutdown();
   }
   return rcc;
}

#ifdef _WIN32
#define CMDLINE_OPTIONS "C:dehi:no:P:ST:u:vw:"
#else
#define CMDLINE_OPTIONS "c:C:dehi:no:P:ST:u:vw:"
#endif

/**
 * Entry point
 */
int main(int argc, char *argv[])
{
   int ch;
   uint32_t rcc = RCC_INVALID_ARGUMENT;
   bool start = true, repeat = false;
   uint32_t repeatInterval = 100, repeatCount = 10000;

   InitNetXMSProcess(true, true);

   // Parse command line
   opterr = 1;
   while((ch = getopt(argc, argv, CMDLINE_OPTIONS)) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
            printf("Usage: nxevent [<options>] <server> <event> [<param_1> [... <param_N>]]\n"
                   "Valid options are:\n"
#ifndef _WIN32
                   "   -c            : Codepage (default is " ICONV_DEFAULT_CODEPAGE ")\n"
#endif
                   "   -C <count>    : Repeat event sending given number of times.\n"
                   "   -d            : Turn on debug mode.\n"
                   "   -e            : Encrypt session (for compatibility only, session is always encrypted).\n"
                   "   -h            : Display help and exit.\n"
                   "   -i <interval> : Repeat event sending with given interval in milliseconds.\n"
                   "   -n            : Parameters are provided in named format (name=value).\n"
                   "   -o <id>       : Specify source object ID.\n"
                   "   -P <password> : Specify user's password. Default is empty password.\n"
                   "   -S            : Skip protocol version check (use with care).\n"
                   "   -T <tag>      : User tag to be associated with the message. Default is empty.\n"
                   "   -u <user>     : Login to server as <user>. Default is \"guest\".\n"
                   "   -v            : Display version and exit.\n"
                   "   -w <seconds>  : Specify command timeout (default is 3 seconds).\n"
                   "Event could be specified either by code or by name\n"
                   "\n");
            start = false;
            break;
#ifndef _WIN32
         case 'c':
            SetDefaultCodepage(optarg);
            break;
#endif
         case 'C':
            repeat = true;
            repeatCount = strtoul(optarg, nullptr, 0);
            break;
         case 'd':
            s_debug = true;
            break;
         case 'e':
            // do nothing, server is always encrypted
            break;
         case 'i':
            repeat = true;
            repeatInterval = strtoul(optarg, nullptr, 0);
            break;
         case 'n':
            s_namedParameters = true;
            break;
         case 'o':
            s_objectId = strtoul(optarg, nullptr, 0);
            break;
         case 'u':
#ifdef UNICODE
				MultiByteToWideCharSysLocale(optarg, s_login, MAX_DB_STRING);
				s_login[MAX_DB_STRING - 1] = 0;
#else
            strlcpy(s_login, optarg, MAX_DB_STRING);
#endif
            break;
         case 'P':
#ifdef UNICODE
            MultiByteToWideCharSysLocale(optarg, s_password, MAX_DB_STRING);
				s_password[MAX_DB_STRING - 1] = 0;
#else
            strlcpy(s_password, optarg, MAX_DB_STRING);
#endif
            break;
         case 'S':
            s_ignoreProtocolVersion = true;
            break;
         case 'T':
#ifdef UNICODE
            MultiByteToWideCharSysLocale(optarg, s_userTag, MAX_USERTAG_LENGTH);
				s_userTag[MAX_USERTAG_LENGTH - 1] = 0;
#else
            strlcpy(s_userTag, optarg, MAX_USERTAG_LENGTH);
#endif
            break;
         case 'w':
            s_timeout = strtoul(optarg, nullptr, 0);
            if ((s_timeout < 1) || (s_timeout > 120))
            {
               _tprintf(_T("Invalid timeout %hs\n"), optarg);
               start = false;
            }
            break;
         case 'v':
            _tprintf(_T("NetXMS Event Sender  Version ") NETXMS_VERSION_STRING _T("\n"));
            start = false;
            break;
         case '?':
            start = false;
            break;
         default:
            break;
      }
   }

   // Do requested action if everything is OK
   if (start)
   {
      if (argc - optind < 2)
      {
         _tprintf(_T("Required arguments missing. Use nxevent -h for help.\n"));
      }
      else
      {
#ifdef _WIN32
         WSADATA wsaData;

         if (WSAStartup(2, &wsaData) != 0)
         {
            _tprintf(_T("Unable to initialize Windows sockets\n"));
            return RCC_COMM_FAILURE;
         }
#endif
#ifdef UNICODE
         MultiByteToWideCharSysLocale(argv[optind], s_server, 256);
			s_server[255] = 0;
#else
         strlcpy(s_server, argv[optind], 256);
#endif

         char *eptr;
         s_eventCode = strtoul(argv[optind + 1], &eptr, 0);
         if (*eptr != 0)
         {
            // assume that event is specified by name
            s_eventCode = 0;
#ifdef UNICODE
            MultiByteToWideCharSysLocale(argv[optind + 1], s_eventName, MAX_EVENT_NAME);
            s_eventName[MAX_EVENT_NAME - 1] = 0;
#else
            strlcpy(s_eventName, argv[optind + 1], MAX_EVENT_NAME);
#endif
         }

         rcc = SendEvent(argc - optind - 2, &argv[optind + 2], repeat, repeatInterval, repeatCount);
      }
   }

   return (int)rcc;
}
