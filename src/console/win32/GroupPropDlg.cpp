// GroupPropDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "GroupPropDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGroupPropDlg dialog


CGroupPropDlg::CGroupPropDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGroupPropDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGroupPropDlg)
	m_bDisabled = FALSE;
	m_bDropConn = FALSE;
	m_bEditConfig = FALSE;
	m_bEditEventDB = FALSE;
	m_bManageUsers = FALSE;
	m_bViewConfig = FALSE;
	m_bViewEventDB = FALSE;
	m_strDescription = _T("");
	m_strName = _T("");
	//}}AFX_DATA_INIT
}


void CGroupPropDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGroupPropDlg)
	DDX_Control(pDX, IDC_LIST_MEMBERS, m_wndListCtrl);
	DDX_Check(pDX, IDC_CHECK_DISABLED, m_bDisabled);
	DDX_Check(pDX, IDC_CHECK_DROP_CONN, m_bDropConn);
	DDX_Check(pDX, IDC_CHECK_EDIT_CONFIG, m_bEditConfig);
	DDX_Check(pDX, IDC_CHECK_EDIT_EVENTDB, m_bEditEventDB);
	DDX_Check(pDX, IDC_CHECK_MANAGE_USERS, m_bManageUsers);
	DDX_Check(pDX, IDC_CHECK_VIEW_CONFIG, m_bViewConfig);
	DDX_Check(pDX, IDC_CHECK_VIEW_EVENTDB, m_bViewEventDB);
	DDX_Text(pDX, IDC_EDIT_DESCRIPTION, m_strDescription);
	DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGroupPropDlg, CDialog)
	//{{AFX_MSG_MAP(CGroupPropDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGroupPropDlg message handlers
