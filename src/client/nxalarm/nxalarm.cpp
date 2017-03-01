/* 
** NetXMS - Network Management System
** nxalarm - manage alarms from command line
** Copyright (C) 2003-2015 Victor Kirhenshtein
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
** File: nxalarm.cpp
**
**/

#include "nxalarm.h"

/**
 * Alarm list output format
 */
static TCHAR m_outFormat[MAX_DB_STRING] = _T("%I %S %H %m");

/**
 * List alarms
 */
static UINT32 ListAlarms(NXCSession *session)
{
	UINT32 rcc = ((EventController *)session->getController(CONTROLLER_EVENTS))->syncEventTemplates();
	if (rcc != RCC_SUCCESS)
		_tprintf(_T("WARNING: cannot load event database (%s)\n"), NXCGetErrorText(rcc));

   AlarmController *ctrl = (AlarmController *)session->getController(CONTROLLER_ALARMS);

   ObjectArray<NXC_ALARM> *alarms;
   rcc = ctrl->getAll(&alarms);
	if (rcc == RCC_SUCCESS)
	{
      for(int i = 0; i < alarms->size(); i++)
		{
         TCHAR *text = ctrl->formatAlarmText(alarms->get(i), m_outFormat);
			_tprintf(_T("%s\n"), text);
			free(text);
		}
      _tprintf(_T("\n%d active alarms\n"), alarms->size());
      delete alarms;
	}
	else
	{
		_tprintf(_T("Cannot get alarm list: %s\n"), NXCGetErrorText(rcc));
	}
	return rcc;
}

/**
 * Callback function for debug printing
 */
static void DebugCallback(const TCHAR *pMsg)
{
   _tprintf(_T("NXCL: %s\n"), pMsg);
}

#ifdef _WIN32
#define CMDLINE_OPTIONS "Deho:P:sS:u:vw:"
#else
#define CMDLINE_OPTIONS "c:Deho:P:sS:u:vw:"
#endif

/**
 * main
 */
int main(int argc, char *argv[])
{
	TCHAR login[MAX_DB_STRING] = _T("guest");
   TCHAR password[MAX_DB_STRING] = _T("");
	char *eptr;
	bool isDebug = false, isEncrypt = false, isSticky = false;
	UINT32 rcc, alarmId;
   int timeout = 3, ackTimeout = 0;
	int ch;

   InitNetXMSProcess(true);

   // Parse command line
   opterr = 1;
   while((ch = getopt(argc, argv, CMDLINE_OPTIONS)) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
            printf("Usage: nxalarm [<options>] <server> <command> [<alarm_id>]\n"
				       "Possible commands are:\n"
						 "   ack <id>                : Acknowledge alarm\n"
                   "   add-comment <id> <text> : Add comment to alarm\n"
						 "   get-comments <id>       : Get comments of alarm\n"
						 "   list                    : List active alarms\n"
						 "   open <id>               : Open helpdesk issue from alarm\n"
						 "   resolve <id>            : Resolve alarm\n"
						 "   terminate <id>          : Terminate alarm\n"
                   "Valid options are:\n"
#ifndef _WIN32
                   "   -c            : Codepage (default is " ICONV_DEFAULT_CODEPAGE ")\n"
#endif
                   "   -D             : Turn on debug mode.\n"
                   "   -e             : Encrypt session.\n"
                   "   -h             : Display help and exit.\n"
						 "   -o <format>    : Output format for list (see below).\n"
                   "   -P <password>  : Specify user's password. Default is empty password.\n"
                   "   -s             : Sticky acknowledge (only for \"ack\" command).\n"
                   "   -S <minutes>   : Sticky acknowledge with timeout (only for \"ack\" command).\n"
                   "   -u <user>      : Login to server as <user>. Default is \"guest\".\n"
                   "   -v             : Display version and exit.\n"
                   "   -w <seconds>   : Specify command timeout (default is 3 seconds).\n"
						 "Output format string syntax:\n"
						 "   %%a Primary IP address of source object\n"
						 "   %%A Primary host name of source object\n"
						 "   %%c Repeat count\n"
						 "   %%d Related DCI ID\n"
						 "   %%e Event code\n"
						 "   %%E Event name\n"
						 "   %%h Helpdesk state as number\n"
						 "   %%H Helpdesk state as text\n"
						 "   %%i Source object identifier\n"
						 "   %%I Alarm identifier\n"
						 "   %%m Message text\n"
						 "   %%n Source object name\n"
						 "   %%s Severity as number\n"
						 "   %%S Severity as text\n"
						 "   %%x Alarm state as number\n"
						 "   %%X Alarm state as text\n"
						 "   %%%% Percent sign\n"
                   "Default format is %%I %%S %%H %%m\n"
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
            isEncrypt = true;
            break;
         case 'o':
#ifdef UNICODE
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, m_outFormat, MAX_DB_STRING);
				m_outFormat[MAX_DB_STRING - 1] = 0;
#else
            nx_strncpy(m_outFormat, optarg, MAX_DB_STRING);
#endif
            break;
         case 'P':
#ifdef UNICODE
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, password, MAX_DB_STRING);
				password[MAX_DB_STRING - 1] = 0;
#else
            nx_strncpy(password, optarg, MAX_DB_STRING);
#endif
            break;
         case 's':
            isSticky = true;
            break;
         case 'S':
            isSticky = true;
            ackTimeout = strtol(optarg, NULL, 0);
            if (ackTimeout < 1)
            {
               _tprintf(_T("Invalid timeout %hs\n"), optarg);
               return 1;
            }
            break;
         case 'u':
#ifdef UNICODE
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, login, MAX_DB_STRING);
				login[MAX_DB_STRING - 1] = 0;
