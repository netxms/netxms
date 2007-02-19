// DBSelectPage.cpp : implementation file
//

#include "stdafx.h"
#include "nxconfig.h"
#include "DBSelectPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Constants
//

#define MAX_DB_ENGINES     5
#define MAX_DB_DRIVERS     2


//
// Global data
//

TCHAR *g_pszDBEngines[MAX_DB_ENGINES] = { _T("MySQL"), _T("PostgreSQL"),
                                          _T("Microsoft SQL Server"), _T("Oracle"),
                                          _T("SQLite Embedded DB") };


//
// Static data
//

static TCHAR *m_pszValidDrivers[MAX_DB_ENGINES][MAX_DB_DRIVERS] =
{
   { _T("mysql.ddr"), _T("odbc.ddr") },
   { _T("pgsql.ddr"), _T("odbc.ddr") },
   { _T("mssql.ddr"), _T("odbc.ddr") },
   { _T("oracle.ddr"), _T("odbc.ddr") },
   { _T("sqlite.ddr"), NULL }
};


/////////////////////////////////////////////////////////////////////////////
// CDBSelectPage property page

IMPLEMENT_DYNCREATE(CDBSelectPage, CPropertyPage)

CDBSelectPage::CDBSelectPage() : CPropertyPage(CDBSelectPage::IDD)
{
	//{{AFX_DATA_INIT(CDBSelectPage)
	//}}AFX_DATA_INIT
}

CDBSelectPage::~CDBSelectPage()
{
}

void CDBSelectPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDBSelectPage)
	DDX_Control(pDX, IDC_COMBO_DBENGINE, m_wndEngineList);
	DDX_Control(pDX, IDC_COMBO_DBDRV, m_wndDrvList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDBSelectPage, CPropertyPage)
	//{{AFX_MSG_MAP(CDBSelectPage)
	ON_CBN_SELCHANGE(IDC_COMBO_DBENGINE, OnSelchangeComboDbengine)
	ON_BN_CLICKED(IDC_RADIO_NEWDB, OnRadioNewdb)
	ON_BN_CLICKED(IDC_RADIO_EXISTINGDB, OnRadioExistingdb)
	ON_BN_CLICKED(IDC_CHECK_INITDB, OnCheckInitdb)
	ON_CBN_SELCHANGE(IDC_COMBO_DBDRV, OnSelchangeComboDbdrv)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDBSelectPage message handlers


//
// WM_INITDIALOG message handler
//

BOOL CDBSelectPage::OnInitDialog() 
{
   DWORD i;
   WIZARD_CFG_INFO *pc = &((CConfigWizard *)GetParent())->m_cfg;	

	CPropertyPage::OnInitDialog();
	
   // Initialize new/existing DB selection
   if (pc->m_bCreateDB)
      SendDlgItemMessage(IDC_RADIO_NEWDB, BM_SETCHECK, TRUE);
   else
      SendDlgItemMessage(IDC_RADIO_EXISTINGDB, BM_SETCHECK, TRUE);

   // Initialize DB engines list
   for(i = 0; i < MAX_DB_ENGINES; i++)
      m_wndEngineList.AddString(g_pszDBEngines[i]);
   m_wndEngineList.SelectString(-1, g_pszDBEngines[pc->m_iDBEngine]);
   OnEngineSelect();
   OnDriverSelect();

   // Initialize text fields
   SetDlgItemText(IDC_EDIT_SERVER, pc->m_szDBServer);
   SetDlgItemText(IDC_EDIT_DBA_LOGIN, pc->m_szDBALogin);
   SetDlgItemText(IDC_EDIT_DBA_PASSWORD, pc->m_szDBAPassword);
   SetDlgItemText(IDC_EDIT_DB_NAME, pc->m_szDBName);
   SetDlgItemText(IDC_EDIT_DB_LOGIN, pc->m_szDBLogin);
   SetDlgItemText(IDC_EDIT_DB_PASSWORD, pc->m_szDBPassword);
	
	return TRUE;
}


//
// Page activation handler
//

BOOL CDBSelectPage::OnSetActive()
{
   ((CPropertySheet *)GetParent())->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);
	return CPropertyPage::OnSetActive();
}


//
// Handler for "Next" buttin
//

