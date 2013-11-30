// ConfigWizard.cpp : implementation file
//

#include "stdafx.h"
#include "nxconfig.h"
#include "ConfigWizard.h"
#include <nxconfig.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CConfigWizard

IMPLEMENT_DYNAMIC(CConfigWizard, CPropertySheet)

CConfigWizard::CConfigWizard(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
   memset(&m_cfg, 0, sizeof(WIZARD_CFG_INFO));
   DefaultConfig();
}

CConfigWizard::CConfigWizard(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
   memset(&m_cfg, 0, sizeof(WIZARD_CFG_INFO));
   DefaultConfig();
}

CConfigWizard::~CConfigWizard()
{
   safe_free(m_cfg.m_pszDependencyList);
}


BEGIN_MESSAGE_MAP(CConfigWizard, CPropertySheet)
	//{{AFX_MSG_MAP(CConfigWizard)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfigWizard message handlers

//
// Create default configuration
//

void CConfigWizard::DefaultConfig()
{
   HKEY hKey;
   DWORD dwSize;

   m_cfg.m_bCreateDB = TRUE;
   m_cfg.m_bInitDB = TRUE;
   m_cfg.m_iDBEngine = DB_ENGINE_MSSQL;
   _tcscpy(m_cfg.m_szDBServer, _T("localhost"));
   _tcscpy(m_cfg.m_szDBLogin, _T("netxms"));
   _tcscpy(m_cfg.m_szDBName, _T("netxms_db"));
   _tcscpy(m_cfg.m_szDBALogin, _T("sa"));
   m_cfg.m_bLogFailedSQLQueries = TRUE;
   m_cfg.m_bEnableAdminInterface = TRUE;
   m_cfg.m_bRunAutoDiscovery = FALSE;
   m_cfg.m_dwDiscoveryPI = 14400;
   m_cfg.m_dwConfigurationPI = 3600;
   m_cfg.m_dwNumConfigPollers = 4;
   m_cfg.m_dwStatusPI = 60;
   m_cfg.m_dwNumStatusPollers = 10;
   m_cfg.m_bLogToSyslog = TRUE;
   _tcscpy(m_cfg.m_szSMSDriver, _T("generic.sms"));
   _tcscpy(m_cfg.m_szSMSDrvParam, _T("COM1:"));

   // Read installation info from registry
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\NetXMS\\Server"), 0,
                    KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
   {
      dwSize = MAX_PATH * sizeof(TCHAR);
      if (RegQueryValueEx(hKey, _T("ConfigFile"), NULL, NULL, 
                          (BYTE *)m_cfg.m_szConfigFile, &dwSize) == ERROR_SUCCESS)
      {
         m_cfg.m_bConfigFileDetected = TRUE;
      }

      dwSize = (MAX_PATH - 16) * sizeof(TCHAR);
      if (RegQueryValueEx(hKey, _T("InstallPath"), NULL, NULL, 
                          (BYTE *)m_cfg.m_szInstallDir, &dwSize) == ERROR_SUCCESS)
      {
         _tcscpy(m_cfg.m_szLogFile, m_cfg.m_szInstallDir);
         _tcscat(m_cfg.m_szLogFile, _T("\\log\\netxmsd.log"));
      }
      
      RegCloseKey(hKey);
   }

   // Read configuration file, if it's exist
   if (m_cfg.m_bConfigFileDetected)
   {
      static NX_CFG_TEMPLATE cfgTemplate[] =
      {
         { _T("DBDriver"), CT_STRING, 0, 0, MAX_PATH, 0, NULL, NULL },
         { _T("DBDrvParams"), CT_STRING, 0, 0, MAX_PATH, 0, NULL, NULL },
         { _T("DBLogin"), CT_STRING, 0, 0, MAX_DB_LOGIN, 0, NULL, NULL },
         { _T("DBName"), CT_STRING, 0, 0, MAX_DB_NAME, 0, NULL, NULL },
         { _T("DBPassword"), CT_STRING, 0, 0, MAX_DB_PASSWORD, 0, NULL, NULL },
         { _T("DBServer"), CT_STRING, 0, 0, MAX_PATH, 0, NULL, NULL },
         { _T("LogFailedSQLQueries"), CT_BOOLEAN, 0, 0, 1, 0, NULL, NULL },
         { _T("LogFile"), CT_STRING, 0, 0, MAX_PATH, 0, NULL, NULL },
         { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL, NULL }
      };
      DWORD dwFlags;

      cfgTemplate[0].buffer = m_cfg.m_szDBDriver;
      cfgTemplate[1].buffer = m_cfg.m_szDBDrvParams;
      cfgTemplate[2].buffer = m_cfg.m_szDBLogin;
      cfgTemplate[3].buffer = m_cfg.m_szDBName;
      cfgTemplate[4].buffer = m_cfg.m_szDBPassword;
      cfgTemplate[5].buffer = m_cfg.m_szDBServer;
      cfgTemplate[6].buffer = &dwFlags;
      cfgTemplate[7].buffer = m_cfg.m_szLogFile;

		Config config;
      if (config.loadConfig(m_cfg.m_szConfigFile, _T("server")))
      {
			if (config.parseTemplate(_T("server"), cfgTemplate))
			{
				m_cfg.m_bLogFailedSQLQueries = (dwFlags ? TRUE : FALSE);
			}
      }
   }
}
