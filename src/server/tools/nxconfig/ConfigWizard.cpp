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

CConfigWizard::CConfigWizard(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage, const TCHAR *installDir)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
   memset(&m_cfg, 0, sizeof(WIZARD_CFG_INFO));
   DefaultConfig(CHECK_NULL_EX(installDir));
}

CConfigWizard::~CConfigWizard()
{
   free(m_cfg.m_pszDependencyList);
}


BEGIN_MESSAGE_MAP(CConfigWizard, CPropertySheet)
	//{{AFX_MSG_MAP(CConfigWizard)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfigWizard message handlers

/**
 * Create default configuration
 */
void CConfigWizard::DefaultConfig(const TCHAR *installDir)
{
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
   m_cfg.m_discoveryPollingInterval = 14400;
   m_cfg.m_configurationPollingInterval = 3600;
   m_cfg.m_statusPollingInterval = 60;
   m_cfg.m_topologyPollingInterval = 1800;
   m_cfg.m_pollerPoolBaseSize = 10;
   m_cfg.m_pollerPoolMaxSize = 250;
   m_cfg.m_bLogToSyslog = FALSE;
   _tcscpy(m_cfg.m_szSMSDriver, _T("<none>"));
   _tcscpy(m_cfg.m_szSMSDrvParam, _T("COM1:"));

   // Read installation info from registry
   HKEY hKey;
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\NetXMS\\Server"), 0,
      KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
   {
      DWORD dwSize = MAX_PATH * sizeof(TCHAR);
      if (RegQueryValueEx(hKey, _T("ConfigFile"), NULL, NULL,
         (BYTE *)m_cfg.m_szConfigFile, &dwSize) == ERROR_SUCCESS)
      {
         m_cfg.m_bConfigFileDetected = TRUE;
      }

      if (*installDir == 0)
      {
         dwSize = (MAX_PATH - 16) * sizeof(TCHAR);
         RegQueryValueEx(hKey, _T("InstallPath"), NULL, NULL, (BYTE *)m_cfg.m_szInstallDir, &dwSize);
      }
      else
      {
         _tcslcpy(m_cfg.m_szInstallDir, installDir, MAX_PATH);
      }

      RegCloseKey(hKey);
   }
   else
   {
      _tcslcpy(m_cfg.m_szInstallDir, installDir, MAX_PATH);
   }
   _tcscpy(m_cfg.m_szLogFile, m_cfg.m_szInstallDir);
   _tcscat(m_cfg.m_szLogFile, _T("\\log\\netxmsd.log"));

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
