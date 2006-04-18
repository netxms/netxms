// nxconfig.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "nxconfig.h"

#include "MainFrm.h"
#include "ConfigWizard.h"
#include "IntroPage.h"
#include "DBSelectPage.h"
#include "ODBCPage.h"
#include "PollCfgPage.h"
#include "SMTPPage.h"
#include "SummaryPage.h"
#include "ProcessingPage.h"
#include "ConfigFilePage.h"
#include "WinSrvPage.h"
#include "LoggingPage.h"
#include "FinishPage.h"

#include <iphlpapi.h>
#include <iprtrmib.h>
#include <rtinfo.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNxconfigApp

BEGIN_MESSAGE_MAP(CNxconfigApp, CWinApp)
	//{{AFX_MSG_MAP(CNxconfigApp)
	ON_COMMAND(ID_FILE_CFG_WIZARD, OnFileCfgWizard)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNxconfigApp construction

CNxconfigApp::CNxconfigApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CNxconfigApp object

CNxconfigApp appNxConfig;

/////////////////////////////////////////////////////////////////////////////
// CNxconfigApp initialization

BOOL CNxconfigApp::InitInstance()
{
   HKEY hKey;
   DWORD dwSize, dwData = 0;
   TCHAR *pszArg, szCmd[1024];

	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}

   // Read installation data from registry
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\NetXMS\\Server"), 0,
                    KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
   {
      dwSize = sizeof(DWORD);
      RegQueryValueEx(hKey, _T("ServerIsConfigured"), NULL, NULL, (BYTE *)&dwData, &dwSize);

      dwSize = (MAX_PATH - 16) * sizeof(TCHAR);
      if (RegQueryValueEx(hKey, _T("InstallPath"), NULL, NULL, 
                          (BYTE *)m_szInstallDir, &dwSize) != ERROR_SUCCESS)
      {
         AfxMessageBox(_T("Unable to determine NetXMS installation directory"));
         m_szInstallDir[0] = 0;
      }

      RegCloseKey(hKey);
   }
   else
   {
      AfxMessageBox(_T("Unable to determine NetXMS installation directory"));
      m_szInstallDir[0] = 0;
   }

   // Parse command line
   if (!_tcsicmp(m_lpCmdLine, _T("--create-agent-config")))
   {
      CreateAgentConfig();
      return FALSE;
   }
   pszArg = ExtractWord(m_lpCmdLine, szCmd);
   if (!_tcsicmp(szCmd, _T("--create-nxhttpd-config")))
   {
      CreateWebConfig(pszArg);
      return FALSE;
   }

   // Check if server is already configured or we cannot determine
   // installation directory
   if ((dwData != 0) || (m_szInstallDir[0] == 0))
   {
      if ((dwData != 0) && (_tcsicmp(m_lpCmdLine, _T("--configure-if-needed"))))
         AfxMessageBox(_T("Server already configured"));
      return FALSE;
   }

	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));


	// To create the main window, this code creates a new frame window
	// object and then sets it as the application's main window object.

	CMainFrame* pFrame = new CMainFrame;
	m_pMainWnd = pFrame;

	// create and load the frame with its resources

	pFrame->LoadFrame(IDR_MAINFRAME,
		WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL,
		NULL);

	// The one and only window has been initialized, so show and update it.
	//pFrame->ShowWindow(SW_SHOW);
	//pFrame->UpdateWindow();

   pFrame->PostMessage(WM_COMMAND, ID_FILE_CFG_WIZARD);

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CNxconfigApp message handlers

