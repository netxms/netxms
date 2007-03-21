/* $Id: main.cpp,v 1.1 2007-03-21 10:15:18 alk Exp $ */

#pragma warning(disable: 4786)

#include "nxhttpd.h"
#include "nxweb.h"

using namespace std;


//
// Global variables
//

char g_szMasterServer[MAX_OBJECT_NAME] = "localhost";
#ifdef _WIN32
char g_szConfigFile[MAX_PATH] = "C:\\nxhttpd.conf";
char g_szLogFile[MAX_PATH] = "C:\\nxhttpd.log";
#else
char g_szConfigFile[MAX_PATH] = "/etc/nxhttpd.conf";
char g_szLogFile[MAX_PATH] = "/var/log/nxhttpd";
#endif


//
// Static data
//

static WORD m_wListenPort = 8080;
static NxWeb m_server;
#ifdef _WIN32
static char m_szDocumentRoot[MAX_PATH] = "C:\\NetXMS\\var\\www\\data";
static char m_szTemplateRoot[MAX_PATH] = "C:\\NetXMS\\var\\www\\tpl";
static HANDLE m_eventMainStopped = INVALID_HANDLE_VALUE;
#else
static char m_szDocumentRoot[MAX_PATH] = DATADIR "/nxhttpd/data";
static char m_szTemplateRoot[MAX_PATH] = DATADIR "/nxhttpd/tpl";
char m_szPidFile[MAX_PATH] = "/var/run/nxhttpd.pid";
#endif


//
// Configuration file template
//

static NX_CFG_TEMPLATE m_cfgTemplate[] =
{
	{ "DocumentRoot", CT_STRING, 0, 0, MAX_PATH, 0, m_szDocumentRoot },
	{ "TemplateRoot", CT_STRING, 0, 0, MAX_PATH, 0, m_szTemplateRoot },
	{ "ListenPort", CT_WORD, 0, 0, 0, 0, &m_wListenPort },
	{ "LogFile", CT_STRING, 0, 0, MAX_PATH, 0, g_szLogFile },
	{ "MasterServer", CT_STRING, 0, 0, MAX_PATH, 0, g_szMasterServer },
	{ "", CT_END_OF_LIST, 0, 0, 0, 0, NULL }
};


//
// Control/signal handlers
//

#ifdef _WIN32

static BOOL WINAPI CtrlHandler(DWORD dwCtrlType)
{
	printf("Initiating web server shutdown...\n");
	m_server.Stop();
	return TRUE;
}

#else

static void SignalHandlerStub(int nSignal)
{
	// should be unused, but JIC...
	if (nSignal == SIGCHLD)
	{
		while (waitpid(-1, NULL, WNOHANG) > 0)
			;
	}
}

static THREAD_RESULT THREAD_CALL SignalHandler(void *pArg)
{
	sigset_t signals;
	int nSignal;

	// default for SIGCHLD: ignore
	signal(SIGCHLD, SignalHandlerStub);

	sigemptyset(&signals);
	sigaddset(&signals, SIGTERM);
	sigaddset(&signals, SIGINT);
	sigaddset(&signals, SIGPIPE);
	sigaddset(&signals, SIGSEGV);
	sigaddset(&signals, SIGCHLD);
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
				case SIGCHLD:
					while (waitpid(-1, NULL, WNOHANG) > 0)
						;
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
	m_server.Stop();
	return THREAD_OK;
}

#endif


//
// Initialization
//

BOOL Initialize(void)
{
#ifdef _WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	SetConsoleCtrlHandler(CtrlHandler, TRUE);
#endif

	return TRUE;
}


//
// Initiate shutdown from service control handler
//

void Shutdown(void)
{
	m_server.Stop();
#ifdef _WIN32
	// Wait for Main()
	WaitForSingleObject(m_eventMainStopped, INFINITE);
#endif
}


//
// Main loop
//

THREAD_RESULT THREAD_CALL Main(void *)
{
	// Main() termination event
#ifdef _WIN32
	m_eventMainStopped = CreateEvent(NULL, TRUE, FALSE, NULL);
#endif

	m_server.SetDocumentRoot(m_szDocumentRoot);
	m_server.SetPort(m_wListenPort);
	m_server.Start();

#ifdef _WIN32
	WSACleanup();
	SetEvent(m_eventMainStopped);
#endif

	return THREAD_OK;
}


//
// Command line options
//

#ifdef _WIN32
#define VALID_OPTIONS    "c:dhIRsS"
#else
#define VALID_OPTIONS    "c:dhp:"
#endif


//
// Help text
//

static char m_szHelpText[] =
"NetXMS Web Server  Version " NETXMS_VERSION_STRING "\n"
"Copyright (c) 2006 Alex Kirhenshtein\n\n"
"Usage:\n"
"   nxhttpd [options]\n\n"
"Where valid options are:\n"
"   -c <file>  : Read configuration from given file\n"
"   -d         : Start as daemon (service)\n"
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
	BOOL bDaemon = FALSE;
#ifdef _WIN32
	char szModuleName[MAX_PATH];
#endif

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
				bDaemon = TRUE;
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
			if (NxLoadConfig(g_szConfigFile, "", m_cfgTemplate, !bDaemon) == NXCFG_ERR_OK)
			{
#ifdef _WIN32
				if (bDaemon)
				{
					InitService();
				}
				else
				{
					if (Initialize())
					{
						printf("Server started. Press Ctrl+C to terminate.\n");
						Main(NULL);
					}
				}
#else
				if (bDaemon)
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
					StartMainLoop(SignalHandler, Main);
					unlink(m_szPidFile);
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

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $

*/
