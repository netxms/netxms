/* 
** NetXMS - Network Management System
** HTTP Server
** Copyright (C) 2006, 2007 Alex Kirhenshtein and Victor Kirhenshtein
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
** File: nxhttpd.cpp
**
**/

#include "nxhttpd.h"

#ifdef _WIN32
#include <conio.h>
#else
#include <signal.h>
#endif


//
// Externals
//

THREAD_RESULT THREAD_CALL ListenerThread(void *);
THREAD_RESULT THREAD_CALL SessionWatchdog(void *);


//
// Global variables
//

DWORD g_dwFlags = 0;
TCHAR g_szMasterServer[MAX_OBJECT_NAME] = _T("localhost");
#ifdef _WIN32
TCHAR g_szConfigFile[MAX_PATH] = _T("C:\\nxhttpd.conf");
TCHAR g_szLogFile[MAX_PATH] = _T("C:\\nxhttpd.log");
TCHAR g_szDocumentRoot[MAX_PATH] = _T("C:\\NetXMS\\var\\www");
#else
TCHAR g_szConfigFile[MAX_PATH] = _T("/etc/nxhttpd.conf");
TCHAR g_szLogFile[MAX_PATH] = _T("/var/log/nxhttpd");
TCHAR g_szDocumentRoot[MAX_PATH] = DATADIR _T("/nxhttpd");
TCHAR g_szPidFile[MAX_PATH] = _T("/var/run/nxhttpd.pid");
#endif
WORD g_wListenPort = 8080;
DWORD g_dwSessionTimeout = 300;		// 5 minutes


//
// Global string constants
//

const TCHAR *g_szStatusText[] = { _T("NORMAL"), _T("WARNING"), _T("MINOR"), _T("MAJOR"),
                            _T("CRITICAL"), _T("UNKNOWN"), _T("UNMANAGED"),
                            _T("DISABLED"), _T("TESTING"), NULL };
const TCHAR *g_szStatusTextSmall[] = { _T("Normal"), _T("Warning"), _T("Minor"), _T("Major"),
                                 _T("Critical"), _T("Unknown"), _T("Unmanaged"),
                                 _T("Disabled"), _T("Testing"), NULL };
const TCHAR *g_szStatusImageName[] = { _T("normal"), _T("warning"), _T("minor"), _T("major"),
                                 _T("critical"), _T("unknown"), _T("unmanaged"),
                                 _T("disabled"), _T("testing"), NULL };
const TCHAR *g_szAlarmState[] = { _T("Outstanding"), _T("Acknowledged"), _T("Terminated") };
const TCHAR *g_szObjectClass[] = { _T("Generic"), _T("Subnet"), _T("Node"), _T("Interface"), _T("Network"), 
                             _T("Container"), _T("Zone"), _T("ServiceRoot"), _T("Template"), 
                             _T("TemplateGroup"), _T("TemplateRoot"), _T("NetworkService"),
                             _T("VPNConnector"), _T("Condition"), _T("Cluster") };
const TCHAR *g_szInterfaceTypes[] = 
{
   _T("Unknown"),
   _T("Other"),
   _T("Regular 1822"),
   _T("HDH 1822"),
   _T("DDN X.25"),
   _T("RFC877 X.25"),
   _T("Ethernet CSMA/CD"),
   _T("ISO 802.3 CSMA/CD"),
   _T("ISO 802.4 Token Bus"),
   _T("ISO 802.5 Token Ring"),
   _T("ISO 802.6 MAN"),
   _T("StarLan"),
   _T("PROTEON 10 Mbps"),
   _T("PROTEON 80 Mbps"),
   _T("Hyper Channel"),
   _T("FDDI"),
   _T("LAPB"),
   _T("SDLC"),
   _T("DS1"),
   _T("E1"),
   _T("ISDN BRI"),
   _T("ISDN PRI"),
   _T("Proprietary Serial Pt-to-Pt"),
   _T("PPP"),
   _T("Software Loopback"),
   _T("EON (CLNP over IP)"),
   _T("Ethernet 3 Mbps"),
   _T("NSIP (XNS over IP)"),
   _T("SLIP"),
   _T("DS3"),
   _T("SMDS"),
   _T("Frame Relay"),
   _T("RS-232"),
   _T("PARA"),
   _T("ArcNet"),
   _T("ArcNet Plus"),
   _T("ATM"),
   _T("MIO X.25"),
   _T("SONET"),
   _T("X.25 PLE"),
   _T("ISO 88022 LLC"),
   _T("LocalTalk"),
   _T("SMDS DXI"),
   _T("Frame Relay Service"),
   _T("V.35"),
   _T("HSSI"),
   _T("HIPPI"),
   _T("Modem"),
   _T("AAL5"),
   _T("SONET PATH"),
   _T("SONET VT"),
   _T("SMDS ICIP"),
   _T("Proprietary Virtual"),
   _T("Proprietary Multiplexor"),
   _T("IEEE 802.12"),
   _T("FibreChannel")
};
CODE_TO_TEXT g_ctNodeType[] =
{
   { NODE_TYPE_GENERIC, _T("Generic") },
   { NODE_TYPE_NORTEL_ACCELAR, _T("Nortel Networks Passport switch") },
   { NODE_TYPE_NETSCREEN, _T("NetScreen Firewall/VPN") },
   { NODE_TYPE_NORTEL_BAYSTACK, _T("Nortel Ethernet switch (former BayStack)") },
   { 0, NULL }    // End of list
};


