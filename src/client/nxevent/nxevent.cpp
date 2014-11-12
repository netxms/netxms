/* 
** NetXMS - Network Management System
** Command line event sender
** Copyright (C) 2003, 2004, 2005, 206, 2007 Victor Kirhenshtein
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

/**
 * Callback function for debug printing
 */
static void DebugCallback(TCHAR *pMsg)
{
   _tprintf(_T("*debug* %s\n"), pMsg);
}

/**
 * Send event to server
 */
static DWORD SendEvent(int iNumArgs, char **pArgList, BOOL bEncrypt)
{
   DWORD dwResult;
   NXC_SESSION hSession;

   if (!NXCInitialize())
   {
      _tprintf(_T("Failed to initialize NetXMS client library\n"));
   }
   else
   {
      if (m_bDebug)
         NXCSetDebugCallback(DebugCallback);

      dwResult = NXCConnect(bEncrypt ? NXCF_ENCRYPT : 0, m_szServer, m_szLogin,
		                      m_szPassword, 0, NULL, NULL, &hSession,
                            _T("nxevent/") NETXMS_VERSION_STRING, NULL);
      if (dwResult != RCC_SUCCESS)
      {
         _tprintf(_T("Unable to connect to server: %s\n"), NXCGetErrorText(dwResult));
      }
      else
      {
         NXCSetCommandTimeout(hSession, m_dwTimeOut * 1000);
#ifdef UNICODE
			WCHAR **argList = (WCHAR **)malloc(sizeof(WCHAR *) * iNumArgs);
			for(int i = 0; i < iNumArgs; i++)
				argList[i] = WideStringFromMBString(pArgList[i]);
         dwResult = NXCSendEvent(hSession, m_dwEventCode, m_eventName, m_dwObjectId, iNumArgs, argList, m_szUserTag);
			for(int i = 0; i < iNumArgs; i++)
				free(argList[i]);
			free(argList);
#else
         dwResult = NXCSendEvent(hSession, m_dwEventCode, m_eventName, m_dwObjectId, iNumArgs, pArgList, m_szUserTag);
#endif
         if (dwResult != RCC_SUCCESS)
            _tprintf(_T("Unable to send event: %s\n"), NXCGetErrorText(dwResult));
         NXCDisconnect(hSession);
      }
   }
   return dwResult;
}

#ifdef _WIN32
#define CMDLINE_OPTIONS "deho:P:T:u:vw:"
#else
#define CMDLINE_OPTIONS "c:deho:P:T:u:vw:"
#endif

/**
 * Entry point
 */
int main(int argc, char *argv[])
{
   int ch, rcc = RCC_INVALID_ARGUMENT;
   BOOL bStart = TRUE, bEncrypt = FALSE;

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
                   "   -d            : Turn on debug mode.\n"
                   "   -e            : Encrypt session.\n"
                   "   -h            : Display help and exit.\n"
                   "   -o <id>       : Specify source object ID.\n"
                   "   -P <password> : Specify user's password. Default is empty password.\n"
                   "   -T <tag>      : User tag to be associated with the message. Default is empty.\n"
                   "   -u <user>     : Login to server as <user>. Default is \"guest\".\n"
                   "   -v            : Display version and exit.\n"
                   "   -w <seconds>  : Specify command timeout (default is 3 seconds).\n"
                   "Event could be specified either by code or by name\n"
                   "\n");
            bStart = FALSE;
            break;
#ifndef _WIN32
         case 'c':
            SetDefaultCodePage(optarg);
            break;
#endif
         case 'd':
            m_bDebug = TRUE;
            break;
         case 'e':
            bEncrypt = TRUE;
            break;
         case 'o':
            m_dwObjectId = strtoul(optarg, NULL, 0);
            break;
         case 'u':
#ifdef UNICODE
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, m_szLogin, MAX_DB_STRING);
				m_szLogin[MAX_DB_STRING - 1] = 0;
#else
            nx_strncpy(m_szLogin, optarg, MAX_DB_STRING);
#endif
            break;
         case 'P':
#ifdef UNICODE
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, m_szPassword, MAX_DB_STRING);
				m_szPassword[MAX_DB_STRING - 1] = 0;
#else
            nx_strncpy(m_szPassword, optarg, MAX_DB_STRING);
#endif
            break;
         case 'T':
#ifdef UNICODE
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, m_szUserTag, MAX_USERTAG_LENGTH);
				m_szUserTag[MAX_USERTAG_LENGTH - 1] = 0;
#else
            nx_strncpy(m_szUserTag, optarg, MAX_USERTAG_LENGTH);
#endif
            break;
         case 'w':
            m_dwTimeOut = strtoul(optarg, NULL, 0);
            if ((m_dwTimeOut < 1) || (m_dwTimeOut > 120))
            {
               _tprintf(_T("Invalid timeout %hs\n"), optarg);
               bStart = FALSE;
            }
            break;
         case 'v':
            _tprintf(_T("NetXMS Event Sender  Version ") NETXMS_VERSION_STRING _T("\n"));
            bStart = FALSE;
            break;
         case '?':
            bStart = FALSE;
            break;
         default:
            break;
      }
   }

   // Do requested action if everything is OK
   if (bStart)
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
         nx_strncpy(m_szServer, argv[optind], 256);
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
            nx_strncpy(m_eventName, argv[optind + 1], MAX_EVENT_NAME);
#endif
         }

         rcc = (int)SendEvent(argc - optind - 2, &argv[optind + 2], bEncrypt);
      }
   }

   return rcc;
}
