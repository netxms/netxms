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
	//}}AFX_DATA_INIT
}


void CUserPropDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CUserPropDlg)
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
	CDialog::OnInitDialog();

   if (m_pUser->dwId == 0)
   {
      // Disable checkboxes with system access rights for superuser
      GetDlgItem(IDC_CHECK_DISABLED)->EnableWindow(FALSE);
      GetDlgItem(IDC_CHECK_DROP_CONN)->EnableWindow(FALSE);
      GetDlgItem(IDC_CHECK_MANAGE_USERS)->EnableWindow(FALSE);
      GetDlgItem(IDC_CHECK_SNMP_TRAPS)->EnableWindow(FALSE);
      GetDlgItem(IDC_CHECK_MANAGE_CONFIG)->EnableWindow(FALSE);
      GetDlgItem(IDC_CHECK_VIEW_EVENTDB)->EnableWindow(FALSE);
      GetDlgItem(IDC_CHECK_EDIT_EVENTDB)->EnableWindow(FALSE);
      GetDlgItem(IDC_CHECK_MANAGE_ACTIONS)->EnableWindow(FALSE);
      GetDlgItem(IDC_CHECK_MANAGE_EPP)->EnableWindow(FALSE);
   }

	return TRUE;
}