//
// Static data
//

#if defined(_WIN32) || defined(_NETWARE)
static CONDITION m_hCondShutdown = INVALID_CONDITION_HANDLE;
#else
char m_szPidFile[MAX_PATH] = "/var/run/nxhttpd.pid";
#endif
static THREAD m_thSessionWatchdog = INVALID_THREAD_HANDLE;
static THREAD m_thListener = INVALID_THREAD_HANDLE;


//
// Configuration file template
//

static NX_CFG_TEMPLATE m_cfgTemplate[] =
{
   { "DocumentRoot", CT_STRING, 0, 0, MAX_PATH, 0, g_szDocumentRoot },
   { "ListenPort", CT_WORD, 0, 0, 0, 0, &g_wListenPort },
   { "LogFile", CT_STRING, 0, 0, MAX_PATH, 0, g_szLogFile },
   { "MasterServer", CT_STRING, 0, 0, MAX_PATH, 0, g_szMasterServer },
   { "SessionTimeout", CT_LONG, 0, 0, 0, 0, &g_dwSessionTimeout },
   { "", CT_END_OF_LIST, 0, 0, 0, 0, NULL }
};


//
// Control/signal handlers
//

#ifndef _WIN32

static THREAD_RESULT THREAD_CALL SignalHandler(void *pArg)
{
	sigset_t signals;
	int nSignal;

	sigemptyset(&signals);
	sigaddset(&signals, SIGTERM);
	sigaddset(&signals, SIGINT);
	sigaddset(&signals, SIGPIPE);
	sigaddset(&signals, SIGSEGV);
	sigaddset(&signals, SIGHUP);
	sigaddset(&signals, SIGUSR1);
	sigaddset(&signals, SIGUSR2);

	sigprocmask(SIG_BLOCK, &signals, NULL);

	while(1)
	{
		if (sigwait(&signals, &nSignal) == 0)
		{
			switch(nSignal)
			{
				case SIGTERM:
				case SIGINT:
					goto stop_handler;
				case SIGSEGV:
					abort();
					break;
				default:
					break;
			}
		}
		else
		{
			ThreadSleepMs(100);
		}
	}

stop_handler:
	sigprocmask(SIG_UNBLOCK, &signals, NULL);
	return THREAD_OK;
}

#endif


//
// Initialization
//

BOOL Initialize(void)
{
	InitLog();

#ifdef _WIN32
	WSADATA wsaData;

   if (WSAStartup(2, &wsaData) != 0)
   {
      WriteLog(MSG_WSASTARTUP_FAILED, EVENTLOG_ERROR_TYPE, "e", WSAGetLastError());
      return FALSE;
   }
#endif

   if (!NXCInitialize())
   {
      WriteLog(MSG_NXCL_INIT_FAILED, EVENTLOG_ERROR_TYPE, NULL);
		return FALSE;
   }

	InitSessions();

   // Start network listener and session watchdog
   m_thListener = ThreadCreateEx(ListenerThread, 0, NULL);
   m_thSessionWatchdog = ThreadCreateEx(SessionWatchdog, 0, NULL);

	return TRUE;
}


//
// Initiate shutdown from service control handler
//

void Shutdown(void)
{
   // Set shutdowm flag
   g_dwFlags |= AF_SHUTDOWN;
   ThreadJoin(m_thSessionWatchdog);
   ThreadJoin(m_thListener);

   CloseLog();

   // Notify main thread about shutdown
#ifdef _WIN32
   ConditionSet(m_hCondShutdown);
#endif
   
   // Remove PID file
#if !defined(_WIN32) && !defined(_NETWARE)
   remove(g_szPidFile);
#endif
}