void CNxconfigApp::OnFileCfgWizard() 
{
   CConfigWizard dlg(_T("Configure NetXMS Server"), m_pMainWnd, 0);
   CIntroPage pgIntro;
   CConfigFilePage pgConfigFile;
   CDBSelectPage pgSelectDB;
   CODBCPage pgCheckODBC;
   CPollCfgPage pgPollConfig;
   CSMTPPage pgSMTP;
   CLoggingPage pgLogging;
   CWinSrvPage pgWinSrv;
   CSummaryPage pgSummary;
   CProcessingPage pgProcessing;
   CFinishPage pgFinish;

   dlg.m_psh.dwFlags |= PSH_WIZARD;
   dlg.AddPage(&pgIntro);
   dlg.AddPage(&pgConfigFile);
   dlg.AddPage(&pgSelectDB);
   dlg.AddPage(&pgCheckODBC);
   dlg.AddPage(&pgPollConfig);
   dlg.AddPage(&pgSMTP);
   dlg.AddPage(&pgLogging);
   dlg.AddPage(&pgWinSrv);
   dlg.AddPage(&pgSummary);
   dlg.AddPage(&pgProcessing);
   dlg.AddPage(&pgFinish);

   if (dlg.DoModal() == ID_WIZFINISH)
   {
      HKEY hKey;
      DWORD dwData = 1;

      if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\NetXMS\\Server"), 0,
                       KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
      {
         RegSetValueEx(hKey, _T("ServerIsConfigured"), 0, REG_DWORD, (BYTE *)&dwData, sizeof(DWORD));
         RegCloseKey(hKey);
      }
   }

   m_pMainWnd->PostMessage(WM_COMMAND, ID_APP_EXIT);
}


//
// Create configuration file for local agent
//

void CNxconfigApp::CreateAgentConfig()
{
   FILE *fp;
   time_t currTime;
   DWORD dwSize;
   IP_ADAPTER_INFO *pBuffer, *pInfo;
   IP_ADDR_STRING *pAddr;
   TCHAR szAddrList[4096], szFile[MAX_PATH];

   _sntprintf(szFile, MAX_PATH, _T("%s\\etc\\nxagentd.conf"), m_szInstallDir);
   if (_taccess(szFile, 0) == 0)
      return;  // File already exist, we shouldn't overwrite it

   // Get local interface list
   szAddrList[0] = 0;
   if (GetAdaptersInfo(NULL, &dwSize) == ERROR_BUFFER_OVERFLOW)
   {
      pBuffer = (IP_ADAPTER_INFO *)malloc(dwSize);
      if (GetAdaptersInfo(pBuffer, &dwSize) == ERROR_SUCCESS)
      {
         for(pInfo = pBuffer; pInfo != NULL; pInfo = pInfo->Next)
         {
            // Read all IP addresses for adapter
            for(pAddr = &pInfo->IpAddressList; pAddr != NULL; pAddr = pAddr->Next)
            {
               if (_tcscmp(pAddr->IpAddress.String, _T("0.0.0.0")))
               {
                  if (szAddrList[0] != 0)
                     _tcscat(szAddrList, _T(", "));
                  _tcscat(szAddrList, pAddr->IpAddress.String);
               }
            }
         }
      }
      free(pBuffer);
   }

   fp = _tfopen(szFile, _T("w"));
   if (fp != NULL)
   {
      currTime = time(NULL);
      _ftprintf(fp, _T("#\n# NetXMS agent configuration file\n# Created by server installer at %s#\n\n"),
                _tctime(&currTime));
      _ftprintf(fp, _T("LogFile = {syslog}\nServers = 127.0.0.1\n"));
      if (szAddrList[0] != 0)
         _ftprintf(fp, _T("InstallationServers = %s\n"), szAddrList);
      _ftprintf(fp, _T("FileStore = %s\\var\n"), m_szInstallDir);
      _ftprintf(fp, _T("RequireAuthentication = no\n"));
      _ftprintf(fp, _T("SubAgent = winperf.nsm\nSubAgent = portcheck.nsm\n"));
      fclose(fp);
   }
}


//
// Create configuration file for nxhttpd
//

void CNxconfigApp::CreateWebConfig(TCHAR *pszServer)
{
   FILE *fp;
   time_t currTime;
   TCHAR szFile[MAX_PATH];

   _sntprintf(szFile, MAX_PATH, _T("%s\\etc\\nxhttpd.conf"), m_szInstallDir);
   if (_taccess(szFile, 0) == 0)
      return;  // File already exist, we shouldn't overwrite it

   fp = _tfopen(szFile, _T("w"));
   if (fp != NULL)
   {
      currTime = time(NULL);
      _ftprintf(fp, _T("#\n# NetXMS web server configuration file\n# Created by server installer at %s#\n\n"),
                _tctime(&currTime));
      _ftprintf(fp, _T("LogFile = {syslog}\nMasterServer = %s\n"), pszServer);
      _ftprintf(fp, _T("DocumentRoot = %s\\var\\www\n"), m_szInstallDir);
      fclose(fp);
   }
}