#else
            nx_strncpy(login, optarg, MAX_DB_STRING);
#endif
            break;
         case 'w':
            timeout = strtol(optarg, NULL, 0);
            if ((timeout < 1) || (timeout > 120))
            {
               _tprintf(_T("Invalid timeout %hs\n"), optarg);
               return 1;
            }
            break;
         case 'v':
            _tprintf(_T("NetXMS Alarm Control  Version ") NETXMS_VERSION_STRING _T("\n"));
            return 0;
         case '?':
            return 1;
         default:
            break;
		}
	}

   if (argc - optind < 2)
   {
      _tprintf(_T("Required arguments missing. Use nxalarm -h for help.\n"));
		return 1;
	}

	// All commands except "list" requirs alarm id
   if ((stricmp(argv[optind + 1], "list") && (argc - optind < 3)) ||
       (!stricmp(argv[optind + 1], "add-comment") && (argc - optind < 4)))
   {
      _tprintf(_T("Required arguments missing. Use nxalarm -h for help.\n"));
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

   NXCSession *session = new NXCSession();
   static UINT32 protocolVersions[] = { CPV_INDEX_ALARMS };
   rcc = session->connect(_HOST, login, password, isEncrypt ? NXCF_ENCRYPT : 0, _T("nxalarm/") NETXMS_VERSION_STRING, 
                          protocolVersions, sizeof(protocolVersions) / sizeof(UINT32));
	if (rcc != RCC_SUCCESS)
	{
		_tprintf(_T("Unable to connect to server: %s\n"), NXCGetErrorText(rcc));
      delete session;
      NXCShutdown();
		return 2;
	}

	session->setCommandTimeout((UINT32)timeout * 1000);

	// Execute command
	if (!stricmp(argv[optind + 1], "list"))
	{
		rcc = ListAlarms(session);
	}
	else
	{
		alarmId = strtoul(argv[optind + 2], &eptr, 0);
		if ((*eptr != 0) || (alarmId == 0))
		{
			_tprintf(_T("Invalid alarm ID \"%hs\"\n"), argv[optind + 2]);
         delete session;
			return 1;
		}

      AlarmController *ctrl = (AlarmController *)session->getController(CONTROLLER_ALARMS);
		if (!stricmp(argv[optind + 1], "ack"))
		{
         rcc = ctrl->acknowledge(alarmId, isSticky, (UINT32)ackTimeout * 60);
			if (rcc != RCC_SUCCESS)
				_tprintf(_T("Cannot acknowledge alarm: %s\n"), NXCGetErrorText(rcc));
		}
		else if (!stricmp(argv[optind + 1], "add-comment"))
		{
#ifdef UNICODE
         WCHAR *wtext = WideStringFromMBString(argv[optind + 3]);
         rcc = ctrl->addComment(alarmId, wtext);
         free(wtext);
#else
			rcc = ctrl->addComment(alarmId, argv[optind + 3]);
#endif
			if (rcc != RCC_SUCCESS)
				_tprintf(_T("Cannot add alarm comment: %s\n"), NXCGetErrorText(rcc));
		}
		else if (!stricmp(argv[optind + 1], "get-comments"))
		{
         ObjectArray<AlarmComment> *comments;
         rcc = ctrl->getComments(alarmId, &comments);
         if (rcc == RCC_SUCCESS)
         {
            for(int i = 0; i < comments->size(); i++)
            {
               AlarmComment *c = comments->get(i);
               time_t t = c->getTimestamp();
               struct tm *ltm = localtime(&t);
               TCHAR timeText[64];
               _tcsftime(timeText, 64, _T("%d-%b-%Y %H:%M:%S"), ltm);
               _tprintf(_T("[%d] %s by %s\n%s\n\n"), c->getId(), timeText, c->getUserName(), c->getText());
            }
            delete comments;
         }
         else
         {
				_tprintf(_T("Cannot get alarm comments: %s\n"), NXCGetErrorText(rcc));
         }
      }
		else if (!stricmp(argv[optind + 1], "open"))
		{
         TCHAR hdref[MAX_HELPDESK_REF_LEN];
			rcc = ctrl->openHelpdeskIssue(alarmId, hdref);
			if (rcc == RCC_SUCCESS)
				_tprintf(_T("Helpdesk issue open, reference ID is \"%s\"\n"), hdref);
         else
				_tprintf(_T("Cannot open helpdesk issue: %s\n"), NXCGetErrorText(rcc));
		}
		else if (!stricmp(argv[optind + 1], "resolve"))
		{
         rcc = ctrl->resolve(alarmId);
			if (rcc != RCC_SUCCESS)
				_tprintf(_T("Cannot resolve alarm: %s\n"), NXCGetErrorText(rcc));
		}
		else if (!stricmp(argv[optind + 1], "terminate"))
		{
         rcc = ctrl->terminate(alarmId);
			if (rcc != RCC_SUCCESS)
				_tprintf(_T("Cannot terminate alarm: %s\n"), NXCGetErrorText(rcc));
		}
		else
		{
			_tprintf(_T("Invalid command \"%hs\"\n"), argv[optind + 1]);
			delete session;
			return 1;
		}
	}

   delete session;
   NXCShutdown();
	return (rcc == RCC_SUCCESS) ? 0 : 5;
}