//
// Main loop
//

void Main(void)
{
   WriteLog(MSG_NXHTTPD_STARTED, EVENTLOG_INFORMATION_TYPE, NULL);

   if (g_dwFlags & AF_DAEMON)
   {
#if defined(_WIN32) || defined(_NETWARE)
      ConditionWait(m_hCondShutdown, INFINITE);
#else
      StartMainLoop(SignalHandler, NULL);
#endif
   }
   else
   {
#if defined(_WIN32)
      printf("NXHTTPD running. Press ESC to shutdown.\n");
      while(getch() != 27);
      printf("NXHTTPD shutting down...\n");
      Shutdown();
#else
      printf("NXHTTPD running. Press Ctrl+C to shutdown.\n");
      StartMainLoop(SignalHandler, NULL);
      printf("\nStopping nxhttpd...\n");
#endif
   }
}


//
// Command line options
//

#ifdef _WIN32
#define VALID_OPTIONS    "c:dDhIRsS"
#else
#define VALID_OPTIONS    "c:dDhp:"
#endif


//
// Help text
//

static char m_szHelpText[] =
   "NetXMS Web Server  Version " NETXMS_VERSION_STRING "\n"
   "Copyright (c) 2006, 2007 NetXMS Team\n\n"
   "Usage:\n"
   "   nxhttpd [options]\n\n"
   "Where valid options are:\n"
   "   -c <file>  : Read configuration from given file\n"
   "   -d         : Start as daemon (service)\n"
	"   -D         : Enable debug output\n"
   "   -h         : Show this help\n"
#ifdef _WIN32
   "   -I         : Install service\n"
   "   -R         : Remove service\n"
   "   -s         : Start service\n"
   "   -S         : Stop service\n"
#else
   "   -p <file>  : Write PID to given file\n"
#endif
   "\n";


//
// Entry point
//

int main(int argc, char *argv[])
{
	int ch, nAction = 0;
#ifdef _WIN32
	char szModuleName[MAX_PATH];
#endif

   InitThreadLibrary();

	// Parse command line
	opterr = 1;
	while((ch = getopt(argc, argv, VALID_OPTIONS)) != -1)
	{
		switch(ch)
		{
			case 'h':   // Display help and exit
				printf(m_szHelpText);
				return 0;
			case 'd':   // Run as daemon
				g_dwFlags |= AF_DAEMON;
				break;
			case 'D':   // Debug
				g_dwFlags |= AF_DEBUG;
				break;
			case 'c':
				nx_strncpy(g_szConfigFile, optarg, MAX_PATH);
				break;
#ifdef _WIN32
			case 'I':
				nAction = 1;
				break;
			case 'R':
				nAction = 2;
				break;
			case 's':
				nAction = 3;
				break;
			case 'S':
				nAction = 4;
				break;
#else
			case 'p':
				nx_strncpy(m_szPidFile, optarg, MAX_PATH);
				break;
#endif
			case '?':
				return 1;
			default:
				break;
		}
	}

	switch(nAction)
	{
		case 0:  // Start server
         if (NxLoadConfig(g_szConfigFile, "", m_cfgTemplate, !(g_dwFlags & AF_DAEMON)) == NXCFG_ERR_OK)
         {
#ifdef _WIN32
			   if (g_dwFlags & AF_DAEMON)
			   {
				   InitService();
			   }
			   else
			   {
				   if (Initialize())
               {
					   Main();
               }
			   }
#else
			   if (g_dwFlags & AF_DAEMON)
			   {
				   if (daemon(0, 0) == -1)
				   {
					   perror("Unable to setup itself as a daemon");
					   return 2;
				   }
			   }

			   if (Initialize())
			   {
				   FILE *pf;

				   pf = fopen(m_szPidFile, "w");
				   if (pf != NULL)
				   {
					   fprintf(pf, "%d", getpid());
					   fclose(pf);
				   }
					Main();
			   }
#endif
         }
         else
         {
            printf("Error loading configuration file\n");
            return 2;
         }
			break;
#ifdef _WIN32
		case 1:  // Install service
			GetModuleFileName(GetModuleHandle(NULL), szModuleName, MAX_PATH);
			NXHTTPDInstallService(szModuleName);
			break;
		case 2:
			NXHTTPDRemoveService();
			break;
		case 3:
			NXHTTPDStartService();
			break;
		case 4:
			NXHTTPDStopService();
			break;
#endif
		default:
			break;
	}

	return 0;
}
