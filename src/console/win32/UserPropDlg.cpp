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
	m_bEditConfig = FALSE;
	m_bEditEventDB = FALSE;
	m_bManageUsers = FALSE;
	m_bChangePassword = FALSE;
	m_bViewConfig = FALSE;
	m_bViewEventDB = FALSE;
	m_strDescription = _T("");
	m_strLogin = _T("");
	m_strFullName = _T("");
	//}}AFX_DATA_INIT
}


void CUserPropDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CUserPropDlg)
	DDX_Check(pDX, IDC_CHECK_DISABLED, m_bAccountDisabled);
	DDX_Check(pDX, IDC_CHECK_DROP_CONN, m_bDropConn);
	DDX_Check(pDX, IDC_CHECK_EDIT_CONFIG, m_bEditConfig);
	DDX_Check(pDX, IDC_CHECK_EDIT_EVENTDB, m_bEditEventDB);
	DDX_Check(pDX, IDC_CHECK_MANAGE_USERS, m_bManageUsers);
	DDX_Check(pDX, IDC_CHECK_PASSWORD, m_bChangePassword);
	DDX_Check(pDX, IDC_CHECK_VIEW_CONFIG, m_bViewConfig);
	DDX_Check(pDX, IDC_CHECK_VIEW_EVENTDB, m_bViewEventDB);
	DDX_Text(pDX, IDC_EDIT_DESCRIPTION, m_strDescription);
	DDV_MaxChars(pDX, m_strDescription, 255);
	DDX_Text(pDX, IDC_EDIT_LOGIN, m_strLogin);
	DDV_MaxChars(pDX, m_strLogin, 63);
	DDX_Text(pDX, IDC_EDIT_NAME, m_strFullName);
	DDV_MaxChars(pDX, m_strFullName, 127);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CUserPropDlg, CDialog)
	//{{AFX_MSG_MAP(CUserPropDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUserPropDlg message handlers