LRESULT CDBSelectPage::OnWizardNext() 
{
   WIZARD_CFG_INFO *pc = &((CConfigWizard *)GetParent())->m_cfg;	

   m_wndDrvList.GetWindowText(pc->m_szDBDriver, MAX_DB_STRING);
   GetDlgItemText(IDC_EDIT_SERVER, pc->m_szDBServer, MAX_DB_STRING);
   GetDlgItemText(IDC_EDIT_DBA_LOGIN, pc->m_szDBALogin, MAX_DB_LOGIN);
   GetDlgItemText(IDC_EDIT_DBA_PASSWORD, pc->m_szDBAPassword, MAX_DB_PASSWORD);
   GetDlgItemText(IDC_EDIT_DB_NAME, pc->m_szDBName, MAX_DB_NAME);
   GetDlgItemText(IDC_EDIT_DB_LOGIN, pc->m_szDBLogin, MAX_DB_LOGIN);
   GetDlgItemText(IDC_EDIT_DB_PASSWORD, pc->m_szDBPassword, MAX_DB_PASSWORD);
   return !_tcsicmp(pc->m_szDBDriver, _T("odbc.ddr")) ? IDD_ODBC : IDD_POLLING;
}


//
// Change driver list when database engine selected
//

void CDBSelectPage::OnEngineSelect()
{
   TCHAR szBuffer[256];
   WIZARD_CFG_INFO *pc = &((CConfigWizard *)GetParent())->m_cfg;
   DWORD i;

   m_wndDrvList.GetWindowText(szBuffer, 256);
   m_wndDrvList.ResetContent();
   for(i = 0; i < MAX_DB_DRIVERS; i++)
      if (m_pszValidDrivers[pc->m_iDBEngine][i] != NULL)
         m_wndDrvList.AddString(m_pszValidDrivers[pc->m_iDBEngine][i]);
   if (m_wndDrvList.SelectString(-1, szBuffer) == CB_ERR)
      m_wndDrvList.SelectString(-1, m_pszValidDrivers[pc->m_iDBEngine][0]);
   m_wndDrvList.GetWindowText(pc->m_szDBDriver, MAX_DB_STRING);
   OnDriverSelect();
}


//
// DB engine selection change handler
//

void CDBSelectPage::OnSelchangeComboDbengine() 
{
   DWORD i;
   TCHAR szBuffer[256];
   WIZARD_CFG_INFO *pc = &((CConfigWizard *)GetParent())->m_cfg;
   
   m_wndEngineList.GetWindowText(szBuffer, 256);
   for(i = 0; i < MAX_DB_ENGINES; i++)
      if (!_tcscmp(szBuffer, g_pszDBEngines[i]))
      {
         pc->m_iDBEngine = i;
         OnEngineSelect();
         break;
      }
}


//
// Enable/disable dialog items when new or existing database selected
//

void CDBSelectPage::OnDBCreationSelect()
{
   WIZARD_CFG_INFO *pc = &((CConfigWizard *)GetParent())->m_cfg;

   pc->m_bCreateDB = (SendDlgItemMessage(IDC_RADIO_NEWDB, BM_GETCHECK) == BST_CHECKED);
   if (pc->m_bCreateDB)
   {
      EnableDlgItem(this, IDC_CHECK_INITDB, FALSE);
      if (!_tcscmp(pc->m_szDBDriver, _T("sqlite.ddr")))
      {
         EnableDlgItem(this, IDC_EDIT_DBA_LOGIN, FALSE);
         EnableDlgItem(this, IDC_EDIT_DBA_PASSWORD, FALSE);
      }
      else
      {
         EnableDlgItem(this, IDC_EDIT_DBA_LOGIN, TRUE);
         EnableDlgItem(this, IDC_EDIT_DBA_PASSWORD, TRUE);
      }
   }
   else
   {
      EnableDlgItem(this, IDC_CHECK_INITDB, TRUE);
      SendDlgItemMessage(IDC_CHECK_INITDB, BM_SETCHECK,
                         pc->m_bInitDB ? BST_CHECKED : BST_UNCHECKED);
      EnableDlgItem(this, IDC_EDIT_DBA_LOGIN, FALSE);
      EnableDlgItem(this, IDC_EDIT_DBA_PASSWORD, FALSE);
   }
}


//
// Radio buttons click handlers
//

void CDBSelectPage::OnRadioNewdb() 
{
   OnDBCreationSelect();
}

void CDBSelectPage::OnRadioExistingdb() 
{
   OnDBCreationSelect();
}


//
// Handler for "Initialize DB" check box
//

