// UserPropDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "UserPropDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUserPropDlg dialog


CUserPropDlg::CUserPropDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CUserPropDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CUserPropDlg)
	m_bAccountDisabled = FALSE;
	m_bDropConn = FALSE;
	m_bEditEventDB = FALSE;
	m_bManageUsers = FALSE;
	m_bChangePassword = FALSE;
	m_bViewEventDB = FALSE;
	m_strDescription = _T("");
	m_strLogin = _T("");
	m_strFullName = _T("");
	m_bManageActions = FALSE;
	m_bManageEPP = FALSE;
	m_bManageConfig = FALSE;
	m_bConfigureTraps = FALSE;
	m_bDeleteAlarms = FALSE;
	m_bManagePkg = FALSE;
	m_bManageAgentCfg = FALSE;
	m_bManageScripts = FALSE;
	m_bManageTools = FALSE;
	m_bViewTrapLog = FALSE;
	m_strMappingData = _T("");
	m_bRegisterAgents = FALSE;
	m_bAccessFiles = FALSE;
	//}}AFX_DATA_INIT
   m_nAuthMethod = 0;
}


void CUserPropDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CUserPropDlg)
	DDX_Control(pDX, IDC_COMBO_MAPPING, m_wndComboMapping);
	DDX_Control(pDX, IDC_COMBO_AUTH, m_wndComboAuth);
	DDX_Check(pDX, IDC_CHECK_DISABLED, m_bAccountDisabled);
	DDX_Check(pDX, IDC_CHECK_DROP_CONN, m_bDropConn);
	DDX_Check(pDX, IDC_CHECK_EDIT_EVENTDB, m_bEditEventDB);
	DDX_Check(pDX, IDC_CHECK_MANAGE_USERS, m_bManageUsers);
	DDX_Check(pDX, IDC_CHECK_PASSWORD, m_bChangePassword);
	DDX_Check(pDX, IDC_CHECK_VIEW_EVENTDB, m_bViewEventDB);
	DDX_Text(pDX, IDC_EDIT_DESCRIPTION, m_strDescription);
	DDV_MaxChars(pDX, m_strDescription, 255);
	DDX_Text(pDX, IDC_EDIT_LOGIN_NAME, m_strLogin);
	DDV_MaxChars(pDX, m_strLogin, 63);
	DDX_Text(pDX, IDC_EDIT_NAME, m_strFullName);
	DDV_MaxChars(pDX, m_strFullName, 127);
	DDX_Check(pDX, IDC_CHECK_MANAGE_ACTIONS, m_bManageActions);
	DDX_Check(pDX, IDC_CHECK_MANAGE_EPP, m_bManageEPP);
	DDX_Check(pDX, IDC_CHECK_MANAGE_CONFIG, m_bManageConfig);
	DDX_Check(pDX, IDC_CHECK_SNMP_TRAPS, m_bConfigureTraps);
	DDX_Check(pDX, IDC_CHECK_DELETE_ALARMS, m_bDeleteAlarms);
	DDX_Check(pDX, IDC_CHECK_MANAGE_PKG, m_bManagePkg);
	DDX_Check(pDX, IDC_CHECK_MANAGE_AGENT_CFG, m_bManageAgentCfg);
	DDX_Check(pDX, IDC_CHECK_MANAGE_SCRIPTS, m_bManageScripts);
	DDX_Check(pDX, IDC_CHECK_MANAGE_TOOLS, m_bManageTools);
	DDX_Check(pDX, IDC_CHECK_VIEW_TRAP_LOG, m_bViewTrapLog);
	DDX_Text(pDX, IDC_EDIT_MAPPING_DATA, m_strMappingData);
	DDX_Check(pDX, IDC_CHECK_REGISTER_AGENTS, m_bRegisterAgents);
	DDX_Check(pDX, IDC_CHECK_ACCESS_FILES, m_bAccessFiles);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CUserPropDlg, CDialog)
	//{{AFX_MSG_MAP(CUserPropDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUserPropDlg message handlers

BOOL CUserPropDlg::OnInitDialog() 
{
   int i;

	CDialog::OnInitDialog();

   if (m_pUser->dwId == 0)
   {
      // Disable checkboxes with system access rights for superuser
      EnableDlgItem(this, IDC_CHECK_DISABLED, FALSE);
      EnableDlgItem(this, IDC_CHECK_DROP_CONN, FALSE);
      EnableDlgItem(this, IDC_CHECK_MANAGE_USERS, FALSE);
      EnableDlgItem(this, IDC_CHECK_SNMP_TRAPS, FALSE);
      EnableDlgItem(this, IDC_CHECK_MANAGE_CONFIG, FALSE);
      EnableDlgItem(this, IDC_CHECK_VIEW_EVENTDB, FALSE);
      EnableDlgItem(this, IDC_CHECK_EDIT_EVENTDB, FALSE);
      EnableDlgItem(this, IDC_CHECK_MANAGE_ACTIONS, FALSE);
      EnableDlgItem(this, IDC_CHECK_MANAGE_EPP, FALSE);
      EnableDlgItem(this, IDC_CHECK_MANAGE_PKG, FALSE);
      EnableDlgItem(this, IDC_CHECK_DELETE_ALARMS, FALSE);
      EnableDlgItem(this, IDC_CHECK_MANAGE_TOOLS, FALSE);
      EnableDlgItem(this, IDC_CHECK_MANAGE_MODULES, FALSE);
      EnableDlgItem(this, IDC_CHECK_MANAGE_AGENT_CFG, FALSE);
      EnableDlgItem(this, IDC_CHECK_MANAGE_SCRIPTS, FALSE);
      EnableDlgItem(this, IDC_CHECK_VIEW_TRAP_LOG, FALSE);
      EnableDlgItem(this, IDC_CHECK_REGISTER_AGENTS, FALSE);
      EnableDlgItem(this, IDC_CHECK_ACCESS_FILES, FALSE);
   }

   for(i = 0; g_szAuthMethod[i] != NULL; i++)
      m_wndComboAuth.AddString(g_szAuthMethod[i]);
   if ((m_nAuthMethod < 0) || (m_nAuthMethod >= i))
      m_nAuthMethod = 0;
   m_wndComboAuth.SelectString(-1, g_szAuthMethod[m_nAuthMethod]);

   for(i = 0; g_szCertMappingMethod[i] != NULL; i++)
      m_wndComboMapping.AddString(g_szCertMappingMethod[i]);
   if ((m_nMappingMethod < 0) || (m_nMappingMethod >= i))
      m_nMappingMethod = 0;
   m_wndComboMapping.SelectString(-1, g_szCertMappingMethod[m_nMappingMethod]);

	return TRUE;
}


//
// OK button handler
//

void CUserPropDlg::OnOK() 
{
   int i;
   TCHAR szText[256];

   m_wndComboAuth.GetWindowText(szText, 256);
   for(i = 0; g_szAuthMethod[i] != NULL; i++)
      if (!_tcscmp(szText, g_szAuthMethod[i]))
      {
         m_nAuthMethod = i;
         break;
      }

   m_wndComboMapping.GetWindowText(szText, 256);
   for(i = 0; g_szCertMappingMethod[i] != NULL; i++)
      if (!_tcscmp(szText, g_szCertMappingMethod[i]))
      {
         m_nMappingMethod = i;
         break;
      }

	CDialog::OnOK();
}
