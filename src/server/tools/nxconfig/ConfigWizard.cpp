// ConfigWizard.cpp : implementation file
//

#include "stdafx.h"
#include "nxconfig.h"
#include "ConfigWizard.h"

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
   m_cfg.m_bCreateDB = TRUE;
   m_cfg.m_bInitDB = TRUE;
   m_cfg.m_iDBEngine = DB_ENGINE_MSSQL;
   _tcscpy(m_cfg.m_szDBServer, _T("localhost"));
   _tcscpy(m_cfg.m_szDBLogin, _T("netxms"));
   _tcscpy(m_cfg.m_szDBName, _T("netxms_db"));
   _tcscpy(m_cfg.m_szDBALogin, _T("sa"));
   m_cfg.m_bEnableAdminInterface = TRUE;
   m_cfg.m_bRunAutoDiscovery = FALSE;
   m_cfg.m_dwDiscoveryPI = 14400;
   m_cfg.m_dwConfigurationPI = 3600;
   m_cfg.m_dwNumConfigPollers = 4;
   m_cfg.m_dwStatusPI = 60;
   m_cfg.m_dwNumStatusPollers = 10;
}