void CDBSelectPage::OnCheckInitdb() 
{
   WIZARD_CFG_INFO *pc = &((CConfigWizard *)GetParent())->m_cfg;

   pc->m_bInitDB = (SendDlgItemMessage(IDC_CHECK_INITDB, BM_GETCHECK) == BST_CHECKED);
}


//
// Handler for "Back" button
//

LRESULT CDBSelectPage::OnWizardBack() 
{
   WIZARD_CFG_INFO *pc = &((CConfigWizard *)GetParent())->m_cfg;
   return pc->m_bConfigFileDetected ? IDD_INTRO : IDD_CFG_FILE;
}


//
// DB driver selection change handler
//

void CDBSelectPage::OnSelchangeComboDbdrv() 
{
   WIZARD_CFG_INFO *pc = &((CConfigWizard *)GetParent())->m_cfg;
   
   m_wndDrvList.GetWindowText(pc->m_szDBDriver, MAX_DB_STRING);
   OnDriverSelect();
}


//
// Change dialog appearance when database driver selected
//

void CDBSelectPage::OnDriverSelect()
{
   WIZARD_CFG_INFO *pc = &((CConfigWizard *)GetParent())->m_cfg;

   if (!_tcscmp(pc->m_szDBDriver, _T("odbc.ddr")))
   {
      SetDlgItemText(IDC_STATIC_SERVER, _T("ODBC data source"));
      SetDlgItemText(IDC_STATIC_DBNAME, _T("Database name"));
      EnableDlgItem(this, IDC_EDIT_SERVER, TRUE);
      EnableDlgItem(this, IDC_EDIT_DB_NAME, FALSE);
      EnableDlgItem(this, IDC_EDIT_DB_LOGIN, TRUE);
      EnableDlgItem(this, IDC_EDIT_DB_PASSWORD, TRUE);
      SendDlgItemMessage(IDC_RADIO_NEWDB, BM_SETCHECK, BST_UNCHECKED);
      SendDlgItemMessage(IDC_RADIO_EXISTINGDB, BM_SETCHECK, BST_CHECKED);
      pc->m_bCreateDB = FALSE;
      EnableDlgItem(this, IDC_RADIO_NEWDB, FALSE);
   }
   else if (!_tcscmp(pc->m_szDBDriver, _T("sqlite.ddr")))
   {
      SetDlgItemText(IDC_STATIC_SERVER, _T("Database server"));
      SetDlgItemText(IDC_STATIC_DBNAME, _T("Database file"));
      EnableDlgItem(this, IDC_EDIT_SERVER, FALSE);
      EnableDlgItem(this, IDC_EDIT_DB_LOGIN, FALSE);
      EnableDlgItem(this, IDC_EDIT_DB_PASSWORD, FALSE);
      EnableDlgItem(this, IDC_EDIT_DB_NAME, TRUE);
      EnableDlgItem(this, IDC_RADIO_NEWDB, TRUE);
   }
   else if (!_tcscmp(pc->m_szDBDriver, _T("oracle.ddr")))
   {
      SetDlgItemText(IDC_STATIC_SERVER, _T("Oracle connect string"));
      SetDlgItemText(IDC_STATIC_DBNAME, _T("Database name"));
      EnableDlgItem(this, IDC_EDIT_SERVER, TRUE);
      EnableDlgItem(this, IDC_EDIT_DB_LOGIN, TRUE);
      EnableDlgItem(this, IDC_EDIT_DB_PASSWORD, TRUE);
      EnableDlgItem(this, IDC_EDIT_DB_NAME, FALSE);
      SendDlgItemMessage(IDC_RADIO_NEWDB, BM_SETCHECK, BST_UNCHECKED);
      SendDlgItemMessage(IDC_RADIO_EXISTINGDB, BM_SETCHECK, BST_CHECKED);
      pc->m_bCreateDB = FALSE;
      EnableDlgItem(this, IDC_RADIO_NEWDB, FALSE);
   }
   else
   {
      SetDlgItemText(IDC_STATIC_SERVER, _T("Database server"));
      SetDlgItemText(IDC_STATIC_DBNAME, _T("Database name"));
      EnableDlgItem(this, IDC_EDIT_SERVER, TRUE);
      EnableDlgItem(this, IDC_EDIT_DB_NAME, TRUE);
      EnableDlgItem(this, IDC_RADIO_NEWDB, TRUE);
      EnableDlgItem(this, IDC_EDIT_DB_LOGIN, TRUE);
      EnableDlgItem(this, IDC_EDIT_DB_PASSWORD, TRUE);
   }
   OnDBCreationSelect();
}
