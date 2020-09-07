/* 
** NetXMS - Network Management System
** Command line event sender
** Copyright (C) 2003-2019 Victor Kirhenshtein
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

NETXMS_EXECUTABLE_HEADER(nxevent)

/**
 * Static data
 */
static BOOL m_bDebug = FALSE;
static TCHAR m_szServer[MAX_DB_STRING] = _T("127.0.0.1");
static TCHAR m_szLogin[MAX_DB_STRING] = _T("guest");
static TCHAR m_szPassword[MAX_DB_STRING] = _T("");
static TCHAR m_szUserTag[MAX_USERTAG_LENGTH] = _T("");
static DWORD m_dwEventCode = 0;
static TCHAR m_eventName[MAX_EVENT_NAME] = _T("");
static DWORD m_dwObjectId = 0;
static DWORD m_dwTimeOut = 3;
static bool s_ignoreProtocolVersion = false;

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
static UINT32 SendEventInLoop(EventController *ctrl, UINT32 code, const TCHAR *name, UINT32 objectId, int argc, TCHAR **argv,
   const TCHAR *userTag, UINT32 repeatInterval, UINT32 repeatCount)
{
   UINT32 sendCount = 0;
   INT64 lastReportTime = GetCurrentTimeMs();
   INT64 startTime = lastReportTime;
   for(UINT32 i = 0; i < repeatCount; i++)
   {
      UINT32 rcc = ctrl->sendEvent(code, name, m_dwObjectId, argc, argv, userTag);
      if (rcc != RCC_SUCCESS)
         return rcc;
      sendCount++;

      INT64 now = GetCurrentTimeMs();
      if (now - lastReportTime >= 2000)
      {
         UINT32 eventsPerSecond = sendCount * 1000 / static_cast<UINT32>(now - lastReportTime);
         _tprintf(_T("%u events/sec\n"), eventsPerSecond);
         sendCount = 0;
         lastReportTime = GetCurrentTimeMs();
      }

      if (repeatInterval > 0)
         ThreadSleepMs(repeatInterval);
   }
   _tprintf(_T("Batch average: %u events/sec\n"), static_cast<UINT32>(static_cast<INT64>(repeatCount) * _LL(1000) / (GetCurrentTimeMs() - startTime)));
   return RCC_SUCCESS;
}

/**
 * Send event to server
 */
static DWORD SendEvent(int iNumArgs, char **pArgList, bool encrypt, bool repeat, UINT32 repeatInterval, UINT32 repeatCount)
{
   DWORD dwResult = RCC_SYSTEM_FAILURE;

   if (!NXCInitialize())
   {
      _tprintf(_T("Failed to initialize NetXMS client library\n"));
   }
   else
   {
      if (m_bDebug)
         NXCSetDebugCallback(DebugCallback);

      NXCSession *session = new NXCSession();
      static UINT32 protocolVersions[] = { CPV_INDEX_TRAP };
      dwResult = session->connect(m_szServer, m_szLogin, m_szPassword,
            (encrypt ? NXCF_ENCRYPT : 0) | (s_ignoreProtocolVersion ? NXCF_IGNORE_PROTOCOL_VERSION : 0),
            _T("nxevent/") NETXMS_VERSION_STRING, s_ignoreProtocolVersion ? NULL : protocolVersions,
            s_ignoreProtocolVersion ? 0 : sizeof(protocolVersions) / sizeof(UINT32));
      if (dwResult != RCC_SUCCESS)
      {
         _tprintf(_T("Unable to connect to server: %s\n"), NXCGetErrorText(dwResult));
      }
      else
      {
         session->setCommandTimeout(m_dwTimeOut * 1000);
         EventController *ctrl = (EventController *)session->getController(CONTROLLER_EVENTS);
#ifdef UNICODE
			WCHAR **argList = MemAllocArray<WCHAR*>(iNumArgs);
			for(int i = 0; i < iNumArgs; i++)
				argList[i] = WideStringFromMBStringSysLocale(pArgList[i]);
			if (repeat)
			   dwResult = SendEventInLoop(ctrl, m_dwEventCode, m_eventName, m_dwObjectId, iNumArgs, argList, m_szUserTag, repeatInterval, repeatCount);
			else
			   dwResult = ctrl->sendEvent(m_dwEventCode, m_eventName, m_dwObjectId, iNumArgs, argList, m_szUserTag);
			for(int i = 0; i < iNumArgs; i++)
				MemFree(argList[i]);
			MemFree(argList);
#else
			if (repeat)
            dwResult = SendEventInLoop(ctrl, m_dwEventCode, m_eventName, m_dwObjectId, iNumArgs, pArgList, m_szUserTag, repeatInterval, repeatCount);
         else
			   dwResult = ctrl->sendEvent(m_dwEventCode, m_eventName, m_dwObjectId, iNumArgs, pArgList, m_szUserTag);
#endif
         if (dwResult != RCC_SUCCESS)
            _tprintf(_T("Unable to send event: %s\n"), NXCGetErrorText(dwResult));
      }
      delete session;
      NXCShutdown();
   }
   return dwResult;
}

