// LoginDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxpc.h"
#include "LoginDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLoginDlg dialog


CLoginDlg::CLoginDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CLoginDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLoginDlg)
	m_strLogin = _T("");
	m_strPassword = _T("");
	m_strServer = _T("");
	m_bClearCache = FALSE;
	//}}AFX_DATA_INIT
}


void CLoginDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLoginDlg)
	DDX_Text(pDX, IDC_EDIT_LOGIN, m_strLogin);
	DDV_MaxChars(pDX, m_strLogin, 63);
	DDX_Text(pDX, IDC_EDIT_PASSWORD, m_strPassword);
	DDV_MaxChars(pDX, m_strPassword, 63);
	DDX_Text(pDX, IDC_EDIT_SERVER, m_strServer);
	DDV_MaxChars(pDX, m_strServer, 63);
	DDX_Check(pDX, IDC_CHECK_CACHE, m_bClearCache);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLoginDlg, CDialog)
	//{{AFX_MSG_MAP(CLoginDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLoginDlg message handlers


//
// WM_INITDIALOG message handler
//

BOOL CLoginDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
   if (m_strLogin.IsEmpty() || m_strServer.IsEmpty())
		return TRUE;

   // Server and login already entered, change focus to password field
   ::SetFocus(::GetDlgItem(m_hWnd, IDC_EDIT_PASSWORD));
   return FALSE;
}

