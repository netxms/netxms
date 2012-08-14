/* 
** NetXMS - Network Management System
** nxsms - send SMS via NetXMS server
** Copyright (C) 2003-2012 Victor Kirhenshtein
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
** File: nxsms.cpp
**
**/

#include "nxsms.h"


//
// Callback function for debug printing
//

static void DebugCallback(TCHAR *pMsg)
{
   _tprintf(_T("*debug* %s\n"), pMsg);
}


//
// main
//

int main(int argc, char *argv[])
{
	TCHAR login[MAX_DB_STRING] = _T("guest"), password[MAX_DB_STRING] = _T("");
	BOOL isDebug = FALSE, isEncrypt = FALSE;
	DWORD rcc, timeout = 3;
   NXC_SESSION session;
	int ch;

   // Parse command line
   opterr = 1;
   while((ch = getopt(argc, argv, "DehP:u:vw:")) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
            printf("Usage: nxsms [<options>] <server> <phone number> <message>\n"
                   "Valid options are:\n"
                   "   -D            : Turn on debug mode.\n"
                   "   -e            : Encrypt session.\n"
                   "   -h            : Display help and exit.\n"
                   "   -P <password> : Specify user's password. Default is empty password.\n"
                   "   -u <user>     : Login to server as <user>. Default is \"guest\".\n"
                   "   -v            : Display version and exit.\n"
                   "   -w <seconds>  : Specify command timeout (default is 3 seconds).\n"
                   "\n");
            return 1;
         case 'D':
            isDebug = TRUE;
            break;
         case 'e':
            isEncrypt = TRUE;
            break;
         case 'u':
#ifdef UNICODE
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, login, MAX_DB_STRING);
				login[MAX_DB_STRING] = 0;
#else
            nx_strncpy(login, optarg, MAX_DB_STRING);
#endif
            break;
         case 'P':
#ifdef UNICODE
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, password, MAX_DB_STRING);
				password[MAX_DB_STRING] = 0;
#else
            nx_strncpy(password, optarg, MAX_DB_STRING);
#endif
            break;
         case 'w':
            timeout = strtoul(optarg, NULL, 0);
            if ((timeout < 1) || (timeout > 120))
            {
               _tprintf(_T("Invalid timeout %hs\n"), optarg);
               return 1;
            }
            break;
         case 'v':
            _tprintf(_T("NetXMS SMS Sender  Version ") NETXMS_VERSION_STRING _T("\n"));
            return 1;
         case '?':
            return 1;
         default:
            break;
		}
	}

   if (argc - optind < 3)
   {
      _tprintf(_T("Required arguments missing. Use nxsms -h for help.\n"));
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
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, argv[optind], -1, whost, 256);
	whost[255] = 0;
#define _HOST whost
#else
#define _HOST argv[optind]
#endif

	rcc = NXCConnect(isEncrypt ? NXCF_ENCRYPT : 0, _HOST, login,
		              password, 0, NULL, NULL, &session,
                    _T("nxsms/") NETXMS_VERSION_STRING, NULL);
	if (rcc != RCC_SUCCESS)
	{
		_tprintf(_T("Unable to connect to server: %s\n"), NXCGetErrorText(rcc));
		return 2;
	}

	NXCSetCommandTimeout(session, timeout * 1000);

#ifdef UNICODE
	WCHAR *rcpt = WideStringFromMBString(argv[optind + 1]);
	WCHAR *text = WideStringFromMBString(argv[optind + 2]);
	rcc = NXCSendSMS(session, rcpt, text);
	free(rcpt);
	free(text);
#else
	rcc = NXCSendSMS(session, argv[optind + 1], argv[optind + 2]);
#endif
	if (rcc != RCC_SUCCESS)
	{
		_tprintf(_T("Unable to send SMS: %s\n"), NXCGetErrorText(rcc));
	}

   NXCDisconnect(session);
	return (rcc == RCC_SUCCESS) ? 0 : 5;
}