#ifdef _WIN32
#define CMDLINE_OPTIONS "C:dehi:o:P:ST:u:vw:"
#else
#define CMDLINE_OPTIONS "c:C:dehi:o:P:ST:u:vw:"
#endif

/**
 * Entry point
 */
int main(int argc, char *argv[])
{
   int ch, rcc = RCC_INVALID_ARGUMENT;
   bool start = true, encrypt = true, repeat = false;
   UINT32 repeatInterval = 100, repeatCount = 10000;

   InitNetXMSProcess(true);

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
                   "   -e            : Encrypt session (default).\n"
                   "   -h            : Display help and exit.\n"
                   "   -i <interval> : Repeat event sending with given interval in milliseconds.\n"
                   "   -n            : Do not encrypt session.\n"
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
            repeatCount = strtoul(optarg, NULL, 0);
            break;
         case 'd':
            m_bDebug = true;
            break;
         case 'e':
            encrypt = true;
            break;
         case 'i':
            repeat = true;
            repeatInterval = strtoul(optarg, NULL, 0);
            break;
         case 'n':
            encrypt = false;
            break;
         case 'o':
            m_dwObjectId = strtoul(optarg, NULL, 0);
            break;
         case 'u':
#ifdef UNICODE
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, m_szLogin, MAX_DB_STRING);
				m_szLogin[MAX_DB_STRING - 1] = 0;
#else
            strlcpy(m_szLogin, optarg, MAX_DB_STRING);
#endif
            break;
         case 'P':
#ifdef UNICODE
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, m_szPassword, MAX_DB_STRING);
				m_szPassword[MAX_DB_STRING - 1] = 0;
#else
            strlcpy(m_szPassword, optarg, MAX_DB_STRING);
#endif
            break;
         case 'S':
            s_ignoreProtocolVersion = true;
            break;
         case 'T':
#ifdef UNICODE
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, m_szUserTag, MAX_USERTAG_LENGTH);
				m_szUserTag[MAX_USERTAG_LENGTH - 1] = 0;
#else
            strlcpy(m_szUserTag, optarg, MAX_USERTAG_LENGTH);
#endif
            break;
         case 'w':
            m_dwTimeOut = strtoul(optarg, NULL, 0);
            if ((m_dwTimeOut < 1) || (m_dwTimeOut > 120))
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
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, argv[optind], -1, m_szServer, 256);
			m_szServer[255] = 0;
#else
         strlcpy(m_szServer, argv[optind], 256);
#endif

         char *eptr;
         m_dwEventCode = strtoul(argv[optind + 1], &eptr, 0);
         if (*eptr != 0)
         {
            // assume that event is specified by name
            m_dwEventCode = 0;
#ifdef UNICODE
            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, argv[optind + 1], -1, m_eventName, MAX_EVENT_NAME);
            m_eventName[MAX_EVENT_NAME - 1] = 0;
#else
            strlcpy(m_eventName, argv[optind + 1], MAX_EVENT_NAME);
#endif
         }

         rcc = (int)SendEvent(argc - optind - 2, &argv[optind + 2], encrypt, repeat, repeatInterval, repeatCount);
      }
   }

   return rcc;
}
